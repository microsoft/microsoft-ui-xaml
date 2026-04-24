// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "Module.h"
#include "TraceSession.h"
#include <ppltasks.h>
#include "wil\resource.h"
#include "helpers.h"
#include "wil_resource.h"

static const GUID c_sessionGuid = { 0xdd119882, 0x64a7, 0x4ad5,{ 0xa5, 0x9b, 0xe7, 0x5b, 0x48, 0x24, 0xc1, 0x1a } };
const GUID c_xamlDiagnosticGuid = { 0x59e7a714, 0x73a4, 0x4147,{ 0xb4, 0x7e, 0x09, 0x57, 0x04, 0x8c, 0x75, 0xc4 } };
const GUID c_xamlGuid = { 0x531a35ab, 0x63ce, 0x4bcf,{ 0xaa, 0x98, 0xf8, 0x8c, 0x7a, 0x89, 0xe4, 0x55 } };
const std::wstring c_sessionName = L"AppAnalysis_ToolTrace";

const GUID c_providerGuids[] = {
    c_xamlGuid,
    c_xamlDiagnosticGuid
};

using namespace Microsoft::Diagnostics::AppAnalysis;

////////////////////////////////////////////////////////////////////////////////
//
AppModule::AppModule()
    : m_fShutdown(FALSE)
    , m_liveTraceStarted(false)
{
}

////////////////////////////////////////////////////////////////////////////////
//
AppModule::~AppModule()
{
    Shutdown();
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
AppModule::Initialize(_In_ const std::wstring& customRuleSet)
{

    HMODULE dll = GetModuleHandle(L"Microsoft.Diagnostics.AppAnalysis.dll");
    if (!dll)
    {
        IFC_RETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // Start writing the Report Xml 
    //
    IFC_RETURN(InitializeXmlReport());

    //
    // Load all rules.
    //
    IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"RuleSets", nullptr));
    IFC_RETURN(GetRuleSetFromModule(dll, RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwRuleSet, &m_ruleSet));
    IFC_RETURN(RegisterRuleSet(m_ruleSet));
    if (!customRuleSet.empty())
    {
        std::wstring customRuleSetDll = customRuleSet + L".dll";
        m_customRuleSetModule.reset(LoadLibrary(customRuleSetDll.c_str()));
        if (m_customRuleSetModule)
        {
            IFC_RETURN(GetRuleSetFromModule(m_customRuleSetModule.get(), customRuleSet.c_str(), &m_customRuleSet));
            IFC_RETURN(RegisterRuleSet(m_customRuleSet));
        }
    }
    IFC_RETURN(m_spReportWriter->WriteEndElement());

    return S_OK;
}

HRESULT
AppModule::RegisterRuleSet(const wrl::ComPtr<wfc::IVectorView<appanalysis::EtwRule*>>& ruleSet)
{

    UINT count = 0;
    wil::unique_hstring className;
    IFC_RETURN(ruleSet->GetRuntimeClassName(&className));

    IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"RuleSet", nullptr));
    IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"ActivationId", nullptr, WindowsGetStringRawBuffer(className.get(), nullptr)));

    IFC_RETURN(ruleSet->get_Size(&count));
    for (UINT i = 0; i < count; ++i)
    {
        wil::unique_hstring displayedRuleTitle;
        wil::unique_hstring displayedRuleId;
        wrl::ComPtr<appanalysis::IEtwRule> runtimeRule;
        IFC_RETURN(ruleSet->GetAt(i, &runtimeRule));

        wrl::ComPtr<appanalysis::IRule> spRule;
        IFC_RETURN(runtimeRule->get_BackingRule(&spRule));

        IFC_RETURN(spRule->get_Id(&displayedRuleId));
        IFC_RETURN(spRule->get_Title(&displayedRuleTitle));

        // advise for callbacks

        auto notificationCallback = wrl::Callback<wf::ITypedEventHandler<appanalysis::IRule*, appanalysis::RuleTriggeredEventArgs*>>(this, &AppModule::Notify);

        EventRegistrationToken token = { 0 };
        IFC_RETURN(spRule->add_Triggered(notificationCallback.Get(), &token));
        m_tokens.push_back(token); // save the token for later, they will be 1-1

        appanalysis::RuleCategories categories = appanalysis::RuleCategories_None;
        IFC_RETURN(spRule->get_Categories(&categories));

        IFC_RETURN(runtimeRule->Start());

        IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"Rule", nullptr));
        IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Id", nullptr, WindowsGetStringRawBuffer(displayedRuleId.get(), nullptr)));
        IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Title", nullptr, WindowsGetStringRawBuffer(displayedRuleTitle.get(), nullptr)));
        IFC_RETURN(m_spReportWriter->WriteEndElement());
    }

    IFC_RETURN(m_spReportWriter->WriteEndElement());

    return S_OK;
}

