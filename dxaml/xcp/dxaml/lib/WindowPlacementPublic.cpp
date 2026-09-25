// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementStore.h"
#include "WindowPlacementPublic.h"
#include <new>

namespace DirectUI::WindowPlacementPersistence
{
    namespace
    {
        void RecordLoadFailure(PlacementFailureScope& diagnostic, const LoadResult& result) noexcept
        {
            if (result.Status != LoadStatus::Loaded && result.Status != LoadStatus::Missing)
            {
                diagnostic.Record(result.Category, result.Error);
            }
        }
    }

    static _Check_return_ long ReadPersistPlacementCore(
        IPlacementSettingsBackend& backend,
        const char16_t* applicationId,
        size_t applicationIdLength,
        const char16_t* placementId,
        size_t placementIdLength,
        LoadResult& result)
    {
        result = {};
        if (!placementId || placementIdLength == 0 || !applicationId || applicationIdLength == 0)
        {
            result.Category = PlacementFailureCategory::Identity;
            return result.Error = E_INVALIDARG;
        }

        try
        {
            PlacementStore store(backend);
            result = store.Load(applicationId, applicationIdLength, placementId, placementIdLength);
            if (result.Status == LoadStatus::Unexpected && FAILED(result.Error))
            {
                return result.Error;
            }
            return S_OK;
        }
        catch (const std::bad_alloc&)
        {
            result.Status = LoadStatus::Unexpected;
            return result.Error = E_OUTOFMEMORY;
        }
    }

    _Check_return_ long ReadPersistPlacementCore(
        const char16_t* placementId,
        size_t placementIdLength,
        LoadResult& result)
    {
        result = {};
        result.Category = PlacementFailureCategory::Identity;
        if (!placementId || placementIdLength == 0) return result.Error = E_INVALIDARG;

        std::u16string applicationId;
        try
        {
            const HRESULT identity = TryGetPackagedApplicationId(applicationId);
            if (identity == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE) ||
                identity == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_APPLICATION))
            {
                result.Status = LoadStatus::Unavailable;
                result.Error = identity;
                return S_OK;
            }
            if (FAILED(identity)) return result.Error = identity;
        }
        catch (const std::bad_alloc&)
        {
            return result.Error = E_OUTOFMEMORY;
        }

        ApplicationDataPlacementSettingsBackend backend;
        return ReadPersistPlacementCore(
            backend,
            applicationId.data(),
            applicationId.size(),
            placementId,
            placementIdLength,
            result);
    }

    _Check_return_ long ReadPersistPlacement(
        IPlacementSettingsBackend& backend,
        const char16_t* applicationId,
        size_t applicationIdLength,
        const char16_t* placementId,
        size_t placementIdLength,
        LoadResult& result)
    {
        PlacementFailureScope diagnostic(PlacementOperation::Load);
        const auto hr = ReadPersistPlacementCore(
            backend, applicationId, applicationIdLength, placementId, placementIdLength, result);
        RecordLoadFailure(diagnostic, result);
        return hr;
    }

    _Check_return_ long ReadPersistPlacement(
        const char16_t* placementId,
        size_t placementIdLength,
        LoadResult& result)
    {
        PlacementFailureScope diagnostic(PlacementOperation::Load);
        const auto hr = ReadPersistPlacementCore(placementId, placementIdLength, result);
        RecordLoadFailure(diagnostic, result);
        return hr;
    }

    bool SavePersistPlacement(
        IPlacementSettingsBackend& backend,
        const Snapshot* placement,
        const char16_t* placementId,
        size_t placementIdLength,
        long (*getApplicationId)(std::u16string&) noexcept) noexcept
    {
        PlacementFailureScope diagnostic(PlacementOperation::Save);
        if (!placement)
        {
            diagnostic.Record(PlacementFailureCategory::Capture);
            return false;
        }

        auto category = PlacementFailureCategory::Identity;
        try
        {
            std::u16string applicationId;
            const auto hr = getApplicationId(applicationId);
            if (FAILED(hr))
            {
                diagnostic.Record(category, hr);
                return false;
            }
            category = PlacementFailureCategory::Write;
            PlacementStore store(backend);
            PlacementFailure failure;
            const bool saved = store.Save(applicationId.data(), applicationId.size(),
                placementId, placementIdLength, *placement, &failure);
            if (!saved) diagnostic.Record(failure.Category, failure.Error);
            return saved;
        }
        catch (const std::bad_alloc&)
        {
            diagnostic.Record(category, E_OUTOFMEMORY);
            return false;
        }
    }

    bool TryCopySnapshot(const Snapshot& source, Snapshot& destination) noexcept
    {
        try
        {
            auto copy = source;
            destination = std::move(copy);
            return true;
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }
    }

    bool TryCopyDeviceName(const char16_t* text, size_t length, std::u16string& destination) noexcept
    {
        if (text == nullptr && length != 0) return false;

        try
        {
            std::u16string copy;
            if (length != 0)
            {
                copy.assign(text, length);
            }
            destination = std::move(copy);
            return true;
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }
    }
}
