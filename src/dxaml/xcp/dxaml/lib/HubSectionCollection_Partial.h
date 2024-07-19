// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "HubSectionCollection.g.h"

namespace DirectUI
{
    // CollectionViewSource::put_Source() expects IIterable<IInspectable> so we need a custom iterator that provides that interface.
    class HubSectionCollectionIterator
        : public TrackerIterator<xaml_controls::HubSection*>
        , public wfc::IIterator<IInspectable*>

    {
        BEGIN_INTERFACE_MAP(HubSectionCollectionIterator, TrackerIterator<xaml_controls::HubSection*>)
            INTERFACE_ENTRY(HubSectionCollectionIterator, wfc::IIterator<IInspectable*>)
        END_INTERFACE_MAP(HubSectionCollectionIterator, TrackerIterator<xaml_controls::HubSection*>)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterator<IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IIterator<IInspectable*>*>(this);
            }
            else
            {
                RRETURN(TrackerIterator<xaml_controls::HubSection*>::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

        // IIterator<IInspectable*> implementation
        IFACEMETHOD(get_Current)(_Outptr_ IInspectable** current) override;
        IFACEMETHOD(get_HasCurrent)(_Out_ BOOLEAN *hasCurrent) override;
        IFACEMETHOD(MoveNext)(_Out_ BOOLEAN *hasCurrent) override;
    };

    PARTIAL_CLASS(HubSectionCollection)
        , public wfc::IIterable<IInspectable*>
        , public wfc::IVector<IInspectable*>

    {

    protected:
        BEGIN_INTERFACE_MAP(HubSectionCollection, HubSectionCollectionGenerated)
            INTERFACE_ENTRY(HubSectionCollection, wfc::IIterable<IInspectable*>)
            INTERFACE_ENTRY(HubSectionCollection, wfc::IVector<IInspectable*>)
        END_INTERFACE_MAP(HubSectionCollection, HubSectionCollectionGenerated)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IIterable<IInspectable*>*>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wfc::IVector<IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IVector<IInspectable*>*>(this);
            }
            else
            {
                RRETURN(HubSectionCollectionGenerated::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

    public:
        // IIterable<IInspectable*> implementation
        IFACEMETHOD(First)(
            _Outptr_ wfc::IIterator<IInspectable*> **iterator) override;

        // IIterable<xaml_controls::IHubSection*> overrides
        IFACEMETHOD(First)(
            _Outptr_ wfc::IIterator<xaml_controls::HubSection*> **iterator) override;

        // IVector<IInspectable*> implementation
        IFACEMETHOD(GetAt)(_In_ UINT index, _Outptr_ IInspectable** item) override;
        IFACEMETHOD(get_Size)(_Out_ UINT* value) override;
        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable*>** view) override;
        IFACEMETHOD(IndexOf)(_In_opt_ IInspectable* value, _Out_ UINT* index, _Out_ BOOLEAN* found) override;
        IFACEMETHOD(SetAt)(_In_ UINT index, _In_opt_ IInspectable* item) override;
        IFACEMETHOD(InsertAt)(_In_ UINT index, _In_ IInspectable* item) override;
        IFACEMETHOD(RemoveAt)(_In_ UINT index) override;
        IFACEMETHOD(Append)(_In_opt_ IInspectable* item) override;
        IFACEMETHOD(RemoveAtEnd)() override;
        IFACEMETHOD(Clear)() override;

        // Ensures the item type is valid based on the type of the content property.
        static _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject* pItem);
    };
}
