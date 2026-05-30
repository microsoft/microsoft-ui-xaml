// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ResolveResourceTests.h"
#include "XamlDiagnosticsTestHelpers.h"
#include <CustomUserControl.h>
#include "CustomTypes.XamlTypeInfo.g.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <TestEvent.h>
#include "MainPage.xaml.h"
#include <CustomMetadataRegistrar.h>
#include "RuleTesterHelper.h"
#include <ThemeHelper.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Data;
using namespace ::Tests::Tools::XamlDiagnostics;
using namespace test_infra;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;

namespace shared_types = ::Tests::Tools::Shared;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        #pragma region Test Methods

        bool ResolveResourceTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool ResolveResourceTests::ClassCleanup()
        {
            return true;
        }

        bool ResolveResourceTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());
            return EnsureTapLoaded();
        }

        bool ResolveResourceTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            test_infra::TestServices::WindowHelper->VerifyTestCleanup();

            return true;
        }

        void ResolveResourceTests::ResolveThemeResource()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/MainPage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto ellipse = callback->GetElementByName(L"ellipse");

            unsigned int fillIndex = GetPropertyIndex(ellipse.Handle, L"Microsoft.UI.Xaml.Shapes.Shape.Fill");
            ResolveResource(ellipse.Handle, L"MyCustomThemedBrush", fillIndex, ResourceTypeTheme);

            RunOnUIThread([&]
            {
                auto ellipseObj = GetFromInstanceHandle<xaml_shapes::Ellipse>(ellipse.Handle);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Yellow, safe_cast<xaml_media::SolidColorBrush^>(ellipseObj->Fill)->Color);

                // Switch ellipses requested theme to light and verify we get the theme update
                ellipseObj->RequestedTheme = Microsoft::UI::Xaml::ElementTheme::Light;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                auto ellipseObj = GetFromInstanceHandle<xaml_shapes::Ellipse>(ellipse.Handle);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, safe_cast<xaml_media::SolidColorBrush^>(ellipseObj->Fill)->Color);
            });
        }

        void ResolveResourceTests::ResolveStaticResource()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);
            auto button = callback->GetElementByName(L"buttonNoResource");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            // Resolve resource to 'RedBrush' and make sure it's, well, red.
            ResolveResource(button.Handle, L"RedBrush", GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background"), ResourceTypeStatic);
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
        }

        void ResolveResourceTests::ResolveStaticResourceInThemeDictionary()
        {
            // We want to make sure that the resolved color matches the current application theme
            std::pair<Microsoft::UI::Xaml::ApplicationTheme, ::Windows::UI::Color> themes[] =
            { std::make_pair(Microsoft::UI::Xaml::ApplicationTheme::Light, Microsoft::UI::Colors::Black),
              std::make_pair(Microsoft::UI::Xaml::ApplicationTheme::Dark, Microsoft::UI::Colors::Yellow) };

            for (const auto& themeAndExpectedColor : themes)
            {
                ApplicationThemeOverrider themeHelper(themeAndExpectedColor.first);

                wrl::ComPtr<VisualTreeServiceCallback> callback;
                ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/MainPage.xaml");
                auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

                auto ellipse = callback->GetElementByName(L"ellipse");

                unsigned int fillIndex = GetPropertyIndex(ellipse.Handle, L"Microsoft.UI.Xaml.Shapes.Shape.Fill");
                ResolveResource(ellipse.Handle, L"MyCustomThemedBrush", fillIndex, ResourceTypeTheme);

                RunOnUIThread([&]
                {
                    auto ellipseObj = GetFromInstanceHandle<xaml_shapes::Ellipse>(ellipse.Handle);
                    VERIFY_ARE_EQUAL(themeAndExpectedColor.second, safe_cast<xaml_media::SolidColorBrush^>(ellipseObj->Fill)->Color);
                });
            }

        }

        void ResolveResourceTests::ResolveStaticResourceInMergedDictionary()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);
            auto button = callback->GetElementByName(L"buttonNoResource");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            auto backgroundIndex = GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");
            // Resolve resource to 'testOverwritten' and make sure it's green. testOverwritten is in a merged dictionary
            ResolveResource(button.Handle, L"testOverwritten", backgroundIndex, ResourceTypeStatic);
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
        }

        void ResolveResourceTests::ResolveThemeResourceInMergedDictionary()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);
            auto button = callback->GetElementByName(L"buttonNoResource");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            auto backgroundIndex = GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");
            // Resolve resource to 'testOverwritten' and make sure it's green. testOverwritten is in a merged dictionary
            ResolveResource(button.Handle, L"testThemed", backgroundIndex, ResourceTypeTheme);
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
            // Update the theme, make sure it's blue.
            RunOnUIThread([&]()
            {
                auto grid = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                grid->RequestedTheme = Microsoft::UI::Xaml::ElementTheme::Light;
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Yellow, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
        }

        void ResolveResourceTests::ValidateResourceResolutionLogic()
        {
            // This test validates the resource resolution logic for the framework. It's purpose is to ensure that if we change this behavior,
            // to make sure that xaml diagnostics isn't broken.
            // 1. UserControl has only a light ThemeDictionary
            // 2. MainPage has light and dark ThemeDictionaries
            // 3. MainPage has a UserControl, with a brush with a value to the ThemeResource
            // 4. MainPage has a Rectangle, with Fill to the value of the same ThemeResource
            // 5. Button template defined in CommonStyles.xaml has a StaticResource to a MyBrush defined in CommonStyles.xaml ResourceDictionary
            // 6. MyBrush is also defined in MainPage.Resources.
            // 7. Button on MainPage references button template in CommonStyles.xaml.

            // What we expect to happen:
            // Since the application theme is light, the brush in the user control collection will resolve to the color defined in the user control.
            // Since there is only a Light value provided, it will not update color's when theme changes. If the UserControl defined an empty "Dark" dictionary,
            // a theme change from Light->Dark would result in a theme change. The Rectangle's fill will resolve to the
            // theme resource on the MainPage, and will update with theme changes. The StaticResource for the button template will resolve the the MyBrush
            // declared in CommonStyles.xaml, not on MainPage.Resources.

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/MainPage.xaml");
            ApplicationThemeOverrider themeHelper(Microsoft::UI::Xaml::ApplicationTheme::Light);

            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            RunOnUIThread([&]
            {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);

                auto customControl = safe_cast<shared_types::CustomUserControl^>(page->FindName("customControl"));

                // Verify the rectangle in the user control has the color orange, which is defined in the CustomUserControl.xaml
                // page
                auto rectangle = safe_cast<xaml_shapes::Rectangle^>(customControl->CustomUserCollection->GetAt(0));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, safe_cast<xaml_media::SolidColorBrush^>(rectangle->Fill)->Color);

                rectangle = safe_cast<xaml_shapes::Rectangle^>(page->FindName("rectangle"));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, safe_cast<xaml_media::SolidColorBrush^>(rectangle->Fill)->Color);

                rectangle = safe_cast<xaml_shapes::Rectangle^>(page->FindName("styledRectangle"));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, safe_cast<xaml_media::SolidColorBrush^>(rectangle->Fill)->Color);

                LOG_OUTPUT(L"Verifying static resource resolved values");
                auto myBrushGrid = callback->GetElementByName(L"MyBrushButtonRoot");
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(myBrushGrid.Handle, L"Background", BaseValueSourceLocal));

                auto grid = safe_cast<xaml_controls::Grid^>(page->FindName("LayoutRoot"));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::AliceBlue, safe_cast<xaml_media::SolidColorBrush^>(grid->Background)->Color);

                // IconTextBlock is in a ControlTemplate for a ContentControl. MyBrush is defined locally in the ControlTemplate as well
                // as in the dictionary that contains the control template. Validate that this resolves to value in the dictionary, not the
                // one in the control template
                auto smileTextBlock = callback->GetElementByName(L"IconTextBlock");
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(smileTextBlock.Handle, L"Foreground", BaseValueSourceLocal));

                LOG_OUTPUT(L"Updating page theme to Dark");
                page->RequestedTheme = Microsoft::UI::Xaml::ElementTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);

                auto customControl = safe_cast<shared_types::CustomUserControl^>(page->FindName("customControl"));
                auto rectangle = safe_cast<xaml_shapes::Rectangle^>(customControl->CustomUserCollection->GetAt(0));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, safe_cast<xaml_media::SolidColorBrush^>(rectangle->Fill)->Color);

                rectangle = safe_cast<xaml_shapes::Rectangle^>(page->FindName("rectangle"));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Yellow, safe_cast<xaml_media::SolidColorBrush^>(rectangle->Fill)->Color);
            });
        }

        void ResolveResourceTests::ResolveThemeResourceInStyle()
        {
            ResolveResourceInStyle(ResourceTypeTheme);
        }

        void ResolveResourceTests::ResolveStaticResourceInStyle()
        {
            ResolveResourceInStyle(ResourceTypeStatic);
        }

        void ResolveResourceTests::ResolveResourceInStyle(ResourceType type)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/MainPage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            // We're going to crawl through the dictionaries to find the style in (what will end up being) a merged dictionary.
            InstanceHandle pageHandle = 0;
            RunOnUIThread([&]()
            {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                pageHandle = XamlDiagnosticsTestHelpers::GetInstanceHandleForObject(page);
            });

            auto resources = GetProperty(pageHandle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            VERIFY_IS_GREATER_THAN(mergedDictionaries.Count, 0u);

            // There is only one merged dictionary on the page. Let's get it!
            auto mergedDictionary = mergedDictionaries.Elements[0];
            auto styleHandle = GetDictionaryItem(std::stoll(mergedDictionary.Value), L"UnthemedEllipseFillStyle");

            // Get an elipse that was using this style, and make sure it's all groovy
            auto resolvedEllipse = callback->GetElementByName(L"unthemedEllipse");
            auto themedEllipse = callback->GetElementByName(L"themedEllipse");

            // Find the setter for this property. There is only one setter, so it's the first one
            auto setters = GetCollectionProperty(styleHandle, L"Microsoft.UI.Xaml.Style.Setters");
            auto fillSetter = std::stoll(setters.Elements[0].Value);

            ResolveResource(fillSetter, L"MyCustomThemedBrush", GetPropertyIndex(fillSetter, L"Microsoft.UI.Xaml.Setter.Value"), type);
            auto expectedColor = GetColorProperty(themedEllipse.Handle, L"Fill", BaseValueSourceStyle);
            VERIFY_ARE_EQUAL(expectedColor, GetColorProperty(resolvedEllipse.Handle, L"Fill", BaseValueSourceStyle));

            // Now update the theme and make sure it's expected
            RunOnUIThread([&]()
            {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                page->RequestedTheme = Microsoft::UI::Xaml::ElementTheme::Light;
            });

            TestServices::WindowHelper->WaitForIdle();

            // If the resource type is ResourceTypeTheme, then the resolvedEllipse should match the themedEllipse after the theme update.
            if (type == ResourceTypeTheme)
            {
                expectedColor = GetColorProperty(themedEllipse.Handle, L"Fill", BaseValueSourceStyle);
            }

            VERIFY_ARE_EQUAL(expectedColor, GetColorProperty(resolvedEllipse.Handle, L"Fill", BaseValueSourceStyle));
        }

        void ResolveResourceTests::ResolveThemeResourceOnSetter()
        {
            for (int i = 0; i < 2; ++i)
            {
                LOG_OUTPUT(L"Enable XBF: %d", i);
                Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, !!i);
                ResolveResourceOnSetter(ResourceTypeTheme);
            }
        }

        void ResolveResourceTests::ResolveStaticResourceOnSetter()
        {
            for (int i = 0; i < 2; ++i)
            {
                LOG_OUTPUT(L"Enable XBF: %d", i);
                Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, !!i);
                ResolveResourceOnSetter(ResourceTypeStatic);
            }
        }

        void ResolveResourceTests::ResolveResourceOnSetter(ResourceType type)
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <Color x:Key='c1'>Pink</Color>\r\n"
                L"    <Color x:Key='c2'>Blue</Color>\r\n"
                L"    <Style TargetType='Rectangle' x:Key='rectStyle'>\r\n"
                L"      <Setter Property='Fill' Value='Red' />\r\n"
                L"    </Style>\r\n"
                L"    <Style TargetType='TextBox'>\r\n"
                L"      <Setter Property='Foreground' Value='Orange'/>\r\n"
                L"    </Style>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Button x:Name='button' />\r\n"
                L"  <Ellipse x:Name='elli' />\r\n"
                L"  <TextBox x:Name='text' />\r\n"
                L"  <Rectangle x:Name='rect' Style='{StaticResource rectStyle}'/>\r\n"
                L"</Grid>\r\n");
                wrl::ComPtr<VisualTreeServiceCallback> callback;
                auto cleanup = m_connectionHelper->Advise(content, callback);

                auto root = callback->GetElementByName(L"root");
                unsigned int setterValueIndex = 0;
                // Add two styles, one implicit and one regular
                LOG_OUTPUT(L"Resolving on button style created at run-time");
                {
                    auto newButtonStyle = CreateStyle(L"Microsoft.UI.Xaml.Controls.Button");
                    auto buttonStyleSetter = CreateSetterWithProperty(0, L"Microsoft.UI.Xaml.Controls.Control.Background");
                    setterValueIndex = GetPropertyIndex(buttonStyleSetter, L"Microsoft.UI.Xaml.Setter.Value");
                    AddSetterToStyle(newButtonStyle, buttonStyleSetter);

                    AddItemToElementResources(root, L"NewButtonStyle", newButtonStyle);

                    LOG_OUTPUT(L"Resolving on button style setter");
                    ResolveResource(buttonStyleSetter, L"c1", setterValueIndex, type);
                    auto button = callback->GetElementByName(L"button");
                    ResolveResource(button.Handle, L"NewButtonStyle", GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Style"), ResourceTypeStatic);
                    // Validate the style was applied correctly
                    VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceStyle));
                }


                // Do the same thing with the ellipse, but this will be an implicit style
                LOG_OUTPUT(L"Resolving on implicit ellipse style created at run-time");
                {
                    auto newEllipseStyle = CreateStyle(L"Microsoft.UI.Xaml.Shapes.Ellipse");
                    auto ellipseStyleSetter = CreateSetterWithProperty(0, L"Microsoft.UI.Xaml.Shapes.Shape.Fill");
                    AddSetterToStyle(newEllipseStyle, ellipseStyleSetter);
                    AddImplicitStyleToElementResources(root, newEllipseStyle);
                    ResolveResource(ellipseStyleSetter, L"c2",setterValueIndex, type);
                    // Validate the style was applied correctly
                    auto elli = callback->GetElementByName(L"elli");
                    VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(elli.Handle, L"Fill", BaseValueSourceStyle));
                }

                LOG_OUTPUT(L"Resolving on rectangle style created at parse time");
                {
                    auto rectStyle = GetItemFromElementResources(root, L"rectStyle");
                    auto setters = GetCollectionProperty(rectStyle, L"Microsoft.UI.Xaml.Style.Setters");
                    auto fillSetter = ih_cast(setters.Elements[0].Value);
                    ResolveResource(fillSetter, L"c1", setterValueIndex, type);
                    auto rect = callback->GetElementByName(L"rect");
                    VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(rect.Handle, L"Fill", BaseValueSourceStyle));
                }

                LOG_OUTPUT(L"Resolving on implicit text box style created at parse time");
                {
                    auto textBoxStyle = GetImplicitStyleFromElementResources(root, L"Microsoft.UI.Xaml.Controls.TextBox");
                    auto setters = GetCollectionProperty(textBoxStyle, L"Microsoft.UI.Xaml.Style.Setters");
                    auto foregroundSetter = ih_cast(setters.Elements[0].Value);
                    ResolveResource(foregroundSetter, L"c2", setterValueIndex, type);
                    auto text = callback->GetElementByName(L"text");
                    VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(text.Handle, L"Foreground", BaseValueSourceStyle));
                }
        }

        void ResolveResourceTests::CorrectlyResolvesStaticResourceInTemplate()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/MainPage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);
            auto root = callback->GetElementByName(L"LayoutRoot");

            auto children = GetCollectionProperty(root.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Children");
            VERIFY_ARE_EQUAL(children.Elements[0].MetadataBits & MetadataBit::IsValueHandle, MetadataBit::IsValueHandle);
            auto stackPanelHandle = std::stoll(children.Elements[0].Value);

            auto stackPanelChildren = GetCollectionProperty(stackPanelHandle, L"Microsoft.UI.Xaml.Controls.Panel.Children");

            // Create a button and add it to the stack panel.
            auto newButton = CreateControlWithBackground(L"Button", L"Red", L"transparentButton");

            VERIFY_SUCCEEDED(m_tap->AddChild(stackPanelChildren.Handle, newButton, stackPanelChildren.Count));

            // Now, add the static resource reference to the template
            ResolveResource(newButton, L"BlackTemplatedButtonStyle", GetPropertyIndex(newButton, L"Microsoft.UI.Xaml.FrameworkElement.Style"), ResourceTypeStatic);
            VERIFY_ARE_EQUAL(0u, callback->GetErrorCount(newButton), L"No message was actually sent, so verify it's empty");

            auto blackGrid  = callback->GetElementByName(L"BlackButtonRoot");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(blackGrid.Handle, L"Background", BaseValueSourceLocal));

            ResolveResource(blackGrid.Handle, L"BackgroundBrush", GetPropertyIndex(blackGrid.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Background"), ResourceTypeStatic);
            auto lastError = callback->GetLastError(blackGrid.Handle);
            // Now, try to resolve to BackgroundBrush, which is defined in a parent's resource dictionary. This should not succeed.
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, lastError.State);
            VERIFY_IS_TRUE(lastError.Message.compare(L"Background:Microsoft.UI.Xaml.Controls.Panel") == 0);

            // Now, call ResolveResource on the ButtonRoot background, this should resolve to Blue, not AliceBlue. Validate that the error was resolved.
            ResolveResource(blackGrid.Handle, L"MyBrush", GetPropertyIndex(blackGrid.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Background"), ResourceTypeStatic);
            VERIFY_ARE_EQUAL(0u, callback->GetErrorCount(blackGrid.Handle)),

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(blackGrid.Handle, L"Background", BaseValueSourceLocal));
        }

        void ResolveResourceTests::TestNonDPResolveResource()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControl.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto customUserControl = callback->GetElementByName(L"userControl1");

            unsigned int nonDPIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(customUserControl.Handle, L"Tests.Tools.Shared.CustomUserControl.CustomBrushNonDP", &nonDPIndex));
            VERIFY_ARE_NOT_EQUAL(nonDPIndex, 0u);

            // Verify we can get the non-DP brush and its initial color is correct
            // given the markup (Orange)
            wrl::ComPtr<IInspectable> customBrushVal;
            InstanceHandle customBrushValHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(customUserControl.Handle, nonDPIndex, &customBrushValHandle));
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(customBrushValHandle, &customBrushVal));

            RunOnUIThread([&]()
            {
                auto brush = safe_cast<SolidColorBrush^>(reinterpret_cast<Platform::Object^>(customBrushVal.Get()));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, brush->Color);
            });

            // Resolve to the blue brush in the resource dictionary
            ResolveResource(customUserControl.Handle, L"blueBrush", nonDPIndex, ResourceTypeStatic);

            // Get the new brush from the non-DP property and verify its color is appropriately Blue
            VERIFY_SUCCEEDED(m_tap->GetProperty(customUserControl.Handle, nonDPIndex, &customBrushValHandle));
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(customBrushValHandle, &customBrushVal));

            RunOnUIThread([&]()
            {
                auto brush = safe_cast<SolidColorBrush^>(reinterpret_cast<Platform::Object^>(customBrushVal.Get()));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, brush->Color);
            });

            // Try resolving to an invalid resource
            VERIFY_IS_FALSE(callback->HasError());
            ResolveResource(customUserControl.Handle, L"purpleBrush", nonDPIndex, ResourceTypeStatic);

            // Verify we couldn't resolve and logged the correct error
            VERIFY_IS_TRUE(callback->HasError());
            auto error = callback->GetLastError(customUserControl.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);

            // Insert a new resource which will resolve the error
            auto root = callback->GetElementByName(L"root");
            InstanceHandle purpleBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.SolidColorBrush", L"Purple");
            AddItemToElementResources(root, L"purpleBrush", purpleBrush);
            VERIFY_IS_FALSE(callback->HasError());

            // Get the new brush from the non-DP property and verify its color is appropriately Purple
            VERIFY_SUCCEEDED(m_tap->GetProperty(customUserControl.Handle, nonDPIndex, &customBrushValHandle));
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(customBrushValHandle, &customBrushVal));

            RunOnUIThread([&]()
            {
                auto brush = safe_cast<SolidColorBrush^>(reinterpret_cast<Platform::Object^>(customBrushVal.Get()));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Purple, brush->Color);
            });
        }

        void ResolveResourceTests::VerifyResolveResourceOnDictioaryItemCorrectlyUpdatesReferences()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);

            // Get the resources of the grid and resolve the resource to the brush
            auto parentGrid = callback->GetElementByName(L"parent");
            auto parentResources = GetProperty(parentGrid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto brush = GetDictionaryItem(parentResources, L"RedBrush");

            // Resolve the 'RedBrush' to the 'MyColor' (it's blue, I know quite silly). Make sure that redBrushButton and redBrushEllipse
            // both are correctly updated.
            auto colorIndex = GetPropertyIndex(brush, L"Microsoft.UI.Xaml.Media.SolidColorBrush.Color");

            ResolveResource(brush, L"MyColor", colorIndex, ResourceTypeStatic);

            auto redButton = callback->GetElementByName(L"redBrushButton");
            auto redEllipse = callback->GetElementByName(L"redBrushEllipse");

            // Verify they are now blue
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(redButton.Handle, L"Background", BaseValueSourceLocal));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(redEllipse.Handle, L"Fill", BaseValueSourceLocal));
        }

        void ResolveResourceTests::VerifyResolveStaticResourceInVisualState()
        {
            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/StaticResourcePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            // 1. Make sure bananas1 is Blue
            auto bananas1 = callback->GetElementByName(L"bananas1");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(bananas1.Handle, L"Background", BaseValueSource::Animation));

            // 2. findName on DeferredBananas, should return null since the static resource in the style can't be resolved.
            RunOnUIThread([&]
            {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                auto deferredBananas = page->FindName("deferredBananas");
                VERIFY_IS_NULL(deferredBananas);
            });

            // 3. ResolveResource on setter that sets value bananas2 and make sure it's blue
            auto root = callback->GetElementByName(L"LayoutRoot");
            auto visualGroupCollection = GetVisualStateGroupsForElement(root.Handle);

            // Get the actual visual state groups.
            auto visualStateCollection = GetStatesInVisualStateGroup(std::stoll(visualGroupCollection.Elements[0].Value));
            auto setters = GetVisualStateProperty(std::stoll(visualStateCollection.Elements[0].Value), L"Setters");
            VERIFY_ARE_EQUAL(2u, setters.Count);

            auto bananas2 = callback->GetElementByName(L"bananas2");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(bananas2.Handle, L"Background", BaseValueSource::Animation));

            InstanceHandle setterHandle = std::stoll(setters.Elements[1].Value);
            ResolveResource(setterHandle, L"Foo", GetPropertyIndex(setterHandle, L"Microsoft.UI.Xaml.Setter.Value"), ResourceTypeStatic);

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(bananas2.Handle, L"Background", BaseValueSource::Animation));
        }

        void ResolveResourceTests::ResolveStaticResourceStyleInParentDictionary()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/StaticResourcePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            // Get the dictionary for the StackPanel and get the style from the dictionary
            auto stack = callback->GetElementByName(L"stack");
            auto stackResources = GetProperty(stack.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto rectStyle = GetDictionaryItem(stackResources, L"RectStyle");

            // Find the setter for the Rectangle.Width property. There is only one setter, so it's the first one
            auto setters = GetCollectionProperty(rectStyle, L"Microsoft.UI.Xaml.Style.Setters");
            auto widthSetter = std::stoll(setters.Elements[0].Value);

            auto rect = callback->GetElementByName(L"rect");

            auto width = GetPropertyChainValue(rect.Handle, L"Width", BaseValueSourceStyle);
            VERIFY_ARE_EQUAL(200, std::stoi(width.Value));

            // Resolve the value to "RectWidthSize" and verify the width changed
            ResolveResource(widthSetter, L"RectWidthSize", GetPropertyIndex(widthSetter, L"Microsoft.UI.Xaml.Setter.Value"), ResourceTypeStatic);

            width = GetPropertyChainValue(rect.Handle, L"Width", BaseValueSourceStyle);
            VERIFY_ARE_EQUAL(300, std::stoi(width.Value));
        }

        void ResolveResourceTests::CanResolveAppAndSystemResources()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);
            auto button = callback->GetElementByName(L"buttonOverwritten");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            ResolveResource(button.Handle, L"testApp", GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background"), ResourceTypeStatic);

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            Platform::String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='{StaticResource ButtonBackground}' />";

            auto expectedColor = Microsoft::UI::Colors::Transparent;
            RunOnUIThread([&]
            {
                auto button = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(xamlString));
                expectedColor = safe_cast<xaml_media::SolidColorBrush^>(button->Background)->Color;
            });

            ResolveResource(button.Handle, L"ButtonBackground", GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background"), ResourceTypeStatic);

            VERIFY_ARE_EQUAL(expectedColor, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
        }

        void ResolveResourceTests::ResolveResourceFromElementStyleProperty()
        {
            auto content = ref new Platform::String(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
            L"  <Grid.Resources>\r\n"
            L"    <Color x:Key='c1'>Pink</Color>\r\n"
            L"    <Color x:Key='c2'>Blue</Color>\r\n"
            L"  </Grid.Resources>\r\n"
            L"  <Button Content='Hello World' Click='Button_Click' x:Name='button'>\r\n"
            L"    <Button.Background>\r\n"
            L"      <SolidColorBrush Color='{StaticResource c1}' x:Name='buttonbackground'>\r\n"
            L"      </SolidColorBrush>\r\n"
            L"    </Button.Background>\r\n"
            L"  </Button>\r\n"
            L"  <Button x:Name='button2' Background='{Binding ElementName=button, Path=Background}'/>\r\n"
            L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto button = callback->GetElementByName(L"button");
            auto button2 = callback->GetElementByName(L"button2");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button2.Handle, L"Background", BaseValueSourceLocal));

            auto backgroundBrush = GetProperty(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");
            ResolveResource(backgroundBrush, L"c2", GetPropertyIndex(backgroundBrush, L"Microsoft.UI.Xaml.Media.SolidColorBrush.Color"), ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(button2.Handle, L"Background", BaseValueSourceLocal));

            ResolveResource(backgroundBrush, L"c1", GetPropertyIndex(backgroundBrush, L"Microsoft.UI.Xaml.Media.SolidColorBrush.Color"), ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button2.Handle, L"Background", BaseValueSourceLocal));
        }

        void ResolveResourceTests::ResolveResourceFromElementStylePropertyInStyle()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      x:Name='parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <Color x:Key='c1'>Pink</Color>\r\n"
                L"    <Color x:Key='c2'>Blue</Color>\r\n"
                L"    <x:Double x:Key='d1'>10</x:Double>\r\n"
                L"    <x:Double x:Key='d2'>20</x:Double>\r\n"
                L"    <Style TargetType='Button' x:Key='b'>\r\n"
                L"       <Setter Property='Background'>\r\n"
                L"         <Setter.Value>\r\n"
                L"           <SolidColorBrush Color = '{StaticResource c1}'/>\r\n"
                L"         </Setter.Value>\r\n"
                L"       </Setter>"
                L"       <Setter Property='Width'>\r\n"
                L"         <Setter.Value>\r\n"
                L"           <StaticResource ResourceKey='d1'/>\r\n"
                L"         </Setter.Value>\r\n"
                L"       </Setter>"
                L"    </Style>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Button Style='{StaticResource b}' Content='Hello World' Click='Button_Click' x:Name='button' />\r\n"
                L"  <Button x:Name='button2' Background='{Binding ElementName=button, Path=Background}' Width='{Binding ElementName=button, Path=Width}'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto button = callback->GetElementByName(L"button");
            auto button2 = callback->GetElementByName(L"button2");

            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto style = GetDictionaryItem(resources, L"b");
            auto setters = GetCollectionProperty(style, L"Microsoft.UI.Xaml.Style.Setters");
            auto setterHandle = ih_cast(setters.Elements[0].Value);
            auto brush = GetProperty(setterHandle, L"Microsoft.UI.Xaml.Setter.Value");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceStyle));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button2.Handle, L"Background", BaseValueSourceLocal));

            ResolveResource(brush, L"c2", GetPropertyIndex(brush, L"Microsoft.UI.Xaml.Media.SolidColorBrush.Color"), ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(button.Handle, L"Background", BaseValueSourceStyle));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(button2.Handle, L"Background", BaseValueSourceLocal));

            ResolveResource(brush, L"c1", GetPropertyIndex(brush, L"Microsoft.UI.Xaml.Media.SolidColorBrush.Color"), ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());
            VERIFY_ARE_EQUAL(10, (int)ih_cast<Platform::IBox<double>>(GetProperty(button.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Width"))->Value);
            VERIFY_ARE_EQUAL(10, (int)ih_cast<Platform::IBox<double>>(GetProperty(button2.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Width"))->Value);

            LOG_OUTPUT(L"Verifying primitives resolve correctly");
            setterHandle = ih_cast(setters.Elements[1].Value);

            ResolveResource(setterHandle, L"d2", GetPropertyIndex(setterHandle, L"Microsoft.UI.Xaml.Setter.Value"), ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());
            VERIFY_ARE_EQUAL(20, (int)ih_cast<Platform::IBox<double>>(GetProperty(button.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Width"))->Value);
            VERIFY_ARE_EQUAL(20, (int)ih_cast<Platform::IBox<double>>(GetProperty(button2.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Width"))->Value);

            ResolveResource(setterHandle, L"d1", GetPropertyIndex(setterHandle, L"Microsoft.UI.Xaml.Setter.Value"), ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());
            VERIFY_ARE_EQUAL(10, (int)ih_cast<Platform::IBox<double>>(GetProperty(button.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Width"))->Value);
            VERIFY_ARE_EQUAL(10, (int)ih_cast<Platform::IBox<double>>(GetProperty(button2.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Width"))->Value);
        }

        void ResolveResourceTests::CanResolveEnumTypes()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <TextWrapping x:Key='w1'>Wrap</TextWrapping>\r\n"
                L"    <TextWrapping x:Key='w2'>Wrap</TextWrapping>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <TextBox Text='Hello World' TextWrapping='NoWrap' x:Name='box'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto box = callback->GetElementByName(L"box");
            VERIFY_ARE_EQUAL(xaml::TextWrapping::NoWrap, (xaml::TextWrapping)ih_cast<Platform::IBox<xaml::TextWrapping>>(GetProperty(box.Handle, L"Microsoft.UI.Xaml.Controls.TextBox.TextWrapping"))->Value);

            // Resolve to w1 and make sure it changes
            ResolveResource(box.Handle, L"w1", GetPropertyIndex(box.Handle, L"Microsoft.UI.Xaml.Controls.TextBox.TextWrapping"), ResourceTypeStatic);

            VERIFY_ARE_EQUAL(xaml::TextWrapping::Wrap, (xaml::TextWrapping)ih_cast<Platform::IBox<xaml::TextWrapping>>(GetProperty(box.Handle, L"Microsoft.UI.Xaml.Controls.TextBox.TextWrapping"))->Value);

            // Use replace resource to change w2 to NoWrap and make sure resolve still works
            auto root = callback->GetElementByName(L"root");
            auto resources = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto nowrap = CreateInstance(L"Microsoft.UI.Xaml.TextWrapping", L"NoWrap");
            ReplaceResource(resources, nowrap, L"w2");
            ResolveResource(box.Handle, L"w2", GetPropertyIndex(box.Handle, L"Microsoft.UI.Xaml.Controls.TextBox.TextWrapping"), ResourceTypeStatic);
            VERIFY_ARE_EQUAL(xaml::TextWrapping::NoWrap, (xaml::TextWrapping)ih_cast<Platform::IBox<xaml::TextWrapping>>(GetProperty(box.Handle, L"Microsoft.UI.Xaml.Controls.TextBox.TextWrapping"))->Value);
        }

        void ResolveResourceTests::DontCrashResolvingNonDO()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <local:IconConverter x:Key='IconConverter'></local:IconConverter>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Button Content='{StaticResource IconConverter}' x:Name='button'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto button = callback->GetElementByName(L"button");

            ResolveResource(button.Handle, L"IconConverter", GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.ContentControl.Content"), ResourceTypeStatic);

            // We should just display the result of the ToString() method for non-UIElement types
            auto value = GetProperty<::Tests::Tools::Shared::IconConverter>(button.Handle, L"Microsoft.UI.Xaml.Controls.ContentControl.Content");
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Tests.Tools.Shared.IconConverter"), value->ToString());
        }

        void ResolveResourceTests::VerifyResolveFallbackValueUpdatesTarget()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <x:String x:Key='myText'>Hello World</x:String>\r\n"
                L"    <x:String x:Key='myText2'>Goodbye</x:String>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <TextBlock x:Name='block' Text='{Binding DoesNotResolve}'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto block = callback->GetElementByName(L"block");
            auto binding = GetPropertyChainValue(block.Handle, L"Text", BaseValueSourceLocal);
            const auto fallbackValueIndex = GetPropertyIndex(ih_cast(binding.Value), L"Microsoft.UI.Xaml.Data.Binding.FallbackValue");
            ResolveResource(ih_cast(binding.Value), L"myText", fallbackValueIndex, ResourceTypeStatic);

            auto textBlock = ih_cast<xaml_controls::TextBlock>(block.Handle);
            RunOnUIThread([&textBlock]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"Hello World"), textBlock->Text);
            });

            // Now test resolving theme resource
            ResolveResource(ih_cast(binding.Value), L"myText2", fallbackValueIndex, ResourceTypeTheme);

            RunOnUIThread([&textBlock]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"Goodbye"), textBlock->Text);
            });
        }

        void ResolveResourceTests::VerifyResolveTargetNullValueUpdatesTarget()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <x:String x:Key='myText'>Hello World</x:String>\r\n"
                L"    <x:String x:Key='myText2'>Goodbye</x:String>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <TextBlock x:Name='block' Text='{Binding CustomUnknown, ElementName=custom}'/>\r\n"
                L"  <local:CustomUserControl x:Name='custom' />\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto block = callback->GetElementByName(L"block");

            // Mimick VS and create a new binding
            auto newBinding = CreateBinding(L"custom", L"CustomUnknown");
            const auto targetNullValueIndex = GetPropertyIndex(newBinding, L"Microsoft.UI.Xaml.Data.Binding.TargetNullValue");
            SetProperty(block.Handle, newBinding, L"Microsoft.UI.Xaml.Controls.TextBlock.Text");

            ResolveResource(newBinding, L"myText", targetNullValueIndex, ResourceTypeStatic);

            auto textBlock = ih_cast<xaml_controls::TextBlock>(block.Handle);
            RunOnUIThread([&textBlock]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"Hello World"), textBlock->Text);
            });

            // Now test resolving theme resource
            ResolveResource(newBinding, L"myText2", targetNullValueIndex, ResourceTypeTheme);

            RunOnUIThread([&textBlock]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"Goodbye"), textBlock->Text);
            });
        }

        void ResolveResourceTests::VerifyResolveConverterUpdatesTarget()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <local:IconConverter x:Key='IconConverter'></local:IconConverter>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <TextBlock x:Name='block' Text='Smile'/>\r\n"
                L"  <Button x:Name='button' Content='{Binding Text, ElementName=block}' />\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto button = callback->GetElementByName(L"button");

            auto buttonObj = ih_cast<xaml_controls::Button>(button.Handle);
            RunOnUIThread([&buttonObj]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"Smile"), safe_cast<Platform::String^>(buttonObj->Content));
            });

            // Mimick VS and create a new binding
            auto newBinding = CreateBinding(L"block", L"Text");
            const auto converterIndex = GetPropertyIndex(newBinding, L"Microsoft.UI.Xaml.Data.Binding.Converter");
            SetProperty(button.Handle, newBinding, L"Microsoft.UI.Xaml.Controls.ContentControl.Content");

            ResolveResource(newBinding, L"IconConverter", converterIndex, ResourceTypeStatic);

            RunOnUIThread([&buttonObj]{
                auto converter = ref new ::Tests::Tools::Shared::IconConverter();
                auto convertedText = safe_cast<Platform::String^>(converter->Convert(ref new Platform::String(L"Smile"), wxaml_interop::TypeName{}, nullptr, nullptr));
                VERIFY_ARE_EQUAL(convertedText, safe_cast<Platform::String^>(buttonObj->Content));
            });
        }

        void ResolveResourceTests::VerifyResolveNestedStyle()
        {
            auto content = ref new Platform::String(
                L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'>\r\n"
                L"<Grid Background='{ThemeResource ApplicationPageBackgroundThemeBrush}' x:Name='root'>\r\n"
                L" <Grid.Resources>\r\n"
                L"  <SolidColorBrush x:Key='B1' Color='Blue'/>\r\n"
                L"  <Style TargetType='ListView'>\r\n"
                L"   <Setter Property='ItemContainerStyle'>\r\n"
                L"    <Setter.Value>\r\n"
                L"     <Style TargetType='ListViewItem'>\r\n"
                L"      <Setter Property='Foreground' Value='Red'/>\r\n"
                L"     </Style>\r\n"
                L"    </Setter.Value>\r\n"
                L"   </Setter>\r\n"
                L"  </Style>\r\n"
                L" </Grid.Resources>\r\n"
                L" <ListView Width='200' Height='300'>\r\n"
                L"  <ListViewItem Content='One' x:Name='one'/>\r\n"
                L"  <ListViewItem Content='Two' x:Name='two'/>\r\n"
                L" </ListView>\r\n"
                L"</Grid>\r\n"
                L"</Page>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto root = callback->GetElementByName(L"root");
            auto listViewImplicitStyle = GetImplicitStyleFromElementResources(root, L"Microsoft.UI.Xaml.Controls.ListView");
            auto setter = GetCollectionItem(listViewImplicitStyle, L"Microsoft.UI.Xaml.Style.Setters", 0);

            auto listViewItemImplicitStyle = GetProperty(setter, L"Microsoft.UI.Xaml.Setter.Value");
            auto setterToResolve = GetCollectionItem(listViewItemImplicitStyle, L"Microsoft.UI.Xaml.Style.Setters", 0);
            auto propertyIndex = GetPropertyIndex(setterToResolve, L"Microsoft.UI.Xaml.Setter.Value");

            LOG_OUTPUT(L"Resolving setter added from markup");
            ResolveResource(setterToResolve, L"B1", propertyIndex, ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            auto one = callback->GetElementByName(L"one");
            auto asOneListViewItem = ih_cast<ListViewItem>(one.Handle);
            RunOnUIThread([&] {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<SolidColorBrush^>(asOneListViewItem->Foreground)->Color);
            });
            LOG_OUTPUT(L"Remove style and validate the ListViewItem updates");
            RemoveImplicitStyleFromElementResources(root, L"Microsoft.UI.Xaml.Controls.ListView");
            RunOnUIThread([&] {
                VERIFY_ARE_NOT_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<SolidColorBrush^>(asOneListViewItem->Foreground)->Color);
            });

            m_tap->UnregisterInstance(listViewImplicitStyle);
            listViewImplicitStyle = CreateStyle(L"Microsoft.UI.Xaml.Controls.ListView");
            AddImplicitStyleToElementResources(root, listViewImplicitStyle);

            listViewItemImplicitStyle = CreateStyle(L"Microsoft.UI.Xaml.Controls.ListViewItem");

            auto listViewItemStylePropertySetter = CreateSetterWithProperty(listViewItemImplicitStyle, L"Microsoft.UI.Xaml.Controls.ItemsControl.ItemContainerStyle");
            AddSetterToStyle(listViewImplicitStyle, listViewItemStylePropertySetter);

            setterToResolve = CreateSetterWithProperty(0, L"Microsoft.UI.Xaml.Controls.Control.Foreground");
            AddSetterToStyle(listViewItemImplicitStyle, setterToResolve);
            LOG_OUTPUT(L"Resolving setter added at runtime");
            ResolveResource(setterToResolve, L"B1", propertyIndex, ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());
            RunOnUIThread([&] {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<SolidColorBrush^>(asOneListViewItem->Foreground)->Color);
            });
        }

        void ResolveResourceTests::VerifyResolveCustomPropertyWithCustomType()
        {
            // This test is a little peculiar in that we don't use the regular advise method. This is because when the connection
            // helper loads the visual tree, it calls VisualTreeServiceCallback::ValidateTreeState, which fails since the ResourceDictionary
            // calls enter on it's children. In this very uncommon case, the child is a UIElement, so while we don't signal that element (since TryGetVisualTreeParent returns false),
            // any children of the UIElement do get added. This has always been the case, and doesn't seem to provide any issues to VS. Presumably, since the UserControl
            // never goes through SignalMutation, VS doesn't display the orphaned VisualTree as it's waiting for a parentless mutation event to occur.
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <local:CustomUserControl x:Key='Resource'></local:CustomUserControl>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <local:CustomUserControl x:Name='Local1'></local:CustomUserControl>\r\n"
                L"  <local:CustomUserControl x:Name='Local2'></local:CustomUserControl>\r\n"
                L"  <local:CustomUserControl x:Name='Local3' CustomChild='{StaticResource Resource}'></local:CustomUserControl>\r\n"
                L"</Grid>\r\n");
            auto callback = m_connectionHelper->Advise();

            TestCleanupWrapper cleanup([&] {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
                m_connectionHelper->OnTestComplete(callback);
            });

            RunOnUIThread([&] {
                auto rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(content));
                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto local1 = callback->GetElementByName(L"Local1");
            const uint32_t index = GetPropertyIndex(local1.Handle, L"Tests.Tools.Shared.CustomUserControl.CustomChild");
            ResolveResource(local1.Handle, L"Resource", index, ResourceTypeStatic );

            VERIFY_IS_FALSE(callback->HasError());
            auto local2 = callback->GetElementByName(L"Local2");
            ResolveResource(local2.Handle, L"Resource", index, ResourceTypeStatic );
            VERIFY_IS_FALSE(callback->HasError());

            auto local3 = callback->GetElementByName(L"Local3");
            RunOnUIThread([&]{
                auto local1_obj = ih_cast<::Tests::Tools::Shared::CustomUserControl>(local1.Handle);
                auto local2_obj = ih_cast<::Tests::Tools::Shared::CustomUserControl>(local2.Handle);
                VERIFY_ARE_EQUAL(local1_obj->CustomChild, local2_obj->CustomChild);

                auto local3_obj = ih_cast<::Tests::Tools::Shared::CustomUserControl>(local3.Handle);
                VERIFY_ARE_EQUAL(local3_obj->CustomChild, local2_obj->CustomChild);
            });
        }

        void ResolveResourceTests::VerifyResolveDataTemplateSelector()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/DataTemplateSelectorPage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto conMan = callback->GetElementByName(L"ConMan");

            const uint32_t selectorIndex = GetPropertyIndex(conMan.Handle, L"Microsoft.UI.Xaml.Controls.ContentControl.ContentTemplateSelector");
            ResolveResource(conMan.Handle, L"foo", selectorIndex, ResourceTypeStatic );
            VERIFY_IS_FALSE(callback->HasError());

            // Add a new TestDataTemplateSelector and do the business. Adding the item with the key 'foo' to the Root should cause
            // this item to be resolved
            auto selector = CreateInstance(L"Tests.Tools.Shared.TestDataTemplateSelector");
            auto root = callback->GetElementByName(L"Root");
            AddItemToElementResources(root, L"foo", selector);
            VERIFY_IS_FALSE(callback->HasError());

            // Remove foo and change to 'bar'
            RemoveItemFromElementResources(root, L"foo");
            VERIFY_IS_FALSE(callback->HasError());
            auto page = callback->GetParent(root.Handle);
            RemoveItemFromElementResources(page, L"foo");
            VERIFY_IS_TRUE(callback->HasError());
            auto error = callback->GetLastError(conMan.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);

            selector = CreateInstance(L"Tests.Tools.Shared.TestDataTemplateSelector");
            AddItemToElementResources(page, L"bar", selector);

            // This shouldn't actually change anything
            VERIFY_IS_TRUE(callback->HasError());
            error = callback->GetLastError(conMan.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);

            // Remove bar and put "foo" back
            RemoveItemFromElementResources(page, L"bar");

            selector = CreateInstance(L"Tests.Tools.Shared.TestDataTemplateSelector");
            AddItemToElementResources(root, L"foo", selector);
            VERIFY_IS_FALSE(callback->HasError());
        }

        void ResolveResourceTests::VerifyDontThrowUnhandledExceptionOnInvalidResource()
        {
            auto xaml = ref new Platform::String(
                L" <StackPanel \r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>\r\n"
                L"   <StackPanel.Resources>\r\n"
                L"       <RotateTransform x:Key='T1' Angle='45'/>\r\n"
                L"       <Color x:Key='C1'>Red</Color>\r\n"
                L"       <SolidColorBrush x:Key='B1' Color='{StaticResource C1}' />\r\n"
                L"       <Style x:Key='S1' TargetType='Button'>\r\n"
                L"           <Setter Property='Background' Value='{StaticResource B1}' />\r\n"
                L"       </Style>\r\n"
                L"   </StackPanel.Resources>\r\n"
                L"   <Button Style ='{StaticResource S1}' x:Name='button'/>\r\n"
                L" </StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xaml, callback);

            auto root = callback->GetElementByName(L"root");
            auto style = GetItemFromElementResources(root, L"S1");
            auto setter = GetCollectionItem(style, L"Microsoft.UI.Xaml.Style.Setters", 0);
            auto setterValueIndex = GetPropertyIndex(setter, L"Microsoft.UI.Xaml.Setter.Value");
            ResolveResource(setter, L"T1", setterValueIndex, ResourceTypeStatic);
            VERIFY_IS_TRUE(callback->HasError());
            VERIFY_ARE_EQUAL(VisualElementState::ErrorInvalidResource, callback->GetLastError(root.Handle).State);

            ResolveResource(setter, L"B1", setterValueIndex, ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            auto button = callback->GetElementByName(L"button");
            auto backgroundIndex = GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");
            ResolveResource(button.Handle, L"T1", backgroundIndex, ResourceTypeStatic);
            VERIFY_IS_TRUE(callback->HasError());
            VERIFY_ARE_EQUAL(VisualElementState::ErrorInvalidResource, callback->GetLastError(button.Handle).State);
        }

        void ResolveResourceTests::CanResolveNullExtension()
        {
            auto xaml = ref new Platform::String(
                L" <StackPanel \r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>\r\n"
                L"   <StackPanel.Resources>\r\n"
                L"       <NullExtension x:Key='Null'/>\r\n"
                L"   </StackPanel.Resources>\r\n"
                L"   <Button Background='Red' x:Name='button'/>\r\n"
                L" </StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xaml, callback);

            auto button = callback->GetElementByName(L"button");
            auto backgroundIndex = GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");
            ResolveResource(button.Handle, L"Null", backgroundIndex, ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            auto buttonObj = ih_cast<xaml_controls::Button>(button.Handle);
            RunOnUIThread([&] {
                VERIFY_IS_NULL(buttonObj->Background);
            });
        }

        void ResolveResourceTests::CanResolveColorToBrush()
        {
            auto xaml = ref new Platform::String(
                L" <StackPanel \r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>\r\n"
                L"   <StackPanel.Resources>\r\n"
                L"       <Color x:Key='MyColor'>Red</Color>\r\n"
                L"   </StackPanel.Resources>\r\n"
                L"   <Button Background='Red' x:Name='button'/>\r\n"
                L" </StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xaml, callback);

            auto button = callback->GetElementByName(L"button");
            auto backgroundIndex = GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");
            ResolveResource(button.Handle, L"MyColor", backgroundIndex, ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            auto buttonObj = ih_cast<xaml_controls::Button>(button.Handle);
            RunOnUIThread([&] {
                auto scb = safe_cast<xaml_media::SolidColorBrush^>(buttonObj->Background);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, scb->Color);
            });
        }

        void ResolveResourceTests::CanResolveStringToBrush()
        {
            auto xaml = ref new Platform::String(
                L" <StackPanel \r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>\r\n"
                L"   <StackPanel.Resources>\r\n"
                L"       <x:String x:Key='MyColor'>Red</x:String>\r\n"
                L"   </StackPanel.Resources>\r\n"
                L"   <Button Background='Red' x:Name='button'/>\r\n"
                L" </StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xaml, callback);

            auto button = callback->GetElementByName(L"button");
            auto backgroundIndex = GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");
            ResolveResource(button.Handle, L"MyColor", backgroundIndex, ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            auto buttonObj = ih_cast<xaml_controls::Button>(button.Handle);
            RunOnUIThread([&] {
                auto scb = safe_cast<xaml_media::SolidColorBrush^>(buttonObj->Background);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, scb->Color);
            });
        }

        void ResolveResourceTests::CorrectlyResolvesStaticResourceInRuntimeBuiltTemplate()
        {
            // Commented out code is what this test is adding and validating works
            auto xaml = ref new Platform::String(
                L" <StackPanel \r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>\r\n"
                L"   <StackPanel.Resources>\r\n"
                L"       <Color x:Key='MyColor'>Red</Color>\r\n"
                L"       <Thickness x:Key='MyThickness'>5</Thickness>\r\n"
              //L"       <ControlTemplate x:Key='T1' TargetType='Button'>\r\n"
              //L"         <Grid x:Name='templateRoot' Background='{StaticResource MyColor}'/>
              //L"       </ControlTemplate> \r\n"
                L"       <Style TargetType='Pivot' x:Key='pivotStyle'>\r\n"
                L"           <Setter Property='Foreground' Value='White' />\r\n"
              //L"           <Setter Property='Template'>\r\n"
              //L"             <Setter.Value>\r\n"
              //L"               <ControlTemplate TargetType='Pivot'>\r\n"
              //L"                  <ItemsPresenter x:Name='PivotItemPresenter'/>\r\n"
              //L"               </ControlTemplate>\r\n"
              //L"             </Setter.Value>\r\n"
              //L"           </Setter>\r\n"
                L"       </Style>\r\n"
                L"   </StackPanel.Resources>\r\n"
                L"   <Button x:Name='button'/>\r\n"
                L"   <Pivot Style='{StaticResource pivotStyle}'>\r\n"
                L"     <PivotItem>\r\n"
                L"       <Rectangle x:Name='rect' Fill='Yellow' />\r\n"
                L"     </PivotItem>\r\n"
                L"   </Pivot>\r\n"
                L" </StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xaml, callback);

            auto button = callback->GetElementByName(L"button");

            xaml_controls::ControlTemplate^ ctrlTemplate = nullptr;

            auto root = callback->GetElementByName(L"root");
            RunOnUIThread([&]{
                ctrlTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                    L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' TargetType='Button'>\r\n"
                    L"  <Grid x:Name='templateRoot'/>\r\n"
                    L"</ControlTemplate>\r\n"));
            });

            AddItemToElementResources(root, L"T1", ih_cast(ctrlTemplate));

            ResolveResource(button.Handle, L"T1", L"Microsoft.UI.Xaml.Controls.Control.Template", ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            auto templateRoot = callback->GetElementByName(L"templateRoot");
            ResolveResource(templateRoot.Handle, L"MyColor", L"Microsoft.UI.Xaml.Controls.Panel.Background", ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            LOG_OUTPUT(L"Verify can resolve system resources");
            ResolveResource(button.Handle, L"SystemColorGrayTextColor", L"Microsoft.UI.Xaml.Controls.Control.Background", ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            auto templateRootObj = ih_cast<xaml_controls::Panel>(templateRoot.Handle);
            auto buttonObj = ih_cast<xaml_controls::Button>(button.Handle);
            RunOnUIThread([&] {
                auto scb = safe_cast<xaml_media::SolidColorBrush^>(templateRootObj->Background);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, scb->Color);

                scb = safe_cast<xaml_media::SolidColorBrush^>(buttonObj->Background);

                auto systemColor = safe_cast<::Windows::UI::Color>(xaml::Application::Current->Resources->Lookup(L"SystemColorGrayTextColor"));
                VERIFY_ARE_EQUAL(systemColor, scb->Color);
            });

            LOG_OUTPUT(L"Validate custom Pivot template placed in Setter");
            RunOnUIThread([&]{
                ctrlTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                    L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' TargetType='Pivot'>\r\n"
                    L"  <ItemsPresenter x:Name='PivotItemPresenter'/>\r\n"
                    L"</ControlTemplate>\r\n"));
            });
            auto pivotTemplateSetter = CreateSetterWithProperty(ih_cast(ctrlTemplate), L"Microsoft.UI.Xaml.Controls.Control.Template");

            auto style = GetItemFromElementResources(root, L"pivotStyle");
            AddSetterToStyle(style, pivotTemplateSetter);

            auto rect = callback->GetElementByName(L"rect");
            ResolveResource(rect.Handle, L"MyColor", L"Microsoft.UI.Xaml.Shapes.Shape.Fill", ResourceTypeStatic);

            auto pivotItemPresenter = callback->GetElementByName(L"PivotItemPresenter");
            ResolveResource(pivotItemPresenter.Handle, L"MyThickness", L"Microsoft.UI.Xaml.FrameworkElement.Margin", ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());
        }

        void ResolveResourceTests::ResolveResourceOutsideDataTemplate()
        {
             // Commented out code is what this test is adding and validating works
            auto xaml = ref new Platform::String(
                L" <StackPanel \r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>\r\n"
                L"   <StackPanel.Resources>\r\n"
                L"       <SolidColorBrush x:Key='MyBrush'>Red</SolidColorBrush>\r\n"
              //L"       <DataTemplate x:Key='T1' TargetType='Button'>\r\n"
              //L"          <TextBlock x:Name='templateTextBlock' Foreground='{StaticResoure MyBrush}' />\r\n"
              //L"       </DataTemplate> \r\n"
                L"       <Style TargetType='ListBox' x:Key='listBoxStyle'>\r\n"
                L"           <Setter Property='Foreground' Value='White' />\r\n"
              //L"           <Setter Property='ItemTemplate'>\r\n"
              //L"             <Setter.Value>\r\n"
              //L"               <DataTemplate>\r\n"
              //L"                  <TextBlock x:Name='setterTemplateTextBlock' Foreground='{StaticResoure MyBrush}' />\r\n"
              //L"               </ControlTemplate>\r\n"
              //L"             </Setter.Value>\r\n"
              //L"           </Setter>\r\n"
                L"       </Style>\r\n"
                L"   </StackPanel.Resources>\r\n"
                L"   <ListBox x:Name='StyledListBox' Style='{StaticResource listBoxStyle}'>\r\n"
                L"     <x:String>Hello World</x:String>\r\n"
                L"   </ListBox>\r\n"
                L"   <ListBox x:Name='ListBox'>\r\n"
                L"     <x:String>Hello World</x:String>\r\n"
                L"   </ListBox>\r\n"
                L" </StackPanel>\r\n");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xaml, callback);

            auto listBox = callback->GetElementByName(L"ListBox");

            xaml::DataTemplate^ dataTemplate = nullptr;

            auto root = callback->GetElementByName(L"root");
            RunOnUIThread([&]{
                dataTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                    L"  <TextBlock x:Name='templateTextBlock' />\r\n"
                    L"</DataTemplate>\r\n"));
            });

            AddItemToElementResources(root, L"T1", ih_cast(dataTemplate));

            ResolveResource(listBox.Handle, L"T1", L"Microsoft.UI.Xaml.Controls.ItemsControl.ItemTemplate", ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            auto templateTextBlock = callback->GetElementByName(L"templateTextBlock");
            ResolveResource(templateTextBlock.Handle, L"MyBrush", L"Microsoft.UI.Xaml.Controls.TextBlock.Foreground", ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            auto templateTextBlockObj = ih_cast<xaml_controls::TextBlock>(templateTextBlock.Handle);
            RunOnUIThread([&] {
                auto scb = safe_cast<xaml_media::SolidColorBrush^>(templateTextBlockObj->Foreground);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, scb->Color);
            });

            LOG_OUTPUT(L"Validate custom ListBox ItemTemplate placed in Setter");
            RunOnUIThread([&]{
                dataTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                    L"  <TextBlock x:Name='setterTemplateTextBlock' />\r\n"
                    L"</DataTemplate>\r\n"));
            });
            auto itemTemplateSetter = CreateSetterWithProperty(ih_cast(dataTemplate), L"Microsoft.UI.Xaml.Controls.ItemsControl.ItemTemplate");

            auto style = GetItemFromElementResources(root, L"listBoxStyle");
            AddSetterToStyle(style, itemTemplateSetter);

            auto setterTemplateTextBlock = callback->GetElementByName(L"setterTemplateTextBlock");
            ResolveResource(setterTemplateTextBlock.Handle, L"MyBrush", L"Microsoft.UI.Xaml.Controls.TextBlock.Foreground", ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());
        }
        #pragma endregion

    }
} } } } }
