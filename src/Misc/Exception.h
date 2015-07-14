#pragma once

#include <Windows.h>
#include <Dbghelp.h>

#include <string>

class Exception
{
public:
	static const DWORD MS_VC_EXCEPTION = 0x406D1388ul;

	static LONG CALLBACK ExceptionFilter(PEXCEPTION_POINTERS pExs);
	static __declspec(noreturn) LONG CALLBACK ExceptionHandler(PEXCEPTION_POINTERS pExs);

	static std::wstring FullDump(PMINIDUMP_EXCEPTION_INFORMATION pException = nullptr);
	static std::wstring FullDump(std::wstring destinationFolder,
		PMINIDUMP_EXCEPTION_INFORMATION pException = nullptr);

	static std::wstring PrepareSnapshotDirectory();

	static __declspec(noreturn) void Exit(UINT ExitCode = 1u);
};