// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Generic {

    template <typename TClassUnderTest>
    class ControlTests
    {
    public:
        typedef std::map<Platform::String^, Platform::Object^> PropertyValuesMap;

        static PropertyValuesMap GetDefaultValuesMap()
        {
            PropertyValuesMap defaultValuesMap;

            defaultValuesMap[L"FontSize"] = 14.0;
            defaultValuesMap[L"FontFamily"] = ref new Platform::String(L"Segoe UI");
            defaultValuesMap[L"FontWeight"] = 400;
            defaultValuesMap[L"FontStyle"] = wut::FontStyle::Normal;
            defaultValuesMap[L"FontStretch"] = wut::FontStretch::Normal;
            defaultValuesMap[L"CharacterSpacing"] = safe_cast<Platform::Object^>(0);
            defaultValuesMap[L"IsTabStop"] = true;
            defaultValuesMap[L"IsEnabled"] = true;
            defaultValuesMap[L"FocusState"] = xaml::FocusState::Unfocused;
            defaultValuesMap[L"TabIndex"] = 2147483647;
            defaultValuesMap[L"TabNavigation"] = xaml_input::KeyboardNavigationMode::Local;
            defaultValuesMap[L"HorizontalContentAlignment"] = xaml::HorizontalAlignment::Center;
            defaultValuesMap[L"VerticalContentAlignment"] = xaml::VerticalAlignment::Center;
            defaultValuesMap[L"Padding"] = xaml::ThicknessHelper::FromUniformLength(0.0);
            defaultValuesMap[L"BorderThickness"] = xaml::ThicknessHelper::FromUniformLength(0.0);
            defaultValuesMap[L"Background"] = nullptr;
            defaultValuesMap[L"BorderBrush"] = nullptr;
            defaultValuesMap[L"Template"] = nullptr;
            defaultValuesMap[L"IsTextScaleFactorEnabled"] = true;

            return defaultValuesMap;
        }

        static void ValidatePropertyValues(PropertyValuesMap valuesMap = GetDefaultValuesMap())
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                auto control = ref new TClassUnderTest();

                VERIFY_ARE_EQUAL(control->FontSize, safe_cast<double>(valuesMap[L"FontSize"]));
                VERIFY_ARE_EQUAL(control->FontFamily->Source, safe_cast<Platform::String^>(valuesMap[L"FontFamily"]));
                VERIFY_ARE_EQUAL(control->FontWeight.Weight, safe_cast<int>(valuesMap[L"FontWeight"]));
                VERIFY_ARE_EQUAL(control->FontStyle, safe_cast<wut::FontStyle>(valuesMap[L"FontStyle"]));
                VERIFY_ARE_EQUAL(control->FontStretch, safe_cast<wut::FontStretch>(valuesMap[L"FontStretch"]));
                VERIFY_ARE_EQUAL(control->CharacterSpacing, safe_cast<int>(valuesMap[L"CharacterSpacing"]));
                VERIFY_ARE_EQUAL(control->IsTabStop, safe_cast<bool>(valuesMap[L"IsTabStop"]));
                VERIFY_ARE_EQUAL(control->IsEnabled, safe_cast<bool>(valuesMap[L"IsEnabled"]));
                VERIFY_ARE_EQUAL(control->FocusState, safe_cast<xaml::FocusState>(valuesMap[L"FocusState"]));
                VERIFY_ARE_EQUAL(control->TabIndex, safe_cast<int>(valuesMap[L"TabIndex"]));
                VERIFY_ARE_EQUAL(control->TabNavigation, safe_cast<xaml_input::KeyboardNavigationMode>(valuesMap[L"TabNavigation"]));
                VERIFY_ARE_EQUAL(control->HorizontalContentAlignment, safe_cast<xaml::HorizontalAlignment>(valuesMap[L"HorizontalContentAlignment"]));
                VERIFY_ARE_EQUAL(control->VerticalContentAlignment, safe_cast<xaml::VerticalAlignment>(valuesMap[L"VerticalContentAlignment"]));

                auto thickL = control->Padding;
                auto thickR = safe_cast<xaml::Thickness>(valuesMap[L"Padding"]);
                VERIFY_ARE_EQUAL(thickL.Left, thickR.Left);
                VERIFY_ARE_EQUAL(thickL.Top, thickR.Top);
                VERIFY_ARE_EQUAL(thickL.Right, thickR.Right);
                VERIFY_ARE_EQUAL(thickL.Bottom, thickR.Bottom);

                thickL = control->BorderThickness;
                thickR = safe_cast<xaml::Thickness>(valuesMap[L"BorderThickness"]);
                VERIFY_ARE_EQUAL(thickL.Left, thickR.Left);
                VERIFY_ARE_EQUAL(thickL.Top, thickR.Top);
                VERIFY_ARE_EQUAL(thickL.Right, thickR.Right);
                VERIFY_ARE_EQUAL(thickL.Bottom, thickR.Bottom);

                VERIFY_ARE_EQUAL(control->Foreground, safe_cast<xaml_media::Brush^>(xaml::Application::Current->Resources->Lookup(L"DefaultTextForegroundThemeBrush")));
                VERIFY_ARE_EQUAL(control->Background, safe_cast<xaml_media::Brush^>(valuesMap[L"Background"]));
                VERIFY_ARE_EQUAL(control->BorderBrush, safe_cast<xaml_media::Brush^>(valuesMap[L"BorderBrush"]));
                VERIFY_ARE_EQUAL(control->Template, safe_cast<xaml_controls::ControlTemplate^>(valuesMap[L"Template"]));
                VERIFY_ARE_EQUAL(control->IsTextScaleFactorEnabled, safe_cast<bool>(valuesMap[L"IsTextScaleFactorEnabled"]));
            });
        }

        static void DoesFireIsEnabledChanged()
        {
            TestCleanupWrapper cleanup;
            TClassUnderTest^ control = nullptr;
            xaml_controls::Grid^ rootGrid = nullptr;

            auto spIsEnabledChangedEvent = std::make_shared<Event>();
            auto isEnabledChangedRegistration = CreateSafeEventRegistration(TClassUnderTest, IsEnabledChanged);

            RunOnUIThread([&]()
            {
                control = ref new TClassUnderTest();

                isEnabledChangedRegistration.Attach(
                    control,
                    ref new xaml::DependencyPropertyChangedEventHandler([spIsEnabledChangedEvent](Platform::Object^ sender, xaml::DependencyPropertyChangedEventArgs^ args)
                    {
                        spIsEnabledChangedEvent->Set();
                    }));

                TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(control);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                control->IsEnabled = false;
            });
            spIsEnabledChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(control->IsEnabled);

                control->IsEnabled = true;
            });
            spIsEnabledChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(control->IsEnabled);
            });
        }

    }; // class ControlTests

} } } } } // namespace Microsoft::UI::Xaml::Tests::Generic
