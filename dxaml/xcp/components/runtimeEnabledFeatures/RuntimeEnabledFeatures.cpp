// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <type_traits>

namespace RuntimeFeatureBehavior
{
    static const wchar_t* rootXamlKey = XAML_ROOT_KEY;

    std::shared_ptr<RuntimeEnabledFeatureDetector> GetRuntimeEnabledFeatureDetector()
    {
        static PROVIDE_RAW_ALLOC_DEPENDENCY(RuntimeEnabledFeatureDetector, DependencyLocator::StoragePolicyFlags::None);
        static DependencyLocator::Dependency<RuntimeFeatureBehavior::RuntimeEnabledFeatureDetector> s_runtimeEnabledFeatureDetector;

        return s_runtimeEnabledFeatureDetector.Get();
    }

    RuntimeEnabledFeatureState&
    RuntimeEnabledFeatureDetector::GetFeatureState(_In_ RuntimeEnabledFeature feature)
    {
        auto pos = static_cast<unsigned int>(feature);
        ASSERT(pos < s_runtimeFeatureLength);
        return m_runtimeFeatureStates[pos];
    }

    const RuntimeEnabledFeatureData&
    RuntimeEnabledFeatureDetector::GetFeatureData(_In_ RuntimeEnabledFeature feature)
    {
        auto pos = static_cast<unsigned int>(feature);
        ASSERT(pos < s_runtimeFeatureLength);
        ASSERT(s_runtimeFeatureData[pos].Feature == feature);
        return s_runtimeFeatureData[pos];
    }

    _Check_return_ bool
    RuntimeEnabledFeatureDetector::IsFeatureEnabled(_In_ RuntimeEnabledFeature feature, _In_ bool disableCaching)
    {
        RuntimeEnabledFeatureState& state = GetFeatureState(feature);
        RuntimeEnabledFeatureData featureData = GetFeatureData(feature);

        if (state.IsOverrideSet)
            return state.OverrideValue;

        if (state.IsCacheSet)
            return state.CacheValue;

        const wchar_t* keyname = featureData.FeatureName;

        bool defaultOrRegKeyValue = featureData.DefaultEnabledState;
        DWORD data = 0;
        HKEY hkXaml = NULL;

        // It's possible the XAML registry key doesn't exist at all. That's just fine.
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, rootXamlKey, 0, KEY_READ, &hkXaml) == ERROR_SUCCESS)
        {
            DWORD dwSize = sizeof(DWORD);

            // It's also possible the key for our feature doesn't exist at all. That's just fine too.
            if (RegQueryValueEx(hkXaml, keyname, 0, NULL, reinterpret_cast<LPBYTE>(&data), &dwSize) == ERROR_SUCCESS)
            {
                defaultOrRegKeyValue = (data != featureData.DisableValue);
            }

            RegCloseKey(hkXaml);
        }

        if (!disableCaching)
        {
            state.CacheValue = defaultOrRegKeyValue;
            state.CacheDwordData = data;
            state.IsCacheSet = true;
            state.IsDWordDataCacheSet = true;
        }

        return defaultOrRegKeyValue;
    }

    _Check_return_ int RuntimeEnabledFeatureDetector::GetFeatureValue(_In_ RuntimeEnabledFeature feature, _In_ bool disableCaching)
    {
        RuntimeEnabledFeatureState& state = GetFeatureState(feature);

        if (!disableCaching && state.IsDWordDataCacheSet)
        {
            return state.CacheDwordData;
        }

        const wchar_t* keyname = GetFeatureData(feature).FeatureName;

        DWORD value = GetFeatureData(feature).DefaultDwordValue;
        HKEY hkXaml = NULL;

        // It's possible the XAML registry key doesn't exist at all. That's just fine.
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, rootXamlKey, 0, KEY_READ, &hkXaml) == ERROR_SUCCESS)
        {
            DWORD dwSize = sizeof(DWORD);
            RegQueryValueEx(hkXaml, keyname, 0, NULL, reinterpret_cast<LPBYTE>(&value), &dwSize);
            RegCloseKey(hkXaml);
        }

        if (!disableCaching)
        {
            state.CacheDwordData = value;
            state.IsDWordDataCacheSet = true;
        }

        return value;
    }

    void RuntimeEnabledFeatureDetector::SetFeatureOverride(_In_ RuntimeEnabledFeature feature, _In_ bool enabled, _Out_opt_ bool* wasPreviouslyEnabled)
    {
        RuntimeEnabledFeatureState& state = GetFeatureState(feature);
        if (wasPreviouslyEnabled != nullptr)
        {
            *wasPreviouslyEnabled = IsFeatureEnabled(feature, true /*disableCaching*/);
        }
        state.OverrideValue = enabled;
        state.IsOverrideSet = true;
    }

    void RuntimeEnabledFeatureDetector::ClearFeatureOverride(_In_ RuntimeEnabledFeature feature)
    {
        RuntimeEnabledFeatureState& state = GetFeatureState(feature);
        state.IsOverrideSet = false;
    }

    void RuntimeEnabledFeatureDetector::ClearAllFeatureOverrides()
    {
        for (unsigned int i = 0; i < s_runtimeFeatureLength; i++)
            m_runtimeFeatureStates[i].IsOverrideSet = false;
    }

    void RuntimeEnabledFeatureDetector::ClearCache()
    {
        for (unsigned int i = 0; i < s_runtimeFeatureLength; i++)
            m_runtimeFeatureStates[i].IsCacheSet = false;
    }

    void RuntimeEnabledFeatureDetector::ClearDwordDataCache()
    {
        for (unsigned int i = 0; i < s_runtimeFeatureLength; i++)
        {
            m_runtimeFeatureStates[i].IsDWordDataCacheSet = false;
            m_runtimeFeatureStates[i].CacheDwordData = s_runtimeFeatureData[i].DefaultDwordValue;
        }
    }

    RuntimeEnabledFeatureScopeGuard::RuntimeEnabledFeatureScopeGuard(RuntimeEnabledFeature feature)
        : m_feature(feature)
    {
        auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        runtimeEnabledFeatureDetector->SetFeatureOverride(m_feature, true);
    }

    RuntimeEnabledFeatureScopeGuard::~RuntimeEnabledFeatureScopeGuard()
    {
        auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        runtimeEnabledFeatureDetector->ClearFeatureOverride(m_feature);
    }
}

