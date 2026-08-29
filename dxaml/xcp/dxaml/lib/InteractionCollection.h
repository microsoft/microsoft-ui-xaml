// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InteractionBase.g.h"

namespace DirectUI
{
#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    constexpr unsigned int EventTypeCountsArraySize =  static_cast<unsigned int>(LastControlEvent) + 1;

    class __declspec(uuid("dba293b4-85ba-4f60-8b41-d8d5c5280690")) InteractionCollection :
        public wfc::IVector<xaml::InteractionBase*>,
        public wfc::IIterable<xaml::InteractionBase*>,
        public ctl::WeakReferenceSource
    {
    protected:
        BEGIN_INTERFACE_MAP(InteractionCollection, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(InteractionCollection, wfc::IVector<xaml::InteractionBase*>)
            INTERFACE_ENTRY(InteractionCollection, wfc::IIterable<xaml::InteractionBase*>)
        END_INTERFACE_MAP(InteractionCollection, ctl::WeakReferenceSource)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

    public:
        _Check_return_ HRESULT Initialize(_In_ xaml::IUIElement* owner);

        // IVector<xaml::InteractionBase*>
        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IVectorView<xaml::InteractionBase*>** view) override;

        IFACEMETHOD(get_Size)(_Out_ unsigned *size) override;

        IFACEMETHOD(IndexOf)(_In_opt_ xaml::IInteractionBase* item, _Out_ unsigned *index, _Out_ boolean *found) override;

        IFACEMETHOD(GetAt)(_In_opt_ unsigned index, _Out_ xaml::IInteractionBase** item) override;

        IFACEMETHOD(SetAt)(_In_ unsigned index, _In_opt_ xaml::IInteractionBase* item) override;

        IFACEMETHOD(InsertAt)(_In_ unsigned index, _In_ xaml::IInteractionBase* item) override;

        IFACEMETHOD(Append)(_In_opt_ xaml::IInteractionBase* item) override;

        IFACEMETHOD(RemoveAt)(_In_ unsigned index) override;

        IFACEMETHOD(RemoveAtEnd)() override;

        IFACEMETHOD(Clear)() override;

        // IIterable<xaml::InteractionBase*>
        IFACEMETHOD(First)(_Outptr_ wfc::IIterator<xaml::InteractionBase*>** value) override;

        // FxCallback
        static _Check_return_ HRESULT HasInteractionForEvent(KnownEventIndex eventId, _In_ CUIElement* coreSender, _Out_ bool& hasInteraction);
        static _Check_return_ HRESULT DispatchInteraction(KnownEventIndex eventId, _In_ CUIElement* coreSender, _In_ CEventArgs* coreArgs);

    private:
        _Check_return_ HRESULT EnsureCollection();
        _Check_return_ HRESULT OnInteractionAdded(_In_opt_ xaml::IInteractionBase* interaction);
        _Check_return_ HRESULT OnInteractionRemoved(_In_opt_ xaml::IInteractionBase* interaction);
        _Check_return_ HRESULT OnInteractionsCleared();

        _Check_return_ HRESULT HasInteractionForEventWorker(KnownEventIndex eventId, _In_ const ctl::ComPtr<UIElement>& sender, _Out_ bool& hasInteraction);
        _Check_return_ HRESULT DispatchInteractionWorker(KnownEventIndex eventId, _In_ const ctl::ComPtr<UIElement>& sender, _In_ const ctl::ComPtr<RoutedEventArgs>& args);

        ctl::WeakRefPtr m_wrOwner;
        TrackerPtr<TrackerCollection<xaml::InteractionBase*>> m_collection;

        std::array<unsigned char, EventTypeCountsArraySize> m_eventTypeCountsArray{};
    };
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
}
