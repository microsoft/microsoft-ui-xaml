// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "BackingEtwRule.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis { 
// The ResourceUsingXNameRule is a rule that detects if developers are assigning resources
// an x:Name. This causes the resource to be created during Parse and stored in a backing field.
// Guidance is to use a property that will lazily retrieve the resource from the dictionary so 
// it is only created when needed.

class ResourceUsingXNameRule
    : public EtwRuleImpl<ResourceUsingXNameRule, appanalysis::RuleCategories_Performance>
{

public:

    ResourceUsingXNameRule()
    {
    }

    virtual ~ResourceUsingXNameRule()
    {
    }

    HRESULT ProcessResourceUsingXNameEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        InstanceHandle elementId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &elementId));
        
        RuleTriggeredEventArgs::CreateParams params;
        
        wrl::ComPtr<appanalysis::ISourceInfoRuleService> sourceInfoService;
        IFC_RETURN(GetRuleService(&sourceInfoService));
        IFC_RETURN(sourceInfoService->GetSourceInfo(elementId, &params.sourceInfo));

        if (AppAnalysisHelpers::IsGenericXaml(params.sourceInfo))
        {
            return S_OK;
        }

        IFC_RETURN(sourceInfoService->GetVisualTreeId(elementId, &params.elementId));

        wil::shared_hstring className;
        IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"Name"), &className));
        
        wil::shared_hstring classType;
        IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"Type"), &classType));
        
        IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.solution, RESOURCE_USING_XNAME_SOLUTION));
        IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, RESOURCE_USING_XNAME_DESCRIPTION));

        IFC_RETURN(params.description->Append(className.get()));
        IFC_RETURN(params.description->Append(classType.get()));

        IFC_RETURN(pEvent->get_Timestamp(&params.timeline.Start));
        params.timeline.Stop = params.timeline.Start;

        params.measurement.Unit = appanalysis::MeasurementUnit_Elements;
        params.measurement.Value = 1.0;

        wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> devirtualizedListInfo;

        IFC_RETURN(CreateRuleTriggeredEventArgs(params, &devirtualizedListInfo));
        FireNotification(devirtualizedListInfo.Get());
        
        return S_OK;
    }
};

BEGIN_PROVIDERS(ResourceUsingXNameRule)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(ResourceUsingXNameRule)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, ResourceUsingXNameInfo_value, EventVersion_0, &ResourceUsingXNameRule::ProcessResourceUsingXNameEvent)
END_CALLBACKS()

////////////////////////////////////////////////////////////////////////////////
//
HRESULT ResourceUsingXNameRule_CreateInstance(
    _COM_Outptr_ appanalysis::IEtwRule** ppInstance
    )
{
    wrl::ComPtr<ResourceUsingXNameRule> rule;
    IFC_RETURN(ResourceUsingXNameRule::CreateInstance(
        RESOURCE_USING_XNAME_ID, RESOURCE_USING_XNAME_TITLE, RESOURCE_USING_XNAME_IMPACT,
        RESOURCE_USING_XNAME_LINK_TITLE, RESOURCE_USING_XNAME_LINK_URL,
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterEvents(&watcher));

    IFC_RETURN(wrl::MakeAndInitialize<EtwRule>(ppInstance, rule.Get(), watcher.Get()));

    return S_OK;
}

} } }
