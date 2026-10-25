// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <type_traits>
#include <vector>
#include "../../../dxaml/test/native/external/framework/profiler/ProfilerLifecycleEtw.h"
#if __has_include("WinUIProfilerGraphicsHooks.h")
#include "WinUIProfilerGraphicsHooks.h"
#include "WinUIProfilerHooks.h"
#define HAS_GRAPHICS_HOOKS 1
#endif
#if __has_include("WinUIProfilerResourceHooks.h")
#include "WinUIProfilerResourceHooks.h"
#define HAS_RESOURCE_HOOKS 1
#endif

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)

int main(int argc, char** argv)
{
#ifndef HAS_GRAPHICS_HOOKS
    (void)argc; (void)argv;
    CHECK(false && "authoritative graphics hooks not implemented");
#else
    using namespace WinUIProfiler;
    using namespace WinUIProfiler::Hooks;
    InitializeRuntime();
    LifecycleEtw::Session session(argc > 2 && std::strcmp(argv[2], "--retain-etl") == 0);
    session.Enable(33);
    int device{}, surface{}, other{}, late{};
    if (argc > 1 && std::strcmp(argv[1], "--brush-xaml-owner") == 0)
    {
#ifndef HAS_RESOURCE_HOOKS
        CHECK(false && "resource owner hooks are not implemented");
#else
        auto run = [&](auto change)
        {
            if constexpr (std::is_invocable_v<decltype(change), const void*, const void*, GraphicsResourceKind,
                GraphicsSurfaceOperation, uint32_t, uint32_t, HRESULT, const void*>)
            {
                session.Enable(35);
                int owner{}, lateOwner{};
                Born(Domain::XamlCore, &owner);
                change(&surface, nullptr, GraphicsResourceKind::CompositionBrush,
                    GraphicsSurfaceOperation::Create, 0, 0, S_OK, &owner);
                change(&surface, nullptr, GraphicsResourceKind::CompositionBrush,
                    GraphicsSurfaceOperation::BaselinePresent, 0, 0, S_OK, &owner);
                Destroyed(Domain::XamlCore, &owner);
                Born(Domain::XamlCore, &owner);
                change(&surface, nullptr, GraphicsResourceKind::CompositionBrush,
                    GraphicsSurfaceOperation::OwnershipRelease, 0, 0, S_OK, nullptr);
                change(&surface, nullptr, GraphicsResourceKind::CompositionBrush,
                    GraphicsSurfaceOperation::Destroy, 0, 0, S_OK, nullptr);
                change(&surface, nullptr, GraphicsResourceKind::CompositionBrush,
                    GraphicsSurfaceOperation::Create, 0, 0, S_OK, &owner);
                change(&late, nullptr, GraphicsResourceKind::CompositionBrush,
                    GraphicsSurfaceOperation::BaselinePresent, 0, 0, S_OK, &lateOwner);
                const auto observed = session.Read();
                CHECK(observed.Resources.size() == 5 && observed.Devices.empty());
                CHECK(observed.LifecycleRecords.size() == 4);
                if (observed.Resources.size() == 5 && observed.LifecycleRecords.size() == 4)
                {
                    for (const auto& resource : observed.Resources)
                    {
                        CHECK(resource.HasOwner && resource.OwnerDomain == 0 && resource.OwnerEpoch == resource.Epoch);
                        CHECK(resource.Kind == 3 && !resource.HasBacking && !resource.HasBytes && resource.Coverage == 1);
                    }
                    CHECK(observed.Resources[0].OwnerId == observed.LifecycleRecords[0].Object);
                    CHECK(observed.Resources[1].OwnerId == observed.Resources[0].OwnerId &&
                        observed.Resources[2].OwnerId == observed.Resources[0].OwnerId);
                    CHECK(observed.Resources[3].OwnerId == observed.LifecycleRecords[2].Object &&
                        observed.Resources[3].OwnerId != observed.Resources[0].OwnerId);
                    CHECK(observed.Resources[4].OwnerId == observed.LifecycleRecords[3].Object &&
                        observed.LifecycleRecords[3].Operation == 2);
                }
                CHECK(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost);
            }
            else CHECK(false && "resource-to-XAML owner linkage API is missing");
        };
        run(GraphicsResourceChanged);
#endif
    }
    else if (argc > 1 && std::strcmp(argv[1], "--brush-owner-gap") == 0)
    {
        const auto kind = GraphicsResourceKind::CompositionBrush;
        int owner{};
        session.Enable(35);
        Born(Domain::XamlCore, &owner);
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::Create, 0, 0, S_OK, &owner);
        session.Enable(33);
        Destroyed(Domain::XamlCore, &owner);
        Born(Domain::XamlCore, &owner);
        GraphicsResourceChanged(&other, nullptr, kind, GraphicsSurfaceOperation::Create, 0, 0, S_OK, &owner);
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::OwnershipRelease, 0, 0);
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::Destroy, 0, 0);
        session.Enable(35);
        GraphicsResourceChanged(&other, nullptr, kind, GraphicsSurfaceOperation::Rebind, 0, 0, S_OK, &owner);
        session.Enable(3);
        Destroyed(Domain::XamlCore, &owner);
        Born(Domain::XamlCore, &owner);
        session.Enable(35);
        GraphicsResourceChanged(&other, nullptr, kind, GraphicsSurfaceOperation::BaselinePresent, 0, 0, S_OK, &owner);
        session.Disable();
        Destroyed(Domain::XamlCore, &owner);
        Born(Domain::XamlCore, &owner);
        session.Enable(35);
        GraphicsResourceChanged(&other, nullptr, kind, GraphicsSurfaceOperation::BaselinePresent, 0, 0, S_OK, &owner);
        const auto observed = session.Read();
        CHECK(observed.Resources.size() == 7 && observed.LifecycleRecords.size() == 5 && observed.Devices.empty());
        if (observed.Resources.size() == 7 && observed.LifecycleRecords.size() == 5)
        {
            const auto& resource = observed.Resources;
            const auto& object = observed.LifecycleRecords;
            for (const auto& value : resource)
                CHECK(value.HasOwner && value.OwnerDomain == 0 && value.OwnerEpoch == value.Epoch);
            CHECK(resource[0].OwnerId == object[0].Object && resource[0].OwnerId != resource[1].OwnerId);
            CHECK(resource[2].OwnerId == resource[0].OwnerId && resource[3].OwnerId == resource[0].OwnerId);
            CHECK(resource[1].OwnerId == resource[4].OwnerId && resource[4].OwnerId == object[1].Object &&
                object[1].Operation == 2 && resource[1].Id == resource[4].Id);
            CHECK(resource[5].OwnerId == object[3].Object && resource[5].OwnerId != resource[4].OwnerId &&
                resource[5].Id != resource[4].Id && resource[5].Operation == 7);
            CHECK(resource[6].OwnerEpoch != resource[5].OwnerEpoch && resource[6].OwnerEpoch == object[4].Epoch &&
                resource[6].OwnerId == object[4].Object && resource[6].Operation == 7 && object[4].Operation == 2);
        }
        CHECK(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost);
    }
    else if (argc > 1 && std::strcmp(argv[1], "--brush-owner") == 0)
    {
#ifndef HAS_RESOURCE_HOOKS
        CHECK(false && "logical composition-brush owner hooks are not implemented");
#else
        const auto kind = GraphicsResourceKind::CompositionBrush;
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::Create, 0, 0);
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::BaselinePresent, 0, 0);
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::BaselinePresent, 0, 0);
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::Rebind, 0, 0);
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::OwnershipRelease, 0, 0);
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::Destroy, 0, 0);
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::Destroy, 0, 0);
        GraphicsResourceChanged(&surface, nullptr, kind, GraphicsSurfaceOperation::Create, 0, 0);
        GraphicsResourceChanged(&other, nullptr, kind, GraphicsSurfaceOperation::Create, 0, 0, E_FAIL);
        GraphicsResourceChanged(&late, nullptr, kind, GraphicsSurfaceOperation::BaselinePresent, 0, 0);
        GraphicsResourceChanged(&late, nullptr, kind, GraphicsSurfaceOperation::OwnershipRelease, 0, 0);
        GraphicsResourceChanged(&late, nullptr, kind, GraphicsSurfaceOperation::Destroy, 0, 0);
        const auto observed = session.Read();
        CHECK(observed.Resources.size() == 8 && observed.Devices.empty());
        if (observed.Resources.size() == 8)
        {
            const uint8_t expected[] = {0, 4, 2, 1, 0, 7, 2, 1};
            for (size_t i = 0; i != observed.Resources.size(); ++i)
            {
                const auto& resource = observed.Resources[i];
                CHECK(resource.Operation == expected[i] && resource.Kind == 3);
                CHECK(resource.Coverage == 1 && !resource.HasBacking && !resource.HasBytes);
                CHECK(resource.Width == 0 && resource.Height == 0);
            }
            CHECK(observed.Resources[0].Id == observed.Resources[3].Id &&
                observed.Resources[0].Id != observed.Resources[4].Id);
            CHECK(observed.Resources[0].DeviceEpoch != observed.Resources[4].DeviceEpoch &&
                observed.Resources[4].DeviceEpoch != observed.Resources[5].DeviceEpoch);
        }
        CHECK(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost);
