// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h> // to define CONTEXT

// From https://docs.microsoft.com/en-us/windows/win32/wer/stowed-exception-information-v2
typedef struct _STOWED_EXCEPTION_INFORMATION_HEADER {
  ULONG Size;
  ULONG Signature;
} STOWED_EXCEPTION_INFORMATION_HEADER, *PSTOWED_EXCEPTION_INFORMATION_HEADER;

typedef struct _STOWED_EXCEPTION_INFORMATION_V2 {
  STOWED_EXCEPTION_INFORMATION_HEADER Header;
  HRESULT                             ResultCode;
  struct {
    DWORD ExceptionForm  :2;
    DWORD ThreadId  :30;
  };
  union {
    struct {
      PVOID ExceptionAddress;
      ULONG StackTraceWordSize;
      ULONG StackTraceWords;
      PVOID StackTrace;
    };
    struct {
      PWSTR ErrorText;
    };
  };
  ULONG                               NestedExceptionType;
  PVOID                               NestedException;
} STOWED_EXCEPTION_INFORMATION_V2, *PSTOWED_EXCEPTION_INFORMATION_V2;

// The maximum number of stack frames supported when capturing a stack trace.
const size_t MAX_FRAMES = 512;

struct ContextBase
{
    // The time this context was captured.
    FILETIME captureTime;

    // The thread this context was captured on.
    DWORD threadId;

    // The number of frames in this context.
    USHORT frameCount;

    // The array of frame instruction addresses.
    INSTRUCTION_ADDRESS frameAddresses[MAX_FRAMES];

    // A string array of context-specific extra info.
    std::vector<std::wstring> extraInfo;

    // Used during fail-fast. This is the structure that WER/Watson understand.
    STOWED_EXCEPTION_INFORMATION_V2 stowedException;
};


struct WarningContext : ContextBase
{
    WarningContextLog::WarningContextType type;

    // WarningContexts are stored as a per-thread linked list.
    WarningContext* next{ nullptr };
};


struct ErrorContext : public ContextBase
{
    // Bitwise flags - see ERROR_CONTEXT_FLAG_* above.
    DWORD flags;

    // The array of frame HRESULTs.
    HRESULT frameHRs[MAX_FRAMES];

    // An index of the current frame. Used during stack unwind to track our place in the error context.
    USHORT currentFrame;

    // See UpdateErrorContext().
    // The last address we tried to match to this context.
    // For debugging purposes only, in that this is not used by the mechanism for any internal purpose.
    INSTRUCTION_ADDRESS currentMatchAddress;

    // The processor context record at the time of the failure.
#pragma warning(push)
#pragma warning(disable : 4324)
    CONTEXT contextRecord;
#pragma warning(pop)

    // ErrorContexts are stored as a per-thread linked list.
    ErrorContext* next;
};
