// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollerSnapPoint.h"

// Required for Modern Idl bug, should never be called.
SnapPointBase::SnapPointBase()
{
    // throw (ERROR_CALL_NOT_IMPLEMENTED);
}

winrt::hstring SnapPointBase::GetTargetExpression(winrt::hstring target) const
{
    return StringUtil::FormatString(L"this.Target.%1!s!", target.data());
}

bool SnapPointBase::operator<(SnapPointBase* snapPoint)
{
    ScrollerSnapPointSortPredicate mySortPredicate = SortPredicate();
    ScrollerSnapPointSortPredicate theirSortPredicate = snapPoint->SortPredicate();
    if (mySortPredicate.primary < theirSortPredicate.primary)
    {
        return true;
    } 
    if (theirSortPredicate.primary < mySortPredicate.primary)
    {
        return false;
    }

    if (mySortPredicate.secondary < theirSortPredicate.secondary)
    {
        return true;
    }
    if (theirSortPredicate.secondary < mySortPredicate.secondary)
    {
        return false;
    }

    if (mySortPredicate.tertiary < theirSortPredicate.tertiary)
    {
        return true;
    }
    return false;
}

bool SnapPointBase::operator==(SnapPointBase* snapPoint)
{
    ScrollerSnapPointSortPredicate mySortPredicate = SortPredicate();
    ScrollerSnapPointSortPredicate theirSortPredicate = snapPoint->SortPredicate();
    if (std::abs(mySortPredicate.primary - theirSortPredicate.primary) < s_equalityEpsilon
        && std::abs(mySortPredicate.secondary - theirSortPredicate.secondary) < s_equalityEpsilon
        && mySortPredicate.tertiary == theirSortPredicate.tertiary)
    {
        return true;
    }
    return false;
}

#ifdef ApplicableRangeType
double SnapPointBase::ApplicableRange()
{
    return m_specifiedApplicableRange;
}

winrt::SnapPointApplicableRangeType SnapPointBase::ApplicableRangeType()
{
    return m_applicableRangeType;
}
#endif

int SnapPointBase::CombinationCount()
{
    return m_combinationCount;
}

#ifdef _DEBUG
winrt::Color SnapPointBase::VisualizationColor()
{
    return m_visualizationColor;
}

void SnapPointBase::VisualizationColor(winrt::Color color)
{       
    m_visualizationColor = color;
}
#endif // _DEBUG

std::tuple<double, double> SnapPointBase::ActualApplicableZone()
{
    return m_actualApplicableZone;
}

void SnapPointBase::ActualApplicableZone(std::tuple<double, double> zone)
{
    m_actualApplicableZone = zone;
}

/////////////////////////////////////////////////////////////////////
/////////////////      Scroll Snap Points     ///////////////////////
/////////////////////////////////////////////////////////////////////

// Required for Modern Idl bug, should never be called.
ScrollSnapPointBase::ScrollSnapPointBase()
{
    // throw (ERROR_CALL_NOT_IMPLEMENTED);
}

winrt::ScrollSnapPointsAlignment ScrollSnapPointBase::Alignment()
{
    return m_alignment;
}

void ScrollSnapPointBase::Alignment(winrt::ScrollSnapPointsAlignment alignment)
{
    m_alignment = alignment;
}

/////////////////////////////////////////////////////////////////////
//////////////    Irregular Scroll Snap Points   ////////////////////
/////////////////////////////////////////////////////////////////////
CppWinRTActivatableClassWithBasicFactory(ScrollSnapPoint);

ScrollSnapPoint::ScrollSnapPoint(
    double snapPointValue,
    winrt::ScrollSnapPointsAlignment alignment)
{
    m_specifiedValue = snapPointValue;
    m_actualValue = snapPointValue;
    m_alignment = alignment;
}

#ifdef ApplicableRangeType
ScrollSnapPoint::ScrollSnapPoint(
    double snapPointValue,
    double applicableRange,
    winrt::ScrollSnapPointsAlignment alignment)
{
    if (applicableRange <= 0)
    {
        throw winrt::hresult_invalid_argument(L"'applicableRange' must be strictly positive.");
    }

    m_specifiedValue = snapPointValue;
    m_actualValue = snapPointValue;
    m_alignment = alignment;
    m_specifiedApplicableRange = applicableRange;
    m_actualApplicableZone = std::tuple<double, double>{ snapPointValue - applicableRange, snapPointValue + applicableRange};
    m_applicableRangeType = winrt::SnapPointApplicableRangeType::Optional;
}
#endif

double ScrollSnapPoint::Value()
{
    return m_specifiedValue;
}

winrt::ExpressionAnimation ScrollSnapPoint::CreateRestingPointExpression(
    winrt::Compositor compositor, winrt::hstring, winrt::hstring scale)
{
    winrt::hstring expression = StringUtil::FormatString(L"snapPointValue * %1!s!", scale.data());
    winrt::ExpressionAnimation restingPointExpressionAnimation = compositor.CreateExpressionAnimation(expression);

    restingPointExpressionAnimation.SetScalarParameter(L"snapPointValue", static_cast<float>(m_actualValue));
    return restingPointExpressionAnimation;
}