#endif
    }
    else if (argc > 1 && std::strcmp(argv[1], "--texture-wrapper") == 0)
    {
#ifndef HAS_RESOURCE_HOOKS
        CHECK(false && "logical staging-texture resource hooks are not implemented");
#else
        GraphicsDeviceCreated(&device);
        GraphicsResourceChanged(&surface, &device, GraphicsResourceKind::Texture2D,
            GraphicsSurfaceOperation::Create, 64, 32);
        GraphicsResourceChanged(&surface, &device, GraphicsResourceKind::Texture2D,
            GraphicsSurfaceOperation::BaselinePresent, 64, 32);
        GraphicsResourceChanged(&surface, &device, GraphicsResourceKind::Texture2D,
            GraphicsSurfaceOperation::BaselinePresent, 64, 32);
        GraphicsResourceChanged(&other, nullptr, GraphicsResourceKind::Texture2D,
            GraphicsSurfaceOperation::Create, 1024, 512, E_FAIL);
        GraphicsResourceChanged(&surface, nullptr, GraphicsResourceKind::Texture2D,
            GraphicsSurfaceOperation::OwnershipRelease, 0, 0);
        GraphicsResourceChanged(&surface, nullptr, GraphicsResourceKind::Texture2D,
            GraphicsSurfaceOperation::Destroy, 0, 0);
        GraphicsResourceChanged(&surface, &device, GraphicsResourceKind::Texture2D,
            GraphicsSurfaceOperation::Create, 8, 4);
        GraphicsResourceChanged(&late, &device, GraphicsResourceKind::Texture2D,
            GraphicsSurfaceOperation::BaselinePresent, 48, 24);
        GraphicsResourceChanged(&late, &device, GraphicsResourceKind::Texture2D,
            GraphicsSurfaceOperation::BaselinePresent, 48, 24);
        GraphicsResourceChanged(&late, &device, GraphicsResourceKind::Texture2D,
            GraphicsSurfaceOperation::Resize, 128, 64);
        const auto observed = session.Read();
        CHECK(observed.Resources.size() == 6 && observed.Devices.size() == 1);
        if (observed.Resources.size() == 6)
        {
            const auto& resource = observed.Resources;
            const uint8_t expected[] = {0, 2, 1, 0, 7, 5};
            for (size_t i = 0; i != resource.size(); ++i)
            {
                CHECK(resource[i].Operation == expected[i]);
                CHECK(resource[i].Kind == 1 && resource[i].Coverage == 1 &&
                    !resource[i].HasBacking && !resource[i].HasBytes);
            }
            CHECK(resource[0].Id == resource[1].Id && resource[1].Id == resource[2].Id &&
                resource[0].Id != resource[3].Id);
            CHECK(resource[4].Id == resource[5].Id && resource[4].Operation == 7);
        }
        CHECK(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost);
#endif
    }
    else if (argc > 1 && (std::strcmp(argv[1], "--surface-capacity") == 0 ||
        std::strcmp(argv[1], "--device-capacity") == 0))
    {
        const bool surfaces = std::strcmp(argv[1], "--surface-capacity") == 0;
        constexpr size_t capacity = 65'536;
        std::vector<uint8_t> addresses(capacity + 1);
        if (surfaces) GraphicsDeviceCreated(&device);
        size_t emitted = 0;
        for (auto& address : addresses)
        {
            if (surfaces)
                GraphicsSurfaceChanged(&address, &device, GraphicsSurfaceOperation::Create, 1, 1);
            else
                GraphicsDeviceCreated(&address);
            if (++emitted % 256 == 0) session.Flush();
        }
        size_t deviceCount = 0, surfaceCount = 0;
        GetGraphicsRegistryCountsForTest(deviceCount, surfaceCount);
        CHECK(deviceCount == (surfaces ? 1 : capacity));
        CHECK(surfaceCount == (surfaces ? capacity : 0));
        ProtocolInfo info{};
        const HRESULT status = GetProtocolInfo(ProtocolVersion, sizeof(info), &info);
        std::printf("GRAPHICS_CAPACITY: devices=%zu surfaces=%zu status=0x%08lx\n",
            deviceCount, surfaceCount, static_cast<unsigned long>(status));
        CHECK(status == HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
        ShutdownRuntime();
        GetGraphicsRegistryCountsForTest(deviceCount, surfaceCount);
        CHECK(deviceCount == 0 && surfaceCount == 0);
    }
    else if (argc > 1 && (std::strcmp(argv[1], "--selection-race") == 0 ||
        std::strcmp(argv[1], "--epoch-race") == 0 || std::strcmp(argv[1], "--owner-selection-race") == 0 ||
        std::strcmp(argv[1], "--owner-epoch-race") == 0))
    {
        const bool epochRace = std::strcmp(argv[1], "--epoch-race") == 0 ||
            std::strcmp(argv[1], "--owner-epoch-race") == 0;
        const bool ownerRace = std::strstr(argv[1], "--owner-") == argv[1];
        int owner{};
        auto change = [&](const void* resource, GraphicsSurfaceOperation operation, uint32_t width, uint32_t height)
        {
            GraphicsResourceChanged(resource, &device, ownerRace ? GraphicsResourceKind::CompositionBrush :
                GraphicsResourceKind::CompositionSurface, operation, width, height, S_OK, ownerRace ? &owner : nullptr);
        };
        GraphicsDeviceCreated(&device);
        change(&surface, GraphicsSurfaceOperation::Create, 64, 32);
        HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE resume = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        LifecycleEtw::Require(ready && resume, "create graphics emission fences");
        ArmGraphicsEmissionFenceForTest(Event::Resource, ready, resume);
        std::thread observer([&]
        {
            change(&other, GraphicsSurfaceOperation::Create, 16, 8);
        });
        CHECK(WaitForSingleObject(ready, 10'000) == WAIT_OBJECT_0);
        if (epochRace) session.Disable(); else session.Enable(1);
        session.Enable(33);
        SetEvent(resume);
        observer.join();
        CloseHandle(ready); CloseHandle(resume);
        change(&other, GraphicsSurfaceOperation::Resize, 128, 64);
        ProtocolInfo info{};
        CHECK(GetProtocolInfo(ProtocolVersion, sizeof(info), &info) == S_OK);
        size_t deviceCount = 0, surfaceCount = 0;
        GetGraphicsRegistryCountsForTest(deviceCount, surfaceCount);
        CHECK(deviceCount == 1 && surfaceCount == 1);
        const auto observed = session.Read();
        CHECK(observed.Resources.size() == 3 && observed.Devices.size() == 2);
        if (observed.Resources.size() == 3 && observed.Devices.size() == 2)
        {
            const auto& resource = observed.Resources;
            CHECK(resource[0].Operation == 0 && resource[1].Operation == 7 && resource[2].Operation == 5);
            CHECK((resource[0].Epoch != resource[1].Epoch || resource[0].Id != resource[1].Id) &&
                resource[1].Id == resource[2].Id);
            CHECK((resource[0].Producer != resource[1].Producer) == epochRace);
            CHECK(resource[1].Sequence == (epochRace ? 1u : 3u) &&
                resource[2].Sequence == resource[1].Sequence + 1);
            CHECK(observed.Devices[0].Epoch != observed.Devices[1].Epoch &&
                observed.Devices[1].Operation == 3);
            if (ownerRace)
            {
                for (const auto& value : resource)
                    CHECK(value.HasOwner && value.OwnerDomain == 0 && value.OwnerEpoch == value.Epoch);
                CHECK(resource[1].OwnerId == resource[2].OwnerId &&
                    (resource[0].OwnerEpoch != resource[1].OwnerEpoch ||
                     resource[0].OwnerId != resource[1].OwnerId));
            }
        }
        CHECK(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost);
    }
    else if (argc > 1 && std::strcmp(argv[1], "--termination") == 0)
    {
        HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE resume = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        LifecycleEtw::Require(ready && resume && done, "create graphics termination fences");
        std::thread holder([&] { HoldEligibilityForTerminationTest(ready, resume); });
        CHECK(WaitForSingleObject(ready, 10'000) == WAIT_OBJECT_0);
        ShutdownRuntime(true);
        std::thread observer([&]
        {
            GraphicsDeviceCreated(&device);
            GraphicsDeviceReleased(&device, true);
            GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Create, 64, 32);
            SetEvent(done);
        });
        CHECK(WaitForSingleObject(done, 1000) == WAIT_OBJECT_0);
        SetEvent(resume);
        observer.join();
        holder.join();
        CloseHandle(ready); CloseHandle(resume); CloseHandle(done);
        const auto observed = session.Read();
        CHECK(observed.Resources.empty() && observed.Devices.empty());
    }
    else if (argc > 1 && std::strcmp(argv[1], "--external-device") == 0)
    {
        GraphicsSurfaceChanged(&surface, nullptr, GraphicsSurfaceOperation::Create, 64, 32);
        GraphicsSurfaceChanged(&other, nullptr, GraphicsSurfaceOperation::Create, 16, 8);
        GraphicsSurfaceChanged(&surface, nullptr, GraphicsSurfaceOperation::Resize, 128, 64);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Rebind, 128, 64);
        GraphicsSurfaceChanged(&other, &device, GraphicsSurfaceOperation::Rebind, 16, 8);
        GraphicsSurfaceChanged(&late, &device, GraphicsSurfaceOperation::Create, 100, 50, E_FAIL);
        GraphicsSurfaceChanged(&surface, nullptr, GraphicsSurfaceOperation::Resize, 100, 50, E_FAIL);
        GraphicsSurfaceChanged(&surface, nullptr, GraphicsSurfaceOperation::OwnershipRelease, 0, 0);
        GraphicsSurfaceChanged(&surface, nullptr, GraphicsSurfaceOperation::OwnershipRelease, 0, 0);
        GraphicsSurfaceChanged(&surface, nullptr, GraphicsSurfaceOperation::Destroy, 0, 0);
        GraphicsSurfaceChanged(&other, nullptr, GraphicsSurfaceOperation::Destroy, 0, 0);
        GraphicsDeviceReleased(&device, false);
        GraphicsDeviceReleased(&device, false);
        const auto observed = session.Read();
        CHECK(observed.Resources.size() == 8 && observed.Devices.size() == 2);
        if (observed.Resources.size() == 8 && observed.Devices.size() == 2)
        {
            const auto& resource = observed.Resources;
            const uint8_t expected[] = {0, 0, 5, 4, 4, 2, 1, 1};
            for (size_t i = 0; i != resource.size(); ++i)
            {
                CHECK(resource[i].Operation == expected[i]);
                CHECK(resource[i].Coverage == 1 && !resource[i].HasBacking && !resource[i].HasBytes);
            }
            CHECK(resource[0].DeviceEpoch != resource[1].DeviceEpoch);
            CHECK(resource[0].DeviceEpoch == resource[2].DeviceEpoch);
            CHECK(resource[0].Id == resource[3].Id && resource[1].Id == resource[4].Id);
            CHECK(resource[3].DeviceEpoch == resource[4].DeviceEpoch && resource[3].DeviceEpoch == observed.Devices[0].Epoch);
            CHECK(observed.Devices[0].Operation == 3 && observed.Devices[1].Operation == 4 &&
                observed.Devices[0].Epoch == observed.Devices[1].Epoch);
        }
        CHECK(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost);
    }
    else if (argc > 1 && (std::strcmp(argv[1], "--full-gap") == 0 ||
        std::strcmp(argv[1], "--runtime-restart") == 0))
    {
        const bool restart = std::strcmp(argv[1], "--runtime-restart") == 0;
        GraphicsDeviceCreated(&device);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Create, 64, 32);
        if (restart) ShutdownRuntime(); else session.Disable();
        GraphicsSurfaceChanged(&surface, nullptr, GraphicsSurfaceOperation::Destroy, 0, 0);
        GraphicsDeviceReleased(&device, false);
        GraphicsDeviceCreated(&device);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Create, 128, 64);
        if (restart) InitializeRuntime(); else session.Enable(33);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Resize, 256, 128);
        GraphicsSurfaceChanged(&surface, nullptr, GraphicsSurfaceOperation::Destroy, 0, 0);
        const auto observed = session.Read();
        CHECK(observed.Resources.size() == 4 && observed.Devices.size() == 2);
        if (observed.Resources.size() == 4 && observed.Devices.size() == 2)
        {
            const auto& resource = observed.Resources;
            CHECK(resource[0].Operation == 0 && resource[1].Operation == 7 &&
                resource[2].Operation == 5 && resource[3].Operation == 1);
            CHECK((resource[0].Epoch != resource[1].Epoch || resource[0].Id != resource[1].Id) &&
                resource[1].Id == resource[2].Id &&
                resource[2].Id == resource[3].Id);
            CHECK((resource[0].Producer == resource[1].Producer) == restart);
            CHECK(observed.Devices[0].Operation == 0 && observed.Devices[1].Operation == 3 &&
                observed.Devices[0].Epoch != observed.Devices[1].Epoch);
            CHECK(resource[0].DeviceEpoch != resource[1].DeviceEpoch &&
                resource[1].DeviceEpoch == observed.Devices[1].Epoch);
        }
        CHECK(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost);
    }
    else if (argc > 1 && std::strcmp(argv[1], "--selection-gap") == 0)
    {
        LifecycleEtw::Session keeper;
        keeper.Enable(2);
        GraphicsDeviceCreated(&device);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Create, 64, 32);
        ProtocolInfo before{};
        CHECK(GetProtocolInfo(ProtocolVersion, sizeof(before), &before) == S_OK);
        session.Enable(1);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Resize, 100, 80);
        session.Enable(33);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Resize, 128, 64);
        ProtocolInfo after{};
        CHECK(GetProtocolInfo(ProtocolVersion, sizeof(after), &after) == S_OK &&
            after.ProducerEpoch == before.ProducerEpoch);
        const auto observed = session.Read();
        CHECK(observed.Resources.size() == 3 && observed.Devices.size() == 2);
        if (observed.Resources.size() == 3 && observed.Devices.size() == 2)
        {
            CHECK(observed.Resources[0].Operation == 0 && observed.Resources[1].Operation == 7 &&
                observed.Resources[2].Operation == 5);
            CHECK(observed.Resources[0].Id != observed.Resources[1].Id &&
                observed.Resources[1].Id == observed.Resources[2].Id);
            CHECK(observed.Devices[0].Operation == 0 && observed.Devices[1].Operation == 3 &&
                observed.Devices[0].Epoch != observed.Devices[1].Epoch);
        }
        CHECK(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost);
    }
    else
    {
        SetLastError(314159);
        GraphicsDeviceCreated(&device);
        CHECK(GetLastError() == 314159);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Create, 64, 32);
        GraphicsSurfaceChanged(&other, &device, GraphicsSurfaceOperation::Create, 16, 8);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Rebind, 128, 64);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Resize, 256, 128);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Resize, 1024, 1024, E_FAIL);
        GraphicsSurfaceChanged(&surface, nullptr, GraphicsSurfaceOperation::OwnershipRelease, 0, 0);
        GraphicsSurfaceChanged(&surface, nullptr, GraphicsSurfaceOperation::Destroy, 0, 0);
        GraphicsSurfaceChanged(&surface, &device, GraphicsSurfaceOperation::Create, 8, 4);
        GraphicsSurfaceChanged(&late, &device, GraphicsSurfaceOperation::Resize, 48, 24);
        GraphicsDeviceReleased(&device, true);
        const auto observed = session.Read();
        CHECK(!observed.EventsLost && !observed.LogBuffersLost && !observed.RealTimeBuffersLost);
        CHECK(observed.Devices.size() == 3 && observed.Resources.size() == 9);
        if (observed.Devices.size() == 3)
        {
            CHECK(observed.Devices[0].Operation == 0 && observed.Devices[1].Operation == 1 &&
                observed.Devices[2].Operation == 4);
            CHECK(observed.Devices[0].Epoch == observed.Devices[1].Epoch &&
                observed.Devices[1].Epoch == observed.Devices[2].Epoch);
        }
        if (observed.Resources.size() == 9)
        {
            const auto& resource = observed.Resources;
            const uint8_t expected[] = {0, 0, 4, 5, 2, 1, 0, 7, 5};
            for (size_t i = 0; i != resource.size(); ++i)
            {
                CHECK(resource[i].Operation == expected[i]);
                CHECK(resource[i].Producer == resource[i].Epoch && resource[i].Sequence == i + 1);
                CHECK(resource[i].Kind == 2 && resource[i].Coverage == 1 &&
                    !resource[i].HasBacking && !resource[i].HasBytes);
            }
            CHECK(resource[0].Id == resource[2].Id && resource[2].Id == resource[3].Id &&
                resource[3].Id == resource[4].Id && resource[4].Id == resource[5].Id);
            CHECK(resource[0].Id != resource[1].Id && resource[0].Id != resource[6].Id);
            CHECK(resource[3].Width == 256 && resource[3].Height == 128);
            CHECK(resource[7].Operation == 7 && resource[7].Id == resource[8].Id &&
                resource[8].Operation == 5);
        }
    }
    ShutdownRuntime();
#endif
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
