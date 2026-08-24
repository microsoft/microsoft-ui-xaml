#include "pch.h"
#include <algorithm>
#include <functional>
#include "Tap.h"
#include <unknwn.h>
#include <ocidl.h>
#include <string>
#include <string_view>
#include <wil/com.h>
#include <wil/resource.h>
#include <xamlom.h>
#include <thread>
#include <winrt/windows.data.json.h>
#include <winrt/windows.system.h>

using namespace std::string_view_literals;

HMODULE XamlProfilerTap::s_hModule = nullptr;

// ---- Diagnostic logger ------------------------------------------------------
// Emits to the debug console (OutputDebugString) so it shows in the Visual Studio
// Output window when the target app is run under the debugger. Lets us compare the
// command the profiler SENT against what the tap actually RECEIVED + resolved.
static void TapLog(const std::wstring& msg)
{
    OutputDebugString((L"[XamlProfilerTap] " + msg + L"\n").c_str());
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        XamlProfilerTap::SetModuleHandle(hModule);
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

STDMETHODIMP XamlProfilerTap::SetSite(IUnknown* pUnkSite)
{
    m_site = pUnkSite;
    m_dispatcherQueue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();

    m_site.query_to(&m_xamlDiagnostics);
    m_site.query_to(&m_visualTreeService);

    wil::unique_bstr initializationData;
    FAIL_FAST_IF_FAILED(m_xamlDiagnostics->GetInitializationData(&initializationData));
    if (initializationData)
    {
        // TODO:
        typedef IInspectable* (_stdcall* pfnGetWindowRoot)();
        void* pWriteHandle = nullptr;
        void* pReadHandle = nullptr;
        swscanf_s(initializationData.get(), L"%p %p", &pWriteHandle, &pReadHandle);

        m_writePipe = wil::unique_handle((HANDLE)pWriteHandle);
        m_readPipe = wil::unique_handle((HANDLE)pReadHandle);
        //DWORD writeBufferSize = 0;
        //GetNamedPipeInfo(m_writePipe.get(), nullptr, &writeBufferSize, nullptr, nullptr);

        winrt::Windows::Data::Json::JsonObject obj = winrt::Windows::Data::Json::JsonObject();
        obj.SetNamedValue(L"TapType", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"Connected"));
        // Send our module handle so the snoop app can eject us via FreeLibrary
        if (s_hModule)
        {
            wchar_t buf[32];
            swprintf_s(buf, L"0x%p", s_hModule);
            obj.SetNamedValue(L"ModuleHandle", winrt::Windows::Data::Json::JsonValue::CreateStringValue(buf));
        }
        WriteToPipe(obj);
        TapLog(L"SetSite: tap attached, 'Connected' sent; advising visual-tree enumeration");

        // Register for tree change events. This must happen on a background thread,
        // since AdviseVisualTreeChange() calls Advising::RunOnUIThread() which waits
        // for that call to finish, and since SetSite() gets called on the UI thread,
        // making that call here or even waiting for the background thread will cause
        // a hang.
        auto f = [](XamlProfilerTap* tap) {
            tap->m_visualTreeService->AdviseVisualTreeChange(tap);
            OutputDebugString(L"advise thread exiting\n");
        };
        std::thread adviseThread(f, this);
        adviseThread.detach(); // just let it run -- it will exit right away

        // Start the reader thread
        std::thread readerThread(ReaderThreadProc, this);
        readerThread.detach();
    }

    // We are responsible for the lifetime of our own object at this point. A Detach message
    // will release memory.
    AddRef();

    return S_OK;
}

// The reader thread listens for commands from the Snoop app.
void XamlProfilerTap::ReaderThreadProc(wil::com_ptr<XamlProfilerTap> tap)
{
    byte buffer[1026];
    DWORD dwRead = 0;
    for (;;)
    {
        BOOL success = ReadFile(tap->m_readPipe.get(), buffer, ARRAYSIZE(buffer) - 2, &dwRead, nullptr);
        if (!success || dwRead == 0)
            break;

        buffer[dwRead] = 0;
        buffer[dwRead + 1] = 0;

        std::wstring_view command = (const wchar_t*)buffer;
        const wchar_t BYTE_ORDER_MARK = 0xFEFF;
        if (command[0] == BYTE_ORDER_MARK)
            command.remove_prefix(1);
        TapLog(L"RECV command: \"" + std::wstring(command) + L"\"");
        if (command._Starts_with(L"HIGHLIGHT-VISUAL"sv))
        {
            unsigned long long id = 0;
            swscanf_s(command.data(), L"HIGHLIGHT-VISUAL: %llx", &id);

            wchar_t dbg[80];
            swprintf_s(dbg, L"HIGHLIGHT-VISUAL parsed id = 0x%llx", id);
            TapLog(dbg);

            uint64_t visualId = static_cast<uint64_t>(id);
            auto callback = [visualId, tap]() {
                tap->HighlightVisual(visualId);
            };
            tap->m_dispatcherQueue.TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueueHandler(callback));
        }
        else if (command._Starts_with(L"HIGHLIGHT"sv))
        {
            void* p = nullptr;
            swscanf_s(command.data(), L"HIGHLIGHT: %p", &p);
            InstanceHandle element = (InstanceHandle)p;

            wchar_t dbg[64];
            swprintf_s(dbg, L"HIGHLIGHT parsed handle = 0x%p", p);
            TapLog(dbg);

            auto callback = [element, tap]() {
                tap->HighlightElement(element);
            };
            tap->m_dispatcherQueue.TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueueHandler(callback));
        }
        else if (command._Starts_with(L"CLEAR-HIGHLIGHT"sv))
        {
            auto callback = [tap]() {
                tap->ClearHighlight();
            };
            tap->m_dispatcherQueue.TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueueHandler(callback));
        }
        else if (command._Starts_with(L"START-PICK"sv))
        {
            auto callback = [tap]() { tap->StartPick(); };
            tap->m_dispatcherQueue.TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueueHandler(callback));
        }
        else if (command._Starts_with(L"STOP-PICK"sv))
        {
            auto callback = [tap]() { tap->StopPick(); };
            tap->m_dispatcherQueue.TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueueHandler(callback));
        }
        else if (command._Starts_with(L"CLOSE"sv))
        {
            tap->Shutdown();
            break;
        }
    }
    OutputDebugString(L"reader thread exiting\n");
}

