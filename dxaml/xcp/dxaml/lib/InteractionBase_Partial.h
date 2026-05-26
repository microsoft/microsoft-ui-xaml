// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InteractionBase.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(InteractionBase)
    {
    public:
        _Check_return_ HRESULT GetSupportedEventsImpl(_Outptr_ wfc::IVectorView<xaml::RoutedEvent*>** returnVal);
        _Check_return_ HRESULT GetSupportedEventsCoreImpl(_Outptr_ wfc::IVectorView<xaml::RoutedEvent*>** returnVal) { return S_OK; }

        _Check_return_ HRESULT OnKeyDownImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IKeyRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnKeyUpImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IKeyRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnPointerEnteredImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IPointerRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnPointerExitedImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IPointerRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnPointerMovedImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IPointerRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnPointerPressedImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IPointerRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnPointerReleasedImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IPointerRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnPointerCaptureLostImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IPointerRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnPointerCanceledImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IPointerRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnPointerWheelChangedImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IPointerRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnTappedImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::ITappedRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnDoubleTappedImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IDoubleTappedRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnHoldingImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IHoldingRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnRightTappedImpl(_In_ xaml::IUIElement* sender, _In_ xaml_input::IRightTappedRoutedEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnDragEnterImpl(_In_ xaml::IUIElement* sender, _In_ xaml::IDragEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnDragLeaveImpl(_In_ xaml::IUIElement* sender, _In_ xaml::IDragEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnDragOverImpl(_In_ xaml::IUIElement* sender, _In_ xaml::IDragEventArgs* args) { return S_OK; }
        _Check_return_ HRESULT OnDropImpl(_In_ xaml::IUIElement* sender, _In_ xaml::IDragEventArgs* args) { return S_OK; }

    protected:
        InteractionBase() = default;
    };
}
