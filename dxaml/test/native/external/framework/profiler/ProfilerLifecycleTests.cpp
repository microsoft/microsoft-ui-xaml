// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#if !defined(MIDL_NS_PREFIX)
#define MIDL_NS_PREFIX
#endif
#include <windows.h>
#include <roapi.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.foundation.h>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <thread>
#include <exception>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>
#include <memory>
#include <future>
#include <list>
#include "ProfilerLifecycleEtw.h"
#undef GetCurrentTime
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Microsoft.UI.Xaml.Media.Animation.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <IXamlTestHooks-win.h>

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Wrappers::HStringReference;

namespace
{
    void Check(HRESULT result, const char* operation)
    {
        std::printf("%s: 0x%08lx\n", operation, static_cast<unsigned long>(result));
        std::fflush(stdout);
        if (FAILED(result)) throw std::runtime_error(operation);
    }
    template<typename T>
    ComPtr<T> Factory(const wchar_t* name)
    {
        ComPtr<T> factory;
        Check(RoGetActivationFactory(HStringReference(name).Get(), IID_PPV_ARGS(&factory)), "RoGetActivationFactory");
        return factory;
    }
    void Pump(DWORD milliseconds)
    {
        const ULONGLONG end = GetTickCount64() + milliseconds;
        do
        {
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            Sleep(1);
        } while (GetTickCount64() < end);
    }
    void AllocatorTransitionsTest()
    {
        using namespace winrt::Microsoft::UI::Xaml::Controls;
        LifecycleEtw::Session session;
        Grid preEnable;
        auto churn = []
        {
            Grid parent;
            for (unsigned i = 0; i != 64; ++i)
            {
                Border child;
                child.Width(10 + i);
                parent.Children().Append(child);
            }
            parent.Children().Clear();
        };
        auto timestamp = [] { LARGE_INTEGER now{}; QueryPerformanceCounter(&now); return now.QuadPart; };
        const int64_t firstStart = timestamp();
        session.Enable(65);
        preEnable = nullptr;
        churn();
        session.Disable();
        const int64_t gapStart = timestamp();
        churn();
        const int64_t gapEnd = timestamp();
        session.Enable(65);
        churn();
        const auto observed = session.Read();
        LifecycleEtw::Require(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost,
            "actual allocator qualification requires zero ETW transport loss");
        LifecycleEtw::Require(observed.Counts[15] > 0,
            "late-enable unmatched retirements explicitly diagnose incomplete allocator coverage");
        for (const auto& loss : observed.Losses)
        {
            LifecycleEtw::Require(!loss.HasCount && loss.Begin >= firstStart &&
                loss.Begin < loss.End && loss.End <= loss.Timestamp,
                "actual allocator coverage loss has unknown count and a real enclosing interval");
        }
        struct Phase
        {
            GUID Epoch{}, Producer{};
            uint64_t Sequence = 0, Births = 0, Retirements = 0;
            std::map<uint64_t, std::pair<uint64_t, uint64_t>> Live;
        } phases[2];
        std::set<uint64_t> identities;
        for (const auto& allocation : observed.Allocations)
        {
            LifecycleEtw::Require(allocation.Timestamp >= firstStart &&
                (allocation.Timestamp <= gapStart || allocation.Timestamp >= gapEnd),
                "no Allocation event during the actual disabled interval");
            auto& phase = phases[allocation.Timestamp >= gapEnd ? 1 : 0];
            if (!phase.Sequence) { phase.Epoch = allocation.Epoch; phase.Producer = allocation.Producer; }
            LifecycleEtw::Require(allocation.Epoch != GUID_NULL && allocation.Epoch != allocation.Producer &&
                allocation.Epoch == phase.Epoch && allocation.Producer == phase.Producer,
                "stable allocator epoch distinct from producer within each enable interval");
            LifecycleEtw::Require(allocation.Sequence == ++phase.Sequence,
                "actual allocator wire sequence is contiguous within each producer");
            LifecycleEtw::Require(allocation.Provenance == 0 && allocation.Coverage == 1 && !allocation.HasBaseline,
                "actual transitions are measured partial, never a fabricated complete baseline");
            LifecycleEtw::Require(!allocation.HasActual || allocation.Actual >= allocation.Requested,
                "actual heap size cannot be smaller than requested bytes");
            if (allocation.Operation == 0 || allocation.Operation == 2)
            {
                LifecycleEtw::Require(identities.insert(allocation.Id).second,
                    "allocator lifetime IDs are never reused across enable gaps");
                if (allocation.Operation == 2)
                {
                    LifecycleEtw::Require(allocation.HasPrevious && allocation.Previous != allocation.Id &&
                        phase.Live.erase(allocation.Previous) == 1, "reallocation retires its observed predecessor");
                }
                else LifecycleEtw::Require(!allocation.HasPrevious, "allocation has no invented predecessor");
                phase.Live.emplace(allocation.Id, std::make_pair(allocation.Requested, allocation.Actual));
                ++phase.Births;
            }
            else
            {
                LifecycleEtw::Require(allocation.Operation == 1 && !allocation.HasPrevious,
                    "only successful allocate/reallocate/free transitions occur");
                const auto found = phase.Live.find(allocation.Id);
                LifecycleEtw::Require(found != phase.Live.end() && found->second.first == allocation.Requested &&
                    found->second.second == allocation.Actual, "free preserves its tracked requested and actual bytes");
                phase.Live.erase(found);
                ++phase.Retirements;
            }
        }
        LifecycleEtw::Require(phases[0].Births && phases[0].Retirements && phases[1].Births && phases[1].Retirements,
            "actual XAML churn produces allocator births and retirements on both sides of a selection gap");
        LifecycleEtw::Require(phases[0].Epoch != phases[1].Epoch && phases[0].Producer != phases[1].Producer,
            "full provider disable/re-enable renews both allocator and producer epochs");
        std::printf("ALLOCATOR_TRANSITIONS: firstBirths=%llu firstRetirements=%llu secondBirths=%llu secondRetirements=%llu\n",
            phases[0].Births, phases[0].Retirements, phases[1].Births, phases[1].Retirements);
        std::puts("PASS: actual allocator wire lifetimes, size semantics and disable/re-enable gap");
    }
    void GraphicsResourcesTest(const winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager& manager, bool resetDevice)
    {
        using namespace winrt::Microsoft::UI::Xaml;
        LifecycleEtw::Session session;
        session.Enable(35);
        int64_t colorChangeBegin = 0, colorChangeEnd = 0;
        {
            Window window;
            Controls::Grid content;
            Media::SolidColorBrush fill(winrt::Windows::UI::Color{255, 32, 64, 192});
            content.Background(fill);
            Controls::TextBlock text;
            text.Text(L"Graphics resource lifetime qualification");
            text.FontSize(42);
            content.Children().Append(text);
            Controls::Image image;
            image.Source(Media::Imaging::WriteableBitmap(128, 64));
            image.Width(256);
            image.Height(128);
            content.Children().Append(image);
            window.Content(content);
            window.Activate();
            Pump(400);
            LARGE_INTEGER now{};
            QueryPerformanceCounter(&now);
            colorChangeBegin = now.QuadPart;
            fill.Color(winrt::Windows::UI::Color{255, 64, 192, 32});
            content.UpdateLayout();
            Pump(200);
            QueryPerformanceCounter(&now);
            colorChangeEnd = now.QuadPart;
            image.Source(Media::Imaging::WriteableBitmap(256, 128));
            text.Text(L"Different glyphs and resized bitmap resource");
            content.UpdateLayout();
            Pump(400);
            if (resetDevice)
            {
                auto factory = Factory<ABI::Microsoft::UI::Xaml::IDxamlCoreTestHooksStatics>(
                    RuntimeClass_Microsoft_UI_Xaml_DxamlCoreTestHooks);
                ComPtr<ABI::Microsoft::UI::Xaml::IDxamlCoreTestHooks> coreHooks;
                Check(factory->GetForCurrentThread(&coreHooks), "get graphics reset test hooks");
                ComPtr<IXamlTestHooks> hooks;
                Check(coreHooks.As(&hooks), "query graphics reset interface");
                Check(hooks->MarkDeviceInstanceLost(), "mark only this process graphics device lost");
                Check(hooks->ResetDeviceOnly(), "request controlled graphics device recovery");
                text.Text(L"Recovered graphics device and fresh bitmap upload");
                image.Source(Media::Imaging::WriteableBitmap(384, 192));
                content.UpdateLayout();
                Pump(800);
            }
            window.Content(nullptr);
            content.Children().Clear();
            image.Source(nullptr);
            window.Close();
        }
        Pump(100);
        manager.Close();
        Pump(100);
        const auto observed = session.Read();
        LifecycleEtw::Require(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost,
            "actual graphics qualification requires zero ETW transport loss");
        LifecycleEtw::Require(!observed.Resources.empty() && !observed.Devices.empty(),
            "actual XAML text/bitmap rendering emits resource and device observations");
        std::map<uint64_t, uint8_t> live;
        std::map<uint64_t, std::pair<GUID, uint64_t>> brushOwners;
        size_t created = 0, released = 0, destroyed = 0, texturesCreated = 0, texturesDestroyed = 0;
        size_t brushesCreated = 0, brushesDestroyed = 0;
        for (const auto& resource : observed.Resources)
        {
            LifecycleEtw::Require(resource.Epoch == resource.Producer && resource.Id &&
                resource.Producer == observed.Resources.front().Producer &&
                (resource.Kind == 1 || resource.Kind == 2 || resource.Kind == 3) &&
                resource.Coverage == 1 && !resource.HasBacking && !resource.HasBytes,
                "logical surface observations never invent physical allocations, bytes or complete coverage");
            if (resource.Kind == 3)
            {
                LifecycleEtw::Require(resource.HasOwner && resource.OwnerDomain == 0 &&
                    resource.OwnerEpoch == resource.Epoch &&
                    std::any_of(observed.LifecycleRecords.begin(), observed.LifecycleRecords.end(),
                        [&](const auto& object) { return object.Domain == 0 && object.Epoch == resource.OwnerEpoch &&
                            object.Object == resource.OwnerId; }),
                    "composition-brush holder owner joins an actual qualified XAML Core identity");
                const auto owner = std::make_pair(resource.OwnerEpoch, resource.OwnerId);
                const auto previous = brushOwners.emplace(resource.Id, owner);
                LifecycleEtw::Require(previous.second || previous.first->second == owner,
                    "composition-brush holder retains its XAML owner through reuse and actual holder destruction");
            }
            if (resource.Kind == 3 && resource.Timestamp >= colorChangeBegin && resource.Timestamp < colorChangeEnd)
                LifecycleEtw::Require(resource.Operation != 0 && resource.Operation != 4,
                    "changing only brush color reuses its composition-brush ownership slot without creation/rebind");
            const auto key = resource.Id;
            if (resource.Operation == 0 || resource.Operation == 7)
            {
                LifecycleEtw::Require(live.emplace(key, resource.Kind).second, "logical resource has one observed lifetime");
                if (resource.Operation == 0)
                {
                    ++created;
                    if (resource.Kind == 1) ++texturesCreated;
                    if (resource.Kind == 3) ++brushesCreated;
                }
            }
            else
            {
                LifecycleEtw::Require(live.count(key) == 1 && live.at(key) == resource.Kind,
                    "resource mutation/release follows observed presence with stable classification");
                if (resource.Operation == 2) ++released;
                if (resource.Operation == 1)
                {
                    ++destroyed;
                    if (resource.Kind == 1) ++texturesDestroyed;
                    if (resource.Kind == 3) ++brushesDestroyed;
                    live.erase(key);
                }
            }
        }
        LifecycleEtw::Require(created && released && destroyed,
            "real graphics workload observes creation, ownership release and logical wrapper destruction");
        LifecycleEtw::Require(texturesCreated != 0,
            "actual bitmap uploads observe newly created driver-visible staging-texture owners");
        LifecycleEtw::Require(brushesCreated != 0 && brushesDestroyed != 0,
            "actual solid-color rendering observes composition-brush ownership-slot creation and destruction");
        for (const auto& device : observed.Devices)
            LifecycleEtw::Require(device.Operation != 2 && device.Coverage == 1,
                "WinUI device release is not fabricated final physical device destruction");
        if (resetDevice)
        {
            const auto lost = std::find_if(observed.Devices.begin(), observed.Devices.end(),
                [](const auto& device) { return device.Operation == 1; });
            LifecycleEtw::Require(lost != observed.Devices.end(), "controlled reset observes actual WinUI device-loss cleanup");
            const auto recovered = std::find_if(observed.Devices.begin(), observed.Devices.end(),
                [&](const auto& device) { return device.Operation == 0 && device.Epoch != lost->Epoch && device.Sequence > lost->Sequence; });
            LifecycleEtw::Require(recovered != observed.Devices.end(), "controlled reset observes successful replacement device creation after loss");
            LifecycleEtw::Require(std::any_of(observed.Resources.begin(), observed.Resources.end(),
                [&](const auto& resource) { return resource.Kind == 1 && resource.Operation == 1 && resource.DeviceEpoch == lost->Epoch; }),
                "controlled reset destroys staging-texture owners of the retired device");
            LifecycleEtw::Require(std::any_of(observed.Resources.begin(), observed.Resources.end(),
                [&](const auto& resource) { return resource.Kind == 1 && resource.Operation == 0 && resource.DeviceEpoch == recovered->Epoch; }),
                "controlled reset creates staging-texture owners under a new device epoch");
            std::puts("PASS: controlled WinUI device reset retires staging textures and recovers with new device identity");
        }
        std::printf("PASS: actual graphics logical resources created=%zu released=%zu destroyed=%zu texturesCreated=%zu texturesDestroyed=%zu brushesCreated=%zu brushesDestroyed=%zu\n",
            created, released, destroyed, texturesCreated, texturesDestroyed, brushesCreated, brushesDestroyed);
    }
    void ManagerRestartTest(bool verifyDeployment)
    {
        using namespace winrt::Microsoft::UI;
        LifecycleEtw::Session session;
        session.Enable(3);
        int64_t shutdownQpc[2]{};
        for (unsigned round = 0; round != 2; ++round)
        {
            std::exception_ptr failure;
            std::thread ui([&]
            {
                try
                {
                    winrt::init_apartment(winrt::apartment_type::single_threaded);
                    {
                        auto dispatcher = Dispatching::DispatcherQueueController::CreateOnCurrentThread();
                        auto manager = Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
                        bool xamlShutdown = false, queueShutdown = false;
                        manager.XamlShutdownCompletedOnThread([&](auto const&, auto const&) { xamlShutdown = true; });
                        dispatcher.DispatcherQueue().ShutdownCompleted([&](auto const&, auto const&) { queueShutdown = true; });
                        if (round == 0)
                        {
                            wchar_t path[32768]{};
                            GetModuleFileNameW(GetModuleHandleW(L"Microsoft.UI.Xaml.dll"), path, ARRAYSIZE(path));
                            std::wprintf(L"LOADED_LOCAL_PRODUCT: %s\n", path);
                            std::puts("HOST_READY"); std::fflush(stdout);
                            if (verifyDeployment) Sleep(10000);
                        }
                        {
                            Xaml::Controls::Grid probe;
                            probe.Width(901.0 + round);
                        }
                        manager.Close();
                        dispatcher.ShutdownQueue();
                        LifecycleEtw::Require(xamlShutdown && queueShutdown,
                            "real XamlShutdownCompletedOnThread and dispatcher ShutdownCompleted both fire");
                        LifecycleEtw::Require(Xaml::Application::Current() == nullptr,
                            "last real core shutdown clears Application.Current before restart");
                        LARGE_INTEGER now{};
                        QueryPerformanceCounter(&now);
                        shutdownQpc[round] = now.QuadPart;
                        std::printf("MANAGER_RESTART: round=%u thread=%lu XamlShutdown=1 QueueShutdown=1 ApplicationNull=1\n",
                            round, GetCurrentThreadId());
                    }
                    winrt::uninit_apartment();
                }
                catch (...) { failure = std::current_exception(); }
            });
            ui.join();
            if (failure) std::rethrow_exception(failure);
        }
        const auto observed = session.Read();
        std::pair<uint64_t, GUID> first{}, second{};
        for (size_t i = 0; i != observed.PropertyValues.size(); ++i)
        {
            const auto& property = observed.PropertyValues[i];
            if (property.Number == 901) first = observed.PropertyObjects[i];
            if (property.Number == 902) second = observed.PropertyObjects[i];
        }
        LifecycleEtw::Require(first.first && second.first && first.first != second.first && first.second == second.second,
            "full core restart preserves continuous producer epoch without reusing qualified object IDs");
        LifecycleEtw::Require(observed.Destroyed.count(first.first) && observed.Destroyed.count(second.first),
            "both real probe lifetimes close across full dispatcher/core shutdowns");
        LifecycleEtw::Require(observed.Domains.at(first.first) == 0 && observed.Domains.at(second.first) == 0,
            "restart compares actual XamlCore lifetime domain, not an unqualified address");
        unsigned births = 0, deaths = 0;
        for (const auto& record : observed.LifecycleRecords)
        {
            if (record.Event != 2 || (record.Object != first.first && record.Object != second.first)) continue;
            LifecycleEtw::Require(record.Epoch == first.second, "probe lifetime records retain qualified continuous epoch");
            const unsigned round = record.Object == first.first ? 0 : 1;
            LifecycleEtw::Require(record.Timestamp <= shutdownQpc[round] && (!round || record.Timestamp > shutdownQpc[0]),
                "probe births/destruction belong to the correct fully separated core lifecycle");
            if (record.Operation == 0) ++births;
            if (record.Operation == 1) ++deaths;
        }
        LifecycleEtw::Require(births == 2 && deaths == 2, "exactly two measured probe births and destructions across core restart");
        std::printf("MANAGER_RESTART_IDS: domain=0 first=%llu second=%llu sameEpoch=1 births=%u deaths=%u\n",
            static_cast<unsigned long long>(first.first), static_cast<unsigned long long>(second.first), births, deaths);
        std::puts("PASS: actual full manager/core shutdown and restart under continuous listener");
    }
    class IndependentManager
    {
        HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE command = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        HANDLE done = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        HANDLE stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        std::atomic<int> value{0};
        std::mutex failureLock;
        std::exception_ptr failure;
    public:
        int64_t AttachBeginQpc = 0;
    private:
        std::thread ui;
        void CheckFailure()
        {
            std::lock_guard<std::mutex> lock(failureLock);
            if (failure) std::rethrow_exception(failure);
        }
    public:
        int64_t ShutdownQpc = 0;
        explicit IndependentManager(int initial) : value(initial), ui([this]
        {
            try
            {
                using namespace winrt::Microsoft::UI;
                winrt::init_apartment(winrt::apartment_type::single_threaded);
                {
                    auto dispatcher = Dispatching::DispatcherQueueController::CreateOnCurrentThread();
                    auto manager = Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
                    bool xamlShutdown = false, queueShutdown = false;
                    manager.XamlShutdownCompletedOnThread([&](auto const&, auto const&) { xamlShutdown = true; });
                    dispatcher.DispatcherQueue().ShutdownCompleted([&](auto const&, auto const&) { queueShutdown = true; });
                    {
                        Xaml::Window window;
                        Xaml::Controls::Grid content;
                        Xaml::Shapes::Rectangle probe;
                        content.Children().Append(probe);
                        LARGE_INTEGER attach{};
                        QueryPerformanceCounter(&attach);
                        AttachBeginQpc = attach.QuadPart;
                        std::printf("ACTOR_PHASE: tag=%d attachBegin=%lld\n", value.load(), static_cast<long long>(AttachBeginQpc));
                        window.Content(content);
                        window.Activate();
                        Pump(100);
                        LifecycleEtw::Require(probe.XamlRoot() == content.XamlRoot(), "probe and known parent share actual public XamlRoot");
                        probe.Width(value.load());
                        content.UpdateLayout();
                        SetEvent(ready);
                        while (WaitForSingleObject(stop, 0) != WAIT_OBJECT_0)
                        {
                            if (WaitForSingleObject(command, 0) == WAIT_OBJECT_0)
                            {
                                probe.Width(value.load());
                                content.UpdateLayout();
                                Pump(25);
                                SetEvent(done);
                            }
                            Pump(1);
                        }
                        LARGE_INTEGER phase{};
                        QueryPerformanceCounter(&phase);
                        std::printf("ACTOR_PHASE: tag=%d detachBegin=%lld\n", value.load(), static_cast<long long>(phase.QuadPart));
                        window.Content(nullptr);
                        QueryPerformanceCounter(&phase);
                        std::printf("ACTOR_PHASE: tag=%d contentDetached=%lld\n", value.load(), static_cast<long long>(phase.QuadPart));
                        window.Close();
                        QueryPerformanceCounter(&phase);
                        std::printf("ACTOR_PHASE: tag=%d windowClosed=%lld\n", value.load(), static_cast<long long>(phase.QuadPart));
                    }
                    manager.Close();
                    dispatcher.ShutdownQueue();
                    LifecycleEtw::Require(xamlShutdown && queueShutdown,
                        "independent manager completes real XAML and dispatcher shutdown");
                    LARGE_INTEGER now{};
                    QueryPerformanceCounter(&now);
                    ShutdownQpc = now.QuadPart;
                    std::printf("SIMULTANEOUS_SHUTDOWN: thread=%lu XamlShutdown=1 QueueShutdown=1\n", GetCurrentThreadId());
                }
                winrt::uninit_apartment();
            }
            catch (...)
            {
                std::lock_guard<std::mutex> lock(failureLock);
                failure = std::current_exception();
            }
            SetEvent(ready);
            SetEvent(done);
        }) {}
        ~IndependentManager()
        {
            SetEvent(stop);
            if (ui.joinable()) ui.join();
            CloseHandle(ready); CloseHandle(command); CloseHandle(done); CloseHandle(stop);
        }
        void Ready()
        {
            LifecycleEtw::Require(WaitForSingleObject(ready, 10000) == WAIT_OBJECT_0, "independent manager becomes ready");
            CheckFailure();
        }
        void SetWidth(int width)
        {
            value = width;
            SetEvent(command);
            LifecycleEtw::Require(WaitForSingleObject(done, 10000) == WAIT_OBJECT_0, "independent UI-thread mutation completes");
            CheckFailure();
        }
        void Close()
        {
            SetEvent(stop);
            if (ui.joinable()) ui.join();
            CheckFailure();
        }
    };
    void SimultaneousManagersTest(bool verifyDeployment)
    {
        LifecycleEtw::Session session(true);
        session.Enable(31);
        IndependentManager first(1001);
        first.Ready();
        IndependentManager survivor(1101);
        survivor.Ready();
        wchar_t path[32768]{};
        GetModuleFileNameW(GetModuleHandleW(L"Microsoft.UI.Xaml.dll"), path, ARRAYSIZE(path));
        std::wprintf(L"LOADED_LOCAL_PRODUCT: %s\n", path);
        std::puts("HOST_READY"); std::fflush(stdout);
        if (verifyDeployment) Sleep(10000);
        survivor.SetWidth(1102);
        first.Close();
        const auto firstShutdown = first.ShutdownQpc;
        survivor.SetWidth(1103);
        IndependentManager restarted(1201);
        restarted.Ready();
        survivor.SetWidth(1104);
        restarted.Close();
        survivor.SetWidth(1105);
        survivor.Close();
        const auto observed = session.Read();
        std::map<int, std::pair<uint64_t, GUID>> tagged;
        for (size_t i = 0; i != observed.PropertyValues.size(); ++i)
        {
            const int width = static_cast<int>(observed.PropertyValues[i].Number);
            if (width == 1001 || width == 1201 || (width >= 1101 && width <= 1105))
                tagged[width] = observed.PropertyObjects[i];
        }
        LifecycleEtw::Require(tagged.size() == 7, "all actual independent-manager phases observed");
        const auto stable = tagged.at(1101);
        for (int width = 1102; width <= 1105; ++width)
            LifecycleEtw::Require(tagged.at(width) == stable, "other core shutdown/restart never retires survivor identity");
        LifecycleEtw::Require(tagged.at(1001).first != stable.first && tagged.at(1201).first != stable.first &&
            tagged.at(1001).first != tagged.at(1201).first && tagged.at(1001).second == stable.second &&
            tagged.at(1201).second == stable.second, "independent/restarted lifetimes stay distinct within continuous epoch");
        for (const auto& edge : observed.ScopedEdges)
        {
            if (edge.Child != stable.first && edge.Child != tagged.at(1001).first && edge.Child != tagged.at(1201).first) continue;
            wchar_t epochText[40]{}, rootText[40]{};
            StringFromGUID2(edge.Epoch, epochText, ARRAYSIZE(epochText));
            StringFromGUID2(edge.Root, rootText, ARRAYSIZE(rootText));
            const auto nativeChild = observed.BirthAddresses.find(edge.Child);
            const auto nativeParent = observed.BirthAddresses.find(edge.Parent);
            std::printf("SCOPE_NATIVE: child=%llu address=%llx parent=%llu address=%llx\n",
                edge.Child, nativeChild == observed.BirthAddresses.end() ? 0ULL : nativeChild->second,
                edge.Parent, nativeParent == observed.BirthAddresses.end() ? 0ULL : nativeParent->second);
            std::printf("SCOPE_EDGE: child=%llu domain=%u parent=%llu member=%u rel=%u connected=%d dispatcher=%llu seq=%llu thread=%lu qpc=%lld\n",
                static_cast<unsigned long long>(edge.Child), edge.Domain, static_cast<unsigned long long>(edge.Parent),
                edge.Membership, edge.Relationship, edge.Connected, static_cast<unsigned long long>(edge.Dispatcher),
                static_cast<unsigned long long>(edge.Sequence), static_cast<unsigned long>(edge.Thread), static_cast<long long>(edge.Timestamp));
            std::wprintf(L"SCOPE_GUID: epoch=%s root=%s\n", epochText, rootText);
        }
        std::map<uint64_t, std::pair<GUID, uint64_t>> scopes;
        for (const auto& edge : observed.ScopedEdges)
        {
            if (edge.Child != stable.first && edge.Child != tagged.at(1001).first && edge.Child != tagged.at(1201).first) continue;
            LifecycleEtw::Require(edge.Epoch == stable.second && edge.Root != GUID_NULL && edge.Dispatcher != 0,
                "actual child edge has qualified root/dispatcher scope");
            if (edge.Connected && edge.Membership == 2)
            {
                const auto attach = edge.Child == stable.first ? survivor.AttachBeginQpc :
                    edge.Child == tagged.at(1001).first ? first.AttachBeginQpc : restarted.AttachBeginQpc;
                LifecycleEtw::Require(attach != 0 && edge.Timestamp >= attach,
                    "rootless construction never emits a visual connect before actual content attachment");
            }
            const auto scope = std::make_pair(edge.Root, edge.Dispatcher);
            if (scopes.count(edge.Child))
            {
                if (scopes.at(edge.Child) != scope)
                    std::printf("SCOPE_CHANGE: child=%llu survivor=%llu oldRoot=%08lx newRoot=%08lx oldDispatcher=%llu newDispatcher=%llu time=%lld firstShutdown=%lld restartShutdown=%lld\n",
                        static_cast<unsigned long long>(edge.Child), static_cast<unsigned long long>(stable.first),
                        static_cast<unsigned long>(scopes.at(edge.Child).first.Data1), static_cast<unsigned long>(edge.Root.Data1),
                        static_cast<unsigned long long>(scopes.at(edge.Child).second), static_cast<unsigned long long>(edge.Dispatcher),
                        static_cast<long long>(edge.Timestamp), static_cast<long long>(firstShutdown),
                        static_cast<long long>(restarted.ShutdownQpc));
                LifecycleEtw::Require(scopes.at(edge.Child) == scope, "live object cannot cross-attribute to another root/dispatcher");
            }
            scopes[edge.Child] = scope;
        }
        LifecycleEtw::Require(scopes.size() == 3, "all three actual root/object scopes are observed");
        LifecycleEtw::Require(scopes.at(stable.first).first != scopes.at(tagged.at(1001).first).first &&
            scopes.at(stable.first).first != scopes.at(tagged.at(1201).first).first &&
            scopes.at(stable.first).second != scopes.at(tagged.at(1001).first).second &&
            scopes.at(stable.first).second != scopes.at(tagged.at(1201).first).second,
            "simultaneous independent cores use isolated root GUIDs and dispatcher IDs");
        for (const auto& record : observed.LifecycleRecords)
            if (record.Event == 2 && record.Object == stable.first && record.Operation == 1)
                LifecycleEtw::Require(record.Timestamp > firstShutdown && record.Timestamp > restarted.ShutdownQpc,
                    "survivor destruction occurs only after both other core shutdowns");
        LifecycleEtw::Require(observed.Destroyed.count(stable.first) && observed.Destroyed.count(tagged.at(1001).first) &&
            observed.Destroyed.count(tagged.at(1201).first), "all three actual object lifetimes eventually close");
        std::printf("SIMULTANEOUS_IDS: first=%llu survivor=%llu restarted=%llu rootScopes=%zu stablePhases=5\n",
            static_cast<unsigned long long>(tagged.at(1001).first), static_cast<unsigned long long>(stable.first),
            static_cast<unsigned long long>(tagged.at(1201).first), scopes.size());
        std::puts("PASS: simultaneous independent cores, survivor identity and qualified root/dispatcher isolation");
    }
    void RootMutationTest(bool frameAssertionsOnly = false, bool orderedEdges = false)
    {
        using namespace winrt::Microsoft::UI::Xaml;
        LifecycleEtw::Session session;
        session.Enable(31);
        {
            std::puts("ROOTS: constructing windows"); std::fflush(stdout);
            Window first;
            Window second;
            std::puts("ROOTS: constructing content"); std::fflush(stdout);
            Controls::Grid left;
            Controls::Grid right;
            Shapes::Rectangle child;
            Shapes::Rectangle sibling;
            child.Width(24);
            child.Height(24);
            wchar_t paint[2]{};
            if (GetEnvironmentVariableW(L"WINUI_PROFILER_TEST_PAINT", paint, ARRAYSIZE(paint)) == 1 && paint[0] == L'1')
            {
                child.Fill(Media::SolidColorBrush(winrt::Windows::UI::Color{255, 255, 0, 0}));
                std::puts("FIXTURE: existing Rectangle has opaque Fill");
            }
            left.Children().Append(child);
            first.Content(left);
            second.Content(right);
            if (orderedEdges)
            {
                sibling.Width(25); sibling.Height(25); sibling.Opacity(0.8);
                left.Children().Append(sibling);
                Hosting::ElementCompositionPreview::GetElementVisual(child);
                Hosting::ElementCompositionPreview::GetElementVisual(sibling);
                auto visual = Hosting::ElementCompositionPreview::GetElementVisual(left);
                Hosting::ElementCompositionPreview::GetElementVisual(right);
                Hosting::ElementCompositionPreview::SetElementChildVisual(left, visual.Compositor().CreateContainerVisual());
            }
            std::puts("ROOTS: activating windows"); std::fflush(stdout);
            first.Activate();
            second.Activate();
            Pump(500);
            if (orderedEdges)
            {
                Hosting::ElementCompositionPreview::SetElementChildVisual(left, nullptr);
                Pump(100);
            }
            left.Children().RemoveAt(0);
            right.Children().Append(child);
            if (orderedEdges)
            {
                left.Children().RemoveAt(0);
                right.Children().Append(sibling);
            }
            child.Width(48);
            child.Opacity(0.5);
            right.UpdateLayout();
            left.Width(111);
            right.Width(112);
            Pump(500);
            right.Children().Clear();
            first.Close();
            second.Close();
        }
        Pump(100);
        const auto observed = session.Read();
        if (orderedEdges)
        {
            bool top = false, removed = false, interNode = false;
            for (const auto& edge : observed.VisualEdges)
            {
                top |= edge.Connected && edge.Order > 0;
                removed |= !edge.Connected && edge.Order > 0;
                const auto from = observed.VisualOwners.find(edge.Parent);
                const auto to = observed.VisualOwners.find(edge.Child);
                if (edge.Connected && from != observed.VisualOwners.end() && to != observed.VisualOwners.end())
                {
                    bool common = false;
                    for (const auto owner : from->second) common |= to->second.count(owner) != 0;
                    interNode |= !common;
                }
            }
            std::printf("ORDERED_EDGES: total=%zu nonzeroInsert=%d nonzeroRemove=%d interNode=%d\n",
                observed.VisualEdges.size(), top, removed, interNode);
            LifecycleEtw::Require(top, "actual nonempty-parent top insertion has measured nonzero order");
            LifecycleEtw::Require(removed, "actual visual removal retains measured nonzero order");
            LifecycleEtw::Require(interNode, "actual ordinary inter-CompNode edge has both qualified owners");
            for (const auto id : observed.Destroyed)
                LifecycleEtw::Require(observed.Domains.at(id) != 3, "owner reparent/teardown never fabricates COM visual destruction");
            std::puts("PASS: actual ordered visual mutations and known inter-node edges");
            return;
        }
        if (!frameAssertionsOnly)
        {
            LifecycleEtw::Require(observed.Counts[5] != 0, "actual tree mutations emit scoped membership");
            LifecycleEtw::Require(observed.Counts[16] != 0, "actual property setters emit typed changes");
            LifecycleEtw::Require(observed.Counts[7] != 0, "actual XAML/comp/visual correlation");
            LifecycleEtw::Require(observed.Counts[6] != 0, "actual observed visual property writes");
        }
        LifecycleEtw::Require(observed.Counts[8] != 0, "actual layout/render boundaries");
        LifecycleEtw::Require(observed.Counts[9] != 0, "actual qualifying root submissions");
        LifecycleEtw::Require(!observed.Submissions.empty(), "a frame is an actual primary submission, not an attempt/tick");
        bool shared = false;
        for (const auto& submission : observed.Submissions)
        {
            const auto end = observed.AttemptEnds.find(submission.AttemptId);
            LifecycleEtw::Require(end != observed.AttemptEnds.end() && end->second >= submission.Timestamp,
                "submitted attempt remains open through actual commit");
            for (const auto& other : observed.Submissions)
            {
                if (other.Id == submission.Id && other.Root != submission.Root)
                {
                    LifecycleEtw::Require(other.Timestamp == submission.Timestamp,
                        "root fanout uses one exact physical commit QPC timestamp");
                    shared = true;
                }
            }
        }
        LifecycleEtw::Require(shared, "two participating roots share one physical SubmittedFrameId");
        std::puts(frameAssertionsOnly ?
            "PASS: actual multiple-root physical submissions, shared timestamps and attempt containment" :
            "PASS: real multiple roots, reparenting, typed changes and rendering");
    }
    void ExactOrderedEdgesTest()
    {
        using namespace winrt::Microsoft::UI;
        using namespace winrt::Microsoft::UI::Xaml;
        auto clock = [] { LARGE_INTEGER now{}; QueryPerformanceCounter(&now); return now.QuadPart; };
        auto address = [](const Composition::Visual& visual)
        {
            const auto canonical = visual.as<::IUnknown>();
            return reinterpret_cast<uint64_t>(canonical.get());
        };
        struct Snapshot
        {
            uint64_t Parent, Child;
            int32_t Order;
            int64_t Time;
            std::vector<uint64_t> Siblings;
        };
        auto snapshot = [&](const Composition::Visual& visual)
        {
            const auto parent = visual.Parent();
            LifecycleEtw::Require(parent != nullptr, "actual visual has observable native parent");
            int32_t index = 0, order = -1;
            std::vector<uint64_t> siblings;
            for (const auto& sibling : parent.Children())
            {
                const auto canonical = address(sibling);
                siblings.push_back(canonical);
                if (canonical == address(visual)) order = index;
                ++index;
            }
            LifecycleEtw::Require(order >= 0, "actual native collection contains target visual");
            return Snapshot{address(parent), address(visual), order, clock(), std::move(siblings)};
        };
        LifecycleEtw::Session session;
        session.Enable(31);
        Window first, second;
        Controls::Grid left, right;
        Shapes::Rectangle a, b;
        a.Width(24); a.Height(24); b.Width(25); b.Height(25);
        a.Opacity(0.8); b.Opacity(0.8);
        left.Children().Append(a); left.Children().Append(b);
        first.Content(left); second.Content(right);
        right.Visibility(Visibility::Collapsed);
        auto av = Hosting::ElementCompositionPreview::GetElementVisual(a);
        auto bv = Hosting::ElementCompositionPreview::GetElementVisual(b);
        auto handIn = av.Compositor().CreateContainerVisual();
        Hosting::ElementCompositionPreview::SetElementChildVisual(left, handIn);
        first.Activate(); Pump(400);
        const auto oldHand = snapshot(handIn);
        LifecycleEtw::Require(oldHand.Order > 0, "fixture actually has a nonempty parent below hand-in");
        const Snapshot oldPrimary[] = {snapshot(av), snapshot(bv)};
        const Snapshot oldInter[] = {snapshot(av.Parent()), snapshot(bv.Parent())};
        LifecycleEtw::Require(oldInter[0].Parent == oldInter[1].Parent &&
            oldInter[0].Child != oldInter[1].Child && oldInter[1].Order > 0,
            "both actual child nodes share source collection with measured nonzero sibling index");
        const auto moveStart = clock();
        // The collapsed destination cache holds the real visual before source ownership ends.
        Hosting::ElementCompositionPreview::SetElementChildVisual(right, handIn);
        Hosting::ElementCompositionPreview::SetElementChildVisual(left, nullptr);
        Pump(200);
        LifecycleEtw::Require(handIn.Parent() == nullptr, "source native caller actually removed hand-in");
        right.Visibility(Visibility::Visible);
        second.Activate(); Pump(300);
        const auto newHand = snapshot(handIn);
        left.Children().RemoveAt(0); right.Children().Append(a);
        left.Children().RemoveAt(0); right.Children().Append(b);
        Pump(400);
        const Snapshot newPrimary[] = {snapshot(av), snapshot(bv)};
        const Snapshot newInter[] = {snapshot(av.Parent()), snapshot(bv.Parent())};
        const auto moveEnd = clock();
        first.Close(); second.Close(); Pump(100);
        const auto observed = session.Read();
        auto identity = [&](uint64_t pointer, int64_t time)
        {
            uint64_t result = 0;
            const auto known = observed.VisualAddresses.find(pointer);
            if (known != observed.VisualAddresses.end())
                for (const auto& value : known->second) if (value.first <= time) result = value.second;
            LifecycleEtw::Require(result != 0, "actual canonical visual maps to qualified baseline ID at phase");
            return result;
        };
        auto edge = [&](const Snapshot& expected, bool connected, int64_t start, int64_t end)
        {
            const auto parent = identity(expected.Parent, expected.Time);
            const auto child = identity(expected.Child, expected.Time);
            bool found = false;
            int32_t mutationOrder = expected.Order;
            if (connected)
            {
                for (const auto& value : observed.VisualEdges)
                    found |= value.Parent == parent && value.Child == child && value.Order == expected.Order &&
                        value.Connected && value.Timestamp >= start && value.Timestamp <= end;
            }
            else
            {
                std::vector<uint64_t> remaining;
                for (const auto sibling : expected.Siblings)
                    remaining.push_back(identity(sibling, expected.Time));
                for (const auto& value : observed.VisualEdges)
                {
                    if (value.Parent != parent || value.Timestamp <= expected.Time ||
                        value.Timestamp < start || value.Timestamp > end) continue;
                    LifecycleEtw::Require(!value.Connected, "source-parent mutation phase contains only native removals");
                    const auto position = std::find(remaining.begin(), remaining.end(), value.Child);
                    LifecycleEtw::Require(position != remaining.end(), "every intermediate removal targets actual snapshotted sibling");
                    const auto exactIndex = static_cast<int32_t>(position - remaining.begin());
                    std::printf("EXACT_REMOVAL: parent=%llu child=%llu initialCount=%zu remainingCount=%zu expected=%d actual=%d\n",
                        parent, value.Child, expected.Siblings.size(), remaining.size(), exactIndex, value.Order);
                    LifecycleEtw::Require(value.Order == exactIndex, "every native removal index matches independently snapshotted evolving collection");
                    remaining.erase(position);
                    if (value.Child == child) { mutationOrder = exactIndex; found = true; break; }
                }
            }
            std::printf("EXACT_EDGE: parent=%llu child=%llu order=%d connected=%d found=%d\n",
                parent, child, mutationOrder, connected, found);
            if (!found)
            {
                std::printf("EDGE_PHASE: start=%lld end=%lld snapshot=%lld parentAddress=%llx childAddress=%llx\n",
                    start, end, expected.Time, expected.Parent, expected.Child);
                for (const auto& value : observed.VisualEdges)
                    if (value.Child == child || value.Parent == parent)
                        std::printf("EDGE_OBSERVED: parent=%llu child=%llu order=%d connected=%d timestamp=%lld inPhase=%d\n",
                            value.Parent, value.Child, value.Order, value.Connected, value.Timestamp,
                            value.Timestamp >= start && value.Timestamp <= end);
                for (const auto& loss : observed.LossReasons)
                    if (loss.first >= start && loss.first <= end)
                        std::printf("EDGE_LOSS: timestamp=%lld reason=%s\n", loss.first, loss.second.c_str());
            }
            LifecycleEtw::Require(found, "exact actual collection index/qualified edge and phase");
            LifecycleEtw::Require(observed.Destroyed.count(child) == 0, "physical movement never fabricates visual COM destruction");
        };
        edge(oldHand, true, 0, moveStart);
        edge(oldHand, false, moveStart, moveEnd);
        edge(newHand, true, moveStart, moveEnd);
        LifecycleEtw::Require(identity(oldHand.Parent, oldHand.Time) != identity(newHand.Parent, newHand.Time),
            "hand-in physically reparents between distinct qualified native parents");
        LifecycleEtw::Require(identity(oldHand.Child, oldHand.Time) == identity(newHand.Child, newHand.Time),
            "overlapping actual cached owners preserve hand-in qualified identity");
        for (size_t i = 0; i < 2; ++i)
        {
            edge(oldPrimary[i], true, 0, moveStart);
            edge(oldPrimary[i], false, moveStart, moveEnd);
            edge(newPrimary[i], true, moveStart, moveEnd);
            edge(oldInter[i], true, 0, moveStart);
            edge(oldInter[i], false, moveStart, moveEnd);
            edge(newInter[i], true, moveStart, moveEnd);
            LifecycleEtw::Require(identity(oldInter[i].Parent, oldInter[i].Time) !=
                identity(newInter[i].Parent, newInter[i].Time), "both actual child nodes reparent to distinct native parent");
            LifecycleEtw::Require(identity(oldPrimary[i].Child, oldPrimary[i].Time) ==
                identity(newPrimary[i].Child, newPrimary[i].Time), "both moved child primary identities remain continuous");
        }
        std::puts("PASS: canonical-ID exact ordered hand-in and both inter-node child moves, phase pairs, no fake Destroy");
    }

