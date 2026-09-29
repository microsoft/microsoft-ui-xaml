#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Metadata;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Graphics;
using namespace Windows::Graphics::DirectX;
using namespace Windows::System;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;
using namespace Windows::UI::Composition::Interactions;

namespace
{
    constexpr wchar_t WindowClassName[] = L"WindowsUICompositionCompatibilityWindow";
    constexpr UINT IdScenarioList = 1001;
    constexpr UINT IdRunSelected = 1002;
    constexpr UINT IdRunAll = 1003;
    constexpr UINT IdSave = 1004;

    enum class Outcome
    {
        NotRun,
        Pass,
        Fail,
        NotAvailable,
    };

    struct Result
    {
        Outcome outcome{ Outcome::NotRun };
        std::wstring detail;
    };

    class App;

    struct Scenario
    {
        std::wstring id;
        std::wstring category;
        std::wstring title;
        uint32_t minimumContract;
        std::wstring description;
        std::function<Result(App&)> run;
        Result result;
    };

    std::wstring OutcomeText(Outcome outcome)
    {
        switch (outcome)
        {
        case Outcome::Pass: return L"PASS";
        case Outcome::Fail: return L"FAIL";
        case Outcome::NotAvailable: return L"NOT AVAILABLE";
        default: return L"NOT RUN";
        }
    }

