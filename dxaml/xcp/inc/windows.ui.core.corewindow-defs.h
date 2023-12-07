// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// From windows.ui.core.corewindow.h internal header

#if defined(XAML_USE_PUBLIC_SDK)

#include <abi/xaml_abi.h>

#include <fwd/windows.foundation.h>
#include <fwd/windows.ui.core.h>

#if !defined(MIDL_NS_PREFIX)
#define MakeAgileDispatcherCallback          ::Microsoft::WRL::Callback<::Microsoft::WRL::Implements<::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>, ::Windows::UI::Core::IDispatchedHandler, ::Microsoft::WRL::FtmBase>>
#define MakeAgileIdleDispatcherCallback      ::Microsoft::WRL::Callback<::Microsoft::WRL::Implements<::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>, ::Windows::UI::Core::IIdleDispatchedHandler, ::Microsoft::WRL::FtmBase>>
#else
#define MakeAgileDispatcherCallback          ::Microsoft::WRL::Callback<::Microsoft::WRL::Implements<::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>, ::ABI::Windows::UI::Core::IDispatchedHandler, ::Microsoft::WRL::FtmBase>>
#define MakeAgileIdleDispatcherCallback      ::Microsoft::WRL::Callback<::Microsoft::WRL::Implements<::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>, ::ABI::Windows::UI::Core::IIdleDispatchedHandler, ::Microsoft::WRL::FtmBase>>
#endif // MIDL_NS_PREFIX

XAML_ABI_NAMESPACE_BEGIN namespace Windows { namespace UI { namespace Core {
using IWindowActivatedEventHandler                      = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::WindowActivatedEventArgs*>;
using IAutomationProviderRequestedEventHandler          = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::AutomationProviderRequestedEventArgs*>;
using ICharacterReceivedEventHandler                    = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::CharacterReceivedEventArgs*>;
using IWindowClosedEventHandler                         = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::CoreWindowEventArgs*>;
using IInputEnabledEventHandler                         = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::InputEnabledEventArgs*>;
using IKeyDownEventHandler                              = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::KeyEventArgs *>;
using IKeyUpEventHandler                                = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::KeyEventArgs *>;
using IPointerCaptureLostEventHandler                   = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::PointerEventArgs*>;
using IPointerEnteredEventHandler                       = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::PointerEventArgs*>;
using IPointerExitedEventHandler                        = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::PointerEventArgs*>;
using IPointerMovedEventHandler                         = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::PointerEventArgs*>;
using IPointerPressedEventHandler                       = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::PointerEventArgs*>;
using IPointerReleasedEventHandler                      = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::PointerEventArgs*>;
using ITouchHitTestingEventHandler                      = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::TouchHitTestingEventArgs*>;
using IClosestInteractiveBoundsRequestedEventHandler    = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::ClosestInteractiveBoundsRequestedEventArgs*>;
using IPointerWheelChangedEventHandler                  = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::PointerEventArgs*>;
using IWindowSizeChangedEventHandler                    = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::WindowSizeChangedEventArgs*>;
using IVisibilityChangedEventHandler                    = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, Windows::UI::Core::VisibilityChangedEventArgs*>;
using IResizeEventHandler                               = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreWindow*, IInspectable*>;
using IAcceleratorKeyActivatedEventHandler              = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreDispatcher*, Windows::UI::Core::AcceleratorKeyEventArgs*>;
using InputEnabledEventHandler                          = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::InputEnabledEventArgs*>;
using PointerCaptureLostEventHandler                    = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::PointerEventArgs*>;
using PointerEnteredEventHandler                        = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::PointerEventArgs*>;
using PointerExitedEventHandler                         = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::PointerEventArgs*>;
using PointerMovedEventHandler                          = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::PointerEventArgs*>;
using PointerPressedEventHandler                        = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::PointerEventArgs*>;
using PointerReleasedEventHandler                       = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::PointerEventArgs*>;
using PointerWheelChangedEventHandler                   = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::PointerEventArgs*>;
using KeyDownEventHandler                               = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::KeyEventArgs*>;
using KeyUpEventHandler                                 = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::KeyEventArgs*>;
using CharacterReceivedEventHandler                     = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::CharacterReceivedEventArgs*>;
using GotFocusEventHandler                              = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::CoreWindowEventArgs*>;
using LostFocusEventHandler                             = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::CoreWindowEventArgs*>;
using TouchHitTestingEventHandler                       = Windows::Foundation::ITypedEventHandler<IInspectable*, Windows::UI::Core::TouchHitTestingEventArgs*>;
using ClosestInteractiveBoundsRequestedEventHandler     = Windows::Foundation::ITypedEventHandler<Windows::UI::Core::CoreComponentInputSource*, Windows::UI::Core::ClosestInteractiveBoundsRequestedEventArgs*>;
using IPointerRoutedAwayEventHandler                    = Windows::Foundation::ITypedEventHandler<ICorePointerRedirector*, Windows::UI::Core::PointerEventArgs*>;
using IPointerRoutedToEventHandler                      = Windows::Foundation::ITypedEventHandler<ICorePointerRedirector*, Windows::UI::Core::PointerEventArgs*>;
using IPointerRoutedReleasedEventHandler                = Windows::Foundation::ITypedEventHandler<ICorePointerRedirector*, Windows::UI::Core::PointerEventArgs*>;
}}} XAML_ABI_NAMESPACE_END

#endif // !defined(XAML_USE_PUBLIC_SDK)
