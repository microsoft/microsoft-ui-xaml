// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DebugOutput.h"
#include "DebugSettings.g.h"
#include "FrameworkApplication.g.h"
#include "BindingFailedEventArgs.g.h"
#include "XamlResourceReferenceFailedEventArgs.g.h"
#include <XStringUtils.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

DebugOutput& DebugOutput::GetInstance()
{
    static INIT_ONCE s_initOnce = INIT_ONCE_STATIC_INIT;
    static DebugOutput* s_instance = nullptr;
    InitOnceExecuteOnce(
        &s_initOnce,
        InitOnceCallback,
        &s_instance,
        nullptr);

    return *s_instance;
}

BOOL CALLBACK DebugOutput::InitOnceCallback(
    PINIT_ONCE /* unused */,
    PVOID Parameter,
    PVOID * /* unused */)
{
    // This is guaranteed to run exactly once, so we initialize the singleton instance
    // here as a local static, and return its address via the passed in pointer
    static DebugOutput instance;
    instance.InitializeDebugSettings();
    *(static_cast<DebugOutput**>(Parameter)) = &instance;

    return TRUE;
}

void DebugOutput::InitializeDebugSettings()
{
    // Note: this is a one-time check to determine which tracing events can be raised
    BOOLEAN fBindingTracingEnabled = FALSE;
    BOOLEAN fXamlResourceReferenceTracingEnabled = FALSE;
    FrameworkApplication *pApp = FrameworkApplication::GetCurrentNoRef();
    wrl::ComPtr<xaml::IDebugSettings> debugSettings;
    wrl::ComPtr<xaml::IDebugSettings2> debugSettings2;

    // Get the debug settings
    if (pApp != nullptr)
    {
        IFCFAILFAST(pApp->get_DebugSettings(&debugSettings));
        IFCFAILFAST(debugSettings.As(&debugSettings2));

        // Binding tracing is only enabled if the debugger is present
        if (IsDebuggerPresent())
        {
            IFCFAILFAST(debugSettings->get_IsBindingTracingEnabled(&fBindingTracingEnabled));
        }
        
        IFCFAILFAST(debugSettings2->get_IsXamlResourceReferenceTracingEnabled(&fXamlResourceReferenceTracingEnabled));
    }
    else
    {
        // If there's no app then assume that the properties are set to TRUE
        fBindingTracingEnabled = TRUE;
        fXamlResourceReferenceTracingEnabled = TRUE;
    }

    m_isLoggingForBindingEnabled = !!fBindingTracingEnabled;
    m_isLoggingForXamlResourceReferenceEnabled = !!fXamlResourceReferenceTracingEnabled;
}

/* static */ bool DebugOutput::IsLoggingForBindingEnabled()
{
    return GetInstance().m_isLoggingForBindingEnabled || GetInstance().m_forceDebugSettingsTracingEvents;
}

/* static */ bool DebugOutput::IsLoggingForXamlResourceReferenceEnabled()
{
    return GetInstance().m_isLoggingForXamlResourceReferenceEnabled || GetInstance().m_forceDebugSettingsTracingEvents;
}

/* static */ void DebugOutput::LogBindingErrorMessage(_In_ xstring_ptr message)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    GetPALDebuggingServices()->DebugOutputSz(message.GetBuffer());

    FrameworkApplication *pApp = FrameworkApplication::GetCurrentNoRef();
    if (pApp)
    {
        wrl::ComPtr<xaml::IDebugSettings> debugSettings;
        DebugSettings *pDbg = nullptr;

        IFC(pApp->get_DebugSettings(&debugSettings));
        pDbg = static_cast<DebugSettings *>(debugSettings.Get());

        DebugSettings::BindingFailedEventSourceType* pEventSource = nullptr;
        ctl::ComPtr<BindingFailedEventArgs> spEventArgs;
        IFC(pDbg->GetBindingFailedEventSourceNoRef(&pEventSource));

        if (pEventSource->HasHandlers())
        {
            IFC(ctl::make(&spEventArgs));
            IFC(spEventArgs->put_Message(wrl_wrappers::HStringReference(message.GetBuffer(), message.GetCount()).Get()));
            IFC(pEventSource->Raise(nullptr, spEventArgs.Get()));
        }
    }

Cleanup:
    return;
}

/* static */ void DebugOutput::LogXamlResourceReferenceErrorMessage(_In_ xstring_ptr message)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    GetPALDebuggingServices()->DebugOutputSz(message.GetBuffer());

    FrameworkApplication *pApp = FrameworkApplication::GetCurrentNoRef();
    if (pApp)
    {
        wrl::ComPtr<xaml::IDebugSettings> debugSettings;
        DebugSettings *pDbg = nullptr;

        IFC(pApp->get_DebugSettings(&debugSettings));
        pDbg = static_cast<DebugSettings *>(debugSettings.Get());

        DebugSettings::XamlResourceReferenceFailedEventSourceType* pEventSource = nullptr;
        ctl::ComPtr<XamlResourceReferenceFailedEventArgs> spEventArgs;
        IFC(pDbg->GetXamlResourceReferenceFailedEventSourceNoRef(&pEventSource));

        if (pEventSource->HasHandlers())
        {
            IFC(ctl::make(&spEventArgs));
            IFC(spEventArgs->put_Message(wrl_wrappers::HStringReference(message.GetBuffer(), message.GetCount()).Get()));
            IFC(pEventSource->Raise(nullptr, spEventArgs.Get()));
        }
    }

Cleanup:
    return;
}