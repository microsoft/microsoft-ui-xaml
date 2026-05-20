// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PivotCurveGeneratorUnitTests.h"
#include <PivotCurveGenerator.h>

using namespace xaml_controls;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Pivot {

void PivotCurveGeneratorUnitTests::ValidateZeroItemCurve()
{
    PivotCurveGenerator pcg;

    const DOUBLE itemOffset = 1000.0;
 
    std::vector<DOUBLE> headerSizeVect;
    VERIFY_SUCCEEDED(pcg.SyncItemsSizeAndOrder(headerSizeVect, 0.0));
    VERIFY_SUCCEEDED(pcg.SyncSectionWidth(1000));
    VERIFY_SUCCEEDED(pcg.SyncSelectedItemOffset(itemOffset));

    VERIFY_IS_TRUE(pcg.AreCurvesDirty(true /* usingDynamicHeaders */));

    std::vector<DOUBLE> primaryOffsets;
    std::vector<DOUBLE> secondaryOffsets;
    pcg.GetDynamicCurveSegments(primaryOffsets, secondaryOffsets, 1000u /* panelMultiplier */);

    std::vector<DOUBLE> primaryExpectedOffsets;
    std::vector<DOUBLE> secondaryExpectedOffsets;

    primaryExpectedOffsets.push_back(0);
    primaryExpectedOffsets.push_back(itemOffset * 2);

    secondaryExpectedOffsets.push_back(itemOffset);
    secondaryExpectedOffsets.push_back(itemOffset);

    VERIFY_ARE_EQUAL(primaryOffsets.size(), primaryExpectedOffsets.size());
    VERIFY_ARE_EQUAL(secondaryOffsets.size(), secondaryExpectedOffsets.size());

    VERIFY_IS_TRUE(primaryOffsets == primaryExpectedOffsets);
    VERIFY_IS_TRUE(secondaryOffsets == secondaryExpectedOffsets);
}

} } } } } }