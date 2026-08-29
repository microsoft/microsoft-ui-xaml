// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "StyleWithXBindTests.h"

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Markup;

using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Common;

using Colors = Microsoft::UI::Colors;
using Color = ::Windows::UI::Color;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Styles {

    StyleWithXBindTests::StyleWithXBindTests()
    {
    }

    bool StyleWithXBindTests::ClassSetup()
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

    bool StyleWithXBindTests::TestSetup()
    {
        // Initialize the framework before anything else. We need to make sure the test
        // is in a good state in case the framework was shut down after the previous test.
        // We pass in our registar to inform the WindowHelper that there is custom metadata
        // that needs to be initialized and subsequently cleared when we call ShutdownXaml.
        // Without the custom metadata provider types would not be activatable from XAML and certain
        // parts of Jupiter would fail to function (Jupiter will sometimes look up
        // properties by name, without this metadata that lookup will fail and create
        // lots of HRESULT spew).
        test_infra::TestServices::WindowHelper->InitializeXaml();

        return true;
    }

    bool StyleWithXBindTests::TestCleanup()
    {
        // Shutdown the framework. The purpose of this is to deallocate everything that
        // was allocated during the test and get to a "idle" state. We can then verify test
        // cleanup and check for leaks.
        test_infra::TestServices::WindowHelper->ShutdownXaml();
                    
        // It's very important to have your test clean up the window contents
        // when it completes. When creating new tests be sure to copy this
        // method over or implement it in a similar way. By cleaning
        // up the window content and waiting for the page to go idle you ensure
        // that if your test fails while the UI element tree is being torn down
        // that the failure is associated with your test and doesn't occur
        // nondeterministically in the future. By waiting for the page to go
        // idle you ensure that all transitions have completed and that jupiter
        // is in a 'tabula rasa' state for the next test.
        //
        // Use the TestCleanupWrapper in each test method to handle cleanup, even
        // in cases of failure or repeated runs. Use VerifyTestCleanup here to
        // ensure that the test was cleaned up correctly.
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void StyleWithXBindTests::BasicXBind_Simulated()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

        BasicXBind_Simulated_Helper(false);
   }

    void StyleWithXBindTests::BasicXBindWithFaultIn_Simulated()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

        BasicXBind_Simulated_Helper(true);
    }

    void StyleWithXBindTests::BasicXBind_Simulated_Helper(bool faultIn)
    {
        StackPanel^ stackPanel;
        RunOnUIThread([faultIn, &stackPanel]() 
        {
            stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                L"<StackPanel"
                L"  xmlns='http://schemas.microsoft.com/client/2007' "
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <Style x:Key='theStyle' TargetType='TextBlock'>"
                L"      <Setter Property='Text' x:Name='setter0' x:ConnectionId='0' />"
                L"      <Setter Property='SelectionHighlightColor' Value='Blue' />"
                L"      <Setter Property='Foreground' x:Name='setter1' x:ConnectionId='1' />"
                L"    </Style>"
                L"  </StackPanel.Resources>"
                L"  <TextBlock x:Name='theTextBlock' Style='{StaticResource theStyle}' />"
                L"</StackPanel>"
            ));

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([faultIn, &stackPanel]() 
        {
            auto theTextBlock = safe_cast<TextBlock^>(stackPanel->FindName(Platform::StringReference(L"theTextBlock")));
            auto mutableSetter_Text = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter0")));
            auto mutableSetter_Foreground = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter1")));

            if (faultIn)
            {
                LOG_OUTPUT(L"Cause Style to fault-in");
                auto setters = safe_cast<Style^>(stackPanel->Resources->Lookup(Platform::StringReference(L"theStyle")))->Setters;
            }

            LOG_OUTPUT(L"Verify initial value of TextBlock's properties");
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock->Text);
            VERIFY_ARE_EQUAL(nullptr, theTextBlock->Foreground);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock->SelectionHighlightColor)->Color);

            LOG_OUTPUT(L"Change Setter.Value and verify that TextBlock properties have updated values");
            auto newTextValue = Platform::StringReference(L"it is up with the blue and gold");
            auto newForegroundValue = Colors::Gold;
            mutableSetter_Text->Value = newTextValue;
            mutableSetter_Foreground->Value = newForegroundValue;
            VERIFY_ARE_EQUAL(newTextValue, theTextBlock->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock->SelectionHighlightColor)->Color);

            LOG_OUTPUT(L"Change Setter.Value and verify that Text property has new value");
            newTextValue = Platform::StringReference(L"down with the red");
            newForegroundValue = Colors::Green;
            mutableSetter_Text->Value = newTextValue;
            mutableSetter_Foreground->Value = newForegroundValue;
            VERIFY_ARE_EQUAL(newTextValue, theTextBlock->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock->SelectionHighlightColor)->Color);
        });
    }

    void StyleWithXBindTests::XBindInBaseStyle_Simulated()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

        XBindInBaseStyle_Simulated_Helper(false);
    }

    void StyleWithXBindTests::XBindInBaseStyleWithFaultIn_Simulated()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

        XBindInBaseStyle_Simulated_Helper(true);
    }

    void StyleWithXBindTests::XBindInBaseStyle_Simulated_Helper(bool faultIn)
    {
        StackPanel^ stackPanel;

        RunOnUIThread([faultIn, &stackPanel]() {
            stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                L"<StackPanel"
                L"  xmlns='http://schemas.microsoft.com/client/2007' "
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <Style x:Key='baseStyle' TargetType='TextBlock'>"
                L"      <Setter Property='Text' x:Name='setter0' x:ConnectionId='0' />"
                L"      <Setter Property='Foreground' x:Name='setter1' x:ConnectionId='1' />"
                L"    </Style>"
                L"    <Style x:Key='theStyle_OverrideText' TargetType='TextBlock' BasedOn='{StaticResource baseStyle}'>"
                L"      <Setter Property='Text' x:Name='setter2' x:ConnectionId='2' />"
                L"    </Style>"
                L"    <Style x:Key='theStyle_NoOverride' TargetType='TextBlock' BasedOn='{StaticResource baseStyle}'>"
                L"      <Setter Property='SelectionHighlightColor' Value='Blue' />"
                L"    </Style>"
                L"  </StackPanel.Resources>"
                L"  <TextBlock x:Name='theTextBlock0' Style='{StaticResource baseStyle}' />"
                L"  <TextBlock x:Name='theTextBlock1' Style='{StaticResource theStyle_OverrideText}' />"
                L"  <TextBlock x:Name='theTextBlock2' Style='{StaticResource theStyle_NoOverride}' />"
                L"</StackPanel>"
            ));

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([faultIn, &stackPanel]() 
        {
            auto theTextBlock0 = safe_cast<TextBlock^>(stackPanel->FindName(Platform::StringReference(L"theTextBlock0")));
            auto theTextBlock1 = safe_cast<TextBlock^>(stackPanel->FindName(Platform::StringReference(L"theTextBlock1")));
            auto theTextBlock2 = safe_cast<TextBlock^>(stackPanel->FindName(Platform::StringReference(L"theTextBlock2")));

            auto setter0 = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter0")));
            auto setter1 = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter1")));
            auto setter2 = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter2")));

            if (faultIn)
            {
                LOG_OUTPUT(L"Trigger fault in of styles");
                auto setters = safe_cast<Style^>(stackPanel->Resources->Lookup(Platform::StringReference(L"baseStyle")))->Setters;
                setters = safe_cast<Style^>(stackPanel->Resources->Lookup(Platform::StringReference(L"theStyle_OverrideText")))->Setters;
                setters = safe_cast<Style^>(stackPanel->Resources->Lookup(Platform::StringReference(L"theStyle_NoOverride")))->Setters;
            }

            LOG_OUTPUT(L"Verify initial value of TextBlock properties");
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock0->Text);
            VERIFY_ARE_EQUAL(nullptr, theTextBlock0->Foreground);
            
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock1->Text);
            VERIFY_ARE_EQUAL(nullptr, theTextBlock1->Foreground);

            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock2->Text);
            VERIFY_ARE_EQUAL(nullptr, theTextBlock2->Foreground);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock2->SelectionHighlightColor)->Color);


            LOG_OUTPUT(L"Change Text's Setter.Value on baseStyle");
            auto newBaseTextValue = Platform::StringReference(L"it is up with the blue and gold");
            setter0->Value = newBaseTextValue;
            LOG_OUTPUT(L"Verify new values of TextBlock properties (only theTextBlock0 and theTextBlock2 should have new text)");
            VERIFY_ARE_EQUAL(newBaseTextValue, theTextBlock0->Text);
            VERIFY_ARE_EQUAL(nullptr, theTextBlock0->Foreground);
            
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock1->Text);
            VERIFY_ARE_EQUAL(nullptr, theTextBlock1->Foreground);

            VERIFY_ARE_EQUAL(newBaseTextValue, theTextBlock2->Text);
            VERIFY_ARE_EQUAL(nullptr, theTextBlock2->Foreground);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock2->SelectionHighlightColor)->Color);


            LOG_OUTPUT(L"Change Foreground's Setter.Value on baseStyle");
            auto newForegroundValue = Colors::Pink;
            setter1->Value = newForegroundValue;
            LOG_OUTPUT(L"Verify new values of TextBlock properties (all TextBlocks should have new Foreground color)");
            VERIFY_ARE_EQUAL(newBaseTextValue, theTextBlock0->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock0->Foreground)->Color);
            
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock1->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock1->Foreground)->Color);

            VERIFY_ARE_EQUAL(newBaseTextValue, theTextBlock2->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock2->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock2->SelectionHighlightColor)->Color);


            LOG_OUTPUT(L"Change Text's Setter.Value on theStyle_OverrideText");
            auto newOverrideTextValue = Platform::StringReference(L"down with the red");
            setter2->Value = newOverrideTextValue;
            LOG_OUTPUT(L"Verify new values of TextBlock properties (only theTextBlock1's Text should have changed)");
            VERIFY_ARE_EQUAL(newBaseTextValue, theTextBlock0->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock0->Foreground)->Color);
            
            VERIFY_ARE_EQUAL(newOverrideTextValue, theTextBlock1->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock1->Foreground)->Color);

            VERIFY_ARE_EQUAL(newBaseTextValue, theTextBlock2->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock2->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock2->SelectionHighlightColor)->Color);

        });
    }

    void StyleWithXBindTests::ChangeAppliedStyle()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
        StackPanel^ stackPanel;

        RunOnUIThread([&stackPanel]() {
            stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                L"<StackPanel"
                L"  xmlns='http://schemas.microsoft.com/client/2007' "
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <Style x:Key='styleA' TargetType='TextBlock'>"
                L"      <Setter Property='Text' x:Name='setter0' x:ConnectionId='0' />"
                L"    </Style>"
                L"    <Style x:Key='styleB' TargetType='TextBlock'>"
                L"      <Setter Property='Text' x:Name='setter1' x:ConnectionId='1' />"
                L"    </Style>"
                L"    <Style x:Key='styleC' TargetType='TextBlock'>"
                L"      <Setter Property='Text' x:Name='setter2' x:ConnectionId='2' />"
                L"    </Style>"
                L"  </StackPanel.Resources>"
                L"  <TextBlock x:Name='theTextBlock0' Style='{StaticResource styleA}' />"
                L"  <TextBlock x:Name='theTextBlock1' Style='{StaticResource styleB}' />"
                L"  <TextBlock x:Name='theTextBlock2' Style='{StaticResource styleB}' />"
                L"</StackPanel>"
            ));


            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&stackPanel]() 
        {
            auto theTextBlock0 = safe_cast<TextBlock^>(stackPanel->FindName(Platform::StringReference(L"theTextBlock0")));
            auto theTextBlock1 = safe_cast<TextBlock^>(stackPanel->FindName(Platform::StringReference(L"theTextBlock1")));
            auto theTextBlock2 = safe_cast<TextBlock^>(stackPanel->FindName(Platform::StringReference(L"theTextBlock2")));

            auto setter0 = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter0")));
            auto setter1 = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter1")));
            
            LOG_OUTPUT(L"Verify initial value of TextBlock properties");
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock0->Text);
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock1->Text);
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock2->Text);


            LOG_OUTPUT(L"Change Text's Setter.Value on styleB");
            auto newStyleBTextValue = Platform::StringReference(L"We're the bloody Ubersreik five!");
            setter1->Value = newStyleBTextValue;
            LOG_OUTPUT(L"Verify new values of TextBlock properties (only theTextBlock1's and theTextBlock2's Text should have changed)");
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock0->Text);
            VERIFY_ARE_EQUAL(newStyleBTextValue, theTextBlock1->Text);
            VERIFY_ARE_EQUAL(newStyleBTextValue, theTextBlock2->Text);


            LOG_OUTPUT(L"Changing Style on theTextBlock2");
            auto styleC = safe_cast<Style^>(stackPanel->Resources->Lookup(Platform::StringReference(L"styleC")));
            auto setter2 = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter2")));
            theTextBlock2->Style = styleC;
            LOG_OUTPUT(L"Verify value of TextBlock properties");
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock0->Text);
            VERIFY_ARE_EQUAL(newStyleBTextValue, theTextBlock1->Text);
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock2->Text);


            LOG_OUTPUT(L"Change Text's Setter.Value on styleB");
            auto newerStyleBTextValue = Platform::StringReference(L"Or four.");
            setter1->Value = newerStyleBTextValue;
            LOG_OUTPUT(L"Verify new values of TextBlock properties (only theTextBlock1's Text should have changed)");
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock0->Text);
            VERIFY_ARE_EQUAL(newerStyleBTextValue, theTextBlock1->Text);
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock2->Text);


            LOG_OUTPUT(L"Change Text's Setter.Value on styleC");
            auto newStyleCTextValue = Platform::StringReference(L"It doesn't really matter!");
            setter2->Value = newStyleCTextValue;
            LOG_OUTPUT(L"Verify new values of TextBlock properties (only theTextBlock2's Text should have changed)");
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock0->Text);
            VERIFY_ARE_EQUAL(newerStyleBTextValue, theTextBlock1->Text);
            VERIFY_ARE_EQUAL(newStyleCTextValue, theTextBlock2->Text);
        });
    }

    void StyleWithXBindTests::XBindInImplicitStyle_Simulated()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
        StackPanel^ stackPanel;

        RunOnUIThread([&stackPanel]() {
            stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                L"<StackPanel"
                L"  xmlns='http://schemas.microsoft.com/client/2007' "
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <Style TargetType='TextBlock'>"
                L"      <Setter Property='Text' x:Name='setter0' x:ConnectionId='0' />"
                L"      <Setter Property='SelectionHighlightColor' Value='Blue' />"
                L"      <Setter Property='Foreground' x:Name='setter1' x:ConnectionId='1' />"
                L"    </Style>"
                L"  </StackPanel.Resources>"
                L"  <TextBlock x:Name='theTextBlock' />"
                L"</StackPanel>"
            ));

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&stackPanel]() 
        {
            auto theTextBlock = safe_cast<TextBlock^>(stackPanel->FindName(Platform::StringReference(L"theTextBlock")));
            auto mutableSetter_Text = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter0")));
            auto mutableSetter_Foreground = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter1")));

            LOG_OUTPUT(L"Verify initial value of TextBlock's properties");
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock->Text);
            VERIFY_ARE_EQUAL(nullptr, theTextBlock->Foreground);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock->SelectionHighlightColor)->Color);

            LOG_OUTPUT(L"Change Setter.Value and verify that TextBlock properties have updated values");
            auto newTextValue = Platform::StringReference(L"it is up with the blue and gold");
            auto newForegroundValue = Colors::Gold;
            mutableSetter_Text->Value = newTextValue;
            mutableSetter_Foreground->Value = newForegroundValue;
            VERIFY_ARE_EQUAL(newTextValue, theTextBlock->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock->SelectionHighlightColor)->Color);

            LOG_OUTPUT(L"Change Setter.Value and verify that Text property has new value");
            newTextValue = Platform::StringReference(L"down with the red");
            newForegroundValue = Colors::Green;
            mutableSetter_Text->Value = newTextValue;
            mutableSetter_Foreground->Value = newForegroundValue;
            VERIFY_ARE_EQUAL(newTextValue, theTextBlock->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock->SelectionHighlightColor)->Color);
        });
    }

    void StyleWithXBindTests::XBindInStyleOverriddenByLocalValue_Simulated()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
        StackPanel^ stackPanel;

        RunOnUIThread([&stackPanel]() {
            stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                L"<StackPanel"
                L"  xmlns='http://schemas.microsoft.com/client/2007' "
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <Style TargetType='TextBlock' x:Key='theStyle'>"
                L"      <Setter Property='Text' x:Name='setter0' x:ConnectionId='0' />"
                L"      <Setter Property='SelectionHighlightColor' Value='Blue' />"
                L"      <Setter Property='Foreground' x:Name='setter1' x:ConnectionId='1' />"
                L"    </Style>"
                L"  </StackPanel.Resources>"
                L"  <TextBlock x:Name='theTextBlock' Style='{StaticResource theStyle}' Text='foobar'/>"
                L"</StackPanel>"
            ));

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&stackPanel]() 
        {
            auto theTextBlock = safe_cast<TextBlock^>(stackPanel->FindName(Platform::StringReference(L"theTextBlock")));
            auto mutableSetter_Text = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter0")));
            auto mutableSetter_Foreground = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter1")));

            LOG_OUTPUT(L"Verify initial value of TextBlock's properties");
            VERIFY_ARE_EQUAL(Platform::StringReference(L"foobar"), theTextBlock->Text);
            VERIFY_ARE_EQUAL(nullptr, theTextBlock->Foreground);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock->SelectionHighlightColor)->Color);

            LOG_OUTPUT(L"Change Setter.Value and verify that TextBlock.Foreground has updated value but TextBlock.Text keeps local value");
            auto newTextValue = Platform::StringReference(L"it is up with the blue and gold");
            auto newForegroundValue = Colors::Gold;
            mutableSetter_Text->Value = newTextValue;
            mutableSetter_Foreground->Value = newForegroundValue;
            VERIFY_ARE_EQUAL(Platform::StringReference(L"foobar"), theTextBlock->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock->SelectionHighlightColor)->Color);

            LOG_OUTPUT(L"Change Setter.Value and verify that Text property has local value");
            newTextValue = Platform::StringReference(L"down with the red");
            newForegroundValue = Colors::Green;
            mutableSetter_Text->Value = newTextValue;
            mutableSetter_Foreground->Value = newForegroundValue;
            VERIFY_ARE_EQUAL(Platform::StringReference(L"foobar"), theTextBlock->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock->SelectionHighlightColor)->Color);

            LOG_OUTPUT(L"Clear local value of TextBlock.Text and verify it now matches the value set by the Style");
            theTextBlock->ClearValue(TextBlock::TextProperty);
            VERIFY_ARE_EQUAL(newTextValue, theTextBlock->Text);
            VERIFY_ARE_EQUAL(newForegroundValue, safe_cast<SolidColorBrush^>(theTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(theTextBlock->SelectionHighlightColor)->Color);
        });
    }

    void StyleWithXBindTests::XBindInStyleMultipleProperties_Simulated()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
        StackPanel^ stackPanel;

        RunOnUIThread([&stackPanel]() {
            stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                L"<StackPanel"
                L"  xmlns='http://schemas.microsoft.com/client/2007' "
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <Style x:Key='theStyle' TargetType='TextBlock'>"
                L"      <Setter Property='Text' x:Name='setter0' x:ConnectionId='0' />"
                L"      <Setter Property='Text' x:Name='setter1' x:ConnectionId='1' />"
                L"      <Setter Property='Text' x:Name='setter2' x:ConnectionId='2' />"
                L"      <Setter Property='Text' x:Name='setter3' x:ConnectionId='3' />"
                L"    </Style>"
                L"  </StackPanel.Resources>"
                L"  <TextBlock x:Name='theTextBlock' Style='{StaticResource theStyle}' />"
                L"</StackPanel>"
            ));

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&stackPanel]() 
        {            
            auto theTextBlock = safe_cast<TextBlock^>(stackPanel->FindName(Platform::StringReference(L"theTextBlock")));
            auto setter0 = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter0")));
            auto setter1 = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter1")));
            auto setter2 = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter2")));
            auto setter3 = safe_cast<Setter^>(stackPanel->FindName(Platform::StringReference(L"setter3")));

            LOG_OUTPUT(L"Verify initial value of TextBlock.Text");
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock->Text);

            LOG_OUTPUT(L"Change all Setter.Value and verify that TextBlock.Text matches setter3");
            auto winningTextValue = Platform::StringReference(L"it is up with the blue and gold");
            
            setter0->Value = Platform::StringReference(L"lorem ipsum");
            VERIFY_ARE_EQUAL(Platform::StringReference(L""), theTextBlock->Text);

            setter3->Value = winningTextValue;
            VERIFY_ARE_EQUAL(winningTextValue, theTextBlock->Text);

            setter2->Value = Platform::StringReference(L"down with the red");
            VERIFY_ARE_EQUAL(winningTextValue, theTextBlock->Text);

            setter1->Value = Platform::StringReference(L"these stairs go up!");
            VERIFY_ARE_EQUAL(winningTextValue, theTextBlock->Text);
        });
    }

} } } } } }
