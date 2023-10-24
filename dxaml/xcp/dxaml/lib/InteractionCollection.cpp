// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "InteractionCollection.h"
#include "RoutedEvent.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
_Check_return_ HRESULT InteractionCollection::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(wfc::IVector<xaml::InteractionBase*>)))
    {
        *ppObject = static_cast<wfc::IVector<xaml::InteractionBase*>*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<xaml::InteractionBase*>)))
    {
        *ppObject = static_cast<wfc::IIterable<xaml::InteractionBase*>*>(this);
    }
    else
    {
        IFC_RETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    return S_OK;
}

_Check_return_ HRESULT InteractionCollection::Initialize(_In_ xaml::IUIElement* owner)
{
    IFC_RETURN(ctl::AsWeak(owner, &m_wrOwner));
    return S_OK;
}

// IVector
IFACEMETHODIMP InteractionCollection::GetView(_Outptr_result_maybenull_ wfc::IVectorView<xaml::InteractionBase*>** view)
{
    IFC_RETURN(EnsureCollection());
    IFC_RETURN(m_collection->GetView(view));
    return S_OK;
}

IFACEMETHODIMP InteractionCollection::get_Size(_Out_ unsigned *size)
{
    IFC_RETURN(EnsureCollection());
    IFC_RETURN(m_collection->get_Size(size));
    return S_OK;
}

IFACEMETHODIMP InteractionCollection::IndexOf(_In_opt_ xaml::IInteractionBase* item, _Out_ unsigned *index, _Out_ BOOLEAN *found)
{
    IFC_RETURN(EnsureCollection());
    IFC_RETURN(m_collection->IndexOf(item, index, found));
    return S_OK;
}

IFACEMETHODIMP InteractionCollection::GetAt(_In_opt_ unsigned index, _Out_ xaml::IInteractionBase** item)
{
    IFC_RETURN(EnsureCollection());
    IFC_RETURN(m_collection->GetAt(index, item));
    return S_OK;
}

IFACEMETHODIMP InteractionCollection::SetAt(_In_ unsigned index, _In_opt_ xaml::IInteractionBase* item)
{
    IFC_RETURN(EnsureCollection());

    ctl::ComPtr<xaml::IInteractionBase> removedItem;
    IFC_RETURN(GetAt(index, removedItem.GetAddressOf()));
    IFC_RETURN(OnInteractionRemoved(removedItem.Get()));

    IFC_RETURN(OnInteractionAdded(item));

    IFC_RETURN(m_collection->SetAt(index, item));

    return S_OK;
}

IFACEMETHODIMP InteractionCollection::InsertAt(_In_ unsigned index, _In_ xaml::IInteractionBase* item)
{
    IFC_RETURN(EnsureCollection());
    IFC_RETURN(OnInteractionAdded(item));
    IFC_RETURN(m_collection->InsertAt(index, item));
    return S_OK;
}

IFACEMETHODIMP InteractionCollection::Append(_In_opt_ xaml::IInteractionBase* item)
{
    IFC_RETURN(EnsureCollection());
    IFC_RETURN(OnInteractionAdded(item));
    IFC_RETURN(m_collection->Append(item));

    return S_OK;
}

IFACEMETHODIMP InteractionCollection::RemoveAt(_In_ unsigned index)
{
    IFC_RETURN(EnsureCollection());

    ctl::ComPtr<xaml::IInteractionBase> removedItem;
    IFC_RETURN(GetAt(index, removedItem.GetAddressOf()));
    IFC_RETURN(OnInteractionRemoved(removedItem.Get()));

    IFC_RETURN(m_collection->RemoveAt(index));

    return S_OK;
}

IFACEMETHODIMP InteractionCollection::RemoveAtEnd()
{
    IFC_RETURN(EnsureCollection());

    ctl::ComPtr<xaml::IInteractionBase> removedItem;
    unsigned int size = 0;
    IFC_RETURN(get_Size(&size));
    if (size > 0)
    {
        IFC_RETURN(GetAt(size - 1, removedItem.GetAddressOf()));
        IFC_RETURN(OnInteractionRemoved(removedItem.Get()));

        IFC_RETURN(m_collection->RemoveAtEnd());
    }

    return S_OK;
}

IFACEMETHODIMP InteractionCollection::Clear()
{
    IFC_RETURN(EnsureCollection());
    IFC_RETURN(m_collection->Clear());
    IFC_RETURN(OnInteractionsCleared());
    return S_OK;
}

// IIterable
IFACEMETHODIMP InteractionCollection::First(_Outptr_ wfc::IIterator<xaml::InteractionBase*>** value)
{
    IFC_RETURN(EnsureCollection());
    IFC_RETURN(m_collection->First(value));
    return S_OK;
}