HRESULT
AppModule::GetRuleSetFromModule(_In_ HMODULE module, _In_z_ const wchar_t* className, _COM_Outptr_ wfc::IVectorView<appanalysis::EtwRule*>** ruleSetInstance)
{
    wrl::ComPtr<IActivationFactory> ruleSetAsFactory;
    pfnGetActivationFactory GetFactory = (pfnGetActivationFactory)(GetProcAddress(module, "DllGetActivationFactory"));
    IFC_RETURN(GetFactory(StringRef(className), &ruleSetAsFactory));
    wrl::ComPtr<IInspectable> ruleSetAsInsp;
    IFC_RETURN(ruleSetAsFactory->ActivateInstance(&ruleSetAsInsp));

    IFC_RETURN(ruleSetAsInsp.CopyTo(ruleSetInstance));
    return S_OK;

}
////////////////////////////////////////////////////////////////////////////////
//
HRESULT
AppModule::Shutdown()
{
    HRESULT returnHr = S_OK;
    if (!InterlockedCompareExchange(&m_fShutdown, TRUE, FALSE))
    {
        HRESULT hr = S_OK;
        std::wstring manifestEtlLog;
        if (m_spTraceSession)
        {
            // We need to query for the trace details before shutting down, otherwise the 
            // trace session is no longer valid
            if (m_liveTraceStarted)
            {
                hr = m_spTraceSession->GetTraceDetails(&manifestEtlLog, nullptr);
                returnHr = FAILED(hr) ? hr : returnHr;
            }

            hr = m_spTraceSession->Shutdown();
            returnHr = FAILED(hr) ? hr : returnHr;
            if (m_manifestShutdownCompleteEvent)
            {
                m_manifestShutdownCompleteEvent.wait();
            }
        }

        std::wstring kernelEtlLog;
        if (m_spKernelTraceSession)
        {
            // We need to query for the trace details before shutting down, otherwise the 
            // trace session is no longer valid
            if (m_liveTraceStarted)
            {
                hr = m_spKernelTraceSession->GetTraceDetails(&kernelEtlLog, nullptr);
                returnHr = FAILED(hr) ? hr : returnHr;
            }

            hr = m_spKernelTraceSession->Shutdown();
            returnHr = FAILED(hr) ? hr : returnHr;

            if (m_kernelShutdownCompleteEvent)
            {
                m_kernelShutdownCompleteEvent.wait();
            }
        }

        // We need to merge the etl files before we cleanup the trace sessions because otherwise they
        // will delete their unmerged files.
        if (!manifestEtlLog.empty() && !kernelEtlLog.empty())
        {
            DWORD status = TraceSession::MergeEtl(manifestEtlLog, kernelEtlLog, L"AppAnalysis_merged.etl");
            // The KernelTraceControl.dll should be on the sytem
            ASSERT(status != ERROR_MOD_NOT_FOUND);
            if ((status != ERROR_SUCCESS) && (status != ERROR_INSUFFICIENT_BUFFER))
            {
                returnHr = HRESULT_FROM_WIN32(status);
            }
        }

        m_spTraceSession.reset();
        m_spKernelTraceSession.reset();

        // close the appanalysis report tag
        m_spReportWriter->WriteEndElement();

        // unadvise all rules
        UINT count = 0;
        IFC_RETURN(m_ruleSet->get_Size(&count));
        for (UINT i = 0; i < count; ++i)
        {
            wrl::ComPtr<appanalysis::IEtwRule> rule;
            if (SUCCEEDED(m_ruleSet->GetAt(i, &rule)))
            {
                wrl::ComPtr<appanalysis::IRule> backingRule;
                if (SUCCEEDED(rule->get_BackingRule(&backingRule)))
                {
                    hr = backingRule->remove_Triggered(m_tokens[i]);
                    returnHr = FAILED(hr) ? hr : returnHr;
                }
                hr = rule->Stop();
                returnHr = FAILED(hr) ? hr : returnHr;
            }
        }

        m_spReportWriter.Reset();
    }
   
    return returnHr;
}