void XamlProfilerTap::Shutdown()
{
    if (m_shutdownCalled) return;
    m_shutdownCalled = true;

    OutputDebugString(L"XamlProfilerTap::Shutdown - begin\n");

    // 1. Stop receiving tree change notifications
    if (m_visualTreeService)
    {
        m_visualTreeService->UnadviseVisualTreeChange(this);
    }

    // 2. Clean up XAML objects on the UI thread (highlight overlay)
    if (m_dispatcherQueue)
    {
        // Use an event to wait for UI cleanup to complete
        wil::unique_event cleanupDone(wil::EventOptions::ManualReset);
        HANDLE hEvent = cleanupDone.get();

        auto callback = [this, hEvent]() {
            StopPick();
            ClearHighlight();
            m_highlightContainer = nullptr;
            SetEvent(hEvent);
        };

        if (m_dispatcherQueue.TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueueHandler(callback)))
        {
            // Wait up to 2 seconds for UI cleanup
            WaitForSingleObject(hEvent, 2000);
        }
    }

    // 3. Close pipes
    m_writePipe.reset();
    m_readPipe.reset();

    // 4. Release COM references
    m_visualTreeService = nullptr;
    m_xamlDiagnostics = nullptr;
    m_site = nullptr;
    m_dispatcherQueue = nullptr;

    OutputDebugString(L"XamlProfilerTap::Shutdown - complete\n");

    // 5. Balance the AddRef() from SetSite on a separate thread.
    //    The actual DLL unload is done by the snoop app via CreateRemoteThread + FreeLibrary,
    //    since the DLL may have multiple LoadLibrary refs from XAML diagnostics/COM.
    XamlProfilerTap* rawThis = this;
    std::thread releaseThread([rawThis]() {
        // Balance the AddRef() from SetSite
        rawThis->Release();
        OutputDebugString(L"XamlProfilerTap::Shutdown - Release done\n");
    });
    releaseThread.detach();
}

void XamlProfilerTap::WriteToPipe(const winrt::Windows::Data::Json::JsonObject& obj)
{
    WriteLineToPipe(obj.Stringify());
}

void XamlProfilerTap::WriteLineToPipe(winrt::hstring string)
{
    if (m_shutdownCalled || !m_writePipe) return;
    LOG_IF_WIN32_BOOL_FALSE(WriteFile(m_writePipe.get(), string.c_str(), string.size() * 2, nullptr, nullptr));
    LOG_IF_WIN32_BOOL_FALSE(WriteFile(m_writePipe.get(), L"\n", 1 * 2, nullptr, nullptr));
}

STDMETHODIMP XamlProfilerTap::GetSite(REFIID /*riid*/, void** /*ppvSite*/)
{
    //return m_site == nullptr ? E_FAIL : m_site.Get()->QueryInterface(riid, ppvSite);
    return E_NOTIMPL;
}

void XamlProfilerTap::SetNamedPointerValue(winrt::Windows::Data::Json::JsonObject& obj, winrt::hstring name, InstanceHandle ptr)
{
    wchar_t buf[32] = {};
    swprintf_s(buf, L"%p", (void*)ptr);
    obj.SetNamedValue(name, winrt::Windows::Data::Json::JsonValue::CreateStringValue(buf));
}