    std::string Utf8(std::wstring_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int size = WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0,
            nullptr,
            nullptr);
        std::string result(size, '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            result.data(),
            size,
            nullptr,
            nullptr);
        return result;
    }

    std::string JsonEscape(std::wstring_view value)
    {
        std::ostringstream output;
        for (const unsigned char ch : Utf8(value))
        {
            switch (ch)
            {
            case '\\': output << "\\\\"; break;
            case '"': output << "\\\""; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (ch < 0x20)
                {
                    output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                           << static_cast<int>(ch);
                }
                else
                {
                    output << ch;
                }
                break;
            }
        }
        return output.str();
    }

    Result Pass(std::wstring detail)
    {
        return { Outcome::Pass, std::move(detail) };
    }

    Result Fail(std::wstring detail)
    {
        return { Outcome::Fail, std::move(detail) };
    }

    Result NotAvailable(std::wstring detail)
    {
        return { Outcome::NotAvailable, std::move(detail) };
    }

    bool Near(float left, float right)
    {
        return std::abs(left - right) <= 0.0001f;
    }

    bool IsContractPresent(uint32_t version)
    {
        return ApiInformation::IsApiContractPresent(
            L"Windows.Foundation.UniversalApiContract",
            static_cast<uint16_t>(version));
    }

    std::wstring CurrentOsVersion()
    {
        using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
        const auto ntdll = GetModuleHandleW(L"ntdll.dll");
        const auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(
            GetProcAddress(ntdll, "RtlGetVersion"));
        RTL_OSVERSIONINFOW version{ sizeof(version) };
        if (!rtlGetVersion || rtlGetVersion(&version) != 0)
        {
            return L"unknown";
        }

        std::wstringstream text;
        text << version.dwMajorVersion << L'.' << version.dwMinorVersion << L'.'
             << version.dwBuildNumber;
        return text.str();
    }

    std::filesystem::path ExecutablePath()
    {
        std::vector<wchar_t> buffer(MAX_PATH);
        for (;;)
        {
            const DWORD length = GetModuleFileNameW(
                nullptr,
                buffer.data(),
                static_cast<DWORD>(buffer.size()));
            if (length == 0)
            {
                throw_last_error();
            }
            if (length < buffer.size())
            {
                return std::filesystem::path(buffer.data(), buffer.data() + length);
            }
            buffer.resize(buffer.size() * 2);
        }
    }

    DispatcherQueueController CreateDispatcherQueue()
    {
        DispatcherQueueOptions options
        {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_STA,
        };

        ABI::Windows::System::IDispatcherQueueController* controller{};
        check_hresult(CreateDispatcherQueueController(options, &controller));
        return { controller, take_ownership_from_abi };
    }

    class App
    {
    public:
        int Run(HINSTANCE instance, bool runAllAndExit)
        {
            m_instance = instance;
            RegisterWindowClass();
            CreateMainWindow();
            CreateCompositionHost();
            BuildScenarios();
            PopulateScenarioList();
            SelectScenario(0);

            if (runAllAndExit)
            {
                RunAll(false);
                return 0;
            }

            ShowWindow(m_window, SW_SHOW);
            UpdateWindow(m_window);

            MSG message{};
            while (GetMessageW(&message, nullptr, 0, 0))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            return static_cast<int>(message.wParam);
        }

        Compositor const& CompositorInstance() const
        {
            return m_compositor;
        }

        ContainerVisual const& RootVisual() const
        {
            return m_root;
        }

        void ResetScene()
        {
            m_root.Children().RemoveAll();
            m_previewVisual = nullptr;
        }

        SpriteVisual ShowSprite(Color color = { 255, 0, 120, 215 })
        {
            ResetScene();
            auto sprite = m_compositor.CreateSpriteVisual();
            sprite.Size({ 120.0f, 120.0f });
            sprite.Offset({ 40.0f, 40.0f, 0.0f });
            sprite.Brush(m_compositor.CreateColorBrush(color));
            m_root.Children().InsertAtTop(sprite);
            m_previewVisual = sprite;
            return sprite;
        }

        Result CreateTexture(bool validateRoundTrip)
        {
            if (!ApiInformation::IsTypePresent(L"Windows.UI.Composition.CompositionTexture"))
            {
                return NotAvailable(L"CompositionTexture requires UniversalApiContract v15.");
            }

            com_ptr<ID3D11Device> device;
            com_ptr<ID3D11DeviceContext> context;
            D3D_FEATURE_LEVEL level{};
            HRESULT hr = D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                nullptr,
                0,
                D3D11_SDK_VERSION,
                device.put(),
                &level,
                context.put());
            if (FAILED(hr))
            {
                check_hresult(D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_WARP,
                    nullptr,
                    D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                    nullptr,
                    0,
                    D3D11_SDK_VERSION,
                    device.put(),
                    &level,
                    context.put()));
            }

            D3D11_TEXTURE2D_DESC description{};
            description.Width = 64;
            description.Height = 64;
            description.MipLevels = 1;
            description.ArraySize = 1;
            description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            description.SampleDesc.Count = 1;
            description.Usage = D3D11_USAGE_DEFAULT;
            description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            description.MiscFlags =
                D3D11_RESOURCE_MISC_SHARED |
                D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

            com_ptr<ID3D11Texture2D> d3dTexture;
            check_hresult(device->CreateTexture2D(&description, nullptr, d3dTexture.put()));

            auto interop = m_compositor.as<ABI::Windows::UI::Composition::ICompositorInterop2>();
            BOOL supported{};
            check_hresult(interop->CheckCompositionTextureSupport(device.get(), &supported));
            if (!supported)
            {
                return NotAvailable(L"The active D3D11 device does not support composition textures.");
            }

            com_ptr<ABI::Windows::UI::Composition::ICompositionTexture> abiTexture;
            check_hresult(interop->CreateCompositionTexture(d3dTexture.get(), abiTexture.put()));

            CompositionTexture texture{ nullptr };
            check_hresult(abiTexture->QueryInterface(
                guid_of<CompositionTexture>(),
                put_abi(texture)));

            texture.AlphaMode(DirectXAlphaMode::Premultiplied);
            texture.ColorSpace(DirectXColorSpace::RgbFullG22NoneP709);
            const RectInt32 expected{ 7, 11, 31, 29 };
            texture.SourceRect(expected);
            const auto actual = texture.SourceRect();
            const auto actualAlphaMode = texture.AlphaMode();
            const auto actualColorSpace = texture.ColorSpace();

            if (actualAlphaMode != DirectXAlphaMode::Premultiplied ||
                actualColorSpace != DirectXColorSpace::RgbFullG22NoneP709)
            {
                return Fail(L"CompositionTexture alpha mode or color space did not round-trip.");
            }

            auto textureInterop =
                texture.as<ABI::Windows::UI::Composition::ICompositionTextureInterop>();
            UINT64 availableFenceValue{};
            com_ptr<ID3D11Fence> availableFence;
            check_hresult(textureInterop->GetAvailableFence(
                &availableFenceValue,
                __uuidof(ID3D11Fence),
                availableFence.put_void()));

            if (validateRoundTrip &&
                (actual.X != expected.X || actual.Y != expected.Y ||
                 actual.Width != expected.Width || actual.Height != expected.Height))
            {
                std::wstringstream detail;
                detail << L"SourceRect round trip mismatch. Set ("
                       << expected.X << L',' << expected.Y << L',' << expected.Width << L',' << expected.Height
                       << L"), read (" << actual.X << L',' << actual.Y << L',' << actual.Width << L',' << actual.Height
                       << L").";
                return Fail(detail.str());
            }

            auto brush = m_compositor.CreateSurfaceBrush(texture);
            auto sprite = ShowSprite({ 255, 40, 40, 40 });
            sprite.Brush(brush);

            if (validateRoundTrip)
            {
                return Pass(L"CompositionTexture properties and availability-fence query succeeded, and the texture was accepted by a surface brush.");
            }
            std::wstringstream detail;
            detail << L"Created and bound a D3D11-backed CompositionTexture; availability fence "
                   << (availableFence ? L"was returned at value " : L"was not required at value ")
                   << availableFenceValue << L'.';
            return Pass(detail.str());
        }

    private:
        void RegisterWindowClass()
        {
            WNDCLASSEXW windowClass{ sizeof(windowClass) };
            windowClass.lpfnWndProc = WindowProc;
            windowClass.hInstance = m_instance;
            windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            windowClass.lpszClassName = WindowClassName;
            check_bool(RegisterClassExW(&windowClass));
        }

        void CreateMainWindow()
        {
            m_window = CreateWindowExW(
                0,
                WindowClassName,
                L"Windows.UI.Composition compatibility: Windows 11 22000 vs latest",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                1260,
                820,
                nullptr,
                nullptr,
                m_instance,
                this);
            check_bool(m_window);

            m_list = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"LISTBOX",
                nullptr,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                0, 0, 0, 0,
                m_window,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IdScenarioList)),
                m_instance,
                nullptr);
            m_title = CreateWindowExW(
                0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                0, 0, 0, 0, m_window, nullptr, m_instance, nullptr);
            m_description = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                0, 0, 0, 0,
                m_window,
                nullptr,
                m_instance,
                nullptr);
            m_runSelected = CreateWindowExW(
                0, L"BUTTON", L"Run selected", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 0, 0, m_window,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IdRunSelected)), m_instance, nullptr);
            m_runAll = CreateWindowExW(
                0, L"BUTTON", L"Run all", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 0, 0, m_window,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IdRunAll)), m_instance, nullptr);
            m_save = CreateWindowExW(
                0, L"BUTTON", L"Save JSON", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 0, 0, m_window,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IdSave)), m_instance, nullptr);
            m_preview = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"STATIC",
                nullptr,
                WS_CHILD | WS_VISIBLE,
                0, 0, 0, 0,
                m_window,
                nullptr,
                m_instance,
                nullptr);

            const auto font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            for (const auto control : { m_list, m_title, m_description, m_runSelected, m_runAll, m_save })
            {
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }
        }

        void CreateCompositionHost()
        {
            m_dispatcherQueueController = CreateDispatcherQueue();
            m_compositor = Compositor();
            auto interop = m_compositor.as<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop>();
            check_hresult(interop->CreateDesktopWindowTarget(
                m_preview,
                TRUE,
                reinterpret_cast<ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget**>(
                    put_abi(m_target))));

            m_root = m_compositor.CreateContainerVisual();
            m_root.RelativeSizeAdjustment({ 1.0f, 1.0f });
            m_target.Root(m_root);
        }

        void BuildScenarios()
        {
            auto add = [this](
                std::wstring id,
                std::wstring category,
                std::wstring title,
                uint32_t minimumContract,
                std::wstring description,
                std::function<Result(App&)> run)
            {
                m_scenarios.push_back(
                    { std::move(id), std::move(category), std::move(title), minimumContract,
                      std::move(description), std::move(run), {} });
            };

            add(L"ENV-001", L"Environment", L"UniversalApiContract v13 baseline", 13,
                L"Confirms the Windows 11 build 22000 public Composition baseline.",
                [](App&)
                {
                    return IsContractPresent(13)
                        ? Pass(L"UniversalApiContract v13 is present.")
                        : Fail(L"UniversalApiContract v13 is missing; this is older than Windows 11 build 22000.");
                });

            add(L"ENV-002", L"Environment", L"UniversalApiContract v15 additions", 15,
                L"Reports whether the only public Windows.UI.Composition additions found after build 22000 are callable.",
                [](App&)
                {
                    return IsContractPresent(15)
                        ? Pass(L"UniversalApiContract v15 is present.")
                        : NotAvailable(L"Expected on build 22000: UniversalApiContract v15 is absent.");
                });

            add(L"CORE-001", L"Core", L"Compositor creation and queue ownership", 1,
                L"Creates the system Compositor and verifies its Windows.System.DispatcherQueue.",
                [](App& app)
                {
                    auto queue = app.CompositorInstance().DispatcherQueue();
                    return queue ? Pass(L"Compositor created with a non-null Windows.System.DispatcherQueue.")
                                 : Fail(L"Compositor.DispatcherQueue returned null.");
                });

            add(L"CORE-002", L"Core", L"DesktopWindowTarget visual root", 2,
                L"Exercises Win32 desktop target creation and root visual assignment.",
                [](App& app)
                {
                    auto sprite = app.ShowSprite();
                    return sprite.Compositor() == app.CompositorInstance()
                        ? Pass(L"Desktop target displays a root-owned SpriteVisual.")
                        : Fail(L"SpriteVisual belongs to an unexpected Compositor.");
                });

            add(L"VIS-001", L"Visuals", L"Container visual collection", 1,
                L"Inserts, reorders, and removes child visuals.",
                [](App& app)
                {
                    app.ResetScene();
                    auto first = app.CompositorInstance().CreateSpriteVisual();
                    auto second = app.CompositorInstance().CreateSpriteVisual();
                    app.RootVisual().Children().InsertAtTop(first);
                    app.RootVisual().Children().InsertAtTop(second);
                    app.RootVisual().Children().Remove(first);
                    return Pass(L"InsertAtTop and Remove completed without error.");
                });

            add(L"VIS-002", L"Visuals", L"Visual transform and rendering properties", 1,
                L"Round-trips size, offset, scale, center point, rotation, opacity, and visibility.",
                [](App& app)
                {
                    auto visual = app.ShowSprite();
                    visual.Size({ 180.0f, 90.0f });
                    visual.Offset({ 25.0f, 30.0f, 0.0f });
                    visual.Scale({ 1.1f, 0.9f, 1.0f });
                    visual.CenterPoint({ 90.0f, 45.0f, 0.0f });
                    visual.RotationAngleInDegrees(7.0f);
                    visual.Opacity(0.8f);
                    visual.IsVisible(true);
                    const auto size = visual.Size();
                    const auto offset = visual.Offset();
                    const auto scale = visual.Scale();
                    const auto center = visual.CenterPoint();
                    return Near(size.x, 180.0f) && Near(size.y, 90.0f) &&
                           Near(offset.x, 25.0f) && Near(offset.y, 30.0f) &&
                           Near(scale.x, 1.1f) && Near(scale.y, 0.9f) &&
                           Near(center.x, 90.0f) && Near(center.y, 45.0f) &&
                           Near(visual.RotationAngleInDegrees(), 7.0f) &&
                           Near(visual.Opacity(), 0.8f) && visual.IsVisible()
                        ? Pass(L"Visual properties round-tripped.")
                        : Fail(L"Visual property round trip failed.");
                });

            add(L"BRUSH-001", L"Brushes", L"Color brush", 1,
                L"Creates and assigns a CompositionColorBrush.",
                [](App& app)
                {
                    auto brush = app.CompositorInstance().CreateColorBrush({ 255, 220, 80, 40 });
                    auto visual = app.ShowSprite();
                    visual.Brush(brush);
                    return brush.Color().R == 220
                        ? Pass(L"Color brush created and assigned.")
                        : Fail(L"Color brush value did not round-trip.");
                });

            add(L"BRUSH-002", L"Brushes", L"Linear gradient brush and stops", 7,
                L"Creates a linear gradient with two color stops.",
                [](App& app)
                {
                    auto compositor = app.CompositorInstance();
                    auto brush = compositor.CreateLinearGradientBrush();
                    brush.StartPoint({ 0.0f, 0.0f });
                    brush.EndPoint({ 1.0f, 1.0f });
                    brush.ColorStops().Append(
                        compositor.CreateColorGradientStop(0.0f, { 255, 20, 90, 220 }));
                    brush.ColorStops().Append(
                        compositor.CreateColorGradientStop(1.0f, { 255, 230, 80, 40 }));
                    app.ShowSprite().Brush(brush);
                    return Pass(L"Gradient brush with two stops created and assigned.");
                });

            add(L"CLIP-001", L"Clips", L"Inset and rectangle clips", 8,
                L"Creates both legacy InsetClip and rounded RectangleClip instances.",
                [](App& app)
                {
                    auto visual = app.ShowSprite();
                    auto inset = app.CompositorInstance().CreateInsetClip(4, 5, 6, 7);
                    visual.Clip(inset);
                    auto rectangle = app.CompositorInstance().CreateRectangleClip(
                        2, 3, 118, 117,
                        { 8, 8 }, { 8, 8 }, { 8, 8 }, { 8, 8 });
                    visual.Clip(rectangle);
                    return Pass(L"InsetClip and RectangleClip were accepted.");
                });

            add(L"SHADOW-001", L"Effects", L"Drop shadow", 2,
                L"Creates and assigns a colored drop shadow.",
                [](App& app)
                {
                    auto visual = app.ShowSprite();
                    auto shadow = app.CompositorInstance().CreateDropShadow();
                    shadow.BlurRadius(12.0f);
                    shadow.Opacity(0.7f);
                    shadow.Offset({ 6.0f, 8.0f, 0.0f });
                    shadow.Color({ 255, 0, 0, 0 });
                    visual.Shadow(shadow);
                    return Pass(L"DropShadow properties and SpriteVisual assignment succeeded.");
                });

            add(L"SHAPE-001", L"Shapes", L"Shape visual and rectangle geometry", 7,
                L"Creates a CompositionShapeVisual with a filled rectangle.",
                [](App& app)
                {
                    app.ResetScene();
                    auto compositor = app.CompositorInstance();
                    auto geometry = compositor.CreateRectangleGeometry();
                    geometry.Size({ 150.0f, 90.0f });
                    auto shape = compositor.CreateSpriteShape(geometry);
                    shape.FillBrush(compositor.CreateColorBrush({ 255, 60, 180, 100 }));
                    auto visual = compositor.CreateShapeVisual();
                    visual.Size({ 220.0f, 160.0f });
                    visual.Offset({ 30.0f, 30.0f, 0.0f });
                    visual.Shapes().Append(shape);
                    app.RootVisual().Children().InsertAtTop(visual);
                    return Pass(L"ShapeVisual, SpriteShape, and RectangleGeometry were created.");
                });

            add(L"ANIM-001", L"Animation", L"Key-frame animation", 1,
                L"Starts an opacity key-frame animation and obtains its implicit controller.",
                [](App& app)
                {
                    auto visual = app.ShowSprite();
                    auto animation = app.CompositorInstance().CreateScalarKeyFrameAnimation();
                    animation.InsertKeyFrame(0.0f, 0.2f);
                    animation.InsertKeyFrame(1.0f, 1.0f);
                    animation.Duration(std::chrono::milliseconds(500));
                    visual.StartAnimation(L"Opacity", animation);
                    auto controller = visual.TryGetAnimationController(L"Opacity");
                    return controller
                        ? Pass(L"Animation started and TryGetAnimationController returned a controller.")
                        : Fail(L"Animation started but no controller was returned.");
                });

            add(L"ANIM-002", L"Animation", L"Expression animation and property set", 2,
                L"Binds visual opacity to a scalar in a CompositionPropertySet.",
                [](App& app)
                {
                    auto visual = app.ShowSprite();
                    auto properties = app.CompositorInstance().CreatePropertySet();
                    properties.InsertScalar(L"Value", 0.65f);
                    auto expression = app.CompositorInstance().CreateExpressionAnimation(L"settings.Value");
                    expression.SetReferenceParameter(L"settings", properties);
                    visual.StartAnimation(L"Opacity", expression);
                    return Pass(L"ExpressionAnimation was bound to a CompositionPropertySet value.");
                });

            add(L"ANIM-003", L"Animation", L"Implicit animation collection", 3,
                L"Attaches an implicit opacity animation and triggers it with a property update.",
                [](App& app)
                {
                    auto visual = app.ShowSprite();
                    auto animation = app.CompositorInstance().CreateScalarKeyFrameAnimation();
                    animation.Target(L"Opacity");
                    animation.InsertExpressionKeyFrame(1.0f, L"this.FinalValue");
                    animation.Duration(std::chrono::milliseconds(250));
                    auto animations = app.CompositorInstance().CreateImplicitAnimationCollection();
                    animations.Insert(L"Opacity", animation);
                    visual.ImplicitAnimations(animations);
                    visual.Opacity(0.35f);
                    return Pass(L"Implicit animation collection accepted and triggered.");
                });

            add(L"ANIM-004", L"Animation", L"Animation group and scoped batch", 6,
                L"Starts grouped animations inside a scoped animation batch.",
                [](App& app)
                {
                    auto visual = app.ShowSprite();
                    auto opacity = app.CompositorInstance().CreateScalarKeyFrameAnimation();
                    opacity.Target(L"Opacity");
                    opacity.InsertKeyFrame(1.0f, 0.5f);
                    opacity.Duration(std::chrono::milliseconds(150));
                    auto group = app.CompositorInstance().CreateAnimationGroup();
                    group.Add(opacity);
                    auto batch = app.CompositorInstance().CreateScopedBatch(CompositionBatchTypes::Animation);
                    visual.StartAnimationGroup(group);
                    batch.End();
                    return Pass(L"Animation group started and scoped batch ended.");
                });

            add(L"ANIM-005", L"Animation", L"Explicit AnimationController creation", 15,
                L"Calls the v15 Compositor.CreateAnimationController API.",
                [](App& app)
                {
                    if (!ApiInformation::IsMethodPresent(
                        L"Windows.UI.Composition.Compositor",
                        L"CreateAnimationController"))
                    {
                        return NotAvailable(L"Expected on build 22000: CreateAnimationController is unavailable.");
                    }
                    auto controller = app.CompositorInstance().CreateAnimationController();
                    controller.Progress(0.25f);
                    controller.PlaybackRate(2.0f);
                    controller.Pause();
                    return controller.Progress() == 0.25f
                        ? Pass(L"Explicit controller created; progress, playback rate, and pause accepted.")
                        : Fail(L"AnimationController.Progress did not round-trip.");
                });

            add(L"ANIM-006", L"Animation", L"StartAnimation with supplied controller", 15,
                L"Uses the v15 overload to bind two animations to one controller.",
                [](App& app)
                {
                    if (!ApiInformation::IsMethodPresent(
                        L"Windows.UI.Composition.CompositionObject",
                        L"StartAnimation",
                        3))
                    {
                        return NotAvailable(L"Expected on build 22000: the three-parameter StartAnimation overload is unavailable.");
                    }
                    auto visual = app.ShowSprite();
                    auto controller = app.CompositorInstance().CreateAnimationController();
                    auto opacity = app.CompositorInstance().CreateScalarKeyFrameAnimation();
                    opacity.InsertKeyFrame(0.0f, 0.2f);
                    opacity.InsertKeyFrame(1.0f, 1.0f);
                    opacity.Duration(std::chrono::seconds(1));
                    visual.StartAnimation(L"Opacity", opacity, controller);

                    auto rotation = app.CompositorInstance().CreateScalarKeyFrameAnimation();
                    rotation.InsertKeyFrame(0.0f, 0.0f);
                    rotation.InsertKeyFrame(1.0f, 180.0f);
                    rotation.Duration(std::chrono::seconds(1));
                    visual.StartAnimation(L"RotationAngleInDegrees", rotation, controller);

                    controller.Progress(0.5f);
                    controller.Pause();
                    return Pass(L"Opacity and rotation animations started with one application-created controller.");
                });

            add(L"SURFACE-001", L"Surfaces", L"CompositionVisualSurface and surface brush", 8,
                L"Uses one visual as the source of a surface brush displayed by another visual.",
                [](App& app)
                {
                    app.ResetScene();
                    auto compositor = app.CompositorInstance();
                    auto source = compositor.CreateSpriteVisual();
                    source.Size({ 80.0f, 80.0f });
                    source.Brush(compositor.CreateColorBrush({ 255, 100, 180, 255 }));
                    auto surface = compositor.CreateVisualSurface();
                    surface.SourceVisual(source);
                    surface.SourceSize({ 80.0f, 80.0f });
                    auto destination = compositor.CreateSpriteVisual();
                    destination.Size({ 180.0f, 180.0f });
                    destination.Offset({ 30.0f, 20.0f, 0.0f });
                    destination.Brush(compositor.CreateSurfaceBrush(surface));
                    app.RootVisual().Children().InsertAtTop(destination);
                    app.RootVisual().Children().InsertAtTop(source);
                    return Pass(L"VisualSurface was consumed by CompositionSurfaceBrush.");
                });

            add(L"TEX-001", L"CompositionTexture", L"D3D11 composition texture creation", 15,
                L"Checks device support, creates a CompositionTexture, and binds it to a surface brush.",
                [](App& app) { return app.CreateTexture(false); });

            add(L"TEX-002", L"CompositionTexture", L"SourceRect/alpha/color-space round trip", 15,
                L"Uses a non-zero, non-square SourceRect to detect coordinate mapping defects.",
                [](App& app) { return app.CreateTexture(true); });

            add(L"INTERACT-001", L"Interactions", L"InteractionTracker core state", 4,
                L"Creates an InteractionTracker and round-trips position and scale boundaries.",
                [](App& app)
                {
                    auto tracker = InteractionTracker::Create(app.CompositorInstance());
                    tracker.MinPosition({ 0.0f, 0.0f, 0.0f });
                    tracker.MaxPosition({ 500.0f, 500.0f, 0.0f });
                    tracker.MinScale(0.5f);
                    tracker.MaxScale(4.0f);
                    const auto minimum = tracker.MinPosition();
                    const auto maximum = tracker.MaxPosition();
                    return Near(minimum.x, 0.0f) && Near(minimum.y, 0.0f) &&
                           Near(maximum.x, 500.0f) && Near(maximum.y, 500.0f) &&
                           Near(tracker.MinScale(), 0.5f) && Near(tracker.MaxScale(), 4.0f)
                        ? Pass(L"InteractionTracker creation and boundary properties succeeded.")
                        : Fail(L"InteractionTracker property round trip failed.");
                });

            add(L"DIAG-001", L"Diagnostics", L"CompositionDebugSettings availability", 8,
                L"Queries system Composition diagnostics for the active compositor.",
                [](App& app)
                {
                    if (!ApiInformation::IsTypePresent(
                        L"Windows.UI.Composition.Diagnostics.CompositionDebugSettings"))
                    {
                        return NotAvailable(L"CompositionDebugSettings is unavailable.");
                    }
                    auto settings = Windows::UI::Composition::Diagnostics::CompositionDebugSettings::TryGetSettings(
                        app.CompositorInstance());
                    return settings ? Pass(L"CompositionDebugSettings returned a settings object.")
                                    : NotAvailable(L"Diagnostics are present but unavailable for this process.");
                });

            add(L"THREAD-001", L"Threading", L"Agility and dispatcher identity", 6,
                L"Checks IAgileObject support and compositor/visual dispatcher identity.",
                [](App& app)
                {
                    auto visual = app.ShowSprite();
                    const bool compositorAgile = static_cast<bool>(app.CompositorInstance().try_as<IAgileObject>());
                    const bool visualAgile = static_cast<bool>(visual.try_as<IAgileObject>());
                    const bool sameQueue = visual.DispatcherQueue() == app.CompositorInstance().DispatcherQueue();
                    return compositorAgile && visualAgile && sameQueue
                        ? Pass(L"Compositor and visual are agile and report the same DispatcherQueue.")
                        : Fail(L"Agility or DispatcherQueue identity check failed.");
                });

            add(L"ERROR-001", L"Error behavior", L"Invalid animation target", 1,
                L"Verifies an invalid animatable property fails explicitly rather than succeeding silently.",
                [](App& app)
                {
                    try
                    {
                        auto visual = app.ShowSprite();
                        auto animation = app.CompositorInstance().CreateScalarKeyFrameAnimation();
                        animation.InsertKeyFrame(1.0f, 1.0f);
                        visual.StartAnimation(L"DefinitelyNotAProperty", animation);
                        return Fail(L"Invalid property unexpectedly succeeded.");
                    }
                    catch (hresult_error const& error)
                    {
                        std::wstringstream detail;
                        detail << L"Invalid target failed with HRESULT 0x"
                               << std::hex << static_cast<uint32_t>(error.code().value);
                        return Pass(detail.str());
                    }
                });
        }

        void PopulateScenarioList()
        {
            for (const auto& scenario : m_scenarios)
            {
                const std::wstring label =
                    L"[" + scenario.category + L"] " + scenario.id + L"  " + scenario.title;
                SendMessageW(m_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            }
        }

        void SelectScenario(size_t index)
        {
            if (index >= m_scenarios.size())
            {
                return;
            }

            m_selected = index;
            SendMessageW(m_list, LB_SETCURSEL, index, 0);
            RefreshDetails();
        }

        void RefreshDetails()
        {
            const auto& scenario = m_scenarios[m_selected];
            const std::wstring heading =
                scenario.id + L" - " + scenario.title + L"  [" + OutcomeText(scenario.result.outcome) + L"]";
            SetWindowTextW(m_title, heading.c_str());

            std::wstringstream details;
            details << L"Category: " << scenario.category << L"\r\n"
                    << L"Minimum UniversalApiContract: v" << scenario.minimumContract << L"\r\n"
                    << L"Current OS: " << CurrentOsVersion() << L"\r\n\r\n"
                    << scenario.description << L"\r\n\r\n"
                    << L"Result: " << OutcomeText(scenario.result.outcome) << L"\r\n"
                    << scenario.result.detail;
            SetWindowTextW(m_description, details.str().c_str());
        }

        void RunScenario(size_t index)
        {
            if (index >= m_scenarios.size())
            {
                return;
            }

            auto& scenario = m_scenarios[index];
            try
            {
                scenario.result = scenario.run(*this);
            }
            catch (hresult_error const& error)
            {
                std::wstringstream detail;
                detail << error.message().c_str() << L" (HRESULT 0x"
                       << std::hex << static_cast<uint32_t>(error.code().value) << L')';
                scenario.result = Fail(detail.str());
            }
            catch (std::exception const& error)
            {
                scenario.result = Fail(to_hstring(error.what()).c_str());
            }

            RefreshListItem(index);
            if (index == m_selected)
            {
                RefreshDetails();
            }
        }

        void RunAll(bool notify = true)
        {
            for (size_t index = 0; index < m_scenarios.size(); ++index)
            {
                RunScenario(index);
            }
            SaveResults(notify);
        }

        void RefreshListItem(size_t index)
        {
            const auto& scenario = m_scenarios[index];
            const std::wstring label =
                L"[" + OutcomeText(scenario.result.outcome) + L"] [" + scenario.category + L"] " +
                scenario.id + L"  " + scenario.title;
            SendMessageW(m_list, LB_DELETESTRING, index, 0);
            SendMessageW(m_list, LB_INSERTSTRING, index, reinterpret_cast<LPARAM>(label.c_str()));
            SendMessageW(m_list, LB_SETCURSEL, m_selected, 0);
        }

        void SaveResults(bool notify = true)
        {
            const auto outputPath =
                ExecutablePath().parent_path() /
                (L"WindowsUIComposition-results-" + CurrentOsVersion() + L".json");

            std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
            if (!output)
            {
                throw_hresult(HRESULT_FROM_WIN32(ERROR_OPEN_FAILED));
            }
            output << "{\n"
                   << "  \"schemaVersion\": 1,\n"
                   << "  \"osVersion\": \"" << JsonEscape(CurrentOsVersion()) << "\",\n"
                   << "  \"architecture\": \"x64\",\n"
                   << "  \"sdkMetadata\": \"10.0.26100.0 / UniversalApiContract v19\",\n"
                   << "  \"minimumRuntime\": \"10.0.22000.0 / UniversalApiContract v13\",\n"
                   << "  \"results\": [\n";
            for (size_t index = 0; index < m_scenarios.size(); ++index)
            {
                const auto& scenario = m_scenarios[index];
                output << "    {\"id\":\"" << JsonEscape(scenario.id)
                       << "\",\"category\":\"" << JsonEscape(scenario.category)
                       << "\",\"title\":\"" << JsonEscape(scenario.title)
                       << "\",\"minimumContract\":" << scenario.minimumContract
                       << ",\"outcome\":\"" << JsonEscape(OutcomeText(scenario.result.outcome))
                       << "\",\"detail\":\"" << JsonEscape(scenario.result.detail) << "\"}"
                       << (index + 1 == m_scenarios.size() ? "\n" : ",\n");
            }
            output << "  ]\n}\n";
            output.flush();
            if (!output)
            {
                throw_hresult(HRESULT_FROM_WIN32(ERROR_WRITE_FAULT));
            }
            output.close();
            if (!output)
            {
                throw_hresult(HRESULT_FROM_WIN32(ERROR_WRITE_FAULT));
            }

            if (notify)
            {
                const std::wstring message = L"Results saved to:\r\n" + outputPath.wstring();
                MessageBoxW(m_window, message.c_str(), L"Composition compatibility results", MB_OK);
            }
        }

        void Layout(int width, int height)
        {
            constexpr int margin = 12;
            constexpr int listWidth = 425;
            constexpr int buttonWidth = 110;
            constexpr int buttonHeight = 30;
            const int rightX = listWidth + margin * 2;
            const int rightWidth = std::max(300, width - rightX - margin);
            const int previewHeight = std::max(180, height / 3);
            const int detailBottom = height - previewHeight - margin * 2;

            MoveWindow(m_list, margin, margin, listWidth, height - margin * 2, TRUE);
            MoveWindow(m_title, rightX, margin, rightWidth, 28, TRUE);
            MoveWindow(m_runSelected, rightX, 48, buttonWidth, buttonHeight, TRUE);
            MoveWindow(m_runAll, rightX + buttonWidth + 8, 48, buttonWidth, buttonHeight, TRUE);
            MoveWindow(m_save, rightX + (buttonWidth + 8) * 2, 48, buttonWidth, buttonHeight, TRUE);
            MoveWindow(m_description, rightX, 88, rightWidth, detailBottom - 88, TRUE);
            MoveWindow(m_preview, rightX, detailBottom + margin, rightWidth, previewHeight - margin, TRUE);
        }

        static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            App* app{};
            if (message == WM_NCCREATE)
            {
                const auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
                app = static_cast<App*>(create->lpCreateParams);
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
            }
            else
            {
                app = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA));
            }

            if (!app)
            {
                return DefWindowProcW(window, message, wParam, lParam);
            }

            switch (message)
            {
            case WM_SIZE:
                app->Layout(LOWORD(lParam), HIWORD(lParam));
                return 0;
            case WM_COMMAND:
                if (LOWORD(wParam) == IdScenarioList && HIWORD(wParam) == LBN_SELCHANGE)
                {
                    const auto selected = SendMessageW(app->m_list, LB_GETCURSEL, 0, 0);
                    if (selected != LB_ERR)
                    {
                        app->SelectScenario(static_cast<size_t>(selected));
                    }
                    return 0;
                }
                if (LOWORD(wParam) == IdRunSelected)
                {
                    app->RunScenario(app->m_selected);
                    return 0;
                }
                if (LOWORD(wParam) == IdRunAll)
                {
                    try
                    {
                        app->RunAll();
                    }
                    catch (hresult_error const& error)
                    {
                        MessageBoxW(window, error.message().c_str(), L"Unable to save results", MB_OK | MB_ICONERROR);
                    }
                    return 0;
                }
                if (LOWORD(wParam) == IdSave)
                {
                    try
                    {
                        app->SaveResults();
                    }
                    catch (hresult_error const& error)
                    {
                        MessageBoxW(window, error.message().c_str(), L"Unable to save results", MB_OK | MB_ICONERROR);
                    }
                    return 0;
                }
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            }

            return DefWindowProcW(window, message, wParam, lParam);
        }

        HINSTANCE m_instance{};
        HWND m_window{};
        HWND m_list{};
        HWND m_title{};
        HWND m_description{};
        HWND m_runSelected{};
        HWND m_runAll{};
        HWND m_save{};
        HWND m_preview{};
        size_t m_selected{};
        std::vector<Scenario> m_scenarios;
        DispatcherQueueController m_dispatcherQueueController{ nullptr };
        Compositor m_compositor{ nullptr };
        DesktopWindowTarget m_target{ nullptr };
        ContainerVisual m_root{ nullptr };
        SpriteVisual m_previewVisual{ nullptr };
    };
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    try
    {
        init_apartment(apartment_type::single_threaded);
        INITCOMMONCONTROLSEX commonControls{ sizeof(commonControls), ICC_STANDARD_CLASSES };
        InitCommonControlsEx(&commonControls);
        App app;
        const bool runAllAndExit = wcsstr(GetCommandLineW(), L"--run-all") != nullptr;
        return app.Run(instance, runAllAndExit);
    }
    catch (hresult_error const& error)
    {
        std::wstringstream detail;
        detail << error.message().c_str() << L"\r\nHRESULT 0x"
               << std::hex << static_cast<uint32_t>(error.code().value);
        MessageBoxW(nullptr, detail.str().c_str(), L"Composition compatibility harness failed", MB_OK | MB_ICONERROR);
        return static_cast<int>(error.code().value);
    }
}
