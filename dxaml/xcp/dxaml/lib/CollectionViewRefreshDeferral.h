// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    // Returned by DeferRefresh on a shaping view or a CollectionViewSource. Holds its owner (as WPF's
    // deferral holds its view) so a pending refresh still runs; Close is idempotent.
    template <typename TOwner>
    class CollectionViewRefreshDeferral final :
        public wrl::RuntimeClass<wrl::RuntimeClassFlags<wrl::WinRt>, wf::IClosable>
    {
        InspectableClass(L"Microsoft.UI.Xaml.Data.CollectionViewSourceRefreshDeferral", BaseTrust);

    public:
        HRESULT RuntimeClassInitialize(_In_ TOwner* pOwner)
        {
            m_spOwner = ctl::as_iinspectable(pOwner);
            m_pOwner = pOwner;
            return S_OK;
        }

        IFACEMETHODIMP Close() override
        {
            if (!m_spOwner)
            {
                return S_OK;
            }

            IFC_RETURN(m_pOwner->CheckThread());

            // Released first so a Close re-entered from the refresh is a no-op.
            auto spOwner = std::move(m_spOwner);
            auto pOwner = std::exchange(m_pOwner, nullptr);
            IFC_RETURN(pOwner->EndDeferRefresh());
            return S_OK;
        }

    private:
        Microsoft::WRL::ComPtr<IInspectable> m_spOwner;
        TOwner* m_pOwner = nullptr;
    };
}
