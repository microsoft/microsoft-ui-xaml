// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <vector>

////////////////////////////////////////////////////////////////////////////////
//
// The RuleSet is a collection of rules that a module will expose.
//
namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    class ResourceStringFactory :
        public wrl::AgileActivationFactory<appanalysis::IResourceStringFactory>
    {
        IFACEMETHOD(CreateInstance)(
            _In_ UINT32 identifier,
            _COM_Outptr_ appanalysis::IResourceString** resourceString
            );
    };

    class ResourceString :
        public wrl::RuntimeClass<wrl::Implements<wfc::IVector<HSTRING>, wfc::IIterable<HSTRING>, appanalysis::IResourceString>, wrl::FtmBase>
    {
        InspectableClass(
            RuntimeClass_Microsoft_Diagnostics_AppAnalysis_ResourceString,
            BaseTrust
            );

    public:

        ResourceString()
           : m_identifier(0)
        {
        }

        virtual ~ResourceString()
        {
        }

        HRESULT RuntimeClassInitialize(_In_ UINT32 identifier);

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // IResourceString
        IFACEMETHOD(get_Identifier)(
            _Out_ UINT32* identifier
            ) override;

        IFACEMETHOD(put_Identifier)(
            _In_ UINT32 identifier
            ) override;

        IFACEMETHOD(GetResourceStringView)(
            _COM_Outptr_ appanalysis::IResourceStringView** view
            ) override;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // Vector read methods
        IFACEMETHOD(First)(
            _COM_Outptr_ wfc::IIterator<HSTRING> **first
            ) override;

        IFACEMETHOD(GetAt)(
            _In_ UINT32 index,
            _Outptr_ HSTRING *ppEntry) override;

        IFACEMETHOD(get_Size)(
            _Out_ UINT32 *size) override;

        IFACEMETHOD(GetView)(
            _COM_Outptr_result_maybenull_ wfc::IVectorView<HSTRING> **view
            ) override;

        IFACEMETHOD(IndexOf)(
            _In_ HSTRING arg,
            _Out_ UINT32 *index,
            _Out_ boolean *found) override;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // Vector write methods
        IFACEMETHOD(SetAt)(
            _In_ unsigned index,
            _In_opt_ HSTRING item
            ) override;

        IFACEMETHOD(InsertAt)(
            _In_ unsigned index,
            _In_opt_ HSTRING item
            ) override;

        IFACEMETHOD(RemoveAt)(
            _In_ unsigned index
            ) override;

        IFACEMETHOD(Append)(
            _In_opt_ HSTRING item
            ) override;

        IFACEMETHOD(RemoveAtEnd)() override;
        IFACEMETHOD(Clear)() override;

    private:

        wrl::ComPtr<wfci_::Vector<HSTRING>> m_args;
        UINT32 m_identifier;
    };

    class ResourceStringView :
        public wrl::RuntimeClass<wrl::Implements<wfc::IVectorView<HSTRING>, wfc::IIterable<HSTRING>, appanalysis::IResourceStringView>, wrl::FtmBase>
    {
        InspectableClass(
            L"Microsoft.Diagnostics.AppAnalysis.ResourceStringView",
            BaseTrust
            );

    public:

        ResourceStringView()
            :m_identifier(0)
        {
        }

        virtual ~ResourceStringView()
        {
        }

        HRESULT RuntimeClassInitialize(_In_opt_ appanalysis::IResourceString* resourceString);

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // IResourceString
        IFACEMETHOD(get_Identifier)(
            _Out_ UINT32* identifier
            ) override;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // Vector read methods
        IFACEMETHOD(First)(
            _COM_Outptr_ wfc::IIterator<HSTRING> **first
            ) override;

        IFACEMETHOD(GetAt)(
            _In_ UINT32 index,
            _Outptr_ HSTRING *ppEntry) override;

        IFACEMETHOD(get_Size)(
            _Out_ UINT32 *size) override;

        IFACEMETHOD(IndexOf)(
            _In_ HSTRING arg,
            _Out_ UINT32 *index,
            _Out_ boolean *found) override;

    private:
        HRESULT CheckBounds();

        wrl::ComPtr<wfc::IVectorView<HSTRING>> m_args;
        wrl::ComPtr<wfc::IIterable<HSTRING>> m_iterable;
        UINT32 m_identifier;

    };
} } }