winrt::ExpressionAnimation ScrollSnapPoint::CreateConditionalExpression(
    winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale)
{
    winrt::hstring targetExpression = GetTargetExpression(target);
    winrt::hstring scaledMinApplicableRange = StringUtil::FormatString(
        L"(minApplicableValue * %1!s!)",
        scale.data());
    winrt::hstring scaledMaxApplicableRange = StringUtil::FormatString(
        L"(maxApplicableValue * %1!s!)",
        scale.data());
    winrt::hstring expression = StringUtil::FormatString(
        L"%1!s! >= %2!s! && %1!s! <= %3!s!",
        targetExpression.data(),
        scaledMinApplicableRange.data(),
        scaledMaxApplicableRange.data());
    winrt::ExpressionAnimation conditionExpressionAnimation = compositor.CreateExpressionAnimation(expression);

    conditionExpressionAnimation.SetScalarParameter(L"minApplicableValue", static_cast<float>(std::get<0>(m_actualApplicableZone)));
    conditionExpressionAnimation.SetScalarParameter(L"maxApplicableValue", static_cast<float>(std::get<1>(m_actualApplicableZone)));
    return conditionExpressionAnimation;
}

ScrollerSnapPointSortPredicate ScrollSnapPoint::SortPredicate()
{
    // Irregular snap point should be sorted before regular snap points so it give a tertiary sort value of 0 (regular snap points get 1)
    return ScrollerSnapPointSortPredicate{ m_actualValue, m_actualValue, 0 };
}

void ScrollSnapPoint::DetermineActualApplicableZone(SnapPointBase* previousSnapPoint, SnapPointBase* nextSnapPoint)
{
    m_actualApplicableZone = std::tuple<double, double>{ DetermineMinActualApplicableZone(previousSnapPoint), DetermineMaxActualApplicableZone(nextSnapPoint) };
}

double ScrollSnapPoint::DetermineMinActualApplicableZone(SnapPointBase* previousSnapPoint)
{
    // If we are not passed a previousSnapPoint it means we are the first in the list, see if we expand to negative Infinity or stay put.
    if (!previousSnapPoint)
    {
#ifdef ApplicableRangeType
        if (m_applicableRangeType != winrt::SnapPointApplicableRangeType::Optional)
        {
            return -INFINITY;
        }
        else
        {
            return m_actualValue - m_specifiedApplicableRange;
        }
#else
        return -INFINITY;
#endif
    }
    // If we are passed a previousSnapPoint then we need to account for its influence on us.
    else
    {
        double previousMaxInfluence = previousSnapPoint->Influence(m_actualValue);

#ifdef ApplicableRangeType
        switch (m_applicableRangeType)
        {
        case winrt::SnapPointApplicableRangeType::Optional:
            return std::max(previousMaxInfluence, m_actualValue - m_specifiedApplicableRange);
        case winrt::SnapPointApplicableRangeType::Mandatory:
            return previousMaxInfluence;
        default:
            MUX_ASSERT(false);
            return 0.0f;
        }
#else
        return previousMaxInfluence;
#endif
    }
}

double ScrollSnapPoint::DetermineMaxActualApplicableZone(SnapPointBase* nextSnapPoint)
{
    // If we are not passed a nextSnapPoint it means we are the last in the list, see if we expand to Infinity or stay put.
    if (!nextSnapPoint)
    {
#ifdef ApplicableRangeType
        if (m_applicableRangeType != winrt::SnapPointApplicableRangeType::Optional)
        {
            return INFINITY;
        }
        else
        {
            return m_actualValue + m_specifiedApplicableRange;
        }
#else
        return INFINITY;
#endif
    }
    // If we are passed a nextSnapPoint then we need to account for its influence on us.
    else
    {
        double nextMinInfluence = nextSnapPoint->Influence(m_actualValue);

#ifdef ApplicableRangeType
        switch (m_applicableRangeType)
        {
        case winrt::SnapPointApplicableRangeType::Optional:
            return std::min(m_actualValue + m_specifiedApplicableRange, nextMinInfluence);
        case winrt::SnapPointApplicableRangeType::Mandatory:
            return nextMinInfluence;
        default:
            MUX_ASSERT(false);
            return 0.0f;
        }
#else
        return nextMinInfluence;
#endif
    }
}

double ScrollSnapPoint::Influence(double edgeOfMidpoint)
{
    double midPoint = (m_actualValue + edgeOfMidpoint) / 2;

#ifdef ApplicableRangeType
    switch (m_applicableRangeType)
    {
    case winrt::SnapPointApplicableRangeType::Optional:
        if (m_actualValue <= edgeOfMidpoint)
        {
            return std::min(m_actualValue + m_specifiedApplicableRange, midPoint);
        }
        else
        {
            return std::max(m_actualValue - m_specifiedApplicableRange, midPoint);
        }
    case winrt::SnapPointApplicableRangeType::Mandatory:
        return midPoint;
    default:
        MUX_ASSERT(false);
        return 0.0f;
    }
#else
    return midPoint;
#endif
}

