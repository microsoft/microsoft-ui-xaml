// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SecondaryContentRelationship.g.h"

namespace DirectUI
{
    _Check_return_ HRESULT CreateNPointSecondaryContentRelationship(
        _In_ xaml::IUIElement *pPrimaryContent,
        _In_ xaml::IUIElement *pSecondaryContent,
        _In_ xaml::IDependencyObject *pDependencyPropertyHolder,
        _In_ HSTRING strPrimaryContentProperty,
        _In_ DirectUI::DirectManipulationProperty secondaryContentProperty,
        _In_ HSTRING strAssociatedDependencyProperty,
        _In_opt_ HSTRING strOrthogonalPrimaryContentProperty,
        _In_opt_ DirectUI::DirectManipulationProperty orthogonalSecondaryContentProperty,
        _In_opt_ HSTRING strOrthogonalAssociatedDependencyProperty,
        _In_ UINT numPoints,
        _In_reads_(numPoints) const DOUBLE *pPrimaryContentValues,
        _In_reads_(numPoints) const DOUBLE *pSecondaryContentValues,
        _Outptr_result_maybenull_ xaml::Internal::ISecondaryContentRelationship **ppSecondaryContentRelationship);

    _Check_return_ HRESULT UpdateStickyHeaderCurve(
        _In_ DirectUI::SecondaryContentRelationship* pRelationship,
        _In_ DOUBLE groupTopY,
        _In_ DOUBLE groupBottomY,
        _In_ DOUBLE headerHeight);

}
