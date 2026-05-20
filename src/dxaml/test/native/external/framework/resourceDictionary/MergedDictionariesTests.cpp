// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include "MergedDictionariesTests.h"
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { 
    namespace Tests { namespace Framework {

    bool MergedDictionariesTests::ClassSetup()
    {
        // It's very important to call EnsureInitialized on TestServices
        // from ClassSetup. This method will wait for the window to be
        // activated on launch, which avoids a race condition that will block
        // input from being routed to the app. It will also wait for the
        // debugger to attach when the waitForDebugger runtime parameter is
        // specified.
        CommonTestSetupHelper::CommonTestClassSetup();

        return true;
    }
     
        bool MergedDictionariesTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

    bool MergedDictionariesTests::TestCleanup()
    {
        // It's very important to have your test clean up the window contents
        // when it completes. When creating new tests be sure to copy this
        // method over or implement it in a similar way. By cleaning
        // up the window content and waiting for the page to go idle you ensure
        // that if your test fails while the UI element tree is being torn down
        // that the failure is associated with your test and doesn't occur
        // nondeterministically in the future. By waiting for the page to go
        // idle you ensure that all transitions have completed and that jupiter
        // is in a 'tabula rasa' state for the next test.
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    // Validates that if a key is present in both the primary and merged 
    // dictionaries, the value in the primary dictionary wins.
    void MergedDictionariesTests::VerifyPrimaryOverridesMergedDictionaries_CodeBehind()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto targetKey = ref new Platform::String(L"KeyAlpha");

            // Create the primary dictionary
            auto primaryDict = CreatePrimaryDictionary();

            // Create a dictionary to merge with the primary, and which contains
            // a key that is also present in the primary
            auto mergedDict = ref new ResourceDictionary();
            mergedDict->Insert(targetKey, ref new Platform::String(L"ValueAlpha from Merged dictionary"));
            mergedDict->Insert(ref new Platform::String(L"KeyDelta"), ref new Platform::String(L"ValueDelta from Merged dictionary"));
            mergedDict->Insert(ref new Platform::String(L"KeyEpsilon"), ref new Platform::String(L"ValueEpsilon from Merged dictionary"));
            primaryDict->MergedDictionaries->Append(mergedDict);

            // Verify that the value from the primary dictionary wins
            auto value = safe_cast<Platform::String^>(primaryDict->Lookup(targetKey));
            VERIFY_IS_TRUE(value->Equals(ref new Platform::String(L"ValueAlpha from Primary dictionary")));
        });
    }

    // Validates that if a key is present in both the primary and merged 
    // dictionaries, the value in the primary dictionary wins.
    void MergedDictionariesTests::VerifyPrimaryOverridesMergedDictionaries_Markup()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

        RunOnUIThread([&]()
        {
            auto targetKey = ref new Platform::String(L"KeyAlpha");

            // Create the dictionary
            auto dictionary = safe_cast<ResourceDictionary^>(XamlReader::Load(
                L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\n"
                L"    <ResourceDictionary.MergedDictionaries>\n"
                L"        <ResourceDictionary>\n"
                L"            <x:String x:Key='KeyAlpha'>ValueAlpha from Merged dictionary</x:String>\n"
                L"            <x:String x:Key='KeyDelta'>ValueDelta from Merged dictionary</x:String>\n"
                L"            <x:String x:Key='KeyEpsilon'>ValueEpsilon from Merged dictionary</x:String>\n"
                L"        </ResourceDictionary>\n"
                L"    </ResourceDictionary.MergedDictionaries>\n"
                L"    <x:String x:Key='KeyAlpha'>ValueAlpha from Primary dictionary</x:String>\n"
                L"    <x:String x:Key='KeyBeta'>ValueBeta from Primary dictionary</x:String>\n"
                L"    <x:String x:Key='KeyGamma'>ValueGamma from Primary dictionary</x:String>\n"
                L"</ResourceDictionary>"
                ));

            // Verify that the value from the primary dictionary wins
            auto value = safe_cast<Platform::String^>(dictionary->Lookup(targetKey));
            VERIFY_IS_TRUE(value->Equals(ref new Platform::String(L"ValueAlpha from Primary dictionary")));
        });
    }

    // Validates that if a key is not found in the primary resource
    // dictionary, the lookup falls back to the merged dictionaries.
    void MergedDictionariesTests::VerifyFallbackToMergedDictionaries_CodeBehind()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto targetKey = ref new Platform::String(L"KeyDelta");

            // Create the primary dictionary
            auto primaryDict = CreatePrimaryDictionary();

            // Create a dictionary to merge with the primary, and which contains
            // a key that is not present in the primary
            auto mergedDict = ref new ResourceDictionary();
            mergedDict->Insert(targetKey, ref new Platform::String(L"ValueDelta from Merged dictionary"));
            mergedDict->Insert(ref new Platform::String(L"KeyEpsilon"), ref new Platform::String(L"ValueEpsilon from Merged dictionary"));
            primaryDict->MergedDictionaries->Append(mergedDict);

            // Verify that the value from the merged dictionary is returned
            auto value = safe_cast<Platform::String^>(primaryDict->Lookup(targetKey));
            VERIFY_IS_TRUE(value->Equals(ref new Platform::String(L"ValueDelta from Merged dictionary")));
        });
    }

    // Validates that if a key is not found in the primary resource
    // dictionary, the lookup falls back to the merged dictionaries.
    void MergedDictionariesTests::VerifyFallbackToMergedDictionaries_Markup()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

        RunOnUIThread([&]()
        {
            auto targetKey = ref new Platform::String(L"KeyDelta");

            // Create the dictionary
            auto dictionary = safe_cast<ResourceDictionary^>(XamlReader::Load(
                L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\n"
                L"    <ResourceDictionary.MergedDictionaries>\n"
                L"        <ResourceDictionary>\n"
                L"            <x:String x:Key='KeyDelta'>ValueDelta from Merged dictionary</x:String>\n"
                L"            <x:String x:Key='KeyEpsilon'>ValueEpsilon from Merged dictionary</x:String>\n"
                L"        </ResourceDictionary>\n"
                L"    </ResourceDictionary.MergedDictionaries>\n"
                L"    <x:String x:Key='KeyAlpha'>ValueAlpha from Primary dictionary</x:String>\n"
                L"    <x:String x:Key='KeyBeta'>ValueBeta from Primary dictionary</x:String>\n"
                L"    <x:String x:Key='KeyGamma'>ValueGamma from Primary dictionary</x:String>\n"
                L"</ResourceDictionary>"
                ));

            // Verify that the value from the primary dictionary wins
            auto value = safe_cast<Platform::String^>(dictionary->Lookup(targetKey));
            VERIFY_IS_TRUE(value->Equals(ref new Platform::String(L"ValueDelta from Merged dictionary")));
        });
    }

    // Validates that if a key is not found in the primary resource
    // dictionary, the lookup can properly fall back to a nested merged dictionary
    void MergedDictionariesTests::VerifyFallbackToNestedMergedDictionaries_CodeBehind()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto targetKey = ref new Platform::String(L"KeyZeta");

            // Create the primary dictionary
            auto primaryDict = CreatePrimaryDictionary();

            // Create a dictionary to merge with the primary, and which contains
            // a key that is not present in the primary
            auto mergedDict = ref new ResourceDictionary();
            mergedDict->Insert(ref new Platform::String(L"KeyDelta"), ref new Platform::String(L"ValueDelta from Primary Merged dictionary"));
            mergedDict->Insert(ref new Platform::String(L"KeyEpsilon"), ref new Platform::String(L"ValueEpsilon from Primary Merged dictionary"));

            // Create a dictionary to merge with the first merged dictionary,
            // and which contains a key that is not present in either ancestor dictionary
            auto secondMergedDict = ref new ResourceDictionary();
            secondMergedDict->Insert(targetKey, ref new Platform::String(L"ValueZeta from Secondary Merged dictionary"));
            secondMergedDict->Insert(ref new Platform::String(L"KeyEta"), ref new Platform::String(L"ValueEta from Secondary Merged dictionary"));

            // Add the merged dictionaries
            mergedDict->MergedDictionaries->Append(secondMergedDict);
            primaryDict->MergedDictionaries->Append(mergedDict);

            // Verify that the value from the merged dictionary is returned
            auto value = safe_cast<Platform::String^>(primaryDict->Lookup(targetKey));
            VERIFY_IS_TRUE(value->Equals(ref new Platform::String(L"ValueZeta from Secondary Merged dictionary")));
        });
    }

    // Validates that if a key is not found in the primary resource
    // dictionary,the lookup can properly fall back to a nested merged dictionary
    void MergedDictionariesTests::VerifyFallbackToNestedMergedDictionaries_Markup()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

        RunOnUIThread([&]()
        {
            auto targetKey = ref new Platform::String(L"KeyZeta");

            // Create the dictionary
            auto dictionary = safe_cast<ResourceDictionary^>(XamlReader::Load(
                L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\n"
                L"    <ResourceDictionary.MergedDictionaries>\n"
                L"        <ResourceDictionary>\n"
                L"            <ResourceDictionary.MergedDictionaries>\n"
                L"                <ResourceDictionary>\n"
                L"                    <x:String x:Key='KeyZeta'>ValueZeta from Secondary Merged dictionary</x:String>\n"
                L"                    <x:String x:Key='KeyEta'>ValueEta from Secondary Merged dictionary</x:String>\n"
                L"                </ResourceDictionary>\n"
                L"            </ResourceDictionary.MergedDictionaries>\n"
                L"            <x:String x:Key='KeyDelta'>ValueDelta from Primary Merged dictionary</x:String>\n"
                L"            <x:String x:Key='KeyEpsilon'>ValueEpsilon from Primary Merged dictionary</x:String>\n"
                L"        </ResourceDictionary>\n"
                L"    </ResourceDictionary.MergedDictionaries>\n"
                L"    <x:String x:Key='KeyAlpha'>ValueAlpha from Primary dictionary</x:String>\n"
                L"    <x:String x:Key='KeyBeta'>ValueBeta from Primary dictionary</x:String>\n"
                L"    <x:String x:Key='KeyGamma'>ValueGamma from Primary dictionary</x:String>\n"
                L"</ResourceDictionary>"
                ));

            // Verify that the value from the primary dictionary wins
            auto value = safe_cast<Platform::String^>(dictionary->Lookup(targetKey));
            VERIFY_IS_TRUE(value->Equals(ref new Platform::String(L"ValueZeta from Secondary Merged dictionary")));
        });
    }

    // Validates the fallback sequence within MergedDictionaries if a
    // key is not found in the primary dictionary.
    void MergedDictionariesTests::VerifyIntraMergedDictionariesFallback_CodeBehind()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto targetKey = ref new Platform::String(L"KeyDelta");

            // Create the primary dictionary
            auto primaryDict = CreatePrimaryDictionary();

            // Create a dictionary to merge with the primary, and which does not
            // contain the target key
            auto mergedDict = ref new ResourceDictionary();

            mergedDict->Insert(ref new Platform::String(L"KeyEpsilon"), ref new Platform::String(L"ValueEpsilon from First Merged dictionary"));
            mergedDict->Insert(ref new Platform::String(L"KeyZeta"), ref new Platform::String(L"ValueZeta from First Merged dictionary"));
            mergedDict->Insert(ref new Platform::String(L"KeyEta"), ref new Platform::String(L"ValueEta from First Merged dictionary"));
            primaryDict->MergedDictionaries->Append(mergedDict);

            mergedDict = ref new ResourceDictionary();
            mergedDict->Insert(targetKey, ref new Platform::String(L"ValueDelta from Second Merged dictionary"));
            mergedDict->Insert(ref new Platform::String(L"KeyEpsilon"), ref new Platform::String(L"ValueEpsilon from Second Merged dictionary"));
            mergedDict->Insert(ref new Platform::String(L"KeyZeta"), ref new Platform::String(L"ValueZeta from Second Merged dictionary"));
            mergedDict->Insert(ref new Platform::String(L"KeyEta"), ref new Platform::String(L"ValueEta from Second Merged dictionary"));
            primaryDict->MergedDictionaries->Append(mergedDict);

            // Verify that the value from the merged dictionary is returned
            auto value = safe_cast<Platform::String^>(primaryDict->Lookup(targetKey));
            VERIFY_IS_TRUE(value->Equals(ref new Platform::String(L"ValueDelta from Second Merged dictionary")));
        });
    }

    // Validates the fallback sequence within MergedDictionaries if a
    // key is not found in the primary dictionary.
    void MergedDictionariesTests::VerifyIntraMergedDictionariesFallback_Markup()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

        RunOnUIThread([&]()
        {
            auto targetKey = ref new Platform::String(L"KeyDelta");

            // Create the dictionary
            auto dictionary = safe_cast<ResourceDictionary^>(XamlReader::Load(
                L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\n"
                L"    <ResourceDictionary.MergedDictionaries>\n"
                L"        <ResourceDictionary>\n"
                L"            <x:String x:Key='KeyEpsilon'>ValueEpsilon from First Merged dictionary</x:String>\n"
                L"            <x:String x:Key='KeyZeta'>ValueZeta from First Merged dictionary</x:String>\n"
                L"            <x:String x:Key='KeyEta'>ValueEta from First Merged dictionary</x:String>\n"
                L"        </ResourceDictionary>\n"
                L"        <ResourceDictionary>\n"
                L"            <x:String x:Key='KeyDelta'>ValueDelta from Second Merged dictionary</x:String>\n"
                L"            <x:String x:Key='KeyEpsilon'>ValueEpsilon from Second Merged dictionary</x:String>\n"
                L"            <x:String x:Key='KeyZeta'>ValueZeta from Second Merged dictionary</x:String>\n"
                L"            <x:String x:Key='KeyEta'>ValueEta from Second Merged dictionary</x:String>\n"
                L"        </ResourceDictionary>\n"
                L"    </ResourceDictionary.MergedDictionaries>\n"
                L"    <x:String x:Key='KeyAlpha'>ValueAlpha from Primary dictionary</x:String>\n"
                L"    <x:String x:Key='KeyBeta'>ValueBeta from Primary dictionary</x:String>\n"
                L"    <x:String x:Key='KeyGamma'>ValueGamma from Primary dictionary</x:String>\n"
                L"</ResourceDictionary>"
                ));

            // Verify that the value from the primary dictionary wins
            auto value = safe_cast<Platform::String^>(dictionary->Lookup(targetKey));
            VERIFY_IS_TRUE(value->Equals(ref new Platform::String(L"ValueDelta from Second Merged dictionary")));
        });
    }

    // Validates that a resource dictionary cannot be added to its
    // own MergedDictionaries.
    void MergedDictionariesTests::VerifyMergedDictionariesCycleCheck()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto dictionary = CreatePrimaryDictionary();

            VERIFY_THROWS_WINRT(dictionary->MergedDictionaries->Append(dictionary), Platform::Exception^,
                L"An exception should be thrown if a ResourceDictionary is added to its own MergedDictionaries collection");
        });
    }

#pragma region Helper methods

    // Helper method which creates the primary resource dictionary,
    // containing the keys 'KeyAlpha', 'KeyBeta', and 'KeyGamma'
    ResourceDictionary^ MergedDictionariesTests::CreatePrimaryDictionary()
    {
        auto dictionary = ref new ResourceDictionary();
        dictionary->Insert(ref new Platform::String(L"KeyAlpha"), ref new Platform::String(L"ValueAlpha from Primary dictionary"));
        dictionary->Insert(ref new Platform::String(L"KeyBeta"), ref new Platform::String(L"ValueBeta from Primary dictionary"));
        dictionary->Insert(ref new Platform::String(L"KeyGamma"), ref new Platform::String(L"ValueGamma from Primary dictionary"));

        return dictionary;
    }

#pragma endregion Helper methods

} } } } }