//////////////////////////////////////////////////////////////////////////////
//
HRESULT
AppModule::ProcessETL(
    _In_ const std::wstring& pathToETL
    )
{
    IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"Notifications",nullptr));
    
    ASSERT(!m_spTraceSession);
    IFC_RETURN(TraceSession::CreateInstance(&m_spTraceSession));
    IFC_RETURN(m_spTraceSession->StartEtlTraceSession(pathToETL));
    // Process Trace will block until the session is stopped or
    // all events in the etl were processed.
    IFC_RETURN(m_spTraceSession->ProcessTrace());

    IFC_RETURN(m_spReportWriter->WriteEndElement());

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
HRESULT
AppModule::ProcessLive(_In_ DWORD processId)
{
    IFC_RETURN(TraceSession::CreateInstance(&m_spTraceSession));
    IFC_RETURN(m_spTraceSession->StartLiveTraceSession(c_sessionName, processId, c_sessionGuid));

    IFC_RETURN(TraceSession::CreateInstance(&m_spKernelTraceSession));
    IFC_RETURN(m_spKernelTraceSession->StartLiveTraceSession(KERNEL_LOGGER_NAME, processId, SystemTraceControlGuid, EVENT_TRACE_FLAG_CSWITCH));

    IFC_RETURN(ProcessLiveInternal());
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
HRESULT
AppModule::ProcessLive(_In_ const std::wstring& processName)
{
    IFC_RETURN(TraceSession::CreateInstance(&m_spTraceSession));
    IFC_RETURN(m_spTraceSession->StartLiveTraceSession(c_sessionName, processName, c_sessionGuid));
    
    IFC_RETURN(TraceSession::CreateInstance(&m_spKernelTraceSession));
    IFC_RETURN(m_spKernelTraceSession->StartLiveTraceSession(KERNEL_LOGGER_NAME, processName, SystemTraceControlGuid, EVENT_TRACE_FLAG_CSWITCH));

    IFC_RETURN(ProcessLiveInternal());
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
HRESULT
AppModule::ProcessLiveInternal()
{
    for (auto& guid : c_providerGuids)
    {
        IFC_RETURN(m_spTraceSession->EnableProvider(guid));
    }

    IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"Notifications", nullptr));

    m_liveTraceStarted = true;

    // We need two different trace sessions for the kernel and manifest based providers since
    // the OpenTrace() API can only accept one realtime trace handle. We'll create two separate threads
    // so that we don't block the main application thread and allow the user to press a key to end the session.
    // We don't need to worry about race conditions since we don't fire notifications based off the kernel
    // provider so we'll still get the notifications on a single thread.
    m_kernelShutdownCompleteEvent.create(wil::EventOptions::ManualReset);
    Concurrency::create_task([&] {
        m_spKernelTraceSession->ProcessTrace();
        // once process trace returns, then we can signal the shutdown event. this occurs once ETW
        // stops sending callbacks
        m_kernelShutdownCompleteEvent.SetEvent();
    });

    m_manifestShutdownCompleteEvent.create(wil::EventOptions::ManualReset);
    Concurrency::create_task([&] {
        m_spTraceSession->ProcessTrace();
        // once process trace returns, then we can signal the shutdown event. this occurs once ETW
        // stops sending callbacks
        m_manifestShutdownCompleteEvent.SetEvent();
    });

    IFC_RETURN(m_spReportWriter->WriteEndElement());

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
HRESULT
AppModule::Notify(
    _In_ appanalysis::IRule* rule,
    _In_ appanalysis::IRuleTriggeredEventArgs* pNotification
    )
{
    wrl::ComPtr<appanalysis::IRule> spRule = rule;
    wil::unique_hstring displayRuleId, displayRuleTitle, displayRuleImpact;

    wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> spInfo = pNotification;
    IFC_RETURN(spRule->get_Id(&displayRuleId));
    IFC_RETURN(spRule->get_Title(&displayRuleTitle));
    IFC_RETURN(spRule->get_Impact(&displayRuleImpact));

    IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"Notification", nullptr));
    IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"RuleId", nullptr, WindowsGetStringRawBuffer(displayRuleId.get(), nullptr)));
    IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"RuleTitle", nullptr, WindowsGetStringRawBuffer(displayRuleTitle.get(), nullptr)));
   
    IFC_RETURN(WriteElement(L"Impact", WindowsGetStringRawBuffer(displayRuleImpact.get(), nullptr)))
    IFC_RETURN(WriteLinkInfo(spRule));

    IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"Information", nullptr));

    IFC_RETURN(WriteChild(spRule, spInfo));

    IFC_RETURN(m_spReportWriter->WriteEndElement());

    IFC_RETURN(m_spReportWriter->WriteEndElement());
   
    return S_OK;
}

