// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A mixin that overrides the writting IVector methods 
//      to be not implemented

#pragma once

namespace DirectUI
{
    template <class T>
    class __declspec(novtable) ReadOnlyCollectionView: public T
    {
    public:

        // Override the "writting" methods to treat this
        // as a read only array
        IFACEMETHODIMP SetAt(_In_ UINT index, _In_opt_ IInspectable *item) override { RRETURN(E_NOTIMPL); }
        IFACEMETHODIMP InsertAt(_In_ UINT index, _In_opt_ IInspectable *item) override { RRETURN(E_NOTIMPL); }
        IFACEMETHODIMP RemoveAt(_In_ UINT index) override { RRETURN(E_NOTIMPL); }
        IFACEMETHODIMP Append(_In_opt_ IInspectable *item) override { RRETURN(E_NOTIMPL); }
        IFACEMETHODIMP RemoveAtEnd() override { RRETURN(E_NOTIMPL); }
        IFACEMETHODIMP Clear() override { RRETURN(E_NOTIMPL); }

    protected:
        // This class is marked novtable, so must not be instantiated directly.
        ReadOnlyCollectionView() = default;
    };
}
