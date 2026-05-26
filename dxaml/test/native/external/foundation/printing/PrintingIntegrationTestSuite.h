// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Printing {

class PrintingTests : public WEX::TestClass<PrintingTests>
{
public:
    BEGIN_TEST_CLASS(PrintingTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Ignore", L"True") // Re-enable printing tests that were disabled as part of lifting Xaml tests.
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1d91ef47-c885-45e2-a578-7aaf1a1b1296;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(BasicPrint)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD();

    BEGIN_TEST_METHOD(BasicPrintFacades)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD();

    BEGIN_TEST_METHOD(XamlCompositionBrushBasePrint)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Ignore until PDF baseline file is approved
        TEST_METHOD_PROPERTY(L"Description", L"Test printing of XamlCompositionBrushBase - should use solid FallbackColor.")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD();

    BEGIN_TEST_METHOD(RasterizationScale)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD();

private:
     };
        } }
    } } } }