// Draw a semi-transparent overlay over the given element on the target app's UI thread.
//
// We deliberately do NOT mutate the XAML visual tree (no Popup, no injected Rectangle):
// any UIElement we added would be picked up by IVisualTreeService::OnVisualTreeChange
// and surface as a phantom node in the snoop's tree.
//
// Instead we use a Composition SpriteVisual attached to the XamlRoot's root content via
// ElementCompositionPreview.SetElementChildVisual -- this is the same mechanism Visual
// Studio's Live Visual Tree adorner uses. Composition visuals live below XAML, so they
// are invisible to IVisualTreeService and they don't participate in layout or hit-testing.
// Multiple sprites can be stacked in the container for future margin/padding adornments.
void XamlProfilerTap::HighlightElement(InstanceHandle element)
{
    if (!element)
    {
        TapLog(L"HighlightElement: handle is null -> clearing");
        ClearHighlight();
        return;
    }

    winrt::Windows::Foundation::IInspectable inspectable;
    if (FAILED(m_xamlDiagnostics->GetIInspectableFromHandle(element, (::IInspectable**)winrt::put_abi(inspectable))) || !inspectable)
    {
        TapLog(L"HighlightElement: GetIInspectableFromHandle FAILED (handle not in cache)");
        return;
    }

    auto fe = inspectable.try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
    if (!fe)
    {
        TapLog(L"HighlightElement: resolved object is not a FrameworkElement");
        return;
    }

    auto xamlRoot = fe.XamlRoot();
    if (!xamlRoot)
    {
        TapLog(L"HighlightElement: element has no XamlRoot");
        return;
    }

    auto host = xamlRoot.Content();
    if (!host)
    {
        TapLog(L"HighlightElement: XamlRoot has no Content host");
        return;
    }

    double width = fe.ActualWidth();
    double height = fe.ActualHeight();
    if (width <= 0 || height <= 0)
    {
        TapLog(L"HighlightElement: element has zero size (ActualWidth/Height == 0)");
        return;
    }

    winrt::Windows::Foundation::Point offset{ 0, 0 };
    try
    {
        auto transform = fe.TransformToVisual(host);
        offset = transform.TransformPoint({ 0, 0 });
    }
    catch (...)
    {
        TapLog(L"HighlightElement: TransformToVisual threw");
        return;
    }

    {
        wchar_t dbg[160];
        swprintf_s(dbg, L"HighlightElement: drawing overlay at (%.1f, %.1f) size %.1f x %.1f",
            offset.X, offset.Y, width, height);
        TapLog(dbg);
    }

    namespace MUXH = winrt::Microsoft::UI::Xaml::Hosting;

    // If the XamlRoot (i.e. the island) changed, detach from the previous host first.
    auto previousHost = m_highlightHost.get();
    if (previousHost && previousHost != host)
    {
        MUXH::ElementCompositionPreview::SetElementChildVisual(previousHost, nullptr);
        m_highlightContainer = nullptr;
    }

    if (!m_highlightContainer)
    {
        auto hostVisual = MUXH::ElementCompositionPreview::GetElementVisual(host);
        auto compositor = hostVisual.Compositor();
        m_highlightContainer = compositor.CreateContainerVisual();
        m_highlightContainer.Comment(L"__xp_adorner"); // producer skips this so the box adorner never enters the tree
        MUXH::ElementCompositionPreview::SetElementChildVisual(host, m_highlightContainer);
        m_highlightHost = host;
    }

    auto compositor = m_highlightContainer.Compositor();
    m_highlightContainer.Children().RemoveAll();

    // Also clear any in-place IVisual adorner (HighlightVisual) so the two highlight kinds
    // never coexist — switching from an IVisual highlight to an element highlight replaces it.
    if (m_visualAdorner && m_visualAdornerParent)
    {
        try { m_visualAdornerParent.Children().Remove(m_visualAdorner); } catch (...) {}
    }
    m_visualAdorner = nullptr;
    m_visualAdornerParent = nullptr;

    // Read margin (FrameworkElement) and padding (only on a handful of types).
    auto margin = fe.Margin();
    winrt::Microsoft::UI::Xaml::Thickness padding{ 0, 0, 0, 0 };
    if (auto ctrl = inspectable.try_as<winrt::Microsoft::UI::Xaml::Controls::Control>())
        padding = ctrl.Padding();
    else if (auto border = inspectable.try_as<winrt::Microsoft::UI::Xaml::Controls::Border>())
        padding = border.Padding();
    else if (auto grid = inspectable.try_as<winrt::Microsoft::UI::Xaml::Controls::Grid>())
        padding = grid.Padding();
    else if (auto sp = inspectable.try_as<winrt::Microsoft::UI::Xaml::Controls::StackPanel>())
        padding = sp.Padding();
    else if (auto rp = inspectable.try_as<winrt::Microsoft::UI::Xaml::Controls::RelativePanel>())
        padding = rp.Padding();
    else if (auto cp = inspectable.try_as<winrt::Microsoft::UI::Xaml::Controls::ContentPresenter>())
        padding = cp.Padding();
    else if (auto ip = inspectable.try_as<winrt::Microsoft::UI::Xaml::Controls::ItemsPresenter>())
        padding = ip.Padding();

    // Box-model rectangles (CSS DevTools convention):
    //   margin rect  : element rect expanded by Margin (the slot the parent gave us)
    //   element rect : ActualWidth x ActualHeight at the transformed origin
    //   content rect : element rect shrunk by Padding
    float ex = static_cast<float>(offset.X);
    float ey = static_cast<float>(offset.Y);
    float ew = static_cast<float>(width);
    float eh = static_cast<float>(height);

    float ml = static_cast<float>(margin.Left);
    float mt = static_cast<float>(margin.Top);
    float mr = static_cast<float>(margin.Right);
    float mb = static_cast<float>(margin.Bottom);

    float pl = static_cast<float>(padding.Left);
    float pt = static_cast<float>(padding.Top);
    float pr = static_cast<float>(padding.Right);
    float pb = static_cast<float>(padding.Bottom);

    // Clamp content rect so absurd padding doesn't go negative.
    float cw = (std::max)(0.0f, ew - pl - pr);
    float ch = (std::max)(0.0f, eh - pt - pb);
    float cx = ex + pl;
    float cy = ey + pt;

    auto addFill = [&](float x, float y, float w, float h, winrt::Windows::UI::Color color)
    {
        if (w <= 0 || h <= 0)
            return;
        auto sprite = compositor.CreateSpriteVisual();
        sprite.Comment(L"__xp_adorner"); // producer skips this so the box adorner never enters the tree
        sprite.Offset({ x, y, 0.0f });
        sprite.Size({ w, h });
        sprite.Brush(compositor.CreateColorBrush(color));
        m_highlightContainer.Children().InsertAtTop(sprite);
    };

    // Add 4 strips around an "inner" rect that together fill an "outer" ring.
    // (outer = element + margin, inner = element)  for margin
    // (outer = element,          inner = content)  for padding
    auto addRing = [&](float ox, float oy, float ow, float oh,
                       float ix, float iy, float iw, float ih,
                       winrt::Windows::UI::Color color)
    {
        if (ow <= 0 || oh <= 0)
            return;
        // Top
        addFill(ox, oy, ow, iy - oy, color);
        // Bottom
        addFill(ox, iy + ih, ow, (oy + oh) - (iy + ih), color);
        // Left
        addFill(ox, iy, ix - ox, ih, color);
        // Right
        addFill(ix + iw, iy, (ox + ow) - (ix + iw), ih, color);
    };

    // DevTools-style palette. Content fill is a bold, clearly-visible blue with a thick
    // high-contrast outline so the Ctrl+Click highlight pops even over busy/colored UI.
    constexpr winrt::Windows::UI::Color marginColor  { 0x80, 0xF6, 0xB2, 0x6B }; // orange ~50%
    constexpr winrt::Windows::UI::Color paddingColor { 0x90, 0x93, 0xC4, 0x7D }; // green  ~56%
    constexpr winrt::Windows::UI::Color contentColor { 0xB0, 0x1E, 0x8A, 0xFF }; // vivid blue ~69%
    constexpr winrt::Windows::UI::Color borderColor  { 0xFF, 0xFF, 0x29, 0x00 }; // solid red-orange outline

    // Margin ring (outside element bounds).
    addRing(ex - ml, ey - mt, ew + ml + mr, eh + mt + mb,
            ex, ey, ew, eh, marginColor);

    // Padding ring (inside element bounds).
    addRing(ex, ey, ew, eh,
            cx, cy, cw, ch, paddingColor);

    // Content area fill (inside padding).
    addFill(cx, cy, cw, ch, contentColor);

    // Crisp element-rect outline drawn on top — thick so the selection is unmistakable.
    constexpr float borderThickness = 4.0f;
    addFill(ex,                       ey,                         ew,              borderThickness, borderColor); // top
    addFill(ex,                       ey + eh - borderThickness,  ew,              borderThickness, borderColor); // bottom
    addFill(ex,                       ey,                         borderThickness, eh,              borderColor); // left
    addFill(ex + ew - borderThickness, ey,                        borderThickness, eh,              borderColor); // right
}

