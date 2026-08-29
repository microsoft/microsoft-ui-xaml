// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace IconSourceElement {

    class IconSourceElementIntegrationTests : public WEX::TestClass<IconSourceElementIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(IconSourceElementIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;a69ddfa4-5142-4bed-887d-6d0ca14a3473;b34da8d2-333d-40a9-a19c-94b1f9785580")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)
 
        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(ValidateBitmapIconSource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that assigning a BitmapIconSource to an IconSourceElement causes a BitmapIcon to be put into the visual tree.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateFontIconSource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that assigning a FontIconSource to an IconSourceElement causes a FontIcon to be put into the visual tree.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidatePathIconSource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that assigning a PathIconSource to an IconSourceElement causes a PathIcon to be put into the visual tree.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateSymbolIconSource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that assigning a SymbolIconSource to an IconSourceElement causes a SymbolIcon to be put into the visual tree.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //
    };

} } } } } }