// FxCallback
_Check_return_ HRESULT InteractionCollection::HasInteractionForEvent(KnownEventIndex eventId, _In_ CUIElement* coreSender, _Out_ bool& hasInteraction)
{
    ASSERT(static_cast<unsigned int>(eventId) <= static_cast<unsigned int>(LastControlEvent));

    // If it doesn't have a peer, then it won't have attached interactions.
    hasInteraction = false;

    ctl::ComPtr<DependencyObject> senderDO;
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(coreSender, &senderDO));
    if (senderDO)
    {
        ctl::ComPtr<UIElement> sender;
        IFC_RETURN(senderDO.As(&sender));

        ctl::ComPtr<wfc::IVector<xaml::InteractionBase*>> interactions;
        IFC_RETURN(sender->get_Interactions(&interactions));

        IFC_RETURN(interactions.Cast<InteractionCollection>()->HasInteractionForEventWorker(eventId, sender, hasInteraction));
    }

    return S_OK;
}

_Check_return_ HRESULT InteractionCollection::HasInteractionForEventWorker(KnownEventIndex eventId, _In_ const ctl::ComPtr<UIElement>& sender, _Out_ bool& hasInteraction)
{
    auto eventIndex = static_cast<unsigned int>(eventId);
    if (eventIndex < EventTypeCountsArraySize)
    {
        hasInteraction = (m_eventTypeCountsArray[eventIndex] > 0);
    }

    return S_OK;
}

_Check_return_ HRESULT InteractionCollection::DispatchInteraction(KnownEventIndex eventId, _In_ CUIElement* coreSender, _In_ CEventArgs* coreArgs)
{
    ASSERT(static_cast<unsigned int>(eventId) <= static_cast<unsigned int>(LastControlEvent));

    ctl::ComPtr<DependencyObject> senderDO;
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(coreSender, &senderDO));
    if (senderDO)
    {
        ctl::ComPtr<UIElement> sender;
        IFC_RETURN(senderDO.As(&sender));

        ctl::ComPtr<IInspectable> args;
        IFC_RETURN(coreArgs->GetFrameworkPeer(&args));

        ctl::ComPtr<wfc::IVector<xaml::InteractionBase*>> interactions;
        IFC_RETURN(sender->get_Interactions(&interactions));

        IFC_RETURN(interactions.Cast<InteractionCollection>()->DispatchInteractionWorker(eventId, sender, args.AsOrNull<RoutedEventArgs>()));
    }
    else
    {
        // The element should be checked for whether it has an attached interaction for the given
        // event before calling this method so it should have a peer.
        ASSERT(false);
    }

    return S_OK;
}

