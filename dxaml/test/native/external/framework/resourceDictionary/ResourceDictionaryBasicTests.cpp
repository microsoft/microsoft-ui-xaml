// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <FileLoader.h>
#include <Utils.h>
#include "ResourceDictionaryBasicTests.h"
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <TreeHelper.h>
#include <SafeEventRegistration.h>
#include <ThemeHelper.h>
#include <CustomTypeMetadataProvider.h>

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Interop;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace std;

using namespace ::Windows::UI;

namespace
{
    bool IsInWPFHostingMode()
    {
        WEX::Common::String hostingMode;
        WEX::TestExecution::RuntimeParameters::TryGetValue(L"HostingMode", hostingMode);
        return hostingMode.CompareNoCase(L"WPF") == 0;
    }

    void VerifyXamlResourceReferenceTraceHelper(Platform::String^ xamlString, Platform::String^ expectedMessage, Platform::String^ expectedMessage2, bool exceptionExpected = true)
    {
        xaml::DebugSettings^ debugSettings;
        bool origIsXamlResourceReferenceTracingEnabled = FALSE;
        auto xamlResourceReferenceFailedRegistration = CreateSafeEventRegistration(DebugSettings, XamlResourceReferenceFailed);
        Platform::String^ actualMessage;

        TestServices::Utilities->SetForceDebugSettingsTracingEvents(TRUE);

        TestCleanupWrapper cleanup([&]()
        {
            if (debugSettings != nullptr)
            {
                RunOnUIThread([&]
                {
                    debugSettings->IsXamlResourceReferenceTracingEnabled = origIsXamlResourceReferenceTracingEnabled;
                    TestServices::Utilities->SetForceDebugSettingsTracingEvents(FALSE);
                });
            }
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        RunOnUIThread([&]
        {
            debugSettings = Application::Current->DebugSettings;
            origIsXamlResourceReferenceTracingEnabled = debugSettings->IsXamlResourceReferenceTracingEnabled;
            debugSettings->IsXamlResourceReferenceTracingEnabled = TRUE;

            xamlResourceReferenceFailedRegistration.Attach(
                debugSettings,
                ref new TypedEventHandler<xaml::DebugSettings^, xaml::XamlResourceReferenceFailedEventArgs^>(
                    [&actualMessage](Platform::Object^ sender, xaml::XamlResourceReferenceFailedEventArgs^ eventArgs)
                    {
                        LOG_OUTPUT(L"XamlResourceReferenceFailed Event Fired");
                        actualMessage = eventArgs->Message;
                    }
                )
            );

            if (exceptionExpected)
            {
                VERIFY_THROWS_WINRT(XamlReader::Load(xamlString), Platform::COMException^,
                L"Resource lookup failures always throw");
            }
            else
            {
                XamlReader::Load(xamlString);
            }

            VERIFY_IS_TRUE(expectedMessage == actualMessage || expectedMessage2 == actualMessage);
        });
    }
}

namespace Microsoft { namespace UI { namespace Xaml {
    namespace {

        Grid^ LoadDefaultRoot()
        {
            Grid^ result = nullptr;
            RunOnUIThread([&result]()
            {
                result = safe_cast<Grid^>(XamlReader::Load(
                    L"<Grid  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                    L"         xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                    L"</Grid>"
                    ));

                TestServices::WindowHelper->WindowContent = result;
            });
            TestServices::WindowHelper->WaitForIdle();
            return result;
        }

        Platform::String^ GetFilePath()
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\framework\\resourcedictionary\\");
        }

        bool IsSameColor(Color expected, Color actual)
        {
            return (expected.R == actual.R
                && expected.G == actual.G
                && expected.B == actual.B
                && expected.A == actual.A);
        }
    }

    namespace Tests { namespace Framework {

        bool ResourceDictionaryBasicTests::ClassSetup()
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

        bool ResourceDictionaryBasicTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
            return true;
        }

        bool ResourceDictionaryBasicTests::TestCleanup()
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

        void ResourceDictionaryBasicTests::VerifyResourceDictionarySize()
        {
            TestCleanupWrapper cleanup;
            auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"RDzeroItem.xaml"));
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rootPanel->Resources->Size, (unsigned int) 0);
            });

            rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"RDoneItem.xaml"));
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rootPanel->Resources->Size, (unsigned int) 1);
            });

            rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"RDfourItems.xaml"));
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rootPanel->Resources->Size, (unsigned int) 4);
            });
        }

        void ResourceDictionaryBasicTests::CanAddItemToResourceDictionary()
        {
            TestCleanupWrapper cleanup;
            auto rootPanel = LoadDefaultRoot();

            RunOnUIThread([rootPanel]()
            {
                //auto rootPanel = ref new xaml_controls::Grid();
                auto rdSize = rootPanel->Resources->Size;
                auto button = ref new xaml_controls::Button();

                // Add custom RD item via code
                auto style = ref new xaml::Style(button->GetType());
                style->Setters->Append(ref new xaml::Setter(xaml_controls::Button::BackgroundProperty,
                    ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red)));
                style->Setters->Append(ref new xaml::Setter(xaml_controls::Button::ContentProperty, L"RD item added via code"));

                rootPanel->Resources->Insert(L"testStyle", style);

                // Verify item is added correctly to the RD
                VERIFY_ARE_EQUAL(rootPanel->Resources->Size, (rdSize + 1));

                style = safe_cast<Style^> (rootPanel->Resources->Lookup(L"testStyle"));
                VERIFY_IS_NOT_NULL(style);

                // Add new button and refer a newly added style
                button->Name = L"testButton";
                button->Style = style;
                rootPanel->Children->Append(button);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([rootPanel]()
            {
                auto button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"testButton"));
                auto buttonContent = safe_cast<String^>(button->Content);

                VERIFY_IS_TRUE(buttonContent->Equals(L"RD item added via code"));
            });
        }

        void ResourceDictionaryBasicTests::CanRemoveItemFromResourceDictionary()
        {
            TestCleanupWrapper cleanup;
            auto rootPanel = LoadDefaultRoot();

            RunOnUIThread([rootPanel]()
            {
                auto button = ref new xaml_controls::Button();
                auto style = ref new xaml::Style(button->GetType());
                style->Setters->Append(ref new xaml::Setter(xaml_controls::Button::BackgroundProperty,
                    ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red)));
                style->Setters->Append(ref new xaml::Setter(xaml_controls::Button::ContentProperty, L"RD item added via code"));

                rootPanel->Resources->Insert(L"testStyle", style);

                rootPanel->Resources->Remove(L"testStyle");
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void ResourceDictionaryBasicTests::CanAccessMultiLevelResources()
        {
            TestCleanupWrapper cleanup;
            auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"RDMultiLevel.xaml"));

            RunOnUIThread([rootPanel]()
            {
                auto itemPanel = safe_cast<StackPanel^>(rootPanel->FindName(L"mainPageItemPanel"));
                VERIFY_IS_NOT_NULL(itemPanel);

                // Multi-level resource
                auto style = safe_cast<Style^> (itemPanel->Resources->Lookup(L"MultiLevelResource"));
                VERIFY_IS_NOT_NULL(style);

                auto button = ref new xaml_controls::Button();
                button->Style = style;
                itemPanel->Children->Append(button);

                auto buttonContent = safe_cast<String^>(button->Content);
                VERIFY_IS_TRUE(buttonContent->Equals(L"MultiLevelResource"));

                auto brush = safe_cast<SolidColorBrush^>(button->Foreground);
                VERIFY_IS_TRUE(IsSameColor(brush->Color, Microsoft::UI::Colors::Green));
            });
        }

        void ResourceDictionaryBasicTests::CanGetStyleResourcesByName()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel.Resources> "
                    L"  <Style x:Name='testStyle' TargetType='Button'> "
                    L"      <Setter Property='Background' Value='Orange' /> "
                    L"  </Style> "
                    L"</StackPanel.Resources> "
                    L"  <Button x:Name='testButton' Content='Button' /> "
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto style = safe_cast<Style^>(rootPanel->Resources->Lookup(L"testStyle"));
                auto button = safe_cast<Button^>(rootPanel->FindName(L"testButton"));
                button->Style = style;

                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_IS_TRUE(IsSameColor(brush->Color, Microsoft::UI::Colors::Orange));
            });
        }

        void ResourceDictionaryBasicTests::CanOverrideResourceDictionaryKey()
        {
            TestCleanupWrapper cleanup;
            auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"RDkeyOverride.xaml"));

            RunOnUIThread([rootPanel]()
            {
                auto itemPanel = safe_cast<StackPanel^>(rootPanel->FindName(L"mainPageItemPanel"));
                VERIFY_IS_NOT_NULL(itemPanel);

                auto resValue1 = safe_cast<String^>(itemPanel->Resources->Lookup(L"AppRes01"));
                VERIFY_IS_TRUE(resValue1->Equals(L"AppRes01 Redef MainPageGrid StPanel"), L"Verifying original RD key");

                auto testPanel = ref new xaml_controls::StackPanel();
                testPanel->Resources->Insert(L"AppRes01", L"AppRes01 Redef test stackpanel");
                itemPanel->Children->Append(testPanel);

                resValue1 = safe_cast<String^>(testPanel->Resources->Lookup(L"AppRes01"));
                VERIFY_IS_TRUE(resValue1->Equals(L"AppRes01 Redef test stackpanel"), L"Verifying overridden RD key");

                resValue1 = safe_cast<String^>(rootPanel->Resources->Lookup("AppRes01"));
                VERIFY_IS_TRUE(resValue1->Equals(L"AppRes01 Redef MainPage"), L"Verifying overridden key in child panel");
            });
        }

        void ResourceDictionaryBasicTests::VerifyImplicitKeyResources()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel.Resources> "
                    L"  <Style TargetType='Button'> "
                    L"      <Setter Property='Background' Value='Yellow' /> "
                    L"  </Style> "
                    L"</StackPanel.Resources> "
                    L"  <Button x:Name='testButton' Content='Button' /> "
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(rootPanel->FindName(L"testButton"));

                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_IS_TRUE(IsSameColor(brush->Color, Microsoft::UI::Colors::Yellow));
            });
        }

        void ResourceDictionaryBasicTests::StringsStartingWith0()
        {
            RunOnUIThread([&]()
            {
                auto rd = ref new xaml::ResourceDictionary();

                VERIFY_IS_FALSE(rd->HasKey(L"0a"));
                VERIFY_IS_FALSE(rd->HasKey(L"0b"));

                rd->Insert(L"0a", 1);
                rd->Insert(L"0b", 1);

                VERIFY_IS_TRUE(rd->HasKey(L"0a"));
                VERIFY_IS_TRUE(rd->HasKey(L"0b"));
            });
        }

        void ResourceDictionaryBasicTests::ThemeDictionariesDeclaredAfterContent()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            xaml_controls::StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <Color x:Key='MyColor'>#FF0000FF</Color>"
                    L"      <Color x:Key='MyColor2'>#FF00FFFF</Color>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <SolidColorBrush x:Key='MyBrush' Color='{StaticResource MyColor}' />"
                    L"          <SolidColorBrush x:Key='MyBrush2' Color='{StaticResource MyColor2}' />"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='{ThemeResource MyBrush}'/> "
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;

                RunOnUIThread([&]()
                {
                    auto rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                    auto brush = safe_cast<SolidColorBrush^>(rectangle->Fill);
                    VERIFY_IS_TRUE(IsSameColor(brush->Color, Microsoft::UI::Colors::Blue));
                });
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ResourceDictionaryBasicTests::VerifyBindingAsAResource()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto rootDictionary = safe_cast<xaml::ResourceDictionary^> (xaml_markup::XamlReader::Load(
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Binding x:Key='TitleBinding' Path='Name' />"
                    L"  <TextBlock x:Key='SampleTextBlock' Text='{StaticResource TitleBinding}' />"
                    L"</ResourceDictionary>"));

                auto binding = safe_cast<Binding^>(rootDictionary->Lookup(L"TitleBinding"));

                VERIFY_IS_NOT_NULL(binding);
            });
        }

        void ResourceDictionaryBasicTests::VerifyStaticResourceAliasing()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto rootDictionary = safe_cast<xaml::ResourceDictionary^> (xaml_markup::XamlReader::Load(
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <SolidColorBrush x:Key='BlueBrush' Color='Blue' />"
                    L"  <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                    L"  <StaticResource x:Key='RedBrushAlias' ResourceKey='RedBrush' />"
                    L"</ResourceDictionary>"));


                auto redBrush = safe_cast<SolidColorBrush^>(rootDictionary->Lookup(L"RedBrush"));
                VERIFY_IS_NOT_NULL(redBrush);

                auto redBrushAlias = safe_cast<SolidColorBrush^>(rootDictionary->Lookup(L"RedBrushAlias"));
                VERIFY_IS_NOT_NULL(redBrushAlias);

                VERIFY_IS_TRUE(redBrush == redBrushAlias);
                VERIFY_IS_TRUE(IsSameColor(redBrush->Color, Microsoft::UI::Colors::Red));
                VERIFY_IS_TRUE(IsSameColor(redBrushAlias->Color, Microsoft::UI::Colors::Red));
            });
        }

        void ResourceDictionaryBasicTests::UndeferredThemeDictionaryDeclaredAfterContent()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            xaml_controls::StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <Color x:Key='MyColor'>#FF0000FF</Color>"
                    L"      <Color x:Key='MyColor2'>#FF00FFFF</Color>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <SolidColorBrush x:Key='MyBrush' Color='{StaticResource MyColor}' />"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='{ThemeResource MyBrush}'/> "
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;

                RunOnUIThread([&]()
                {
                    auto rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                    auto brush = safe_cast<SolidColorBrush^>(rectangle->Fill);
                    VERIFY_IS_TRUE(IsSameColor(brush->Color, Microsoft::UI::Colors::Blue));
                });
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ResourceDictionaryBasicTests::VerifyResourceDictionarySizeBeforeAndAfterLookup()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto rootDictionary = safe_cast<xaml::ResourceDictionary^> (xaml_markup::XamlReader::Load(
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Binding x:Key='TitleBinding' Path='Name' />"
                    L"  <TextBlock x:Key='SampleTextBlock' Text='{StaticResource TitleBinding}' />"
                    L"</ResourceDictionary>"));

                auto sizeBeforeLookup = rootDictionary->Size;
                auto binding = safe_cast<Binding^>(rootDictionary->Lookup(L"TitleBinding"));
                auto sizeAfterLookup = rootDictionary->Size;

                VERIFY_IS_NOT_NULL(binding);
                VERIFY_ARE_EQUAL(sizeBeforeLookup, sizeAfterLookup);
            });
        }

        void ResourceDictionaryBasicTests::VerifyThemeDictionaryLookup()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            xaml_controls::StackPanel^ rootPanel;

            LOG_OUTPUT(L"Verify theme dictionary lookup for Default and Light");
            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <SolidColorBrush x:Key='Brush1' Color='Red' />"
                    L"          <SolidColorBrush x:Key='Brush2' Color='Green' />"
                    L"          <SolidColorBrush x:Key='Brush3' Color='Blue' />"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='Light'>"
                    L"          <SolidColorBrush x:Key='Brush1' Color='Black' />"
                    L"          <SolidColorBrush x:Key='Brush2' Color='White' />"
                    L"          <SolidColorBrush x:Key='Brush3' Color='Cyan' />"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='{ThemeResource Brush1}'/> "
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                auto fill = safe_cast<SolidColorBrush^>(rectangle->Fill);
                VERIFY_IS_TRUE(IsSameColor(fill->Color, Microsoft::UI::Colors::Red));

                rootPanel->RequestedTheme = ElementTheme::Light;

                auto fill2 = safe_cast<SolidColorBrush^>(rectangle->Fill);
                VERIFY_IS_TRUE(IsSameColor(fill2->Color, Microsoft::UI::Colors::Black));

                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });

            LOG_OUTPUT(L"Verify theme dictionary lookup for HighContrast");
            auto highContrastDefs = ref new Platform::String(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <SolidColorBrush x:Key='Brush1' Color='Red' />"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='HighContrastWhite'>"
                    L"          <SolidColorBrush x:Key='Brush1' Color='Black' />"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='HighContrastBlack'>"
                    L"          <SolidColorBrush x:Key='Brush1' Color='White' />"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='HighContrastCustom'>"
                    L"          <SolidColorBrush x:Key='Brush1' Color='Purple' />"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='{ThemeResource Brush1}'/> "
                    L"</StackPanel>");

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(highContrastDefs));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));
                auto fill = safe_cast<SolidColorBrush^>(rectangle->Fill);
                VERIFY_IS_TRUE(IsSameColor(fill->Color, Microsoft::UI::Colors::White));
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::White;
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(highContrastDefs));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));
                auto fill = safe_cast<SolidColorBrush^>(rectangle->Fill);
                VERIFY_IS_TRUE(IsSameColor(fill->Color, Microsoft::UI::Colors::Black));
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Custom;
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(highContrastDefs));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));
                auto fill = safe_cast<SolidColorBrush^>(rectangle->Fill);
                VERIFY_IS_TRUE(IsSameColor(fill->Color, Microsoft::UI::Colors::Purple));
            });
        }

        void ResourceDictionaryBasicTests::VerifyOutOfOrderDeferredKeyReferences()
        {
            RunOnUIThread([&]() {
                auto rootDictionary = safe_cast<xaml::ResourceDictionary^> (xaml_markup::XamlReader::Load(
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                    L"  <SolidColorBrush x:Key='BlueBrush' Color='{StaticResource BlueColor}' />"
                    L"  <Color x:Key='BlueColor'>#FF000000</Color>"
                    L"</ResourceDictionary>"));

                // This causes deferral, which will force the BlueBrush to instantiate, and resolve it's StaticResource
                // to the BlueColor resource, which will typically not be instantiated until after the BlueBrush is, but
                // in this special case must be instantiated on-demand without fault.
                rootDictionary->Remove(L"RedBrush");
            });
        }

        void ResourceDictionaryBasicTests::ThemeResourceMissing()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"            RequestedTheme='Dark'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <Color x:Key='MyColor'>#FFFF0000</Color>"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='Dark'>"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"      <SolidColorBrush x:Key='MyBrush' Color='{ThemeResource MyColor}' />"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <Button Background='{StaticResource MyBrush}'/> "
                    L"</StackPanel>";

                VERIFY_THROWS_WINRT(xaml_markup::XamlReader::Load(xamlString), Platform::Exception^,
                    L"Exception should be thrown if a theme resource isn't found while parsing");
            });
        }

        void ResourceDictionaryBasicTests::ThemeResourceMissingAfterThemeChange()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"            RequestedTheme='Dark'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <Color x:Key='MyColor'>#FFFF0000</Color>"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='Dark'>"
                    L"          <Color x:Key='MyColor'>#FF0000FF</Color>"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='Light'>"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"      <SolidColorBrush x:Key='MyBrush' Color='{ThemeResource MyColor}' />"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <Button Background='{StaticResource MyBrush}'/> "
                    L"</StackPanel>";

                auto panel = safe_cast<xaml_controls::Panel^> (xaml_markup::XamlReader::Load(xamlString));
                auto button = safe_cast<Button^>(panel->Children->GetAt(0));
                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_IS_NOT_NULL(brush);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");

                VERIFY_THROWS_WINRT(panel->RequestedTheme = xaml::ElementTheme::Light, Platform::Exception^,
                    L"Exception should be thrown if a theme resource isn't found while changing a theme");
            });
        }

        void ResourceDictionaryBasicTests::StyleResourceWithXName()
        {
            TestCleanupWrapper cleanup;

            LOG_OUTPUT(L"Verifying style resource with x:Name.");
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"        <Style x:Name='myStyle' TargetType='Button'>"
                    L"            <Setter Property='Background' Value='Red' />"
                    L"        </Style>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Button Style='{StaticResource myStyle}' /> "
                    L"  </Grid>"
                    L"</Page>";

                auto page = safe_cast<xaml_controls::Page^> (xaml_markup::XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = page;

                auto panel = safe_cast<xaml_controls::Panel^>(page->Content);
                auto button = safe_cast<Button^>(panel->Children->GetAt(0));
                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_IS_NOT_NULL(brush);
                VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");
            });

            LOG_OUTPUT(L"Verifying style resource with x:ConnectionId and x:Name.");
            RunOnUIThread([&]()
            {
                auto page = ref new Page();
                Application::LoadComponent(
                    page,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/resourcedictionary/RDWithXConnectionIdAndXName.xaml"),
                    Primitives::ComponentResourceLocation::Application);
                TestServices::WindowHelper->WindowContent = page;

                auto panel = safe_cast<xaml_controls::Panel^>(page->Content);
                auto button = safe_cast<Button^>(panel->Children->GetAt(0));
                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_IS_NOT_NULL(brush);
                VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");
            });

            LOG_OUTPUT(L"Verifying style resource with x:ConnectionId, x:Name, and x:Key.");
            RunOnUIThread([&]()
            {
                auto page = ref new Page();
                Application::LoadComponent(
                    page,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/resourcedictionary/RDWithXConnectionIdAndXNameAndXKey.xaml"),
                    Primitives::ComponentResourceLocation::Application);
                TestServices::WindowHelper->WindowContent = page;

                auto panel = safe_cast<xaml_controls::Panel^>(page->Content);
                auto button = safe_cast<Button^>(panel->Children->GetAt(0));
                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_IS_NOT_NULL(brush);
                VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");
            });
        }

        void ResourceDictionaryBasicTests::ValueTypeAndEnumResources()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                String^ xamlString =
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <x:Double x:Key='Double1'>100.0</x:Double>"
                    L"    <x:Double x:Key='Double2'>50.0</x:Double>"
                    L"    <x:Int32 x:Key='Int1'>100</x:Int32>"
                    L"    <x:Int32 x:Key='Int2'>50</x:Int32>"
                    L"    <x:Boolean x:Key='Bool1'>true</x:Boolean>"
                    L"    <x:Boolean x:Key='Bool2'>false</x:Boolean>"
                    L"    <Color x:Key='Color1'>#ffff0000</Color>"
                    L"    <Color x:Key='Color2'>#ff0000ff</Color>"
                    L"    <Visibility x:Key='Visibility1'>Visible</Visibility>"
                    L"    <Visibility x:Key='Visibility2'>Collapsed</Visibility>"
                    L"</ResourceDictionary>";

                LOG_OUTPUT(L"Verifying Double resources.");
                auto resources = safe_cast<xaml::ResourceDictionary^>(XamlReader::Load(xamlString));
                auto valueDouble = safe_cast<double>(resources->Lookup(L"Double1"));
                VERIFY_ARE_EQUAL(valueDouble, 100.0);
                valueDouble = safe_cast<double>(resources->Lookup(L"Double2"));
                VERIFY_ARE_EQUAL(valueDouble, 50.0);

                LOG_OUTPUT(L"Verifying Int32 resources.");
                auto valueInt = safe_cast<int>(resources->Lookup(L"Int1"));
                VERIFY_ARE_EQUAL(valueInt, 100);
                valueInt = safe_cast<int>(resources->Lookup(L"Int2"));
                VERIFY_ARE_EQUAL(valueInt, 50);

                LOG_OUTPUT(L"Verifying Boolean resources.");
                auto valueBool = safe_cast<bool>(resources->Lookup(L"Bool1"));
                VERIFY_ARE_EQUAL(valueBool, true);
                valueBool = safe_cast<bool>(resources->Lookup(L"Bool2"));
                VERIFY_ARE_EQUAL(valueBool, false);

                LOG_OUTPUT(L"Verifying Color resources.");
                auto valueColor = safe_cast<Color>(resources->Lookup(L"Color1"));
                VERIFY_ARE_EQUAL(valueColor, Colors::Red);
                valueColor = safe_cast<Color>(resources->Lookup(L"Color2"));
                VERIFY_ARE_EQUAL(valueColor, Colors::Blue);

                LOG_OUTPUT(L"Verifying Enum resources.");
                auto valueEnum = safe_cast<Visibility>(resources->Lookup(L"Visibility1"));
                VERIFY_ARE_EQUAL(valueEnum, Visibility::Visible);
                valueEnum = safe_cast<Visibility>(resources->Lookup(L"Visibility2"));
                VERIFY_ARE_EQUAL(valueEnum, Visibility::Collapsed);
            });

            RunOnUIThread([&]()
            {
                String^ xamlString =
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <x:Double x:Key='Double1'>100.0</x:Double>"
                    L"</ResourceDictionary>";

                LOG_OUTPUT(L"Verifying single Double resource.");
                auto resources = safe_cast<xaml::ResourceDictionary^>(XamlReader::Load(xamlString));
                auto valueDouble = safe_cast<double>(resources->Lookup(L"Double1"));
                VERIFY_ARE_EQUAL(valueDouble, 100.0);
            });

            RunOnUIThread([&]()
            {
                String^ xamlString =
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <x:Int32 x:Key='Int1'>100</x:Int32>"
                    L"</ResourceDictionary>";

                LOG_OUTPUT(L"Verifying single Double resource.");
                auto resources = safe_cast<xaml::ResourceDictionary^>(XamlReader::Load(xamlString));
                auto valueInt = safe_cast<int>(resources->Lookup(L"Int1"));
                VERIFY_ARE_EQUAL(valueInt, 100);
            });

            RunOnUIThread([&]()
            {
                String^ xamlString =
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <x:Boolean x:Key='Bool1'>true</x:Boolean>"
                    L"</ResourceDictionary>";

                LOG_OUTPUT(L"Verifying single Bool resource.");
                auto resources = safe_cast<xaml::ResourceDictionary^>(XamlReader::Load(xamlString));
                auto valueBool = safe_cast<bool>(resources->Lookup(L"Bool1"));
                VERIFY_ARE_EQUAL(valueBool, true);
            });

            RunOnUIThread([&]()
            {
                String^ xamlString =
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Color x:Key='Color1'>#ffff0000</Color>"
                    L"</ResourceDictionary>";

                LOG_OUTPUT(L"Verifying single Color resource.");
                auto resources = safe_cast<xaml::ResourceDictionary^>(XamlReader::Load(xamlString));
                auto valueColor = safe_cast<Color>(resources->Lookup(L"Color1"));
                VERIFY_ARE_EQUAL(valueColor, Colors::Red);
            });

            RunOnUIThread([&]()
            {
                String^ xamlString =
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Visibility x:Key='Visibility1'>Collapsed</Visibility>"
                    L"</ResourceDictionary>";

                LOG_OUTPUT(L"Verifying single Enum resource.");
                auto resources = safe_cast<xaml::ResourceDictionary^>(XamlReader::Load(xamlString));
                auto valueEnum = safe_cast<Visibility>(resources->Lookup(L"Visibility1"));
                VERIFY_ARE_EQUAL(valueEnum, Visibility::Collapsed);
            });
        }

        void ResourceDictionaryBasicTests::ParseErrorsBubbleWhenLoadingAllDeferredResources()
        {
            RunOnUIThread([&]() {
                auto rootDictionary = safe_cast<xaml::ResourceDictionary^> (xaml_markup::XamlReader::Load(
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                    L"  <SolidColorBrush x:Key='BlueBrush' Color='{StaticResource IllNeverExist}' />"
                    L"</ResourceDictionary>"));
                VERIFY_THROWS_WINRT(rootDictionary->Remove(L"RedBrush"), Platform::Exception^);
            });
        }

        // Verify that GetView returns a IMapView which contains the expected contents.
        void ResourceDictionaryBasicTests::VerifyGetMapView()
        {
            RunOnUIThread([&]()
            {
                auto dictionary = ref new ResourceDictionary();
                dictionary->Insert(ref new Platform::String(L"KeyAlpha"), ref new Platform::String(L"ValueAlpha from Primary dictionary"));
                dictionary->Insert(ref new Platform::String(L"KeyBeta"), ref new Platform::String(L"ValueBeta from Primary dictionary"));
                dictionary->Insert(ref new Platform::String(L"KeyGamma"), ref new Platform::String(L"ValueGamma from Primary dictionary"));

                auto dictionaryView = dictionary->GetView();

                // Verify size
                VERIFY_ARE_EQUAL((unsigned int)3, dictionaryView->Size);

                // Verify that expected keys are present
                VERIFY_IS_TRUE(dictionaryView->HasKey(ref new Platform::String(L"KeyAlpha")));
                VERIFY_IS_TRUE(dictionaryView->HasKey(ref new Platform::String(L"KeyBeta")));
                VERIFY_IS_TRUE(dictionaryView->HasKey(ref new Platform::String(L"KeyGamma")));

                // Verify that expected values are present
                VERIFY_IS_TRUE(safe_cast<Platform::String^>(dictionaryView->Lookup(ref new Platform::String(L"KeyAlpha")))->Equals(ref new Platform::String(L"ValueAlpha from Primary dictionary")));
                VERIFY_IS_TRUE(safe_cast<Platform::String^>(dictionaryView->Lookup(ref new Platform::String(L"KeyBeta")))->Equals(ref new Platform::String(L"ValueBeta from Primary dictionary")));
                VERIFY_IS_TRUE(safe_cast<Platform::String^>(dictionaryView->Lookup(ref new Platform::String(L"KeyGamma")))->Equals(ref new Platform::String(L"ValueGamma from Primary dictionary")));
            });
        }

        void ResourceDictionaryBasicTests::VerifyExplicitTypeName()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <Style TargetType='Button'>"
                    L"      <Setter Property='Background' Value='Red' />"
                    L"    </Style>"
                    L"    <Style TargetType='Button' x:Key='Microsoft.UI.Xaml.Controls.Button'>"
                    L"      <Setter Property='Background' Value='Blue' />"
                    L"    </Style>"
                    L"  </StackPanel.Resources> "
                    L"  <Button x:Name='testButton1' Content='Button1' /> "
                    L"  <Button x:Name='testButton2' Content='Button2' Style='{StaticResource Microsoft.UI.Xaml.Controls.Button}' /> "
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"testButton1"));
                auto brush1 = safe_cast<xaml_media::SolidColorBrush^>(button1->Background);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, brush1->Color);

                auto button2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"testButton2"));
                auto brush2 = safe_cast<xaml_media::SolidColorBrush^>(button2->Background);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, brush2->Color);
            });
        }

        void ResourceDictionaryBasicTests::XNameResourceWithReferenceToXNameResource()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"        <Style x:Name='myStyle' TargetType='Button'>"
                    L"            <Setter Property='Background' Value='{StaticResource myBrush}' />"
                    L"        </Style>"
                    L"        <SolidColorBrush x:Name='myBrush' Color='Red' />"
                    L"  </StackPanel.Resources>"
                    L"  <Button Style='{StaticResource myStyle}' /> "
                    L"</StackPanel>";

                auto panel = safe_cast<xaml_controls::Panel^> (xaml_markup::XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = panel;
                auto button = safe_cast<Button^>(panel->Children->GetAt(0));
                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_IS_NOT_NULL(brush);
                VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");
            });
        }

        void ResourceDictionaryBasicTests::UndeferResourcesWithForwardReferences()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"        <Style x:Key='myStyle' TargetType='Button'>"
                    L"            <Setter Property='Background' Value='{StaticResource myBrush1}' />"
                    L"        </Style>"
                    L"        <SolidColorBrush x:Key='myBrush1' Color='Red' />"
                    L"        <SolidColorBrush x:Key='myBrush2' Color='Blue' />"
                    L"  </StackPanel.Resources>"
                    L"  <Button /> "
                    L"</StackPanel>";

                auto panel = safe_cast<xaml_controls::Panel^> (xaml_markup::XamlReader::Load(xamlString));
                panel->Resources->Remove(L"myBrush2"); // this step implicitly undefers all resources
                auto style = safe_cast<Style^>(panel->Resources->Lookup(L"myStyle"));
                VERIFY_IS_NOT_NULL(style);
                auto brush = safe_cast<SolidColorBrush^>(panel->Resources->Lookup(L"myBrush1"));
                VERIFY_IS_NOT_NULL(brush);
                VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");
            });
        }

        void ResourceDictionaryBasicTests::AppThemeResource()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto appResources = Application::Current->Resources;

                if (!appResources->ThemeDictionaries->HasKey("Dark"))
                    appResources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!appResources->ThemeDictionaries->HasKey("Light"))
                    appResources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(appResources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(appResources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            LOG_OUTPUT(L"Reference on descendant.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <SolidColorBrush x:Key='GreenBrush' Color='Green' />"
                        L"        <SolidColorBrush x:Key='BlackBrush' Color='Black' />"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);
                    auto brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Pink, brush->Color);
                });
            }

            LOG_OUTPUT(L"Reference from ControlTemplate.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <SolidColorBrush x:Key='GreenBrush' Color='Green' />"
                        L"        <SolidColorBrush x:Key='BlackBrush' Color='Black' />"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button>"
                        L"      <Button.Template>"
                        L"        <ControlTemplate TargetType='Button'>"
                        L"          <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                        L"        </ControlTemplate>"
                        L"      </Button.Template>"
                        L"    </Button>"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);
                    button->ApplyTemplate();
                    auto child = TreeHelper::GetVisualChildByType<Grid>(button);

                    auto brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::Pink, brush->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::AppThemeResourceImplicitStyle()
        {
            TestCleanupWrapper cleanup;
            Page^ page = nullptr;

            auto initAppResources = []()
            {
                // The MUXC theme resources clash with these changes, so we'll clear them out first.
                Application::Current->Resources->MergedDictionaries->Clear();

                auto appThemes = Application::Current->Resources->ThemeDictionaries;

                auto buttonType = ref new Platform::Box<::Windows::UI::Xaml::Interop::TypeName>(Button::typeid);
                Platform::String^ styleXaml =
                    L"<Style TargetType='Button' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    L"  <Setter Property='Background' Value='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                    L"</Style>";

                // Dark
                if (!appThemes->HasKey("Dark")) appThemes->Insert(L"Dark", ref new ResourceDictionary());
                auto darkResources = safe_cast<ResourceDictionary^>(appThemes->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));
                darkResources->Insert(buttonType, XamlReader::Load(styleXaml));

                // Light
                if (!appThemes->HasKey("Light")) appThemes->Insert(L"Light", ref new ResourceDictionary());
                auto lightResources = safe_cast<ResourceDictionary^>(appThemes->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
                lightResources->Insert(buttonType, XamlReader::Load(styleXaml));
            };

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Button />"
                    L"</Page>";

                initAppResources();

                page = safe_cast<Page^>(XamlReader::Load(xamlString));

                page->RequestedTheme = ElementTheme::Dark;

                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(page->Content);
                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Red, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme

                button = safe_cast<Button^>(page->Content);
                brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Pink, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::AppThemeResourceUpdated()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto appResources = Application::Current->Resources;

                if (!appResources->ThemeDictionaries->HasKey("Dark"))
                    appResources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!appResources->ThemeDictionaries->HasKey("Light"))
                    appResources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(appResources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Clear();
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(appResources->ThemeDictionaries->Lookup("Light"));
                lightResources->Clear();
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            auto updateAppResources = []()
            {
                auto appResources = Application::Current->Resources;

                auto darkResources = safe_cast<ResourceDictionary^>(appResources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Clear();
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Blue));

                auto lightResources = safe_cast<ResourceDictionary^>(appResources->ThemeDictionaries->Lookup("Light"));
                lightResources->Clear();
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::LightBlue));
            };

            LOG_OUTPUT(L"Reference on descendant.");
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <SolidColorBrush x:Key='GreenBrush' Color='Green' />"
                    L"        <SolidColorBrush x:Key='BlackBrush' Color='Black' />"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                    L"</Page>";

                initAppResources();

                auto page = safe_cast<Page^>(XamlReader::Load(xamlString));

                page->RequestedTheme = ElementTheme::Dark;

                auto button = safe_cast<Button^>(page->Content);

                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Red, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme

                brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Pink, brush->Color);

                // update app theme resources
                updateAppResources();

                page->RequestedTheme = ElementTheme::Dark; // change theme

                brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme

                brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
            });

            LOG_OUTPUT(L"Reference from ControlTemplate.");
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <SolidColorBrush x:Key='GreenBrush' Color='Green' />"
                    L"        <SolidColorBrush x:Key='BlackBrush' Color='Black' />"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <Button>"
                    L"      <Button.Template>"
                    L"        <ControlTemplate TargetType='Button'>"
                    L"          <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                    L"        </ControlTemplate>"
                    L"      </Button.Template>"
                    L"    </Button>"
                    L"</Page>";

                initAppResources();

                auto page = safe_cast<Page^>(XamlReader::Load(xamlString));

                page->RequestedTheme = ElementTheme::Dark;

                auto button = safe_cast<Button^>(page->Content);
                button->ApplyTemplate();
                auto child = TreeHelper::GetVisualChildByType<Grid>(button);

                auto brush = safe_cast<SolidColorBrush^>(child->Background);
                VERIFY_ARE_EQUAL(Colors::Red, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme

                brush = safe_cast<SolidColorBrush^>(child->Background);
                VERIFY_ARE_EQUAL(Colors::Pink, brush->Color);

                // update app theme resources
                updateAppResources();

                page->RequestedTheme = ElementTheme::Dark;

                brush = safe_cast<SolidColorBrush^>(child->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme

                brush = safe_cast<SolidColorBrush^>(child->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::PageThemeResource()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            LOG_OUTPUT(L"Reference on descendant.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);

                    auto brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
                });
            }

            LOG_OUTPUT(L"Reference from ControlTemplate.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button>"
                        L"      <Button.Template>"
                        L"        <ControlTemplate TargetType='Button'>"
                        L"          <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                        L"        </ControlTemplate>"
                        L"      </Button.Template>"
                        L"    </Button>"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);
                    button->ApplyTemplate();
                    auto child = TreeHelper::GetVisualChildByType<Grid>(button);

                    auto brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
                });
            }

            LOG_OUTPUT(L"Reference from Style Setter.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button>"
                        L"      <Button.Style>"
                        L"        <Style TargetType='Button'>"
                        L"          <Setter Property='Background' Value='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                        L"        </Style>"
                        L"      </Button.Style>"
                        L"    </Button>"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);

                    auto brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
                });
            }

            LOG_OUTPUT(L"Reference via static ref to non-theme Style resource.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"        <Style x:Key='style1' TargetType='Button'>"
                        L"          <Setter Property='Background' Value='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                        L"        </Style>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button Style='{StaticResource style1}' />"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);

                    auto brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
                });
            }

            LOG_OUTPUT(L"Reference via static ref to non-theme ControlTemplate resource.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"        <ControlTemplate x:Key='template1' TargetType='Button'>"
                        L"          <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                        L"        </ControlTemplate>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button Template='{StaticResource template1}' />"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);
                    button->ApplyTemplate();
                    auto child = TreeHelper::GetVisualChildByType<Grid>(button);

                    auto brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
                });
            }

            LOG_OUTPUT(L"Reference via theme ref to non-theme Style resource.");
            {
                Panel^ panel = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <StackPanel.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Default'>"
                        L"            <SolidColorBrush x:Key='MyButtonBackground' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='MyButtonBackground' Color='LightBlue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"        <Style TargetType='Button' >"
                        L"          <Setter Property='Background' Value='{ThemeResource MyButtonBackground}' />"
                        L"        </Style>"
                        L"      </ResourceDictionary>"
                        L"    </StackPanel.Resources>"
                        L"    <Button />"
                        L"    <StackPanel>"
                        L"      <StackPanel.Resources>"
                        L"        <ResourceDictionary>"
                        L"          <ResourceDictionary.ThemeDictionaries>"
                        L"            <ResourceDictionary x:Key='Default'>"
                        L"              <SolidColorBrush x:Key='MyButtonBackground' Color='Red' />"
                        L"            </ResourceDictionary>"
                        L"            <ResourceDictionary x:Key='Light'>"
                        L"              <SolidColorBrush x:Key='MyButtonBackground' Color='Pink' />"
                        L"            </ResourceDictionary>"
                        L"          </ResourceDictionary.ThemeDictionaries>"
                        L"        </ResourceDictionary>"
                        L"      </StackPanel.Resources>"
                        L"      <Button />"
                        L"    </StackPanel>"
                        L"</StackPanel>";

                    panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

                    TestServices::WindowHelper->WindowContent = panel;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto innerPanel = safe_cast<Panel^>(panel->Children->GetAt(1));
                    auto button2 = safe_cast<Button^>(innerPanel->Children->GetAt(0));
                    auto brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush2->Color);

                    auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    auto brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);

                    panel->RequestedTheme = ElementTheme::Dark;

                    button2 = safe_cast<Button^>(innerPanel->Children->GetAt(0));
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush2->Color);

                    button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);

                    panel->RequestedTheme = ElementTheme::Light;

                    button2 = safe_cast<Button^>(innerPanel->Children->GetAt(0));
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Pink, brush2->Color);

                    button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush1->Color);

                    // move the inner button to the root panel
                    button2 = safe_cast<Button^>(innerPanel->Children->GetAt(0));
                    innerPanel->Children->RemoveAt(0);
                    panel->Children->Append(button2);

                    button2 = safe_cast<Button^>(panel->Children->GetAt(2));
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush2->Color);

                    button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush1->Color);

                    panel->RequestedTheme = ElementTheme::Dark;

                    button2 = safe_cast<Button^>(panel->Children->GetAt(2));
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush2->Color);

                    button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);

                    panel->RequestedTheme = ElementTheme::Light;

                    button2 = safe_cast<Button^>(panel->Children->GetAt(2));
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush2->Color);

                    button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush1->Color);
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            LOG_OUTPUT(L"Reference on descendant with default page theme dictionary.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Default'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);

                    auto brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::PageThemeResourceViaGlobalTemplate()
        {
            TestCleanupWrapper cleanup;

            {
                Panel^ panel = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <StackPanel.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='ButtonBackground' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='ButtonBackground' Color='LightBlue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </StackPanel.Resources>"
                        L"    <Button />"
                        L"    <StackPanel>"
                        L"      <StackPanel.Resources>"
                        L"        <ResourceDictionary>"
                        L"          <ResourceDictionary.ThemeDictionaries>"
                        L"            <ResourceDictionary x:Key='Dark'>"
                        L"              <SolidColorBrush x:Key='ButtonBackground' Color='Red' />"
                        L"            </ResourceDictionary>"
                        L"            <ResourceDictionary x:Key='Light'>"
                        L"              <SolidColorBrush x:Key='ButtonBackground' Color='Pink' />"
                        L"            </ResourceDictionary>"
                        L"          </ResourceDictionary.ThemeDictionaries>"
                        L"        </ResourceDictionary>"
                        L"      </StackPanel.Resources>"
                        L"      <Button />"
                        L"    </StackPanel>"
                        L"</StackPanel>";

                    panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

                    TestServices::WindowHelper->WindowContent = panel;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto innerPanel = safe_cast<Panel^>(panel->Children->GetAt(1));
                    auto button2 = safe_cast<Button^>(innerPanel->Children->GetAt(0));
                    auto brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush2->Color);

                    auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    auto brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);

                    panel->RequestedTheme = ElementTheme::Dark;

                    button2 = safe_cast<Button^>(innerPanel->Children->GetAt(0));
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush2->Color);

                    button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);

                    panel->RequestedTheme = ElementTheme::Light;

                    button2 = safe_cast<Button^>(innerPanel->Children->GetAt(0));
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Pink, brush2->Color);

                    button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush1->Color);

                    // move the inner button to the root panel
                    button2 = safe_cast<Button^>(innerPanel->Children->GetAt(0));
                    innerPanel->Children->RemoveAt(0);
                    panel->Children->Append(button2);

                    button2 = safe_cast<Button^>(panel->Children->GetAt(2));
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush2->Color);

                    button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush1->Color);

                    panel->RequestedTheme = ElementTheme::Dark;

                    button2 = safe_cast<Button^>(panel->Children->GetAt(2));
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush2->Color);

                    button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);

                    panel->RequestedTheme = ElementTheme::Light;

                    button2 = safe_cast<Button^>(panel->Children->GetAt(2));
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush2->Color);

                    button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush1->Color);
                });
                TestServices::WindowHelper->WaitForIdle();
            }
        }

        void ResourceDictionaryBasicTests::PageThemeResourceMixedRefs()
        {
            TestCleanupWrapper cleanup;
            Page^ page = nullptr;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Light'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <StackPanel>"
                    L"      <StackPanel RequestedTheme='Dark'>"
                    L"        <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                    L"      </StackPanel>"
                    L"      <StackPanel RequestedTheme='Light'>"
                    L"        <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                    L"      </StackPanel>"
                    L"    </StackPanel>"
                    L"</Page>";

                initAppResources();

                page = safe_cast<Page^>(XamlReader::Load(xamlString));

                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto topPanel = safe_cast<StackPanel^>(page->Content);

                auto panel1 = safe_cast<StackPanel^>(topPanel->Children->GetAt(0));
                VERIFY_ARE_EQUAL(ElementTheme::Dark, panel1->RequestedTheme);
                auto panel2 = safe_cast<StackPanel^>(topPanel->Children->GetAt(1));
                VERIFY_ARE_EQUAL(ElementTheme::Light, panel2->RequestedTheme);

                auto button1 = safe_cast<Button^>(panel1->Children->GetAt(0));
                auto button2 = safe_cast<Button^>(panel2->Children->GetAt(0));

                auto brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);
                auto brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush2->Color);

                panel1->RequestedTheme = ElementTheme::Light;

                brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush1->Color);
                brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush2->Color);

                panel2->RequestedTheme = ElementTheme::Dark;

                brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush1->Color);
                brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush2->Color);

                panel1->RequestedTheme = ElementTheme::Dark;

                brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);
                brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush2->Color);
            });
        }

        void ResourceDictionaryBasicTests::PageThemeResourceFallback()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            LOG_OUTPUT(L"Reference on descendant.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);

                    auto brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Pink, brush->Color);
                });
            }

            LOG_OUTPUT(L"Reference from ControlTemplate.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button>"
                        L"      <Button.Template>"
                        L"        <ControlTemplate TargetType='Button'>"
                        L"          <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                        L"        </ControlTemplate>"
                        L"      </Button.Template>"
                        L"    </Button>"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);
                    button->ApplyTemplate();
                    auto child = TreeHelper::GetVisualChildByType<Grid>(button);

                    auto brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::Pink, brush->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::PageThemeResourceMultiplePages()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            LOG_OUTPUT(L"Reference on descendant with default page resource.");
            {
                Page^ page1 = nullptr;
                Page^ page2 = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString1 =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Default'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                        L"</Page>";

                    Platform::String^ xamlString2 =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Default'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Green' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                        L"</Page>";

                    initAppResources();

                    page1 = safe_cast<Page^>(XamlReader::Load(xamlString1));
                    page2 = safe_cast<Page^>(XamlReader::Load(xamlString2));

                    TestServices::WindowHelper->WindowContent = page1;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    page1->RequestedTheme = ElementTheme::Dark;
                    auto button1 = safe_cast<Button^>(page1->Content);
                    auto brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);
                    page1->RequestedTheme = ElementTheme::Light; // change theme
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = page2;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    page2->RequestedTheme = ElementTheme::Dark;
                    auto button2 = safe_cast<Button^>(page2->Content);
                    auto brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Green, brush2->Color);
                    page2->RequestedTheme = ElementTheme::Light; // change theme
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Green, brush2->Color);
                });
            }

            LOG_OUTPUT(L"Page resource overriding resource ref'd by global theme template.");
            {
                Page^ page1 = nullptr;
                Page^ page2 = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString1 =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='ButtonBackground' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='ButtonBackground' Color='LightBlue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button />"
                        L"</Page>";

                    initAppResources();

                    page1 = safe_cast<Page^>(XamlReader::Load(xamlString1));
                    page1->RequestedTheme = ElementTheme::Dark;
                    TestServices::WindowHelper->WindowContent = page1;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button1 = safe_cast<Button^>(page1->Content);
                    auto brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);
                    page1->RequestedTheme = ElementTheme::Light; // change theme
                    brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush1->Color);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString2 =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='ButtonBackground' Color='Green' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='ButtonBackground' Color='LightGreen' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button />"
                        L"</Page>";

                    page2 = safe_cast<Page^>(XamlReader::Load(xamlString2));
                    page2->RequestedTheme = ElementTheme::Dark;
                    TestServices::WindowHelper->WindowContent = page2;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button2 = safe_cast<Button^>(page2->Content);
                    auto brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Green, brush2->Color);
                    page2->RequestedTheme = ElementTheme::Light; // change theme
                    brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::LightGreen, brush2->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::PageThemeResourceViaThemeStyle()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            Page^ page = nullptr;
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                    L"            <Style x:Key='style1' TargetType='Button'>"
                    L"              <Setter Property='Background' Value='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                    L"            </Style>"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Light'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                    L"            <Style x:Key='style1' TargetType='Button'>"
                    L"              <Setter Property='Background' Value='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                    L"            </Style>"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <Button Style='{ThemeResource style1}' />"
                    L"</Page>";

                initAppResources();

                page = safe_cast<Page^>(XamlReader::Load(xamlString));

                page->RequestedTheme = ElementTheme::Dark;

                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(page->Content);

                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme

                brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::PageThemeResourceViaThemeTemplate()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            Page^ page = nullptr;
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                    L"            <ControlTemplate x:Key='template1' TargetType='Button'>"
                    L"              <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                    L"            </ControlTemplate>"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Light'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                    L"            <ControlTemplate x:Key='template1' TargetType='Button'>"
                    L"              <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                    L"            </ControlTemplate>"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <Button Template='{ThemeResource template1}' />"
                    L"</Page>";

                initAppResources();

                page = safe_cast<Page^>(XamlReader::Load(xamlString));

                page->RequestedTheme = ElementTheme::Dark;

                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(page->Content);
                auto child = TreeHelper::GetVisualChildByType<Grid>(button);

                auto brush = safe_cast<SolidColorBrush^>(child->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme

                button->ApplyTemplate();
                child = TreeHelper::GetVisualChildByType<Grid>(button);
                brush = safe_cast<SolidColorBrush^>(child->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::PageThemeResourceMixedRefsViaThemeTemplate()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));
                darkResources->Insert(L"SystemControlBackgroundAltHighBrush", ref new SolidColorBrush(Colors::Gray));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
                lightResources->Insert(L"SystemControlBackgroundAltHighBrush", ref new SolidColorBrush(Colors::LightGray));
            };

            Page^ page = nullptr;
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                    L"            <ControlTemplate x:Key='template1' TargetType='Button'>"
                    L"              <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                    L"            </ControlTemplate>"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Light'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAltHighBrush' Color='LightGreen' />"
                    L"            <ControlTemplate x:Key='template1' TargetType='Button'>"
                    L"              <Grid Background='{ThemeResource SystemControlBackgroundAltHighBrush}' Height='200' Width='200' />"
                    L"            </ControlTemplate>"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <Button Template='{ThemeResource template1}' />"
                    L"</Page>";

                initAppResources();

                page = safe_cast<Page^>(XamlReader::Load(xamlString));

                page->RequestedTheme = ElementTheme::Dark;

                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(page->Content);
                auto child = TreeHelper::GetVisualChildByType<Grid>(button);
                auto brush = safe_cast<SolidColorBrush^>(child->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(page->Content);
                auto child = TreeHelper::GetVisualChildByType<Grid>(button);
                auto brush = safe_cast<SolidColorBrush^>(child->Background);
                VERIFY_ARE_EQUAL(Colors::LightGreen, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::PageThemeResourceMixedRefsViaThemeStyle()
        {
            TestCleanupWrapper cleanup;

            Panel^ panel = nullptr;
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <StackPanel.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"            <SolidColorBrush x:Key='myBrush1' Color='Blue' />"
                    L"            <Style x:Key='myStyle' TargetType='Button'>"
                    L"                <Setter Property='Background' Value='{ThemeResource myBrush1}' />"
                    L"            </Style>"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Light'>"
                    L"            <SolidColorBrush x:Key='myBrush2' Color='LightBlue' />"
                    L"            <Style x:Key='myStyle' TargetType='Button'>"
                    L"                <Setter Property='Background' Value='{ThemeResource myBrush2}' />"
                    L"            </Style>"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </StackPanel.Resources>"
                    L"    <Button Style='{ThemeResource myStyle}' />"
                    L"</StackPanel>";

                panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

                panel->RequestedTheme = ElementTheme::Dark;

                TestServices::WindowHelper->WindowContent = panel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(panel->Children->GetAt(0));

                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                panel->RequestedTheme = ElementTheme::Light;

                brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::PageThemeResourceViaThemeStyleTemplate()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            Page^ page = nullptr;
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                    L"            <Style x:Key='style1' TargetType='Button'>"
                    L"              <Setter Property='Template'>"
                    L"                <Setter.Value>"
                    L"                  <ControlTemplate x:Key='template1' TargetType='Button'>"
                    L"                    <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                    L"                  </ControlTemplate>"
                    L"                </Setter.Value>"
                    L"              </Setter>"
                    L"            </Style>"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Light'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                    L"            <Style x:Key='style1' TargetType='Button'>"
                    L"              <Setter Property='Template'>"
                    L"                <Setter.Value>"
                    L"                  <ControlTemplate x:Key='template1' TargetType='Button'>"
                    L"                    <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                    L"                  </ControlTemplate>"
                    L"                </Setter.Value>"
                    L"              </Setter>"
                    L"            </Style>"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <Button Style='{ThemeResource style1}' />"
                    L"</Page>";

                initAppResources();

                page = safe_cast<Page^>(XamlReader::Load(xamlString));

                page->RequestedTheme = ElementTheme::Dark;

                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(page->Content);
                auto child = TreeHelper::GetVisualChildByType<Grid>(button);
                auto brush = safe_cast<SolidColorBrush^>(child->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(page->Content);
                auto child = TreeHelper::GetVisualChildByType<Grid>(button);
                auto brush = safe_cast<SolidColorBrush^>(child->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::PageThemeResourceViaThemeImplicitStyle()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            Page^ page = nullptr;
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                    L"            <Style TargetType='Button'>"
                    L"              <Setter Property='Background' Value='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                    L"            </Style>"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Light'>"
                    L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                    L"            <Style TargetType='Button'>"
                    L"              <Setter Property='Background' Value='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                    L"            </Style>"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <Button />"
                    L"</Page>";

                initAppResources();

                page = safe_cast<Page^>(XamlReader::Load(xamlString));

                page->RequestedTheme = ElementTheme::Dark;

                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(page->Content);
                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(page->Content);
                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::PageThemeResourceCustomResource()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"PageDefinedResourceBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"PageDefinedResourceBrush", ref new SolidColorBrush(Colors::Pink));
            };

            Page^ page = nullptr;
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"            <SolidColorBrush x:Key='PageDefinedResourceBrush' Color='Blue' />"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Light'>"
                    L"            <SolidColorBrush x:Key='PageDefinedResourceBrush' Color='LightBlue' />"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <Button Background='{ThemeResource PageDefinedResourceBrush}' />"
                    L"</Page>";

                initAppResources();

                page = safe_cast<Page^>(XamlReader::Load(xamlString));

                page->RequestedTheme = ElementTheme::Dark;

                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(page->Content);

                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                page->RequestedTheme = ElementTheme::Light; // change theme

                brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::PageThemeResourceNested()
        {
            TestCleanupWrapper cleanup;

            auto initAppResources = []()
            {
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            LOG_OUTPUT(L"Reference on descendant.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <StackPanel>"
                        L"      <StackPanel.Resources>"
                        L"        <ResourceDictionary>"
                        L"          <ResourceDictionary.ThemeDictionaries>"
                        L"            <ResourceDictionary x:Key='Dark' />"
                        L"            <ResourceDictionary x:Key='Light' />"
                        L"          </ResourceDictionary.ThemeDictionaries>"
                        L"        </ResourceDictionary>"
                        L"      </StackPanel.Resources>"
                        L"      <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                        L"    </StackPanel>"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto panel = safe_cast<Panel^>(page->Content);
                    auto button = safe_cast<Button^>(panel->Children->GetAt(0));

                    auto brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
                });
            }

            LOG_OUTPUT(L"Reference from ControlTemplate.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='Blue' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='SystemControlBackgroundAccentBrush' Color='LightBlue' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <StackPanel>"
                        L"      <StackPanel.Resources>"
                        L"        <ResourceDictionary>"
                        L"          <ResourceDictionary.ThemeDictionaries>"
                        L"            <ResourceDictionary x:Key='Dark' />"
                        L"            <ResourceDictionary x:Key='Light' />"
                        L"          </ResourceDictionary.ThemeDictionaries>"
                        L"        </ResourceDictionary>"
                        L"      </StackPanel.Resources>"
                        L"      <Button>"
                        L"        <Button.Template>"
                        L"          <ControlTemplate TargetType='Button'>"
                        L"            <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                        L"          </ControlTemplate>"
                        L"        </Button.Template>"
                        L"      </Button>"
                        L"    </StackPanel>"
                        L"</Page>";

                    initAppResources();

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    page->RequestedTheme = ElementTheme::Dark;

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto panel = safe_cast<Panel^>(page->Content);
                    auto button = safe_cast<Button^>(panel->Children->GetAt(0));
                    button->ApplyTemplate();
                    auto child = TreeHelper::GetVisualChildByType<Grid>(button);

                    auto brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::PageThemeResourceUpdated()
        {
            TestCleanupWrapper cleanup;

            auto initThemeResources = [](ResourceDictionary^ resources)
            {
                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Pink));
            };

            auto updateThemeResources = [](ResourceDictionary^ resources)
            {
                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::Blue));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"SystemControlBackgroundAccentBrush", ref new SolidColorBrush(Colors::LightBlue));
            };

            LOG_OUTPUT(L"Reference on descendant.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Button Background='{ThemeResource SystemControlBackgroundAccentBrush}' />"
                        L"</Page>";

                    initThemeResources(Application::Current->Resources);

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    page->RequestedTheme = ElementTheme::Dark;

                    auto button = safe_cast<Button^>(page->Content);

                    auto brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Pink, brush->Color);

                    // update page theme resources
                    initThemeResources(page->Resources);
                    updateThemeResources(page->Resources);

                    page->RequestedTheme = ElementTheme::Dark;

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
                });
            }

            LOG_OUTPUT(L"Reference from ControlTemplate.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Button>"
                        L"      <Button.Template>"
                        L"        <ControlTemplate TargetType='Button'>"
                        L"          <Grid Background='{ThemeResource SystemControlBackgroundAccentBrush}' Height='200' Width='200' />"
                        L"        </ControlTemplate>"
                        L"      </Button.Template>"
                        L"    </Button>"
                        L"</Page>";

                    initThemeResources(Application::Current->Resources);

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    page->RequestedTheme = ElementTheme::Dark;

                    auto button = safe_cast<Button^>(page->Content);
                    button->ApplyTemplate();
                    auto child = TreeHelper::GetVisualChildByType<Grid>(button);

                    auto brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::Pink, brush->Color);

                    // update page theme resources
                    initThemeResources(page->Resources);
                    updateThemeResources(page->Resources);

                    page->RequestedTheme = ElementTheme::Dark; // change theme

                    brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light; // change theme

                    brush = safe_cast<SolidColorBrush^>(child->Background);
                    VERIFY_ARE_EQUAL(Colors::LightBlue, brush->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::LooseXamlWithStaticResourceAsThemeResource()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

            RunOnUIThread([&]()
            {
                std::wstring xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                    L"    <Page.Resources>\r\n"
                    L"      <ResourceDictionary>\r\n"
                    L"        <ResourceDictionary.ThemeDictionaries>\r\n"
                    L"          <ResourceDictionary x:Key='Default'>\r\n";
                for (int i = 0; i < 200; i++)
                {
                    xamlString += L"            <SolidColorBrush x:Key='MyBrush";
                    xamlString += std::to_wstring(i);
                    xamlString += L"' Color='Red' />\r\n";
                }
                xamlString +=
                    L"            <Color x:Key='MyColor'>#FF0000FF</Color>\r\n"
                    L"            <SolidColorBrush x:Key='MyBrush' Color='{StaticResource MyColor}' />\r\n"
                    L"            <StaticResource x:Key='MyBackground' ResourceKey='MyBrush' />\r\n"
                    L"          </ResourceDictionary>\r\n"
                    L"        </ResourceDictionary.ThemeDictionaries>\r\n"
                    L"      </ResourceDictionary>\r\n"
                    L"    </Page.Resources>\r\n"
                    L"    <Button Background='{ThemeResource MyBackground}' />\r\n"
                    L"</Page>";

                auto page = safe_cast<Page^>(XamlReader::Load(ref new Platform::String(xamlString.c_str())));
                auto button = safe_cast<Button^>(page->Content);
                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::VSLooseGenericXaml()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

            // Get generic.xaml content
            auto deploymentDir = GetTestDeploymentDir();
            auto genericXamlPath = ref new Platform::String(deploymentDir + L"resources\\native\\controls\\general\\generic.xaml");

            Platform::String^ xamlString;
            Concurrency::create_task(::Windows::Storage::StorageFile::GetFileFromPathAsync(genericXamlPath))
                .then([&xamlString](::Windows::Storage::StorageFile^ file)
            {
                Concurrency::create_task(::Windows::Storage::FileIO::ReadTextAsync(file))
                    .then([&xamlString](Platform::String^ xamlContents)
                {
                    xamlString = xamlContents;
                }).wait();
            }).wait();

            // Load generic.xaml content
            RunOnUIThread([&xamlString]()
            {
                auto resources = safe_cast<ResourceDictionary^>(XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(resources);
            });
        }

        void ResourceDictionaryBasicTests::AccentColorChangeUpdatesGlobalThemes()
        {
            AccentColorChangeUpdatesGlobalThemesTest();
        }

        void ResourceDictionaryBasicTests::AccentColorChangeUpdatesGlobalThemesTest()
        {
            TestCleanupWrapper cleanup;

            Page^ page = nullptr;
            Button^ button = nullptr;

            // Load empty page into live tree.
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' />";

                page = safe_cast<Page^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Set accent color to Red
            TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Dark;
            TestServices::ThemingHelper->AccentColor = 0xFFFF0000;

            // Add new button to live tree with ref to global theme brush.
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"        Background='{ThemeResource SystemControlForegroundAccentBrush}' />";

                button = safe_cast<Button^>(XamlReader::Load(xamlString));
                page->Content = button;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Verify the resolved value of the button's theme ref, then remove
            // the button from the live tree.
            RunOnUIThread([&]()
            {
                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Red, brush->Color);

                page->Content = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Change the accent color again while the button is not in the live
            // tree, i.e. there's no ref to the global theme brush in the tree.
            TestServices::ThemingHelper->AccentColor = 0xFF0000FF;

            // Add the button back to the live tree, and verify that the resolved
            // value of the button's theme ref has updated.
            RunOnUIThread([&]()
            {
                page->Content = button;

                auto brush = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
            });
        }

        void ResourceDictionaryBasicTests::ThemeResourceOutsideThemeDictionary()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                // The MUXC theme resources clash with these changes, so we'll clear them out first.
                Application::Current->Resources->MergedDictionaries->Clear();

                auto appResources = Application::Current->Resources;

                if (!appResources->ThemeDictionaries->HasKey("Dark"))
                    appResources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!appResources->ThemeDictionaries->HasKey("Light"))
                    appResources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(appResources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"ButtonBackground", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(appResources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"ButtonBackground", ref new SolidColorBrush(Colors::Pink));

                appResources->Insert(L"ButtonBackground", ref new SolidColorBrush(Colors::Blue));
            });

            LOG_OUTPUT(L"Verify app theme resource override defined outside ThemeDictionaries");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page"
                        L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"  <Button Content='Test' />"
                        L"</Page>";

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));
                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);
                    auto brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light;

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Dark;

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
                });
            }

            LOG_OUTPUT(L"Verify page theme resource override defined outside ThemeDictionaries");
            {

                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page"
                        L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                        L"    RequestedTheme='Dark'>"
                        L"  <Page.Resources>"
                        L"    <ResourceDictionary>"
                        L"      <ResourceDictionary.ThemeDictionaries>"
                        L"        <ResourceDictionary x:Key='Dark'>"
                        L"          <SolidColorBrush x:Key='ButtonBackground' Color='Red'/>"
                        L"        </ResourceDictionary>"
                        L"        <ResourceDictionary x:Key='Light'>"
                        L"          <SolidColorBrush x:Key='ButtonBackground' Color='Pink'/>"
                        L"        </ResourceDictionary>"
                        L"      </ResourceDictionary.ThemeDictionaries>"
                        L"      <SolidColorBrush x:Key='ButtonBackground' Color='Blue'/>"
                        L"    </ResourceDictionary>"
                        L"  </Page.Resources>"
                        L"  <Button Content='Test' />"
                        L"</Page>";

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));
                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button = safe_cast<Button^>(page->Content);
                    auto brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Light;

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Dark;

                    brush = safe_cast<SolidColorBrush^>(button->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::ThemeDictionariesInMergedDictionaries()
        {
            TestCleanupWrapper cleanup;
            ApplicationThemeOverrider themeGuard;
            auto cleanupAppResources = wil::scope_exit([]{
                RunOnUIThread([]()
                {
                    Application::Current->Resources->Clear();
                });
            });
            auto initAppResources = []()
            {
                Platform::String^ xamlString =
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <ResourceDictionary.ThemeDictionaries>"
                    L"    <ResourceDictionary x:Key='Light'>"
                    L"        <Color x:Key='MyThemeColor'>#FF0000FF</Color>"
                    L"        <SolidColorBrush x:Key='MyThemeBrush' Color='{ThemeResource MyThemeColor}' />"
                    L"    </ResourceDictionary>"
                    L"    <ResourceDictionary x:Key='Dark'>"
                    L"        <Color x:Key='MyThemeColor'>#FFFF0000</Color>"
                    L"        <SolidColorBrush x:Key='MyThemeBrush' Color='{ThemeResource MyThemeColor}' />"
                    L"    </ResourceDictionary>"
                    L"  </ResourceDictionary.ThemeDictionaries>"
                    L"</ResourceDictionary>";

                auto resources = safe_cast<ResourceDictionary^>(XamlReader::Load(xamlString));
                Application::Current->Resources->MergedDictionaries->Append(resources);
            };

            Page^ page = nullptr;
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);
                initAppResources();

                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Light'>"
                    L"              <Color x:Key='MyPageThemeColor'>Yellow</Color>"
                    L"              <SolidColorBrush x:Key='MyPageThemeBrush' Color='{ThemeResource MyPageThemeColor}' />"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"              <Color x:Key='MyPageThemeColor'>Orange</Color>"
                    L"              <SolidColorBrush x:Key='MyPageThemeBrush' Color='{ThemeResource MyPageThemeColor}' />"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"        <Style TargetType='Button'>"
                    L"          <Setter Property='Foreground' Value='{ThemeResource MyPageThemeBrush}' />"
                    L"          <Setter Property='Template'>"
                    L"            <Setter.Value>"
                    L"              <ControlTemplate TargetType='Button'>"
                    L"                <Grid x:Name='RootGrid' Background='{ThemeResource MyThemeBrush}' Width='300' Height='35' />"
                    L"              </ControlTemplate>"
                    L"            </Setter.Value>"
                    L"          </Setter>"
                    L"        </Style>"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <StackPanel>"
                    L"      <Button />"
                    L"      <Button RequestedTheme='Light' />"
                    L"      <Button />"
                    L"      <Button RequestedTheme='Dark' />"
                    L"      <Button />"
                    L"    </StackPanel>"
                    L"</Page>";

                page = safe_cast<Page^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto panel = safe_cast<StackPanel^>(page->Content);

                auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                auto panel1 = safe_cast<Panel^>(TreeHelper::GetVisualChildByName(button1, L"RootGrid"));
                auto brush1 = safe_cast<SolidColorBrush^>(panel1->Background);
                auto button2 = safe_cast<Button^>(panel->Children->GetAt(1));
                auto panel2 = safe_cast<Panel^>(TreeHelper::GetVisualChildByName(button2, L"RootGrid"));
                auto brush2 = safe_cast<SolidColorBrush^>(panel2->Background);
                auto button3 = safe_cast<Button^>(panel->Children->GetAt(2));
                auto panel3 = safe_cast<Panel^>(TreeHelper::GetVisualChildByName(button3, L"RootGrid"));
                auto brush3 = safe_cast<SolidColorBrush^>(panel3->Background);
                auto button4 = safe_cast<Button^>(panel->Children->GetAt(3));
                auto panel4 = safe_cast<Panel^>(TreeHelper::GetVisualChildByName(button4, L"RootGrid"));
                auto brush4 = safe_cast<SolidColorBrush^>(panel4->Background);
                auto button5 = safe_cast<Button^>(panel->Children->GetAt(4));
                auto panel5 = safe_cast<Panel^>(TreeHelper::GetVisualChildByName(button5, L"RootGrid"));
                auto brush5 = safe_cast<SolidColorBrush^>(panel5->Background);

                LOG_OUTPUT(L"Verify Application resources");
                VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                VERIFY_ARE_EQUAL(Colors::Blue, brush2->Color);
                VERIFY_ARE_EQUAL(Colors::Red, brush3->Color);
                VERIFY_ARE_EQUAL(Colors::Red, brush4->Color);
                VERIFY_ARE_EQUAL(Colors::Red, brush5->Color);

                brush1 = safe_cast<SolidColorBrush^>(button1->Foreground);
                brush2 = safe_cast<SolidColorBrush^>(button2->Foreground);
                brush3 = safe_cast<SolidColorBrush^>(button3->Foreground);
                brush4 = safe_cast<SolidColorBrush^>(button4->Foreground);
                brush5 = safe_cast<SolidColorBrush^>(button5->Foreground);

                LOG_OUTPUT(L"Verify page resources");
                VERIFY_ARE_EQUAL(Colors::Orange, brush1->Color);
                VERIFY_ARE_EQUAL(Colors::Yellow, brush2->Color);
                VERIFY_ARE_EQUAL(Colors::Orange, brush3->Color);
                VERIFY_ARE_EQUAL(Colors::Orange, brush4->Color);
                VERIFY_ARE_EQUAL(Colors::Orange, brush5->Color);
            });
        }

        void ResourceDictionaryBasicTests::MergedDictionariesInThemeDictionaries()
        {
            MergedDictionariesInThemeDictionariesTest();
        }

        void ResourceDictionaryBasicTests::MergedDictionariesInThemeDictionariesTest()
        {
            TestCleanupWrapper cleanup;
            ApplicationThemeOverrider themeGuard;
            auto cleanupAppResources = wil::scope_exit([]{
                RunOnUIThread([]()
                {
                    Application::Current->Resources->Clear();
                });
            });
            auto initAppResources = []()
            {
                Platform::String^ xamlString =
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <ResourceDictionary.ThemeDictionaries>"
                    L"    <ResourceDictionary x:Key='Light'>"
                    L"        <ResourceDictionary.MergedDictionaries>"
                    L"           <ResourceDictionary>"
                    L"              <Color x:Key='MyThemeColor'>#FF0000FF</Color>"
                    L"              <SolidColorBrush x:Key='MyThemeBrush' Color='{ThemeResource MyThemeColor}' />"
                    L"           </ResourceDictionary>"
                    L"        </ResourceDictionary.MergedDictionaries>"
                    L"    </ResourceDictionary>"
                    L"    <ResourceDictionary x:Key='Dark'>"
                    L"        <ResourceDictionary.MergedDictionaries>"
                    L"           <ResourceDictionary>"
                    L"              <Color x:Key='MyThemeColor'>#FFFF0000</Color>"
                    L"              <SolidColorBrush x:Key='MyThemeBrush' Color='{ThemeResource MyThemeColor}' />"
                    L"           </ResourceDictionary>"
                    L"        </ResourceDictionary.MergedDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </ResourceDictionary.ThemeDictionaries>"
                    L"</ResourceDictionary>";

                auto resources = safe_cast<ResourceDictionary^>(XamlReader::Load(xamlString));
                Application::Current->Resources = resources;
            };

            Page^ page = nullptr;
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
                initAppResources();

                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Page.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Light'>"
                    L"            <ResourceDictionary.MergedDictionaries>"
                    L"               <ResourceDictionary>"
                    L"                  <Color x:Key='MyPageThemeColor'>Yellow</Color>"
                    L"                  <SolidColorBrush x:Key='MyPageThemeBrush' Color='{ThemeResource MyPageThemeColor}' />"
                    L"               </ResourceDictionary>"
                    L"            </ResourceDictionary.MergedDictionaries>"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"             <ResourceDictionary.MergedDictionaries>"
                    L"                <ResourceDictionary>"
                    L"                   <Color x:Key='MyPageThemeColor'>Orange</Color>"
                    L"                   <SolidColorBrush x:Key='MyPageThemeBrush' Color='{ThemeResource MyPageThemeColor}' />"
                    L"                </ResourceDictionary>"
                    L"             </ResourceDictionary.MergedDictionaries>"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"        <Style TargetType='Button'>"
                    L"          <Setter Property='Foreground' Value='{ThemeResource MyPageThemeBrush}' />"
                    L"          <Setter Property='Background' Value='{ThemeResource MyThemeBrush}' />"
                    L"        </Style>"
                    L"      </ResourceDictionary>"
                    L"    </Page.Resources>"
                    L"    <StackPanel>"
                    L"      <Button />"
                    L"      <Button RequestedTheme='Light' />"
                    L"      <Button />"
                    L"      <Button RequestedTheme='Dark' />"
                    L"      <Button />"
                    L"    </StackPanel>"
                    L"</Page>";

                page = safe_cast<Page^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            Button ^button1, ^button2, ^button3, ^button4, ^button5;
            SolidColorBrush ^brush1, ^brush2, ^brush3, ^brush4, ^brush5;

            RunOnUIThread([&]()
            {
                auto panel = safe_cast<StackPanel^>(page->Content);

                button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                button2 = safe_cast<Button^>(panel->Children->GetAt(1));
                brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                button3 = safe_cast<Button^>(panel->Children->GetAt(2));
                brush3 = safe_cast<SolidColorBrush^>(button3->Background);
                button4 = safe_cast<Button^>(panel->Children->GetAt(3));
                brush4 = safe_cast<SolidColorBrush^>(button4->Background);
                button5 = safe_cast<Button^>(panel->Children->GetAt(4));
                brush5 = safe_cast<SolidColorBrush^>(button5->Background);

                LOG_OUTPUT(L"Verify Application resources");
                VERIFY_ARE_EQUAL(Colors::Blue, brush1->Color);
                VERIFY_ARE_EQUAL(Colors::Blue, brush2->Color);
                VERIFY_ARE_EQUAL(Colors::Blue, brush3->Color);
                VERIFY_ARE_EQUAL(Colors::Red, brush4->Color);
                VERIFY_ARE_EQUAL(Colors::Blue, brush5->Color);

                brush1 = safe_cast<SolidColorBrush^>(button1->Foreground);
                brush2 = safe_cast<SolidColorBrush^>(button2->Foreground);
                brush3 = safe_cast<SolidColorBrush^>(button3->Foreground);
                brush4 = safe_cast<SolidColorBrush^>(button4->Foreground);
                brush5 = safe_cast<SolidColorBrush^>(button5->Foreground);

                LOG_OUTPUT(L"Verify page resources");
                VERIFY_ARE_EQUAL(Colors::Yellow, brush1->Color);
                VERIFY_ARE_EQUAL(Colors::Yellow, brush2->Color);
                VERIFY_ARE_EQUAL(Colors::Yellow, brush3->Color);
                VERIFY_ARE_EQUAL(Colors::Orange, brush4->Color);
                VERIFY_ARE_EQUAL(Colors::Yellow, brush5->Color);
            });

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                brush3 = safe_cast<SolidColorBrush^>(button3->Background);
                brush4 = safe_cast<SolidColorBrush^>(button4->Background);
                brush5 = safe_cast<SolidColorBrush^>(button5->Background);

                LOG_OUTPUT(L"Verify Application resources after theme change");
                VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                VERIFY_ARE_EQUAL(Colors::Blue, brush2->Color);
                VERIFY_ARE_EQUAL(Colors::Red, brush3->Color);
                VERIFY_ARE_EQUAL(Colors::Red, brush4->Color);
                VERIFY_ARE_EQUAL(Colors::Red, brush5->Color);

                brush1 = safe_cast<SolidColorBrush^>(button1->Foreground);
                brush2 = safe_cast<SolidColorBrush^>(button2->Foreground);
                brush3 = safe_cast<SolidColorBrush^>(button3->Foreground);
                brush4 = safe_cast<SolidColorBrush^>(button4->Foreground);
                brush5 = safe_cast<SolidColorBrush^>(button5->Foreground);

                LOG_OUTPUT(L"Verify page resources after theme change");
                VERIFY_ARE_EQUAL(Colors::Orange, brush1->Color);
                VERIFY_ARE_EQUAL(Colors::Yellow, brush2->Color);
                VERIFY_ARE_EQUAL(Colors::Orange, brush3->Color);
                VERIFY_ARE_EQUAL(Colors::Orange, brush4->Color);
                VERIFY_ARE_EQUAL(Colors::Orange, brush5->Color);

            });
        }

        void ResourceDictionaryBasicTests::PageThemeResourceMultiplePagesOneOverride()
        {
            TestCleanupWrapper cleanup;
            ApplicationThemeOverrider themeGuard;

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);

                // Add MyThemeBrush definitions to app's Dark and Light theme resources.
                // Add style and template to app's resources that have a theme ref to MyThemeBrush.
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"MyThemeBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"MyThemeBrush", ref new SolidColorBrush(Colors::Pink));

                Platform::String^ xamlString1 =
                    L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"       TargetType='Button'>"
                    L"  <Setter Property='Background' Value='{ThemeResource MyThemeBrush}' />"
                    L"</Style>";

                auto style1 = safe_cast<Style^>(XamlReader::Load(xamlString1));
                Application::Current->Resources->Insert("MyStyle1", style1);

                Platform::String^ xamlString2 =
                    L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"       TargetType='Button'>"
                    L"  <Setter Property='Template'>"
                    L"    <Setter.Value>"
                    L"      <ControlTemplate TargetType='Button'>"
                    L"        <Grid x:Name='RootGrid' Background='{ThemeResource MyThemeBrush}' Width='300' Height='35' />"
                    L"      </ControlTemplate>"
                    L"    </Setter.Value>"
                    L"  </Setter>"
                    L"</Style>";

                auto style2 = safe_cast<Style^>(XamlReader::Load(xamlString2));
                Application::Current->Resources->Insert("MyStyle2", style2);
            });

            LOG_OUTPUT(L"Verify that a theme ref in a style used in two pages resolves correctly "
                       L"when the theme resource is overridden in one page but not the other.");
            {
                Page^ page1 = nullptr;
                Page^ page2 = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString1 =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Button Style='{ThemeResource MyStyle1}' />"
                        L"</Page>";

                    Platform::String^ xamlString2 =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='MyThemeBrush' Color='Green' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='MyThemeBrush' Color='LightGreen' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button Style='{ThemeResource MyStyle1}' />"
                        L"</Page>";

                    page1 = safe_cast<Page^>(XamlReader::Load(xamlString1));
                    page2 = safe_cast<Page^>(XamlReader::Load(xamlString2));

                    TestServices::WindowHelper->WindowContent = page1;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button1 = safe_cast<Button^>(page1->Content);
                    auto brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = page2;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button2 = safe_cast<Button^>(page2->Content);
                    auto brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Green, brush2->Color);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = page1;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button1 = safe_cast<Button^>(page1->Content);
                    auto brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                });
            }

            LOG_OUTPUT(L"Verify that a theme ref in a template used in two pages resolves correctly "
                       L"when the theme resource is overridden in one page but not the other.");
            {
                Page^ page1 = nullptr;
                Page^ page2 = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString1 =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Button Style='{ThemeResource MyStyle2}' />"
                        L"</Page>";

                    Platform::String^ xamlString2 =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <SolidColorBrush x:Key='MyThemeBrush' Color='Green' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <SolidColorBrush x:Key='MyThemeBrush' Color='LightGreen' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <Button Style='{ThemeResource MyStyle2}' />"
                        L"</Page>";

                    page1 = safe_cast<Page^>(XamlReader::Load(xamlString1));
                    page2 = safe_cast<Page^>(XamlReader::Load(xamlString2));

                    TestServices::WindowHelper->WindowContent = page1;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button1 = safe_cast<Button^>(page1->Content);
                    auto panel1 = safe_cast<Panel^>(TreeHelper::GetVisualChildByName(button1, L"RootGrid"));
                    auto brush1 = safe_cast<SolidColorBrush^>(panel1->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = page2;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button2 = safe_cast<Button^>(page2->Content);
                    auto panel2 = safe_cast<Panel^>(TreeHelper::GetVisualChildByName(button2, L"RootGrid"));
                    auto brush2 = safe_cast<SolidColorBrush^>(panel2->Background);
                    VERIFY_ARE_EQUAL(Colors::Green, brush2->Color);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = page1;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto button1 = safe_cast<Button^>(page1->Content);
                    auto panel1 = safe_cast<Panel^>(TreeHelper::GetVisualChildByName(button1, L"RootGrid"));
                    auto brush1 = safe_cast<SolidColorBrush^>(panel1->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::PageThemeResourceNestedOneOverride()
        {
            TestCleanupWrapper cleanup;
            ApplicationThemeOverrider themeGuard;

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);

                // Add MyThemeBrush definitions to app's Dark and Light theme resources.
                // Add style and template to app's resources that have a theme ref to MyThemeBrush.
                auto resources = Application::Current->Resources;

                if (!resources->ThemeDictionaries->HasKey("Dark"))
                    resources->ThemeDictionaries->Insert(L"Dark", ref new ResourceDictionary());
                if (!resources->ThemeDictionaries->HasKey("Light"))
                    resources->ThemeDictionaries->Insert(L"Light", ref new ResourceDictionary());

                auto darkResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Dark"));
                darkResources->Insert(L"MyThemeBrush", ref new SolidColorBrush(Colors::Red));

                auto lightResources = safe_cast<ResourceDictionary^>(resources->ThemeDictionaries->Lookup("Light"));
                lightResources->Insert(L"MyThemeBrush", ref new SolidColorBrush(Colors::Pink));

                Platform::String^ xamlString1 =
                    L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"       TargetType='Button'>"
                    L"  <Setter Property='Background' Value='{ThemeResource MyThemeBrush}' />"
                    L"</Style>";

                auto style1 = safe_cast<Style^>(XamlReader::Load(xamlString1));
                Application::Current->Resources->Insert("MyStyle1", style1);

                Platform::String^ xamlString2 =
                    L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"       TargetType='Button'>"
                    L"  <Setter Property='Template'>"
                    L"    <Setter.Value>"
                    L"      <ControlTemplate TargetType='Button'>"
                    L"        <ContentPresenter x:Name='ContentPresenter' Background='{ThemeResource MyThemeBrush}' Width='300' Height='35' />"
                    L"      </ControlTemplate>"
                    L"    </Setter.Value>"
                    L"  </Setter>"
                    L"</Style>";

                auto style2 = safe_cast<Style^>(XamlReader::Load(xamlString2));
                Application::Current->Resources->Insert("MyStyle2", style2);
            });

            LOG_OUTPUT(L"Verify that a theme ref in a style used in two locations resolves correctly "
                       L"when the theme resource is overridden in a nested location but not the outer.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <StackPanel>"
                        L"      <Button Style='{ThemeResource MyStyle1}' />"
                        L"      <UserControl>"
                        L"        <UserControl.Resources>"
                        L"          <ResourceDictionary>"
                        L"            <ResourceDictionary.ThemeDictionaries>"
                        L"              <ResourceDictionary x:Key='Dark'>"
                        L"                <SolidColorBrush x:Key='MyThemeBrush' Color='Green' />"
                        L"              </ResourceDictionary>"
                        L"              <ResourceDictionary x:Key='Light'>"
                        L"                <SolidColorBrush x:Key='MyThemeBrush' Color='LightGreen' />"
                        L"              </ResourceDictionary>"
                        L"            </ResourceDictionary.ThemeDictionaries>"
                        L"          </ResourceDictionary>"
                        L"        </UserControl.Resources>"
                        L"        <Button Style='{ThemeResource MyStyle1}' />"
                        L"      </UserControl>"
                        L"    </StackPanel>"
                        L"</Page>";

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));
                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto panel = safe_cast<Panel^>(page->Content);
                    auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    auto userControl = safe_cast<UserControl^>(panel->Children->GetAt(1));
                    auto button2 = safe_cast<Button^>(userControl->Content);
                    auto brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    auto brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                    VERIFY_ARE_EQUAL(Colors::Green, brush2->Color);

                    TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto panel = safe_cast<Panel^>(page->Content);
                    auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    auto userControl = safe_cast<UserControl^>(panel->Children->GetAt(1));
                    auto button2 = safe_cast<Button^>(userControl->Content);
                    auto brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    auto brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Pink, brush1->Color);
                    VERIFY_ARE_EQUAL(Colors::LightGreen, brush2->Color);

                    TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto panel = safe_cast<Panel^>(page->Content);
                    auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    auto userControl = safe_cast<UserControl^>(panel->Children->GetAt(1));
                    auto button2 = safe_cast<Button^>(userControl->Content);
                    auto brush1 = safe_cast<SolidColorBrush^>(button1->Background);
                    auto brush2 = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                    VERIFY_ARE_EQUAL(Colors::Green, brush2->Color);
                });
            }

            LOG_OUTPUT(L"Verify that a theme ref in a template used in two locations resolves correctly "
                       L"when the theme resource is overridden in a nested location but not the outer.");
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <StackPanel>"
                        L"      <Button Style='{ThemeResource MyStyle1}' />"
                        L"      <UserControl>"
                        L"        <UserControl.Resources>"
                        L"          <ResourceDictionary>"
                        L"            <ResourceDictionary.ThemeDictionaries>"
                        L"              <ResourceDictionary x:Key='Dark'>"
                        L"                <SolidColorBrush x:Key='MyThemeBrush' Color='Green' />"
                        L"              </ResourceDictionary>"
                        L"              <ResourceDictionary x:Key='Light'>"
                        L"                <SolidColorBrush x:Key='MyThemeBrush' Color='LightGreen' />"
                        L"              </ResourceDictionary>"
                        L"            </ResourceDictionary.ThemeDictionaries>"
                        L"          </ResourceDictionary>"
                        L"        </UserControl.Resources>"
                        L"        <Button Style='{ThemeResource MyStyle1}' />"
                        L"      </UserControl>"
                        L"    </StackPanel>"
                        L"</Page>";

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));
                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto panel = safe_cast<Panel^>(page->Content);
                    auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    auto userControl = safe_cast<UserControl^>(panel->Children->GetAt(1));
                    auto button2 = safe_cast<Button^>(userControl->Content);
                    auto presenter1 = safe_cast<ContentPresenter^>(TreeHelper::GetVisualChildByName(button1, L"ContentPresenter"));
                    auto presenter2 = safe_cast<ContentPresenter^>(TreeHelper::GetVisualChildByName(button2, L"ContentPresenter"));
                    auto brush1 = safe_cast<SolidColorBrush^>(presenter1->Background);
                    auto brush2 = safe_cast<SolidColorBrush^>(presenter2->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                    VERIFY_ARE_EQUAL(Colors::Green, brush2->Color);

                    TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto panel = safe_cast<Panel^>(page->Content);
                    auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    auto userControl = safe_cast<UserControl^>(panel->Children->GetAt(1));
                    auto button2 = safe_cast<Button^>(userControl->Content);
                    auto presenter1 = safe_cast<ContentPresenter^>(TreeHelper::GetVisualChildByName(button1, L"ContentPresenter"));
                    auto presenter2 = safe_cast<ContentPresenter^>(TreeHelper::GetVisualChildByName(button2, L"ContentPresenter"));
                    auto brush1 = safe_cast<SolidColorBrush^>(presenter1->Background);
                    auto brush2 = safe_cast<SolidColorBrush^>(presenter2->Background);
                    VERIFY_ARE_EQUAL(Colors::Pink, brush1->Color);
                    VERIFY_ARE_EQUAL(Colors::LightGreen, brush2->Color);

                    TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto panel = safe_cast<Panel^>(page->Content);
                    auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
                    auto userControl = safe_cast<UserControl^>(panel->Children->GetAt(1));
                    auto button2 = safe_cast<Button^>(userControl->Content);
                    auto presenter1 = safe_cast<ContentPresenter^>(TreeHelper::GetVisualChildByName(button1, L"ContentPresenter"));
                    auto presenter2 = safe_cast<ContentPresenter^>(TreeHelper::GetVisualChildByName(button2, L"ContentPresenter"));
                    auto brush1 = safe_cast<SolidColorBrush^>(presenter1->Background);
                    auto brush2 = safe_cast<SolidColorBrush^>(presenter2->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                    VERIFY_ARE_EQUAL(Colors::Green, brush2->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::ThemeRefInThemeResource()
        {
            ThemeRefInThemeResourceTest();
        }

        void ResourceDictionaryBasicTests::ThemeRefInThemeResourceTest()
        {
            TestCleanupWrapper cleanup;

            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <Color x:Key='MyThemeColor'>#FF0000FF</Color>"
                        L"            <SolidColorBrush x:Key='MyThemeBrush' Color='{ThemeResource MyThemeColor}' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <Color x:Key='MyThemeColor'>#FFFF0000</Color>"
                        L"            <SolidColorBrush x:Key='MyThemeBrush' Color='{ThemeResource MyThemeColor}' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"        <SolidColorBrush x:Key='MyThemeBrush_WillChange' Color='{ThemeResource MyThemeColor}' />"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <StackPanel>"
                        L"      <StackPanel RequestedTheme='Dark'>"
                        L"        <Button Background='{ThemeResource MyThemeBrush}' />"
                        L"      </StackPanel>"
                        L"      <StackPanel RequestedTheme='Light'>"
                        L"        <Button Background='{ThemeResource MyThemeBrush}' />"
                        L"        <Button Background='{ThemeResource MyThemeBrush}' />"
                        L"        <Button Background='{ThemeResource MyThemeBrush_WillChange}' />"
                        L"      </StackPanel>"
                        L"    </StackPanel>"
                        L"</Page>";

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));
                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto topPanel = safe_cast<StackPanel^>(page->Content);

                    auto panel1 = safe_cast<StackPanel^>(topPanel->Children->GetAt(0));
                    VERIFY_ARE_EQUAL(ElementTheme::Dark, panel1->RequestedTheme);
                    auto panel2 = safe_cast<StackPanel^>(topPanel->Children->GetAt(1));
                    VERIFY_ARE_EQUAL(ElementTheme::Light, panel2->RequestedTheme);

                    auto button1 = safe_cast<Button^>(panel1->Children->GetAt(0));
                    auto button2 = safe_cast<Button^>(panel2->Children->GetAt(0));
                    auto button3 = safe_cast<Button^>(panel2->Children->GetAt(1));

                    // We expect the background on button4 to change since the brush itself is not inside
                    // a theme dictionary because otherwise we'd have to keep multiple instances of the brush
                    // around. Developers are encouraged to put the brush inside the ThemeDictionary
                    auto button4_backgroundshouldchange = safe_cast<Button^>(panel2->Children->GetAt(2));

                    auto brush = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->Color);
                    brush = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
                    brush = safe_cast<SolidColorBrush^>(button3->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
                    brush = safe_cast<SolidColorBrush^>(button4_backgroundshouldchange->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Dark;

                    brush = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->Color);
                    brush = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
                    brush = safe_cast<SolidColorBrush^>(button3->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
                    brush = safe_cast<SolidColorBrush^>(button4_backgroundshouldchange->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"This should update with the theme change");
                });
            }

            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <ResourceDictionary>"
                        L"        <ResourceDictionary.ThemeDictionaries>"
                        L"          <ResourceDictionary x:Key='Light'>"
                        L"            <Color x:Key='MyThemeColor'>#FF0000FF</Color>"
                        L"            <SolidColorBrush x:Key='MyThemeBrush' Color='{ThemeResource MyThemeColor}' />"
                        L"          </ResourceDictionary>"
                        L"          <ResourceDictionary x:Key='Dark'>"
                        L"            <Color x:Key='MyThemeColor'>#FFFF0000</Color>"
                        L"            <SolidColorBrush x:Key='MyThemeBrush' Color='{ThemeResource MyThemeColor}' />"
                        L"          </ResourceDictionary>"
                        L"        </ResourceDictionary.ThemeDictionaries>"
                        L"      </ResourceDictionary>"
                        L"    </Page.Resources>"
                        L"    <StackPanel>"
                        L"      <StackPanel RequestedTheme='Dark'>"
                        L"        <Button Background='{ThemeResource MyThemeBrush}' />"
                        L"      </StackPanel>"
                        L"      <StackPanel RequestedTheme='Light'>"
                        L"        <Button Background='{ThemeResource MyThemeBrush}' />"
                        L"      </StackPanel>"
                        L"    </StackPanel>"
                        L"</Page>";

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));
                    TestServices::WindowHelper->WindowContent = page;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    auto topPanel = safe_cast<StackPanel^>(page->Content);

                    auto panel1 = safe_cast<StackPanel^>(topPanel->Children->GetAt(0));
                    VERIFY_ARE_EQUAL(ElementTheme::Dark, panel1->RequestedTheme);
                    auto panel2 = safe_cast<StackPanel^>(topPanel->Children->GetAt(1));
                    VERIFY_ARE_EQUAL(ElementTheme::Light, panel2->RequestedTheme);

                    auto button1 = safe_cast<Button^>(panel1->Children->GetAt(0));
                    auto button2 = safe_cast<Button^>(panel2->Children->GetAt(0));

                    auto brush = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->Color);
                    brush = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

                    page->RequestedTheme = ElementTheme::Dark;

                    brush = safe_cast<SolidColorBrush^>(button1->Background);
                    VERIFY_ARE_EQUAL(Colors::Red, brush->Color);
                    brush = safe_cast<SolidColorBrush^>(button2->Background);
                    VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::PageResourceCanOverrideThemeResources()
        {
            TestCleanupWrapper cleanup;
            {
                Page^ page = nullptr;
                RunOnUIThread([&]()
                {
                    Platform::String^ xamlString =
                        L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Page.Resources>"
                        L"      <SolidColorBrush x:Key='SystemControlBackgroundChromeBlackHighBrush'>Purple</SolidColorBrush>"
                        L"    </Page.Resources>"
                        L"    <StackPanel>"
                        L"      <StackPanel.Resources>"
                        L"        <x:Int32 x:Key='MyInt'>123</x:Int32>"
                        L"      </StackPanel.Resources>"
                        L"      <Rectangle Name='MyRect' Width='100' Height='100' Fill='{StaticResource SystemControlBackgroundChromeBlackHighBrush}'/>"
                        L"    </StackPanel>"
                        L"</Page>";

                    page = safe_cast<Page^>(XamlReader::Load(xamlString));

                    TestServices::WindowHelper->WindowContent = page;
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    auto panel = safe_cast<StackPanel^>(page->Content);
                    auto rect = safe_cast<xaml_shapes::Rectangle^>(panel->FindName(L"MyRect"));
                    auto brush = safe_cast<SolidColorBrush^>(rect->Fill);

                    VERIFY_ARE_EQUAL(Colors::Purple, brush->Color);
                });
            }
        }

        void ResourceDictionaryBasicTests::ActualThemeBasic()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootPanel;
            xaml_shapes::Rectangle^ rectangle;

            LOG_OUTPUT(L"Verify ActualTheme property and ActualThemeChanged event");
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->UnsetApplicationRequestedTheme();

                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='Red' /> "
                    L"</StackPanel>"));

                rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            int rootPanelThemeChangedCount = 0;
            int rectangleThemeChangedCount = 0;
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);

                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                    {
                        rootPanelThemeChangedCount++;
                    });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                    {
                        rectangleThemeChangedCount++;
                    });

                // Set root's RequestedTheme to same value as current actual theme.
                rootPanel->RequestedTheme = ElementTheme::Dark;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change RequestedTheme at the root several times.
                rootPanel->RequestedTheme = ElementTheme::Light;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);

                rootPanel->RequestedTheme = ElementTheme::Dark;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);

                rootPanel->RequestedTheme = ElementTheme::Light;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(3, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(3, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rootPanelThemeChangedCount = rectangleThemeChangedCount = 0;

                // Change RequestedTheme only at the child element.
                rectangle->RequestedTheme = ElementTheme::Dark;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);

                rectangle->RequestedTheme = ElementTheme::Light;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ResourceDictionaryBasicTests::ActualThemeWithAppThemeChanges()
        {
            TestCleanupWrapper cleanup;
            ApplicationThemeOverrider themeGuard;

            xaml_controls::StackPanel^ rootPanel;
            xaml_shapes::Rectangle^ rectangle;
            int rootPanelThemeChangedCount = 0;
            int rectangleThemeChangedCount = 0;

            auto xaml = ref new Platform::String(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='Red'/> "
                L"</StackPanel>");

            //
            // Set app's RequestedTheme after loading content.
            //
            LOG_OUTPUT(L"Set app theme before loading content.");
            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xaml));

                rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);

                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rootPanelThemeChangedCount++;
                });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rectangleThemeChangedCount++;
                });

                // Set app's RequestedTheme to same value as current actual theme.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change RequestedTheme at the root several times.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);

                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);

                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(3, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(3, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rootPanelThemeChangedCount = rectangleThemeChangedCount = 0;

                // Change child element's RequestedTheme.
                rectangle->RequestedTheme = ElementTheme::Dark;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);

                rectangle->RequestedTheme = ElementTheme::Light;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rectangle = nullptr;
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            //
            // Set app's RequestedTheme before loading content.
            //
            LOG_OUTPUT(L"Set app theme after loading content.");
            rootPanelThemeChangedCount = rectangleThemeChangedCount = 0;
            RunOnUIThread([&]()
            {
                // Set app's RequestedTheme before loading content.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);

                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='Red' /> "
                    L"</StackPanel>"));

                rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rootPanelThemeChangedCount++;
                });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rectangleThemeChangedCount++;
                });

                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);

                TestServices::WindowHelper->WindowContent = rootPanel;

                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Set app's RequestedTheme to same value as its current theme.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Set app's RequestedTheme to same value as current actual theme.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(1, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rectangle = nullptr;
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            //
            // Change system theme while content is not in live tree.
            //
            LOG_OUTPUT(L"Change app theme while content is not in live tree.");
            rootPanelThemeChangedCount = rectangleThemeChangedCount = 0;
            wf::EventRegistrationToken eventToken;
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = ref new StackPanel();

                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);

                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xaml));
                rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rootPanelThemeChangedCount++;
                });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rectangleThemeChangedCount++;
                });

                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change app theme.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rectangleThemeChangedCount);
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                eventToken = rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rootPanelThemeChangedCount++;
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change app theme.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(3, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rootPanel->ActualThemeChanged -= eventToken;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change app theme.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(4, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(3, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change app theme.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(5, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(4, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ResourceDictionaryBasicTests::ActualThemeWithSystemThemeChanges()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootPanel;
            xaml_shapes::Rectangle^ rectangle;
            int rootPanelThemeChangedCount = 0;
            int rectangleThemeChangedCount = 0;

            auto xaml = ref new Platform::String(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='Red'/> "
                L"</StackPanel>");

            //
            // Change system theme after loading content.
            //
            LOG_OUTPUT(L"Change system theme after loading content.");
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->UnsetApplicationRequestedTheme();

                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xaml));
                rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rootPanelThemeChangedCount++;
                });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rectangleThemeChangedCount++;
                });

                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);

                TestServices::WindowHelper->WindowContent = rootPanel;

                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change system theme.
                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Light;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(1, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change system theme.
                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rectangle = nullptr;
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            //
            // Change system theme before loading content.
            //
            LOG_OUTPUT(L"Change system theme before loading content.");
            rootPanelThemeChangedCount = rectangleThemeChangedCount = 0;
            RunOnUIThread([&]()
            {
                // Change system theme.
                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Light;

                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xaml));
                rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rootPanelThemeChangedCount++;
                });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rectangleThemeChangedCount++;
                });

                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);

                TestServices::WindowHelper->WindowContent = rootPanel;

                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change system theme.
                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(1, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rectangle = nullptr;
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            //
            // Change system theme while content is not in live tree.
            //
            LOG_OUTPUT(L"Change system theme while content is not in live tree.");
            rootPanelThemeChangedCount = rectangleThemeChangedCount = 0;
            wf::EventRegistrationToken eventToken;
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = ref new StackPanel();

                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Light;

                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xaml));
                rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                    {
                        rootPanelThemeChangedCount++;
                    });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                    {
                        rectangleThemeChangedCount++;
                    });

                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change system theme.
                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rectangleThemeChangedCount);
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                eventToken = rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                    {
                        rootPanelThemeChangedCount++;
                    });
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change system theme.
                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Light;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(3, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rootPanel->ActualThemeChanged -= eventToken;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change system theme.
                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(4, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(3, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change system theme.
                TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Light;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(5, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(4, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ResourceDictionaryBasicTests::ActualThemeWithHighContrast()
        {
            TestCleanupWrapper cleanup;
            ApplicationThemeOverrider themeGuard;

            xaml_controls::StackPanel^ rootPanel;
            xaml_shapes::Rectangle^ rectangle;
            int rootPanelThemeChangedCount = 0;
            int rectangleThemeChangedCount = 0;

            auto xaml = ref new Platform::String(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='Red'/> "
                L"</StackPanel>");

            //
            // Start with HighContrast off.
            //
            LOG_OUTPUT(L"Start with HighContrast off.");
            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xaml));
                rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rootPanelThemeChangedCount++;
                });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rectangleThemeChangedCount++;
                });

                TestServices::WindowHelper->WindowContent = rootPanel;

                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Turn on high contrast.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::White;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            RunOnUIThread([&]()
            {
                // Change high contrast.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Turn off high contrast.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::None;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Turn on high contrast again.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Set root's RequestedTheme while high contrast is on.
                rootPanel->RequestedTheme = ElementTheme::Light;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(1, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change root's RequestedTheme while high contrast is on.
                rootPanel->RequestedTheme = ElementTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Reset root's RequestedTheme to same value while high contrast is on.
                rootPanel->RequestedTheme = ElementTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change high contrast.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::White;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rectangle = nullptr;
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            //
            // Start with HighContrast on.
            //
            LOG_OUTPUT(L"Start with HighContrast on.");
            rootPanelThemeChangedCount = rectangleThemeChangedCount = 0;
            RunOnUIThread([&]()
            {
                // Turn on high contrast before loading.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::White;

                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xaml));
                rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rootPanelThemeChangedCount++;
                });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rectangleThemeChangedCount++;
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Set root's RequestedTheme while high contrast is on.
                rootPanel->RequestedTheme = ElementTheme::Light;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(1, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change root's RequestedTheme while high contrast is on.
                rootPanel->RequestedTheme = ElementTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Reset root's RequestedTheme to same value while high contrast is on.
                rootPanel->RequestedTheme = ElementTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change high contrast.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rectangle = nullptr;
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            //
            // Change app theme with HighContrast on/off.
            //
            LOG_OUTPUT(L"Change app theme with HighContrast on/off.");
            rootPanelThemeChangedCount = rectangleThemeChangedCount = 0;
            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xaml));
                rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));

                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rootPanelThemeChangedCount++;
                });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rectangleThemeChangedCount++;
                });

                TestServices::WindowHelper->WindowContent = rootPanel;

                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Turn on high contrast.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::White;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            RunOnUIThread([&]()
            {
                // Turn off high contrast.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::None;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Turn on high contrast again.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::White;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Set app's RequestedTheme while high contrast is on.
                // ActualTheme values should not change -- the framework skips
                // the theme walk for app theme changes while high contrast is on.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Reset app's RequestedTheme to same value while high contrast is on.
                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(0, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Turn off high contrast.
                // ActualTheme values should change since the app theme changed
                // while high contrast theme was on.
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::None;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(1, rectangleThemeChangedCount);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rectangle = nullptr;
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ResourceDictionaryBasicTests::ActualThemeWithTreeChanges()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootPanel;
            xaml_shapes::Rectangle^ rectangle;
            int rootPanelThemeChangedCount = 0;
            int rectangleThemeChangedCount = 0;

            auto rootXaml = ref new String(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='Red' /> "
                L"</StackPanel>");

            auto rectangleXaml = ref new String(
                L"<Rectangle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"           x:Name='testRectangle' Width='50' Height='50' Fill='Red' />");

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->UnsetApplicationRequestedTheme();

                // Load empty root panel.
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(rootXaml));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Load rectangle but don't add to live tree.
                rectangle = safe_cast<xaml_shapes::Rectangle^> (xaml_markup::XamlReader::Load(rectangleXaml));

                rootPanel->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rootPanelThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rootPanelThemeChangedCount++;
                });
                rectangle->ActualThemeChanged +=
                    ref new TypedEventHandler<FrameworkElement^, Object^>([&rectangleThemeChangedCount](FrameworkElement^ sender, Object^ args)
                {
                    rectangleThemeChangedCount++;
                });

                // Set child's RequestedTheme.
                rectangle->RequestedTheme = ElementTheme::Light;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(1, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change RequestedTheme on both elements.
                rootPanel->RequestedTheme = ElementTheme::Light;
                rectangle->RequestedTheme = ElementTheme::Default;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(2, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Add child to root.
                rootPanel->Children->Append(rectangle);
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(3, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change child's RequestedTheme to different value than root.
                rectangle->RequestedTheme = ElementTheme::Dark;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(4, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change child's RequestedTheme to Default.
                rectangle->RequestedTheme = ElementTheme::Default;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(5, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change root's RequestedTheme.
                rootPanel->RequestedTheme = ElementTheme::Dark;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(6, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change root's RequestedTheme.
                rootPanel->RequestedTheme = ElementTheme::Light;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Light);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Light);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(3, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(7, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Change root's RequestedTheme to Default.
                rootPanel->RequestedTheme = ElementTheme::Default;
                VERIFY_IS_TRUE(rootPanel->ActualTheme == ElementTheme::Dark);
                VERIFY_IS_TRUE(rectangle->ActualTheme == ElementTheme::Dark);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(4, rootPanelThemeChangedCount);
                VERIFY_ARE_EQUAL(8, rectangleThemeChangedCount);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rectangle = nullptr;
                TestServices::WindowHelper->WindowContent = rootPanel = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ResourceDictionaryBasicTests::CorrectThemeChangesForMergedDictionaryInAppResources()
        {
            TestCleanupWrapper cleanup([&]{
                RunOnUIThread([&]()
                {
                    Application::Current->Resources->Clear();
                });
            });

            xaml_controls::StackPanel^ rootPanel = nullptr;
            xaml_shapes::Rectangle^ testRectangle = nullptr;
            auto rootXaml = ref new String(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='{ThemeResource MyBrush}' /> "
                L"</StackPanel>");

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->UnsetApplicationRequestedTheme();
                Application::Current->Resources->Clear();

                auto redBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                auto darkMergedDictionary = ref new ResourceDictionary();
                darkMergedDictionary->Insert("MyBrush", redBrush);

                auto darkDictionary = ref new ResourceDictionary();
                darkDictionary->MergedDictionaries->Append(darkMergedDictionary);
                Application::Current->Resources->ThemeDictionaries->Insert("Dark", darkDictionary);

                auto blueBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
                auto lightMergedDictionary = ref new ResourceDictionary();
                lightMergedDictionary->Insert("MyBrush", blueBrush);
                auto lightDictionary = ref new ResourceDictionary();
                lightDictionary->MergedDictionaries->Append(lightMergedDictionary);
                Application::Current->Resources->ThemeDictionaries->Insert("Light", lightDictionary);


                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(rootXaml));
                rootPanel->RequestedTheme = ElementTheme::Light;
                TestServices::WindowHelper->WindowContent = rootPanel;

                testRectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName("testRectangle"));
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(testRectangle->Fill)->Color);
                rootPanel->RequestedTheme = ElementTheme::Dark;
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, safe_cast<xaml_media::SolidColorBrush^>(testRectangle->Fill)->Color);
            });
        }

        void ResourceDictionaryBasicTests::VerifyDictionaryReentryOnThemeChange()
        {
            TestCleanupWrapper cleanup;

            Panel^ panel = nullptr;

            RunOnUIThread([&]()
            {
                auto xamlString = ref new String(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <Style x:Key='Style1' TargetType='Button'>"
                    L"            <Setter Property='Background' Value='{StaticResource BlueBrush}' />"
                    L"          </Style>"
                    L"          <SolidColorBrush x:Key='DummyBrush' Color='Red' />"
                    L"          <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                    L"          <Color x:Key='BlueColor'>#ff0000ff</Color>"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='Dark'>"
                    L"          <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                    L"          <Color x:Key='BlueColor'>#ff0000ff</Color>"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"      <SolidColorBrush x:Key='BlueBrush' Color='{ThemeResource BlueColor}' />"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <StackPanel>"
                    L"    <StackPanel Width='50' Height='50' Background='{ThemeResource RedBrush}' />"
                    L"  </StackPanel>"
                    L"</StackPanel>");

                LOG_OUTPUT(L"Loading xaml");
                panel = safe_cast<Panel^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = panel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Ensure all resources except BlueColor are undeferred.
                auto defaultResources = safe_cast<ResourceDictionary^>(panel->Resources->ThemeDictionaries->Lookup(L"Default"));
                auto resourcePanel = defaultResources->Lookup(L"Style1");
                auto dummy = defaultResources->Lookup(L"DummyBrush");

                // Start theme walk to the inner panel. The theme ref to RedBrush will lead the walk
                // to the Default theme dictionary. The static ref to BlueBrush in the style and the
                // brush's theme ref back will cause undeferral of BlueColor while the theme walk
                // is iterating through the Default dictionary.
                auto innerPanel = safe_cast<Panel^>(panel->Children->GetAt(0));
                VERIFY_IS_TRUE(innerPanel->ActualTheme == ElementTheme::Dark);
                LOG_OUTPUT(L"Setting innerPanel.RequestedTheme=Light");
                innerPanel->RequestedTheme = ElementTheme::Light;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting innerPanel.RequestedTheme=Dark");
                auto innerPanel = safe_cast<Panel^>(panel->Children->GetAt(0));
                innerPanel->RequestedTheme = ElementTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                panel = nullptr;
                TestServices::WindowHelper->WindowContent = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ResourceDictionaryBasicTests::VerifyMarkupExtensionAsResource()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            xaml_controls::StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <NullExtension x:Key='NullThemeResource'/>"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"      <NullExtension x:Key='AResourceThatIsInitiallyNull' />"
                    L"      <Binding x:Key='Foo' Mode='OneTime' />"
                    L"      <SolidColorBrush x:Key='TheBrush' Color='Red' />"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='{StaticResource AResourceThatIsInitiallyNull}'/> "
                    L"  <Rectangle x:Name='testRectangle2' Width='50' Height='50' Fill='{ThemeResource NullThemeResource}'/> "
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;

            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));
                auto rectangle2 = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle2"));

                VERIFY_IS_NULL(rectangle->Fill);
                VERIFY_IS_NULL(rectangle2->Fill);
            });
        }

        void ResourceDictionaryBasicTests::VerifyMarkupExtensionAsResource_NoXbfV2()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

            xaml_controls::StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <NullExtension x:Key='NullThemeResource'/>"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"      <NullExtension x:Key='AResourceThatIsInitiallyNull' />"
                    L"      <Binding x:Key='Foo' Mode='OneTime' />"
                    L"      <SolidColorBrush x:Key='TheBrush' Color='Red' />"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <Rectangle x:Name='testRectangle' Width='50' Height='50' Fill='{StaticResource AResourceThatIsInitiallyNull}'/> "
                    L"  <Rectangle x:Name='testRectangle2' Width='50' Height='50' Fill='{ThemeResource NullThemeResource}'/> "
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;

            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle"));
                auto rectangle2 = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"testRectangle2"));

                VERIFY_IS_NULL(rectangle->Fill);
                VERIFY_IS_NULL(rectangle2->Fill);
            });
        }

        void ResourceDictionaryBasicTests::VerifyCanAddNullResource()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto grid = ref new xaml_controls::Grid();
                grid->Resources->Insert(L"Foobar", nullptr);
                grid->Resources->Insert(L"Foobar2", nullptr);
                grid->Resources->Insert(L"Foobar3", nullptr);

                VERIFY_IS_NULL(grid->Resources->Lookup(L"Foobar"));
                VERIFY_IS_NULL(grid->Resources->Lookup(L"Foobar2"));
                VERIFY_IS_NULL(grid->Resources->Lookup(L"Foobar3"));
            });
        }

        void ResourceDictionaryBasicTests::DontCrashOnNonExistentKey()
        {
            // Enable failfast on stowed exception. The bug was accidentally originating an error on ResourceDictionary.HasKey. Even though
            // this caused a stowed exception, it only becames an issue when stowed exceptions cause failfasts.
            DebugSettings^ debugSettings;
            bool origFailFastOnErrors = false;
            RunOnUIThread([&]
            {
                debugSettings = Application::Current->DebugSettings;
                origFailFastOnErrors = debugSettings->FailFastOnErrors;
                debugSettings->FailFastOnErrors = true;
            });

            TestCleanupWrapper cleanup([&]()
            {
                if (debugSettings != nullptr)
                {
                    RunOnUIThread([&]
                    {
                        debugSettings->FailFastOnErrors = origFailFastOnErrors;
                    });
                }
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            auto loadingRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::FrameworkElement, Loading);
            RunOnUIThread([&]()
            {
                auto grid = ref new xaml_controls::Grid();
                loadingRegistration.Attach(grid, ref new TypedEventHandler<FrameworkElement^, Object^>([](FrameworkElement^ sender, Object^ e){
                    VERIFY_IS_FALSE(sender->Resources->HasKey(L"Foobar"));
                }));

                TestServices::WindowHelper->WindowContent = grid;
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        static void FollowPopupsInThemeResourceResolutionScenario(const ::Windows::UI::Color& buttonBackgroundColor)
        {
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(
                RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream,
                true);

            Page^ page = nullptr;
            Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Page.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <SolidColorBrush x:Key='ButtonBackground'>Purple</SolidColorBrush>"
                    L"    </ResourceDictionary>"
                    L"  </Page.Resources>"
                    L"  <Grid>"
                    L"    <Popup IsOpen='True'>"
                    L"      <Button x:Name='button'/>"
                    L"    </Popup>"
                    L"  </Grid>"
                    L"</Page>";

                page = safe_cast<Page^> (xaml_markup::XamlReader::Load(xamlString));
                button = safe_cast<Button^>(page->FindName(L"button"));

                TestServices::WindowHelper->WindowContent = page;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(buttonBackgroundColor, safe_cast<SolidColorBrush^>(button->Background)->Color);
            });
        }

        void ResourceDictionaryBasicTests::FollowPopupsInThemeResourceResolution()
        {
            TestCleanupWrapper cleanup;

            LOG_OUTPUT(L"RS5 behavior, follow popup, and find override");
            FollowPopupsInThemeResourceResolutionScenario(Microsoft::UI::Colors::Purple);
        }

        void ResourceDictionaryBasicTests::VerifyThatThemeResourceAliasesDontExist()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            xaml_controls::StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <SolidColorBrush x:Key='ButtonBackgroundBrush' Color='Blue' />"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='Dark'>"
                    L"          <SolidColorBrush x:Key='ButtonBackgroundBrush' Color='Green' />"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"      <Thickness x:Key='BorderThickness'>2,2,2,2</Thickness>"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <Border Background='Red' x:Name='m_border'>"
                    L"    <Border.Resources>"
                    L"      <StaticResource x:Key='MyStaticResourceAlias' ResourceKey='BorderThickness' />"
                    L"      <ThemeResource x:Key='MyThemeResourceAlias' ResourceKey='ButtonBackgroundBrush' />"
                    L"    </Border.Resources>"
                    L"  </Border>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;

            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto border = safe_cast<xaml_controls::Border^>(rootPanel->FindName(L"m_border"));

                VERIFY_IS_FALSE(border->Resources->HasKey(Platform::StringReference(L"MyThemeResourceAlias")));
            });
        }

        static void VerifyKeyNotFoundCacheInvalidationScenario(
            FrameworkElement^ element,
            Platform::String^ resourceName,
            Color color,
            std::function<void(FrameworkElement^, Platform::String^, Color)> modifier)
        {
            // First, failed lookup to add key to cache.
            VERIFY_THROWS_WINRT(element->Resources->Lookup(resourceName), Platform::COMException^);

            // Some action which will invalidate cache.
            modifier(element, resourceName, color);

            // And now, it should be found.
            auto resource = safe_cast<SolidColorBrush^>(element->Resources->Lookup(resourceName));
            VERIFY_IS_NOT_NULL(resource);
            VERIFY_IS_TRUE(IsSameColor(color, resource->Color));
        }

        static void VerifyExisitingMergedDictionaryScenario(
            FrameworkElement^ element,
            ResourceDictionary^ parentDictionary,
            Platform::String^ resourceName,
            Color color)
        {
            VerifyKeyNotFoundCacheInvalidationScenario(
                element,
                resourceName,
                color,
                [&](FrameworkElement^ element, Platform::String^ resourceName, Color color)
                {
                    parentDictionary->Insert(resourceName, ref new SolidColorBrush(color));
                });
        }

        static void VerifyNewMergedDictionaryScenario(
            FrameworkElement^ element,
            ResourceDictionary^ parent,
            Platform::String^ resourceName,
            Color color)
        {
            VerifyKeyNotFoundCacheInvalidationScenario(
                element,
                resourceName,
                color,
                [&](FrameworkElement^ element, Platform::String^ resourceName, Color color)
                {
                    auto mergedDict = ref new ResourceDictionary();
                    mergedDict->Insert(resourceName, ref new SolidColorBrush(color));
                    parent->MergedDictionaries->Append(mergedDict);
                });
        }

        void ResourceDictionaryBasicTests::VerifyKeyNotFoundCacheInvalidation()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            xaml_controls::StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.MergedDictionaries>"
                    L"        <ResourceDictionary>"
                    L"          <ResourceDictionary.MergedDictionaries>"
                    L"            <ResourceDictionary/>"
                    L"          </ResourceDictionary.MergedDictionaries>"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary>"
                    L"          <ResourceDictionary.MergedDictionaries>"
                    L"            <ResourceDictionary/>"
                    L"          </ResourceDictionary.MergedDictionaries>"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.MergedDictionaries>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"          <ResourceDictionary.MergedDictionaries>"
                    L"            <ResourceDictionary/>"
                    L"          </ResourceDictionary.MergedDictionaries>"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='Light'>"
                    L"          <SolidColorBrush x:Key='color6' Color='Yellow' />"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item is added to root dictionary

                VerifyKeyNotFoundCacheInvalidationScenario(
                    rootPanel,
                    L"color1",
                    Colors::AliceBlue,
                    [](FrameworkElement^ element, Platform::String^ resourceName, Color color)
                    {
                        element->Resources->Insert(resourceName, ref new SolidColorBrush(color));
                    });

                // item is added to existing merged dictionary

                VerifyExisitingMergedDictionaryScenario(
                    rootPanel,
                    rootPanel->Resources->MergedDictionaries->GetAt(0),
                    L"color2_0",
                    Colors::AntiqueWhite);

                VerifyExisitingMergedDictionaryScenario(
                    rootPanel,
                    rootPanel->Resources->MergedDictionaries->GetAt(1),
                    L"color2_1",
                    Colors::Aqua);

                // item is added to existing 2nd level merged dictionary

                VerifyExisitingMergedDictionaryScenario(
                    rootPanel,
                    rootPanel->Resources->MergedDictionaries->GetAt(0)->MergedDictionaries->GetAt(0),
                    L"color3_0",
                    Colors::Aquamarine);

                VerifyExisitingMergedDictionaryScenario(
                    rootPanel,
                    rootPanel->Resources->MergedDictionaries->GetAt(1)->MergedDictionaries->GetAt(0),
                    L"color3_1",
                    Colors::Azure);

                // new merged dictionary with item is added

                VerifyNewMergedDictionaryScenario(
                    rootPanel,
                    rootPanel->Resources,
                    L"color4_0",
                    Colors::Beige);

                // 2nd/3rd level new merged dictionary with item is added

                VerifyNewMergedDictionaryScenario(
                    rootPanel,
                    rootPanel->Resources->MergedDictionaries->GetAt(0),
                    L"color5_0",
                    Colors::Black);

                VerifyNewMergedDictionaryScenario(
                    rootPanel,
                    rootPanel->Resources->MergedDictionaries->GetAt(1),
                    L"color5_1",
                    Colors::BlanchedAlmond);

                VerifyNewMergedDictionaryScenario(
                    rootPanel,
                    rootPanel->Resources->MergedDictionaries->GetAt(0)->MergedDictionaries->GetAt(0),
                    L"color5_2",
                    Colors::BurlyWood);

                VerifyNewMergedDictionaryScenario(
                    rootPanel,
                    rootPanel->Resources->MergedDictionaries->GetAt(1)->MergedDictionaries->GetAt(0),
                    L"color5_3",
                    Colors::CadetBlue);

                // item is added to existing theme dictionary

                VerifyKeyNotFoundCacheInvalidationScenario(
                    rootPanel,
                    L"color6_0",
                    Colors::Blue,
                    [&](FrameworkElement^ element, Platform::String^ resourceName, Color color)
                    {
                        safe_cast<ResourceDictionary^>(element->Resources->ThemeDictionaries->Lookup(L"Default"))->Insert(resourceName, ref new SolidColorBrush(color));
                    });

                // item is added to existing merged dictionary within theme dictionary

                VerifyKeyNotFoundCacheInvalidationScenario(
                    rootPanel,
                    L"color7_0",
                    Colors::BlueViolet,
                    [&](FrameworkElement^ element, Platform::String^ resourceName, Color color)
                    {
                        safe_cast<ResourceDictionary^>(element->Resources->ThemeDictionaries->Lookup(L"Default"))->MergedDictionaries->GetAt(0)->Insert(resourceName, ref new SolidColorBrush(color));
                    });

                // new theme dictionary with item is added

                VerifyKeyNotFoundCacheInvalidationScenario(
                    rootPanel,
                    L"color8_0",
                    Colors::Brown,
                    [&](FrameworkElement^ element, Platform::String^ resourceName, Color color)
                    {
                        auto themeDict = ref new ResourceDictionary();
                        themeDict->Insert(resourceName, ref new SolidColorBrush(color));
                        element->Resources->ThemeDictionaries->Insert(L"Dark", themeDict);
                    });

                // Switch theme where existing theme dictionary doesn't have a key, but the new one does.
                // This is not a valid scenario.

                rootPanel->RequestedTheme = ElementTheme::Light;
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void ResourceDictionaryBasicTests::VerifyXamlResourceReferenceTraceSimpleStaticResource()
        {
            Platform::String^ xamlString =
                L"<StackPanel  x:Name='m_stackPanel' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\n"
                L"   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\n"
                L"   xmlns:local='using:Tests.Native.External.Framework'>\n"
                L"  <StackPanel.Resources>\n"
                L"    <ResourceDictionary>\n"
                L"      <local:IdentityConverter x:Key='IdentityConverter' />\n"
                L"      <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />\n"
                L"    </ResourceDictionary>\n"
                L"  </StackPanel.Resources>\n"
                L"  <ToggleSwitch x:Name='m_switch' Width='100' />\n"
                L"  <Rectangle Height='50' Width='50' Fill='{StaticResource NoResourceWithThisKey}' />\n"
                L"</StackPanel>";

            // When tests are run in WPF hosting mode `app.xaml` has a different URI
            auto appXamlUri = IsInWPFHostingMode() ? L"ms-resource:///Files/app.xaml" : L"ms-appx:///App.xaml";

            const auto stringTemplate =
                L"Beginning search for resource with key 'NoResourceWithThisKey'.\n"
                L"  Searching dictionary '<anonymous dictionary>' for resource with key 'NoResourceWithThisKey'.\n"
                L"  Finished searching dictionary '<anonymous dictionary>'.\n"
                L"  Searching dictionary 'Framework-defined colors' for resource with key 'NoResourceWithThisKey'.\n"
                L"  Finished searching dictionary 'Framework-defined colors'.\n"
                L"  Searching dictionary 'Framework ThemeResources.xbf' for resource with key 'NoResourceWithThisKey'.\n"
                L"    Searching theme dictionary (active theme: 'Dark') for resource with key 'NoResourceWithThisKey'.\n"
                L"      Searching dictionary '<anonymous dictionary>' for resource with key 'NoResourceWithThisKey'.\n"
                L"      Finished searching dictionary '<anonymous dictionary>'.\n"
                L"    Finished searching theme dictionary (active theme: 'Dark').\n"
                L"  Finished searching dictionary 'Framework ThemeResources.xbf'.\n"
                L"  Searching dictionary '%s' for resource with key 'NoResourceWithThisKey'.\n"
                L"    Searching merged dictionary with index '0' for resource with key 'NoResourceWithThisKey'.\n"
                L"      Searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml' for resource with key 'NoResourceWithThisKey'.\n"
                L"        Searching theme dictionary (active theme: 'Dark') for resource with key 'NoResourceWithThisKey'.\n"
                L"          Searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml' for resource with key 'NoResourceWithThisKey'.\n"
                L"          Finished searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml'.\n"
                L"        Finished searching theme dictionary (active theme: 'Dark').\n"
                L"      Finished searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml'.\n"
                L"    Finished searching merged dictionary with index '0'.\n"
                L"  Finished searching dictionary '%s'.\n"
                L"Finished search for resource with key 'NoResourceWithThisKey'.";

            Platform::String^ expectedMessage = ref new Platform::String(WEX::Common::String().Format(
                stringTemplate,
                appXamlUri,
                L"themeresources",
                L"themeresources",
                L"themeresources",
                L"themeresources",
                appXamlUri));

            Platform::String^ expectedMessage2 = ref new Platform::String(WEX::Common::String().Format(
                stringTemplate,
                appXamlUri,
                L"themeresources_perf2026",
                L"themeresources_perf2026",
                L"themeresources_perf2026",
                L"themeresources_perf2026",
                appXamlUri));

            VerifyXamlResourceReferenceTraceHelper(xamlString, expectedMessage, expectedMessage2);
        }

        void ResourceDictionaryBasicTests::VerifyXamlResourceReferenceTraceSimpleThemeResource()
        {
            Platform::String^ xamlString =
                L"<StackPanel  x:Name='m_stackPanel' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\n"
                L"   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\n"
                L"   xmlns:local='using:Tests.Native.External.Framework'>\n"
                L"  <StackPanel.Resources>\n"
                L"    <ResourceDictionary>\n"
                L"      <ResourceDictionary.ThemeDictionaries>\n"
                L"        <ResourceDictionary x:Key='Light'>\n"
                L"          <SolidColorBrush x:Key='SomeBrush' Color='Blue' />\n"
                L"        </ResourceDictionary>\n"
                L"        <ResourceDictionary x:Key='Dark'>\n"
                L"          <SolidColorBrush x:Key='SomeBrush' Color='Gold' />\n"
                L"        </ResourceDictionary>\n"
                L"      </ResourceDictionary.ThemeDictionaries>\n"
                L"      <local:IdentityConverter x:Key='IdentityConverter' />\n"
                L"      <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />\n"
                L"    </ResourceDictionary>\n"
                L"  </StackPanel.Resources>\n"
                L"  <ToggleSwitch x:Name='m_switch' Width='100' />\n"
                L"  <Rectangle Height='50' Width='50' Fill='{ThemeResource NoResourceWithThisKey}' />\n"
                L"</StackPanel>";

            // When tests are run in WPF hosting mode `app.xaml` has a different URI
            auto appXamlUri = IsInWPFHostingMode() ? L"ms-resource:///Files/app.xaml" : L"ms-appx:///App.xaml";

            const auto stringTemplate =
                L"Beginning search for resource with key 'NoResourceWithThisKey'.\n"
                L"  Searching dictionary '<anonymous dictionary>' for resource with key 'NoResourceWithThisKey'.\n"
                L"    Searching theme dictionary (active theme: 'Dark') for resource with key 'NoResourceWithThisKey'.\n"
                L"      Searching dictionary '<anonymous dictionary>' for resource with key 'NoResourceWithThisKey'.\n"
                L"      Finished searching dictionary '<anonymous dictionary>'.\n"
                L"    Finished searching theme dictionary (active theme: 'Dark').\n"
                L"    Searching dictionary 'Framework-defined colors' for resource with key 'NoResourceWithThisKey'.\n"
                L"    Finished searching dictionary 'Framework-defined colors'.\n"
                L"    Searching dictionary 'Framework ThemeResources.xbf' for resource with key 'NoResourceWithThisKey'.\n"
                L"      Searching theme dictionary (active theme: 'Dark') for resource with key 'NoResourceWithThisKey'.\n"
                L"        Searching dictionary '<anonymous dictionary>' for resource with key 'NoResourceWithThisKey'.\n"
                L"        Finished searching dictionary '<anonymous dictionary>'.\n"
                L"      Finished searching theme dictionary (active theme: 'Dark').\n"
                L"    Finished searching dictionary 'Framework ThemeResources.xbf'.\n"
                L"  Finished searching dictionary '<anonymous dictionary>'.\n"
                L"  Searching dictionary '%s' for resource with key 'NoResourceWithThisKey'.\n"
                L"    Searching merged dictionary with index '0' for resource with key 'NoResourceWithThisKey'.\n"
                L"      Searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml' for resource with key 'NoResourceWithThisKey'.\n"
                L"        Searching theme dictionary (active theme: 'Dark') for resource with key 'NoResourceWithThisKey'.\n"
                L"          Searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml' for resource with key 'NoResourceWithThisKey'.\n"
                L"          Finished searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml'.\n"
                L"        Finished searching theme dictionary (active theme: 'Dark').\n"
                L"      Finished searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml'.\n"
                L"    Finished searching merged dictionary with index '0'.\n"
                L"  Finished searching dictionary '%s'.\n"
                L"Finished search for resource with key 'NoResourceWithThisKey'.";

            Platform::String^ expectedMessage = ref new Platform::String(WEX::Common::String().Format(
                stringTemplate,
                appXamlUri,
                L"themeresources",
                L"themeresources",
                L"themeresources",
                L"themeresources",
                appXamlUri));

            Platform::String^ expectedMessage2 = ref new Platform::String(WEX::Common::String().Format(
                stringTemplate,
                appXamlUri,
                L"themeresources_perf2026",
                L"themeresources_perf2026",
                L"themeresources_perf2026",
                L"themeresources_perf2026",
                appXamlUri));

            VerifyXamlResourceReferenceTraceHelper(xamlString, expectedMessage, expectedMessage2);
        }

        void ResourceDictionaryBasicTests::VerifyXamlResourceReferenceTraceResourceDictionarySource()
        {
            Platform::String^ xamlString =
                L"<StackPanel  x:Name='m_stackPanel' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\n"
                L"   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\n"
                L"   xmlns:local='using:Tests.Native.External.Framework'\n"
                L"   xmlns:rd='using:Tests.Native.External.Framework.ResourceDictionary'>\n"
                L"  <StackPanel.Resources>\n"
                L"    <ResourceDictionary>\n"
                L"      <ResourceDictionary.ThemeDictionaries>\n"
                L"        <ResourceDictionary x:Key='Light'>\n"
                L"          <SolidColorBrush x:Key='SomeBrush' Color='Blue' />\n"
                L"        </ResourceDictionary>\n"
                L"        <ResourceDictionary x:Key='Dark'>\n"
                L"          <SolidColorBrush x:Key='SomeBrush' Color='Gold' />\n"
                L"        </ResourceDictionary>\n"
                L"      </ResourceDictionary.ThemeDictionaries>\n"
                L"      <local:IdentityConverter x:Key='IdentityConverter' />\n"
                L"      <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />\n"
                L"    </ResourceDictionary>\n"
                L"  </StackPanel.Resources>\n"
                L"  <rd:UserControlWithUnresolveableStaticResource />\n"
                L"</StackPanel>";

            // When tests are run in WPF hosting mode `app.xaml` has a different URI
            auto appXamlUri = IsInWPFHostingMode() ? L"ms-resource:///Files/app.xaml" : L"ms-appx:///App.xaml";

            const auto stringTemplate =
                L"/UserControlWithUnresolveableStaticResource.xaml\n"
                L"Beginning search for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"  Searching dictionary 'UserControlWithUnresolveableStaticResource.ResourceDictionary.xaml' for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"  Finished searching dictionary 'UserControlWithUnresolveableStaticResource.ResourceDictionary.xaml'.\n"
                L"  Searching dictionary 'Framework-defined colors' for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"  Finished searching dictionary 'Framework-defined colors'.\n"
                L"  Searching dictionary 'Framework ThemeResources.xbf' for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"    Searching theme dictionary (active theme: 'Dark') for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"      Searching dictionary '<anonymous dictionary>' for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"      Finished searching dictionary '<anonymous dictionary>'.\n"
                L"    Finished searching theme dictionary (active theme: 'Dark').\n"
                L"  Finished searching dictionary 'Framework ThemeResources.xbf'.\n"
                L"  Searching dictionary '%s' for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"    Searching merged dictionary with index '0' for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"      Searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml' for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"        Searching theme dictionary (active theme: 'Dark') for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"          Searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml' for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.\n"
                L"          Finished searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml'.\n"
                L"        Finished searching theme dictionary (active theme: 'Dark').\n"
                L"      Finished searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/%s.xaml'.\n"
                L"    Finished searching merged dictionary with index '0'.\n"
                L"  Finished searching dictionary '%s'.\n"
                L"Finished search for resource with key '9D2842E4-FE97-4BEE-97F4-A9BFC846F2FD'.";

            Platform::String^ expectedMessage = ref new Platform::String(WEX::Common::String().Format(
                stringTemplate,
                appXamlUri,
                L"themeresources",
                L"themeresources",
                L"themeresources",
                L"themeresources",
                appXamlUri));

            Platform::String^ expectedMessage2 = ref new Platform::String(WEX::Common::String().Format(
                stringTemplate,
                appXamlUri,
                L"themeresources_perf2026",
                L"themeresources_perf2026",
                L"themeresources_perf2026",
                L"themeresources_perf2026",
                appXamlUri));

            // Because of how this test is setup (failure to resolve a resource key inside a control's constructor)
            // we have to swallow the exception that would come out of Application::LoadComponent() otherwise the
            // control will be leaked.
            VerifyXamlResourceReferenceTraceHelper(xamlString, expectedMessage, expectedMessage2, false);
        }

} } } } }
