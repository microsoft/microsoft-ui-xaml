// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BringIntoViewOptions.g.h"

using namespace DirectUI;

_Check_return_ HRESULT BringIntoViewOptions::get_HorizontalAlignmentRatioImpl(_Out_ DOUBLE* value)
{
    *value = m_horizontalAlignmentRatio;
    return S_OK;
}

_Check_return_ HRESULT BringIntoViewOptions::put_HorizontalAlignmentRatioImpl(DOUBLE value)
{
    SetClampedAlignmentRatio(true /*isForHorizontalAlignmentRatio*/, value);
    return S_OK;
}

_Check_return_ HRESULT BringIntoViewOptions::get_VerticalAlignmentRatioImpl(_Out_ DOUBLE* value)
{
    *value = m_verticalAlignmentRatio;
    return S_OK;
}

_Check_return_ HRESULT BringIntoViewOptions::put_VerticalAlignmentRatioImpl(DOUBLE value)
{
    SetClampedAlignmentRatio(false /*isForHorizontalAlignmentRatio*/, value);
    return S_OK;
}

void BringIntoViewOptions::SetClampedAlignmentRatio(bool isForHorizontalAlignmentRatio, DOUBLE alignmentRatio)
{
    if (!DoubleUtil::IsNaN(alignmentRatio))
    {
        if (alignmentRatio < 0.0)
        {
            alignmentRatio = 0.0;
        }
        else if (alignmentRatio > 1.0)
        {
            alignmentRatio = 1.0;
        }
    }
    if (isForHorizontalAlignmentRatio)
    {
        m_horizontalAlignmentRatio = alignmentRatio;
    }
    else
    {
        m_verticalAlignmentRatio = alignmentRatio;
    }
}

_Check_return_ HRESULT BringIntoViewOptions::get_HorizontalOffsetImpl(_Out_ DOUBLE* value)
{
    IFCPTR_RETURN(value);
    *value = m_horizontalOffset;
    return S_OK;
}

_Check_return_ HRESULT BringIntoViewOptions::put_HorizontalOffsetImpl(DOUBLE value)
{
    if (DoubleUtil::IsNaN(value) || DoubleUtil::IsInfinity(value))
    {
        IFC_RETURN(E_INVALIDARG);
    }
    m_horizontalOffset = value;
    return S_OK;
}

_Check_return_ HRESULT BringIntoViewOptions::get_VerticalOffsetImpl(_Out_ DOUBLE* value)
{
    IFCPTR_RETURN(value);
    *value = m_verticalOffset;
    return S_OK;
}

_Check_return_ HRESULT BringIntoViewOptions::put_VerticalOffsetImpl(DOUBLE value)
{
    if (DoubleUtil::IsNaN(value) || DoubleUtil::IsInfinity(value))
    {
        IFC_RETURN(E_INVALIDARG);
    }
    m_verticalOffset = value;
    return S_OK;
}
