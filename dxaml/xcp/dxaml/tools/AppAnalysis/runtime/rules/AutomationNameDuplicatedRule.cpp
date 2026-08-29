// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "BackingEtwRule.h"
#include <map>

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

// This rule detects when two elements of the same control type, with the same parent, have 
// the same Automation Name.
class AutomationNameDuplicatedRule
    : public EtwRuleImpl<AutomationNameDuplicatedRule, appanalysis::RuleCategories_Accessibility>
{

public:

    AutomationNameDuplicatedRule()
    {
    }
    
    virtual ~AutomationNameDuplicatedRule()
    {
    }

    HRESULT AutomationNameDuplicatedRule::ProcessElementAccessibilityEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        LONGLONG timeStamp = 0;
        IFC_RETURN(pEvent->get_Timestamp(&timeStamp));

        ElementAndParent idInfo;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &idInfo.element));

        // if we've already flagged this element, then ignore it
        if (m_elementsFlagged.count(idInfo.element) > 0)
        {
            return S_OK;
        }

        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ParentId"), &idInfo.parent));
        
        AccessibilityElement nameType;
        IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"Name"), &nameType.name));
        IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"Type"), &nameType.controlType));

        // if this element doesn't have a name, then ignore it. that will be flagged by 
        // a different rule
        if (!!WindowsIsStringEmpty(nameType.name.get()))
        {
            return S_OK;
        }

        // if we don't have an element with this name then that's great, but we
        // still want to cache this for later lookup
        auto iter = m_cache.find(nameType);
        if (iter == m_cache.end())
        {
            IFCSTL_RETURN(m_cache.emplace(std::move(nameType), std::move(idInfo)));
            return S_OK;
        }

        // we have at least one match. iterate through all the possible matches and find
        // offenders
        auto possibleViolations = m_cache.equal_range(nameType);
        
        for (auto element = possibleViolations.first; element != possibleViolations.second; ++element)
        {
            // skip this element if self or if have different parents
            if (element->second.element == idInfo.element || element->second.parent != idInfo.parent)
            {
                continue;
            }

            RuleTriggeredEventArgs::CreateParams params;
            params.measurement.Unit = appanalysis::MeasurementUnit_Elements;
            params.measurement.Value = 1.0;

            params.timeline.Start = timeStamp;
            params.timeline.Stop = timeStamp;

            IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, ACC_NAME_DUPLICATE_DESCRIPTION));
            IFC_RETURN(params.description->Append(nameType.name.get()));
            IFC_RETURN(params.description->Append(nameType.controlType.get()));

            wrl::ComPtr<appanalysis::ISourceInfoRuleService> sourceInfoService;
            IFC_RETURN(GetRuleService(&sourceInfoService));
            IFC_RETURN(sourceInfoService->GetSourceInfo(idInfo.element, &params.sourceInfo));
            IFC_RETURN(sourceInfoService->GetVisualTreeId(idInfo.element, &params.elementId));

            IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.solution, ACC_NAME_DUPLICATE_SOLUTION));
            wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> notificationInfo;
            IFC_RETURN(CreateRuleTriggeredEventArgs(params, &notificationInfo));

            FireNotification(notificationInfo.Get());

            m_elementsFlagged.emplace(idInfo.element);
        }

        return S_OK;
    }

private:

    struct ElementAndParent
    {
        ElementAndParent()
            : element(0)
            , parent(0)
        {
        }

        ElementAndParent(InstanceHandle elem, InstanceHandle par)
        {
            element = elem;
            parent = par;
        }

        InstanceHandle element;
        InstanceHandle parent;
    };

    struct AccessibilityElement
    {
        AccessibilityElement()
        {
        }

        AccessibilityElement(AccessibilityElement&& rhs) noexcept
        {
            name = std::move(rhs.name);
            controlType = std::move(rhs.controlType);
        }

        bool operator<(const AccessibilityElement& right) const
        {
            int nameComparison = AppAnalysisHelpers::CompareStrings(name.get(), right.name.get());

            // if names are the same, compare the control types
            if (nameComparison == 0)
            {
                int controlTypeComparison = AppAnalysisHelpers::CompareStrings(controlType.get(), right.controlType.get());
                return controlTypeComparison < 0;
            }
  
            // otherwise if names differ, we'll return the comparison (greater than or less than)
            return nameComparison < 0;
        }

        wil::shared_hstring name;
        wil::shared_hstring controlType;
    private:
        AccessibilityElement(const AccessibilityElement& rhs) = delete;
        AccessibilityElement& operator=(const AccessibilityElement& rhs) = delete;
    };

    std::multimap<AccessibilityElement, ElementAndParent> m_cache;
    std::unordered_set<ULONGLONG> m_elementsFlagged;
};

BEGIN_PROVIDERS(AutomationNameDuplicatedRule)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(AutomationNameDuplicatedRule)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, ElementAccessibilityInfo_value, EventVersion_0, &AutomationNameDuplicatedRule::ProcessElementAccessibilityEvent)
END_CALLBACKS()

////////////////////////////////////////////////////////////////////////////////
//
HRESULT AutomationNameDuplicatedRule_CreateInstance(
    _COM_Outptr_ appanalysis::IEtwRule** ppInstance
    )
{
    wrl::ComPtr<AutomationNameDuplicatedRule> rule;
    IFC_RETURN(AutomationNameDuplicatedRule::CreateInstance(
        ACC_NAME_DUPLICATE_ID, ACC_NAME_DUPLICATE_TITLE, ACC_NAME_DUPLICATE_IMPACT,
        ACC_NAME_DUPLICATE_LINK_TITLE, ACC_NAME_DUPLICATE_LINK_URL,
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterEvents(&watcher));

    IFC_RETURN(wrl::MakeAndInitialize<EtwRule>(ppInstance, rule.Get(), watcher.Get()));

    return S_OK;
}

} } }
