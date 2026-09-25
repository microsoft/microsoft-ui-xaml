// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowPlacementStorageFormat.h"
#include "WindowPlacementDiagnostics.h"

#include <cstdint>
#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

namespace DirectUI::WindowPlacementPersistence
{
    enum class StorageResult
    {
        Success,
        Missing,
        WrongType,
        Unavailable,
        Failure
    };

    struct StorageOperationResult
    {
        StorageResult Status{StorageResult::Failure};
        HRESULT Error{E_FAIL};
    };

    struct StoredValue
    {
        std::u16string Name;
        bool IsString{true};
        std::u16string Text;
    };

    // This is the narrow ABI boundary used by the store adapter. The packaged
    // ApplicationData implementation supplies these operations; tests can use
    // an in-memory implementation without creating a package container.
    class IPlacementSettingsBackend
    {
    public:
        virtual ~IPlacementSettingsBackend() = default;
        virtual StorageOperationResult ReadValue(
            const std::u16string& containerName,
            const std::u16string& valueName,
            StoredValue& value) = 0;
        virtual StorageOperationResult ReplaceValue(
            const std::u16string& containerName,
            const std::u16string& valueName,
            const std::u16string& text) = 0;
        virtual StorageOperationResult EnumerateValues(
            const std::u16string& containerName,
            std::vector<StoredValue>& values) = 0;
        virtual StorageOperationResult DeleteValue(
            const std::u16string& containerName,
            const std::u16string& valueName) = 0;
    };

    enum class LoadStatus
    {
        Loaded,
        Missing,
        Unavailable,
        Invalid,
        Unexpected
    };

    struct LoadResult
    {
        LoadStatus Status{LoadStatus::Unexpected};
        Record Placement{};
        HRESULT Error{E_FAIL};
        PlacementFailureCategory Category{PlacementFailureCategory::Read};
    };

    // The packaged ApplicationData.LocalSettings implementation. Reads only open
    // existing containers; writes create the placement container as needed.
    class ApplicationDataPlacementSettingsBackend final : public IPlacementSettingsBackend
    {
    public:
        StorageOperationResult ReadValue(
            const std::u16string& containerName,
            const std::u16string& valueName,
            StoredValue& value) override;
        StorageOperationResult ReplaceValue(
            const std::u16string& containerName,
            const std::u16string& valueName,
            const std::u16string& text) override;
        StorageOperationResult EnumerateValues(
            const std::u16string& containerName,
            std::vector<StoredValue>& values) override;
        StorageOperationResult DeleteValue(
            const std::u16string& containerName,
            const std::u16string& valueName) override;
    };

    class PlacementStore final
    {
    public:
        explicit PlacementStore(IPlacementSettingsBackend& backend) noexcept;

        LoadResult Load(
            const char16_t* applicationId,
            size_t applicationIdLength,
            const char16_t* placementId,
            size_t placementIdLength) const;

        // A successful replacement is reported as true even if best-effort
        // retention cleanup cannot delete an old value.
        bool Save(
            const char16_t* applicationId,
            size_t applicationIdLength,
            const char16_t* placementId,
            size_t placementIdLength,
            const Snapshot& placement,
            PlacementFailure* failure = nullptr);

    private:
        IPlacementSettingsBackend& m_backend;
    };

    // Resolves only a registered packaged application identity. Failure never
    // falls back to package-wide or unpackaged storage.
    HRESULT TryGetPackagedApplicationId(std::u16string& applicationId) noexcept;

    // ApplicationDataContainerSettings reports a missing container or value as
    // S_OK with a null result instead of E_BOUNDS, so both forms must be treated
    // as missing. Callers dereference the result, so a null one must never be
    // reported as success. A failure HRESULT other than E_BOUNDS is not missing;
    // the caller classifies it.
    bool IsMissingLookupResult(HRESULT lookupResult, bool hasResult) noexcept;
}
