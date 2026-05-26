// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h> // to define CONTEXT
#include <vector>
#include <string>

// Default top frames that are skipped when recording a WarningContext stack trace.
// For layout cycles, these are:
//   OnWarningEncountered(type, extraInfo, framesToSkip)
//   CDependencyObject::StoreWarningContext(type, warningInfo, framesToSkip)
//   CUIElement::StoreLayoutCycleWarningContext(warningInfo, layoutManager, framesToSkip)
const size_t DEFAULT_WARNING_FRAMES_TO_SKIP = 3;

// INSTRUCTION_ADDRESS represents frame addresses in a stack trace.
typedef void* INSTRUCTION_ADDRESS;
typedef struct _STOWED_EXCEPTION_INFORMATION_V2 *PSTOWED_EXCEPTION_INFORMATION_V2;

// Various types for WarningContext.
namespace WarningContextLog
{
    enum class WarningContextType
    {
        LayoutCycle, // Warnings about layout cycles likely to end up in a crash.
        // Add your own new warning type here.
        Other
    };
}

extern "C" {
    // Declare the _ReturnAddress intrinsic.
    void * _ReturnAddress(void);


    // Called when the XAML framework DLL or extension DLLs are loaded.
    // If errorContextTlsIndex is a valid index, then the TLS index is initialized to that value;
    // otherwise, we allocate a new TLS index.
    _Check_return_ HRESULT ErrorContextGlobalInit(DWORD errorContextTlsIndex);


    // Called when the XAML framework DLL or extension DLLs are loaded.
    // If warningContextTlsIndex is a valid index, then the TLS index is initialized to that value;
    // otherwise, we allocate a new TLS index.
    _Check_return_ HRESULT WarningContextGlobalInit(DWORD warningContextTlsIndex);


    // Called when the XAML framework DLL or extension DLLs are unloaded.
    // This de-allocates global resources used by the ErrorContext system if this DLL has initialized them,
    // or just sets our TLS index to be uninitialized if it hasn't.
    _Check_return_ HRESULT ErrorContextGlobalDeinit();


    // Called when the XAML framework DLL or extension DLLs are unloaded.
    // This de-allocates global resources used by the WarningContext system if this DLL has initialized them,
    // or just sets our TLS index to be uninitialized if it hasn't.
    _Check_return_ HRESULT WarningContextGlobalDeinit();


    // Called when a thread is deinitialized. This de-allocates memory the
    // error context system has allocated that was specific to the thread.
    _Check_return_ HRESULT ErrorContextThreadDeinit();


    // Called when a thread is deinitialized. This de-allocates memory the
    // warning context system has allocated that was specific to the thread.
    _Check_return_ HRESULT WarningContextThreadDeinit();


    // Captures a new ErrorContext.
    // This function should be called as closely as possible to the error origination point.
    bool CaptureErrorContext(HRESULT failedHR, INSTRUCTION_ADDRESS callerReturnAddress, _In_opt_ CONTEXT* contextRecord, _In_opt_ std::vector<std::wstring>* extraInfo = nullptr);


    // Called to update the existing ErrorContext with new information as the stack unwinds during error propagation.
    // The caller supplies the current frame address and the current HR as of that frame.
    // If the frame address is found within the error context, the context is updated with the HR, and TRUE is returned.
    // If the frame address can't be found, false is returned.
    bool UpdateErrorContext(HRESULT frameHR, INSTRUCTION_ADDRESS callerReturnAddress, _In_opt_ std::vector<std::wstring>* extraInfo = nullptr);


    // Clears the existing error context.
    void ClearErrorContext();


    // Deletes the existing warning contexts with the provided type.
    void ClearWarningContexts(WarningContextLog::WarningContextType type);


    // Called by DxamlCoreTestHooks::PostTestCheckForLeaks to discard the extra info strings
    // after a test completes and before ErrorContextThreadDeinit() is called.
    void ResetErrorContextsExtraInfo();


    // Returns TRUE if the calling thread has captured any error contexts.
    bool HasCapturedErrorContexts();


    // Returns the TLS index for the error context.
    DWORD GetErrorContextTlsIndex();


    // Returns the TLS index for the error context.
    DWORD GetWarningContextTlsIndex();


    // Helper function that combines several of the above operations.
    // Intended to be called when originating a new failure.
    //
    //  1) captures a new error context: CaptureErrorContext()
    //  2) updates the captured context: UpdateErrorContext()
    //
    // Returns TRUE if both operations succeed.
    __declspec(noinline) bool OnNewFailureEncountered(HRESULT failedFrameHR, INSTRUCTION_ADDRESS directCallerReturnAddress, _In_opt_ CONTEXT* contextRecord);


    // Helper function that combines several of the above operations.
    // Intended to be called when either originating a new failure or propagating an existing failure.
    //
    //  1) propagation case:
    //     - first, try to update the existing error context: UpdateErrorContext()
    //     - if that succeeds, return TRUE
    //
    //  2) else, origination case:
    //     - capture a new error context: CaptureErrorContext()
    //     - update it: UpdateErrorContext()
    //     - returns TRUE if both operations succeed
    __declspec(noinline) bool OnFailureEncountered(HRESULT failedFrameHR, INSTRUCTION_ADDRESS directCallerReturnAddress, _In_opt_ CONTEXT* contextRecord, _In_opt_ std::vector<std::wstring>* extraInfo = nullptr);

    __declspec(noinline) bool OnWarningEncountered(WarningContextLog::WarningContextType type, _In_opt_ std::vector<std::wstring>* extraInfo = nullptr, size_t framesToSkip = DEFAULT_WARNING_FRAMES_TO_SKIP);


    // Returns the thread's accumulated stowed exceptions so they can be used
    // in a fail-fast call.
    void GetStowedExceptionsForFailFast(
        _Outptr_result_buffer_maybenull_(*pcStowedExceptions) PSTOWED_EXCEPTION_INFORMATION_V2** pppStowedExceptions,
        _Out_ unsigned int* pcStowedExceptions);

    void TraceForFailFast(HRESULT errorCode);
}

