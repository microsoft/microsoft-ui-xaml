// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Parser {

    class ConditionalXamlTests : public WEX::TestClass<ConditionalXamlTests>
    {
    public:
        BEGIN_TEST_CLASS(ConditionalXamlTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyBuiltinPredicates)
            TEST_METHOD_PROPERTY(L"Description", L"Validates basic functionality of each built-in XamlPredicate.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionallyDeclaredUnknownTypes)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when unknown type is conditionally declared.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionallyDeclaredUnknownProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when unknown property is conditionally declared.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionallyDeclaredKnownProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when known properties are conditionally declared.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDuplicatePropertiesThrowsError)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that duplicate properties, even when conditionally declared "
                L"(and evaluating to true) will cause an error to be thrown.")
        END_TEST_METHOD()        

        BEGIN_TEST_METHOD(VerifyConditionallyDeclaredPropertySetByThemeResource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when a conditionally declared property is assigned its "
                L"value by a {ThemeResource}")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionallyDeclaredPropertySetByStaticResource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when a conditionally declared property is assigned its "
                L"value by a {StaticResource}")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionalXamlWithXUid)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when x:Uid is used in conjunction with conditionally declared properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionalXamlWithTypeConversion)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when conditionally declared properties require type conversion.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionalXamlInTemplate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when a Template uses conditional XAML.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionalXamlInResourceDictionary)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when a ResourceDictionary uses conditional XAML.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDuplicateResourceKeysThrowsError)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an error is thrown if conditional XAML results in duplicate resources keys.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionalXamlInStyle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when a Style uses conditional XAML.")
            // This test runs in compat mode to apply style immediately.
            TEST_METHOD_PROPERTY(L"Data:PerfOptIn", L"{false}")
            TEST_METHOD_PROPERTY(L"Data:XamlOptionalChanges", L"{OptimizeApplyStyles:false}")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionalXamlInVsm)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when a VSM uses conditional XAML.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyConditionalXamlOnThemeDictionaryAfterMainContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates behavior when a ThemeDictionary uses conditional XAML, but is specified after the parent dictionary's primary content.")
            TEST_METHOD_PROPERTY(L"RegressionBug", L"MSVSO:18031587")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyXamlPredicateServiceStringLifetime)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the strings cached by XamlPredicateService survive past individual CCoreServices shutdown.")
            TEST_METHOD_PROPERTY(L"Bug", L"MSVSO:29271514")
        END_TEST_METHOD()

    };

} } } } } }
