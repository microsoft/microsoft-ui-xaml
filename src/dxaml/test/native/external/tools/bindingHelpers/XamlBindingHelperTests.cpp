// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "XamlBindingHelperTests.h"
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeaturesEnum.h>
#include <TreeHelper.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Markup;
using namespace test_infra;
using namespace ::Tests::Native::External::Tools::BindingHelpers;

Platform::String^ oldGridName = "layoutRoot";
Platform::String^ newGridName = "NewName";

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools  {

        bool XamlBindingHelperTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool XamlBindingHelperTests::ClassCleanup()
        {
            return true;
        }

        bool XamlBindingHelperTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void XamlBindingHelperTests::ValidateSetWidthOnGrid()
        {
            TestCleanupWrapper cleanup([]{
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            RunOnUIThread([&]()
            {
                // Create a custom user control, where the markup has a ConnectionId on a Grid.  During LoadComponent, MyPage's
                // Connect method will consequently be called, and in this method it sets a name on that Grid.
                Page^ page1 = ref new MyPage();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/XamlBindingHelperSetters.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                // We should be able to FindName for the name that was set during the Connect callback.
                auto connectedGrid = static_cast<Grid^>(page1->FindName(newGridName));
                VERIFY_IS_NOT_NULL(connectedGrid);
                Platform::Object^ doAsObject = safe_cast<MyPage^>(page1)->getDependencyObject();
                XamlBindingHelper::SetPropertyFromDouble(doAsObject, Grid::HeightProperty, 300);
                VERIFY_IS_TRUE(connectedGrid->Height == 300);
            });

        }


        void XamlBindingHelperTests::ValidateThrowWhenSettingColorFromString()
        {
            TestCleanupWrapper cleanup([]{
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            RunOnUIThread([&]()
            {
                // Create a custom user control, where the markup has a ConnectionId on a Grid.  During LoadComponent, MyPage's
                // Connect method will consequently be called, and in this method it sets a name on that Grid.

                Page^ page1 = ref new MyPage();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/XamlBindingHelperSetters.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                // We should be able to FindName for the name that was set during the Connect callback.
                auto connectedGrid = static_cast<Grid^>(page1->FindName(newGridName));
                VERIFY_IS_NOT_NULL(connectedGrid);
                Platform::Object^ doAsObject = safe_cast<MyPage^>(page1)->getDependencyObject();

                LOG_OUTPUT(L"Verify changing the color of the grid throws when from string");

                // try changing color using string, this should throw an excpetion.

                VERIFY_THROWS_WINRT(XamlBindingHelper::SetPropertyFromString(doAsObject, Grid::BackgroundProperty, "Blue"), Platform::COMException^);
            });
        }

        void XamlBindingHelperTests::ValidateSetColorOnGridBackgroundFromColor()
        {
            TestCleanupWrapper cleanup([]{
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });


            RunOnUIThread([&]()
            {
                // Create a custom user control, where the markup has a ConnectionId on a Grid.  During LoadComponent, MyPage's
                // Connect method will consequently be called, and in this method it sets a name on that Grid.
                Page^ page1 = ref new MyPage();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/XamlBindingHelperSetters.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                // We should be able to FindName for the name that was set during the Connect callback.
                auto connectedGrid = static_cast<Grid^>(page1->FindName(newGridName));
                VERIFY_IS_NOT_NULL(connectedGrid);
                Platform::Object^ doAsObject = safe_cast<MyPage^>(page1)->getDependencyObject();
                // First verify that the color is Yellow
                VERIFY_IS_TRUE(static_cast<xaml_media::SolidColorBrush^>(connectedGrid->Background)->Color.R == Microsoft::UI::Colors::Yellow.R);
                VERIFY_IS_TRUE(static_cast<xaml_media::SolidColorBrush^>(connectedGrid->Background)->Color.B == Microsoft::UI::Colors::Yellow.B);
                VERIFY_IS_TRUE(static_cast<xaml_media::SolidColorBrush^>(connectedGrid->Background)->Color.G == Microsoft::UI::Colors::Yellow.G);

                LOG_OUTPUT(L"Changing the color of the grid to blue using the SetPropertyFromObject method");
                // change the color and verify the update works
                auto brush = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                XamlBindingHelper::SetPropertyFromObject(doAsObject, Grid::BackgroundProperty, brush);
                VERIFY_IS_TRUE(static_cast<xaml_media::SolidColorBrush^>(connectedGrid->Background)->Color.R == Microsoft::UI::Colors::Blue.R);
                VERIFY_IS_TRUE(static_cast<xaml_media::SolidColorBrush^>(connectedGrid->Background)->Color.B == Microsoft::UI::Colors::Blue.B);
                VERIFY_IS_TRUE(static_cast<xaml_media::SolidColorBrush^>(connectedGrid->Background)->Color.G == Microsoft::UI::Colors::Blue.G);
            });
        }

    }


} } } }

void ::Tests::Native::External::Tools::BindingHelpers::MyPage::Connect(int id, Platform::Object^ target)
{
    if (id == 100)
    {
        XamlBindingHelper::SetPropertyFromString(target, Grid::NameProperty, newGridName);
        m_dependencyObject = target;
    }

}

Microsoft::UI::Xaml::Markup::IComponentConnector^ ::Tests::Native::External::Tools::BindingHelpers::MyPage::GetBindingConnector(
    int, 
    ::Platform::Object^)
{
    // We return null here beccause this method is not invoked anywhere
    return nullptr;
}
