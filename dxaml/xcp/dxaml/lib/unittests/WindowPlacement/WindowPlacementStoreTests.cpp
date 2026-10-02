// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include "WindowPlacementStore.h"

#include <map>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <string>
#include <thread>

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    class MemorySettingsBackend final : public IPlacementSettingsBackend
    {
    public:
        std::map<std::u16string, std::map<std::u16string, StoredValue>> Containers;
        bool FailReplace{false};
        HRESULT ReadError{S_OK};
        bool FailDelete{false};
        size_t DeleteSuccessesBeforeFailure{SIZE_MAX};
        size_t DeleteCount{0};
        std::u16string LastReadContainer;

        StorageOperationResult ReadValue(
            const std::u16string& container,
            const std::u16string& name,
            StoredValue& value) override
        {
            LastReadContainer = container;
            if (FAILED(ReadError)) return {StorageResult::Failure, ReadError};
            const auto containerIt = Containers.find(container);
            if (containerIt == Containers.end()) return {StorageResult::Missing, S_OK};
            const auto valueIt = containerIt->second.find(name);
            if (valueIt == containerIt->second.end()) return {StorageResult::Missing, S_OK};
            value = valueIt->second;
            return value.IsString ?
                StorageOperationResult{StorageResult::Success, S_OK} :
                StorageOperationResult{StorageResult::WrongType, S_OK};
        }

        StorageOperationResult ReplaceValue(
            const std::u16string& container,
            const std::u16string& name,
            const std::u16string& text) override
        {
            if (FailReplace) return {StorageResult::Failure, E_FAIL};
            Containers[container][name] = {name, true, text};
            return {StorageResult::Success, S_OK};
        }

        StorageOperationResult EnumerateValues(
            const std::u16string& container,
            std::vector<StoredValue>& values) override
        {
            const auto containerIt = Containers.find(container);
            if (containerIt == Containers.end()) return {StorageResult::Missing, S_OK};
            for (const auto& [name, value] : containerIt->second) values.push_back(value);
            return {StorageResult::Success, S_OK};
        }

        StorageOperationResult DeleteValue(
            const std::u16string& container,
            const std::u16string& name) override
        {
            if (FailDelete || DeleteCount >= DeleteSuccessesBeforeFailure)
            {
                return {StorageResult::Failure, E_FAIL};
            }
            ++DeleteCount;
            const auto containerIt = Containers.find(container);
            if (containerIt != Containers.end()) containerIt->second.erase(name);
            return {StorageResult::Success, S_OK};
        }
    };

    class AtomicMemorySettingsBackend final : public IPlacementSettingsBackend
    {
    public:
        StorageOperationResult ReadValue(
            const std::u16string& container,
            const std::u16string& name,
            StoredValue& value) override
        {
            std::lock_guard lock(m_lock);
            const auto containerIt = Containers.find(container);
            if (containerIt == Containers.end()) return {StorageResult::Missing, S_OK};
            const auto valueIt = containerIt->second.find(name);
            if (valueIt == containerIt->second.end()) return {StorageResult::Missing, S_OK};
            value = valueIt->second;
            return value.IsString ?
                StorageOperationResult{StorageResult::Success, S_OK} :
                StorageOperationResult{StorageResult::WrongType, S_OK};
        }

        StorageOperationResult ReplaceValue(
            const std::u16string& container,
            const std::u16string& name,
            const std::u16string& text) override
        {
            std::unique_lock lock(m_lock);
            if (PauseNextReplace)
            {
                PauseNextReplace = false;
                ReplaceEntered = true;
                ReplaceEnteredCondition.notify_one();
                ContinueCondition.wait(lock, [this] { return ContinueReplace; });
            }
            Containers[container][name] = {name, true, text};
            return {StorageResult::Success, S_OK};
        }

        StorageOperationResult EnumerateValues(
            const std::u16string& container,
            std::vector<StoredValue>& values) override
        {
            std::lock_guard lock(m_lock);
            const auto containerIt = Containers.find(container);
            if (containerIt == Containers.end()) return {StorageResult::Missing, S_OK};
            for (const auto& [name, value] : containerIt->second) values.push_back(value);
            return {StorageResult::Success, S_OK};
        }

        StorageOperationResult DeleteValue(
            const std::u16string& container,
            const std::u16string& name) override
        {
            std::lock_guard lock(m_lock);
            const auto containerIt = Containers.find(container);
            if (containerIt != Containers.end()) containerIt->second.erase(name);
            return {StorageResult::Success, S_OK};
        }

        std::u16string StoredText(const std::u16string& container, const std::u16string& name)
        {
            std::lock_guard lock(m_lock);
            return Containers.at(container).at(name).Text;
        }

        void WaitForReplaceEntry()
        {
            std::unique_lock lock(m_lock);
            ReplaceEnteredCondition.wait(lock, [this] { return ReplaceEntered; });
        }

        void ReleaseReplace()
        {
            std::lock_guard lock(m_lock);
            ContinueReplace = true;
            ContinueCondition.notify_one();
        }

        std::map<std::u16string, std::map<std::u16string, StoredValue>> Containers;
        bool PauseNextReplace{false};

    private:
        std::mutex m_lock;
        std::condition_variable ReplaceEnteredCondition;
        std::condition_variable ContinueCondition;
        bool ReplaceEntered{false};
        bool ContinueReplace{false};
    };

    Snapshot Sample()
    {
        Snapshot placement;
        placement.NormalRect = {-20, -10, 40, 30};
        placement.WorkArea = {-100, -50, 200, 100};
        placement.Dpi = 96;
        return placement;
    }

    Snapshot SampleFor(int32_t seed)
    {
        Snapshot placement = Sample();
        placement.NormalRect = {seed * 10, seed * -10, seed * 20, seed * 30};
        placement.WorkArea = {seed * -100, seed * -50, seed * 200, seed * 100};
        placement.Dpi = 96 + seed;
        return placement;
    }

    bool SameSnapshot(const Snapshot& left, const Snapshot& right)
    {
        const bool sameSnapRect = left.SnapRect.has_value() == right.SnapRect.has_value() &&
            (!left.SnapRect.has_value() ||
                (left.SnapRect->X == right.SnapRect->X &&
                 left.SnapRect->Y == right.SnapRect->Y &&
                 left.SnapRect->Width == right.SnapRect->Width &&
                 left.SnapRect->Height == right.SnapRect->Height));
        return left.NormalRect.X == right.NormalRect.X &&
            left.NormalRect.Y == right.NormalRect.Y &&
            left.NormalRect.Width == right.NormalRect.Width &&
            left.NormalRect.Height == right.NormalRect.Height &&
            left.WorkArea.X == right.WorkArea.X &&
            left.WorkArea.Y == right.WorkArea.Y &&
            left.WorkArea.Width == right.WorkArea.Width &&
            left.WorkArea.Height == right.WorkArea.Height &&
            left.Dpi == right.Dpi &&
            left.PlacementState == right.PlacementState &&
            sameSnapRect &&
            left.DisplayDeviceName == right.DisplayDeviceName &&
            left.VirtualDesktopId == right.VirtualDesktopId;
    }

    std::u16string ContainerName()
    {
        std::u16string name;
        VERIFY_IS_TRUE(TryMakeApplicationContainerName(u"App", 3, name));
        return name;
    }

    std::u16string ValueName(const std::u16string& id)
    {
        std::u16string name;
        VERIFY_IS_TRUE(TryMakePlacementValueName(id.c_str(), id.size(), name));
        return name;
    }

    std::u16string NumberedId(const char16_t* prefix, int number)
    {
        const auto suffix = std::to_string(number);
        return std::u16string(prefix) + std::u16string(suffix.begin(), suffix.end());
    }

    void AddRecord(MemorySettingsBackend& backend, const std::u16string& id, uint64_t sequence)
    {
        Record record;
        record.Placement = Sample();
        record.SaveSequence = sequence;
        std::u16string text;
        VERIFY_IS_TRUE(EncodeRecordText(record, text));
        backend.Containers[ContainerName()][ValueName(id)] = {ValueName(id), true, text};
    }

    // A record whose header declares a major version this build does not support.
    void AddUnsupportedFormatRecord(MemorySettingsBackend& backend, const std::u16string& id)
    {
        Record record;
        record.Placement = Sample();
        record.SaveSequence = 1;
        std::u16string text;
        VERIFY_IS_TRUE(EncodeRecordText(record, text));
        text[6] = u'I'; // Unsupported major version in the encoded header.

        Record decoded;
        VERIFY_IS_TRUE(DecodeRecordText(text.c_str(), text.size(), decoded) == DecodeResult::UnsupportedVersion);
        backend.Containers[ContainerName()][ValueName(id)] = {ValueName(id), true, text};
    }
}