// Highlight an IVisual / Composition-only node (one with no XAML peer handle).
//
// The profiler can't resolve these via XAML Diagnostics, so the producer (in mux's
// WucVisualTreeProfiler) stamps each live visual's Comment with "xpid:<IVisual* as hex>".
// That hex is exactly the node.Id the profiler shows. Here we walk the live Composition
// tree (rooted at the XamlRoot content's element visual), find the visual whose Comment
// matches, and adorn it in place with a translucent child SpriteVisual. The adorner rides
// the target's transform (RelativeSizeAdjustment 1,1) so no coordinate math is needed.
void XamlProfilerTap::HighlightVisual(uint64_t visualId)
{
    namespace WUC = winrt::Microsoft::UI::Composition;
    namespace MUXH = winrt::Microsoft::UI::Xaml::Hosting;

    // Always clear any previous in-place adorner first.
    if (m_visualAdorner && m_visualAdornerParent)
    {
        try { m_visualAdornerParent.Children().Remove(m_visualAdorner); } catch (...) {}
    }
    m_visualAdorner = nullptr;
    m_visualAdornerParent = nullptr;

    // Also clear the element-box adorner (HighlightElement) so the two highlight kinds never
    // coexist — switching from an element highlight to an IVisual highlight replaces it.
    if (m_highlightContainer)
    {
        try { m_highlightContainer.Children().RemoveAll(); } catch (...) {}
    }

    if (!visualId)
    {
        TapLog(L"HighlightVisual: id is 0 -> cleared");
        return;
    }

    wchar_t targetBuf[40];
    swprintf_s(targetBuf, L"xpid:%llx", visualId);
    std::wstring target = targetBuf;

    // Search every live window's composition tree (one per XamlRoot) for the visual whose
    // Comment matches; a picked visual can live in any window, not just the first one seen.
    std::vector<winrt::Microsoft::UI::Xaml::XamlRoot> roots = m_xamlRoots;
    if (roots.empty())
    {
        TapLog(L"HighlightVisual: no XamlRoot tracked yet (no tree change seen)");
        return;
    }

    // Depth-first search for the visual whose Comment matches, rooted at a given host visual.
    WUC::Visual found{ nullptr };
    std::function<bool(WUC::Visual const&)> dfs = [&](WUC::Visual const& v) -> bool
    {
        if (v.Comment() == winrt::hstring(target))
        {
            found = v;
            return true;
        }
        if (auto container = v.try_as<WUC::ContainerVisual>())
        {
            for (auto const& child : container.Children())
            {
                if (dfs(child))
                {
                    return true;
                }
            }
        }
        return false;
    };

    size_t rootIndex = 0;
    for (auto const& root : roots)
    {
        size_t thisIndex = rootIndex++;
        if (!root)
            continue;
        auto host = root.Content();
        if (!host)
            continue;
        auto hostVisual = MUXH::ElementCompositionPreview::GetElementVisual(host);
        if (dfs(hostVisual))
            break; // found it in this window

        // Not in this window's tree — tell the profiler which root was searched.
        wchar_t miss[160];
        swprintf_s(miss, L"HighlightVisual: '%s' not found in root [%zu] XamlRoot=0x%p",
            target.c_str(), thisIndex, (void*)winrt::get_abi(root));
        SendTapInfo(miss);
    }

    if (!found)
    {
        SendTapInfo(L"HighlightVisual: no live visual with matching Comment '" + target + L"' found in any tracked root");
        return;
    }

    auto container = found.try_as<WUC::ContainerVisual>();
    if (!container)
    {
        TapLog(L"HighlightVisual: matched visual is not a ContainerVisual; cannot adorn in place");
        return;
    }

    auto compositor = found.Compositor();

    // Match HighlightElement's DevTools look: a vivid-blue content fill under a thick,
    // high-contrast red-orange outline. HighlightElement knows the element's pixel rect and
    // draws fixed-offset strips; here the target visual's size is only known at composition
    // time, so we build the same look purely with RelativeSizeAdjustment / RelativeOffsetAdjustment
    // (each strip rides one edge of the target) and no coordinate math.
    constexpr winrt::Windows::UI::Color contentColor{ 0xB0, 0x1E, 0x8A, 0xFF }; // vivid blue ~69% (same as HighlightElement)
    constexpr winrt::Windows::UI::Color borderColor { 0xFF, 0xFF, 0x29, 0x00 }; // solid red-orange outline
    constexpr float borderThickness = 4.0f;

    // A single container adorner sized to the target; all fill/border strips are its children so
    // ClearHighlight only has to remove this one visual. It matches the target's size for free.
    auto adorner = compositor.CreateContainerVisual();
    adorner.Comment(L"__xp_adorner"); // producer skips this so it never enters the tree
    adorner.RelativeSizeAdjustment({ 1.0f, 1.0f });

    auto addStrip = [&](winrt::Windows::Foundation::Numerics::float2 relSize,
                        winrt::Windows::Foundation::Numerics::float2 size,
                        winrt::Windows::Foundation::Numerics::float3 relOffset,
                        winrt::Windows::Foundation::Numerics::float3 offset,
                        winrt::Windows::UI::Color color)
    {
        auto s = compositor.CreateSpriteVisual();
        s.Comment(L"__xp_adorner");
        s.RelativeSizeAdjustment(relSize);
        s.Size(size);
        s.RelativeOffsetAdjustment(relOffset);
        s.Offset(offset);
        s.Brush(compositor.CreateColorBrush(color));
        adorner.Children().InsertAtTop(s);
    };

    // Content fill: full target size.
    addStrip({ 1.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, contentColor);

    // Border outline: 4 edge strips that ride the target's edges via relative offset/size.
    // Top:    full width, fixed height, pinned to top.
    addStrip({ 1.0f, 0.0f }, { 0.0f, borderThickness }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, borderColor);
    // Bottom: full width, fixed height, pinned to bottom (relOffset y=1, pull back up by thickness).
    addStrip({ 1.0f, 0.0f }, { 0.0f, borderThickness }, { 0.0f, 1.0f, 0.0f }, { 0.0f, -borderThickness, 0.0f }, borderColor);
    // Left:   fixed width, full height, pinned to left.
    addStrip({ 0.0f, 1.0f }, { borderThickness, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, borderColor);
    // Right:  fixed width, full height, pinned to right (relOffset x=1, pull back left by thickness).
    addStrip({ 0.0f, 1.0f }, { borderThickness, 0.0f }, { 1.0f, 0.0f, 0.0f }, { -borderThickness, 0.0f, 0.0f }, borderColor);

    container.Children().InsertAtTop(adorner);

    m_visualAdorner = adorner;
    m_visualAdornerParent = container;

    TapLog(L"HighlightVisual: adorned live visual with Comment '" + target + L"'");
}

void XamlProfilerTap::ClearHighlight()
{
    if (m_highlightContainer)
    {
        m_highlightContainer.Children().RemoveAll();
    }
    if (m_visualAdorner && m_visualAdornerParent)
    {
        try
        {
            m_visualAdornerParent.Children().Remove(m_visualAdorner);
        }
        catch (...)
        {
        }
    }
    m_visualAdorner = nullptr;
    m_visualAdornerParent = nullptr;
}

// ---- Pick mode (app -> profiler hover/click selection) ----------------------

// Record a window's XamlRoot if we haven't seen it. Distinctness is by COM identity of the
// projected object (get_abi of the default interface is stable for a given XamlRoot).
void XamlProfilerTap::RememberXamlRoot(winrt::Microsoft::UI::Xaml::XamlRoot const& root)
{
    if (!root)
        return;

    void* id = winrt::get_abi(root);
    for (auto const& r : m_xamlRoots)
    {
        if (winrt::get_abi(r) == id)
            return; // already known
    }

    m_xamlRoots.push_back(root);

    wchar_t dbg[96];
    swprintf_s(dbg, L"RememberXamlRoot: now tracking %zu window(s)", m_xamlRoots.size());
    TapLog(dbg);
}

// Turn on pick mode: cover EVERY live window with a full-window transparent overlay (a
// Popup) and subscribe PointerMoved/PointerPressed/PointerWheelChanged on each. Because an
// overlay is the top-most hit-test target for its window it intercepts ALL pointer input
// there, so the app's controls never act while picking. Each overlay carries its own
// XamlRoot so its handlers hit-test/convert coordinates against the right window. Torn down
// by StopPick. (No WH_MOUSE_LL hook -- denied inside a packaged app.)
void XamlProfilerTap::StartPick()
{
    if (m_picking)
        return;

    // Cover every window we know about.
    std::vector<winrt::Microsoft::UI::Xaml::XamlRoot> roots = m_xamlRoots;
    if (roots.empty())
    {
        TapLog(L"StartPick: no XamlRoot tracked yet -> ignoring");
        return;
    }

    m_lastHitHandle = 0;
    m_wheelAccum = 0;
    m_pickCandidates.clear();
    m_pickIndex = 0;

    namespace MUX = winrt::Microsoft::UI::Xaml;
    namespace MUXC = winrt::Microsoft::UI::Xaml::Controls;
    namespace MUXCP = winrt::Microsoft::UI::Xaml::Controls::Primitives;
    namespace MUXH = winrt::Microsoft::UI::Xaml::Hosting;

    for (auto const& root : roots)
    {
        if (!root)
            continue;
        auto content = root.Content();
        if (!content)
            continue;

        auto po = std::make_shared<PickOverlay>();
        po->root = root;

        // Transparent capture surface sized to this window's island. A SolidColorBrush
        // (even alpha 0) makes the Border hit-testable; a null Background would NOT be.
        auto size = root.Size();
        MUXC::Border surface;
        surface.Name(L"__xp_pick_overlay");
        surface.Background(MUX::Media::SolidColorBrush(winrt::Windows::UI::Color{ 0, 0, 0, 0 }));
        surface.Width(size.Width);
        surface.Height(size.Height);

        // Capture a weak handle to this overlay so each pointer handler knows which window
        // it fired for (and thus which content to hit-test / which surface for coordinates).
        std::weak_ptr<PickOverlay> wpo = po;
        po->movedToken = surface.PointerMoved(
            [weak = get_weak(), wpo](winrt::Windows::Foundation::IInspectable const&,
                                     MUX::Input::PointerRoutedEventArgs const& e)
            {
                if (auto self = weak.get())
                    if (auto ctx = wpo.lock())
                        self->OnPickPointerMoved(*ctx, e);
            });
        po->pressedToken = surface.PointerPressed(
            [weak = get_weak(), wpo](winrt::Windows::Foundation::IInspectable const&,
                                     MUX::Input::PointerRoutedEventArgs const& e)
            {
                if (auto self = weak.get())
                    if (auto ctx = wpo.lock())
                        self->OnPickPointerPressed(*ctx, e);
            });
        po->wheelToken = surface.PointerWheelChanged(
            [weak = get_weak(), wpo](winrt::Windows::Foundation::IInspectable const&,
                                     MUX::Input::PointerRoutedEventArgs const& e)
            {
                if (auto self = weak.get())
                    if (auto ctx = wpo.lock())
                        self->OnPickPointerWheel(*ctx, e);
            });

        // Host the surface in a Popup so it renders above the app content regardless of the
        // root panel type, without disturbing the app's layout.
        MUXCP::Popup overlay;
        overlay.Name(L"__xp_pick_overlay");
        overlay.XamlRoot(root);
        overlay.Child(surface);
        overlay.IsOpen(true);

        po->surface = surface;
        po->overlay = overlay;

        // Stamp the capture surface's element (hand-in) visual with the adorner marker so the
        // producer's IsProfilerAdorner skip (in NotifyChildInserted) suppresses it from the
        // profiler's IVisual tree. This is an in-island Popup, so its spine flows through
        // NotifyChildInserted (not NotifyRootSet) — the same path that honors the marker.
        try
        {
            auto surfaceVisual = MUXH::ElementCompositionPreview::GetElementVisual(surface);
            if (surfaceVisual)
                surfaceVisual.Comment(L"__xp_adorner");
        }
        catch (...) {}

        m_pickOverlays.push_back(po);
    }

    if (m_pickOverlays.empty())
    {
        TapLog(L"StartPick: no window had content -> ignoring");
        return;
    }

    m_picking = true;
    wchar_t dbg[96];
    swprintf_s(dbg, L"StartPick: pick mode ON (%zu window overlay(s) capturing input)", m_pickOverlays.size());
    TapLog(dbg);
}

// Turn off pick mode: close/detach the overlay and clear any preview highlight.
void XamlProfilerTap::StopPick()
{
    // Detach handlers and close every per-window overlay.
    for (auto const& po : m_pickOverlays)
    {
        if (!po)
            continue;
        if (po->surface)
        {
            if (po->movedToken)
                po->surface.PointerMoved(po->movedToken);
            if (po->pressedToken)
                po->surface.PointerPressed(po->pressedToken);
            if (po->wheelToken)
                po->surface.PointerWheelChanged(po->wheelToken);
        }
        if (po->overlay)
        {
            po->overlay.IsOpen(false);
            po->overlay.Child(nullptr);
        }
    }
    m_pickOverlays.clear();
    m_pickCandidates.clear();
    m_pickIndex = 0;
    m_lastHitHandle = 0;
    m_wheelAccum = 0;

    bool wasPicking = m_picking;
    m_picking = false;
    if (wasPicking)
    {
        ClearHighlight();
        TapLog(L"StopPick: pick mode OFF");
    }
}

// Called (on the UI thread) when the user clicks during pick mode: report the
// selected element's handle up the pipe so the profiler can select its node, then
// leave pick mode.
// Send a free-text info line to the profiler so it shows in the profiler's log.
void XamlProfilerTap::SendTapInfo(std::wstring const& text)
{
    TapLog(text);
    winrt::Windows::Data::Json::JsonObject obj = winrt::Windows::Data::Json::JsonObject();
    obj.SetNamedValue(L"TapType", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"Info"));
    obj.SetNamedValue(L"Text", winrt::Windows::Data::Json::JsonValue::CreateStringValue(winrt::hstring(text)));
    WriteToPipe(obj);
}

void XamlProfilerTap::CommitPick(InstanceHandle handle)
{
    if (handle)
    {
        // Report the composition visual responsible for the clicked element (its xpid
        // comment) to the profiler log, so the user can see the specific sprite/container.
        LogResponsibleVisual(handle);

        winrt::Windows::Data::Json::JsonObject obj = winrt::Windows::Data::Json::JsonObject();
        obj.SetNamedValue(L"TapType", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"Select"));
        SetNamedPointerValue(obj, L"Handle", handle);

        // Also resolve the clicked element's composition visual id (its GetElementVisual:
        // the xpid Comment if stamped, else the raw IVisual* pointer). The profiler uses
        // it to find that visual node and glow its whole subtree in the IVisual/Comp tree.
        uint64_t visualId = ResolveElementVisualId(handle);
        if (visualId)
            SetNamedPointerValue(obj, L"VisualId", static_cast<InstanceHandle>(visualId));

        WriteToPipe(obj);

        wchar_t dbg[96];
        swprintf_s(dbg, L"CommitPick: sent Select handle = 0x%p visualId = 0x%llx", (void*)handle, visualId);
        TapLog(dbg);
    }
    StopPick();
}

