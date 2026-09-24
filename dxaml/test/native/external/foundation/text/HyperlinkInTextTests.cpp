// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "HyperlinkInTextTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <TreeHelper.h>
#include <SafeEventRegistration.h>
#include <Utils.h>
#include "KeyboardInjectionOverride.h"
#include <WUCRenderingScopeGuard.h>
#include <TestComparisonGuards.h>
#include <FocusTestHelper.h>
#include <AutomationClient\AutomationClientManager.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::UI::ViewManagement;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml::Tests::Automation;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Text {

Platform::String^ HyperlinkInTextTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\text\\";
}

bool HyperlinkInTextTests::ClassSetup()
{
    LOG_OUTPUT(L"Setting up test class");
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool HyperlinkInTextTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool HyperlinkInTextTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

//------------------------------------------------------------------------
// Test case: Verifies that hyperlink in text appears correctly for
// both desktop and phone (both should have an underline due to
// the converged behavior)
//------------------------------------------------------------------------
void HyperlinkInTextTests::ValidateHyperlinkInText()
{
    ValidateHyperlinkInText(false /*testPointerOver*/, true /*withUnderline*/, false /*useHighContrast*/);
}

// Validates the look of hyperlink in text without underline in PointerOver visual state
void HyperlinkInTextTests::ValidateHyperlinkInTextWithoutPointerOverUnderline()
{
    // On Win11 25H2, the PointerOver hover color shifted from light-blue to orange
    // and a new underline visual appears. We use $$variables$$ in the PointerOver
    // XML master so one file works for both OS versions.
    // Text glyph antialiasing also shifted slightly (max 15/255 grayscale),
    // so we set tolerance=15 on 25H2 to share the PNG surface masters.
    if (IsOSBuildAtLeast(26200))
    {
        TestServices::Utilities->SetDCompXmlVariable(L"HyperlinkHoverColor", L"rgb {1, 0.6471, 0}");
        TestServices::Utilities->SetDCompXmlVariable(L"HyperlinkHoverUnderline",
            L"                <wucSpriteVisual SizeX=\"60.84\" SizeY=\"0.87\" BorderMode=\"Soft\" OffsetX=\"140.1\" OffsetY=\"99\" OffsetZ=\"0\">\r\n"
            L"                  <ColorBrush Color=\"rgb {1, 0.6471, 0}\" />\r\n"
            L"                </wucSpriteVisual>\r\n");
    }
    else
    {
        TestServices::Utilities->SetDCompXmlVariable(L"HyperlinkHoverColor", L"rgb {0.651, 0.8471, 1}");
        TestServices::Utilities->SetDCompXmlVariable(L"HyperlinkHoverUnderline", L"");
    }

    ImageCompareToleranceGuard tolerance(IsOSBuildAtLeast(26200) ? 15 : 0);
    ValidateHyperlinkInText(true /*testPointerOver*/, false /*withUnderline*/, false /*useHighContrast*/);
}

// Validates the look of hyperlink in text with underline in HighContrast theme
void HyperlinkInTextTests::ValidateHyperlinkInTextWithHighContrastUnderline()
{
    ValidateHyperlinkInText(true /*testPointerOver*/, false /*withUnderline*/, true /*useHighContrast*/);
}

void HyperlinkInTextTests::ValidateHyperlinkInText(bool testPointerOver, bool withUnderline, bool useHighContrast)
{
    auto resourcesCleanup = wil::scope_exit([]
    {
        RunOnUIThread([]()
        {
            auto mergedDictionaries = Application::Current->Resources->MergedDictionaries;

            if (mergedDictionaries->Size > 0)
            {
                LOG_OUTPUT(L"Removing dictionary with HyperlinkUnderlineVisible=False.");
                mergedDictionaries->RemoveAt(mergedDictionaries->Size - 1);
            }

            TestServices::Utilities->DeleteResourceDictionaryCaches();
        });
    });

    if (testPointerOver)
    {
        TestServices::InputHelper->MoveMouse(wf::Point(0, 0));
    }

    TextBlock^ textBlock;

    auto pointerEnteredTextBlockEvent = std::make_shared<Event>();
    auto pointerEnteredTextBlockRegistration = CreateSafeEventRegistration(TextBlock, PointerEntered);

    // Clear out the current window content before injecting MockDComp, to
    // MockDComp doesn't capture an image for anything currently in the content,
    // since that will interfere with the expected surface counts.
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = nullptr;

        if (!withUnderline)
        {
            auto dictionary = safe_cast<xaml::ResourceDictionary^>(xaml_markup::XamlReader::Load(
                LR"(<ResourceDictionary
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <x:Boolean x:Key="HyperlinkUnderlineVisible">False</x:Boolean>
                    </ResourceDictionary>)"));

            LOG_OUTPUT(L"Adding dictionary with HyperlinkUnderlineVisible=False.");
            Application::Current->Resources->MergedDictionaries->Append(dictionary);
        }

        if (useHighContrast)
        {
            TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
        }
    });

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"HyperlinkInTextTests.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = root;
        textBlock = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(root, L"TextBlock2"));
        VERIFY_IS_NOT_NULL(textBlock);

        pointerEnteredTextBlockRegistration.Attach(textBlock, ref new PointerEventHandler(
            [pointerEnteredTextBlockEvent](Platform::Object^, PointerRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Pointer entered TextBlock.");
                pointerEnteredTextBlockEvent->Set();
            }));
    });

    TestServices::WindowHelper->WaitForIdle();

    if (testPointerOver)
    {
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "Normal");

        LOG_OUTPUT(L"Move the mouse to the TextBlock");
        TestServices::InputHelper->MoveMouse(textBlock, 50, 2);
        pointerEnteredTextBlockEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "PointerOver");
    }
    else
    {
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
    }
}

void HyperlinkInTextTests::UnderlineStyle()
{
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = nullptr;
    });

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Grid^ rootGrid = nullptr;
    Hyperlink^ hyperlink = nullptr;

    RunOnUIThread([&]()
    {
        rootGrid = ref new Grid();
        StackPanel^ stackPanel = ref new StackPanel();
        TextBlock^ tb = ref new TextBlock();
        tb->FontSize = 15;
        hyperlink = ref new Hyperlink();
        hyperlink->FontSize = 15;
        Run^ run = ref new Run();
        run->Text = "Hyperlink";
        hyperlink->UnderlineStyle = UnderlineStyle::None;
        hyperlink->Inlines->Append(run);
        tb->Inlines->Append(hyperlink);
        stackPanel->Children->Append(tb);
        rootGrid->Children->Append(stackPanel);
    });

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "None");

    RunOnUIThread([&]()
    {
        hyperlink->UnderlineStyle = UnderlineStyle::Single;
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "Single");
}

void HyperlinkInTextTests::ValidateHyperlinkWithRequestedTheme()
{
    TestCleanupWrapper cleanup;
    Panel^ panel;
    TextBlock^ textBlock;
    Hyperlink^ hyperlink;
    Button^ button;
    Color colorLight;
    Color colorDark;

    TestServices::InputHelper->MoveMouse(wf::Point(0, 0));

    auto pointerEnteredTextBlockEvent = std::make_shared<Event>();
    auto pointerEnteredTextBlockRegistration = CreateSafeEventRegistration(TextBlock, PointerEntered);
    auto pointerEnteredButtonEvent = std::make_shared<Event>();
    auto pointerEnteredButtonRegistration = CreateSafeEventRegistration(Button, PointerEntered);
    auto loadedRegistration = CreateSafeEventRegistration(Button, Loaded);
    auto loadedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      Background='LightBlue'>"
            L"  <StackPanel VerticalAlignment='Center'>"
            L"    <Button x:Name='Button1' Height='200' Width='200' Content='Button1' />"
            L"    <TextBlock x:Name='TextBlock1' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='Hyperlink1'>hyperlink</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>"
            L"</Page>";

        auto page = safe_cast<Page^> (xaml_markup::XamlReader::Load(xamlString));

        panel = safe_cast<Panel^>(page->Content);
        textBlock = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(panel, L"TextBlock1"));
        button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(panel, L"Button1"));
        hyperlink = safe_cast<Hyperlink^>(textBlock->Inlines->GetAt(0));

        pointerEnteredTextBlockRegistration.Attach(textBlock, ref new PointerEventHandler(
            [pointerEnteredTextBlockEvent](Platform::Object^, PointerRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Pointer entered TextBlock.");
                pointerEnteredTextBlockEvent->Set();
            }));

        pointerEnteredButtonRegistration.Attach(button, ref new PointerEventHandler(
            [pointerEnteredButtonEvent](Platform::Object^, PointerRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Pointer entered Button.");
                pointerEnteredButtonEvent->Set();
            }));

        loadedRegistration.Attach(button, ref new RoutedEventHandler(
            [loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
                LOG_OUTPUT(L"Button loaded");
                loadedEvent->Set();
        }));

        // Set hyperlink's RequestedTheme (inherited) to Light.
        panel->RequestedTheme = ElementTheme::Light;

        TestServices::WindowHelper->WindowContent = page;
    });

    LOG_OUTPUT(L"Wait for Button loaded");
    loadedEvent->WaitForDefault();

    TestServices::InputHelper->LeftMouseClick(textBlock);

    // Force PointerOver change on TextBlock and Hyperlink.
    // This causes a new lookup for the hyperlink's Foreground brush resource.
    LOG_OUTPUT(L"Move the mouse to the Button");
    TestServices::InputHelper->MoveMouse(button);
    pointerEnteredButtonEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Move the mouse to the TextBlock");
    TestServices::InputHelper->MoveMouse(textBlock);
    pointerEnteredTextBlockEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Get the hyperlink's Foreground color while the mouse is over it and RequestedTheme=Light.
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        colorLight = brush->Color;

        // Change hyperlink's RequestedTheme (inherited) to Dark.
        panel->RequestedTheme = ElementTheme::Dark;
    });

    // Force PointerOver change on TextBlock and Hyperlink.
    // This causes a new lookup for the hyperlink's Foreground brush resource.
    LOG_OUTPUT(L"Move the mouse to the Button");
    TestServices::InputHelper->MoveMouse(button);
    pointerEnteredButtonEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Move the mouse to the TextBlock");
    TestServices::InputHelper->MoveMouse(textBlock);
    pointerEnteredTextBlockEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Get the hyperlink's Foreground color while the mouse is over it and RequestedTheme=Dark.
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        colorDark = brush->Color;

        // Verify that the hyperlink's Foreground color changed.
        VERIFY_ARE_NOT_EQUAL(colorLight, colorDark);
    });
}

