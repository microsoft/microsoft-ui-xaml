// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TickBar.g.h"
#include "Slider.g.h"
#include "Rectangle.g.h"
#include "RootScale.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Right now DEBUG_FLOAT_EPSILON is only used in Asserts to verify the correct layout of tick marks.
// The FLOAT value of finalLength cannot be compared precisely with DOUBLEs.
// TODO: We should make a FloatUtil class that uses FLOAT_EPSILON for AreClose() checks.
#ifndef DEBUG_FLOAT_EPSILON
#define DEBUG_FLOAT_EPSILON   (0.0001)
#endif

// Uncomment to get TickBar debug traces
// #define TICKBAR_DBG

#ifdef TICKBAR_DBG
#define g_szTickBarDbgLen 300
WCHAR g_szTickBarDbg[g_szTickBarDbgLen];
#endif // TICKBAR_DBG

// Initializes a new instance of the TickBar class.
TickBar::TickBar()
{
}

// Destroys an instance of the TickBar class.
TickBar::~TickBar()
{
}

// Arranges the tick marks of a TickBar.
IFACEMETHODIMP
TickBar::ArrangeOverride(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTemplatedParent;
    ctl::ComPtr<ISlider> spParentSlider;
    DOUBLE tickFrequency = 0;
    xaml_controls::Orientation orientation =
        xaml_controls::Orientation_Vertical;
    BOOLEAN bVerticalMode = FALSE;
    DOUBLE finalLength = 0;
    DOUBLE thumbLength = 0;
    DOUBLE thumbOffset = 0;
    DOUBLE visualRange = 0;
    DOUBLE min = 0;
    DOUBLE max = 0;
    UINT tickMarkNumber = 0;
    DOUBLE tickMarkInterval = 0;
    DOUBLE numIntervals = 0;
    UINT ratioOfLogicalToVisibleTickMarks = 0;
    UINT childrenSize = 0;
    INT tickMarkDelta = 0;
    INT i = 0;
    BOOLEAN bIsDirectionReversed = FALSE;
    wf::Rect rcTick = {};
    UINT j = 0;
    DOUBLE zoomScale = 0.0;
    DOUBLE singlePixelWidthScaled = 0.0;
    DOUBLE currentX = 0;
    DOUBLE currentY = 0;

#ifdef TICKBAR_DBG
    Trace(L"BEGIN TickBar::ArrangeOverride()");
#endif // TICKBAR_DBG

    IFCPTR(returnValue);

    IFC(this->get_TemplatedParent(&spTemplatedParent));
    spParentSlider = spTemplatedParent.AsOrNull<ISlider>();
    if (spParentSlider)
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

        // If tickFrequency <= 0, do nothing.
        IFC(spParentSlider->get_TickFrequency(&tickFrequency));
        if (DoubleUtil::LessThanOrClose(tickFrequency, 0))
        {
            goto Cleanup;
        }

        zoomScale = RootScale::GetRasterizationScaleForElement(GetHandle());
        singlePixelWidthScaled = 1 / zoomScale;

#ifdef TICKBAR_DBG
        swprintf_s(g_szTickBarDbg, g_szTickBarDbgLen,
            L"zoomScale=%.2f, singlePixelWidthScaled=%.2f", zoomScale, singlePixelWidthScaled);
        Trace(g_szTickBarDbg);
#endif // TICKBAR_DBG

        IFC(spParentSlider->get_Orientation(&orientation));
        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            finalLength = finalSize.Width;
            rcTick.Width = static_cast<FLOAT>(singlePixelWidthScaled);
            rcTick.Height = finalSize.Height;
        }
        else
        {
            bVerticalMode = TRUE;
            finalLength = finalSize.Height;
            rcTick.Width = finalSize.Width;
            rcTick.Height = static_cast<FLOAT>(singlePixelWidthScaled);
        }

        // Determine visualRange, the range in which we lay out the tick marks.  This distance
        // is equal to the track length of the Slider minus the Thumb length in the direction of orientation.
        IFC(spParentSlider.Cast<Slider>()->GetThumbLength(&thumbLength));
        visualRange = finalLength - thumbLength;

        // Determine tickInterval, the amount of space that goes between visual tick marks.
        IFC(spParentSlider.Cast<Slider>()->get_Minimum(&min));
        IFC(spParentSlider.Cast<Slider>()->get_Maximum(&max));

        // Determine the number of intervals between tick marks, and then the number of tick marks.
        // There is one tick mark at the end of each full interval, and an additional tick mark at the start.
        numIntervals = DoubleUtil::Max(1, (max - min)/tickFrequency);
        tickMarkNumber = static_cast<UINT>(DoubleUtil::Floor(numIntervals));

        tickMarkInterval = DoubleUtil::Max(1, visualRange / numIntervals);

