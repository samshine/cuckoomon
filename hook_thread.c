/*
Cuckoo Sandbox - Automated Malware Analysis
Copyright (C) 2010-2012 Cuckoo Sandbox Developers

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <windows.h>
#include "hooking.h"
#include "ntapi.h"
#include "log.h"
#include "pipe.h"
#include "misc.h"

static IS_SUCCESS_NTSTATUS();
static const char *module_name = "threading";

HOOKDEF(NTSTATUS, WINAPI, NtCreateThread,
    __out     PHANDLE ThreadHandle,
    __in      ACCESS_MASK DesiredAccess,
    __in_opt  POBJECT_ATTRIBUTES ObjectAttributes,
    __in      HANDLE ProcessHandle,
    __out     PCLIENT_ID ClientId,
    __in      PCONTEXT ThreadContext,
    __in      PINITIAL_TEB InitialTeb,
    __in      BOOLEAN CreateSuspended
) {
    char buf[4]; int len = sizeof(buf);
    pipe2(buf, &len, "PID:%d", pid_from_process_handle(ProcessHandle));

    NTSTATUS ret = Old_NtCreateThread(ThreadHandle, DesiredAccess,
        ObjectAttributes, ProcessHandle, ClientId, ThreadContext,
        InitialTeb, CreateSuspended);
    LOQ("PpO", "ThreadHandle", ThreadHandle, "ProcessHandle", ProcessHandle,
        "ObjectAttributes", ObjectAttributes);
    return ret;
}

HOOKDEF(NTSTATUS, WINAPI, NtOpenThread,
    __out  PHANDLE ThreadHandle,
    __in   ACCESS_MASK DesiredAccess,
    __in   POBJECT_ATTRIBUTES ObjectAttributes,
    __in   PCLIENT_ID ClientId
) {
    NTSTATUS ret = Old_NtOpenThread(ThreadHandle, DesiredAccess,
        ObjectAttributes, ClientId);
    LOQ("PlO", "ThreadHandle", ThreadHandle, "DesiredAccess", DesiredAccess,
        "ObjectAttributes", ObjectAttributes);
    if(NT_SUCCESS(ret)) {
        char buf[4]; int len = sizeof(buf);
        pipe2(buf, &len, "PID:%d", ClientId->UniqueProcess);
    }
    return ret;
}

HOOKDEF(NTSTATUS, WINAPI, NtGetContextThread,
    __in     HANDLE ThreadHandle,
    __inout  LPCONTEXT Context
) {
    NTSTATUS ret = Old_NtGetContextThread(ThreadHandle, Context);
    LOQ("p", "ThreadHandle", ThreadHandle);
    return ret;
}

HOOKDEF(NTSTATUS, WINAPI, NtSetContextThread,
    __in  HANDLE ThreadHandle,
    __in  const CONTEXT *Context
) {
    NTSTATUS ret = Old_NtSetContextThread(ThreadHandle, Context);
    LOQ("p", "ThreadHandle", ThreadHandle);

    char buf[4]; int len = sizeof(buf);
    pipe2(buf, &len, "PID:%d", pid_from_thread_handle(ThreadHandle));
    return ret;
}

HOOKDEF(NTSTATUS, WINAPI, NtSuspendThread,
    __in        HANDLE ThreadHandle,
    __out_opt   ULONG *PreviousSuspendCount
) {
    ENSURE_ULONG(PreviousSuspendCount);

    NTSTATUS ret = Old_NtSuspendThread(ThreadHandle, PreviousSuspendCount);
    LOQ("pL", "ThreadHandle", ThreadHandle,
        "SuspendCount", PreviousSuspendCount);
    return ret;
}

HOOKDEF(NTSTATUS, WINAPI, NtResumeThread,
    __in        HANDLE ThreadHandle,
    __out_opt   ULONG *SuspendCount
) {
    ENSURE_ULONG(SuspendCount);

    NTSTATUS ret = Old_NtResumeThread(ThreadHandle, SuspendCount);
    LOQ("pL", "ThreadHandle", ThreadHandle, "SuspendCount", SuspendCount);
    return ret;
}

HOOKDEF(NTSTATUS, WINAPI, NtTerminateThread,
    __in  HANDLE ThreadHandle,
    __in  NTSTATUS ExitStatus
) {
    NTSTATUS ret = Old_NtTerminateThread(ThreadHandle, ExitStatus);
    LOQ("pl", "ThreadHandle", ThreadHandle, "ExitStatus", ExitStatus);
    return ret;
}

HOOKDEF(HANDLE, WINAPI, CreateThread,
  __in   LPSECURITY_ATTRIBUTES lpThreadAttributes,
  __in   SIZE_T dwStackSize,
  __in   LPTHREAD_START_ROUTINE lpStartAddress,
  __in   LPVOID lpParameter,
  __in   DWORD dwCreationFlags,
  __out  LPDWORD lpThreadId
) {
    IS_SUCCESS_HANDLE();

    HANDLE ret = Old_CreateThread(lpThreadAttributes, dwStackSize,
        lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
    LOQ("pplL", "StartRoutine", lpStartAddress, "Parameter", lpParameter,
        "CreationFlags", dwCreationFlags, "ThreadId", lpThreadId);
    return ret;
}

HOOKDEF(HANDLE, WINAPI, CreateRemoteThread,
  __in   HANDLE hProcess,
  __in   LPSECURITY_ATTRIBUTES lpThreadAttributes,
  __in   SIZE_T dwStackSize,
  __in   LPTHREAD_START_ROUTINE lpStartAddress,
  __in   LPVOID lpParameter,
  __in   DWORD dwCreationFlags,
  __out  LPDWORD lpThreadId
) {
    IS_SUCCESS_HANDLE();

    char buf[4]; int len = sizeof(buf);
    pipe2(buf, &len, "PID:%d", pid_from_process_handle(hProcess));

    HANDLE ret = Old_CreateRemoteThread(hProcess, lpThreadAttributes,
        dwStackSize, lpStartAddress, lpParameter, dwCreationFlags,
        lpThreadId);
    LOQ("3plL", "ProcessHandle", hProcess, "StartRoutine", lpStartAddress,
        "Parameter", lpParameter, "CreationFlags", dwCreationFlags,
        "ThreadId", lpThreadId);
    return ret;
}

HOOKDEF(VOID, WINAPI, ExitThread,
  __in  DWORD dwExitCode
) {
    IS_SUCCESS_VOID();

    int ret = 0;
    LOQ("l", "ExitCode", dwExitCode);
    Old_ExitThread(dwExitCode);
}
