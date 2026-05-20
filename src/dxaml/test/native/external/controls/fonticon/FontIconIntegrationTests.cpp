// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "FontIconIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <Utils.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Markup;
using namespace ::Windows::UI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace FontIcon {

    bool FontIconIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool FontIconIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool FontIconIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void FontIconIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::FontIcon>::CanInstantiate();
    }

    void FontIconIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::FontIcon>::CanEnterAndLeaveLiveTree();
    }

    void FontIconIntegrationTests::CanSetAndGetProperties()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto fontIcon = ref new xaml_controls::FontIcon();

            LOG_OUTPUT(L"Verifying default values for FontIcon properties.");
            VERIFY_IS_TRUE(fontIcon->Glyph->IsEmpty());
            VERIFY_ARE_EQUAL(fontIcon->FontSize, 20.0);
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(fontIcon->FontFamily->Source, L"Segoe Fluent Icons,Segoe MDL2 Assets") == 0);
            VERIFY_ARE_EQUAL(fontIcon->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Normal.Weight);
            VERIFY_ARE_EQUAL(fontIcon->FontStyle, wut::FontStyle::Normal);
            VERIFY_ARE_EQUAL(fontIcon->IsTextScaleFactorEnabled, true);

            LOG_OUTPUT(L"Verifying set/get for FontIcon properties.");
            Platform::String^ glyph = "glyph";
            fontIcon->Glyph = glyph;
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(fontIcon->Glyph, glyph) == 0);

            const double fontSize = 35.0;
            fontIcon->FontSize = 35.0;
            VERIFY_ARE_EQUAL(fontIcon->FontSize, fontSize);

            auto fontFamily = ref new xaml_media::FontFamily("Wingdings");
            fontIcon->FontFamily = fontFamily;
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(fontIcon->FontFamily->Source, fontFamily->Source) == 0);

            wut::FontWeight fontWeight = { 500 };
            fontIcon->FontWeight = fontWeight;
            VERIFY_ARE_EQUAL(fontIcon->FontWeight.Weight, fontWeight.Weight);

            auto fontStyle = wut::FontStyle::Italic;
            fontIcon->FontStyle = fontStyle;
            VERIFY_ARE_EQUAL(fontIcon->FontStyle, fontStyle);

            fontIcon->IsTextScaleFactorEnabled = false;
            VERIFY_ARE_EQUAL(fontIcon->IsTextScaleFactorEnabled, false);
        });
    }

    void FontIconIntegrationTests::CanSetForegroundWithVSM()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Control^ control;
        xaml_controls::FontIcon^ fontIcon;
        xaml_controls::TextBlock^ child;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button Foreground='Green'>"
                L"      <Button.Template>"
                L"        <ControlTemplate TargetType='Button'>"
                L"          <StackPanel Background='Transparent'>"
                L""
                L"            <FontIcon"
                L"              x:Name='ContentHost'"
                L"              FontFamily='Arial'"
                L"              FontSize='12'"
                L"              Glyph='Foo' />"
                L""
                L"            <VisualStateManager.VisualStateGroups>"
                L"              <VisualStateGroup x:Name='CommonStates'>"
                L"                <VisualState x:Name='Normal' />"
                L"                <VisualState x:Name='VisualState1'>"
                L"                  <Storyboard>"
                L"                    <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ContentHost' Storyboard.TargetProperty='Foreground'>"
                L"                      <DiscreteObjectKeyFrame KeyTime='0' Value='Red' />"
                L"                    </ObjectAnimationUsingKeyFrames>"
                L"                  </Storyboard>"
                L"                </VisualState>"
                L"              </VisualStateGroup>"
                L"            </VisualStateManager.VisualStateGroups>"
                L"          </StackPanel>"
                L"        </ControlTemplate>"
                L"      </Button.Template>"
                L"    </Button>"
                L"</StackPanel>";

            auto panel = static_cast<xaml_controls::Panel^>(xaml_markup::XamlReader::Load(xamlString));
            control = safe_cast<xaml_controls::Control^>(panel->Children->GetAt(0));

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Get FontIcon and its TextBlock child.
            fontIcon = safe_cast<xaml_controls::FontIcon^>(TreeHelper::GetVisualChildByName(control, L"ContentHost"));
            VERIFY_IS_NOT_NULL(fontIcon);

            child = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(fontIcon);
            VERIFY_IS_NOT_NULL(child);

            // Foregrounds should be Green before animation.
            auto brush = static_cast<xaml_media::SolidColorBrush^>(fontIcon->Foreground);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"FontIcon.Foreground should be Green");
            brush = static_cast<xaml_media::SolidColorBrush^>(child->Foreground);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"TextBlock.Foreground should be Green");

            // Start VisualState1 animation and verify the foregrounds change to Red.
            VisualStateManager::GoToState(control, "VisualState1", true);

            brush = static_cast<xaml_media::SolidColorBrush^>(fontIcon->Foreground);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"FontIcon.Foreground should be Red");
            brush = static_cast<xaml_media::SolidColorBrush^>(child->Foreground);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"TextBlock.Foreground should be Red");
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Clear VisualState1 animation and verify the foregrounds change to Green.
            VisualStateManager::GoToState(control, "Normal", true);

            auto brush = static_cast<xaml_media::SolidColorBrush^>(fontIcon->Foreground);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"FontIcon.Foreground should be Green");
            brush = static_cast<xaml_media::SolidColorBrush^>(child->Foreground);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"TextBlock.Foreground should be Green");
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void FontIconIntegrationTests::TextFormattingPropertiesInheritRS4()
    {
        TextFormattingPropertiesInheritInternal(false /*areFontIconFontPropertiesInheritable*/);
    }

    void FontIconIntegrationTests::TextFormattingPropertiesInheritInternal(bool areFontIconFontPropertiesInheritable)
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Control^ control;
        xaml_controls::FontIcon^ fontIcon;
        xaml_controls::TextBlock^ child;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button Foreground='Green' "
                L"            FontFamily='Arial' "
                L"            FontSize='12' "
                L"            FontWeight='Bold' "
                L"            FontStyle='Italic' "
                L"            IsTextScaleFactorEnabled='True'>"
                L"      <Button.Template>"
                L"        <ControlTemplate TargetType='Button'>"
                L"          <StackPanel>"
                L"            <FontIcon x:Name='ContentHost'"
                L"                      Glyph='Foo' />"
                L"          </StackPanel>"
                L"        </ControlTemplate>"
                L"      </Button.Template>"
                L"    </Button>"
                L"</StackPanel>";

            auto panel = static_cast<xaml_controls::Panel^>(xaml_markup::XamlReader::Load(xamlString));
            control = safe_cast<xaml_controls::Control^>(panel->Children->GetAt(0));

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Get FontIcon and its TextBlock child.
            fontIcon = safe_cast<xaml_controls::FontIcon^>(TreeHelper::GetVisualChildByName(control, L"ContentHost"));
            VERIFY_IS_NOT_NULL(fontIcon);

            child = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(fontIcon);
            VERIFY_IS_NOT_NULL(child);

            // Clear properties that FontIcon sets automatically when it initializes.
            fontIcon->ClearValue(xaml_controls::FontIcon::FontFamilyProperty);
            fontIcon->ClearValue(xaml_controls::FontIcon::FontSizeProperty);
            fontIcon->ClearValue(xaml_controls::FontIcon::FontWeightProperty);
            fontIcon->ClearValue(xaml_controls::FontIcon::FontStyleProperty);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Foreground
            auto brushControl = static_cast<xaml_media::SolidColorBrush^>(control->Foreground);
            auto brushIcon = static_cast<xaml_media::SolidColorBrush^>(fontIcon->Foreground);
            auto brushChild = static_cast<xaml_media::SolidColorBrush^>(child->Foreground);
            VERIFY_ARE_EQUAL(brushControl->Color, Colors::Green);
            VERIFY_ARE_EQUAL(brushIcon->Color, Colors::Green);
            VERIFY_ARE_EQUAL(brushChild->Color, Colors::Green);

            // FontFamily
            Platform::String^ arial = ref new Platform::String(L"Arial");
            VERIFY_ARE_EQUAL(control->FontFamily->Source, arial);
            if (areFontIconFontPropertiesInheritable)
            {
                VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, arial);
                VERIFY_ARE_EQUAL(child->FontFamily->Source, arial);
            }
            else
            {
                Platform::String^ fontIconDefaultFontFamily = ref new Platform::String(L"Segoe Fluent Icons,Segoe MDL2 Assets");
                VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, fontIconDefaultFontFamily);
                VERIFY_ARE_EQUAL(child->FontFamily->Source, fontIconDefaultFontFamily);
            }

            // FontSize
            VERIFY_ARE_EQUAL(control->FontSize, 12);
            if (areFontIconFontPropertiesInheritable)
            {
                VERIFY_ARE_EQUAL(fontIcon->FontSize, 12);
                VERIFY_ARE_EQUAL(child->FontSize, 12);
            }
            else
            {
                double fontIconDefaultFontSize = 20;
                VERIFY_ARE_EQUAL(fontIcon->FontSize, fontIconDefaultFontSize);
                VERIFY_ARE_EQUAL(child->FontSize, fontIconDefaultFontSize);
            }

            // FontWeight
            VERIFY_ARE_EQUAL(control->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Bold.Weight);
            if (areFontIconFontPropertiesInheritable)
            {
                VERIFY_ARE_EQUAL(fontIcon->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Bold.Weight);
                VERIFY_ARE_EQUAL(child->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Bold.Weight);
            }
            else
            {
                VERIFY_ARE_EQUAL(fontIcon->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Normal.Weight);
                VERIFY_ARE_EQUAL(child->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Normal.Weight);
            }

            // FontStyle
            VERIFY_ARE_EQUAL(control->FontStyle, ::Windows::UI::Text::FontStyle::Italic);
            if (areFontIconFontPropertiesInheritable)
            {
                VERIFY_ARE_EQUAL(fontIcon->FontStyle, ::Windows::UI::Text::FontStyle::Italic);
                VERIFY_ARE_EQUAL(child->FontStyle, ::Windows::UI::Text::FontStyle::Italic);
            }
            else
            {
                VERIFY_ARE_EQUAL(fontIcon->FontStyle, ::Windows::UI::Text::FontStyle::Normal);
                VERIFY_ARE_EQUAL(child->FontStyle, ::Windows::UI::Text::FontStyle::Normal);
            }

            // IsTextScaleFactorEnabled
            // FontIcon always sets this to false on the child.
            VERIFY_ARE_EQUAL(control->IsTextScaleFactorEnabled, true);
            VERIFY_ARE_EQUAL(fontIcon->IsTextScaleFactorEnabled, true);
            VERIFY_ARE_EQUAL(child->IsTextScaleFactorEnabled, false);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void FontIconIntegrationTests::TextFormattingPropertiesOverride()
    {
        using namespace Microsoft::UI::Text;

        TestCleanupWrapper cleanup;
        xaml_controls::Control^ control;
        xaml_controls::FontIcon^ fontIcon;
        xaml_controls::TextBlock^ child;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button Foreground='Green' "
                L"            FontFamily='Arial' "
                L"            FontSize='12' "
                L"            FontWeight='Bold' "
                L"            FontStyle='Italic' "
                L"            IsTextScaleFactorEnabled='True'>"
                L"      <Button.Template>"
                L"        <ControlTemplate TargetType='Button'>"
                L"          <StackPanel>"
                L"            <FontIcon x:Name='ContentHost'"
                L"                      Glyph='Foo' "
                L"                      Foreground='Red' "
                L"                      FontFamily='Times New Roman' "
                L"                      FontSize='14' "
                L"                      FontWeight='ExtraBold' "
                L"                      FontStyle='Oblique' "
                L"                      IsTextScaleFactorEnabled='False' />"
                L"          </StackPanel>"
                L"        </ControlTemplate>"
                L"      </Button.Template>"
                L"    </Button>"
                L"</StackPanel>";

            auto panel = static_cast<xaml_controls::Panel^>(xaml_markup::XamlReader::Load(xamlString));
            control = safe_cast<xaml_controls::Control^>(panel->Children->GetAt(0));

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Get FontIcon and its TextBlock child.
            fontIcon = safe_cast<xaml_controls::FontIcon^>(TreeHelper::GetVisualChildByName(control, L"ContentHost"));
            VERIFY_IS_NOT_NULL(fontIcon);

            child = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(fontIcon);
            VERIFY_IS_NOT_NULL(child);

            // Foreground
            auto brushControl = static_cast<xaml_media::SolidColorBrush^>(control->Foreground);
            auto brushIcon = static_cast<xaml_media::SolidColorBrush^>(fontIcon->Foreground);
            auto brushChild = static_cast<xaml_media::SolidColorBrush^>(child->Foreground);
            VERIFY_ARE_EQUAL(brushControl->Color, Colors::Green);
            VERIFY_ARE_EQUAL(brushIcon->Color, Colors::Red);
            VERIFY_ARE_EQUAL(brushChild->Color, Colors::Red);

            // FontFamily
            Platform::String^ arial = ref new Platform::String(L"Arial");
            Platform::String^ times = ref new Platform::String(L"Times New Roman");
            VERIFY_ARE_EQUAL(control->FontFamily->Source, arial);
            VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, times);
            VERIFY_ARE_EQUAL(child->FontFamily->Source, times);

            // FontSize
            VERIFY_ARE_EQUAL(control->FontSize, 12);
            VERIFY_ARE_EQUAL(fontIcon->FontSize, 14);
            VERIFY_ARE_EQUAL(child->FontSize, 14);

            // FontWeight
            VERIFY_ARE_EQUAL(control->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Bold.Weight);
            VERIFY_ARE_EQUAL(fontIcon->FontWeight.Weight, Microsoft::UI::Text::FontWeights::ExtraBold.Weight);
            VERIFY_ARE_EQUAL(child->FontWeight.Weight, Microsoft::UI::Text::FontWeights::ExtraBold.Weight);

            // FontStyle
            VERIFY_ARE_EQUAL(control->FontStyle, ::Windows::UI::Text::FontStyle::Italic);
            VERIFY_ARE_EQUAL(fontIcon->FontStyle, ::Windows::UI::Text::FontStyle::Oblique);
            VERIFY_ARE_EQUAL(child->FontStyle, ::Windows::UI::Text::FontStyle::Oblique);

            // IsTextScaleFactorEnabled
            // FontIcon always sets this to false on the child.
            VERIFY_ARE_EQUAL(control->IsTextScaleFactorEnabled, true);
            VERIFY_ARE_EQUAL(fontIcon->IsTextScaleFactorEnabled, false);
            VERIFY_ARE_EQUAL(child->IsTextScaleFactorEnabled, false);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void FontIconIntegrationTests::TextBlockIgnoresImplicitStyle()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::FontIcon^ fontIcon;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <StackPanel.Resources>"
                L"        <Style TargetType='TextBlock'>"
                L"            <Setter Property='FontFamily' Value='Times New Roman' />"
                L"        </Style>"
                L"    </StackPanel.Resources>"
                L"    <FontIcon x:Name='FontIcon1' Glyph='B' FontFamily='Arial' />"
                L"</StackPanel>";

            auto panel = static_cast<xaml_controls::Panel^>(xaml_markup::XamlReader::Load(xamlString));
            fontIcon = safe_cast<xaml_controls::FontIcon^>(panel->Children->GetAt(0));

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Verify that the implicit style didn't affect the FontIcon's child TextBlock.
            auto child = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(fontIcon);
            Platform::String^ arial = ref new Platform::String(L"Arial");
            VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, arial);
            VERIFY_ARE_EQUAL(child->FontFamily->Source, arial);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void FontIconIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedFontIconWidth = 20;
        double const expectedFontIconHeight = 20;

        xaml_controls::FontIcon^ fontIcon;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <FontIcon x:Name="fontIcon" Glyph="&#xE0E5;" VerticalAlignment="Center" HorizontalAlignment="Center" />
                    </Grid>)"));

            fontIcon = safe_cast<xaml_controls::FontIcon^>(rootPanel->FindName(L"fontIcon"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedFontIconWidth, fontIcon->ActualWidth);
            VERIFY_ARE_EQUAL(expectedFontIconHeight, fontIcon->ActualHeight);
        });
    }

    void FontIconIntegrationTests::FontPropertiesDefaultValuesRS4()
    {
        FontPropertiesDefaultValuesInternal(true /*isRS4andNewer*/);
    }

    void FontIconIntegrationTests::FontPropertiesDefaultValuesInternal(bool isRS4andNewer)
    {
        TestCleanupWrapper cleanup;
        xaml_controls::FontIcon^ fontIcon;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <FontIcon x:Name='FontIcon1' Glyph='B' />"
                L"</StackPanel>";

            auto panel = static_cast<xaml_controls::Panel^>(xaml_markup::XamlReader::Load(xamlString));
            fontIcon = safe_cast<xaml_controls::FontIcon^>(panel->Children->GetAt(0));

            // Clear any local values (only necessary for <=RS3) to ensure these properties
            // will be at their default property values.
            fontIcon->ClearValue(xaml_controls::FontIcon::FontFamilyProperty);
            fontIcon->ClearValue(xaml_controls::FontIcon::FontSizeProperty);
            fontIcon->ClearValue(xaml_controls::FontIcon::FontWeightProperty);
            fontIcon->ClearValue(xaml_controls::FontIcon::FontStyleProperty);

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            if (isRS4andNewer)
            {
                VERIFY_ARE_EQUAL(fontIcon->FontSize, 20.0);
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(fontIcon->FontFamily->Source, L"Segoe Fluent Icons,Segoe MDL2 Assets") == 0);
                VERIFY_ARE_EQUAL(fontIcon->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Normal.Weight);
                VERIFY_ARE_EQUAL(fontIcon->FontStyle, wut::FontStyle::Normal);
            }
            else
            {
                // Note: These FontSize and FontFamily values aren't what they were supposed to default to,
                // but this is what they would get in RS3 and before.
                VERIFY_ARE_EQUAL(fontIcon->FontSize, 15.0); // 15 = defaultFontSize in TextFormatting::CreateDefault()
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(fontIcon->FontFamily->Source, L"Segoe UI") == 0);
                VERIFY_ARE_EQUAL(fontIcon->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Normal.Weight);
                VERIFY_ARE_EQUAL(fontIcon->FontStyle, wut::FontStyle::Normal);
            }
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void FontIconIntegrationTests::FontPropertiesFromStyleRS4()
    {
        FontPropertiesFromStyleInternal(true /*canValuesComeFromStyle*/);
    }

    void FontIconIntegrationTests::FontPropertiesFromStyleInternal(bool canValuesComeFromStyle)
    {
        TestCleanupWrapper cleanup;
        xaml_controls::FontIcon^ fontIcon;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <StackPanel.Resources>"
                L"        <Style x:Name='FontIconStyle' TargetType='FontIcon'>"
                L"            <Setter Property='FontFamily' Value='Arial'/>"
                L"            <Setter Property='FontSize' Value='14'/>"
                L"            <Setter Property='Foreground' Value='Green'/>"
                L"        </Style>"
                L"    </StackPanel.Resources>"
                L"    <FontIcon x:Name='FontIcon1' Glyph='B' Style='{StaticResource ResourceKey=FontIconStyle}' />"
                L"</StackPanel>";

            auto panel = static_cast<xaml_controls::Panel^>(xaml_markup::XamlReader::Load(xamlString));
            fontIcon = safe_cast<xaml_controls::FontIcon^>(panel->Children->GetAt(0));

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Style should have successfully set the Foreground property
            auto brush = static_cast<xaml_media::SolidColorBrush^>(fontIcon->Foreground);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"FontIcon.Foreground should be Green");

            // Verify Style did or didn't set the Font* property values based on canValuesComeFromStyle.
            auto child = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(fontIcon);
            if (canValuesComeFromStyle)
            {
                Platform::String^ arial = ref new Platform::String(L"Arial");
                VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, arial);
                VERIFY_ARE_EQUAL(child->FontFamily->Source, arial);

                VERIFY_ARE_EQUAL(fontIcon->FontSize, 14);
                VERIFY_ARE_EQUAL(child->FontSize, 14);
            }
            else
            {
                // The locally-set values in RS3 and before prevent the style values from being set.
                Platform::String^ fontIconDefaultFontFamily = ref new Platform::String(L"Segoe Fluent Icons,Segoe MDL2 Assets");
                VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, fontIconDefaultFontFamily);
                VERIFY_ARE_EQUAL(child->FontFamily->Source, fontIconDefaultFontFamily);

                double fontIconDefaultFontSize = 20;
                VERIFY_ARE_EQUAL(fontIcon->FontSize, fontIconDefaultFontSize);
                VERIFY_ARE_EQUAL(child->FontSize, fontIconDefaultFontSize);

            }
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void FontIconIntegrationTests::VerifyFontPropertyChanges()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::FontIcon^ fontIcon;
        xaml_controls::FontIcon^ fontIcon2;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <StackPanel.Resources>"
                L"        <Style x:Name='FontIconStyle' TargetType='FontIcon'>"
                L"            <Setter Property='FontFamily' Value='Arial'/>"
                L"            <Setter Property='FontSize' Value='14'/>"
                L"        </Style>"
                L"    </StackPanel.Resources>"
                L"    <FontIcon x:Name='FontIcon1' Glyph='B' />"
                L"    <FontIcon x:Name='FontIcon2' Glyph='B' Style='{StaticResource ResourceKey=FontIconStyle}' />"
                L"</StackPanel>";

            auto panel = static_cast<xaml_controls::Panel^>(xaml_markup::XamlReader::Load(xamlString));
            fontIcon = safe_cast<xaml_controls::FontIcon^>(panel->Children->GetAt(0));
            fontIcon2 = safe_cast<xaml_controls::FontIcon^>(panel->Children->GetAt(1));

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // The property default values
        Platform::String^ defaultFontFamily = ref new Platform::String(L"Segoe Fluent Icons,Segoe MDL2 Assets");
        double defaultFontSize = 20;

        // The style values
        Platform::String^ styleFontFamily = ref new Platform::String(L"Arial");
        double styleFontSize = 14;

        // The local values
        Platform::String^ localFontFamily = ref new Platform::String(L"Times New Roman");
        double localFontSize = 25;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying initial default values.");
            VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, defaultFontFamily);
            VERIFY_ARE_EQUAL(fontIcon->FontSize, defaultFontSize);

            LOG_OUTPUT(L"Setting Style and verifying values.");
            fontIcon->Style = fontIcon2->Style;
            VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, styleFontFamily);
            VERIFY_ARE_EQUAL(fontIcon->FontSize, styleFontSize);

            LOG_OUTPUT(L"Setting local values and verifying values.");
            fontIcon->FontFamily = ref new xaml_media::FontFamily(localFontFamily);
            fontIcon->FontSize = localFontSize;
            VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, localFontFamily);
            VERIFY_ARE_EQUAL(fontIcon->FontSize, localFontSize);

            LOG_OUTPUT(L"Removing local values and verifying return to Style values.");
            fontIcon->ClearValue(xaml_controls::FontIcon::FontFamilyProperty);
            fontIcon->ClearValue(xaml_controls::FontIcon::FontSizeProperty);
            VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, styleFontFamily);
            VERIFY_ARE_EQUAL(fontIcon->FontSize, styleFontSize);

            LOG_OUTPUT(L"Setting local values again, removing style, and verifying values as local.");
            fontIcon->FontFamily = ref new xaml_media::FontFamily(localFontFamily);
            fontIcon->FontSize = localFontSize;
            fontIcon->ClearValue(xaml_controls::FontIcon::StyleProperty);
            VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, localFontFamily);
            VERIFY_ARE_EQUAL(fontIcon->FontSize, localFontSize);

            LOG_OUTPUT(L"Removing local values and verifying return to default values.");
            fontIcon->ClearValue(xaml_controls::FontIcon::FontFamilyProperty);
            fontIcon->ClearValue(xaml_controls::FontIcon::FontSizeProperty);
            VERIFY_ARE_EQUAL(fontIcon->FontFamily->Source, defaultFontFamily);
            VERIFY_ARE_EQUAL(fontIcon->FontSize, defaultFontSize);

            LOG_OUTPUT(L"Setting local values for FontWeight and FontStyle and verifying values.");
            fontIcon->FontWeight = Microsoft::UI::Text::FontWeights::Bold;
            fontIcon->FontStyle = wut::FontStyle::Italic;
            VERIFY_ARE_EQUAL(fontIcon->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Bold.Weight);
            VERIFY_ARE_EQUAL(fontIcon->FontStyle, wut::FontStyle::Italic);

            LOG_OUTPUT(L"Removing local values for FontWeight and FontStyle and verifying default values.");
            fontIcon->ClearValue(xaml_controls::FontIcon::FontWeightProperty);
            fontIcon->ClearValue(xaml_controls::FontIcon::FontStyleProperty);
            VERIFY_ARE_EQUAL(fontIcon->FontWeight.Weight, Microsoft::UI::Text::FontWeights::Normal.Weight);
            VERIFY_ARE_EQUAL(fontIcon->FontStyle, wut::FontStyle::Normal);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void FontIconIntegrationTests::MirroredWhenRightToLeft()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ spDefaultLTR;
        xaml_controls::StackPanel^ spDefaultRTL;
        xaml_controls::FontIcon^ fontIconInDefaultLTRStackPanel;
        xaml_controls::FontIcon^ fontIconInDefaultRTLStackPanel;
        xaml_controls::FontIcon^ fontIconLocalDefaultLTR;
        xaml_controls::FontIcon^ fontIconLocalDefaultRTL;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <StackPanel x:Name='spDefaultLTR'>"
                L"      <FontIcon Glyph='&#xE751;' MirroredWhenRightToLeft='True' />"
                L"    </StackPanel>"
                L"    <StackPanel x:Name='spDefaultRTL' FlowDirection='RightToleft'>"
                L"      <FontIcon Glyph='&#xE751;' MirroredWhenRightToLeft='True' />"
                L"    </StackPanel>"
                L"    <StackPanel>"
                L"      <FontIcon x:Name='fontIconLocalDefaultLTR' FlowDirection='LeftToRight' Glyph='&#xE751;' MirroredWhenRightToLeft='True' />"
                L"      <FontIcon x:Name='fontIconLocalDefaultRTL' FlowDirection='RightToLeft' Glyph='&#xE751;' MirroredWhenRightToLeft='True' />"
                L"    </StackPanel>"
                L"</StackPanel>";

            auto rootPanel = static_cast<xaml_controls::Panel^>(xaml_markup::XamlReader::Load(xamlString));

            spDefaultLTR = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"spDefaultLTR"));
            spDefaultRTL = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"spDefaultRTL"));
            fontIconInDefaultLTRStackPanel = safe_cast<xaml_controls::FontIcon^>(spDefaultLTR->Children->GetAt(0));
            fontIconInDefaultRTLStackPanel = safe_cast<xaml_controls::FontIcon^>(spDefaultRTL->Children->GetAt(0));
            fontIconLocalDefaultLTR = safe_cast<xaml_controls::FontIcon^>(rootPanel->FindName(L"fontIconLocalDefaultLTR"));
            fontIconLocalDefaultRTL = safe_cast<xaml_controls::FontIcon^>(rootPanel->FindName(L"fontIconLocalDefaultRTL"));

            VERIFY_IS_NOT_NULL(fontIconInDefaultLTRStackPanel);
            VERIFY_IS_NOT_NULL(fontIconInDefaultRTLStackPanel);
            VERIFY_IS_NOT_NULL(fontIconLocalDefaultLTR);
            VERIFY_IS_NOT_NULL(fontIconLocalDefaultRTL);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying initial state of FontIcons.");
            VERIFY_IS_NOT_NULL(fontIconInDefaultLTRStackPanel->RenderTransform);
            VERIFY_IS_NOT_NULL(fontIconInDefaultRTLStackPanel->RenderTransform);
            VERIFY_IS_NOT_NULL(fontIconLocalDefaultLTR->RenderTransform);
            VERIFY_IS_NOT_NULL(fontIconLocalDefaultRTL->RenderTransform);

            // Note: the LTR ones have never been flipped yet and will therefore have a default GeneralTransform
            // rather than a ScaleTransform.
            VERIFY_IS_NOT_NULL(safe_cast<xaml_media::GeneralTransform^>(fontIconInDefaultLTRStackPanel->RenderTransform)); // just a GeneralTransform!
            VERIFY_IS_NOT_NULL(safe_cast<xaml_media::ScaleTransform^>(fontIconInDefaultRTLStackPanel->RenderTransform));
            VERIFY_IS_NOT_NULL(safe_cast<xaml_media::GeneralTransform^>(fontIconLocalDefaultLTR->RenderTransform)); // just a GeneralTransform!
            VERIFY_IS_NOT_NULL(safe_cast<xaml_media::ScaleTransform^>(fontIconLocalDefaultRTL->RenderTransform));

            // Test the ScaleX on the two RTL elements
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::ScaleTransform^>(fontIconInDefaultRTLStackPanel->RenderTransform)->ScaleX, -1.0);
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::ScaleTransform^>(fontIconLocalDefaultRTL->RenderTransform)->ScaleX, -1.0);

            LOG_OUTPUT(L"Swapping FlowDirection to non-default state");
            spDefaultLTR->FlowDirection = FlowDirection::RightToLeft;
            spDefaultRTL->FlowDirection = FlowDirection::LeftToRight;
            fontIconLocalDefaultLTR->FlowDirection = FlowDirection::RightToLeft;
            fontIconLocalDefaultRTL->FlowDirection = FlowDirection::LeftToRight;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying FontIcons with swapped FlowDirections");
            // All elements have now been flipped at some point, so all should now have a ScaleTransform
            VERIFY_IS_NOT_NULL(safe_cast<xaml_media::ScaleTransform^>(fontIconInDefaultLTRStackPanel->RenderTransform));
            VERIFY_IS_NOT_NULL(safe_cast<xaml_media::ScaleTransform^>(fontIconInDefaultRTLStackPanel->RenderTransform));
            VERIFY_IS_NOT_NULL(safe_cast<xaml_media::ScaleTransform^>(fontIconLocalDefaultLTR->RenderTransform));
            VERIFY_IS_NOT_NULL(safe_cast<xaml_media::ScaleTransform^>(fontIconLocalDefaultRTL->RenderTransform));

            VERIFY_ARE_EQUAL(safe_cast<xaml_media::ScaleTransform^>(fontIconInDefaultLTRStackPanel->RenderTransform)->ScaleX, -1.0);
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::ScaleTransform^>(fontIconInDefaultRTLStackPanel->RenderTransform)->ScaleX,  1.0);
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::ScaleTransform^>(fontIconLocalDefaultLTR->RenderTransform)->ScaleX, -1.0);
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::ScaleTransform^>(fontIconLocalDefaultRTL->RenderTransform)->ScaleX,  1.0);

            LOG_OUTPUT(L"Swapping FlowDirection back to default state");
            spDefaultLTR->FlowDirection = FlowDirection::LeftToRight;
            spDefaultRTL->FlowDirection = FlowDirection::RightToLeft;
            fontIconLocalDefaultLTR->FlowDirection = FlowDirection::LeftToRight;
            fontIconLocalDefaultRTL->FlowDirection = FlowDirection::RightToLeft;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying FontIcons back in default FlowDirections");
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::ScaleTransform^>(fontIconInDefaultLTRStackPanel->RenderTransform)->ScaleX,  1.0);
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::ScaleTransform^>(fontIconInDefaultRTLStackPanel->RenderTransform)->ScaleX, -1.0);
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::ScaleTransform^>(fontIconLocalDefaultLTR->RenderTransform)->ScaleX,  1.0);
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::ScaleTransform^>(fontIconLocalDefaultRTL->RenderTransform)->ScaleX, -1.0);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::FontIcon
