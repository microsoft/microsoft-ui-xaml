// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include <strsafe.h>
#include "EtwRuleSet.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

typedef std::function<HRESULT(appanalysis::IEtwRule**)> fpRuleCreateInstance;
////////////////////////////////////////////////////////////////////////////////
// Rule prototypes
//
HRESULT ImageDecodingRule_CreateInstance(appanalysis::IEtwRule**);
HRESULT CollapsedElementsRule_CreateInstance(appanalysis::IEtwRule**);
HRESULT ListViewDevirtualizedRule_CreateInstance(appanalysis::IEtwRule**);
HRESULT UIThreadUtilization_CreateInstance(appanalysis::IEtwRule**);
HRESULT DynamicBindingRule_CreateInstance(appanalysis::IEtwRule**);
HRESULT NoAutomationNameRule_CreateInstance(appanalysis::IEtwRule**);
HRESULT AutomationNameDuplicatedRule_CreateInstance(appanalysis::IEtwRule**);
HRESULT ResourceUsingXNameRule_CreateInstance(appanalysis::IEtwRule**);
HRESULT NonVirtualizingItemsPanelRule_CreateInstance(appanalysis::IEtwRule**);

fpRuleCreateInstance g_fpRules[] =
{
    &ImageDecodingRule_CreateInstance,
    &CollapsedElementsRule_CreateInstance,
    &ListViewDevirtualizedRule_CreateInstance,
    &UIThreadUtilization_CreateInstance,
    &DynamicBindingRule_CreateInstance,
    &NoAutomationNameRule_CreateInstance,
    &AutomationNameDuplicatedRule_CreateInstance,
    &ResourceUsingXNameRule_CreateInstance,
    &NonVirtualizingItemsPanelRule_CreateInstance,
};

////////////////////////////////////////////////////////////////////////////////
//
EtwRuleSet::EtwRuleSet()
{
}

////////////////////////////////////////////////////////////////////////////////
//
EtwRuleSet::~EtwRuleSet()
{

}

////////////////////////////////////////////////////////////////////////////////
HRESULT
EtwRuleSet::RuntimeClassInitialize()
{
    IFC_RETURN(wfci_::Vector<appanalysis::EtwRule*>::Make(m_rules.GetAddressOf()));

    for (UINT i = 0; i < _countof(g_fpRules); i++)
    {
        fpRuleCreateInstance createInstance = nullptr;
        createInstance = g_fpRules[i];

        wrl::ComPtr<appanalysis::IEtwRule> rule;
        IFC_RETURN(createInstance(&rule));
        IFC_RETURN(m_rules->Append(rule.Get()));
    }

    return S_OK;
}

IFACEMETHODIMP
EtwRuleSet::First(
    _COM_Outptr_ wfc::IIterator<appanalysis::EtwRule*> **iterator)
{
    ARG_VALIDRETURNPOINTER(iterator);

    *iterator = nullptr;
    IFC_RETURN(m_rules->First(iterator));

    return S_OK;
}

IFACEMETHODIMP
EtwRuleSet::GetAt(
    _In_ unsigned int index,
    _COM_Outptr_ appanalysis::IEtwRule **rule)
{
    ARG_VALIDRETURNPOINTER(rule);
    IFC_RETURN(m_rules->GetAt(index, rule));

    return S_OK;
}

IFACEMETHODIMP
EtwRuleSet::get_Size(
    _Out_ unsigned int *size)
{
    ARG_VALIDRETURNPOINTER(size);
    IFC_RETURN(m_rules->get_Size(size));

    return S_OK;
}

IFACEMETHODIMP
EtwRuleSet::IndexOf(
    _In_ appanalysis::IEtwRule* rule,
    _Out_ unsigned int *index,
    _Out_ boolean *found)
{
    ARG_VALIDRETURNPOINTER(index);
    ARG_VALIDRETURNPOINTER(found);

    IFC_RETURN(m_rules->IndexOf(rule, index, found));

    return S_OK;
}

} } }
