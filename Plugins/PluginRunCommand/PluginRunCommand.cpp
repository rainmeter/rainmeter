/*
  Copyright (C) 2013 Brian Ferguson

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "PluginRunCommand.h"

#define MAX_LINE_LENGTH	4096

const WCHAR* err_UnknownCmd = L"Error 100: Unknown command: %s";
const WCHAR* err_CmdRunning = L"Error 101: Program still running: %s";
const WCHAR* err_NotRunning = L"Error 102: Program not running: %s";
const WCHAR* err_Process    = L"Error 103: Cannot start program: %s";
const WCHAR* err_SaveFile   = L"Error 104: Cannot save file: %s";
const WCHAR* err_Terminate  = L"Error 105: Cannot terminate process: %s";	// Rare!
const WCHAR* err_CreatePipe = L"Error 106: Cannot create pipe";				// Rare!


void RunCommand(Measure* measure);
BOOL WINAPI TerminateApp(HANDLE& hProc, DWORD& dwPID, const bool& force);
BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			// Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notification calls.
			DisableThreadLibraryCalls(hinstDLL);
			break;

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;

	measure->skin = RmGetSkin(rm);
	measure->rm = rm;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;

	std::lock_guard<std::recursive_mutex> lock(measure->mutex);

	measure->parameter = RmReadString(rm, L"Parameter", L"");
	measure->finishAction = RmReadString(rm, L"FinishAction", L"", FALSE);
	measure->outputFile = RmReadPath(rm, L"OutputFile", L"");
	measure->folder = RmReadPath(rm, L"StartInFolder", L" ");	// Space is intentional!
	measure->timeout = RmReadInt(rm, L"Timeout", -1);

	const WCHAR* state = RmReadString(rm, L"State", L"HIDE");
	if (_wcsicmp(state, L"SHOW") == 0)
	{
		measure->state = SW_SHOW;
	}
	else if (_wcsicmp(state, L"MAXIMIZE") == 0)
	{
		measure->state = SW_MAXIMIZE;
	}
	else if (_wcsicmp(state, L"MINIMIZE") == 0)
	{
		measure->state = SW_MINIMIZE;
	}
	else
	{
		measure->state = SW_HIDE;
	}

	// Grab "%COMSPEC% environment variable
	measure->program = RmReadString(rm, L"Program", RmReplaceVariables(rm, L"\"%COMSPEC%\" /U /C"));
	if (measure->program.empty())
	{
		// Assume "cmd.exe" exists!
		measure->program = L"cmd.exe /U /C";
	}

	const WCHAR* type = RmReadString(rm, L"OutputType", L"UTF16");
	if (_wcsicmp(type, L"ANSI") == 0)
	{
		measure->outputType = OUTPUTTYPE_ANSI;
	}
	else if (_wcsicmp(type, L"UTF8") == 0)
	{
		measure->outputType = OUTPUTTYPE_UTF8;
	}
	else
	{
		measure->outputType = OUTPUTTYPE_UTF16;
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;

	std::lock_guard<std::recursive_mutex> lock(measure->mutex);

	return measure->value;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;

	std::lock_guard<std::recursive_mutex> lock(measure->mutex);

	return measure->result.c_str();
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;

	std::lock_guard<std::recursive_mutex> lock(measure->mutex);

	if (_wcsicmp(args, L"RUN") == 0)
	{
		if (!measure->threadActive && !measure->program.empty())
		{
			std::thread thread(RunCommand, measure);
			thread.detach();

			measure->value = 0.0f;
			measure->threadActive = true;
		}
		else
		{
			measure->value = 101.0f;
			RmLogF(measure->rm, LOG_NOTICE, err_CmdRunning, measure->program.c_str());	// Command still running
		}
	}
	else if (_wcsicmp(args, L"CLOSE") == 0 || _wcsicmp(args, L"KILL") == 0)
	{
		if (measure->threadActive && measure->hProc != INVALID_HANDLE_VALUE)
		{
			if (!TerminateApp(measure->hProc, measure->dwPID, (_wcsicmp(args, L"KILL") == 0)))
			{
				measure->value = 105.0f;
				RmLogF(measure->rm, LOG_ERROR, err_Terminate, measure->program.c_str());	// Could not terminate process (very rare!)
			}
		}
		else
		{
			measure->value = 102.0f;
			RmLogF(measure->rm, LOG_ERROR, err_NotRunning, measure->program.c_str());	// Command not running
		}
	}
	else
	{
		measure->value = 100.0f;
		RmLogF(measure->rm, LOG_NOTICE, err_UnknownCmd, args);	// Unknown command
	}
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;

	std::unique_lock<std::recursive_mutex> lock(measure->mutex);

	if (measure->threadActive)
	{
		if (!TerminateApp(measure->hProc, measure->dwPID, (measure->state == SW_HIDE)))
		{
			measure->value = 105.0f;
			RmLogF(measure->rm, LOG_ERROR, err_Terminate, measure->program.c_str());	// Could not terminate process (very rare!)
		}

		// Increment ref count of this module so that it will not be
		// unloaded prior to thread completion.
		DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS;
		HMODULE module;
		GetModuleHandleEx(flags, (LPCWSTR)DllMain, &module);

		// Tell the thread to perform any cleanup
		measure->threadActive = false;
		return;
	}

	lock.unlock();
	delete measure;
}

void RunCommand(Measure* measure)
{
	std::unique_lock<std::recursive_mutex> lock(measure->mutex);
	
	std::wstring command = measure->program + L" " + measure->parameter;
	std::wstring folder = measure->folder;
	WORD state = measure->state;
	int timeout = measure->timeout;
	OutputType type = measure->outputType;

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
	HANDLE loadHandles[5];
	for (int i = 0; i < sizeof(loadHandles) / sizeof(loadHandles[0]); ++i)
	{
		loadHandles[i] = INVALID_HANDLE_VALUE;
	}

	SECURITY_ATTRIBUTES sa;
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
		BYTE buffer[MAX_LINE_LENGTH + 3];
		DWORD bytesRead = 0;
		DWORD totalBytes = 0;
		DWORD bytesLeft = 0;
		DWORD exit = 0;

		PROCESS_INFORMATION pi;
		SecureZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

		STARTUPINFO si;
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
				measure->hProc = pi.hProcess;
				measure->dwPID = pi.dwProcessId;
			lock.unlock();

			// Send command
			DWORD written;
			WriteFile(write, &command[0], MAX_LINE_LENGTH, &written, NULL);

			std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

			// Read output of program (if any)
			while(true)
			{
				auto ReadFileAndSetResult = [&]() -> void
				{
					ReadFile(read, buffer, MAX_LINE_LENGTH, &bytesRead, NULL);

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

				// If a timeout is defined, attempt to terminate program and detach it from the plugin
				if ((timeout >= 0 && std::chrono::duration_cast<std::chrono::milliseconds>
					(std::chrono::system_clock::now() - start).count() > timeout))
				{
					if (!TerminateApp(pi.hProcess, pi.dwProcessId, (state == SW_HIDE)))
					{
						lock.lock();
							measure->value = 105.0f;
							RmLogF(measure->rm, LOG_ERROR, err_Terminate, measure->program.c_str());	// Could not terminate process (very rare!)
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
				measure->hProc = INVALID_HANDLE_VALUE;
				measure->dwPID = 0;
			lock.unlock();
		}
		else
		{
			lock.lock();
				measure->value = 103.0f;
				RmLogF(measure->rm, LOG_ERROR, err_Process, measure->program.c_str());	// Cannot start process
				error = true;
			lock.unlock();
		}
	}
	else
	{
		lock.lock();
			measure->value = 106.0f;
			RmLog(measure->rm, LOG_ERROR, err_CreatePipe);	// Cannot create pipe
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

	HMODULE module = nullptr;

	lock.lock();

	if (measure->threadActive)
	{
		measure->result = result;
		measure->result.shrink_to_fit();

		if (!measure->outputFile.empty())
		{
			std::wstring encoding = L"w+";
			switch (type)
			{
			case OUTPUTTYPE_UTF8: { encoding.append(L", ccs=UTF-8"); break; }
			case OUTPUTTYPE_UTF16: { encoding.append(L", ccs=UTF-16LE"); break; }
			}

			FILE* file;
			if (_wfopen_s(&file, measure->outputFile.c_str(), encoding.c_str()) == 0)
			{
				fputws(result.c_str(), file);
			}
			else
			{
				measure->value = 104.0f;
				RmLogF(measure->rm, LOG_ERROR, err_SaveFile, measure->outputFile.c_str());	// Cannot save file
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
			measure->value = 1.0f;
		}

		measure->threadActive = false;

		lock.unlock();

		if (!measure->finishAction.empty())
		{
			RmExecute(measure->skin, measure->finishAction.c_str());
		}

		return;
	}

	lock.unlock();
	delete measure;

	DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
	GetModuleHandleEx(flags, (LPCWSTR)DllMain, &module);

	if (module)
	{
		// Decrement the ref count and possibly unload the module if this is the last instance.
		FreeLibraryAndExitThread(module, 0);
	}
}

BOOL WINAPI TerminateApp(HANDLE& hProc, DWORD& dwPID, const bool& force)
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

	return result;
}

BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam)
{
	DWORD dwID;
	GetWindowThreadProcessId(hwnd, &dwID);

	if (dwID == (DWORD)lParam)
	{
		PostMessage(hwnd, WM_CLOSE, 0, 0);
	}

	return TRUE;
}
