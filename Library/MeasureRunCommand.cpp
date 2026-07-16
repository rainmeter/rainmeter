/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureRunCommand.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"
#include <chrono>
#include <optional>

const WCHAR* err_UnknownCmd = L"Error 100: Unknown command: %s";
const WCHAR* err_CmdRunning = L"Error 101: Program still running: %s";
const WCHAR* err_NotRunning = L"Error 102: Program not running: %s";
const WCHAR* err_Process    = L"Error 103: Cannot start program: %s";
const WCHAR* err_SaveFile   = L"Error 104: Cannot save file: %s";
const WCHAR* err_Terminate  = L"Error 105: Cannot terminate process: %s";	// Rare!
const WCHAR* err_CreatePipe = L"Error 106: Cannot create pipe";				// Rare!

namespace {

struct PipeHandles
{
	HANDLE outputReadTmp = INVALID_HANDLE_VALUE;
	HANDLE inputWriteTmp = INVALID_HANDLE_VALUE;
	HANDLE errorWrite = INVALID_HANDLE_VALUE;
	HANDLE outputRead = INVALID_HANDLE_VALUE;
	HANDLE inputWrite = INVALID_HANDLE_VALUE;
};

void CloseHandleIfValid(HANDLE& handle)
{
	if (handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(handle);
		handle = INVALID_HANDLE_VALUE;
	}
}

void ClosePipeHandles(PipeHandles& pipes)
{
	CloseHandleIfValid(pipes.outputReadTmp);
	CloseHandleIfValid(pipes.inputWriteTmp);
	CloseHandleIfValid(pipes.errorWrite);
	CloseHandleIfValid(pipes.inputWrite);
	CloseHandleIfValid(pipes.outputRead);
}

std::optional<PipeHandles> CreateProcessPipes(STARTUPINFO& startupInfo)
{
	PipeHandles pipes;
	SECURITY_ATTRIBUTES security = {};
	security.nLength = sizeof(security);
	security.bInheritHandle = TRUE;

	const HANDLE currentProcess = GetCurrentProcess();

	if (
		CreatePipe(&pipes.outputReadTmp, &startupInfo.hStdOutput, &security, 0) &&
		DuplicateHandle(currentProcess, startupInfo.hStdOutput, currentProcess, &pipes.errorWrite, 0, TRUE, DUPLICATE_SAME_ACCESS) &&
		CreatePipe(&startupInfo.hStdInput, &pipes.inputWriteTmp, &security, 0) &&
		DuplicateHandle(currentProcess, pipes.outputReadTmp, currentProcess, &pipes.outputRead, 0, FALSE, DUPLICATE_SAME_ACCESS) &&
		DuplicateHandle(currentProcess, pipes.inputWriteTmp, currentProcess, &pipes.inputWrite, 0, FALSE, DUPLICATE_SAME_ACCESS))
	{
		startupInfo.hStdError = pipes.errorWrite;
		return pipes;
	}

	ClosePipeHandles(pipes);
	return std::nullopt;
}

void ReadAvailableOutput(HANDLE outputRead, OutputType type, std::wstring& result)
{
	auto appendOutputChunk = [&](const BYTE* buffer, DWORD bytesRead)
	{
		BYTE chunk[MAX_LINE_LENGTH + 3] = { 0 };
		memcpy(chunk, buffer, bytesRead);

		// Triple "null" the buffer in case an odd number of bytes is converted
		// to a multi-byte string.
		chunk[bytesRead] = '\0';
		chunk[bytesRead + 1] = '\0';
		chunk[bytesRead + 2] = '\0';

		switch (type)
		{
		case OUTPUTTYPE_ANSI:
			result += StringUtil::Widen(reinterpret_cast<LPCSTR>(chunk));
			break;

		case OUTPUTTYPE_UTF8:
			result += StringUtil::WidenUTF8(reinterpret_cast<LPCSTR>(chunk));
			break;

		case OUTPUTTYPE_UTF16:
		default:
			result += reinterpret_cast<LPCWSTR>(chunk);
			break;
		}
	};

	BYTE peekBuffer[MAX_LINE_LENGTH + 3] = { 0 };
	DWORD bytesAvailableNow = 0UL;
	DWORD totalBytesAvailable = 0UL;
	DWORD bytesLeftThisMessage = 0UL;

	if (!PeekNamedPipe(outputRead, peekBuffer, MAX_LINE_LENGTH, &bytesAvailableNow, &totalBytesAvailable, &bytesLeftThisMessage) ||
		bytesAvailableNow == 0)
	{
		return;
	}

	do
	{
		BYTE readBuffer[MAX_LINE_LENGTH + 3] = { 0 };
		DWORD bytesRead = 0UL;
		if (ReadFile(outputRead, readBuffer, MAX_LINE_LENGTH, &bytesRead, nullptr) == FALSE || bytesRead == 0)
		{
			break;
		}

		appendOutputChunk(readBuffer, bytesRead);
		totalBytesAvailable = (totalBytesAvailable > bytesRead) ? (totalBytesAvailable - bytesRead) : 0;
	}
	while (totalBytesAvailable > 0);
}

bool SaveOutputToFile(const std::wstring& path, const std::wstring& output, OutputType type)
{
	std::wstring encoding = L"w+";
	switch (type)
	{
	case OUTPUTTYPE_UTF8:
		encoding.append(L", ccs=UTF-8");
		break;

	case OUTPUTTYPE_UTF16:
		encoding.append(L", ccs=UTF-16LE");
		break;

	case OUTPUTTYPE_ANSI:
	default:
		break;
	}

	FILE* file = nullptr;
	const bool opened = (_wfopen_s(&file, path.c_str(), encoding.c_str()) == 0) && file;
	if (opened)
	{
		fputws(output.c_str(), file);
		fclose(file);
	}

	return opened;
}

}  // namespace

