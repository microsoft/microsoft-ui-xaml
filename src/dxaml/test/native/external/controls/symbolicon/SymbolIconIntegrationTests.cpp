// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <FileLoader.h>
#include <WUCRenderingScopeGuard.h>
#include "SymbolIconIntegrationTests.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace SymbolIcon {

    bool SymbolIconIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool SymbolIconIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool SymbolIconIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void SymbolIconIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::SymbolIcon>::CanInstantiate();
    }

    void SymbolIconIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::SymbolIcon>::CanEnterAndLeaveLiveTree();
    }

    void SymbolIconIntegrationTests::CanSetAndGetSymbolProperty()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]
        {
            auto symbolIcon = ref new xaml_controls::SymbolIcon();
            VERIFY_ARE_EQUAL(symbolIcon->Symbol, xaml_controls::Symbol::Emoji);

            auto symbol = xaml_controls::Symbol::HangUp;
            symbolIcon->Symbol = symbol;
            VERIFY_ARE_EQUAL(symbolIcon->Symbol, symbol);
        });
    }

    void SymbolIconIntegrationTests::CanCreateFromXamlUsingTypeConverter()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]
        {
            auto button = safe_cast<xaml_controls::AppBarButton^>(
                xaml_markup::XamlReader::Load(L"<AppBarButton xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Icon='Video'/>"));

            VERIFY_IS_NOT_NULL(button->Icon);

            auto symbolIcon = safe_cast<xaml_controls::SymbolIcon^>(button->Icon);
            VERIFY_ARE_EQUAL(symbolIcon->Symbol, xaml_controls::Symbol::Video);
        });
    }

    void SymbolIconIntegrationTests::VerifySymbolGlyphText()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SymbolIcon^ symbolIcon;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <SymbolIcon x:Name="symbolIcon" Symbol="Accept" VerticalAlignment="Center" HorizontalAlignment="Center" />
                    </Grid>)"));

            symbolIcon = safe_cast<xaml_controls::SymbolIcon^>(rootPanel->FindName(L"symbolIcon"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            auto grid = safe_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetChild(symbolIcon, 0));
            VERIFY_IS_NOT_NULL(grid);

            auto textBlock = safe_cast<xaml_controls::TextBlock^>(xaml_media::VisualTreeHelper::GetChild(grid, 0));
            VERIFY_IS_NOT_NULL(textBlock);
            
            auto actualGlyph = textBlock->Text;
            auto expectedGlyph = ref new Platform::String(L"\uE8FB");

            LOG_OUTPUT(L"Expected glyph: %s", expectedGlyph->Data());
            LOG_OUTPUT(L"Actual glyph: %s", actualGlyph->Data());

            VERIFY_ARE_EQUAL(textBlock->Text, expectedGlyph);
        });
    }

    void SymbolIconIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedSymbolIconWidth = 20;
        double const expectedSymbolIconHeight = 20;

        xaml_controls::SymbolIcon^ symbolIcon;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <SymbolIcon x:Name="symbolIcon" Symbol="Accept" VerticalAlignment="Center" HorizontalAlignment="Center" />
                    </Grid>)"));

            symbolIcon = safe_cast<xaml_controls::SymbolIcon^>(rootPanel->FindName(L"symbolIcon"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedSymbolIconWidth, symbolIcon->ActualWidth);
            VERIFY_ARE_EQUAL(expectedSymbolIconHeight, symbolIcon->ActualHeight);
        });
    }

    void SymbolIconIntegrationTests::ValidateSymbolsRS3()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);

        xaml_controls::CommandBar^ bar;

        RunOnUIThread([&]()
        {
            bar = ref new xaml_controls::CommandBar();

            openedRegistration.Attach(bar, [&]() { openedEvent->Set(); });

            auto button1 = ref new xaml_controls::AppBarButton();
            button1->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::Print);
            auto button2 = ref new xaml_controls::AppBarButton();
            button2->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::GlobalNavigationButton);
            auto button3 = ref new xaml_controls::AppBarButton();
            button3->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::Share);
            auto button4 = ref new xaml_controls::AppBarButton();
            button4->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::XboxOneConsole);

            bar->PrimaryCommands->Append(button1);
            bar->PrimaryCommands->Append(button2);
            bar->PrimaryCommands->Append(button3);
            bar->PrimaryCommands->Append(button4);

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(bar);

            TestServices::WindowHelper->WindowContent = root;

        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            bar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        openedEvent->WaitForDefault();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::SymbolIcon
