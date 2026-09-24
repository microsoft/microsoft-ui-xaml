#include "pch.h"
#include <algorithm>
#include "Tap.h"
#include <unknwn.h>
#include <ocidl.h>
#include <string_view>
#include <wil/com.h>
#include <wil/resource.h>
#include <xamlom.h>
#include <thread>
#include <winrt/windows.data.json.h>
#include <winrt/windows.system.h>

using namespace std::string_view_literals;

HMODULE XamlSnoopTap::s_hModule = nullptr;

BOOL WINAPI DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        XamlSnoopTap::SetModuleHandle(hModule);
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

STDMETHODIMP XamlSnoopTap::SetSite(IUnknown* pUnkSite)
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

        // Register for tree change events. This must happen on a background thread,
        // since AdviseVisualTreeChange() calls Advising::RunOnUIThread() which waits
        // for that call to finish, and since SetSite() gets called on the UI thread,
        // making that call here or even waiting for the background thread will cause
        // a hang.
        auto f = [](XamlSnoopTap* tap) {
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
void XamlSnoopTap::ReaderThreadProc(wil::com_ptr<XamlSnoopTap> tap)
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
        OutputDebugString(L"read some data!:");
        OutputDebugString(command.data());
        OutputDebugString(L"\n");
        if (command._Starts_with(L"HACK-SUSPENDTREECHANGE"sv))
        {
            tap->m_suspendTreeChanges = true;
        }
        else if (command._Starts_with(L"GET-PROPERTIES"sv))
        {
            void* p = nullptr;
            swscanf_s(command.data(), L"GET-PROPERTIES: %p", &p);
            if (p)
            {
                InstanceHandle element = (InstanceHandle)p;

                auto callback = [element, tap]() {
                    tap->RetrieveElementProperties(element);
                };

                tap->m_dispatcherQueue.TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueueHandler(callback));
            }
        }
        else if (command._Starts_with(L"GET-SUBTREE-PROPERTIES"sv))
        {
            void* p = nullptr;
            swscanf_s(command.data(), L"GET-SUBTREE-PROPERTIES: %p", &p);
            if (p)
            {
                InstanceHandle element = (InstanceHandle)p;

                auto callback = [element, tap]() {
                    tap->RetrieveSubTreeProperties(element);
                };

                tap->m_dispatcherQueue.TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueueHandler(callback));
            }
        }
        else if (command._Starts_with(L"HIGHLIGHT"sv))
        {
            void* p = nullptr;
            swscanf_s(command.data(), L"HIGHLIGHT: %p", &p);
            InstanceHandle element = (InstanceHandle)p;

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
        else if (command._Starts_with(L"CLOSE"sv))
        {
            tap->Shutdown();
            break;
        }
    }
    OutputDebugString(L"reader thread exiting\n");
}

void XamlSnoopTap::Shutdown()
{
    if (m_shutdownCalled) return;
    m_shutdownCalled = true;

    OutputDebugString(L"XamlSnoopTap::Shutdown - begin\n");

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

    OutputDebugString(L"XamlSnoopTap::Shutdown - complete\n");

    // 5. Balance the AddRef() from SetSite on a separate thread.
    //    The actual DLL unload is done by the snoop app via CreateRemoteThread + FreeLibrary,
    //    since the DLL may have multiple LoadLibrary refs from XAML diagnostics/COM.
    XamlSnoopTap* rawThis = this;
    std::thread releaseThread([rawThis]() {
        // Balance the AddRef() from SetSite
        rawThis->Release();
        OutputDebugString(L"XamlSnoopTap::Shutdown - Release done\n");
    });
    releaseThread.detach();
}

void XamlSnoopTap::WriteToPipe(const winrt::Windows::Data::Json::JsonObject& obj)
{
    WriteLineToPipe(obj.Stringify());
}

