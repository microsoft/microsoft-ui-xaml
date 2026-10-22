// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

#include <windows.h>
#include <unknwn.h>
#include <inspectable.h>
#include <cstdint>

namespace WinUIProfiler
{
    constexpr uint32_t ProtocolVersion = 1;
    constexpr uint8_t EventVersion = 1;
    constexpr uint32_t MaximumWireBytes = 48 * 1024;
    constexpr GUID RuntimeProviderId = {0x43a73524, 0xbc37, 0x4f6e, {0x9b, 0xa8, 0x84, 0xb5, 0x34, 0x6f, 0x61, 0xc2}};

    enum class Keyword : uint64_t
    {
        Capabilities = 1, Objects = 2, XamlTree = 4, Composition = 8,
        Frames = 16, Resources = 32, Allocator = 64, Capture = 128, Metrics = 256
    };

    enum class Event : uint16_t
    {
        RuntimeCapabilities = 1, ObjectLifetime, TypeResolved, CorePeerAssociation,
        TreeEdge, VisualDelta, Correlation, Boundary, Frame, Device, Resource,
        Allocation, CaptureBoundary, CaptureChunk, Loss, XamlPropertyChanged
    };

    enum class Domain : uint8_t { XamlCore, XamlPeer, CompNode, CompositionVisual, Resource };

    // Private COM ABI: naturally aligned to 8 on both supported architectures.
    // Size/version are inputs; outputs never contain caller-owned pointers.
#pragma pack(push, 8)
    struct ProcessIdentity
    {
        uint32_t ProcessId;
        uint32_t HasCreationTime;
        int64_t CreationTimeFileTime;
        GUID InstanceId;
    };

    struct ProtocolInfo
    {
        uint32_t Size;
        uint32_t Version;
        GUID ProducerEpoch;
        ProcessIdentity Process;
        int64_t QpcFrequency;
        uint64_t SupportedEvents;
        uint32_t Architecture;
        uint32_t CapabilityCount;
    };

    struct SequenceCut
    {
        uint32_t Size;
        uint32_t Version;
        GUID CutId;
        GUID ProducerEpoch;
        ProcessIdentity Process;
        uint64_t ReservedThrough;
        uint64_t SettledThrough;
        uint16_t EventId;
        uint8_t EventSchemaVersion;
        uint8_t Reserved[5];
    };

    static_assert(sizeof(ProcessIdentity) == 32);
    static_assert(sizeof(ProtocolInfo) == 80);
    static_assert(sizeof(SequenceCut) == 96);
#pragma pack(pop)

    MIDL_INTERFACE("289e7441-674b-45fa-a276-b9f59b7ec233")
    IWinUIProfilerRuntimeDiagnostics : IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE GetProfilerProtocolInfo(uint32_t version, uint32_t size, ProtocolInfo* info) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetProfilerSequenceCut(uint32_t version, uint16_t eventId, uint32_t size, SequenceCut* cut) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetProfilerDispatcher(uint32_t version, uint32_t threadId, IInspectable** dispatcher) = 0;
        virtual HRESULT STDMETHODCALLTYPE RequestProfilerCapture(uint32_t version, uint32_t kind, GUID captureId) = 0;
    };
}
