// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Centralized for event callbacks for all events in the system

#pragma once

// needed for the ITypedEventHandler specialization
#include <windows.graphics.display.h>
#include <TrackerPtr.h>
#include <fwd/windows.ui.core.h>
#include <fwd/Microsoft.UI.Xaml.interop.h>

class CDependencyProperty;

namespace DirectUI
{
    struct IModernCollectionBasePanel;
    struct IDPChangedEventHandler;

    // Helper method for detaching events from EventPtr.
    template <class Callback, class EventInterface>
    static _Check_return_ HRESULT DetachHandler(
        _In_ ctl::EventPtr<Callback>& eventPtr,
        _In_ TrackerPtr<EventInterface>& tpHandler)
    {
        HRESULT hr = S_OK;

        if (eventPtr)
        {
            auto spRef = tpHandler.GetSafeReference();

            if (spRef)
            {
                IFC(eventPtr.DetachEventHandler(spRef.Get()));
            }
    }

    Cleanup:
        return hr;
    }

    // Callback for INotifyCollectionChanged::CollectionChanged event
    struct CollectionChangedTraits
    {
        typedef xaml_interop::INotifyCollectionChanged event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_interop::INotifyCollectionChanged *pSource,
            xaml_interop::INotifyCollectionChangedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_CollectionChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_interop::INotifyCollectionChanged *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_CollectionChanged(token);
        }
    };
    typedef ctl::event_handler<
        xaml_interop::INotifyCollectionChangedEventHandler,
        IInspectable,
        xaml_interop::INotifyCollectionChangedEventArgs,
        CollectionChangedTraits> CollectionChangedEventCallback;

    // Callback for IBindableObservableVector::VectorChanged event
    struct BindableVectorChangedTraits
    {
        typedef xaml_interop::IBindableObservableVector event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_interop::IBindableObservableVector *pSource,
            xaml_interop::IBindableVectorChangedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_VectorChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_interop::IBindableObservableVector *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_VectorChanged(token);
        }
    };

    typedef ctl::event_handler<
        xaml_interop::IBindableVectorChangedEventHandler,
        xaml_interop::IBindableObservableVector,
        IInspectable,
        BindableVectorChangedTraits> BindableVectorChangedEventCallback;

    struct SelectionChangedTraits
    {
         typedef xaml_primitives::ISelector event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_primitives::ISelector *pSource,
            xaml_controls::ISelectionChangedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_SelectionChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_primitives::ISelector *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_SelectionChanged(token);
        }
    };

    typedef ctl::event_handler<
        xaml_controls::ISelectionChangedEventHandler,
        IInspectable,
        xaml_controls::ISelectionChangedEventArgs,
        SelectionChangedTraits> SelectionChangedEventCallback;

    // Callback for IObservableVector::VectorChanged event
    struct VectorChangedTraits
    {
        typedef wfc::IObservableVector<IInspectable *> event_interface;

        static _Check_return_ HRESULT attach_handler(
            wfc::IObservableVector<IInspectable *> *pSource,
            wfc::VectorChangedEventHandler<IInspectable *> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_VectorChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            wfc::IObservableVector<IInspectable *> *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_VectorChanged(token);
        }
    };
    typedef ctl::event_handler<
            wfc::VectorChangedEventHandler<IInspectable *>,
            wfc::IObservableVector<IInspectable *>,
            wfc::IVectorChangedEventArgs,
            VectorChangedTraits> VectorChangedEventCallback;

    // Callback for IObservableMap::MapChanged event
    struct MapChangedTraits
    {
        typedef wfc::IObservableMap<HSTRING, IInspectable *> event_interface;

        static _Check_return_ HRESULT attach_handler(
            _In_ wfc::IObservableMap<HSTRING, IInspectable *> *pSource,
            _In_ wfc::MapChangedEventHandler<HSTRING, IInspectable *> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_MapChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            _In_ wfc::IObservableMap<HSTRING, IInspectable *> *pSource,
            _In_ EventRegistrationToken token)
        {
            return pSource->remove_MapChanged(token);
        }
    };
    typedef ctl::event_handler<
        wfc::MapChangedEventHandler<HSTRING, IInspectable *>,
        wfc::IObservableMap<HSTRING, IInspectable *>,
        wfc::IMapChangedEventArgs<HSTRING>,
        MapChangedTraits> MapChangedEventCallback;

    // Callback for ICollectionView::CurrentChanged event
    struct CurrentChangedTraits
    {
        typedef xaml_data::ICollectionView event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_data::ICollectionView *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_CurrentChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_data::ICollectionView *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_CurrentChanged(token);
        }
    };
    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        CurrentChangedTraits> CurrentChangedEventCallback;

    // Callback for ICommand::CanExecuteChanged
    template <class TIFACE, class THANDLER>
    struct CommandCanExecuteChangedTraits
    {
        typedef TIFACE event_interface;

        static _Check_return_ HRESULT attach_handler(
            TIFACE *pSource,
            THANDLER *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_CanExecuteChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            TIFACE *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_CanExecuteChanged(token);
        }
    };


    // Use a weak subscription for CanExecuteChanged, so that the command doesn't cause a cycle.
    typedef ctl::weak_event_handler<
            wf::IEventHandler<IInspectable*>,
            IInspectable,
            IInspectable,
            CommandCanExecuteChangedTraits<
                xaml_input::ICommand,
                wf::IEventHandler<IInspectable*>>> CommandCanExecuteChangedCallback;


    // Callback for INotifyPropertyChanged::PropertyChanged event (Also the IMarshallNotifyPropertyChanged::PropertyChanged)
    template <class TIFACE, class THANDLER>
    struct NotifyPropertyChangedTraits
    {
        typedef TIFACE event_interface;

        static _Check_return_ HRESULT attach_handler(
            TIFACE *pSource,
            THANDLER *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_PropertyChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            TIFACE *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_PropertyChanged(token);
        }
    };

    typedef ctl::event_handler<
            xaml_data::IPropertyChangedEventHandler,
            IInspectable,
            xaml_data::IPropertyChangedEventArgs,
            NotifyPropertyChangedTraits<
                xaml_data::INotifyPropertyChanged,
                xaml_data::IPropertyChangedEventHandler>> PropertyChangedEventCallback;

    interface IDPChangedEventHandler;
    struct DependencyPropertyChangedTraits
    {
        typedef xaml::IDependencyObject event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface* pSource,
            DirectUI::IDPChangedEventHandler* pHandler);

        static _Check_return_ HRESULT detach_handler(
            event_interface* pSource,
            DirectUI::IDPChangedEventHandler* pHandler);
    };

    typedef ctl::tokenless_event_handler<
        DirectUI::IDPChangedEventHandler,
        xaml::IDependencyObject,
        const CDependencyProperty,
        DependencyPropertyChangedTraits> DependencyPropertyChangedCallback;

    struct TimelineCompletedTraits
    {
        typedef xaml_animation::ITimeline event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Completed(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Completed(token);
        }

    };

    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        TimelineCompletedTraits> TimelineCompletedEventCallback;

    // Callback for IFrameworkElement::Loaded event

    struct FrameworkElementLoadedTraits
    {
        typedef xaml::IFrameworkElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::IRoutedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Loaded(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Loaded(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs,
        FrameworkElementLoadedTraits> FrameworkElementLoadedEventCallback;

    // Callback for IFrameworkElement::Unloaded event

    struct FrameworkElementUnloadedTraits
    {
        typedef xaml::IFrameworkElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::IRoutedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Unloaded(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Unloaded(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs,
        FrameworkElementUnloadedTraits> FrameworkElementUnloadedEventCallback;

    // Callback for IScrollViewer::ViewChanged event
    struct ViewChangedTraits
    {
        typedef xaml_controls::IScrollViewer event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_controls::IScrollViewer *pSource,
            wf::IEventHandler<xaml_controls::ScrollViewerViewChangedEventArgs*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_ViewChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_controls::IScrollViewer *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_ViewChanged(token);
        }
    };
    typedef ctl::event_handler<
        wf::IEventHandler<xaml_controls::ScrollViewerViewChangedEventArgs*>,
        IInspectable,
        xaml_controls::IScrollViewerViewChangedEventArgs,
        ViewChangedTraits> ViewChangedEventCallback;

    // Callback for IScrollViewer::ViewChanging event
    struct ViewChangingTraits
    {
        typedef xaml_controls::IScrollViewer event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_controls::IScrollViewer *pSource,
            wf::IEventHandler<xaml_controls::ScrollViewerViewChangingEventArgs*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_ViewChanging(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_controls::IScrollViewer *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_ViewChanging(token);
        }
    };
    typedef ctl::event_handler<
        wf::IEventHandler<xaml_controls::ScrollViewerViewChangingEventArgs*>,
        IInspectable,
        xaml_controls::IScrollViewerViewChangingEventArgs,
        ViewChangingTraits> ViewChangingEventCallback;

    // Callback for IFrameworkElement::SizeChanged event

    struct FrameworkElementSizeChangedTraits
    {
        typedef xaml::IFrameworkElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::ISizeChangedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_SizeChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_SizeChanged(token);
        }
    };

    typedef ctl::event_handler<
        xaml::ISizeChangedEventHandler,
        IInspectable,
        xaml::ISizeChangedEventArgs,
        FrameworkElementSizeChangedTraits> FrameworkElementSizeChangedEventCallback;

    // Callback for IFrameworkElement::LayoutUpdated event

    struct FrameworkElementLayoutUpdatedTraits
    {
        typedef xaml::IFrameworkElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_LayoutUpdated(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_LayoutUpdated(token);
        }
    };

    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        FrameworkElementLayoutUpdatedTraits> FrameworkElementLayoutUpdatedEventCallback;


    struct FrameworkElementActualThemeChangedTraits
    {
        typedef xaml::IFrameworkElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<xaml::FrameworkElement*, IInspectable*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_ActualThemeChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_ActualThemeChanged(token);
        }
    };
    
    typedef ctl::event_handler<
        wf::ITypedEventHandler<xaml::FrameworkElement*, IInspectable*>,
        xaml::IFrameworkElement,
        IInspectable,
        FrameworkElementActualThemeChangedTraits> FrameworkElementActualThemeChangedEventCallback;

    // Callback for IButtonBase::Click event

    struct ButtonBaseClickTraits
    {
        typedef xaml_primitives::IButtonBase event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::IRoutedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Click(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Click(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs,
        ButtonBaseClickTraits> ButtonBaseClickEventCallback;

    // Callback for IRangeBase::ValueChanged event

    struct RangeBaseValueChangedTraits
    {
        typedef xaml_primitives::IRangeBase event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_primitives::IRangeBaseValueChangedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_ValueChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_ValueChanged(token);
        }
    };

    typedef ctl::event_handler<
        xaml_primitives::IRangeBaseValueChangedEventHandler,
        IInspectable,
        xaml_primitives::IRangeBaseValueChangedEventArgs,
        RangeBaseValueChangedTraits> RangeBaseValueChangedEventCallback;

    // Callback for IUIElement::PointerMoved event

    struct UIElementPointerMovedTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::IPointerEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_PointerMoved(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_PointerMoved(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::IPointerEventHandler,
        IInspectable,
        xaml_input::IPointerRoutedEventArgs,
        UIElementPointerMovedTraits> UIElementPointerMovedEventCallback;

    // Callback for IUIElement::PointerPressed event

    struct UIElementPointerPressedTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::IPointerEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_PointerPressed(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_PointerPressed(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::IPointerEventHandler,
        IInspectable,
        xaml_input::IPointerRoutedEventArgs,
        UIElementPointerPressedTraits> UIElementPointerPressedEventCallback;

    // Callback for IUIElement::PointerReleased event

    struct UIElementPointerReleasedTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::IPointerEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_PointerReleased(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_PointerReleased(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::IPointerEventHandler,
        IInspectable,
        xaml_input::IPointerRoutedEventArgs,
        UIElementPointerReleasedTraits> UIElementPointerReleasedEventCallback;

    // Callback for IUIElement::PointerEntered event

    struct UIElementPointerEnteredTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::IPointerEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_PointerEntered(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_PointerEntered(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::IPointerEventHandler,
        IInspectable,
        xaml_input::IPointerRoutedEventArgs,
        UIElementPointerEnteredTraits> UIElementPointerEnteredEventCallback;

    // Callback for IUIElement::PointerExited event

    struct UIElementPointerExitedTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::IPointerEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_PointerExited(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_PointerExited(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::IPointerEventHandler,
        IInspectable,
        xaml_input::IPointerRoutedEventArgs,
        UIElementPointerExitedTraits> UIElementPointerExitedEventCallback;

    // Callback for IUIElement::PointerCaptureLost event

    struct UIElementPointerCaptureLostTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::IPointerEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_PointerCaptureLost(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_PointerCaptureLost(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::IPointerEventHandler,
        IInspectable,
        xaml_input::IPointerRoutedEventArgs,
        UIElementPointerCaptureLostTraits> UIElementPointerCaptureLostEventCallback;

    // Callback for IUIElement::PointerWheelChanged event

    struct UIElementPointerWheelChangedTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::IPointerEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_PointerWheelChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_PointerWheelChanged(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::IPointerEventHandler,
        IInspectable,
        xaml_input::IPointerRoutedEventArgs,
        UIElementPointerWheelChangedTraits> UIElementPointerWheelChangedEventCallback;

    // Callback for IUIElement::GotFocus event
    struct UIElementGotFocusTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::IRoutedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_GotFocus(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_GotFocus(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs,
        UIElementGotFocusTraits> UIElementGotFocusEventCallback;

    // Callback for IUIElement::GettingFocus event
    struct UIElementGettingFocusTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<xaml::UIElement*, xaml_input::GettingFocusEventArgs*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_GettingFocus(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_GettingFocus(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<xaml::UIElement*, xaml_input::GettingFocusEventArgs*>,
        xaml::IUIElement,
        xaml_input::IGettingFocusEventArgs,
        UIElementGettingFocusTraits> UIElementGettingFocusEventCallback;

    // Callback for IUIElement::LostFocus event
    struct UIElementLostFocusTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::IRoutedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_LostFocus(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_LostFocus(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs,
        UIElementLostFocusTraits> UIElementLostFocusEventCallback;

    // Callback for IUIElement::LosingFocus event
    struct UIElementLosingFocusTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<xaml::UIElement*, xaml_input::LosingFocusEventArgs*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_LosingFocus(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_LosingFocus(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<xaml::UIElement*, xaml_input::LosingFocusEventArgs*>,
        xaml::IUIElement,
        xaml_input::ILosingFocusEventArgs,
        UIElementLosingFocusTraits> UIElementLosingFocusEventCallback;

    // Callback for IUIElement::BringIntoViewRequested event
    struct UIElementBringIntoViewRequestedTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<xaml::UIElement*, xaml::BringIntoViewRequestedEventArgs*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_BringIntoViewRequested(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_BringIntoViewRequested(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<xaml::UIElement*, xaml::BringIntoViewRequestedEventArgs*>,
        xaml::IUIElement,
        xaml::IBringIntoViewRequestedEventArgs,
        UIElementBringIntoViewRequestedTraits> UIElementBringIntoViewRequestedEventCallback;

    // Callback for IUIElement::KeyDown event

    struct UIElementKeyDownTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::IKeyEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_KeyDown(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_KeyDown(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::IKeyEventHandler,
        IInspectable,
        xaml_input::IKeyRoutedEventArgs,
        UIElementKeyDownTraits> UIElementKeyDownEventCallback;

    // Callback for IUIElement::KeyUp event

    struct UIElementKeyUpTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::IKeyEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_KeyUp(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_KeyUp(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::IKeyEventHandler,
        IInspectable,
        xaml_input::IKeyRoutedEventArgs,
        UIElementKeyUpTraits> UIElementKeyUpEventCallback;

    // Callback for IUIElement::PreviewKeyDown event

    struct UIElementPreviewKeyDownTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::IKeyEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_PreviewKeyDown(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_PreviewKeyDown(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::IKeyEventHandler,
        IInspectable,
        xaml_input::IKeyRoutedEventArgs,
        UIElementPreviewKeyDownTraits> UIElementPreviewKeyDownEventCallback;

    // Callback for IDispatcherTimer::Tick event

    struct DispatcherTimerTickTraits
    {
        typedef xaml::IDispatcherTimer event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Tick(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Tick(token);
        }
    };

    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        DispatcherTimerTickTraits> DispatcherTimerTickEventCallback;

    // Callback for ITextBox::TextChanged event

    struct TextBoxTextChangedTraits
    {
        typedef xaml_controls::ITextBox event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_controls::ITextChangedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_TextChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_TextChanged(token);
        }
    };

    typedef ctl::event_handler<
        xaml_controls::ITextChangedEventHandler,
        IInspectable,
        xaml_controls::ITextChangedEventArgs,
        TextBoxTextChangedTraits> TextBoxTextChangedEventCallback;

    // Callback for ITextBox::CandidateWindowBoundsChanged event

    struct TextBoxCandidateWindowBoundsChangedTraits
    {
        typedef xaml_controls::ITextBox event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<
                xaml_controls::TextBox*,
                xaml_controls::CandidateWindowBoundsChangedEventArgs*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_CandidateWindowBoundsChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_CandidateWindowBoundsChanged(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<
            xaml_controls::TextBox*,
            xaml_controls::CandidateWindowBoundsChangedEventArgs*>,
        xaml_controls::ITextBox,
        xaml_controls::ICandidateWindowBoundsChangedEventArgs,
        TextBoxCandidateWindowBoundsChangedTraits> TextBoxCandidateWindowBoundsChangedEventCallback;

    // Callback for IWindow::SizeChanged event

    struct WindowSizeChangedTraits
    {
        typedef xaml::IWindow event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_SizeChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_SizeChanged(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*>,
        IInspectable,
        xaml::IWindowSizeChangedEventArgs,
        WindowSizeChangedTraits> WindowSizeChangedEventCallback;

    // Callback for IWindow::Activated event

    struct WindowActivatedTraits
    {
        typedef xaml::IWindow event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Activated(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Activated(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>,
        IInspectable,
        xaml::IWindowActivatedEventArgs,
        WindowActivatedTraits> WindowActivatedEventCallback;

    // Callback for IXamlRoot::Changed event
    
    struct XamlRootChangedTraits
    {
        typedef xaml::IXamlRoot event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<xaml::XamlRoot*, xaml::XamlRootChangedEventArgs*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Changed(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Changed(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<xaml::XamlRoot*, xaml::XamlRootChangedEventArgs*>,
        xaml::IXamlRoot,
        xaml::IXamlRootChangedEventArgs,
        XamlRootChangedTraits> XamlRootChangedEventCallback;

    // Callback for IListViewBase::ContainerContentChanging event
    struct ListViewBaseContainerContentChangingTraits
   {
        typedef xaml_controls::IListViewBase event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<
                xaml_controls::ListViewBase*,
                xaml_controls::ContainerContentChangingEventArgs*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_ContainerContentChanging(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_ContainerContentChanging(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<
            xaml_controls::ListViewBase*,
            xaml_controls::ContainerContentChangingEventArgs*>,
        xaml_controls::IListViewBase,
        xaml_controls::IContainerContentChangingEventArgs,
        ListViewBaseContainerContentChangingTraits> ListViewBaseContainerContentChangingEventCallback;

    // Callback for IWindow::SizeChanged event

    struct ListViewBaseItemClickTraits
    {
        typedef xaml_controls::IListViewBase event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_controls::IItemClickEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_ItemClick(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_ItemClick(token);
        }
    };

    typedef ctl::event_handler<
        xaml_controls::IItemClickEventHandler,
        IInspectable,
        xaml_controls::IItemClickEventArgs,
        ListViewBaseItemClickTraits> ListViewBaseItemClickEventCallback;

    // Callback for IUIElement::Tapped event

    struct UIElementTappedTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml_input::ITappedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Tapped(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Tapped(token);
        }
    };

    typedef ctl::event_handler<
        xaml_input::ITappedEventHandler,
        IInspectable,
        xaml_input::ITappedRoutedEventArgs,
        UIElementTappedTraits> UIElementTappedEventCallback;

    // Callback for IMenuFlyoutItem::Click event

    struct MenuFlyoutItemClickTraits
    {
        typedef xaml_controls::IMenuFlyoutItem event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::IRoutedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Click(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Click(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs,
        MenuFlyoutItemClickTraits> MenuFlyoutItemClickEventCallback;

    // Callback for IVisualStateGroup::CurrentStateChanging
    struct VisualStateGroupCurrentStateChangingTraits
    {
        typedef xaml::IVisualStateGroup event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface* pSource,
            xaml::IVisualStateChangedEventHandler* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_CurrentStateChanging(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_CurrentStateChanging(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IVisualStateChangedEventHandler,
        IInspectable,
        xaml::IVisualStateChangedEventArgs,
        VisualStateGroupCurrentStateChangingTraits> VisualStateGroupCurrentStateChangingEventCallback;

    // Callback for IVisualStateGroup::CurrentStateChanged
    struct VisualStateGroupCurrentStateChangedTraits
    {
        typedef xaml::IVisualStateGroup event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface* pSource,
            xaml::IVisualStateChangedEventHandler* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_CurrentStateChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_CurrentStateChanged(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IVisualStateChangedEventHandler,
        IInspectable,
        xaml::IVisualStateChangedEventArgs,
        VisualStateGroupCurrentStateChangedTraits> VisualStateGroupCurrentStateChangedEventCallback;

    // Callback for Storyboard::Completedevent
    struct StoryboardCompletedTraits
    {
        typedef xaml_animation::IStoryboard event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken);

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token);
    };

    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        StoryboardCompletedTraits> StoryboardCompletedEventCallback;

    // Callback for IImageBrush::ImageOpened event
    struct ImageBrushImageOpenedTraits
    {
        typedef xaml_media::IImageBrush event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::IRoutedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_ImageOpened(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_ImageOpened(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs,
        ImageBrushImageOpenedTraits> ImageBrushImageOpenedEventCallback;

    // Callback for IImageBrush::ImageFailed event
    struct ImageBrushImageFailedTraits
    {
        typedef xaml_media::IImageBrush event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::IExceptionRoutedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_ImageFailed(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_ImageFailed(token);
        }
    };
    typedef ctl::event_handler<
        xaml::IExceptionRoutedEventHandler,
        IInspectable,
        xaml::IExceptionRoutedEventArgs,
        ImageBrushImageFailedTraits> ImageBrushImageFailedEventCallback;

    // Callback for IToggleButton::Checked event
    struct ToggleButtonCheckedTraits
    {
        typedef xaml_primitives::IToggleButton event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::IRoutedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Checked(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Checked(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs,
        ToggleButtonCheckedTraits> ToggleButtonCheckedEventCallback;

    // Callback for IToggleButton::Unchecked event
    struct ToggleButtonUncheckedTraits
    {
        typedef xaml_primitives::IToggleButton event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            xaml::IRoutedEventHandler *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Unchecked(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Unchecked(token);
        }
    };

    typedef ctl::event_handler<
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs,
        ToggleButtonUncheckedTraits> ToggleButtonUncheckedEventCallback;

    // Callback for IScrollSnapPointsInfo::HorizontalSnapPointsChanged
    struct ScrollSnapPointsInfoHorizontalSnapPointsChangedTraits
    {
        typedef xaml_primitives::IScrollSnapPointsInfo event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_HorizontalSnapPointsChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_HorizontalSnapPointsChanged(token);
        }
    };

    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        ScrollSnapPointsInfoHorizontalSnapPointsChangedTraits> ScrollSnapPointsInfoHorizontalSnapPointsChangedCallback;

    // Callback for wgrd::DisplayInformation::OrientationChanged evetn
    struct DisplayInformationOrientationChangedTraits
    {
        typedef wgrd::IDisplayInformation event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<wgrd::DisplayInformation*, IInspectable*> *pHandler,
            EventRegistrationToken *pToken);

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token);
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<wgrd::DisplayInformation*, IInspectable*>,
        wgrd::IDisplayInformation,
        IInspectable,
        DisplayInformationOrientationChangedTraits> DisplayInformationOrientationChangedCallback;


    // Callback for ModernCollectionBasePanel::VisibleIndicesUpdated event
    struct VisibleIndicesUpdatedTraits
    {
        typedef IModernCollectionBasePanel event_interface;

        static _Check_return_ HRESULT attach_handler(
            IModernCollectionBasePanel *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken);

        static _Check_return_ HRESULT detach_handler(
            IModernCollectionBasePanel *pSource,
            EventRegistrationToken token);
    };

    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        VisibleIndicesUpdatedTraits> VisibleIndicesUpdatedEventCallback;


    // Callback for IObservableVector<DateTime>::VectorChanged event
    struct DateTimeVectorChangedTraits
    {
        typedef wfc::IObservableVector<wf::DateTime> event_interface;

        static _Check_return_ HRESULT attach_handler(
            wfc::IObservableVector<wf::DateTime> *pSource,
            wfc::VectorChangedEventHandler<wf::DateTime> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_VectorChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            wfc::IObservableVector<wf::DateTime> *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_VectorChanged(token);
        }
    };
    typedef ctl::event_handler<
        wfc::VectorChangedEventHandler<wf::DateTime>,
        wfc::IObservableVector<wf::DateTime>,
        wfc::IVectorChangedEventArgs,
        DateTimeVectorChangedTraits> DateTimeVectorChangedEventCallback;

    // Callback for IFlyoutBase::Opened event
    struct FlyoutBaseOpenedTraits
    {
        typedef xaml_primitives::IFlyoutBase event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_primitives::IFlyoutBase *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Opened(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_primitives::IFlyoutBase *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Opened(token);
        }
    };
    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        FlyoutBaseOpenedTraits> FlyoutBaseOpenedEventCallback;

    // Callback for IFlyoutBase::Closed event
    struct FlyoutBaseClosedTraits
    {
        typedef xaml_primitives::IFlyoutBase event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Closed(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Closed(token);
        }
    };
    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        FlyoutBaseClosedTraits> FlyoutBaseClosedEventCallback;

    // Callback for IFlyoutBase::Opening event
    struct FlyoutBaseOpeningTraits
    {
        typedef xaml_primitives::IFlyoutBase event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Opening(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Opening(token);
        }
    };
    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        FlyoutBaseOpeningTraits> FlyoutBaseOpeningEventCallback;

    // Callback for IFlyoutBase::Closing event
    struct FlyoutBaseClosingTraits
    {
        typedef xaml_primitives::IFlyoutBase event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<xaml_primitives::FlyoutBase*, xaml_primitives::FlyoutBaseClosingEventArgs*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Closing(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Closing(token);
        }
    };
    typedef ctl::event_handler<
        wf::ITypedEventHandler<xaml_primitives::FlyoutBase*, xaml_primitives::FlyoutBaseClosingEventArgs*>,
        xaml_primitives::IFlyoutBase,
        xaml_primitives::IFlyoutBaseClosingEventArgs,
        FlyoutBaseClosingTraits> FlyoutBaseClosingEventCallback;

    // Callback for ICalendarView::CalendarViewDayItemChanging event
    struct CalendarViewCalendarViewDayItemChangingTraits
    {
        typedef xaml_controls::ICalendarView event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_controls::ICalendarView *pSource,
            wf::ITypedEventHandler<
                xaml_controls::CalendarView*,
                xaml_controls::CalendarViewDayItemChangingEventArgs*
            > *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_CalendarViewDayItemChanging(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_controls::ICalendarView *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_CalendarViewDayItemChanging(token);
        }
    };
    typedef ctl::event_handler<
        wf::ITypedEventHandler<
            xaml_controls::CalendarView*,
            xaml_controls::CalendarViewDayItemChangingEventArgs*
        >,
        xaml_controls::ICalendarView,
        xaml_controls::ICalendarViewDayItemChangingEventArgs,
        CalendarViewCalendarViewDayItemChangingTraits> CalendarViewCalendarViewDayItemChangingCallback;

    // Callback for ICalendarView::SelectedDatesChanged event
    struct CalendarViewSelectedDatesChangedTraits
    {
        typedef xaml_controls::ICalendarView event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_controls::ICalendarView *pSource,
            wf::ITypedEventHandler<
                xaml_controls::CalendarView*,
                xaml_controls::CalendarViewSelectedDatesChangedEventArgs*
            > *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_SelectedDatesChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_controls::ICalendarView *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_SelectedDatesChanged(token);
        }
    };
    typedef ctl::event_handler<
        wf::ITypedEventHandler<
            xaml_controls::CalendarView*,
            xaml_controls::CalendarViewSelectedDatesChangedEventArgs*
        >,
        xaml_controls::ICalendarView,
        xaml_controls::ICalendarViewSelectedDatesChangedEventArgs,
        CalendarViewSelectedDatesChangedTraits> CalendarViewSelectedDatesChangedCallback;

    // Callback for IPopup::Opened event
    struct PopupOpenedTraits
    {
        typedef xaml_primitives::IPopup event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_primitives::IPopup *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Opened(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_primitives::IPopup *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Opened(token);
        }
    };

    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        PopupOpenedTraits> PopupOpenedEventCallback;

    // Callback for IPopup::Closed event
    struct PopupClosedTraits
    {
        typedef xaml_primitives::IPopup event_interface;

        static _Check_return_ HRESULT attach_handler(
            xaml_primitives::IPopup *pSource,
            wf::IEventHandler<IInspectable*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_Closed(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            xaml_primitives::IPopup *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_Closed(token);
        }
    };
    typedef ctl::event_handler<
        wf::IEventHandler<IInspectable*>,
        IInspectable,
        IInspectable,
        PopupClosedTraits> PopupClosedEventCallback;

    // Callback for IObservableVector<ICommandBarElement*>::VectorChanged event
    struct CommandBarElementVectorChangedTraits
    {
        typedef wfc::IObservableVector<xaml_controls::ICommandBarElement*> event_interface;

        static _Check_return_ HRESULT attach_handler(
            wfc::IObservableVector<xaml_controls::ICommandBarElement*> *pSource,
            wfc::VectorChangedEventHandler<xaml_controls::ICommandBarElement*> *pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_VectorChanged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            wfc::IObservableVector<xaml_controls::ICommandBarElement*> *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_VectorChanged(token);
        }
    };
    typedef ctl::event_handler<
        wfc::VectorChangedEventHandler<xaml_controls::ICommandBarElement*>,
        wfc::IObservableVector<xaml_controls::ICommandBarElement*>,
        wfc::IVectorChangedEventArgs,
        CommandBarElementVectorChangedTraits> CommandBarElementVectorChangedEventCallback;

    // Callback for IControl::FocusEngaged event
    struct ControlFocusEngagedTraits
    {
        typedef xaml_controls::IControl event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<
            xaml_controls::Control*,
            xaml_controls::FocusEngagedEventArgs*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_FocusEngaged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_FocusEngaged(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<
        xaml_controls::Control*,
        xaml_controls::FocusEngagedEventArgs*>,
        xaml_controls::IControl,
        xaml_controls::IFocusEngagedEventArgs,
        ControlFocusEngagedTraits> ControlFocusEngagedEventCallback;

    // Callback for IControl::FocusDisengaged event
    struct ControlFocusDisengagedTraits
    {
        typedef xaml_controls::IControl event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<
            xaml_controls::Control*,
            xaml_controls::FocusDisengagedEventArgs*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_FocusDisengaged(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_FocusDisengaged(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<
        xaml_controls::Control*,
        xaml_controls::FocusDisengagedEventArgs*>,
        xaml_controls::IControl,
        xaml_controls::IFocusDisengagedEventArgs,
        ControlFocusDisengagedTraits> ControlFocusDisengagedEventCallback;

    // Callback for IUIElement::ProcessKeyboardAccelerators event
    struct UIElementProcessKeyboardAcceleratorsTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<xaml::UIElement*, xaml_input::ProcessKeyboardAcceleratorEventArgs*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_ProcessKeyboardAccelerators(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_ProcessKeyboardAccelerators(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<
            xaml::UIElement*,
            xaml_input::ProcessKeyboardAcceleratorEventArgs*>,
        xaml::IUIElement,
        xaml_input::IProcessKeyboardAcceleratorEventArgs,
        UIElementProcessKeyboardAcceleratorsTraits> UIElementProcessKeyboardAcceleratorsEventCallback;

    // Callback for IUIElement::AccessKeyInvoked event
    struct UIElementAccessKeyInvokedTraits
    {
        typedef xaml::IUIElement event_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface *pSource,
            wf::ITypedEventHandler<xaml::UIElement*, xaml_input::AccessKeyInvokedEventArgs*>* pHandler,
            EventRegistrationToken *pToken)
        {
            return pSource->add_AccessKeyInvoked(pHandler, pToken);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface *pSource,
            EventRegistrationToken token)
        {
            return pSource->remove_AccessKeyInvoked(token);
        }
    };

    typedef ctl::event_handler<
        wf::ITypedEventHandler<xaml::UIElement*, xaml_input::AccessKeyInvokedEventArgs*>,
        xaml::IUIElement,
        xaml_input::IAccessKeyInvokedEventArgs,
        UIElementAccessKeyInvokedTraits> UIElementAccessKeyInvokedEventCallback;

    // Callback for IObservableVector<IValidationError*>::VectorChanged event
    struct ValidationErrorVectorChangedTraits
    {
        typedef wfc::IObservableVector<xaml_controls::InputValidationError*> event_interface;
        typedef wfc::VectorChangedEventHandler<xaml_controls::InputValidationError*> handler_interface;

        static _Check_return_ HRESULT attach_handler(
            event_interface* source,
            handler_interface *handler,
            EventRegistrationToken *token)
        {
            return source->add_VectorChanged(handler, token);
        }

        static _Check_return_ HRESULT detach_handler(
            event_interface* source,
            EventRegistrationToken token)
        {
            return source->remove_VectorChanged(token);
        }
    };

    typedef ctl::event_handler<
        ValidationErrorVectorChangedTraits::handler_interface,
        ValidationErrorVectorChangedTraits::event_interface,
        wfc::IVectorChangedEventArgs,
        ValidationErrorVectorChangedTraits> ValidationErrorsVectorChangedEventCallback;
}
