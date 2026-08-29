// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "BackingEtwRule.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
// The ListViewDevirtualizedRule is a rule that detects if developers are accidentally devirtualizing
// their ListView. This can be done by one of two ways: retemplating with a non-modern panel or 
// putting a their listview insdide a scroll viewer, which has infinite layout.

class ListViewDevirtualizedRule
    : public EtwRuleImpl<ListViewDevirtualizedRule, appanalysis::RuleCategories_Performance>
{

public:

    ListViewDevirtualizedRule()
    {
    }

    virtual ~ListViewDevirtualizedRule()
    {
    }

    HRESULT ListViewDevirtualizedRule::ProcessVirtualizationIsEnabledByLayoutInfoEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        boolean isEnabled = TRUE;

        IFC_RETURN(pEvent->GetBooleanProperty(StringRef(L"IsEnabled"), &isEnabled));

        InstanceHandle elementId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &elementId));

        if (!isEnabled && m_elementsTriggered.find(elementId) == m_elementsTriggered.end())
        {
            wrl::ComPtr<appanalysis::ISourceInfoRuleService> sourceInfoService;
            IFC_RETURN(GetRuleService(&sourceInfoService));

            RuleTriggeredEventArgs::CreateParams params;
            wil::shared_hstring className;
            
            IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"PropertyType"), &className));
            //duplicate the class name for the solution string
            wil::shared_hstring elementName;
            IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"Name"), &elementName));

            // do a case sensitive comparison of the name. The ETW framework replaces nullptr with the string L"NULL"
            if (AppAnalysisHelpers::CompareStrings(elementName.get(), StringRef(L"NULL")) == 0)
            {
                IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, LV_DEVIRTUALIZED_LAYOUT_DESCRIPTION_NO_NAME));
                IFC_RETURN(params.description->Append(className.get()));
            }
            else
            {
                IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, LV_DEVIRTUALIZED_LAYOUT_DESCRIPTION));
                IFC_RETURN(params.description->Append(elementName.get()));
                IFC_RETURN(params.description->Append(className.get()));
            }

            IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.solution, LV_DEVIRTUALIZED_LAYOUT_SOLUTION));
            IFC_RETURN(params.solution->Append(className.get()));

            IFC_RETURN(sourceInfoService->GetSourceInfo(elementId, &params.sourceInfo));
            IFC_RETURN(sourceInfoService->GetVisualTreeId(elementId, &params.elementId));

            wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> devirtualizedListInfo;

            IFC_RETURN(CreateRuleTriggeredEventArgs(params, &devirtualizedListInfo));
            FireNotification(devirtualizedListInfo.Get());
            m_elementsTriggered.emplace(elementId);
        }

        return S_OK;
    }

private:
    std::unordered_set<InstanceHandle> m_elementsTriggered;
};

BEGIN_PROVIDERS(ListViewDevirtualizedRule)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
    // Even though the ListViewDevirtualizedRule doesn't depend directly on the Diagnostic provider, it depends 
    // on SourceInfoRuleService which does. If this was the only rule enabled we want to make sure that provider
    // still gets enabled
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(ListViewDevirtualizedRule)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, VirtualizationIsEnabledByLayoutInfo_value, EventVersion_1, &ListViewDevirtualizedRule::ProcessVirtualizationIsEnabledByLayoutInfoEvent)
END_CALLBACKS()

////////////////////////////////////////////////////////////////////////////////
//
HRESULT ListViewDevirtualizedRule_CreateInstance(
    _COM_Outptr_ appanalysis::IEtwRule** ppInstance
    )
{
    wrl::ComPtr<ListViewDevirtualizedRule> rule;
    IFC_RETURN(ListViewDevirtualizedRule::CreateInstance(
        LV_DEVIRTUALIZED_ID, LV_DEVIRTUALIZED_TITLE, LV_DEVIRTUALIZED_IMPACT,
        LV_DEVIRTUALIZED_LINK_TITLE, LV_DEVIRTUALIZED_LINK_URL,
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterEvents(&watcher));

    IFC_RETURN(wrl::MakeAndInitialize<EtwRule>(ppInstance, rule.Get(), watcher.Get()));

    return S_OK;
}
} } }
