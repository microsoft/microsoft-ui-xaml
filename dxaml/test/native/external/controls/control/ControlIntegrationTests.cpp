// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ControlIntegrationTests.h"

#include <generic\ControlTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Local {

    [wf::Metadata::WebHostHidden]
    public ref class TestControl sealed : public xaml_controls::Control
    {
    public:
        TestControl()
        {}

        xaml::DependencyObject^ GetTemplateChildPublic(Platform::String^ childName)
        {
            return Control::GetTemplateChild(childName);
        }

    }; // pubic ref class TestControl

    [wf::Metadata::WebHostHidden]
    public ref class TestTextBox sealed : public xaml_controls::TextBox
    {
    public:
        TestTextBox()
            : m_isMeasured(false)
        {}

    public:
        void ClearMeasuredFlag()
        {
            m_isMeasured = false;
        }

        bool IsMeasured() { return m_isMeasured; }

    protected:

        ::Windows::Foundation::Size MeasureOverride(::Windows::Foundation::Size availableSize) override
        {
            m_isMeasured = true;
            return xaml_controls::Control::MeasureOverride(availableSize);
        }

    private:
        bool m_isMeasured;
    }; // pubic ref class TestTextBox

} // namespace Local

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Control {

    bool ControlIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ControlIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ControlIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ControlIntegrationTests::ValidateDefaultPropertyValues()
    {
        Generic::ControlTests<Local::TestControl>::ValidatePropertyValues();
    }

    void ControlIntegrationTests::DoesFireIsEnabledChanged()
    {
        Generic::ControlTests<Local::TestControl>::DoesFireIsEnabledChanged();
    }

    void ControlIntegrationTests::CanSetCustomControlTemplate()
    {
        TestCleanupWrapper cleanup;

        Local::TestControl^ control = nullptr;
        Platform::String^ expectedString = L"control template string";

        RunOnUIThread([&]()
        {
            auto controlTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  TargetType='Control'>"
                L"  <TextBox x:Name='Root' Text='" + expectedString + L"'/>"
                L"</ControlTemplate>"
                ));

            control = ref new Local::TestControl();
            control->Template = controlTemplate;

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(control);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto rootTextBox = safe_cast<xaml_controls::TextBox^>(control->GetTemplateChildPublic(L"Root"));
            VERIFY_IS_NOT_NULL(rootTextBox);
            VERIFY_ARE_EQUAL(rootTextBox->Text, expectedString);
        });
    }

    void ControlIntegrationTests::CanSetNestedPopupControlTemplate()
    {
        TestCleanupWrapper cleanup;

        Local::TestControl^ control = nullptr;
        Platform::String^ expectedString = L"control template string";

        RunOnUIThread([&]()
        {
            auto controlTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  TargetType='Control'>"
                L" <Popup x:Name='popup1' IsOpen='True' >"
                L"   <StackPanel>"
                L"   <Rectangle Width='100' Height='100' Fill='Green'/>"
                L"   <Popup x:Name='popup2' IsOpen='True' >"
                L"      <TextBlock x:Name='tb' Text='" + expectedString + L"'/>"
                L"   </Popup>"
                L"   </StackPanel>"
                L" </Popup>"
                L"</ControlTemplate>"
                ));

            control = ref new Local::TestControl();
            control->Template = controlTemplate;

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(control);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto textBlock = safe_cast<xaml_controls::TextBlock^>(control->GetTemplateChildPublic(L"tb"));
            auto popup1 =  safe_cast<xaml_primitives::Popup^>(control->GetTemplateChildPublic(L"popup1"));
            auto popup2 =  safe_cast<xaml_primitives::Popup^>(control->GetTemplateChildPublic(L"popup2"));
            VERIFY_IS_NOT_NULL(textBlock);
            VERIFY_IS_NOT_NULL(popup1);
            VERIFY_IS_NOT_NULL(popup2);
            VERIFY_ARE_EQUAL(textBlock->Text, expectedString);
            popup2->IsOpen = false;
            popup1->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ControlIntegrationTests::DoNotPropragateMeasureDirtyDownWhenReassignSamePropertyValue()
    {
        TestCleanupWrapper cleanup;

        Local::TestControl^ control = nullptr;

        RunOnUIThread([&]()
        {
            control = ref new Local::TestControl();

            auto controlTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  TargetType='Control'>"
                L"  <Grid x:Name='rootGrid'/>"
                L"</ControlTemplate>"
                ));

            control->Template = controlTemplate;

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(control);
        });
        TestServices::WindowHelper->WaitForIdle();


        RunOnUIThread([&]()
        {
            auto rootGrid = safe_cast<xaml_controls::Grid^>(control->GetTemplateChildPublic(L"rootGrid"));
            auto textBox = ref new Local::TestTextBox();
            rootGrid->Children->Append(textBox);
            control->UpdateLayout();

            textBox->ClearMeasuredFlag();

            // Reassign same value, textbox is not measure dirty
            LOG_OUTPUT(L"Setting flowdirection to LTR (not changed).");
            control->FlowDirection = control->FlowDirection;
            control->UpdateLayout();
            VERIFY_IS_FALSE(textBox->IsMeasured());

            // Assign a new value, textbox is measure dirty
            LOG_OUTPUT(L"Setting flowdirection to RTL (changed).");
            control->FlowDirection = Microsoft::UI::Xaml::FlowDirection::RightToLeft;
            control->UpdateLayout();
            VERIFY_IS_TRUE(textBox->IsMeasured());
        });
    }

    void ControlIntegrationTests::DoesTemplateBindingMatchThemeWhenChangedDuringAnimation()
    {
        TestCleanupWrapper cleanup;

        Local::TestControl^ control = nullptr;

        RunOnUIThread([&]()
        {
            auto controlStyle = safe_cast<xaml::Style^>(xaml_markup::XamlReader::Load(
                L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' TargetType='Control'>"
                L"    <Setter Property='Background' Value='{ThemeResource ApplicationPageBackgroundThemeBrush}'/>"
                L"    <Setter Property='Template'>"
                L"        <Setter.Value>"
                L"            <ControlTemplate TargetType='Control'>"
                L"                <Grid>"
                L"                    <VisualStateManager.VisualStateGroups>"
                L"                        <VisualStateGroup x:Name='States'>"
                L"                            <VisualState x:Name='Default'/>"
                L"                            <VisualState x:Name='SomeState'>"
                L"                                <VisualState.Setters>"
                L"                                    <Setter Target='rect.(Shape.Fill)' Value='Orange'/>"
                L"                                </VisualState.Setters>"
                L"                            </VisualState>"
                L"                        </VisualStateGroup>"
                L"                    </VisualStateManager.VisualStateGroups>"
                L"                    <Rectangle x:Name='rect' Fill='{TemplateBinding Background}'/>"
                L"                </Grid>"
                L"            </ControlTemplate>"
                L"        </Setter.Value>"
                L"    </Setter>"
                L"</Style>"
                ));

            control = ref new Local::TestControl();
            control->Style = controlStyle;

            // Explicitly set the requested theme.  The bug only reproes when you change the requested theme
            // before templates are applied (such as when you specify RequestedTheme on <Page/> in your
            // MainPage.xaml.
            control->RequestedTheme = xaml::ElementTheme::Light;

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(control);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Put the control in a non-default state.
        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(control, L"SomeState", false /*useTransitions*/);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Change the theme while not in the default state.
        RunOnUIThread([&]()
        {
            control->RequestedTheme = xaml::ElementTheme::Dark;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Put the control back in the default state.
        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(control, L"Default", false /*useTransitions*/);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Validate that our template bound property matches the control's source property.
        RunOnUIThread([&]()
        {
            auto rect = safe_cast<xaml_shapes::Rectangle^>(control->GetTemplateChildPublic(L"rect"));
            auto actualColor = safe_cast<xaml_media::SolidColorBrush^>(rect->Fill)->Color;
            auto expectedColor = safe_cast<xaml_media::SolidColorBrush^>(control->Background)->Color;

            bool doesTargetPropertyMatchSource = expectedColor.A == actualColor.A
                && expectedColor.R == actualColor.R
                && expectedColor.G == actualColor.G
                && expectedColor.B == actualColor.B;

            VERIFY_IS_TRUE(doesTargetPropertyMatchSource);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Button
