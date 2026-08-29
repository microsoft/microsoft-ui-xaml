// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft{ namespace UI { namespace Xaml { namespace Controls
{

// Helps the PivotHeaderManager generate secondary content relationships for
// the pivot's content so that it stays static relative to the viewport. Except,
// of course, during overpan. In which case the content will bounce.
class PivotStaticContentCurve
{
public:
    bool IsCurveDirty(unsigned itemsCount, double sectionWidth, bool isHeaderItemsCarouselEnabled) const;

    _Check_return_ HRESULT GetCurveSegments(
        _Inout_ std::vector<double>& primaryOffsets,
        _Inout_ std::vector<double>& secondaryOffsets,
        _In_ unsigned itemsCount,
        _In_ const double sectionWidth,
        _In_ bool isHeaderItemsCarouselEnabled,
        _In_ bool invert);

private:
    // Whenever the items count or the viewport size changes, the secondary relationship
    // needs to be updated.
    int m_itemsCount = -1;
    double m_sectionWidth = -1.0;
    bool m_isHeaderItemsCarouselEnabled = false;
};

} } } } XAML_ABI_NAMESPACE_END
