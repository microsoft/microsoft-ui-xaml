// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XcpAutoLock.h"

namespace DirectUI
{
    class ErrorInfo;

    //-----------------------------------------------------------------------------
    //
    // FinalUnhandledErrorDetectedRegistration manages our registration for the
    // CoreApplication FinalUnhandledErrorDetected event.
    //
    // The registration for the event needs some special treatment. It's a global
    // event and we only want to have a single handler. However, for various reasons
    // we must unregister before our dll is unloaded and before our DllMain unload
    // is called.
    //
    // This class uses a refcount mechanism to allow us to control the lifetime of
    // the registration.
    //
    //-----------------------------------------------------------------------------
    class FinalUnhandledErrorDetectedRegistration
    {
    public:
        FinalUnhandledErrorDetectedRegistration();
        ~FinalUnhandledErrorDetectedRegistration();

        bool IsRegistered();
        _Check_return_ HRESULT AddRefRegistration();
        _Check_return_ HRESULT ReleaseRegistration();

    private:
        _Check_return_ HRESULT Register();
        _Check_return_ HRESULT Unregister();

        static _Check_return_ HRESULT OnFinalUnhandledErrorDetected(_In_ IInspectable* pSender, _In_ wac::IUnhandledErrorDetectedEventArgs* pArgs);

        XcpAutoCriticalSection m_lock;
        XUINT32 m_cRegistrationRefs;
        EventRegistrationToken m_registrationToken;
    };

    class ErrorHelper
    {
    public:
        static _Check_return_ HRESULT GetCoreErrorMessage(_Out_ xstring_ptr* pstrMessage);
        static _Check_return_ HRESULT CleanupCoreErrorState();
        static _Check_return_ HRESULT GetNonLocalizedErrorString(_In_ XUINT32 stringId, _Out_ xstring_ptr* pstrMessage);
        static _Check_return_ HRESULT GetMessageFromAgErrorCode(_In_ HRESULT hrError, _Out_ xstring_ptr* pstrMessage);
        static _Check_return_ HRESULT GetBestErrorMessage(_In_ ErrorInfo& errorInfo, HRESULT hrError, _Out_ xstring_ptr* pstrMessage);
        static _Check_return_ HRESULT RaiseUnhandledExceptionEvent(_In_ HRESULT hrToReport, _In_ const xstring_ptr& strMessage, _Inout_ bool* pfHandled);
        static _Check_return_ HRESULT MapHresult(_In_ HRESULT hrEncountered, _Out_ HRESULT* phrToReport);
        static bool ShouldForceFailFast();
        static void ReportUnhandledErrorFromWrappedDelegate(_In_ HRESULT hrError);
        static _Check_return_ HRESULT ReportUnhandledError(_In_ HRESULT hrError);
        static _Check_return_ HRESULT ProcessUnhandledError(_In_ ErrorInfo& errorInfo, bool fSkipFailFastIfNoErrorContext, _Inout_ bool* pfHandled);
        static _Check_return_ HRESULT GetSystemMessageForHresult(HRESULT hrError, _Out_ xstring_ptr* pstrMessage);

        // TODO: Add support for passing arguments that can be used inside the error message.
        static _Check_return_ HRESULT OriginateError(_In_ HRESULT hrEncountered, _In_opt_ HSTRING hErrorMessage = NULL);
        static _Check_return_ HRESULT OriginateError(_In_ HRESULT hrEncountered, _In_ XUINT32 nErrorMessageLength, _In_reads_(nErrorMessageLength) const WCHAR* pszErrorMessage = NULL);
        static _Check_return_ HRESULT OriginateError(_In_ HRESULT hrEncountered, _In_ const xstring_ptr_view& strErrorMessage, bool outputToDebugger = false);
        static _Check_return_ HRESULT OriginateErrorUsingResourceID(_In_ HRESULT hrEncountered, _In_ XUINT32 resourceStringID);
        static _Check_return_ HRESULT OriginateErrorUsingFormattedResourceID(_In_ HRESULT hrEncountered, _In_ XUINT32 resourceStringFormatID, const xstring_ptr_view& strParam1, const WCHAR* param2 = nullptr);

        static _Check_return_ HRESULT ClearError();
        static bool AreWRLDelegateErrorsReported();

        static FinalUnhandledErrorDetectedRegistration* GetFinalUnhandledErrorDetectedRegistration();

    private:
        static FinalUnhandledErrorDetectedRegistration m_finalUnhandledErrorDetectedRegistration;
    };
}
