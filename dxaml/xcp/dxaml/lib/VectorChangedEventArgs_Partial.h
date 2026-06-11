// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VectorChangedEventArgs.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(VectorChangedEventArgs),
        public wfc::IVectorChangedEventArgs
    {
        BEGIN_INTERFACE_MAP(VectorChangedEventArgs, VectorChangedEventArgsGenerated)
            INTERFACE_ENTRY(VectorChangedEventArgs, wfc::IVectorChangedEventArgs )
        END_INTERFACE_MAP(VectorChangedEventArgs, VectorChangedEventArgsGenerated)

    public:
        // Constructors/destructors.
        VectorChangedEventArgs()
            : m_action(wfc::CollectionChange_Reset)
            , m_index(0)
        {
        }

        // Properties.
        IFACEMETHOD(get_CollectionChange)(
            _Out_ wfc::CollectionChange* value)
        {
            HRESULT hr = S_OK;
            IFCPTR(value);

            *value = m_action;

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHOD(get_Index) (_Out_ UINT* value)
        {
            HRESULT hr = S_OK;
            IFCPTR(value);

            *value = m_index;

        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT put_CollectionChange(
            _In_ wfc::CollectionChange value)

        {
            m_action = value;
            RRETURN(S_OK);
        }

        _Check_return_ HRESULT put_Index(_In_ UINT value)
        {
            m_index = value;
            RRETURN(S_OK);
        }

        // Virtual properties.

        // Protected properties.

        // Events.

        // Methods.

        // Virtual methods.

        // Protected methods.

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(
            _In_ REFIID iid,
            _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IVectorChangedEventArgs)))
            {
                *ppObject = static_cast<wfc::IVectorChangedEventArgs*>(this);
            }
            else
            {
                RRETURN(VectorChangedEventArgsGenerated::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }


    private:
        // Private Setters

        // Private properties.
        wfc::CollectionChange m_action;
        UINT m_index;
    };
};
