// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ErrorHelper.h"
#include "FrameworkApplication.g.h"
#include "host.h"
#include <DesignMode.h>
#include <FrameworkUdk/FinalUnhandledErrorDetected.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;

FinalUnhandledErrorDetectedRegistration ErrorHelper::m_finalUnhandledErrorDetectedRegistration;

//-----------------------------------------------------------------------------
//
// Gets the last reported error message to the core error service, if any.
//
// Does NOT clear the core error state.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT ErrorHelper::GetCoreErrorMessage(_Out_ xstring_ptr* pstrMessage)
{
    pstrMessage->Reset();

    DXamlCore* pCore = DXamlCore::GetCurrentNoCreate();
    if (!pCore)
    {
        return S_OK;
    }

    CCoreServices* pCoreServices = pCore->GetHandle();
    if (!pCoreServices)
    {
        return S_OK;
    }

    IError* pError = nullptr;
    IErrorService* pErrorService = nullptr;
    IFC_NOTRACE_RETURN(pCoreServices->getErrorService(&pErrorService));
    if (FAILED(pErrorService->GetLastReportedError(&pError)) || !pError)
    {
        return S_OK;
    }

    IFC_NOTRACE_RETURN(pError->GetErrorMessage(pstrMessage));

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Resets the CCoreServices error state:
//     - cleans any errors from the error services
//     - resets the "error during layout" flag
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT ErrorHelper::CleanupCoreErrorState()
{
    DXamlCore* pCore = DXamlCore::GetCurrentNoCreate();
    if (!pCore)
    {
        return S_OK;
    }

    CCoreServices* pCoreServices = pCore->GetHandle();
    if (!pCoreServices)
    {
        return S_OK;
    }

    // cleanup the core error service
    IErrorService* pErrorService = nullptr;
    if (SUCCEEDED(pCoreServices->getErrorService(&pErrorService)))
    {
        pErrorService->CleanupErrors();
    }

    // reset the "error occurred during layout" flag
    IFC_NOTRACE_RETURN(CoreImports::LayoutManager_ClearErrorOccurredDuringLayout(pCoreServices));

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Looks up an unlocalized error string.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT ErrorHelper::GetNonLocalizedErrorString(_In_ XUINT32 stringId, _Out_ xstring_ptr* pstrMessage)
{
    pstrMessage->Reset();

    DXamlCore* pCore = DXamlCore::GetCurrentNoCreate();
    if (!pCore)
    {
        return S_OK;
    }

    CCoreServices* pCoreServices = pCore->GetHandle();
    if (!pCoreServices)
    {
        return S_OK;
    }

    IXcpBrowserHost* pBrowserHost = pCoreServices->GetBrowserHost();
    if (!pBrowserHost)
    {
        return S_OK;
    }

    // Note that if the stringId isn't found, we still want to return success
    // from this function, and leave the out param nullptr.
    IGNOREHR(pBrowserHost->GetNonLocalizedErrorString(stringId, pstrMessage));

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// If hrError is an encoded AgCode, looks up the unlocalized message string
// associated with that AgCode.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT ErrorHelper::GetMessageFromAgErrorCode(_In_ HRESULT hrError, _Out_ xstring_ptr* pstrMessage)
{
    HRESULT hr = S_OK;

    pstrMessage->Reset();

    XUINT32 agCode = AgCodeFromHResult(hrError);
    if (0 == agCode)
    {
        // not an AgErrorCode
        goto Cleanup;
    }

    IFC_NOTRACE(GetNonLocalizedErrorString(agCode, pstrMessage));

Cleanup:
    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
// Attempts to look up an error message string from the system message table
// corresponding to the passed-in HRESULT.
//
// If no system message is available, the function succeeds and the out param
// is set to NULL.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT ErrorHelper::GetSystemMessageForHresult(HRESULT hrError, _Out_ xstring_ptr* pstrMessage)
{
    HRESULT hr = S_OK;
    WCHAR* wszMessage = NULL;

    pstrMessage->Reset();

    // Is passing 0 for dwLanguageId correct? The documentation for FormatMessage explains the
    // sequence of LANGIDs that will be tried in this case. The first attempt is the neutral language,
    // but perhaps we would want to try using the application language first?
    //
    // I think this is fine for now, it's just an error message used in a fallback case,
    // but we can revisit this if we find a real-world scenario where it's an issue.

    DWORD cchMessage = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        hrError,
        0,  // dwLanguageId
        reinterpret_cast<LPWSTR>(&wszMessage),
        0,
        NULL);

    if (cchMessage)
    {
        IFC(xstring_ptr::CloneBuffer(wszMessage, cchMessage, pstrMessage));
    }

Cleanup:
    if (wszMessage)
    {
        LocalFree(wszMessage);
    }

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Determines the "best" error message to use, trying a variety of sources of
// error information in sequence.
//
// This will always return some message - as a last resort it will fall back
// to a hard-coded message.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT ErrorHelper::GetBestErrorMessage(_In_ ErrorInfo& errorInfo, HRESULT hrError, _Out_ xstring_ptr* pstrMessage)
{
    HRESULT hr = S_OK;
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strFallbackErrorString, L"Unknown error.");

    //
    // We're going to use the message to create an IRestrictedErrorInfo.
    //
    // IRestrictedErrorInfo::SetErrorDetails requires non-null, non-empty description/restrictedDescription,
    // or E_INVALIDARG will be returned. Therefore we need to return *something* for the error message.
    //
    // We'll try a sequence of things, going from high-quality data to lower-quality data. As a last resort
    // we'll return a hard-coded string.
    //

    // first try the error info object's message
    if (errorInfo.HasMessage())
    {
        IFC_NOTRACE(errorInfo.GetMessage(pstrMessage));
        goto Cleanup;
    }

    // next try the core error service
    IFC_NOTRACE(GetCoreErrorMessage(pstrMessage));
    if (!pstrMessage->IsNullOrEmpty())
    {
        goto Cleanup;
    }

    // next see if the hr is an encoded AgCode that can be used to resolve an error message
    if (S_OK != hrError)
    {
        IFC_NOTRACE(GetMessageFromAgErrorCode(hrError, pstrMessage));
        if (!pstrMessage->IsNullOrEmpty())
        {
            goto Cleanup;
        }
    }

    // next try the system message for the HRESULT
    IFC_NOTRACE(GetSystemMessageForHresult(hrError, pstrMessage));
    if (!pstrMessage->IsNullOrEmpty())
    {
        goto Cleanup;
    }

    // next try a well-known HRESULT that maps to "The text associated with this error code could not be found."
    IFC_NOTRACE(GetSystemMessageForHresult(RO_E_ERROR_STRING_NOT_FOUND, pstrMessage));
    if (!pstrMessage->IsNullOrEmpty())
    {
        goto Cleanup;
    }

    // last resort
    *pstrMessage = c_strFallbackErrorString;

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Attempts to raise the Application.UnhandledException event. If this isn't
// possible (e.g. there is no Application instance available), no-ops.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT ErrorHelper::RaiseUnhandledExceptionEvent(_In_ HRESULT hrToReport, _In_ const xstring_ptr& strMessage, _Inout_ bool* pfHandled)
{
    HRESULT hr = S_OK;

    xruntime_string_ptr strRuntimeMessage;

    FrameworkApplication* pApplication = FrameworkApplication::GetCurrentNoRef();
    if (!pApplication)
    {
        // It's expected that sometimes we won't have an application instance at this point.
        // We may be running in a hosting mode that doesn't use Application. Or we may be
        // processing an unhandled error that occurred before we created the Application
        // instance. This isn't an error - just no-op and return success.
        goto Cleanup;
    }

    if (!strMessage.IsNullOrEmpty())
    {
        IFC(strMessage.Promote(&strRuntimeMessage));
    }

    IFC_NOTRACE(pApplication->RaiseUnhandledExceptionEvent(hrToReport, strRuntimeMessage.GetHSTRING(), pfHandled));

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Maps an encountered HRESULT to a possibly different HRESULT that we can
// use for reporting to the app.
//
// Currently just handles encoded AgCodes.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT ErrorHelper::MapHresult(_In_ HRESULT hrEncountered, _Out_ HRESULT* phrToReport)
{
    HRESULT hr = S_OK;

    XUINT32 agCode = AgCodeFromHResult(hrEncountered);
    if (0 == agCode)
    {
        // not an AgErrorCode
        *phrToReport = hrEncountered;
        goto Cleanup;
    }

    switch (agCode)
    {
        case AG_E_LAYOUT_CYCLE:
            *phrToReport = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_LAYOUTCYCLE);
            break;

        case UIA_ELEMENT_IS_VIRTUALIZED:
            *phrToReport = UIA_E_ELEMENTNOTAVAILABLE;
            break;

        case UIA_OPERATION_CANNOT_BE_PERFORMED:
        case UIA_INVALID_ITEMSCONTROL_PARENT:
            *phrToReport = UIA_E_INVALIDOPERATION;
            break;
        case UIA_GETTEXT_OUTOFRANGE_LENGTH:
            *phrToReport = E_INVALIDARG;
            break;

        default:
            // We don't want to report an AgCode to the app - they are internal.
            // Report generic E_FAIL instead.
            *phrToReport = E_FAIL;
            break;
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Returns TRUE if we should fail-fast in response to an unhandled error,
// even if the app sets handled=true.
//
// Currently this only happens for errors that occur during layout.
//
//-----------------------------------------------------------------------------
bool ErrorHelper::ShouldForceFailFast()
{
    bool fLayoutError;

    DXamlCore* pCore = DXamlCore::GetCurrentNoCreate();
    if (!pCore)
    {
        return false;
    }

    CCoreServices* pCoreServices = pCore->GetHandle();
    if (!pCoreServices)
    {
        return false;
    }

    if (DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
    {
        return false;
    }

    if (FAILED(CoreImports::LayoutManager_DidErrorOccurDuringLayout(pCoreServices, &fLayoutError)))
    {
        return false;
    }

    // We force fail-fast when there was an error during layout.
    return fLayoutError;
}

//-----------------------------------------------------------------------------
//
// Called when we encounter an unhandled error returned from a delegate that
// we are wrapping.
//
// In other words, this function is used in the following situation:
//     Event -> XAML handler -> wrapped handler
// For example:
//     CoreWindow.SizeChanged -> RaiseSizeChangedEvent (xaml) -> OnSizeChanged (app)
//
// We check to see if WRL itself will report the error if we propagate it back
// to the event source. If so, we do nothing and let the error propagate.
//
// If not, we need to report the error directly from the wrapper event handler,
// or it will be ignored.
//
//-----------------------------------------------------------------------------
void ErrorHelper::ReportUnhandledErrorFromWrappedDelegate(_In_ HRESULT hrError)
{
    if (AreWRLDelegateErrorsReported())
    {
        return;
    }

    IGNOREHR(ReportUnhandledError(hrError));
}

//-----------------------------------------------------------------------------
//
// Called when we encounter an unhandled error in XAML code. This is called
// after app code no longer has any chance to handle the error through normal
// propagation.
//
// We will raise the last-chance error events
// (CoreApplication.UnhandledErrorDetected and Application.UnhandledException).
//
// If the app doesn't mark the error has handled from the last-chance events,
// we will fail-fast the process.
//
// TODO: we should change this function to return void instead of an error code.
// All callers currently ignore the return value of this function. It will never
// propagate an error because:
//     - Normally, calling this function will result in a fail-fast.
//
//     - Any errors encountered within this function - as a result of processing
//       the unhandled error - will always result in a fail-fast.
//
//     - If the app marks the error as handled and we don't fail-fast, the
//       caller will continue on. In that case, the caller should decide whether
//       to keep propagating the unhandled error or stop propagating it. Stopping
//       propagation is preferred but may not always be the right thing to do.
//       See callers of this function for several comment blocks explaining this
//       situation.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT ErrorHelper::ReportUnhandledError(_In_ HRESULT hrError)
{
    HRESULT hr = S_OK;
    ErrorInfo errorInfo;
    HRESULT hrToReport;
    xstring_ptr strMessage;
    bool fHandled = false;

    // Get the thread's error info (if any).
    //
    // Note that this will leave the error info on the thread. Leaving the error on the thread
    // is done for several reasons:
    //     - The VS debugger wants to be able to show the original error info when
    //       broken in at the UnhandledException handler.
    //     - RoFailFast will use the thread's error info as the first stowed exception
    //       in the array of stowed exceptions it creates. This is the primary mechanism
    //       by which the app exception gets reported in the crashdump.
    //
    // The downside is that if the app hits another failure inside the UnhandledException
    // handler, the original error info may be incorrectly attributed to the new failure.
    //
    // However, the benefit of leaving the error info on the thread outweighs the downside.
    IFC_NOTRACE(errorInfo.GetFromThread());

    // If the thread didn't already have an IRestrictedErrorInfo, we need to create
    // one now and set it on the thread. RoReportUnhandledError requires a non-NULL
    // IRestrictedErrorInfo.
    if (!errorInfo.HasRestrictedErrorInfo())
    {
        IFC_NOTRACE(MapHresult(hrError, &hrToReport));
        IFC_NOTRACE(GetBestErrorMessage(errorInfo, hrError, &strMessage));
        IFC_NOTRACE(errorInfo.CreateRestrictedErrorInfo(hrToReport, strMessage));
        IFC_NOTRACE(errorInfo.SetOnThread());
    }

    // Raise the CoreApplication [Final]UnhandledErrorDetected events.
    // If we have a registered handler for FinalUnhandledErrorDetected it will be invoked
    // during this call.
    hr = RoReportUnhandledError(errorInfo.GetRestrictedErrorInfoNoRef());

    // If RoReportUnhandledError fails that means something went wrong in CoreApplication
    // and the [Final]UnhandledErrorDetected events weren't raised. For example if
    // CoreApplication has already been uninitialized this can happen.
    //
    // In this case we need to directly call into our ProcessUnhandledError code path
    // that is normally called from our FinalUnhandledErrorDetected handler.
    //
    // Likewise, if we don't have a registered handler for FinalUnhandledErrorDetected then
    // call into our ProcessUnhandledError path directly.
    if (FAILED(hr) || !m_finalUnhandledErrorDetectedRegistration.IsRegistered())
    {
        IFC_NOTRACE(ProcessUnhandledError(errorInfo, FALSE /* fSkipFailFastIfNoErrorContext */, &fHandled));
    }

Cleanup:
    if (FAILED(hr))
    {
        // If we hit a secondary failure while attempting to process the (primary) unhandled error,
        // fail fast.
        //
        // Note that we do not fail-fast with the HRESULT of the secondary error, but use the primary
        // error's HRESULT.
        //
        // This means that we could fail-fast here because of a bug in our error handling code,
        // and the crashdump will be bucketized using the original error. This is an acceptable
        // tradeoff - it means we're less likely to notice bugs in our error handling code, but
        // we won't be losing the more valuable original errors.
        FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(hrError);
    }

    // If the error was marked as handled (meaning we didn't fail fast), clear
    // the error info and core error state, since they are now stale.
    IGNOREHR(SetErrorInfo(0, NULL));
    IGNOREHR(CleanupCoreErrorState());

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Called to process an unhandled error that's previously been reported.
//
// This is normally called from our handler for CoreApplication's
// FinalUnhandledErrorDetected event. We will also call it directly in cases
// where this event isn't available.
//
// This function raises the Application.UnhandledException event. If app code
// doesn't mark the error as handled within that event, we will fail-fast the
// process.
//
// fSkipFailFastIfNoErrorContext allows us to unwind back into CoreApplication
// code when we're processing an error not related to XAML. If the following
// are true:
//     - fSkipFailFastIfNoErrorContext is TRUE
//     - we have no XAML error context
//     - app code does not mark the error as handled
// then instead of failing fast we will return with *pfHandled==FALSE.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT ErrorHelper::ProcessUnhandledError(_In_ ErrorInfo& errorInfo, bool fSkipFailFastIfNoErrorContext, _Inout_ bool* pfHandled)
{
    xstring_ptr strMessage;
    HRESULT hrErrorToReport;

    // Obtain the error message to report, if any
    if (errorInfo.HasMessage())
    {
        IFC_NOTRACE_RETURN(errorInfo.GetMessage(&strMessage));
    }

    // Obtain and map the HRESULT to report
    IFCEXPECT_ASSERT_NOTRACE_RETURN(errorInfo.HasErrorCode());
    IFC_NOTRACE_RETURN(MapHresult(errorInfo.GetErrorCode(), &hrErrorToReport));

    // raise the UnhandledException event
    IFC_NOTRACE_RETURN(RaiseUnhandledExceptionEvent(hrErrorToReport, strMessage, pfHandled));

    bool fForceFailFast = ShouldForceFailFast();

    // If a handler marked the error as handled, don't fail-fast (unless we're forced to)
    if (*pfHandled && !fForceFailFast)
    {
        return S_OK;
    }

    // There is chance that an UnhandledException handler has removed the error info from
    // the thread.  Without the error info on the thread, crash dumps will not be able to
    // properly !analyze to the correct crash stack.  Set the errorInfo back on the thread
    // to ensure the thread (and therefore crash dumps) will have access to this originating
    // error info.
    // Ignore the returned HRESULT, to always continue to process the original error.
    IGNOREHR(errorInfo.SetOnThread());

    // If the caller wants to skip fail-fast if there are no error contexts,
    // don't fail-fast when there are no error contexts (unless we're forced to)
    if (fSkipFailFastIfNoErrorContext && !HasCapturedErrorContexts() && !fForceFailFast)
    {
        return S_OK;
    }

    // Note: the HR we pass in here must match the HR of the thread's error info object. If they
    // don't match, RoFailFast will ignore the thread's error info object and it won't be used
    // to build the first stowed exception.
    FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(errorInfo.GetErrorCode());

    return S_OK;
}

_Check_return_ HRESULT ErrorHelper::OriginateError(
    _In_ HRESULT hrEncountered,
    _In_opt_ HSTRING hErrorMessage)
{
    // Not using IFC here is important. The pattern is that OriginateError will return the reported
    // error as its own return code. This allows callers to call OriginateError() and propagate
    // the error as a single step. We need to NOTRACE here so that the captured error context
    // begins at the caller of OriginateError().
    return OriginateError(
        hrEncountered,
        XSTRING_PTR_EPHEMERAL_FROM_HSTRING(hErrorMessage));
}

_Check_return_ HRESULT ErrorHelper::OriginateError(
    _In_ HRESULT hrEncountered,
    _In_ XUINT32 nErrorMessageLength,
    _In_reads_opt_(nErrorMessageLength) const WCHAR* pszErrorMessage)
{
    // Not using IFC here is important. The pattern is that OriginateError will return the reported
    // error as its own return code. This allows callers to call OriginateError() and propagate
    // the error as a single step. We need to NOTRACE here so that the captured error context
    // begins at the caller of OriginateError().
    return OriginateError(
        hrEncountered,
        XSTRING_PTR_EPHEMERAL2(pszErrorMessage, nErrorMessageLength));
}

_Check_return_ HRESULT ErrorHelper::OriginateError(
    _In_ HRESULT hrEncountered,
    _In_ const xstring_ptr_view& strErrorMessage,
    bool outputToDebugger /* =false*/)
{
    HRESULT hr = S_OK;
    HRESULT hrToReport = E_FAIL;

    xstring_ptr strAdjustedErrorMessage;
    ICreateErrorInfo* pCreateErrorinfo = NULL;
    IErrorInfo* pErrorinfo = NULL;

    IFC(MapHresult(hrEncountered, &hrToReport));

    if (!strErrorMessage.IsNullOrEmpty())
    {
        IFC(strErrorMessage.Promote(&strAdjustedErrorMessage));
    }
    else
    {
        IFC(GetMessageFromAgErrorCode(hrEncountered, &strAdjustedErrorMessage));
    }

    // We should always have an error message that goes with our error code.
    ASSERT(!strAdjustedErrorMessage.IsNullOrEmpty());

    if (!strAdjustedErrorMessage.IsNullOrEmpty())
    {
        if (outputToDebugger)
        {
            ::OutputDebugString(L"ERROR: ");
            ::OutputDebugString(strAdjustedErrorMessage.GetBuffer());
            ::OutputDebugString(L"\n");
        }

        IFCCHECK(wf::Diagnostics::OriginateError(hrToReport, strAdjustedErrorMessage.GetCount(), strAdjustedErrorMessage.GetBuffer()));

        // check if the WinRT error mechanism is currently configured to call SetErrorInfo
        // if not, we will call it ourselves to ensure that we always propagate this error string
        UINT32 winrtErrorReportingFlags;
        IFC(wf::Diagnostics::GetErrorReportingFlags(&winrtErrorReportingFlags));

        if (((winrtErrorReportingFlags & wf::Diagnostics::SuppressSetErrorInfo) != 0) ||
            ((winrtErrorReportingFlags & wf::Diagnostics::UseSetErrorInfo) == 0 && !IsDebuggerPresent()))
        {
            IFC(CreateErrorInfo(&pCreateErrorinfo));
            IFC(pCreateErrorinfo->SetDescription(const_cast<WCHAR*>(strErrorMessage.GetBuffer())));
            IFC(pCreateErrorinfo->QueryInterface(IID_IErrorInfo, reinterpret_cast<void**>(&pErrorinfo)));
            IFC(SetErrorInfo(0, pErrorinfo));
        }
    }

Cleanup:
    ReleaseInterface(pCreateErrorinfo);
    ReleaseInterface(pErrorinfo);
    return hrToReport;
}

_Check_return_ HRESULT ErrorHelper::OriginateErrorUsingResourceID(
    _In_ HRESULT hrEncountered,
    _In_ XUINT32 resourceStringID)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strResourceString;

    IFC(DXamlCore::GetCurrent()->GetNonLocalizedErrorString(resourceStringID, strResourceString.GetAddressOf()));
    IFCEXPECT(strResourceString.Get());

    // NOTRACE here is important. The pattern is that OriginateError will return the reported
    // error as its own return code. This allows callers to call OriginateError() and propagate
    // the error as a single step. We need to NOTRACE here so that the captured error context
    // begins at the caller of OriginateError().
    IFC_NOTRACE(OriginateError(hrEncountered, strResourceString.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ErrorHelper::OriginateErrorUsingFormattedResourceID(
    _In_ HRESULT hrEncountered,
    _In_ XUINT32 resourceStringFormatID,
    _In_ const xstring_ptr_view& strParam1,
    const WCHAR* param2)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strResourceString;
    size_t bufferSize = 0;
    WCHAR * pszBuffer = nullptr;

    IFC(DXamlCore::GetCurrent()->GetNonLocalizedErrorString(resourceStringFormatID, strResourceString.GetAddressOf()));
    IFCEXPECT(strResourceString.Get());

    // NOTRACE here is important. The pattern is that OriginateError will return the reported
    // error as its own return code. This allows callers to call OriginateError() and propagate
    // the error as a single step. We need to NOTRACE here so that the captured error context
    // begins at the caller of OriginateError().
    bufferSize = strResourceString.Length() + strParam1.GetCount() + 1;
    if (param2 != nullptr)
    {
        bufferSize += wcslen(param2);
    }
    pszBuffer = new WCHAR[bufferSize];
    IFCEXPECT(pszBuffer);
    if (param2 != nullptr)
    {
        IFCEXPECT(swprintf_s(pszBuffer, bufferSize, strResourceString.GetRawBuffer(nullptr), strParam1.GetBuffer(), param2) >= 0);
    }
    else
    {
        IFCEXPECT(swprintf_s(pszBuffer, bufferSize, strResourceString.GetRawBuffer(nullptr), strParam1.GetBuffer()) >= 0);
    }
    IFC_NOTRACE(OriginateError(hrEncountered, bufferSize, pszBuffer));

Cleanup:
    delete[] pszBuffer;
    return hr;
}

_Check_return_ HRESULT ErrorHelper::ClearError()
{
    HRESULT hr = S_OK;
    IErrorInfo* pErrorInfo = NULL;

    // GetErrorInfo will clear the current error
    IFC(::GetErrorInfo(0, &pErrorInfo));

Cleanup:
    ReleaseInterface(pErrorInfo);
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Returns TRUE if WRL event delegate errors are reported as unhandled errors
// back to XAML.
//
// If this returns FALSE, WRL delegate errors won't be reported back to XAML,
// so we need to call ReportUnhandledError() directly for such errors.
//
//-----------------------------------------------------------------------------
bool ErrorHelper::AreWRLDelegateErrorsReported()
{
    return m_finalUnhandledErrorDetectedRegistration.IsRegistered();
}

//-----------------------------------------------------------------------------
//
// Gets the FinalUnhandledErrorDetectedRegistration global object.
//
//-----------------------------------------------------------------------------
FinalUnhandledErrorDetectedRegistration* ErrorHelper::GetFinalUnhandledErrorDetectedRegistration()
{
    return &m_finalUnhandledErrorDetectedRegistration;
}


FinalUnhandledErrorDetectedRegistration::FinalUnhandledErrorDetectedRegistration() :
    m_cRegistrationRefs(0)
{
    m_registrationToken.value = 0;
}

FinalUnhandledErrorDetectedRegistration::~FinalUnhandledErrorDetectedRegistration()
{
}

//-----------------------------------------------------------------------------
//
// Returns TRUE if we currently have a registration for the
// CoreApplication FinalUnhandledErrorDetected event.
//
//-----------------------------------------------------------------------------
bool FinalUnhandledErrorDetectedRegistration::IsRegistered()
{
    XcpAutoLock lock(m_lock);

    return m_registrationToken.value != 0;
}

//-----------------------------------------------------------------------------
//
// Increments the refcount of our FinalUnhandledErrorDetected event registration.
//
// If this is the first AddRefRegistration() call it will trigger registering for
// the event.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT FinalUnhandledErrorDetectedRegistration::AddRefRegistration()
{
    HRESULT hr = S_OK;
    XcpAutoLock lock(m_lock);

    if (++m_cRegistrationRefs == 1)
    {
        IFC(Register());
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Decrements the refcount of our FinalUnhandledErrorDetected event registration.
//
// If this is the final ReleaseRegistration() call it will trigger unregistering for
// the event.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT FinalUnhandledErrorDetectedRegistration::ReleaseRegistration()
{
    HRESULT hr = S_OK;
    XcpAutoLock lock(m_lock);

    if (--m_cRegistrationRefs == 0)
    {
        IFC(Unregister());
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Registers for the CoreApplication FinalUnhandledErrorDetected event.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT FinalUnhandledErrorDetectedRegistration::Register()
{
    IFCEXPECT_ASSERT_RETURN(!m_registrationToken.value);

    IFC_RETURN(FinalUnhandledErrorDetected_RegisterHandler(
        Microsoft::WRL::Callback<
            Microsoft::WRL::Implements<
                Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                wf::IEventHandler<wac::UnhandledErrorDetectedEventArgs*>,
                Microsoft::WRL::FtmBase>>(
            &FinalUnhandledErrorDetectedRegistration::OnFinalUnhandledErrorDetected).Get(),
        &m_registrationToken
    ));

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Unregisters for the CoreApplication FinalUnhandledErrorDetected event.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT FinalUnhandledErrorDetectedRegistration::Unregister()
{
    if (m_registrationToken.value)
    {
        IFC_RETURN(FinalUnhandledErrorDetected_UnregisterHandler(&m_registrationToken));
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// This is our initial handler for the CoreApplication FinalUnhandledErrorDetected
// event.
//
// We pull the necessary data from the event args and then call
// ErrorHelper::ProcessUnhandledError.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT FinalUnhandledErrorDetectedRegistration::OnFinalUnhandledErrorDetected(_In_ IInspectable* pSender, _In_ wac::IUnhandledErrorDetectedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    bool fPropagateFailureBackToCoreApp = false;
    ctl::ComPtr<wac::IUnhandledError> spUnhandledError;
    ErrorInfo errorInfo;
    boolean fInitiallyHandled;
    bool fHandled;

    IFC_NOTRACE(pArgs->get_UnhandledError(&spUnhandledError));

    // The order of calling get_Handled() and Propagate() is important.
    //
    // Propagate() will set the handled property to true (and CoreApplication will
    // reset the handled property to false if this delegate returns a failure).
    //
    // Therefore we must call get_Handled() before Propagate(). We want to pick
    // up the handled value that was set by any handlers for the UnhandledErrorDetected event.

    IFC_NOTRACE(spUnhandledError->get_Handled(&fInitiallyHandled));
    fHandled = !!fInitiallyHandled;

    // Calling Propagate() will place the restricted error info on the thread.
    // Ignore the return value, which will be the failure HR associated with unhandled error.
    IGNOREHR(spUnhandledError->Propagate());

    // Get the restricted error info off the thread.
    //
    // This will retain the error info on the thread. For rationale, see the comments in
    // ProcessUnhandledError().
    IFC_NOTRACE(errorInfo.GetFromThread());

    IFC_NOTRACE(ErrorHelper::ProcessUnhandledError(errorInfo, TRUE /* fSkipFailFastIfNoErrorContext */, &fHandled));

    // If we get to here and the unhandled error was not marked as handled, that's because
    // ProcessUnhandledError skipped the fail-fast due to not having any error contexts.
    if (!fHandled)
    {
        // Returning a failure code from this handler will cause CoreApplication to treat the
        // unhandled error as truly unhandled, and CoreApplication will fail-fast the process.
        // (see CoreApplication code - UnhandledErrorInvokeHelper - for details)
        IFCEXPECT_ASSERT_NOTRACE(errorInfo.HasErrorCode());
        hr = errorInfo.GetErrorCode();
        fPropagateFailureBackToCoreApp = TRUE;
    }

Cleanup:
    if (FAILED(hr) && !fPropagateFailureBackToCoreApp)
    {
        // If we hit a secondary failure while attempting to process the (primary) unhandled error,
        // fail fast.
        //
        // We could always propagate the failure back to CoreApplication, which will fail-fast.
        // However, if we do our own fail-fast, we can include any xaml error context. So
        // we only propagate the failure in the case where we didn't hit any secondary error,
        // and there is no xaml error context.
        //
        // Note that we do not fail-fast with the HRESULT of the secondary error, but use the primary
        // error's HRESULT.
        //
        // This means that we could fail-fast here because of a bug in our error handling code,
        // and the crashdump will be bucketized using the original error. This is an acceptable
        // tradeoff - it means we're less likely to notice bugs in our error handling code, but
        // we won't be losing the more valuable original errors.
        HRESULT hrToFailFastWith = errorInfo.HasErrorCode() ? errorInfo.GetErrorCode() : hr;
        FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(hrToFailFastWith);
    }

    RRETURN(hr);
}



