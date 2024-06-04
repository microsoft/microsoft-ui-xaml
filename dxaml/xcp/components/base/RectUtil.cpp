// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RectUtil.h"
#include "DoubleUtil.h"

using namespace DirectUI;

// This is a read-only alias for X.  If this is the empty rectangle,
// the value will be positive infinity.
_Check_return_ HRESULT RectUtil::GetLeft(
    _In_ wf::Rect rect,
    _Out_ FLOAT* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = rect.X;
    
    return S_OK;
}

// This is a read-only alias for Y.  If this is the empty rectangle,
// the value will be positive infinity.
_Check_return_ HRESULT RectUtil::GetTop(
    _In_ wf::Rect rect,
    _Out_ FLOAT* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = rect.Y;
    
    return S_OK;
}

// This is a read-only alias for X + Width.  If this is the empty
// rectangle, the value will be negative infinity.
_Check_return_ HRESULT RectUtil::GetRight(
    _In_ wf::Rect rect,
    _Out_ FLOAT* pValue)
{
    BOOLEAN bEmpty = FALSE;
    
    IFCPTR_RETURN(pValue);

    IFC_RETURN(GetIsEmpty(rect, &bEmpty));

    *pValue = static_cast<FLOAT>((bEmpty) ?
        DoubleUtil::NegativeInfinity :
        rect.X + rect.Width);
    
    return S_OK;
}

// This is a read-only alias for Y + Height.  If this is the empty
// rectangle, the value will be negative infinity.
_Check_return_ HRESULT RectUtil::GetBottom(
    _In_ wf::Rect rect,
    _Out_ FLOAT* pValue)
{
    BOOLEAN bEmpty = FALSE;
    
    IFCPTR_RETURN(pValue);

    IFC_RETURN(GetIsEmpty(rect, &bEmpty));

    *pValue = static_cast<FLOAT>((bEmpty) ?
        DoubleUtil::NegativeInfinity :
        rect.Y + rect.Height);
    
    return S_OK;
}

// This returns true if this rect is the Empty rectangle.  Note: If
// width or height are 0 this Rectangle still contains a 0 or 1
// dimensional set of points, so this method should not be used to
// check for 0 area.
BOOLEAN RectUtil::GetIsEmpty(
    _In_ const wf::Rect& rect)
{
    return rect.Width < 0;
}

// This returns true if this rect is the Empty rectangle.  Note: If
// width or height are 0 this Rectangle still contains a 0 or 1
// dimensional set of points, so this method should not be used to
// check for 0 area.
_Check_return_ HRESULT RectUtil::GetIsEmpty(
    _In_ wf::Rect rect,
    _Out_ BOOLEAN* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = GetIsEmpty(rect);
    
    return S_OK;
}

// Returns true if the Point is within the rectangle, inclusive of
// the edges.  Returns false otherwise.
bool RectUtil::Contains(
    _In_ wf::Rect rect,
    _In_ wf::Point point)
{
    // We include points on the edge as "contained".  We do "x - _width <= _x"
    // instead of "x <= _x + _width" so that this check works when _width is
    // PositiveInfinity and _x is NegativeInfinity.
    return
        (point.X >= rect.X) &&
        (point.X - rect.Width <= rect.X) &&
        (point.Y >= rect.Y) &&
        (point.Y - rect.Height <= rect.Y);
}

// Update this rectangle to be the intersection of this and rect.
// If either this or rect are Empty, the result is Empty as well.
_Check_return_ HRESULT RectUtil::Intersect(
    _In_ wf::Rect& rect,
    _In_ wf::Rect rect2)
{
    HRESULT hr = S_OK;
    DOUBLE left = 0.0;
    DOUBLE top = 0.0;
    
    if (AreDisjoint(rect, rect2))
    {
        rect.X = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
        rect.Y = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
        rect.Width = static_cast<FLOAT>(DoubleUtil::NegativeInfinity);
        rect.Height = static_cast<FLOAT>(DoubleUtil::NegativeInfinity);
    }
    else
    {
        left = DoubleUtil::Max(rect.X, rect2.X);
        top = DoubleUtil::Max(rect.Y, rect2.Y);

        //  Max with 0 to prevent double weirdness from causing us to be
        // (-epsilon..0)
        rect.Width = static_cast<FLOAT>(DoubleUtil::Max(DoubleUtil::Min(rect.X + rect.Width, rect2.X + rect2.Width) - left, 0));
        rect.Height = static_cast<FLOAT>(DoubleUtil::Max(DoubleUtil::Min(rect.Y + rect.Height, rect2.Y + rect2.Height) - top, 0));
        
        rect.X = static_cast<FLOAT>(left);
        rect.Y = static_cast<FLOAT>(top);
    }
    
    RRETURN(hr);
}

