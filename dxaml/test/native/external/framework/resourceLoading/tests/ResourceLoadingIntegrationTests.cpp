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
} } } } } }