// Resolve the clicked element's composition visual to a profiler-recognizable id: the
// element's own (hand-in) visual from GetElementVisual. If that visual's Comment carries
// the producer's "xpid:<hex>" stamp we return that hex (matches the profiler's WucVisual
// node Id); otherwise we return the raw IVisual* pointer (get_abi) as the id.
uint64_t XamlProfilerTap::ResolveElementVisualId(InstanceHandle handle)
{
    namespace WUC = winrt::Microsoft::UI::Composition;
    namespace MUXH = winrt::Microsoft::UI::Xaml::Hosting;

    if (!handle)
        return 0;

    winrt::Windows::Foundation::IInspectable inspectable;
    if (FAILED(m_xamlDiagnostics->GetIInspectableFromHandle(handle, (::IInspectable**)winrt::put_abi(inspectable))) || !inspectable)
        return 0;

    auto uie = inspectable.try_as<winrt::Microsoft::UI::Xaml::UIElement>();
    if (!uie)
        return 0;

    auto v = MUXH::ElementCompositionPreview::GetElementVisual(uie);

    auto comment = v.Comment();
    if (comment.size() >= 5 && std::wstring_view(comment).substr(0, 5) == L"xpid:")
    {
        uint64_t id = 0;
        if (swscanf_s(comment.c_str() + 5, L"%llx", &id) == 1 && id)
            return id;
    }

    return reinterpret_cast<uint64_t>(winrt::get_abi(v));
}

