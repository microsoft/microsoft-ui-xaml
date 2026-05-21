// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "BackingEtwRule.h"
#include "Microsoft.UI.Xaml.h"
#include <string>
#include <map>
#include "helpers.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
// The CollapsedElementsRule is a rule that detects if developers are defaulting to their
// elements being collapsed at load time. This is detrimental to perf because we create the
// elements only to not show them. The recommendation is for developers to use x:DeferLoadStrategy
// to defer creation of elements.
class CollapsedElementsRule
    : public EtwRuleImpl<CollapsedElementsRule, appanalysis::RuleCategories_Performance>
{
public:

    CollapsedElementsRule()
        : m_lastFrameEvent(0)
    {
    }

    virtual ~CollapsedElementsRule()
    {
        Reset();
    }

    HRESULT CollapsedElementsRule::ProcessFrameEndEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {

        LONGLONG currentFrameEvent = 0;
        IFC_RETURN(pEvent->get_Timestamp(&currentFrameEvent));

        auto resetOnExit = wil::scope_exit([&] {
            Reset();
        });

        // If we have source info for the elements, that's awesome and we can use that to our
        // advantage and give the developer (or interested party) detailed information about the
        // problems.
        if (m_collapsedElementSourceInfo.size() > 0)
        {
            wrl::ComPtr<appanalysis::ISourceInfoRuleService> sourceInfoService;
            IFC_RETURN(GetRuleService<appanalysis::ISourceInfoRuleService>(&sourceInfoService));

            // iterate through all different source infos we found and fire a notification for each one.
            for (SourceInfoCache::iterator sourceIter = m_collapsedElementSourceInfo.begin();
                sourceIter != m_collapsedElementSourceInfo.end();
                // Once we have fired the notification, we can remove the item from our cache
                sourceIter = m_collapsedElementSourceInfo.erase(sourceIter))
            {
                RuleTriggeredEventArgs::CreateParams elementParams;
                elementParams.measurement.Unit = appanalysis::MeasurementUnit_Elements;
                elementParams.measurement.Value = static_cast<DOUBLE>(sourceIter->second.first);
                elementParams.sourceInfo = sourceIter->first;
                IFC_RETURN(sourceInfoService->GetVisualTreeId(sourceIter->second.second, &elementParams.elementId));

                elementParams.timeline.Start = m_lastFrameEvent;
                elementParams.timeline.Start = currentFrameEvent;

                IFC_RETURN(CreateDescriptionStringForElement(sourceIter->second.second, sourceInfoService, &elementParams.description));

                IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&elementParams.solution, COLLAPSED_ELEMENTS_SOLUTION));

                wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> elementInfo;
                IFC_RETURN(CreateRuleTriggeredEventArgs(elementParams, &elementInfo));

                FireNotification(elementInfo.Get());
            }
        }

        m_lastFrameEvent = currentFrameEvent;

        return S_OK;
    }

    HRESULT CollapsedElementsRule::ProcessPropertyChangedEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        wil::shared_hstring propertyName;
        IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"PropertyName"), &propertyName));

        if (AppAnalysisHelpers::CompareStrings(propertyName.get(), StringRef(L"Visibility")) == 0)
        {
            UINT64 propertyValue = 0;
            IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ValueInt"), &propertyValue));
            if (propertyValue == xaml::Visibility_Collapsed)
            {
                IFC_RETURN(ProcessInvisibleElementInternal(pEvent));
            }
        }

        return S_OK;
    }

    HRESULT CollapsedElementsRule::ProcessElementCreatedEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        InstanceHandle elementId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &elementId));

        m_elementsCreatedInCurrentFrame.emplace(elementId, false);

        return S_OK;
    }


    HRESULT Reset()
    {
        m_elementsCreatedInCurrentFrame.clear();
        m_collapsedElementSourceInfo.clear();
        return S_OK;
    }

