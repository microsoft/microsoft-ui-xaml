// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ElementStateChangedTests.h"
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
#include <WUCRenderingScopeGuard.h>
#include <SafeEventRegistration.h>

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

        bool ElementStateChangedTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool ElementStateChangedTests::ClassCleanup()
        {
            return true;
        }

        bool ElementStateChangedTests::TestSetup()
        {
             TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());

            return EnsureTapLoaded();
        }

        bool ElementStateChangedTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void ElementStateChangedTests::VerifyPreviousErrorResolves()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <SolidColorBrush x:Key='a' Color='{StaticResource c}'/>\r\n"
                L"    <Color x:Key='c'>Black</Color>r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Ellipse x:Name='ellipse' Fill='{StaticResource a}' />\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto ellipse = callback->GetElementByName(L"ellipse");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(ellipse.Handle, L"Fill", BaseValueSourceLocal));

            RemoveItemFromElementResources(callback->GetElementByName(L"parent"), L"a");
            VERIFY_IS_TRUE(callback->HasError());

            auto error = callback->GetLastError(ellipse.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            // Since the ellipse is in the LVT, we just need a simple full property name
            VERIFY_ARE_EQUAL(std::wstring(L"Fill:Microsoft.UI.Xaml.Shapes.Shape"), error.Message);

            // Add a double, which creates an invalid reference
            auto double10 = CreateInstance(L"Windows.Foundation.Double", L"10.0");
            AddItemToElementResources(callback->GetElementByName(L"parent"), L"a", double10);
            error = callback->GetLastError(ellipse.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorInvalidResource, error.State);
            RemoveItemFromElementResources(callback->GetElementByName(L"parent"), L"a");

            // We should still have the resource not found error
            error = callback->GetLastError(ellipse.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);

            // Now, finally fix it
            auto blueBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Blue");
            AddItemToElementResources(callback->GetElementByName(L"parent"), L"a", blueBrush);

            VERIFY_IS_FALSE(callback->HasError());
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(ellipse.Handle, L"Fill", BaseValueSourceLocal));
        }

        void ElementStateChangedTests::VerifyVSMErrorPath()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);
            auto manual = callback->GetElementByName(L"visualStateManualTest");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Yellow, GetColorProperty(manual.Handle, L"Foreground", BaseValueSourceLocal));
            InstanceHandle pageHandle = 0;
            RunOnUIThread([&]
            {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                pageHandle = ih_cast(page);
                VERIFY_IS_TRUE(VisualStateManager::GoToState(page, "VisualStateManualTest", false));
            });

            // The Brush.Color property is animated, not the Forerground property
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(manual.Handle, L"Foreground", BaseValueSourceLocal));
            RemoveItemFromElementResources(callback->GetElementByHandle(pageHandle), L"manualForegroundBrushColor");
            VERIFY_IS_TRUE(callback->HasError());

            // This error will go up to the root and the path will point to the setter
            auto error = callback->GetLastError(callback->GetElementByName(L"LayoutRoot").Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            // This one is a mouthful
            VERIFY_ARE_EQUAL(std::wstring(L"VisualStateGroups:Microsoft.UI.Xaml.VisualStateManager[1]/States:Microsoft.UI.Xaml.VisualStateGroup[0]/Setters:Microsoft.UI.Xaml.VisualState[1]/Value:Microsoft.UI.Xaml.Setter"), error.Message);
        }

        void ElementStateChangedTests::VerifyResourceDictionaryPath()
        {
             auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <SolidColorBrush x:Key='a' Color='{StaticResource c}'/>\r\n"
                L"    <Color x:Key='c'>Black</Color>r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Ellipse x:Name='ellipse' Fill='{StaticResource a}' />\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto parent = callback->GetElementByName(L"parent");
            RemoveItemFromElementResources(parent, L"c");
            VERIFY_IS_TRUE(callback->HasError());

            auto error = callback->GetLastError(parent.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"Resources:Microsoft.UI.Xaml.FrameworkElement['a']/Color:Microsoft.UI.Xaml.Media.SolidColorBrush"), error.Message);
        }

        void ElementStateChangedTests::VerifyPathToBindingProperty()
        {
            auto content = ref new Platform::String(
                L"<Grid x:Name='parent' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Height='45' Width='45'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <Rectangle x:Key='rectBind' Height='53' />\r\n>"
                L"  </Grid.Resources>\r\n"
                L"  <Rectangle x:Name='rect' Height='30' Width='{Binding Source={StaticResource rectBind}, Path=Height}'  />\r\n"
                L"</Grid>");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto parent = callback->GetElementByName(L"parent");
            RemoveItemFromElementResources(parent, L"rectBind");
            VERIFY_IS_TRUE(callback->HasError());

            auto element = callback->GetElementByName(L"rect");
            auto error = callback->GetLastError(element.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"Width:Microsoft.UI.Xaml.FrameworkElement/Source:Microsoft.UI.Xaml.Data.Binding"), error.Message);
        }

        void ElementStateChangedTests::VerifyResourceDictionaryPathStyle()
        {
            auto componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/MainPage.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ApplicationThemeOverrider themeHelper(Microsoft::UI::Xaml::ApplicationTheme::Light);
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);
            InstanceHandle pageHandle = 0;
            RunOnUIThread([&]
            {
                pageHandle = ih_cast(TestServices::WindowHelper->WindowContent);
            });

            auto stacky = callback->GetElementByName(L"stacky");
            auto themeDictionaries = GetProperty(GetProperty(stacky.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources"), L"Microsoft.UI.Xaml.ResourceDictionary.ThemeDictionaries");
            VERIFY_SUCCEEDED(m_tap->ClearChildren(themeDictionaries));
            // This should be OK. There are ones in CommonStyles.xaml that it should resolve to now.
            VERIFY_IS_FALSE(callback->HasError());

            // Clearing these theme dictionaries in CommonStyles.xaml should cause all sorts of trouble.
            auto mergedDictionaries = GetCollectionProperty(GetProperty(pageHandle, L"Microsoft.UI.Xaml.FrameworkElement.Resources"), L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            auto dictionaryHandle = ih_cast(mergedDictionaries.Elements[0].Value);
            themeDictionaries = GetProperty(dictionaryHandle, L"Microsoft.UI.Xaml.ResourceDictionary.ThemeDictionaries");
            VERIFY_SUCCEEDED(m_tap->ClearChildren(themeDictionaries));
            VERIFY_IS_TRUE(callback->HasError());
            auto errors = callback->GetErrorsForElement(pageHandle);
            for (const auto& error : errors)
            {
                VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            }
        }

        void ElementStateChangedTests::VerifyResourceDictionaryPathImplicitStyle()
        {
            auto componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/MainPage.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);
            auto parent = callback->GetElementByName(L"g");
            RemoveItemFromElementResources(parent, L"scb");
            VERIFY_IS_TRUE(callback->HasError());

            auto error = callback->GetLastError(parent.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"Resources:Microsoft.UI.Xaml.FrameworkElement[{x:Type Microsoft.UI.Xaml.Controls.Button}]/Setters:Microsoft.UI.Xaml.Style[0]/Value:Microsoft.UI.Xaml.Setter"), error.Message);

            auto r_inline = callback->GetElementByName(L"r_inline");
            auto r_inlineError = callback->GetLastError(r_inline.Handle);
            VERIFY_ARE_EQUAL(std::wstring(L"Style:Microsoft.UI.Xaml.FrameworkElement/Setters:Microsoft.UI.Xaml.Style[0]/Value:Microsoft.UI.Xaml.Setter"), r_inlineError.Message);

            auto brush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Blue");
            AddItemToElementResources(parent, L"abc", brush);
            auto style = GetImplicitStyleFromElementResources(parent, L"Microsoft.UI.Xaml.Controls.Button");
            auto setters = GetCollectionProperty(style, L"Microsoft.UI.Xaml.Style.Setters");
            InstanceHandle setter = ih_cast(setters.Elements[0].Value);
            ResolveResource(setter, L"abc", GetPropertyIndex(setter, L"Microsoft.UI.Xaml.Setter.Value"), ResourceTypeStatic);
            error = callback->GetLastError(parent.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResolved, error.State);

            VERIFY_IS_TRUE(callback->HasError()); // We should still have the problem with the rectangle Style

            auto b = callback->GetElementByName(L"b");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(b.Handle, L"Background", BaseValueSourceStyle));
        }

        void ElementStateChangedTests::VerifyPathWithCustomType()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"    xmlns:local='using:Tests.Tools.Shared'\r\n"
                L"    x:Name='parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <ResourceDictionary>\r\n"
                L"      <ResourceDictionary.MergedDictionaries>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='{StaticResource c}'/>\r\n"
                L"          <Color x:Key='c'>Black</Color>r\n"
                L"        </ResourceDictionary>\r\n"
                L"      </ResourceDictionary.MergedDictionaries>\r\n"
                L"    </ResourceDictionary>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <local:CustomUserControl x:Name='cuc' local:CustomBrush='{StaticResource a}' />\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto parent = callback->GetElementByName(L"parent");

            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            RemoveDictionaryItem(ih_cast(mergedDictionaries.Elements[0].Value), L"a");
            VERIFY_IS_TRUE(callback->HasError());

            auto cuc = callback->GetElementByName(L"cuc");
            auto error = callback->GetLastError(cuc.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"CustomBrush:Tests.Tools.Shared.CustomUserControl"), error.Message);
        }

        void ElementStateChangedTests::VerifyMergedDictionaryPath()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <ResourceDictionary>\r\n"
                L"      <ResourceDictionary.MergedDictionaries>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='{StaticResource c}'/>\r\n"
                L"          <Color x:Key='c'>Black</Color>r\n"
                L"        </ResourceDictionary>\r\n"
                L"      </ResourceDictionary.MergedDictionaries>\r\n"
                L"    </ResourceDictionary>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Ellipse x:Name='ellipse' Fill='{StaticResource a}' />\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto parent = callback->GetElementByName(L"parent");

            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            RemoveDictionaryItem(ih_cast(mergedDictionaries.Elements[0].Value), L"c");
            VERIFY_IS_TRUE(callback->HasError());

            auto error = callback->GetLastError(parent.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"Resources:Microsoft.UI.Xaml.FrameworkElement/MergedDictionaries:Microsoft.UI.Xaml.ResourceDictionary[0]['a']/Color:Microsoft.UI.Xaml.Media.SolidColorBrush"), error.Message);
        }

        void ElementStateChangedTests::VerifyPathWithUIElementParent()
        {
             auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <Color x:Key='a'>Red</Color>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Ellipse x:Name='ellipse'>\r\n"
                L"    <Ellipse.Fill>\r\n"
                L"      <SolidColorBrush Color='{StaticResource a}' />\r\n"
                L"    </Ellipse.Fill>\r\n"
                L"  </Ellipse>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto parent = callback->GetElementByName(L"parent");
            RemoveItemFromElementResources(parent, L"a");
            VERIFY_IS_TRUE(callback->HasError());

            auto ellipse = callback->GetElementByName(L"ellipse");
            auto error = callback->GetLastError(ellipse.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"Fill:Microsoft.UI.Xaml.Shapes.Shape/Color:Microsoft.UI.Xaml.Media.SolidColorBrush"), error.Message);
        }

        void ElementStateChangedTests::VerifyRemovedApplicationResourceError()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            // The "MyBrush" entry in the App's resource dictionary has a static resource reference to "MyColor" - try removing "MyColor"
            // and ensure we get the proper error that "MyBrush" can't resolve its color, instead of crashing

            InstanceHandle col = CreateInstance(L"Windows.UI.Color", L"Red");
            InstanceHandle brush = CreateInstance(L"Microsoft.UI.Xaml.Media.SolidColorBrush", L"Orange");

            InstanceHandle appResources = GetApplicationResources();
            InstanceHandle app;
            IInspectable* pAppInspectable;
            VERIFY_SUCCEEDED(m_tap->GetApplication(&pAppInspectable));
            VERIFY_SUCCEEDED(m_tap->GetHandleFromIInspectable(pAppInspectable, &app));

            AddDictionaryItem(appResources, L"MyColor", col);
            AddDictionaryItem(appResources, L"MyBrush", brush);
            ResolveResource(brush, L"MyColor", GetPropertyIndex(brush, L"Microsoft.UI.Xaml.Media.SolidColorBrush.Color"), ResourceTypeStatic);

            // Actually remove "MyColor" which should trigger in the app object since "MyBrush" can't resolve its color
            RemoveDictionaryItem(appResources, L"MyColor");
            VERIFY_IS_TRUE(callback->HasError());

            auto error = callback->GetLastError(app);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"Resources:Microsoft.UI.Xaml.Application['MyBrush']/Color:Microsoft.UI.Xaml.Media.SolidColorBrush"), error.Message);

            // We also need to remove MyBrush to clean up the app's dictionary in case this test is being run multiple times in stress mode, as
            // the app object isn't reset between runs
            RemoveDictionaryItem(appResources, L"MyBrush");
        }

        void ElementStateChangedTests::VerifyStateChangeWhenNotInTree()
        {
            // Verify that if a state change occurs while a element is not in the live tree, when it is shown it will be shown with
            // the right state and the right theme.  Note:  There are significantly different code paths that can be taken depending on
            // whether the current theme is the theme in effect when the app is initially loaded (i.e. a either system theme or the app 
            // requested theme) or not.  Since this test (or test class) has no control over this, we test in both themes knowing that one
            // of them won't be the initial one.

            WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

            xaml_controls::Button^ button = nullptr;
            xaml_controls::Flyout^ flyout = nullptr;
            xaml_controls::InfoBar^ infobar = nullptr;
            xaml_controls::Grid^ rootPanel = nullptr;
            auto flyoutOpenedEvent = std::make_shared<Event>();
            auto flyoutClosedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
            auto closedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

            RunOnUIThread([&]()
                {
                    // Note that we don't show the icon.  This is because we use really old fonts for these tests and the symbols don't
                    // exist.  But since we just want to make sure we are picking up the right visual state/theme combination, the
                    // severity colors will allow us to do that.
                    rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                        L"      x:Name='root' Background='SlateBlue' Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                        L"  <Button x:Name='button' Content='button.flyout' HorizontalAlignment='Center' VerticalAlignment='Center' FontSize='25' > "
                        L"    <Button.Flyout> "
                        L"      <Flyout Placement='TopEdgeAlignedLeft'> "
                        L"        <InfoBar x:Name='infobar' IsOpen='True' Severity='Error' Title='InforBar Title' Message='Message' IsIconVisible='False'/> "
                        L"      </Flyout> "
                        L"    </Button.Flyout> "
                        L"  </Button> "
                        L"</Grid>"));

                    VERIFY_IS_NOT_NULL(rootPanel);
                    TestServices::WindowHelper->WindowContent = rootPanel;

                    button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                    VERIFY_IS_NOT_NULL(button);

                    flyout = dynamic_cast<xaml_controls::Flyout^>(button->Flyout);
                    VERIFY_IS_NOT_NULL(flyout);

                    infobar = dynamic_cast<xaml_controls::InfoBar^>(rootPanel->FindName(L"infobar"));
                    VERIFY_IS_NOT_NULL(flyout);

                    openedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
                        {
                            LOG_OUTPUT(L"PopupOpenClose: Flyout Opened event is fired!");
                            flyoutOpenedEvent->Set();
                        }));

                    closedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^)
                        {
                            LOG_OUTPUT(L"PopupOpenClose: Flyout Closed event is fired!");
                            flyoutClosedEvent->Set();
                        }));
                });
            TestServices::WindowHelper->WaitForIdle();

            // Create a lambda to show/hide the flyout and optionally validating the dcomp tree
            auto ShowAndHideFlyout = [&](Platform::String^ label) {
                LOG_OUTPUT(L"Button Tap operation to show the Flyout and infobar");
                TestServices::InputHelper->Tap(button);
                flyoutOpenedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                if (label)
                {
                    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, label);
                }

                LOG_OUTPUT(L"Hide the flyout and infobar");
                RunOnUIThread([&]()
                    {
                        flyout->Hide();
                    });
                flyoutClosedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();
            };

            // Initially Show/Hide the flyout and InfoBar to "warm it up".  Until this happens we don't expand the templates so the element(s)
            // we are concerned about don't actually exist yet.
            ShowAndHideFlyout(nullptr);

            // Verify the light theme with Error Severity
            RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Change to Light theme");
                    rootPanel->RequestedTheme = xaml::ElementTheme::Light;
                });
            TestServices::WindowHelper->WaitForIdle();
            ShowAndHideFlyout(L"LightError");

            // Change the severity to success and verify
            RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Change Severity to Success");
                    infobar->Severity = xaml_controls::InfoBarSeverity::Success;
                });
            TestServices::WindowHelper->WaitForIdle();
            ShowAndHideFlyout(L"LightSuccess");

            // Change the theme to dark and go again
            RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Change to dark theme");
                    rootPanel->RequestedTheme = xaml::ElementTheme::Dark;
                });
            TestServices::WindowHelper->WaitForIdle();
            ShowAndHideFlyout(L"DarkSuccess");

            // Change the severity to success and verify
            RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Change Severity to Warning");
                    infobar->Severity = xaml_controls::InfoBarSeverity::Warning;
                });
            TestServices::WindowHelper->WaitForIdle();
            ShowAndHideFlyout(L"DarkWarning");
        }

        void ElementStateChangedTests::VerifyPathWithNestedStyles()
        {
            auto content = ref new Platform::String(
                L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'>\r\n"
                L"<Grid Background='{ThemeResource ApplicationPageBackgroundThemeBrush}' x:Name='root'>\r\n"
                L" <Grid.Resources>\r\n"
                L"  <SolidColorBrush x:Key='B1' Color='Red'/>\r\n"
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
                L"  <ListViewItem Content='One'/>\r\n"
                L"  <ListViewItem Content='Two'/>\r\n"
                L" </ListView>\r\n"
                L"</Grid>\r\n"
                L"</Page>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto root = callback->GetElementByName(L"root");
            auto lisViewImplicitStyle = GetImplicitStyleFromElementResources(root, L"Microsoft.UI.Xaml.Controls.ListView");
            auto setter = GetCollectionItem(lisViewImplicitStyle, L"Microsoft.UI.Xaml.Style.Setters", 0);

            auto listViewItemImplicitStyle = GetProperty(setter, L"Microsoft.UI.Xaml.Setter.Value");

            auto setterToResolve = GetCollectionItem(listViewItemImplicitStyle, L"Microsoft.UI.Xaml.Style.Setters", 0);
            auto propertyIndex = GetPropertyIndex(setterToResolve, L"Microsoft.UI.Xaml.Setter.Value");

            LOG_OUTPUT(L"Verifying we get the correct path to the object");
            ResolveResource(setterToResolve, L"B", propertyIndex, ResourceTypeStatic);
            VERIFY_IS_TRUE(callback->HasError());

            auto error = callback->GetLastError(root.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"Resources:Microsoft.UI.Xaml.FrameworkElement[{x:Type Microsoft.UI.Xaml.Controls.ListView}]/Setters:Microsoft.UI.Xaml.Style[0]/Value:Microsoft.UI.Xaml.Setter/Setters:Microsoft.UI.Xaml.Style[0]/Value:Microsoft.UI.Xaml.Setter"), error.Message);
        }

        void ElementStateChangedTests::VerifyPathToItemsControlItemContainerStyle()
        {
            // Test runs in DesignMode V2 so we don't crash and instead get the callback

            auto content = ref new Platform::String(
                L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'>\r\n"
                L"<Grid Background='{ThemeResource ApplicationPageBackgroundThemeBrush}' x:Name='root'>\r\n"
                L" <Grid.Resources>\r\n"
                L"  <Style TargetType='Button' x:Key='BadListViewItemStyle'>\r\n"
                L"    <Setter Property='Foreground' Value='Red'/>\r\n"
                L"  </Style>\r\n"
                L" </Grid.Resources>\r\n"
                L" <ListView Width='200' Height='300' x:Name='list'>\r\n"
                L"  <ListViewItem Content='One'/>\r\n"
                L"  <ListViewItem Content='Two'/>\r\n"
                L" </ListView>\r\n"
                L"</Grid>\r\n"
                L"</Page>\r\n");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto list = callback->GetElementByName(L"list");
            ResolveResource(list.Handle, L"BadListViewItemStyle", L"Microsoft.UI.Xaml.Controls.ItemsControl.ItemContainerStyle", ResourceTypeStatic);
            VERIFY_IS_TRUE(callback->HasError());

            auto error = callback->GetLastError(list.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorInvalidResource, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"ItemContainerStyle:Microsoft.UI.Xaml.Controls.ItemsControl"), error.Message);
        }

        void ElementStateChangedTests::VerifyPathToFlyoutPresenterStyle()
        {
            // This Bug has been around for a while. This test was also added to validate that a scenario works, so no new
            // product code was added that could've caused a leak.
            // Leak: TemplateContent peer not being unpegged
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
            auto content = ref new Platform::String(
                L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'>\r\n"
                L"<StackPanel Background='{ThemeResource ApplicationPageBackgroundThemeBrush}' x:Name='root'>\r\n"
                L" <StackPanel.Resources>\r\n"
                L"  <Style TargetType='FlyoutPresenter' x:Key='FPStyleDark'>\r\n"
                L"     <Setter Property='RequestedTheme' Value='Dark' />\r\n"
                L"  </Style>\r\n"
                L"  <Style TargetType='FlyoutPresenter' x:Key='FPStyleLight'>\r\n"
                L"     <Setter Property='RequestedTheme' Value='Light' />\r\n"
                L"  </Style>\r\n"
                L" </StackPanel.Resources>\r\n"
                L" <Button x:Name='button' Content='Dark Flyout' Margin='20'>\r\n"
                L"     <Button.Flyout>\r\n"
                L"         <Flyout Placement='Left'>\r\n"
                L"         </Flyout>\r\n"
                L"     </Button.Flyout>\r\n"
                L" </Button>\r\n"
                L" <TextBlock x:Name='text' Text='Light Flyout' Margin='20'>\r\n"
                L"     <FlyoutBase.AttachedFlyout>\r\n"
                L"         <Flyout Placement='Right'>\r\n"
                L"           <TextBlock Text='Hey yeah!!' \r\n/>"
                L"         </Flyout>\r\n"
                L"     </FlyoutBase.AttachedFlyout>\r\n"
                L" </TextBlock>\r\n"
                L"</StackPanel>\r\n"
                L"</Page>\r\n");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto button = callback->GetElementByName(L"button");
            auto flyHandle = GetProperty(button.Handle, L"Microsoft.UI.Xaml.Controls.Button.Flyout");
            ResolveResource(flyHandle, L"FPStyleDark", L"Microsoft.UI.Xaml.Controls.Flyout.FlyoutPresenterStyle", ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            ResolveResource(flyHandle, L"FPStyleDark_DNE", L"Microsoft.UI.Xaml.Controls.Flyout.FlyoutPresenterStyle", ResourceTypeStatic);
            VERIFY_IS_TRUE(callback->HasError());
            auto error = callback->GetLastError(button.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"Flyout:Microsoft.UI.Xaml.Controls.Button/FlyoutPresenterStyle:Microsoft.UI.Xaml.Controls.Flyout"), error.Message);

            auto text = callback->GetElementByName(L"text");
            RunOnUIThread([&]{
                xaml_controls::Primitives::FlyoutBase::ShowAttachedFlyout(ih_cast<TextBlock>(text.Handle));
            });
            TestServices::WindowHelper->WaitForIdle();
            flyHandle = GetProperty(text.Handle, L"Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase.AttachedFlyout");
            ResolveResource(flyHandle, L"FPStyleLight", L"Microsoft.UI.Xaml.Controls.Flyout.FlyoutPresenterStyle", ResourceTypeStatic);
            ResolveResource(flyHandle, L"FPStyleLight_DNE", L"Microsoft.UI.Xaml.Controls.Flyout.FlyoutPresenterStyle", ResourceTypeStatic);
            VERIFY_IS_TRUE(callback->HasError());
            error = callback->GetLastError(text.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, error.State);
            VERIFY_ARE_EQUAL(std::wstring(L"AttachedFlyout:Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase/FlyoutPresenterStyle:Microsoft.UI.Xaml.Controls.Flyout"), error.Message);
        }
        #pragma endregion
    }
} } } } }
