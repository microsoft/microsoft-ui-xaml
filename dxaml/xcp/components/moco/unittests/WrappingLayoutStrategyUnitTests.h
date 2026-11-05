// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls {

class WrappingLayoutStrategyUnitTests : public WEX::TestClass<WrappingLayoutStrategyUnitTests>
{
public:
    BEGIN_TEST_CLASS(WrappingLayoutStrategyUnitTests)
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(ItemsPerPageHonorsLimitWithUnboundedStackingExtent);
    TEST_METHOD(ItemsPerPageHonorsLimitWithFiniteStackingExtent);
    TEST_METHOD(ItemsPerPageCountsCompleteStackingLines);
    TEST_METHOD(ItemsPerPageAccountsForGroupPaddingAndParallelHeader);
    TEST_METHOD(ItemsPerPagePreservesFiniteUncappedEstimate);
    TEST_METHOD(ItemsPerPageAllowsOneOversizedItem);
    TEST_METHOD(ItemsPerPageRequiresMeasuredCell);
    TEST_METHOD(ItemsPerPageRequiresMeasuredGroupHeader);
    TEST_METHOD(ItemsPerPageRequiresNonemptyViewport);
    TEST_METHOD(ItemsPerPageRequiresFiniteVirtualizingExtent);
};

} } } } }
