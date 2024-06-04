// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <CDependencyObject.h>
#include <EnumDefs.g.h>
#include <GridLength.h>

_Check_return_ HRESULT CGridLength::Validate(const XGRIDLENGTH& gridLength)
{
    // NB: MAKE SURE ISFINITEF COMES FIRST!  Normally, testing NaN < 0 is OK to do,
    // but DLLs can enable floating-point exceptions in their code in a way that
    // affects the whole process, even when the NaN is non-signaling. To avoid that,
    // we'll test for NaN first by doing a comparison here that doesn't raise
    // a floating-point exception.
    if (!IsFiniteF(gridLength.value) || gridLength.value < 0.0f)
    {
        // Negative values, NaN and Infinity are not valid
        IFC_RETURN(E_INVALIDARG);
    }

    if (gridLength.type != DirectUI::GridUnitType::Auto &&
        gridLength.type != DirectUI::GridUnitType::Pixel &&
        gridLength.type != DirectUI::GridUnitType::Star)
    {
        // Undefined types are not valid
        IFC_RETURN(E_INVALIDARG);
    }
    return S_OK;
}
