// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollerSnapPoint.h"

//Required for Modern Idl bug, should never be called.
ScrollerSnapPointBase::ScrollerSnapPointBase()
{
    //throw (ERROR_CALL_NOT_IMPLEMENTED);
}

winrt::hstring ScrollerSnapPointBase::GetTargetExpression(winrt::hstring target) const
{
    return StringUtil::FormatString(L"this.Target.%1!s!", target.data());
}

bool ScrollerSnapPointBase::operator<(ScrollerSnapPointBase* snapPoint)
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

bool ScrollerSnapPointBase::operator==(ScrollerSnapPointBase* snapPoint)
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

winrt::ScrollerSnapPointAlignment ScrollerSnapPointBase::Alignment()
{
    return m_alignment;
}

void ScrollerSnapPointBase::Alignment(winrt::ScrollerSnapPointAlignment alignment)
{
    m_alignment = alignment;
}

double ScrollerSnapPointBase::SpecifiedApplicableRange()
{
    return m_specifiedApplicableRange;
}

winrt::ScrollerSnapPointApplicableRangeType ScrollerSnapPointBase::ApplicableRangeType()
{
    return m_applicableRangeType;
}

int ScrollerSnapPointBase::CombinationCount()
{
    return m_combinationCount;
}


#ifdef _DEBUG
winrt::Color ScrollerSnapPointBase::VisualizationColor()
{
    return m_visualizationColor;
}

void ScrollerSnapPointBase::VisualizationColor(winrt::Color color)
{       
    m_visualizationColor = color;
}
#endif // _DEBUG

std::tuple<double, double> ScrollerSnapPointBase::ActualApplicableZone()
{
    return m_actualApplicableZone;
}

void ScrollerSnapPointBase::ActualApplicableZone(std::tuple<double, double> zone)
{
    m_actualApplicableZone = zone;
}

/////////////////////////////////////////////////////////////////////
/////////////////    Irregular Snap Points    ///////////////////////
/////////////////////////////////////////////////////////////////////
CppWinRTActivatableClassWithBasicFactory(ScrollerSnapPointIrregular);

ScrollerSnapPointIrregular::ScrollerSnapPointIrregular(
    double snapPointValue,
    winrt::ScrollerSnapPointAlignment alignment)
{
    m_specifiedValue = snapPointValue;
    m_actualValue = snapPointValue;
    m_alignment = alignment;
}

ScrollerSnapPointIrregular::ScrollerSnapPointIrregular(
    double snapPointValue,
    double applicableRange,
    winrt::ScrollerSnapPointAlignment alignment, 
    winrt::ScrollerSnapPointApplicableRangeType applicableRangeType)
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
    m_applicableRangeType = applicableRangeType;
}

double ScrollerSnapPointIrregular::Value()
{
    return m_specifiedValue;
}

winrt::ExpressionAnimation ScrollerSnapPointIrregular::CreateRestingPointExpression(winrt::Compositor compositor, winrt::hstring, winrt::hstring scale)
{
    winrt::hstring expression = StringUtil::FormatString(L"snapPointValue * %1!s!", scale.data());
    winrt::ExpressionAnimation restingPointExpressionAnimation = compositor.CreateExpressionAnimation(expression);

    restingPointExpressionAnimation.SetScalarParameter(L"snapPointValue", static_cast<float>(m_actualValue));
    return restingPointExpressionAnimation;
}

winrt::ExpressionAnimation ScrollerSnapPointIrregular::CreateConditionalExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale)
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

ScrollerSnapPointSortPredicate ScrollerSnapPointIrregular::SortPredicate()
{
    //Irregular snap point should be sorted before regular snap points so it give a tertiary sort value of 0 (regular snap points get 1)
    return ScrollerSnapPointSortPredicate{ m_actualValue, m_actualValue, 0 };
}

void ScrollerSnapPointIrregular::DetermineActualApplicableZone(ScrollerSnapPointBase* previousSnapPoint, ScrollerSnapPointBase* nextSnapPoint)
{
    m_actualApplicableZone = std::tuple<double, double>{ DetermineMinActualApplicableZone(previousSnapPoint), DetermineMaxActualApplicableZone(nextSnapPoint) };
}