private:

    HRESULT CreateDescriptionStringForElement(_In_ UINT64 elementId, const wrl::ComPtr<appanalysis::ISourceInfoRuleService>& sourceInfoService, _COM_Outptr_ ResourceString** description)
    {
        wrl::ComPtr<ResourceString> descriptionString;

        wil::shared_hstring elementName;
        HRESULT hr = sourceInfoService->GetElementName(elementId, &elementName);
        IFC_RETURN(hr);

        if (hr == S_FALSE)
        {
            IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&descriptionString, COLLAPSED_ELEMENTS_DESCRIPTION_NO_NAME));
        }
        else
        {
            IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&descriptionString, COLLAPSED_ELEMENTS_DESCRIPTION));
            IFC_RETURN(descriptionString->Append(elementName.get()));
        }

        wil::shared_hstring elementTypeName;
        IFC_RETURN(sourceInfoService->GetElementTypeName(elementId, &elementTypeName));
        IFC_RETURN(descriptionString->Append(elementTypeName.get()));

        *description = descriptionString.Detach();

        return S_OK;
    }

    HRESULT CollapsedElementsRule::ProcessInvisibleElementInternal(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        InstanceHandle elementId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &elementId));

        // If the element is being collapsed, check to see that it was created in this frame.
        // If it was, we can assume that this element was created with Collapsed and we should flag it.
        auto allElementsIter = m_elementsCreatedInCurrentFrame.find(elementId);

        // We check for false becaue the value in the hash map represents whether or not
        // we have already set the visiblity property for this element. We'll only ever
        // add this element to the collapsed cache if it was created in this frame
        if (allElementsIter != m_elementsCreatedInCurrentFrame.end() && allElementsIter->second == false)
        {
            wil::shared_sourceinfo info;
            wrl::ComPtr<appanalysis::ISourceInfoRuleService> sourceInfoService;
            IFC_RETURN(GetRuleService<appanalysis::ISourceInfoRuleService>(&sourceInfoService));
            IFC_RETURN(sourceInfoService->GetSourceInfo(elementId, &info));

            // check to see if the source info comes from generic.xaml
            auto iter = m_collapsedElementSourceInfo.find(info);
            if (info.FileName && !AppAnalysisHelpers::IsGenericXaml(info) && iter == m_collapsedElementSourceInfo.end())
            {
                // pass ownership to our m_collapsedElementSourceInfo map. We store non-RAII SourceInfo
                // structs in here because otherwise we'd have to incur an extra allocation when we
                // create the RuleTriggeredEventArgs object
                IFCSTL_RETURN(m_collapsedElementSourceInfo.emplace(info, std::make_pair(1u, elementId)));
            }
            else if (iter != m_collapsedElementSourceInfo.end())
            {
                iter->second.first++;
            }

            allElementsIter->second = true;
        }

        return S_OK;
    }

    INT64 m_lastFrameEvent;

    // this map correlates whether or not the element has gotten the property changed
    // event for the Visibility property. if this is false and we get a propertychanged
    // even which says Visiblity was changed to Collapsed, we place the element in the
    // m_collapsedElements cache.
    std::map<InstanceHandle, bool> m_elementsCreatedInCurrentFrame;
    using SourceInfoCache = std::map<wil::shared_sourceinfo, std::pair<unsigned int, InstanceHandle>, AppAnalysisHelpers::SourceInfoComparer>;
    SourceInfoCache m_collapsedElementSourceInfo;
};

BEGIN_PROVIDERS(CollapsedElementsRule)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(CollapsedElementsRule)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, FrameEnd_value, EventVersion_0, &CollapsedElementsRule::ProcessFrameEndEvent)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, ElementCreatedInfo_value, EventVersion_0, &CollapsedElementsRule::ProcessElementCreatedEvent)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, PropertyChangedInfo_value, EventVersion_1, &CollapsedElementsRule::ProcessPropertyChangedEvent)
END_CALLBACKS()

////////////////////////////////////////////////////////////////////////////////
//
HRESULT CollapsedElementsRule_CreateInstance(
    _COM_Outptr_ appanalysis::IEtwRule** ppInstance
    )
{
    wrl::ComPtr<CollapsedElementsRule> rule;
    IFC_RETURN(CollapsedElementsRule::CreateInstance(
        COLLAPSED_ELEMENTS_ID, COLLAPSED_ELEMENTS_TITLE, COLLAPSED_ELEMENTS_IMPACT,
        COLLAPSED_ELEMENTS_LINK_TITLE, COLLAPSED_ELEMENTS_LINK_URL,
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterEvents(&watcher));

    IFC_RETURN(wrl::MakeAndInitialize<EtwRule>(ppInstance, rule.Get(), watcher.Get()));
    return S_OK;
}
} } }