void XamlSnoopTap::WriteLineToPipe(winrt::hstring string)
{
    if (m_shutdownCalled || !m_writePipe) return;
    LOG_IF_WIN32_BOOL_FALSE(WriteFile(m_writePipe.get(), string.c_str(), string.size() * 2, nullptr, nullptr));
    LOG_IF_WIN32_BOOL_FALSE(WriteFile(m_writePipe.get(), L"\n", 1 * 2, nullptr, nullptr));
}

STDMETHODIMP XamlSnoopTap::GetSite(REFIID /*riid*/, void** /*ppvSite*/)
{
    //return m_site == nullptr ? E_FAIL : m_site.Get()->QueryInterface(riid, ppvSite);
    return E_NOTIMPL;
}

const wchar_t* XamlSnoopTap::SourceToString(BaseValueSource source)
{
    switch (source)
    {
    case BaseValueSourceUnknown: return L"Unknown";
    case BaseValueSourceDefault: return L"Default";
    case BaseValueSourceBuiltInStyle: return L"BuiltInStyle";
    case BaseValueSourceStyle: return L"Style";
    case BaseValueSourceLocal: return L"Local";
    case Inherited: return L"Inherited";
    case DefaultStyleTrigger: return L"DefaultStyleTrigger";
    case TemplateTrigger: return L"TemplateTrigger";
    case StyleTrigger: return L"StyleTrigger";
    case ImplicitStyleReference: return L"ImplicitStyleReference";
    case ParentTemplate: return L"ParentTemplate";
    case ParentTemplateTrigger: return L"ParentTemplateTrigger";
    case Animation: return L"Animation";
    case Coercion: return L"Coercion";
    case BaseValueSourceVisualState: return L"VisualState";
    }
    return L"???";
}

winrt::hstring XamlSnoopTap::GetPropertyValueAsString(InstanceHandle element, unsigned int propertyIndex, BSTR propertyType)
{
    winrt::hstring strValue = L"object";

    wil::com_ptr<IVisualTreeService2> visualTreeService2;
    m_visualTreeService.query_to(&visualTreeService2);
    InstanceHandle directValue = {};
    visualTreeService2->GetProperty(element, propertyIndex, &directValue);
    if (directValue)
    {
        winrt::Windows::Foundation::IInspectable inspectable;
        if (SUCCEEDED(m_xamlDiagnostics->GetIInspectableFromHandle(directValue, (::IInspectable**)winrt::put_abi(inspectable))))
        {
            strValue = winrt::hstring();
            std::wstring_view propertyTypeSV = propertyType;
            if (propertyTypeSV == L"Windows.Foundation.Double"sv)
                strValue = winrt::to_hstring(winrt::unbox_value_or<double>(inspectable, -1));
            else if (propertyTypeSV == L"Windows.Foundation.String"sv)
                strValue = winrt::unbox_value_or<winrt::hstring>(inspectable, L"object of unexpected type");
            else if (propertyTypeSV == L"Microsoft.UI.Xaml.Media.Brush"sv)
            {
                auto brush = inspectable.try_as<winrt::Microsoft::UI::Xaml::Media::SolidColorBrush>();
                if (brush)
                {
                    auto color = brush.Color();
                    // TODO: Attempt to use ColorPicker's name?  Or attempt that lookup inside Snoop using the Colors enum?
                    wchar_t propbuf[256] = {};
                    swprintf_s(propbuf, L"#%02X%02X%02X%02X", color.A, color.R, color.G, color.B);
                    strValue = propbuf;
                }
            }

            if (strValue.size() == 0)
            {
                // TODO: More types!!
                wchar_t propbuf[256] = {};
                swprintf_s(propbuf, L"(%s*)0x%p", propertyType, winrt::get_abi(inspectable));
                strValue = propbuf;
            }
        }
    }
    return strValue;
}

