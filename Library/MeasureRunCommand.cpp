/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureRunCommand.h"
#include "AsyncTask.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "../Common/CriticalSection.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"
#include <atomic>
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
	if (handle && handle != INVALID_HANDLE_VALUE)
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

bool TerminateApp(HANDLE processHandle, DWORD processId, bool force);
BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam);
bool TerminateTarget(HANDLE processHandle, HANDLE jobHandle, DWORD processId, bool force);

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

bool TerminateApp(HANDLE processHandle, DWORD processId, bool force)
{
	BOOL result = FALSE;
	HANDLE openedProcess = INVALID_HANDLE_VALUE;

	if (force)
	{
		if ((processHandle == nullptr || processHandle == INVALID_HANDLE_VALUE) && processId != 0)
		{
			openedProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
			processHandle = openedProcess;
		}

		if (processHandle != nullptr && processHandle != INVALID_HANDLE_VALUE)
		{
			result = TerminateProcess(processHandle, 0);
		}
	}
	else
	{
		result = EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)processId);
	}

	if (openedProcess != nullptr && openedProcess != INVALID_HANDLE_VALUE)
	{
		CloseHandle(openedProcess);
	}

	return result != FALSE;
}

bool TerminateTarget(HANDLE processHandle, HANDLE jobHandle, DWORD processId, bool force)
{
	if (force && jobHandle && jobHandle != INVALID_HANDLE_VALUE)
	{
		return TerminateJobObject(jobHandle, 0) != FALSE;
	}

	return TerminateApp(processHandle, processId, force);
}

BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam)
{
	DWORD processId = 0UL;
	GetWindowThreadProcessId(hwnd, &processId);

	if (processId == (DWORD)lParam)
	{
		PostMessage(hwnd, WM_CLOSE, 0, 0);
	}

	return TRUE;
}

}  // namespace

struct MeasureRunCommand::SharedData
{
	explicit SharedData(MeasureRunCommand* measure) : measure(measure) {}

	std::atomic<bool> active = true;
	CriticalSection criticalSection;
	DWORD processId = 0;
	HANDLE m_ProcessHandle = INVALID_HANDLE_VALUE;
	HANDLE m_JobHandle = INVALID_HANDLE_VALUE;
	MeasureRunCommand* measure = nullptr;
};

class MeasureRunCommand::RunCommandTask : public AsyncTask
{
public:
	static RunCommandTask* Create(
		MeasureRunCommand* measure,
		const std::shared_ptr<SharedData>& data,
		std::wstring program,
		std::wstring parameter,
		std::wstring finishAction,
		std::wstring outputFile,
		std::wstring folder,
		WORD state,
		int timeout,
		OutputType outputType)
	{
		auto* task = new RunCommandTask(measure);
		task->m_Data = std::move(data);
		task->m_Program = std::move(program);
		task->m_Command = task->m_Program + L" " + parameter;
		task->m_FinishAction = std::move(finishAction);
		task->m_OutputFile = std::move(outputFile);
		task->m_Folder = std::move(folder);
		task->m_State = state;
		task->m_Timeout = timeout;
		task->m_OutputType = outputType;

		if (!task->Start())
		{
			delete task;
			return nullptr;
		}

		return task;
	}

private:
	RunCommandTask(MeasureRunCommand* measure) : AsyncTask(measure) {}

	void StartWorkOnWorkerThread() override;
	void FinishWorkOnMainThread() override;

	std::shared_ptr<SharedData> m_Data;
	std::wstring m_Program;
	std::wstring m_Command;
	std::wstring m_FinishAction;
	std::wstring m_OutputFile;
	std::wstring m_Folder;
	WORD m_State;
	int m_Timeout;
	OutputType m_OutputType;
	std::wstring m_Result;
	double m_Value = 1.0;
};

