// InvokeWMPlayer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

HANDLE FindWmplayerProcess(ULONG& pid) {
	HANDLE hProcess, hPrevProcess = nullptr;
	BYTE buffer[1024];
	for (;;) {
		if (!NT_SUCCESS(NtGetNextProcess(hPrevProcess, PROCESS_QUERY_INFORMATION | SYNCHRONIZE | PROCESS_DUP_HANDLE, 0, 0, &hProcess)))
			break;
		if (NT_SUCCESS(NtQueryInformationProcess(hProcess, ProcessImageFileName, buffer, sizeof(buffer), nullptr))) {
			auto name = (UNICODE_STRING*)buffer;
			name->Buffer[name->Length / sizeof(WCHAR)] = 0;
			if (_wcsicmp(wcsrchr(name->Buffer, L'\\'), L"\\wmplayer.exe") == 0) {
				LARGE_INTEGER interval{};
				if (NtWaitForSingleObject(hProcess, FALSE, &interval) == STATUS_TIMEOUT) {
					pid = GetProcessId(hProcess);
					return hProcess;
				}
			}
		}
		if (hPrevProcess)
			NtClose(hPrevProcess);
		hPrevProcess = hProcess;
	}
	return nullptr;
}

int main() {
	ULONG pid;
	auto hProcess = FindWmplayerProcess(pid);
	if (!hProcess) {
		printf("No live wmplayer detected. launching a new one...\n");
	}
	else {
		printf("Located WMPlayer (%d)\n", pid);
		ULONG handles[1024];
		ULONG len;
		if (NT_SUCCESS(NtQueryInformationProcess(hProcess, ProcessHandleTable, handles, sizeof(handles), &len))) {
			len /= sizeof(ULONG);
			HANDLE hObject;
			BYTE buffer[1024];
			for (ULONG i = 0; i < len; i++) {
				if (NT_SUCCESS(NtDuplicateObject(hProcess, ULongToHandle(handles[i]), NtCurrentProcess(), &hObject, GENERIC_READ, 0, 0))) {
					if (NT_SUCCESS(NtQueryObject(hObject, ObjectNameInformation, buffer, sizeof(buffer), nullptr))) {
						NtClose(hObject);
						auto name = (UNICODE_STRING*)buffer;
						if (name->Length > 0) {
							name->Buffer[name->Length / sizeof(WCHAR)] = 0;
							if (_wcsicmp(wcsrchr(name->Buffer, L'\\'), L"\\Microsoft_WMP_70_CheckForOtherInstanceMutex") == 0) {
								NtDuplicateObject(hProcess, ULongToHandle(handles[i]), nullptr, nullptr, 0, 0, DUPLICATE_CLOSE_SOURCE);
								break;
							}
						}
					}
				}
			}
		}
	}
	ShellExecute(nullptr, L"open", L"wmplayer", nullptr, nullptr, SW_SHOWDEFAULT);
	return 0;
}

