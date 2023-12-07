// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RuntimeEnabledFeaturesEnum.h"

namespace RuntimeFeatureBehavior
{
    // Holds a feature's static data.
    struct RuntimeEnabledFeatureData
    {
        const wchar_t* FeatureName;
        RuntimeEnabledFeature Feature;
        bool DefaultEnabledState; // If the key does not exist, we will use this to determine whether the feature should be considered enabled
        int DisableValue; // We use this to define what it means to be disabled or enabled
        int DefaultDwordValue; // We use this as the default value for the dword when the key does not exist
    };

    // Represents a feature's current state.
    struct RuntimeEnabledFeatureState
    {
        RuntimeEnabledFeatureState()
            : OverrideValue(false), CacheValue(false), IsOverrideSet(false), IsCacheSet(false), IsDWordDataCacheSet(false), CacheDwordData(0) {};
        bool OverrideValue : 1;
        bool CacheValue : 1;
        bool IsOverrideSet : 1;
        bool IsCacheSet : 1;
        bool IsDWordDataCacheSet : 1;

        int CacheDwordData;
    };

    // The count of valid RuntimeEnabledFeature enum values.
    const unsigned int s_runtimeFeatureLength = static_cast<unsigned int>(RuntimeEnabledFeature::LastValue) + 1;

    // A static table of default values for RuntimeEnabledFeatures. This data
    // is stored in the DLL data section such that it is shared across processes
    // and not loaded into the heap.
    extern const RuntimeEnabledFeatureData s_runtimeFeatureData[s_runtimeFeatureLength];

    // Provides an abstraction away from registry keys and other ways to enable/disable
    // features at runtime. This isn't meant to replace our quirking mechanism, which serves
    // the purpose of quirking individual aspects of a feature at runtime depending on which
    // version of the OS the app was built against, but is a way to preflight features that
    // we're planning to ship within a release. One can imagine turning these features on/off
    // for small sets of self-hosters in the future to get feedback, enabling them only
    // for a certain set of tests to test only partially-complete feature work, or turning
    // them on/off to calculate perf deltas.
    //
    // Usage:
    // To add a feature to the feature table:
    // 1) Add an enum value to the RuntimeEnabledFeatue enum, in RuntimeEnabledFeaturesEnum.h
    // 2) Add an entry to the s_runtimeFeatureData array, in RuntimeFeatureData.cpp
    //
    // In product code, use static RuntimeEnabledFeatureDetector::IsFeatureEnabled API to query the feature value
    // In test code, use the IXamlTestHooks::SetRuntimeEnabledFeatureOverride API set a feature value
    class __declspec(uuid("8876af72-5a66-4f70-85be-e5b7826f8b10")) RuntimeEnabledFeatureDetector
    {
    public:
        // Returns a bool indicating whether the feature is enabled by examining
        // the override vector, the registry, and finally the default value in the
        // DLL. When disableCaching is set to true this method will avoid any caching
        // of the value, which will avoid any possible allocations. This flag
        // is useful for scenarios where RuntimeEnabledFeatureDetector needs to be
        // invoked early in the application startup pathway before the global heap
        // is initialized.
        _Check_return_ bool IsFeatureEnabled(_In_ RuntimeEnabledFeature feature, _In_ bool disableCaching = false);

        // Returns an int representing the DWORD value obtain from the registry.
        // We first examine the registry before using the defaultValue provided.
        // When disableCaching is set to true this method will avoid any caching
        // of the value, which will avoid any possible allocations. This flag
        // is useful for scenarios where RuntimeEnabledFeatureDetector needs to be
        // invoked early in the application startup pathway before the global heap
        // is initialized.
        _Check_return_ int GetFeatureValue(_In_ RuntimeEnabledFeature feature, _In_ bool disableCaching = false);

        // By default we read whether a feature is enabled from the registry, tests
        // can override this using a test hook to enable a feature for the duration
        // of a test.
        void SetFeatureOverride(_In_ RuntimeEnabledFeature feature, _In_ bool enabled, _Out_opt_ bool* wasPreviouslyEnabled = nullptr);
        void ClearFeatureOverride(_In_ RuntimeEnabledFeature feature);
        void ClearAllFeatureOverrides();

        // Registry reads are cached. For testing and changing keys on the fly
        // this cache can be cleared.
        void ClearCache();
        void ClearDwordDataCache();

    private:
        const RuntimeEnabledFeatureData& GetFeatureData(_In_ RuntimeEnabledFeature feature);
        RuntimeEnabledFeatureState& GetFeatureState(_In_ RuntimeEnabledFeature feature);
        RuntimeEnabledFeatureState m_runtimeFeatureStates[s_runtimeFeatureLength];
    };

    std::shared_ptr<RuntimeEnabledFeatureDetector> GetRuntimeEnabledFeatureDetector();

    struct RuntimeEnabledFeatureScopeGuard
    {
        explicit RuntimeEnabledFeatureScopeGuard(RuntimeEnabledFeature feature);
        ~RuntimeEnabledFeatureScopeGuard();

    private:
        RuntimeEnabledFeature m_feature;
    };
}