#ifdef TICKBAR_DBG
        swprintf_s(g_szTickBarDbg, g_szTickBarDbgLen,
            L"finalLength=%.2f, thumbLength=%.2f", finalLength, thumbLength);
        Trace(g_szTickBarDbg);
        swprintf_s(g_szTickBarDbg, g_szTickBarDbgLen,
            L"visualRange=%.2f, numIntervals=%.2f", visualRange, numIntervals);
        Trace(g_szTickBarDbg);
        swprintf_s(g_szTickBarDbg, g_szTickBarDbgLen,
            L"tickMarkNumber=%d, tickMarkInterval=%.2f", tickMarkNumber, tickMarkInterval);
        Trace(g_szTickBarDbg);
#endif // TICKBAR_DBG

        if (DoubleUtil::LessThan(tickMarkInterval, MIN_TICKMARK_GAP))
        {
            // Windows has a requirement we must honor that visual tick marks are not closer than MIN_TICKMARK_GAP.
            // If our calculated tickMarkInterval does not meet this requirement, we find the smallest multiple
            // of tickMarkInterval > MIN_TICKMARK_GAP, and omit drawing tick marks whose values are not multiples
            // of that multiple.
            ratioOfLogicalToVisibleTickMarks = static_cast<UINT>(DoubleUtil::Ceil(MIN_TICKMARK_GAP / tickMarkInterval));
            tickMarkInterval *= ratioOfLogicalToVisibleTickMarks;
            tickMarkNumber /= ratioOfLogicalToVisibleTickMarks;

#ifdef TICKBAR_DBG
            Trace(L"tickMarkInterval < MIN_TICKMARK_GAP... will use smallest multiple of tickMarkInterval that is > MIN_TICKMARK_GAP");
            swprintf_s(g_szTickBarDbg, g_szTickBarDbgLen,
                L"ratioOfLogicalToVisibleTickMarks=%d", ratioOfLogicalToVisibleTickMarks);
            Trace(g_szTickBarDbg);
            swprintf_s(g_szTickBarDbg, g_szTickBarDbgLen,
                L"tickMarkNumber=%d, tickMarkInterval=%.2f", tickMarkNumber, tickMarkInterval);
            Trace(g_szTickBarDbg);
#endif // TICKBAR_DBG
        }

        // Draw the first tick mark. We need to account for this after doing the division by ratioOfLogicalToVisibleTickMarks,
        // so that it's not lost in that division.
        ++tickMarkNumber;
        Trace(L"++tickMarkNumber, to draw the first tick mark");

        // Create the tick marks in the Children collection.
        IFC(get_ChildrenInternal(&spChildren));
        IFC(spChildren->get_Size(&childrenSize));

        // tickMarkDelta is the number of tick marks we need to add or remove to reach tickMarkNumber, the
        // desired number of tick marks in the children collection.
        tickMarkDelta = static_cast<INT>(tickMarkNumber) - childrenSize;

        if (tickMarkDelta < 0)
        {
            // If we have more tick marks than we need, slough off the extra ones.
            for (i = tickMarkDelta; i < 0; ++i)
            {
                IFC(spChildren->RemoveAtEnd());
            }
        }
        else
        {
            ctl::ComPtr<IBrush> spTickBarFillBrush;

            // Create additional tick marks until we have enough.
            IFC(this->get_Fill(&spTickBarFillBrush));

            for (; i < tickMarkDelta; ++i)
            {
                ctl::ComPtr<Rectangle> spTickMark;

                IFC(ctl::make<Rectangle>(&spTickMark));
                if (bVerticalMode)
                {
                    IFC(spTickMark->put_Width(finalSize.Width));
                    IFC(spTickMark->put_Height(singlePixelWidthScaled));
                }
                else
                {
                    IFC(spTickMark->put_Width(singlePixelWidthScaled));
                    IFC(spTickMark->put_Height(finalSize.Height));
                }
                IFC(spTickMark->put_Fill(spTickBarFillBrush.Get()));

                IFC(spChildren->Append(spTickMark.Get()));
            }
        }

        // Actually arrange the tick marks.
        //
        // Note that the first tick mark should appear in the middle of the thumb when the thumb is
        // at its starting position.  If the thumb width is even, round up.
        //
        // When IsDirectionReversed is TRUE, we modify the value by 1px to account for the width of the tick mark.

        thumbOffset = DoubleUtil::Max(0, (thumbLength - singlePixelWidthScaled) / 2);
        IFC(spParentSlider->get_IsDirectionReversed(&bIsDirectionReversed));

