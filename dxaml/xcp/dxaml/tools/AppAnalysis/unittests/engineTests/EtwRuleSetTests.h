// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    
namespace AppAnalysis {
    class ETWRuleSetTests : public WEX::TestClass<ETWRuleSetTests>
    {
    public:
        BEGIN_TEST_CLASS(ETWRuleSetTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateBasicProperties)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Platform", L"Desktop")
            TEST_METHOD_PROPERTY(L"BCQ", L"TRUE")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that basic properties are properly implemented.")
        END_TEST_METHOD()
    };
}

} } } } }