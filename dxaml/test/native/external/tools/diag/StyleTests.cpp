// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "StyleTests.h"
#include <CustomUserControl.h>
#include "CustomTypes.XamlTypeInfo.g.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "MainPage.xaml.h"
#include <CustomMetadataRegistrar.h>
#include "RuntimeEnabledFeatures.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Data;
using namespace test_infra;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;

namespace shared_types = ::Tests::Tools::Shared;
#define HNS_FROM_SECOND(x) ((x)*10 * 1000 * 1000)

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        #pragma region Test Methods

        bool StyleTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool StyleTests::ClassCleanup()
        {
            return true;
        }

        bool StyleTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());

            return EnsureTapLoaded();
        }

        bool StyleTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            test_infra::TestServices::WindowHelper->VerifyTestCleanup();

            return true;
        }

        void StyleTests::GetPropertyValuesChainReportsCorrectSetterValue()
        {
            auto xamlText = ref new Platform::String(
            L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Wheat'>"
            L"  <Grid.Resources>"
            L"    <Style x:Key='a1' TargetType='Button'>"
            L"       <Setter Property='HorizontalAlignment' Value='Center' />"
            L"    </Style>"
            L"  </Grid.Resources>"
            L"  <Button x:Name='steakSauce' Content='Button' Height='200' Width='200' Style='{StaticResource a1}' Background='Red'/>"
            L"</Grid>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);
            auto root = callback->GetElementByName(L"root");

            auto style = GetItemFromElementResources(root, L"a1");

            auto horizontalAlignmentSetter = GetCollectionItem(style, L"Microsoft.UI.Xaml.Style.Setters", 0);

            auto setterValuePropertyChainValue = GetPropertyChainValue(horizontalAlignmentSetter, L"Value", BaseValueSourceLocal);
            auto convertedEnum = m_tap->ConvertEnumValue(setterValuePropertyChainValue.ValueType, std::stoi(setterValuePropertyChainValue.Value));
            VERIFY_IS_TRUE(wcscmp(convertedEnum.c_str(), L"Center") == 0);

            auto steakSauce = ih_cast<Button>(callback->GetElementByName(L"steakSauce").Handle);

            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(HorizontalAlignment::Center, steakSauce->HorizontalAlignment);
            });

            RemoveFromCollection(style, L"Microsoft.UI.Xaml.Style.Setters", 0);

            auto value = CreateInstance(L"Microsoft.UI.Xaml.HorizontalAlignment", L"Right");
            horizontalAlignmentSetter = CreateSetterWithProperty(value, L"Microsoft.UI.Xaml.FrameworkElement.HorizontalAlignment");
            AddSetterToStyle(style,horizontalAlignmentSetter);
            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(HorizontalAlignment::Right, steakSauce->HorizontalAlignment);
            });

            setterValuePropertyChainValue = GetPropertyChainValue(horizontalAlignmentSetter, L"Value", BaseValueSourceLocal);
            convertedEnum = m_tap->ConvertEnumValue(setterValuePropertyChainValue.ValueType, std::stoi(setterValuePropertyChainValue.Value));
            VERIFY_IS_TRUE(wcscmp(convertedEnum.c_str(), L"Right") == 0);

            auto newValue = CreateInstance(L"Microsoft.UI.Xaml.HorizontalAlignment", L"Left");
            SetProperty(horizontalAlignmentSetter, newValue, L"Microsoft.UI.Xaml.Setter.Value");

            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(HorizontalAlignment::Left, steakSauce->HorizontalAlignment);
            });

            setterValuePropertyChainValue = GetPropertyChainValue(horizontalAlignmentSetter, L"Value", BaseValueSourceLocal);
            convertedEnum = m_tap->ConvertEnumValue(setterValuePropertyChainValue.ValueType, std::stoi(setterValuePropertyChainValue.Value));
            VERIFY_IS_TRUE(wcscmp(convertedEnum.c_str(), L"Left") == 0);
        }

        void StyleTests::ResolvingBasedOnUpdatesElement()
        {
            auto xamlText = ref new Platform::String(
            L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Wheat'>"
            L"  <Grid.Resources>"
            L"    <Style x:Key='a1' TargetType='Button'>"
            L"       <Setter Property='HorizontalAlignment' Value='Center' />"
            L"    </Style>"
          //L"    <Syle x:Key='a2' TargetType='Button' BasedOn='{StaticResource a1}'/>"
            L"  </Grid.Resources>"
            L"  <Button x:Name='steakSauce' Content='Button' Height='200' Width='200' Background='Red'/>"
          //L"  <Button x:Name='steakSauce' Content='Button' Height='200' Width='200' Background='Red' Style='{StaticResource a2}' />"
            L"</Grid>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);
            auto root = callback->GetElementByName(L"root");

            auto steakSauce = callback->GetElementByName(L"steakSauce");
            ResolveResource(steakSauce.Handle, L"a2", L"Microsoft.UI.Xaml.FrameworkElement.Style", ResourceTypeStatic);
            VERIFY_IS_TRUE(callback->HasError());

            auto a2Style = CreateStyle(L"Microsoft.UI.Xaml.Controls.Button");
            AddItemToElementResources(root, L"a2", a2Style);
            ResolveResource(a2Style, L"a1", L"Microsoft.UI.Xaml.Style.BasedOn", ResourceTypeStatic);

            auto button = ih_cast<xaml_controls::Button>(steakSauce.Handle);
            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(HorizontalAlignment::Center, button->HorizontalAlignment);
            });
        }

        void StyleTests::VerifyChangeSetterValueDesigner()
        {
                auto xamlText = ref new Platform::String(
            L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Wheat'>"
            L"  <Grid.Resources>"
            L"    <Style TargetType='Button'>"
            L"       <Setter Property='Background' Value='Red' />"
            L"    </Style>"
            L"  </Grid.Resources>"
            L"  <Button x:Name='steakSauce' Content='Button' Height='200' Width='200'/>"
            L"</Grid>");

                wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);
            auto root = callback->GetElementByName(L"root");

            // Mimick the designer and call GetPropertyValuesChain on the Style
            auto style = GetImplicitStyleFromElementResources(root, L"Microsoft.UI.Xaml.Controls.Button");
            GetPropertyChainValue(style, L"TargetType", BaseValueSourceLocal);

            auto setter = GetCollectionItem(style, L"Microsoft.UI.Xaml.Style.Setters", 0);

            auto steakSauce = callback->GetElementByName(L"steakSauce");
            auto button = ih_cast<xaml_controls::Button>(steakSauce.Handle);
            RunOnUIThread([&]
            {
                auto background = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, background->Color);
            });

            auto blueBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Blue");
            SetProperty(setter, blueBrush, L"Microsoft.UI.Xaml.Setter.Value");

            RunOnUIThread([&]
            {
                auto background = safe_cast<SolidColorBrush^>(button->Background);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, background->Color);
            });
        }

        void StyleTests::DeferredSetterValueCanBeInspectedAndEditedWhileShadowed()
        {
            unsigned int constructionCount = 0;
            RunOnUIThread([&]
            {
                constructionCount = shared_types::CustomUserControl::GetConstructionCount();
            });

            auto componentLocation = ref new Uri(L"ms-appx:///resources/native/tools/PageWithDeferredStyleSetter.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);
            auto root = callback->GetElementByName(L"root");
            auto buttonHandle = callback->GetElementByName(L"testButton").Handle;
            auto button = ih_cast<Button>(buttonHandle);

            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(constructionCount, shared_types::CustomUserControl::GetConstructionCount(),
                    L"Applying the style must leave the shadowed setter value deferred before inspection");
                VERIFY_IS_TRUE(wcscmp(safe_cast<Platform::String^>(button->Tag)->Data(), L"Local override") == 0);
            });

            // Even though Tag is set locally to "Local override", we can go get its shadowed BaseValueSourceStyle value
            auto styleValue = GetPropertyChainValue(buttonHandle, L"Tag", BaseValueSourceStyle);
            auto valueHandle = ih_cast(styleValue.Value);
            RunOnUIThread([&]
            {
                auto value = ih_cast<shared_types::CustomUserControl>(valueHandle);
                VERIFY_IS_NOT_NULL(value);
                VERIFY_ARE_EQUAL(37, value->CustomInt);
                VERIFY_ARE_EQUAL(constructionCount + 1, shared_types::CustomUserControl::GetConstructionCount(),
                    L"Diagnostics must realize the shadowed value on demand");
                // The setter value was realized, but the button's local Tag value must not have changed.
                VERIFY_IS_TRUE(wcscmp(safe_cast<Platform::String^>(button->Tag)->Data(), L"Local override") == 0);
            });

            auto style = GetItemFromElementResources(root, L"DeferredStyle");
            auto setter = GetCollectionItem(style, L"Microsoft.UI.Xaml.Style.Setters", 0);
            auto setterValue = GetPropertyChainValue(setter, L"Value", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(valueHandle, ih_cast(setterValue.Value),
                L"Inspecting Style.Setters must expose the same realized value");

            auto editedValue = CreateInstance(L"Windows.Foundation.String", L"Edited setter value");
            SetProperty(setter, editedValue, L"Microsoft.UI.Xaml.Setter.Value");
            auto editedSetterValue = GetPropertyChainValue(setter, L"Value", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(editedSetterValue.Value, L"Edited setter value") == 0);

            RunOnUIThread([&]
            {
                VERIFY_IS_TRUE(wcscmp(safe_cast<Platform::String^>(button->Tag)->Data(), L"Local override") == 0,
                    L"Editing the setter must not replace the local override");
            });

            // Clear the local value and verify the edited style setter value becomes effective.
            ClearProperty(buttonHandle, L"Microsoft.UI.Xaml.FrameworkElement.Tag");
            RunOnUIThread([&]
            {
                VERIFY_IS_TRUE(wcscmp(safe_cast<Platform::String^>(button->Tag)->Data(), L"Edited setter value") == 0,
                    L"Clearing the override must expose the value edited through diagnostics");
                VERIFY_ARE_EQUAL(constructionCount + 1, shared_types::CustomUserControl::GetConstructionCount(),
                    L"Inspection and editing must not recreate the original setter value");
            });
        }

        #pragma endregion

    }
} } } } }
