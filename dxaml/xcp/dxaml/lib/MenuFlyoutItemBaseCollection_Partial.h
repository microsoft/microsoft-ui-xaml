// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuFlyoutItemBaseCollection.g.h"

namespace DirectUI
{
    // CollectionViewSource::put_Source() expects IIterable<IInspectable*> so we need a custom iterator that provides that interface.
    // MenuFlyoutItemBaseCollectionGenerated gives us IIterable<IMenuFlyoutItemBase*>, but QI for IIterable<IInspectable*> will fail.
    // Thus, we must implement IIterable<IInspectable*> and provide implementation wrappers that drive IIterable<IIMenuFlyoutItemBase*>.
    class MenuFlyoutItemBaseCollectionIterator
        : public TrackerIterator<xaml_controls::MenuFlyoutItemBase*>
        , public wfc::IIterator<IInspectable*>

    {
        BEGIN_INTERFACE_MAP(MenuFlyoutItemBaseCollectionIterator, TrackerIterator<xaml_controls::MenuFlyoutItemBase*>)
            INTERFACE_ENTRY(MenuFlyoutItemBaseCollectionIterator, wfc::IIterator<IInspectable*>)
        END_INTERFACE_MAP(MenuFlyoutItemBaseCollectionIterator, TrackerIterator<xaml_controls::MenuFlyoutItemBase*>)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterator<IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IIterator<IInspectable*>*>(this);
            }
            else
            {
                RRETURN(TrackerIterator<xaml_controls::MenuFlyoutItemBase*>::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

        // IIterator<IInspectable*> implementation
        IFACEMETHOD(get_Current)(_Outptr_ IInspectable** current) override;
        IFACEMETHOD(get_HasCurrent)(_Out_ BOOLEAN *hasCurrent) override;
        IFACEMETHOD(MoveNext)(_Out_ BOOLEAN *hasCurrent) override;
    };

    PARTIAL_CLASS(MenuFlyoutItemBaseCollection)
        , public wfc::IIterable<IInspectable*>
    {
    protected:
        BEGIN_INTERFACE_MAP(MenuFlyoutItemBaseCollection, MenuFlyoutItemBaseCollectionGenerated)
            INTERFACE_ENTRY(MenuFlyoutItemBaseCollection, wfc::IIterable<IInspectable*>)
        END_INTERFACE_MAP(MenuFlyoutItemBaseCollection, MenuFlyoutItemBaseCollectionGenerated)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IIterable<IInspectable*>*>(this);
            }
            else
            {
                RRETURN(MenuFlyoutItemBaseCollectionGenerated::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

    public:
        // Override these methods so that we notify the MenuFlyout that its items collection has changed.
        IFACEMETHOD(SetAt)(_In_ UINT index, _In_opt_ xaml_controls::IMenuFlyoutItemBase* item) override;
        IFACEMETHOD(InsertAt)(_In_ UINT index, _In_opt_ xaml_controls::IMenuFlyoutItemBase* item) override;
        IFACEMETHOD(RemoveAt)(_In_ UINT index) override;
        IFACEMETHOD(Append)(_In_opt_ xaml_controls::IMenuFlyoutItemBase* item) override;
        IFACEMETHOD(RemoveAtEnd)() override;
        IFACEMETHOD(Clear)() override;

        // IIterable<IInspectable*> implementation
        IFACEMETHOD(First)(
            _Outptr_ wfc::IIterator<IInspectable*> **iterator) override;

        // IIterable<ABI::Microsoft::UI::Xaml::Controls::MenuFlyoutItemBase*> overrides
        IFACEMETHOD(First)(
            _Outptr_ wfc::IIterator<xaml_controls::MenuFlyoutItemBase*> **iterator) override;

        // Ensures the item type is valid based on the type of the content property.
        static _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject* pItem);

    private:
        _Check_return_ HRESULT NotifyMenuFlyoutOfCollectionChange();
    };
}