void HyperlinkInTextTests::ValidateHyperlinkWithLocalForeground()
{
    TestCleanupWrapper cleanup;
    Panel^ panel;
    TextBlock^ textBlock;
    Hyperlink^ hyperlink;
    Button^ button;

    TestServices::InputHelper->MoveMouse(wf::Point(5, 5));

    auto pointerEnteredTextBlockEvent = std::make_shared<Event>();
    auto pointerEnteredTextBlockRegistration = CreateSafeEventRegistration(TextBlock, PointerEntered);
    auto pointerEnteredButtonEvent = std::make_shared<Event>();
    auto pointerEnteredButtonRegistration = CreateSafeEventRegistration(Button, PointerEntered);
    auto loadedRegistration = CreateSafeEventRegistration(Button, Loaded);
    auto loadedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      Background='LightBlue'>"
            L"  <StackPanel VerticalAlignment='Center'>"
            L"    <Button x:Name='Button1' Height='200' Width='200' Content='Button1' />"
            L"    <TextBlock x:Name='TextBlock1' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='Hyperlink1' Foreground='Red'>hyperlink</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>"
            L"</Page>";

        auto page = safe_cast<Page^> (xaml_markup::XamlReader::Load(xamlString));

        panel = safe_cast<Panel^>(page->Content);
        textBlock = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(panel, L"TextBlock1"));
        button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(panel, L"Button1"));
        hyperlink = safe_cast<Hyperlink^>(textBlock->Inlines->GetAt(0));

        pointerEnteredTextBlockRegistration.Attach(textBlock, ref new PointerEventHandler(
            [pointerEnteredTextBlockEvent](Platform::Object^, PointerRoutedEventArgs^)
        {
            LOG_OUTPUT(L"Pointer entered TextBlock.");
            pointerEnteredTextBlockEvent->Set();
        }));

        pointerEnteredButtonRegistration.Attach(button, ref new PointerEventHandler(
            [pointerEnteredButtonEvent](Platform::Object^, PointerRoutedEventArgs^)
        {
            LOG_OUTPUT(L"Pointer entered Button.");
            pointerEnteredButtonEvent->Set();
        }));

        loadedRegistration.Attach(button, ref new RoutedEventHandler(
            [loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
        {
            LOG_OUTPUT(L"Button loaded");
            loadedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = page;
    });

    LOG_OUTPUT(L"Wait for Button loaded");
    loadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    TestServices::InputHelper->LeftMouseClick(textBlock);
    TestServices::WindowHelper->WaitForIdle();

    // Force PointerOver change on TextBlock and Hyperlink.
    // This causes a new lookup for the hyperlink's Foreground brush resource.
    LOG_OUTPUT(L"Move the mouse to the Button");
    TestServices::InputHelper->MoveMouse(button);
    pointerEnteredButtonEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Move the mouse to the TextBlock");
    pointerEnteredTextBlockEvent->Reset();
    TestServices::InputHelper->MoveMouse(textBlock);
    pointerEnteredTextBlockEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Verify hyperlink's Foreground is not the local value while
        // the pointer is over it.
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        VERIFY_ARE_NOT_EQUAL(brush->Color, Colors::Red);
    });

    // Force pointer away from TextBlock and Hyperlink.
    // This causes a new lookup for the hyperlink's Foreground brush resource.
    LOG_OUTPUT(L"Move the mouse to the Button");
    TestServices::InputHelper->MoveMouse(button);
    pointerEnteredButtonEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Verify hyperlink's Foreground is the local value while
        // the pointer is not over it.
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        VERIFY_ARE_EQUAL(brush->Color, Colors::Red);
    });
}

void HyperlinkInTextTests::ValidateHyperlinkEnterKeyInput()
{
    ValidateHyperlinkKeyInput(L"enter");
}

void HyperlinkInTextTests::ValidateHyperlinkSpaceKeyInput()
{
    ValidateHyperlinkKeyInput(L" ");
}

void HyperlinkInTextTests::ValidateHyperlinkGamePadAKeyInput()
{
    ValidateHyperlinkKeyInput(L"GamepadA");
}

void HyperlinkInTextTests::ValidateHyperlinkKeyInput(const Platform::String ^key)
{
    TestCleanupWrapper cleanup;
    Panel^ panel;
    TextBlock^ textBlock;
    Hyperlink^ hyperlink;
    Button^ button;
    Color colorNormal;
    Color colorPressed;
    Color colorReleased;

    auto buttonLostFocusEvent = std::make_shared<Event>();
    auto buttonLostFocusRegistration = CreateSafeEventRegistration(Button, LostFocus);

    auto hyperlinkClickEvent = std::make_shared<Event>();
    auto hyperlinkClickRegistration = CreateSafeEventRegistration(Hyperlink, Click);

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      Background='LightBlue'>"
            L"  <StackPanel VerticalAlignment='Center'>"
            L"    <Button x:Name='Button1' Height='200' Width='200' Content='Button1' />"
            L"    <TextBlock x:Name='TextBlock1' Height='200' Width='200' IsTextSelectionEnabled='True'>"
            L"      <Hyperlink x:Name='Hyperlink1'>hyperlink</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>"
            L"</Page>";

        auto page = safe_cast<Page^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = page;

        panel = safe_cast<Panel^>(page->Content);
        textBlock = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(panel, L"TextBlock1"));
        button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(panel, L"Button1"));
        hyperlink = safe_cast<Hyperlink^>(textBlock->Inlines->GetAt(0));

        buttonLostFocusRegistration.Attach(button, [buttonLostFocusEvent]
        {
            LOG_OUTPUT(L"Button lost focus.");
            buttonLostFocusEvent->Set();
        });

        hyperlinkClickRegistration.Attach(hyperlink, [hyperlinkClickEvent]
        {
            LOG_OUTPUT(L"Hyperlink clicked.");
            hyperlinkClickEvent->Set();
        });

    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Focus on button first.");
        button->Focus(FocusState::Keyboard);
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Tab to hyperlink.");
    TestServices::KeyboardHelper->Tab();
    TestServices::WindowHelper->WaitForIdle();
    buttonLostFocusEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        // Get the hyperlink's Foreground color
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        colorNormal = brush->Color;
    });

    LOG_OUTPUT(L"Press invocation key.");
    TestServices::KeyboardHelper->PressKeySequence(ref new Platform::String(L"$d$_") + key);
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Get the hyperlink's Foreground color
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        colorPressed = brush->Color;
    });

    LOG_OUTPUT(L"Release invocation key.");
    TestServices::KeyboardHelper->PressKeySequence(ref new Platform::String(L"$u$_") + key);
    hyperlinkClickEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        // Get the hyperlink's Foreground color
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        colorReleased = brush->Color;
    });

    TestServices::WindowHelper->WaitForIdle();
    VERIFY_ARE_EQUAL(colorNormal, colorReleased);
    VERIFY_ARE_NOT_EQUAL(colorPressed, colorReleased);
}

