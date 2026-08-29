// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PivotStaticContentCurve.h"
#include "PivotCommon.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

bool PivotStaticContentCurve::IsCurveDirty(
    unsigned itemsCount,
    double sectionWidth,
    bool isHeaderItemsCarouselEnabled) const
{
    return m_itemsCount != itemsCount ||
           m_sectionWidth != sectionWidth ||
           m_isHeaderItemsCarouselEnabled != isHeaderItemsCarouselEnabled;
}

_Check_return_ HRESULT
PivotStaticContentCurve::GetCurveSegments(
    _Inout_ std::vector<double>& primaryOffsets,
    _Inout_ std::vector<double>& secondaryOffsets,
    _In_ unsigned itemsCount,
    _In_ const double sectionWidth,
    _In_ bool isHeaderItemsCarouselEnabled,
    _In_ bool invert)
{
    primaryOffsets.reserve(4);
    secondaryOffsets.reserve(4);

    // The exact value of overpanRange is not important
    // as long as it's bigger than the overpan magnitude
    // allowed by DManip.
    const double overpanRange = 1000.0;
    const double viewportOffset =
        isHeaderItemsCarouselEnabled ?
        GetPivotPanelMultiplier() * sectionWidth :
        (itemsCount == 0 ? 0.0 : (itemsCount - 1) * sectionWidth);

    // The independant variable values
    primaryOffsets.push_back(-overpanRange);
    primaryOffsets.push_back(0.0);
    primaryOffsets.push_back(viewportOffset);
    primaryOffsets.push_back(viewportOffset + overpanRange);

    // The dependant variable values
    secondaryOffsets.push_back(0.0);
    secondaryOffsets.push_back(0.0);
    secondaryOffsets.push_back(invert ? viewportOffset : -viewportOffset);
    secondaryOffsets.push_back(invert ? viewportOffset : -viewportOffset);

    // Update the items count and viewport size so that next time AreCurvesDirty
    // is called, we return false.
    m_itemsCount = static_cast<int>(itemsCount);
    m_sectionWidth = sectionWidth;
    m_isHeaderItemsCarouselEnabled = isHeaderItemsCarouselEnabled;

    return S_OK;
}

} } } } XAML_ABI_NAMESPACE_END
