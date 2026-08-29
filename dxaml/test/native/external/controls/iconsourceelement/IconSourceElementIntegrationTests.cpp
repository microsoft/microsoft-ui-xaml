// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "IconSourceElementIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace IconSourceElement {

    bool IconSourceElementIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool IconSourceElementIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool IconSourceElementIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void IconSourceElementIntegrationTests::ValidateBitmapIconSource()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::IconSourceElement^ iconSourceElement = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::IconSourceElement, Loaded);

        RunOnUIThread([&]()
        {
            iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(xaml_markup::XamlReader::Load(
                LR"(<IconSourceElement xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                      <BitmapIconSource />
                    </IconSourceElement>)"));

            loadedRegistration.Attach(iconSourceElement, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = iconSourceElement;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto bitmapIcon = TreeHelper::GetVisualChildByType<xaml_controls::BitmapIcon>(iconSourceElement);

            LOG_OUTPUT(L"Verifying BitmapIcon's default values...");
            VERIFY_IS_NOT_NULL(bitmapIcon);
            VERIFY_IS_NULL(bitmapIcon->UriSource);
            VERIFY_IS_TRUE(bitmapIcon->ShowAsMonochrome);

            iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(xaml_markup::XamlReader::Load(
                LR"(<IconSourceElement xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                      <BitmapIconSource UriSource='ms-appx:///resources/native/controls/iconsourceelement/bitimg.png' ShowAsMonochrome='False' />
                    </IconSourceElement>)"));

            loadedRegistration.Detach();
            loadedRegistration.Attach(iconSourceElement, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = iconSourceElement;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto bitmapIcon = TreeHelper::GetVisualChildByType<xaml_controls::BitmapIcon>(iconSourceElement);

            LOG_OUTPUT(L"Verifying BitmapIcon values set locally...");
            VERIFY_IS_NOT_NULL(bitmapIcon);

            auto expectedUri = ref new Platform::String(L"ms-appx:///resources/native/controls/iconsourceelement/bitimg.png");
            auto actualUri = bitmapIcon->UriSource->ToString();

            LOG_OUTPUT(L"Expected URI: %s", expectedUri->Data());
            LOG_OUTPUT(L"Actual URI: %s", actualUri->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedUri, actualUri) == 0);
            VERIFY_IS_FALSE(bitmapIcon->ShowAsMonochrome);
        });
    }

    void IconSourceElementIntegrationTests::ValidateFontIconSource()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::IconSourceElement^ iconSourceElement = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::IconSourceElement, Loaded);

        RunOnUIThread([&]()
        {
            iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(xaml_markup::XamlReader::Load(
                LR"(<IconSourceElement xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                      <FontIconSource />
                    </IconSourceElement>)"));

            loadedRegistration.Attach(iconSourceElement, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = iconSourceElement;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto fontIcon = TreeHelper::GetVisualChildByType<xaml_controls::FontIcon>(iconSourceElement);

            LOG_OUTPUT(L"Verifying FontIcon's default values...");
            VERIFY_IS_NOT_NULL(fontIcon);

            auto expectedGlyph = ref new Platform::String(L"");
            LOG_OUTPUT(L"Expected glyph: %s", expectedGlyph->Data());
            LOG_OUTPUT(L"Actual glyph: %s", fontIcon->Glyph->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedGlyph, fontIcon->Glyph) == 0);

            auto expectedFontFamily = ref new Platform::String(L"Segoe Fluent Icons,Segoe MDL2 Assets");
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedFontFamily, fontIcon->FontFamily->Source) == 0);

            VERIFY_ARE_EQUAL(20, fontIcon->FontSize);
            VERIFY_ARE_EQUAL(400, fontIcon->FontWeight.Weight);
            VERIFY_ARE_EQUAL(wut::FontStyle::Normal, fontIcon->FontStyle);
            VERIFY_IS_TRUE(fontIcon->IsTextScaleFactorEnabled);
            VERIFY_IS_FALSE(fontIcon->MirroredWhenRightToLeft);

            iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(xaml_markup::XamlReader::Load(
                LR"(<IconSourceElement xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                      <FontIconSource
                        Glyph='a'
                        FontFamily='Wingdings'
                        FontSize='10'
                        FontWeight='500'
                        FontStyle='Italic'
                        IsTextScaleFactorEnabled='False'
                        MirroredWhenRightToLeft='True' />
                    </IconSourceElement>)"));

            loadedRegistration.Detach();
            loadedRegistration.Attach(iconSourceElement, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = iconSourceElement;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto fontIcon = TreeHelper::GetVisualChildByType<xaml_controls::FontIcon>(iconSourceElement);

            LOG_OUTPUT(L"Verifying FontIcon values set locally...");
            VERIFY_IS_NOT_NULL(fontIcon);

            auto expectedGlyph = ref new Platform::String(L"a");
            LOG_OUTPUT(L"Expected glyph: %s", expectedGlyph->Data());
            LOG_OUTPUT(L"Actual glyph: %s", fontIcon->Glyph->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedGlyph, fontIcon->Glyph) == 0);

            auto expectedFontFamily = ref new Platform::String(L"Wingdings");
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedFontFamily, fontIcon->FontFamily->Source) == 0);

            VERIFY_ARE_EQUAL(10, fontIcon->FontSize);
            VERIFY_ARE_EQUAL(500, fontIcon->FontWeight.Weight);
            VERIFY_ARE_EQUAL(wut::FontStyle::Italic, fontIcon->FontStyle);
            VERIFY_IS_FALSE(fontIcon->IsTextScaleFactorEnabled);
            VERIFY_IS_TRUE(fontIcon->MirroredWhenRightToLeft);
        });
    }

    void IconSourceElementIntegrationTests::ValidatePathIconSource()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::IconSourceElement^ iconSourceElement = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::IconSourceElement, Loaded);

        RunOnUIThread([&]()
        {
            iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(xaml_markup::XamlReader::Load(
                LR"(<IconSourceElement xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                      <PathIconSource />
                    </IconSourceElement>)"));

            loadedRegistration.Attach(iconSourceElement, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = iconSourceElement;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto pathIcon = TreeHelper::GetVisualChildByType<xaml_controls::PathIcon>(iconSourceElement);

            LOG_OUTPUT(L"Verifying PathIcon's default values...");
            VERIFY_IS_NOT_NULL(pathIcon);
            VERIFY_IS_NULL(pathIcon->Data);

            iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(xaml_markup::XamlReader::Load(
                LR"(<IconSourceElement xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                      <PathIconSource Data='F1 M 16,12 20,2L 20,16 1,16' />
                    </IconSourceElement>)"));

            loadedRegistration.Detach();
            loadedRegistration.Attach(iconSourceElement, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = iconSourceElement;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto pathIcon = TreeHelper::GetVisualChildByType<xaml_controls::PathIcon>(iconSourceElement);

            LOG_OUTPUT(L"Verifying PathIcon values set locally...");
            VERIFY_IS_NOT_NULL(pathIcon);
            VERIFY_IS_NOT_NULL(pathIcon->Data);
            VERIFY_ARE_EQUAL(1, pathIcon->Data->Bounds.X);
            VERIFY_ARE_EQUAL(2, pathIcon->Data->Bounds.Y);
            VERIFY_ARE_EQUAL(19, pathIcon->Data->Bounds.Width);
            VERIFY_ARE_EQUAL(14, pathIcon->Data->Bounds.Height);
        });
    }

    void IconSourceElementIntegrationTests::ValidateSymbolIconSource()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::IconSourceElement^ iconSourceElement = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::IconSourceElement, Loaded);

        RunOnUIThread([&]()
        {
            iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(xaml_markup::XamlReader::Load(
                LR"(<IconSourceElement xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                      <SymbolIconSource />
                    </IconSourceElement>)"));

            loadedRegistration.Attach(iconSourceElement, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = iconSourceElement;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto symbolIcon = TreeHelper::GetVisualChildByType<xaml_controls::SymbolIcon>(iconSourceElement);

            LOG_OUTPUT(L"Verifying SymbolIcon's default values...");
            VERIFY_IS_NOT_NULL(symbolIcon);
            VERIFY_ARE_EQUAL(xaml_controls::Symbol::Emoji, symbolIcon->Symbol);

            iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(xaml_markup::XamlReader::Load(
                LR"(<IconSourceElement xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                      <SymbolIconSource Symbol="Accept" />
                    </IconSourceElement>)"));

            loadedRegistration.Detach();
            loadedRegistration.Attach(iconSourceElement, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = iconSourceElement;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto symbolIcon = TreeHelper::GetVisualChildByType<xaml_controls::SymbolIcon>(iconSourceElement);

            LOG_OUTPUT(L"Verifying SymbolIcon values set locally...");
            VERIFY_IS_NOT_NULL(symbolIcon);
            VERIFY_ARE_EQUAL(xaml_controls::Symbol::Accept, symbolIcon->Symbol);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::IconSourceElement
