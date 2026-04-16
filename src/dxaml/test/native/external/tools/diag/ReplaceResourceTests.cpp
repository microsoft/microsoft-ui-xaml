// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ReplaceResourceTests.h"
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
#include "ThemeHelper.h"

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

        bool ReplaceResourceTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool ReplaceResourceTests::ClassCleanup()
        {
            return true;
        }

        bool ReplaceResourceTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());
            return EnsureTapLoaded();
        }

        bool ReplaceResourceTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            test_infra::TestServices::WindowHelper->VerifyTestCleanup();

            return true;
        }


        void ReplaceResourceTests::TestReplaceResourceBasic()
        {
            //Tests replacing the same static resource twice, and verifies that
            //the two Rectangle.Fill properties dependent on it update correctly each time.
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::resourcesString, callback);

            VisualElement rect = callback->GetElementByName(L"childVerified");
            VisualElement rect2 = callback->GetElementByName(L"childVerified2");
            VisualElement rootGrid = callback->GetElementByName(L"root");

            wil::unique_propertychainvalue fillProperty = GetPropertyChainValue(rect.Handle, L"Fill", BaseValueSourceLocal);
            wil::unique_propertychainvalue fillProperty2 = GetPropertyChainValue(rect2.Handle, L"Fill", BaseValueSourceLocal);

            // The fill color is bound to a StaticResource which is AliceBlue - verify the color is correct
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"AliceBlue") == 0);

            wil::unique_propertychainvalue colorProperty2 = GetPropertyChainValue(std::stoll(fillProperty2.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty2.Value, L"AliceBlue") == 0);

            //Create new SolidColorBrushs to replace the current Fill with
            SolidColorBrush ^newBrush, ^newBrush2;

            RunOnUIThread([&]()
            {
                const ::Windows::UI::Color replaceColor = Microsoft::UI::Colors::Red;
                const ::Windows::UI::Color replaceColor2 = Microsoft::UI::Colors::Green;
                newBrush = ref new SolidColorBrush(replaceColor);
                newBrush2 = ref new SolidColorBrush(replaceColor2);
            });
            IInspectable* pBrushInspectable, *pBrushInspectable2;

            pBrushInspectable = reinterpret_cast<IInspectable*>(safe_cast<Platform::Object^>(newBrush));
            pBrushInspectable2 = reinterpret_cast<IInspectable*>(safe_cast<Platform::Object^>(newBrush2));

            //Get the ResourceDictionary where the StaticResource is being stored
            unsigned int indexGetPropertyIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(rootGrid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &indexGetPropertyIndex));
            InstanceHandle dictionaryHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(rootGrid.Handle, indexGetPropertyIndex, &dictionaryHandle));

            InstanceHandle brushHandle, brushHandle2;
            VERIFY_SUCCEEDED(m_tap->RegisterInstance(pBrushInspectable, &brushHandle));
            VERIFY_SUCCEEDED(m_tap->RegisterInstance(pBrushInspectable2, &brushHandle2));

            //Replace the blue SolidColorBrush in the dictionary with the red brush, and verify the two Fill properties
            //for the dependent Rectangles have also updated
            ReplaceResource(dictionaryHandle, brushHandle, L"blueCol");

            //Verify first rectangle's fill is now red
            fillProperty = GetPropertyChainValue(rect.Handle, L"Fill", BaseValueSourceLocal);
            colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);

            //Verify second rectangle is also red
            fillProperty2 = GetPropertyChainValue(rect2.Handle, L"Fill", BaseValueSourceLocal);
            colorProperty2 = GetPropertyChainValue(std::stoll(fillProperty2.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty2.Value, L"Red") == 0);

            //Now replace the same element in the dictionary (was initially blue, now red) again
            //with a green color
            ReplaceResource(dictionaryHandle, brushHandle2, L"blueCol");

            //Verify both rectangles are now green
            fillProperty = GetPropertyChainValue(rect.Handle, L"Fill", BaseValueSourceLocal);
            colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Green") == 0);

            fillProperty2 = GetPropertyChainValue(rect2.Handle, L"Fill", BaseValueSourceLocal);
            colorProperty2 = GetPropertyChainValue(std::stoll(fillProperty2.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty2.Value, L"Green") == 0);
        }

        void ReplaceResourceTests::TestReplaceResourceInDictionary()
        {
            //Tests replacing a Color StaticResource twice, which is used as a StaticResource
            //by a SolidColorBrush in the same dictionary.  Verifies that
            //the two Rectangle.Fill properties dependent on the SolidColorBrush update correctly each time.
            //There is another static resource also dependent on the one being replaced
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::resourcesInDictionaryString, callback);

            VisualElement rect = callback->GetElementByName(L"childVerified");
            VisualElement rect2 = callback->GetElementByName(L"childVerified2");
            VisualElement rootGrid = callback->GetElementByName(L"root");

            wil::unique_propertychainvalue fillProperty = GetPropertyChainValue(rect.Handle, L"Fill", BaseValueSourceLocal);
            wil::unique_propertychainvalue fillProperty2 = GetPropertyChainValue(rect2.Handle, L"Fill", BaseValueSourceLocal);

            // The fill color is bound to a StaticResource which is AliceBlue - verify the color is correct
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"AliceBlue") == 0);

            wil::unique_propertychainvalue colorProperty2 = GetPropertyChainValue(std::stoll(fillProperty2.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty2.Value, L"AliceBlue") == 0);

            const ::Windows::UI::Color replaceColor = Microsoft::UI::Colors::Red;
            const ::Windows::UI::Color replaceColor2 = Microsoft::UI::Colors::Green;

            IInspectable *pColorInspectable, *pColorInspectable2;

            //Instead of casting to Platform::Object and then to IInspectable* in one operation,
            //have two variables which hold on to the Platform::Object^ values - that way they
            //don't go out of scope/get garbage collected until the test completes.
            //safe_cast will box the Colors for us nicely.
            Platform::Object ^pegColor, ^pegColor2;

            pegColor = safe_cast<Platform::Object^>(replaceColor);
            pegColor2 = safe_cast<Platform::Object^>(replaceColor2);

            pColorInspectable = reinterpret_cast<IInspectable*>(pegColor);
            pColorInspectable2 = reinterpret_cast<IInspectable*>(pegColor2);

            //Get the ResourceDictionary where the StaticResource is being stored
            unsigned int indexGetPropertyIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(rootGrid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &indexGetPropertyIndex));
            InstanceHandle dictionaryHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(rootGrid.Handle, indexGetPropertyIndex, &dictionaryHandle));

            InstanceHandle colorHandle, colorHandle2;
            VERIFY_SUCCEEDED(m_tap->RegisterInstance(pColorInspectable, &colorHandle));
            VERIFY_SUCCEEDED(m_tap->RegisterInstance(pColorInspectable2, &colorHandle2));

            //Replace the blue Color in the dictionary with a red Color, and verify the two Fill properties
            //for the dependent Rectangles have also updated
            ReplaceResource(dictionaryHandle, colorHandle, L"col");

            //Verify first rectangle's fill is now red
            fillProperty = GetPropertyChainValue(rect.Handle, L"Fill", BaseValueSourceLocal);
            colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);

            //Verify second rectangle is also red
            fillProperty2 = GetPropertyChainValue(rect2.Handle, L"Fill", BaseValueSourceLocal);
            colorProperty2 = GetPropertyChainValue(std::stoll(fillProperty2.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty2.Value, L"Red") == 0);

            //Now replace the same element in the dictionary (was initially blue, now red) again
            //with a green color
            ReplaceResource(dictionaryHandle, colorHandle2, L"col");

            //Verify both rectangles are now green
            fillProperty = GetPropertyChainValue(rect.Handle, L"Fill", BaseValueSourceLocal);
            colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Green") == 0);

            fillProperty2 = GetPropertyChainValue(rect2.Handle, L"Fill", BaseValueSourceLocal);
            colorProperty2 = GetPropertyChainValue(std::stoll(fillProperty2.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty2.Value, L"Green") == 0);
        }

        void ReplaceResourceTests::TestReplaceResourceCustomObjectProperties()
        {
            //Tests resolving a StaticResource on a custom control with a custom property on a custom control.
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControlReferenceDictionary.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            //Verify we can get the initial static resource used by userControl1, which should have a CustomInt value of 1
            auto userControl = callback->GetElementByName(L"userControl1");
            auto rootGrid = callback->GetElementByName(L"parentGrid");
            wil::unique_propertychainvalue customIntProperty;

            //Verify the initial static resource resolved correctly and CustomInt is currently 6.
            customIntProperty = GetPropertyChainValue(userControl.Handle, L"CustomInt", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(customIntProperty.Value, L"6") == 0);

            //Replace the control with int in the resource dictionary which is currently 6 with a new int
            //with value 32.
            unsigned int indexGetPropertyIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(rootGrid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &indexGetPropertyIndex));
            InstanceHandle dictionaryHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(rootGrid.Handle, indexGetPropertyIndex, &dictionaryHandle));

            //Instead of casting to Platform::Object and then to IInspectable* in one operation,
            //have a variable which holds on to the Platform::Object^ values - that way it
            //doesn't go out of scope/get garbage collected until the test completes.
            //safe_cast will box the intfor us nicely.
            Platform::Object ^pegNum;
            pegNum = safe_cast<Platform::Object^>(32);

            IInspectable* pNumInspectable = reinterpret_cast<IInspectable*>(pegNum);

            InstanceHandle numHandle;

            VERIFY_SUCCEEDED(m_tap->RegisterInstance(pNumInspectable, &numHandle));
            //Replace the resource and verify the value on the dependent custom user control has also updated.
            ReplaceResource(dictionaryHandle, numHandle, L"customResource");

            customIntProperty = GetPropertyChainValue(userControl.Handle, L"CustomInt", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(customIntProperty.Value, L"32") == 0);
        }

        void ReplaceResourceTests::TestReplaceResourceOptimizedSetterDependency()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithStaticResourceSetters.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto rootGrid = callback->GetElementByName(L"rootGrid");
            auto testGrid = callback->GetElementByName(L"testGrid");

            wil::unique_propertychainvalue fillProperty, colorProperty;
            fillProperty = GetPropertyChainValue(testGrid.Handle, L"Background", BaseValueSourceStyle);
            colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);

            //Replace the color in the dictionary which should force the setter, style, and rectangle to update
            unsigned int indexGetPropertyIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(rootGrid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &indexGetPropertyIndex));
            InstanceHandle dictionaryHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(rootGrid.Handle, indexGetPropertyIndex, &dictionaryHandle));

            //Create new SolidColorBrushs to replace the current Fill with
            SolidColorBrush ^newBrush;

            RunOnUIThread([&]()
            {
                const ::Windows::UI::Color replaceColor = Microsoft::UI::Colors::Blue;
                newBrush = ref new SolidColorBrush(replaceColor);
            });
            IInspectable* pBrushInspectable = reinterpret_cast<IInspectable*>(safe_cast<Platform::Object^>(newBrush));
            InstanceHandle brushHandle;
            VERIFY_SUCCEEDED(m_tap->RegisterInstance(pBrushInspectable, &brushHandle));
            ReplaceResource(dictionaryHandle, brushHandle, L"resourceBrush");

            //Verify the rectangle's color is now Blue.
            fillProperty = GetPropertyChainValue(testGrid.Handle, L"Background", BaseValueSourceStyle);
            colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Blue") == 0);
        }

        void ReplaceResourceTests::TestReplaceResourceStyle()
        {
            //This test tries replacing a Style in a ResourceDictionary twice with ReplaceResource,
            //verifying the style changes affect an element each time.
            //This test also uses a DependencyProperty created from CreateInstance in the Style to verify
            //CreateInstance with a DependencyProperty works correctly.
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            VisualElement rootGrid = callback->GetElementByName(L"root");

            //Get the ResourceDictionary where the Style is being stored
            auto dictionaryHandle = GetProperty(rootGrid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            InstanceHandle blueBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Blue");
            InstanceHandle redBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red");

            //Create the two new Styles that will replace the current style, using the Shape.Fill DP created above to target
            //Red and Blue Fill properties

            InstanceHandle originalStyleHandle = GetDictionaryItem(dictionaryHandle, L"rectangleStyle");
            InstanceHandle style2Handle = CloneAndModifySingleSetterStyle(originalStyleHandle, L"Microsoft.UI.Xaml.Shapes.Shape.Fill", redBrush);
            InstanceHandle style3Handle = CloneAndModifySingleSetterStyle(originalStyleHandle, L"Microsoft.UI.Xaml.Shapes.Shape.Fill", blueBrush);

            //Replace the Style in the dictionary with one that sets Fill to Red instead of Green, verify
            //the Rectangle using the Style gets updated correctly
            ReplaceResource(dictionaryHandle, style2Handle, L"rectangleStyle");

            VisualElement verified = callback->GetElementByName(L"Verified");

            wil::unique_propertychainvalue fillProperty = GetPropertyChainValue(verified.Handle, L"Fill", BaseValueSourceStyle);
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);

            //Now replace the Red Style with a Blue Style
            ReplaceResource(dictionaryHandle, style3Handle, L"rectangleStyle");

            fillProperty = GetPropertyChainValue(verified.Handle, L"Fill", BaseValueSourceStyle);
            colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Blue") == 0);
        }

        void ReplaceResourceTests::TestUnregisterStaticResource()
        {
            //Set a property which is bound to a StaticResource to a local property,
            //change the static resource with ReplaceResource, and verify the property
            //is still equal to its local value instead of the new value in the dictionary.
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::resourcesString, callback);

            VisualElement rect = callback->GetElementByName(L"childVerified");
            VisualElement rootGrid = callback->GetElementByName(L"root");

            wil::unique_propertychainvalue fillProperty = GetPropertyChainValue(rect.Handle, L"Fill", BaseValueSourceLocal);

            // The fill color is bound to a StaticResource which is AliceBlue - verify the color is correct
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"AliceBlue") == 0);

            //Get the ResourceDictionary where the StaticResource is being stored
            unsigned int indexGetPropertyIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(rootGrid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &indexGetPropertyIndex));
            InstanceHandle dictionaryHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(rootGrid.Handle, indexGetPropertyIndex, &dictionaryHandle));

            //Create new SolidColorBrushs - one will be set as the local value for  the rectangle,
            //then the other will replace the old StaticResource in the dictionary.
            auto brushHandle = CreateInstance(fillProperty.Type, L"Red");
            auto brushHandle2 = CreateInstance(fillProperty.Type, L"Green");

            //Set the local value for the rectangle to Red
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(rect.Handle, brushHandle, fillProperty.Index));

            //Verify first rectangle's fill is now red
            fillProperty = GetPropertyChainValue(rect.Handle, L"Fill", BaseValueSourceLocal);
            colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);

            //Now replace the resource in the dictionary with a Green color
            ReplaceResource(dictionaryHandle, brushHandle2, L"blueCol");

            //Verify the rectangle keeps its locally set Red value instead of the Green value
            //now in the dictionary
            fillProperty = GetPropertyChainValue(rect.Handle, L"Fill", BaseValueSourceLocal);
            colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);
        }

        void ReplaceResourceTests::TestReplaceResourceApplicationImplicit()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = LoadXamlFromFunction(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);

            auto button = callback->GetElementByName(L"buttonApp");

            // Verify buttonApp has correctly looked up its current color from Application.Resources
            wil::unique_propertychainvalue background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceLocal);
            wil::unique_propertychainvalue color = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(L"Black", color.Value) == 0);

            //Now replace the dictionary entry modifying buttonApp in Application.Resources
            InstanceHandle dictionaryHandle, brushHandle;
            SolidColorBrush^ newBrush;
            ResourceDictionary^ appDictionary;

            RunOnUIThread([&]()
            {
                appDictionary = Application::Current->Resources;
                newBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Orange);
            });

            IInspectable* pDictionaryInspectable = reinterpret_cast<IInspectable*>(safe_cast<Platform::Object^>(appDictionary));
            IInspectable* pBrushInspectable = reinterpret_cast<IInspectable*>(safe_cast<Platform::Object^>(newBrush));
            VERIFY_SUCCEEDED(m_tap->GetHandleFromIInspectable(pDictionaryInspectable, &dictionaryHandle));
            VERIFY_SUCCEEDED(m_tap->GetHandleFromIInspectable(pBrushInspectable, &brushHandle));
            ReplaceResource(dictionaryHandle, brushHandle, L"testApp");

            //Verify the property using Application.Resource's entry updated correctly
            background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceLocal);
            color = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(L"Orange", color.Value) == 0);
        }

        void ReplaceResourceTests::TestReplaceResourceMergedDictionaries()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = LoadXamlFromFunction(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);

            //Grab the ResourceDictionary which owns the merged dictionaries.
            //Get the ResourceDictionary where the Style is being stored
            auto grid = callback->GetElementByName(L"parent");
            unsigned int resourcesIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(grid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &resourcesIndex));
            InstanceHandle parentDictionaryHandle = 0;
            VERIFY_SUCCEEDED(m_tap->GetProperty(grid.Handle, resourcesIndex, &parentDictionaryHandle));

            //First, try ReplaceResource in a merged dictionary with a key that is overwritten by another merged
            //dictionary - the object using that element shouldn't change
            auto button = callback->GetElementByName(L"buttonOverwritten");

            //Verify base value is from the second merged dictionary instead of the first we'll replace
            wil::unique_propertychainvalue background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceLocal);
            wil::unique_propertychainvalue color = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(L"Green", color.Value) == 0);

            //Get the overwritten dictionary and ReplaceResource the entry the button uses
            //Now replace the dictionary entry modifying buttonApp in Application.Resources

            unsigned int mergedDictionaryIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(parentDictionaryHandle, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries", &mergedDictionaryIndex));
            InstanceHandle mergedDictionaryHandle = 0;
            VERIFY_SUCCEEDED(m_tap->GetProperty(parentDictionaryHandle, mergedDictionaryIndex, &mergedDictionaryHandle));

            UINT32 count = 0;
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(mergedDictionaryHandle, &count));
            VERIFY_IS_GREATER_THAN(count, 0u);

            CoTaskMemPtr<CollectionElementValue> mergedDictionaries;
            VERIFY_SUCCEEDED(m_tap->GetCollectionElements(mergedDictionaryHandle, 0, &count, &mergedDictionaries));

            InstanceHandle brushHandle = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Orange");

            InstanceHandle dictionaryHandle = std::stoll(mergedDictionaries[0].Value);
            ReplaceResource(dictionaryHandle, brushHandle, L"testOverwritten");

            //Verify the property hasn't changed
            background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceLocal);
            color = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(L"Green", color.Value) == 0);

            //Now ReplaceResource on the MergedDictionary the button actually uses, and verify its
            //property did change

            dictionaryHandle = std::stoll(mergedDictionaries[1].Value);
            ReplaceResource(dictionaryHandle, brushHandle, L"testOverwritten");

            background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceLocal);
            color = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(L"Orange", color.Value) == 0);
        }

        void ReplaceResourceTests::ReplaceStaticResourceInThemeDictionary()
        {
            ApplicationThemeOverrider themeHelper(Microsoft::UI::Xaml::ApplicationTheme::Dark);
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);

            auto grid = callback->GetElementByName(L"parent");

            //First, try ReplaceResource in a merged dictionary with a key that is overwritten by another merged
            //dictionary - the object using that element shouldn't change
            auto button = callback->GetElementByName(L"buttonStaticThemed");

            //Verify base value is from the second merged dictionary instead of the first we'll replace
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));

            TestReplaceThemeResource(grid.Handle, L"Dark");
            // Verify that we've changed
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, GetColorProperty(button.Handle, L"Background", BaseValueSourceLocal));
        }

        void ReplaceResourceTests::ReplaceThemeResource()
        {
            ApplicationThemeOverrider themeHelper(Microsoft::UI::Xaml::ApplicationTheme::Dark);
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);

            auto grid = callback->GetElementByName(L"parent");

            auto themedButton = callback->GetElementByName(L"buttonThemed");

            //Verify base value is from the second merged dictionary instead of the first we'll replace
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(themedButton.Handle, L"Background", BaseValueSourceLocal));

            TestReplaceThemeResource(grid.Handle, L"Dark");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, GetColorProperty(themedButton.Handle, L"Background", BaseValueSourceLocal));

            // Verify that changing the root elements theme will now update the theme that has a ThemeResource to what we just changed
            RunOnUIThread([&]()
            {
                auto parentGrid = GetFromInstanceHandle<xaml_controls::Grid>(grid.Handle);
                parentGrid->RequestedTheme = Microsoft::UI::Xaml::ElementTheme::Light;
            });

            // Let theme changes take place
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Yellow, GetColorProperty(themedButton.Handle, L"Background", BaseValueSourceLocal));
        }

        void ReplaceResourceTests::ReplaceNonActiveThemeResource()
        {
            ApplicationThemeOverrider themeHelper(Microsoft::UI::Xaml::ApplicationTheme::Dark);
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::SetupAppAndMergedDictionaries, callback);

            auto grid = callback->GetElementByName(L"parent");

            auto themedButton = callback->GetElementByName(L"buttonThemed");

            //Verify base value is from the second merged dictionary instead of the first we'll replace
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(themedButton.Handle, L"Background", BaseValueSourceLocal));

            // Replace property in the light theme dictionary. Since the current theme is Dark this shouldn't affect the value
            TestReplaceThemeResource(grid.Handle, L"Light");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(themedButton.Handle, L"Background", BaseValueSourceLocal));

            // Change to the theme that we just replaced and make sure it takes place after the update
            RunOnUIThread([&]()
            {
                auto parentGrid = GetFromInstanceHandle<xaml_controls::Grid>(grid.Handle);
                parentGrid->RequestedTheme = Microsoft::UI::Xaml::ElementTheme::Light;
            });

            //Verify base value is from the second merged dictionary instead of the first we'll replace
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, GetColorProperty(themedButton.Handle, L"Background", BaseValueSourceLocal));

        }

        void ReplaceResourceTests::TestReplaceThemeResource(InstanceHandle rootElement, const std::wstring& themeDictionaryKey)
        {
            auto parentDictionaryHandle = GetProperty(rootElement, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto mergedDictionary = GetCollectionItem(parentDictionaryHandle, L"Microsoft.UI.Xaml.ResourceDictionary.MergedDictionaries", 2);
            // Get the theme dictionary of the item we are replacing
            auto themeDictionaries = GetProperty(mergedDictionary, L"Microsoft.UI.Xaml.ResourceDictionary.ThemeDictionaries");
            auto actualThemeDictionary = GetDictionaryItem(themeDictionaries, themeDictionaryKey);
            auto orangeBrushHandle = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Orange");
            ReplaceResource(actualThemeDictionary, orangeBrushHandle, L"testThemed");
        }

        void ReplaceResourceTests::ReplaceLanguagePrimitives()
        {
            auto xamlStr = ref new Platform::String(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
            L"  <Grid.Resources>\r\n"
            L"    <x:String x:Key='MyString'>Hello World</x:String>"
            L"    <x:Double x:Key='MyDouble'>13.1859</x:Double>"
            L"    <x:Boolean x:Key='MyBool'>True</x:Boolean>"
            L"    <x:Int32 x:Key='MyInt'>1</x:Int32>"
            L"  </Grid.Resources>\r\n"
            L"  <TextBlock x:Name='texty' Text='{StaticResource MyString}' Width='{StaticResource MyDouble}' AllowDrop='{StaticResource MyBool}' CharacterSpacing='{StaticResource MyInt}'/>"
            L"</Grid>\r\n");

            TestReplaceLanguagePrimitives(xamlStr);
        }

           void ReplaceResourceTests::ReplaceLanguagePrimitivesInControlTemplate()
        {
            auto xamlStr = ref new Platform::String(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
            L"  <Grid.Resources>\r\n"
            L"    <x:String x:Key='MyString'>Hello World</x:String>"
            L"    <x:Double x:Key='MyDouble'>13.1859</x:Double>"
            L"    <x:Boolean x:Key='MyBool'>True</x:Boolean>"
            L"    <x:Int32 x:Key='MyInt'>1</x:Int32>"
            L"    <ControlTemplate x:Key='T1'>"
            L"      <Border>"
            L"       <TextBlock x:Name='texty' Text='{StaticResource MyString}' Width='{StaticResource MyDouble}' AllowDrop='{StaticResource MyBool}' CharacterSpacing='{StaticResource MyInt}'/>"
            L"      </Border>"
            L"    </ControlTemplate>"
            L"  </Grid.Resources>\r\n"
            L"  <Button Template='{StaticResource T1}' Width='100' Height='100' />"
            L"</Grid>\r\n");

            TestReplaceLanguagePrimitives(xamlStr);
        }

        void ReplaceResourceTests::ReplaceLanguagePrimitivesInControlTemplateSetterValue()
        {
            auto xamlStr = ref new Platform::String(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
            L"  <Grid.Resources>\r\n"
            L"    <x:String x:Key='MyString'>Hello World</x:String>"
            L"    <x:Double x:Key='MyDouble'>13.1859</x:Double>"
            L"    <x:Boolean x:Key='MyBool'>True</x:Boolean>"
            L"    <x:Int32 x:Key='MyInt'>1</x:Int32>"
            L"    <Style x:Key='MyStyle' TargetType='Button'>"
            L"      <Setter Property='Template'>"
            L"        <Setter.Value>"
            L"          <ControlTemplate TargetType='Button'>"
            L"            <Border>"
            L"              <TextBlock x:Name='texty' Text='{StaticResource MyString}' Width='{StaticResource MyDouble}' AllowDrop='{StaticResource MyBool}' CharacterSpacing='{StaticResource MyInt}'/>"
            L"            </Border>"
            L"          </ControlTemplate>"
            L"        </Setter.Value>"
            L"      </Setter>"
            L"    </Style>"
            L"  </Grid.Resources>\r\n"
            L"  <Button Style='{StaticResource MyStyle}' Width='100' Height='100' />"
            L"</Grid>\r\n");

            TestReplaceLanguagePrimitives(xamlStr);
        }

        void ReplaceResourceTests::ReplaceLanguagePrimitivesInControlTemplateSetterValue_Optimized()
        {
            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            auto componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/SimpleGrid.xaml");
            TestReplaceLanguagePrimitives(componentLocation);
        }

        void ReplaceResourceTests::ReplaceBrushInControlTemplate()
        {
               auto xamlStr = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <SolidColorBrush x:Key='MyBrush'>Red</SolidColorBrush>"
                L"    <ControlTemplate x:Key='T1'>"
                L"      <Border>"
                L"        <Rectangle Fill='{StaticResource MyBrush}' x:Name='rect'/>"
                L"      </Border>"
                L"    </ControlTemplate>"
                L"  </Grid.Resources>\r\n"
                L"  <Button Template='{StaticResource T1}' Width='100' Height='100' />"
                L"</Grid>\r\n");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlStr, callback);

            auto grid = callback->GetElementByName(L"parent");
            auto dictionaryHandle = GetProperty(grid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto newBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Blue");
            auto rectangle = callback->GetElementByName(L"rect");

            auto beforeBrush = GetProperty<SolidColorBrush>(rectangle.Handle, L"Microsoft.UI.Xaml.Shapes.Shape.Fill");
            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, beforeBrush->Color);
            });
            ReplaceResource(dictionaryHandle, newBrush, L"MyBrush");

            auto afterColor = GetProperty<SolidColorBrush>(rectangle.Handle, L"Microsoft.UI.Xaml.Shapes.Shape.Fill");
            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, afterColor->Color);
            });
        }

        void ReplaceResourceTests::TestReplaceLanguagePrimitives(Platform::String^ markup)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(markup, callback);

            ReplaceAllLanguagePrimitives(callback);
        }

        void ReplaceResourceTests::TestReplaceLanguagePrimitives(::Windows::Foundation::Uri^ location)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(location, callback);

            ReplaceAllLanguagePrimitives(callback);
        }

        void ReplaceResourceTests::ReplaceAllLanguagePrimitives(const wrl::ComPtr<VisualTreeServiceCallback>& callback)
        {
            ReplacePrimitiveData data;

            LOG_OUTPUT(L"Replacing x:String");
            data.Type = L"Windows.Foundation.String";
            data.Property = L"Text";
            data.Key = L"MyString";
            data.BeforeValue = L"Hello World";
            data.AfterValue = L"Hello World!!";
            ReplaceLanguagePrimitive(callback, data);

            LOG_OUTPUT(L"Replacing x:Double");
            data.Type = L"Windows.Foundation.Double";
            data.Property = L"Width";
            data.Key = L"MyDouble";
            data.BeforeValue = L"13.1859";
            data.AfterValue = L"100.33";
            ReplaceLanguagePrimitive(callback, data);

            LOG_OUTPUT(L"Replacing x:Boolean");
            data.Type = L"Windows.Foundation.Boolean";
            data.Property = L"AllowDrop";
            data.Key = L"MyBool";
            data.BeforeValue = L"1";
            data.AfterValue = L"0";
            ReplaceLanguagePrimitive(callback, data);

            LOG_OUTPUT(L"Replacing x:Int32");
            data.Type = L"Windows.Foundation.Int32";
            data.Property = L"CharacterSpacing";
            data.Key = L"MyInt";
            data.BeforeValue = L"1";
            data.AfterValue = L"12";
            ReplaceLanguagePrimitive(callback, data);
        }

        void ReplaceResourceTests::ReplaceLanguagePrimitive(const wrl::ComPtr<VisualTreeServiceCallback>& callback, const ReplacePrimitiveData& replaceData)
        {
            auto grid = callback->GetElementByName(L"parent");
            auto dictionaryHandle = GetProperty(grid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto newValue = CreateInstance(replaceData.Type.c_str(), replaceData.AfterValue.c_str());
            auto textBox = callback->GetElementByName(L"texty");

            auto before = GetPropertyChainValue(textBox.Handle, replaceData.Property.c_str(), BaseValueSourceLocal);
            LOG_OUTPUT(L"Before: %s", before.Value);

            if(wcscmp(before.Value, replaceData.BeforeValue.c_str()) != 0)
            {
                VERIFY_FAIL(WEX::Common::String().Format(L"Expected before: %s", replaceData.BeforeValue.c_str()));
            }
            ReplaceResource(dictionaryHandle, newValue, replaceData.Key);

            auto after = GetPropertyChainValue(textBox.Handle, replaceData.Property.c_str(), BaseValueSourceLocal);
            LOG_OUTPUT(L"After: %s", after.Value);

            if(wcscmp(after.Value, replaceData.AfterValue.c_str()) != 0)
            {
                VERIFY_FAIL(WEX::Common::String().Format(L"Expected after: %s", replaceData.AfterValue.c_str()));
            }
        }

        void ReplaceResourceTests::ReplaceThemeResourceSetToSetterValue()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <Thickness x:Key='M1'>30</Thickness>\r\n"
                L"    <Style TargetType='Button'>\r\n"
                L"       <Setter Property='Margin' Value='{ThemeResource M1}' />\r\n"
                L"    </Style>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Button Content='Hello World' x:Name='but'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto but = callback->GetElementByName(L"but");

            auto root = callback->GetElementByName(L"root");
            auto resources = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto colt45 = CreateInstance(L"Microsoft.UI.Xaml.Thickness", L"45");
            ReplaceResource(resources, colt45, L"M1");

            auto margin = GetPropertyChainValue(but.Handle, L"Margin", BaseValueSourceStyle);
            VERIFY_ARE_EQUAL(std::wstring(L"45,45,45,45"), std::wstring(margin.Value));

            // Verify the actual effective value
            RunOnUIThread([&]()
            {
                auto buttonAsDO = GetFromInstanceHandle<xaml_controls::Button>(but.Handle);
                VERIFY_ARE_EQUAL(45, buttonAsDO->Margin.Bottom);
                VERIFY_ARE_EQUAL(45, buttonAsDO->Margin.Left);
                VERIFY_ARE_EQUAL(45, buttonAsDO->Margin.Right);
                VERIFY_ARE_EQUAL(45, buttonAsDO->Margin.Top);
            });

        }

        void ReplaceResourceTests::VerifyReplaceResourceUpdatesFallbackValue()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <x:String x:Key='myText'>Hello World</x:String>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <TextBlock x:Name='block' Text='{Binding DoesNotResolve, FallbackValue={StaticResource myText}}'/>\r\n"
                L"  <TextBlock x:Name='block2' Text='{Binding DoesNotResolve, FallbackValue={ThemeResource myText}}'/>\r\n"
                L"  <TextBlock x:Name='block3' Text='Golly willicker'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto block = callback->GetElementByName(L"block");
            auto block2 = callback->GetElementByName(L"block2");
            auto block3 = callback->GetElementByName(L"block3");

            // Create a binding at runtime and add it
            auto binding = CreateBinding(L"", L"DoesNotResolve", L"Fallback");
            SetProperty(block3.Handle, binding, L"Microsoft.UI.Xaml.Controls.TextBlock.Text");
            auto textBlock3 = ih_cast<xaml_controls::TextBlock>(block3.Handle);

            // Verify that actually worked
            RunOnUIThread([&]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"Fallback"), textBlock3->Text);
            });

            // Verify we can resolve

            const auto fallbackValueIndex = GetPropertyIndex(binding, L"Microsoft.UI.Xaml.Data.Binding.FallbackValue");
            ResolveResource(binding, L"myText", fallbackValueIndex, ResourceTypeStatic);
            RunOnUIThread([&]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"Hello World"), textBlock3->Text);
            });

            auto root = callback->GetElementByName(L"root");
            auto resources = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto ahhh = CreateInstance(L"Windows.Foundation.String", L"AHHHH");
            ReplaceResource(resources, ahhh, L"myText");

            auto textBlock = ih_cast<xaml_controls::TextBlock>(block.Handle);
            auto textBlock2 = ih_cast<xaml_controls::TextBlock>(block2.Handle);
            RunOnUIThread([&]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"AHHHH"), textBlock->Text);
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"AHHHH"), textBlock2->Text);
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"AHHHH"), textBlock3->Text);
            });

            // Clear the Text property on Block and then change the resource and make sure we don't get an update
            ClearProperty(block.Handle, L"Microsoft.UI.Xaml.Controls.TextBlock.Text");
            auto pizzapizza = CreateInstance(L"Windows.Foundation.String", L"PizzaPizza!");
            ReplaceResource(resources, pizzapizza, L"myText");

            VERIFY_IS_FALSE(callback->HasError()); // Since we cleared the binding, we shouldn't have a problem here.

            RunOnUIThread([&]{
                // TextBlock shouldn't change since we cleared the binding
                 VERIFY_ARE_EQUAL(ref new Platform::String(L""), textBlock->Text);
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"PizzaPizza!"), textBlock2->Text);
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"PizzaPizza!"), textBlock3->Text);
            });
        }

        void ReplaceResourceTests::VerifyReplaceResourceUpdatesTargetNullValue()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <x:String x:Key='myText'>Hello World</x:String>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <TextBlock x:Name='block' Text='{Binding CustomUnknown, ElementName=custom, TargetNullValue={StaticResource myText}}'/>\r\n"
                L"  <TextBlock x:Name='block2' Text='{Binding CustomUnknown, ElementName=custom, TargetNullValue={ThemeResource myText}}'/>\r\n"
                L"  <TextBlock x:Name='block3' Text='Golly williker'/>\r\n"
                L"  <local:CustomUserControl x:Name='custom' />\r\n"
                L"</Grid>\r\n");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto block = callback->GetElementByName(L"block");
            auto block2 = callback->GetElementByName(L"block2");
            auto block3 = callback->GetElementByName(L"block3");

            // Create a binding at runtime and add it
            auto binding = CreateBinding(L"custom", L"CustomUnknown");
            auto targetIsNull = CreateInstance(L"Windows.Foundation.String", L"TargetIsNull");
            SetProperty(binding, targetIsNull, L"Microsoft.UI.Xaml.Data.Binding.TargetNullValue");
            SetProperty(block3.Handle, binding, L"Microsoft.UI.Xaml.Controls.TextBlock.Text");
            auto textBlock3 = ih_cast<xaml_controls::TextBlock>(block3.Handle);

            // Verify that actually worked
            RunOnUIThread([&]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"TargetIsNull"), textBlock3->Text);
            });

            // Verify we can resolve

            const auto targetNullValueIndex = GetPropertyIndex(binding, L"Microsoft.UI.Xaml.Data.Binding.TargetNullValue");
            ResolveResource(binding, L"myText", targetNullValueIndex, ResourceTypeStatic);
            RunOnUIThread([&]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"Hello World"), textBlock3->Text);
            });

            auto root = callback->GetElementByName(L"root");
            auto resources = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            auto ahhh = CreateInstance(L"Windows.Foundation.String", L"AHHHH");
            ReplaceResource(resources, ahhh, L"myText");

            auto textBlock = ih_cast<xaml_controls::TextBlock>(block.Handle);
            auto textBlock2 = ih_cast<xaml_controls::TextBlock>(block2.Handle);
            RunOnUIThread([&]{
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"AHHHH"), textBlock->Text);
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"AHHHH"), textBlock2->Text);
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"AHHHH"), textBlock3->Text);
            });


            // Clear the Text property on Block and then change the resource and make sure we don't get an update
            ClearProperty(block.Handle, L"Microsoft.UI.Xaml.Controls.TextBlock.Text");
            auto pizzapizza = CreateInstance(L"Windows.Foundation.String", L"PizzaPizza!");
            ReplaceResource(resources, pizzapizza, L"myText");
            VERIFY_IS_FALSE(callback->HasError()); // Since we cleared the binding, we shouldn't have a problem here.

            RunOnUIThread([&]{
                // TextBlock shouldn't change since we cleared the binding
                 VERIFY_ARE_EQUAL(ref new Platform::String(L""), textBlock->Text);
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"PizzaPizza!"), textBlock2->Text);
                 VERIFY_ARE_EQUAL(ref new Platform::String(L"PizzaPizza!"), textBlock3->Text);
            });
        }

        void ReplaceResourceTests::VerifyReplaceResourceOfStyleDoesntAffectResourceResolution()
        {
            auto content = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <SolidColorBrush x:Key='MyBrush' Color='Blue' />\r\n"
                L"    <Style x:Key='MyStyle' TargetType='Button'>\r\n"
                L"      <Setter Property='Background' Value='{StaticResource MyBrush}'/>\r\n"
                L"    </Style>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Button x:Name='button' Content='Yolo' Width='100' Height='100' Style='{StaticResource MyStyle}'/>\r\n"
                L"</Grid>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto root = callback->GetElementByName(L"root");
            auto rootDictionary = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto style = CreateStyle(L"Microsoft.UI.Xaml.Controls.Button");

            auto setter = CreateSetterWithProperty(0, L"Microsoft.UI.Xaml.Controls.Control.Background");
            AddSetterToStyle(style, setter);
            ReplaceResource(rootDictionary, style, L"MyStyle");

            // Now make sure we can resolve the "MyBrush" on the setter
            const auto setterValueIndex = GetPropertyIndex(setter, L"Microsoft.UI.Xaml.Setter.Value");
            ResolveResource(setter, L"MyBrush", setterValueIndex, ResourceTypeStatic);
            VERIFY_IS_FALSE(callback->HasError());

            auto button = callback->GetElementByName(L"button");
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, GetColorProperty(button.Handle, L"Background", BaseValueSourceStyle));
        }

        void ReplaceResourceTests::VerifyReplacingValueTypesDoesntCrash()
        {
            auto content = ref new Platform::String(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <StackPanel.Resources>\r\n"
                L"    <Thickness  x:Key='T1'>5</Thickness>\r\n"
                L"    <Style x:Key='MyStyle' TargetType='Button'>\r\n"
                L"      <Setter Property='Margin' Value='{StaticResource T1}'/>\r\n"
                L"    </Style>\r\n"
                L"  </StackPanel.Resources>\r\n"
                L"  <Button x:Name='button' Content='Yolo' Width='100' Height='100' Style='{StaticResource MyStyle}'/>\r\n"
                L"</StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto root = callback->GetElementByName(L"root");
            auto rootDictionary = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            // Mimick the designer and call GetPropertyValuesChain on the Style
            auto style = GetDictionaryItem(rootDictionary, L"MyStyle");
            GetPropertyChainValue(style, L"TargetType", BaseValueSourceLocal);

            auto thicknessHandle  = CreateInstance(L"Microsoft.UI.Xaml.Thickness", L"20");
            ReplaceResource(rootDictionary, thicknessHandle, L"T1");

            auto button = callback->GetElementByName(L"button");
            auto thickness = GetPropertyChainValue(button.Handle, L"Margin", BaseValueSourceStyle);
            VERIFY_IS_TRUE(wcscmp(thickness.Value, L"20,20,20,20") == 0);

            auto buttonObj = ih_cast<Button>(button.Handle);
            RunOnUIThread([&]
            {
                Thickness buttonMargin = buttonObj->Margin;
                VERIFY_ARE_EQUAL(20.0, buttonMargin.Left);
                VERIFY_ARE_EQUAL(20.0, buttonMargin.Top);
                VERIFY_ARE_EQUAL(20.0, buttonMargin.Right);
                VERIFY_ARE_EQUAL(20.0, buttonMargin.Bottom);
            });
        }

        void ReplaceResourceTests::VerifyReplacingValueTypesDoesntCrashImplicitStyle()
        {
            auto content = ref new Platform::String(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"      xmlns:local='using:Tests.Tools.Shared'\r\n"
                L"      x:Name='root'>\r\n"
                L"  <StackPanel.Resources>\r\n"
                L"    <Thickness x:Key='T1'>5</Thickness>\r\n"
                L"    <Style TargetType='Button'>\r\n"
                L"      <Setter Property='Margin' Value='{StaticResource T1}'/>\r\n"
                L"    </Style>\r\n"
                L"  </StackPanel.Resources>\r\n"
                L"  <Button x:Name='button' Content='Yolo' Width='100' Height='100' />\r\n"
                L"</StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);
            auto root = callback->GetElementByName(L"root");
            auto rootDictionary = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            // Mimick the designer and call GetPropertyValuesChain on the Style
            auto style = GetImplicitStyle(rootDictionary, L"Microsoft.UI.Xaml.Controls.Button");
            GetPropertyChainValue(style, L"TargetType", BaseValueSourceLocal);

            auto thicknessHandle  = CreateInstance(L"Microsoft.UI.Xaml.Thickness", L"20");
            ReplaceResource(rootDictionary, thicknessHandle, L"T1");

            auto button = callback->GetElementByName(L"button");
            auto thickness = GetPropertyChainValue(button.Handle, L"Margin", BaseValueSourceStyle);
            VERIFY_IS_TRUE(wcscmp(thickness.Value, L"20,20,20,20") == 0);

            auto buttonObj = ih_cast<Button>(button.Handle);
            RunOnUIThread([&]
            {
                Thickness buttonMargin = buttonObj->Margin;
                VERIFY_ARE_EQUAL(20.0, buttonMargin.Left);
                VERIFY_ARE_EQUAL(20.0, buttonMargin.Top);
                VERIFY_ARE_EQUAL(20.0, buttonMargin.Right);
                VERIFY_ARE_EQUAL(20.0, buttonMargin.Bottom);
            });
        }

        void ReplaceResourceTests::VerifyReplaceInvalidResourceClearsErrors()
        {
            auto content = ref new Platform::String(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"   <StackPanel.Resources>\r\n"
                L"    <ControlTemplate x:Key='T1' TargetType='ListView'>\r\n"
                L"      <Border>\r\n"
                L"        <TextBlock Text='{TemplateBinding Content}' />\r\n"
                L"      </Border>\r\n"
                L"    </ControlTemplate>\r\n"
                L"   </StackPanel.Resources>\r\n"
                L"   <Button x:Name='button' Width='200' Height='50' Content='Hello world' />\r\n"
                L"</StackPanel>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto button = callback->GetElementByName(L"button");
            ResolveResource(button.Handle, L"T1", L"Microsoft.UI.Xaml.Controls.Control.Template", ResourceTypeStatic);
            VERIFY_IS_TRUE(callback->HasError());

            xaml_controls::ControlTemplate^ ctrlTemplate = nullptr;
            RunOnUIThread([&]{
                ctrlTemplate = safe_cast< xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                    L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='Button'>\r\n"
                    L"  <Border>\r\n"
                    L"    <TextBlock Text='{TemplateBinding Content}' />\r\n"
                    L"  </Border>\r\n"
                    L"</ControlTemplate>\r\n"));
            });

            auto ctrlTemplateHandle = ih_cast(ctrlTemplate);
            auto root = callback->GetElementByName(L"root");
            auto resources = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
            ReplaceResource(resources,ctrlTemplateHandle, L"T1");
            VERIFY_IS_FALSE(callback->HasError());
        }

        void ReplaceResourceTests::VerifyReplaceNonDP()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControl.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto customUserControl = callback->GetElementByName(L"userControl1");
            auto customUserControl2 = callback->GetElementByName(L"userControl2");

            auto root = callback->GetElementByName(L"root");
            auto rootDictionary = GetProperty(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            unsigned int nonDPIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(customUserControl.Handle, L"Tests.Tools.Shared.CustomUserControl.CustomBrushNonDP", &nonDPIndex));
            VERIFY_ARE_NOT_EQUAL(nonDPIndex, 0u);

            // Resolve userControl1's brush property to the blue brush in the resource dictionary
            // (userControl2 already references it)
            ResolveResource(customUserControl.Handle, L"blueBrush", nonDPIndex, ResourceTypeStatic);

            // Replace the blue brush with a cyan brush
            auto cyanHandle = CreateInstance(L"Microsoft.UI.Xaml.Media.SolidColorBrush", L"Cyan");
            ReplaceResource(rootDictionary, cyanHandle, L"blueBrush");

            // Get the new brush from the non-DP property and verify its color is appropriately Blue for userControl1
            wrl::ComPtr<IInspectable> customBrushVal;
            InstanceHandle customBrushValHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(customUserControl.Handle, nonDPIndex, &customBrushValHandle));
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(customBrushValHandle, &customBrushVal));

            RunOnUIThread([&]()
            {
                auto brush = safe_cast<SolidColorBrush^>(reinterpret_cast<Platform::Object^>(customBrushVal.Get()));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Cyan, brush->Color);
            });

            // Do the same for userControl2
            VERIFY_SUCCEEDED(m_tap->GetProperty(customUserControl2.Handle, nonDPIndex, &customBrushValHandle));
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(customBrushValHandle, &customBrushVal));

            RunOnUIThread([&]()
            {
                auto brush = safe_cast<SolidColorBrush^>(reinterpret_cast<Platform::Object^>(customBrushVal.Get()));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Cyan, brush->Color);
            });
        }

        void ReplaceResourceTests::VerifyReplaceResourceUpdatesItemInDataTemplate()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/StaticResourcePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto root = callback->GetElementByName(L"SRP_MyBrushListViewRoot");
            auto backgroundBrush = GetProperty<xaml_media::SolidColorBrush>(root.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Background");
            RunOnUIThread([&]{
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, backgroundBrush->Color);
            });

            InstanceHandle rootPage{};
            RunOnUIThread([&]{
                rootPage = ih_cast(TestServices::WindowHelper->WindowContent);
            });

            auto resources = GetProperty(rootPage, L"Microsoft.UI.Xaml.FrameworkElement.Resources");

            auto newBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red");
            ReplaceResource(resources, newBrush, L"SRP_MyBrush");
            backgroundBrush = GetProperty<xaml_media::SolidColorBrush>(root.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Background");
            RunOnUIThread([&]{
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, backgroundBrush->Color);
            });
        }

        Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper ReplaceResourceTests::LoadXamlFromFunction(const std::function<UIElement ^ ()> func, wrl::ComPtr<VisualTreeServiceCallback>& callback)
        {
            callback = m_connectionHelper->Advise();

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper cleanup([&] {
                m_connectionHelper->OnTestComplete(callback);
            });

            UIElement^ uielement = func();
            Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]()
            {
                test_infra::TestServices::WindowHelper->WindowContent = uielement;
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();
            return cleanup;
        }

        #pragma endregion

    }
} } } } }