void ScrollSnapPoint::Combine(winrt::SnapPointBase const& snapPoint)
{
    auto snapPointAsIrregular = snapPoint.try_as<winrt::ScrollSnapPoint>();
    if (snapPointAsIrregular)
    {
#ifdef ApplicableRangeType
        m_specifiedApplicableRange = std::max(snapPointAsIrregular.ApplicableRange(), m_specifiedApplicableRange);
#else
        MUX_ASSERT(m_specifiedApplicableRange == INFINITY);
#endif
        m_combinationCount++;
    }
    else
    {
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

double ScrollSnapPoint::Evaluate(double value)
{
    if (value >= std::get<0>(m_actualApplicableZone) && value <= std::get<1>(m_actualApplicableZone))
    {
        return m_actualValue;
    }
    return value;
}

/////////////////////////////////////////////////////////////////////
/////////////////    Regular Snap Points    /////////////////////////
/////////////////////////////////////////////////////////////////////
CppWinRTActivatableClassWithBasicFactory(RepeatedScrollSnapPoint);

RepeatedScrollSnapPoint::RepeatedScrollSnapPoint(
    double offset,
    double interval,
    double start,
    double end,
    winrt::ScrollSnapPointsAlignment alignment)
{
    ValidateConstructorParameters(
#ifdef ApplicableRangeType
        false /*applicableRangeToo*/,
        0 /*applicableRange*/,
#endif
        offset,
        interval,
        start,
        end);

    m_offset = offset;
    m_interval = interval;
    m_specifiedStart = start;
    m_actualStart = start;
    m_specifiedEnd = end;
    m_actualEnd = end;
}

#ifdef ApplicableRangeType
RepeatedScrollSnapPoint::RepeatedScrollSnapPoint(
    double offset,
    double interval,
    double start,
    double end,
    double applicableRange,
    winrt::ScrollSnapPointsAlignment alignment)
{
    ValidateConstructorParameters(
        true /*applicableRangeToo*/,
        applicableRange,
        offset,
        interval,
        start,
        end);
    
    m_offset = offset;
    m_interval = interval;
    m_specifiedStart = start;
    m_actualStart = start;
    m_specifiedEnd = end;
    m_actualEnd = end;
    m_specifiedApplicableRange = applicableRange;
    m_applicableRangeType = winrt::SnapPointApplicableRangeType::Optional;
}
#endif

double RepeatedScrollSnapPoint::Offset()
{
    return m_offset;
}

double RepeatedScrollSnapPoint::Interval()
{
    return m_interval;
}

double RepeatedScrollSnapPoint::Start()
{
    return m_specifiedStart;
}

double RepeatedScrollSnapPoint::End()
{
    return m_specifiedEnd;
}

winrt::ExpressionAnimation RepeatedScrollSnapPoint::CreateRestingPointExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale)
{
    /*
    ((Abs(target -                                         //The absolute value of the natural resting point minus
    ((Floor((target - scaledFirst) / scaledInterval)       //How many intervals the natural resting point has passed
     * scaledInterval) + scaledFirst)                      //The location of the regular snap point just before the natural resting point
    ) >= Abs(target -                                      //If it's greater than the absolute value of the natural resting point minus
    ((Ceil((target - scaledFirst) / scaledInterval)
     * scaledInterval) + scaledFirst)                      //The location of the regular snap point just after the natural resting point
    )) && (((Ceil((target - scaledFirst) / scaledInterval)
     * scaledInterval) + scaledFirst) <= scaledEnd)        //And the location of the regular snap point just after the natural resting point is before the end value
    ) ? ((Ceil((target - scaledFirst) / scaledInterval)    //Then return the location of the regular snap point just after the natural resting point
     * scaledInterval) + scaledFirst)
     : ((Floor((target - scaledFirst) / scaledInterval)    //Otherwise return the location of the regular snap point just before the natural resting point 
     * scaledInterval) + scaledFirst)
    */

    winrt::hstring targetExpression = GetTargetExpression(target);
    winrt::hstring scaledInterval = StringUtil::FormatString(
        L"(itv * %1!s!)",
        scale.data());
    winrt::hstring scaledEnd = StringUtil::FormatString(
        L"(end * %1!s!)",
        scale.data());
    winrt::hstring scaledFirst = StringUtil::FormatString(
        L"(first * %1!s!)",
        scale.data());
    winrt::hstring expression = StringUtil::FormatString(
        L"((Abs(%1!s! - ((Floor((%1!s! - %4!s!) / %2!s!) * %2!s!) + %4!s!)) >= Abs(%1!s! - ((Ceil((%1!s! - %4!s!) / %2!s!) * %2!s!) + %4!s!))) && (((Ceil((%1!s! - %4!s!) / %2!s!) * %2!s!) + %4!s!) <= %3!s!)) ? ((Ceil((%1!s! - %4!s!) / %2!s!) * %2!s!) + %4!s!) : ((Floor((%1!s! - %4!s!) / %2!s!) * %2!s!) + %4!s!)",
        targetExpression.data(),
        scaledInterval.data(),
        scaledEnd.data(),
        scaledFirst.data());
    winrt::ExpressionAnimation restingPointExpressionAnimation = compositor.CreateExpressionAnimation(expression);

    restingPointExpressionAnimation.SetScalarParameter(L"itv", static_cast<float>(m_interval));
    restingPointExpressionAnimation.SetScalarParameter(L"end", static_cast<float>(m_actualEnd));
    restingPointExpressionAnimation.SetScalarParameter(L"first", static_cast<float>(DetermineFirstRepeatedSnapPointValue()));
    return restingPointExpressionAnimation;
}

winrt::ExpressionAnimation RepeatedScrollSnapPoint::CreateConditionalExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale)
{
    /*
    target >= scaledStart &&
    target <= scaledEnd &&                                  //If we are within the start and end and...
    (((((Floor((target - scaledFirst) / scaledInterval)     //How many intervals the natural resting point has passed
     * scaledInterval) + scaledFirst)                       //The location of the regular snap point just before the natural resting point
     + scaledApplicableRange) >= target) || (               //Plus the applicable range is greater than the natural resting point or... 
    ((((Ceil((target - scaledFirst) / scaledInterval)
     * scaledInterval) + scaledFirst)                       //The location of the regular snap point just after the natural resting point
     - scaledApplicableRange) <= target)                    //Minus the applicable range is less than the natural resting point.
     && (((Ceil((target - scaledFirst) / scaledInterval)    //And the snap point after the natural resting point is less than or equal to the end value
     * scaledInterval) + scaledFirst) <= scaledEnd)))
    */

    winrt::hstring targetExpression = GetTargetExpression(target);
    winrt::hstring scaledInterval = StringUtil::FormatString(
        L"(itv * %1!s!)",
        scale.data());
    winrt::hstring scaledStart = StringUtil::FormatString(
        L"(start * %1!s!)",
        scale.data());
    winrt::hstring scaledEnd = StringUtil::FormatString(
        L"(end * %1!s!)",
        scale.data());
    winrt::hstring scaledApplicableRange = StringUtil::FormatString(
        L"(applicableRange * %1!s!)",
        scale.data());
    winrt::hstring scaledFirst = StringUtil::FormatString(
        L"(first * %1!s!)",
        scale.data());
    winrt::hstring expression = StringUtil::FormatString(
        L"%1!s! >= %3!s! && %1!s! <= %4!s! && (((((Floor((%1!s! - %6!s!) / %2!s!) * %2!s!) + %6!s!) + %5!s!) >= %1!s!) || (((((Ceil((%1!s! - %6!s!) / %2!s!) * %2!s!) + %6!s!) - %5!s!) <= %1!s!) && (((Ceil((%1!s! - %6!s!) / %2!s!) * %2!s!) + %6!s!) <= %4!s!)))",
        targetExpression.data(),
        scaledInterval.data(),
        scaledStart.data(),
        scaledEnd.data(),
        scaledApplicableRange.data(),
        scaledFirst.data());
    winrt::ExpressionAnimation conditionExpressionAnimation = compositor.CreateExpressionAnimation(expression);

    conditionExpressionAnimation.SetScalarParameter(L"itv", static_cast<float>(m_interval));
    conditionExpressionAnimation.SetScalarParameter(L"start", static_cast<float>(m_actualStart));
    conditionExpressionAnimation.SetScalarParameter(L"end", static_cast<float>(m_actualEnd));
    conditionExpressionAnimation.SetScalarParameter(L"applicableRange", static_cast<float>(m_specifiedApplicableRange));
    conditionExpressionAnimation.SetScalarParameter(L"first", static_cast<float>(DetermineFirstRepeatedSnapPointValue()));
    return conditionExpressionAnimation;
}

