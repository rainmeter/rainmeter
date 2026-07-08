/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureRunCommand.h"
#include "../Common/PathUtil.h"
#include "AsyncTask.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "Skin.h"

namespace {

const WCHAR* err_UnknownCmd = L"Error 100: Unknown command: %s";
const WCHAR* err_CmdRunning = L"Error 101: Program still running: %s";
const WCHAR* err_NotRunning = L"Error 102: Program not running: %s";
const WCHAR* err_Process = L"Error 103: Cannot start program: %s";
const WCHAR* err_SaveFile = L"Error 104: Cannot save file: %s";
const WCHAR* err_Terminate = L"Error 105: Cannot terminate process: %s";
const WCHAR* err_CreatePipe = L"Error 106: Cannot create pipe";

}  // namespace

class MeasureRunCommand::RunCommandTask : public AsyncTask
{
public:
	static RunCommandTask* Create(MeasureRunCommand* measure)
	{
		auto* task = new RunCommandTask(measure);
		if (!task->Start())
		{
			delete task;
			return nullptr;
		}

		return task;
	}

private:
	RunCommandTask(MeasureRunCommand* measure) :
		AsyncTask(measure),
		m_CriticalSection(measure->m_CriticalSection),
		m_Program(measure->m_Program),
		m_Parameter(measure->m_Parameter),
		m_FinishAction(measure->m_FinishAction),
		m_OutputFile(measure->m_OutputFile),
		m_Folder(measure->m_Folder),
		m_Result(),
		m_State(measure->m_State),
		m_Timeout(measure->m_Timeout),
		m_OutputType(measure->m_OutputType),
		m_Value(1.0),
		m_ErrorMessage(nullptr)
	{
	}

	void StartWorkOnWorkerThread() override;
	void FinishWorkOnMainThread() override;
	void SetError(double value, const WCHAR* message);

	std::shared_ptr<CriticalSection> m_CriticalSection;
	std::wstring m_Program;
	std::wstring m_Parameter;
	std::wstring m_FinishAction;
	std::wstring m_OutputFile;
	std::wstring m_Folder;
	std::wstring m_Result;
	WORD m_State;
	int m_Timeout;
	OutputType m_OutputType;
	double m_Value;
	const WCHAR* m_ErrorMessage;
};