void HyperlinkInTextTests::FocusHyperLinkWithSIPShowing()
{
    TestCleanupWrapper cleanup;

    StackPanel^ rootPanel;
    TextBlock^ textBlock;
    TextBox^ textBox;
    InputPane^ inputPane;
    wf::EventRegistrationToken inputPaneShowToken;
    wf::EventRegistrationToken inputPaneHideToken;

    auto textboxGotFocusEvent = std::make_shared<Event>();
    auto textboxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
    auto SIPShowingEvent = std::make_shared<Event>();
    auto SIPHidingEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
            L"    <Button x:Name='Button1' Height='20' Width='200' Content='Button1' />"
            L"    <TextBox x:Name='TextBox1' Height='80' Width='200'/>"
            L"    <TextBlock x:Name='TextBlock1' Height='20' Width='80'>"
            L"      <Hyperlink x:Name='HyperlinkHyperlinkHyperlinkHyperlinkHyperlinkHyperlink' Foreground='Red'>hyperlink</Hyperlink>"
            L"    </TextBlock>"
            L"</StackPanel>"));

        TestServices::WindowHelper->WindowContent = rootPanel;

        textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"TextBox1"));
        textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"TextBlock1"));

        textboxGotFocusRegistration.Attach(
            textBox,
            ref new xaml::RoutedEventHandler(
                [textboxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
        {
            LOG_OUTPUT(L"Textbox GotFocus.");
            textboxGotFocusEvent->Set();
        }));
    });

    RunOnUIThread([&]()
    {
        inputPane = InputPane::GetForCurrentView();
        inputPaneShowToken = inputPane->Showing += ref new TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
        {
            LOG_OUTPUT(L"SIP Showing...");
            SIPShowingEvent->Set();
        });
        inputPaneHideToken = inputPane->Hiding += ref new TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
        {
            LOG_OUTPUT(L"SIP Hiding...");
            SIPHidingEvent->Set();
        });
    });

    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Focus TextBox to bringup SIP");
    RunOnUIThread([&]()
    {
        textBox->Focus(xaml::FocusState::Pointer);
    });

    textboxGotFocusEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    SIPShowingEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    BOOLEAN inputPaneOpen = FALSE;

    int maxAttempts = 1000; // Just to make sure SIP will be fully open before tapping on the Hyperlink
    int attempts = 0;
    while (!inputPaneOpen && attempts < maxAttempts)
    {
        LOG_OUTPUT(L"Wait for inputpane to be fully open...");
        attempts++;
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        RunOnUIThread([&]()
        {
            inputPaneOpen = TestServices::WindowHelper->IsInputPaneOpen;
        });
    }

    SIPHidingEvent->Reset();
    LOG_OUTPUT(L"Tap the textblock which contains the HyperLink.");
    TestServices::InputHelper->Tap(textBlock);
    TestServices::WindowHelper->WaitForIdle();
    SIPHidingEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Prevent the text control from getting focus again so input pane remains hidden.
        textBox->IsEnabled = false;
        inputPane->Showing -= inputPaneShowToken;
        inputPane->Hiding -= inputPaneHideToken;
    });
}

void HyperlinkInTextTests::HyperLinkBringIntoView()
{
    TestCleanupWrapper cleanup;

    xaml_controls::ScrollViewer^ rootScrollViewer = nullptr;
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        rootScrollViewer = safe_cast<xaml_controls::ScrollViewer^> (xaml_markup::XamlReader::Load(
            L"<ScrollViewer x:Name='scrollViewer' Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <StackPanel>"
            L"        <Button x:Name='button' Content='Button1'/>"
            L"        <TextBlock x:Name='TextBlock1' Margin='20, 600, 0, 0' FontSize='15'>"
            L"            line"
            L"            <LineBreak/>"
            L"            line"
            L"            <LineBreak/>"
            L"            line"
            L"            <LineBreak/>"
            L"            line"
            L"            <LineBreak/>"
            L"            line"
            L"            <LineBreak/>"
            L"            line"
            L"            <LineBreak/>"
            L"            line"
            L"            <LineBreak/>"
            L"            line"
            L"            <LineBreak/>"
            L"            line"
            L"            <LineBreak/>"
            L"            line"
            L"            <LineBreak/>"
            L"            <Hyperlink x:Name='HyperlinkHyperlinkHyperlinkHyperlinkHyperlinkHyperlink' Foreground='Red'>hyperlink</Hyperlink>"
            L"        </TextBlock>"
            L"    </StackPanel>"
            L" </ScrollViewer>"));

        TestServices::WindowHelper->WindowContent = rootScrollViewer;
        button = safe_cast<xaml_controls::Button^>(rootScrollViewer->FindName("button"));
        VERIFY_IS_NOT_NULL(button);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Focus on Button");
        button->Focus(FocusState::Pointer);
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Tab forward the focus to Hyperlink.");
    TestServices::KeyboardHelper->Tab();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Root SV's VerticalOffset = %f", rootScrollViewer->VerticalOffset);
        // make sure scroll into view happened for the HyperLink at bottom
        VERIFY_IS_TRUE(rootScrollViewer->VerticalOffset > 450.0f);
    });
    TestServices::WindowHelper->WaitForIdle();
}

void HyperlinkInTextTests::HyperLinkBringIntoViewWithGamepad()
{
    TestCleanupWrapper cleanup;

    xaml_controls::ScrollViewer^ rootScrollViewer;
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        rootScrollViewer = safe_cast<xaml_controls::ScrollViewer^> (xaml_markup::XamlReader::Load(
            L"<ScrollViewer x:Name='scrollViewer' Width='100' Height='100' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <StackPanel>"
            L"        <Button x:Name='button' Width='100' Height='100' Content='Button1'/>"
            L"        <TextBlock x:Name='TextBlock1'>"
            L"            <Hyperlink x:Name='HyperlinkHyperlinkHyperlinkHyperlinkHyperlinkHyperlink' Foreground='Red'>hyperlink</Hyperlink>"
            L"        </TextBlock>"
            L"    </StackPanel>"
            L" </ScrollViewer>"));

        TestServices::WindowHelper->WindowContent = rootScrollViewer;
        button = safe_cast<xaml_controls::Button^>(rootScrollViewer->FindName("button"));
        VERIFY_IS_NOT_NULL(button);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Focus on Button");
        button->Focus(FocusState::Keyboard);
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Navigate to the Hyperlink.");
    TestServices::KeyboardHelper->GamepadDpadDown();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Root SV's VerticalOffset = %f", rootScrollViewer->VerticalOffset);
        // make sure scroll into view happened for the HyperLink at bottom
        VERIFY_IS_TRUE(rootScrollViewer->VerticalOffset > 15.0f);
    });
    TestServices::WindowHelper->WaitForIdle();
}

void HyperlinkInTextTests::ValidateNavigationOnKeyUp()
{
    TestCleanupWrapper cleanup;
    KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

    Button^ btn = nullptr;
    TextBlock^ txbl = nullptr;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;

    auto hyperlinkClickRegistration = CreateSafeEventRegistration(Hyperlink, Click);
    auto hyperlinkClickEvent = std::make_shared<Event>();

    auto btnGotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
    auto btnGotFocusEvent = std::make_shared<Event>();

    auto btnLostFocusRegistration = CreateSafeEventRegistration(Button, LostFocus);
    auto btnLostFocusEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        btn = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootPanel, L"btn1"));
        txbl = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(rootPanel, L"txbl"));
        hyperlink = safe_cast<Hyperlink^>(txbl->Inlines->GetAt(0));

        hyperlinkClickRegistration.Attach(hyperlink, [hyperlinkClickEvent]
        {
            LOG_OUTPUT(L"Hyperlink clicked");
            hyperlinkClickEvent->Set();
        });

        btnGotFocusRegistration.Attach(btn, [btnGotFocusEvent]
        {
            LOG_OUTPUT(L"Button Got Focus");
            btnGotFocusEvent->Set();
        });

        btnLostFocusRegistration.Attach(btn, [btnLostFocusEvent]
        {
            LOG_OUTPUT(L"Button Lost Focus");
            btnLostFocusEvent->Set();
        });

        btn->Focus(xaml::FocusState::Keyboard);
    });
    btnGotFocusEvent->WaitForDefault();
    TestServices::KeyboardHelper->Tab(); //No direct way to set programmatic focus on the Hyperlink
    btnLostFocusEvent->WaitForDefault();

    //Only Inject Key up for Enter, make sure that Click event doesn't fire as a result!
    TestServices::KeyboardHelper->PressKeySequence(L"$u$_enter");
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_FALSE(hyperlinkClickEvent->HasFired());

    //Inject a non navigation key followed by a Navigation Up, make sure click doesn't fire as a result!
    TestServices::KeyboardHelper->PressKeySequence("A");
    TestServices::WindowHelper->WaitForIdle();
    TestServices::KeyboardHelper->PressKeySequence(L"$u$_enter");
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_FALSE(hyperlinkClickEvent->HasFired());

    //Now try Down/Up to make sure that Click does fire with Down followed by up
    TestServices::KeyboardHelper->Enter();
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(hyperlinkClickEvent->HasFired());

    hyperlinkClickEvent->Reset();

    //Any navigation Up key will do, followed by a navigation down key
    TestServices::KeyboardHelper->PressKeySequence(L"$d$_enter");
    TestServices::WindowHelper->WaitForIdle();
    TestServices::KeyboardHelper->PressKeySequence(L" ");
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(hyperlinkClickEvent->HasFired());
    TestServices::KeyboardHelper->PressKeySequence(L"$u$_enter");
    TestServices::WindowHelper->WaitForIdle();
}