    void FailedHandInInsertionTest()
    {
        using namespace winrt::Microsoft::UI;
        using namespace winrt::Microsoft::UI::Xaml;
        auto clock = [] { LARGE_INTEGER now{}; QueryPerformanceCounter(&now); return now.QuadPart; };
        auto address = [](const Composition::Visual& visual)
        {
            const auto canonical = visual.as<::IUnknown>();
            return reinterpret_cast<uint64_t>(canonical.get());
        };
        LifecycleEtw::Session session;
        session.Enable(31);
        Window first, second;
        Controls::Grid left, right;
        first.Content(left); second.Content(right);
        Shapes::Rectangle painted;
        painted.Width(24); painted.Height(24);
        painted.Fill(Media::SolidColorBrush{winrt::Windows::UI::Colors::Red()});
        left.Children().Append(painted);
        right.Background(Media::SolidColorBrush{winrt::Windows::UI::Colors::Blue()});
        auto leftPrimary = Hosting::ElementCompositionPreview::GetElementVisual(left);
        auto rightPrimary = Hosting::ElementCompositionPreview::GetElementVisual(right);
        auto handIn = leftPrimary.Compositor().CreateContainerVisual();
        auto probe = leftPrimary.Compositor().CreateContainerVisual();
        Hosting::ElementCompositionPreview::SetElementChildVisual(left, handIn);
        Hosting::ElementCompositionPreview::SetElementChildVisual(right, probe);
        first.Activate(); Pump(400);
        second.Activate(); Pump(400);
        LifecycleEtw::Require(handIn.Parent() != nullptr,
            "first live native parent owns actual shared hand-in");
        const auto firstParent = handIn.Parent();
        LifecycleEtw::Require(probe.Parent() != nullptr, "real probe establishes exact second native insertion parent");
        const auto secondParent = probe.Parent();
        const auto previousChildren = secondParent.Children().Count();
        const auto start = clock();
        Hosting::ElementCompositionPreview::SetElementChildVisual(right, handIn);
        Pump(400);
        const auto end = clock();
        LifecycleEtw::Require(handIn.Parent() && address(handIn.Parent()) == address(firstParent),
            "ignored native insertion leaves first actual parent owning visual");
        LifecycleEtw::Require(probe.Parent() == nullptr, "actual replacement caller removed old probe in rejection phase");
        LifecycleEtw::Require(secondParent.Children().Count() + 1 == previousChildren,
            "failed actual native insertion adds no second-parent child");
        const auto childAddress = address(handIn);
        const auto parentAddress = address(secondParent);
        first.Close(); second.Close(); Pump(100);
        const auto observed = session.Read();
        auto identity = [&](uint64_t pointer)
        {
            uint64_t result = 0;
            const auto found = observed.VisualAddresses.find(pointer);
            if (found != observed.VisualAddresses.end())
                for (const auto& value : found->second) if (value.first <= end) result = value.second;
            LifecycleEtw::Require(result != 0, "failed operation canonical endpoint has qualified visual ID");
            return result;
        };
        const auto parent = identity(parentAddress), child = identity(childAddress);
        bool rejected = false, falseSuccess = false;
        for (const auto timestamp : observed.RejectedHandInInsertions)
            rejected |= timestamp >= start && timestamp <= end;
        for (const auto& edge : observed.VisualEdges)
            falseSuccess |= edge.Parent == parent && edge.Child == child && edge.Connected &&
                edge.Timestamp >= start && edge.Timestamp <= end;
        std::printf("FAILED_HANDIN: parent=%llu child=%llu nativeInvalidArg=%d falseSuccess=%d\n",
            parent, child, rejected, falseSuccess);
        LifecycleEtw::Require(rejected, "exact native ignored InsertAtTop E_INVALIDARG diagnostic reached in phase");
        LifecycleEtw::Require(!falseSuccess, "rejected native operation never emits successful second-parent edge");
        LifecycleEtw::Require(observed.Destroyed.count(child) == 0, "failed insertion never fabricates visual destruction");
        std::puts("PASS: actual ignored native E_INVALIDARG, first parent preserved, no successful second-parent edge");
    }

