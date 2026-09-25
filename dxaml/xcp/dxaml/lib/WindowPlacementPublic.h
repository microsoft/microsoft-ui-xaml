// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowPlacementRecord.h"

namespace DirectUI::WindowPlacementPersistence
{
    struct LoadResult;
    class IPlacementSettingsBackend;

    // Allocation boundary, compiled with C++ exception handling independently of the XAML PCH.
    bool TryCopySnapshot(const Snapshot& source, Snapshot& destination) noexcept;
    bool TryCopyDeviceName(const char16_t* text, size_t length, std::u16string& destination) noexcept;

    // This layer owns turning a failed load into a failing HRESULT. Callers that get S_OK
    // can trust LoadResult::Status and never need to re-check LoadResult::Error.
    _Check_return_ long ReadPersistPlacement(
        const char16_t* placementId,
        size_t placementIdLength,
        LoadResult& result);

    // Same contract, with the storage backend and application identity supplied by the
    // caller. This is the seam tests use to reach the storage failure path.
    _Check_return_ long ReadPersistPlacement(
        IPlacementSettingsBackend& backend,
        const char16_t* applicationId,
        size_t applicationIdLength,
        const char16_t* placementId,
        size_t placementIdLength,
        LoadResult& result);

    // Shared reader without reporting: automatic restore owns the boundary through apply.
    _Check_return_ long ReadPersistPlacementCore(
        const char16_t* placementId,
        size_t placementIdLength,
        LoadResult& result);

    // A null snapshot means capture produced no usable placement. Identity is injectable
    // without exposing either it or the placement to the reporting seam.
    bool SavePersistPlacement(
        IPlacementSettingsBackend& backend,
        const Snapshot* placement,
        const char16_t* placementId,
        size_t placementIdLength,
        long (*getApplicationId)(std::u16string&) noexcept) noexcept;

    // Value-only boundary between the public projections and the future coordinator.
    // The coordinator must classify late/reentrant calls before requesting validation.
    struct InitialRequest
    {
        std::optional<Snapshot> Placement;
        int32_t Reason{};
        int32_t CascadeBehavior{};
        bool DoNotActivate{};
        bool SkipInitialPlacement{};
    };

    inline bool IsValidInitialRequest(const InitialRequest& request, bool isHiddenApplication) noexcept
    {
        return request.Reason >= 0 && request.Reason <= 2 &&
            request.CascadeBehavior >= 0 && request.CascadeBehavior <= 2 &&
            !(request.SkipInitialPlacement && (isHiddenApplication || request.Placement.has_value())) &&
            (!request.Placement || IsValid(*request.Placement));
    }
}
