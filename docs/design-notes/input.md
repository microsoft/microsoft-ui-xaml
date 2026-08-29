# Xaml Input

## Table of Contents

- [Overview](#overview)
- [Spatial](#spatial)
  - [Entrypoint](#entrypoint)
    - [Sample breakpoint](#sample-breakpoint)
  - [Hit testing](#hit-testing)
    - [Sample breakpoint](#sample-breakpoint-1)
  - [Events](#events)
    - [Sample breakpoint](#sample-breakpoint-2)
  - [Gestures](#gestures)
    - [Sample breakpoint](#sample-breakpoint-3)
  - [Touch](#touch)
  - [ReplayPointerUpdate](#replaypointerupdate)
- [Keyboard](#keyboard)
  - [Entrypoint](#entrypoint-1)
    - [Sample Breakpoint](#sample-breakpoint-4)
  - [Focus](#focus)
  - [Events](#events-1)
    - [Sample Breakpoint](#sample-breakpoint-5)
  - [Input Method Editor (IME)](#input-method-editor-ime)
    - [Sample breakpoint](#sample-breakpoint-6)
  - [Access Keys](#access-keys)
    - [Sample breakpoints](#sample-breakpoints)
  - [Accelerators](#accelerators)
    - [Focus](#focus-1)
      - [Sample breakpoints](#sample-breakpoints-1)
    - [Accelerators](#accelerators-1)
      - [Sample breakpoint](#sample-breakpoint-7)

# Overview

This document gives an overview of input in Xaml.


# Spatial

See the public doc on [handling pointer input](https://learn.microsoft.com/en-us/windows/apps/design/input/handle-pointer-input).


## Entrypoint

The entrypoint of spatial hit testing is on the XamlIsland. Composition's
[InputPointerSource](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.input.inputpointersource?view=windows-app-sdk-1.6)
raises WinRT events for input events like PointerMoved or PointerPressed. Xaml subscribes to these
events (see `dxaml/xcp/core/core/elements/XamlIslandRoot.cpp`)
and they start our spatial input handling.

Note that Xaml synthesizes window messages like `WM_POINTERUPDATE` when calling into our input handling. Before
integrating with islands, Xaml had its own message loop and responded to these messages directly. When we plugged in
islands, we reused the same code path by injecting the same WM input messages that we used to respond to.

There's actually a third layer to this translation. Xaml's input handling will turn WM_* messages into our own XCP_*
messages. This is because Xaml came from Silverlight, which ran cross-plat as a browser plugin. When it ran on Mac, it
didn't have any WM_* messages to work with, so it defined its own enums of messages. After cross-plat support was no
longer a thing, we never went back and removed this abstraction layer.

As the point is passed down the stack,
`ContentRootInput::PointerInputProcessor::ProcessPointerInput` (in `dxaml/xcp/components/ContentRoot/PointerInputProcessor.cpp`)
is the place that calls out to the hit testing walk.

### Sample breakpoint

`bu Microsoft_UI_Xaml!ContentRootInput::PointerInputProcessor::ProcessPointerInput`

```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!ContentRootInput::PointerInputProcessor::ProcessPointerInput
01 Microsoft_UI_Xaml!CInputServices::ProcessInput
02 Microsoft_UI_Xaml!CCoreServices::ProcessInput
03 Microsoft_UI_Xaml!CXcpBrowserHost::HandleInputMessage
04 Microsoft_UI_Xaml!CJupiterControl::HandlePointerMessage
05 Microsoft_UI_Xaml!CJupiterWindow::OnIslandPointerMessage
06 Microsoft_UI_Xaml!CXamlIslandRoot::InjectPointerMessage
07 Microsoft_UI_Xaml!CXamlIslandRoot::OnIslandPointerMoved
08 Microsoft_UI_Xaml!CXamlIslandRoot::SubscribeToInputPointerSourceEvents::__l27::<lambda_4>::operator()
...
```


## Hit testing

Hit testing takes some incoming input (either a point or a rect) and returns the UIElement(s) located at that position
in the island. Hit testing is a large part of spatial input processing.

`CUIElement::HitTestEntry` (in `dxaml/xcp/core/dll/xcpcore.cpp`)
    is the entrypoint to the hit testing walk. It has two overloads, one that takes a point and one that takes a rect. •
    The point overload is called from
    `CCoreServices::HitTest` (also in `dxaml/xcp/core/dll/xcpcore.cpp`),
    used during spatial input processing. • The rect overload is called from
    [VisualTreeHelper.FindElementsInHostCoordinates](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.media.visualtreehelper.findelementsinhostcoordinates?view=windows-app-sdk-1.6),
    an API that apps can call to return a list of UIElements in some logical rect.

Hit testing is a guided walk down the tree of UIElements. At each element the hit test point/rect are transformed by the
element's properties like the layout position, scale, clip, and transforms. As we walk down the tree, we intersect the
hit test point/rect against the render bounds cached on each element to help guide the walk and avoid walking the entire
tree. The cached bounds themselves are updated based on dirty flags which get
marked (via `InvalidateElementBounds` in `dxaml/xcp/components/elements/UIElementLayout.cpp`)
and propagated up the tree as UIElement properties change.

> Terminology note:
> * Inner bounds are bounds not considering the element's properties. On a 100x100 element with a 2x scale, they will be
  100x100. We have separate fields for the element itself and the children of the element.
> * Outer bounds are bounds with the element's properties included. On the same 100x100 element with a 2x scale, they
  will be 200x200. The outer bounds include both the element itself and all its children.

As the hit testing walks down the tree, it passes an object with a
`OnElementHit` (in `dxaml/xcp/core/core/elements/uielement.cpp`)
function on it. OnElementHit gets called when an element's content directly intersects the hit test point/rect. We then
check the hit test point against the
content (via `CBoundedHitTestVisitor::OnElementHitImpl`)
of the UIElement, and add it to a list of m_pHitElements if a hit is confirmed. This covers cases like hit testing a
circle, where the point might intersect the circle's bounding box but miss the circle because the point is near the
top-left corner of the bounding box.

For spatial input, CUIElement::HitTestEntry will return a single element being hit, which is the first element in the
list of m_pHitElements. FindElementsInHostCoordinates allows multiple elements to be returned since it takes a rect.


### Sample breakpoint

`bu Microsoft_UI_Xaml!CBorder::DoesBorderRectIntersectHitType<XPOINTF>`

```
0:000> kc
    # Call Site
00 Microsoft_UI_Xaml!CBorder::DoesBorderRectIntersectHitType<XPOINTF>
01 Microsoft_UI_Xaml!CBorder::HitTestLocalInternalImpl<XPOINTF>
02 Microsoft_UI_Xaml!CUIElement::HitTestLocal
03 Microsoft_UI_Xaml!CBoundedHitTestVisitor::OnElementHitImpl
04 Microsoft_UI_Xaml!CBoundedHitTestVisitor::OnElementHit
05 Microsoft_UI_Xaml!CUIElement::BoundsTestContentAndChildren<XPOINTF>
06 Microsoft_UI_Xaml!CUIElement::BoundsTestInternalImpl<XPOINTF>
07 Microsoft_UI_Xaml!CUIElement::BoundsTestChildrenImpl
...
68 Microsoft_UI_Xaml!CUIElement::BoundsTestChildren
69 Microsoft_UI_Xaml!CUIElement::BoundsTestContentAndChildren<XPOINTF>
6a Microsoft_UI_Xaml!CUIElement::BoundsTestInternalImpl<XPOINTF>
6b Microsoft_UI_Xaml!CUIElement::BoundsTestEntry<XPOINTF>
6c Microsoft_UI_Xaml!CUIElement::HitTestEntry
6d Microsoft_UI_Xaml!CUIElement::HitTestEntry
6e Microsoft_UI_Xaml!CCoreServices::HitTest
6f Microsoft_UI_Xaml!ContentRootInput::PointerInputProcessor::HitTestHelper
70 Microsoft_UI_Xaml!ContentRootInput::PointerInputProcessor::HitTestWithLightDismissAwareness
71 Microsoft_UI_Xaml!ContentRootInput::PointerInputProcessor::ProcessPointerInput
...
```


## Events

After PointerInputProcessor::ProcessPointerInput calls out to the hit testing walk, it raises input-related events. See
`UIElement.PointerMoved` (in `dxaml/xcp/components/ContentRoot/PointerInputProcessor.cpp`)
as an example. Note that these input events are marked with `fRaiseSync=true`, which means they will immediately call
out to app event handlers rather than queue an event to be raised later on the UI thread.


### Sample breakpoint

`bu Microsoft_UI_Xaml!DirectUI::TextBox::OnPointerMoved`

```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!DirectUI::TextBox::OnPointerMoved
01 Microsoft_UI_Xaml!DirectUI::ControlGenerated::OnPointerMovedProtected
02 Microsoft_UI_Xaml!DirectUI::Control::FireEvent
03 Microsoft_UI_Xaml!DirectUI::DXamlCore::FireEvent
04 Microsoft_UI_Xaml!AgCoreCallbacks::FireEvent
05 Microsoft_UI_Xaml!FxCallbacks::JoltHelper_FireEvent
06 Microsoft_UI_Xaml!CCoreServices::CLR_FireEvent
07 Microsoft_UI_Xaml!CommonBrowserHost::CLR_FireEvent
08 Microsoft_UI_Xaml!CControlBase::ScriptCallback
09 Microsoft_UI_Xaml!CXcpDispatcher::OnScriptCallback
0a Microsoft_UI_Xaml!CXcpDispatcher::OnWindowMessage
0b Microsoft_UI_Xaml!CXcpDispatcher::SendMessageW
0c Microsoft_UI_Xaml!CXcpBrowserHost::SyncScriptCallbackRequest
0d Microsoft_UI_Xaml!CEventManager::RaiseControlEvents
0e Microsoft_UI_Xaml!CEventManager::Raise
0f Microsoft_UI_Xaml!CEventManager::RaiseRoutedEventBubbling
10 Microsoft_UI_Xaml!ContentRootInput::PointerInputProcessor::ProcessPointerInput
...
```

## Gestures

Some events are gestures detected by the gesture recognizer. Tapped is an example of this. In these cases, Xaml feeds
pointer input into the gesture recognizer, which calls us back synchronously when a gesture is detected. Xaml then
synchronously raises the event.


### Sample breakpoint

`bu Microsoft_UI_Xaml!DirectUI::TextBox::OnTapped`

```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!DirectUI::TextBox::OnTapped
01 Microsoft_UI_Xaml!DirectUI::ControlGenerated::OnTappedProtected
02 Microsoft_UI_Xaml!DirectUI::Control::FireEvent
03 Microsoft_UI_Xaml!DirectUI::DXamlCore::FireEvent
04 Microsoft_UI_Xaml!AgCoreCallbacks::FireEvent
05 Microsoft_UI_Xaml!FxCallbacks::JoltHelper_FireEvent
06 Microsoft_UI_Xaml!CCoreServices::CLR_FireEvent
07 Microsoft_UI_Xaml!CommonBrowserHost::CLR_FireEvent
08 Microsoft_UI_Xaml!CControlBase::ScriptCallback
09 Microsoft_UI_Xaml!CXcpDispatcher::OnScriptCallback
0a Microsoft_UI_Xaml!CXcpDispatcher::OnWindowMessage
0b Microsoft_UI_Xaml!CXcpDispatcher::SendMessageW
0c Microsoft_UI_Xaml!CXcpBrowserHost::SyncScriptCallbackRequest
0d Microsoft_UI_Xaml!CEventManager::RaiseControlEvents
0e Microsoft_UI_Xaml!CEventManager::Raise
0f Microsoft_UI_Xaml!CEventManager::RaiseRoutedEventBubbling
10 Microsoft_UI_Xaml!ReleaseInterface
11 Microsoft_UI_Xaml!CInputServices::ProcessGestureInput
12 Microsoft_UI_Xaml!CInputServices::ProcessTouchInteractionCallback
13 Microsoft_UI_Xaml!CCoreServices::ProcessTouchInteractionCallback
14 Microsoft_UI_Xaml!GestureRecognizerAdapter::OnTapped
15 Microsoft_UI_Xaml!GestureRecognizerAdapter::Init::__l3::<lambda_1>::operator()
16 Microsoft_UI_Xaml!Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl ABI::Windows::Foundation::ITypedEventHandler_impl<ABI::Windows::Foundation::Internal::AggregateType<ABI::Microsoft::UI::Input::GestureRecognizer *,ABI::Microsoft::UI::Input::IGestureRecognizer *>,ABI::Windows::Foundation::Internal::AggregateType<ABI::Microsoft::UI::Input::TappedEventArgs *,ABI::Microsoft::UI::Input::ITappedEventArgs *> >::*)(ABI::Microsoft::UI::Input::IGestureRecognizer *,ABI::Microsoft::UI::Input::ITappedEventArgs *)>::DelegateInvokeHelper<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<2>,ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Input::GestureRecognizer *,ABI::Microsoft::UI::Input::TappedEventArgs *>,Microsoft::WRL::FtmBase>,`GestureRecognizerAdapter::Init'::`3'::<lambda_1> &,1,ABI::Microsoft::UI::Input::IGestureRecognizer *,ABI::Microsoft::UI::Input::ITappedEventArgs *>::Invoke
17 Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::ExitedMoveSizeEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll::__l2::<lambda_1>::operator()
18 Microsoft_UI_Input!Microsoft::WRL::InvokeTraits<-2>::InvokeDelegates<`Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::ExitedMoveSizeEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll<InputNonClientPointerSourceWinRT::Api *,Microsoft::UI::Input::IExitedMoveSizeEventArgs *>'::`2'::<lambda_1>,Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::ExitedMoveSizeEventArgs *> >
19 Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::GestureRecognizer *,Microsoft::UI::Input::TappedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::DoInvoke
1a Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::GestureRecognizer *,Microsoft::UI::Input::TappedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll
1b Microsoft_UI_Input!GestureRecognizerServer::OnTappedEvent
1c Microsoft_UI_Input!GestureRecognizerServer::OnOutput
1d Microsoft_UI_Input!GestureRecognizerServer::OutputCallback
1e NInput!COutputConverter::Process
1f NInput!CInteractionContextImpl::OutputCallback
20 NInput!CInteractionGroupingFilter::_SendOutput
21 NInput!CInteractionGroupingFilter::_OnInput
22 NInput!CInteractionGroupingFilter::Input
23 NInput!COutputCoalescingFilter::Flush
24 NInput!COutputCoalescingFilter::_OnInput
25 NInput!COutputCoalescingFilter::Input
26 NInput!CInteractionEngineImpl::DigitizerInput
27 NInput!ProcessInputInteraction
28 NInput!CInteractionContextImpl::ProcessFrameHistory
29 NInput!CInteractionContextImpl::ProcessBufferedPackets
2a NInput!ProcessBufferedPacketsInteractionContext
2b Microsoft_UI_Input!GestureRecognizerServer::ProcessUpEvent
2c Microsoft_UI_Xaml!ElementGestureTracker::ProcessPointerInformation
2d Microsoft_UI_Xaml!ElementGestureTracker::ProcessPointerMessage
2e Microsoft_UI_Xaml!CInputServices::ProcessPointerMessagesWithInteractionEngine
2f Microsoft_UI_Xaml!CInputServices::ProcessInteractionPointerMessages
30 Microsoft_UI_Xaml!ContentRootInput::PointerInputProcessor::ProcessPointerInput
```
* Frames 30-2c are Xaml feeding the input (in this case a pointer up) into the gesture recognizer
* Frames 2b-17 are the gesture recognizer detecting a Tapped event (based on this pointer up and a previous pointer down)
* Frames 16-3 are Xaml responding to the Tapped event from the gesture recognizer and raising our own UIElement.Tapped event


## Touch

Touch input is received by Xaml as pointer events, and Xaml responds to them in the same way as we respond to mouse.

There is one piece of special treatment in system Xaml (WinUI 2) with UWPs called
`OnTouchHitTesting`.
This solves the problem of a finger being imprecise and the touched area being more of a rect than a single point. This
event handler will do a hit test of the full rect, evaluate the list of hit elements, and pick the best one. It returns
a CoreProximityEvaluation object with an
[AdjustedPoint](https://learn.microsoft.com/en-us/uwp/api/windows.ui.core.coreproximityevaluation.adjustedpoint?view=winrt-26100)
property that is then used by the CoreWindow as the actual point of contact for the input.

This code path does not
exist
in WinUI 3 (see `dxaml/xcp/dxaml/lib/InputSiteAdapter.cpp`). The event that triggers it is on CoreWindow, which we no longer have. InputPointerSource has no equivalent
event.


## ReplayPointerUpdate

Xaml has a feature called ReplayPointerUpdate meant to handle animation scenarios. When the pointer is moving, we
constantly get new WinRT events, which trigger hit testing walks and update things like element hover. But if the cursor
is static while the UI animates underneath the cursor, then we don't get any input WinRT events to kick off hit testing.
Our solution was to remember the most recent pointer event, and to replay it after animations so that the new hover
element gets a hover visual.


# Keyboard

Keyboard input in Win32 is a very different code path from spatial input. Keyboard input deals with the concept of Win32
focus, where the focused hwnd gets keyboard messages. For Xaml, the lifted
[InputKeyboardSource](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.input.inputkeyboardsource?view=windows-app-sdk-1.6)
object deals with the Win32 messages and raises WinRT events for Xaml.

## Entrypoint

The entrypoint of keyboard input is also on the XamlIsland. Composition's
[InputKeyboardSource](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.input.inputkeyboardsource?view=windows-app-sdk-1.6)
raises WinRT events for input events like PointerMoved or PointerPressed. Xaml subscribes to these
events (see `dxaml/xcp/core/core/elements/XamlIslandRoot.cpp`)
and they start our keyboard input handling.

Much like with spatial input, Xaml also synthesizes its own WM input messages after getting the WinRT event. We did this
to integrate with our previous message-based input handling code when adding islands. Also like with spatial input,
there's a third layer of XCP_* messages carried over from the Silverlight days.

### Sample Breakpoint

`bu Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessKeyboardInput`

```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessKeyboardInput
01 Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessKeyEvent
02 Microsoft_UI_Xaml!CInputManager::ProcessKeyEvent
03 Microsoft_UI_Xaml!CJupiterWindow::ProcessKeyEvents
04 Microsoft_UI_Xaml!CJupiterWindow::OnIslandKeyDown
05 Microsoft_UI_Xaml!CXamlIslandRoot::OnIslandKeyDown
06 Microsoft_UI_Xaml!CXamlIslandRoot::SubscribeToInputKeyboardSourceEvents::__l12::<lambda_2>::operator()
07 Microsoft_UI_Xaml!Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl ABI::Windows::Foundation::ITypedEventHandler_impl<ABI::Windows::Foundation::Internal::AggregateType<ABI::Microsoft::UI::Input::InputKeyboardSource *,ABI::Microsoft::UI::Input::IInputKeyboardSource *>,ABI::Windows::Foundation::Internal::AggregateType<ABI::Microsoft::UI::Input::KeyEventArgs *,ABI::Microsoft::UI::Input::IKeyEventArgs *> >::*)(ABI::Microsoft::UI::Input::IInputKeyboardSource *,ABI::Microsoft::UI::Input::IKeyEventArgs *)>::DelegateInvokeHelper<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<2>,ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Input::InputKeyboardSource *,ABI::Microsoft::UI::Input::KeyEventArgs *>,Microsoft::WRL::FtmBase>,`CXamlIslandRoot::SubscribeToInputKeyboardSourceEvents'::`12'::<lambda_2> &,1,ABI::Microsoft::UI::Input::IInputKeyboardSource *,ABI::Microsoft::UI::Input::IKeyEventArgs *>::Invoke
08 Microsoft_UI_Input!Microsoft::WRL::Details::CreateAgileHelper::__l2::<lambda_1>::operator()
09 Microsoft_UI_Input!Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl Windows::Foundation::ITypedEventHandler_impl<Windows::Foundation::Internal::AggregateType<Microsoft::UI::Input::InputKeyboardSource *,Microsoft::UI::Input::IInputKeyboardSource *>,Windows::Foundation::Internal::AggregateType<Microsoft::UI::Input::KeyEventArgs *,Microsoft::UI::Input::IKeyEventArgs *> >::*)(Microsoft::UI::Input::IInputKeyboardSource *,Microsoft::UI::Input::IKeyEventArgs *)>::DelegateInvokeHelper<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<2>,Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputKeyboardSource *,Microsoft::UI::Input::KeyEventArgs *>,Microsoft::WRL::FtmBase>,`Microsoft::WRL::Details::CreateAgileHelper<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputKeyboardSource *,Microsoft::UI::Input::KeyEventArgs *> >'::`2'::<lambda_1>,-1,Microsoft::UI::Input::IInputKeyboardSource *,Microsoft::UI::Input::IKeyEventArgs *>::Invoke
0a Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::EnteringMoveSizeEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll::__l2::<lambda_1>::operator()
0b Microsoft_UI_Input!Microsoft::WRL::InvokeTraits<-2>::InvokeDelegates<`Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::EnteringMoveSizeEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll<InputNonClientPointerSourceWinRT::Api *,EnteringMoveSizeEventArgs *>'::`2'::<lambda_1>,Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::EnteringMoveSizeEventArgs *> >
0c Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputFocusController *,Microsoft::UI::Input::FocusChangedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::DoInvoke
0d Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputFocusController *,Microsoft::UI::Input::FocusChangedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll<InputFocusControllerWinRT::Api *,Microsoft::UI::Input::IFocusChangedEventArgs *>
0e Microsoft_UI_Input!KeyboardInputWinRT::OnKeyUpEvent_Callback::__l6::<lambda_1>::operator()
0f Microsoft_UI_Input!Microsoft::WRL2::ContextSession::LeaveSession_Callback<`KeyboardInputWinRT::OnKeyUpEvent_Callback'::`6'::<lambda_1> >
10 Microsoft_UI_Input!Microsoft::WRL2::ContextRuntimeClass::LeaveSessionAndValidate_Callback
11 Microsoft_UI_Input!KeyboardInputWinRT::OnKeyDownEvent_Callback
```


## Focus

Whereas spatial input needs to do a hit test walk to determine which UIElement receives the input, keyboard is much
easier. Xaml knows the currently focused element, and that
element (see `dxaml/xcp/components/ContentRoot/KeyboardInputProcessor.cpp`)
is forwarded the keyboard input directly.

Not all Xaml elements are focusable. See
`CUIElement::IsFocusable` (in `dxaml/xcp/core/core/elements/uielement.cpp`)
- typically an element needs to be a tab stop, or needs to be a Control with the
[IsFocusEngagementEnabled](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.control.isfocusengagementenabled?view=windows-app-sdk-1.6)
property set to true while the user is using a gamepad.


## Events

Keyboard input events are raised synchronously, like with spatial input. See
`UIElement.KeyDown` (in `dxaml/xcp/components/ContentRoot/KeyboardInputProcessor.cpp`)
as an example.

Note that before the key down event is sent, Xaml sends a
[PreviewKeyDown](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.uielement.previewkeydown?view=windows-app-sdk-1.6)
event. These preview key events are tunneling (raised on the ancestor element before the descendant element), as opposed
to the key event which is bubbling (raised on the descendant element first, then raised on ancestors up the tree).


### Sample Breakpoint

`bu Microsoft_UI_Xaml!DirectUI::TextBox::OnKeyDown`

```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!DirectUI::RichEditBox::OnKeyDown
01 Microsoft_UI_Xaml!DirectUI::ControlGenerated::OnKeyDownProtected
02 Microsoft_UI_Xaml!DirectUI::Control::FireEvent
03 Microsoft_UI_Xaml!DirectUI::DXamlCore::FireEvent
04 Microsoft_UI_Xaml!AgCoreCallbacks::FireEvent
05 Microsoft_UI_Xaml!FxCallbacks::JoltHelper_FireEvent
06 Microsoft_UI_Xaml!CCoreServices::CLR_FireEvent
07 Microsoft_UI_Xaml!CommonBrowserHost::CLR_FireEvent
08 Microsoft_UI_Xaml!CControlBase::ScriptCallback
09 Microsoft_UI_Xaml!CXcpDispatcher::OnScriptCallback
0a Microsoft_UI_Xaml!CXcpDispatcher::OnWindowMessage
0b Microsoft_UI_Xaml!CXcpDispatcher::SendMessageW
0c Microsoft_UI_Xaml!CXcpBrowserHost::SyncScriptCallbackRequest
0d Microsoft_UI_Xaml!CEventManager::RaiseControlEvents
0e Microsoft_UI_Xaml!CEventManager::Raise
0f Microsoft_UI_Xaml!CEventManager::RaiseRoutedEventBubbling
10 Microsoft_UI_Xaml!CEventManager::RaiseRoutedEvent
11 Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessXCPKeydown
12 Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessKeyboardInput
13 Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessKeyEvent
14 Microsoft_UI_Xaml!CInputManager::ProcessKeyEvent
15 Microsoft_UI_Xaml!CJupiterWindow::ProcessKeyEvents
16 Microsoft_UI_Xaml!CJupiterWindow::OnIslandKeyDown
17 Microsoft_UI_Xaml!CXamlIslandRoot::OnIslandKeyDown
18 Microsoft_UI_Xaml!CXamlIslandRoot::SubscribeToInputKeyboardSourceEvents::__l12::<lambda_2>::operator()
19 Microsoft_UI_Xaml!Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl ABI::Windows::Foundation::ITypedEventHandler_impl<ABI::Windows::Foundation::Internal::AggregateType<ABI::Microsoft::UI::Input::InputKeyboardSource *,ABI::Microsoft::UI::Input::IInputKeyboardSource *>,ABI::Windows::Foundation::Internal::AggregateType<ABI::Microsoft::UI::Input::KeyEventArgs *,ABI::Microsoft::UI::Input::IKeyEventArgs *> >::*)(ABI::Microsoft::UI::Input::IInputKeyboardSource *,ABI::Microsoft::UI::Input::IKeyEventArgs *)>::DelegateInvokeHelper<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<2>,ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Input::InputKeyboardSource *,ABI::Microsoft::UI::Input::KeyEventArgs *>,Microsoft::WRL::FtmBase>,`CXamlIslandRoot::SubscribeToInputKeyboardSourceEvents'::`12'::<lambda_2> &,1,ABI::Microsoft::UI::Input::IInputKeyboardSource *,ABI::Microsoft::UI::Input::IKeyEventArgs *>::Invoke
1a Microsoft_UI_Input!Microsoft::WRL::Details::CreateAgileHelper::__l2::<lambda_1>::operator()
1b Microsoft_UI_Input!Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl Windows::Foundation::ITypedEventHandler_impl<Windows::Foundation::Internal::AggregateType<Microsoft::UI::Input::InputKeyboardSource *,Microsoft::UI::Input::IInputKeyboardSource *>,Windows::Foundation::Internal::AggregateType<Microsoft::UI::Input::KeyEventArgs *,Microsoft::UI::Input::IKeyEventArgs *> >::*)(Microsoft::UI::Input::IInputKeyboardSource *,Microsoft::UI::Input::IKeyEventArgs *)>::DelegateInvokeHelper<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<2>,Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputKeyboardSource *,Microsoft::UI::Input::KeyEventArgs *>,Microsoft::WRL::FtmBase>,`Microsoft::WRL::Details::CreateAgileHelper<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputKeyboardSource *,Microsoft::UI::Input::KeyEventArgs *> >'::`2'::<lambda_1>,-1,Microsoft::UI::Input::IInputKeyboardSource *,Microsoft::UI::Input::IKeyEventArgs *>::Invoke
1c Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::EnteringMoveSizeEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll::__l2::<lambda_1>::operator()
1d Microsoft_UI_Input!Microsoft::WRL::InvokeTraits<-2>::InvokeDelegates<`Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::EnteringMoveSizeEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll<InputNonClientPointerSourceWinRT::Api *,EnteringMoveSizeEventArgs *>'::`2'::<lambda_1>,Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::EnteringMoveSizeEventArgs *> >
1e Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputFocusController *,Microsoft::UI::Input::FocusChangedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::DoInvoke
1f Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputFocusController *,Microsoft::UI::Input::FocusChangedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll<InputFocusControllerWinRT::Api *,Microsoft::UI::Input::IFocusChangedEventArgs *>
20 Microsoft_UI_Input!KeyboardInputWinRT::OnKeyUpEvent_Callback::__l6::<lambda_1>::operator()
21 Microsoft_UI_Input!Microsoft::WRL2::ContextSession::LeaveSession_Callback<`KeyboardInputWinRT::OnKeyUpEvent_Callback'::`6'::<lambda_1> >
22 Microsoft_UI_Input!Microsoft::WRL2::ContextRuntimeClass::LeaveSessionAndValidate_Callback
23 Microsoft_UI_Input!KeyboardInputWinRT::OnKeyDownEvent_Callback
```
* Frames 23-1a are lifted input dealing with the keyboard inout
* Frames 18-17 are the entrypoint into Xaml - the InputKeyboardSource's KeyDown event handler
* Frames 16-11 are Xaml processing the keyboard input and preparing to fire an event
* Frames 10-00 are Xaml synchronously firing the KeyDown event and the TextBox/RichEditBox responding to it


## Input Method Editor (IME)

IME is special in that keyboard input does not come in from the InputKeyboardSource. Instead it comes from WinUIEdit,
who gets called directly by the IME. WinUIEdit calls Xaml's
`TextServicesHost::TxNotify` (in `dxaml/xcp/core/native/text/Controls/TextServicesHost.cpp`)
where it goes to the TextBox.

This causes Xaml to raise its
[TextBox.TextCompositionChanged](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.textbox.textcompositionchanged?view=windows-app-sdk-1.6)/Started/Ended
events. These events are synchronous, except when they cause reentrancy where the nested TextComposition events will be
async.


### Sample breakpoint

`bu Microsoft_UI_Xaml!CTextBoxBase::SendTextCompositionEvent`

```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!CTextBoxBase::SendTextCompositionEvent
01 Microsoft_UI_Xaml!CTextBoxBase::TxNotify
02 Microsoft_UI_Xaml!TextServicesHost::TxNotify
03 WinUIEdit!CTxtEdit::SendNotification
04 WinUIEdit!CUIM::OnStartComposition
05 textinputframework!CInputContext::_StartComposition
06 textinputframework!CInputContext::StartComposition
07 textinputframework!CInputContext::CreateComposition
08 textinputframework!CInputContextAdapter::SetInComposition
09 textinputframework!CTSF3CompositionChange::Execute
0a textinputframework!CInputContextAdapter::_LeaveEditSession
0b textinputframework!CInputContextAdapter::DoEditSession
0c textinputframework!CInputContext::_DoEditSession
0d textinputframework!CInputContext::_EditSessionQiCallback
0e textinputframework!CInputContext::_DispatchQueueItem
0f textinputframework!CInputContext::_EmptyLockQueue
10 textinputframework!CInputContext::OnLockGranted
11 textinputframework!CACPWrap::OnLockGranted
12 WinUIEdit!CUIM::RequestLock
13 textinputframework!SafeRequestLock
14 textinputframework!CInputContext::RequestLock
15 msctf!CThreadInputMgr::OnPostponedLockRequest
16 msctf!_GetMsgHook
17 msctf!TF_Notify
18 USER32!CtfHookProcWorker
19 USER32!CallHookWithSEH
1a USER32!__fnHkINLPMSG
...
1d USER32!GetMessageW
1e Microsoft_UI_Xaml!DirectUI::FrameworkApplication::RunDesktopWindowMessageLoop
```
* Frames 11-05 are the IME
* Frames 04-03 are WinUIEdit responding to the IME starting text composition and calling out to Xaml
* Frames 02-00 are Xaml handling the IME composition event


## Access Keys

Access keys are a usability and accessibility feature that let users invoke controls with successive key presses (as
opposed to accelerators which need multiple keys to be pressed at the same time). See the [public
documentation](https://learn.microsoft.com/en-us/windows/apps/design/input/access-keys).

Access keys have a concept of scope to help the app build a hierarchy of keys and prevent lots of key tips from showing
up on screen at the same time. See the [public
documentation](https://learn.microsoft.com/en-us/windows/apps/design/input/access-keys#set-access-key-scope)

In Xaml, access keys are powered by the same keyboard input code path in KeyboardInputProcessor. Access keys get the
first
chance (via `TryProcessInputForAccessKey` in `dxaml/xcp/components/ContentRoot/KeyboardInputProcessor.cpp`)
at handling key presses, before element events get a chance to handle it.

At the top of the stack, opening the key tip is an async operation.
`KeyTipManager::ShowAutoKeyTipForElement` (in `dxaml/xcp/core/input/KeyTipManager.cpp`)
queues the async work via GoToState(..., WaitingToUpdateKeyTips), which starts a 1ms timer. When that timer fires,
`KeyTipManager::OnStateTimerFired`
does an ExecuteOnUIThread so that KeyTipManager::Execute is called back later.
`KeyTipManager::Execute`
then creates and opens the popup and positions it in the correct place.


### Sample breakpoints

`bm Microsoft_UI_Xaml!*TryProcessInputForAccessKey`

`bu microsoft_ui_xaml!KeyTipManager::ShowAutoKeyTipForElement`

```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!KeyTipManager::ShowAutoKeyTipForElement
01 Microsoft_UI_Xaml!CUIElement::RaiseAccessKeyShown
02 Microsoft_UI_Xaml!AccessKeys::AKOwnerEvents::RaiseAccessKeyShown
03 Microsoft_UI_Xaml!AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider>::ShowAccessKey
04 Microsoft_UI_Xaml!AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >::ShowAccessKeys
05 Microsoft_UI_Xaml!AccessKeys::AKScopeTree<AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >,AccessKeys::AKScopeBuilder<AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,AccessKeys::AKParser,CDependencyObject,AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> > >,AccessKeys::AKModeContainer,CDependencyObject,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,CFocusManager>::EnterScope
06 Microsoft_UI_Xaml!AccessKeys::AKScopeTree<AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >,AccessKeys::AKScopeBuilder<AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,AccessKeys::AKParser,CDependencyObject,AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> > >,AccessKeys::AKModeContainer,CDependencyObject,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,CFocusManager>::UpdateScopeImpl
07 Microsoft_UI_Xaml!AccessKeys::AKScopeTree<AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >,AccessKeys::AKScopeBuilder<AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,AccessKeys::AKParser,CDependencyObject,AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> > >,AccessKeys::AKModeContainer,CDependencyObject,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,CFocusManager>::ProcessCharacter
08 Microsoft_UI_Xaml!AccessKeys::AKInputInterceptor<AccessKeys::AKModeContainer,AccessKeys::AKScopeTree<AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >,AccessKeys::AKScopeBuilder<AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,AccessKeys::AKParser,CDependencyObject,AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> > >,AccessKeys::AKModeContainer,CDependencyObject,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,CFocusManager>,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject> >::TryProcessKeyImpl
09 Microsoft_UI_Xaml!AccessKeys::AKInputInterceptor<AccessKeys::AKModeContainer,AccessKeys::AKScopeTree<AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >,AccessKeys::AKScopeBuilder<AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,AccessKeys::AKParser,CDependencyObject,AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> > >,AccessKeys::AKModeContainer,CDependencyObject,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,CFocusManager>,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject> >::TryProcessInputForAccessKey
0a Microsoft_UI_Xaml!AccessKeys::AccessKeyExport::TryProcessInputForAccessKey
0b Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessKeyboardInput
0c Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessKeyEvent
0d Microsoft_UI_Xaml!CInputManager::ProcessKeyEvent
0e Microsoft_UI_Xaml!CJupiterWindow::ProcessKeyEvents
0f Microsoft_UI_Xaml!CJupiterWindow::OnIslandSysKeyUp
10 Microsoft_UI_Xaml!CXamlIslandRoot::OnIslandSysKeyUp
11 Microsoft_UI_Xaml!CXamlIslandRoot::SubscribeToInputKeyboardSourceEvents::__l36::<lambda_5>::operator()
```
* For the ALT key press that displays key tips - note that we go through KeyDown


```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!KeyTipManager::ShowAutoKeyTipForElement
01 Microsoft_UI_Xaml!CUIElement::RaiseAccessKeyShown
02 Microsoft_UI_Xaml!AccessKeys::AKOwnerEvents::RaiseAccessKeyShown
03 Microsoft_UI_Xaml!AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider>::ShowAccessKey
04 Microsoft_UI_Xaml!AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >::UpdatePartialMatchAccessKeyVisibility
05 Microsoft_UI_Xaml!AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >::Invoke
06 Microsoft_UI_Xaml!AccessKeys::AKScopeTree<AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >,AccessKeys::AKScopeBuilder<AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,AccessKeys::AKParser,CDependencyObject,AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> > >,AccessKeys::AKModeContainer,CDependencyObject,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,CFocusManager>::ProcessNormalKey
07 Microsoft_UI_Xaml!AccessKeys::AKScopeTree<AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >,AccessKeys::AKScopeBuilder<AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,AccessKeys::AKParser,CDependencyObject,AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> > >,AccessKeys::AKModeContainer,CDependencyObject,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,CFocusManager>::ProcessCharacter
08 Microsoft_UI_Xaml!AccessKeys::AKInputInterceptor<AccessKeys::AKModeContainer,AccessKeys::AKScopeTree<AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >,AccessKeys::AKScopeBuilder<AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,AccessKeys::AKParser,CDependencyObject,AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> > >,AccessKeys::AKModeContainer,CDependencyObject,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,CFocusManager>,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject> >::TryProcessKeyImpl
09 Microsoft_UI_Xaml!AccessKeys::AKInputInterceptor<AccessKeys::AKModeContainer,AccessKeys::AKScopeTree<AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> >,AccessKeys::AKScopeBuilder<AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,AccessKeys::AKParser,CDependencyObject,AccessKeys::AKScope<CDependencyObject,AccessKeys::AKOwner<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider> > >,AccessKeys::AKModeContainer,CDependencyObject,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject>,CFocusManager>,AccessKeys::AKTreeAnalyzer<CDOCollection,VisualTree,AccessKeys::AKVisualTreeFinder,CDependencyObject> >::TryProcessInputForCharacterReceived
0a Microsoft_UI_Xaml!AccessKeys::AccessKeyExport::TryProcessInputForCharacterReceived
0b Microsoft_UI_Xaml!CJupiterWindow::ProcessCharEvents
0c Microsoft_UI_Xaml!CJupiterWindow::OnIslandCharacterReceived
0d Microsoft_UI_Xaml!CXamlIslandRoot::OnIslandCharacterReceived
0e Microsoft_UI_Xaml!CXamlIslandRoot::SubscribeToInputKeyboardSourceEvents::__l4::<lambda_1>::operator()
```
* For the keys after the ALT key press - note that we don't go through KeyDown but go through CharacterReceived instead
  * The public docs for
    [InputKeyboardSource.KeyDown](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.input.inputkeyboardsource.keydown?view=windows-app-sdk-1.6)
    mention that these events are only raised of ALT isn't also pressed. Access keys are triggered with alt, so maybe we
    wanted to intentionally work even when the user holds down ALT, which made us use CharacterReceived.
  * The rest of the stack looks similar to KeyDown, except we don't go through KeyboardInputProcessor. CJupiterWindow
    calls AccessKeyExport directly, rather than going through InputManager and KeyboardInputProcessor. AccessKeyExport
    still gets the first chance to handle the key before the rest of Xaml.


`bu microsoft_ui_xaml!CPopup::Open`

```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!CPopup::Open
01 Microsoft_UI_Xaml!CPopup::SetChild
02 Microsoft_UI_Xaml!CPopup::Child
03 Microsoft_UI_Xaml!CDependencyObject::SetEffectiveValueViaMethodCall
04 Microsoft_UI_Xaml!CDependencyObject::SetEffectiveValue
05 Microsoft_UI_Xaml!CDependencyObject::UpdateEffectiveValue
06 Microsoft_UI_Xaml!CDependencyObject::SetValue
07 Microsoft_UI_Xaml!CUIElement::SetValue
08 Microsoft_UI_Xaml!CFrameworkElement::SetValue
09 Microsoft_UI_Xaml!CPopup::SetValue
0a Microsoft_UI_Xaml!CDependencyObject::SetValue
0b Microsoft_UI_Xaml!CDependencyObject::SetValueByIndex
0c Microsoft_UI_Xaml!CDependencyObject::SetValueByKnownIndex
0d Microsoft_UI_Xaml!KeyTipManager::CreatePopup
0e Microsoft_UI_Xaml!KeyTipManager::Execute
0f Microsoft_UI_Xaml!CommonBrowserHost::ProcessExecuteMessage
10 Microsoft_UI_Xaml!CXcpDispatcher::OnReentrancyProtectedWindowMessage
11 Microsoft_UI_Xaml!CXcpDispatcher::ProcessMessage
12 Microsoft_UI_Xaml!CDeferredInvoke::DispatchQueuedMessage
13 Microsoft_UI_Xaml!CXcpDispatcher::MessageTimerCallback
14 Microsoft_UI_Xaml!CXcpDispatcher::MessageTimerCallbackStatic
15 Microsoft_UI_Xaml!CXcpDispatcher::Init::__l46::<lambda_1>::operator()
```


## Accelerators

Accelerators represent a scenario where hwnds that are _not_ focused may still want to respond to keyboard input.
Suppose an app has a top-level window with two nested child hwnds inside. Focus is on the top-level hwnd, and the user
presses ctrl+s. All hwnds should get a chance to save, even though the two child hwnds do not have focus. This is
typically achieved through a PreTranslateMessage convention. See
ContentPreTranslateMessage documentation
for a description of it, and how islands participate in this convention.

Xaml currently plugs into ContentPreTranslateMessage via an
[IInputPreTranslateKeyboardSourceHandler](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/win32/microsoft.ui.input.inputpretranslatesource.interop/nn-microsoft-ui-input-inputpretranslatesource-interop-iinputpretranslatekeyboardsourcehandler).
Xaml's PreTranslateHandler calls back to
`CXamlIslandRoot::PreTranslateMessage` (in `dxaml/xcp/core/core/elements/XamlIslandRoot.cpp`)
for both OnDirectMessage (where the Xaml island is the one with Win32 focus, and an accelerator key came in) and for
OnTreeMessage (where some other hwnd had Win32 focus, and an accelerator key came in). Xaml then calls into
CJupiterWindow::PreTranslateMessage to handle the accelerator.

Unlike InputKeyboardSource, PreTranslateMessage comes in as window messages and not WinRT events.

The current design is for everything to happen on the
OnDirectMessage (see the `!focusPass` check in `dxaml/xcp/dxaml/lib/JupiterWindow.cpp`)
(also called the focus pass). There's a comment about how we want to eventually support accelerator events for Xaml
islands that aren't focused.

A consequence of this design (or maybe the reasoning behind it?) is that Xaml only handles Tab and focus
navigation (see the `focusPass` check in `JupiterWindow.cpp`)
during PreTranslateMessage processing. All other accelerators are deferred to regular keyboard message processing.


### Focus

Even tab and focus navigation is handled by calling into regular keyboard message processing code from
PreTranslateMessage.

#### Sample breakpoints

`bu Microsoft_UI_Xaml!CJupiterWindow::PreTranslateMessage`

`bu Microsoft_UI_Xaml!CFocusManager::UpdateFocus`

(Can be triggered by tab navigation in WinUI 3 Gallery)

```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!CFocusManager::UpdateFocus
01 Microsoft_UI_Xaml!CFocusManager::SetFocusedElement
02 Microsoft_UI_Xaml!CFocusManager::ProcessTabStop
03 Microsoft_UI_Xaml!ContentRootAdapters::ContentRootEventListener::TabProcessingEventListener
04 Microsoft_UI_Xaml!CControlBase::ScriptCallback
05 Microsoft_UI_Xaml!CXcpDispatcher::OnScriptCallback
06 Microsoft_UI_Xaml!CXcpDispatcher::OnWindowMessage
07 Microsoft_UI_Xaml!CXcpDispatcher::SendMessageW
08 Microsoft_UI_Xaml!CXcpBrowserHost::SyncScriptCallbackRequest
09 Microsoft_UI_Xaml!CEventManager::RaiseHelper
0a Microsoft_UI_Xaml!CEventManager::Raise
0b Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessXCPKeydown
0c Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessKeyboardInput
0d Microsoft_UI_Xaml!CInputManager::ProcessKeyboardInput
0e Microsoft_UI_Xaml!CInputServices::ProcessInput
0f Microsoft_UI_Xaml!CCoreServices::ProcessInput
10 Microsoft_UI_Xaml!CXcpBrowserHost::HandleInputMessage
11 Microsoft_UI_Xaml!CJupiterControl::HandleKeyMessage
12 Microsoft_UI_Xaml!CJupiterControl::HandleWindowMessage
13 Microsoft_UI_Xaml!CJupiterWindow::AcceleratorKeyActivated
14 Microsoft_UI_Xaml!CJupiterWindow::PreTranslateMessage
15 Microsoft_UI_Xaml!CXamlIslandRoot::PreTranslateMessage
16 Microsoft_UI_Xaml!DirectUI::PreTranslateHandler<CXamlIslandRoot>::OnDirectMessage
17 Microsoft_UI_Input!PreTranslateKeyboardInputWinRT::ProcessPreTranslateMessage_Callback::__l13::<lambda_1>::operator()
18 Microsoft_UI_Input!Microsoft::WRL2::ContextSession::LeaveSession_Callback<`PreTranslateKeyboardInputWinRT::ProcessPreTranslateMessage_Callback'::`13'::<lambda_1> >
19 Microsoft_UI_Input!PreTranslateKeyboardInputWinRT::ProcessPreTranslateMessage_Callback
1a Microsoft_UI_Input!std::_Func_class<bool,IPreTranslateHandlerObject *>::operator()
1b Microsoft_UI_Input!InputSiteWinRT::ForEachPreTranslateHandlerObject
1c Microsoft_UI_Input!IslandInputSiteWinRT::PartnerCOM::ProcessPreTranslateKeyboardMessage
1d Microsoft_UI_Input!ContentIsland::ContentNodeProc_NoLock
1e Microsoft_UI_Windowing_Core!Core::YieldAndCall::ContentNodeHandler
1f Microsoft_UI_Windowing_Core!Content::ExternalContentNode::YieldAndCallOwner
20 Microsoft_UI_Windowing_Core!Content::ExternalContentNode::YieldAndCallOwner
21 Microsoft_UI_Windowing_Core!Content::ExternalContentNode::DoPreTranslate
22 Microsoft_UI_Windowing_Core!Content::ContentNode::RevalidateAndDoPreTranslate
23 Microsoft_UI_Windowing_Core!Content::ContentNodeManager::PreTranslateMessage
24 Microsoft_UI_Windowing_Core!Api::ReunionApi32::ContentPreTranslateMessage_Full
25 Microsoft_UI_Windowing_Core!ContentPreTranslateMessage_Full_ContextThunk
26 Microsoft_UI_Windowing_Core!Api::ReunionApi32::DoEnterAndFullPreTranslate
27 Microsoft_UI_Windowing_Core!Api::ReunionApi32::ContentPreTranslateMessage_QuickFilter_NoContextWrapper
28 Microsoft_UI_Xaml!DirectUI::FrameworkApplication::RunDesktopWindowMessageLoop
```
* Frames 27-17 are Content code handling PreTranslateMessage
* Frames 16-13 are Xaml's PreTranslateMessage handler - we detect that Tab was pressed and inject a WM_KEYDOWN for Tab
* Frames 12-0b are Xaml's keyboard processing code detecting and synchronously raising a UIElement.TabProcessing event
* Frames 0a-04 are Xaml raising the event
* Frames 03-00 are Xaml responding to its own TabProcessing event and updating the FocusManager


### Accelerators

What's traditionally considered to be accelerators is called from the regular key down processing code via
`KeyboardAcceleratorUtility::ProcessGlobalAccelerators` (in `dxaml/xcp/components/ContentRoot/KeyboardInputProcessor.cpp`).
Xaml keeps a list of all registered accelerators in
`ContentRoot.m_allLiveKeyboardAccelerators` (in `dxaml/xcp/components/ContentRoot/inc/ContentRoot.h`),
and we walk this list if key down processing didn't mark the key as handled.

#### Sample breakpoint

`bu Microsoft_UI_Xaml!DirectUI::XamlUICommand::ExecuteImpl`

(Can be triggered with WinUI 3 Gallery's XamlUICommand page)

```
0:000> kc
 # Call Site
00 Microsoft_UI_Xaml!DirectUI::XamlUICommand::ExecuteImpl
01 Microsoft_UI_Xaml!DirectUI::XamlUICommandGenerated::Execute
02 Microsoft_UI_Xaml!DirectUI::ButtonBase::ExecuteCommand
03 Microsoft_UI_Xaml!DirectUI::ButtonBase::OnClick
04 Microsoft_UI_Xaml!DirectUI::Button::OnClick
05 Microsoft_UI_Xaml!DirectUI::AppBarButton::OnClick
06 Microsoft_UI_Xaml!DirectUI::ButtonBase::ProgrammaticClick
07 Microsoft_UI_Xaml!DirectUI::ButtonAutomationPeer::InvokeImpl
08 Microsoft_UI_Xaml!DirectUI::ButtonAutomationPeerGenerated::Invoke
09 Microsoft_UI_Xaml!DirectUI::AutomationPeer::UIAPatternInvoke
0a Microsoft_UI_Xaml!FxCallbacks::AutomationPeer_UIAPatternInvoke
0b Microsoft_UI_Xaml!CManagedInvokeProvider::Invoke
0c Microsoft_UI_Xaml!KeyboardAutomationInvoker::InvokeAutomationAction<CDependencyObject,CAutomationPeer,IUIAInvokeProvider,IUIAToggleProvider,IUIASelectionItemProvider,IUIAExpandCollapseProvider>
0d Microsoft_UI_Xaml!RaiseKeyboardAcceleratorInvoked
0e Microsoft_UI_Xaml!ProcessAllLiveAccelerators
0f Microsoft_UI_Xaml!KeyboardAcceleratorUtility::ProcessGlobalAccelerators
10 Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessKeyboardInput
11 Microsoft_UI_Xaml!ContentRootInput::KeyboardInputProcessor::ProcessKeyEvent
12 Microsoft_UI_Xaml!CInputManager::ProcessKeyEvent
13 Microsoft_UI_Xaml!CJupiterWindow::ProcessKeyEvents
14 Microsoft_UI_Xaml!CJupiterWindow::OnIslandKeyDown
15 Microsoft_UI_Xaml!CXamlIslandRoot::OnIslandKeyDown
16 Microsoft_UI_Xaml!CXamlIslandRoot::SubscribeToInputKeyboardSourceEvents::__l12::<lambda_2>::operator()
17 Microsoft_UI_Xaml!Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl ABI::Windows::Foundation::ITypedEventHandler_impl<ABI::Windows::Foundation::Internal::AggregateType<ABI::Microsoft::UI::Input::InputKeyboardSource *,ABI::Microsoft::UI::Input::IInputKeyboardSource *>,ABI::Windows::Foundation::Internal::AggregateType<ABI::Microsoft::UI::Input::KeyEventArgs *,ABI::Microsoft::UI::Input::IKeyEventArgs *> >::*)(ABI::Microsoft::UI::Input::IInputKeyboardSource *,ABI::Microsoft::UI::Input::IKeyEventArgs *)>::DelegateInvokeHelper<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<2>,ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Input::InputKeyboardSource *,ABI::Microsoft::UI::Input::KeyEventArgs *>,Microsoft::WRL::FtmBase>,`CXamlIslandRoot::SubscribeToInputKeyboardSourceEvents'::`12'::<lambda_2> &,1,ABI::Microsoft::UI::Input::IInputKeyboardSource *,ABI::Microsoft::UI::Input::IKeyEventArgs *>::Invoke
18 Microsoft_UI_Input!Microsoft::WRL::Details::CreateAgileHelper::__l2::<lambda_1>::operator()
19 Microsoft_UI_Input!Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl Windows::Foundation::ITypedEventHandler_impl<Windows::Foundation::Internal::AggregateType<Microsoft::UI::Input::InputKeyboardSource *,Microsoft::UI::Input::IInputKeyboardSource *>,Windows::Foundation::Internal::AggregateType<Microsoft::UI::Input::KeyEventArgs *,Microsoft::UI::Input::IKeyEventArgs *> >::*)(Microsoft::UI::Input::IInputKeyboardSource *,Microsoft::UI::Input::IKeyEventArgs *)>::DelegateInvokeHelper<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<2>,Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputKeyboardSource *,Microsoft::UI::Input::KeyEventArgs *>,Microsoft::WRL::FtmBase>,`Microsoft::WRL::Details::CreateAgileHelper<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputKeyboardSource *,Microsoft::UI::Input::KeyEventArgs *> >'::`2'::<lambda_1>,-1,Microsoft::UI::Input::IInputKeyboardSource *,Microsoft::UI::Input::IKeyEventArgs *>::Invoke
1a Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::EnteringMoveSizeEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll::__l2::<lambda_1>::operator()
1b Microsoft_UI_Input!Microsoft::WRL::InvokeTraits<-2>::InvokeDelegates<`Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::EnteringMoveSizeEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll<InputNonClientPointerSourceWinRT::Api *,EnteringMoveSizeEventArgs *>'::`2'::<lambda_1>,Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputNonClientPointerSource *,Microsoft::UI::Input::EnteringMoveSizeEventArgs *> >
1c Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputFocusController *,Microsoft::UI::Input::FocusChangedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::DoInvoke
1d Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Input::InputFocusController *,Microsoft::UI::Input::FocusChangedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll<InputFocusControllerWinRT::Api *,Microsoft::UI::Input::IFocusChangedEventArgs *>
1e Microsoft_UI_Input!KeyboardInputWinRT::OnKeyUpEvent_Callback::__l6::<lambda_1>::operator()
1f Microsoft_UI_Input!Microsoft::WRL2::ContextSession::LeaveSession_Callback<`KeyboardInputWinRT::OnKeyUpEvent_Callback'::`6'::<lambda_1> >
20 Microsoft_UI_Input!Microsoft::WRL2::ContextRuntimeClass::LeaveSessionAndValidate_Callback
21 Microsoft_UI_Input!KeyboardInputWinRT::OnKeyDownEvent_Callback
```
* Note the bottom of the stack - this is _not_ from PreTranslateMessage
* Frames 21-17 are lifted input handling a normal key down message
* Frames 16-10 are Xaml's normal key down processing, which didn't mark the key as handled
* Frames 0f-0d are Xaml checking its registered accelerators
* Frames 0c-00 are clicking the button associated with the accelerator - note that we actually go through a UIAutomation
  peer object to do the click