double ScrollerSnapPointIrregular::DetermineMinActualApplicableZone(ScrollerSnapPointBase* previousSnapPoint)
{
    //If we are not passed a previousSnapPoint it means we are the first in the list, see if we expand to negative Infinity or stay put.
    if (!previousSnapPoint)
    {
        if (m_applicableRangeType != winrt::ScrollerSnapPointApplicableRangeType::Optional)
        {
            return -INFINITY;
        }
        else
        {
            return m_actualValue - m_specifiedApplicableRange;
        }
    }
    //If we are passed a previousSnapPoint then we need to account for its influence on us.
    else
    {
        double previousMaxInfluence = previousSnapPoint->Influence(m_actualValue);
        switch (m_applicableRangeType)
        {
        case winrt::ScrollerSnapPointApplicableRangeType::Optional:
            return std::max(previousMaxInfluence, m_actualValue - m_specifiedApplicableRange);
        case winrt::ScrollerSnapPointApplicableRangeType::Mandatory:
            return previousMaxInfluence;
        case winrt::ScrollerSnapPointApplicableRangeType::MandatorySingle:
            throw winrt::hresult_error(E_NOTIMPL);
        default:
            assert(false);
        }
    }
    return 0.0f;
}

double ScrollerSnapPointIrregular::DetermineMaxActualApplicableZone(ScrollerSnapPointBase* nextSnapPoint)
{
    //If we are not passed a nextSnapPoint it means we are the last in the list, see if we expand to Infinity or stay put.
    if (!nextSnapPoint)
    {
        if (m_applicableRangeType != winrt::ScrollerSnapPointApplicableRangeType::Optional)
        {
            return INFINITY;
        }
        else
        {
            return m_actualValue + m_specifiedApplicableRange;
        }
    }
    //If we are passed a nextSnapPoint then we need to account for its influence on us.
    else
    {
        double nextMinInfluence = nextSnapPoint->Influence(m_actualValue);
        switch (m_applicableRangeType)
        {
        case winrt::ScrollerSnapPointApplicableRangeType::Optional:
            return std::min(m_actualValue + m_specifiedApplicableRange, nextMinInfluence);
        case winrt::ScrollerSnapPointApplicableRangeType::Mandatory:
            return nextMinInfluence;
        case winrt::ScrollerSnapPointApplicableRangeType::MandatorySingle:
            throw winrt::hresult_error(E_NOTIMPL);
        default:
            assert(false);
        }
    }
    return 0.0f;
}

double ScrollerSnapPointIrregular::Influence(double edgeOfMidpoint)
{
    double midPoint = (m_actualValue + edgeOfMidpoint) / 2;
    switch (m_applicableRangeType)
    {
    case winrt::ScrollerSnapPointApplicableRangeType::Optional:
        if (m_actualValue <= edgeOfMidpoint)
        {
            return std::min(m_actualValue + m_specifiedApplicableRange, midPoint);
        }
        else
        {
            return std::max(m_actualValue - m_specifiedApplicableRange, midPoint);
        }
    case winrt::ScrollerSnapPointApplicableRangeType::Mandatory:
        return midPoint;
    case winrt::ScrollerSnapPointApplicableRangeType::MandatorySingle:
        throw winrt::hresult_error(E_NOTIMPL);
    default:
        assert(false);
    }
    return 0.0f;
}

void ScrollerSnapPointIrregular::Combine(winrt::ScrollerSnapPointBase snapPoint)
{
    auto snapPointAsIrregular = snapPoint.try_as<winrt::ScrollerSnapPointIrregular>();
    if (snapPointAsIrregular)
    {
        m_specifiedApplicableRange = std::max(snapPointAsIrregular.SpecifiedApplicableRange(), m_specifiedApplicableRange);
        m_combinationCount++;
    }
    else
    {
        //TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

double ScrollerSnapPointIrregular::Evaluate(double value)
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
CppWinRTActivatableClassWithBasicFactory(ScrollerSnapPointRegular);

ScrollerSnapPointRegular::ScrollerSnapPointRegular(
    double offset,
    double interval,
    double start,
    double end,
    winrt::ScrollerSnapPointAlignment alignment)
{
    ValidateConstructorParameters(
        offset,
        interval,
        start,
        end,
        0 /*applicableRange*/,
        false /*applicableRangeTypeToo*/);

    m_offset = offset;
    m_interval = interval;
    m_specifiedStart = start;
    m_actualStart = start;
    m_specifiedEnd = end;
    m_actualEnd = end;
}

ScrollerSnapPointRegular::ScrollerSnapPointRegular(
    double offset,
    double interval,
    double start,
    double end,
    double applicableRange,
    winrt::ScrollerSnapPointAlignment alignment,
    winrt::ScrollerSnapPointApplicableRangeType applicableRangeType)
{
    ValidateConstructorParameters(
        offset,
        interval,
        start,
        end,
        applicableRange,
        true /*applicableRangeTypeToo*/);
    
    m_offset = offset;
    m_interval = interval;
    m_specifiedStart = start;
    m_actualStart = start;
    m_specifiedEnd = end;
    m_actualEnd = end;
    m_specifiedApplicableRange = applicableRange;
    m_applicableRangeType = applicableRangeType;
}

double ScrollerSnapPointRegular::Offset()
{
    return m_offset;
}

double ScrollerSnapPointRegular::Interval()
{
    return m_interval;
}

double ScrollerSnapPointRegular::Start()
{
    return m_specifiedStart;
}

double ScrollerSnapPointRegular::End()
{
    return m_specifiedEnd;
}

winrt::ExpressionAnimation ScrollerSnapPointRegular::CreateRestingPointExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale)
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
    restingPointExpressionAnimation.SetScalarParameter(L"first", static_cast<float>(DetermineFirstRegularSnapPointValue()));
    return restingPointExpressionAnimation;
}