void HyperlinkInTextTests::PointerOverHighContrast()
{
    TestCleanupWrapper cleanup;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    StackPanel^ rootPanel = nullptr;
    Hyperlink^ hyperlink = nullptr;
    TextBlock^ textBlock = nullptr;
    const Color expectedColor = Colors::Yellow;
    auto pointerEnteredTextBlockEvent = std::make_shared<Event>();
    auto pointerEnteredTextBlockRegistration = CreateSafeEventRegistration(TextBlock, PointerEntered);

    TestServices::InputHelper->MoveMouse(wf::Point(0, 0));

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"    VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <TextBlock x:Name='textBlock'>"
            L"        <Hyperlink x:Name='hyperlink'>This is a Hyperlink!</Hyperlink>"
            L"    </TextBlock>"
            L"</StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        VERIFY_IS_NOT_NULL(rootPanel);
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));
        VERIFY_IS_NOT_NULL(hyperlink);
        textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"textBlock"));
        VERIFY_IS_NOT_NULL(textBlock);
        TestServices::WindowHelper->WindowContent = rootPanel;

        pointerEnteredTextBlockRegistration.Attach(textBlock, ref new PointerEventHandler(
             [pointerEnteredTextBlockEvent](Platform::Object^, PointerRoutedEventArgs^)
             {
                 LOG_OUTPUT(L"Pointer entered TextBlock.");
                 pointerEnteredTextBlockEvent->Set();
             }));
    });
    TestServices::WindowHelper->WaitForIdle();

    // Switch to high contrast theme.
    RunOnUIThread([&]()
    {
        TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Default;
        rootPanel->Background = ref new xaml_media::SolidColorBrush(Colors::Black);
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Move the mouse to the TextBlock");
    TestServices::InputHelper->MoveMouse(textBlock);
    pointerEnteredTextBlockEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

     RunOnUIThread([&]()
    {
        // Get the hyperlink's Foreground color
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        VERIFY_IS_TRUE(IsSameColor(expectedColor, brush->Color));
    });
}

// Validates Hyperlink's usage of default SystemControlHyperlinkTextBrush, SystemControlHyperlinkBaseMediumBrush & SystemControlHighlightBaseMediumLowBrush fallback theme resources.
void HyperlinkInTextTests::ValidateDefaultSystemControlForegroundBrushes()
{
    ValidateForegroundBrushes(true /*withSystemControlColors*/);
}

// Validates Hyperlink's usage of custom SystemControlHyperlinkTextBrush, SystemControlHyperlinkBaseMediumBrush & SystemControlHighlightBaseMediumLowBrush fallback theme resources.
void HyperlinkInTextTests::ValidateCustomSystemControlForegroundBrushes()
{
    ValidateForegroundBrushes(true /*withSystemControlColors*/, true /*withCustomSystemControlColors*/);
}

// Validates Hyperlink's usage of custom HyperlinkForeground, HyperlinkForegroundPointerOver & HyperlinkForegroundPressed theme resources.
void HyperlinkInTextTests::ValidateHyperlinkForegroundBrushes()
{
    ValidateForegroundBrushes(false /*withSystemControlColors*/);
}

void HyperlinkInTextTests::ValidateForegroundBrushes(bool withSystemControlColors, bool withCustomSystemControlColors)
{
    TestCleanupWrapper cleanup;

    Hyperlink^ hyperlink = nullptr;
    TextBlock^ textBlock = nullptr;
    PointerEventHandler^ pointerPressedHandler = nullptr;

    auto pointerEnteredTextBlockEvent = std::make_shared<Event>();
    auto pointerPressedTextBlockEvent = std::make_shared<Event>();
    auto pointerReleasedTextBlockEvent = std::make_shared<Event>();
    auto pointerEnteredTextBlockRegistration = CreateSafeEventRegistration(TextBlock, PointerEntered);
    auto pointerReleasedTextBlockRegistration = CreateSafeEventRegistration(TextBlock, PointerReleased);

    TestServices::InputHelper->MoveMouse(wf::Point(0, 0));

    RunOnUIThread([&]()
    {
        if (!withSystemControlColors)
        {
            auto dictionary = safe_cast<xaml::ResourceDictionary^>(xaml_markup::XamlReader::Load(
                LR"(
                <ResourceDictionary
                    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <SolidColorBrush x:Key='HyperlinkForeground' Color='Blue'/>
                    <SolidColorBrush x:Key='HyperlinkForegroundPointerOver' Color='White'/>
                    <SolidColorBrush x:Key='HyperlinkForegroundPressed' Color='Red'/>
                </ResourceDictionary>)"));

            Application::Current->Resources->MergedDictionaries->Append(dictionary);
        }
        else if (withCustomSystemControlColors)
        {
            auto dictionary = safe_cast<xaml::ResourceDictionary^>(xaml_markup::XamlReader::Load(
                LR"(
                <ResourceDictionary
                    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                    <SolidColorBrush x:Key='SystemControlHyperlinkTextBrush' Color='Green'/>
                    <SolidColorBrush x:Key='SystemControlHyperlinkBaseMediumBrush' Color='Yellow'/>
                    <SolidColorBrush x:Key='SystemControlHighlightBaseMediumLowBrush' Color='Orange'/>
                </ResourceDictionary>)"));

            Application::Current->Resources->MergedDictionaries->Append(dictionary);
        }

        auto rootPanel = safe_cast<StackPanel^>(xaml_markup::XamlReader::Load(
            LR"(
            <StackPanel
                xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                VerticalAlignment='Center' HorizontalAlignment='Center'>
                <TextBlock x:Name='textBlock' FontSize='50'>
                    <Hyperlink x:Name='hyperlink'>This is a Hyperlink!</Hyperlink>
                </TextBlock>
            </StackPanel>)"));
        VERIFY_IS_NOT_NULL(rootPanel);
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));
        VERIFY_IS_NOT_NULL(hyperlink);
        textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"textBlock"));
        VERIFY_IS_NOT_NULL(textBlock);
        TestServices::WindowHelper->WindowContent = rootPanel;

        pointerEnteredTextBlockRegistration.Attach(textBlock, ref new PointerEventHandler(
             [pointerEnteredTextBlockEvent](Platform::Object^, PointerRoutedEventArgs^)
             {
                 LOG_OUTPUT(L"TextBlock.PointerEntered raised.");
                 pointerEnteredTextBlockEvent->Set();
             }));

        pointerReleasedTextBlockRegistration.Attach(textBlock, ref new PointerEventHandler(
             [pointerReleasedTextBlockEvent](Platform::Object^, PointerRoutedEventArgs^)
             {
                 LOG_OUTPUT(L"TextBlock.PointerReleased raised.");
                 pointerReleasedTextBlockEvent->Set();
             }));

        pointerPressedHandler = ref new PointerEventHandler(
            [pointerPressedTextBlockEvent](Platform::Object^, PointerRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"TextBlock.PointerPressed raised.");
                pointerPressedTextBlockEvent->Set();
            });

        textBlock->AddHandler(xaml::UIElement::PointerPressedEvent, pointerPressedHandler, true);
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        LOG_OUTPUT(L"Default Hyperlink.Foreground Color A=%d, R=%d, G=%d, B=%d.",
            brush->Color.A, brush->Color.R, brush->Color.G, brush->Color.B);
        if (withSystemControlColors)
        {
            if (withCustomSystemControlColors)
            {
                // Expected Color: Green
                VERIFY_IS_TRUE(IsSameColor(Colors::Green, brush->Color));
            }
            else
            {
                // Expected Color A=255, R=91, G=46, B=197 == #FF5B2EC5
                VERIFY_ARE_EQUAL(255, brush->Color.A);
                VERIFY_ARE_EQUAL(91, brush->Color.R);
                VERIFY_ARE_EQUAL(46, brush->Color.G);
                VERIFY_ARE_EQUAL(197, brush->Color.B);
            }
        }
        else
        {
            // Expected Color: Blue
            VERIFY_IS_TRUE(IsSameColor(Colors::Blue, brush->Color));
        }
    });

    LOG_OUTPUT(L"Move the mouse over the TextBlock");
    TestServices::InputHelper->MoveMouse(textBlock);
    pointerEnteredTextBlockEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        LOG_OUTPUT(L"Pointer-over Hyperlink.Foreground Color A=%d, R=%d, G=%d, B=%d.",
            brush->Color.A, brush->Color.R, brush->Color.G, brush->Color.B);
        if (withSystemControlColors)
        {
            if (withCustomSystemControlColors)
            {
                // Expected Color: Yellow
                VERIFY_IS_TRUE(IsSameColor(Colors::Yellow, brush->Color));
            }
            else
            {
                // Expected Color A=153, R=255, G=255, B=255 == #99FFFFFF
                VERIFY_ARE_EQUAL(153, brush->Color.A);
                VERIFY_ARE_EQUAL(255, brush->Color.R);
                VERIFY_ARE_EQUAL(255, brush->Color.G);
                VERIFY_ARE_EQUAL(255, brush->Color.B);
            }
        }
        else
        {
            // Expected Color: White
            VERIFY_IS_TRUE(IsSameColor(Colors::White, brush->Color));
        }
    });

    LOG_OUTPUT(L"Press the mouse button over the TextBlock");
    TestServices::InputHelper->MouseButtonDown(textBlock, 0, 0, MouseButton::Left);
    pointerPressedTextBlockEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        LOG_OUTPUT(L"Pointer-pressed Hyperlink.Foreground Color A=%d, R=%d, G=%d, B=%d.",
            brush->Color.A, brush->Color.R, brush->Color.G, brush->Color.B);
        if (withSystemControlColors)
        {
            if (withCustomSystemControlColors)
            {
                // Expected Color: Orange
                VERIFY_IS_TRUE(IsSameColor(Colors::Orange, brush->Color));
            }
            else
            {
                // Expected Color Color A=102, R=255, G=255, B=255 == #66FFFFFF
                VERIFY_ARE_EQUAL(102, brush->Color.A);
                VERIFY_ARE_EQUAL(255, brush->Color.R);
                VERIFY_ARE_EQUAL(255, brush->Color.G);
                VERIFY_ARE_EQUAL(255, brush->Color.B);
            }
        }
        else
        {
            // Expected Color: Red
            VERIFY_IS_TRUE(IsSameColor(Colors::Red, brush->Color));
        }
    });

    TestServices::InputHelper->MouseButtonUp(textBlock, 0, 0, MouseButton::Left);
    pointerReleasedTextBlockEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto brush = safe_cast<SolidColorBrush^>(hyperlink->Foreground);
        VERIFY_IS_NOT_NULL(brush);
        LOG_OUTPUT(L"Pointer-over Hyperlink.Foreground Color A=%d, R=%d, G=%d, B=%d.",
            brush->Color.A, brush->Color.R, brush->Color.G, brush->Color.B);
        if (withSystemControlColors)
        {
            if (withCustomSystemControlColors)
            {
                // Expected Color: Yellow
                VERIFY_IS_TRUE(IsSameColor(Colors::Yellow, brush->Color));
            }
            else
            {
                // Expected Color A=153, R=255, G=255, B=255 == #99FFFFFF
                VERIFY_ARE_EQUAL(153, brush->Color.A);
                VERIFY_ARE_EQUAL(255, brush->Color.R);
                VERIFY_ARE_EQUAL(255, brush->Color.G);
                VERIFY_ARE_EQUAL(255, brush->Color.B);
            }
        }
        else
        {
            // Expected Color: White
            VERIFY_IS_TRUE(IsSameColor(Colors::White, brush->Color));
        }

        LOG_OUTPUT(L"Unhooking TextBlock.PointerPressed handler.");
        textBlock->RemoveHandler(xaml::UIElement::PointerPressedEvent, pointerPressedHandler);
        pointerPressedHandler = nullptr;
    });
}