void MeasureRunCommand::RunCommandTask::StartWorkOnWorkerThread()
{
	std::wstring command = m_Program + L" " + m_Parameter;
	bool error = false;

	HANDLE read = INVALID_HANDLE_VALUE;
	HANDLE write = INVALID_HANDLE_VALUE;
	HANDLE loadHandles[5] = { 0 };
	for (size_t i = 0; i < _countof(loadHandles); ++i)
	{
		loadHandles[i] = INVALID_HANDLE_VALUE;
	}

	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = nullptr;

	HANDLE process = GetCurrentProcess();

	if (CreatePipe(&loadHandles[0], &loadHandles[1], &sa, 0) &&
		DuplicateHandle(process, loadHandles[1], process, &loadHandles[4], 0, TRUE,
			DUPLICATE_SAME_ACCESS) &&
		CreatePipe(&loadHandles[3], &loadHandles[2], &sa, 0) &&
		DuplicateHandle(process, loadHandles[0], process, &read, 0, FALSE,
			DUPLICATE_SAME_ACCESS) &&
		DuplicateHandle(process, loadHandles[2], process, &write, 0, FALSE,
			DUPLICATE_SAME_ACCESS))
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
		si.wShowWindow = m_State;
		si.hStdOutput = loadHandles[1];
		si.hStdInput = loadHandles[3];
		si.hStdError = loadHandles[4];

		if (CreateProcess(nullptr, &command[0], nullptr, nullptr, TRUE, CREATE_NEW_PROCESS_GROUP,
			nullptr, &m_Folder[0], &si, &pi))
		{
			{
				CriticalSectionLock lock(*m_CriticalSection);
				if (!m_AbortRequested)
				{
					auto measure = (MeasureRunCommand*)m_Requestor;
					measure->m_Process = pi.hProcess;
					measure->m_ProcessId = pi.dwProcessId;
				}
			}

			if (m_AbortRequested)
			{
				TerminateApp(pi.hProcess, pi.dwProcessId, (m_State == SW_HIDE));
			}
			else
			{
				DWORD written = 0UL;
				WriteFile(write, &command[0], MAX_LINE_LENGTH, &written, nullptr);

				std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

				while (!m_AbortRequested)
				{
					auto ReadFileAndSetResult = [&]() -> void
					{
						if (ReadFile(read, buffer, MAX_LINE_LENGTH, &bytesRead, nullptr) != FALSE)
						{
							buffer[bytesRead] = '\0';
							buffer[bytesRead + 1] = '\0';
							buffer[bytesRead + 2] = '\0';

							switch (m_OutputType)
							{
							case OUTPUTTYPE_ANSI:
								m_Result += StringUtil::Widen((LPCSTR)buffer);
								break;

							case OUTPUTTYPE_UTF8:
								m_Result += StringUtil::WidenUTF8((LPCSTR)buffer);
								break;

							default:
							case OUTPUTTYPE_UTF16:
								m_Result += (LPCWSTR)buffer;
								break;
							}
						}

						SecureZeroMemory(buffer, sizeof(buffer));
					};

					WaitForSingleObject(pi.hThread, 1);

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

					if (m_Timeout >= 0 && std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::system_clock::now() - start).count() > m_Timeout)
					{
						if (!TerminateApp(pi.hProcess, pi.dwProcessId, (m_State == SW_HIDE)))
						{
							SetError(105.0, err_Terminate);
							error = true;
						}
						break;
					}

					GetExitCodeProcess(pi.hProcess, &exit);
					if (exit != STILL_ACTIVE) break;
				}
			}

			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);

			{
				CriticalSectionLock lock(*m_CriticalSection);
				if (!m_AbortRequested)
				{
					auto measure = (MeasureRunCommand*)m_Requestor;
					measure->m_Process = INVALID_HANDLE_VALUE;
					measure->m_ProcessId = 0;
				}
			}
		}
		else
		{
			SetError(103.0, err_Process);
			error = true;
		}
	}
	else
	{
		SetError(106.0, err_CreatePipe);
		error = true;
	}

	for (size_t i = 0; i < _countof(loadHandles); ++i)
	{
		if (loadHandles[i] != INVALID_HANDLE_VALUE)
		{
			CloseHandle(loadHandles[i]);
		}
	}

	if (write != INVALID_HANDLE_VALUE)
	{
		CloseHandle(write);
	}

	if (read != INVALID_HANDLE_VALUE)
	{
		CloseHandle(read);
	}

	m_Result.erase(std::remove(m_Result.begin(), m_Result.end(), L'\r'), m_Result.end());

	if (!m_AbortRequested && !m_OutputFile.empty())
	{
		std::wstring encoding = L"w+";
		switch (m_OutputType)
		{
		case OUTPUTTYPE_UTF8:
			encoding.append(L", ccs=UTF-8");
			break;

		case OUTPUTTYPE_UTF16:
			encoding.append(L", ccs=UTF-16LE");
			break;
		}

		FILE* file = nullptr;
		if ((_wfopen_s(&file, m_OutputFile.c_str(), encoding.c_str()) == 0) && file)
		{
			fputws(m_Result.c_str(), file);
		}
		else
		{
			SetError(104.0, err_SaveFile);
			error = true;
		}

		if (file)
		{
			fclose(file);
		}
	}

	if (!error)
	{
		m_Value = 1.0;
	}
}

void MeasureRunCommand::RunCommandTask::FinishWorkOnMainThread()
{
	if (m_AbortRequested)
	{
		return;
	}

	auto measure = (MeasureRunCommand*)m_Requestor;
	if (measure->m_Task == this)
	{
		measure->m_Task = nullptr;
		measure->m_Value = m_Value;
		measure->m_Result = m_Result;
		measure->m_Result.shrink_to_fit();

		if (m_ErrorMessage)
		{
			if (m_ErrorMessage == err_CreatePipe)
			{
				LogErrorF(measure, m_ErrorMessage);
			}
			else if (m_ErrorMessage == err_SaveFile)
			{
				LogErrorF(measure, m_ErrorMessage, m_OutputFile.c_str());
			}
			else
			{
				LogErrorF(measure, m_ErrorMessage, m_Program.c_str());
			}
		}

		if (!m_FinishAction.empty())
		{
			GetRainmeter().ExecuteCommand(m_FinishAction.c_str(), measure->GetSkin());
		}
	}
}

