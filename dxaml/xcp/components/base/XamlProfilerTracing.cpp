// Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project
// root for license information.

#include "precomp.h"

// Safeguard: this profiler-only translation unit is gated on XamlProfilerEnabled (which defines
// XAMLPROFILER_ENABLED); the #error below turns an out-of-sync build into a clear diagnostic.
#ifndef XAMLPROFILER_ENABLED
#error "XamlProfilerTracing.cpp must only be compiled when XAMLPROFILER_ENABLED is defined (XamlProfilerEnabled=true in dxaml\Xaml.Cpp.Props); do not remove the XamlProfilerEnabled ClCompile condition in Microsoft.UI.Xaml.Base.vcxproj."
#endif // XAMLPROFILER_ENABLED

#include "XamlProfilerTracing.h"
#include <atomic>

// "Microsoft-Windows-XAML-Profiler" -> {26f28a5c-7d30-54c5-40f4-fff6d3beda7d}, the EventSource name
// hash of the provider name. No provider-group option: only sessions that enable this provider
// turn the instrumentation on.
TRACELOGGING_DEFINE_PROVIDER(
    g_hXamlProfilerProvider,
    "Microsoft-Windows-XAML-Profiler",
    (0x26f28a5c, 0x7d30, 0x54c5, 0x40, 0xf4, 0xff, 0xf6, 0xd3, 0xbe, 0xda, 0x7d));

namespace
{
    // TryMarkTypeFirstSeen's set: one bit per KnownTypeIndex below c_typeTableCapacity.
    std::atomic<uint64_t> s_typesSeen[XamlProfilerTracing::c_typeTableCapacity / 64] {};
}

namespace XamlProfilerTracing
{
    void NTAPI ProviderEnableCallback(
        _In_ LPCGUID /* sourceId */,
        ULONG controlCode,
        UCHAR /* level */,
        ULONGLONG /* matchAnyKeyword */,
        ULONGLONG /* matchAllKeyword */,
        _In_opt_ PEVENT_FILTER_DESCRIPTOR /* filterData */,
        _Inout_opt_ PVOID /* callbackContext */) noexcept
    {
        // EVENT_CONTROL_CODE_ENABLE_PROVIDER / EVENT_CONTROL_CODE_CAPTURE_STATE from evntrace.h,
        // which TraceLoggingProvider.h doesn't pull in.
        constexpr ULONG c_enableProvider = 1;
        constexpr ULONG c_captureState = 2;
        if (controlCode == c_enableProvider || controlCode == c_captureState)
        {
            for (std::atomic<uint64_t>& word : s_typesSeen)
            {
                word.store(0, std::memory_order_relaxed);
            }
        }
    }

    bool TryMarkTypeFirstSeen(uint32_t typeIndex) noexcept
    {
        if (typeIndex >= c_typeTableCapacity)
        {
            return true;
        }

        std::atomic<uint64_t>& word = s_typesSeen[typeIndex / 64];
        const uint64_t bit = uint64_t{ 1 } << (typeIndex % 64);
        if ((word.load(std::memory_order_relaxed) & bit) != 0)
        {
            return false;
        }
        return (word.fetch_or(bit, std::memory_order_relaxed) & bit) == 0;
    }
}