ScrollerSnapPointSortPredicate RepeatedScrollSnapPoint::SortPredicate()
{
    // Regular snap points should be sorted after irregular snap points, so give it a tertiary sort value of 1 (irregular snap points get 0)
    return ScrollerSnapPointSortPredicate{ m_actualStart, m_actualEnd, 1 };
}

void RepeatedScrollSnapPoint::DetermineActualApplicableZone(SnapPointBase* previousSnapPoint, SnapPointBase* nextSnapPoint)
{
    m_actualApplicableZone = std::tuple<double, double>{ DetermineMinActualApplicableZone(previousSnapPoint), DetermineMaxActualApplicableZone(nextSnapPoint) };

    // Influence() will not have thrown if either of the adjacent snap points are also regular snap points which have the same start and end, however this is not allowed.
    // We only need to check the nextSnapPoint because of the symmetry in the algorithm.
    if (nextSnapPoint && *static_cast<SnapPointBase*>(this) == (nextSnapPoint))
    {
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

double RepeatedScrollSnapPoint::DetermineFirstRepeatedSnapPointValue()
{
    MUX_ASSERT(m_offset >= m_actualStart);
    MUX_ASSERT(m_interval > 0.0);

    return m_offset - std::floor((m_offset - m_actualStart) / m_interval) * m_interval;
}

double RepeatedScrollSnapPoint::DetermineMinActualApplicableZone(SnapPointBase* previousSnapPoint)
{
    // The Influence() method of regular snap points has a check to ensure the value does not fall within its range.
    // This call will ensure that we are not in the range of the previous snap point if it is.
    if (previousSnapPoint)
    {
        previousSnapPoint->Influence(m_actualStart);
    }
    return m_actualStart;
}

double RepeatedScrollSnapPoint::DetermineMaxActualApplicableZone(SnapPointBase* nextSnapPoint)
{    
    // The Influence() method of regular snap points has a check to ensure the value does not fall within its range.
    // This call will ensure that we are not in the range of the next snap point if it is.
    if (nextSnapPoint)
    {
        nextSnapPoint->Influence(m_actualEnd);
    }
    return m_actualEnd;
}

void RepeatedScrollSnapPoint::ValidateConstructorParameters(
#ifdef ApplicableRangeType
    bool applicableRangeToo,
    double applicableRange,
#endif
    double offset,
    double interval,
    double start,
    double end)
{
    if (end <= start)
    {
        throw winrt::hresult_invalid_argument(L"'end' must be greater than 'start'.");
    }

    if (offset < start)
    {
        throw winrt::hresult_invalid_argument(L"'offset' must be greater than or equal to 'start'.");
    }

    if (offset > end)
    {
        throw winrt::hresult_invalid_argument(L"'offset' must be smaller than or equal to 'end'.");
    }

    if (interval <= 0)
    {
        throw winrt::hresult_invalid_argument(L"'interval' must be strictly positive.");
    }

#ifdef ApplicableRangeType
    if (applicableRangeToo && applicableRange <= 0)
    {
        throw winrt::hresult_invalid_argument(L"'applicableRange' must be strictly positive.");
    }
#endif
}

double RepeatedScrollSnapPoint::Influence(double edgeOfMidpoint)
{
    if (edgeOfMidpoint <= m_actualStart)
    {
        return m_actualStart;
    }
    else if (edgeOfMidpoint >= m_actualEnd)
    {
        return m_actualEnd;
    }
    else
    {
        // Snap points are not allowed within the bounds (Start thru End) of regular snap points
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
    return 0.0f;
}

void RepeatedScrollSnapPoint::Combine(winrt::SnapPointBase const& snapPoint)
{
    // Snap points are not allowed within the bounds (Start thru End) of regular snap points
    // TODO: Provide custom error message
    throw winrt::hresult_error(E_INVALIDARG);
    m_combinationCount++;
}

double RepeatedScrollSnapPoint::Evaluate(double value)
{
    if (value >= m_actualStart && value <= m_actualEnd)
    {
        double firstSnapPointValue = DetermineFirstRepeatedSnapPointValue();
        double passedSnapPoints = std::floor((value - firstSnapPointValue) / m_interval);
        double previousSnapPointValue = (passedSnapPoints * m_interval) + firstSnapPointValue;
        double nextSnapPointValue = previousSnapPointValue + m_interval;

        if ((value - previousSnapPointValue) <= (nextSnapPointValue - value))
        {
            if (previousSnapPointValue + m_specifiedApplicableRange >= value)
            {
                return previousSnapPointValue;
            }
        }
        else
        {
            if (nextSnapPointValue - m_specifiedApplicableRange <= value)
            {
                return nextSnapPointValue;
            }
        }
    }
    return value;
}

double RepeatedScrollSnapPoint::ActualStart()
{
    return m_actualStart;
}

void RepeatedScrollSnapPoint::ActualStart(double start)
{
    m_actualStart = start;
}

double RepeatedScrollSnapPoint::ActualEnd()
{
    return m_actualEnd;
}

void RepeatedScrollSnapPoint::ActualEnd(double end)
{
    m_actualEnd = end;
}

/////////////////////////////////////////////////////////////////////
/////////////////       Zoom Snap Points      ///////////////////////
/////////////////////////////////////////////////////////////////////

// Required for Modern Idl bug, should never be called.
ZoomSnapPointBase::ZoomSnapPointBase()
{
    // throw (ERROR_CALL_NOT_IMPLEMENTED);
}

/////////////////////////////////////////////////////////////////////
//////////////     Irregular Zoom Snap Points    ////////////////////
/////////////////////////////////////////////////////////////////////
CppWinRTActivatableClassWithBasicFactory(ZoomSnapPoint);

ZoomSnapPoint::ZoomSnapPoint(
    double snapPointValue)
{
    m_specifiedValue = snapPointValue;
    m_actualValue = snapPointValue;
}

#ifdef ApplicableRangeType
ZoomSnapPoint::ZoomSnapPoint(
    double snapPointValue,
    double applicableRange)
{
    if (applicableRange <= 0)
    {
        throw winrt::hresult_invalid_argument(L"'applicableRange' must be strictly positive.");
    }

    m_specifiedValue = snapPointValue;
    m_actualValue = snapPointValue;
    m_specifiedApplicableRange = applicableRange;
    m_actualApplicableZone = std::tuple<double, double>{ snapPointValue - applicableRange, snapPointValue + applicableRange };
    m_applicableRangeType = winrt::SnapPointApplicableRangeType::Optional;
}
#endif

double ZoomSnapPoint::Value()
{
    return m_specifiedValue;
}

// For zoom snap points scale == L"1.0".
winrt::ExpressionAnimation ZoomSnapPoint::CreateRestingPointExpression(
    winrt::Compositor compositor, winrt::hstring, winrt::hstring scale)
{
    winrt::ExpressionAnimation restingPointExpressionAnimation = compositor.CreateExpressionAnimation(L"snapPointValue");

    restingPointExpressionAnimation.SetScalarParameter(L"snapPointValue", static_cast<float>(m_actualValue));
    return restingPointExpressionAnimation;
}

// For zoom snap points scale == L"1.0".
winrt::ExpressionAnimation ZoomSnapPoint::CreateConditionalExpression(
    winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale)
{
    winrt::hstring targetExpression = GetTargetExpression(target);
    winrt::hstring expression = StringUtil::FormatString(
        L"%1!s! >= minApplicableValue && %1!s! <= maxApplicableValue",
        targetExpression.data());
    winrt::ExpressionAnimation conditionExpressionAnimation = compositor.CreateExpressionAnimation(expression);

    conditionExpressionAnimation.SetScalarParameter(L"minApplicableValue", static_cast<float>(std::get<0>(m_actualApplicableZone)));
    conditionExpressionAnimation.SetScalarParameter(L"maxApplicableValue", static_cast<float>(std::get<1>(m_actualApplicableZone)));
    return conditionExpressionAnimation;
}

ScrollerSnapPointSortPredicate ZoomSnapPoint::SortPredicate()
{
    // Irregular snap point should be sorted before regular snap points so it give a tertiary sort value of 0 (regular snap points get 1)
    return ScrollerSnapPointSortPredicate{ m_actualValue, m_actualValue, 0 };
}

void ZoomSnapPoint::DetermineActualApplicableZone(SnapPointBase* previousSnapPoint, SnapPointBase* nextSnapPoint)
{
    m_actualApplicableZone = std::tuple<double, double>{ DetermineMinActualApplicableZone(previousSnapPoint), DetermineMaxActualApplicableZone(nextSnapPoint) };
}

double ZoomSnapPoint::DetermineMinActualApplicableZone(SnapPointBase* previousSnapPoint)
{
    // If we are not passed a previousSnapPoint it means we are the first in the list, see if we expand to negative Infinity or stay put.
    if (!previousSnapPoint)
    {
#ifdef ApplicableRangeType
        if (m_applicableRangeType != winrt::SnapPointApplicableRangeType::Optional)
        {
            return -INFINITY;
        }
        else
        {
            return m_actualValue - m_specifiedApplicableRange;
        }
#else
        return -INFINITY;
#endif
    }
    // If we are passed a previousSnapPoint then we need to account for its influence on us.
    else
    {
        double previousMaxInfluence = previousSnapPoint->Influence(m_actualValue);

#ifdef ApplicableRangeType
        switch (m_applicableRangeType)
        {
        case winrt::SnapPointApplicableRangeType::Optional:
            return std::max(previousMaxInfluence, m_actualValue - m_specifiedApplicableRange);
        case winrt::SnapPointApplicableRangeType::Mandatory:
            return previousMaxInfluence;
        default:
            MUX_ASSERT(false);
            return 0.0f;
        }
#else
        return previousMaxInfluence;
#endif
    }
}

double ZoomSnapPoint::DetermineMaxActualApplicableZone(SnapPointBase* nextSnapPoint)
{
    // If we are not passed a nextSnapPoint it means we are the last in the list, see if we expand to Infinity or stay put.
    if (!nextSnapPoint)
    {
#ifdef ApplicableRangeType
        if (m_applicableRangeType != winrt::SnapPointApplicableRangeType::Optional)
        {
            return INFINITY;
        }
        else
        {
            return m_actualValue + m_specifiedApplicableRange;
        }
#else
        return INFINITY;
#endif
    }
    // If we are passed a nextSnapPoint then we need to account for its influence on us.
    else
    {
        double nextMinInfluence = nextSnapPoint->Influence(m_actualValue);

#ifdef ApplicableRangeType
        switch (m_applicableRangeType)
        {
        case winrt::SnapPointApplicableRangeType::Optional:
            return std::min(m_actualValue + m_specifiedApplicableRange, nextMinInfluence);
        case winrt::SnapPointApplicableRangeType::Mandatory:
            return nextMinInfluence;
        default:
            MUX_ASSERT(false);
            return 0.0f;
        }
#else
        return nextMinInfluence;
#endif
    }
}

double ZoomSnapPoint::Influence(double edgeOfMidpoint)
{
    double midPoint = (m_actualValue + edgeOfMidpoint) / 2;

#ifdef ApplicableRangeType
    switch (m_applicableRangeType)
    {
    case winrt::SnapPointApplicableRangeType::Optional:
        if (m_actualValue <= edgeOfMidpoint)
        {
            return std::min(m_actualValue + m_specifiedApplicableRange, midPoint);
        }
        else
        {
            return std::max(m_actualValue - m_specifiedApplicableRange, midPoint);
        }
    case winrt::SnapPointApplicableRangeType::Mandatory:
        return midPoint;
    default:
        MUX_ASSERT(false);
        return 0.0f;
    }
#else
    return midPoint;
#endif
}

void ZoomSnapPoint::Combine(winrt::SnapPointBase const& snapPoint)
{
    auto snapPointAsIrregular = snapPoint.try_as<winrt::ZoomSnapPoint>();
    if (snapPointAsIrregular)
    {
#ifdef ApplicableRangeType
        m_specifiedApplicableRange = std::max(snapPointAsIrregular.ApplicableRange(), m_specifiedApplicableRange);
#else
        MUX_ASSERT(m_specifiedApplicableRange == INFINITY);
#endif
        m_combinationCount++;
    }
    else
    {
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

double ZoomSnapPoint::Evaluate(double value)
{
    if (value >= std::get<0>(m_actualApplicableZone) && value <= std::get<1>(m_actualApplicableZone))
    {
        return m_actualValue;
    }
    return value;
}

/////////////////////////////////////////////////////////////////////
/////////////////    Regular Snap Points    /////////////////////////
/////////////////////////////////////////////////////////////////////
CppWinRTActivatableClassWithBasicFactory(RepeatedZoomSnapPoint);

RepeatedZoomSnapPoint::RepeatedZoomSnapPoint(
    double offset,
    double interval,
    double start,
    double end)
{
    ValidateConstructorParameters(
#ifdef ApplicableRangeType
        false /*applicableRangeToo*/,
        0 /*applicableRange*/,
#endif
        offset,
        interval,
        start,
        end);

    m_offset = offset;
    m_interval = interval;
    m_specifiedStart = start;
    m_actualStart = start;
    m_specifiedEnd = end;
    m_actualEnd = end;
}

#ifdef ApplicableRangeType
RepeatedZoomSnapPoint::RepeatedZoomSnapPoint(
    double offset,
    double interval,
    double start,
    double end,
    double applicableRange)
{
    ValidateConstructorParameters(
        true /*applicableRangeToo*/,
        applicableRange,
        offset,
        interval,
        start,
        end);

    m_offset = offset;
    m_interval = interval;
    m_specifiedStart = start;
    m_actualStart = start;
    m_specifiedEnd = end;
    m_actualEnd = end;
    m_specifiedApplicableRange = applicableRange;
    m_applicableRangeType = winrt::SnapPointApplicableRangeType::Optional;
}
#endif

double RepeatedZoomSnapPoint::Offset()
{
    return m_offset;
}

double RepeatedZoomSnapPoint::Interval()
{
    return m_interval;
}

double RepeatedZoomSnapPoint::Start()
{
    return m_specifiedStart;
}

double RepeatedZoomSnapPoint::End()
{
    return m_specifiedEnd;
}

// For zoom snap points scale == L"1.0".
winrt::ExpressionAnimation RepeatedZoomSnapPoint::CreateRestingPointExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale)
{
    /*
    ((Abs(target -                             //The absolute value of the natural resting point minus
    ((Floor((target - first) / interval)       //How many intervals the natural resting point has passed
     * interval) + first)                      //The location of the regular snap point just before the natural resting point
    ) >= Abs(target -                          //If it's greater than the absolute value of the natural resting point minus
    ((Ceil((target - first) / interval)
     * interval) + first)                      //The location of the regular snap point just after the natural resting point
    )) && (((Ceil((target - first) / interval)
     * interval) + first) <= end)              //And the location of the regular snap point just after the natural resting point is before the end value
    ) ? ((Ceil((target - first) / interval)    //Then return the location of the regular snap point just after the natural resting point
     * interval) + first)
     : ((Floor((target - first) / interval)    //Otherwise return the location of the regular snap point just before the natural resting point
     * interval) + first)
    */

    winrt::hstring targetExpression = GetTargetExpression(target);
    winrt::hstring expression = StringUtil::FormatString(
        L"((Abs(%1!s! - ((Floor((%1!s! - first) / itv) * itv) +first)) >= Abs(%1!s! - ((Ceil((%1!s! - first) / itv) * itv) + first))) && (((Ceil((%1!s! - first) / itv) * itv) + first) <= end)) ? ((Ceil((%1!s! - first) / itv) * itv) + first) : ((Floor((%1!s! - first) / itv) * itv) + first)",
        targetExpression.data());
    winrt::ExpressionAnimation restingPointExpressionAnimation = compositor.CreateExpressionAnimation(expression);

    restingPointExpressionAnimation.SetScalarParameter(L"itv", static_cast<float>(m_interval));
    restingPointExpressionAnimation.SetScalarParameter(L"end", static_cast<float>(m_actualEnd));
    restingPointExpressionAnimation.SetScalarParameter(L"first", static_cast<float>(DetermineFirstRepeatedSnapPointValue()));
    return restingPointExpressionAnimation;
}

// For zoom snap points scale == L"1.0".
winrt::ExpressionAnimation RepeatedZoomSnapPoint::CreateConditionalExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale)
{
    /*
    target >= start &&
    target <= end &&                            //If we are within the start and end and...
    (((((Floor((target - first) / interval)     //How many intervals the natural resting point has passed
     * interval) + first)                       //The location of the regular snap point just before the natural resting point
     + applicableRange) >= target) || (         //Plus the applicable range is greater than the natural resting point or...
    ((((Ceil((target - first) / interval)
     * interval) + first)                       //The location of the regular snap point just after the natural resting point
     - applicableRange) <= target)              //Minus the applicable range is less than the natural resting point.
     && (((Ceil((target - first) / interval)    //And the snap point after the natural resting point is less than or equal to the end value
     * interval) + first) <= end)))
    */

    winrt::hstring targetExpression = GetTargetExpression(target);
    winrt::hstring expression = StringUtil::FormatString(
        L"%1!s! >= start && %1!s! <= end && (((((Floor((%1!s! - first) / itv) * itv) + first) + applicableRange) >= %1!s!) || (((((Ceil((%1!s! - first) / itv) * itv) + first) - applicableRange) <= %1!s!) && (((Ceil((%1!s! - first) / itv) * itv) + first) <= end)))",
        targetExpression.data());
    winrt::ExpressionAnimation conditionExpressionAnimation = compositor.CreateExpressionAnimation(expression);

    conditionExpressionAnimation.SetScalarParameter(L"itv", static_cast<float>(m_interval));
    conditionExpressionAnimation.SetScalarParameter(L"start", static_cast<float>(m_actualStart));
    conditionExpressionAnimation.SetScalarParameter(L"end", static_cast<float>(m_actualEnd));
    conditionExpressionAnimation.SetScalarParameter(L"applicableRange", static_cast<float>(m_specifiedApplicableRange));
    conditionExpressionAnimation.SetScalarParameter(L"first", static_cast<float>(DetermineFirstRepeatedSnapPointValue()));
    return conditionExpressionAnimation;
}

ScrollerSnapPointSortPredicate RepeatedZoomSnapPoint::SortPredicate()
{
    // Regular snap points should be sorted after irregular snap points, so give it a tertiary sort value of 1 (irregular snap points get 0)
    return ScrollerSnapPointSortPredicate{ m_actualStart, m_actualEnd, 1 };
}

void RepeatedZoomSnapPoint::DetermineActualApplicableZone(SnapPointBase* previousSnapPoint, SnapPointBase* nextSnapPoint)
{
    m_actualApplicableZone = std::tuple<double, double>{ DetermineMinActualApplicableZone(previousSnapPoint), DetermineMaxActualApplicableZone(nextSnapPoint) };

    // Influence() will not have thrown if either of the adjacent snap points are also regular snap points which have the same start and end, however this is not allowed.
    // We only need to check the nextSnapPoint because of the symmetry in the algorithm.
    if (nextSnapPoint && *static_cast<SnapPointBase*>(this) == (nextSnapPoint))
    {
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

double RepeatedZoomSnapPoint::DetermineFirstRepeatedSnapPointValue()
{
    MUX_ASSERT(m_offset >= m_actualStart);
    MUX_ASSERT(m_interval > 0.0);

    return m_offset - std::floor((m_offset - m_actualStart) / m_interval) * m_interval;
}

double RepeatedZoomSnapPoint::DetermineMinActualApplicableZone(SnapPointBase* previousSnapPoint)
{
    // The Influence() method of regular snap points has a check to ensure the value does not fall within its range.
    // This call will ensure that we are not in the range of the previous snap point if it is.
    if (previousSnapPoint)
    {
        previousSnapPoint->Influence(m_actualStart);
    }
    return m_actualStart;
}

double RepeatedZoomSnapPoint::DetermineMaxActualApplicableZone(SnapPointBase* nextSnapPoint)
{
    // The Influence() method of regular snap points has a check to ensure the value does not fall within its range.
    // This call will ensure that we are not in the range of the next snap point if it is.
    if (nextSnapPoint)
    {
        nextSnapPoint->Influence(m_actualEnd);
    }
    return m_actualEnd;
}

void RepeatedZoomSnapPoint::ValidateConstructorParameters(
#ifdef ApplicableRangeType
    bool applicableRangeToo,
    double applicableRange,
#endif
    double offset,
    double interval,
    double start,
    double end)
{
    if (end <= start)
    {
        throw winrt::hresult_invalid_argument(L"'end' must be greater than 'start'.");
    }

    if (offset < start)
    {
        throw winrt::hresult_invalid_argument(L"'offset' must be greater than or equal to 'start'.");
    }

    if (offset > end)
    {
        throw winrt::hresult_invalid_argument(L"'offset' must be smaller than or equal to 'end'.");
    }

    if (interval <= 0)
    {
        throw winrt::hresult_invalid_argument(L"'interval' must be strictly positive.");
    }

#ifdef ApplicableRangeType
    if (applicableRangeToo && applicableRange <= 0)
    {
        throw winrt::hresult_invalid_argument(L"'applicableRange' must be strictly positive.");
    }
#endif
}

double RepeatedZoomSnapPoint::Influence(double edgeOfMidpoint)
{
    if (edgeOfMidpoint <= m_actualStart)
    {
        return m_actualStart;
    }
    else if (edgeOfMidpoint >= m_actualEnd)
    {
        return m_actualEnd;
    }
    else
    {
        // Snap points are not allowed within the bounds (Start thru End) of regular snap points
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
    return 0.0f;
}

void RepeatedZoomSnapPoint::Combine(winrt::SnapPointBase const& snapPoint)
{
    // Snap points are not allowed within the bounds (Start thru End) of regular snap points
    // TODO: Provide custom error message
    throw winrt::hresult_error(E_INVALIDARG);
    m_combinationCount++;
}

double RepeatedZoomSnapPoint::Evaluate(double value)
{
    if (value >= m_actualStart && value <= m_actualEnd)
    {
        double firstSnapPointValue = DetermineFirstRepeatedSnapPointValue();
        double passedSnapPoints = std::floor((value - firstSnapPointValue) / m_interval);
        double previousSnapPointValue = (passedSnapPoints * m_interval) + firstSnapPointValue;
        double nextSnapPointValue = previousSnapPointValue + m_interval;

        if ((value - previousSnapPointValue) <= (nextSnapPointValue - value))
        {
            if (previousSnapPointValue + m_specifiedApplicableRange >= value)
            {
                return previousSnapPointValue;
            }
        }
        else
        {
            if (nextSnapPointValue - m_specifiedApplicableRange <= value)
            {
                return nextSnapPointValue;
            }
        }
    }
    return value;
}

double RepeatedZoomSnapPoint::ActualStart()
{
    return m_actualStart;
}

void RepeatedZoomSnapPoint::ActualStart(double start)
{
    m_actualStart = start;
}

double RepeatedZoomSnapPoint::ActualEnd()
{
    return m_actualEnd;
}

void RepeatedZoomSnapPoint::ActualEnd(double end)
{
    m_actualEnd = end;
}
