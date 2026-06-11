// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ScopedResourcesTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace ::Windows::UI;

namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Framework {

        bool ScopedResourcesTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            featureEnforceXbfV2Stream.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
            return true;
        }

        bool ScopedResourcesTests::ClassCleanup()
        {
            return true;
        }

        bool ScopedResourcesTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool ScopedResourcesTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        enum class ResourceRefType
        {
            StaticResourceRef,
            ThemeResourceRef
        };

        enum class DictionaryType
        {
            MergedDictionary,
            ThemeDictionary
        };

        const wchar_t* ToString(ResourceRefType refType)
        {
            return (refType == ResourceRefType::StaticResourceRef) ? L"StaticResource" : L"ThemeResource";
        }

        const wchar_t* ToString(DictionaryType dictType)
        {
            return (dictType == DictionaryType::MergedDictionary) ? L"MergedDictionaries" : L"ThemeDictionaries";
        }

        static void ValidateRDContents(ResourceDictionary^ rd, const std::vector<const wchar_t*>& v)
        {
            auto vCopy = v;
            auto iter = rd->First();

            while (iter->HasCurrent)
            {
                const wchar_t* curr = safe_cast<Platform::String^>(iter->Current->Key)->Data();

                auto f = std::find_if(
                    vCopy.begin(),
                    vCopy.end(),
                    [curr](const wchar_t* str)
                    {
                        return String(curr) == String(str);
                    });

                if (f != vCopy.end())
                {
                    vCopy.erase(f);
                }
                else
                {
                    // element not found
                    VERIFY_FAIL();
                }

                iter->MoveNext();
            }

            // unexpected element if fail
            VERIFY_IS_TRUE(vCopy.empty());
        }

        void ScopedResourcesTests::IMapAPITests()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto res = ref new ColorPaletteResources();

                // VERIFY_IS_FALSE(res->HasKey(L"BaseLow"));
                VERIFY_ARE_EQUAL(0U, res->Size);
                res->BaseLow = Colors::DarkRed;
                // VERIFY_IS_TRUE(res->HasKey(L"BaseLow"));
                VERIFY_ARE_EQUAL(1U, res->Size);

                std::vector<const wchar_t*> ref;
                ref.push_back(L"SystemBaseLowColor");
                ValidateRDContents(res, ref);

                // VERIFY_IS_FALSE(res->HasKey(L"ButtonBackground"));
                res->Lookup(L"ButtonBackground");
                // VERIFY_IS_TRUE(res->HasKey(L"ButtonBackground"));
                //VERIFY_ARE_EQUAL(2U, res->Size);
                //ref.push_back(L"ButtonBackground");
                //ValidateRDContents(res, ref);
            });
        }

        void ScopedResourcesTests::BasicFunctionality()
        {
            TestCleanupWrapper cleanup;

            Log::Comment(L"Color tests");

            RunOnUIThread([&]()
            {
                {
                    Log::Comment(L" * Default value");
                    auto res = ref new ColorPaletteResources();
                    VERIFY_ARE_EQUAL(nullptr, res->Accent);
                }

                {
                    Log::Comment(L" * Set and get via code");
                    auto res = ref new ColorPaletteResources();

                    res->Accent = Colors::DarkRed;
                    VERIFY_ARE_EQUAL(Colors::DarkRed, res->Accent->Value);

                    Log::Comment(L" * Set and get again");
                    res->Accent = Colors::Red;
                    VERIFY_ARE_EQUAL(Colors::Red, res->Accent->Value);
                }

                {
                    Log::Comment(L" * Property and x:key present -- throw");
                    DisableErrorReportingScopeGuard disableErrors;

                    VERIFY_THROWS_WINRT(safe_cast<ColorPaletteResources^>(xaml_markup::XamlReader::Load(
                        L"<ColorPaletteResources xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Accent='Red'>"
                        L"  <Color x:Key='SystemAccentColor'>Green</Color>"
                        L"</ColorPaletteResources>")),
                        Platform::COMException^,
                        L"XAML parse exception should be thrown when color property and resource are specified");
                }

                {
                    Log::Comment(L" * Last one wins -- set via x:key then set via property");
                    auto resParsed = safe_cast<ColorPaletteResources^>(xaml_markup::XamlReader::Load(
                        L"<ColorPaletteResources xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"  <Color x:Key='SystemAccentColor'>Green</Color>"
                        L"</ColorPaletteResources>"));

                    resParsed->Accent = Colors::Red;
                    VERIFY_ARE_EQUAL(Colors::Red, resParsed->Accent->Value);
                }

                {
                    Log::Comment(L" * Set via x:key, get through property");
                    auto resParsed = safe_cast<ColorPaletteResources^>(xaml_markup::XamlReader::Load(
                        L"<ColorPaletteResources xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"  <Color x:Key='SystemAccentColor'>Green</Color>"
                        L"</ColorPaletteResources>"));

                    VERIFY_ARE_EQUAL(Colors::Green, resParsed->Accent->Value);
                }

                {
                    Log::Comment(L" * Insert key, get via property");
                    auto resCode = ref new ColorPaletteResources();
                    resCode->Insert(L"SystemAccentColor", Colors::Green);
                    resCode->Accent = Colors::Red;
                    VERIFY_ARE_EQUAL(Colors::Red, resCode->Accent->Value);
                }

                {
                    Log::Comment(L" * Set via property, overwrite through insert, get via property");
                    auto resCode = ref new ColorPaletteResources();
                    resCode->Accent = Colors::Red;
                    resCode->Insert(L"SystemAccentColor", Colors::Green);
                    VERIFY_ARE_EQUAL(Colors::Green, resCode->Accent->Value);
                }
            });

            Log::Comment(L"Brush tests");

            RunOnUIThread([&]()
            {
                {
                    Log::Comment(L" * Don't overwrite existing brush");
                    auto resParsed = safe_cast<ColorPaletteResources^>(xaml_markup::XamlReader::Load(
                        L"<ColorPaletteResources xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Accent='Red'>"
                        L"  <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Green'/>"
                        L"</ColorPaletteResources>"));

                    VERIFY_ARE_EQUAL(Colors::Green, safe_cast<SolidColorBrush^>(resParsed->Lookup(L"SystemControlBackgroundAccentBrush"))->Color);
                }

                {
                    Log::Comment(L" * Create from inserted color key");
                    auto resCode = ref new ColorPaletteResources();
                    resCode->Insert(L"SystemAccentColor", Colors::Red);
                    VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(resCode->Lookup(L"SystemControlBackgroundAccentBrush"))->Color);
                }

                {
                    Log::Comment(L" * Create from color set via property");
                    auto resCode = ref new ColorPaletteResources();
                    resCode->Accent = Colors::Red;
                    VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(resCode->Lookup(L"SystemControlBackgroundAccentBrush"))->Color);
                }
            });
        }

        static void ValidateAllProperties_Scenario(const wchar_t* keyName, const std::function<void(ColorPaletteResources^)>& setter)
        {
            RunOnUIThread([&]()
            {
                std::vector<const wchar_t*> ref;
                auto resCode = ref new ColorPaletteResources();
                setter(resCode);
                ref.push_back(keyName);
                ValidateRDContents(resCode, ref);
            });
        }

        void ScopedResourcesTests::ValidateAllProperties()
        {
            TestCleanupWrapper cleanup;

            ValidateAllProperties_Scenario(L"SystemAltHighColor", [](ColorPaletteResources^ res) { res->AltHigh = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemAltLowColor", [](ColorPaletteResources^ res) { res->AltLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemAltMediumColor", [](ColorPaletteResources^ res) { res->AltMedium = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemAltMediumHighColor", [](ColorPaletteResources^ res) { res->AltMediumHigh = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemAltMediumLowColor", [](ColorPaletteResources^ res) { res->AltMediumLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemBaseHighColor", [](ColorPaletteResources^ res) { res->BaseHigh = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemBaseLowColor", [](ColorPaletteResources^ res) { res->BaseLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemBaseMediumColor", [](ColorPaletteResources^ res) { res->BaseMedium = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemBaseMediumHighColor", [](ColorPaletteResources^ res) { res->BaseMediumHigh = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemBaseMediumLowColor", [](ColorPaletteResources^ res) { res->BaseMediumLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeAltLowColor", [](ColorPaletteResources^ res) { res->ChromeAltLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeBlackHighColor", [](ColorPaletteResources^ res) { res->ChromeBlackHigh = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeBlackLowColor", [](ColorPaletteResources^ res) { res->ChromeBlackLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeBlackMediumLowColor", [](ColorPaletteResources^ res) { res->ChromeBlackMediumLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeBlackMediumColor", [](ColorPaletteResources^ res) { res->ChromeBlackMedium = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeDisabledHighColor", [](ColorPaletteResources^ res) { res->ChromeDisabledHigh = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeDisabledLowColor", [](ColorPaletteResources^ res) { res->ChromeDisabledLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeHighColor", [](ColorPaletteResources^ res) { res->ChromeHigh = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeLowColor", [](ColorPaletteResources^ res) { res->ChromeLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeMediumColor", [](ColorPaletteResources^ res) { res->ChromeMedium = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeMediumLowColor", [](ColorPaletteResources^ res) { res->ChromeMediumLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeWhiteColor", [](ColorPaletteResources^ res) { res->ChromeWhite = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemChromeGrayColor", [](ColorPaletteResources^ res) { res->ChromeGray = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemListLowColor", [](ColorPaletteResources^ res) { res->ListLow = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemListMediumColor", [](ColorPaletteResources^ res) { res->ListMedium = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemAccentColor", [](ColorPaletteResources^ res) { res->Accent = Colors::Red; });
            ValidateAllProperties_Scenario(L"SystemErrorTextColor", [](ColorPaletteResources^ res) { res->ErrorText = Colors::Red; });
        }

        static void OverrideLookup_PropertySet(ResourceRefType refType)
        {
            Log::Comment(String().Format(
                L"* OverrideLookup_PropertySet, refType = %s",
                ToString(refType)));

            RunOnUIThread([&]()
            {
                auto root = safe_cast<Page^>(xaml_markup::XamlReader::Load(
                    Platform::StringReference(String().Format(
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"  <Page.Resources>"
                        L"    <ColorPaletteResources AltHigh='Red'/>"
                        L"  </Page.Resources>"
                        L"  <Grid>"
                        L"    <Button x:Name='obj' Background='{%s SystemControlBackgroundAltHighBrush}'/>"
                        L"  </Grid>"
                        L"</Page>",
                        ToString(refType)))));

                auto obj = safe_cast<Button^>(root->FindName(L"obj"));
                VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(obj->Background)->Color);
            });
        }

        static String EmitStylesDictionary(DictionaryType dictType, const wchar_t* loc, const wchar_t* color, bool implicit = false)
        {
            String style;

            if (!implicit)
            {
                style.Format(
                    L"      <Style x:Key='style_%s_direct' TargetType='Button'>"
                    L"        <Setter Property='Background' Value='{ThemeResource SystemControlBackgroundAltHighBrush}'/>"
                    L"      </Style>"
                    L"      <Style x:Key='style_%s_indirect_tr' TargetType='Button'>"
                    L"        <Setter Property='Background' Value='{ThemeResource tr_brush_ref}'/>"
                    L"      </Style>",
                    loc,
                    loc);
            }
            else
            {
                style.Format(
                    L"      <Style TargetType='Button'>"
                    L"        <Setter Property='Background' Value='{ThemeResource SystemControlBackgroundAltHighBrush}'/>"
                    L"      </Style>");
            }

            return String().Format(
                L"    <ResourceDictionary>"
                L"      <ResourceDictionary.%s>"
                L"        <ColorPaletteResources x:Key='Default' AltHigh='%s'>"
                L"          <StaticResource x:Key='tr_brush_ref' ResourceKey='SystemControlBackgroundAltHighBrush'/>"
                L"        </ColorPaletteResources>"
                L"      </ResourceDictionary.%s>"
                L"%s"
                L"    </ResourceDictionary>",
                ToString(dictType),
                color,
                ToString(dictType),
                static_cast<const wchar_t*>(style));
        }

        template <typename T>
        static void OverrideLookup_Scenario(String& xaml, std::function<void(T^)> verify)
        {
            Page^ root = nullptr;
            T^ obj = nullptr;

            RunOnUIThread([&]()
            {
                root = safe_cast<Page^>(xaml_markup::XamlReader::Load(Platform::StringReference(xaml)));
                obj = safe_cast<T^>(root->FindName(L"obj"));
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                verify(obj);
            });
        }

        static void OverrideLookup_StyleOnPage(DictionaryType dictType, const wchar_t* style)
        {
            Log::Comment(String().Format(
                L"* OverrideLookup_StyleOnPage, dictType = %s, style = %s",
                ToString(dictType),
                style));

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>%s</Page.Resources>"
                    L"  <Grid>"
                    L"    <Button x:Name='obj' Style='{StaticResource %s}'/>"
                    L"  </Grid>"
                    L"</Page>",
                    static_cast<const wchar_t*>(EmitStylesDictionary(dictType, L"page", L"Red")),
                    style),
                [](Button^ obj)
                {
                    VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                });
        }

        static void OverrideLookup_StyleOnPage_Nested(DictionaryType dictType, const wchar_t* style)
        {
            Log::Comment(String().Format(
                L"* OverrideLookup_StyleOnPage_Nested, dictType = %s, style = %s",
                ToString(dictType),
                style));

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>%s</Page.Resources>"
                    L"  <Grid>"
                    L"    <Grid.Resources>%s</Grid.Resources>"
                    L"    <Button x:Name='obj' Style='{StaticResource %s}'/>"
                    L"  </Grid>"
                    L"</Page>",
                    static_cast<const wchar_t*>(EmitStylesDictionary(dictType, L"page", L"Red")),
                    static_cast<const wchar_t*>(EmitStylesDictionary(dictType, L"fe", L"Yellow")),
                    style),
                [](Button^ obj)
                {
                    VERIFY_ARE_EQUAL(Colors::Yellow, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                });
        }

        static void OverrideLookup_ImplicitStyleOnPage(DictionaryType dictType)
        {
            Log::Comment(String().Format(
                L"* OverrideLookup_ImplicitStyleOnPage, dictType = %s",
                ToString(dictType)));

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>%s</Page.Resources>"
                    L"  <Grid>"
                    L"    <Button x:Name='obj'/>"
                    L"  </Grid>"
                    L"</Page>",
                    static_cast<const wchar_t*>(EmitStylesDictionary(dictType, nullptr, L"Red", true))),
                [](Button^ obj)
                {
                    VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                });
        }

        static void OverrideLookup_ImplicitStyleOnPage_Nested(DictionaryType dictType)
        {
            Log::Comment(String().Format(
                L"* OverrideLookup_ImplicitStyleOnPage_Nested, dictType = %s",
                ToString(dictType)));

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>%s</Page.Resources>"
                    L"  <Grid>"
                    L"    <Grid.Resources>%s</Grid.Resources>"
                    L"    <Button x:Name='obj'/>"
                    L"  </Grid>"
                    L"</Page>",
                    static_cast<const wchar_t*>(EmitStylesDictionary(dictType, nullptr, L"Red", true)),
                    static_cast<const wchar_t*>(EmitStylesDictionary(dictType, nullptr, L"Yellow", true))),
                [](Button^ obj)
                {
                    VERIFY_ARE_EQUAL(Colors::Yellow, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                });
        }

        static void OverrideLookupOnPage_Basic(const ::Windows::UI::Color& expected)
        {
            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ColorPaletteResources BaseLow='Red'/>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Button x:Name='obj' Background='{ThemeResource SystemControlBackgroundBaseLowBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [&expected](Button^ obj)
                {
                    VERIFY_ARE_EQUAL(expected, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                });
        }

        void ScopedResourcesTests::OverrideLookupOnPage()
        {
            TestCleanupWrapper cleanup;

            OverrideLookupOnPage_Basic(Colors::Red);

            Log::Comment(L"Property set");

            OverrideLookup_PropertySet(ResourceRefType::StaticResourceRef);
            OverrideLookup_PropertySet(ResourceRefType::ThemeResourceRef);

            Log::Comment(L"Implicit style");

            OverrideLookup_ImplicitStyleOnPage(DictionaryType::MergedDictionary);
            OverrideLookup_ImplicitStyleOnPage(DictionaryType::ThemeDictionary);
            OverrideLookup_ImplicitStyleOnPage_Nested(DictionaryType::MergedDictionary);
            OverrideLookup_ImplicitStyleOnPage_Nested(DictionaryType::ThemeDictionary);

            Log::Comment(L"Style reference");

            OverrideLookup_StyleOnPage(DictionaryType::MergedDictionary, L"style_page_direct");
            OverrideLookup_StyleOnPage(DictionaryType::ThemeDictionary, L"style_page_direct");
            OverrideLookup_StyleOnPage(DictionaryType::ThemeDictionary, L"style_page_indirect_tr");

            OverrideLookup_StyleOnPage_Nested(DictionaryType::MergedDictionary, L"style_page_direct");
            OverrideLookup_StyleOnPage_Nested(DictionaryType::ThemeDictionary, L"style_page_direct");
            OverrideLookup_StyleOnPage_Nested(DictionaryType::ThemeDictionary, L"style_page_indirect_tr");
            OverrideLookup_StyleOnPage_Nested(DictionaryType::MergedDictionary, L"style_fe_direct");
            OverrideLookup_StyleOnPage_Nested(DictionaryType::MergedDictionary, L"style_fe_indirect_tr");
            OverrideLookup_StyleOnPage_Nested(DictionaryType::ThemeDictionary, L"style_fe_direct");
            OverrideLookup_StyleOnPage_Nested(DictionaryType::ThemeDictionary, L"style_fe_indirect_tr");
        }

        static String EmitThemeDictionaries()
        {
            return String().Format(
                L"    <ResourceDictionary>"
                L"      <ResourceDictionary.ThemeDictionaries>"
                L"        <ColorPaletteResources x:Key='Light' BaseLow='Red'/>"
                L"        <ColorPaletteResources x:Key='Dark' BaseLow='Green'/>"
                L"      </ResourceDictionary.ThemeDictionaries>"
                L"    </ResourceDictionary>");
        }

        template <typename T>
        static void ThemeChange_Scenario(String& xaml, std::function<void(T^)> lightVerify, std::function<void(T^)> darkVerify)
        {
            Page^ root = nullptr;
            T^ obj = nullptr;

            RunOnUIThread([&]()
            {
                root = safe_cast<Page^>(xaml_markup::XamlReader::Load(Platform::StringReference(xaml)));
                obj = safe_cast<T^>(root->FindName(L"obj"));
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Dark theme validation.");
                darkVerify(obj);
                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Light;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Light theme validation.");
                lightVerify(obj);
                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Dark;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Dark theme validation, again.");
                darkVerify(obj);
            });
        }

        void ScopedResourcesTests::ThemeChange()
        {
            TestCleanupWrapper cleanup;

            ThemeChange_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"%s"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Button x:Name='obj' Background='{ThemeResource SystemControlBackgroundBaseLowBrush}'/>"
                    L"  </Grid>"
                    L"</Page>",
                    static_cast<const wchar_t*>(EmitThemeDictionaries())),
                [](Button^ obj)
                {
                    VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                },
                [](Button^ obj)
                {
                    VERIFY_ARE_EQUAL(Colors::Green, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                });
        }

        void ScopedResourcesTests::PickupOverrideFromAppXAML()
        {
            TestCleanupWrapper cleanup;

            // This leak is unrelated to what's being tested here.  In this test scenario Application object
            // is not destroyed, but we set BaseUri on it, which causes a leak.
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            RunOnUIThread([&]()
            {
                Application::LoadComponent(
                    Application::Current,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/ScopedResources/App.xaml"),
                    Primitives::ComponentResourceLocation::Application);
            });

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid>"
                    L"    <Button x:Name='obj'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Button^ obj)
                {
                    VERIFY_ARE_EQUAL(Colors::Green, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                    VERIFY_ARE_EQUAL(Colors::Yellow, safe_cast<SolidColorBrush^>(obj->Foreground)->Color);
                });
        }

        template <typename T>
        static void HCThemeChange_Scenario(String& xaml, std::function<void(T^)> HCVerify, std::function<void(T^)> nonHCVerify)
        {
            Page^ root = nullptr;
            T^ obj = nullptr;

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Test;
                root = safe_cast<Page^>(xaml_markup::XamlReader::Load(Platform::StringReference(xaml)));
                obj = safe_cast<T^>(root->FindName(L"obj"));
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"HC theme validation.");
                HCVerify(obj);
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::None;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Non-HC theme validation.");
                nonHCVerify(obj);
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Test;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"HC theme validation, again.");
                HCVerify(obj);
            });
        }

        void ScopedResourcesTests::HighContrast()
        {
            TestCleanupWrapper cleanup;

            // Assumes SystemControlRevealFocusVisualBrush.Color == SystemAccentColor in all themes

            Log::Comment(L"Ignore non-HC theme resource dictionary overrides");

            HCThemeChange_Scenario<Microsoft::UI::Xaml::Shapes::Rectangle>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ColorPaletteResources Accent='#12324354'/>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Rectangle x:Name='obj' Fill='{ThemeResource SystemControlRevealFocusVisualBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- HC
                    VERIFY_ARE_NOT_EQUAL(
                        Microsoft::UI::ColorHelper::FromArgb(0x12, 0x32, 0x43, 0x54),
                        safe_cast<SolidColorBrush^>(obj->Fill)->Color);
                },
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- Non-HC
                    VERIFY_ARE_EQUAL(
                        Microsoft::UI::ColorHelper::FromArgb(0x12, 0x32, 0x43, 0x54),
                        safe_cast<SolidColorBrush^>(obj->Fill)->Color);
                });

            Log::Comment(L"Pick up HC theme resource dictionary overrides");

            HCThemeChange_Scenario<Microsoft::UI::Xaml::Shapes::Rectangle>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ColorPaletteResources x:Key='HighContrast' Accent='#12324354'/>"
                    L"        <ColorPaletteResources x:Key='Default'/>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Rectangle x:Name='obj' Fill='{ThemeResource SystemControlRevealFocusVisualBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- HC
                    VERIFY_ARE_EQUAL(
                        Microsoft::UI::ColorHelper::FromArgb(0x12, 0x32, 0x43, 0x54),
                        safe_cast<SolidColorBrush^>(obj->Fill)->Color);
                },
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- Non-HC
                    VERIFY_ARE_NOT_EQUAL(
                       Microsoft::UI::ColorHelper::FromArgb(0x12, 0x32, 0x43, 0x54),
                       safe_cast<SolidColorBrush^>(obj->Fill)->Color);
                });
        }

        void ScopedResourcesTests::NoopForResourceDictionary()
        {
            TestCleanupWrapper cleanup;

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <Color x:Key='SystemBaseLowColor'>Green</Color>"
                    L"    </ResourceDictionary>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Button x:Name='obj'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Button^ obj)
                {
                    VERIFY_ARE_NOT_EQUAL(Colors::Green, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                });
        }

        void ScopedResourcesTests::RedefinedResourcesOnPageDontTriggerOverride()
        {
            TestCleanupWrapper cleanup;

            Log::Comment(L"Redefine alias as concrete brush referencing framework color -- no override");

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <SolidColorBrush x:Key='ButtonBackground' Color='{StaticResource SystemAltHighColor}'/>"
                    L"    </ResourceDictionary>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources AltHigh='Purple'/>"
                    L"    </Grid.Resources>"
                    L"    <Button x:Name='obj'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Button^ obj)
                {
                    VERIFY_ARE_NOT_EQUAL(Colors::Purple, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                });

            Log::Comment(L"Redefine framework brush as concrete brush referencing framework color -- no override");

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <SolidColorBrush x:Key='SystemControlBackgroundBaseLowBrush' Color='{StaticResource SystemAltHighColor}'/>"
                    L"    </ResourceDictionary>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources AltHigh='Purple'/>"
                    L"    </Grid.Resources>"
                    L"    <Button x:Name='obj'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Button^ obj)
                {
                    VERIFY_ARE_NOT_EQUAL(Colors::Purple, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                });
        }

        void ScopedResourcesTests::RedefinedResourcesInAppDontTriggerOverride()
        {
            TestCleanupWrapper cleanup;

            // This leak is unrelated to what's being tested here.  In this test scenario Application object
            // is not destroyed, but we set BaseUri on it, which causes a leak.
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            RunOnUIThread([&]()
            {
                Application::LoadComponent(
                    Application::Current,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/ScopedResources/RedefinedResourcesInAppDontTriggerOverride.xaml"),
                    Primitives::ComponentResourceLocation::Application);
            });

            Log::Comment(L"Redefine alias as concrete brush referencing framework color -- no override");

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources AltHigh='Purple'/>"
                    L"    </Grid.Resources>"
                    L"    <Button x:Name='obj'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Button^ obj)
                {
                    VERIFY_ARE_NOT_EQUAL(Colors::Purple, safe_cast<SolidColorBrush^>(obj->Background)->Color);
                });

            Log::Comment(L"Redefine framework brush as concrete brush referencing framework color -- no override");

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources AltHigh='Purple'/>"
                    L"    </Grid.Resources>"
                    L"    <Button x:Name='obj'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Button^ obj)
                {
                    VERIFY_ARE_NOT_EQUAL(Colors::Purple, safe_cast<SolidColorBrush^>(obj->Foreground)->Color);
                });

            Log::Comment(L"Redefine alias to framework brush -- override");

            OverrideLookup_Scenario<Button>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources BaseLow='Purple'/>"
                    L"    </Grid.Resources>"
                    L"    <Button x:Name='obj'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Button^ obj)
                {
                    VERIFY_ARE_EQUAL(Colors::Purple, safe_cast<SolidColorBrush^>(obj->BorderBrush)->Color);
                });
        }

        // _MUXC_REMOVAL
        /*
        void ScopedResourcesTests::CanCloneSupportedTypes()
        {
            TestCleanupWrapper cleanup;

            Log::Comment(L"Clone AcrylicBrush");

            OverrideLookup_Scenario<Microsoft::UI::Xaml::Shapes::Rectangle>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources Accent='Purple'/>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='obj' Fill='{StaticResource SystemControlAccentAcrylicElementAccentMediumHighBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml
                    AcrylicBrush^ brush = safe_cast<AcrylicBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(Colors::Purple, brush->TintColor);
                    VERIFY_ARE_EQUAL(static_cast<double>(0.7f), brush->TintOpacity);
                    VERIFY_ARE_EQUAL(Colors::Purple, brush->FallbackColor);
                });

            Log::Comment(L"Clone RevealBorderBrush");

            OverrideLookup_Scenario<Microsoft::UI::Xaml::Shapes::Rectangle>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources Accent='Purple'/>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='obj' Fill='{StaticResource SystemControlBackgroundAccentRevealBorderBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml
                    RevealBorderBrush^ brush = safe_cast<RevealBorderBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(ApplicationTheme::Dark, brush->TargetTheme);
                    VERIFY_ARE_EQUAL(Colors::Purple, brush->Color);
                    VERIFY_ARE_EQUAL(Colors::Purple, brush->FallbackColor);
                });

            Log::Comment(L"Clone RevealBackgroundBrush");

            OverrideLookup_Scenario<Microsoft::UI::Xaml::Shapes::Rectangle>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources Accent='Purple' BaseMediumLow='Red'/>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='obj' Fill='{StaticResource SystemControlHighlightBaseMediumLowRevealAccentBackgroundBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml
                    RevealBackgroundBrush^ brush = safe_cast<RevealBackgroundBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(ApplicationTheme::Dark, brush->TargetTheme);
                    VERIFY_ARE_EQUAL(Colors::Purple, brush->Color);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->FallbackColor);
                });

            Log::Comment(L"Clone SolidColorBrush");

            OverrideLookup_Scenario<Microsoft::UI::Xaml::Shapes::Rectangle>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources Accent='Purple'/>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='obj' Fill='{StaticResource SystemControlHighlightListAccentHighBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml
                    SolidColorBrush^ brush = safe_cast<SolidColorBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(Colors::Purple, brush->Color);
                    VERIFY_ARE_EQUAL(static_cast<double>(0.9f), brush->Opacity);
                });
        }
        

        void ScopedResourcesTests::DestinationDictionaryTests()
        {
            TestCleanupWrapper cleanup;

            // SystemControlHighlightBaseMediumLowRevealAccentBackgroundBrush is special.
            // It has theme resource reference on Color property (SystemAccentColor)
            // and static resource reference on FallbackColor (SystemBaseMediumLowColor).

            Log::Comment(L"Overrides in different theme dictionaries, static resource reference in closer scope");

            ThemeChange_Scenario<Microsoft::UI::Xaml::Shapes::Rectangle>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ColorPaletteResources x:Key='Light' Accent='Purple'/>"
                    L"        <ColorPaletteResources x:Key='Dark' Accent='Green'/>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ColorPaletteResources x:Key='Light' BaseMediumLow='Red'/>"
                    L"          <ColorPaletteResources x:Key='Dark' BaseMediumLow='Orange'/>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='obj' Fill='{ThemeResource SystemControlHighlightBaseMediumLowRevealAccentBackgroundBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- Light
                    RevealBackgroundBrush^ brush = safe_cast<RevealBackgroundBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(Colors::Purple, brush->Color);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->FallbackColor);
                },
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- Dark
                    RevealBackgroundBrush^ brush = safe_cast<RevealBackgroundBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(Colors::Green, brush->Color);
                    VERIFY_ARE_EQUAL(Colors::Orange, brush->FallbackColor);
                });

            Log::Comment(L"Override in resource dictionary and theme dictionary, static resource reference in closer scope");

            ThemeChange_Scenario<Microsoft::UI::Xaml::Shapes::Rectangle>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ColorPaletteResources x:Key='Light' Accent='Purple'/>"
                    L"        <ColorPaletteResources x:Key='Dark' Accent='Green'/>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources BaseMediumLow='Red'/>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='obj' Fill='{ThemeResource SystemControlHighlightBaseMediumLowRevealAccentBackgroundBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- Light
                    RevealBackgroundBrush^ brush = safe_cast<RevealBackgroundBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(Colors::Purple, brush->Color);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->FallbackColor);
                },
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- Dark
                    RevealBackgroundBrush^ brush = safe_cast<RevealBackgroundBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(Colors::Green, brush->Color);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->FallbackColor);
                });

            Log::Comment(L"Overrides in different theme dictionaries, theme resource reference in closer scope");

            ThemeChange_Scenario<Microsoft::UI::Xaml::Shapes::Rectangle>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ColorPaletteResources x:Key='Light' BaseMediumLow='Red'/>"
                    L"        <ColorPaletteResources x:Key='Dark' BaseMediumLow='Orange'/>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ColorPaletteResources x:Key='Light' Accent='Purple'/>"
                    L"          <ColorPaletteResources x:Key='Dark' Accent='Green'/>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='obj' Fill='{ThemeResource SystemControlHighlightBaseMediumLowRevealAccentBackgroundBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- Light
                    RevealBackgroundBrush^ brush = safe_cast<RevealBackgroundBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(Colors::Purple, brush->Color);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->FallbackColor);
                },
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- Dark
                    RevealBackgroundBrush^ brush = safe_cast<RevealBackgroundBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(Colors::Green, brush->Color);
                    VERIFY_ARE_EQUAL(Colors::Orange, brush->FallbackColor);
                });

            Log::Comment(L"Override in resource dictionary and theme dictionary, theme resource reference in closer scope");

            ThemeChange_Scenario<Microsoft::UI::Xaml::Shapes::Rectangle>(
                String().Format(
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ColorPaletteResources x:Key='Light' BaseMediumLow='Red'/>"
                    L"        <ColorPaletteResources x:Key='Dark' BaseMediumLow='Orange'/>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Grid.Resources>"
                    L"      <ColorPaletteResources Accent='Green'/>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='obj' Fill='{ThemeResource SystemControlHighlightBaseMediumLowRevealAccentBackgroundBrush}'/>"
                    L"  </Grid>"
                    L"</Page>"),
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- Light
                    RevealBackgroundBrush^ brush = safe_cast<RevealBackgroundBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(Colors::Green, brush->Color);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->FallbackColor);
                },
                [](Microsoft::UI::Xaml::Shapes::Rectangle^ obj)
                {
                    // Validate against generic.xaml -- Dark
                    RevealBackgroundBrush^ brush = safe_cast<RevealBackgroundBrush^>(obj->Fill);
                    VERIFY_ARE_EQUAL(Colors::Green, brush->Color);
                    VERIFY_ARE_EQUAL(Colors::Orange, brush->FallbackColor);
                });
        }
        */
    }
} } } }