// Update this rectangle to be the union of this and rect.
_Check_return_ HRESULT RectUtil::Union(
    _In_ wf::Rect& rect,
    _In_ wf::Rect rect2)
{
    BOOLEAN isEmpty = FALSE;
    
    IFC_RETURN(GetIsEmpty(rect, &isEmpty));
    if (isEmpty)
    {
        rect.X = rect2.X;
        rect.Y = rect2.Y;
        rect.Width = rect2.Width;
        rect.Height = rect2.Height;
    }
    else
    {
        IFC_RETURN(GetIsEmpty(rect2, &isEmpty));
        if (!isEmpty)
        {
            DOUBLE left = 0.0;
            DOUBLE top = 0.0;
            
            left = DoubleUtil::Min(rect.X, rect2.X);
            top = DoubleUtil::Min(rect.Y, rect2.Y);
            
            // We need this check so that the math does not result in NaN
            if (DoubleUtil::IsPositiveInfinity(rect2.Width) || DoubleUtil::IsPositiveInfinity(rect.Width))
            {
                rect.Width = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
            }
            else
            {
                FLOAT right = 0.0;
                FLOAT rectRight = 0.0;
                FLOAT maxRight = 0.0;
                
                //  Max with 0 to prevent double weirdness from causing us to be
                // (-epsilon..0)
                IFC_RETURN(GetRight(rect, &right));
                IFC_RETURN(GetRight(rect2, &rectRight));
                maxRight = static_cast<FLOAT>(DoubleUtil::Max(right, rectRight));
                rect.Width = static_cast<FLOAT>(DoubleUtil::Max(maxRight - left, 0.0));
            }
            
            // We need this check so that the math does not result in NaN
            if (DoubleUtil::IsPositiveInfinity(rect2.Height) || DoubleUtil::IsPositiveInfinity(rect.Height))
            {
                rect.Height = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
            }
            else
            {
                FLOAT bottom = 0.0;
                FLOAT rectBottom = 0.0;
                FLOAT maxBottom = 0.0;
                
                //  Max with 0 to prevent double weirdness from causing us to be
                // (-epsilon..0)
                IFC_RETURN(GetBottom(rect, &bottom));
                IFC_RETURN(GetBottom(rect2, &rectBottom));
                maxBottom = static_cast<FLOAT>(DoubleUtil::Max(bottom, rectBottom));
                rect.Height = static_cast<FLOAT>(DoubleUtil::Max(maxBottom - top, 0.0));
            }
            
            rect.X = static_cast<FLOAT>(left);
            rect.Y = static_cast<FLOAT>(top);
        }
    }
    
    return S_OK;
}

// Create an empty rect
_Check_return_ wf::Rect RectUtil::CreateEmptyRect()
{
    wf::Rect result;

    result.X = std::numeric_limits<float>::infinity();
    result.Y = std::numeric_limits<float>::infinity();
    result.Width = -std::numeric_limits<float>::infinity();
    result.Height = -std::numeric_limits<float>::infinity();

    return result;
}

_Check_return_ HRESULT RectUtil::CreateEmptyRect(
    _Out_ wf::Rect* pEmptyRect)
{
    IFCPTR_RETURN(pEmptyRect);

    *pEmptyRect = CreateEmptyRect();

    return S_OK;
}

// Returns true if the rect2 is within rect, inclusive of
// the edges.  Returns false otherwise.
BOOLEAN RectUtil::ContainsFully(
    _In_ const wf::Rect rect,
    _In_ const wf::Rect rect2)
{
    return  rect2.X >= rect.X &&
            rect2.X + rect2.Width <= rect.X + rect.Width &&
            rect2.Y >= rect.Y &&
            rect2.Y + rect2.Height <= rect.Y + rect.Height;
}

// Returns TRUE if either rect is empty or the rects
// have an empty intersection.
BOOLEAN RectUtil::AreDisjoint(
    _In_ const wf::Rect& rect,
    _In_ const wf::Rect& rect2)
{
    const BOOLEAN doIntersect = 
        !(rect.Width < 0 || rect2.Width < 0) && 
        (rect2.X <= rect.X + rect.Width) &&
        (rect2.X + rect2.Width >= rect.X) &&
        (rect2.Y <= rect.Y + rect.Height) &&
        (rect2.Y + rect2.Height >= rect.Y);
    return !doIntersect;
}