class WindowPlacementStoreTests
{
public:
    TEST_CLASS(WindowPlacementStoreTests);

    TEST_METHOD(MissingLookupResultTreatsNullSuccessAsMissing)
    {
        // ApplicationDataContainerSettings reports a missing container or value
        // as S_OK with a null result. Reporting that as success handed a null
        // pointer to the caller, which dereferenced it.
        VERIFY_IS_TRUE(IsMissingLookupResult(S_OK, false));
        VERIFY_IS_FALSE(IsMissingLookupResult(S_OK, true));
    }

    TEST_METHOD(MissingLookupResultTreatsEBoundsAsMissing)
    {
        VERIFY_IS_TRUE(IsMissingLookupResult(E_BOUNDS, false));
        VERIFY_IS_TRUE(IsMissingLookupResult(E_BOUNDS, true));
    }

    TEST_METHOD(MissingLookupResultLeavesOtherFailuresToTheCaller)
    {
        // A failure HRESULT leaves the result null, but it is not missing. The
        // caller must still classify it as Unavailable or Failure.
        VERIFY_IS_FALSE(IsMissingLookupResult(E_ACCESSDENIED, false));
        VERIFY_IS_FALSE(IsMissingLookupResult(E_OUTOFMEMORY, false));
        VERIFY_IS_FALSE(IsMissingLookupResult(E_FAIL, true));
    }

