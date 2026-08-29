// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "BackingEtwRule.h"
#include "Microsoft.UI.Xaml.private.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
// The DynamicBindingRule is a rule that detects if developers are using dynamic binding
// in a scenario where x:Bind is supported

class DynamicBindingRule
    : public EtwRuleImpl<DynamicBindingRule, appanalysis::RuleCategories_Performance>
{

public:

    DynamicBindingRule()
    {
    }

    virtual ~DynamicBindingRule()
    {
    }

    HRESULT DynamicBindingRule::ProcessUpdateTargetBindingEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        boolean templateBound = FALSE;
        IFC_RETURN(pEvent->GetBooleanProperty(StringRef(L"IsPropertyTemplateBound"), &templateBound));

        UINT32 sourceType = wux_data::EffectiveSourceType_None;
        IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"EffectiveSourceType"), &sourceType));

        // Ignore this if this scenario requires binding
        if(DynamicBindingIsRequired(!!templateBound, static_cast< wux_data::EffectiveSourceType>(sourceType)))
        {
            return S_OK;
        }

        InstanceHandle elementId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &elementId));

        wrl::ComPtr<appanalysis::ISourceInfoRuleService> sourceInfoService;
        IFC_RETURN(GetRuleService(&sourceInfoService));

        RuleTriggeredEventArgs::CreateParams params;

        IFC_RETURN(sourceInfoService->GetSourceInfo(elementId, &params.sourceInfo));

        // if this comes from generic xaml, then ignore it
        if (AppAnalysisHelpers::IsGenericXaml(params.sourceInfo))
        {
            return S_OK;
        }

        IFC_RETURN(sourceInfoService->GetVisualTreeId(elementId, &params.elementId));

        wil::shared_hstring modelTypeName;
        IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"ModelTypeName"), &modelTypeName));

        wil::shared_hstring propertyName;
        IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"PropertyName"), &propertyName));

        wil::shared_hstring modelPropertyName;
        IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"ModelPropertyName"), &modelPropertyName));

        wil::shared_hstring elementTypeName;
        IFC_RETURN(sourceInfoService->GetElementTypeName(elementId, &elementTypeName));

        wil::shared_hstring elementName;
        HRESULT hr = sourceInfoService->GetElementName(elementId, &elementName);
        IFC_RETURN(hr);

        if (hr == S_FALSE)
        {
            IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, DYNAMIC_BINDING_DESCRIPTION_NO_NAME));
        }
        else
        {
            IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, DYNAMIC_BINDING_DESCRIPTION));
            IFC_RETURN(params.description->Append(elementName.get()));
        }

        IFC_RETURN(params.description->Append(elementTypeName.get()));
        IFC_RETURN(params.description->Append(propertyName.get()));
        IFC_RETURN(params.description->Append(modelTypeName.get()));
        IFC_RETURN(params.description->Append(modelPropertyName.get()));

        IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.solution, DYNAMIC_BINDING_SOLUTION));

        wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> RuleTriggeredEventArgs;
        IFC_RETURN(CreateRuleTriggeredEventArgs(params, &RuleTriggeredEventArgs));

        FireNotification(RuleTriggeredEventArgs.Get());
        return S_OK;
    }

private:

    bool DynamicBindingIsRequired(bool isPropertyTemplateBound, wux_data::EffectiveSourceType sourceType)
    {
       return isPropertyTemplateBound ||
           (sourceType == wux_data::EffectiveSourceType_Binding_Source) ||
            (sourceType == wux_data::EffectiveSourceType_TemplatedParent) ||
           (sourceType == wux_data::EffectiveSourceType_Mentor_TemplatedParent);
    }

};

BEGIN_PROVIDERS(DynamicBindingRule)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(DynamicBindingRule)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, UpdateTargetBindingBegin_value, EventVersion_1, &DynamicBindingRule::ProcessUpdateTargetBindingEvent)
END_CALLBACKS()


////////////////////////////////////////////////////////////////////////////////
//
HRESULT DynamicBindingRule_CreateInstance(
    _COM_Outptr_ appanalysis::IEtwRule** ppInstance
    )
{
    wrl::ComPtr<DynamicBindingRule> rule;
    IFC_RETURN(DynamicBindingRule::CreateInstance(
        DYNAMIC_BINDING_ID, DYNAMIC_BINDING_TITLE, DYNAMIC_BINDING_IMPACT,
        DYNAMIC_BINDING_LINK_TITLE, DYNAMIC_BINDING_LINK_URL,
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterEvents(&watcher));

    IFC_RETURN(wrl::MakeAndInitialize<EtwRule>(ppInstance, rule.Get(), watcher.Get()));

    return S_OK;
}
} } }