// Resolve the clicked element to the composition visual responsible for painting it and
// report that visual's identity + xpid Comment back to the profiler. We start from the
// element's own (hand-in) visual; that container usually has no xpid stamp, so we descend
// to the nearest descendant that carries an "xpid:" Comment -- that's the sprite/container
// the producer recognizes as this element's visual. The id is get_abi(v) == the producer's
// xpid == the node Id shown in the profiler.
void XamlProfilerTap::LogResponsibleVisual(InstanceHandle handle)
{
    namespace WUC = winrt::Microsoft::UI::Composition;
    namespace MUXH = winrt::Microsoft::UI::Xaml::Hosting;

    if (!handle)
        return;

    winrt::Windows::Foundation::IInspectable inspectable;
    if (FAILED(m_xamlDiagnostics->GetIInspectableFromHandle(handle, (::IInspectable**)winrt::put_abi(inspectable))) || !inspectable)
    {
        SendTapInfo(L"ResponsibleVisual: could not resolve clicked handle to a live object");
        return;
    }

    auto uie = inspectable.try_as<winrt::Microsoft::UI::Xaml::UIElement>();
    if (!uie)
    {
        SendTapInfo(L"ResponsibleVisual: clicked object is not a UIElement");
        return;
    }

    auto elementVisual = MUXH::ElementCompositionPreview::GetElementVisual(uie);

    // Walk the element's visual subtree for the first visual carrying an "xpid:" Comment.
    WUC::Visual responsible{ nullptr };
    std::wstring responsibleType;
    std::function<bool(WUC::Visual const&)> findStamped = [&](WUC::Visual const& v) -> bool
    {
        auto comment = v.Comment();
        if (comment.size() >= 5 && std::wstring_view(comment).substr(0, 5) == L"xpid:")
        {
            responsible = v;
            if (v.try_as<WUC::SpriteVisual>())          responsibleType = L"SpriteVisual";
            else if (v.try_as<WUC::ContainerVisual>())  responsibleType = L"ContainerVisual";
            else                                        responsibleType = L"Visual";
            return true;
        }
        if (auto c = v.try_as<WUC::ContainerVisual>())
        {
            for (auto const& child : c.Children())
            {
                if (findStamped(child))
                    return true;
            }
        }
        return false;
    };
    findStamped(elementVisual);

    uint64_t elementVisualId = reinterpret_cast<uint64_t>(winrt::get_abi(elementVisual));

    if (responsible)
    {
        uint64_t id = reinterpret_cast<uint64_t>(winrt::get_abi(responsible));
        auto size = responsible.Size();
        wchar_t msg[256];
        swprintf_s(msg,
            L"ResponsibleVisual: %s id=0x%llx comment='%s' size=(%.0f,%.0f)  [elementVisual=0x%llx]",
            responsibleType.c_str(), id, responsible.Comment().c_str(),
            size.x, size.y, elementVisualId);
        SendTapInfo(msg);
    }
    else
    {
        // No xpid-stamped descendant: report the element's own (hand-in) visual instead.
        std::wstring kind = L"Visual";
        if (elementVisual.try_as<WUC::SpriteVisual>())          kind = L"SpriteVisual";
        else if (elementVisual.try_as<WUC::ContainerVisual>())  kind = L"ContainerVisual";
        wchar_t msg[256];
        swprintf_s(msg,
            L"ResponsibleVisual: no xpid-stamped visual found; element hand-in visual is %s id=0x%llx comment='%s'",
            kind.c_str(), elementVisualId, elementVisual.Comment().c_str());
        SendTapInfo(msg);
    }
}