    void LateEnableTest()
    {
        LifecycleEtw::RequireObjectsDisabledBeforeConstruction();
        winrt::Microsoft::UI::Xaml::Controls::Grid existing;
        LifecycleEtw::Session session;
        session.Enable(2);
        existing.Width(43);
        existing = nullptr;
        const auto observed = session.Read();
        LifecycleEtw::Require(!observed.PropertyObjects.empty(), "late enable observes existing object at actual property boundary");
        for (const auto& value : observed.LifecycleRecords)
            std::printf("LATE_EVENT: event=%u seq=%llu objectOrType=%llu op=%u domain=%u hasType=%u schema=%u epoch=%08lx-%04x-%04x timestamp=%lld\n",
                value.Event, value.Sequence, value.Object, value.Operation, value.Domain, value.HasType,
                value.Schema, value.Epoch.Data1, value.Epoch.Data2, value.Epoch.Data3, value.Timestamp);
        uint64_t id = 0;
        for (const auto& value : observed.PropertyValues)
        {
            if (value.CurrentKind == 1 && value.Number == 43)
            {
                LifecycleEtw::Require(value.PreviousKind == 4 && value.PreviousUnit == 6 && value.CurrentUnit == 6,
                    "late-enable Width preserves Missing old value and matching Scalar units");
                id = value.ObjectId;
            }
        }
        LifecycleEtw::Require(id != 0, "actual Width(43) emits its numeric property observation");
        LifecycleEtw::Require(observed.BirthAddresses.count(id) == 0 && observed.Baselines.count(id) != 0,
            "late observation is baseline-present, never a fabricated birth");
        LifecycleEtw::Require(observed.Destroyed.count(id) != 0, "late-observed lifetime ends at actual destruction");
        LifecycleEtw::Require(observed.Counts[3] != 0, "late observation safely resolves the constructed type");
        std::puts("PASS: actual late-enable baseline/type/destruction");
    }
    void LateEnablePreconditionTest()
    {
        LifecycleEtw::RequireObjectsDisabledBeforeConstruction();
        LifecycleEtw::Session intruder;
        intruder.Enable(1 | 2 | 32, 32);
        LifecycleEtw::RequireObjectsDisabledBeforeConstruction();
        std::puts("PASS: actual Resource-only MatchAll listener does not contaminate Objects");
        intruder.Enable(2);
        bool rejected = false;
        try { LifecycleEtw::RequireObjectsDisabledBeforeConstruction(); }
        catch (const std::runtime_error& error)
        {
            rejected = std::string(error.what()) ==
                "late-enable fixture precondition: Objects already enabled by another ETW listener before construction";
        }
        intruder.Disable();
        intruder.Read();
        LifecycleEtw::Require(rejected, "late-enable precondition detects actual extra Objects listener");
        LifecycleEtw::RequireObjectsDisabledBeforeConstruction();
        std::puts("PASS: actual ETW listener contamination detected before object construction");
    }
    void ReenableTest()
    {
        LifecycleEtw::Session continuous;
        continuous.Enable(4 | 16 | 1);
        LifecycleEtw::Session objects;
        objects.Enable(2);
        winrt::Microsoft::UI::Xaml::Controls::Grid existing;
        existing.Width(51);
        objects.Disable();
        existing.Width(52);
        objects.Enable(2);
        existing.Width(53);
        existing = nullptr;
        const auto observed = objects.Read();
        const auto other = continuous.Read();
        UNREFERENCED_PARAMETER(other);
        LifecycleEtw::Require(observed.PropertyObjects.size() >= 2, "property events resume after family reenable");
        const auto first = observed.PropertyObjects.front();
        const auto last = observed.PropertyObjects.back();
        LifecycleEtw::Require(first.first == last.first && first.second == last.second,
            "another continuous identity listener preserves epoch and identity");
        LifecycleEtw::Require(observed.Baselines.count(first.first) != 0, "reenabled Objects receives a new baseline");
        std::puts("PASS: actual keyword reenable without resetting other listener continuity");
    }
    void FailedConstructionAndReuseTest()
    {
        LifecycleEtw::Session session;
        session.Enable(2);
        bool failed = false;
        try
        {
            winrt::Microsoft::UI::Xaml::Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Width='not-a-number'/>");
        }
        catch (const winrt::hresult_error&) { failed = true; }
        LifecycleEtw::Require(failed, "real parser/factory construction fails");
        for (int i = 0; i < 128; ++i)
        {
            winrt::Microsoft::UI::Xaml::Controls::Grid transient;
        }
        const auto observed = session.Read();
        std::map<std::pair<uint8_t, uint64_t>, uint64_t> addresses;
        bool reused = false;
        for (const auto& birth : observed.BirthAddresses)
        {
            const auto key = std::make_pair(observed.Domains.at(birth.first), birth.second);
            const auto previous = addresses.find(key);
            if (previous != addresses.end())
            {
                LifecycleEtw::Require(previous->second != birth.first && observed.Destroyed.count(previous->second) != 0,
                    "actual reused address gets a different closed lifetime");
                reused = true;
            }
            addresses[key] = birth.first;
        }
        LifecycleEtw::Require(reused, "exercise actual allocator address reuse");
        LifecycleEtw::Require(!observed.Destroyed.empty(), "failed and transient construction cleanup reaches actual destruction");
        std::puts("PASS: actual failed construction and address reuse");
    }
    void NoDeviceTest()
    {
#if defined(WINUI_PROFILER_FRAME_TEST_HOOKS)
        auto factory = Factory<ABI::Microsoft::UI::Xaml::IDxamlCoreTestHooksStatics>(
            RuntimeClass_Microsoft_UI_Xaml_DxamlCoreTestHooks);
        ComPtr<ABI::Microsoft::UI::Xaml::IDxamlCoreTestHooks> coreHooks;
        Check(factory->GetForCurrentThread(&coreHooks), "get current core test hooks");
        ComPtr<IXamlTestHooks> hooks;
        Check(coreHooks.As(&hooks), "query private test command interface");
        winrt::Microsoft::UI::Xaml::Controls::Grid target;
        ComPtr<ABI::Microsoft::UI::Xaml::IDependencyObject> object;
        Check(winrt::get_unknown(target)->QueryInterface(IID_PPV_ARGS(&object)), "query unattached command object");
        LifecycleEtw::Session session(true);
        session.Enable(16);
        LARGE_INTEGER begin{}, end{};
        QueryPerformanceCounter(&begin);
        const HRESULT result = hooks->InvokeInternalCommand(object.Get(),
            Profiler_CommitMainDeviceWithoutDevice, nullptr, nullptr);
        QueryPerformanceCounter(&end);
        std::printf("NO_DEVICE_CALL: result=0x%08lx begin=%lld end=%lld\n", result,
            static_cast<long long>(begin.QuadPart), static_cast<long long>(end.QuadPart));
        const auto observed = session.Read();
        LifecycleEtw::Require(result == S_OK, "actual enabled no-device primary commit succeeds with checked pre/postconditions");
        for (const auto& frame : observed.FrameOutcomes)
            if (frame.first >= begin.QuadPart && frame.first <= end.QuadPart)
                LifecycleEtw::Require(frame.second != 0 && frame.second != 1 && frame.second != 3,
                    "no-device S_OK is neither pending, submitted nor standalone commit");
        std::puts("PASS: actual no-device primary commit S_OK without pending/submitted/standalone outcome");
#else
        throw std::runtime_error("NoDevice requires WINUI_PROFILER_FRAME_TEST_HOOKS in host and diagnostic product");
#endif
    }
    void DebugCounterTest()
    {
        using namespace winrt::Microsoft::UI::Xaml;
        LifecycleEtw::Session session(true);
        session.Enable(31);
        Window window;
        Controls::Grid content;
        Shapes::Rectangle target;
        target.Height(50);
        target.Fill(Media::SolidColorBrush(winrt::Windows::UI::Colors::Blue()));
        content.Children().Append(target);
        window.Content(content);
        window.Activate();
        target.Width(1001);
        Pump(100);
        auto settings = Application::Current().DebugSettings();
        const bool previous = settings.EnableFrameRateCounter();
        LARGE_INTEGER begin{}, end{};
        QueryPerformanceCounter(&begin);
        settings.EnableFrameRateCounter(true);
        LifecycleEtw::Require(settings.EnableFrameRateCounter(), "public frame-rate counter is enabled");
        for (int width = 1002; width != 1005; ++width)
        {
            target.Width(width);
            content.UpdateLayout();
            Pump(70);
        }
        QueryPerformanceCounter(&end);
        const auto observed = session.Read();
        settings.EnableFrameRateCounter(previous);
        window.Close();
        uint64_t targetId = 0;
        GUID epoch{}, root{};
        for (size_t i = 0; i != observed.PropertyValues.size(); ++i)
            if (observed.PropertyValues[i].Number == 1001)
            {
                targetId = observed.PropertyObjects[i].first;
                epoch = observed.PropertyObjects[i].second;
            }
        for (const auto& edge : observed.ScopedEdges)
            if (edge.Child == targetId && edge.Connected && edge.Membership == 2)
            {
                LifecycleEtw::Require(edge.Domain == 0 && edge.Epoch == epoch && edge.Dispatcher != 0,
                    "debug-counter target has qualified core/root/dispatcher identity");
                LifecycleEtw::Require(root == GUID_NULL || root == edge.Root, "debug counter never changes target root");
                root = edge.Root;
            }
        LifecycleEtw::Require(targetId && root != GUID_NULL, "visible debug-counter target root is observed");
        size_t standalone = 0, primary = 0;
        for (const auto& commit : observed.StandaloneCommits)
        {
            LifecycleEtw::Require(!commit.HasAttempt && !commit.HasSubmitted,
                "actual standalone commit never acquires an attempt or physical submitted ID");
            if (commit.Timestamp >= begin.QuadPart && commit.Timestamp <= end.QuadPart &&
                commit.HasHResult && commit.HResult == S_OK) ++standalone;
        }
        for (const auto& submission : observed.Submissions)
        {
            const auto finish = observed.AttemptEnds.find(submission.AttemptId);
            LifecycleEtw::Require(finish != observed.AttemptEnds.end() && finish->second >= submission.Timestamp,
                "debug-enabled primary submission stays inside its root attempt");
            if (submission.Root == root && submission.Timestamp >= begin.QuadPart && submission.Timestamp <= end.QuadPart) ++primary;
            for (const auto& other : observed.Submissions)
                if (other.Id == submission.Id)
                    LifecycleEtw::Require(other.Timestamp == submission.Timestamp, "debug rendering preserves physical fanout timestamp");
        }
        LifecycleEtw::Require(standalone > 0 && primary > 0, "actual debug interval contains successful standalone and qualifying primary commits");
        std::printf("PASS: actual debug counter standalone=%zu primary=%zu noStandaloneAttemptOrSubmittedId=1\n", standalone, primary);
    }
    void DebugCounterHost(bool verifyDeployment)
    {
        using namespace winrt::Microsoft::UI;
        std::exception_ptr failure;
        Xaml::Application::Start([&](auto const&)
        {
            Xaml::Application app;
            const bool queued = Dispatching::DispatcherQueue::GetForCurrentThread().TryEnqueue([&, app]
            {
                try
                {
                    wchar_t path[32768]{};
                    GetModuleFileNameW(GetModuleHandleW(L"Microsoft.UI.Xaml.dll"), path, ARRAYSIZE(path));
                    std::wprintf(L"LOADED_LOCAL_PRODUCT: %s\n", path);
                    std::puts("HOST_READY"); std::fflush(stdout);
                    if (verifyDeployment) Sleep(10000);
                    DebugCounterTest();
                }
                catch (...) { failure = std::current_exception(); }
                app.Exit();
            });
            if (!queued)
            {
                failure = std::make_exception_ptr(std::runtime_error("queue public application debug test"));
                app.Exit();
            }
        });
        if (failure) std::rethrow_exception(failure);
    }
    void PostCommitHost(bool verifyDeployment)
    {
#if defined(WINUI_PROFILER_FRAME_TEST_HOOKS)
        using namespace winrt::Microsoft::UI;
        using namespace WinUIProfiler::FrameTest;
        struct Commands
        {
            ComPtr<IXamlTestHooks> Hooks;
            ComPtr<ABI::Microsoft::UI::Xaml::IDependencyObject> Object;
            void Initialize(Xaml::DependencyObject const& target)
            {
                auto factory = Factory<ABI::Microsoft::UI::Xaml::IDxamlCoreTestHooksStatics>(
                    RuntimeClass_Microsoft_UI_Xaml_DxamlCoreTestHooks);
                ComPtr<ABI::Microsoft::UI::Xaml::IDxamlCoreTestHooks> coreHooks;
                Check(factory->GetForCurrentThread(&coreHooks), "PostCommit current core hooks");
                Check(coreHooks.As(&Hooks), "PostCommit private command interface");
                Check(winrt::get_unknown(target)->QueryInterface(IID_PPV_ARGS(&Object)), "PostCommit current-core target");
            }
            HRESULT Invoke(XamlInternalCommand command)
            {
                return Hooks->InvokeInternalCommand(Object.Get(), command, nullptr, nullptr);
            }
            HRESULT State(uint32_t* state)
            {
                ComPtr<ABI::Windows::Foundation::IPropertyValue> value;
                const HRESULT result = Hooks->InvokeInternalCommand(Object.Get(),
                    Profiler_GetPostCommitState, nullptr, &value);
                if (FAILED(result)) return result;
                return value ? value->GetUInt32(state) : E_UNEXPECTED;
            }
        };
        LifecycleEtw::Session session(true, true);
        session.Enable(31);
        Commands commands;
        Xaml::Window window{nullptr};
        Xaml::Controls::Grid content{nullptr};
        Xaml::Shapes::Rectangle target{nullptr};
        std::exception_ptr startupFailure;
        bool awaiting = false;
        bool xamlShutdown = false, queueShutdown = false, windowClosed = false;
        unsigned handled = 0;
        uint32_t consumedState = 0;
        const DWORD ownerThread = GetCurrentThreadId();
        LARGE_INTEGER begin{}, end{};
        Xaml::Application::Start([&](auto const&)
        {
            Xaml::Application app;
            app.UnhandledException([&](auto const& sender, auto const& args)
            {
                // No catch-all or second allowance. State query also refuses a
                // real layout error, matching the runtime's forced-fail-fast policy.
                if (!awaiting || handled || GetCurrentThreadId() != ownerThread ||
                    args.Exception().value != PostCommitFailure) return;
                if (FAILED(commands.State(&consumedState)) || consumedState != 2 ||
                    commands.Invoke(Profiler_ArmPostCommitFailure) != E_ILLEGAL_METHOD_CALL) return;
                awaiting = false;
                ++handled; // Consume the app allowance before any exit callouts.
                args.Handled(true);
                QueryPerformanceCounter(&end);
                std::printf("POST_COMMIT_TERMINAL: HRESULT=0x%08lx handled=%u consumed=%u begin=%lld end=%lld\n",
                    static_cast<unsigned long>(PostCommitFailure), handled, consumedState,
                    static_cast<long long>(begin.QuadPart), static_cast<long long>(end.QuadPart));
                std::fflush(stdout);
                sender.template as<Xaml::Application>().Exit(); // Ordinary shutdown, never a scheduler unwind.
            });
            const bool queued = Dispatching::DispatcherQueue::GetForCurrentThread().TryEnqueue([&, app]
            {
                try
                {
                    wchar_t path[32768]{};
                    LifecycleEtw::Require(GetModuleFileNameW(GetModuleHandleW(L"Microsoft.UI.Xaml.dll"),
                        path, ARRAYSIZE(path)) != 0, "PostCommit loaded local runtime");
                    std::wprintf(L"LOADED_LOCAL_PRODUCT: %s\n", path);
                    std::puts("HOST_READY"); std::fflush(stdout);
                    if (verifyDeployment) Sleep(10000);
                    auto manager = Xaml::Hosting::WindowsXamlManager::GetForCurrentThread();
                    LifecycleEtw::Require(manager != nullptr, "Application owns current XAML manager");
                    manager.XamlShutdownCompletedOnThread([&](auto const&, auto const&) { xamlShutdown = true; });
                    Dispatching::DispatcherQueue::GetForCurrentThread().ShutdownCompleted(
                        [&](auto const&, auto const&) { queueShutdown = true; });
                    window = Xaml::Window();
                    window.Closed([&](auto const&, auto const&) { windowClosed = true; });
                    content = Xaml::Controls::Grid();
                    target = Xaml::Shapes::Rectangle();
                    target.Height(50);
                    target.Fill(Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Blue()));
                    content.Children().Append(target);
                    window.Content(content);
                    window.Activate();
                    target.Width(2801);
                    Pump(150); // Ordinary startup before arming, never pump the failed core.
                    commands.Initialize(target);
                    uint32_t state = 99;
                    Check(commands.State(&state), "PostCommit initial guarded state (old Products must refuse)");
                    LifecycleEtw::Require(state == 0, "current core starts unarmed");
                    QueryPerformanceCounter(&begin);
                    Check(commands.Invoke(Profiler_ArmPostCommitFailure), "arm real PostCommit once");
                    awaiting = true;
                    Check(commands.State(&state), "PostCommit armed state");
                    LifecycleEtw::Require(state == 1 &&
                        commands.Invoke(Profiler_ArmPostCommitFailure) == E_ILLEGAL_METHOD_CALL,
                        "duplicate arm cannot replace the current-core one-shot");

                    // A separate actual XAML core must remain unarmed while this one
                    // is armed. No calls into the main core are made on that thread.
                    std::packaged_task<void()> control([]
                    {
                        winrt::init_apartment(winrt::apartment_type::single_threaded);
                        {
                            auto dispatcher = Dispatching::DispatcherQueueController::CreateOnCurrentThread();
                            auto manager = Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
                            bool controlXamlShutdown = false, controlQueueShutdown = false;
                            manager.XamlShutdownCompletedOnThread(
                                [&](auto const&, auto const&) { controlXamlShutdown = true; });
                            dispatcher.DispatcherQueue().ShutdownCompleted(
                                [&](auto const&, auto const&) { controlQueueShutdown = true; });
                            {
                                Xaml::Controls::Grid controlTarget;
                                Commands controlCommands;
                                controlCommands.Initialize(controlTarget);
                                uint32_t controlState = 99;
                                Check(controlCommands.State(&controlState), "separate unarmed core state");
                                LifecycleEtw::Require(controlState == 0, "armed state never crosses cores");
                            }
                            manager.Close();
                            dispatcher.ShutdownQueue();
                            LifecycleEtw::Require(controlXamlShutdown && controlQueueShutdown,
                                "unarmed control completes ordinary XAML and dispatcher teardown");
                        }
                        winrt::uninit_apartment();
                    });
                    auto result = control.get_future();
                    std::thread controlThread(std::move(control));
                    controlThread.join();
                    result.get(); // Propagate any control failure, never suppress it.
                    Check(commands.State(&state), "main core remains armed after control teardown");
                    LifecycleEtw::Require(state == 1, "other-core teardown cannot consume the arm");

                    // Public preparation/removal plus a visible property mutation
                    // establish real pending/render work. Only the real PostCommit
                    // reached after the actual primary Commit may consume the arm.
                    auto service = Xaml::Media::Animation::ConnectedAnimationService::GetForCurrentView();
                    auto animation = service.PrepareToAnimate(L"ProfilerPostCommit", target);
                    LifecycleEtw::Require(animation != nullptr, "prepare actual connected animation");
                    content.Children().RemoveAt(0);
                    content.Background(Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Red()));
                    // Return to Application's normal loop. Do not Pump, manually
                    // tick, commit, or schedule recovery after the injected failure.
                }
                catch (winrt::hresult_error const&) { startupFailure = std::current_exception(); app.Exit(); }
                catch (std::exception const&) { startupFailure = std::current_exception(); app.Exit(); }
            });
            if (!queued)
            {
                startupFailure = std::make_exception_ptr(std::runtime_error("queue terminal PostCommit fixture"));
                app.Exit();
            }
        });
        // Reached only after Application's ordinary message-loop and queue/XAML
        // shutdown return normally. A crash or timeout can never reach validation.
        commands.Object.Reset();
        commands.Hooks.Reset();
        target = nullptr; content = nullptr; window = nullptr;
        if (startupFailure) std::rethrow_exception(startupFailure);
        LifecycleEtw::Require(handled == 1 && !awaiting && consumedState == 2 && end.QuadPart > begin.QuadPart,
            "exactly one consumed failure followed by ordinary Application.Start return");
        LifecycleEtw::Require(windowClosed && xamlShutdown && queueShutdown,
            "owned window, XAML core and dispatcher report ordinary terminal shutdown");
        const auto observed = session.Read();
        LifecycleEtw::Require(!observed.EventsLost && !observed.LogBuffersLost &&
            !observed.RealTimeBuffersLost && observed.Counts[15] == 0, "terminal qualification requires gap-free actual ETW");
        uint64_t targetId = 0, dispatcherId = 0;
        GUID epoch{}, root{};
        for (size_t i = 0; i != observed.PropertyValues.size(); ++i)
            if (observed.PropertyValues[i].Number == 2801)
            {
                LifecycleEtw::Require(!targetId, "unique owned PostCommit marker");
                targetId = observed.PropertyObjects[i].first;
                epoch = observed.PropertyObjects[i].second;
            }
        for (const auto& edge : observed.ScopedEdges)
            if (edge.Child == targetId && edge.Connected && edge.Membership == 2)
            {
                LifecycleEtw::Require(edge.Domain == 0 && edge.Epoch == epoch && edge.Dispatcher &&
                    (root == GUID_NULL || root == edge.Root) && (!dispatcherId || dispatcherId == edge.Dispatcher),
                    "owned PostCommit target has stable epoch/root/dispatcher");
                root = edge.Root;
                dispatcherId = edge.Dispatcher;
            }
        LifecycleEtw::Require(targetId != 0, "actual target identity marker");
        LifecycleEtw::ValidatePostCommit(observed.PostCommitFrames, epoch, root, dispatcherId,
            begin.QuadPart, end.QuadPart, PostCommitFailure);
        std::puts("PASS: actual primary submission preserved, independently failed outer attempt, one-shot/core isolation and graceful terminal shutdown");
