// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

using namespace Microsoft::UI::Xaml;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        class MergedDictionariesTests
        {
        public:
            MergedDictionariesTests() {}

            BEGIN_TEST_CLASS(MergedDictionariesTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(VerifyPrimaryOverridesMergedDictionaries_CodeBehind)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"Validates that if a key is present in both the primary and merged "
                    L"dictionaries, the value in the primary dictionary wins.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyPrimaryOverridesMergedDictionaries_Markup)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Validates that if a key is present in both the primary and merged "
                    L"dictionaries, the value in the primary dictionary wins.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyFallbackToMergedDictionaries_CodeBehind)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"Validates that if a key is not found in the primary resource "
                    L"dictionary, the lookup falls back to the merged dictionaries.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyFallbackToMergedDictionaries_Markup)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"Validates that if a key is not found in the primary resource "
                    L"dictionary, the lookup falls back to the merged dictionaries.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyFallbackToNestedMergedDictionaries_CodeBehind)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"Validates that if a key is not found in the primary resource "
                    L"dictionary, the lookup can properly fall back to a nested merged dictionary.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyFallbackToNestedMergedDictionaries_Markup)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"Validates that if a key is not found in the primary resource "
                    L"dictionary, the lookup can properly fall back to a nested merged dictionary.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyIntraMergedDictionariesFallback_CodeBehind)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"Validates the fallback sequence within MergedDictionaries if a "
                    L"key is not found in the primary dictionary.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyIntraMergedDictionariesFallback_Markup)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"Validates the fallback sequence within MergedDictionaries if a "
                    L"key is not found in the primary dictionary.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyMergedDictionariesCycleCheck)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"Validates that a resource dictionary cannot be added to its "
                    L"own MergedDictionaries.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            END_TEST_METHOD()

        private:
            // Helper method which creates the primary resource dictionary,
            // containing the keys 'KeyAlpha', 'KeyBeta', and 'KeyGamma'
            ResourceDictionary^ CreatePrimaryDictionary();
        };
} } } } }
