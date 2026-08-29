// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class RectUtil
    {
        public:
            // This is a read-only alias for X.  If this is the empty rectangle,
            // the value will be positive infinity.
            static _Check_return_ HRESULT GetLeft(
                _In_ wf::Rect rect,
                _Out_ FLOAT* pValue);
            
            // This is a read-only alias for Y.  If this is the empty rectangle,
            // the value will be positive infinity.
            static _Check_return_ HRESULT GetTop(
                _In_ wf::Rect rect,
                _Out_ FLOAT* pValue);
            
            // This is a read-only alias for X + Width.  If this is the empty
            // rectangle, the value will be negative infinity.
            static _Check_return_ HRESULT GetRight(
                _In_ wf::Rect rect,
                _Out_ FLOAT* pValue);
            
            // This is a read-only alias for Y + Height.  If this is the empty
            // rectangle, the value will be negative infinity.
            static _Check_return_ HRESULT GetBottom(
                _In_ wf::Rect rect,
                _Out_ FLOAT* pValue);
            
            // This returns true if this rect is the Empty rectangle.  Note: If
            // width or height are 0 this Rectangle still contains a 0 or 1
            // dimensional set of points, so this method should not be used to
            // check for 0 area.
            static _Check_return_ HRESULT GetIsEmpty(
                _In_ wf::Rect rect,
                _Out_ BOOLEAN* pValue);
                
            // This returns true if this rect is the Empty rectangle.  Note: If
            // width or height are 0 this Rectangle still contains a 0 or 1
            // dimensional set of points, so this method should not be used to
            // check for 0 area.
            static BOOLEAN GetIsEmpty(
                _In_ const wf::Rect& rect);
            
            // Returns true if the Point is within the rectangle, inclusive of
            // the edges.  Returns false otherwise.
            static bool Contains(
                _In_ wf::Rect rect,
                _In_ wf::Point point);

            // Update this rectangle to be the intersection of this and rect.
            // If either this or rect are Empty, the result is Empty as well.
            static _Check_return_ HRESULT Intersect(
                _In_ wf::Rect& rect,
                _In_ wf::Rect rect2);
            
            // Update this rectangle to be the union of this and rect.
            static _Check_return_ HRESULT Union(
                _In_ wf::Rect& rect,
                _In_ wf::Rect rect2);
            
            // Create an empty rect
            static _Check_return_ wf::Rect CreateEmptyRect();
            static _Check_return_ HRESULT CreateEmptyRect(
                _Out_ wf::Rect* pEmptyRect);
                
            // Returns true if the rect2 is within rect, inclusive of
            // the edges.  Returns false otherwise.
            static BOOLEAN ContainsFully(
                _In_ const wf::Rect rect,
                _In_ const wf::Rect rect2);
            
            // Returns TRUE if either rect is empty or the rects
            // have an empty intersection.
            static BOOLEAN AreDisjoint(
                _In_ const wf::Rect& rect,
                _In_ const wf::Rect& rect2);

            // Construct a new Rect from a Point (specifying top-left corner) and a Size
            static wf::Rect CreateRect(
                _In_ const wf::Point& point,
                _In_ const wf::Size& size)
            {
                return {point.X, point.Y, size.Width, size.Height};
            }

            static wf::Size GetSize(_In_ const wf::Rect& rect)
            {
                wf::Size result = {rect.Width, rect.Height};
                return result;
            }

            static wf::Point GetPoint(_In_ const wf::Rect& rect)
            {
                wf::Point result = { rect.X, rect.Y };
                return result;
            }

            static wf::Rect GetInflatedRect(_In_ const wf::Rect& rect, float inflation)
            {
                float sizeInflation = 2 * inflation;
                wf::Rect result = { rect.X - inflation, rect.Y - inflation, rect.Width + sizeInflation, rect.Height + sizeInflation };
                return result;
            }

            static bool AreEqual(
                _In_ const wf::Rect& rect1, 
                _In_ const wf::Rect& rect2)
            {
                return rect1.X == rect2.X &&
                       rect1.Y == rect2.Y &&
                       rect1.Width == rect2.Width &&
                       rect1.Height == rect2.Height;
            }
    };
}

