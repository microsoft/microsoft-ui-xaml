// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ConditionalXamlTests.h"

#include <minerror.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <TestCleanupWrapper.h>
#include <XamlTailored.h>
#include <XamlResourcePropertyBagOverrider.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace {
    // Passes the specified buffer into XamlReader.Load() and returns the result (with and without XBFv2 encoding)
    template<class T>
    std::vector<T^> LoadXamlHelper(const wchar_t* buffer, bool errorExpected = false)
    {
        std::vector<T^> results;

        LOG_OUTPUT(L"Loading XAML buffer with XBFv2 turned off.");
        {
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

            RunOnUIThread([&]()
            {
                if (errorExpected)
                {
                    VERIFY_THROWS_WINRT(xaml_markup::XamlReader::Load(Platform::StringReference(buffer)), Platform::Exception^);
                }
                else
                {
                    auto result = safe_cast<T^>(xaml_markup::XamlReader::Load(Platform::StringReference(buffer)));
                    result->UpdateLayout();
                    results.push_back(result);
                }
            });
        }

        LOG_OUTPUT(L"Loading XAML buffer with XBFv2 turned on.");
        {
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            RunOnUIThread([&]()
            {
                if (errorExpected)
                {
                    VERIFY_THROWS_WINRT(xaml_markup::XamlReader::Load(Platform::StringReference(buffer)), Platform::Exception^);
                }
                else
                {
                    auto result = safe_cast<T^>(xaml_markup::XamlReader::Load(Platform::StringReference(buffer)));
                    result->UpdateLayout();
                    results.push_back(result);
                }
            });
        }

        return results;
    }

    // Verifies that the trees returned by LoadXamlHelper() have the expected strings.
    // Assumption is that they are Panels of TextBlocks, with the strings exposed via the Text properties
    // of the child TextBlocks.
    void VerifyPanelOfTextBlocks(std::vector<xaml_controls::Panel^> trees, std::vector<Platform::StringReference> expectedStrings)
    {
        LOG_OUTPUT(L"Verifying that output matches expectations.");
        RunOnUIThread([&]()
        {
            for (const auto tree : trees)
            {
                auto panel = safe_cast<xaml_controls::Panel^>(tree);

                VERIFY_ARE_EQUAL(expectedStrings.size(), panel->Children->Size);

                for (unsigned int i = 0; i < expectedStrings.size(); i++)
                {
                    VERIFY_ARE_EQUAL(expectedStrings[i], safe_cast<xaml_controls::TextBlock^>(panel->Children->GetAt(i))->Text);
                }
            }
        });
    }
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Parser {

    bool ConditionalXamlTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ConditionalXamlTests::ClassCleanup()
    {
        return true;
    }

    bool ConditionalXamlTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }


    bool ConditionalXamlTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ConditionalXamlTests::VerifyBuiltinPredicates()
    {
        TestCleanupWrapper cleanup;

        const wchar_t markup[] = 
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:winui17_with_minor='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8,0)'"
            L"  xmlns:winui16_with_nonzero_minor='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,7,3)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'"
            L"  xmlns:if_no_bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractNotPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'"
            L"  xmlns:border_child_present='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsPropertyPresent(Microsoft.UI.Xaml.Controls.Border,Child)'"
            L"  xmlns:no_border_child_present='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsPropertyNotPresent(Microsoft.UI.Xaml.Controls.Border,Child)'"
            L"  xmlns:border_present='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsTypePresent(Microsoft.UI.Xaml.Controls.Border)'"
            L"  xmlns:border_not_present='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsTypeNotPresent(Microsoft.UI.Xaml.Controls.Border)'>"
            L"  <winui17:TextBlock Text='Only present if WinUI 1.7 contract exists.' />"
            L"  <winui17_with_minor:TextBlock Text='Only present if WinUI 1.7 [v8.0] contract exists.' />"
            L"  <winui16_with_nonzero_minor:TextBlock Text='Only present if WinUI 1.7 contract exists (WinUI 1.7 [v8.0] implies WinUI 1.6 [v7.3]).' />"
            L"  <bogus_contract:TextBlock Text='Only present if that bogus contract exists' />"
            L"  <if_no_bogus_contract:TextBlock Text='Only present if that bogus contract does not exist.' />"
            L"  <border_child_present:TextBlock Text='Only present if MUX.Border.Child property exists.' />"
            L"  <no_border_child_present:TextBlock Text='Only present if MUX.Border.Child property does not exist.' />"
            L"  <border_present:TextBlock Text='Only present if MUX.Border type exists.' />"
            L"  <border_not_present:TextBlock Text='Only present if MUX.Border type does not exist.' />"
            L"</StackPanel>";

        std::vector<Platform::StringReference> expectedStrings;
        expectedStrings.push_back(Platform::StringReference(L"Only present if WinUI 1.7 contract exists."));
        expectedStrings.push_back(Platform::StringReference(L"Only present if WinUI 1.7 [v8.0] contract exists."));
        expectedStrings.push_back(Platform::StringReference(L"Only present if WinUI 1.7 contract exists (WinUI 1.7 [v8.0] implies WinUI 1.6 [v7.3])."));
        expectedStrings.push_back(Platform::StringReference(L"Only present if that bogus contract does not exist."));
        expectedStrings.push_back(Platform::StringReference(L"Only present if MUX.Border.Child property exists."));
        expectedStrings.push_back(Platform::StringReference(L"Only present if MUX.Border type exists."));

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);
        VerifyPanelOfTextBlocks(results, expectedStrings);
    }

    void ConditionalXamlTests::VerifyConditionallyDeclaredUnknownTypes()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating unknown type conditionally declared with a predicate which evaluates to false.");
        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock Text='So then it&apos;s up with the Blue and Gold' />"
            L"  <bogus_contract:YetAnotherUnknownType Text='Down with the Red' />"
            L"</StackPanel>";

        std::vector<Platform::StringReference> expectedStrings;
        expectedStrings.push_back(Platform::StringReference(L"So then it\'s up with the Blue and Gold"));

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);
        VerifyPanelOfTextBlocks(results, expectedStrings);

        LOG_OUTPUT(L"Validating unknown type conditionally declared with a predicate which evaluates to true.");
        const wchar_t markup2[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'>"
            L"  <TextBlock Text='So then it&apos;s up with the Blue and Gold' />"
            L"  <winui17:YetAnotherUnknownType Text='Down with the Red' />"
            L"</StackPanel>";

        results = LoadXamlHelper<xaml_controls::Panel>(markup2, true /* error expected */);
    }

    void ConditionalXamlTests::VerifyConditionallyDeclaredUnknownProperties()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating unknown property conditionally declared with a predicate which evaluates to false.");
        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock Text='So then it&apos;s up with the Blue and Gold' />"
            L"  <TextBlock bogus_contract:BrandNewTextProperty='Down with the Red' />"
            L"</StackPanel>";

        std::vector<Platform::StringReference> expectedStrings;
        expectedStrings.push_back(Platform::StringReference(L"So then it\'s up with the Blue and Gold"));
        expectedStrings.push_back(Platform::StringReference(L""));

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);
        VerifyPanelOfTextBlocks(results, expectedStrings);

        LOG_OUTPUT(L"Validating unknown property conditionally declared with a predicate which evaluates to true.");
        const wchar_t markup2[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'>"
            L"  <TextBlock Text='So then it&apos;s up with the Blue and Gold' />"
            L"  <TextBlock winui17:BrandNewTextProperty='Down with the Red' />"
            L"</StackPanel>";

        results = LoadXamlHelper<xaml_controls::Panel>(markup2, true /* error expected */);
    }

    void ConditionalXamlTests::VerifyConditionallyDeclaredKnownProperties()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating conditionally declared known properties.");
        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock winui17:Text='So then it&apos;s up with the Blue and Gold' bogus_contract:Text='Down with the Red' />"
            L"  <winui17:TextBlock Name='bears' winui17:Text='California&apos;s out for a victory' />"
            L"  <TextBlock bogus_contract:Name='cardinal' x:Name='chop'>"
            L"    <winui17:TextBlock.Text>We&apos;ll drop our battle axe on Stanford&apos;s head</winui17:TextBlock.Text>"
            L"  </TextBlock>"
            L"  <TextBlock>"
            L"    <bogus_contract:TextBlock.Text>When we meet her, our team will surely beat her.</bogus_contract:TextBlock.Text>"
            L"  </TextBlock>"
            L"</StackPanel>";

        std::vector<Platform::StringReference> expectedStrings;
        expectedStrings.push_back(Platform::StringReference(L"So then it\'s up with the Blue and Gold"));
        expectedStrings.push_back(Platform::StringReference(L"California\'s out for a victory"));
        expectedStrings.push_back(Platform::StringReference(L"We\'ll drop our battle axe on Stanford\'s head"));
        expectedStrings.push_back(Platform::StringReference(L""));

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);
        VerifyPanelOfTextBlocks(results, expectedStrings);

        RunOnUIThread([&]()
        {
            for (const auto& result : results)
            {
                auto panel = safe_cast<xaml_controls::Panel^>(result);

                VERIFY_ARE_EQUAL(Platform::StringReference(L"bears"), safe_cast<xaml_controls::TextBlock^>(panel->Children->GetAt(1))->Name);
                VERIFY_ARE_EQUAL(Platform::StringReference(L"chop"), safe_cast<xaml_controls::TextBlock^>(panel->Children->GetAt(2))->Name);
            }
        });
    }

    void ConditionalXamlTests::VerifyDuplicatePropertiesThrowsError()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating unknown type conditionally declared with a predicate which evaluates to false.");
        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock Text='So then it&apos;s up with the Blue and Gold' winui17:Text='Down with the Red'/>"
            L"</StackPanel>";

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup, true /* error expected */);
    }

    void ConditionalXamlTests::VerifyConditionallyDeclaredPropertySetByThemeResource()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating that a conditionally declared property can be set by a {ThemeResource}");
        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'>"
            L"  <StackPanel.Resources>"
            L"    <ResourceDictionary>"
            L"      <ResourceDictionary.ThemeDictionaries>"
            L"        <ResourceDictionary x:Key='Default'>"
            L"          <x:String x:Key='TextContent'>Go Bears!</x:String>"
            L"        </ResourceDictionary>"
            L"      </ResourceDictionary.ThemeDictionaries>"
            L"    </ResourceDictionary>"
            L"  </StackPanel.Resources>"
            L"  <TextBlock winui17:Text='{ThemeResource TextContent}'/>"
            L"</StackPanel>";

        std::vector<Platform::StringReference> expectedStrings;
        expectedStrings.push_back(Platform::StringReference(L"Go Bears!"));

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);
        VerifyPanelOfTextBlocks(results, expectedStrings);
    }

    void ConditionalXamlTests::VerifyConditionallyDeclaredPropertySetByStaticResource()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating that a conditionally declared property can be set by a {StaticResource}");
        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'>"
            L"  <StackPanel.Resources>"
            L"    <x:String x:Key='TextContent'>Go Bears!</x:String>"
            L"  </StackPanel.Resources>"
            L"  <TextBlock winui17:Text='{StaticResource TextContent}'/>"
            L"</StackPanel>";

        std::vector<Platform::StringReference> expectedStrings;
        expectedStrings.push_back(Platform::StringReference(L"Go Bears!"));

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);
        VerifyPanelOfTextBlocks(results, expectedStrings);
    }

    void ConditionalXamlTests::VerifyConditionalXamlWithXUid()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating that a conditionally declared property can be replaced by x:Uid.");
        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock winui17:Text='placeholder' x:Uid='TextBlock1'/>"
            L"  <TextBlock bogus_contract:Text='placeholder' x:Uid='TextBlock2'/>"
            L"</StackPanel>";

        // Create resource replacement values that will be used 
        // for x:Uid at load time via the override.
        std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>> map;
        std::vector<std::pair<std::wstring, std::wstring>> entries;
        entries.push_back(std::make_pair(L"Text", L"Cal"));
        map.emplace(L"TextBlock1", std::move(entries));
        std::vector<std::pair<std::wstring, std::wstring>> entries2;
        entries.push_back(std::make_pair(L"Text", L"Stanfurd"));
        map.emplace(L"TextBlock2", std::move(entries));

        std::vector<Platform::StringReference> expectedStrings;
        expectedStrings.push_back(Platform::StringReference(L"Cal"));
        expectedStrings.push_back(Platform::StringReference(L"Stanfurd"));

        std::shared_ptr<XamlResourcePropertyBagOverrider> propertyBagOverride;
        RunOnUIThread([&]()
        {
            propertyBagOverride = std::make_shared<XamlResourcePropertyBagOverrider>(&map);
        });

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);
        VerifyPanelOfTextBlocks(results, expectedStrings);

        RunOnUIThread([&]()
        {
            propertyBagOverride.reset();
        });
    }

    void ConditionalXamlTests::VerifyConditionalXamlWithTypeConversion()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating that a conditionally declared property requiring type conversion is set correctly.");
        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock winui17:Width='200' />"
            L"  <Rectangle winui17:Fill='Blue' />"
            L"</StackPanel>";

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);

        RunOnUIThread([&]()
        {
            for (const auto& result : results)
            {
                auto panel = safe_cast<xaml_controls::Panel^>(result);

                VERIFY_ARE_EQUAL(200, safe_cast<xaml_controls::TextBlock^>(panel->Children->GetAt(0))->Width);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(safe_cast<xaml_shapes::Rectangle^>(panel->Children->GetAt(1))->Fill)->Color);
            }
        });
    }

    void ConditionalXamlTests::VerifyConditionalXamlInTemplate()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating that conditional XAML can be used inside a template.");
        const wchar_t markup[] =
            L"<Button"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='Button'>"
            L"      <StackPanel Orientation='Horizontal'>"
            L"        <TextBlock Text='blue' Foreground='Blue' />"
            L"        <winui17:TextBlock Text='gold' Foreground='Gold' />"
            L"        <bogus_contract:TextBlock Text='red' Foreground='Red' />"
            L"      </StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

        auto results = LoadXamlHelper<xaml_controls::Button>(markup);

        for (const auto& result : results)
        {
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = result;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto stackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_media::VisualTreeHelper::GetChild(result, 0));
                VERIFY_ARE_EQUAL(2, xaml_media::VisualTreeHelper::GetChildrenCount(stackPanel));

                VERIFY_ARE_EQUAL(Platform::StringReference(L"blue"), safe_cast<xaml_controls::TextBlock^>(stackPanel->Children->GetAt(0))->Text);
                VERIFY_ARE_EQUAL(Platform::StringReference(L"gold"), safe_cast<xaml_controls::TextBlock^>(stackPanel->Children->GetAt(1))->Text);
            });
        }
        
        LOG_OUTPUT(L"Validating that conditional XAML on the content object of a template.");
        const wchar_t markup2[] =
            L"<Button"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='Button'>"
            L"      <bogus_contract:StackPanel Orientation='Horizontal'>"
            L"        <TextBlock Text='blue' Foreground='Blue' />"
            L"        <winui17:TextBlock Text='gold' Foreground='Gold' />"
            L"        <bogus_contract:TextBlock Text='red' Foreground='Red' />"
            L"      </bogus_contract:StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

        results = LoadXamlHelper<xaml_controls::Button>(markup2);
    }
    
    void ConditionalXamlTests::VerifyConditionalXamlInResourceDictionary()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating that conditional XAML can be used inside a ResourceDictionary.");

        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17_intrinsics='http://schemas.microsoft.com/winfx/2006/xaml?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract_intrinsics='http://schemas.microsoft.com/winfx/2006/xaml?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <StackPanel.Resources>"
            L"    <winui17_intrinsics:String x:Key='TextContent'>Go Bears!</winui17_intrinsics:String>"
            L"    <bogus_contract_intrinsics:String x:Key='TextContent'>Go Cardinals!</bogus_contract_intrinsics:String>"
            L"    <x:String x:Key='TextContent2'>Just a regular string.</x:String>"
            L"  </StackPanel.Resources>"
            L"  <TextBlock Text='{StaticResource TextContent}'/>"
            L"  <TextBlock Text='{StaticResource TextContent2}'/>"
            L"  <TextBlock Text='{StaticResource TextContent}'/>"
            L"</StackPanel>";

        std::vector<Platform::StringReference> expectedStrings;
        expectedStrings.push_back(Platform::StringReference(L"Go Bears!"));
        expectedStrings.push_back(Platform::StringReference(L"Just a regular string."));
        expectedStrings.push_back(Platform::StringReference(L"Go Bears!"));

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);
        VerifyPanelOfTextBlocks(results, expectedStrings);

        LOG_OUTPUT(L"Verifying ResourceDictionary contents.");
        std::vector<std::pair<Platform::StringReference, Platform::StringReference>> expectedResources;
        expectedResources.push_back(std::make_pair(Platform::StringReference(L"TextContent"), Platform::StringReference(L"Go Bears!")));
        expectedResources.push_back(std::make_pair(Platform::StringReference(L"TextContent2"), Platform::StringReference(L"Just a regular string.")));
        RunOnUIThread([&]()
        {
            for (const auto result : results)
            {
                auto panel = safe_cast<xaml_controls::Panel^>(result);

                VERIFY_ARE_EQUAL(expectedResources.size(), panel->Resources->Size);

                for (size_t i = 0; i < expectedResources.size(); i++)
                {
                    auto actualString = static_cast<::Windows::Foundation::IPropertyValue^>(panel->Resources->Lookup(expectedResources[i].first))->GetString();
                    VERIFY_ARE_EQUAL(expectedResources[i].second, actualString);
                }
            }
        });
    }

    void ConditionalXamlTests::VerifyDuplicateResourceKeysThrowsError()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating that an error is thrown if conditional XAML results in duplicate resource keys.");

        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17_intrinsics='http://schemas.microsoft.com/winfx/2006/xaml?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract_intrinsics='http://schemas.microsoft.com/winfx/2006/xaml?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <StackPanel.Resources>"
            L"    <winui17_intrinsics::String x:Key='TextContent'>Go Bears!</winui17_intrinsics::String>"
            L"    <x:String x:Key='TextContent'>Just a regular string.</x:String>"
            L"  </StackPanel.Resources>"
            L"  <TextBlock Text='{StaticResource TextContent}'/>"
            L"</StackPanel>";

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup, true);

        // Check duplicate keys declared in reverse order
        const wchar_t markup2[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17_intrinsics='http://schemas.microsoft.com/winfx/2006/xaml?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract_intrinsics='http://schemas.microsoft.com/winfx/2006/xaml?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <StackPanel.Resources>"
            L"    <x:String x:Key='TextContent'>Just a regular string.</x:String>"
            L"    <winui17_intrinsics::String x:Key='TextContent'>Go Bears!</winui17_intrinsics::String>"
            L"  </StackPanel.Resources>"
            L"  <TextBlock Text='{StaticResource TextContent}'/>"
            L"</StackPanel>";

        results = LoadXamlHelper<xaml_controls::Panel>(markup, true);
    }

    void ConditionalXamlTests::VerifyConditionalXamlInStyle()
    {
        TestCleanupWrapper cleanup;
        // Run this test in compat mode so style is applied immediately (during CreationComplete) without needing to be added to the tree.
        VERIFY_IS_FALSE(xaml_settings::XamlOptionalChanges::IsChangeEnabled(xaml_settings::XamlChangeId::DelayApplyStyleOptimization));

        LOG_OUTPUT(L"Validating that conditional XAML can be used inside a Style's Setter.Value(s).");
        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock>"
            L"    <TextBlock.Style>"
            L"      <Style TargetType='TextBlock'>"
            L"        <Style.Setters>"
            L"          <Setter Property='SelectionHighlightColor'>"
            L"            <Setter.Value>"
            L"              <SolidColorBrush>"
            L"                <winui17:SolidColorBrush.Color>Blue</winui17:SolidColorBrush.Color>"
            L"                <bogus_contract:SolidColorBrush.Color>Red</bogus_contract:SolidColorBrush.Color>"
            L"              </SolidColorBrush>"
            L"            </Setter.Value>"
            L"          </Setter>"
            L"          <Setter Property='Foreground'>"
            L"            <Setter.Value>"
            L"              <SolidColorBrush>"
            L"                <winui17:SolidColorBrush.Color>Gold</winui17:SolidColorBrush.Color>"
            L"                <bogus_contract:SolidColorBrush.Color>White</bogus_contract:SolidColorBrush.Color>"
            L"              </SolidColorBrush>"
            L"            </Setter.Value>"
            L"          </Setter>"
            L"        </Style.Setters>"
            L"      </Style>"
            L"    </TextBlock.Style>"
            L"  </TextBlock>"
            L"</StackPanel>";

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);

        RunOnUIThread([&]()
        {
            for (const auto& result : results)
            {
                auto panel = safe_cast<xaml_controls::Panel^>(result);
                auto textBlock = safe_cast<xaml_controls::TextBlock^>(panel->Children->GetAt(0));

                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(textBlock->SelectionHighlightColor)->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Gold, safe_cast<xaml_media::SolidColorBrush^>(textBlock->Foreground)->Color);
            }
        });

        LOG_OUTPUT(L"Validating that conditional XAML can be used directly on a Style's Setter.Value.");
        const wchar_t markup2[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock>"
            L"    <TextBlock.Style>"
            L"      <Style TargetType='TextBlock'>"
            L"        <Style.Setters>"
            L"          <Setter Property='SelectionHighlightColor'>"
            L"            <winui17:Setter.Value>"
            L"              <SolidColorBrush>"
            L"                <SolidColorBrush.Color>Blue</SolidColorBrush.Color>"
            L"              </SolidColorBrush>"
            L"            </winui17:Setter.Value>"
            L"            <bogus_contract:Setter.Value>"
            L"              <SolidColorBrush>"
            L"                <SolidColorBrush.Color>Red</SolidColorBrush.Color>"
            L"              </SolidColorBrush>"
            L"            </bogus_contract:Setter.Value>"
            L"          </Setter>"
            L"          <Setter Property='Foreground' winui17:Value='Gold' bogus_contract:Value='White' />"
            L"        </Style.Setters>"
            L"      </Style>"
            L"    </TextBlock.Style>"
            L"  </TextBlock>"
            L"</StackPanel>";

        auto results2 = LoadXamlHelper<xaml_controls::Panel>(markup2);

        RunOnUIThread([&]()
        {
            for (const auto& result : results2)
            {
                auto panel = safe_cast<xaml_controls::Panel^>(result);
                auto textBlock = safe_cast<xaml_controls::TextBlock^>(panel->Children->GetAt(0));

                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(textBlock->SelectionHighlightColor)->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Gold, safe_cast<xaml_media::SolidColorBrush^>(textBlock->Foreground)->Color);
            }
        });

        LOG_OUTPUT(L"Validating that conditional XAML can be used directly on a Style's Setter.Property.");
        const wchar_t markup3[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock>"
            L"    <TextBlock.Style>"
            L"      <Style TargetType='TextBlock'>"
            L"        <Style.Setters>"
            L"          <Setter winui17:Property='SelectionHighlightColor' bogus_contract:Property='Text' Value='Blue'  />"
            L"          <Setter Property='Foreground' Value='Gold' />"
            L"        </Style.Setters>"
            L"      </Style>"
            L"    </TextBlock.Style>"
            L"  </TextBlock>"
            L"</StackPanel>";

        auto results3 = LoadXamlHelper<xaml_controls::Panel>(markup3);

        RunOnUIThread([&]()
        {
            for (const auto& result : results3)
            {
                auto panel = safe_cast<xaml_controls::Panel^>(result);
                auto textBlock = safe_cast<xaml_controls::TextBlock^>(panel->Children->GetAt(0));

                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(textBlock->SelectionHighlightColor)->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Gold, safe_cast<xaml_media::SolidColorBrush^>(textBlock->Foreground)->Color);
            }
        });        

        LOG_OUTPUT(L"Validating that conditional XAML can be used directly on a Style's Setter.");
        const wchar_t markup4[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock>"
            L"    <TextBlock.Style>"
            L"      <Style TargetType='TextBlock'>"
            L"        <Style.Setters>"
            L"          <winui17:Setter Property='SelectionHighlightColor' Value='Blue' />"
            L"          <bogus_contract:Setter Property='SelectionHighlightColor' Value='White' />"
            L"          <winui17:Setter Property='Foreground' Value='Gold' />"
            L"          <bogus_contract:Setter Property='Foreground' Value='White' />"
            L"        </Style.Setters>"
            L"      </Style>"
            L"    </TextBlock.Style>"
            L"  </TextBlock>"
            L"</StackPanel>";

        auto results4 = LoadXamlHelper<xaml_controls::Panel>(markup4);

        RunOnUIThread([&]()
        {
            for (const auto& result : results4)
            {
                auto panel = safe_cast<xaml_controls::Panel^>(result);
                auto textBlock = safe_cast<xaml_controls::TextBlock^>(panel->Children->GetAt(0));

                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(textBlock->SelectionHighlightColor)->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Gold, safe_cast<xaml_media::SolidColorBrush^>(textBlock->Foreground)->Color);
            }
        });

        LOG_OUTPUT(L"Validating that conditional XAML can be used directly on the Style.Setters collection.");
        const wchar_t markup5[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock>"
            L"    <TextBlock.Style>"
            L"      <Style TargetType='TextBlock'>"
            L"        <winui17:Style.Setters>"
            L"          <Setter Property='SelectionHighlightColor' Value='Blue' />"
            L"          <Setter Property='Foreground' Value='Gold' />"
            L"        </winui17:Style.Setters>"
            L"        <bogus_contract:Style.Setters>"
            L"          <Setter Property='SelectionHighlightColor' Value='White' />"
            L"          <Setter Property='Foreground' Value='White' />"
            L"        </bogus_contract:Style.Setters>"
            L"      </Style>"
            L"    </TextBlock.Style>"
            L"  </TextBlock>"
            L"</StackPanel>";

        auto results5 = LoadXamlHelper<xaml_controls::Panel>(markup5);

        RunOnUIThread([&]()
        {
            for (const auto& result : results5)
            {
                auto panel = safe_cast<xaml_controls::Panel^>(result);
                auto textBlock = safe_cast<xaml_controls::TextBlock^>(panel->Children->GetAt(0));

                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(textBlock->SelectionHighlightColor)->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Gold, safe_cast<xaml_media::SolidColorBrush^>(textBlock->Foreground)->Color);
            }
        });
    }

    void ConditionalXamlTests::VerifyConditionalXamlInVsm()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating that conditional XAML can be used inside a VSM.");
        const wchar_t markup[] =
            L"<UserControl"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"      <VisualStateGroup x:Name='Group1'>"
            L"        <VisualState x:Name='VisualState1' />"
            L"        <VisualState x:Name='VisualState2'>"
            L"          <VisualState.Setters>"
            L"            <winui17:Setter Target='Rect2.(Shape.Fill).(SolidColorBrush.Color)' Value='Green' />"
            L"            <winui17:Setter Target='Rect2.Width' Value='200' />"
            L"          </VisualState.Setters>"
            L"          <bogus_contract:Storyboard>"
            L"             <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Rect1'"
            L"               Storyboard.TargetProperty='Width'>"
            L"               <DiscreteObjectKeyFrame KeyTime='0' Value='300' />"
            L"             </ObjectAnimationUsingKeyFrames>"
            L"          </bogus_contract:Storyboard>"
            L"        </VisualState>"
            L"        <bogus_contract:VisualState x:Name='VisualState3'>"
            L"          <VisualState.Setters>"
            L"            <Setter Target='Rect1.Width' Value='150' />"
            L"            <Setter Target='Rect2.(Shape.Fill).(SolidColorBrush.Color)' Value='Purple' />"
            L"            <Setter Target='Rect2.(Shape.Fill).(SolidColorBrush.Color)' Value='Yellow' />"
            L"            <Setter Target='Rect2.Width' Value='300' />"
            L"          </VisualState.Setters>"
            L"        </bogus_contract:VisualState>"
            L"      </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <Rectangle Width='100' Height='100' Fill='Red' x:Name='Rect1' />"
            L"    <Rectangle Width='100' Height='100' Fill='Blue' x:Name='Rect2' />"
            L"  </StackPanel>"
            L"</UserControl>";

        auto results = LoadXamlHelper<xaml_controls::UserControl>(markup);

        
        for (const auto& result : results)
        {
            xaml_controls::StackPanel^ stackPanel;
            xaml_shapes::Rectangle^ rect1;
            xaml_shapes::Rectangle^ rect2;

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = result;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                stackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_media::VisualTreeHelper::GetChild(result, 0));
                rect1 = safe_cast<xaml_shapes::Rectangle^>(stackPanel->Children->GetAt(0));
                rect2 = safe_cast<xaml_shapes::Rectangle^>(stackPanel->Children->GetAt(1));

                VERIFY_IS_TRUE(VisualStateManager::GoToState(result, Platform::StringReference(L"VisualState2"), false));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Checking VisualState2");
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, safe_cast<xaml_media::SolidColorBrush^>(rect1->Fill)->Color);
                VERIFY_ARE_EQUAL(100, rect1->Width);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, safe_cast<xaml_media::SolidColorBrush^>(rect2->Fill)->Color);
                VERIFY_ARE_EQUAL(200, rect2->Width);

                VERIFY_IS_FALSE(VisualStateManager::GoToState(result, Platform::StringReference(L"VisualState3"), false));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Checking VisualState3");
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, safe_cast<xaml_media::SolidColorBrush^>(rect1->Fill)->Color);
                VERIFY_ARE_EQUAL(100, rect1->Width);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, safe_cast<xaml_media::SolidColorBrush^>(rect2->Fill)->Color);
                VERIFY_ARE_EQUAL(200, rect2->Width);
            });
        }
    }

    void ConditionalXamlTests::VerifyConditionalXamlOnThemeDictionaryAfterMainContent()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating that conditional XAML can be used in a ThemeDictionary specified after the parent dictionary's primary content");

        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:contract5NotPresent='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractNotPresent(Microsoft.UI.Xaml.WinUIContract,5)'"
            L"  xmlns:contract5Present='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,5)'>"
            L"  <StackPanel.Resources>"
            L"    <ResourceDictionary>"
            L"      <Thickness x:Key='HeaderMargin'>20,41,0,0</Thickness>"
            L"      <ResourceDictionary.ThemeDictionaries>"
            L"        <ResourceDictionary x:Key='Light'>"
            L"          <Color x:Key='BrandColor'>#D1D1D1</Color>"
            L"          <contract5Present:SolidColorBrush x:Key='OptionsPaneBackground' Color='{StaticResource BrandColor}' />"
            L"        </ResourceDictionary>"
            L"        <ResourceDictionary x:Key='Dark'>"
            L"          <Color x:Key='BrandColor'>#3F3F46</Color>"
            L"          <contract5Present:SolidColorBrush x:Key='OptionsPaneBackground' Color='{StaticResource BrandColor}' />"
            L"        </ResourceDictionary>"
            L"      </ResourceDictionary.ThemeDictionaries>"
            L"    </ResourceDictionary>"
            L"  </StackPanel.Resources>"
            L"</StackPanel>";

        auto results = LoadXamlHelper<xaml_controls::Panel>(markup);
    }

    void ConditionalXamlTests::VerifyXamlPredicateServiceStringLifetime()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

        LOG_OUTPUT(L"Validating that the strings cached by XamlPredicateService survive past individual CCoreServices shutdown.");

        auto loadPageWithConditionalXamlHelper = [&]()
        {
            LOG_OUTPUT(L"Loading XBF file with conditional XAML.");
            RunOnUIThread([&]()
            {
                auto page = ref new xaml_controls::Page();

                Application::LoadComponent(
                    page,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/parser/PageWithConditionalXaml.xaml"),
                    xaml_primitives::ComponentResourceLocation::Application);
            });
            TestServices::WindowHelper->WaitForIdle();
        };

        loadPageWithConditionalXamlHelper();

        LOG_OUTPUT(L"Shutting down XAML");
        test_infra::TestServices::WindowHelper->ShutdownXaml();

        LOG_OUTPUT(L"Re-initializing XAML");
        test_infra::TestServices::WindowHelper->InitializeXaml();

        // Try to force freed memory to get used for something else
        const wchar_t markup[] =
            L"<StackPanel"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:winui17_intrinsics='http://schemas.microsoft.com/winfx/2006/xaml?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)'"
            L"  xmlns:bogus_contract_intrinsics='http://schemas.microsoft.com/winfx/2006/xaml?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"  <TextBlock Text='Lorem Ipsum'/>"
            L"  <Rectangle Fill='Pink' Height='250' Width='275' />"
            L"  <Rectangle Fill='Blue' Height='150' Width='375' />"
            L"</StackPanel>";
        RunOnUIThread([&]()
        {
            auto result = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(Platform::StringReference(markup)));
            result->UpdateLayout();
            TestServices::WindowHelper->WindowContent = result;
        });
        TestServices::WindowHelper->WaitForIdle();

        loadPageWithConditionalXamlHelper();
    }

} } } } } }
