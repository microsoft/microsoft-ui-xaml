#include "TreeSnapshot.h"
#include "TreeSnapshotJson.h"
#include "ResourceSnapshot.h"
#include "Tap.h"
#include <xamlom.h>
#include <wrl.h>
// Keep the SDK's C++17 coroutine shim compatible with newer MSVC STLs.
#define _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS
#undef GetCurrentTime
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Numerics.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

namespace WinUIProfiler::Tap
{
    namespace
    {
        namespace X = winrt::Microsoft::UI::Xaml;
        namespace C = winrt::Microsoft::UI::Composition;
        using Inspectable = winrt::Windows::Foundation::IInspectable;
        using Microsoft::WRL::ComPtr;
        constexpr size_t MaxBytes = 64 * 1024 * 1024;
        constexpr size_t MaxEntries = 131072;
        constexpr size_t MaxPropertiesBytes = 64 * 1024;
        constexpr size_t MaxTextCharacters = 4096;
        constexpr size_t MaxRoots = 1024;
        constexpr unsigned MaxDepth = 1024;
        constexpr ULONGLONG DeadlineMs = 20000;
        std::atomic<bool> Running{};

        [[noreturn]] void Fail(HRESULT hr) { throw winrt::hresult_error(hr); }
        int64_t Qpc()
        {
            LARGE_INTEGER value{};
            if (!QueryPerformanceCounter(&value)) Fail(HRESULT_FROM_WIN32(GetLastError()));
            return value.QuadPart;
        }
        std::string Hex(uint64_t value)
        {
            char buffer[19]{ '0', 'x' };
            const auto converted = std::to_chars(buffer + 2, buffer + sizeof(buffer), value, 16);
            return { buffer, converted.ptr };
        }
        // TODO: Use a JSON library instead of manual escaping and string concatenation, preserving 64-bit integer precision.
        std::string Quote(const std::string& value)
        {
            if (value.size() > MaxBytes / 6) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
            std::string out = "\"";
            for (unsigned char ch : value)
            {
                if (ch == '"' || ch == '\\') { out += '\\'; out += static_cast<char>(ch); }
                else if (ch < 0x20)
                {
                    constexpr char digits[] = "0123456789abcdef";
                    out += "\\u00"; out += digits[ch >> 4]; out += digits[ch & 15];
                }
                else out += static_cast<char>(ch);
            }
            out += '"';
            return out;
        }
        std::string String(const winrt::hstring& value)
        {
            if (value.size() > MaxBytes / 6) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
            return Quote(winrt::to_string(value));
        }
        std::string ResourceJson(HRESULT status, const ResourceSnapshotAbi::Snapshot& snapshot,
            int64_t start, int64_t end)
        {
            std::string json = "{\"status\":" + Quote(SUCCEEDED(status) ? "available" :
                status == E_NOINTERFACE ? "unavailable" : "failed") +
                ",\"hResult\":" + std::to_string(static_cast<int32_t>(status)) +
                ",\"startQpc\":" + std::to_string(start) + ",\"endQpc\":" + std::to_string(end) +
                ",\"values\":";
            if (FAILED(status)) return json + "null}";
            json += "{\"allocCount\":" + std::to_string(snapshot.AllocCount) +
                ",\"allocSize\":" + std::to_string(snapshot.AllocSize) +
                ",\"allocatedMemory\":" + std::to_string(snapshot.AllocatedMemory) +
                ",\"outstandingAllocCount\":" + std::to_string(snapshot.OutstandingAllocCount) +
                ",\"outstandingAllocSize\":" + std::to_string(snapshot.OutstandingAllocSize) +
                ",\"resources\":[";
            bool first = true;
            for (size_t index = 0; index < std::size(snapshot.Resources); ++index)
            {
                const auto& resource = snapshot.Resources[index];
                if (!resource.TotalCreated && !resource.TotalDestroyed && !resource.LiveCount) continue;
                if (!first) json += ',';
                first = false;
                json += "{\"index\":" + std::to_string(index) +
                    ",\"totalCreated\":" + std::to_string(resource.TotalCreated) +
                    ",\"totalDestroyed\":" + std::to_string(resource.TotalDestroyed) +
                    ",\"liveCount\":" + std::to_string(resource.LiveCount) + "}";
            }
            return json + "]}}";
        }
        std::string Number(double value)
        {
            if (!std::isfinite(value)) return "{\"error\":\"non-finite native value\"}";
            char buffer[64];
            auto converted = std::to_chars(buffer, buffer + sizeof(buffer), value);
            if (converted.ec != std::errc{}) Fail(E_UNEXPECTED);
            return { buffer, converted.ptr };
        }
        std::string Value(double v) { return Number(v); }
        std::string Value(float v) { return Number(v); }
        std::string Value(bool v) { return v ? "true" : "false"; }
        std::string Value(const winrt::hstring& v) { return String(v); }
        std::string Value(winrt::Windows::Foundation::Point v)
        { return "{\"x\":" + Number(v.X) + ",\"y\":" + Number(v.Y) + "}"; }
        std::string Value(winrt::Windows::Foundation::Rect v)
        {
            return "{\"x\":" + Number(v.X) + ",\"y\":" + Number(v.Y) +
                ",\"width\":" + Number(v.Width) + ",\"height\":" + Number(v.Height) + "}";
        }
        std::string Value(X::Thickness v)
        {
            return "{\"left\":" + Number(v.Left) + ",\"top\":" + Number(v.Top) +
                ",\"right\":" + Number(v.Right) + ",\"bottom\":" + Number(v.Bottom) + "}";
        }
        std::string Value(X::HorizontalAlignment v)
        {
            switch (v)
            {
            case X::HorizontalAlignment::Left: return "\"Left\"";
            case X::HorizontalAlignment::Center: return "\"Center\"";
            case X::HorizontalAlignment::Right: return "\"Right\"";
            case X::HorizontalAlignment::Stretch: return "\"Stretch\"";
            default: Fail(E_UNEXPECTED);
            }
        }
        std::string Value(X::VerticalAlignment v)
        {
            switch (v)
            {
            case X::VerticalAlignment::Top: return "\"Top\"";
            case X::VerticalAlignment::Center: return "\"Center\"";
            case X::VerticalAlignment::Bottom: return "\"Bottom\"";
            case X::VerticalAlignment::Stretch: return "\"Stretch\"";
            default: Fail(E_UNEXPECTED);
            }
        }
        std::string Value(winrt::Windows::UI::Color v)
        {
            constexpr char digits[] = "0123456789ABCDEF";
            std::string color = "#";
            for (const auto channel : { v.A, v.R, v.G, v.B })
            { color += digits[channel >> 4]; color += digits[channel & 15]; }
            return Quote(color);
        }
        std::string Value(X::Media::Matrix v)
        {
            return "[" + Number(v.M11) + "," + Number(v.M12) + "," + Number(v.M21) +
                "," + Number(v.M22) + "," + Number(v.OffsetX) + "," + Number(v.OffsetY) + "]";
        }
        std::string Value(const X::Media::Brush& v);
        std::string Value(const X::Media::Transform& v);
        std::string Value(winrt::Windows::Foundation::Numerics::float2 v)
        { return "[" + Number(v.x) + "," + Number(v.y) + "]"; }
        std::string Value(winrt::Windows::Foundation::Numerics::float3 v)
        { return "[" + Number(v.x) + "," + Number(v.y) + "," + Number(v.z) + "]"; }
        std::string Value(winrt::Windows::Foundation::Numerics::float4x4 v)
        {
            return "[" + Number(v.m11) + "," + Number(v.m12) + "," + Number(v.m13) + "," + Number(v.m14) +
                "," + Number(v.m21) + "," + Number(v.m22) + "," + Number(v.m23) + "," + Number(v.m24) +
                "," + Number(v.m31) + "," + Number(v.m32) + "," + Number(v.m33) + "," + Number(v.m34) +
                "," + Number(v.m41) + "," + Number(v.m42) + "," + Number(v.m43) + "," + Number(v.m44) + "]";
        }
        struct Properties
        {
            std::string Json = "{";
            template<typename F> void Read(const char* name, F getter)
            {
                if (Json.size() != 1) Json += ',';
                Json += Quote(name) + ":";
                try { Json += Value(getter()); }
                catch (const winrt::hresult_error& e)
                {
                    if (e.code() == E_OUTOFMEMORY ||
                        e.code() == HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW)) throw;
                    Json += "{\"error\":" + Quote(Hex(static_cast<uint32_t>(e.code()))) + "}";
                }
                if (Json.size() + 1 > MaxPropertiesBytes) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
            }
            std::string Finish() { return Json + "}"; }
        };
        std::string Value(const X::Media::Brush& v)
        {
            if (!v) return "null";
            Properties properties;
            properties.Read("type", [&] { return winrt::get_class_name(v); });
            properties.Read("opacity", [&] { return v.Opacity(); });
            if (const auto solid = v.try_as<X::Media::SolidColorBrush>())
                properties.Read("color", [&] { return solid.Color(); });
            return properties.Finish();
        }
        std::string Value(const X::Media::Transform& v)
        {
            if (!v) return "null";
            Properties properties;
            properties.Read("type", [&] { return winrt::get_class_name(v); });
            if (const auto composite = v.try_as<X::Media::CompositeTransform>())
            {
                properties.Read("translateX", [&] { return composite.TranslateX(); });
                properties.Read("translateY", [&] { return composite.TranslateY(); });
                properties.Read("rotation", [&] { return composite.Rotation(); });
                properties.Read("scaleX", [&] { return composite.ScaleX(); });
                properties.Read("scaleY", [&] { return composite.ScaleY(); });
                properties.Read("skewX", [&] { return composite.SkewX(); });
                properties.Read("skewY", [&] { return composite.SkewY(); });
                properties.Read("centerX", [&] { return composite.CenterX(); });
                properties.Read("centerY", [&] { return composite.CenterY(); });
            }
            else if (const auto translate = v.try_as<X::Media::TranslateTransform>())
            {
                properties.Read("x", [&] { return translate.X(); });
                properties.Read("y", [&] { return translate.Y(); });
            }
            else if (const auto rotate = v.try_as<X::Media::RotateTransform>())
            {
                properties.Read("angle", [&] { return rotate.Angle(); });
                properties.Read("centerX", [&] { return rotate.CenterX(); });
                properties.Read("centerY", [&] { return rotate.CenterY(); });
            }
            else if (const auto scale = v.try_as<X::Media::ScaleTransform>())
            {
                properties.Read("scaleX", [&] { return scale.ScaleX(); });
                properties.Read("scaleY", [&] { return scale.ScaleY(); });
                properties.Read("centerX", [&] { return scale.CenterX(); });
                properties.Read("centerY", [&] { return scale.CenterY(); });
            }
            else if (const auto skew = v.try_as<X::Media::SkewTransform>())
            {
                properties.Read("angleX", [&] { return skew.AngleX(); });
                properties.Read("angleY", [&] { return skew.AngleY(); });
                properties.Read("centerX", [&] { return skew.CenterX(); });
                properties.Read("centerY", [&] { return skew.CenterY(); });
            }
            else if (const auto matrix = v.try_as<X::Media::MatrixTransform>())
                properties.Read("matrix", [&] { return matrix.Matrix(); });
            else if (const auto group = v.try_as<X::Media::TransformGroup>())
                properties.Read("matrix", [&] { return group.Value(); });
            else Fail(E_NOTIMPL);
            return properties.Finish();
        }
        struct Identity
        {
            winrt::com_ptr<IUnknown> Ref;
            explicit Identity(const Inspectable& value) : Ref(value.as<IUnknown>()) {}
            uintptr_t Address() const { return reinterpret_cast<uintptr_t>(Ref.get()); }
        };
        struct Array
        {
            std::string Json;
            void Add(std::string value)
            {
                if (value.size() >= MaxBytes || Json.size() > MaxBytes - value.size() - 1)
                    Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                if (!Json.empty()) Json += ',';
                Json += value;
            }
        };
        struct Job
        {
            ComPtr<IXamlDiagnostics> Diagnostics;
            Handle Stop;
            Handle Done{ CreateEventW(nullptr, TRUE, FALSE, nullptr) };
            const ULONGLONG Deadline = GetTickCount64() + DeadlineMs;
            std::atomic<bool> Abandoned{};
            std::atomic<bool> Accept{ true };
            std::mutex Mutex;
            HRESULT Status = S_OK;
            const char* Failure{};
            Array Roots, Xaml, Composition, Correlations;
            size_t Entries{};
            size_t Bytes{};
            uint64_t RootNext = 1, XamlNext = 1, VisualNext = 1;
            size_t UnresolvedHandles{};
            std::set<std::pair<DWORD, uintptr_t>> SeenRoots;