//HRESULT AppModule::WriteChildren(const wrl::ComPtr<appanalysis::IRule>& rule, const wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs>& parent)
//{
//    wrl::ComPtr<wfc::IVectorView<appanalysis::IRuleTriggeredEventArgs*>> children;
//    IFC_RETURN(parent->get_Children(&children));
//    UINT childCount = 0;
//    IFC_RETURN(children->get_Size(&childCount));
//    if (childCount > 0)
//    {
//        IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"Children", nullptr));
//        for (UINT j = 0; j < childCount; ++j)
//        {
//            wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> childNotification;
//            IFC_RETURN(children->GetAt(j, &childNotification));
//            IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"Child", nullptr));
//            IFC_RETURN(WriteChild(rule, childNotification));
//            IFC_RETURN(m_spReportWriter->WriteEndElement());
//        }
//        IFC_RETURN(m_spReportWriter->WriteEndElement());
//    }
//
//    return S_OK;
//}

HRESULT AppModule::WriteChild(_In_ const wrl::ComPtr<appanalysis::IRule>& rule, _In_ const wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs>& notificationInfo)
{
    // Write out the description
    wrl::ComPtr<appanalysis::IResourceStringView> description;
    IFC_RETURN(notificationInfo->get_Description(&description));
    UINT descriptionId = 0;
    IFC_RETURN(description->get_Identifier(&descriptionId));
    if (descriptionId != 0)
    {
        IFC_RETURN(WriteResourceString(rule, L"Description", description.Get()));
    }
    
    // Write out solution
    wrl::ComPtr<appanalysis::IResourceStringView> solution;
    IFC_RETURN(notificationInfo->get_Solution(&solution));
    UINT solutionId = 0;
    IFC_RETURN(description->get_Identifier(&solutionId));
    if (solutionId != 0)
    {
        IFC_RETURN(WriteResourceString(rule, L"Solution", solution.Get()));
    }

    // Write out Source Info
    IFC_RETURN(WriteSourceInfo(notificationInfo));

    //Write out measurement
    IFC_RETURN(WriteMeasurement(notificationInfo));

    //Write out timeline data, only if supplied
    IFC_RETURN(WriteRangeInfo(notificationInfo));

    // write children, if none this will return immediately
    //IFC_RETURN(WriteChildren(rule, notificationInfo));
    
    return S_OK;
}