winrt::ExpressionAnimation ScrollerSnapPointRegular::CreateConditionalExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale)
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
    conditionExpressionAnimation.SetScalarParameter(L"first", static_cast<float>(DetermineFirstRegularSnapPointValue()));
    return conditionExpressionAnimation;
}

ScrollerSnapPointSortPredicate ScrollerSnapPointRegular::SortPredicate()
{
    //Regular snap points should be sorted after irregular snap points, so give it a tertiary sort value of 1 (irregular snap points get 0)
    return ScrollerSnapPointSortPredicate{ m_actualStart, m_actualEnd, 1 };
}

void ScrollerSnapPointRegular::DetermineActualApplicableZone(ScrollerSnapPointBase* previousSnapPoint, ScrollerSnapPointBase* nextSnapPoint)
{
    m_actualApplicableZone = std::tuple<double, double>{ DetermineMinActualApplicableZone(previousSnapPoint), DetermineMaxActualApplicableZone(nextSnapPoint) };

    //Influence() will not have thrown if either of the adjacent snap points are also regular snap points which have the same start and end, however this is not allowed.
    //We only need to check the nextSnapPoint because of the symmetry in the algorithm.
    if (nextSnapPoint && *static_cast<ScrollerSnapPointBase*>(this) == (nextSnapPoint))
    {
        //TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

double ScrollerSnapPointRegular::DetermineFirstRegularSnapPointValue()
{
    MUX_ASSERT(m_offset >= m_actualStart);
    MUX_ASSERT(m_interval > 0.0);

    return m_offset - std::floor((m_offset - m_actualStart) / m_interval) * m_interval;
}

double ScrollerSnapPointRegular::DetermineMinActualApplicableZone(ScrollerSnapPointBase* previousSnapPoint)
{
    //The Influence() method of regular snap points has a check to ensure the value does not fall within its range.
    //This call will ensure that we are not in the range of the previous snap point if it is.
    if (previousSnapPoint)
    {
        previousSnapPoint->Influence(m_actualStart);
    }
    return m_actualStart;
}

double ScrollerSnapPointRegular::DetermineMaxActualApplicableZone(ScrollerSnapPointBase* nextSnapPoint)
{    
    //The Influence() method of regular snap points has a check to ensure the value does not fall within its range.
    //This call will ensure that we are not in the range of the next snap point if it is.
    if (nextSnapPoint)
    {
        nextSnapPoint->Influence(m_actualEnd);
    }
    return m_actualEnd;
}

void ScrollerSnapPointRegular::ValidateConstructorParameters(
    double offset,
    double interval,
    double start,
    double end,
    double applicableRange,
    bool applicableRangeTypeToo)
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

    if (applicableRangeTypeToo && applicableRange <= 0)
    {
        throw winrt::hresult_invalid_argument(L"'applicableRange' must be strictly positive.");
    }
}

double ScrollerSnapPointRegular::Influence(double edgeOfMidpoint)
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
        //Snap points are not allowed within the bounds (Start thru End) of regular snap points
        //TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
    return 0.0f;
}

void ScrollerSnapPointRegular::Combine(winrt::ScrollerSnapPointBase snapPoint)
{
    //Snap points are not allowed within the bounds (Start thru End) of regular snap points
    //TODO: Provide custom error message
    throw winrt::hresult_error(E_INVALIDARG);
    m_combinationCount++;
}

double ScrollerSnapPointRegular::Evaluate(double value)
{
    if (value >= m_actualStart && value <= m_actualEnd)
    {
        double firstSnapPointValue = DetermineFirstRegularSnapPointValue();
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

double ScrollerSnapPointRegular::ActualStart()
{
    return m_actualStart;
}

void ScrollerSnapPointRegular::ActualStart(double start)
{
    m_actualStart = start;
}

double ScrollerSnapPointRegular::ActualEnd()
{
    return m_actualEnd;
}

void ScrollerSnapPointRegular::ActualEnd(double end)
{
    m_actualEnd = end;
}