_Check_return_ HRESULT
InteractionCollection::DispatchInteractionWorker(
    KnownEventIndex eventId,
    _In_ const ctl::ComPtr<UIElement>& sender,
    _In_ const ctl::ComPtr<RoutedEventArgs>& args)
{
    unsigned int numInteractions = 0;
    IFC_RETURN(get_Size(&numInteractions));
    for (unsigned int i = 0; i < numInteractions; ++i)
    {
        ctl::ComPtr<xaml::IInteractionBase> interaction;
        IFC_RETURN(m_collection->GetAt(i, interaction.GetAddressOf()));
        if (interaction)
        {
            switch (eventId)
            {
            case KnownEventIndex::UIElement_KeyDown:
                ASSERT(ctl::is<IKeyRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnKeyDownProtected(sender.Get(), args.AsOrNull<IKeyRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_KeyUp:
                ASSERT(ctl::is<IKeyRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnKeyUpProtected(sender.Get(), args.AsOrNull<IKeyRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_PointerEntered:
                ASSERT(ctl::is<IPointerRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnPointerEnteredProtected(sender.Get(), args.AsOrNull<IPointerRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_PointerExited:
                ASSERT(ctl::is<IPointerRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnPointerExitedProtected(sender.Get(), args.AsOrNull<IPointerRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_PointerMoved:
                ASSERT(ctl::is<IPointerRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnPointerMovedProtected(sender.Get(), args.AsOrNull<IPointerRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_PointerPressed:
                ASSERT(ctl::is<IPointerRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnPointerPressedProtected(sender.Get(), args.AsOrNull<IPointerRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_PointerReleased:
                ASSERT(ctl::is<IPointerRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnPointerReleasedProtected(sender.Get(), args.AsOrNull<IPointerRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_PointerCaptureLost:
                ASSERT(ctl::is<IPointerRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnPointerCaptureLostProtected(sender.Get(), args.AsOrNull<IPointerRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_PointerCanceled:
                ASSERT(ctl::is<IPointerRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnPointerCanceledProtected(sender.Get(), args.AsOrNull<IPointerRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_PointerWheelChanged:
                ASSERT(ctl::is<IPointerRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnPointerWheelChangedProtected(sender.Get(), args.AsOrNull<IPointerRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_Tapped:
                ASSERT(ctl::is<ITappedRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnTappedProtected(sender.Get(), args.AsOrNull<ITappedRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_DoubleTapped:
                ASSERT(ctl::is<IDoubleTappedRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnDoubleTappedProtected(sender.Get(), args.AsOrNull<IDoubleTappedRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_Holding:
                ASSERT(ctl::is<IHoldingRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnHoldingProtected(sender.Get(), args.AsOrNull<IHoldingRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_RightTapped:
                ASSERT(ctl::is<IRightTappedRoutedEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnRightTappedProtected(sender.Get(), args.AsOrNull<IRightTappedRoutedEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_DragEnter:
                ASSERT(ctl::is<IDragEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnDragEnterProtected(sender.Get(), args.AsOrNull<IDragEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_DragLeave:
                ASSERT(ctl::is<IDragEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnDragLeaveProtected(sender.Get(), args.AsOrNull<IDragEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_DragOver:
                ASSERT(ctl::is<IDragEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnDragOverProtected(sender.Get(), args.AsOrNull<IDragEventArgs>().Get()));
                break;

            case KnownEventIndex::UIElement_Drop:
                ASSERT(ctl::is<IDragEventArgs>(args.Get()));
                IFC_RETURN(interaction.Cast<InteractionBase>()->OnDropProtected(sender.Get(), args.AsOrNull<IDragEventArgs>().Get()));
                break;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT InteractionCollection::EnsureCollection()
{
    if (!m_collection)
    {
        ctl::ComPtr<TrackerCollection<xaml::InteractionBase*>> collection;
        IFC_RETURN(ctl::make(&collection));
        SetPtrValue(m_collection, collection);
    }

    return S_OK;
}

_Check_return_ HRESULT InteractionCollection::OnInteractionAdded(_In_opt_ xaml::IInteractionBase* interaction)
{
    if (interaction != nullptr)
    {
        ctl::ComPtr<wfc::IVectorView<xaml::RoutedEvent*>> supportedEvents;
        IFC_RETURN(interaction->GetSupportedEvents(supportedEvents.GetAddressOf()));

        unsigned int numEvents = 0;
        IFC_RETURN(supportedEvents->get_Size(&numEvents));
        if (numEvents > 0)
        {
            unsigned int actualSize = 0;
            auto buffer = new xaml::IRoutedEvent*[numEvents];
            auto cleanupBuffer = wil::scope_exit([&]()
            {
                ASSERT(buffer != nullptr);
                for (unsigned int i = 0; i < actualSize; ++i)
                {
                    ReleaseInterface(buffer[i]);
                }
                delete[] buffer;
                buffer = nullptr;
            });

            IFC_RETURN(supportedEvents->GetMany(0, numEvents, buffer, &actualSize));

            for (unsigned int i = 0; i < actualSize; ++i)
            {
                auto routedEvent = static_cast<RoutedEvent*>(buffer[i]);
                if (routedEvent)
                {
                    auto eventId = static_cast<unsigned int>(routedEvent->GetEventId());
                    if (eventId < EventTypeCountsArraySize)
                    {
                        m_eventTypeCountsArray[eventId] += 1;
                    }
                }
            }
        }
    }

    ctl::ComPtr<UIElement> owner;
    IFC_RETURN(m_wrOwner.As(&owner));
    ASSERT(owner);

    owner->GetHandle()->SetHasAttachedInteractions(true);

    return S_OK;
}

_Check_return_ HRESULT InteractionCollection::OnInteractionRemoved(_In_opt_ xaml::IInteractionBase* interaction)
{
    if (interaction != nullptr)
    {
        ctl::ComPtr<wfc::IVectorView<xaml::RoutedEvent*>> supportedEvents;
        IFC_RETURN(interaction->GetSupportedEvents(supportedEvents.GetAddressOf()));

        unsigned int numEvents = 0;
        IFC_RETURN(supportedEvents->get_Size(&numEvents));
        if (numEvents > 0)
        {
            unsigned int actualSize = 0;
            auto buffer = new xaml::IRoutedEvent*[numEvents];
            auto cleanupBuffer = wil::scope_exit([&]()
            {
                ASSERT(buffer != nullptr);
                for (unsigned int i = 0; i < actualSize; ++i)
                {
                    ReleaseInterface(buffer[i]);
                }
                delete[] buffer;
                buffer = nullptr;
            });

            IFC_RETURN(supportedEvents->GetMany(0, numEvents, buffer, &actualSize));

            for (unsigned int i = 0; i < actualSize; ++i)
            {
                auto routedEvent = static_cast<RoutedEvent*>(buffer[i]);
                if (routedEvent)
                {
                    auto eventId = static_cast<unsigned int>(routedEvent->GetEventId());
                    if (eventId < EventTypeCountsArraySize && m_eventTypeCountsArray[eventId] > 0)
                    {
                        m_eventTypeCountsArray[eventId] -= 1;
                    }
                }
            }
        }
    }

    unsigned int size = 0;
    IFC_RETURN(get_Size(&size));
    if (size == 0)
    {
        ctl::ComPtr<UIElement> owner;
        IFC_RETURN(m_wrOwner.As(&owner));
        ASSERT(owner);

        owner->GetHandle()->SetHasAttachedInteractions(false);
    }

    return S_OK;
}

_Check_return_ HRESULT InteractionCollection::OnInteractionsCleared()
{
    for (unsigned int i = 0; i < m_eventTypeCountsArray.size(); ++i)
    {
        m_eventTypeCountsArray[i] = 0;
    }

    ctl::ComPtr<UIElement> owner;
    IFC_RETURN(m_wrOwner.As(&owner));
    ASSERT(owner);

    owner->GetHandle()->SetHasAttachedInteractions(false);

    return S_OK;
}
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