// Build the stack of overlapping elements at a point (in XamlRoot/host coordinates),
// ordered DEEPEST-CHILD FIRST. We search the app content subtree (via
// FindElementsInHostCoordinates) which structurally excludes our capture overlay (it lives
// in the popup root, not under content). Each element is mapped to a diagnostics handle;
// unmapped ones are skipped. We sort by visual-tree depth (descending) so index 0 is always
// the lowermost/most-specific element under the cursor and higher indices walk toward the
// outermost ancestor — independent of the platform's FindElementsInHostCoordinates ordering.
std::vector<InstanceHandle> XamlProfilerTap::HitTestStack(winrt::Microsoft::UI::Xaml::UIElement const& content, winrt::Windows::Foundation::Point const& pt)
{
    std::vector<InstanceHandle> stack;
    if (!content)
        return stack;

    auto elements = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::FindElementsInHostCoordinates(
        pt, content, true /* includeAllElements */);

    // Collect (depth, handle) for every hit element that maps to a diagnostics handle.
    struct Candidate { int depth; InstanceHandle handle; };
    std::vector<Candidate> candidates;
    for (auto const& el : elements)
    {
        auto insp = el.as<winrt::Windows::Foundation::IInspectable>();
        InstanceHandle h = 0;
        if (!(SUCCEEDED(m_xamlDiagnostics->GetHandleFromIInspectable(
                reinterpret_cast<::IInspectable*>(winrt::get_abi(insp)), &h)) && h))
            continue;

        // Depth = number of hops from this element up to the content root.
        int depth = 0;
        winrt::Microsoft::UI::Xaml::DependencyObject node = el;
        while (node)
        {
            auto parent = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::GetParent(node);
            if (!parent)
                break;
            ++depth;
            node = parent;
        }
        candidates.push_back({ depth, h });
    }

    // Deepest first; std::stable_sort preserves the original (z-order) order among ties.
    std::stable_sort(candidates.begin(), candidates.end(),
        [](Candidate const& a, Candidate const& b) { return a.depth > b.depth; });

    stack.reserve(candidates.size());
    for (auto const& c : candidates)
        stack.push_back(c.handle);
    return stack;
}

