// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace SymbolIcon {

    class SymbolIconIntegrationTests : public WEX::TestClass<SymbolIconIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(SymbolIconIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a SymbolIcon.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a SymbolIcon from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetSymbolProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get the SymbolIcon.Symbol property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySymbolGlyphText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the glyph output of the Symbol enum.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCreateFromXamlUsingTypeConverter)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can create a SymbolIcon by specifying just the symbol when setting AppBarButton/AppBarToggleButton.Icon in XAML.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of SymbolIcon.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSymbolsRS3)
            TEST_METHOD_PROPERTY(L"Description", L"Validates new Symbols added to enumeration in Redstone 3.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    };

} } } } } }