void HyperlinkInTextTests::ValidateKeyDownBubbling()
{
    TestCleanupWrapper cleanup;
    KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

    Button^ btn = nullptr;
    TextBlock^ txbl = nullptr;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;

    auto txtBlockKeyDownEventRegistration = CreateSafeEventRegistration(TextBlock, KeyDown);
    auto txtBlockKeyDownEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        btn = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootPanel, L"btn1"));
        txbl = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(rootPanel, L"txbl"));
        hyperlink = safe_cast<Hyperlink^>(txbl->Inlines->GetAt(0));

        txtBlockKeyDownEventRegistration.Attach(txbl, [txtBlockKeyDownEvent]
        {
            LOG_OUTPUT(L"TextBlock received KeyDown");
            txtBlockKeyDownEvent->Set();
        });
    });

    FocusTestHelper::EnsureFocus(btn, FocusState::Keyboard);
    TestServices::KeyboardHelper->Tab(); //No direct way to set programmatic focus on the Hyperlink

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(FocusManager::GetFocusedElement(hyperlink->XamlRoot)->Equals(hyperlink));
        LOG_OUTPUT(L"Hyperlink has focus");
    });

    TestServices::KeyboardHelper->Right();

    txtBlockKeyDownEvent->WaitForDefault();
    VERIFY_IS_TRUE(txtBlockKeyDownEvent->HasFired());

}

void HyperlinkInTextTests::ValidateProgrammaticHyperlinkFocus()
{
    TestCleanupWrapper cleanup;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;

    auto hyperlinkGotFocusEvent = std::make_shared<Event>();
    auto hyperlinkGotFocusRegistration = CreateSafeEventRegistration(xaml::Documents::Hyperlink, GotFocus);

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));

        hyperlinkGotFocusRegistration.Attach(hyperlink, [hyperlinkGotFocusEvent]
        {
            LOG_OUTPUT(L"Hyperlink Got Focus Event Fired");
            hyperlinkGotFocusEvent->Set();
        });

        LOG_OUTPUT(L"Setting Focus on hyperlink");
        hyperlink->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
    });


    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(FocusManager::GetFocusedElement(hyperlink->XamlRoot)->Equals(hyperlink));
        LOG_OUTPUT(L"Hyperlink has focus");
    });

    hyperlinkGotFocusEvent->WaitForDefault();
    VERIFY_IS_TRUE(hyperlinkGotFocusEvent->HasFired());

}

void HyperlinkInTextTests::ValidateHyperlinkLostFocus()
{
    TestCleanupWrapper cleanup;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;
    Button^ btn = nullptr;

    auto hyperlinkLostFocusEvent = std::make_shared<Event>();
    auto hyperlinkLostFocusRegistration = CreateSafeEventRegistration(xaml::Documents::Hyperlink, LostFocus);
    auto hyperlinkGotFocusEvent = std::make_shared<Event>();
    auto hyperlinkGotFocusRegistration = CreateSafeEventRegistration(xaml::Documents::Hyperlink, GotFocus);

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        btn = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootPanel, L"btn1"));
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));
        hyperlinkLostFocusRegistration.Attach(
            hyperlink,
            [hyperlinkLostFocusEvent]()
        {
            LOG_OUTPUT(L"hyperlink LostFocus.");
            hyperlinkLostFocusEvent->Set();
        });

        hyperlinkGotFocusRegistration.Attach(
            hyperlink,
            [hyperlinkGotFocusEvent]()
        {
            LOG_OUTPUT(L"Hyperlink got focus");
            hyperlinkGotFocusEvent->Set();
        });
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Setting Focus on btn");
    FocusTestHelper::EnsureFocus(btn, FocusState::Keyboard);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting Focus on hyperlink");
        hyperlink->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
    });

    hyperlinkGotFocusEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing hyperlink Focus. Focus now on Btn.");
        btn->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(hyperlinkLostFocusEvent->HasFired());
    });
}

void HyperlinkInTextTests::ValidateGotFocusBubbling()
{
    TestCleanupWrapper cleanup;
    TextBlock^ txbl = nullptr;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;
    Button^ btn = nullptr;

    auto txtBlockFocusEventRegistration = CreateSafeEventRegistration(TextBlock, GotFocus);
    auto txtBlockFocusEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        btn = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootPanel, L"btn1"));
        txbl = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(rootPanel, L"txbl"));
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));

        txtBlockFocusEventRegistration.Attach(txbl, [txtBlockFocusEvent]
        {
            LOG_OUTPUT(L"Hyperlink Got Focus");
            txtBlockFocusEvent->Set();
        });
    });

    TestServices::WindowHelper->WaitForIdle();

    FocusTestHelper::EnsureFocus(btn, FocusState::Keyboard);
    TestServices::KeyboardHelper->Tab(); //Set focus on the Hyperlink

    txtBlockFocusEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(FocusManager::GetFocusedElement(hyperlink->XamlRoot)->Equals(hyperlink));
        LOG_OUTPUT(L"GotFocus has bubbled to TextBlock");

    });
}