// Hover preview: rebuild the overlapping-element stack under the cursor and highlight the
// deepest/lowermost child (index 0). Moving the mouse resets the wheel drill-up to that
// deepest element.
void XamlProfilerTap::OnPickPointerMoved(PickOverlay& po, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
{
    if (!m_picking || !po.surface || !po.root)
        return;

    auto content = po.root.Content();
    if (!content)
        return;

    // The capture surface fills this window's island at the origin, so its local coords ==
    // the window's XamlRoot coords. Hit-test against THIS window's content subtree.
    m_pickCandidates = HitTestStack(content, e.GetCurrentPoint(po.surface).Position());
    m_pickIndex = 0;
    m_wheelAccum = 0;   // fresh stack -> drop any leftover sub-notch scroll

    if (!m_pickCandidates.empty())
    {
        InstanceHandle top = m_pickCandidates.front();
        if (top != m_lastHitHandle)
        {
            m_lastHitHandle = top;
            HighlightElement(top);
        }
    }
    else if (m_lastHitHandle != 0)
    {
        m_lastHitHandle = 0;
        ClearHighlight();
    }
}

// Mouse wheel drills through overlapping elements at the cursor (mouse held still). The
// candidate stack is ordered deepest-child first (index 0), so wheel up walks toward the
// uppermost/ancestor element (higher index) and wheel down walks back toward the deepest
// child. The highlighted candidate is what a click commits. Handled so the app underneath
// never scrolls.
void XamlProfilerTap::OnPickPointerWheel(PickOverlay& po, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
{
    if (!m_picking || !po.surface)
        return;
    e.Handled(true);
    if (m_pickCandidates.empty())
        return;

    // Discretize by physical wheel notches instead of stepping on every raw wheel event.
    // High-resolution mice / trackpads fire many sub-notch events per spin, and stepping
    // on each (by sign alone) drilled through the stack far too fast to aim. Accumulate
    // the signed delta and move one element per WHEEL_DELTA (one notch), keeping the
    // leftover remainder so partial scrolls add up smoothly. Tune kUnitsPerStep to change
    // sensitivity: larger == slower (more scroll per change), smaller == faster.
    constexpr int kUnitsPerStep = 120; // WHEEL_DELTA == one wheel notch
    m_wheelAccum += e.GetCurrentPoint(po.surface).Properties().MouseWheelDelta();

    int steps = m_wheelAccum / kUnitsPerStep;
    if (steps == 0)
        return;                              // less than a full notch so far -> keep accumulating
    m_wheelAccum -= steps * kUnitsPerStep;   // retain the sub-notch remainder

    // steps > 0 -> wheel up -> toward uppermost/ancestor (higher index in the deepest-first
    // list); steps < 0 -> toward the deepest child (lower index). Clamp to [0, size-1].
    if (steps > 0)
    {
        m_pickIndex = (std::min)(m_pickIndex + static_cast<size_t>(steps),
                                 m_pickCandidates.size() - 1);
    }
    else
    {
        size_t dec = static_cast<size_t>(-steps);
        m_pickIndex = (dec >= m_pickIndex) ? 0 : m_pickIndex - dec;
    }

    InstanceHandle h = m_pickCandidates[m_pickIndex];
    m_lastHitHandle = h;
    HighlightElement(h);

    wchar_t dbg[96];
    swprintf_s(dbg, L"Pick: drill %zu/%zu -> 0x%p", m_pickIndex + 1, m_pickCandidates.size(), (void*)h);
    TapLog(dbg);
}

// Click on the capture overlay: commit the currently drilled-to candidate as the selection.
// The overlay already absorbed the click (it's the hit-test target), so the app never sees
// it. Runs on the UI thread.
void XamlProfilerTap::OnPickPointerPressed(PickOverlay& po, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
{
    if (!m_picking || !po.surface)
        return;

    e.Handled(true);

    InstanceHandle picked = 0;
    if (m_pickIndex < m_pickCandidates.size())
        picked = m_pickCandidates[m_pickIndex];
    if (!picked && po.root)
    {
        // Fall back to a fresh hit-test of the press point (e.g. clicked without moving).
        if (auto content = po.root.Content())
        {
            auto stack = HitTestStack(content, e.GetCurrentPoint(po.surface).Position());
            if (!stack.empty())
                picked = stack.front();
        }
    }
    if (!picked)
        picked = m_lastHitHandle;

    // Defer the commit: CommitPick calls StopPick, which closes this overlay.
    m_dispatcherQueue.TryEnqueue(
        winrt::Microsoft::UI::Dispatching::DispatcherQueueHandler(
            [weak = get_weak(), picked]()
            {
                if (auto self = weak.get())
                    self->CommitPick(picked);
            }));
}

HRESULT XamlProfilerTap::OnVisualTreeChange(
    ParentChildRelation relation,
    VisualElement element,
    VisualMutationType mutationType)
{
    if (m_shutdownCalled)
        return S_OK;

    // Note: element might not be a DependencyObject if it is a Composition Visual.

#if false
    wchar_t buf[256] = {};
    void* parentPtr = (void*)relation.Parent;
    swprintf_s(buf, L"TreeChange: Parent=%p Element: %p (%s)", parentPtr, element.Handle, element.Type);
    OutputDebugString(buf);
#endif

    assert(relation.Child == element.Handle || relation.Child == 0);

    // Cache XamlRoots so pick/highlight/dump can reach each window's live tree. A new
    // window's elements arrive here as Adds; we resolve the owning XamlRoot from any element
    // that already has a live peer and remember it (RememberXamlRoot dedups by COM identity).
    //
    // We deliberately try on EVERY Add rather than only parentless ones: a window's root
    // content frequently has a lazy peer that GetIInspectableFromHandle can't resolve at the
    // instant its (parentless) Add fires, so keying off the root alone would miss whole
    // windows. Some child of that window will have a resolvable peer and the same XamlRoot,
    // so scanning all Adds reliably catches every window. The lookup is cheap relative to the
    // JSON + pipe write this method already does per element.
    if (element.Handle && mutationType == VisualMutationType::Add)
    {
        winrt::Windows::Foundation::IInspectable inspectable;
        if (SUCCEEDED(m_xamlDiagnostics->GetIInspectableFromHandle(element.Handle, (::IInspectable**)winrt::put_abi(inspectable))) && inspectable)
        {
            if (auto uie = inspectable.try_as<winrt::Microsoft::UI::Xaml::UIElement>())
            {
                if (auto root = uie.XamlRoot())
                {
                    RememberXamlRoot(root);
                }
            }
        }
    }

    winrt::Windows::Data::Json::JsonObject obj = winrt::Windows::Data::Json::JsonObject();
    obj.SetNamedValue(L"TapType", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"VisualTreeChange"));
    SetNamedPointerValue(obj, L"Parent", relation.Parent);
    SetNamedPointerValue(obj, L"Element", element.Handle);
    obj.SetNamedValue(L"ChildIndex", winrt::Windows::Data::Json::JsonValue::CreateNumberValue(relation.ChildIndex));
    if (element.Type != nullptr)
        obj.SetNamedValue(L"ElementType", winrt::Windows::Data::Json::JsonValue::CreateStringValue(element.Type));
    if (element.Name != nullptr)
        obj.SetNamedValue(L"ElementName", winrt::Windows::Data::Json::JsonValue::CreateStringValue(element.Name));
    obj.SetNamedValue(L"MutationType", winrt::Windows::Data::Json::JsonValue::CreateStringValue(mutationType == VisualMutationType::Add ? L"Add" : L"Remove"));
    WriteToPipe(obj);

    return S_OK;
}

HRESULT XamlProfilerTap::OnElementStateChanged(
    InstanceHandle /*element*/,
    VisualElementState /*elementState*/,
    LPCWSTR /*context*/)
{
    //
    OutputDebugString(L"element change\n");
    return S_OK;
}


struct XamlSnoopLauncherFactory : winrt::implements<XamlSnoopLauncherFactory, IClassFactory>
{
public:
    XamlSnoopLauncherFactory() {}

    // IClassFactory methods
    STDMETHOD(CreateInstance)(_Inout_opt_ IUnknown* pUnkOuter, REFIID riid, _Outptr_result_nullonfailure_ void** ppvObject);
    STDMETHOD(LockServer)(BOOL fLock);
};

STDMETHODIMP XamlSnoopLauncherFactory::CreateInstance(_Inout_opt_ IUnknown* /*pUnkOuter*/, REFIID riid, _Outptr_result_nullonfailure_ void** ppvObject)
{
    *ppvObject = NULL;
    if (InlineIsEqualGUID(riid, __uuidof(IObjectWithSite)))
    {
        auto tap = winrt::make<XamlProfilerTap>();
        *ppvObject = tap.detach();
        return S_OK;
    }
    return E_NOTIMPL;
}

STDMETHODIMP XamlSnoopLauncherFactory::LockServer(BOOL /*fLock*/)
{
    return S_OK;
}

_Check_return_ STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ void** ppv)
{
    if (rclsid != __uuidof(XamlProfilerTap))
        return E_NOTIMPL;
    if (riid != IID_IClassFactory)
        return E_NOTIMPL;

    auto classFactory = winrt::make<XamlSnoopLauncherFactory>();
    *ppv = classFactory.detach();
    return S_OK;
}

STDAPI DllCanUnloadNow()
{
    if (winrt::get_module_lock())
    {
        return S_FALSE;
    }
    return S_OK;
}
