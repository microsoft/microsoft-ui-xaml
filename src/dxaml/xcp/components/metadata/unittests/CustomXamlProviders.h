// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Metadata {

        class IXamlMemberTypeMayReturnNull_XamlMember : public xaml_markup::IXamlMember
        {
        public:
            IFACEMETHODIMP_(ULONG) AddRef() override
            {
                return 1;
            }

            IFACEMETHODIMP_(ULONG) Release() override
            {
                return 1;
            }

            IFACEMETHODIMP QueryInterface(REFIID, void**) override
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP GetIids(ULONG*, IID**) override
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP GetRuntimeClassName(HSTRING*) override
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP GetTrustLevel(TrustLevel*) override
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_IsAttachable(BOOLEAN* pValue) override
            {
                *pValue = TRUE;
                return S_OK;
            }

            IFACEMETHODIMP get_IsDependencyProperty(BOOLEAN*) override
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_IsReadOnly(BOOLEAN* pValue) override
            {
                *pValue = TRUE;
                return S_OK;
            }

            IFACEMETHODIMP get_Name(HSTRING* pValue) override
            {
                return wrl_wrappers::HStringReference(L"IXamlMemberTypeMayReturnNull_XamlMember").CopyTo(pValue);
            }

            IFACEMETHODIMP get_Type(xaml_markup::IXamlType** ppValue) override
            {
                // Return null to simulate the VS designer in some cases.
                *ppValue = nullptr;
                return S_OK;
            }

            IFACEMETHODIMP get_TargetType(xaml_markup::IXamlType**) override
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP GetValue(IInspectable*, IInspectable**) override
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP SetValue(IInspectable*, IInspectable*) override
            {
                return E_NOTIMPL;
            }
        };

} } } } }