void MeasureRunCommand::RunCommandTask::StartWorkOnWorkerThread()
{
	PROCESS_INFORMATION processInfo = {};
	STARTUPINFO startupInfo = {};
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	startupInfo.wShowWindow = m_State;

	auto pipes = CreateProcessPipes(startupInfo);
	if (!pipes)
	{
		m_Value = 106.0;
		LogErrorF(L"%s", err_CreatePipe);
		return;
	}

	if (m_AbortRequested || !m_Data->active)
	{
		ClosePipeHandles(*pipes);
		return;
	}

	const auto createFlags = CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED;
	if (!CreateProcess(nullptr, &m_Command[0], nullptr, nullptr, TRUE, createFlags, nullptr, &m_Folder[0], &startupInfo, &processInfo))
	{
		m_Value = 103.0;
		LogErrorF(err_Process, m_Program.c_str());
		ClosePipeHandles(*pipes);
		return;
	}

	HANDLE jobHandle = CreateJobObject(nullptr, nullptr);
	if (jobHandle && !AssignProcessToJobObject(jobHandle, processInfo.hProcess))
	{
		CloseHandle(jobHandle);
		jobHandle = nullptr;
	}

	{
		CriticalSectionLock lock(m_Data->criticalSection);
		m_Data->processId = processInfo.dwProcessId;
		m_Data->m_ProcessHandle = processInfo.hProcess;
		m_Data->m_JobHandle = jobHandle;
	}

	ResumeThread(processInfo.hThread);

	DWORD written = 0UL;
	WriteFile(pipes->inputWrite, &m_Command[0], MAX_LINE_LENGTH, &written, nullptr);

	const std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
	if (m_AbortRequested || !m_Data->active)
	{
		TerminateTarget(processInfo.hProcess, jobHandle, processInfo.dwProcessId, (m_State == SW_HIDE));
	}
	else
	{
		while (!m_AbortRequested)
		{
			DWORD exitCode = 0UL;

			// Wait briefly before checking output so we do not busy-loop.
			WaitForSingleObject(processInfo.hThread, 1);
			ReadAvailableOutput(pipes->outputRead, m_OutputType, m_Result);

			if (m_Timeout >= 0 &&
				std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() > m_Timeout)
			{
				if (!TerminateTarget(processInfo.hProcess, jobHandle, processInfo.dwProcessId, (m_State == SW_HIDE)))
				{
					m_Value = 105.0;
					LogErrorF(err_Terminate, m_Program.c_str());	// Could not terminate process (very rare!)
				}

				break;
			}

			GetExitCodeProcess(processInfo.hProcess, &exitCode);
			if (exitCode != STILL_ACTIVE)
			{
				break;
			}
		}
	}

	{
		CriticalSectionLock lock(m_Data->criticalSection);
		m_Data->processId = 0;
		m_Data->m_ProcessHandle = INVALID_HANDLE_VALUE;
		m_Data->m_JobHandle = INVALID_HANDLE_VALUE;
	}

	CloseHandle(jobHandle);
	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);

	ClosePipeHandles(*pipes);

	if (m_AbortRequested || !m_Data->active) return;

	// Remove any carriage returns
	m_Result.erase(std::remove(m_Result.begin(), m_Result.end(), L'\r'), m_Result.end());

	if (!m_OutputFile.empty() && !SaveOutputToFile(m_OutputFile, m_Result, m_OutputType))
	{
		m_Value = 104.0;
		LogErrorF(err_SaveFile, m_OutputFile.c_str());
	}
}

void MeasureRunCommand::RunCommandTask::FinishWorkOnMainThread()
{
	if (m_AbortRequested || !m_Data->active) return;

	auto* measure = m_Data->measure;
	if (measure && measure->m_Task == this)
	{
		measure->m_Task = nullptr;
		measure->m_Result = m_Result;
		measure->m_Result.shrink_to_fit();
		measure->m_Value = m_Value;

		if (!m_FinishAction.empty())
		{
			GetRainmeter().ExecuteCommand(m_FinishAction.c_str(), measure->GetSkin());
		}
	}
}

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
	m_Data(std::make_shared<SharedData>(this)),
	m_Task(nullptr)
{
	m_Value = -1.0;
}

MeasureRunCommand::~MeasureRunCommand()
{
	m_Data->active = false;
	m_Data->measure = nullptr;

	if (m_Task)
	{
		CriticalSectionLock lock(m_Data->criticalSection);
		if (m_Data->processId != 0 &&
			!TerminateTarget(m_Data->m_ProcessHandle, m_Data->m_JobHandle, m_Data->processId, (m_State == SW_HIDE)))
		{
			m_Value = 105.0;
			LogErrorF(this, err_Terminate, m_Program.c_str());
		}

		m_Task->AbortWhenPossible();
		m_Task = nullptr;
	}
}

void MeasureRunCommand::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

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
}

const WCHAR* MeasureRunCommand::GetStringValue()
{
	return CheckSubstitute(m_Result.c_str());
}

void MeasureRunCommand::Command(const std::wstring& command)
{
	const WCHAR* args = command.c_str();

	if (_wcsicmp(args, L"RUN") == 0)
	{
		if (!m_Task && !m_Program.empty())
		{
			m_Value = 0.0;
			m_Task = RunCommandTask::Create(this, m_Data, m_Program, m_Parameter, m_FinishAction, m_OutputFile, m_Folder, m_State, m_Timeout, m_OutputType);
			if (!m_Task)
			{
				m_Value = 103.0;
				LogErrorF(this, err_Process, m_Program.c_str());
			}
		}
		else
		{
			m_Value = 101.0;
			LogNoticeF(this, err_CmdRunning, m_Program.c_str());
		}
	}
	else if (_wcsicmp(args, L"CLOSE") == 0 || _wcsicmp(args, L"KILL") == 0)
	{
		CriticalSectionLock lock(m_Data->criticalSection);
		if (m_Task && m_Data->processId != 0)
		{
			const auto kill = _wcsicmp(args, L"KILL") == 0;
			if (!TerminateTarget(m_Data->m_ProcessHandle, m_Data->m_JobHandle, m_Data->processId, kill))
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
		LogNoticeF(this, err_UnknownCmd, args);
	}
}
