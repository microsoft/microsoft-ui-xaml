// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    
namespace AppAnalysis {
    class EventProcessorTests : public WEX::TestClass<EventProcessorTests>
    {
    public:
        BEGIN_TEST_CLASS(EventProcessorTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateBasicProperties)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Platform", L"Desktop")
            TEST_METHOD_PROPERTY(L"BCQ", L"TRUE")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the event processor class can add rules and fire notifications.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateRuleDoesntGetCallbackTwice)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Platform", L"Desktop")
            TEST_METHOD_PROPERTY(L"BCQ", L"TRUE")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the event processor doesn't add callbacks multiple times to engine.")
        END_TEST_METHOD()

    };
}

} } } } }