    TEST_METHOD(MissingLoadDoesNotCreateContainer)
    {
        MemorySettingsBackend backend;
        PlacementStore store(backend);
        const auto result = store.Load(u"App", 3, u"Missing", 7);
        VERIFY_IS_TRUE(result.Status == LoadStatus::Missing);
        VERIFY_IS_TRUE(backend.Containers.empty());
    }

    TEST_METHOD(DistinguishesInvalidValues)
    {
        MemorySettingsBackend backend;
        const auto container = ContainerName();
        backend.Containers[container][ValueName(u"Type")] = {ValueName(u"Type"), false, {}};
        backend.Containers[container][ValueName(u"Large")] =
            {ValueName(u"Large"), true, std::u16string(MaximumEncodedCharacters + 1, u'A')};
        PlacementStore store(backend);

        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Type", 4).Status == LoadStatus::Invalid);
        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Large", 5).Status == LoadStatus::Invalid);
    }

    TEST_METHOD(SaveUsesPositiveSequenceAndReplacesWholeValue)
    {
        MemorySettingsBackend backend;
        AddRecord(backend, u"Old", 7);
        PlacementStore store(backend);
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"New", 3, Sample()));

        const auto result = store.Load(u"App", 3, u"New", 3);
        VERIFY_IS_TRUE(result.Status == LoadStatus::Loaded);
        VERIFY_ARE_EQUAL(uint64_t{8}, result.Placement.SaveSequence);
    }

    TEST_METHOD(ThirtyOneThirtyTwoThirtyThreeRetention)
    {
        MemorySettingsBackend backend;
        for (int i = 0; i < 31; ++i) AddRecord(backend, NumberedId(u"Old", i), i + 1);
        PlacementStore store(backend);
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"ThirtyTwo", 9, Sample()));
        VERIFY_ARE_EQUAL(size_t{32}, backend.Containers[ContainerName()].size());

        VERIFY_IS_TRUE(store.Save(u"App", 3, u"ThirtyThree", 11, Sample()));
        VERIFY_ARE_EQUAL(size_t{32}, backend.Containers[ContainerName()].size());
        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Old0", 4).Status == LoadStatus::Missing);
    }

    TEST_METHOD(LoadDoesNotRefreshRecency)
    {
        MemorySettingsBackend backend;
        AddRecord(backend, u"Old", 1);
        PlacementStore store(backend);
        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Old", 3).Status == LoadStatus::Loaded);
        for (int i = 0; i < 32; ++i) AddRecord(backend, NumberedId(u"Fill", i), i + 2);
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"Newest", 7, Sample()));
        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Old", 3).Status == LoadStatus::Missing);
    }

    TEST_METHOD(FailedReplacementLeavesOldValue)
    {
        MemorySettingsBackend backend;
        PlacementStore store(backend);
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"Stable", 6, Sample()));
        const auto before = store.Load(u"App", 3, u"Stable", 6);
        backend.FailReplace = true;
        VERIFY_IS_FALSE(store.Save(u"App", 3, u"Stable", 6, Sample()));
        const auto after = store.Load(u"App", 3, u"Stable", 6);
        VERIFY_ARE_EQUAL(before.Placement.SaveSequence, after.Placement.SaveSequence);
    }

    TEST_METHOD(UnexpectedLoadPreservesBackendError)
    {
        MemorySettingsBackend backend;
        backend.ReadError = RO_E_CLOSED;
        PlacementStore store(backend);
        const auto result = store.Load(u"App", 3, u"Broken", 6);
        VERIFY_IS_TRUE(result.Status == LoadStatus::Unexpected);
        VERIFY_ARE_EQUAL(RO_E_CLOSED, result.Error);
    }

    TEST_METHOD(ApplicationDataBackendReportsUnavailableWithoutPackage)
    {
        ApplicationDataPlacementSettingsBackend backend;
        StoredValue value;
        const auto result = backend.ReadValue(u"missing", u"value", value);
        VERIFY_IS_TRUE(result.Status == StorageResult::Unavailable);
        VERIFY_IS_TRUE(
            result.Error == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE) ||
            result.Error == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_APPLICATION));
    }

    TEST_METHOD(PackagedIdentityPreservesNoPackageError)
    {
        std::u16string applicationId;
        const auto result = TryGetPackagedApplicationId(applicationId);
        VERIFY_IS_TRUE(
            result == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE) ||
            result == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_APPLICATION));
        VERIFY_IS_TRUE(applicationId.empty());
    }

    TEST_METHOD(CleanupFailureDoesNotUndoSave)
    {
        MemorySettingsBackend backend;
        for (int i = 0; i < 32; ++i) AddRecord(backend, NumberedId(u"Old", i), i + 1);
        backend.FailDelete = true;
        PlacementStore store(backend);
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"Newest", 7, Sample()));
        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Newest", 7).Status == LoadStatus::Loaded);
    }

    TEST_METHOD(RetentionLeavesUnrelatedValuesAlone)
    {
        MemorySettingsBackend backend;
        backend.Containers[ContainerName()][u"AppOwnedValue"] =
            {u"AppOwnedValue", true, u"keep"};
        for (int i = 0; i < 32; ++i) AddRecord(backend, NumberedId(u"Old", i), i + 1);

        PlacementStore store(backend);
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"Newest", 7, Sample()));
        const auto& values = backend.Containers[ContainerName()];
        VERIFY_IS_TRUE(values.find(u"AppOwnedValue") != values.end());
        VERIFY_ARE_EQUAL(std::u16string(u"keep"), values.at(u"AppOwnedValue").Text);
    }

    TEST_METHOD(SequenceOverflowFailsWithoutReplacement)
    {
        MemorySettingsBackend backend;
        AddRecord(backend, u"Max", (std::numeric_limits<uint64_t>::max)());
        PlacementStore store(backend);
        VERIFY_IS_FALSE(store.Save(u"App", 3, u"Next", 4, Sample()));
        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Next", 4).Status == LoadStatus::Missing);
    }

    // An unsupported major format cannot be ordered, so the save and the cleanup
    // are both deferred rather than guessing which record is oldest.
    TEST_METHOD(UnsupportedFormatDefersSaveAndCleanup)
    {
        MemorySettingsBackend backend;
        for (int i = 0; i < 32; ++i) AddRecord(backend, NumberedId(u"Old", i), i + 1);
        AddUnsupportedFormatRecord(backend, u"Future");
        const auto before = backend.Containers[ContainerName()].size();

        PlacementStore store(backend);
        VERIFY_IS_FALSE(store.Save(u"App", 3, u"Newest", 7, Sample()));
        VERIFY_ARE_EQUAL(before, backend.Containers[ContainerName()].size());
        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Newest", 7).Status == LoadStatus::Missing);
        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Old0", 4).Status == LoadStatus::Loaded);
        VERIFY_ARE_EQUAL(size_t{0}, backend.DeleteCount);

        // The same store saves normally once the unreadable record is gone.
        backend.Containers[ContainerName()].erase(ValueName(u"Future"));
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"Newest", 7, Sample()));
    }

    // A corrupt record is still evicted first; only an unsupported major format defers.
    TEST_METHOD(CorruptRecordIsEvictedFirst)
    {
        MemorySettingsBackend backend;
        for (int i = 0; i < 31; ++i) AddRecord(backend, NumberedId(u"Old", i), i + 1);
        backend.Containers[ContainerName()][ValueName(u"Corrupt")] =
            {ValueName(u"Corrupt"), true, u"@@@@notbase64@@@@"};

        PlacementStore store(backend);
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"Newest", 7, Sample()));
        VERIFY_ARE_EQUAL(size_t{32}, backend.Containers[ContainerName()].size());
        const auto& values = backend.Containers[ContainerName()];
        VERIFY_IS_TRUE(values.find(ValueName(u"Corrupt")) == values.end());
        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Old0", 4).Status == LoadStatus::Loaded);
    }

    // Cleanup stops at the first deletion failure instead of working through a
    // victim list that a later save must recompute anyway.
    TEST_METHOD(CleanupStopsAtFirstDeletionFailure)
    {
        MemorySettingsBackend backend;
        for (int i = 0; i < 34; ++i) AddRecord(backend, NumberedId(u"Old", i), i + 1);
        backend.DeleteSuccessesBeforeFailure = 1;

        PlacementStore store(backend);
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"Newest", 7, Sample()));
        VERIFY_ARE_EQUAL(size_t{1}, backend.DeleteCount);
        VERIFY_ARE_EQUAL(size_t{34}, backend.Containers[ContainerName()].size());
        VERIFY_IS_TRUE(store.Load(u"App", 3, u"Newest", 7).Status == LoadStatus::Loaded);

        // A later save retries the trim against a fresh enumeration.
        backend.DeleteSuccessesBeforeFailure = SIZE_MAX;
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"Newest", 7, Sample()));
        VERIFY_ARE_EQUAL(size_t{32}, backend.Containers[ContainerName()].size());
    }

    TEST_METHOD(ConcurrentSavesPublishOneWholeSnapshot)
    {
        AtomicMemorySettingsBackend backend;
        PlacementStore store(backend);
        const auto first = SampleFor(1);
        const auto second = SampleFor(2);
        bool firstSaved = false;
        bool secondSaved = false;

        std::thread firstWriter([&] { firstSaved = store.Save(u"App", 3, u"Shared", 6, first); });
        std::thread secondWriter([&] { secondSaved = store.Save(u"App", 3, u"Shared", 6, second); });
        firstWriter.join();
        secondWriter.join();

        VERIFY_IS_TRUE(firstSaved);
        VERIFY_IS_TRUE(secondSaved);
        Record stored;
        const auto text = backend.StoredText(ContainerName(), ValueName(u"Shared"));
        VERIFY_IS_TRUE(DecodeRecordText(text.c_str(), text.size(), stored) == DecodeResult::Success);
        VERIFY_IS_TRUE(SameSnapshot(stored.Placement, first) || SameSnapshot(stored.Placement, second));
    }

    TEST_METHOD(LoadDuringSaveSeesOldOrNewWholeSnapshot)
    {
        AtomicMemorySettingsBackend backend;
        PlacementStore store(backend);
        const auto oldPlacement = SampleFor(1);
        const auto newPlacement = SampleFor(2);
        VERIFY_IS_TRUE(store.Save(u"App", 3, u"Shared", 6, oldPlacement));
        backend.PauseNextReplace = true;

        bool saved = false;
        std::thread writer([&]
            {
                saved = store.Save(u"App", 3, u"Shared", 6, newPlacement);
            });
        backend.WaitForReplaceEntry();
        const auto duringSave = store.Load(u"App", 3, u"Shared", 6);
        backend.ReleaseReplace();
        writer.join();
        const auto afterSave = store.Load(u"App", 3, u"Shared", 6);

        VERIFY_IS_TRUE(saved);
        VERIFY_IS_TRUE(duringSave.Status == LoadStatus::Loaded);
        VERIFY_IS_TRUE(SameSnapshot(duringSave.Placement.Placement, oldPlacement) ||
            SameSnapshot(duringSave.Placement.Placement, newPlacement));
        VERIFY_IS_TRUE(afterSave.Status == LoadStatus::Loaded);
        VERIFY_IS_TRUE(SameSnapshot(afterSave.Placement.Placement, oldPlacement) ||
            SameSnapshot(afterSave.Placement.Placement, newPlacement));
    }
};
