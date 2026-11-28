// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <FileLoader.h>
#include <Utils.h>
#include "ResourceLoadingIntegrationTests.h"
#include <TestCleanupWrapper.h>
#include "CustomResourceLoader.h"
#include "RootCustomResourceDialog.xaml.h"

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Interop;

using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace std;

using Colors = Microsoft::UI::Colors;
using Color = ::Windows::UI::Color;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        namespace ResourceLoading {

    bool ResourceLoadingIntegrationTests::ClassSetup()
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

        bool ResourceLoadingIntegrationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

    bool ResourceLoadingIntegrationTests::TestCleanup()
    {
        // It's very important to have your test clean up the window contents
        // when it completes. When creating new tests be sure to copy this
        // method over or implement it in a similar way. By cleaning
        // up the window content and waiting for the page to go idle you ensure
        // that if your test fails while the UI element tree is being torn down
        // that the failure is associated with your test and doesn't occur
        // non-deterministically in the future. By waiting for the page to go
        // idle you ensure that all transitions have completed and that jupiter
        // is in a 'tabula rasa' state for the next test.
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    Platform::String^ GetFilePath()
    {
        // Get the deployment directory, and then append our test's directory to the end
        auto deploymentDir = GetTestDeploymentDir();
        return ref new Platform::String(deploymentDir + L"resources\\native\\framework\\resourceLoading\\");
    }

    Platform::String^ GetResourceURI(Platform::String^ scenarioName)
    {
        return "ms-appx:///" + "resources/native/framework/resourceLoading/" + scenarioName + ".xaml";
    }

    bool IsSameColor(Color expected, Color actual)
    {
        return (expected.R == actual.R
            && expected.G == actual.G
            && expected.B == actual.B
            && expected.A == actual.A);
    }

    void LoadXamlComponent(DependencyObject^ rootObject, Platform::String^ scenarioName)
    {
        String^ componentLocation = "ms-appx:///" + "resources/native/framework/resourceLoading/" + scenarioName;

        Application::LoadComponent(
            rootObject,
            ref new ::Windows::Foundation::Uri(componentLocation),
            Primitives::ComponentResourceLocation::Application);
    }

    Canvas^ LoadDefaultRoot()
    {
        Canvas^ result = nullptr;
        RunOnUIThread([&result]()
        {
            result = safe_cast<Canvas^>(XamlReader::Load(
                L"<Canvas  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"         xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"</Canvas>"
                ));

            TestServices::WindowHelper->WindowContent = result;
        });
        TestServices::WindowHelper->WaitForIdle();
        return result;
    }

    void ResourceLoadingIntegrationTests::CanLoadComponentOutsideVisualTree()
    {
        TestCleanupWrapper cleanup;
        auto hostPanel = LoadDefaultRoot();
        RunOnUIThread([hostPanel]()
        {
            // Add an element to the visual tree before calling LoadComponent on it
            auto element = ref new StackPanel;
            LoadXamlComponent(element, L"LoadComponent.Panel.xaml");
            hostPanel->Children->Append(element);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([hostPanel]()
        {
            auto loadedTextBlock = safe_cast<TextBlock^>(hostPanel->FindName(L"loadedTextBlock"));

            TestServices::WindowHelper->WindowContent = nullptr;
        });
    }

    void ResourceLoadingIntegrationTests::CanLoadComponentInsideVisualTree()
    {
        TestCleanupWrapper cleanup;
        auto hostPanel = LoadDefaultRoot();
        RunOnUIThread([hostPanel]()
        {
            // Add an element to the visual tree before calling LoadComponent on it
            auto element = ref new StackPanel;
            hostPanel->Children->Append(element);
            LoadXamlComponent(element, L"LoadComponent.Panel.xaml");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([hostPanel]()
        {
            auto loadedTextBlock = safe_cast<TextBlock^>(hostPanel->FindName(L"loadedTextBlock"));
            VERIFY_IS_NOT_NULL(loadedTextBlock, L"LoadComponent before adding to visual tree");

            TestServices::WindowHelper->WindowContent = nullptr;
        });
    }

    void LoadCustomResources(CustomResourceLoader^ loader)
    {
        loader->AddResource(L"simpleString", L"Ren");
        // Xaml runtime is only going to see the WinRT versions of typenames, so this guy needs to be hardcoded for now
        loader->AddResource(L"fullTypeNameString", TextBlock::typeid->FullName, L"Text", L"Windows.Foundation.String", ref new String(L"Stimpy"));
        loader->AddResource(L"myTestUrl", L"http://www.bing.com/");
        loader->AddResource(L"solidColor", SolidColorBrush::typeid->FullName, L"Color", ::Windows::UI::Color::typeid->FullName, Microsoft::UI::Colors::Chartreuse);
        loader->AddResource(L"myMargin", Microsoft::UI::Xaml::Shapes::Rectangle::typeid->FullName, L"Margin", xaml::Thickness::typeid->FullName, xaml::Thickness({1,2,3,4}));
    }

    void TestCustomResources(Panel^ panel)
    {
        auto loadedTextBlock = safe_cast<TextBlock^>(panel->FindName(L"simpleString_TextBlock"));
        VERIFY_IS_NOT_NULL(loadedTextBlock);
        VERIFY_IS_TRUE(loadedTextBlock->Text == L"Ren");

        loadedTextBlock = safe_cast<TextBlock^>(panel->FindName(L"fullTypeNameString_TextBlock"));
        VERIFY_IS_NOT_NULL(loadedTextBlock);
        VERIFY_IS_TRUE(loadedTextBlock->Text == L"Stimpy");

        auto rect = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(panel->FindName(L"solidColorBrush_Rectangle"));
        VERIFY_IS_NOT_NULL(rect);

        auto color = safe_cast<SolidColorBrush^>(rect->Fill);
        VERIFY_ARE_EQUAL(color->Color, Microsoft::UI::Colors::Chartreuse);

        auto hyperLink = safe_cast<HyperlinkButton^>(panel->FindName(L"myLink"));
        VERIFY_IS_NOT_NULL(hyperLink);
        VERIFY_IS_TRUE(hyperLink->NavigateUri->AbsoluteUri == L"http://www.bing.com/");

        auto rectWithCustomMargin = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(panel->FindName(L"customMargin_Rectangle"));
        VERIFY_IS_NOT_NULL(rect);
        xaml::Thickness expectedMargin = {1,2,3,4};
        VERIFY_ARE_EQUAL(rectWithCustomMargin->Margin, expectedMargin);
    }

    void ResourceLoadingIntegrationTests::CanLoadCustomResourcesFromXaml()
    {
        TestCleanupWrapper cleanup([]()
        {
            RunOnUIThread([]()
            {
                Microsoft::UI::Xaml::Resources::CustomXamlResourceLoader::Current = nullptr;
            });
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        RunOnUIThread([]()
        {
            auto customResourceLoader = ref new CustomResourceLoader;
            LoadCustomResources(customResourceLoader);

            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' x:Name='root' "
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <TextBlock Text='{CustomResource simpleString}' x:Name='simpleString_TextBlock' /> "
                L"    <TextBlock Text='{CustomResource fullTypeNameString}' x:Name='fullTypeNameString_TextBlock' />"
                L"    <HyperlinkButton NavigateUri='{CustomResource myTestUrl}' Content='NavigateUri set to CustomResourceLoader text' x:Name='myLink' />"
                L"    <Rectangle x:Name='customMargin_Rectangle' Width='50' Height='50' Margin='{CustomResource myMargin}'/>"
                L"    <Rectangle x:Name='solidColorBrush_Rectangle'>"
                L"        <Rectangle.Fill>"
                L"            <SolidColorBrush Color='{CustomResource solidColor}' />"
                L"        </Rectangle.Fill>"
                L"    </Rectangle>"
                L"</StackPanel>";

            auto panel = safe_cast<StackPanel^>(XamlReader::Load(xamlString));

            TestCustomResources(panel);
        });
    }

    // Regression test for bug 50695292: a markup extension on a property of the ROOT element
    // of a XAML document used to fail with E_XAMLPARSEFAILED "Markup extension could not provide
    // value." because the root collapses to LiveDepth 1, so the parser took the provide-only path
    // and never assigned the value. This requires a provided root instance (LoadComponent), which
    // is exactly how an x:Class root such as a ContentDialog subclass is created.
    void ResourceLoadingIntegrationTests::CanLoadCustomResourceOnRootElement()
    {
        TestCleanupWrapper cleanup([]()
        {
            RunOnUIThread([]()
            {
                Microsoft::UI::Xaml::Resources::CustomXamlResourceLoader::Current = nullptr;
            });
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        RunOnUIThread([]()
        {
            auto customResourceLoader = ref new CustomResourceLoader;
            LoadCustomResources(customResourceLoader);

            // Markup extension on the provided root element's own property (mirrors the ContentDialog repro).
            auto rootTextBlock = ref new TextBlock;
            LoadXamlComponent(rootTextBlock, L"CustomResourceOnRoot.xaml");

            VERIFY_IS_TRUE(rootTextBlock->Text == L"Ren");
        });
    }

    // Faithful repro for bug 50695292: instantiate a markup-COMPILED (genxbf) x:Class ContentDialog
    // whose ROOT element sets PrimaryButtonText via {CustomResource} - exactly the reported scenario
    // (compiled ContentDialog subclass, not loose XAML). The ctor calls LoadComponent(this, compiled xbf).
    void ResourceLoadingIntegrationTests::CanLoadCustomResourceOnCompiledRootElement()
    {
        TestCleanupWrapper cleanup([]()
        {
            RunOnUIThread([]()
            {
                Microsoft::UI::Xaml::Resources::CustomXamlResourceLoader::Current = nullptr;
            });
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        RunOnUIThread([]()
        {
            auto customResourceLoader = ref new CustomResourceLoader;
            LoadCustomResources(customResourceLoader);

            auto dialog = ref new ::Tests::Native::External::Framework::ResourceLoading::RootCustomResourceDialog();
            VERIFY_IS_TRUE(dialog->PrimaryButtonText == L"Ren");
        });
    }

    void ResourceLoadingIntegrationTests::CanLoadCustomResourcesFromComponent()
    {
        TestCleanupWrapper cleanup([]()
        {
            RunOnUIThread([]()
            {
                Microsoft::UI::Xaml::Resources::CustomXamlResourceLoader::Current = nullptr;
            });
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        auto root = LoadDefaultRoot();
        StackPanel^ panel = nullptr;

        RunOnUIThread([&]()
        {
            auto customResourceLoader = ref new CustomResourceLoader;
            LoadCustomResources(customResourceLoader);

            panel = ref new StackPanel;
            root->Children->Append(panel);
            LoadXamlComponent(panel, L"CustomResources.xaml");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            TestCustomResources(panel);
            TestServices::WindowHelper->WindowContent = nullptr;
        });
    }

    void ResourceLoadingIntegrationTests::VerifyPreResolveOfResourcesInTemplate()
    {
         TestCleanupWrapper cleanup;

         auto addBrush = [&](::Windows::UI::Color color) -> auto {
            Application::Current->Resources->Insert(L"MyBrush", color);
            return wil::scope_exit([]
            {
                Application::Current->Resources->Remove(L"MyBrush");
            });
         };

         DataTemplate^ dataTemp = nullptr;

         RunOnUIThread([&]()
         {
            Application::Current->Resources->Insert(L"SystemAccentColor", Colors::Green);
            Platform::String^ xamlString =
            L" <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"   <Grid Background='{StaticResource MyBrush}' />"
            L" </DataTemplate>";
            auto guard = addBrush(Colors::Green);

            dataTemp = safe_cast<DataTemplate^>(xaml_markup::XamlReader::Load(xamlString));
            auto grid = safe_cast<Grid^>(dataTemp->LoadContent());

            VERIFY_ARE_EQUAL(Colors::Green, safe_cast<xaml_media::SolidColorBrush^>(grid->Background)->Color);
         });

         // Add the brush again with a different color, verify when reloading the template that it hasn't changed
         RunOnUIThread([&]()
         {
            auto guard = addBrush(Colors::Blue);
            auto grid = safe_cast<Grid^>(dataTemp->LoadContent());

            VERIFY_ARE_EQUAL(Colors::Green, safe_cast<xaml_media::SolidColorBrush^>(grid->Background)->Color);
         });

     }

    void ResourceLoadingIntegrationTests::VerifyCanReplaceGenericXaml()
    {
        TestCleanupWrapper cleanup([]()
        {
            RunOnUIThread([]()
            {
                TestServices::Utilities->SetGenericXamlFilePathForMUX(nullptr);
            });

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::StackPanel^ rootPanel;

        RunOnUIThread([&]()
        {
            // Default theme button background is transparent #00FFFFFF, it is red in this generic.xaml
            TestServices::Utilities->SetGenericXamlFilePathForMUX(GetResourceURI("genericred"));

            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <Button x:Name='testButton' Content='Button' /> "
                L"</StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto button = safe_cast<Button^>(rootPanel->FindName(L"testButton"));
            VERIFY_IS_NOT_NULL(button->Background);
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_IS_TRUE(IsSameColor(brush->Color, Microsoft::UI::Colors::Red));
        });
    }

    void ResourceLoadingIntegrationTests::VerifyMissingGenericXamlReplacement()
    {
        TestCleanupWrapper cleanup([]()
        {
            RunOnUIThread([]()
            {
                TestServices::Utilities->SetGenericXamlFilePathForMUX(nullptr);
            });

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::StackPanel^ rootPanel;
        xaml_controls::CheckBox^ checkBox;

        RunOnUIThread([&]()
        {
            // Default theme checkbox background is transparent #00FFFFFF
            TestServices::Utilities->SetGenericXamlFilePathForMUX(GetResourceURI("doesnotexist"));

            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <CheckBox x:Name='testCheckBox' Content='CheckBox' /> "
                L"</StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            checkBox = safe_cast<CheckBox^>(rootPanel->FindName(L"testCheckBox"));
            VERIFY_IS_NOT_NULL(checkBox->Background);
            auto brush = safe_cast<SolidColorBrush^>(checkBox->Background);
            VERIFY_IS_TRUE(IsSameColor(brush->Color, Microsoft::UI::Colors::Transparent));
        });
    }

    void ResourceLoadingIntegrationTests::VerifyIncompleteGenericXamlReplacement()
    {
        TestCleanupWrapper cleanup([]()
        {
            RunOnUIThread([]()
            {
                TestServices::Utilities->SetGenericXamlFilePathForMUX(nullptr);
            });

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::StackPanel^ rootPanel;

        RunOnUIThread([&]()
        {
            // If there is no control template defined then background will be null.
            TestServices::Utilities->SetGenericXamlFilePathForMUX(GetResourceURI("genericincomplete"));

            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <Button x:Name='testButton' Content='Button' /> "
                L"</StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto button = safe_cast<Button^>(rootPanel->FindName(L"testButton"));
            VERIFY_IS_NULL(button->Background);
        });
    }

    // Reads a 64-bit integral value without depending on whether it is boxed as Int64 or UInt64.
    static UINT64 ReadNonNegative64BitIntegral(IPropertyValue^ propertyValue)
    {
        switch (propertyValue->Type)
        {
        case PropertyType::UInt64:
            return propertyValue->GetUInt64();
        case PropertyType::Int64:
        {
            const INT64 signedValue = propertyValue->GetInt64();
            VERIFY_IS_GREATER_THAN_OR_EQUAL(signedValue, 0ll);
            return static_cast<UINT64>(signedValue);
        }
        default:
            VERIFY_FAIL(L"Expected a 64-bit integral property value.");
            return 0;
        }
    }

    static void VerifyRuntimeClassName(Platform::Object^ value, const wchar_t* expectedName)
    {
        HSTRING className = nullptr;
        VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(value)->GetRuntimeClassName(&className));
        Microsoft::WRL::Wrappers::HString classNameOwner;
        classNameOwner.Attach(className);
        LOG_OUTPUT(L"Runtime class name: %s", classNameOwner.GetRawBuffer(nullptr));
        VERIFY_ARE_EQUAL(0, wcscmp(expectedName, classNameOwner.GetRawBuffer(nullptr)));
    }

    void ResourceLoadingIntegrationTests::CustomResourceTypedValuesRoundTripThroughObjectProperty()
    {
        TestCleanupWrapper cleanup([]()
        {
            RunOnUIThread([]()
            {
                Microsoft::UI::Xaml::Resources::CustomXamlResourceLoader::Current = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        RunOnUIThread([]()
        {
            Platform::Object^ int64Box = PropertyValue::CreateInt64(-9000000000000ll);
            Platform::Object^ uint64Box = PropertyValue::CreateUInt64(5000000000ull);

            Matrix matrix;
            matrix.M11 = 1.5;
            matrix.M12 = 2;
            matrix.M21 = 3;
            matrix.M22 = 4;
            matrix.OffsetX = 5;
            matrix.OffsetY = -6;
            Platform::Object^ matrixBox = matrix;

            Media3D::Matrix3D matrix3D;
            matrix3D.M11 = 1;  matrix3D.M12 = 2;  matrix3D.M13 = 3;  matrix3D.M14 = 4;
            matrix3D.M21 = 5;  matrix3D.M22 = 6;  matrix3D.M23 = 7;  matrix3D.M24 = 8;
            matrix3D.M31 = 9;  matrix3D.M32 = 10; matrix3D.M33 = 11; matrix3D.M34 = 12;
            matrix3D.OffsetX = 13; matrix3D.OffsetY = 14; matrix3D.OffsetZ = 15; matrix3D.M44 = 16;
            Platform::Object^ matrix3DBox = matrix3D;

            auto customResourceLoader = ref new CustomResourceLoader;
            customResourceLoader->AddResource(L"int64Value", int64Box);
            customResourceLoader->AddResource(L"uint64Value", uint64Box);
            customResourceLoader->AddResource(L"matrixValue", matrixBox);
            customResourceLoader->AddResource(L"matrix3DValue", matrix3DBox);

            auto panel = safe_cast<StackPanel^>(XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Border x:Name='int64Border' Tag='{CustomResource int64Value}' />"
                L"    <Border x:Name='uint64Border' Tag='{CustomResource uint64Value}' />"
                L"    <Border x:Name='matrixBorder' Tag='{CustomResource matrixValue}' />"
                L"    <Border x:Name='matrix3DBorder' Tag='{CustomResource matrix3DValue}' />"
                L"</StackPanel>"));

            LOG_OUTPUT(L"An Int64 value beyond the Int32 range is read back as the same Int64 value.");
            auto int64Tag = safe_cast<Border^>(panel->FindName(L"int64Border"))->Tag;
            auto int64Value = safe_cast<IPropertyValue^>(int64Tag);
            VERIFY_ARE_EQUAL(PropertyType::Int64, int64Value->Type);
            VERIFY_ARE_EQUAL(-9000000000000ll, int64Value->GetInt64());

            LOG_OUTPUT(L"A UInt64 value beyond the UInt32 range is read back with the same value.");
            auto uint64Tag = safe_cast<Border^>(panel->FindName(L"uint64Border"))->Tag;
            VERIFY_ARE_EQUAL(5000000000ull, ReadNonNegative64BitIntegral(safe_cast<IPropertyValue^>(uint64Tag)));

            LOG_OUTPUT(L"A Matrix value is read back with every field unchanged.");
            auto matrixTag = safe_cast<Border^>(panel->FindName(L"matrixBorder"))->Tag;
            auto readMatrix = safe_cast<Matrix>(matrixTag);
            VERIFY_ARE_EQUAL(1.5, readMatrix.M11);
            VERIFY_ARE_EQUAL(2.0, readMatrix.M12);
            VERIFY_ARE_EQUAL(3.0, readMatrix.M21);
            VERIFY_ARE_EQUAL(4.0, readMatrix.M22);
            VERIFY_ARE_EQUAL(5.0, readMatrix.OffsetX);
            VERIFY_ARE_EQUAL(-6.0, readMatrix.OffsetY);
            VerifyRuntimeClassName(matrixTag, L"Windows.Foundation.IReference`1<Microsoft.UI.Xaml.Media.Matrix>");

            LOG_OUTPUT(L"A Matrix3D value is read back with all 16 fields unchanged.");
            auto matrix3DTag = safe_cast<Border^>(panel->FindName(L"matrix3DBorder"))->Tag;
            auto readMatrix3D = safe_cast<Media3D::Matrix3D>(matrix3DTag);
            VERIFY_ARE_EQUAL(1.0, readMatrix3D.M11);
            VERIFY_ARE_EQUAL(2.0, readMatrix3D.M12);
            VERIFY_ARE_EQUAL(3.0, readMatrix3D.M13);
            VERIFY_ARE_EQUAL(4.0, readMatrix3D.M14);
            VERIFY_ARE_EQUAL(5.0, readMatrix3D.M21);
            VERIFY_ARE_EQUAL(6.0, readMatrix3D.M22);
            VERIFY_ARE_EQUAL(7.0, readMatrix3D.M23);
            VERIFY_ARE_EQUAL(8.0, readMatrix3D.M24);
            VERIFY_ARE_EQUAL(9.0, readMatrix3D.M31);
            VERIFY_ARE_EQUAL(10.0, readMatrix3D.M32);
            VERIFY_ARE_EQUAL(11.0, readMatrix3D.M33);
            VERIFY_ARE_EQUAL(12.0, readMatrix3D.M34);
            VERIFY_ARE_EQUAL(13.0, readMatrix3D.OffsetX);
            VERIFY_ARE_EQUAL(14.0, readMatrix3D.OffsetY);
            VERIFY_ARE_EQUAL(15.0, readMatrix3D.OffsetZ);
            VERIFY_ARE_EQUAL(16.0, readMatrix3D.M44);
            VerifyRuntimeClassName(matrix3DTag, L"Windows.Foundation.IReference`1<Microsoft.UI.Xaml.Media.Media3D.Matrix3D>");
        });
    }

} } } } } }
