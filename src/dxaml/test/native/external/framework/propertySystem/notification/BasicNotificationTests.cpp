// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BasicNotificationTests.h"
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "TestCleanupWrapper.h"

using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace External { namespace Frameworks { namespace PropertySystem {
    namespace Notification {

        bool BasicNotificationTests::ClassSetup()
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

        bool BasicNotificationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool BasicNotificationTests::TestCleanup()
        {
            //
            // It's very important to have your test clean up the window contents
            // when it completes. When creating new tests be sure to copy this
            // method over or implement it in a similar way. By cleaning
            // up the window content and waiting for the page to go idle you ensure
            // that if your test fails while the UI element tree is being torn down
            // that the failure is associated with your test and doesn't occur
            // non-deterministically in the future. By waiting for the page to go
            // idle you ensure that all transitions have completed and that jupiter
            // is in a 'tabula rasa' state for the next test.
            //
            // Use the TestCleanupWrapper in each test method to handle cleanup, even
            // in cases of failure or repeated runs. Use VerifyTestCleanup here to
            // ensure that the test was cleaned up correctly.
            //
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        // These tests are designed to test the new property notification system
        // which allows one or more DO's to watch the property changes on another
        // DO. And allow the hookup and removal of the listeners at arbitrary times
        // instead of only during object creation.

        void BasicNotificationTests::DependencyNotificationTests()
        {
// Disable test until API can be brought to spec. Unfortunately that is blocked by
// a compiler issue in the ARM compiler.

            TestCleanupWrapper cleanup;

            RunOnUIThread([&] () {
            // This will keep a record of all the callbacks. When the tests are
            // completed it will be checked for failures.

                std::vector<NotificationRecord> record;

            // These are the callbacks. The first simply records the object and property name
            // to the list

                auto handlerSimple = ref new DependencyPropertyChangedCallback([&record] (DependencyObject^ sender, DependencyProperty^ prop) {
                    NotificationRecord *entry = new NotificationRecord();

                    entry->handler = ref new Platform::String(L"Simple");
                    entry->sender = sender->GetValue(FrameworkElement::NameProperty)->ToString();
                    record.push_back(*entry);
                });

            // This one also calls GetValue on the property and records that

                auto handlerQuery = ref new DependencyPropertyChangedCallback([&record] (DependencyObject^ sender, DependencyProperty^ prop) {
                    NotificationRecord *entry = new NotificationRecord();

                    entry->handler = ref new Platform::String(L"Query");
                    entry->sender = sender->GetValue(FrameworkElement::NameProperty)->ToString();
                    entry->value = sender->GetValue(prop)->ToString();
                    record.push_back(*entry);
                });

            // The same as the above callback to test multiple handler cases

                auto handlerExtra = ref new DependencyPropertyChangedCallback([&record] (DependencyObject^ sender, DependencyProperty^ prop) {
                    NotificationRecord *entry = new NotificationRecord();

                    entry->handler = ref new Platform::String(L"Extra");
                    entry->sender = sender->GetValue(FrameworkElement::NameProperty)->ToString();
                    entry->value = sender->GetValue(prop)->ToString();
                    record.push_back(*entry);
                });

            // Now create some objects. These no longer have to be derived from FrameworkElement (yeah!)

                SolidColorBrush^ brushOne = ref new SolidColorBrush();
                SolidColorBrush^ brushTwo = ref new SolidColorBrush();
                SolidColorBrush^ brushThree = ref new SolidColorBrush();
                Button^ buttonOne = ref new Button();
                Button^ buttonTwo = ref new Button();
                Button^ buttonThree = ref new Button();
                Grid^ gridOne = ref new Grid();
                Grid^ gridTwo = ref new Grid();
                Grid^ gridThree = ref new Grid();
                CheckBox^ checkOne = ref new CheckBox();
                CheckBox^ checkTwo = ref new CheckBox();
                Slider^ sliderOne = ref new Slider();
                Slider^ sliderTwo = ref new Slider();

                INT64 tokens[10];

            // Name all the objects for easy reference

                Platform::String^ name;

                name = "brushOne";
                brushOne->SetValue(FrameworkElement::NameProperty, name);
                name = "brushTwo";
                brushTwo->SetValue(FrameworkElement::NameProperty, name);
                name = "brushThree";
                brushThree->SetValue(FrameworkElement::NameProperty, name);
                name = "buttonOne";
                buttonOne->SetValue(FrameworkElement::NameProperty, name);
                name = "buttonTwo";
                buttonTwo->SetValue(FrameworkElement::NameProperty, name);
                name = "buttonThree";
                buttonThree->SetValue(FrameworkElement::NameProperty, name);
                name = "gridOne";
                gridOne->SetValue(FrameworkElement::NameProperty, name);
                name = "gridTwo";
                gridTwo->SetValue(FrameworkElement::NameProperty, name);
                name = "gridThree";
                gridThree->SetValue(FrameworkElement::NameProperty, name);
                name = "checkOne";
                checkOne->SetValue(FrameworkElement::NameProperty, name);
                name = "checkTwo";
                checkTwo->SetValue(FrameworkElement::NameProperty, name);
                name = "sliderOne";
                sliderOne->SetValue(FrameworkElement::NameProperty, name);
                name = "sliderTwo";
                sliderTwo->SetValue(FrameworkElement::NameProperty, name);

            // Now hook up some handlers

                tokens[0] = brushOne->RegisterPropertyChangedCallback(SolidColorBrush::ColorProperty, handlerSimple);
                tokens[1] = buttonTwo->RegisterPropertyChangedCallback(Button::FontSizeProperty, handlerQuery);
                tokens[2] = gridThree->RegisterPropertyChangedCallback(Grid::HeightProperty, handlerExtra);
                tokens[3] = checkOne->RegisterPropertyChangedCallback(CheckBox::IsCheckedProperty, handlerSimple);
                tokens[4] = sliderTwo->RegisterPropertyChangedCallback(Slider::ValueProperty, handlerQuery);

            // Set properties

                brushOne->SetValue(SolidColorBrush::ColorProperty, Microsoft::UI::Colors::Red);
                brushTwo->SetValue(SolidColorBrush::ColorProperty, Microsoft::UI::Colors::Green);
                buttonTwo->SetValue(Button::FontSizeProperty, 13.5);
                buttonTwo->SetValue(Button::FontWeightProperty, Microsoft::UI::Text::FontWeights::Thin);
                gridThree->SetValue(Grid::HeightProperty, 120.0);
                gridThree->SetValue(Grid::WidthProperty, 180.0);
                checkOne->SetValue(CheckBox::IsCheckedProperty, true);
                sliderTwo->SetValue(Slider::ValueProperty, 2.0);

                VERIFY_ARE_EQUAL(5, (signed) record.size(), L"Should be five recorded events");

            // Add multiple handler for the same properties

                tokens[5] = brushOne->RegisterPropertyChangedCallback(SolidColorBrush::ColorProperty, handlerExtra);
                tokens[6] = gridThree->RegisterPropertyChangedCallback(Grid::HeightProperty, handlerExtra);
                tokens[7] = checkOne->RegisterPropertyChangedCallback(CheckBox::IsCheckedProperty, handlerExtra);

            // Set properties

                brushOne->SetValue(SolidColorBrush::ColorProperty, Microsoft::UI::Colors::Green);
                brushTwo->SetValue(SolidColorBrush::ColorProperty, Microsoft::UI::Colors::Red);
                buttonTwo->SetValue(Button::FontSizeProperty, 14.5);
                buttonTwo->SetValue(Button::FontWeightProperty, Microsoft::UI::Text::FontWeights::Normal);
                gridThree->SetValue(Grid::HeightProperty, 121.0);
                gridThree->SetValue(Grid::WidthProperty, 181.0);
                checkOne->SetValue(CheckBox::IsCheckedProperty, false);
                sliderTwo->SetValue(Slider::ValueProperty, 4.0);

                VERIFY_ARE_EQUAL(13, (signed) record.size(), L"Should be thirteen recorded events");

            // Remove handlers

                brushOne->UnregisterPropertyChangedCallback(SolidColorBrush::ColorProperty, tokens[0]);
                buttonTwo->UnregisterPropertyChangedCallback(Button::FontSizeProperty, tokens[1]);
                buttonTwo->UnregisterPropertyChangedCallback(Button::FontSizeProperty, tokens[1]); // Removing extra times is harmless
                gridThree->UnregisterPropertyChangedCallback(Grid::HeightProperty, tokens[2]);
                checkOne->UnregisterPropertyChangedCallback(CheckBox::IsCheckedProperty, tokens[7]);

            // Set properties

                brushOne->SetValue(SolidColorBrush::ColorProperty, Microsoft::UI::Colors::AliceBlue);
                brushTwo->SetValue(SolidColorBrush::ColorProperty, Microsoft::UI::Colors::Orange);
                buttonTwo->SetValue(Button::FontSizeProperty, 11.5);
                buttonTwo->SetValue(Button::FontWeightProperty, Microsoft::UI::Text::FontWeights::Bold);
                gridThree->SetValue(Grid::HeightProperty, 122.0);
                gridThree->SetValue(Grid::WidthProperty, 182.0);
                checkOne->SetValue(CheckBox::IsCheckedProperty, true);

                VERIFY_ARE_EQUAL(16, (signed) record.size(), L"Should be sixteen recorded events");

            // Try removing handlers that aren't hooked up

                brushTwo->UnregisterPropertyChangedCallback(SolidColorBrush::ColorProperty, tokens[0]);
                buttonTwo->UnregisterPropertyChangedCallback(Button::FontSizeProperty, tokens[2]);
                gridThree->UnregisterPropertyChangedCallback(Grid::WidthProperty, tokens[1]);
                sliderOne->UnregisterPropertyChangedCallback(Slider::ValueProperty, tokens[4]);

            // Set properties

                brushOne->SetValue(SolidColorBrush::ColorProperty, Microsoft::UI::Colors::Orange);
                brushTwo->SetValue(SolidColorBrush::ColorProperty, Microsoft::UI::Colors::AliceBlue);
                buttonTwo->SetValue(Button::FontSizeProperty, 10.5);
                buttonTwo->SetValue(Button::FontWeightProperty, Microsoft::UI::Text::FontWeights::ExtraBold);
                gridThree->SetValue(Grid::HeightProperty, 123.0);
                gridThree->SetValue(Grid::WidthProperty, 183.0);
                checkOne->SetValue(CheckBox::IsCheckedProperty, false);
                sliderOne->SetValue(Slider::ValueProperty, 5.0);

                VERIFY_ARE_EQUAL(19, (signed) record.size(), L"Should be nineteen recorded events");
            });
        }
    }
} } } } } } }
