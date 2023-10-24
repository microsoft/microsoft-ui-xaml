// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RectHelper.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a Rect from the specified coordinates and dimensions.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::FromCoordinatesAndDimensionsImpl(
    _In_ FLOAT x,
    _In_ FLOAT y,
    _In_ FLOAT width,
    _In_ FLOAT height,
    _Out_ wf::Rect* pReturnValue)
{
    if (width < 0.0F || height < 0.0F)
    {
        RRETURN(E_INVALIDARG);
    }

    pReturnValue->X = x;
    pReturnValue->Y = y;
    pReturnValue->Width = width;
    pReturnValue->Height = height;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a Rect from a location and size.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::FromLocationAndSizeImpl(
    _In_ wf::Point location,
    _In_ wf::Size size,
    _Out_ wf::Rect* pReturnValue)
{
    if (size.Width < 0.0F || size.Height < 0.0F)
    {
        // Return an empty rect if the size is empty.
        RRETURN(get_Empty(pReturnValue));
    }
    else
    {
        pReturnValue->X = location.X;
        pReturnValue->Y = location.Y;
        pReturnValue->Width = size.Width;
        pReturnValue->Height = size.Height;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a Rect from two points.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::FromPointsImpl(
    _In_ wf::Point point1,
    _In_ wf::Point point2,
    _Out_ wf::Rect* pReturnValue)
{

    pReturnValue->X = static_cast<FLOAT>(DoubleUtil::Min(point1.X, point2.X));
    pReturnValue->Y = static_cast<FLOAT>(DoubleUtil::Min(point1.Y, point2.Y));

    //  Max with 0 to prevent double weirdness from causing us to be (-epsilon..0)
    pReturnValue->Width = static_cast<FLOAT>(DoubleUtil::Max(DoubleUtil::Max(point1.X, point2.X) - pReturnValue->X, 0));
    pReturnValue->Height = static_cast<FLOAT>(DoubleUtil::Max(DoubleUtil::Max(point1.Y, point2.Y) - pReturnValue->Y, 0));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a rect that represents an Empty rectangle.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::get_EmptyImpl(_Out_ wf::Rect* pValue)
{
    RRETURN(RectUtil::CreateEmptyRect(pValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This returns true if this rect is the Empty rectangle.  Note: If
//      width or height are 0 this Rectangle still contains a 0 or 1
//      dimensional set of points, so this method should not be used to
//      check for 0 area.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::GetIsEmptyImpl(_In_ wf::Rect target, _Out_ BOOLEAN* pValue)
{
    RRETURN(RectUtil::GetIsEmpty(target, pValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This is a read-only alias for Y + Height.  If this is the empty
//      rectangle, the value will be negative infinity.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::GetBottomImpl(_In_ wf::Rect target, _Out_ FLOAT* pValue)
{
    RRETURN(RectUtil::GetBottom(target, pValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This is a read-only alias for X.  If this is the empty rectangle,
//      the value will be positive infinity.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::GetLeftImpl(_In_ wf::Rect target, _Out_ FLOAT* pValue)
{
    RRETURN(RectUtil::GetLeft(target, pValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This is a read-only alias for X + Width.  If this is the empty
//      rectangle, the value will be negative infinity.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::GetRightImpl(_In_ wf::Rect target, _Out_ FLOAT* pValue)
{
    RRETURN(RectUtil::GetRight(target, pValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This is a read-only alias for Y.  If this is the empty rectangle,
//      the value will be positive infinity.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::GetTopImpl(_In_ wf::Rect target, _Out_ FLOAT* pValue)
{
    RRETURN(RectUtil::GetTop(target, pValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if the Point is within the rectangle, inclusive of
//      the edges.  Returns false otherwise.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::ContainsImpl(
    _In_ wf::Rect target,
    _In_ wf::Point point,
    _Out_ BOOLEAN* pReturnValue)
{
    *pReturnValue = !!RectUtil::Contains(target, point);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares two Rect structs for equality.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::EqualsImpl(
    _In_ wf::Rect target,
    _In_ wf::Rect value,
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isEmpty = FALSE;

    IFC(RectUtil::GetIsEmpty(target, &isEmpty));
    if (isEmpty)
    {
        IFC(RectUtil::GetIsEmpty(value, &isEmpty));
        *pReturnValue = isEmpty;
    }
    else
    {
        *pReturnValue = (target.X == value.X &&
                         target.Y == value.Y &&
                         target.Width == value.Width &&
                         target.Height == value.Height);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates this rectangle to be the intersection of this and rect.
//      If either this or rect are Empty, the result is Empty as well.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::IntersectImpl(
    _In_ wf::Rect target,
    _In_ wf::Rect rect,
    _Out_ wf::Rect* pReturnValue)
{
    HRESULT hr = S_OK;

    IFC(RectUtil::Intersect(target, rect));

    pReturnValue->X = target.X;
    pReturnValue->Y = target.Y;
    pReturnValue->Width = target.Width;
    pReturnValue->Height = target.Height;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the union of the target rect with a point.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::UnionWithPointImpl(
    _In_ wf::Rect target,
    _In_ wf::Point point,
    _Out_ wf::Rect* pReturnValue)
{
    wf::Rect rect = {};

    rect.X = point.X;
    rect.Y = point.Y;
    rect.Width = 0.0F;
    rect.Height = 0.0F;

    RRETURN(UnionWithRect(target, rect, pReturnValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the union of the target rect with another rect.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RectHelperFactory::UnionWithRectImpl(
    _In_ wf::Rect target,
    _In_ wf::Rect rect,
    _Out_ wf::Rect* pReturnValue)
{
    HRESULT hr = S_OK;

    IFC(RectUtil::Union(target, rect));

    pReturnValue->X = target.X;
    pReturnValue->Y = target.Y;
    pReturnValue->Width = target.Width;
    pReturnValue->Height = target.Height;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
RectHelperFactory::DipsRectToPhysicalRectImpl(
    _In_ wf::Rect dipsRect,
    _Out_ wf::Rect* pReturnValue)
{
    //BUG: API IRectHelperStaticsPrivate.DipsRectToPhysicalRect is not compatible with AppWindows

    const auto contentRootCoordinator = DXamlCore::GetCurrent()->GetHandle()->GetContentRootCoordinator();
    const auto root = contentRootCoordinator->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot();
    const auto scale = RootScale::GetRasterizationScaleForContentRoot(root);

    DXamlCore::GetCurrent()->DipsToPhysicalPixels(scale, &dipsRect, pReturnValue);

    return S_OK;
}