// Each emitter is noinline so the TraceLoggingWrite expansion (metadata, data descriptors and the
// write call) stays here rather than in the instrumented functions. TraceLoggingWrite rechecks the
// level and keyword itself, so an emitter reached with its group disabled writes nothing.
namespace XamlProfilerTracing
{
    __declspec(noinline) void ChildAdded(
        uint64_t parentId,
        uint64_t childId,
        uint32_t index,
        PCWSTR childLabel,
        bool isTemplateChild,
        uint64_t childPeerHandle) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "ChildAdded",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::XamlTree),
            TraceLoggingValue(parentId, "ParentId"),
            TraceLoggingValue(childId, "ChildId"),
            TraceLoggingValue(index, "Index"),
            TraceLoggingValue(childLabel, "ChildLabel"),
            TraceLoggingValue(isTemplateChild, "IsTemplateChild"),
            TraceLoggingValue(childPeerHandle, "ChildPeerHandle"));
    }

    __declspec(noinline) void ChildInserted(
        uint64_t parentId,
        uint64_t childId,
        uint32_t index,
        PCWSTR childLabel,
        bool isTemplateChild,
        uint64_t childPeerHandle) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "ChildInserted",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::XamlTree),
            TraceLoggingValue(parentId, "ParentId"),
            TraceLoggingValue(childId, "ChildId"),
            TraceLoggingValue(index, "Index"),
            TraceLoggingValue(childLabel, "ChildLabel"),
            TraceLoggingValue(isTemplateChild, "IsTemplateChild"),
            TraceLoggingValue(childPeerHandle, "ChildPeerHandle"));
    }

    __declspec(noinline) void ChildRemoved(
        uint64_t parentId,
        uint64_t childId) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "ChildRemoved",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::XamlTree),
            TraceLoggingValue(parentId, "ParentId"),
            TraceLoggingValue(childId, "ChildId"));
    }

    __declspec(noinline) void ElementEnteredTree(
        uint64_t elementId,
        uint64_t parentId,
        bool isLive,
        bool isTemplateChild,
        uint32_t typeIndex) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "ElementEnteredTree",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::XamlTree),
            TraceLoggingValue(elementId, "ElementId"),
            TraceLoggingValue(parentId, "ParentId"),
            TraceLoggingValue(isLive, "IsLive"),
            TraceLoggingValue(isTemplateChild, "IsTemplateChild"),
            TraceLoggingValue(typeIndex, "TypeIndex"));
    }

    __declspec(noinline) void ElementTypeInfo(
        uint32_t typeIndex,
        PCWSTR typeName,
        uint64_t coreSize) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "ElementTypeInfo",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::XamlTree),
            TraceLoggingValue(typeIndex, "TypeIndex"),
            TraceLoggingValue(typeName, "TypeName"),
            TraceLoggingValue(coreSize, "CoreSize"));
    }

    __declspec(noinline) void ElementLeftTree(
        uint64_t elementId,
        uint64_t parentId,
        bool isLive) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "ElementLeftTree",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::XamlTree),
            TraceLoggingValue(elementId, "ElementId"),
            TraceLoggingValue(parentId, "ParentId"),
            TraceLoggingValue(isLive, "IsLive"));
    }

    __declspec(noinline) void PeerAssociated(
        uint64_t elementId,
        uint64_t peerHandle) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "PeerAssociated",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::XamlTree),
            TraceLoggingValue(elementId, "ElementId"),
            TraceLoggingValue(peerHandle, "PeerHandle"));
    }

    __declspec(noinline) void ContentChanged(
        uint64_t parentId,
        uint64_t oldContentId,
        uint64_t newContentId,
        PCWSTR newContentLabel,
        uint64_t newContentPeerHandle) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "ContentChanged",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::XamlTree),
            TraceLoggingValue(parentId, "ParentId"),
            TraceLoggingValue(oldContentId, "OldContentId"),
            TraceLoggingValue(newContentId, "NewContentId"),
            TraceLoggingValue(newContentLabel, "NewContentLabel"),
            TraceLoggingValue(newContentPeerHandle, "NewContentPeerHandle"));
    }

    __declspec(noinline) void PopupOpened(
        uint64_t popupId,
        uint64_t childId,
        PCWSTR popupLabel,
        PCWSTR childLabel,
        uint64_t childPeerHandle) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "PopupOpened",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::XamlTree),
            TraceLoggingValue(popupId, "PopupId"),
            TraceLoggingValue(childId, "ChildId"),
            TraceLoggingValue(popupLabel, "PopupLabel"),
            TraceLoggingValue(childLabel, "ChildLabel"),
            TraceLoggingValue(childPeerHandle, "ChildPeerHandle"));
    }

    __declspec(noinline) void PopupClosed(
        uint64_t popupId,
        uint64_t childId) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "PopupClosed",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::XamlTree),
            TraceLoggingValue(popupId, "PopupId"),
            TraceLoggingValue(childId, "ChildId"));
    }

    __declspec(noinline) void CompPeerLinked(
        uint64_t uiElementId,
        uint64_t compNodeId,
        uint64_t parentCompNodeId,
        PCWSTR elementLabel,
        uint64_t uiElementPeerHandle) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "CompPeerLinked",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::CompTree),
            TraceLoggingValue(uiElementId, "UIElementId"),
            TraceLoggingValue(compNodeId, "CompNodeId"),
            TraceLoggingValue(parentCompNodeId, "ParentCompNodeId"),
            TraceLoggingValue(elementLabel, "ElementLabel"),
            TraceLoggingValue(uiElementPeerHandle, "UIElementPeerHandle"));
    }

    __declspec(noinline) void CompPeerUnlinked(
        uint64_t uiElementId,
        uint64_t compNodeId) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "CompPeerUnlinked",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::CompTree),
            TraceLoggingValue(uiElementId, "UIElementId"),
            TraceLoggingValue(compNodeId, "CompNodeId"));
    }

    __declspec(noinline) void CompNodeChildInserted(
        uint64_t parentCompNodeId,
        uint64_t childCompNodeId) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "CompNodeChildInserted",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::CompTree),
            TraceLoggingValue(parentCompNodeId, "ParentCompNodeId"),
            TraceLoggingValue(childCompNodeId, "ChildCompNodeId"));
    }

    __declspec(noinline) void CompNodeChildRemoved(
        uint64_t parentCompNodeId,
        uint64_t childCompNodeId) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "CompNodeChildRemoved",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::CompTree),
            TraceLoggingValue(parentCompNodeId, "ParentCompNodeId"),
            TraceLoggingValue(childCompNodeId, "ChildCompNodeId"));
    }

    __declspec(noinline) void WucVisualCreated(
        uint64_t visualId,
        uint32_t visualKind) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "WucVisualCreated",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::WucTree),
            TraceLoggingValue(visualId, "VisualId"),
            TraceLoggingValue(visualKind, "VisualKind"));
    }

    __declspec(noinline) void WucVisualChildInserted(
        uint64_t parentVisualId,
        uint64_t childVisualId,
        uint64_t ownerCompNodeId,
        int32_t index,
        uint32_t visualKind) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "WucVisualChildInserted",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::WucTree),
            TraceLoggingValue(parentVisualId, "ParentVisualId"),
            TraceLoggingValue(childVisualId, "ChildVisualId"),
            TraceLoggingValue(ownerCompNodeId, "OwnerCompNodeId"),
            TraceLoggingValue(index, "Index"),
            TraceLoggingValue(visualKind, "VisualKind"));
    }

    __declspec(noinline) void WucVisualChildRemoved(
        uint64_t parentVisualId,
        uint64_t childVisualId) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "WucVisualChildRemoved",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::WucTree),
            TraceLoggingValue(parentVisualId, "ParentVisualId"),
            TraceLoggingValue(childVisualId, "ChildVisualId"));
    }

    __declspec(noinline) void WucVisualChildrenCleared(
        uint64_t parentVisualId) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "WucVisualChildrenCleared",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::WucTree),
            TraceLoggingValue(parentVisualId, "ParentVisualId"));
    }

    __declspec(noinline) void WucVisualRootSet(
        uint64_t visualId,
        uint64_t targetId,
        uint64_t ownerCompNodeId,
        uint32_t visualKind) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "WucVisualRootSet",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::WucTree),
            TraceLoggingValue(visualId, "VisualId"),
            TraceLoggingValue(targetId, "TargetId"),
            TraceLoggingValue(ownerCompNodeId, "OwnerCompNodeId"),
            TraceLoggingValue(visualKind, "VisualKind"));
    }

    __declspec(noinline) void WucVisualRootCleared(
        uint64_t targetId) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "WucVisualRootCleared",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::WucTree),
            TraceLoggingValue(targetId, "TargetId"));
    }

    __declspec(noinline) void WucVisualPropertyChanged(
        uint64_t visualId,
        uint32_t property,
        float x,
        float y,
        float z,
        float w) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "WucVisualPropertyChanged",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::WucTree),
            TraceLoggingValue(visualId, "VisualId"),
            TraceLoggingValue(property, "Property"),
            TraceLoggingValue(x, "X"),
            TraceLoggingValue(y, "Y"),
            TraceLoggingValue(z, "Z"),
            TraceLoggingValue(w, "W"));
    }

    __declspec(noinline) void WucVisualTransformChanged(
        uint64_t visualId,
        _In_reads_(16) const float* matrix) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "WucVisualTransformChanged",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::WucTree),
            TraceLoggingValue(visualId, "VisualId"),
            TraceLoggingValue(matrix[0], "M11"),
            TraceLoggingValue(matrix[1], "M12"),
            TraceLoggingValue(matrix[2], "M13"),
            TraceLoggingValue(matrix[3], "M14"),
            TraceLoggingValue(matrix[4], "M21"),
            TraceLoggingValue(matrix[5], "M22"),
            TraceLoggingValue(matrix[6], "M23"),
            TraceLoggingValue(matrix[7], "M24"),
            TraceLoggingValue(matrix[8], "M31"),
            TraceLoggingValue(matrix[9], "M32"),
            TraceLoggingValue(matrix[10], "M33"),
            TraceLoggingValue(matrix[11], "M34"),
            TraceLoggingValue(matrix[12], "M41"),
            TraceLoggingValue(matrix[13], "M42"),
            TraceLoggingValue(matrix[14], "M43"),
            TraceLoggingValue(matrix[15], "M44"));
    }

    __declspec(noinline) void WucVisualObjectChanged(
        uint64_t visualId,
        uint32_t property,
        uint64_t objectId,
        PCWSTR typeName) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "WucVisualObjectChanged",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::WucTree),
            TraceLoggingValue(visualId, "VisualId"),
            TraceLoggingValue(property, "Property"),
            TraceLoggingValue(objectId, "ObjectId"),
            TraceLoggingValue(typeName, "TypeName"));
    }

    __declspec(noinline) void DCompSurfaceMemoryChanged(
        uint64_t surfaceId,
        bool isAllocation,
        uint64_t estimatedSizeBytes,
        uint32_t width,
        uint32_t height,
        uint32_t pixelStride,
        bool isVirtual) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "DCompSurfaceMemoryChanged",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::Surfaces),
            TraceLoggingValue(surfaceId, "SurfaceId"),
            TraceLoggingValue(isAllocation, "IsAllocation"),
            TraceLoggingValue(estimatedSizeBytes, "EstimatedSizeBytes"),
            TraceLoggingValue(width, "Width"),
            TraceLoggingValue(height, "Height"),
            TraceLoggingValue(pixelStride, "PixelStride"),
            TraceLoggingValue(isVirtual, "IsVirtual"));
    }

    __declspec(noinline) void DCompSurfaceTextureObserved(
        uint64_t surfaceId,
        uint64_t resourceId,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        uint32_t arraySize,
        uint32_t format,
        uint32_t sampleCount,
        int32_t offsetX,
        int32_t offsetY) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "DCompSurfaceTextureObserved",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::SurfaceTextures),
            TraceLoggingValue(surfaceId, "SurfaceId"),
            TraceLoggingValue(resourceId, "ResourceId"),
            TraceLoggingValue(width, "Width"),
            TraceLoggingValue(height, "Height"),
            TraceLoggingValue(mipLevels, "MipLevels"),
            TraceLoggingValue(arraySize, "ArraySize"),
            TraceLoggingValue(format, "Format"),
            TraceLoggingValue(sampleCount, "SampleCount"),
            TraceLoggingValue(offsetX, "OffsetX"),
            TraceLoggingValue(offsetY, "OffsetY"));
    }

    __declspec(noinline) void VisualSurfaceChanged(
        uint64_t visualId,
        uint64_t surfaceId,
        uint32_t role) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "VisualSurfaceChanged",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::Surfaces),
            TraceLoggingValue(visualId, "VisualId"),
            TraceLoggingValue(surfaceId, "SurfaceId"),
            TraceLoggingValue(role, "Role"));
    }

    __declspec(noinline) void XamlHeapSnapshot(
        uint64_t heapHandle,
        bool isPrivateHeap,
        uint64_t outstandingBytes,
        uint64_t outstandingAllocationCount,
        uint64_t cumulativeAllocatedBytes,
        uint64_t cumulativeAllocationCount,
        uint64_t cumulativeDeallocationCount) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "XamlHeapSnapshot",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::HeapSnapshots),
            TraceLoggingValue(heapHandle, "HeapHandle"),
            TraceLoggingValue(isPrivateHeap, "IsPrivateHeap"),
            TraceLoggingValue(outstandingBytes, "OutstandingBytes"),
            TraceLoggingValue(outstandingAllocationCount, "OutstandingAllocationCount"),
            TraceLoggingValue(cumulativeAllocatedBytes, "CumulativeAllocatedBytes"),
            TraceLoggingValue(cumulativeAllocationCount, "CumulativeAllocationCount"),
            TraceLoggingValue(cumulativeDeallocationCount, "CumulativeDeallocationCount"));
    }

    __declspec(noinline) void GpuMemorySnapshot(
        uint64_t localCurrentUsage,
        uint64_t localBudget,
        bool localAvailable,
        uint64_t nonLocalCurrentUsage,
        uint64_t nonLocalBudget,
        bool nonLocalAvailable) noexcept
    {
        TraceLoggingWrite(g_hXamlProfilerProvider, "GpuMemorySnapshot",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(XamlProfilerKeywords::GpuSnapshots),
            TraceLoggingValue(localCurrentUsage, "LocalCurrentUsage"),
            TraceLoggingValue(localBudget, "LocalBudget"),
            TraceLoggingValue(localAvailable, "LocalAvailable"),
            TraceLoggingValue(nonLocalCurrentUsage, "NonLocalCurrentUsage"),
            TraceLoggingValue(nonLocalBudget, "NonLocalBudget"),
            TraceLoggingValue(nonLocalAvailable, "NonLocalAvailable"));
    }
}
