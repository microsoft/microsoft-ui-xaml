// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomBrushes.h"
#include <Windows.h>
#include <collection.h>
#include "WexTestClass.h"
#include "PrintingIntegrationTestSuite.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <WindowsNumerics.h>
#include <ChangeDPI.h>

using namespace Platform;
using namespace Platform::Collections;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace ::Windows::Graphics::Printing;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Printing;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Composition;

using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Foundation::Graphics;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Printing {

bool PrintingTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool PrintingTests::TestCleanup()
{
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void PrintingTests::BasicPrint()
{
    TestCleanupWrapper cleanup;
    ChangeDPI changeDPI;

    PrintDocument^ printDoc;
    Object^ printSource;
    TextBlock^ tb;
    RunOnUIThread([&] () {
        printDoc = ref new PrintDocument();
        printSource = printDoc->DocumentSource;
        tb = ref new TextBlock();
        tb->Foreground = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        tb->Text="Some text to print";
        tb->FontSize = 40;
        // The TextBlock will be printed only when its layout is not dirty, manually measure/arrange it.
        tb->Measure(wf::Size(400, 400));
        tb->Arrange(::Windows::Foundation::Rect(0,0,800,800));

        printDoc->AddPages += ref new AddPagesEventHandler(
            [&](Object^ sender, AddPagesEventArgs^ args)
            {
                printDoc->AddPage(safe_cast<UIElement^>(tb));
                printDoc->AddPagesComplete();
            });
    });

    // Re-enable printing tests that were disabled as part of lifting Xaml tests.
    //TestServices::Utilities->VerifyPrinting(printSource);
}

void PrintingTests::BasicPrintFacades()
{
    TestCleanupWrapper cleanup;
    ChangeDPI changeDPI;

    PrintDocument^ printDoc;
    Object^ printSource;
    xaml_shapes::Rectangle^ rect;
    RunOnUIThread([&] () {
        printDoc = ref new PrintDocument();
        printSource = printDoc->DocumentSource;
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;
        rect->Translation = {100, 200, 0};
        rect->Scale = {2, 2, 1};

        printDoc->AddPages += ref new AddPagesEventHandler(
            [&](Object^ sender, AddPagesEventArgs^ args)
            {
                printDoc->AddPage(safe_cast<UIElement^>(rect));
                printDoc->AddPagesComplete();
            });
    });

    // Re-enable printing tests that were disabled as part of lifting Xaml tests.
    //TestServices::Utilities->VerifyPrinting(printSource);
}

void PrintingTests::XamlCompositionBrushBasePrint()
{
    TestCleanupWrapper cleanup;
    ChangeDPI changeDPI;

    PrintDocument^ printDoc;
    Object^ printSource;
    TextBlock^ tb;
    StackPanel^ sp;

    XcbPurpleBrush^ xcb1;
    XcbPurpleBrush^ xcb2;


    RunOnUIThread([&]() {
        printDoc = ref new PrintDocument();
        printSource = printDoc->DocumentSource;

        xcb1 = ref new XcbPurpleBrush();
        xcb1->FallbackColor = Microsoft::UI::Colors::Orange;

        xcb2 = ref new XcbPurpleBrush();
        xcb2->FallbackColor = Microsoft::UI::Colors::Blue;

        // SP bg should be Orange
        sp = ref new StackPanel();
        sp->Background = xcb1;

        // Text should be Blue
        tb = ref new TextBlock();
        tb->Foreground = xcb2;
        tb->Text = "Some text to print";
        tb->FontSize = 40;

        sp->Children->Append(tb);

        // The StackPanel will be printed only when its layout is not dirty, manually measure/arrange it.
        sp->Measure(wf::Size(400, 400));
        sp->Arrange(::Windows::Foundation::Rect(0, 0, 400, 400));

        printDoc->AddPages += ref new AddPagesEventHandler(
            [&](Object^ sender, AddPagesEventArgs^ args)
        {
            printDoc->AddPage(safe_cast<UIElement^>(sp));
            printDoc->AddPagesComplete();
        });
    });

    // Re-enable printing tests that were disabled as part of lifting Xaml tests.
    //TestServices::Utilities->VerifyPrinting(printSource);
}

void PrintingTests::RasterizationScale()
{
    TestCleanupWrapper cleanup;

    PrintDocument^ printDoc;
    Object^ printSource;
    xaml_controls::StackPanel^ stackPanel;

    RunOnUIThread([&] ()
    {
        TextBlock^ textBlock = ref new TextBlock();
        textBlock->Foreground = ref new SolidColorBrush(Microsoft::UI::Colors::Purple);
        textBlock->Text = "Some text to print";
        textBlock->FontSize = 40;

        xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
        ellipse->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        ellipse->Width = 100;
        ellipse->Height = 100;

        Border^ border = ref new Border();
        border->Width = 100;
        border->Height = 100;
        border->BorderBrush = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        border->BorderThickness = xaml::ThicknessHelper::FromUniformLength(20);
        border->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(30);

        stackPanel = ref new StackPanel();
        stackPanel->Children->Append(textBlock);
        stackPanel->Children->Append(ellipse);
        stackPanel->Children->Append(border);
        // Printing uses vector geometries instead of masks. This property should be entirely ignored.
        stackPanel->RasterizationScale = 0.1f;

        // The TextBlock will be printed only when its layout is not dirty, manually measure/arrange it.
        stackPanel->Measure(wf::Size(400, 400));
        stackPanel->Arrange(::Windows::Foundation::Rect(0,0,800,800));

        printDoc = ref new PrintDocument();
        printSource = printDoc->DocumentSource;

        printDoc->AddPages += ref new AddPagesEventHandler(
            [&](Object^ sender, AddPagesEventArgs^ args)
            {
                printDoc->AddPage(safe_cast<UIElement^>(stackPanel));
                printDoc->AddPagesComplete();
            });
    });

    // Re-enable printing tests that were disabled as part of lifting Xaml tests.
    //TestServices::Utilities->VerifyPrinting(printSource);
}

} }
} } } }

