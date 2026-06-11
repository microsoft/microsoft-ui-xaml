// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
z////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Microsoft Corporation.  All rights reserved.
//

#include <precomp.h>
#include <strsafe.h>
#include "TestRuleSet.h"
#include "helpers.h"

using namespace Microsoft::Diagnostics::AppAnalysis;
typedef std::function<HRESULT(appanalysis::IEtwRule**)> fpRuleCreateInstance;
namespace AppAnalysis { namespace Test {

////////////////////////////////////////////////////////////////////////////////
// Rule prototypes
//
HRESULT TestRule_CreateInstance(appanalysis::IEtwRule**);
HRESULT TestMultipleProvidersRule_CreateInstance(appanalysis::IEtwRule**);

fpRuleCreateInstance g_fpTestRules[] =
{
    &TestRule_CreateInstance,
    &TestMultipleProvidersRule_CreateInstance
};

////////////////////////////////////////////////////////////////////////////////
//
TestRuleSet::TestRuleSet()
{
}

////////////////////////////////////////////////////////////////////////////////
//
TestRuleSet::~TestRuleSet()
{
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
TestRuleSet::RuntimeClassInitialize(
    )
{
    IFC_RETURN(wfci_::Vector<appanalysis::EtwRule*>::Make(m_rules.GetAddressOf()));
    for (UINT i = 0; i < _countof(g_fpTestRules); i++)
    {
        fpRuleCreateInstance createInstance = nullptr;
        createInstance = g_fpTestRules[i];

        wrl::ComPtr<appanalysis::IEtwRule> rule;
        IFC_RETURN(createInstance(&rule));
        IFC_RETURN(m_rules->Append(rule.Get()));
    }

    return S_OK;
}

IFACEMETHODIMP
TestRuleSet::First(
    _COM_Outptr_ wfc::IIterator<appanalysis::EtwRule*> **iterator)
{
    ARG_VALIDRETURNPOINTER(iterator);

    *iterator = nullptr;
    IFC_RETURN(m_rules->First(iterator));

    return S_OK;
}

IFACEMETHODIMP
TestRuleSet::GetAt(
    _In_ unsigned int index,
    _COM_Outptr_ appanalysis::IEtwRule **rule)
{
    ARG_VALIDRETURNPOINTER(rule);
    IFC_RETURN(m_rules->GetAt(index, rule));

    return S_OK;
}

IFACEMETHODIMP
TestRuleSet::get_Size(
    _Out_ unsigned int *size)
{
    ARG_VALIDRETURNPOINTER(size);
    IFC_RETURN(m_rules->get_Size(size));

    return S_OK;
}

IFACEMETHODIMP
TestRuleSet::IndexOf(
    _In_ appanalysis::IEtwRule* rule,
    _Out_ unsigned int *index,
    _Out_ boolean *found)
{
    ARG_VALIDRETURNPOINTER(index);
    ARG_VALIDRETURNPOINTER(found);

    IFC_RETURN(m_rules->IndexOf(rule, index, found));

    return S_OK;
}

} }