void XamlSnoopTap::RetrieveElementProperties(InstanceHandle element)
{
    // This works to dump properties!
    unsigned int sourceCount = 0;
    unsigned int valueCount = 0;
    wil::unique_cotaskmem_ptr<PropertyChainSource> propertySources;
    wil::unique_cotaskmem_ptr<PropertyChainValue> propertyValues;
    FAIL_FAST_IF_FAILED(m_visualTreeService->GetPropertyValuesChain(element, &sourceCount, wil::out_param(propertySources), &valueCount, wil::out_param(propertyValues)));

    winrt::Windows::Data::Json::JsonObject jsonObjProps = winrt::Windows::Data::Json::JsonObject();
    jsonObjProps.SetNamedValue(L"TapType", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"ElementProperties"));
    SetNamedPointerValue(jsonObjProps, L"Element", element);

    winrt::Windows::Data::Json::JsonArray array = winrt::Windows::Data::Json::JsonArray();

    for (unsigned int p = 0; p < valueCount; p++)
    {
        PropertyChainValue& property = propertyValues.get()[p];

        if (!property.Overridden) // only include the actual in-use value
        {
            winrt::Windows::Data::Json::JsonObject jsonProp = winrt::Windows::Data::Json::JsonObject();
            jsonProp.SetNamedValue(L"Name", winrt::Windows::Data::Json::JsonValue::CreateStringValue(property.PropertyName));

            if (property.MetadataBits & MetadataBit::IsValueHandle)
            {
                winrt::hstring strValue = GetPropertyValueAsString(element, property.Index, property.Type);
                jsonProp.SetNamedValue(L"Value", winrt::Windows::Data::Json::JsonValue::CreateStringValue(strValue));
            }
            else
            {
                jsonProp.SetNamedValue(L"Value", winrt::Windows::Data::Json::JsonValue::CreateStringValue(property.Value));
            }

            if (property.MetadataBits & MetadataBit::IsValueBindingExpression)
            {
                // TODO:  add on binding info
            }

            PropertyChainSource& source = propertySources.get()[property.PropertyChainIndex];
            jsonProp.SetNamedValue(L"Source", winrt::Windows::Data::Json::JsonValue::CreateStringValue(SourceToString(source.Source)));
            array.Append(jsonProp);
        }
    }
    jsonObjProps.SetNamedValue(L"Properties", array);

    WriteToPipe(jsonObjProps);
}

void XamlSnoopTap::RetrieveSubTreeProperties(InstanceHandle rootElement)
{
    // Walk the visual tree starting from rootElement and retrieve properties
    // for every node, sending one ElementProperties message per node.
    RetrieveSubTreePropertiesRecursive(rootElement);

    // Send a completion marker so the snoop app knows all properties have been sent.
    winrt::Windows::Data::Json::JsonObject doneMsg = winrt::Windows::Data::Json::JsonObject();
    doneMsg.SetNamedValue(L"TapType", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"SubTreePropertiesDone"));
    SetNamedPointerValue(doneMsg, L"Element", rootElement);
    WriteToPipe(doneMsg);
}

void XamlSnoopTap::RetrieveSubTreePropertiesRecursive(InstanceHandle element)
{
    // Fetch properties for this element
    RetrieveElementProperties(element);

    // Get the IInspectable for this element, then walk its visual children
    winrt::Windows::Foundation::IInspectable inspectable;
    if (FAILED(m_xamlDiagnostics->GetIInspectableFromHandle(element, (::IInspectable**)winrt::put_abi(inspectable))) || !inspectable)
        return;

    auto depObj = inspectable.try_as<winrt::Microsoft::UI::Xaml::DependencyObject>();
    if (!depObj)
        return;

    int childCount = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::GetChildrenCount(depObj);
    for (int i = 0; i < childCount; i++)
    {
        auto child = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::GetChild(depObj, i);
        if (!child)
            continue;

        InstanceHandle childHandle = 0;
        auto childInsp = child.as<winrt::Windows::Foundation::IInspectable>();
        if (SUCCEEDED(m_xamlDiagnostics->GetHandleFromIInspectable(reinterpret_cast<::IInspectable*>(winrt::get_abi(childInsp)), &childHandle)) && childHandle)
        {
            RetrieveSubTreePropertiesRecursive(childHandle);
        }
    }
}

