// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

#include <VisualStatesHelper.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace VisualStatesHelper {

    class VisualStatesHelperUnitTests : public WEX::TestClass<VisualStatesHelperUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(VisualStatesHelperUnitTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateGetValidVisualStatesListViewBaseItem)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the static function GetValidVisualStatesListViewBaseItem is functioning properly.")
        END_TEST_METHOD()

    private:
    };

} } } } } } 