void HyperlinkInTextTests::ValidateProgrammaticHyperlinkGotFocusBubbling()
{
    TestCleanupWrapper cleanup;
    TextBlock^ txbl = nullptr;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;
    Button^ btn = nullptr;

    auto txtBlockFocusEventRegistration = CreateSafeEventRegistration(TextBlock, GotFocus);
    auto txtBlockFocusEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        btn = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootPanel, L"btn1"));
        txbl = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(rootPanel, L"txbl"));
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));

        txtBlockFocusEventRegistration.Attach(txbl, [txtBlockFocusEvent]
        {
            LOG_OUTPUT(L"Hyperlink Got Focus");
            txtBlockFocusEvent->Set();
        });

        LOG_OUTPUT(L"Setting Focus on hyperlink");
        hyperlink->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
    });

    txtBlockFocusEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink));
        VERIFY_IS_TRUE(txtBlockFocusEvent->HasFired());
        LOG_OUTPUT(L"GotFocus has bubbled to TextBlock");
    });
}

void HyperlinkInTextTests::ValidateFocusRectOnFocusKeyboard()
{
    TestCleanupWrapper cleanup;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;
    Button^ btn = nullptr;

    // MockDComp should be injected and detached per test, since it keeps information like the surfaces
    // that get created.
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

    auto hyperlinkGotFocusEvent = std::make_shared<Event>();
    auto hyperlinkGotFocusRegistration = CreateSafeEventRegistration(Hyperlink, GotFocus);

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Loading window");
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));
        btn = safe_cast<Button^>(rootPanel->FindName("btn1"));

        hyperlinkGotFocusRegistration.Attach(hyperlink, [hyperlinkGotFocusEvent]
        {
            LOG_OUTPUT(L"Hyperlink got focus.");
            hyperlinkGotFocusEvent->Set();
        });
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting Keyboard Focus on hyperlink");
        hyperlink->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
    });

    hyperlinkGotFocusEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(FocusManager::GetFocusedElement(hyperlink->XamlRoot)->Equals(hyperlink));
    });

    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    TestServices::WindowHelper->WaitForIdle();

    //Using keyboard injection to re-place focus on Hyperlink
    FocusTestHelper::EnsureFocus(btn, FocusState::Keyboard);
    TestServices::KeyboardHelper->PressKeySequence("a");

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting Programmatic Focus on hyperlink");
        hyperlink->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(FocusManager::GetFocusedElement(hyperlink->XamlRoot)->Equals(hyperlink));
    });

    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Verifying that Programmatic Focus renders focus rect if previous input device was keyboard");
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void HyperlinkInTextTests::ValidateFocusRectOnFocusPointer()
{
    TestCleanupWrapper cleanup;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;
    Button^ btn = nullptr;
    // MockDComp should be injected and detached per test, since it keeps information like the surfaces
    // that get created.
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Loading window");
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));
        LOG_OUTPUT(L"Setting Pointer Focus on hyperlink");
        hyperlink->Focus(Microsoft::UI::Xaml::FocusState::Pointer);
    });


    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(FocusManager::GetFocusedElement(hyperlink->XamlRoot)->Equals(hyperlink));
    });

    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void HyperlinkInTextTests::ValidateProgrammaticFocusRectOnPreviousInputPointer()
{
    TestCleanupWrapper cleanup;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;

    auto hyperlinkGotFocusEvent = std::make_shared<Event>();
    auto hyperlinkGotFocusRegistration = CreateSafeEventRegistration(Hyperlink, GotFocus);

    // MockDComp should be injected and detached per test, since it keeps information like the surfaces
    // that get created.
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    XamlRoot^ xamlRoot = nullptr;
    RunOnUIThread([&]()
    {
        xamlRoot = rootPanel->XamlRoot;
    });

    TestServices::WindowHelper->SetLastInputMethod(test_infra::LastInputDeviceType::Mouse, xamlRoot);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Loading window");
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));

        hyperlinkGotFocusRegistration.Attach(hyperlink, [hyperlinkGotFocusEvent]
        {
            LOG_OUTPUT(L"Hyperlink got focus.");
            hyperlinkGotFocusEvent->Set();
        });
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting Programmatic Focus on hyperlink");
        hyperlink->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
    });

    hyperlinkGotFocusEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(FocusManager::GetFocusedElement(hyperlink->XamlRoot)->Equals(hyperlink));
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Verifying that Programmatic Focus does not render focus rect when previous input device was pointer.");
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void HyperlinkInTextTests::ValidateHyperlinkGotFocusOnLoad()
{
    TestCleanupWrapper cleanup;
    KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;
    Hyperlink^ hyperlink1 = nullptr;
    StackPanel^ root = nullptr;
    XamlRoot^ xamlRoot = nullptr;

    bool isFocused = false;

    auto lostFocusEvent = std::make_shared<Event>();
    auto lostFocusRegistration = CreateSafeEventRegistration(xaml_docs::Hyperlink, LostFocus);

    auto gotFocusEvent = std::make_shared<Event>();
    auto gotFocusRegistration = CreateSafeEventRegistration(xaml_docs::Hyperlink, GotFocus);

    auto windowActivatedEvent = std::make_shared<Event>();
    auto windowActivatedRegistration = CreateSafeEventRegistration(Window, Activated);

    RunOnUIThread([&]()
    {
        root = safe_cast<StackPanel^>(xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <TextBlock x:Name='txBlk' Margin='20,20,0,0' Width='350' HorizontalAlignment='Left' FontSize='20'>"
            L"        <Hyperlink x:Name='Hyperlink1' NavigateUri = 'http://www.bing.com' >Bing</Hyperlink>"
            L"    </TextBlock>"
            L"</StackPanel>"));

        TestServices::WindowHelper->WindowContent = root;
        hyperlink1 = safe_cast<Hyperlink^>(root->FindName(L"Hyperlink1"));

        windowActivatedRegistration.Attach(Window::Current, [&]()
        {
            LOG_OUTPUT(L"Window activated");
            windowActivatedEvent->Set();
        });

        lostFocusRegistration.Attach(
            hyperlink1,
            [lostFocusEvent]()
        {
            LOG_OUTPUT(L"hyperlink1 LostFocus.");
            lostFocusEvent->Set();
        });

        gotFocusRegistration.Attach(
            hyperlink1,
            [gotFocusEvent]()
        {
            LOG_OUTPUT(L"hyperlink1 GotFocus.");
            gotFocusEvent->Set();
        });
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        xamlRoot = hyperlink1->XamlRoot;
    });

    //Puts Keyboard focus onto the hyperlink.
    TestServices::KeyboardHelper->Tab();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->IsWindowFocused(&isFocused, xamlRoot);

    //Checks that the hyperlink has received focus.
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(xamlRoot)->Equals(hyperlink1));
        VERIFY_IS_TRUE(isFocused);
    });

    //Sends an window key input, causing the window to lose focus.
    TestServices::WindowHelper->WaitForIdle();
    TestServices::KeyboardHelper->PressKeySequence("$d$_lwin#$u$_lwin");
    TestServices::WindowHelper->WaitForIdle();

    windowActivatedEvent->WaitForDefault();
    windowActivatedEvent->Reset();
    TestServices::Utilities->IsWindowFocused(&isFocused, xamlRoot);

    //Checks that the hyperlink has lost focus.
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(xamlRoot)->Equals(hyperlink1));
        VERIFY_IS_FALSE(isFocused);
        VERIFY_IS_TRUE(lostFocusEvent->HasFired());
    });

    //Refocuses primary window
    TestServices::WindowHelper->WaitForIdle();

    //When the window is about to come back, we need to wait for it to send activtions/etc. There is currently no
    //good way to do this, so we've defaulted to just sleeping for a second.
    Sleep(1000);
    //There is a difference between how the XBox and Desktop shells bring back the background app back into foreground from Home
    //On Desktop, we follow the sequence: (FGApp) LWin -> Start -> (Start) Escape -> FGApp
    //On XBox, we follow the sequence: (FGApp) LWin -> Home -> (Home) Enter -> FGApp
    if (TestServices::Utilities->IsXBox)
    {
        TestServices::KeyboardHelper->Enter();
    }
    else
    {
        TestServices::KeyboardHelper->Escape();
    }
    TestServices::WindowHelper->WaitForIdle();
    windowActivatedEvent->WaitForDefault();
    TestServices::Utilities->IsWindowFocused(&isFocused, xamlRoot);

    //Checks that the hyperlink has received focus once more.
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(xamlRoot)->Equals(hyperlink1));
        VERIFY_IS_TRUE(isFocused);
        VERIFY_IS_TRUE(gotFocusEvent->HasFired());
    });

    TestServices::WindowHelper->WaitForIdle();
}

