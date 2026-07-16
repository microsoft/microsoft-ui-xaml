#pragma once

#include <unknwn.h>
#include <ocidl.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <xamlom.h>
#include <thread>
#include <vector>
#include <memory>
#include <winrt/windows.data.json.h>
#include <winrt/microsoft.UI.Dispatching.h>

struct __declspec(uuid("c9f2ef77-1b6a-4810-a490-6ea6a06bf7cb"))
    XamlProfilerTap : winrt::implements<XamlProfilerTap, IObjectWithSite, IVisualTreeServiceCallback2>
{
public:
    XamlProfilerTap() {}

    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown* pUnkSite);
    STDMETHOD(GetSite)(REFIID riid, void** ppvSite);

    // IVisualTreeServiceCallback
    STDMETHOD(OnVisualTreeChange)(ParentChildRelation relation, VisualElement element, VisualMutationType mutationType);

    // IVisualTreeServiceCallback2
    STDMETHOD(OnElementStateChanged)(InstanceHandle element, VisualElementState elementState, LPCWSTR context);

    static void SetModuleHandle(HMODULE hModule) { s_hModule = hModule; }

private:
    // One transparent capture overlay per live window (XamlRoot). Pick mode opens one of
    // these on EVERY window so the user can pick an element in any window, not just the
    // first one seen. Each overlay remembers the XamlRoot + content it covers so its
    // pointer handlers hit-test and convert coordinates against the correct window.
    struct PickOverlay
    {
        winrt::Microsoft::UI::Xaml::XamlRoot root{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::Primitives::Popup overlay{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::Border surface{ nullptr };
        winrt::event_token movedToken{};
        winrt::event_token pressedToken{};
        winrt::event_token wheelToken{};
    };

    // Add a window's XamlRoot to m_xamlRoots if not already present (distinct by identity).
    void RememberXamlRoot(winrt::Microsoft::UI::Xaml::XamlRoot const& root);

    void HighlightElement(InstanceHandle element);
    void HighlightVisual(uint64_t visualId);
    void ClearHighlight();

    // App -> profiler "pick" mode: while active, hovering the target app previews a
    // highlight on the element under the cursor and a click selects it back in the
    // profiler. While picking we cover the app with a full-window transparent overlay
    // (a Popup) that is the top-most hit-test target, so it captures ALL pointer input and
    // the app's controls never act (the app is "frozen" for input, like VS Live Visual
    // Tree's pick mode). Hover/preview hit-tests the tree underneath the overlay; a click
    // on the overlay commits the element under the cursor. This is fully in-process (no
    // WH_MOUSE_LL hook, which is denied inside a packaged/sandboxed app).
    void StartPick();
    void StopPick();
    void CommitPick(InstanceHandle handle);
    void LogResponsibleVisual(InstanceHandle handle);
    uint64_t ResolveElementVisualId(InstanceHandle handle);
    void SendTapInfo(std::wstring const& text);
    void OnPickPointerMoved(PickOverlay& po, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
    void OnPickPointerPressed(PickOverlay& po, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
    void OnPickPointerWheel(PickOverlay& po, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
    std::vector<InstanceHandle> HitTestStack(winrt::Microsoft::UI::Xaml::UIElement const& content, winrt::Windows::Foundation::Point const& pt);

    void WriteToPipe(const winrt::Windows::Data::Json::JsonObject& obj);
    void WriteLineToPipe(winrt::hstring string);

    static void ReaderThreadProc(wil::com_ptr<XamlProfilerTap> tap);

    // Json helpers
    void SetNamedPointerValue(winrt::Windows::Data::Json::JsonObject& obj, winrt::hstring name, InstanceHandle ptr);

    void Shutdown();

private:
    wil::com_ptr<IUnknown> m_site;
    wil::com_ptr<IXamlDiagnostics> m_xamlDiagnostics;
    wil::com_ptr<IVisualTreeService> m_visualTreeService;
    wil::unique_handle m_writePipe;
    wil::unique_handle m_readPipe;
    winrt::Microsoft::UI::Dispatching::DispatcherQueue m_dispatcherQueue = nullptr;
    bool m_shutdownCalled = false;

    winrt::Microsoft::UI::Composition::ContainerVisual m_highlightContainer{nullptr};
    winrt::weak_ref<winrt::Microsoft::UI::Xaml::UIElement> m_highlightHost;

    // All distinct live XamlRoots (one per window) discovered via OnVisualTreeChange.
    // HighlightVisual and pick mode iterate this so they work across every window of the
    // process, not just the first one seen.
    std::vector<winrt::Microsoft::UI::Xaml::XamlRoot> m_xamlRoots;

    // In-place adorner for an IVisual/Composition-only highlight: a translucent child
    // SpriteVisual stamped with Comment "__xp_adorner" (producer skips it). Tracked so
    // ClearHighlight can remove it from its parent's Children.
    winrt::Microsoft::UI::Composition::ContainerVisual m_visualAdornerParent{nullptr};
    winrt::Microsoft::UI::Composition::ContainerVisual m_visualAdorner{nullptr};

    // ---- Pick-mode state (app -> profiler hover/click selection) ----
    bool m_picking = false;
    InstanceHandle m_lastHitHandle = 0;     // last element previewed under the cursor
    // One full-window transparent capture overlay (a Popup) per live window, inserted on
    // top of the app while picking. Each is the top-most hit-test target for its window, so
    // it intercepts ALL pointer input there -- the app's controls never receive it, which
    // freezes the app for interaction during a pick. We resolve the element to preview/select
    // by hit-testing the owning window's content subtree (which structurally excludes the
    // overlay, since it lives in the popup root, not under content).
    std::vector<std::shared_ptr<PickOverlay>> m_pickOverlays;
    // Overlapping elements under the cursor, topmost-first (index 0 = innermost/topmost,
    // higher index = ancestor/outer). The mouse wheel drills along this list; a click
    // commits m_pickCandidates[m_pickIndex]. Rebuilt on each PointerMoved (resets to 0).
    std::vector<InstanceHandle> m_pickCandidates;
    size_t m_pickIndex = 0;
    // Signed wheel-delta accumulator. The wheel drills one element per physical notch
    // (WHEEL_DELTA == 120 units); high-resolution mice/trackpads emit many sub-notch
    // events per spin, so we accumulate here and only step once a full notch is reached,
    // keeping the sub-notch remainder. Reset whenever the candidate stack is rebuilt.
    int m_wheelAccum = 0;

    static HMODULE s_hModule;
};