HRESULT AppModule::WriteSourceInfo(_In_ const wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs>& info)
{
    wil::unique_sourceinfo sourceInfo;
    wchar_t str[64];
    IFC_RETURN(info->get_FileName(&sourceInfo.FileName));
    IFC_RETURN(info->get_ColumnNumber(&sourceInfo.ColumnNumber));
    IFC_RETURN(info->get_LineNumber(&sourceInfo.LineNumber));
    IFC_RETURN(info->get_FileHash(&sourceInfo.FileHash));

    if (!WindowsIsStringEmpty(sourceInfo.FileName))
    {
        IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"File", nullptr));
        IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Name", nullptr, WindowsGetStringRawBuffer(sourceInfo.FileName, nullptr)));
        IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Hash", nullptr, WindowsGetStringRawBuffer(sourceInfo.FileHash, nullptr)));

        if (sourceInfo.LineNumber > 0)
        {
            ASSERT(sourceInfo.ColumnNumber > 0);

            if (_i64tow_s(sourceInfo.LineNumber, str, _countof(str), 10) != 0)
            {
                return E_UNEXPECTED;
            }
            IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Line", nullptr, str));
        }

        if (sourceInfo.ColumnNumber > 0)
        {
            if (_i64tow_s(sourceInfo.ColumnNumber, str, _countof(str), 10) != 0)
            {
                return E_UNEXPECTED;
            }
            IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Column", nullptr, str));
        }


        IFC_RETURN(m_spReportWriter->WriteEndElement());
    }

    return S_OK;
}

HRESULT AppModule::WriteImpact(_In_ const wrl::ComPtr<appanalysis::IRule>& rule)
{
    wil::unique_hstring displayString;
    IFC_RETURN(rule->get_Impact(&displayString));
    IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Impact", nullptr, WindowsGetStringRawBuffer(displayString.get(), nullptr)));

    return S_OK;
}

HRESULT AppModule::WriteLinkInfo(_In_ const wrl::ComPtr<appanalysis::IRule>& rule)
{
    wil::unique_hstring linkTitle;
    wil::unique_hstring linkUri;
    IFC_RETURN(rule->get_LinkTitle(&linkTitle));
    IFC_RETURN(rule->get_LinkUri(&linkUri));

    if (!WindowsIsStringEmpty(linkTitle.get()) && WindowsIsStringEmpty(linkUri.get()))
    {
        IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"References", nullptr));
        IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"Link", nullptr));
        IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Title", nullptr, WindowsGetStringRawBuffer(linkTitle.get(), nullptr)));
        IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"URL", nullptr, WindowsGetStringRawBuffer(linkUri.get(), nullptr)));
        IFC_RETURN(m_spReportWriter->WriteEndElement());
        IFC_RETURN(m_spReportWriter->WriteEndElement());
    }

    return S_OK;
}

HRESULT AppModule::WriteRangeInfo(_In_ const wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs>& info)
{
    appanalysis::TimelineInfo rangeInfo = { 0 };
    wchar_t str[64];
    IFC_RETURN(info->get_TimelineStart(&rangeInfo.Start));
    IFC_RETURN(info->get_TimelineStop(&rangeInfo.Stop));
    if (rangeInfo.Start != 0 && rangeInfo.Stop!= 0)
    {
        IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"Timeline", nullptr));
        if (_i64tow_s(rangeInfo.Start, str, _countof(str), 10) != 0)
        {
            return E_UNEXPECTED;
        }

        IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Start", nullptr, str));
        if (_i64tow_s(rangeInfo.Stop, str, _countof(str), 10) != 0)
        {
            return E_UNEXPECTED;
        }

        IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Stop", nullptr, str));
        IFC_RETURN(m_spReportWriter->WriteEndElement());
    }

    return S_OK;
}