void HyperlinkInTextTests::ValidateCollapsedHyperlinkFocus()
{
    TestCleanupWrapper cleanup;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;
    TextBlock^ txb1 = nullptr;

    auto hyperlinkGotFocusEvent = std::make_shared<Event>();
    auto hyperlinkGotFocusRegistration = CreateSafeEventRegistration(xaml::Documents::Hyperlink, GotFocus);

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        txb1 = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(rootPanel, L"txbl"));
        txb1->Visibility = Microsoft::UI::Xaml::Visibility::Collapsed;
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));

        hyperlinkGotFocusRegistration.Attach(hyperlink, [hyperlinkGotFocusEvent]
        {
            LOG_OUTPUT(L"Hyperlink Got Focus Event Fired");
            hyperlinkGotFocusEvent->Set();
        });

        LOG_OUTPUT(L"Setting Focus on hyperlink");
        VERIFY_IS_FALSE(hyperlink->Focus(Microsoft::UI::Xaml::FocusState::Programmatic));
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_FALSE(hyperlinkGotFocusEvent->HasFired());
    });
}

void HyperlinkInTextTests::ValidateHyperlinkIsTabStop()
{
    TestCleanupWrapper cleanup;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;
    TextBlock^ txb1 = nullptr;
    Button^ btn = nullptr;
    auto hyperlinkGotFocusEvent = std::make_shared<Event>();
    auto hyperlinkGotFocusRegistration = CreateSafeEventRegistration(xaml::Documents::Hyperlink, GotFocus);

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
            L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue' IsTabStop='false'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));

        txb1 = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(rootPanel, L"txbl"));
        btn = safe_cast<Button^>(rootPanel->FindName(L"btn1"));
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));

        hyperlinkGotFocusRegistration.Attach(hyperlink, [hyperlinkGotFocusEvent]
        {
            LOG_OUTPUT(L"Hyperlink Got Focus Event Fired");
            hyperlinkGotFocusEvent->Set();
        });

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    FocusTestHelper::EnsureFocus(btn, FocusState::Keyboard);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Trying to set Focus on hyperlink");
        VERIFY_IS_FALSE(hyperlink->Focus(Microsoft::UI::Xaml::FocusState::Keyboard));
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_FALSE(hyperlinkGotFocusEvent->HasFired());
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(btn->XamlRoot)->Equals(btn));
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Trying to tab to hyperlink");
    TestServices::KeyboardHelper->Tab();

    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_FALSE(hyperlinkGotFocusEvent->HasFired());
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(btn->XamlRoot)->Equals(btn));
    });

    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        hyperlink->IsTabStop = true;
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Trying to tab to hyperlink");
    TestServices::KeyboardHelper->Tab();

    hyperlinkGotFocusEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(hyperlinkGotFocusEvent->HasFired());
        hyperlinkGotFocusEvent->Reset();
    });

    TestServices::WindowHelper->WaitForIdle();

    FocusTestHelper::EnsureFocus(btn, FocusState::Keyboard);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Trying to set Focus on hyperlink");
        VERIFY_IS_TRUE(hyperlink->Focus(Microsoft::UI::Xaml::FocusState::Keyboard));
    });

    hyperlinkGotFocusEvent->WaitForDefault();
}

void HyperlinkInTextTests::ValidateHyperlinkIsTabStopGamepadBehavior()
{
    TestCleanupWrapper cleanup;
    Hyperlink^ hyperlink = nullptr;
    StackPanel^ rootPanel = nullptr;
    RichTextBlock^ rtxb1 = nullptr;
    Button^ btn = nullptr;
    auto hyperlinkGotFocusEvent = std::make_shared<Event>();
    auto hyperlinkGotFocusRegistration = CreateSafeEventRegistration(xaml::Documents::Hyperlink, GotFocus);


    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
            L"    <RichTextBlock x:Name='rtxbl' Height='200' Width='200'>"
            L"      <Paragraph> <Hyperlink x:Name='hyperlink' Foreground='LightBlue' IsTabStop='false'>Navigate!</Hyperlink></Paragraph>"
            L"    </RichTextBlock>"
            L"  </StackPanel>";

        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(xamlString));

        rtxb1 = safe_cast<RichTextBlock^>(TreeHelper::GetVisualChildByName(rootPanel, L"rtxbl"));
        btn = safe_cast<Button^>(rootPanel->FindName(L"btn1"));
        hyperlink = safe_cast<Hyperlink^>(rootPanel->FindName(L"hyperlink"));

        hyperlinkGotFocusRegistration.Attach(hyperlink, [hyperlinkGotFocusEvent]
        {
            LOG_OUTPUT(L"Hyperlink Got Focus Event Fired");
            hyperlinkGotFocusEvent->Set();
        });

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    FocusTestHelper::EnsureFocus(btn, FocusState::Keyboard);

    LOG_OUTPUT(L"Trying to navigate using gamepad to the hyperlink.");
    TestServices::KeyboardHelper->GamepadDpadDown();

    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_FALSE(hyperlinkGotFocusEvent->HasFired());
    });

    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        hyperlink->IsTabStop = true;
    });

    TestServices::WindowHelper->WaitForIdle();


    LOG_OUTPUT(L"Trying to navigate using gamepad to the hyperlink.");
    TestServices::KeyboardHelper->GamepadDpadDown();
    hyperlinkGotFocusEvent->WaitForDefault();
}

void HyperlinkInTextTests::UIAPeerLifetime()
{
    LOG_OUTPUT(L">>> Make sure we don't crash when the Hyperlink's UIA peer outlives the Hyperlink itself.");

    TestCleanupWrapper cleanup;

    const auto& wh = TestServices::WindowHelper;

    Hyperlink^ hyperlink;
    TextBlock^ textBlock;
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Building tree.");

        Run^ run = ref new Run();
        run->Text = L"asdf";

        hyperlink = ref new Hyperlink();
        hyperlink->Inlines->Append(run);
        xaml_automation::AutomationProperties::SetName(hyperlink, ref new Platform::String(L"HyperlinkPeer"));

        textBlock = ref new TextBlock();
        textBlock->Inlines->Append(hyperlink);

        root = ref new Canvas();
        root->Children->Append(textBlock);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto gotFocusEvent = std::make_shared<Event>();
    auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Focusing TextBlock.");

        gotFocusRegistration.Attach(textBlock, ref new RoutedEventHandler([gotFocusEvent](Platform::Object^ sender, RoutedEventArgs^ e)
        {
           LOG_OUTPUT(L"  > TextBlock received focus.");
           gotFocusEvent->Set();
        }));
        hyperlink->Focus(FocusState::Keyboard);
    });
    gotFocusEvent->WaitForDefault();

    wrl::ComPtr<IUIAutomationElement> hyperlinkUIAPeer;
    wrl::ComPtr<IUIAutomationInvokePattern> hyperlinkInvokePattern;
    RunOnUIThread([&]()
    {
        wrl::ComPtr<IUIAutomation> uiAutomation;
        Common::AutoVariant autoVar;

        auto automationClientManager = std::make_shared<AutomationClient::AutomationClientManager>();
        automationClientManager->GetAutomation(&uiAutomation);

        LOG_OUTPUT(L"> Get focused hyperlink using UIA.");
        uiAutomation->GetFocusedElement(&hyperlinkUIAPeer);
        WEX::Common::Throw::IfNull(hyperlinkUIAPeer.Get());
        LogThrow_IfFailed(hyperlinkUIAPeer->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.Storage()));
        VERIFY_ARE_EQUAL(autoVar.Storage()->vt, VT_BSTR);
        VERIFY_IS_TRUE(!wcscmp(L"HyperlinkPeer", (autoVar.Storage())->bstrVal));

        // This takes a reference on the Hyperlink automation peer, keeping it alive.
        VERIFY_SUCCEEDED(hyperlinkUIAPeer->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern), &hyperlinkInvokePattern));
        VERIFY_IS_NOT_NULL(hyperlinkInvokePattern);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Resetting tree.");

        hyperlink = nullptr;

        textBlock->Inlines->Clear();
        textBlock = nullptr;

        Button^ newChild = ref new Button();
        newChild->Width = 50;
        newChild->Height = 50;

        root->Children->Clear();
        root->Children->Append(newChild);

        LOG_OUTPUT(L"> Resetting focus.");
        newChild->Focus(FocusState::Keyboard);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        Common::AutoVariant autoVar;

        LOG_OUTPUT(L"> Check hyperlink UIA properties again. Don't crash.");

        // 30104 corresponds to AutomationComponentProperties_ControlledPeers, which causes us to call into the underlying
        // Hyperlink from the UIA peer.
        LogThrow_IfFailed(hyperlinkUIAPeer->GetCurrentPropertyValue(30104, autoVar.Storage()));

        HRESULT hr = hyperlinkUIAPeer->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.Storage());
        // Returning UIA_E_ELEMENTNOTAVAILABLE would also be a reasonable result, but
        // CUIAWrapper::GetPropertyValueImpl does this:
        //     if (!m_pAP)
        //     {
        //         return E_FAIL;
        //     }
        // This E_FAIL is converted by UIA's ProviderCallouts::GetPropertyValue() into an S_OK
        // with an empty return value.  We could potentially change the above code to return
        // UIA_E_ELEMENTNOTAVAILABLE, but that would be a bigger risk. For now, do as the LOG_OUTPUT
        // says, and just confirm we don't crash and get the expected result for no m_pAP.
        VERIFY_ARE_EQUAL(hr, static_cast<HRESULT>(S_OK));
    });
    wh->WaitForIdle();
}

