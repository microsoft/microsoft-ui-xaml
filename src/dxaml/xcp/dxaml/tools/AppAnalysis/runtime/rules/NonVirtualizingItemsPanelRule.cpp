// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "BackingEtwRule.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
// The NonVirtualizingItemsPanelRule is a rule that detects if developers are accidentally devirtualizing
// their ListView. This can be done by one of two ways: retemplating with a non-modern panel or 
// putting a their listview insdide a scroll viewer, which has infinite layout.

class NonVirtualizingItemsPanelRule
    : public EtwRuleImpl<NonVirtualizingItemsPanelRule, appanalysis::RuleCategories_Performance>
{

public:

    NonVirtualizingItemsPanelRule()
    {
    }

    virtual ~NonVirtualizingItemsPanelRule()
    {
    }

    HRESULT ProcessVirtualizationIsEnabledByModernPanelInfoEvent(
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

            wil::shared_hstring elementName;
            IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"Name"), &elementName));

            wil::shared_hstring propertyType;
            IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"PropertyType"), &propertyType));

            wil::shared_hstring className;
            IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"ClassType"), &className));

            // do a case sensitive comparison of the name. The ETW framework replaces nullptr with the string L"NULL"
            if (AppAnalysisHelpers::CompareStrings(elementName.get(), StringRef(L"NULL")) == 0)
            {
                IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, NON_VIRTUALIZED_ITEMS_PANEL_DESCRIPTION_NO_NAME));
                IFC_RETURN(params.description->Append(className.get()));
                IFC_RETURN(params.description->Append(propertyType.get()));
            }
            else
            {
                IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, NON_VIRTUALIZED_ITEMS_PANEL_DESCRIPTION));
                IFC_RETURN(params.description->Append(elementName.get()));
                IFC_RETURN(params.description->Append(className.get()));
                IFC_RETURN(params.description->Append(propertyType.get()));
            }

            std::vector<wil::shared_hstring> solutionArgs;
            IFC_RETURN(sourceInfoService->GetSourceInfo(elementId, &params.sourceInfo));
            IFC_RETURN(sourceInfoService->GetVisualTreeId(elementId, &params.elementId));

            IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.solution, NON_VIRTUALIZED_ITEMS_PANEL_SOLUTION));
            IFC_RETURN(params.description->Append(className.get()));

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

BEGIN_PROVIDERS(NonVirtualizingItemsPanelRule)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
    // Even though the NonVirtualizingItemsPanelRule doesn't depend directly on the Diagnostic provider, it depends 
    // on SourceInfoRuleService which does. If this was the only rule enabled we want to make sure that provider
    // still gets enabled
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(NonVirtualizingItemsPanelRule)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, VirtualizationIsEnabledByModernPanelUsageInfo_value, EventVersion_1, &NonVirtualizingItemsPanelRule::ProcessVirtualizationIsEnabledByModernPanelInfoEvent)
END_CALLBACKS()

////////////////////////////////////////////////////////////////////////////////
//
HRESULT NonVirtualizingItemsPanelRule_CreateInstance(
    _COM_Outptr_ appanalysis::IEtwRule** ppInstance
    )
{
    wrl::ComPtr<NonVirtualizingItemsPanelRule> rule;
    IFC_RETURN(NonVirtualizingItemsPanelRule::CreateInstance(
        NON_VIRTUALIZED_ITEMS_PANEL_ID, NON_VIRTUALIZED_ITEMS_PANEL_TITLE, NON_VIRTUALIZED_ITEMS_PANEL_IMPACT,
        NON_VIRTUALIZED_ITEMS_PANEL_LINK_TITLE, NON_VIRTUALIZED_ITEMS_PANEL_LINK_URL,
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterEvents(&watcher));

    IFC_RETURN(wrl::MakeAndInitialize<EtwRule>(ppInstance, rule.Get(), watcher.Get()));

    return S_OK;
}
} } }