HRESULT AppModule::WriteMeasurement(_In_ const wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs>& info)
{
    appanalysis::Measurement measurement = { 0 };
    IFC_RETURN(info->get_MeasurementUnit(&measurement.Unit));
    IFC_RETURN(info->get_MeasurementValue(&measurement.Value));
    if (measurement.Value > 0.0)
    {
        IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"Measurement", nullptr));

        IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Value", nullptr, std::to_wstring(measurement.Value).c_str()));
        IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"Unit", nullptr, ConvertMeasurementUnit(measurement.Unit)));
        IFC_RETURN(m_spReportWriter->WriteEndElement());
    }

    return S_OK;
}

HRESULT AppModule::WriteAttribute(_In_ LPCWSTR attributeName, _In_ LPCWSTR attributeValue)
{
    IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, attributeName, nullptr, attributeValue));
    return S_OK;
}

HRESULT AppModule::WriteElement(_In_ LPCWSTR elementName, _In_ LPCWSTR displayString)
{
    IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, elementName, nullptr));
    IFC_RETURN(m_spReportWriter->WriteString(displayString));
    IFC_RETURN(m_spReportWriter->WriteEndElement());
    return S_OK;
}

HRESULT AppModule::WriteResourceString(_In_ const wrl::ComPtr<appanalysis::IRule>& rule, _In_ LPCWSTR attributeName, _In_  appanalysis::IResourceStringView* string)
{
    wil::unique_hstring formattedString;
    IFC_RETURN(rule->FormatString(string, &formattedString));
    IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, attributeName, nullptr, WindowsGetStringRawBuffer(formattedString.get(), nullptr)));
    return S_OK;
}

PCWSTR AppModule::ConvertMeasurementUnit(_In_ appanalysis::MeasurementUnit unit)
{
    switch (unit)
    {
    case appanalysis::MeasurementUnit_Kilobytes:
        return L"KB";
    case appanalysis::MeasurementUnit_Milliseconds:
        return L"ms";
    case appanalysis::MeasurementUnit_Elements:
        return L"Elements";
    case appanalysis::MeasurementUnit_Percentage:
        return L"%";
    default:
        ASSERT(false);
        return L"unknown";
    }
}

HRESULT AppModule::InitializeXmlReport()
{
    wrl::ComPtr<IStream> spReportFileStream;
    SYSTEMTIME utcDateTime = { 0 };
    WCHAR szUTCDateTime[64] = { 0 };
    (void)GetSystemTime(&utcDateTime);
    IFC_RETURN(SHCreateStreamOnFile(L"AppAnalysisReport.xml", STGM_CREATE | STGM_WRITE, &spReportFileStream));
    IFC_RETURN(CreateXmlWriter(__uuidof(IXmlWriter), (void**)&m_spReportWriter, nullptr));
    IFC_RETURN(m_spReportWriter->SetOutput(spReportFileStream.Get()));
    IFC_RETURN(m_spReportWriter->SetProperty(XmlWriterProperty_Indent, TRUE));
    IFC_RETURN(m_spReportWriter->WriteStartDocument(XmlStandalone_Omit));
    IFC_RETURN(m_spReportWriter->WriteDocType(L"AppAnalysisReport", nullptr, nullptr, nullptr));
    IFC_RETURN(m_spReportWriter->WriteProcessingInstruction(L"xml-stylesheet", L"href=\"AppAnalysisReportStylesheet.xslt\" type=\"text/xsl\""));
    IFC_RETURN(m_spReportWriter->WriteStartElement(nullptr, L"AppAnalysisReport", nullptr));
    IFC_RETURN(StringCchPrintf(szUTCDateTime, _countof(szUTCDateTime),
        L"%04d-%02d-%02dT%02d:%02d:%02dZ",
        utcDateTime.wYear, utcDateTime.wMonth, utcDateTime.wDay,
        utcDateTime.wHour, utcDateTime.wMinute, utcDateTime.wSecond));
    IFC_RETURN(m_spReportWriter->WriteAttributeString(nullptr, L"DateTime", nullptr, szUTCDateTime));

    return S_OK;
}