struct HyperlinkTapAndClickHelper
{
    HyperlinkTapAndClickHelper::HyperlinkTapAndClickHelper(Platform::String^ name, FrameworkElement^ element)
    {
        m_name = name;

        RunOnUIThread([&]()
        {
            TextBlock^ tb = dynamic_cast<TextBlock^>(element);
            RichTextBlock^ rtb = dynamic_cast<RichTextBlock^>(element);
            VERIFY_IS_TRUE(tb || rtb);
            if (tb)
            {
                m_element = element;
                m_hyperlink = safe_cast<Hyperlink^>(tb->Inlines->GetAt(0));
            }
            else
            {
                m_element = rtb->OverflowContentTarget ? rtb->OverflowContentTarget : element;
                m_hyperlink = safe_cast<Hyperlink^>(safe_cast<Paragraph^>(rtb->Blocks->GetAt(0))->Inlines->GetAt(0));
            }

            m_pointerEnteredRegistration.Attach(m_element, [&]
            {
                LOG_OUTPUT(L"  > %s - Pointer entered.", m_name->Data());
                m_pointerEnteredEvent->Set();
            });

            m_clickRegistration.Attach(m_hyperlink, [&]
            {
                LOG_OUTPUT(L"  > %s - Hyperlink clicked.", m_name->Data());
                m_clickEvent->Set();
            });
        });
    }

    void TapOnHyperlink()
    {
        const auto& ih = TestServices::InputHelper;

        LOG_OUTPUT(L"> Tap %s's Hyperlink. Expect it to navigate.", m_name->Data());
        ih->Tap(m_element);
        m_clickEvent->WaitFor(1s);
        VERIFY_IS_TRUE(m_clickEvent->HasFired());
        m_clickEvent->Reset();
    }

    void MouseOverAndClickHyperlink()
    {
        const auto& ih = TestServices::InputHelper;

        // There will be a pointer enter before the click so simulate that as well.
        LOG_OUTPUT(L"> Mouse over %s.", m_name->Data());
        ih->MoveMouse(m_element, 30, 2);
        m_pointerEnteredEvent->WaitFor(1s);
        VERIFY_IS_TRUE(m_pointerEnteredEvent->HasFired());
        m_pointerEnteredEvent->Reset();

        LOG_OUTPUT(L"> Click %s's Hyperlink. Expect it to navigate.", m_name->Data());
        ih->LeftMouseClick(m_element);
        m_clickEvent->WaitFor(1s);
        VERIFY_IS_TRUE(m_clickEvent->HasFired());
        m_clickEvent->Reset();
    }

private:
    Platform::String^ m_name;
    Hyperlink^ m_hyperlink;
    FrameworkElement^ m_element;

    SafeEventRegistration<Hyperlink, wf::TypedEventHandler<Hyperlink ^, HyperlinkClickEventArgs ^>> m_clickRegistration { CreateSafeEventRegistration(Hyperlink, Click) };
    SafeEventRegistration<FrameworkElement, PointerEventHandler> m_pointerEnteredRegistration { CreateSafeEventRegistration(FrameworkElement, PointerEntered) };

    std::shared_ptr<Event> m_clickEvent { std::make_shared<Event>() };
    std::shared_ptr<Event> m_pointerEnteredEvent { std::make_shared<Event>() };
};

void HyperlinkInTextTests::ValidateTapAndClick()
{
    const auto& wh = TestServices::WindowHelper;

    TestCleanupWrapper cleanup;

    TextBlock^ tb1;
    TextBlock^ tb2;
    RichTextBlock^ rtb1;
    RichTextBlock^ rtb2;
    RichTextBlock^ rtbO1;
    RichTextBlock^ rtbO2;

    RunOnUIThread([&]()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
            L"    <StackPanel.Resources>"
            L"        <x:Boolean x:Key='HyperlinkUnderlineVisible'>False</x:Boolean>"
            L"    </StackPanel.Resources>"
            L"    <TextBlock x:Name='tb1' Height='100' Width='200' IsTextSelectionEnabled='false'>"
            L"      <Hyperlink Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"    <TextBlock x:Name='tb2' Height='100' Width='200' IsTextSelectionEnabled='true'>"
            L"      <Hyperlink Foreground='LightBlue'>Navigate!</Hyperlink>"
            L"    </TextBlock>"
            L"    <RichTextBlock x:Name='rtb1' Height='100' Width='200' IsTextSelectionEnabled='false'>"
            L"      <Paragraph><Hyperlink Foreground='LightBlue'>Navigate!</Hyperlink></Paragraph>"
            L"    </RichTextBlock>"
            L"    <RichTextBlock x:Name='rtb2' Height='100' Width='200' IsTextSelectionEnabled='true'>"
            L"      <Paragraph><Hyperlink Foreground='LightBlue'>Navigate!</Hyperlink></Paragraph>"
            L"    </RichTextBlock>"
            L"    <RichTextBlock x:Name='rtbO1' OverflowContentTarget='{Binding ElementName=overflow1}' Height='0' Width='0' IsTextSelectionEnabled='false'>"
            L"      <Paragraph> <Hyperlink Foreground='LightBlue'>Navigate!</Hyperlink></Paragraph>"
            L"    </RichTextBlock>"
            L"    <RichTextBlockOverflow x:Name='overflow1' Height='100' Width='200' />"
            L"    <RichTextBlock x:Name='rtbO2' OverflowContentTarget='{Binding ElementName=overflow2}' Height='0' Width='0' IsTextSelectionEnabled='true'>"
            L"      <Paragraph> <Hyperlink Foreground='LightBlue'>Navigate!</Hyperlink></Paragraph>"
            L"    </RichTextBlock>"
            L"    <RichTextBlockOverflow x:Name='overflow2' Height='100' Width='200' />"
            L"  </StackPanel>";

        StackPanel^ rootPanel = safe_cast<StackPanel^>(xaml_markup::XamlReader::Load(xamlString));

        tb1 = safe_cast<TextBlock^>(rootPanel->FindName(L"tb1"));
        tb2 = safe_cast<TextBlock^>(rootPanel->FindName(L"tb2"));
        rtb1 = safe_cast<RichTextBlock^>(rootPanel->FindName(L"rtb1"));
        rtb2 = safe_cast<RichTextBlock^>(rootPanel->FindName(L"rtb2"));
        rtbO1 = safe_cast<RichTextBlock^>(rootPanel->FindName(L"rtbO1"));
        rtbO2 = safe_cast<RichTextBlock^>(rootPanel->FindName(L"rtbO2"));

        wh->WindowContent = rootPanel;
    });
    wh->WaitForIdle();

    HyperlinkTapAndClickHelper tb1Helper(L"<TextBlock IsTextSelectionEnabled=\"false\">", tb1);
    HyperlinkTapAndClickHelper tb2Helper(L"<TextBlock IsTextSelectionEnabled=\"true\">", tb2);
    HyperlinkTapAndClickHelper rtb1Helper(L"<RichTextBlock IsTextSelectionEnabled=\"false\">", rtb1);
    HyperlinkTapAndClickHelper rtb2Helper(L"<RichTextBlock IsTextSelectionEnabled=\"true\">", rtb2);
    HyperlinkTapAndClickHelper rtbO1Helper(L"<RichTextBlockOverflow IsTextSelectionEnabled=\"false\">", rtbO1);
    HyperlinkTapAndClickHelper rtbO2Helper(L"<RichTextBlockOverflow IsTextSelectionEnabled=\"true\">", rtbO2);

    tb1Helper.TapOnHyperlink();
    tb2Helper.TapOnHyperlink();
    rtb1Helper.TapOnHyperlink();
    rtb2Helper.TapOnHyperlink();
    rtbO1Helper.TapOnHyperlink();
    rtbO2Helper.TapOnHyperlink();

    tb1Helper.MouseOverAndClickHyperlink();
    tb2Helper.MouseOverAndClickHyperlink();
    rtb1Helper.MouseOverAndClickHyperlink();
    rtb2Helper.MouseOverAndClickHyperlink();
    rtbO1Helper.MouseOverAndClickHyperlink();
    rtbO2Helper.MouseOverAndClickHyperlink();
}

} } } } } }