// To achieve a unique call site for each IFC(), use a template with an
// integer argument that takes __COUNTER__ as the value, to get a different
// template instantiation for each use of IFC().  However, intentionally
// don't do anything with the integer template parameter, to allow the
// linker to collapse identical functions.  This exploits the fact that
// the compiler will compile a function thinking that the template
// instantiations are potentially different, and won't go back and
// re-compile the function after the compiler/linker has re-combined all
// the instantiations - it just goes back and fixes up each call site,
// without combining the call sites.  Overall, we get exactly what we want,
// which is a non-ambiguous stowed exception, without too much module size
// increase (in fact, the checkin that does this reduced the module size
// overall, so no complaining please).
//
// This is heavily reliant on compiler/linker implementation details, and
// may fail to achieve its goal if the compiler/linker starts aggressively
// re-compiling function A that calls function B after B has been combined
// with function C.
template<unsigned int counter>
__declspec(noinline)
static bool OnFailure(HRESULT failedFrameHR)
{
    CONTEXT context;
    RtlCaptureContext(&context);
    return OnFailureEncountered(failedFrameHR, _ReturnAddress(), &context);
}

// Same as OnFailure above but with the extraInfo string array which provides additional debugging information.
// OnFailureWithExtraInfo is used by IFC_EXTRA_INFO.
// Using 'static bool OnFailure(HRESULT failedFrameHR, _In_opt_ std::vector<std::wstring>* extraInfo = nullptr)'
// would work but add 408KB to the free binaries.
template<unsigned int counter>
__declspec(noinline)
static bool OnFailureWithExtraInfo(HRESULT failedFrameHR, _In_opt_ std::vector<std::wstring>* extraInfo)
{
    CONTEXT context;
    RtlCaptureContext(&context);
    return OnFailureEncountered(failedFrameHR, _ReturnAddress(), &context, extraInfo);
}

template<unsigned int counter>
__declspec(noinline)
static bool OnNewFailure(HRESULT failedFrameHR)
{
    CONTEXT context;
    RtlCaptureContext(&context);
    return OnNewFailureEncountered(failedFrameHR, _ReturnAddress(), &context);
}

namespace ErrorContextLog
{
    enum class ErrorContextLogType
    {
        Suspend,
        Shutdown
    };
}

_Check_return_ HRESULT ErrorContextLogAccumulatedErrors(ErrorContextLog::ErrorContextLogType logType);

void SetProcessFailFastOnErrors(bool enabled);
bool GetProcessFailFastOnErrors();

class SuspendFailFastOnStowedException
{
public:
    SuspendFailFastOnStowedException();
    explicit SuspendFailFastOnStowedException(HRESULT codeToIgnore);

    ~SuspendFailFastOnStowedException();

private:
    bool m_previousSuspendState;
    HRESULT m_previousSuspendCode;
};

struct ContextBase;
struct ErrorContext;
struct WarningContext;
