# WebView2 and UIA Providers

## How Microsoft.UI.Xaml.Controls creates a WebView2:

* WebView2.cpp creates an `CoreWebView2Environment` (called `EBWebViewEnvironment` on the Edge side).
  * Currently, we create a new environment for each WebView2 instance that is created. This could possibly change in the
  future. 
* After the Environment is created, we create a `CoreWebView2Controller` from that environment, which owns the `CoreWebView2` object.
  * On the Edge side, `EBWebViewEnvironment` creates a `Controller`. 
* When UIA tries to interact with the Xaml WebView2  control, it gets a `WebView2AutomationPeer` from 
  `WebView2::OnCreateAutomationPeer()`. Automation Peers are NOT providers, and neither owns the other, although the two
  usually know about each other.

## What happens in Core Xaml:

Xaml's implementation of `IRawElementProviderSimple` (and other IRawElementProvider*s) is `CUIAWrapper`. It is created
by `CUIAWindow`, which can be thought of as the provider for the CoreWindow's content. `CUIAWindow` is also used to host
XAML content for popups and XamlIslandRoots. (The name CUIAWindow may be misleading in this case). 

A CUIAWrapper is created by a CUIAWindow calling `CreateProviderForAP(CAutomationPeer* pAP, CUIAWrapper** ppRet)` inside
`CUIAWindow::GetOverrideProviderForHwndImpl()`. There are two parameters provided to the CUIAWrapper constructor that
are relevant here, and that we can’t get without help from the Edge WebView2:
* The element’s automation peer (in this case, `WebView2AutomationPeer`).
  * Since we’re in `GetOverrideProviderForHwndImpl()`, UIA gives us an HWND and we have to find the Automation Peer that
  goes with it to create the Wrapper for it. 
* The WebView’s input hwnd, which is then put in `UIAHostEnvironmentInfo`.
  * The `IRawElementProviderSimple` method `get_HostRawElementProvider()` always calls the UIA method 
  `UiaHostProviderFromHwnd(HWND)`. In the CUIAWrapper implementation we would need to give it the input hwnd.

These two problems might have been solved by directly asking the CoreWebView2 for its input HWND, but introducing uses
of HWNDs in new and open source code is not preferred (and strongly opposed by the Edge team).

## How Edge communicates UIA Provider information to Xaml

Instead, we implement the `IRawElementProviderSimple` on the Edge side and give it to Xaml so that Xaml has the 
information it needs.

* The `EmbeddedBrowserWebViewWindow` will create and own an `EmbeddedBrowserWebViewUIAProvider`, which implements 
`IRawElementProviderSimple`. Currently, this happens before (and regardless of) any UIA calls, but could be changed to 
create it only when queried by UIA.
  * Note: if we wanted this Provider to implement more RawElementProvider interfaces in the future, it could. More 
  details on this below.
>* Future: Instead of MUXC creating a new Environment each time it wants to create a WebView, we’ll reuse the same one.
>  * Create WebViewElementEnvironment class in MUXC.
>  * This class will have a public static method, GetOrCreateEnvironment(). When called, it will either use 
  CreateWebView2EnvironmentWithDetails to create an environment, or return one that has already been created.
>  * Question: Does it need to contain a list of the WebViews that are using this environment / delete itself when 
  they’re gone?
>  * Note: The “details” are browserExecutableFolder, userDataFolder, and additionalBrowserArguments. UserDataFolder is 
  the only one not hard coded (%LOCALAPPDATA%\Packages\MUXControlsTestApp_6f07fta6qpts2\AC). We don’t think this can 
  change between WebView creations in the same app, so reusing the environment should be safe.

