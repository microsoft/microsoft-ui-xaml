#include "TreeSnapshot.h"
#include "TreeSnapshotJson.h"
#include "Tap.h"
#include <xamlom.h>
#include <wrl.h>
// The SDK C++/WinRT base header includes the C++17 coroutine shim even though
// this collector uses no coroutines. Keep C++17 compatible with newer MSVC STLs.
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
                    // Limits/allocation/cancellation must not be disguised as missing properties.
                    if (e.code() == E_OUTOFMEMORY ||
                        e.code() == HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW)) throw;
                    Json += "{\"error\":" + Quote(Hex(static_cast<uint32_t>(e.code()))) + "}";
                }
                if (Json.size() + 1 > MaxPropertiesBytes) Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
            }
            std::string Finish() { return Json + "}"; }
        };
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
            // Popup.Child is an exposed, disconnected visual subtree. Do not invent a
            // visual-parent relationship from Popup's logical ownership.
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
                    const auto framework = element.try_as<X::FrameworkElement>();
                    if (framework)
                    {
                        // Known native SDK getters only; never enumerate arbitrary DPs,
                        // bindings, custom properties, or app-provided property getters.
                        properties.Read("name", [&] { auto n = framework.Name(); name = String(n); return n; });
                        properties.Read("actualWidth", [&] { return framework.ActualWidth(); });
                        properties.Read("actualHeight", [&] { return framework.ActualHeight(); });
                        properties.Read("width", [&] { return framework.Width(); });
                        properties.Read("height", [&] { return framework.Height(); });
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
                // Explicit IVisual QI address is debugger metadata, distinct from identity.
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
            // Every strong XAML/visual/identity reference above dies on the owning UI
            // thread, before returning from this callback. Nothing is cached globally.
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
                // Native getters can pump messages. Ignore nested discovery on this
                // thread; the outer root walk is already capturing its exposed tree.
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
                        return S_OK; // Lazy diagnostic peers are not reliable root sentinels.
                    }
                    if (const auto ui = object.try_as<X::UIElement>())
                    {
                        // Discover a root from ANY realized element, not only parentless
                        // diagnostic callbacks (the lazy root peer may not exist).
                        if (auto root = ui.XamlRoot()) CaptureRoot(*job, root);
                    }
                }
                catch (...)
                {
                    job->Status = winrt::to_hresult();
                    job->Failure = "UI-thread realized-tree enumeration failed.";
                }
                return S_OK; // Never propagate capture failure into the application's tree mutation.
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
                    // Both operations may synchronously dispatch to UI threads. They
                    // MUST stay off the UI/pipe threads and keep the module/job alive.
                    hr = service->AdviseVisualTreeChange(callback.Get());
                    advised = SUCCEEDED(hr);
                    winrt::check_hresult(hr);
                }
                catch (...) { hr = winrt::to_hresult(); }
                job->Accept = false;
                if (callback && service)
                {
                    // Attempt cleanup even after a failed Advise, which can have
                    // partially registered the sink.
                    const HRESULT unadvise = service->UnadviseVisualTreeChange(callback.Get());
                    if (FAILED(unadvise))
                    {
                        // A runtime retaining the COM sink can call its vtable later.
                        // An unrecoverable subscription failure must therefore pin the
                        // DLL permanently. The inert sink retains no tree/diagnostics.
                        HMODULE ignored{};
                        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(&Worker), &ignored))
                            retainModule = true; // Retain our existing loader reference if pinning fails.
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
            LARGE_INTEGER frequency{};
            if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) Fail(E_UNEXPECTED);
            const int64_t start = Qpc();
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                reinterpret_cast<LPCWSTR>(&Worker), &module)) Fail(HRESULT_FROM_WIN32(GetLastError()));
            auto args = std::make_unique<WorkerArgs>();
            args->State = job; args->Module = module;
            ++ActiveServers;
            Handle worker(CreateThread(nullptr, 0, Worker, args.get(), 0, nullptr));
            if (!worker.Get()) { --ActiveServers; Fail(HRESULT_FROM_WIN32(GetLastError())); }
            args.release();
            ownsRunning = false;
            module = nullptr; // worker owns pin and Running until all cleanup completes
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
            // Done is published only after Unadvise and completion of all callbacks.
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
            result.Json = "{\"schemaVersion\":1,\"captureId\":" + String(winrt::hstring(guid)) +
                ",\"processId\":" + std::to_string(GetCurrentProcessId()) +
                ",\"creationTimeFileTime\":" + std::to_string(actualCreation) +
                ",\"qpcFrequency\":" + std::to_string(frequency.QuadPart) +
                ",\"startQpc\":" + std::to_string(start) + ",\"endQpc\":" + std::to_string(Qpc()) +
                ",\"roots\":[" + job->Roots.Json + "],\"xamlNodes\":[" + job->Xaml.Json +
                "],\"compositionNodes\":[" + job->Composition.Json + "],\"correlations\":[" +
                job->Correlations.Json + "],\"limitations\":["
                "\"Public realized visual-tree scope only; unrealized and nonvisual logical objects are excluded.\","
                "\"Each root is sampled on its owning UI thread over its recorded interval; roots are not globally atomic.\","
                "\"Open popup children are separate visual subtrees, without fabricated logical parent links.\","
                "\"GetElementVisual can materialize public handout visuals; no adorners or visual mutations are requested.\","
                "\"Composition includes all descendants exposed by ContainerVisual.Children and public ancestors of handout visuals, not private compositor trees.\","
                "\"No core/HWCompNode or lifetime-ID mapping is exposed; addresses are debugger metadata only.\","
                "\"Property errors and unsupported values are recorded explicitly inside properties.\","
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
