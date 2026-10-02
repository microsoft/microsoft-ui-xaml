// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementStore.h"

#include <Windows.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.storage.h>
#include <wrl.h>
#include <wrl\wrappers\corewrappers.h>
#include <algorithm>
#include <iterator>
#include <limits>
#include <mutex>
#include <unordered_map>

extern "C" LONG WINAPI GetCurrentApplicationUserModelId(
    UINT32* applicationUserModelIdLength,
    PWSTR applicationUserModelId);

namespace DirectUI::WindowPlacementPersistence
{
    namespace
    {
        namespace wf = ABI::Windows::Foundation;
        namespace wfc = ABI::Windows::Foundation::Collections;
        namespace ws = ABI::Windows::Storage;
        using Microsoft::WRL::ComPtr;
        using Microsoft::WRL::Wrappers::HString;
        using Microsoft::WRL::Wrappers::HStringReference;

        constexpr size_t MaximumStoredRecords = 32;
        constexpr char16_t PlacementValuePrefix[] = u"wp1_";

        std::mutex& LockFor(const std::u16string& containerName)
        {
            static std::mutex mapLock;
            static std::unordered_map<std::u16string, std::unique_ptr<std::mutex>> locks;
            std::lock_guard guard(mapLock);
            auto& lock = locks[containerName];
            if (!lock) lock = std::make_unique<std::mutex>();
            return *lock;
        }

        bool IsPlacementValue(const std::u16string& name) noexcept
        {
            return name.size() > std::size(PlacementValuePrefix) - 1 &&
                name.compare(0, std::size(PlacementValuePrefix) - 1, PlacementValuePrefix) == 0;
        }

        bool IsFailure(StorageResult result) noexcept
        {
            return result == StorageResult::Unavailable || result == StorageResult::Failure;
        }

        bool IsAccessDenied(HRESULT error) noexcept
        {
            return error == E_ACCESSDENIED ||
                error == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }

        StorageOperationResult Success() noexcept
        {
            return {StorageResult::Success, S_OK};
        }

        StorageOperationResult Failure(StorageResult status, HRESULT error) noexcept
        {
            return {status, FAILED(error) ? error : E_FAIL};
        }

        StorageOperationResult ClassifyStorageFailure(HRESULT error) noexcept
        {
            return Failure(
                IsAccessDenied(error) ? StorageResult::Unavailable : StorageResult::Failure,
                error);
        }

        HRESULT GetLocalSettings(ComPtr<ws::IApplicationDataContainer>& settings) noexcept
        {
            ComPtr<ws::IApplicationDataStatics> statics;
            HRESULT hr = RoGetActivationFactory(
                HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(),
                IID_PPV_ARGS(&statics));
            if (FAILED(hr)) return hr;

            ComPtr<ws::IApplicationData> applicationData;
            hr = statics->get_Current(&applicationData);
            if (FAILED(hr)) return hr;
            return applicationData->get_LocalSettings(&settings);
        }

        HRESULT GetContainer(
            ws::IApplicationDataContainer* settings,
            const std::u16string& name,
            bool create,
            ComPtr<ws::IApplicationDataContainer>& container) noexcept
        {
            ComPtr<wfc::__FIMapView_2_HSTRING_Windows__CStorage__CApplicationDataContainer_t> containers;
            HRESULT hr = settings->get_Containers(&containers);
            if (FAILED(hr)) return hr;

            HString key;
            hr = key.Set(reinterpret_cast<const wchar_t*>(name.data()), static_cast<UINT32>(name.size()));
            if (FAILED(hr)) return hr;

            if (!create)
            {
                hr = containers->Lookup(key.Get(), &container);
                if (IsMissingLookupResult(hr, container != nullptr))
                {
                    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                }
                return hr;
            }

            hr = settings->CreateContainer(
                key.Get(),
                ws::ApplicationDataCreateDisposition_Always,
                &container);
            if (FAILED(hr)) return hr;
            return container ? S_OK : E_POINTER;
        }

