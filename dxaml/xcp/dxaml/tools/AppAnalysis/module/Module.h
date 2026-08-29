// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <memory>
#include <string>
#include <vector>
#include "TraceSession.h"
#include "microsoft.diagnostics.appanalysis.h"
#include "microsoft.diagnostics.appanalysis.internal.h"
#include "wil\resource.h"
#include "wrl\client.h"

typedef HRESULT(*pfnGetActivationFactory)(HSTRING, IActivationFactory**);

////////////////////////////////////////////////////////////////////////////////
//
// AppModule implements the main functionality of the Standalone App Analysis tool.
//
class AppModule final
{
public:
    AppModule();
    virtual ~AppModule();

    HRESULT
        Initialize(_In_ const std::wstring& customRuleSetName);

    HRESULT ProcessLive(
        _In_ DWORD processId);

    HRESULT ProcessLive(
        _In_ const std::wstring& exeName
    );

    HRESULT ProcessETL(
        _In_ const std::wstring& pathToETL
    );

    HRESULT Shutdown();

    HRESULT Notify(
        _In_ appanalysis::IRule* rule,
        _In_ appanalysis::IRuleTriggeredEventArgs* pNotification
    );

private:
    //HRESULT WriteChildren(
    //    _In_ const Microsoft::WRL::ComPtr<appanalysis::IRule>&,
    //    _In_ const Microsoft::WRL::ComPtr<appanalysis::IRuleTriggeredEventArgs>& parent);

    HRESULT WriteChild(
        _In_ const Microsoft::WRL::ComPtr<appanalysis::IRule>&,
        _In_ const Microsoft::WRL::ComPtr<appanalysis::IRuleTriggeredEventArgs>& info);

    HRESULT WriteSourceInfo(
        _In_ const Microsoft::WRL::ComPtr<appanalysis::IRuleTriggeredEventArgs>& info
    );

    HRESULT WriteRangeInfo(
        _In_ const Microsoft::WRL::ComPtr<appanalysis::IRuleTriggeredEventArgs>& info);

    HRESULT WriteMeasurement(
        _In_ const Microsoft::WRL::ComPtr<appanalysis::IRuleTriggeredEventArgs>& info);

    HRESULT WriteImpact(
        _In_ const Microsoft::WRL::ComPtr<appanalysis::IRule>& rule);

    HRESULT WriteLinkInfo(
        _In_ const Microsoft::WRL::ComPtr<appanalysis::IRule>& rule);

    HRESULT WriteResourceString(
        _In_ const Microsoft::WRL::ComPtr<appanalysis::IRule>& rule,
        _In_ LPCWSTR attributeName,
        _In_ appanalysis::IResourceStringView* string);

    HRESULT WriteAttribute(
        _In_ LPCWSTR attributeName,
        _In_ LPCWSTR attributeValue);

    HRESULT WriteElement(
        _In_ LPCWSTR elementName,
        _In_ LPCWSTR resourceId);

    PCWSTR  ConvertMeasurementUnit(
        _In_ appanalysis::MeasurementUnit unit);

    HRESULT RegisterRuleSet(
        const Microsoft::WRL::ComPtr<wfc::IVectorView<appanalysis::EtwRule*>>& ruleSet);

    HRESULT ProcessLiveInternal();
    HRESULT InitializeXmlReport();

    template <typename U>
    HRESULT CreateInstance(_In_ PCWSTR name, _Inout_ Microsoft::WRL::Details::ComPtrRef<Microsoft::WRL::ComPtr<U>> instance)
    {
        Microsoft::WRL::ComPtr<IActivationFactory> spClassFactory;
        Microsoft::WRL::ComPtr<IInspectable> instanceAsInsp;
        IFC_RETURN(GetActivationFactory(StringRef(name), &spClassFactory));
        IFC_RETURN(spClassFactory->ActivateInstance(&instanceAsInsp));

        IFC_RETURN(instanceAsInsp.As(instance));

        return S_OK;
    }

    static HRESULT GetRuleSetFromModule(
        _In_ HMODULE module,
        _In_z_ const wchar_t* className,
        _COM_Outptr_ wfc::IVectorView<appanalysis::EtwRule*>** instance);

    std::unique_ptr<TraceSession> m_spTraceSession;
    std::unique_ptr<TraceSession> m_spKernelTraceSession;
    Microsoft::WRL::ComPtr<IXmlWriter> m_spReportWriter;
    Microsoft::WRL::ComPtr<wfc::IVectorView<appanalysis::EtwRule*>> m_ruleSet;
    Microsoft::WRL::ComPtr<wfc::IVectorView<appanalysis::EtwRule*>> m_customRuleSet;

    std::vector<EventRegistrationToken> m_tokens;
    wil::unique_event m_kernelShutdownCompleteEvent;
    wil::unique_event m_manifestShutdownCompleteEvent;

    pfnGetActivationFactory GetActivationFactory{};

    wil::unique_hmodule m_customRuleSetModule;

    volatile UINT m_fShutdown;

    bool m_liveTraceStarted;
};