MeasureRunCommand::MeasureRunCommand(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Program(),
	m_Parameter(),
	m_FinishAction(),
	m_OutputFile(),
	m_Folder(),
	m_State(0),
	m_Timeout(-1),
	m_OutputType(OUTPUTTYPE_UTF16),
	m_Result(),
	m_Mutex(),
	m_ThreadActive(false),
	m_Thread(),
	m_HProc(INVALID_HANDLE_VALUE),
	m_DwPID(0)
{
	m_Value = -1.0;
}

MeasureRunCommand::~MeasureRunCommand()
{
	std::unique_lock<std::recursive_mutex> lock(m_Mutex);

	if (m_ThreadActive)
	{
		if (m_HProc != INVALID_HANDLE_VALUE &&
			!TerminateApp(m_HProc, m_DwPID, (m_State == SW_HIDE)))
		{
			m_Value = 105.0;
			LogErrorF(this, err_Terminate, m_Program.c_str());	// Could not terminate process (very rare!)
		}

		// Tell the thread to perform any cleanup.
		m_ThreadActive = false;
	}

	lock.unlock();

	if (m_Thread.joinable())
	{
		m_Thread.join();
	}
}

void MeasureRunCommand::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	m_Parameter = parser.ReadString(section, L"Parameter", L"");
	m_FinishAction = parser.ReadString(section, L"FinishAction", L"", false);
	m_OutputFile = parser.ReadString(section, L"OutputFile", L"");
	m_Skin->MakePathAbsolute(m_OutputFile);

	m_Folder = parser.ReadString(section, L"StartInFolder", L" ");	// Space is intentional!
	m_Skin->MakePathAbsolute(m_Folder);

	m_Timeout = parser.ReadInt(section, L"Timeout", -1);

	const WCHAR* state = parser.ReadString(section, L"State", L"HIDE").c_str();
	if (_wcsicmp(state, L"SHOW") == 0)
	{
		m_State = SW_SHOW;
	}
	else if (_wcsicmp(state, L"MAXIMIZE") == 0)
	{
		m_State = SW_MAXIMIZE;
	}
	else if (_wcsicmp(state, L"MINIMIZE") == 0)
	{
		m_State = SW_MINIMIZE;
	}
	else
	{
		m_State = SW_HIDE;
	}

	// Grab "%COMSPEC% environment variable
	m_Program = parser.ReadString(section, L"Program", L"\"%COMSPEC%\" /U /C");
	PathUtil::ExpandEnvironmentVariables(m_Program);
	if (m_Program.empty())
	{
		// Assume "cmd.exe" exists!
		m_Program = L"cmd.exe /U /C";
	}

	const WCHAR* type = parser.ReadString(section, L"OutputType", L"UTF16").c_str();
	if (_wcsicmp(type, L"ANSI") == 0)
	{
		m_OutputType = OUTPUTTYPE_ANSI;
	}
	else if (_wcsicmp(type, L"UTF8") == 0)
	{
		m_OutputType = OUTPUTTYPE_UTF8;
	}
	else
	{
		m_OutputType = OUTPUTTYPE_UTF16;
	}
}

void MeasureRunCommand::UpdateValue()
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
}

const WCHAR* MeasureRunCommand::GetStringValue()
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	return CheckSubstitute(m_Result.c_str());
}

void MeasureRunCommand::Command(const std::wstring& command)
{
	const WCHAR* args = command.c_str();

	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	if (_wcsicmp(args, L"RUN") == 0)
	{
		if (!m_ThreadActive && !m_Program.empty())
		{
			if (m_Thread.joinable())
			{
				if (m_Thread.get_id() == std::this_thread::get_id())
				{
					m_Thread.detach();
				}
				else
				{
					m_Thread.join();
				}
			}

			m_Thread = std::thread(&MeasureRunCommand::RunCommand, this);

			m_Value = 0.0;
			m_ThreadActive = true;
		}
		else
		{
			m_Value = 101.0;
			LogNoticeF(this, err_CmdRunning, m_Program.c_str());	// Command still running
		}
	}
	else if (_wcsicmp(args, L"CLOSE") == 0 || _wcsicmp(args, L"KILL") == 0)
	{
		if (m_ThreadActive && m_HProc != INVALID_HANDLE_VALUE)
		{
			if (!TerminateApp(m_HProc, m_DwPID, (_wcsicmp(args, L"KILL") == 0)))
			{
				m_Value = 105.0;
				LogErrorF(this, err_Terminate, m_Program.c_str());	// Could not terminate process (very rare!)
			}
		}
		else
		{
			m_Value = 102.0;
			LogErrorF(this, err_NotRunning, m_Program.c_str());	// Command not running
		}
	}
	else
	{
		m_Value = 100.0;
		LogNoticeF(this, err_UnknownCmd, args);	// Unknown command
	}
}

