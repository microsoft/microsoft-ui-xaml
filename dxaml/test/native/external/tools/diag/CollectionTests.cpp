// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "CollectionTests.h"
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

        bool CollectionTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool CollectionTests::ClassCleanup()
        {
            return true;
        }

        bool CollectionTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());
            return EnsureTapLoaded();
        }

        bool CollectionTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            test_infra::TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void CollectionTests::TestCollections()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto grid = callback->GetElementByName(L"root");

            wil::unique_propertychainvalue children = GetPropertyChainValue(grid.Handle, L"Children");

            // Verify correct metadata
            VERIFY_IS_TRUE((children.MetadataBits & MetadataBit::IsValueCollection) == MetadataBit::IsValueCollection);

            InstanceHandle childrenHandle = std::stoll(children.Value);

            // Test count.
            unsigned int count = 0;
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(childrenHandle, &count));
            VERIFY_ARE_EQUAL(count, 4u);

            // Get elements.
            CoTaskMemPtr<CollectionElementValue> pElements;
            VERIFY_SUCCEEDED(m_tap->GetCollectionElements(childrenHandle, 0, &count, &pElements));
            VERIFY_ARE_EQUAL(count, 4u);

            // Remove element.
            VERIFY_SUCCEEDED(m_tap->RemoveChild(childrenHandle, 0));
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(childrenHandle, &count));
            VERIFY_ARE_EQUAL(count, 3u);

            // Add element.
            InstanceHandle newInstance = CreateInstance(L"Microsoft.UI.Xaml.Shapes.Rectangle");
            VERIFY_ARE_NOT_EQUAL(newInstance, 0);

            VERIFY_SUCCEEDED(m_tap->AddChild(childrenHandle, newInstance, 0));
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(childrenHandle, &count));
            VERIFY_ARE_EQUAL(count, 4u);

            // Clear.
            VERIFY_SUCCEEDED(m_tap->ClearChildren(childrenHandle));
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(childrenHandle, &count));
            VERIFY_ARE_EQUAL(count, 0u);

            TestServices::WindowHelper->WaitForIdle();
        }

        void CollectionTests::TestNestedCollections()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto grid = callback->GetElementByName(L"root");

            wil::unique_propertychainvalue children = GetPropertyChainValue(grid.Handle, L"Children");

            // Verify correct metadata
            VERIFY_IS_TRUE((children.MetadataBits & MetadataBit::IsValueCollection) == MetadataBit::IsValueCollection);
            InstanceHandle childrenHandle = std::stoll(children.Value);

            // Test count.
            unsigned int count = 0;
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(childrenHandle, &count));
            VERIFY_ARE_EQUAL(count, 4u);

            // Get elements.
            CoTaskMemPtr<CollectionElementValue> pElements;
            VERIFY_SUCCEEDED(m_tap->GetCollectionElements(childrenHandle, 0, &count, &pElements));
            VERIFY_ARE_EQUAL(count, 4u);

            // Get the child grid that is contained in this grid.
            CollectionElementValue childGrid = { 0 };
            for (unsigned int i = 0; i < count; i++)
            {
                if (wcscmp(pElements[i].ValueType, L"Microsoft.UI.Xaml.Controls.Grid") == 0)
                {
                    childGrid = pElements[i];
                    break;
                }
            }

            wil::unique_propertychainvalue nestedChildren = GetPropertyChainValue(std::stoll(childGrid.Value), L"Children");

            // Verify correct metadata
            VERIFY_IS_TRUE((nestedChildren.MetadataBits & MetadataBit::IsValueCollection) == MetadataBit::IsValueCollection);

            // Get the count of the children. There is only the rectangle so the count should be one.
            InstanceHandle nestedChildrenHandle = std::stoll(nestedChildren.Value);
            unsigned int childCount = 0;
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(nestedChildrenHandle, &childCount));
            VERIFY_ARE_EQUAL(childCount, 1u);

            // Get elements.
            CoTaskMemPtr<CollectionElementValue> pChildElements;
            VERIFY_SUCCEEDED(m_tap->GetCollectionElements(nestedChildrenHandle, 0, &count, &pChildElements));
            VERIFY_ARE_EQUAL(count, 1u);

            // Let's verify it is the rectangle
            VERIFY_IS_TRUE(wcscmp(pChildElements[0].ValueType, L"Microsoft.UI.Xaml.Shapes.Rectangle") == 0);

            // Remove element.
            VERIFY_SUCCEEDED(m_tap->RemoveChild(nestedChildrenHandle, 0));
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(nestedChildrenHandle, &count));
            VERIFY_ARE_EQUAL(count, 0u);

            // Add element.
            InstanceHandle newInstance = CreateInstance(L"Microsoft.UI.Xaml.Shapes.Rectangle");
            VERIFY_ARE_NOT_EQUAL(newInstance, 0);
            VERIFY_SUCCEEDED(m_tap->AddChild(nestedChildrenHandle, newInstance, 0));
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(nestedChildrenHandle, &count));
            VERIFY_ARE_EQUAL(count, 1u);
        }

        void CollectionTests::TestCommandBarCollectionOperations()
        {
            // Need a way to clear static data from external dlls that hold references to XAML objects
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::simpleListViewAndCommandBarString, callback);

            VisualElement commandBar = callback->GetElementByName(L"commandBar");
            wil::unique_propertychainvalue commandsProperty = GetPropertyChainValue(commandBar.Handle, L"PrimaryCommands", BaseValueSourceLocal);
            InstanceHandle commandsHandle = std::stoll(commandsProperty.Value);

            //Verify the original collection correctly returns 2 elements.
            unsigned int collectionCount = 0;
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(commandsHandle, &collectionCount));
            VERIFY_ARE_EQUAL(collectionCount, 2u);

            xaml_controls::AppBarToggleButton ^newButton;

            RunOnUIThread([&]()
            {
                newButton = ref new xaml_controls::AppBarToggleButton();
            });

            //Insert a new AppBarToggleButton element in front of the 2 AppBarButton elements, verify the collection count is valid
            IInspectable* pButtonInspectable = reinterpret_cast<IInspectable*>(safe_cast<Platform::Object^>(newButton));
            InstanceHandle buttonHandle;
            VERIFY_SUCCEEDED(m_tap->RegisterInstance(pButtonInspectable, &buttonHandle));
            VERIFY_SUCCEEDED(m_tap->AddChild(commandsHandle, buttonHandle, 0));
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(commandsHandle, &collectionCount));
            VERIFY_ARE_EQUAL(collectionCount, 3u);

            //Get the actual collection elements, and verify the first element is the newly inserted AppBarToggleButton
            CoTaskMemPtr<CollectionElementValue> pElements;
            VERIFY_SUCCEEDED(m_tap->GetCollectionElements(commandsHandle, 0, &collectionCount, &pElements));
            VERIFY_ARE_EQUAL(collectionCount, 3u);
            VERIFY_IS_TRUE(wcscmp(pElements.Get()[0].ValueType, L"Microsoft.UI.Xaml.Controls.AppBarToggleButton") == 0);

            //Remove the AppBarToggleButton and verify the count is properly updated
            VERIFY_SUCCEEDED(m_tap->RemoveChild(commandsHandle, 0));
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(commandsHandle, &collectionCount));
            VERIFY_ARE_EQUAL(collectionCount, 2u);

        }

        void CollectionTests::VerifyDontLeakCollection()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback = m_connectionHelper->Advise();
            auto cleanup = XamlDiagnosticsTestHelpers::SetupGridAndWait();

            auto grid = callback->GetElementByName(L"root");

            //Get the ResourceDictionary where the Style is being stored
            unsigned int indexGetPropertyIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(grid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &indexGetPropertyIndex));
            InstanceHandle dictionaryHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(grid.Handle, indexGetPropertyIndex, &dictionaryHandle));

            // verify getting anything other than a resourcedictionary fails
            auto children = GetPropertyChainValue(grid.Handle, L"Children");
            VERIFY_IS_TRUE((children.MetadataBits & MetadataBit::IsValueCollection) == MetadataBit::IsValueCollection);

            InstanceHandle childrenHandle = std::stoll(children.Value);
            auto collectionSource = m_tap->GetPropertyChainSource(childrenHandle, BaseValueSourceDefault);
            auto dictionarySource = m_tap->GetPropertyChainSource(dictionaryHandle, BaseValueSourceDefault);

            m_connectionHelper->OnTestComplete(callback);

            wrl::ComPtr<IInspectable> childrenAsInsp;
            VERIFY_FAILED(m_tap->GetIInspectableFromHandle(childrenHandle, &childrenAsInsp));

            // now see if we can get the resourcedictionary from our cache. verify this object isn't in our map
            wrl::ComPtr<IInspectable> dictionaryAsInp;
            VERIFY_FAILED(m_tap->GetIInspectableFromHandle(dictionaryHandle, &dictionaryAsInp));
        }

        void CollectionTests::TestStyleSetterChangeThroughCollection()
        {
            //Tries removing and adding a Setter to a Style
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::stackPanelWithNestedStylesString, callback);

            //Get the dictionary containing the various Styles.
            VisualElement button = callback->GetElementByName(L"button");
            VisualElement panel = callback->GetElementByName(L"panel");

            auto style2Handle = GetItemFromElementResources(panel, L"style2");

            //Remove the Background Setter from Style2 - the Button should then get the Background value from Style1
            unsigned int settersIndex = 0;
            InstanceHandle settersHandle;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(style2Handle, L"Microsoft.UI.Xaml.Style.Setters", &settersIndex));
            VERIFY_SUCCEEDED(m_tap->GetProperty(style2Handle, settersIndex, &settersHandle));
            VERIFY_SUCCEEDED(m_tap->RemoveChild(settersHandle, 0));

            unsigned int backgroundIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background", &backgroundIndex));
            InstanceHandle backgroundHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(button.Handle, backgroundIndex, &backgroundHandle));
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(backgroundHandle, L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Blue") == 0);

            //Create a new Background Setter with an Orange value

            //Create the Setter itself
            InstanceHandle setterHandle = CreateInstance(L"Microsoft.UI.Xaml.Setter");

            //Create the DependencyProperty for the Property
            InstanceHandle setterProperty = CreateInstance(L"Microsoft.UI.Xaml.DependencyProperty", L"Microsoft.UI.Xaml.Controls.Control.Background");

            //Create the new Brush for the Setter's Value
            InstanceHandle setterValue = CreateInstance(L"Microsoft.UI.Xaml.Media.SolidColorBrush", L"Orange");

            //Get the property indices for the Property/Value properties
            unsigned int propertyIndex = 0;
            unsigned int valueIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(setterHandle, L"Microsoft.UI.Xaml.Setter.Property", &propertyIndex));
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(setterHandle, L"Microsoft.UI.Xaml.Setter.Value", &valueIndex));

            //Put the values into the Setter, and put the Setter into the Setters collection
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(setterHandle, setterProperty, propertyIndex));
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(setterHandle, setterValue, valueIndex));
            VERIFY_SUCCEEDED(m_tap->AddChild(settersHandle, setterHandle, 0));

            //Verify the Button's color is now Orange
            VERIFY_SUCCEEDED(m_tap->GetProperty(button.Handle, backgroundIndex, &backgroundHandle));
            colorProperty = GetPropertyChainValue(backgroundHandle, L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Orange") == 0);

            //Clear Style2's Setter collection and verify the button's color is Blue again
            VERIFY_SUCCEEDED(m_tap->ClearChildren(settersHandle));
            VERIFY_SUCCEEDED(m_tap->GetProperty(button.Handle, backgroundIndex, &backgroundHandle));
            colorProperty = GetPropertyChainValue(backgroundHandle, L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Blue") == 0);
        }

        void CollectionTests::TestNonDependencyObjectCollection()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/MainPage.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement map = callback->GetElementByName(L"customControl");

            wil::unique_propertychainvalue childrenProperty = GetPropertyChainValue(map.Handle, L"CustomUserCollection", BaseValueSourceLocal);
            InstanceHandle childrenHandle = std::stoll(childrenProperty.Value);

            //Verify the original collection correctly returns 2 elements.
            unsigned int collectionCount = 0;
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(childrenHandle, &collectionCount));
            VERIFY_ARE_EQUAL(collectionCount, 2u);

            //Insert a new rectangle into the collection.
            InstanceHandle newRectHandle = CreateInstance(L"Microsoft.UI.Xaml.Shapes.Rectangle");
            VERIFY_SUCCEEDED(m_tap->AddChild(childrenHandle, newRectHandle, 0));
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(childrenHandle, &collectionCount));
            VERIFY_ARE_EQUAL(collectionCount, 3u);

            //Get the actual collection elements, and verify the first element is the newly inserted Rectangle.
            //We only need to check the type since the existing elements are SolidColorBrushes.
            CoTaskMemPtr<CollectionElementValue> pElements;
            VERIFY_SUCCEEDED(m_tap->GetCollectionElements(childrenHandle, 0, &collectionCount, &pElements));
            VERIFY_ARE_EQUAL(collectionCount, 3u);
            VERIFY_IS_TRUE(wcscmp(pElements.Get()[0].ValueType, L"Microsoft.UI.Xaml.Shapes.Rectangle") == 0);

            //Remove the Rectangle and verify the count is properly updated
            VERIFY_SUCCEEDED(m_tap->RemoveChild(childrenHandle, 0));
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(childrenHandle, &collectionCount));
            VERIFY_ARE_EQUAL(collectionCount, 2u);

            //Clear the collection and verify the count is now zero.
            VERIFY_SUCCEEDED(m_tap->ClearChildren(childrenHandle));
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(childrenHandle, &collectionCount));
            VERIFY_ARE_EQUAL(collectionCount, 0u);
        }

        void CollectionTests::CanFindNamedElementAddedToCollectionInTemplate()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement parent = callback->GetElementByName(L"InsetBackground");
            auto children = GetCollectionProperty(parent.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Children");

            InstanceHandle button = CreateControlWithBackground(L"Button", L"Yellow", L"Foo");
            VERIFY_SUCCEEDED(m_tap->AddChild(children.Handle, button, children.Count));

             TestServices::WindowHelper->WaitForIdle();

             RunOnUIThread([&] {
                 auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                 auto button = safe_cast<Button^>(page->FindName("visualStateTemplateTest"));
                 auto foo = safe_cast<Button^>(button->FindName("Foo"));
                 VERIFY_IS_NOT_NULL(foo);
             });
        }

        void CollectionTests::VerifyRowDefinitionsReportAsCollections()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto rootGrid = callback->GetElementByName(L"root");

            auto rowDefs = GetPropertyChainValue(rootGrid.Handle, L"RowDefinitions", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(rowDefs.MetadataBits & MetadataBit::IsValueCollection, MetadataBit::IsValueCollection);
            auto colDefs = GetPropertyChainValue(rootGrid.Handle, L"ColumnDefinitions", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(colDefs.MetadataBits & MetadataBit::IsValueCollection, MetadataBit::IsValueCollection);
        }

        void CollectionTests::RemovingMergedDictionaryUpdatesReferences()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);
            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto button = callback->GetElementByName(L"buttonOverwritten");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            VERIFY_IS_GREATER_THAN(mergedDictionaries.Count, 0u);

            // Remove the second item, the background should be updated to Red now
            VERIFY_SUCCEEDED(m_tap->RemoveChild(mergedDictionaries.Handle, 1u));
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
        }

        void CollectionTests::ClearingMergedDictionariesUpdatesReferences()
        {
            auto content = ref new Platform::String(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
            L"  <Grid.Resources>\r\n"
            L"    <ResourceDictionary>\r\n"
            L"      <ResourceDictionary.MergedDictionaries>\r\n"
            L"        <ResourceDictionary>\r\n"
            L"          <SolidColorBrush x:Key='a' Color='Red'/>\r\n"
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
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto ellipse = callback->GetElementByName(L"ellipse");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetColorProperty(ellipse.Handle, L"Fill", BaseValueSourceLocal));

            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            VERIFY_IS_GREATER_THAN(mergedDictionaries.Count, 0u);

            // Clear the merged dictionary, the background should now be blue
            VERIFY_SUCCEEDED(m_tap->ClearChildren(mergedDictionaries.Handle));
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(ellipse.Handle, L"Fill", BaseValueSourceLocal));
        }

        void CollectionTests::AddingMergedDictionaryUpdatesReferences()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);
            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto button = callback->GetElementByName(L"buttonOverwritten");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            VERIFY_IS_GREATER_THAN(mergedDictionaries.Count, 0u);

            LOG_OUTPUT(L"Adding dictionary to back of collection");
            auto newDictionary = CreateInstance(L"Microsoft.UI.Xaml.ResourceDictionary");
            auto newBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Brown");
            AddDictionaryItem(newDictionary, L"testOverwritten", newBrush);

            // Add an item after the 2nd, so this should cause the button to take a new background color
            VERIFY_SUCCEEDED(m_tap->AddChild(mergedDictionaries.Handle, newDictionary, 2u));
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Brown, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            LOG_OUTPUT(L"Adding merged dictionary to front of collection");
            newDictionary = CreateInstance(L"Microsoft.UI.Xaml.ResourceDictionary");
            newBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Magenta");
            AddDictionaryItem(newDictionary, L"testOverwritten", newBrush);
            VERIFY_SUCCEEDED(m_tap->AddChild(mergedDictionaries.Handle, newDictionary, 0u));
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Verifying color hasn't changed");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Brown, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
        }

        void CollectionTests::AddingMergedDictionaryDoesntOverrideParent()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <ResourceDictionary>\r\n"
                L"      <ResourceDictionary.MergedDictionaries>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Red'/>\r\n"
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
                L"      <SolidColorBrush x:Key='a' Color='Black'/>\r\n"
                L"    </ResourceDictionary>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Ellipse x:Name='ellipse' Fill='{StaticResource a}' />\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto ellipse = callback->GetElementByName(L"ellipse");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(ellipse.Handle, L"Fill", BaseValueSourceLocal));

            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            VERIFY_IS_GREATER_THAN(mergedDictionaries.Count, 0u);

            LOG_OUTPUT(L"Adding dictionary to back of collection");
            auto newDictionary = CreateInstance(L"Microsoft.UI.Xaml.ResourceDictionary");
            auto newBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Brown");
            AddDictionaryItem(newDictionary, L"a", newBrush);

            // Add an item after the 2nd, so this should cause the button to take a new background color
            VERIFY_SUCCEEDED(m_tap->AddChild(mergedDictionaries.Handle, newDictionary, 1u));
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Verifying color hasn't changed");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(ellipse.Handle, L"Fill", BaseValueSourceLocal));

            LOG_OUTPUT(L"Adding merged dictionary to front of collection");
            newDictionary = CreateInstance(L"Microsoft.UI.Xaml.ResourceDictionary");
            newBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Magenta");
            AddDictionaryItem(newDictionary, L"a", newBrush);
            VERIFY_SUCCEEDED(m_tap->AddChild(mergedDictionaries.Handle, newDictionary, 0u));
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Verifying color hasn't changed");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(ellipse.Handle, L"Fill", BaseValueSourceLocal));
        }

        void CollectionTests::RemovingMergedDictionaryDoesntOverrideParent()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <ResourceDictionary>\r\n"
                L"      <ResourceDictionary.MergedDictionaries>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <SolidColorBrush x:Key='a' Color='Red'/>\r\n"
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
                L"      <SolidColorBrush x:Key='a' Color='Black'/>\r\n"
                L"    </ResourceDictionary>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Ellipse x:Name='ellipse' Fill='{StaticResource a}' />\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto ellipse = callback->GetElementByName(L"ellipse");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(ellipse.Handle, L"Fill", BaseValueSourceLocal));

            auto mergedDictionaries = GetCollectionProperty(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries");
            VERIFY_IS_GREATER_THAN(mergedDictionaries.Count, 0u);

            // Clear the merged dictionary, the background should still be black
            LOG_OUTPUT(L"Removing merged dictionary, reference shouldn't change");
            VERIFY_SUCCEEDED(m_tap->RemoveChild(mergedDictionaries.Handle, 0u));
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, GetColorProperty(ellipse.Handle, L"Fill", BaseValueSourceLocal));
        }

        void CollectionTests::ClearingSettersUpdatesApp()
        {
            auto content = ref new Platform::String(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='parent'>\r\n"
                L"  <StackPanel.Resources>\r\n"
                L"    <Style TargetType='TextBox'>\r\n"
                L"      <Setter Property='Foreground' Value='Orange'/>\r\n"
                L"      <Setter Property='Width' Value='10'/>\r\n"
                L"    </Style>\r\n"
                L"    <Style TargetType='TextBox' x:Key='NonImplicit'>\r\n"
                L"      <Setter Property='Foreground' Value='Red'/>\r\n"
                L"      <Setter Property='Width' Value='100'/>\r\n"
                L"    </Style>\r\n"
                L"  </StackPanel.Resources>\r\n"
                L"  <TextBox x:Name='implicit' Text='Im the implicit box' />\r\n"
                L"  <TextBox x:Name='nonimplicit' Style='{StaticResource NonImplicit}' Text='Im the non-implicit box' />\r\n"
                L"</StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto parent = callback->GetElementByName(L"parent");

            auto implicitTextBoxElement = callback->GetElementByName(L"implicit");
            auto implicitTextBox = ih_cast<TextBox>(implicitTextBoxElement.Handle);

            auto nonimplicitTextBoxElement = callback->GetElementByName(L"nonimplicit");
            auto nonimplicitTextBox = ih_cast<TextBox>(nonimplicitTextBoxElement.Handle);

            LOG_OUTPUT(L"Validating state before");
            RunOnUIThread([&] {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, safe_cast<SolidColorBrush^>(implicitTextBox->Foreground)->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, safe_cast<SolidColorBrush^>(nonimplicitTextBox->Foreground)->Color);

                VERIFY_ARE_EQUAL(10, (int)implicitTextBox->Width);
                VERIFY_ARE_EQUAL(100, (int)nonimplicitTextBox->Width);
            });

            LOG_OUTPUT(L"Clearing implicit style collection");
            auto implicitStyle = GetImplicitStyleFromElementResources(parent, L"Microsoft.UI.Xaml.Controls.TextBox");
            ClearCollection(implicitStyle, L"Microsoft.UI.Xaml.Style.Setters");

            RunOnUIThread([&] {
                VERIFY_ARE_NOT_EQUAL(Microsoft::UI::Colors::Orange, safe_cast<SolidColorBrush^>(implicitTextBox->Foreground)->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, safe_cast<SolidColorBrush^>(nonimplicitTextBox->Foreground)->Color);

                VERIFY_ARE_NOT_EQUAL(10, (int)implicitTextBox->Width);
                VERIFY_ARE_EQUAL(100, (int)nonimplicitTextBox->Width);
            });

            LOG_OUTPUT(L"Clearing non-implicit style collection");
            auto nonimplicitStyle = GetItemFromElementResources(parent, L"NonImplicit");
            ClearCollection(nonimplicitStyle, L"Microsoft.UI.Xaml.Style.Setters");

            RunOnUIThread([&] {
                VERIFY_ARE_NOT_EQUAL(Microsoft::UI::Colors::Orange, safe_cast<SolidColorBrush^>(implicitTextBox->Foreground)->Color);
                VERIFY_ARE_NOT_EQUAL(Microsoft::UI::Colors::Red, safe_cast<SolidColorBrush^>(nonimplicitTextBox->Foreground)->Color);

                VERIFY_ARE_NOT_EQUAL(10, (int)implicitTextBox->Width);
                VERIFY_ARE_NOT_EQUAL(100, (int)nonimplicitTextBox->Width);
            });
        }

        void CollectionTests::RemovingSettersFromImplicitStyleUpdatesApp()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <Style TargetType='TextBox'>\r\n"
                L"      <Setter Property='Foreground' Value='Orange'/>\r\n"
                L"      <Setter Property='Width' Value='10'/>\r\n"
                L"    </Style>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <TextBox x:Name='text' Text='This text should change colors'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto parent = callback->GetElementByName(L"parent");

            auto textBoxElement = callback->GetElementByName(L"text");
            auto textBox = ih_cast<TextBox>(textBoxElement.Handle);

            RunOnUIThread([&] {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, safe_cast<SolidColorBrush^>(textBox->Foreground)->Color);
            });

            auto implicitStyle = GetImplicitStyleFromElementResources(parent, L"Microsoft.UI.Xaml.Controls.TextBox");
            RemoveFromCollection(implicitStyle, L"Microsoft.UI.Xaml.Style.Setters", 0);

            RunOnUIThread([&] {
                VERIFY_ARE_NOT_EQUAL(Microsoft::UI::Colors::Orange, safe_cast<SolidColorBrush^>(textBox->Foreground)->Color);
            });
        }

        void CollectionTests::AddingMergedDictionaryUpdatesReferences2()
        {
            // Test runs in DesignMode V2 so we don't crash and instead get the callback

            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"      <ResourceDictionary>\r\n"
                L"          <Style TargetType='ListViewItem' x:Key='ListViewItemStyle'>\r\n"
                L"              <Setter Property='Width' Value='200'/>\r\n"
                L"              <Setter Property='Height' Value='200'/>\r\n"
                L"          </Style>\r\n"
                L"          <ResourceDictionary.MergedDictionaries>\r\n"
                L"              <ResourceDictionary>\r\n"
                L"                  <Style TargetType='TextBlock' x:Key='TextBlockStyle'>\r\n"
                L"                      <Setter Property='Width' Value='200'/>\r\n"
                L"                      <Setter Property='Height' Value='200'/>\r\n"
                L"                  </Style>\r\n"
                L"              </ResourceDictionary>\r\n"
              //L"             <ResourceDictionary>\r\n"
              //L"                  <Style TargetType='Button' x:Key='TextBlockStyle'>\r\n"
              //L"                      <Setter Property='Width' Value='200'/>\r\n"
              //L"                      <Setter Property='Height' Value='200'/>\r\n"
              //L"                  </Style>\r\n"
              //L"              </ResourceDictionary>\r\n"
                L"          </ResourceDictionary.MergedDictionaries>\r\n"
                L"          <ResourceDictionary.ThemeDictionaries>\r\n"
                L"              <ResourceDictionary x:Key='Light'>\r\n"
                L"                  <Style TargetType='Button' x:Key='TextBlockStyle'>\r\n"
                L"                      <Setter Property='Width' Value='200'/>\r\n"
                L"                      <Setter Property='Height' Value='200'/>\r\n"
                L"                   </Style>\r\n"
                L"             </ResourceDictionary>\r\n"
                L"          </ResourceDictionary.ThemeDictionaries>\r\n"
                L"      </ResourceDictionary>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <TextBlock Text='Hey there georgia' Style='{StaticResource TextBlockStyle}'/>\r\n"
                L"</Grid>\r\n");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            LOG_OUTPUT(L"Adding dictionary to back of collection");
            auto newDictionary = CreateInstance(L"Microsoft.UI.Xaml.ResourceDictionary");
            auto style = CreateStyle(L"Microsoft.UI.Xaml.Controls.Button");
            AddDictionaryItem(newDictionary, L"TextBlockStyle", style);

            auto widthSetter = CreateSetterWithProperty(CreateInstance(L"Windows.Foundation.Double", L"200"), L"Microsoft.UI.Xaml.FrameworkElement.Width");
            auto heightSetter = CreateSetterWithProperty(CreateInstance(L"Windows.Foundation.Double", L"200"), L"Microsoft.UI.Xaml.FrameworkElement.Height");

            AddSetterToStyle(style, widthSetter);
            AddSetterToStyle(style, heightSetter);

            AppendToCollection(resources, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries", newDictionary);
            VERIFY_IS_TRUE(callback->HasError());
        }

        void CollectionTests::CanAddGroupedStyle()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='parent'>\r\n"
                L"    <ListView x:Name='list'>\r\n"
                L"      <ListView.ItemTemplate>\r\n"
                L"          <DataTemplate>\r\n"
                L"             <TextBlock Text='Im in an item template' />\r\n"
                L"          </DataTemplate>\r\n"
                L"      </ListView.ItemTemplate>\r\n"
                //L"      <ListView.GroupStyle>\r\n"
                //L"        <GroupStyle>\r\n"
                //L"          <GroupStyle.HeaderTemplate>\r\n"
                //L"             <DataTemplate>\r\n"
                //L"                <TextBlock Text='Im in a group style' />\r\n"
                //L"             </DataTemplate>\r\n"
                //L"          </GroupStyle.HeaderTemplate>\r\n"
                //L"        </GroupStyle>\r\n"
                //L"      </ListView.GroupStyle>\r\n"
                L"   <ListViewItem />\r\n"
                L"   <ListViewItem />\r\n"
                L"  </ListView>\r\n"
                L"</Grid>\r\n");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto list = callback->GetElementByName(L"list");
            auto groupStyle = CreateInstance(L"Microsoft.UI.Xaml.Controls.GroupStyle");

            xaml::DataTemplate^ headerTemplate = nullptr;
            RunOnUIThread([&]{
                headerTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>\r\n"
                    L"  <TextBlock Text='Im in a group style' />\r\n"
                    L"</DataTemplate>\r\n"));
            });

            auto headerTemplateHandle = ih_cast(headerTemplate);
            SetProperty(groupStyle, headerTemplateHandle, L"Microsoft.UI.Xaml.Controls.GroupStyle.HeaderTemplate");

            AppendToCollection(list.Handle, L"Microsoft.UI.Xaml.Controls.ItemsControl.GroupStyle", groupStyle);
        }

        void CollectionTests::ValidateCollectionApisWithResourceDictionary()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"     <SolidColorBrush x:Key='a' Color='Red'/>\r\n"
                L"     <SolidColorBrush x:Key='b' Color='Red'/>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Ellipse x:Name='ellipse' Fill='{StaticResource a}' />\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto parent = callback->GetElementByName(L"parent");
            auto resources = GetProperty(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto item = GetDictionaryItem(resources, L"a");
            // Getting the count and items is required for compat with VS.
            VERIFY_ARE_EQUAL(2u, GetCollectionCount(resources));

            {
                auto elements = GetCollectionElements(resources);
                VERIFY_ARE_EQUAL(2u, elements.Count); // Make sure the count matches
                VERIFY_ARE_NOT_EQUAL(ih_cast(elements.Elements[0].Value), ih_cast(elements.Elements[1].Value));
            }

            // GetDictionaryItem will re-add items, make sure it doesn't
            item = GetDictionaryItem(resources, L"a");
            VERIFY_ARE_EQUAL(2u, GetCollectionCount(resources));
            LOG_OUTPUT(L"Got ResourceDictionary elements");

            item = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush",L"Red");
            AddDictionaryItem(resources, L"c", item);
            VERIFY_ARE_EQUAL(3u, GetCollectionCount(resources));
            {
                auto elements = GetCollectionElements(resources);
                VERIFY_ARE_EQUAL(3u, elements.Count); // Make sure the count matches

                // Make sure all the handles are different
                VERIFY_ARE_NOT_EQUAL(ih_cast(elements.Elements[0].Value), ih_cast(elements.Elements[1].Value));
                VERIFY_ARE_NOT_EQUAL(ih_cast(elements.Elements[0].Value), ih_cast(elements.Elements[2].Value));
                VERIFY_ARE_NOT_EQUAL(ih_cast(elements.Elements[1].Value), ih_cast(elements.Elements[2].Value));
            }

            // Shouldn't be able to remove by index. Indices in ResourceDictionary aren't guaranteed to match
            VERIFY_THROWS(RemoveFromCollection(resources, 0u), WEX::Common::Exception);

            // Clear is fine for when the user selects all items in the dictionary and clears them. VS doesn't do this today, but is an acceptable
            // thing to do.
            VERIFY_NO_THROW(ClearCollection(resources));

            // Add the first item back, this should fail. Indices in ResourceDictionary aren't guaranteed to match
            item = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush",L"Red");
            VERIFY_THROWS(InsertIntoCollection(resources, item, 0u), WEX::Common::Exception);

            LOG_OUTPUT(L"Add items back, make sure GetCollectionItems still works");
            AddDictionaryItem(resources, L"c", item);
            item = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush",L"Blue");
            AddDictionaryItem(resources, L"a", item);
            VERIFY_ARE_EQUAL(2u, GetCollectionCount(resources));
            {
                auto elements = GetCollectionElements(resources);
                VERIFY_ARE_EQUAL(2u, elements.Count); // Make sure the count matches

                // Make sure all the handles are different
                VERIFY_ARE_NOT_EQUAL(ih_cast(elements.Elements[0].Value), ih_cast(elements.Elements[1].Value));
            }

            RemoveDictionaryItem(resources, L"c");
            VERIFY_ARE_EQUAL(1u, GetCollectionCount(resources));
            {
                auto elements = GetCollectionElements(resources);
                VERIFY_ARE_EQUAL(1u, elements.Count); // Make sure the count match
                VERIFY_ARE_EQUAL(item, ih_cast(elements.Elements[0].Value)); // Make sure the 1 item left is what we expect
            }

            LOG_OUTPUT(L"Add primitive objects");
            item = CreateInstance(L"Windows.Foundation.Double", L"30.0");
            AddDictionaryItem(resources, L"d", item);
            {
                VERIFY_ARE_EQUAL(2u, GetCollectionCount(resources));
                auto elements = GetCollectionElements(resources);
                VERIFY_ARE_EQUAL(2u, elements.Count); // Make sure the count matches

                // Make sure all the handles are different
                VERIFY_ARE_NOT_EQUAL(ih_cast(elements.Elements[0].Value), ih_cast(elements.Elements[1].Value));
            }
        }

        void CollectionTests::ValidateAddingSetterToStyleWithBasedOnWhileInvalid()
        {
            // Commented out lines are what we'll be addingmak
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <Style x:Key='BodyTextBlockStyle' TargetType='TextBlock'>"
                L"    </Style>"
                L"    <Style x:Name='S1' BasedOn='{StaticResource BodyTextBlockStyle}' TargetType='TextBlock'>"
                L"      <Setter Property='FontSize' Value='40'/>"
                L"      <Setter Property='HorizontalAlignment' Value='Left'/>"
                //L"      <Setter Property='VerticalAlignment' Value='{StaticResource missing}'/> "
                //L"      <Setter Property='Height' Value='20'/>"
                L"  </Style>"
                L"  </Grid.Resources>\r\n"
                L"  <TextBlock x:Name='text' Text='Hello dolores' Style='{StaticResource S1}'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto parent = callback->GetElementByName(L"parent");
            auto style = GetItemFromElementResources(parent, L"S1");

            auto verticalAlignmentSetter = CreateSetterWithProperty(0, L"Microsoft.UI.Xaml.FrameworkElement.VerticalAlignment");

            AddSetterToStyle(style, verticalAlignmentSetter);
            ResolveResource(verticalAlignmentSetter, L"missing", L"Microsoft.UI.Xaml.Setter.Value", ResourceTypeStatic);
            VERIFY_IS_TRUE(callback->HasError());

            LOG_OUTPUT(L"Validate we can add a Setter that doesn't have a Property or Target set");
            auto heightSetter = CreateSetterWithValueOnly(CreateInstance(L"Windows.Foundation.Double", L"20"));
            AddSetterToStyle(style, heightSetter);
            auto heightDP = CreateInstance(L"Microsoft.UI.Xaml.DependencyProperty", L"Microsoft.UI.Xaml.FrameworkElement.Height");
            SetProperty(heightSetter, heightDP, L"Microsoft.UI.Xaml.Setter.Property");
            auto textBlock = callback->GetElementByName(L"text");

            auto textBlockObject = ih_cast<xaml_controls::TextBlock>(textBlock.Handle);
            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(20.0, textBlockObject->Height);
            });
        }

        void CollectionTests::CanAddNamedListBoxItem()
        {
            // Commented out lines are what we'll be adding
            auto content = ref new Platform::String(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <StackPanel.Resources>\r\n"
                L"    <ControlTemplate TargetType='Button' x:Key='T'>\r\n"
                L"      <ListBox x:Name='template_box'>\r\n"
                L"        <ListBoxItem x:Name='foo'>Hi friend</ListBoxItem>\r\n"
                //L"        <ListBoxItem x:Name='bar'>Hi there</ListBoxItem>\r\n"
                L"      </ListBox>\r\n"
                L"    </ControlTemplate>\r\n"
                L"  </StackPanel.Resources>\r\n"
                L"  <ListBox x:Name='box'>\r\n"
                L"    <ListBoxItem x:Name='foo'>Hi friend</ListBoxItem>\r\n"
                //L"    <ListBoxItem x:Name='bar'>Hi there</ListBoxItem>\r\n"
                L"  </ListBox>\r\n"
                L"  <Button Template='{StaticResource T}'/> \r\n"
                L"</StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto runTest = [&](const std::wstring& listBoxName){
                auto listbox = callback->GetElementByName(listBoxName.c_str());
                auto newListBoxItem = CreateInstance(L"Microsoft.UI.Xaml.Controls.ListBoxItem");
                auto name = CreateInstance(L"Windows.Foundation.String", L"bar");
                SetProperty(newListBoxItem, name, L"Microsoft.UI.Xaml.FrameworkElement.Name");
                AppendToCollection(listbox.Handle, L"Microsoft.UI.Xaml.Controls.ListBox.Items", newListBoxItem);
            };

            LOG_OUTPUT(L"Adding ListBoxItem to standard namescope");
            runTest(L"box");

            LOG_OUTPUT(L"Adding ListBoxItem to template namescope");
            runTest(L"template_box");
        }
        #pragma endregion

    }
} } } } }
