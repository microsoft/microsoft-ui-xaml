// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "BackingEtwRule.h"
#include "helpers.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

class NoAutomationNameRule
    : public EtwRuleImpl<NoAutomationNameRule, appanalysis::RuleCategories_Accessibility>
{

public:

    NoAutomationNameRule()
    {
    }
    
    virtual ~NoAutomationNameRule()
    {
    }

    HRESULT NoAutomationNameRule::ProcessElementAccessibilityEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        INT64 timeStamp = 0;
        IFC_RETURN(pEvent->get_Timestamp(&timeStamp));
        wil::unique_hstring name;
        IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"Name"), &name));
        InstanceHandle elementId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &elementId));

        // if the element name is empty and this element hasn't already been flagged then we'll do so.
        if (!!WindowsIsStringEmpty(name.get()) && m_elementsFlagged.count(elementId) == 0)
        {
            RuleTriggeredEventArgs::CreateParams params;

            wrl::ComPtr<appanalysis::ISourceInfoRuleService> sourceInfoService;
            IFC_RETURN(GetRuleService(&sourceInfoService));
            IFC_RETURN(sourceInfoService->GetSourceInfo(elementId, &params.sourceInfo));
            IFC_RETURN(sourceInfoService->GetVisualTreeId(elementId, &params.elementId));

            params.measurement.Unit = appanalysis::MeasurementUnit_Elements;
            params.measurement.Value = 1.0;

            params.timeline.Start = timeStamp;
            params.timeline.Stop = timeStamp;

            wil::shared_hstring elementName;
            HRESULT hr = sourceInfoService->GetElementName(elementId, &elementName);
            IFC_RETURN(hr);

            if (hr == S_FALSE)
            {
                IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, ACC_NO_NAME_DESCRIPTION_NO_NAME));
            }
            else
            {
                IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, ACC_NO_NAME_DESCRIPTION));
                IFC_RETURN(params.description->Append(elementName.get()));
            }
            
            wil::shared_hstring elementType;
            IFC_RETURN(sourceInfoService->GetElementTypeName(elementId, &elementType));
            IFC_RETURN(params.description->Append(elementType.get()));

            IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.solution, ACC_NO_NAME_SOLUTION));

            wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> notificationInfo;
            IFC_RETURN(CreateRuleTriggeredEventArgs(params, &notificationInfo));
            
            FireNotification(notificationInfo.Get());

            m_elementsFlagged.emplace(elementId); // cache this so we don't flag this again
        }
        return S_OK;
    }

private:
    std::unordered_set<InstanceHandle> m_elementsFlagged;

};

BEGIN_PROVIDERS(NoAutomationNameRule)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(NoAutomationNameRule)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, ElementAccessibilityInfo_value, EventVersion_0,  &NoAutomationNameRule::ProcessElementAccessibilityEvent)
END_CALLBACKS()

////////////////////////////////////////////////////////////////////////////////
//
HRESULT NoAutomationNameRule_CreateInstance(
    _COM_Outptr_ appanalysis::IEtwRule** ppInstance
    )
{
    wrl::ComPtr<NoAutomationNameRule> rule;
    IFC_RETURN(NoAutomationNameRule::CreateInstance(
        ACC_NO_NAME_ID, ACC_NO_NAME_TITLE, ACC_NO_NAME_IMPACT,
        ACC_NO_NAME_LINK_TITLE, ACC_NO_NAME_LINK_URL,
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterEvents(&watcher));

    IFC_RETURN(wrl::MakeAndInitialize<EtwRule>(ppInstance, rule.Get(), watcher.Get()));

    return S_OK;
}
} } }