void MeasureRunCommand::RunCommand()
{
	std::unique_lock<std::recursive_mutex> lock(m_Mutex);

	std::wstring command = m_Program + L" " + m_Parameter;
	std::wstring folder = m_Folder;
	WORD state = m_State;
	int timeout = m_Timeout;
	OutputType outputType = m_OutputType;

	lock.unlock();

	std::wstring result;
	bool error = false;

	PROCESS_INFORMATION processInfo = {};
	STARTUPINFO startupInfo = {};
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	startupInfo.wShowWindow = state;

	auto pipes = CreateProcessPipes(startupInfo);
	if (pipes)
	{
		if (CreateProcess(nullptr, &command[0], nullptr, nullptr, TRUE, CREATE_NEW_PROCESS_GROUP, nullptr, &folder[0], &startupInfo, &processInfo))
		{
			lock.lock();
			m_HProc = processInfo.hProcess;
			m_DwPID = processInfo.dwProcessId;
			lock.unlock();

			DWORD written = 0UL;
			WriteFile(pipes->inputWrite, &command[0], MAX_LINE_LENGTH, &written, nullptr);

			const std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			while (true)
			{
				DWORD exitCode = 0UL;

				// Wait briefly before checking output so we do not busy-loop.
				WaitForSingleObject(processInfo.hThread, 1);
				ReadAvailableOutput(pipes->outputRead, outputType, result);

				if (timeout >= 0 &&
					std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() > timeout)
				{
					if (!TerminateApp(processInfo.hProcess, processInfo.dwProcessId, (state == SW_HIDE)))
					{
						lock.lock();
						m_Value = 105.0;
						LogErrorF(this, err_Terminate, m_Program.c_str());	// Could not terminate process (very rare!)
						error = true;
						lock.unlock();
					}

					break;
				}

				GetExitCodeProcess(processInfo.hProcess, &exitCode);
				if (exitCode != STILL_ACTIVE)
				{
					break;
				}
			}

			CloseHandle(processInfo.hThread);
			CloseHandle(processInfo.hProcess);

			lock.lock();
			m_HProc = INVALID_HANDLE_VALUE;
			m_DwPID = 0;
			lock.unlock();
		}
		else
		{
			lock.lock();
			m_Value = 103.0;
			LogErrorF(this, err_Process, m_Program.c_str());	// Cannot start process
			error = true;
			lock.unlock();
		}
	}
	else
	{
		lock.lock();
		m_Value = 106.0;
		LogErrorF(this, L"%s", err_CreatePipe);	// Cannot create pipe
		error = true;
		lock.unlock();
	}

	if (pipes)
	{
		ClosePipeHandles(*pipes);
	}

	// Remove any carriage returns
	result.erase(std::remove(result.begin(), result.end(), L'\r'), result.end());

	lock.lock();

	if (m_ThreadActive)
	{
		m_Result = result;
		m_Result.shrink_to_fit();

		if (!m_OutputFile.empty())
		{
			if (!SaveOutputToFile(m_OutputFile, result, outputType))
			{
				m_Value = 104.0;
				LogErrorF(this, err_SaveFile, m_OutputFile.c_str());	// Cannot save file
				error = true;
			}
		}

		// If no errors from the thread,
		// set number value of measure to 1 to indicate "Success".
		if (!error)
		{
			m_Value = 1.0;
		}

		m_ThreadActive = false;

		lock.unlock();

		if (!m_FinishAction.empty())
		{
			GetRainmeter().ExecuteCommand(m_FinishAction.c_str(), m_Skin);
		}

		return;
	}
}

bool MeasureRunCommand::TerminateApp(HANDLE& hProc, DWORD& dwPID, const bool& force)
{
	BOOL result = FALSE;

	if (force)
	{
		result = TerminateProcess(hProc, 0);
	}
	else
	{
		result = EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM) dwPID);
	}

	return result != FALSE;
}

BOOL CALLBACK MeasureRunCommand::TerminateAppEnum(HWND hwnd, LPARAM lParam)
{
	DWORD dwID = 0UL;
	GetWindowThreadProcessId(hwnd, &dwID);

	if (dwID == (DWORD)lParam)
	{
		PostMessage(hwnd, WM_CLOSE, 0, 0);
	}

	return TRUE;
}
