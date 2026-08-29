 # UIA

## Table of Contents

- [Background](#background)
- [WM_GETOBJECT in WinUI 3](#wm_getobject-in-winui-3)
  - [IDesktopChildSiteBridge](#idesktopchildsitebridge)
  - [Xaml providers](#xaml-providers)
- [Endpoints and cross-process UIA trees](#endpoints-and-cross-process-uia-trees)
- [WebView2](#webview2)
  - [WebView2 UIA Hit Testing](#webview2-uia-hit-testing)
- [UIA Hit-Testing](#uia-hit-testing)
- [Updates for WCOS](#updates-for-wcos)
  - [UIA Hit-Testing](#uia-hit-testing-1)
  - [Visual-Relative Coordinates](#visual-relative-coordinates)
- [Updates for MAS Requirements](#updates-for-mas-requirements)

 ## Background

**UIA** is the Windows **UI A**utomation (aka Accessibility) framework. There are two sides of UIA:
* "Client" / Assistive Technology (AT) apps like Narrator that present and allow interaction with
* "Provider" apps like Word or a browser.

A UIA client (an AT tool like Narrator) talks to the Windows UIA APIs, then Windows UIA talks to the provider app using
the app-implemented Provider APIs. Provider interfaces allow the content's element tree to be navigated, text to be
read, buttons to be clicked, etc.

The Xaml framework implements many of these provider interfaces, so an app gets good accessibility support without doing
anything besides using Xaml. There are also ways for an application to augment its UIA support, for example using the
Xaml [AutomationProperties](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Automation.AutomationProperties) API.

The root of the communication between Windows UIA (UiaCore) and the Xaml implementation is the `CUIAWindow` class. Then,
there's a tree of `CUIAWrapper` class instances to represent the tree of `UIElements`. `CUIAWrappper` is where the UIA
COM interfaces, like `IRawElementProviderFragment`, are implemented. This is what UiaCore interacts with.

The way UiaCore gets the root of an application's provider interfaces is to send a
[WM_GETOBJECT](https://docs.microsoft.com/en-us/windows/win32/winauto/wm-getobject) message to an hwnd, and the
app/framework returns the provider interface by calling
[UiaReturnRawElementProvider](https://docs.microsoft.com/en-us/windows/win32/api/uiautomationcoreapi/nf-uiautomationcoreapi-uiareturnrawelementprovider)
from its `wndproc`. In System Xaml this is handled in
`CJupiterWindow::WindowProc` (see `dxaml/xcp/dxaml/lib/JupiterWindow.cpp`).

## WM_GETOBJECT in WinUI 3

There are two types of WinUI 3 apps - island apps and desktop apps. Island apps host WinUI 3 via
`DesktopWindowXamlSource` objects, which are each backed by a `IDesktopChildSiteBridge`. Desktop apps also have
`DesktopWindowXamlSource` objects, but they're hidden away inside Xaml's own
[`Window`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window?view=windows-app-sdk-1.3)
objects.

Regardless of the type of app, `WM_GETOBJECT` is handled the same way - by the internal `IDesktopChildSiteBridge`
notifying Xaml.

Note: WinUI 3 also supports UWP apps hosted in a CoreWindow, but that's only for our tests. An app in production will
never be UWP, so they're not covered here.

### IDesktopChildSiteBridge

The `IDesktopChildSiteBridge` is a Content object that internally creates an hwnd for the purposes of UIA (and keyboard
focus). This hwnd responds to `WM_GETOBJECT`.
The call internally plumbs through the
`ContentSite`
to the
`ContentIsland`,
where the island raises an event that someone is asking for a UIA provider.

This design abstracts away the details of hwnds and `WM_GETOBJECT` messages. A consumer of the Content layer just needs
to subscribe to the `ContentIsland`'s `AutomationProviderRequested` event and return a provider via the
`AutomationProvider` property of the event args, and that's exactly what Xaml
does (see `dxaml/xcp/core/core/elements/XamlIslandRoot.cpp`).

The full callstack (as of WASDK 1.3) can be seen here:

```
00  Microsoft_ui_xaml!CXamlIslandRoot::OnContentAutomationProviderRequested
01  Microsoft_ui_xaml!CContentRoot::OnAutomationProviderRequested+0x12
02  Microsoft_ui_xaml!CContentRoot::RegisterCompositionContent::__l9::<lambda_2>::operator()+0x16
03  Microsoft_ui_xaml!Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl ABI::Windows::Foundation::ITypedEventHandler_impl<ABI::Windows::Foundation::Internal::AggregateType<ABI::Microsoft::UI::Content::ContentIsland *,ABI::Microsoft::UI::Content::IContentIsland *>,ABI::Windows::Foundation::Internal::AggregateType<ABI::Microsoft::UI::Content::ContentIslandAutomationProviderRequestedEventArgs *,ABI::Microsoft::UI::Content::IContentIslandAutomationProviderRequestedEventArgs *> >::*)(ABI::Microsoft::UI::Content::IContentIsland *,ABI::Microsoft::UI::Content::IContentIslandAutomationProviderRequestedEventArgs *)>::DelegateInvokeHelper<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<2>,ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Content::ContentIsland *,ABI::Microsoft::UI::Content::ContentIslandAutomationProviderRequestedEventArgs *>,Microsoft::WRL::FtmBase>,`CContentRoot::RegisterCompositionContent'::`9'::<lambda_2> &,1,ABI::Microsoft::UI::Content::IContentIsland *,ABI::Microsoft::UI::Content::IContentIslandAutomationProviderRequestedEventArgs *>::Invoke+0x1c
04  Microsoft_UI_Input!Microsoft::WRL::Details::CreateAgileHelper::__l2::<lambda_7c7ffc8f06f4ccd81ea0d583a8747962>::operator()+0x3d
05  Microsoft_UI_Input!Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl Windows::Foundation::ITypedEventHandler_impl<Windows::Foundation::Internal::AggregateType<Microsoft::UI::Content::ContentIsland *,Microsoft::UI::Content::IContentIsland *>,Windows::Foundation::Internal::AggregateType<Microsoft::UI::Content::ContentIslandAutomationProviderRequestedEventArgs *,Microsoft::UI::Content::IContentIslandAutomationProviderRequestedEventArgs *> >::*)(Microsoft::UI::Content::IContentIsland *,Microsoft::UI::Content::IContentIslandAutomationProviderRequestedEventArgs *)>::DelegateInvokeHelper<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<2>,Windows::Foundation::ITypedEventHandler<Microsoft::UI::Content::ContentIsland *,Microsoft::UI::Content::ContentIslandAutomationProviderRequestedEventArgs *>,Microsoft::WRL::FtmBase>,<lambda_7c7ffc8f06f4ccd81ea0d583a8747962>,-1,Microsoft::UI::Content::IContentIsland *,Microsoft::UI::Content::IContentIslandAutomationProviderRequestedEventArgs *>::Invoke+0x5f
06  Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Content::ContentIsland *,Microsoft::UI::Content::ContentIslandAutomationProviderRequestedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll::__l2::<lambda_6eb18377d7e25b944d09ab9ac7904eac>::operator()+0x28
07  Microsoft_UI_Input!Microsoft::WRL::InvokeTraits<-2>::InvokeDelegates<<lambda_6eb18377d7e25b944d09ab9ac7904eac>,Windows::Foundation::ITypedEventHandler<Microsoft::UI::Content::ContentIsland *,Microsoft::UI::Content::ContentIslandAutomationProviderRequestedEventArgs *> >+0x73
08  Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Content::ContentIsland *,Microsoft::UI::Content::ContentIslandAutomationProviderRequestedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::DoInvoke<<lambda_6eb18377d7e25b944d09ab9ac7904eac> >+0x7d
09  Microsoft_UI_Input!Microsoft::WRL::EventSource<Windows::Foundation::ITypedEventHandler<Microsoft::UI::Content::ContentIsland *,Microsoft::UI::Content::ContentIslandAutomationProviderRequestedEventArgs *>,Microsoft::WRL::InvokeModeOptions<-2> >::InvokeAll+0x35
0a  Microsoft_UI_Input!ContentIsland::TryGetAutomationProvider_Callback::__l13::<lambda_b42b80e68593be170ede19a084ae5db2>::operator()+0x47
0b  Microsoft_UI_Input!Microsoft::WRL2::ContextSession::LeaveSession_Callback<<lambda_b42b80e68593be170ede19a084ae5db2> >+0x71
0c  Microsoft_UI_Input!Microsoft::WRL2::ContextRuntimeClass::LeaveSessionAndValidate_Callback+0x19
0d  Microsoft_UI_Input!ContentIsland::TryGetAutomationProvider_Callback+0xff
0e  Microsoft_UI_Input!ContentSite::Api::TryGetAutomationProvider+0xc0
0f  Microsoft_UI_Input!DesktopSiteBridge::HandleUiaHostProvider+0x16f
10  Microsoft_UI_Input!DesktopSiteBridge::WndProc+0x11a
11  Microsoft_UI_Input!DesktopChildSiteBridge::WndProc+0x103
12  Microsoft_UI_Input!DesktopSiteBridge::WndProcStatic+0x81
13  USER32!UserCallWinProcCheckWow+0x2d1
14  USER32!CallWindowProcAorW+0x6b
```

### Xaml providers

The Content objects route the `WM_GETOBJECT` request to Xaml, but it's still up to Xaml to provide an object that can
respond to the various UIA queries inside the Xaml island.

> todo - CUIAWindow/automation peers
>
> wishlist:
> - what are automation peers? what controls get one?
> - how do UIA clients navigate the UIA tree? first child/next child?
> - how are popups handled? why does the same popup show up in multiple places?

## Endpoints and cross-process UIA trees

> todo - figure out what these things are. Not many docs available.

UIA Endpoints are a feature for stitching multiple UIA trees together across processes. Xaml uses it for WebView2, for
example, to connect the tree of HTML controls inside the WebView2 process with the tree of Xaml controls in the Xaml
process. See the [WebView2](#webview2) section below or the [WebView2 accessibility doc](../../controls/dev/WebView2/WebView2-Accessibility.md) for more details.

When Content enables cross-process islands, UIA Endpoints will be involved in WM_GETOBJECT handling. The hwnd that receives the message is the host (i.e. the bridge and the ContentSite), while the island connected to the bridge/site will live in a different process. UIA Endpoints will be used to connect the two.

## WebView2

Similar to the Xaml WebView control (based on Spartan Edge), the WinUI3 WebView**2** control (based on Anaheim Edge) has
its own UIA providers for its tree. So a Xaml WebView2 control in a Xaml Window means that we're merging two trees into
a single UIA tree.

To accomplish this, `CUIAWrapper` forwards most calls to the root Anaheim provider. But that's a delegation, so
sometimes it adds to that: `CUIAWrapper`'s
[IRawElementProviderSimple::GetPropertyValue](https://docs.microsoft.com/en-us/windows/win32/api/uiautomationcore/nf-uiautomationcore-irawelementprovidersimple-getpropertyvalue)
forwards to the Anaheim provider, but if it doesn't return a value then `CUIAWrapper` attempts to get the property value
itself. For example this is how
[AutomationProperties.GetName](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Automation.AutomationProperties.GetName)
is supported on WebView2.

In particular it's important for `CUIAWrapper`'s implementation of `get_HostRawElementProvider` to forward to Anaheim;
the value returned from that API is the return from
[UiaHostProviderFromHwnd](https://docs.microsoft.com/en-us/windows/win32/api/uiautomationcoreapi/nf-uiautomationcoreapi-uiahostproviderfromhwnd),
which is something that only the Anaheim provider has, since it is created from the hwnd that it has internally.

### WebView2 UIA Hit Testing

WebView2 uses a input hwnd between Xaml and the Anaheim Edge browser hwnd that renders content. This hwnd is of size
(0,0), but that causes an issue for UIA, because zero-sized hwnd confuses hit-testing.

To solve that problem there are UIA-specific properties you can set on an hwnd to tell it what UIA should consider to be
the hwnd's location and size:
* `UIA_HWNDXOffset`
* `UIA_HWNDYOffset`
* `UIA_HWNDWidth`
* `UIA_HWNDHeight`

The input hwnd should have UIA properties set to the same size and location as the browser hwnd, so UIA hit testing
can drill down through the input window and find elements in the browser.

## UIA Hit-Testing

The entrypoint into an app is the provider returned from `WM_GETOBJECT`. The UIA client can use that provider and UIA
APIs to walk the element tree. Alternatively, the client can do a hit-test against a point with that provider and get an
element back.

The UIA client-side API is for hit testing is
[IUIAutomation::ElementFromPoint](https://docs.microsoft.com/en-us/windows/win32/api/uiautomationclient/nf-uiautomationclient-iuiautomation-elementfrompoint).
On the Xaml end, this shows up at the provider as
[IRawElementProviderFragmentRoot.ElementProviderFromPoint](https://docs.microsoft.com/en-us/dotnet/api/system.windows.automation.provider.irawelementproviderfragmentroot.elementproviderfrompoint?view=netcore-3.1)
(implemented in `CUIAWrapper`).

The Point that come in to the provider are screen coordinates and need to be scaled. For example if the system scaling
is 150% and the Point.X is 150, it's converted to 100 before hit-testing the `UIElement` tree.

## Updates for Lightweight Compositing Mode

> todo - is this entire section obsolete?

### UIA Hit-Testing
WinUI 3.0 only supports Desktop SKU, where there's a real hwnd on which content is rendered and to which input is sent.
In the future, in lightweight compositing mode, there is still an hwnd used internally for input, but output is not actually rendered to the
hwnd. And because of that, the hwnd is sized to (0,0). This causes an issue for UIA, because zero-sized hwnd confuses
hit-testing.

To solve that problem there are UIA-specific properties you can set on an hwnd to tell it what UIA should consider to be
the hwnd's location and size:
* `UIA_HWNDXOffset`
* `UIA_HWNDYOffset`
* `UIA_HWNDWidth`
* `UIA_HWNDHeight`

> These UIA properties will need to be set on the child hwnds, like is done for Anaheim Edge (see above).

### Visual-Relative Coordinates

There are three APIs in UIA that describe location in coordinates:
* `BoundingRectangle`
* `CenterPoint`
* `ClickablePoint`

For Desktop SKU those are in desktop coordinates. But in lightweight compositing mode, the provider doesn't know the desktop coordinates
(there's no GetClientRect API). So instead the coordinates are provided relative to a system Visual.

The problem with lifted is that Xaml doesn't have a _system_ Visual. So we'll have some kind of similar solution in
lifted as in system, but it won't be exactly the same.

## Updates for MAS Requirements

To meet WCAG2.1-based MAS (Microsoft Accessibility Standards) requirements:

In order to meet the new WCAG2.1 accessibility standards, the Settings Framework is planning to migrate to WinUI so it
can take advantage of standardized and upgraded controls. However, some of the WinUI controls did not meet these new
standards yet either. WinUI controls needed to be made WCAG2.1
compliant.
