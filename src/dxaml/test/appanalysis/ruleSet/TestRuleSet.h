// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "wil_resource.h"

////////////////////////////////////////////////////////////////////////////////
//
// The RuleSet is a collection of rules that a module will expose.
//

DEFINE_GUID(CLSID_TestRuleSet,
    0x5d4ba714, 0xf43e, 0x429f, 0x92, 0xc8, 0x19, 0x3f, 0xaa, 0x8f, 0x1e, 0xfe);

namespace AppAnalysis { namespace Test {

class TestRuleSet :
   public wrl::RuntimeClass<wrl::Implements<
                            wfc::IVectorView<appanalysis::EtwRule*>,
                            wfc::IIterable<appanalysis::EtwRule*>>,
                            Microsoft::WRL::FtmBase>
{
    InspectableClass(
        L"AppAnalysis.Test.TestRuleSet",
        BaseTrust
        );

public:
    TestRuleSet();
    virtual ~TestRuleSet();
    HRESULT RuntimeClassInitialize(
        );

    IFACEMETHOD(First)(
        _COM_Outptr_ wfc::IIterator<appanalysis::EtwRule*> **first
        );

    IFACEMETHOD(GetAt)(
        _In_ UINT32 index,
        _COM_Outptr_ appanalysis::IEtwRule **ppEntry);

    IFACEMETHOD(get_Size)(
        _Out_ UINT32 *size);

    IFACEMETHOD(IndexOf)(
        _In_ appanalysis::IEtwRule* rule,
        _Out_ UINT32 *index,
        _Out_ boolean *found);

private:
    wrl::ComPtr<wfci_::Vector<appanalysis::EtwRule*>> m_rules;
};
} }