#ifdef TICKBAR_DBG
            swprintf_s(g_szTickBarDbg, g_szTickBarDbgLen,
                L"finalSize.Width=%.2f, finalSize.Height=%.2f", finalSize.Width, finalSize.Height);
            Trace(g_szTickBarDbg);
#endif // TICKBAR_DBG

        for (; j < tickMarkNumber; ++j)
        {
            ctl::ComPtr<xaml::IUIElement> spChild;

            IFC(spChildren->GetAt(j, &spChild));

            DOUBLE tickOffset;
            if (bVerticalMode == bIsDirectionReversed)
            {
                // For horizontal Slider or vertical Slider with IsDirectionReversed, thumb and tick marks
                // start at the beginning of the track rect.
                // Distribution is important because the final gap between ticks may be only a partial interval.
                tickOffset = thumbOffset + j*tickMarkInterval;
            }
            else
            {
                // For vertical Slider or horizontal Slider with IsDirectionReversed, thumb and tick marks
                // start at the end of the track rect.
                // Distribution is important because the final gap between ticks may be only a partial interval.
                tickOffset = finalLength - (thumbOffset + singlePixelWidthScaled) - j*tickMarkInterval;
            }

            if (bVerticalMode)
            {
                currentY = tickOffset;
            }
            else
            {
                currentX = tickOffset;
            }

            // We don't want to use fractional coordinates since tick marks should only occupy 1 screen pixel.
            // Since rounding errors accumulate, we round to the closest position right before arranging the
            // tick mark, and do not pass the rounded value to our next calculation.

#ifdef TICKBAR_DBG
            swprintf_s(g_szTickBarDbg, g_szTickBarDbgLen,
                L"  TM:%d, currentX=%.2f, currentY=%.2f", j, currentX, currentY);
            Trace(g_szTickBarDbg);
#endif // TICKBAR_DBG

            // account for the 1px tick mark thickness
            ASSERT(DoubleUtil::GreaterThan(currentX, -singlePixelWidthScaled - DEBUG_FLOAT_EPSILON));
            ASSERT(DoubleUtil::LessThan(currentX, finalSize.Width + singlePixelWidthScaled + DEBUG_FLOAT_EPSILON));
            ASSERT(DoubleUtil::GreaterThan(currentY, -singlePixelWidthScaled - DEBUG_FLOAT_EPSILON));
            ASSERT(DoubleUtil::LessThan(currentY, finalSize.Height + singlePixelWidthScaled + DEBUG_FLOAT_EPSILON));

            rcTick.X = static_cast<FLOAT>(currentX);
            rcTick.Y = static_cast<FLOAT>(currentY);
            IFC(spChild.Cast<UIElement>()->Arrange(rcTick));
        }

#ifdef TICKBAR_DBG
        Trace(L"  END TickBar::ArrangeOverride()");
#endif // TICKBAR_DBG
    }

    *returnValue = finalSize;

Cleanup:
    RRETURN(hr);
}

// Overrides
_Check_return_ HRESULT
TickBar::OnPropertyChanged2(
    _In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(TickBarGenerated::OnPropertyChanged2(args));

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::TickBar_Fill)
    {
        IFC(PropagateFill());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
TickBar::PropagateFill()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IBrush> spTickBarFillBrush;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    UINT childrenSize = 0;

    IFC(get_Fill(&spTickBarFillBrush));
    IFC(get_ChildrenInternal(&spChildren));
    IFC(spChildren->get_Size(&childrenSize));

    for (UINT i = 0; i < childrenSize; ++i)
    {
        ctl::ComPtr<xaml::IUIElement> spChild;
        ctl::ComPtr<IRectangle> spTickMarkAsIRectangle;

        IFC(spChildren->GetAt(i, &spChild));
        IFC(spChild.As(&spTickMarkAsIRectangle));
        IFC(spTickMarkAsIRectangle.Cast<Rectangle>()->put_Fill(spTickBarFillBrush.Get()));
    }

Cleanup:
    RRETURN(hr);
}

