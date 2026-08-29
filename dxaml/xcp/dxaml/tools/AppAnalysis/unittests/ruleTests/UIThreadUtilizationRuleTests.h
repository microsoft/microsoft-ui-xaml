// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Tools { 
    
namespace AppAnalysis {
    class UIThreadUtilizationRuleTests : public WEX::TestClass<UIThreadUtilizationRuleTests>
    {
    public:
        BEGIN_TEST_CLASS(UIThreadUtilizationRuleTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateUIThreadUtilizationMeasurement)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Platform", L"Desktop")
            TEST_METHOD_PROPERTY(L"BCQ", L"FALSE")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the data we calculate in a static ETL file and the rule fires when under 80%.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIThreadUtilizationNoFire)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Platform", L"Desktop")
            TEST_METHOD_PROPERTY(L"BCQ", L"FALSE")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the rule doesn't fire when percentage is above 80%")
        END_TEST_METHOD()

    };

} } } } } }
