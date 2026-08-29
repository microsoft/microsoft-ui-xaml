// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FeatureFlags.h"
#include <collection.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layering {

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018
    ref class CustomFrameworkElement sealed : public xaml::FrameworkElement
    {
    public:
        CustomFrameworkElement() {}
        virtual ~CustomFrameworkElement() {}
    };

    ref class CustomFrameworkElementEx sealed : public xaml::FrameworkElementEx
    {
    public:
        CustomFrameworkElementEx() {}
        virtual ~CustomFrameworkElementEx() {}

        xaml_controls::UIElementCollection^ GetChildren() { return Children; }
    };

    ref class KeyInteractions sealed : public xaml::InteractionBase
    {
    public:
        KeyInteractions() {}
        virtual ~KeyInteractions() {}

        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::KeyRoutedEventArgs^>^ KeyDown;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::KeyRoutedEventArgs^>^ KeyUp;

    protected:
        wfc::IVectorView<xaml::RoutedEvent^>^ GetSupportedEventsCore() override
        {
            auto vector = ref new Platform::Collections::Vector<xaml::RoutedEvent^>();

            vector->Append(xaml::UIElement::KeyDownEvent);
            vector->Append(xaml::UIElement::KeyUpEvent);

            return vector->GetView();
        }

        void OnKeyDown(xaml::UIElement^ sender, xaml_input::KeyRoutedEventArgs^ args) override { KeyDown(sender, args); }
        void OnKeyUp(xaml::UIElement^ sender, xaml_input::KeyRoutedEventArgs^ args) override { KeyUp(sender, args); }
    };

    ref class PointerInteractions sealed : public xaml::InteractionBase
    {
    public:
        PointerInteractions()
            : m_captureOnPointerPressed(false)
        {}

        virtual ~PointerInteractions() {}

        void CaptureOnPointerPressed(bool capture) { m_captureOnPointerPressed = capture; }

        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::PointerRoutedEventArgs^>^ PointerEntered;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::PointerRoutedEventArgs^>^ PointerExited;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::PointerRoutedEventArgs^>^ PointerMoved;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::PointerRoutedEventArgs^>^ PointerPressed;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::PointerRoutedEventArgs^>^ PointerReleased;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::PointerRoutedEventArgs^>^ PointerCaptureLost;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::PointerRoutedEventArgs^>^ PointerCanceled;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::PointerRoutedEventArgs^>^ PointerWheelChanged;

    protected:
        wfc::IVectorView<xaml::RoutedEvent^>^ GetSupportedEventsCore() override
        {
            auto vector = ref new Platform::Collections::Vector<xaml::RoutedEvent^>();

            vector->Append(xaml::UIElement::PointerEnteredEvent);
            vector->Append(xaml::UIElement::PointerExitedEvent);
            vector->Append(xaml::UIElement::PointerMovedEvent);
            vector->Append(xaml::UIElement::PointerPressedEvent);
            vector->Append(xaml::UIElement::PointerReleasedEvent);
            vector->Append(xaml::UIElement::PointerCaptureLostEvent);
            vector->Append(xaml::UIElement::PointerCanceledEvent);
            vector->Append(xaml::UIElement::PointerWheelChangedEvent);

            return vector->GetView();
        }

        void OnPointerEntered(xaml::UIElement^ sender, xaml_input::PointerRoutedEventArgs^ args) override { PointerEntered(sender, args); }
        void OnPointerExited(xaml::UIElement^ sender, xaml_input::PointerRoutedEventArgs^ args) override { PointerExited(sender, args); }
        void OnPointerMoved(xaml::UIElement^ sender, xaml_input::PointerRoutedEventArgs^ args) override { PointerMoved(sender, args); }
        void OnPointerPressed(xaml::UIElement^ sender, xaml_input::PointerRoutedEventArgs^ args) override
        {
            PointerPressed(sender, args);

            if (m_captureOnPointerPressed)
            {
                sender->CapturePointer(args->Pointer);
            }
        }
        void OnPointerReleased(xaml::UIElement^ sender, xaml_input::PointerRoutedEventArgs^ args) override { PointerReleased(sender, args); }
        void OnPointerCaptureLost(xaml::UIElement^ sender, xaml_input::PointerRoutedEventArgs^ args) override { PointerCaptureLost(sender, args); }
        void OnPointerCanceled(xaml::UIElement^ sender, xaml_input::PointerRoutedEventArgs^ args) override { PointerCanceled(sender, args); }
        void OnPointerWheelChanged(xaml::UIElement^ sender, xaml_input::PointerRoutedEventArgs^ args) override { PointerWheelChanged(sender, args); }

    private:
        bool m_captureOnPointerPressed;
    };

    ref class TouchInteractions sealed : public xaml::InteractionBase
    {
    public:
        TouchInteractions() {}
        virtual ~TouchInteractions() {}

        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::TappedRoutedEventArgs^>^ Tapped;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::DoubleTappedRoutedEventArgs^>^ DoubleTapped;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::HoldingRoutedEventArgs^>^ Holding;
        event wf::TypedEventHandler<xaml::UIElement^, xaml_input::RightTappedRoutedEventArgs^>^ RightTapped;

    protected:
        wfc::IVectorView<xaml::RoutedEvent^>^ GetSupportedEventsCore() override
        {
            auto vector = ref new Platform::Collections::Vector<xaml::RoutedEvent^>();

            vector->Append(xaml::UIElement::TappedEvent);
            vector->Append(xaml::UIElement::DoubleTappedEvent);
            vector->Append(xaml::UIElement::HoldingEvent);
            vector->Append(xaml::UIElement::RightTappedEvent);

            return vector->GetView();
        }

        void OnTapped(xaml::UIElement^ sender, xaml_input::TappedRoutedEventArgs^ args) override { Tapped(sender, args); }
        void OnDoubleTapped(xaml::UIElement^ sender, xaml_input::DoubleTappedRoutedEventArgs^ args) override { DoubleTapped(sender, args); }
        void OnHolding(xaml::UIElement^ sender, xaml_input::HoldingRoutedEventArgs^ args) override { Holding(sender, args); }
        void OnRightTapped(xaml::UIElement^ sender, xaml_input::RightTappedRoutedEventArgs^ args) override { RightTapped(sender, args); }
    };

    ref class DragDropInteractions sealed : public xaml::InteractionBase
    {
    public:
        DragDropInteractions() {}
        virtual ~DragDropInteractions() {}

        event wf::TypedEventHandler<xaml::UIElement^, xaml::DragEventArgs^>^ DragEnter;
        event wf::TypedEventHandler<xaml::UIElement^, xaml::DragEventArgs^>^ DragLeave;
        event wf::TypedEventHandler<xaml::UIElement^, xaml::DragEventArgs^>^ DragOver;
        event wf::TypedEventHandler<xaml::UIElement^, xaml::DragEventArgs^>^ Drop;

    protected:
        wfc::IVectorView<xaml::RoutedEvent^>^ GetSupportedEventsCore() override
        {
            auto vector = ref new Platform::Collections::Vector<xaml::RoutedEvent^>();

            vector->Append(xaml::UIElement::DragEnterEvent);
            vector->Append(xaml::UIElement::DragLeaveEvent);
            vector->Append(xaml::UIElement::DragOverEvent);
            vector->Append(xaml::UIElement::DropEvent);

            return vector->GetView();
        }

        void OnDragEnter(xaml::UIElement^ sender, xaml::DragEventArgs^ args) override { DragEnter(sender, args); }
        void OnDragLeave(xaml::UIElement^ sender, xaml::DragEventArgs^ args) override { DragLeave(sender, args); }
        void OnDragOver(xaml::UIElement^ sender, xaml::DragEventArgs^ args) override { DragOver(sender, args); }
        void OnDrop(xaml::UIElement^ sender, xaml::DragEventArgs^ args) override { Drop(sender, args); }
    };
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

} } } } } }
