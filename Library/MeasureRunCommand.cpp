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

const WCHAR* err_UnknownCmd = L"Error 100: Unknown command: %s";
const WCHAR* err_CmdRunning = L"Error 101: Program still running: %s";
const WCHAR* err_NotRunning = L"Error 102: Program not running: %s";
const WCHAR* err_Process    = L"Error 103: Cannot start program: %s";
const WCHAR* err_SaveFile   = L"Error 104: Cannot save file: %s";
const WCHAR* err_Terminate  = L"Error 105: Cannot terminate process: %s";	// Rare!
const WCHAR* err_CreatePipe = L"Error 106: Cannot create pipe";				// Rare!

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
	if (m_Folder != L" ")
	{
		m_Skin->MakePathAbsolute(m_Folder);
	}
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
	OutputType type = m_OutputType;

	lock.unlock();

	std::wstring result = L"";
	bool error = false;

	HANDLE read = INVALID_HANDLE_VALUE;
	HANDLE write = INVALID_HANDLE_VALUE;

/*	Instead of trying to keep track of the following handles,
	use an array to make cleanup easier.

	HANDLE hOutputReadTmp;	0
	HANDLE hOutputWrite;	1
	HANDLE hInputWriteTmp;	2
	HANDLE hInputRead;		3
	HANDLE hErrorWrite;		4
*/
	HANDLE loadHandles[5] = { 0 };
	for (int i = 0; i < sizeof(loadHandles) / sizeof(loadHandles[0]); ++i)
	{
		loadHandles[i] = INVALID_HANDLE_VALUE;
	}

	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	HANDLE hProc = GetCurrentProcess();

	// Create pipe for stdin, stdout, stderr
	if (CreatePipe(&loadHandles[0], &loadHandles[1], &sa, 0) &&
		DuplicateHandle(hProc, loadHandles[1], hProc, &loadHandles[4], 0, TRUE, DUPLICATE_SAME_ACCESS) &&
		CreatePipe(&loadHandles[3], &loadHandles[2], &sa, 0) &&
		DuplicateHandle(hProc, loadHandles[0], hProc, &read, 0, FALSE, DUPLICATE_SAME_ACCESS) &&
		DuplicateHandle(hProc, loadHandles[2], hProc, &write, 0, FALSE, DUPLICATE_SAME_ACCESS))
	{
		BYTE buffer[MAX_LINE_LENGTH + 3] = { 0 };
		DWORD bytesRead = 0UL;
		DWORD totalBytes = 0UL;
		DWORD bytesLeft = 0UL;
		DWORD exit = 0UL;

		PROCESS_INFORMATION pi = { 0 };
		SecureZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

		STARTUPINFO si = { 0 };
		SecureZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.wShowWindow = state;
		si.hStdOutput  = loadHandles[1];
		si.hStdInput   = loadHandles[3];
		si.hStdError   = loadHandles[4];

		if (CreateProcess(NULL, &command[0], NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, &folder[0], &si, &pi))
		{
			// Store values inside measure for the "Close" or "Kill" command
			lock.lock();
				m_HProc = pi.hProcess;
				m_DwPID = pi.dwProcessId;
			lock.unlock();

			// Send command
			DWORD written = 0UL;
			WriteFile(write, &command[0], MAX_LINE_LENGTH, &written, nullptr);

			std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

			// Read output of program (if any)
			while (true)
			{
				auto ReadFileAndSetResult = [&]() -> void
				{
					if (ReadFile(read, buffer, MAX_LINE_LENGTH, &bytesRead, nullptr) != FALSE)
					{
						// Triple "null" the buffer in case an odd number bytes is
						// converted to a multi-byte string.
						buffer[bytesRead] = '\0';
						buffer[bytesRead + 1] = '\0';
						buffer[bytesRead + 2] = '\0';

						switch (type)
						{
						case OUTPUTTYPE_ANSI:
							result += StringUtil::Widen((LPCSTR)buffer);
							break;

						case OUTPUTTYPE_UTF8:
							result += StringUtil::WidenUTF8((LPCSTR)buffer);
							break;

						default:
						case OUTPUTTYPE_UTF16:
							result += (LPCWSTR)buffer;
							break;
						}
					}

					SecureZeroMemory(buffer, sizeof(buffer));	// clear the buffer
				};

				// Wait for a signal from the process (or timeout).
				// This reduced CPU usage significantly.
				WaitForSingleObject(pi.hThread, 1);

				// Check if there is any data to to read
				PeekNamedPipe(read, buffer, MAX_LINE_LENGTH, &bytesRead, &totalBytes, &bytesLeft);
				if (bytesRead != 0)
				{
					if (totalBytes > MAX_LINE_LENGTH)
					{
						while (bytesRead >= MAX_LINE_LENGTH)
						{
							ReadFileAndSetResult();
						}
					}
					else
					{
						ReadFileAndSetResult();
					}
				}

				// If a timeout is defined, attempt to terminate program and detach it from the measure
				if ((timeout >= 0 && std::chrono::duration_cast<std::chrono::milliseconds>
					(std::chrono::system_clock::now() - start).count() > timeout))
				{
					if (!TerminateApp(pi.hProcess, pi.dwProcessId, (state == SW_HIDE)))
					{
						lock.lock();
							m_Value = 105.0;
							LogErrorF(this, err_Terminate, m_Program.c_str());	// Could not terminate process (very rare!)
							error = true;
						lock.unlock();
					}
					break;
				}

				// Check to see if the program is still running
				GetExitCodeProcess(pi.hProcess, &exit);
				if (exit != STILL_ACTIVE) break;
			}

			// Close process handles
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);

			// Update values in case the "Close" or "Kill" command is called
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

	// Close handles
	for (int i = 0; i < sizeof(loadHandles) / sizeof(loadHandles[0]); ++i)
	{
		if (loadHandles[i] != INVALID_HANDLE_VALUE)
		{
			CloseHandle(loadHandles[i]);
		}
	}

	CloseHandle(write);
	CloseHandle(read);

	// Remove any carriage returns
	result.erase(std::remove(result.begin(), result.end(), L'\r'), result.end());

	lock.lock();

	if (m_ThreadActive)
	{
		m_Result = result;
		m_Result.shrink_to_fit();

		if (!m_OutputFile.empty())
		{
			std::wstring encoding = L"w+";
			switch (type)
			{
			case OUTPUTTYPE_UTF8: { encoding.append(L", ccs=UTF-8"); break; }
			case OUTPUTTYPE_UTF16: { encoding.append(L", ccs=UTF-16LE"); break; }
			}

			FILE* file = nullptr;
			if ((_wfopen_s(&file, m_OutputFile.c_str(), encoding.c_str()) == 0) && file)
			{
				fputws(result.c_str(), file);
			}
			else
			{
				m_Value = 104.0;
				LogErrorF(this, err_SaveFile, m_OutputFile.c_str());	// Cannot save file
				error = true;
			}

			if (file)
			{
				fclose(file);
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
