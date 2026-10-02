// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include "WindowPlacementPublic.h"
#include "WindowPlacementStore.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    // Only ReadValue is exercised by the loader; the write operations are never reached.
    class StubSettingsBackend final : public IPlacementSettingsBackend
    {
    public:
        StorageOperationResult ReadResult{StorageResult::Success, S_OK};
        StoredValue Value;
        bool ReadCalled{false};

        StorageOperationResult ReadValue(
            const std::u16string&,
            const std::u16string&,
            StoredValue& value) override
        {
            ReadCalled = true;
            value = Value;
            return ReadResult;
        }

        StorageOperationResult ReplaceValue(
            const std::u16string&, const std::u16string&, const std::u16string&) override
        {
            VERIFY_FAIL(L"The detached loader must not write.");
            return {StorageResult::Failure, E_UNEXPECTED};
        }

        StorageOperationResult EnumerateValues(
            const std::u16string&, std::vector<StoredValue>&) override
        {
            VERIFY_FAIL(L"The detached loader must not enumerate.");
            return {StorageResult::Failure, E_UNEXPECTED};
        }

        StorageOperationResult DeleteValue(const std::u16string&, const std::u16string&) override
        {
            VERIFY_FAIL(L"The detached loader must not delete.");
            return {StorageResult::Failure, E_UNEXPECTED};
        }
    };

    long Load(StubSettingsBackend& backend, LoadResult& result)
    {
        return ReadPersistPlacement(backend, u"App", 3, u"detached", 8, result);
    }
}

class WindowPlacementDetachedLoaderTests
{
public:
    TEST_CLASS(WindowPlacementDetachedLoaderTests);

    TEST_METHOD(RejectsEmptyIdentityWithoutCreatingAPlacement)
    {
        LoadResult result;
        VERIFY_ARE_EQUAL(E_INVALIDARG, ReadPersistPlacement(nullptr, 0, result));
        VERIFY_IS_TRUE(result.Status == LoadStatus::Unexpected);
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(0), result.Placement.SaveSequence);
        VERIFY_ARE_EQUAL(0, result.Placement.Placement.NormalRect.X);
        VERIFY_ARE_EQUAL(0, result.Placement.Placement.NormalRect.Y);
        VERIFY_ARE_EQUAL(0, result.Placement.Placement.NormalRect.Width);
        VERIFY_ARE_EQUAL(0, result.Placement.Placement.NormalRect.Height);
        VERIFY_ARE_EQUAL(0, result.Placement.Placement.Dpi);
        VERIFY_IS_TRUE(result.Placement.Placement.DisplayDeviceName.empty());
    }

    TEST_METHOD(UnpackagedIdentityIsReportedAsUnavailable)
    {
        // This isolated test DLL is intentionally hosted unpackaged.
        LoadResult result;
        const HRESULT hr = ReadPersistPlacement(u"detached", 8, result);
        VERIFY_ARE_EQUAL(S_OK, hr);
        VERIFY_IS_TRUE(result.Status == LoadStatus::Unavailable);
        VERIFY_IS_TRUE(
            result.Error == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE) ||
            result.Error == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_APPLICATION));
    }

    TEST_METHOD(RejectsEmptyApplicationIdentityWithoutReadingStorage)
    {
        StubSettingsBackend backend;
        LoadResult result;
        VERIFY_ARE_EQUAL(E_INVALIDARG, ReadPersistPlacement(backend, nullptr, 0, u"detached", 8, result));
        VERIFY_IS_FALSE(backend.ReadCalled);
        VERIFY_IS_TRUE(result.Status == LoadStatus::Unexpected);
    }

    TEST_METHOD(StorageFailureIsReturnedAsAFailingHResult)
    {
        StubSettingsBackend backend;
        backend.ReadResult = {StorageResult::Failure, E_ACCESSDENIED};

        LoadResult result;
        VERIFY_ARE_EQUAL(E_ACCESSDENIED, Load(backend, result));
        VERIFY_IS_TRUE(result.Status == LoadStatus::Unexpected);
        VERIFY_ARE_EQUAL(E_ACCESSDENIED, result.Error);
    }

    TEST_METHOD(UnavailableStorageSucceedsWithoutAPlacement)
    {
        StubSettingsBackend backend;
        backend.ReadResult = {StorageResult::Unavailable, S_OK};

        LoadResult result;
        VERIFY_ARE_EQUAL(S_OK, Load(backend, result));
        VERIFY_IS_TRUE(result.Status == LoadStatus::Unavailable);
    }

    TEST_METHOD(MissingValueSucceedsWithoutAPlacement)
    {
        StubSettingsBackend backend;
        backend.ReadResult = {StorageResult::Missing, S_OK};

        LoadResult result;
        VERIFY_ARE_EQUAL(S_OK, Load(backend, result));
        VERIFY_IS_TRUE(result.Status == LoadStatus::Missing);
    }
};
