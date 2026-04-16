// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "AddRemoveDictionaryItems.h"
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

        bool AddRemoveDictionaryItemTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool AddRemoveDictionaryItemTests::ClassCleanup()
        {
            return true;
        }

        bool AddRemoveDictionaryItemTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());
            return EnsureTapLoaded();
        }

        bool AddRemoveDictionaryItemTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            test_infra::TestServices::WindowHelper->VerifyTestCleanup();

            return true;
        }

        void AddRemoveDictionaryItemTests::AddItemBasic()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);

            // buttonApp's Background property resolves to a brush defined in App.xaml resources. We're going to add a brush on the root
            // grid and make sure that buttonApp, which is a child of the grid, resolves correctly
            auto button = callback->GetElementByName(L"buttonApp");

            auto previousColor = GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, previousColor);
            RunOnUIThread([&]
            {
                auto brush = safe_cast<xaml_media::SolidColorBrush^>(Application::Current->Resources->Lookup("testApp"));
                VERIFY_ARE_EQUAL(previousColor, brush->Color);
            });

            auto parent = callback->GetElementByName(L"parent");

            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto redBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red");
            AddDictionaryItem(resources, L"testApp", redBrush);

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            // verify adding a testOverwritten to Application.Resources doesn't affect currently resolved value for buttonOverwritten
            auto appResources = GetApplicationResources();
            AddDictionaryItem(appResources, L"testOverwritten", CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Blue"));

            // We have to remove this from application resources otherwise in stress mode trying to add it again will fail.
            auto removeFromApp = wil::scope_exit([&] {
                RemoveDictionaryItem(appResources, L"testOverwritten");
            });

            button = callback->GetElementByName(L"buttonOverwritten");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
        }

        void AddRemoveDictionaryItemTests::RemoveItemBasic()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);

            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto button = callback->GetElementByName(L"buttonOverwritten");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            VERIFY_IS_GREATER_THAN(mergedDictionaries.Count, 0u);
            InstanceHandle mergedDictionaryHandle = std::stoll(mergedDictionaries.Elements[1].Value);

            // Let's remove the last testOverwritten and make sure that the buttonOverwritten's background updates accordingly
            RemoveDictionaryItem(mergedDictionaryHandle, L"testOverwritten");

            // Let's remove the last testOverwritten and make sure that the buttonOverwritten's background updates accordingly
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            // Try removing testOverwritten again, and since an element still has a reference to it, this should report
            // that the button now has resource reference that can't be resolved.
            mergedDictionaryHandle = std::stoll(mergedDictionaries.Elements[0].Value);
            RemoveDictionaryItem(mergedDictionaryHandle, L"testOverwritten");

            auto lastError = callback->GetLastError(button.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, lastError.State);

            // Now, change the background on buttonOverwritten to resolve to testApp
            unsigned int backgroundIndex = GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");
            ResolveResource(button.Handle, L"testApp", backgroundIndex, ResourceTypeStatic);
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            // Now, verify we can remove testOverwritten that we've removed the last object depending on it
            RemoveDictionaryItem(mergedDictionaryHandle, L"testOverwritten");
            VERIFY_ARE_EQUAL(0u, callback->GetErrorCount(button.Handle));

            // Just being paranoid, make sure the color of the button that was previously referencing it didn't change
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
        }

        void AddRemoveDictionaryItemTests::CanResolveNewlyAddedItem()
        {
            // We want to make sure that the resolved color matches the current application theme
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);

            // See if we can add something to application resources
            auto appResources = GetApplicationResources();
            AddDictionaryItem(appResources, L"foo", CreateInstance(L"Windows.Foundation.Double", L"35"));
            // We have to remove this from application resources otherwise in stress mode trying to add it again will fail.
            auto removeFromApp = wil::scope_exit([&] {
                RemoveDictionaryItem(appResources, L"foo");
            });

            auto button = callback->GetElementByName(L"buttonOverwritten");

            auto width = GetPropertyChainValue(button.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(200, std::stoi(width.Value));

            // Resolve the "foo" and make sure that width is now 35.
            ResolveResource(button.Handle, L"foo", width.Index, ResourceTypeStatic);
            width = GetPropertyChainValue(button.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(35, std::stoi(width.Value));
        }

        void AddRemoveDictionaryItemTests::CannotResolveRemovedItem()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);
            auto button = callback->GetElementByName(L"buttonNoResource");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            // Remove "RedBrush" and verify it can't be resolved.
            RemoveDictionaryItem(resources, L"RedBrush");

            auto backgroundIndex = GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");

            // Resolve resource to 'testOverwritten' and make sure it's green. testOverwritten is in a merged dictionary
            ResolveResource(button.Handle, L"RedBrush", backgroundIndex, ResourceTypeStatic);

            // At this point the markup has a static resource reference to RedBrush, which doesn't exist. The local property
            // should be cleared at this point.
            VERIFY_THROWS(GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal), WEX::Common::Exception);

            auto lastError = callback->GetLastError(button.Handle);
            VERIFY_ARE_EQUAL(VisualElementState::ErrorResourceNotFound, lastError.State);
        }

        void AddRemoveDictionaryItemTests::CanAddStyle()
        {
            TestAddStyle(false);
        }

        void AddRemoveDictionaryItemTests::CanAddImplicitStyle()
        {
            TestAddStyle(true);
        }

        void AddRemoveDictionaryItemTests::TestAddStyle(bool isImplicit)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);
            auto button = callback->GetElementByName(L"buttonNoResource");

            // First, verify the current state. Pink button that hangs out on the right.
            auto horizontalAlignment = GetPropertyChainValue(button.Handle, L"HorizontalAlignment", BaseValueSourceLocal);
            auto convertedEnum = m_tap->ConvertEnumValue(horizontalAlignment.Type, std::stoi(horizontalAlignment.Value));
            VERIFY_IS_TRUE(convertedEnum.compare(L"Right") == 0);

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            // Let's add a button style
            auto buttonStyle = CreateStyle(L"Microsoft.UI.Xaml.Controls.Button");

            auto backgroundSetter = CreateSetterWithProperty(CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Black"), L"Microsoft.UI.Xaml.Controls.Control.Background");
            auto haSetter = CreateSetterWithProperty(CreateInstance(L"Microsoft.UI.Xaml.HorizontalAlignment", L"Stretch"), L"Microsoft.UI.Xaml.FrameworkElement.HorizontalAlignment");

            AddSetterToStyle(buttonStyle, backgroundSetter);
            AddSetterToStyle(buttonStyle, haSetter);

            // Add a style to the parent dictionary
            if (isImplicit)
            {
                AddImplicitStyle(resources, buttonStyle);
                VERIFY_IS_FALSE(callback->HasError());
            }
            else
            {
                AddDictionaryItem(resources, L"MyBlackButtonStyle", buttonStyle);
                VERIFY_IS_FALSE(callback->HasError());

                // Being paranoid, make sure everything is the same
                horizontalAlignment = GetPropertyChainValue(button.Handle, L"HorizontalAlignment", BaseValueSourceLocal);
                convertedEnum = m_tap->ConvertEnumValue(horizontalAlignment.Type, std::stoi(horizontalAlignment.Value));
                VERIFY_IS_TRUE(convertedEnum.compare(L"Right") == 0);

                // Resolve the style since this is not an implicit one
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
                unsigned int styleIndex = GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Style");
                ResolveResource(button.Handle, L"MyBlackButtonStyle", styleIndex, ResourceTypeStatic);
            }

            VERIFY_IS_FALSE(callback->HasError());

            // Make sure everything has changed.
            horizontalAlignment = GetPropertyChainValue(button.Handle, L"HorizontalAlignment", BaseValueSourceStyle);
            convertedEnum = m_tap->ConvertEnumValue(horizontalAlignment.Type, std::stoi(horizontalAlignment.Value));
            VERIFY_IS_TRUE(convertedEnum.compare(L"Stretch") == 0);

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(button.Handle, L"Background", BaseValueSourceStyle));
        }

        void AddRemoveDictionaryItemTests::CanRemoveImplicitStyle()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::resourcesInDictionaryString, callback);

            auto rectangle = callback->GetElementByName(L"childVerified");

            auto horizontalAlignment = GetPropertyChainValue(rectangle.Handle, L"HorizontalAlignment", BaseValueSourceStyle);
            auto convertedEnum = m_tap->ConvertEnumValue(horizontalAlignment.Type, std::stoi(horizontalAlignment.Value));
            VERIFY_IS_TRUE(convertedEnum.compare(L"Right") == 0);

            auto root = callback->GetElementByName(L"root");
            auto rootResources = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            RemoveImplicitStyle(rootResources, L"Microsoft.UI.Xaml.Shapes.Rectangle");

            // Verify it falls back to the default of stretch
            horizontalAlignment = GetPropertyChainValue(rectangle.Handle, L"HorizontalAlignment", BaseValueSourceDefault);
            convertedEnum = m_tap->ConvertEnumValue(horizontalAlignment.Type, std::stoi(horizontalAlignment.Value));
            VERIFY_IS_TRUE(convertedEnum.compare(L"Stretch") == 0);
        }

        void AddRemoveDictionaryItemTests::CanAddPropertyValueReferences()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);

            auto parent = callback->GetElementByName(L"parent");

            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            LOG_OUTPUT(L"Adding color to dictionary");
            auto redColor = CreateInstance(L"Windows.UI.Color", L"Red");
            AddDictionaryItem(resources, L"color", redColor);
            VERIFY_IS_FALSE(callback->HasError());

            LOG_OUTPUT(L"Adding thickness to dictionary");
            auto alan = CreateInstance(L"Microsoft.UI.Xaml.Thickness", L"10,10,20,5");
            AddDictionaryItem(resources, L"AlanThickness", alan);
            VERIFY_IS_FALSE(callback->HasError());

            LOG_OUTPUT(L"Adding double to dictionary");
            auto dubs = CreateInstance(L"Windows.Foundation.Double", L"10.0");
            AddDictionaryItem(resources, L"RollinOn", dubs);
            VERIFY_IS_FALSE(callback->HasError());

            LOG_OUTPUT(L"Adding string to dictionary");
            auto imRonBurgandy = CreateInstance(L"Windows.Foundation.String", L"Unique New York");
            AddDictionaryItem(resources, L"MyString", imRonBurgandy);
            VERIFY_IS_FALSE(callback->HasError());

            LOG_OUTPUT(L"Adding bool to dictionary");
            auto imTheCoolest = CreateInstance(L"Windows.Foundation.Boolean", L"True");
            AddDictionaryItem(resources, L"MyBool", imTheCoolest);
            VERIFY_IS_FALSE(callback->HasError());

            LOG_OUTPUT(L"Adding VerticalAlignment to dictionary");
            auto vert = CreateInstance(L"Microsoft.UI.Xaml.VerticalAlignment", L"Center");
            AddDictionaryItem(resources, L"MyAlignment", vert);
            VERIFY_IS_FALSE(callback->HasError());

            LOG_OUTPUT(L"Adding Int32 to dictionary");
            auto interesting = CreateInstance(L"Windows.Foundation.Int32", L"14");
            AddDictionaryItem(resources, L"InterestingInt", interesting);
            VERIFY_IS_FALSE(callback->HasError());
        }

        void AddRemoveDictionaryItemTests::CanAddToStandaloneDictionary()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/MainPage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            // Get the pages resources and get the merged dictionary, which is the standalone dictionary in CommonStyles.xaml
            InstanceHandle pageHandle = 0;
            RunOnUIThread([&]
            {
                pageHandle = ih_cast(TestServices::WindowHelper->WindowContent);
            });

            auto pageResources = GetProperty(pageHandle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto mergedDictionaries = GetCollectionProperty(pageResources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            InstanceHandle mergedDictionaryHandle = std::stoll(mergedDictionaries.Elements[0].Value);

            auto redBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red");
            AddDictionaryItem(mergedDictionaryHandle, L"testApp", redBrush);

            VERIFY_IS_FALSE(callback->HasError());
        }

        void AddRemoveDictionaryItemTests::ErrorOnAddingWrongTypeToSiblingDictionary()
        {
            TestAddToSiblingMergedDictionary(L"Windows.Foundation.Double", L"20.2", true);
        }

        void AddRemoveDictionaryItemTests::UpdatesOnAddToSiblingDictionary()
        {
            TestAddToSiblingMergedDictionary(L"Microsoft.UI.Xaml.Media.Brush", L"Green", false);
        }

        void AddRemoveDictionaryItemTests::TestAddToSiblingMergedDictionary(const std::wstring& type, const std::wstring& value, bool expectError)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;

            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <ResourceDictionary>\r\n"
                L"      <ResourceDictionary.MergedDictionaries>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Red' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <SolidColorBrush x:Key='dummy' Color='Green' />\r\n" // Placeholder just so the dictionary already exists
                L"        </ResourceDictionary>\r\n"
                L"      </ResourceDictionary.MergedDictionaries>\r\n"
                L"      <ResourceDictionary.ThemeDictionaries>\r\n"
                L"        <ResourceDictionary x:Key='Light'>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Yellow' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"        <ResourceDictionary x:Key='Dark'>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Blue' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"      </ResourceDictionary.ThemeDictionaries>\r\n"
                L"    </ResourceDictionary>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Ellipse x:Name='ellipse' Fill='{StaticResource a}' />\r\n"
                L"</Grid>\r\n");
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");

            auto ellipseA = callback->GetElementByName(L"ellipse");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetColorProperty(ellipseA.Handle, L"Fill", BaseValueSourceLocal));
            InstanceHandle mergedDictionaryHandle = std::stoll(mergedDictionaries.Elements[1].Value);

            auto newAddition = CreateInstance(type, value);
            AddDictionaryItem(mergedDictionaryHandle, L"a", newAddition);

            if (expectError)
            {
                VERIFY_IS_TRUE(callback->HasError());
                auto errors = callback->GetErrorsForElement(ellipseA.Handle);
                VERIFY_ARE_EQUAL(1u, errors.size());
                VERIFY_ARE_EQUAL(VisualElementState::ErrorInvalidResource, errors[0].State);
            }
            else
            {
                LOG_OUTPUT(L"Expected Color: %s", value.c_str());
                auto prop = GetPropertyChainValue(ellipseA.Handle, L"Fill", BaseValueSourceLocal);
                auto colorProp = GetPropertyChainValue(std::stoull(prop.Value), L"Color", BaseValueSourceLocal);
                LOG_OUTPUT(L"Actual Color: %s", colorProp.Value);
                if (wcscmp(value.c_str(), colorProp.Value) != 0)
                {
                    VERIFY_FAIL(L"Fill property not as expected");
                }
            }
        }

        void AddRemoveDictionaryItemTests::UpdatesOnRemovingFromSiblingDictionary()
        {
            // The ellipse references resource 'a' which is a solid color brush. When we remove brush 'a' from the dictionary, the resolution logic should now find
            // the brush in the theme dictionary collection
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <ResourceDictionary>\r\n"
                L"      <ResourceDictionary.MergedDictionaries>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Green' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"      </ResourceDictionary.MergedDictionaries>\r\n"
                L"      <ResourceDictionary.ThemeDictionaries>\r\n"
                L"        <ResourceDictionary x:Key='Light'>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Yellow' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"        <ResourceDictionary x:Key='Dark'>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Blue' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"      </ResourceDictionary.ThemeDictionaries>\r\n"
                L"    </ResourceDictionary>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Ellipse x:Name='ellipse' Fill='{StaticResource a}' />\r\n"
                L"</Grid>\r\n");
            TestRemoveFromSiblingMergedDictionary(content, false, 0);
        }

        void AddRemoveDictionaryItemTests::ErrorOnRemovingFromSiblingDictionary()
        {
            // The ellipse references resource 'a' which is a solid color brush. When we remove brush 'a' from the dictionary, the resolution logic will now find the x:Double
            // in the sibling resource dictionary and this should fail since our resource resolution logic searches through merged dictionaries first before looking in themed
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <ResourceDictionary>\r\n"
                L"      <ResourceDictionary.MergedDictionaries>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <x:Double x:Key='a'>10.22</x:Double>\r\n"
                L"        </ResourceDictionary>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Green' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"      </ResourceDictionary.MergedDictionaries>\r\n"
                L"      <ResourceDictionary.ThemeDictionaries>\r\n"
                L"        <ResourceDictionary x:Key='Light'>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Yellow' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"        <ResourceDictionary x:Key='Dark'>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Blue' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"      </ResourceDictionary.ThemeDictionaries>\r\n"
                L"    </ResourceDictionary>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Ellipse x:Name='ellipse' Fill='{StaticResource a}' />\r\n"
                L"</Grid>\r\n");
            TestRemoveFromSiblingMergedDictionary(content, true, 1);
        }

        void AddRemoveDictionaryItemTests::TestRemoveFromSiblingMergedDictionary(Platform::String^ content, bool expectError, int mergedDictionaryIndex)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");

            auto ellipseA = callback->GetElementByName(L"ellipse");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetColorProperty(ellipseA.Handle, L"Fill", BaseValueSourceLocal));
            InstanceHandle mergedDictionaryHandle = std::stoll(mergedDictionaries.Elements[mergedDictionaryIndex].Value);
            RemoveDictionaryItem(mergedDictionaryHandle, L"a");

            if (expectError)
            {
                VERIFY_IS_TRUE(callback->HasError());
                auto errors = callback->GetErrorsForElement(ellipseA.Handle);
                VERIFY_ARE_EQUAL(1u, errors.size());
                VERIFY_ARE_EQUAL(VisualElementState::ErrorInvalidResource, errors[0].State);
            }
            else
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(ellipseA.Handle, L"Fill", BaseValueSourceLocal));
            }
        }

        void AddRemoveDictionaryItemTests::ReresolveResourceFromElementStyleProperty()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"   <Color x:Key='c1'>Pink</Color>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Grid x:Name='parent'>\r\n"
                L"    <Button Content='Hello World' Click='Button_Click' x:Name='button'>\r\n"
                L"      <Button.Background>\r\n"
                L"        <SolidColorBrush Color='{StaticResource c1}' x:Name='buttonbackground'/>\r\n"
                L"      </Button.Background>\r\n"
                L"    </Button>\r\n"
                L"    <Button x:Name='button2' Background='{Binding ElementName=button, Path=Background}'/>\r\n"
                L"  </Grid>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto button = callback->GetElementByName(L"button");
            auto button2 = callback->GetElementByName(L"button2");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button2.Handle, L"Background", BaseValueSourceLocal));

            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto blueC1 = CreateInstance(L"Windows.UI.Color", L"Blue");
            AddDictionaryItem(resources, L"c1", blueC1);
            VERIFY_IS_FALSE(callback->HasError());
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(button2.Handle, L"Background", BaseValueSourceLocal));

            RemoveDictionaryItem(resources, L"c1");
            VERIFY_IS_FALSE(callback->HasError());
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Pink, GetColorProperty(button2.Handle, L"Background", BaseValueSourceLocal));
        }

        void AddRemoveDictionaryItemTests::TestImplicitStyleSetterDependency()
        {
            auto content = ref new Platform::String(
                L"<Grid x:Name='grandparent' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"   <SolidColorBrush x:Key='MyBrush' Color='Yellow'/>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Grid x:Name='parent'>\r\n"
                L"    <Grid.Resources>\r\n"
                L"      <SolidColorBrush x:Key='MyBrush' Color='Red' />\r\n"
                L"      <Style TargetType='Button'>\r\n"
                L"        <Setter Property='Background' Value='{StaticResource MyBrush}' />\r\n"
                L"      </Style>\r\n"
                L"      <Style x:Key='ExplicitStyle' TargetType='Button'>\r\n"
                L"        <Setter Property='Background' Value='Purple' />\r\n"
                L"      </Style>\r\n"
                L"    </Grid.Resources>\r\n"
                L"    <Button x:Name='implicitButton' Content='Hello' />\r\n"
                L"    <Button x:Name='explicitButton' Style='{StaticResource ExplicitStyle}' Content='World' />\r\n"
                L"  </Grid>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            // Verify the initial button colors.
            auto implicitButton = callback->GetElementByName(L"implicitButton");
            auto explicitButton = callback->GetElementByName(L"explicitButton");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetColorProperty(implicitButton.Handle, L"Background", BaseValueSourceStyle));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Purple, GetColorProperty(explicitButton.Handle, L"Background", BaseValueSourceStyle));

            // Try removing the brush referenced by the implicit style.  It should reresolve to the Yellow brush in the parent.
            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            RemoveDictionaryItem(resources, L"MyBrush");
            VERIFY_IS_FALSE(callback->HasError());

            // Check that the implicitly styled button has changed, but the explicitly styled one hasn't.
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Yellow, GetColorProperty(implicitButton.Handle, L"Background", BaseValueSourceStyle));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Purple, GetColorProperty(explicitButton.Handle, L"Background", BaseValueSourceStyle));

            // Lastly, remove the Yellow brush and verify the implicit button doesn't have a Background BaseValueSourceStyle.
            auto grandparent = callback->GetElementByName(L"grandparent");
            auto gpResources = GetProperty(grandparent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            RemoveDictionaryItem(gpResources, L"MyBrush");

            // Removing the grandparent's brush should trigger an error since the Setter in the implicit style can't resolve anymore
            VERIFY_IS_TRUE(callback->HasError());

            // Verify the explicit style still works
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Purple, GetColorProperty(explicitButton.Handle, L"Background", BaseValueSourceStyle));
        }

        void AddRemoveDictionaryItemTests::TestImplicitStyleSetterDependencyAppResources()
        {
            auto content = ref new Platform::String(
                L"<Grid x:Name='grandparent' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <Grid x:Name='parent'>\r\n"
                L"    <Button x:Name='implicitButton' Content='Hello' />\r\n"
                L"  </Grid>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            // Verify the initial button colors.
            auto implicitButton = callback->GetElementByName(L"implicitButton");

            // Create the Style that will be put in the App's Resources, initially with a fixed value of Red instead of a StaticResource
            auto buttonStyle = CreateStyle(L"Microsoft.UI.Xaml.Controls.Button");

            auto backgroundSetter = CreateSetterWithProperty(CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red"), L"Microsoft.UI.Xaml.Controls.Control.Background");

            AddSetterToStyle(buttonStyle, backgroundSetter);

            auto appResources = GetApplicationResources();
            AddImplicitStyle(appResources, buttonStyle);

            // We have to remove this from application resources otherwise in stress mode trying to add it again will fail.
            auto removeFromApp = wil::scope_exit([&] {
                RemoveImplicitStyle(appResources, L"Microsoft.UI.Xaml.Controls.Button");
            });

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetColorProperty(implicitButton.Handle, L"Background", BaseValueSourceStyle));

            // Create/add the brush that the implicit style will use to App's resources
            auto yellowBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Yellow");
            AddDictionaryItem(appResources, L"MyBrush", yellowBrush);

            auto removeFromApp2 = wil::scope_exit([&] {
                RemoveDictionaryItem(appResources, L"MyBrush");
            });

            // Have the implicit style setter resolve to MyBrush instead, verify the implicitly styled button has changed
            ResolveResource(backgroundSetter, L"MyBrush", GetPropertyIndex(backgroundSetter, L"Microsoft.UI.Xaml.Setter.Value"), ResourceTypeStatic);
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Yellow, GetColorProperty(implicitButton.Handle, L"Background", BaseValueSourceStyle));

            // Now remove the Yellow brush and verify we have an error
            RemoveDictionaryItem(appResources, L"MyBrush");
            VERIFY_IS_TRUE(callback->HasError());
        }

        void AddRemoveDictionaryItemTests::AddRemoveImplicitStyleSetters()
        {
            auto content = ref new Platform::String(
                L"<Grid x:Name='grandparent' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"   <SolidColorBrush x:Key='MyBrush' Color='Yellow'/>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Grid x:Name='parent'>\r\n"
                L"    <Grid.Resources>\r\n"
                L"      <Style x:Key='ExplicitStyle' TargetType='Button'>\r\n"
                L"        <Setter Property='Height' Value='75' />\r\n"
                L"        <Setter Property='Background' Value='Orange' />\r\n"
                L"      </Style>\r\n"
                L"    </Grid.Resources>\r\n"
                L"    <Button x:Name='implicitButton' Content='Hello' />\r\n"
                L"    <Button x:Name='explicitButton' Style='{StaticResource ExplicitStyle}' Content='World' />\r\n"
                L"  </Grid>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto implicitButton = callback->GetElementByName(L"implicitButton");
            auto explicitButton = callback->GetElementByName(L"explicitButton");
            auto parent = callback->GetElementByName(L"parent");

            // Create the implicit style that will be initially empty when placed in the element's dictionary, then modified
            // We need to get the Setters collection before adding it to the dictionary/visual tree so that we fault in
            // an empty Setters collection.  If we try to get Setters after the Style was sealed from being added to a dictionary, we'll fail
            // because the property system tries to create an empty collection and assign it to the sealed Style
            auto implicitStyle = CreateStyle(L"Microsoft.UI.Xaml.Controls.Button");
            auto implicitSetters = GetCollectionProperty(implicitStyle, L"Microsoft.UI.Xaml.Style.Setters");
            AddImplicitStyleToElementResources(parent, implicitStyle);

            //Try adding a new Setter to the implicit style, and make sure the implicit styled button updates while the explicitly styled button doesn't
            auto backgroundSetter = CreateSetterWithProperty(CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red"), L"Microsoft.UI.Xaml.Controls.Control.Background");

            AddSetterToStyle(implicitStyle, backgroundSetter);

            VERIFY_IS_FALSE(callback->HasError());
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetColorProperty(implicitButton.Handle, L"Background", BaseValueSourceStyle));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, GetColorProperty(explicitButton.Handle, L"Background", BaseValueSourceStyle));

            // Try removing the setter we just added, which will be the second Setter in the implicit style
            LogThrow_IfFailedWithMessage(m_tap->RemoveChild(implicitSetters.Handle, 0), L"Failed removing setter from collection");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, GetColorProperty(explicitButton.Handle, L"Background", BaseValueSourceStyle));
        }


        ref class MyType sealed
        {
        public:
            MyType()
            {
            }
        };

        void AddRemoveDictionaryItemTests::CanAddNonDOToDictionary()
        {
            auto content = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"  </Grid.Resources>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto root = callback->GetElementByName(L"root");

            auto newType = ref new MyType();
            auto newTypeHandle = ih_cast(newType);
            VERIFY_NO_THROW(AddItemToElementResources(root, L"MyType", newTypeHandle));
        }

        void AddRemoveDictionaryItemTests::CanAddRemoveNullValues()
        {
            auto content = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <NullExtension x:Key='Null1'/>\r\n"
                L"  </Grid.Resources>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto root = callback->GetElementByName(L"root");

            const InstanceHandle nullHandle = 0u;
            VERIFY_NO_THROW(AddItemToElementResources(root, L"Null2", nullHandle));
            VERIFY_NO_THROW(RemoveItemFromElementResources(root, L"Null1"));
            VERIFY_NO_THROW(RemoveItemFromElementResources(root, L"Null2"));
        }

        void AddRemoveDictionaryItemTests::VerifyAddRemoveProperlyUpdates()
        {
            auto executeTest = [&](wrl::ComPtr<VisualTreeServiceCallback> callback) {
                LOG_OUTPUT(L"Validating the outside template case");
                {
                    LOG_OUTPUT(L">> Removing style");
                    auto borderOutsideTemplate = callback->GetElementByName(L"borderOutsideTemplate");

                    RemoveItemFromElementResources(borderOutsideTemplate, L"OutsideTemplate");
                    VERIFY_IS_TRUE(callback->HasError());
                    auto outerStyle = CreateStyle(L"Microsoft.UI.Xaml.Shapes.Rectangle");
                    auto outerFillSetter = CreateSetterWithProperty(0, L"Microsoft.UI.Xaml.Shapes.Shape.Fill");
                    AddSetterToStyle(outerStyle, outerFillSetter);
                    LOG_OUTPUT(L">> Adding style back");
                    AddItemToElementResources(borderOutsideTemplate, L"OutsideTemplate", outerStyle);
                    VERIFY_IS_FALSE(callback->HasError());
                }

                LOG_OUTPUT(L"Validating the inside template case");
                {
                    LOG_OUTPUT(L">> Removing style from first template instance");
                    auto borderInsideTemplate = callback->GetElementByName(L"borderInsideTemplate");
                    RemoveItemFromElementResources(borderInsideTemplate, L"InsideTemplate");
                    VERIFY_IS_TRUE(callback->HasError());

                    // For items in templates, we have to explicitly remove it from each instantiation
                    VERIFY_ARE_EQUAL(1u, callback->GetErrorCount());

                    LOG_OUTPUT(L">> Removing style from second template instance");
                    const int skipFirst = 1;
                    auto borderInsideTemplate2 = callback->GetElementByName(L"borderInsideTemplate", skipFirst);
                    RemoveItemFromElementResources(borderInsideTemplate2, L"InsideTemplate");
                    VERIFY_ARE_EQUAL(2u, callback->GetErrorCount());

                    LOG_OUTPUT(L">> Adding style back to first template instance");
                    {
                        auto innerStyle = CreateStyle(L"Microsoft.UI.Xaml.Shapes.Rectangle");
                        auto innerFillSetter = CreateSetterWithProperty(0, L"Microsoft.UI.Xaml.Shapes.Shape.Fill");
                        AddSetterToStyle(innerStyle, innerFillSetter);

                        AddItemToElementResources(borderInsideTemplate, L"InsideTemplate", innerStyle);
                        VERIFY_ARE_EQUAL(1u, callback->GetErrorCount());
                    }

                    LOG_OUTPUT(L">> Adding style back to second template instance");
                    {
                        auto innerStyle = CreateStyle(L"Microsoft.UI.Xaml.Shapes.Rectangle");
                        auto innerFillSetter = CreateSetterWithProperty(0, L"Microsoft.UI.Xaml.Shapes.Shape.Fill");
                        AddSetterToStyle(innerStyle, innerFillSetter);

                        AddItemToElementResources(borderInsideTemplate2, L"InsideTemplate", innerStyle);
                        VERIFY_IS_FALSE(callback->HasError());
                    }
                }
            };

            {
                LOG_OUTPUT(L"Executing XamlReader.Load scenario");
                auto content = ref new Platform::String(
                    L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                    L"  <Grid x:Name='Grid1'>\r\n"
                    L"   <Grid.Resources>\r\n"
                    L"    <ControlTemplate x:Key='T1' TargetType='Button'>\r\n"
                    L"      <Border x:Name='borderInsideTemplate'>\r\n"
                    L"        <Border.Resources>\r\n"
                    L"          <SolidColorBrush x:Key='MyBrush' Color='Red' />\r\n"
                    L"          <Style x:Key='InsideTemplate' TargetType='Rectangle'>\r\n"
                    L"            <Setter Property='Fill' Value='{StaticResource MyBrush}' />\r\n"
                    L"          </Style>\r\n"
                    L"        </Border.Resources>\r\n"
                    L"        <Rectangle x:Name='rectInsideTemplate' Width='200' Height='50' Style='{StaticResource InsideTemplate}'/>\r\n"
                    L"      </Border>\r\n"
                    L"    </ControlTemplate>\r\n"
                    L"   </Grid.Resources>\r\n"
                    L"   <Button Width='200' Height='50' Template='{StaticResource T1}' />\r\n"
                    L"   <Button Width='200' Height='50' Template='{StaticResource T1}' />\r\n"
                    L"  </Grid>\r\n"
                    L"  <Border x:Name='borderOutsideTemplate'>\r\n"
                    L"    <Border.Resources>\r\n"
                    L"      <SolidColorBrush x:Key='MyBrush' Color='Green' />\r\n"
                    L"      <Style x:Key='OutsideTemplate' TargetType='Rectangle'>\r\n"
                    L"        <Setter Property='Fill' Value='{StaticResource MyBrush}' />\r\n"
                    L"      </Style>\r\n"
                    L"    </Border.Resources>\r\n"
                    L"    <Rectangle x:Name='rectOutsideTemplate' Width='200' Height='50' Style='{StaticResource OutsideTemplate}' />\r\n"
                    L"  </Border>\r\n"
                    L"</StackPanel>\r\n");
                wrl::ComPtr<VisualTreeServiceCallback> callback;
                auto cleanup = m_connectionHelper->Advise(content, callback);
                executeTest(callback);
            }

            {
                LOG_OUTPUT(L"Executing Application.LoadComponent scenario");
                auto componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/SimpleTemplateResolutionPage.xaml");
                wrl::ComPtr<VisualTreeServiceCallback> callback;
                auto cleanup = m_connectionHelper->Advise(componentLocation, callback);
                LOG_OUTPUT(L"");
                executeTest(callback);
            }
        }

        void AddRemoveDictionaryItemTests::ValidateCanReuseKeys()
        {
            // Visual Studio reuses keys. Make sure that we keep keys around
            auto content = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <Style TargetType='Rectangle' x:Key='RectStyle'>\r\n"
                L"      <Setter Property='Fill' Value='Red' /> \r\n"
                L"    </Style>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Rectangle Width='100' Fill='Red' x:Name='rect'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto root = callback->GetElementByName(L"root");
            auto resources = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto key = CreateInstance(L"Windows.Foundation.String", L"RectStyle");

            // What VS does is weird, they call RemoveDictionaryItem before they call AddDictionaryItem
            VERIFY_NO_THROW(m_tap->RemoveDictionaryItem(resources, key));

            // Our tap doesn't make it possible to add a non-implicit style key without using a string. So just validate
            // that we can get it from GetIInspectableFromHandle.
            wrl::ComPtr<IInspectable> keyInsp;
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(key, &keyInsp));
        }

        void AddRemoveDictionaryItemTests::ValidateCanRenameItems()
        {
             auto content = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <SolidColorBrush Color='Red' x:Key='B0' />\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Button Content='Hello world' Width='100' Background='{StaticResource B0}' x:Name='button'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto root = callback->GetElementByName(L"root");
            RenameItemInElementResources(root, L"B0", L"Invalid");

            VERIFY_IS_TRUE(callback->HasError());

            RenameItemInElementResources(root, L"Invalid", L"B0");

            VERIFY_IS_FALSE(callback->HasError());
        }

        void AddRemoveDictionaryItemTests::VerifyRemoveThicknessResourceUsedByStyleSetter()
        {
            auto content = ref new Platform::String(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <StackPanel.Resources>\r\n"
                L"    <Thickness x:Key='T1'>3</Thickness>\r\n"
                L"    <Style TargetType='Button'>\r\n"
                L"      <Setter Property='Margin' Value='{StaticResource T1}'/>\r\n"
                L"    </Style>\r\n"
                L"  </StackPanel.Resources>\r\n"
                L"  <Button Content='Hello'/>\r\n"
                L"</StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto root = callback->GetElementByName(L"root");

            VERIFY_NO_THROW(RemoveItemFromElementResources(root, L"T1"));
        }

        #pragma endregion

    }
} } } } }
