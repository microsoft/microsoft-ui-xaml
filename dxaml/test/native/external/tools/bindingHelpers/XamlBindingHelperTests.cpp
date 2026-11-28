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

        void XamlBindingHelperTests::ValidateSetThicknessOnFrameworkElement()
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

                auto connectedGrid = static_cast<Grid^>(page1->FindName(newGridName));
                VERIFY_IS_NOT_NULL(connectedGrid);

                LOG_OUTPUT(L"Setting thickness on Setter's Value property using SetPropertyFromThickness");
                // Create a Thickness object and set it on a Setter via SetPropertyFromThickness using Setter::ValueProperty
                auto setter = ref new Setter();
                Platform::Object^ setterAsObject = setter;
                Thickness marginThickness;
                marginThickness.Left = 10;
                marginThickness.Top = 20;
                marginThickness.Right = 30;
                marginThickness.Bottom = 40;
                XamlBindingHelper::SetPropertyFromThickness(setterAsObject, Setter::ValueProperty, marginThickness);
                // Apply Setter's Value to the grid and verify
                connectedGrid->SetValue(Grid::MarginProperty, setter->Value);
                VERIFY_IS_TRUE(connectedGrid->Margin.Left == marginThickness.Left);
                VERIFY_IS_TRUE(connectedGrid->Margin.Top == marginThickness.Top);
                VERIFY_IS_TRUE(connectedGrid->Margin.Right == marginThickness.Right);
                VERIFY_IS_TRUE(connectedGrid->Margin.Bottom == marginThickness.Bottom);
            });
        }

        void XamlBindingHelperTests::ValidateSetCornerRadiusOnControl()
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

                auto connectedGrid = static_cast<Grid^>(page1->FindName(newGridName));
                VERIFY_IS_NOT_NULL(connectedGrid);

                LOG_OUTPUT(L"Setting corner radius on Setter's Value property using SetPropertyFromCornerRadius");

                auto border = ref new Border();
                // Create a CornerRadius object and set it on a Setter via SetPropertyFromCornerRadius using Setter::ValueProperty
                auto setter = ref new Setter();
                Platform::Object^ setterAsObject = setter;
                CornerRadius cornerRadius;
                cornerRadius.TopLeft = 5;
                cornerRadius.TopRight = 10;
                cornerRadius.BottomRight = 15;
                cornerRadius.BottomLeft = 20;
                XamlBindingHelper::SetPropertyFromCornerRadius(setterAsObject, Setter::ValueProperty, cornerRadius);
                // Apply Setter's Value to the border and verify
                border->SetValue(Border::CornerRadiusProperty, setter->Value);
                VERIFY_IS_TRUE(border->CornerRadius.TopLeft == cornerRadius.TopLeft);
                VERIFY_IS_TRUE(border->CornerRadius.TopRight == cornerRadius.TopRight);
                VERIFY_IS_TRUE(border->CornerRadius.BottomRight == cornerRadius.BottomRight);
                VERIFY_IS_TRUE(border->CornerRadius.BottomLeft == cornerRadius.BottomLeft);
            });
        }

        void XamlBindingHelperTests::ValidateSetColorOnShape()
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

                LOG_OUTPUT(L"Setting color on Setter's Value property using SetPropertyFromColor");
                // Create a brush and set its color on a Setter via SetPropertyFromColor using Setter::ValueProperty
                auto brush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                auto setter = ref new Setter();
                Platform::Object^ setterAsObject = setter;
                auto expectedColor = Microsoft::UI::Colors::Green;
                XamlBindingHelper::SetPropertyFromColor(setterAsObject, Setter::ValueProperty, expectedColor);
                // Apply Setter's Value to the brush and verify
                brush->SetValue(SolidColorBrush::ColorProperty, setter->Value);
                VERIFY_IS_NOT_NULL(brush);
                VERIFY_IS_TRUE(brush->Color.R == expectedColor.R);
                VERIFY_IS_TRUE(brush->Color.G == expectedColor.G);
                VERIFY_IS_TRUE(brush->Color.B == expectedColor.B);
            });
        }

        void XamlBindingHelperTests::ValidateSetPropertyFromTypedValuesOnObjectProperty()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]()
            {
                auto border = ref new Border();
                Platform::Object^ target = border;
                auto tagProperty = FrameworkElement::TagProperty;

                auto readTag = [border]()
                {
                    VERIFY_IS_NOT_NULL(border->Tag);
                    return safe_cast<::Windows::Foundation::IPropertyValue^>(border->Tag);
                };

                // Reads a non-negative integral value regardless of which integral property type it was boxed with,
                // so the checks below only depend on the value round-tripping unchanged.
                auto readNonNegativeIntegral = [](::Windows::Foundation::IPropertyValue^ propertyValue) -> UINT64
                {
                    switch (propertyValue->Type)
                    {
                    case ::Windows::Foundation::PropertyType::UInt8:
                        return propertyValue->GetUInt8();
                    case ::Windows::Foundation::PropertyType::UInt16:
                        return propertyValue->GetUInt16();
                    case ::Windows::Foundation::PropertyType::UInt32:
                        return propertyValue->GetUInt32();
                    case ::Windows::Foundation::PropertyType::UInt64:
                        return propertyValue->GetUInt64();
                    case ::Windows::Foundation::PropertyType::Int16:
                    {
                        const INT16 signedValue = propertyValue->GetInt16();
                        VERIFY_IS_GREATER_THAN_OR_EQUAL(signedValue, static_cast<INT16>(0));
                        return static_cast<UINT64>(signedValue);
                    }
                    case ::Windows::Foundation::PropertyType::Int32:
                    {
                        const INT32 signedValue = propertyValue->GetInt32();
                        VERIFY_IS_GREATER_THAN_OR_EQUAL(signedValue, 0);
                        return static_cast<UINT64>(signedValue);
                    }
                    case ::Windows::Foundation::PropertyType::Int64:
                    {
                        const INT64 signedValue = propertyValue->GetInt64();
                        VERIFY_IS_GREATER_THAN_OR_EQUAL(signedValue, 0ll);
                        return static_cast<UINT64>(signedValue);
                    }
                    default:
                        VERIFY_FAIL(L"Expected an integral property value.");
                        return 0;
                    }
                };

                // Reads a floating-point value regardless of whether it was boxed as Single or Double.
                auto readFloatingPoint = [](::Windows::Foundation::IPropertyValue^ propertyValue) -> double
                {
                    switch (propertyValue->Type)
                    {
                    case ::Windows::Foundation::PropertyType::Single:
                        return propertyValue->GetSingle();
                    case ::Windows::Foundation::PropertyType::Double:
                        return propertyValue->GetDouble();
                    default:
                        VERIFY_FAIL(L"Expected a floating-point property value.");
                        return 0;
                    }
                };

                LOG_OUTPUT(L"Int64 values (beyond the Int32 range) are stored and read back as Int64.");
                XamlBindingHelper::SetPropertyFromInt64(target, tagProperty, -5000000000ll);
                auto value = readTag();
                VERIFY_ARE_EQUAL(::Windows::Foundation::PropertyType::Int64, value->Type);
                VERIFY_ARE_EQUAL(-5000000000ll, value->GetInt64());

                LOG_OUTPUT(L"In-range UInt64 values (beyond the UInt32 range, up to the Int64 maximum) round-trip unchanged.");
                XamlBindingHelper::SetPropertyFromUInt64(target, tagProperty, 5000000000ull);
                VERIFY_ARE_EQUAL(5000000000ull, readNonNegativeIntegral(readTag()));
                XamlBindingHelper::SetPropertyFromUInt64(target, tagProperty, 0x7FFFFFFFFFFFFFFFull);
                VERIFY_ARE_EQUAL(0x7FFFFFFFFFFFFFFFull, readNonNegativeIntegral(readTag()));

                LOG_OUTPUT(L"Int32 values are stored and read back as Int32.");
                XamlBindingHelper::SetPropertyFromInt32(target, tagProperty, -3);
                value = readTag();
                VERIFY_ARE_EQUAL(::Windows::Foundation::PropertyType::Int32, value->Type);
                VERIFY_ARE_EQUAL(-3, value->GetInt32());

                LOG_OUTPUT(L"In-range UInt32 values (up to the Int32 maximum) round-trip unchanged.");
                XamlBindingHelper::SetPropertyFromUInt32(target, tagProperty, 7u);
                VERIFY_ARE_EQUAL(7ull, readNonNegativeIntegral(readTag()));
                XamlBindingHelper::SetPropertyFromUInt32(target, tagProperty, 0x7FFFFFFFu);
                VERIFY_ARE_EQUAL(0x7FFFFFFFull, readNonNegativeIntegral(readTag()));

                LOG_OUTPUT(L"Boolean values.");
                XamlBindingHelper::SetPropertyFromBoolean(target, tagProperty, true);
                value = readTag();
                VERIFY_ARE_EQUAL(::Windows::Foundation::PropertyType::Boolean, value->Type);
                VERIFY_IS_TRUE(value->GetBoolean());
                XamlBindingHelper::SetPropertyFromBoolean(target, tagProperty, false);
                value = readTag();
                VERIFY_ARE_EQUAL(::Windows::Foundation::PropertyType::Boolean, value->Type);
                VERIFY_IS_FALSE(value->GetBoolean());

                LOG_OUTPUT(L"Single values round-trip unchanged as a floating-point value.");
                XamlBindingHelper::SetPropertyFromSingle(target, tagProperty, 1.5f);
                VERIFY_ARE_EQUAL(1.5, readFloatingPoint(readTag()));
                XamlBindingHelper::SetPropertyFromSingle(target, tagProperty, -0.25f);
                VERIFY_ARE_EQUAL(-0.25, readFloatingPoint(readTag()));

                LOG_OUTPUT(L"TimeSpan values.");
                ::Windows::Foundation::TimeSpan timeSpan;
                timeSpan.Duration = 12345678;
                XamlBindingHelper::SetPropertyFromTimeSpan(target, tagProperty, timeSpan);
                value = readTag();
                VERIFY_ARE_EQUAL(::Windows::Foundation::PropertyType::TimeSpan, value->Type);
                VERIFY_ARE_EQUAL(12345678ll, value->GetTimeSpan().Duration);

                LOG_OUTPUT(L"DateTime values.");
                ::Windows::Foundation::DateTime dateTime;
                dateTime.UniversalTime = 132000000000000001ll;
                XamlBindingHelper::SetPropertyFromDateTime(target, tagProperty, dateTime);
                value = readTag();
                VERIFY_ARE_EQUAL(::Windows::Foundation::PropertyType::DateTime, value->Type);
                VERIFY_ARE_EQUAL(132000000000000001ll, value->GetDateTime().UniversalTime);

                LOG_OUTPUT(L"Point values.");
                XamlBindingHelper::SetPropertyFromPoint(target, tagProperty, ::Windows::Foundation::Point(1.5f, -2.0f));
                value = readTag();
                VERIFY_ARE_EQUAL(::Windows::Foundation::PropertyType::Point, value->Type);
                VERIFY_ARE_EQUAL(1.5f, value->GetPoint().X);
                VERIFY_ARE_EQUAL(-2.0f, value->GetPoint().Y);

                LOG_OUTPUT(L"Size values.");
                XamlBindingHelper::SetPropertyFromSize(target, tagProperty, ::Windows::Foundation::Size(3.0f, 4.5f));
                value = readTag();
                VERIFY_ARE_EQUAL(::Windows::Foundation::PropertyType::Size, value->Type);
                VERIFY_ARE_EQUAL(3.0f, value->GetSize().Width);
                VERIFY_ARE_EQUAL(4.5f, value->GetSize().Height);

                LOG_OUTPUT(L"Rect values.");
                XamlBindingHelper::SetPropertyFromRect(target, tagProperty, ::Windows::Foundation::Rect(1.0f, 2.0f, 3.0f, 4.0f));
                value = readTag();
                VERIFY_ARE_EQUAL(::Windows::Foundation::PropertyType::Rect, value->Type);
                VERIFY_ARE_EQUAL(1.0f, value->GetRect().X);
                VERIFY_ARE_EQUAL(2.0f, value->GetRect().Y);
                VERIFY_ARE_EQUAL(3.0f, value->GetRect().Width);
                VERIFY_ARE_EQUAL(4.0f, value->GetRect().Height);

                LOG_OUTPUT(L"Uri values keep their absolute URI on an Object-typed property.");
                XamlBindingHelper::SetPropertyFromUri(target, tagProperty, ref new ::Windows::Foundation::Uri(L"http://www.example.com/path?query=1"));
                VERIFY_IS_NOT_NULL(border->Tag);
                auto uriOnObjectProperty = dynamic_cast<::Windows::Foundation::Uri^>(border->Tag);
                Platform::String^ uriText = (uriOnObjectProperty != nullptr) ? uriOnObjectProperty->AbsoluteUri : dynamic_cast<Platform::String^>(border->Tag);
                VERIFY_IS_NOT_NULL(uriText);
                VERIFY_ARE_EQUAL(0, wcscmp(L"http://www.example.com/path?query=1", uriText->Data()));

                LOG_OUTPUT(L"Uri values set on a Uri-typed property read back as the same Uri.");
                auto hyperlinkButton = ref new HyperlinkButton();
                XamlBindingHelper::SetPropertyFromUri(hyperlinkButton, HyperlinkButton::NavigateUriProperty, ref new ::Windows::Foundation::Uri(L"http://www.example.com/path?query=1"));
                VERIFY_IS_NOT_NULL(hyperlinkButton->NavigateUri);
                VERIFY_ARE_EQUAL(0, wcscmp(L"http://www.example.com/path?query=1", hyperlinkButton->NavigateUri->AbsoluteUri->Data()));
                VERIFY_THROWS_WINRT(XamlBindingHelper::SetPropertyFromUri(target, tagProperty, nullptr), Platform::Exception^);
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