        HRESULT GetStringValue(
            IInspectable* object,
            std::u16string& text) noexcept
        {
            if (object == nullptr) return E_POINTER;

            ComPtr<wf::IPropertyValue> propertyValue;
            HRESULT hr = object->QueryInterface(IID_PPV_ARGS(&propertyValue));
            if (FAILED(hr)) return hr;

            wf::PropertyType type{};
            hr = propertyValue->get_Type(&type);
            if (FAILED(hr)) return hr;
            if (type != wf::PropertyType_String) return DISP_E_TYPEMISMATCH;

            HString value;
            hr = propertyValue->GetString(value.GetAddressOf());
            if (FAILED(hr)) return hr;
            UINT32 length = 0;
            const wchar_t* buffer = value.GetRawBuffer(&length);
            text.assign(
                reinterpret_cast<const char16_t*>(buffer),
                reinterpret_cast<const char16_t*>(buffer) + length);
            return S_OK;
        }

        HRESULT CreateStringValue(const std::u16string& text, ComPtr<IInspectable>& value) noexcept
        {
            ComPtr<wf::IPropertyValueStatics> statics;
            HRESULT hr = RoGetActivationFactory(
                HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
                IID_PPV_ARGS(&statics));
            if (FAILED(hr)) return hr;

            HString string;
            hr = string.Set(
                reinterpret_cast<const wchar_t*>(text.data()),
                static_cast<UINT32>(text.size()));
            if (FAILED(hr)) return hr;
            return statics->CreateString(string.Get(), &value);
        }

        StorageOperationResult GetSettingsContainer(
            const std::u16string& name,
            bool create,
            ComPtr<ws::IApplicationDataContainer>& container) noexcept
        {
            std::u16string applicationId;
            const HRESULT identity = TryGetPackagedApplicationId(applicationId);
            if (FAILED(identity))
            {
                if (identity == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE) ||
                    identity == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_APPLICATION))
                {
                    return Failure(StorageResult::Unavailable, identity);
                }
                return ClassifyStorageFailure(identity);
            }

