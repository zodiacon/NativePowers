// EnvVars.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: EnvVars <pid>\n");
		return 0;
	}

	auto pid = strtol(argv[1], nullptr, 0);

	OBJECT_ATTRIBUTES procAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
	CLIENT_ID cid{ ULongToHandle(pid) };
	HANDLE hProcess;
	auto status = NtOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, &procAttr, &cid);
	if (!NT_SUCCESS(status)) {
		printf("Error opening process (0x%X)\n", status);
		return status;
	}
	PROCESS_BASIC_INFORMATION pbi;
	status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), nullptr);
	if (!NT_SUCCESS(status)) {
		printf("Error querying information (0x%X)\n", status);
		return status;
	}

	if (pbi.PebBaseAddress == nullptr) {
		printf("Process has no PEB!\n");
		return 1;
	}
	PEB peb;
	NtReadVirtualMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(PEB), nullptr);

	RTL_USER_PROCESS_PARAMETERS params;
	NtReadVirtualMemory(hProcess, peb.ProcessParameters, &params, sizeof(params), nullptr);
	if (params.Environment == nullptr) {
		printf("Process has no environment variables!\n");
		return 1;
	}

	auto buffer = std::make_unique<BYTE[]>(1 << 16);
	NtReadVirtualMemory(hProcess, params.Environment, buffer.get(), 1 << 16, nullptr);

	auto chars = (PWSTR)buffer.get();
	while (*chars) {
		auto equal = wcschr(chars, L'=');
		auto next = chars + wcslen(chars) + 1;
		*equal = 0;
		printf("%ws=%ws\n", chars, equal + 1);
		chars = next;
	}

	NtClose(hProcess);
	return 0;
}