* There are two ways to get an `IRawElementProviderSimple` from the Edge WebView2. Each is an API on the Edge 
side, with a corresponding method in Xaml's WebView2 element to get the Provider to Core Xaml.
  * EmbeddedBrowserWebView has a `get_UIAProvider()` that returns that WebView’s `EmbeddedBrowserWebViewUIAProvider`. 
  WebView2AutomationPeer has a method `GetRawElementProviderSimple()` that returns the 
  `EmbeddedBrowserWebViewUIAProvider` as an `IRawElementProviderSimple`.
  * EBWebViewEnvironment has a method `GetProviderForHwnd(HWND)`. This method takes in an HWND (that corresponds to an
  input HWND) and returns the `EmbeddedBrowserWebViewUIAProvider` of the corresponding WebView impl. The new 
  WebView2AutomationPeer has an `IsCorrectPeerForHwnd(HWND)` method that calls `get_UIAProvider()` and 
  `GetProviderForHwnd()` and returns true if the provider for this WebView is the same as the one that corresponds to 
  the given HWND.

WinUI consumes this new provider in two main ways:

* When creating the WebView2’s CUIAWrapper. 
  * Normally, UIA provides an HWND and we need to find the AutomationPeer that goes with it so we can create the 
  Wrapper. We do this in GetAPForHwnd by walking through the tree of Automation Peers until the AP’s InteropHwnd matches
  the hwnd we were given. 
  * For WebView2, we walk through the AutomationPeer tree calling `IsCorrectPeerForHwnd()` on any 
  WebView2AutomationPeers until we find the match. This shouldn't be a performance concern, since we only have to do 
  this tree walk once for each WebView2. As it is unlikely to have more than a handful of WebView2s in an app, this 
  shouldn’t get called too often.
* In CUIAWrapper’s implementation of `IRawElementProviderSimple` methods.
  * When we create the `CUIAWrapper`, we save the `EmbeddedBrowserWebViewUIAProvider` and then for each of the 
  `IRawElementProviderSimple` method implementations, ask the `EmbeddedBrowserWebViewUIAProvider` for its answer, 
  falling back on the CUIAWrapper implementation if necessary. This is most significant for the 
  `get_HostRawElementProvider()` method, since it required calling `UiaHostProviderFromHwnd(HWND)` with the input HWND,
  which the wrapper does not have. The `EmbeddedBrowserWebViewUIAProvider`, on the other hand, has no problem accessing
  it on the Edge side.

This approach has a few advantages. `EmbeddedBrowserWebViewUIAProvider` can be used by other third-party consumers in
their accessibility solutions. It can also be expanded to implement other providers, moving more logic out of 
CUIAWrapper and possibly eventually removing WebView2’s need for the Xaml wrapper altogether.

# Additional Reference information

## IRawElementProvider* Methods (implemented by CUIAWrapper)
``` cpp
// IRawElementProviderSimple methods
HRESULT get_ProviderOptions(_Out_ ProviderOptions * pRetVal);
HRESULT GetPatternProvider(_In_ PATTERNID patternId, _Out_ IUnknown ** pRetVal);
HRESULT GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT * pRetVal);
HRESULT get_HostRawElementProvider(_Out_ IRawElementProviderSimple ** pRetVal);
 
// IRawElementProviderSimple2 methods
HRESULT ShowContextMenu();
 
// IRawElementProviderFragment methods
HRESULT get_BoundingRectangle(_Out_ UiaRect * pRetVal);
HRESULT get_FragmentRoot(_Out_ IRawElementProviderFragmentRoot** pRetVal);
HRESULT GetEmbeddedFragmentRoots(_Out_ SAFEARRAY **pRetVal);
HRESULT GetRuntimeId(_Out_ SAFEARRAY ** pRetVal);
HRESULT Navigate(NavigateDirection direction, _Out_ IRawElementProviderFragment ** pRetVal);
HRESULT SetFocus();

// IRawElementProviderVisualRelative methods 
GetVisualRelativeBoundingRectangle(_Out_ UiaVisualRelativeRectangle* visualRelativeRect);
GetVisualRelativeCenterPoint(_Out_ UiaVisualRelativePoint* visualRelativePoint);
GetVisualRelativeClickablePoint(_Out_ UiaVisualRelativePoint* visualRelativePoint);

// IRawElementProviderAdviseEvents methods 
HRESULT AdviseEventAdded(_In_ EVENTID eventId, _Out_ SAFEARRAY *propertyIDs);
HRESULT AdviseEventRemoved(_In_ EVENTID eventId, _Out_ SAFEARRAY *propertyIDs);
```