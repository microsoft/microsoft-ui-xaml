// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the a collection of ICommandBarElements.

#pragma once

#include "CommandBarElementCollection.g.h"

namespace DirectUI
{
    class CommandBar;

    PARTIAL_CLASS(CommandBarElementCollection)
    {
    public:
        // Override these methods so that we can raise vector changed events.
        IFACEMETHOD(SetAt)(_In_ UINT index, _In_opt_ xaml_controls::ICommandBarElement* item) override;
        IFACEMETHOD(InsertAt)(_In_ UINT index, _In_opt_ xaml_controls::ICommandBarElement* item) override;
        IFACEMETHOD(RemoveAt)(_In_ UINT index) override;
        IFACEMETHOD(Append)(_In_opt_ xaml_controls::ICommandBarElement* item) override;
        IFACEMETHOD(RemoveAtEnd)() override;
        IFACEMETHOD(Clear)() override;

        // Ensures the item type is valid based on the type of the content property.
        static _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject* pItem);

        void Init(_In_ bool notifyCollectionChanging);

    private:
        _Check_return_ HRESULT RaiseVectorChanged(_In_ wfc::CollectionChange change, _In_ XUINT32 changeIndex);

        _Check_return_ HRESULT RaiseVectorChanging(
            _In_ wfc::CollectionChange change,
            _In_ UINT32 changeIndex);

        bool m_notifyCollectionChanging = false;
    };
}