#else
        UNREFERENCED_PARAMETER(verifyDeployment);
        throw std::runtime_error("PostCommit requires WINUI_PROFILER_FRAME_TEST_HOOKS in host and diagnostic product");
#endif
    }
    void AnimationRootTest()
    {
        using namespace winrt::Microsoft::UI::Xaml;
        LifecycleEtw::Session session(true);
        session.Enable(31);
        Window animatedWindow, otherWindow;
        Controls::Grid animatedContent, otherContent;
        Shapes::Rectangle target, otherTarget;
        animatedContent.Children().Append(target);
        otherContent.Children().Append(otherTarget);
        animatedWindow.Content(animatedContent);
        otherWindow.Content(otherContent);
        otherWindow.Activate();
        animatedWindow.Activate();
        Pump(100);
        target.Width(1001);
        otherTarget.Width(2001);
        animatedContent.UpdateLayout();
        otherContent.UpdateLayout();
        Pump(50);
        Media::Animation::DoubleAnimation animation;
        animation.EnableDependentAnimation(true);
        animation.From(1001);
        animation.To(1041);
        animation.Duration(DurationHelper::FromTimeSpan(std::chrono::milliseconds(350)));
        Media::Animation::Storyboard::SetTarget(animation, target);
        Media::Animation::Storyboard::SetTargetProperty(animation, L"Width");
        Media::Animation::Storyboard storyboard;
        storyboard.Children().Append(animation);
        bool completed = false;
        storyboard.Completed([&](auto const&, auto const&) { completed = true; });
        LARGE_INTEGER begin{}, end{};
        QueryPerformanceCounter(&begin);
        storyboard.Begin();
        Pump(700);
        QueryPerformanceCounter(&end);
        std::printf("ANIMATION_PUBLIC: completed=%d width=%g actualWidth=%g\n", completed, target.Width(), target.ActualWidth());
        LifecycleEtw::Require(completed && target.ActualWidth() > 1001,
            "public dependent Width animation completes and changes actual layout");
        storyboard.Stop();
        animatedWindow.Close();
        otherWindow.Close();
        Pump(50);
        const auto observed = session.Read();
        uint64_t targetId = 0, otherId = 0;
        GUID targetEpoch{};
        for (size_t i = 0; i != observed.PropertyValues.size(); ++i)
        {
            if (observed.PropertyValues[i].Number == 1001)
            {
                targetId = observed.PropertyObjects[i].first;
                targetEpoch = observed.PropertyObjects[i].second;
            }
            if (observed.PropertyValues[i].Number == 2001) otherId = observed.PropertyObjects[i].first;
        }
        LifecycleEtw::Require(targetId && otherId && targetId != otherId, "both actual roots have distinct tagged elements");
        GUID targetRoot{}, otherRoot{};
        for (const auto& edge : observed.ScopedEdges)
        {
            if (!edge.Connected || edge.Membership != 2) continue;
            if (edge.Child == targetId || edge.Child == otherId)
                LifecycleEtw::Require(edge.Domain == 0 && edge.Epoch == targetEpoch && edge.Dispatcher != 0,
                    "animation participants have lifetime-qualified core identities and dispatchers");
            if (edge.Child == targetId && targetRoot != GUID_NULL)
                LifecycleEtw::Require(targetRoot == edge.Root, "animated object keeps its actual root");
            if (edge.Child == otherId && otherRoot != GUID_NULL)
                LifecycleEtw::Require(otherRoot == edge.Root, "other object keeps its actual root");
            if (edge.Child == targetId) targetRoot = edge.Root;
            if (edge.Child == otherId) otherRoot = edge.Root;
        }
        LifecycleEtw::Require(targetRoot != GUID_NULL && otherRoot != GUID_NULL && targetRoot != otherRoot,
            "animated and other element resolve to distinct actual content roots");
        std::set<uint64_t> animatedFrames;
        for (const auto& submission : observed.Submissions)
        {
            const auto finish = observed.AttemptEnds.find(submission.AttemptId);
            LifecycleEtw::Require(finish != observed.AttemptEnds.end() && finish->second >= submission.Timestamp,
                "actual animation submission remains inside its qualified root attempt");
            if (submission.Root == targetRoot && submission.Timestamp >= begin.QuadPart && submission.Timestamp <= end.QuadPart)
                animatedFrames.insert(submission.Id);
            for (const auto& other : observed.Submissions)
                if (other.Id == submission.Id)
                    LifecycleEtw::Require(other.Timestamp == submission.Timestamp,
                        "animation root fanout preserves one physical submission timestamp");
        }
        LifecycleEtw::Require(animatedFrames.size() > 1, "dependent animation root participates in multiple actual physical submissions");
        std::printf("PASS: actual dependent animation target=%llu other=%llu animatedPhysical=%zu completed=1\n",
            targetId, otherId, animatedFrames.size());
    }
    void DetachedLayoutTest()
    {
        using namespace winrt::Microsoft::UI::Xaml;
        LifecycleEtw::Session session(true);
        session.Enable(16);
        Controls::Grid content;
        Shapes::Rectangle child;
        content.Children().Append(child);
        for (int width = 20; width != 23; ++width)
        {
            child.Width(width);
            content.Measure({100, 100});
            content.Arrange({0, 0, 100, 100});
            content.UpdateLayout();
        }
        const auto observed = session.Read();
        LifecycleEtw::Require(observed.MeasureBegins > 0 && observed.ArrangeBegins > 0,
            "detached public layout executes actual measure and arrange producers");
        LifecycleEtw::Require(observed.MeasureBegins == observed.MeasureEnds &&
            observed.ArrangeBegins == observed.ArrangeEnds, "actual detached layout boundaries balance");
        LifecycleEtw::Require(observed.Submissions.empty(), "detached layout is not a physical submission");
        std::printf("PASS: detached layout measure=%llu arrange=%llu submitted=0\n",
            observed.MeasureBegins, observed.ArrangeBegins);
    }
    void IdleStartupTest()
    {
        using namespace winrt::Microsoft::UI::Xaml;
        Window window;
        LifecycleEtw::Session session;
        session.Enable(1 | 16);
        window.Content(Controls::Grid());
        window.Activate();
        Pump(1200);
        const auto observed = session.Read();
        std::set<uint64_t> physical;
        for (const auto& submission : observed.Submissions) physical.insert(submission.Id);
        LifecycleEtw::Require(!physical.empty() && physical.size() < 10, "idle startup never manufactures ten submissions");
        std::printf("PASS: idle startup has %zu distinct physical submissions, not event records/ticks\n", physical.size());
        window.Close();
        Pump(100);
    }
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    try
    {
        const bool lateEnableOnly = argc >= 2 && std::string(argv[1]) == "--late-enable";
        const bool lifetimeOnly = argc >= 2 && std::string(argv[1]) == "--lifetime";
        const bool roots = argc >= 2 && std::string(argv[1]) == "--roots";
        const bool rootDiagnostic = argc >= 2 && std::string(argv[1]) == "--root-diagnostic";
        const bool idleStartupOnly = argc >= 2 && std::string(argv[1]) == "--idle-startup";
        const bool frameSubmissionsOnly = argc >= 2 && std::string(argv[1]) == "--frame-submissions";
        const bool detachedLayoutOnly = argc >= 2 && std::string(argv[1]) == "--detached-layout";
        const bool animationRootOnly = argc >= 2 && std::string(argv[1]) == "--animation-root";
        const bool debugCounterOnly = argc >= 2 && std::string(argv[1]) == "--debug-counter";
        const bool noDeviceOnly = argc >= 2 && std::string(argv[1]) == "--no-device";
        const bool postCommitOnly = argc >= 2 && std::string(argv[1]) == "--post-commit";
        const bool orderedEdgesOnly = argc >= 2 && std::string(argv[1]) == "--ordered-edges";
        const bool failedInsertionOnly = argc >= 2 && std::string(argv[1]) == "--failed-insertion";
        const bool managerRestartOnly = argc >= 2 && std::string(argv[1]) == "--manager-restart";
        const bool simultaneousOnly = argc >= 2 && std::string(argv[1]) == "--simultaneous-managers";
        const bool allocatorOnly = argc >= 2 && std::string(argv[1]) == "--allocator-transitions";
        const bool graphicsReset = argc >= 2 && std::string(argv[1]) == "--graphics-reset";
        const bool graphicsOnly = graphicsReset || (argc >= 2 && std::string(argv[1]) == "--graphics-resources");
        const bool verifyDeployment = argc == 3 && (lateEnableOnly || lifetimeOnly || roots || rootDiagnostic || idleStartupOnly || frameSubmissionsOnly || detachedLayoutOnly || animationRootOnly || debugCounterOnly || noDeviceOnly || postCommitOnly || orderedEdgesOnly || failedInsertionOnly || managerRestartOnly || simultaneousOnly || allocatorOnly || graphicsOnly) && std::string(argv[2]) == "--verify-deployment";
        if (argc > 3 || (argc == 3 && !verifyDeployment) ||
            (argc >= 2 && !lateEnableOnly && !roots && !lifetimeOnly && !rootDiagnostic && !idleStartupOnly && !frameSubmissionsOnly && !detachedLayoutOnly && !animationRootOnly && !debugCounterOnly && !noDeviceOnly && !postCommitOnly && !orderedEdgesOnly && !failedInsertionOnly && !managerRestartOnly && !simultaneousOnly && !allocatorOnly && !graphicsOnly))
            throw std::runtime_error("expected a supported lifecycle mode, including --allocator-transitions [--verify-deployment]");
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        if (debugCounterOnly) { DebugCounterHost(verifyDeployment); return 0; }
        if (postCommitOnly) { PostCommitHost(verifyDeployment); return 0; }
        if (managerRestartOnly) { ManagerRestartTest(verifyDeployment); return 0; }
        if (simultaneousOnly) { SimultaneousManagersTest(verifyDeployment); return 0; }
        const auto dispatcher = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();
        auto manager = winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
        wchar_t path[32768]{};
        const HMODULE module = GetModuleHandleW(L"Microsoft.ui.xaml.dll");
        if (!module || !GetModuleFileNameW(module, path, ARRAYSIZE(path))) throw std::runtime_error("loaded local WinUI identity");
        std::wprintf(L"LOADED_LOCAL_PRODUCT: %s\n", path);
        if (lateEnableOnly || lifetimeOnly || roots || rootDiagnostic || idleStartupOnly || frameSubmissionsOnly || detachedLayoutOnly || animationRootOnly || noDeviceOnly || orderedEdgesOnly || failedInsertionOnly || allocatorOnly || graphicsOnly)
        {
            std::puts("HOST_READY");
            std::fflush(stdout);
            if (verifyDeployment) Sleep(10000);
        }
        if (allocatorOnly)
        {
            AllocatorTransitionsTest();
            manager.Close();
            return 0;
        }
        if (graphicsOnly)
        {
            GraphicsResourcesTest(manager, graphicsReset);
            return 0;
        }
        if (noDeviceOnly)
        {
            NoDeviceTest();
            manager.Close();
            return 0;
        }
        if (lateEnableOnly)
        {
            LateEnablePreconditionTest();
            LateEnableTest();
            manager.Close();
            std::puts("PASS: focused late-enable test and WindowsXamlManager teardown");
            return 0;
        }
        if (failedInsertionOnly)
        {
            FailedHandInInsertionTest();
            manager.Close();
            return 0;
        }
        if (orderedEdgesOnly)
        {
            ExactOrderedEdgesTest();
            manager.Close();
            return 0;
        }
        if (rootDiagnostic)
        {
            RootMutationTest();
            manager.Close();
            return 0;
        }
        if (idleStartupOnly)
        {
            IdleStartupTest();
            manager.Close();
            std::puts("PASS: focused idle-startup test and WindowsXamlManager teardown");
            return 0;
        }
        if (animationRootOnly)
        {
            AnimationRootTest();
            manager.Close();
            return 0;
        }
        if (detachedLayoutOnly)
        {
            DetachedLayoutTest();
            manager.Close();
            return 0;
        }
        if (frameSubmissionsOnly)
        {
            RootMutationTest(true);
            manager.Close();
            std::puts("PASS: focused frame-submission test and WindowsXamlManager teardown");
            return 0;
        }
        {
            LifecycleEtw::Session session;
            session.Enable(3);
            auto objectFactory = Factory<IActivationFactory>(L"Microsoft.UI.Xaml.Controls.Grid");
            for (int i = 0; i < 3; ++i)
            {
                ComPtr<IInspectable> object;
                Check(objectFactory->ActivateInstance(&object), "Grid.ActivateInstance");
                object.Reset();
            }
            auto observations = session.Read();
            LifecycleEtw::Require(!observations.BirthAddresses.empty(), "actual core constructor emits minimal lifetime birth");
            LifecycleEtw::Require(!observations.Destroyed.empty(), "actual core destructor closes lifetime");
            LifecycleEtw::Require(observations.Counts[3] != 0, "actual postconstruction type resolution");
            LifecycleEtw::Require(observations.Counts[4] != 0, "actual core/peer association");
            std::puts("PASS: actual local core/peer construction, type resolution and destruction");
        }
        LateEnableTest();
        ReenableTest();
        FailedConstructionAndReuseTest();
        if (argc == 1 || std::string(argv[1]) == "--roots")
        {
            RootMutationTest();
            IdleStartupTest();
        }
        manager.Close();
        std::puts("PASS: lifecycle suite and WindowsXamlManager teardown");
        return 0;
    }
    catch (const winrt::hresult_error& error)
    {
        std::printf("FAIL: WinRT HRESULT 0x%08lx\n", static_cast<unsigned long>(error.code().value));
        std::wprintf(L"%s\n", error.message().c_str());
        return 1;
    }
    catch (const std::exception& error)
    {
        std::printf("FAIL: %s\n", error.what());
        return 1;
    }
}