            void Check()
            {
                if (Abandoned || (Stop.Get() && WaitForSingleObject(Stop.Get(), 0) == WAIT_OBJECT_0))
                    Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
                if (GetTickCount64() >= Deadline) Fail(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
            }
            void Entry(size_t count = 1)
            {
                Check();
                if (count > MaxEntries - Entries) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                Entries += count;
            }
            void Add(Array& array, std::string value)
            {
                Check();
                if (value.size() + 1 > MaxBytes - Bytes) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                Bytes += value.size() + 1;
                array.Add(std::move(value));
            }
        };
        std::string Parent(uint64_t id) { return id ? std::to_string(id) : "null"; }

        struct XamlItem
        {
            X::DependencyObject Object{ nullptr };
            uint64_t Id{}, ParentId{};
            unsigned Index{}, Depth{};
        };
        struct VisualItem
        {
            C::Visual Object{ nullptr };
            uint64_t ParentId{};
            unsigned Index{}, Depth{};
        };

        void CaptureRoot(Job& job, const X::XamlRoot& root)
        {
            job.Check();
            Identity rootIdentity(root);
            if (!job.SeenRoots.emplace(GetCurrentThreadId(), rootIdentity.Address()).second) return;
            if (job.SeenRoots.size() > MaxRoots) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
            job.Entry();
            const uint64_t rootId = job.RootNext++;
            const int64_t start = Qpc();
            std::vector<XamlItem> pending;
            const auto content = root.Content();
            if (content) pending.push_back({ content, 0, 0, 0, 0 });
            // Popup children are disconnected visual subtrees, not logical-parent edges.
            const auto popups = X::Media::VisualTreeHelper::GetOpenPopupsForXamlRoot(root);
            unsigned popupIndex = 1;
            for (const auto& popup : popups)
            {
                job.Check();
                const auto child = popup.Child();
                if (child) pending.push_back({ child, 0, 0, popupIndex++, 0 });
                if (pending.size() > MaxEntries) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
            }
            std::unordered_map<uintptr_t, uint64_t> xamlIds;
            std::vector<Identity> xamlIdentities;
            std::vector<std::pair<uint64_t, C::Visual>> handouts;
            std::vector<X::DependencyObject> xamlKeepAlive;
            while (!pending.empty())
            {
                job.Check();
                auto item = std::move(pending.back()); pending.pop_back();
                if (item.Depth > MaxDepth) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                Identity identity(item.Object);
                if (xamlIds.count(identity.Address())) continue;
                job.Entry(item.ParentId ? 2 : 1);
                item.Id = job.XamlNext++;
                xamlIds.emplace(identity.Address(), item.Id);
                xamlIdentities.push_back(std::move(identity));
                xamlKeepAlive.push_back(item.Object);
                Properties properties;
                std::string name = "null";
                const auto element = item.Object.try_as<X::UIElement>();
                if (element)
                {
                    properties.Read("opacity", [&] { return element.Opacity(); });
                    properties.Read("visibility", [&] { return winrt::hstring(element.Visibility() ==
                        X::Visibility::Visible ? L"Visible" : L"Collapsed"); });
                    properties.Read("renderTransform", [&] { return element.RenderTransform(); });
                    properties.Read("renderTransformOrigin", [&] { return element.RenderTransformOrigin(); });
                    const auto framework = element.try_as<X::FrameworkElement>();
                    if (framework)
                    {
                        // Only SDK getters; do not evaluate arbitrary app properties or bindings.
                        properties.Read("name", [&] { auto n = framework.Name(); name = String(n); return n; });
                        properties.Read("actualWidth", [&] { return framework.ActualWidth(); });
                        properties.Read("actualHeight", [&] { return framework.ActualHeight(); });
                        properties.Read("width", [&] { return framework.Width(); });
                        properties.Read("height", [&] { return framework.Height(); });
                        properties.Read("margin", [&] { return framework.Margin(); });
                        properties.Read("horizontalAlignment", [&] { return framework.HorizontalAlignment(); });
                        properties.Read("verticalAlignment", [&] { return framework.VerticalAlignment(); });
                        properties.Read("layoutSlot", [&] { return X::Controls::Primitives::LayoutInformation::GetLayoutSlot(framework); });
                        properties.Read("boundsInParent", [&]
                        {
                            const auto parent = X::Media::VisualTreeHelper::GetParent(element).try_as<X::UIElement>();
                            return element.TransformToVisual(parent).TransformBounds({ 0, 0,
                                static_cast<float>(framework.ActualWidth()), static_cast<float>(framework.ActualHeight()) });
                        });
                    }
                    if (const auto control = element.try_as<X::Controls::Control>())
                        properties.Read("background", [&] { return control.Background(); });
                    else if (const auto panel = element.try_as<X::Controls::Panel>())
                        properties.Read("background", [&] { return panel.Background(); });
                    else if (const auto border = element.try_as<X::Controls::Border>())
                        properties.Read("background", [&] { return border.Background(); });
                    if (const auto text = element.try_as<X::Controls::TextBlock>())
                    {
                        properties.Read("text", [&]
                        {
                            auto value = text.Text();
                            if (value.size() > MaxTextCharacters) Fail(HRESULT_FROM_WIN32(ERROR_MORE_DATA));
                            return value;
                        });
                        properties.Read("foreground", [&] { return text.Foreground(); });
                    }
                    properties.Read("elementVisualAvailable", [&]
                    {
                        auto visual = X::Hosting::ElementCompositionPreview::GetElementVisual(element);
                        if (!visual) Fail(E_NOINTERFACE);
                        handouts.emplace_back(item.Id, std::move(visual));
                        return true;
                    });
                }
                InstanceHandle handle{};
                const auto handleHr = job.Diagnostics->GetHandleFromIInspectable(
                    reinterpret_cast<IInspectable*>(winrt::get_abi(item.Object)), &handle);
                std::string diagnostic = SUCCEEDED(handleHr) && handle ? Quote(Hex(handle)) : "null";
                if (FAILED(handleHr))
                    properties.Read("diagnosticsHandle", [&]() -> bool { Fail(handleHr); });
                job.Add(job.Xaml, "{\"id\":" + std::to_string(item.Id) + ",\"rootId\":" +
                    std::to_string(rootId) + ",\"parentId\":" + Parent(item.ParentId) +
                    ",\"childIndex\":" + std::to_string(item.Index) + ",\"type\":" +
                    String(winrt::get_class_name(item.Object)) + ",\"name\":" + name +
                    ",\"objectAddress\":" + Quote(Hex(xamlIdentities.back().Address())) +
                    ",\"diagnosticsHandle\":" + diagnostic + ",\"properties\":" + properties.Finish() + "}");
                const int count = X::Media::VisualTreeHelper::GetChildrenCount(item.Object);
                if (count < 0 || static_cast<size_t>(count) > MaxEntries - pending.size())
                    Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                for (int index = count; index-- > 0;)
                {
                    job.Check();
                    auto child = X::Media::VisualTreeHelper::GetChild(item.Object, index);
                    if (!child) Fail(E_UNEXPECTED);
                    pending.push_back({ std::move(child), 0, item.Id, static_cast<unsigned>(index), item.Depth + 1 });
                }
            }
            if (xamlIds.empty()) Fail(HRESULT_FROM_WIN32(ERROR_NOT_FOUND));

            std::vector<C::Visual> seeds;
            std::set<uintptr_t> seedIds;
            for (const auto& pair : handouts)
            {
                job.Check();
                C::Visual top = pair.second;
                std::set<uintptr_t> ancestors;
                for (unsigned depth = 0;; ++depth)
                {
                    job.Check();
                    if (depth > MaxDepth || !ancestors.emplace(Identity(top).Address()).second)
                        Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                    auto parent = top.Parent();
                    if (!parent) break;
                    top = parent;
                }
                if (seedIds.emplace(Identity(top).Address()).second) seeds.push_back(std::move(top));
            }
            std::vector<VisualItem> visuals;
            for (unsigned i = 0; i < seeds.size(); ++i) visuals.push_back({ seeds[i], 0, i, 0 });
            std::unordered_map<uintptr_t, uint64_t> visualIds;
            std::vector<Identity> visualIdentities;
            std::vector<C::Visual> visualKeepAlive;
            while (!visuals.empty())
            {
                job.Check();
                auto item = std::move(visuals.back()); visuals.pop_back();
                if (item.Depth > MaxDepth) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                Identity identity(item.Object);
                if (visualIds.count(identity.Address())) continue;
                job.Entry(item.ParentId ? 2 : 1);
                const uint64_t id = job.VisualNext++;
                visualIds.emplace(identity.Address(), id);
                visualIdentities.push_back(std::move(identity));
                visualKeepAlive.push_back(item.Object);
                // IVisual's interface address is distinct from canonical COM identity.
                auto visualInterface = item.Object.as<C::IVisual>();
                Properties properties;
                properties.Read("offset", [&] { return item.Object.Offset(); });
                properties.Read("size", [&] { return item.Object.Size(); });
                properties.Read("transformMatrix", [&] { return item.Object.TransformMatrix(); });
                properties.Read("opacity", [&] { return item.Object.Opacity(); });
                properties.Read("isVisible", [&] { return item.Object.IsVisible(); });
                properties.Read("scale", [&] { return item.Object.Scale(); });
                properties.Read("centerPoint", [&] { return item.Object.CenterPoint(); });
                properties.Read("rotationAngle", [&] { return item.Object.RotationAngle(); });
                if (const auto sprite = item.Object.try_as<C::SpriteVisual>())
                {
                    properties.Read("brushType", [&]
                    {
                        auto brush = sprite.Brush();
                        return brush ? winrt::get_class_name(brush) : winrt::hstring(L"<null>");
                    });
                }
                job.Add(job.Composition, "{\"id\":" + std::to_string(id) + ",\"rootId\":" +
                    std::to_string(rootId) + ",\"parentId\":" + Parent(item.ParentId) +
                    ",\"childIndex\":" + std::to_string(item.Index) + ",\"type\":" +
                    String(winrt::get_class_name(item.Object)) + ",\"objectAddress\":" +
                    Quote(Hex(visualIdentities.back().Address())) + ",\"visualAddress\":" +
                    Quote(Hex(reinterpret_cast<uintptr_t>(winrt::get_abi(visualInterface)))) +
                    ",\"properties\":" + properties.Finish() + "}");
                if (const auto container = item.Object.try_as<C::ContainerVisual>())
                {
                    unsigned index{};
                    for (const auto& child : container.Children())
                    {
                        job.Check();
                        if (visuals.size() >= MaxEntries) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                        visuals.push_back({ child, id, index++, item.Depth + 1 });
                    }
                }
            }
            if (visualIds.empty()) Fail(HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
            for (const auto& pair : handouts)
            {
                job.Entry();
                const auto found = visualIds.find(Identity(pair.second).Address());
                if (found == visualIds.end()) Fail(E_UNEXPECTED);
                job.Add(job.Correlations, "{\"xamlNodeId\":" + std::to_string(pair.first) +
                    ",\"visualNodeId\":" + std::to_string(found->second) + "}");
            }
            job.Add(job.Roots, "{\"id\":" + std::to_string(rootId) + ",\"threadId\":" +
                std::to_string(GetCurrentThreadId()) + ",\"startQpc\":" + std::to_string(start) +
                ",\"endQpc\":" + std::to_string(Qpc()) + ",\"xamlRootAddress\":" +
                Quote(Hex(rootIdentity.Address())) + "}");
            // All strong tree references are released on this owning UI thread.
        }

        class Callback final : public Microsoft::WRL::RuntimeClass<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            IVisualTreeServiceCallback, Microsoft::WRL::FtmBase>
        {
        public:
            explicit Callback(std::shared_ptr<Job> job) : State(std::move(job)) {}
            HRESULT STDMETHODCALLTYPE OnVisualTreeChange(ParentChildRelation, VisualElement element,
                VisualMutationType mutation) noexcept override
            {
                auto job = State.lock();
                if (!job || !job->Accept || job->Abandoned || mutation != Add) return S_OK;
                // Native getters can pump messages; the outer walk owns nested discovery.
                static thread_local bool inCallback{};
                if (inCallback) return S_OK;
                struct ReentrancyGuard
                {
                    bool& Flag;
                    explicit ReentrancyGuard(bool& flag) : Flag(flag) { Flag = true; }
                    ~ReentrancyGuard() { Flag = false; }
                } reentrancy(inCallback);
                std::lock_guard<std::mutex> guard(job->Mutex);
                if (!job->Accept || FAILED(job->Status)) return S_OK;
                try
                {
                    job->Check();
                    Inspectable object{ nullptr };
                    const auto resolved = job->Diagnostics->GetIInspectableFromHandle(
                        element.Handle, reinterpret_cast<IInspectable**>(winrt::put_abi(object)));
                    if (FAILED(resolved) || !object)
                    {
                        ++job->UnresolvedHandles;
                        return S_OK;
                    }
                    if (const auto ui = object.try_as<X::UIElement>())
                    {
                        // Any realized element can discover a root whose lazy peer is absent.
                        if (auto root = ui.XamlRoot()) CaptureRoot(*job, root);
                    }
                }
                catch (...)
                {
                    job->Status = winrt::to_hresult();
                    job->Failure = "UI-thread realized-tree enumeration failed.";
                }
                return S_OK; // Capture errors must not fail the application's tree mutation.
            }
        private:
            std::weak_ptr<Job> State;
        };

        struct WorkerArgs { std::shared_ptr<Job> State; HMODULE Module{}; };
        DWORD WINAPI Worker(void* raw) noexcept
        {
            HMODULE module{};
            bool retainModule{};
            {
                std::unique_ptr<WorkerArgs> args(static_cast<WorkerArgs*>(raw));
                module = args->Module;
                auto job = args->State;
                const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
                ComPtr<IVisualTreeService> service;
                ComPtr<Callback> callback;
                bool advised{};
                HRESULT hr = initialized;
                try
                {
                    winrt::check_hresult(initialized);
                    job->Check();
                    hr = job->Diagnostics.As(&service);
                    if (FAILED(hr)) Fail(E_NOINTERFACE);
                    callback = Microsoft::WRL::Make<Callback>(job);
                    if (!callback) Fail(E_OUTOFMEMORY);
                    // Advise/Unadvise synchronously dispatch to UI threads, not the pipe thread.
                    hr = service->AdviseVisualTreeChange(callback.Get());
                    advised = SUCCEEDED(hr);
                    winrt::check_hresult(hr);
                }
                catch (...) { hr = winrt::to_hresult(); }
                job->Accept = false;
                if (callback && service)
                {
                    const HRESULT unadvise = service->UnadviseVisualTreeChange(callback.Get());
                    if (FAILED(unadvise))
                    {
                        // A retained COM sink can call this DLL's vtable after cleanup.
                        HMODULE ignored{};
                        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(&Worker), &ignored))
                            retainModule = true;
                        if (advised && SUCCEEDED(hr)) hr = unadvise;
                    }
                }
                {
                    std::lock_guard<std::mutex> guard(job->Mutex);
                    if (FAILED(hr) && SUCCEEDED(job->Status)) job->Status = hr;
                    if (SUCCEEDED(job->Status) && job->Roots.Json.empty())
                        job->Status = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                    if (FAILED(job->Status) && !job->Failure)
                        job->Failure = job->Status == E_NOINTERFACE ?
                            "Standard IVisualTreeService is unavailable (no private bridge fallback)." :
                            "Standard tree discovery/subscription failed or produced no realized roots.";
                    job->Diagnostics.Reset();
                }
                callback.Reset();
                service.Reset();
                if (SUCCEEDED(initialized)) CoUninitialize();
                SetEvent(job->Done.Get());
            }
            Running = false;
            --ActiveServers;
            if (retainModule) ExitThread(0);
            FreeLibraryAndExitThread(module, 0);
        }
    }

    HRESULT CaptureTreeSnapshot(IXamlDiagnostics* diagnostics, HANDLE stop, GUID captureId,
        uint64_t targetCreationFileTime, TreeSnapshotResult& result, std::string& failure) noexcept
    {
        result.Json.clear();
        failure.clear();
        bool ownsRunning{};
        HMODULE module{};
        try
        {
            if (!diagnostics) { failure = "IXamlDiagnostics is null."; return E_POINTER; }
            if (stop && WaitForSingleObject(stop, 0) == WAIT_OBJECT_0)
            { failure = "Tree snapshot cancelled."; return HRESULT_FROM_WIN32(ERROR_CANCELLED); }
            FILETIME creation{}, exit{}, kernel{}, user{};
            if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
                Fail(HRESULT_FROM_WIN32(GetLastError()));
            const uint64_t actualCreation = (static_cast<uint64_t>(creation.dwHighDateTime) << 32) |
                creation.dwLowDateTime;
            if (!targetCreationFileTime || targetCreationFileTime != actualCreation)
            { failure = "Target process creation time does not match this injected process."; return E_INVALIDARG; }
            if (Running.exchange(true))
            { failure = "A previous tree subscription is still running or awaiting UI dispatch."; return HRESULT_FROM_WIN32(ERROR_BUSY); }
            ownsRunning = true;
            LARGE_INTEGER frequency{};
            if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) Fail(E_UNEXPECTED);
            const int64_t start = Qpc();
            const int64_t resourceStart = Qpc();
            ResourceSnapshotAbi::Snapshot resources{};
            const HRESULT resourceStatus = ResourceSnapshotAbi::Read(diagnostics, resources);
            const int64_t resourceEnd = Qpc();
            auto job = std::make_shared<Job>();
            if (!job->Done.Get()) Fail(HRESULT_FROM_WIN32(GetLastError()));
            job->Diagnostics = diagnostics;
            if (stop)
            {
                HANDLE duplicate{};
                if (!DuplicateHandle(GetCurrentProcess(), stop, GetCurrentProcess(), &duplicate,
                    SYNCHRONIZE, FALSE, 0)) Fail(HRESULT_FROM_WIN32(GetLastError()));
                job->Stop.Reset(duplicate);
            }
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                reinterpret_cast<LPCWSTR>(&Worker), &module)) Fail(HRESULT_FROM_WIN32(GetLastError()));
            auto args = std::make_unique<WorkerArgs>();
            args->State = job; args->Module = module;
            ++ActiveServers;
            Handle worker(CreateThread(nullptr, 0, Worker, args.get(), 0, nullptr));
            if (!worker.Get()) { --ActiveServers; Fail(HRESULT_FROM_WIN32(GetLastError())); }
            args.release();
            ownsRunning = false;
            module = nullptr; // The worker retains the module/job through asynchronous cleanup.
            HANDLE events[]{ job->Done.Get(), job->Stop.Get() };
            const auto now = GetTickCount64();
            const DWORD waited = WaitForMultipleObjects(stop ? 2 : 1, events, FALSE,
                now >= job->Deadline ? 0 : static_cast<DWORD>(job->Deadline - now));
            if (waited != WAIT_OBJECT_0)
            {
                job->Abandoned = true;
                failure = waited == WAIT_OBJECT_0 + 1 ? "Tree snapshot cancelled." :
                    "Tree snapshot exceeded its 20 second deadline (UI dispatch may still be completing).";
                return HRESULT_FROM_WIN32(waited == WAIT_OBJECT_0 + 1 ? ERROR_CANCELLED : ERROR_TIMEOUT);
            }
            // Done is published only after Unadvise and completion of callbacks.
            std::lock_guard<std::mutex> guard(job->Mutex);
            if (FAILED(job->Status))
            {
                failure = std::string(job->Failure ? job->Failure : "Tree snapshot failed.") +
                    " HRESULT=" + Hex(static_cast<uint32_t>(job->Status));
                return job->Status;
            }
            job->Check();
            const auto guid = SnapshotJson::CaptureIdText(captureId);
            if (guid.empty()) Fail(E_UNEXPECTED);
            result.Json = "{\"schemaVersion\":3,\"captureId\":" + String(winrt::hstring(guid)) +
                ",\"processId\":" + std::to_string(GetCurrentProcessId()) +
                ",\"creationTimeFileTime\":" + std::to_string(actualCreation) +
                ",\"qpcFrequency\":" + std::to_string(frequency.QuadPart) +
                ",\"startQpc\":" + std::to_string(start) + ",\"endQpc\":" + std::to_string(Qpc()) +
                ",\"resourceSnapshot\":" + ResourceJson(resourceStatus, resources, resourceStart, resourceEnd) +
                ",\"roots\":[" + job->Roots.Json + "],\"xamlNodes\":[" + job->Xaml.Json +
                "],\"compositionNodes\":[" + job->Composition.Json + "],\"correlations\":[" +
                job->Correlations.Json + "],\"limitations\":["
                "\"Public realized visual-tree scope only; unrealized and nonvisual logical objects are excluded.\","
                "\"Each root is sampled on its owning UI thread over its recorded interval; roots are not globally atomic.\","
                "\"XAML resources are sampled before tree enumeration, not atomically with the trees. Allocator counters cover enabled profiling only; transition-spanning allocations/frees can be unbalanced. AllocatedMemory is the independent GC heuristic, not process heap size. All-zero graphics resource slots are not measurements.\","
                "\"Open popup children are separate visual subtrees, without fabricated logical parent links.\","
                "\"GetElementVisual can materialize public handout visuals; no adorners or visual mutations are requested.\","
                "\"Composition includes all descendants exposed by ContainerVisual.Children and public ancestors of handout visuals, not private compositor trees.\","
                "\"No core/HWCompNode or lifetime-ID mapping is exposed; addresses are debugger metadata only.\","
                "\"Property errors and unsupported values are recorded explicitly inside properties.\","
                "\"XAML properties are allow-listed SDK getters, not every dependency or app-defined property; TextBlock text over 4096 UTF-16 code units is recorded as an error.\","
                "\"XAML boundsInParent includes transforms but not clipping; parentless nodes use their visual root. TransformGroup captures its combined matrix.\","
                "\"Brush capture includes type and opacity, plus ARGB color for SolidColorBrush; other brush content is not captured.\","
                "\"Unresolvable diagnostic Add handles during discovery: " +
                std::to_string(job->UnresolvedHandles) + ".\"]}";
            if (result.Json.size() > MaxBytes) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
            job->Check();
            return S_OK;
        }
        catch (...)
        {
            const auto hr = winrt::to_hresult();
            if (module) FreeLibrary(module);
            if (ownsRunning) Running = false;
            result.Json.clear();
            try { failure = "Tree snapshot failed: " + Hex(static_cast<uint32_t>(hr)); }
            catch (...) { failure.clear(); }
            return hr;
        }
    }
}