void XamlSnoopTap::SetNamedPointerValue(winrt::Windows::Data::Json::JsonObject& obj, winrt::hstring name, InstanceHandle ptr)
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
void XamlSnoopTap::HighlightElement(InstanceHandle element)
{
    if (!element)
    {
        ClearHighlight();
        return;
    }

    winrt::Windows::Foundation::IInspectable inspectable;
    if (FAILED(m_xamlDiagnostics->GetIInspectableFromHandle(element, (::IInspectable**)winrt::put_abi(inspectable))) || !inspectable)
        return;

    auto fe = inspectable.try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
    if (!fe)
        return;

    auto xamlRoot = fe.XamlRoot();
    if (!xamlRoot)
        return;

    auto host = xamlRoot.Content();
    if (!host)
        return;

    double width = fe.ActualWidth();
    double height = fe.ActualHeight();
    if (width <= 0 || height <= 0)
        return;

    winrt::Windows::Foundation::Point offset{ 0, 0 };
    try
    {
        auto transform = fe.TransformToVisual(host);
        offset = transform.TransformPoint({ 0, 0 });
    }
    catch (...)
    {
        return;
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
        MUXH::ElementCompositionPreview::SetElementChildVisual(host, m_highlightContainer);
        m_highlightHost = host;
    }

    auto compositor = m_highlightContainer.Compositor();
    m_highlightContainer.Children().RemoveAll();

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

    // DevTools-style palette.
    constexpr winrt::Windows::UI::Color marginColor  { 0x80, 0xF6, 0xB2, 0x6B }; // orange ~50%
    constexpr winrt::Windows::UI::Color paddingColor { 0x90, 0x93, 0xC4, 0x7D }; // green  ~56%
    constexpr winrt::Windows::UI::Color contentColor { 0x70, 0x6F, 0xA8, 0xDC }; // blue   ~44%
    constexpr winrt::Windows::UI::Color borderColor  { 0xFF, 0x33, 0x99, 0xFF }; // solid magenta outline

    // Margin ring (outside element bounds).
    addRing(ex - ml, ey - mt, ew + ml + mr, eh + mt + mb,
            ex, ey, ew, eh, marginColor);

    // Padding ring (inside element bounds).
    addRing(ex, ey, ew, eh,
            cx, cy, cw, ch, paddingColor);

    // Content area fill (inside padding).
    addFill(cx, cy, cw, ch, contentColor);

    // Crisp element-rect outline drawn on top.
    constexpr float borderThickness = 1.5f;
    addFill(ex,                       ey,                         ew,              borderThickness, borderColor); // top
    addFill(ex,                       ey + eh - borderThickness,  ew,              borderThickness, borderColor); // bottom
    addFill(ex,                       ey,                         borderThickness, eh,              borderColor); // left
    addFill(ex + ew - borderThickness, ey,                        borderThickness, eh,              borderColor); // right
}

void XamlSnoopTap::ClearHighlight()
{
    if (m_highlightContainer)
    {
        m_highlightContainer.Children().RemoveAll();
    }
}

HRESULT XamlSnoopTap::OnVisualTreeChange(
    ParentChildRelation relation,
    VisualElement element,
    VisualMutationType mutationType)
{
    if (m_suspendTreeChanges || m_shutdownCalled)
        return S_OK;

    // Note: element might not be a DependencyObject if it is a Composition Visual.

#if false
    wchar_t buf[256] = {};
    void* parentPtr = (void*)relation.Parent;
    swprintf_s(buf, L"TreeChange: Parent=%p Element: %p (%s)", parentPtr, element.Handle, element.Type);
    OutputDebugString(buf);
#endif

    assert(relation.Child == element.Handle || relation.Child == 0);

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

HRESULT XamlSnoopTap::OnElementStateChanged(
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
        auto tap = winrt::make<XamlSnoopTap>();
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
    if (rclsid != __uuidof(XamlSnoopTap))
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
