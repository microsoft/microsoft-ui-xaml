// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <vector>

////////////////////////////////////////////////////////////////////////////////
//
// The RuleSet is a collection of rules that a module will expose.
//
namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    class EtwRuleSet :
        public wrl::RuntimeClass<wrl::Implements<wfc::IVectorView<appanalysis::EtwRule*>, wfc::IIterable<appanalysis::EtwRule*>>, Microsoft::WRL::FtmBase>
    {
        InspectableClass(
            RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwRuleSet,
            BaseTrust
            );

    public:

        EtwRuleSet();
        virtual ~EtwRuleSet();

        HRESULT RuntimeClassInitialize();

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
} } }