void MeasureRunCommand::RunCommandTask::SetError(double value, const WCHAR* message)
{
	m_Value = value;
	m_ErrorMessage = message;
}

MeasureRunCommand::MeasureRunCommand(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Program(),
	m_Parameter(),
	m_FinishAction(),
	m_OutputFile(),
	m_Folder(),
	m_Result(),
	m_State(SW_HIDE),
	m_Timeout(-1),
	m_OutputType(OUTPUTTYPE_UTF16),
	m_CriticalSection(std::make_shared<CriticalSection>()),
	m_Task(nullptr),
	m_Process(INVALID_HANDLE_VALUE),
	m_ProcessId(0)
{
	m_Value = -1.0;
}

MeasureRunCommand::~MeasureRunCommand()
{
	CriticalSectionLock lock(*m_CriticalSection);

	if (m_Task)
	{
		m_Task->AbortWhenPossible();
		m_Task = nullptr;

		if (m_Process != INVALID_HANDLE_VALUE &&
			!TerminateApp(m_Process, m_ProcessId, (m_State == SW_HIDE)))
		{
			m_Value = 105.0;
			LogErrorF(this, err_Terminate, m_Program.c_str());
		}
	}
}

void MeasureRunCommand::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	CriticalSectionLock lock(*m_CriticalSection);

	m_Parameter = parser.ReadString(section, L"Parameter", L"");
	m_FinishAction = parser.ReadString(section, L"FinishAction", L"", false);
	m_OutputFile = parser.ReadString(section, L"OutputFile", L"");
	GetSkin()->MakePathAbsolute(m_OutputFile);
	m_Folder = parser.ReadString(section, L"StartInFolder", L" ");
	GetSkin()->MakePathAbsolute(m_Folder);
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

	m_Program = parser.ReadString(section, L"Program", L"\"%COMSPEC%\" /U /C");
	PathUtil::ExpandEnvironmentVariables(m_Program);
	if (m_Program.empty())
	{
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

const WCHAR* MeasureRunCommand::GetStringValue()
{
	CriticalSectionLock lock(*m_CriticalSection);

	return m_Result.c_str();
}

void MeasureRunCommand::Command(const std::wstring& command)
{
	CriticalSectionLock lock(*m_CriticalSection);

	if (_wcsicmp(command.c_str(), L"RUN") == 0)
	{
		if (!m_Task && !m_Program.empty())
		{
			m_Task = RunCommandTask::Create(this);
			if (m_Task)
			{
				m_Value = 0.0;
			}
		}
		else
		{
			m_Value = 101.0;
			LogNoticeF(this, err_CmdRunning, m_Program.c_str());
		}
	}
	else if (_wcsicmp(command.c_str(), L"CLOSE") == 0 || _wcsicmp(command.c_str(), L"KILL") == 0)
	{
		if (m_Task && m_Process != INVALID_HANDLE_VALUE)
		{
			if (!TerminateApp(m_Process, m_ProcessId, (_wcsicmp(command.c_str(), L"KILL") == 0)))
			{
				m_Value = 105.0;
				LogErrorF(this, err_Terminate, m_Program.c_str());
			}
		}
		else
		{
			m_Value = 102.0;
			LogErrorF(this, err_NotRunning, m_Program.c_str());
		}
	}
	else
	{
		m_Value = 100.0;
		LogNoticeF(this, err_UnknownCmd, command.c_str());
	}
}

bool MeasureRunCommand::TerminateApp(HANDLE& process, DWORD& processId, bool force)
{
	BOOL result = FALSE;

	if (force)
	{
		result = TerminateProcess(process, 0);
	}
	else
	{
		result = EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)processId);
	}

	return result != FALSE;
}

BOOL CALLBACK MeasureRunCommand::TerminateAppEnum(HWND hwnd, LPARAM lParam)
{
	DWORD processId = 0UL;
	GetWindowThreadProcessId(hwnd, &processId);

	if (processId == (DWORD)lParam)
	{
		PostMessage(hwnd, WM_CLOSE, 0, 0);
	}

	return TRUE;
}