            ComPtr<ws::IApplicationDataContainer> settings;
            HRESULT hr = GetLocalSettings(settings);
            if (FAILED(hr))
            {
                if (hr == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE) ||
                    hr == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_APPLICATION))
                {
                    return Failure(StorageResult::Unavailable, hr);
                }
                return ClassifyStorageFailure(hr);
            }

            hr = GetContainer(settings.Get(), name, create, container);
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                return {StorageResult::Missing, S_OK};
            }
            if (FAILED(hr)) return ClassifyStorageFailure(hr);
            return Success();
        }

        bool TryGetName(
            const char16_t* applicationId,
            size_t applicationIdLength,
            const char16_t* placementId,
            size_t placementIdLength,
            std::u16string& container,
            std::u16string& value) noexcept
        {
            return TryMakeApplicationContainerName(applicationId, applicationIdLength, container) &&
                TryMakePlacementValueName(placementId, placementIdLength, value);
        }
    }

    PlacementStore::PlacementStore(IPlacementSettingsBackend& backend) noexcept
        : m_backend(backend)
    {
    }

    LoadResult PlacementStore::Load(
        const char16_t* applicationId,
        size_t applicationIdLength,
        const char16_t* placementId,
        size_t placementIdLength) const
    {
        LoadResult result;
        result.Error = S_OK;
        std::u16string container;
        std::u16string valueName;
        if (!TryGetName(applicationId, applicationIdLength, placementId, placementIdLength, container, valueName))
        {
            result.Status = LoadStatus::Invalid;
            result.Category = PlacementFailureCategory::Identity;
            return result;
        }

        StoredValue value;
        const auto readResult = m_backend.ReadValue(container, valueName, value);
        result.Error = readResult.Error;
        if (readResult.Status == StorageResult::Missing)
        {
            result.Status = LoadStatus::Missing;
        }
        else if (readResult.Status == StorageResult::WrongType)
        {
            result.Status = LoadStatus::Invalid;
            result.Category = PlacementFailureCategory::Decode;
        }
        else if (readResult.Status == StorageResult::Unavailable)
        {
            result.Status = LoadStatus::Unavailable;
        }
        else if (readResult.Status != StorageResult::Success)
        {
            result.Status = LoadStatus::Unexpected;
            result.Error = FAILED(readResult.Error) ? readResult.Error : E_FAIL;
        }
        else if (!value.IsString || value.Text.size() > MaximumEncodedCharacters)
        {
            result.Status = LoadStatus::Invalid;
            result.Category = PlacementFailureCategory::Decode;
        }
        else
        {
            const auto decodeResult = DecodeRecordText(value.Text.c_str(), value.Text.size(), result.Placement);
            result.Error = S_OK;
            result.Category = PlacementFailureCategory::Decode;
            result.Status = decodeResult == DecodeResult::Success ? LoadStatus::Loaded :
                decodeResult == DecodeResult::UnsupportedVersion ? LoadStatus::Invalid : LoadStatus::Invalid;
        }
        return result;
    }

    bool PlacementStore::Save(
        const char16_t* applicationId,
        size_t applicationIdLength,
        const char16_t* placementId,
        size_t placementIdLength,
        const Snapshot& placement,
        PlacementFailure* failure)
    {
        if (failure) *failure = {};
        auto fail = [failure](PlacementFailureCategory category, HRESULT error = S_OK)
        {
            if (failure) *failure = {category, error};
            return false;
        };
        std::u16string container;
        std::u16string valueName;
        if (!TryGetName(applicationId, applicationIdLength, placementId, placementIdLength, container, valueName))
        {
            return fail(PlacementFailureCategory::Identity);
        }
        if (!IsValid(placement))
        {
            return fail(PlacementFailureCategory::Write);
        }

        std::unique_lock lock(LockFor(container));
        std::vector<StoredValue> values;
        const auto enumerateResult = m_backend.EnumerateValues(container, values);
        if (IsFailure(enumerateResult.Status))
        {
            return fail(PlacementFailureCategory::Write, enumerateResult.Error);
        }
        if (enumerateResult.Status != StorageResult::Success && enumerateResult.Status != StorageResult::Missing)
        {
            return fail(PlacementFailureCategory::Write, enumerateResult.Error);
        }

        uint64_t maximumSequence = 0;
        for (const auto& value : values)
        {
            if (!IsPlacementValue(value.Name) || !value.IsString ||
                value.Text.size() > MaximumEncodedCharacters)
            {
                continue;
            }
            Record existing;
            const auto decodeResult = DecodeRecordText(value.Text.c_str(), value.Text.size(), existing);
            if (decodeResult == DecodeResult::UnsupportedVersion)
            {
                // A record written by an unsupported major format cannot be ordered
                // safely, so this cycle cannot tell which record is oldest. Defer the
                // whole save and cleanup instead of guessing and evicting newer data.
                return fail(PlacementFailureCategory::Decode);
            }
            if (decodeResult == DecodeResult::Success)
            {
                maximumSequence = (std::max)(maximumSequence, existing.SaveSequence);
            }
        }
        if (maximumSequence == (std::numeric_limits<uint64_t>::max)())
        {
            return fail(PlacementFailureCategory::Write);
        }

        Record record;
        record.Placement = placement;
        record.SaveSequence = maximumSequence + 1;
        std::u16string encoded;
        if (!EncodeRecordText(record, encoded))
        {
            return fail(PlacementFailureCategory::Write);
        }
        const auto replaceResult = m_backend.ReplaceValue(container, valueName, encoded);
        if (replaceResult.Status != StorageResult::Success)
        {
            return fail(PlacementFailureCategory::Write, replaceResult.Error);
        }

        values.erase(std::remove_if(values.begin(), values.end(), [&](const StoredValue& value)
            {
                return value.Name == valueName;
            }), values.end());
        values.push_back({valueName, true, encoded});

        struct Candidate
        {
            std::u16string Name;
            uint64_t Sequence{};
        };
        std::vector<Candidate> candidates;
        for (const auto& value : values)
        {
            if (!IsPlacementValue(value.Name)) continue;
            Record existing;
            uint64_t sequence = 0;
            if (value.IsString && value.Text.size() <= MaximumEncodedCharacters &&
                DecodeRecordText(value.Text.c_str(), value.Text.size(), existing) == DecodeResult::Success)
            {
                sequence = existing.SaveSequence;
            }
            candidates.push_back({value.Name, sequence});
        }
        std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right)
            {
                return left.Sequence != right.Sequence ? left.Sequence < right.Sequence : left.Name < right.Name;
            });
        while (candidates.size() > MaximumStoredRecords)
        {
            // Cleanup is deliberately best effort. The replacement above is
            // already a successful save and must not be rolled back. Stop on the
            // first deletion failure; a later save retries against a fresh
            // enumeration rather than trusting this victim list.
            if (m_backend.DeleteValue(container, candidates.front().Name).Status != StorageResult::Success)
            {
                break;
            }
            candidates.erase(candidates.begin());
        }
        return true;
    }

    StorageOperationResult ApplicationDataPlacementSettingsBackend::ReadValue(
        const std::u16string& containerName,
        const std::u16string& valueName,
        StoredValue& value)
    {
        ComPtr<ws::IApplicationDataContainer> container;
        const auto containerResult = GetSettingsContainer(containerName, false, container);
        if (containerResult.Status != StorageResult::Success) return containerResult;

        ComPtr<wfc::IPropertySet> propertySet;
        HRESULT hr = container->get_Values(&propertySet);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);
        ComPtr<wfc::IMap<HSTRING, IInspectable*>> values;
        hr = propertySet.As(&values);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);

        HString key;
        hr = key.Set(
            reinterpret_cast<const wchar_t*>(valueName.data()),
            static_cast<UINT32>(valueName.size()));
        if (FAILED(hr)) return Failure(StorageResult::Failure, hr);

        ComPtr<IInspectable> object;
        hr = values->Lookup(key.Get(), &object);
        if (IsMissingLookupResult(hr, object != nullptr)) return {StorageResult::Missing, S_OK};
        if (FAILED(hr)) return ClassifyStorageFailure(hr);

        std::u16string text;
        hr = GetStringValue(object.Get(), text);
        if (hr == DISP_E_TYPEMISMATCH) return {StorageResult::WrongType, S_OK};
        if (FAILED(hr)) return ClassifyStorageFailure(hr);
        value = {valueName, true, std::move(text)};
        return Success();
    }

    StorageOperationResult ApplicationDataPlacementSettingsBackend::ReplaceValue(
        const std::u16string& containerName,
        const std::u16string& valueName,
        const std::u16string& text)
    {
        ComPtr<ws::IApplicationDataContainer> container;
        const auto containerResult = GetSettingsContainer(containerName, true, container);
        if (containerResult.Status != StorageResult::Success) return containerResult;

        ComPtr<wfc::IPropertySet> propertySet;
        HRESULT hr = container->get_Values(&propertySet);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);
        ComPtr<wfc::IMap<HSTRING, IInspectable*>> values;
        hr = propertySet.As(&values);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);

        HString key;
        hr = key.Set(
            reinterpret_cast<const wchar_t*>(valueName.data()),
            static_cast<UINT32>(valueName.size()));
        if (FAILED(hr)) return Failure(StorageResult::Failure, hr);

        ComPtr<IInspectable> object;
        hr = CreateStringValue(text, object);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);
        boolean replaced = false;
        hr = values->Insert(key.Get(), object.Get(), &replaced);
        return SUCCEEDED(hr) ? Success() : ClassifyStorageFailure(hr);
    }

    StorageOperationResult ApplicationDataPlacementSettingsBackend::EnumerateValues(
        const std::u16string& containerName,
        std::vector<StoredValue>& values)
    {
        ComPtr<ws::IApplicationDataContainer> container;
        const auto containerResult = GetSettingsContainer(containerName, false, container);
        if (containerResult.Status != StorageResult::Success) return containerResult;

        ComPtr<wfc::IPropertySet> propertySet;
        HRESULT hr = container->get_Values(&propertySet);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);
        ComPtr<wfc::IMap<HSTRING, IInspectable*>> storedValues;
        hr = propertySet.As(&storedValues);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);

        ComPtr<wfc::IIterable<wfc::IKeyValuePair<HSTRING, IInspectable*>*>> iterable;
        hr = storedValues.As(&iterable);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);
        ComPtr<wfc::IIterator<wfc::IKeyValuePair<HSTRING, IInspectable*>*>> iterator;
        hr = iterable->First(&iterator);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);

        boolean hasCurrent = false;
        while (SUCCEEDED(iterator->get_HasCurrent(&hasCurrent)) && hasCurrent)
        {
            ComPtr<wfc::IKeyValuePair<HSTRING, IInspectable*>> pair;
            hr = iterator->get_Current(&pair);
            if (FAILED(hr)) return ClassifyStorageFailure(hr);

            HString key;
            hr = pair->get_Key(key.GetAddressOf());
            if (FAILED(hr)) return ClassifyStorageFailure(hr);
            UINT32 keyLength = 0;
            const wchar_t* keyBuffer = key.GetRawBuffer(&keyLength);
            std::u16string name(
                reinterpret_cast<const char16_t*>(keyBuffer),
                reinterpret_cast<const char16_t*>(keyBuffer) + keyLength);

            ComPtr<IInspectable> object;
            hr = pair->get_Value(&object);
            if (FAILED(hr)) return ClassifyStorageFailure(hr);
            std::u16string text;
            const HRESULT valueResult = GetStringValue(object.Get(), text);
            values.push_back({std::move(name), valueResult == S_OK, std::move(text)});
            hr = iterator->MoveNext(&hasCurrent);
            if (FAILED(hr)) return ClassifyStorageFailure(hr);
        }
        return Success();
    }

    StorageOperationResult ApplicationDataPlacementSettingsBackend::DeleteValue(
        const std::u16string& containerName,
        const std::u16string& valueName)
    {
        ComPtr<ws::IApplicationDataContainer> container;
        const auto containerResult = GetSettingsContainer(containerName, false, container);
        if (containerResult.Status != StorageResult::Success) return containerResult;

        ComPtr<wfc::IPropertySet> propertySet;
        HRESULT hr = container->get_Values(&propertySet);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);
        ComPtr<wfc::IMap<HSTRING, IInspectable*>> values;
        hr = propertySet.As(&values);
        if (FAILED(hr)) return ClassifyStorageFailure(hr);
        HString key;
        hr = key.Set(
            reinterpret_cast<const wchar_t*>(valueName.data()),
            static_cast<UINT32>(valueName.size()));
        if (FAILED(hr)) return Failure(StorageResult::Failure, hr);
        hr = values->Remove(key.Get());
        return hr == E_BOUNDS || SUCCEEDED(hr) ? Success() : ClassifyStorageFailure(hr);
    }

    HRESULT TryGetPackagedApplicationId(std::u16string& applicationId) noexcept
    {
        UINT32 length = 0;
        const LONG firstResult = GetCurrentApplicationUserModelId(&length, nullptr);
        if (firstResult == APPMODEL_ERROR_NO_PACKAGE || firstResult == APPMODEL_ERROR_NO_APPLICATION)
        {
            return HRESULT_FROM_WIN32(firstResult);
        }
        if (firstResult != ERROR_INSUFFICIENT_BUFFER || length == 0)
        {
            return HRESULT_FROM_WIN32(firstResult);
        }

        std::unique_ptr<wchar_t[]> buffer(new (std::nothrow) wchar_t[length]);
        if (!buffer) return E_OUTOFMEMORY;
        const LONG result = GetCurrentApplicationUserModelId(&length, buffer.get());
        if (result != ERROR_SUCCESS) return HRESULT_FROM_WIN32(result);
        applicationId.assign(
            reinterpret_cast<const char16_t*>(buffer.get()),
            static_cast<size_t>(length - 1));
        return applicationId.empty() ? E_FAIL : S_OK;
    }

    bool IsMissingLookupResult(HRESULT lookupResult, bool hasResult) noexcept
    {
        return lookupResult == E_BOUNDS || (SUCCEEDED(lookupResult) && !hasResult);
    }
}
