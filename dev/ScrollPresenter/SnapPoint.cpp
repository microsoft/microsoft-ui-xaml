// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "SnapPoint.h"

// Required for Modern Idl bug, should never be called.
SnapPointBase::SnapPointBase()
{
    // throw (ERROR_CALL_NOT_IMPLEMENTED);
}

winrt::hstring SnapPointBase::GetTargetExpression(winrt::hstring const& target) const
{
    return StringUtil::FormatString(L"this.Target.%1!s!", target.data());
}

winrt::hstring SnapPointBase::GetIsInertiaFromImpulseExpression(winrt::hstring const& target) const
{
    // Returns 'T.IsInertiaFromImpulse' or 'this.Target.IsInertiaFromImpulse' starting with RS5, and 'iIFI' prior to RS5.
    return SharedHelpers::IsRS5OrHigher() ? StringUtil::FormatString(L"%1!s!.IsInertiaFromImpulse", target.data()) : s_isInertiaFromImpulse.data();
}

bool SnapPointBase::operator<(const SnapPointBase* snapPoint)
{
    const SnapPointSortPredicate mySortPredicate = SortPredicate();
    const SnapPointSortPredicate theirSortPredicate = snapPoint->SortPredicate();
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

bool SnapPointBase::operator==(const SnapPointBase* snapPoint)
{
    const SnapPointSortPredicate mySortPredicate = SortPredicate();
    const SnapPointSortPredicate theirSortPredicate = snapPoint->SortPredicate();
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

// Returns True if this snap point snaps around the provided value.
bool SnapPointBase::SnapsAt(
    std::tuple<double, double> actualApplicableZone,
    double value) const
{
    if (std::get<0>(actualApplicableZone) <= value &&
        std::get<1>(actualApplicableZone) >= value)
    {
        const double snappedValue = Evaluate(actualApplicableZone, static_cast<float>(value));

        return std::abs(value - snappedValue) < s_equalityEpsilon;
    }

    return false;
}

// Updates the s_isInertiaFromImpulse boolean parameter with the provided isInertiaFromImpulse value.
void SnapPointBase::UpdateExpressionAnimationForImpulse(
    winrt::ExpressionAnimation const& expressionAnimation,
    bool isInertiaFromImpulse) const
{
    if (!SharedHelpers::IsRS5OrHigher())
    {
        SetBooleanParameter(expressionAnimation, s_isInertiaFromImpulse, isInertiaFromImpulse);
    }
}

void SnapPointBase::SetBooleanParameter(
    winrt::ExpressionAnimation const& expressionAnimation,
    wstring_view const& booleanName,
    bool booleanValue) const
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, booleanName, booleanValue);

    expressionAnimation.SetBooleanParameter(booleanName, booleanValue);
}

void SnapPointBase::SetScalarParameter(
    winrt::ExpressionAnimation const& expressionAnimation,
    wstring_view const& scalarName,
    float scalarValue) const
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_FLT, METH_NAME, this, scalarName, scalarValue);

    expressionAnimation.SetScalarParameter(scalarName, scalarValue);
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

// Returns True when this snap point is sensitive to the viewport size and is interested in future updates.
bool ScrollSnapPointBase::OnUpdateViewport(double newViewport)
{
    switch (m_alignment)
    {
    case winrt::ScrollSnapPointsAlignment::Near:
        MUX_ASSERT(m_alignmentAdjustment == 0.0);
        return false;
    case winrt::ScrollSnapPointsAlignment::Center:
        m_alignmentAdjustment = -newViewport / 2.0;
        break;
    case winrt::ScrollSnapPointsAlignment::Far:
        m_alignmentAdjustment = -newViewport;
        break;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////
//////////////    Irregular Scroll Snap Points   ////////////////////
/////////////////////////////////////////////////////////////////////
#include "ScrollSnapPoint.properties.cpp"

ScrollSnapPoint::ScrollSnapPoint(
    double snapPointValue,
    winrt::ScrollSnapPointsAlignment alignment)
{
    m_value = snapPointValue;
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

    m_value = snapPointValue;
    m_alignment = alignment;
    m_specifiedApplicableRange = applicableRange;
    m_actualApplicableZone = std::tuple<double, double>{ snapPointValue - applicableRange, snapPointValue + applicableRange};
    m_applicableRangeType = winrt::SnapPointApplicableRangeType::Optional;
}
#endif

double ScrollSnapPoint::Value()
{
    return m_value;
}

winrt::ExpressionAnimation ScrollSnapPoint::CreateRestingPointExpression(
    double ignoredValue,
    std::tuple<double, double> actualImpulseApplicableZone,
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse)
{
    winrt::hstring expression = StringUtil::FormatString(L"%1!s!*%2!s!", s_snapPointValue.data(), scale.data());

    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, expression.c_str());

    auto restingPointExpressionAnimation = interactionTracker.Compositor().CreateExpressionAnimation(expression);

    SetScalarParameter(restingPointExpressionAnimation, s_snapPointValue, static_cast<float>(ActualValue()));

    return restingPointExpressionAnimation;
}

winrt::ExpressionAnimation ScrollSnapPoint::CreateConditionalExpression(
    std::tuple<double, double> actualApplicableZone,
    std::tuple<double, double> actualImpulseApplicableZone,
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse)
{
    const wstring_view scaledValue = L"(%1!s!*%2!s!)";
    winrt::hstring isInertiaFromImpulseExpression = GetIsInertiaFromImpulseExpression(L"this.Target");
    winrt::hstring targetExpression = GetTargetExpression(target);
    winrt::hstring scaledMinApplicableRange = StringUtil::FormatString(
        scaledValue.data(),
        s_minApplicableValue.data(),
        scale.data());
    winrt::hstring scaledMaxApplicableRange = StringUtil::FormatString(
        scaledValue.data(),
        s_maxApplicableValue.data(),
        scale.data());
    winrt::hstring scaledMinImpulseApplicableRange = StringUtil::FormatString(
        scaledValue.data(),
        s_minImpulseApplicableValue.data(),
        scale.data());
    winrt::hstring scaledMaxImpulseApplicableRange = StringUtil::FormatString(
        scaledValue.data(),
        s_maxImpulseApplicableValue.data(),
        scale.data());
    winrt::hstring expression = StringUtil::FormatString(
        L"%1!s!?(%2!s!>=%5!s!&&%2!s!<=%6!s!):(%2!s!>=%3!s!&&%2!s!<= %4!s!)",
        isInertiaFromImpulseExpression.data(),
        targetExpression.data(),
        scaledMinApplicableRange.data(),
        scaledMaxApplicableRange.data(),
        scaledMinImpulseApplicableRange.data(),
        scaledMaxImpulseApplicableRange.data());

    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, expression.c_str());

    auto conditionExpressionAnimation = interactionTracker.Compositor().CreateExpressionAnimation(expression);

    SetScalarParameter(conditionExpressionAnimation, s_minApplicableValue, static_cast<float>(std::get<0>(actualApplicableZone)));
    SetScalarParameter(conditionExpressionAnimation, s_maxApplicableValue, static_cast<float>(std::get<1>(actualApplicableZone)));

    UpdateConditionalExpressionAnimationForImpulse(
        conditionExpressionAnimation,
        actualImpulseApplicableZone);

    UpdateExpressionAnimationForImpulse(
        conditionExpressionAnimation,
        isInertiaFromImpulse);

    return conditionExpressionAnimation;
}

void ScrollSnapPoint::UpdateConditionalExpressionAnimationForImpulse(
    winrt::ExpressionAnimation const& conditionExpressionAnimation,
    std::tuple<double, double> actualImpulseApplicableZone) const
{
    SetScalarParameter(conditionExpressionAnimation, s_minImpulseApplicableValue, static_cast<float>(std::get<0>(actualImpulseApplicableZone)));
    SetScalarParameter(conditionExpressionAnimation, s_maxImpulseApplicableValue, static_cast<float>(std::get<1>(actualImpulseApplicableZone)));
}

void ScrollSnapPoint::UpdateRestingPointExpressionAnimationForImpulse(
    winrt::ExpressionAnimation const& restingValueExpressionAnimation,
    double ignoredValue,
    std::tuple<double, double> actualImpulseApplicableZone) const
{
    // An irregular snap point like ScrollSnapPoint is either completely ignored in impulse mode or not ignored at all, unlike repeated snap points
    // which can be partially ignored. Its conditional expression depends on the impulse mode, whereas its resting point expression does not,
    // thus this method has no job to do.
}

SnapPointSortPredicate ScrollSnapPoint::SortPredicate() const
{
    const double actualValue = ActualValue();

    // Irregular snap point should be sorted before repeated snap points so it gives a tertiary sort value of 0 (repeated snap points get 1)
    return SnapPointSortPredicate{ actualValue, actualValue, 0 };
}

std::tuple<double, double> ScrollSnapPoint::DetermineActualApplicableZone(
    const SnapPointBase* previousSnapPoint,
    const SnapPointBase* nextSnapPoint)
{
    return std::make_tuple(
        DetermineMinActualApplicableZone(previousSnapPoint),
        DetermineMaxActualApplicableZone(nextSnapPoint));
}

std::tuple<double, double> ScrollSnapPoint::DetermineActualImpulseApplicableZone(
    const SnapPointBase* previousSnapPoint,
    const SnapPointBase* nextSnapPoint,
    double currentIgnoredValue,
    double previousIgnoredValue,
    double nextIgnoredValue)
{
    return std::make_tuple(
        DetermineMinActualImpulseApplicableZone(
            previousSnapPoint,
            currentIgnoredValue,
            previousIgnoredValue),
        DetermineMaxActualImpulseApplicableZone(
            nextSnapPoint,
            currentIgnoredValue,
            nextIgnoredValue));
}

double ScrollSnapPoint::ActualValue() const
{
    return m_value + m_alignmentAdjustment;
}

double ScrollSnapPoint::DetermineMinActualApplicableZone(
    const SnapPointBase* previousSnapPoint) const
{
    // If we are not passed a previousSnapPoint it means we are the first in the list, see if we expand to negative Infinity or stay put.
    if (!previousSnapPoint)
    {
#ifdef ApplicableRangeType
        if (applicableRangeType != winrt::SnapPointApplicableRangeType::Optional)
        {
            return -INFINITY;
        }
        else
        {
            return ActualValue() - m_specifiedApplicableRange;
        }
#else
        return -INFINITY;
#endif
    }
    // If we are passed a previousSnapPoint then we need to account for its influence on us.
    else
    {
        const double previousMaxInfluence = previousSnapPoint->Influence(ActualValue());

#ifdef ApplicableRangeType
        switch (m_applicableRangeType)
        {
        case winrt::SnapPointApplicableRangeType::Optional:
            return std::max(previousMaxInfluence, ActualValue() - m_specifiedApplicableRange);
        case winrt::SnapPointApplicableRangeType::Mandatory:
            return previousMaxInfluence;
        default:
            MUX_ASSERT(false);
            return 0.0;
        }
#else
        return previousMaxInfluence;
#endif
    }
}

double ScrollSnapPoint::DetermineMinActualImpulseApplicableZone(
    const SnapPointBase* previousSnapPoint,
    double currentIgnoredValue,
    double previousIgnoredValue) const
{
    if (!previousSnapPoint)
    {
        return -INFINITY;
    }
    else
    {
        const double previousMaxInfluence = previousSnapPoint->ImpulseInfluence(ActualValue(), previousIgnoredValue);

        if (isnan(currentIgnoredValue))
        {
            return previousMaxInfluence;
        }
        else
        {
            return std::max(previousMaxInfluence, ActualValue());
        }
    }
}

double ScrollSnapPoint::DetermineMaxActualApplicableZone(
    const SnapPointBase* nextSnapPoint) const
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
            return ActualValue() + m_specifiedApplicableRange;
        }
#else
        return INFINITY;
#endif
    }
    // If we are passed a nextSnapPoint then we need to account for its influence on us.
    else
    {
        const double nextMinInfluence = nextSnapPoint->Influence(ActualValue());

#ifdef ApplicableRangeType
        switch (m_applicableRangeType)
        {
        case winrt::SnapPointApplicableRangeType::Optional:
            return std::min(ActualValue() + m_specifiedApplicableRange, nextMinInfluence);
        case winrt::SnapPointApplicableRangeType::Mandatory:
            return nextMinInfluence;
        default:
            MUX_ASSERT(false);
            return 0.0;
        }
#else
        return nextMinInfluence;
#endif
    }
}

double ScrollSnapPoint::DetermineMaxActualImpulseApplicableZone(
    const SnapPointBase* nextSnapPoint,
    double currentIgnoredValue,
    double nextIgnoredValue) const
{
    if (!nextSnapPoint)
    {
        return INFINITY;
    }
    else
    {
        const double nextMinInfluence = nextSnapPoint->ImpulseInfluence(ActualValue(), nextIgnoredValue);

        if (isnan(currentIgnoredValue))
        {
            return nextMinInfluence;
        }
        else
        {
            return std::min(ActualValue(), nextMinInfluence);
        }
    }
}

double ScrollSnapPoint::Influence(double edgeOfMidpoint) const
{
    const double actualValue = ActualValue();
    const double midPoint = (actualValue + edgeOfMidpoint) / 2;

#ifdef ApplicableRangeType
    switch (m_applicableRangeType)
    {
    case winrt::SnapPointApplicableRangeType::Optional:
        if (actualValue <= edgeOfMidpoint)
        {
            return std::min(actualValue + m_specifiedApplicableRange, midPoint);
        }
        else
        {
            return std::max(actualValue - m_specifiedApplicableRange, midPoint);
        }
    case winrt::SnapPointApplicableRangeType::Mandatory:
        return midPoint;
    default:
        MUX_ASSERT(false);
        return 0.0;
    }
#else
    return midPoint;
#endif
}

double ScrollSnapPoint::ImpulseInfluence(double edgeOfMidpoint, double ignoredValue) const
{
    const double actualValue = ActualValue();
    const double midPoint = (actualValue + edgeOfMidpoint) / 2.0;

    if (isnan(ignoredValue))
    {
        return midPoint;
    }
    else
    {
        if (actualValue <= edgeOfMidpoint)
        {
            return std::min(actualValue, midPoint);
        }
        else
        {
            return std::max(actualValue, midPoint);
        }
    }
}

void ScrollSnapPoint::Combine(
    int& combinationCount,
    winrt::SnapPointBase const& snapPoint) const
{
    auto snapPointAsIrregular = snapPoint.try_as<winrt::ScrollSnapPoint>();
    if (snapPointAsIrregular)
    {
#ifdef ApplicableRangeType
        //TODO: The m_specifiedApplicableRange field is never expected to change after creation. A correction will be needed here.
        m_specifiedApplicableRange = std::max(snapPointAsIrregular.ApplicableRange(), m_specifiedApplicableRange);
#else
        MUX_ASSERT(m_specifiedApplicableRange == INFINITY);
#endif
        combinationCount++;
    }
    else
    {
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

int ScrollSnapPoint::SnapCount() const
{
    return 1;
}

double ScrollSnapPoint::Evaluate(
    std::tuple<double, double> actualApplicableZone,
    double value) const
{
    if (value >= std::get<0>(actualApplicableZone) && value <= std::get<1>(actualApplicableZone))
    {
        return ActualValue();
    }
    return value;
}

/////////////////////////////////////////////////////////////////////
/////////////////    Repeated Snap Points    ////////////////////////
/////////////////////////////////////////////////////////////////////
#include "RepeatedScrollSnapPoint.properties.cpp"

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
    m_start = start;
    m_end = end;
    m_alignment = alignment;
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
    m_start = start;
    m_end = end;
    m_specifiedApplicableRange = applicableRange;
    m_applicableRangeType = winrt::SnapPointApplicableRangeType::Optional;
    m_alignment = alignment;
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
    return m_start;
}

double RepeatedScrollSnapPoint::End()
{
    return m_end;
}

winrt::ExpressionAnimation RepeatedScrollSnapPoint::CreateRestingPointExpression(
    double ignoredValue,
    std::tuple<double, double> actualImpulseApplicableZone,
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse)
{
    /*
    fracTarget = (target / scale - first) / interval       // Unsnapped value in fractional unscaled intervals from first snapping value
    prevSnap = ((Floor(fracTarget) * interval) + first)    // First unscaled snapped value before unsnapped value
    nextSnap = ((Ceil(fracTarget) * interval) + first)     // First unscaled snapped value after unsnapped value
    effectiveEnd = (IsInertiaFromImpulse ? impEnd : end)   // Regular or impulse upper bound of unscaled applicable zone
     
    Expression:
     ((Abs(target / scale - prevSnap) >= Abs(target / scale - nextSnap)) && (nextSnap <= effectiveEnd))
     ?
     // nextSnap value is closer to unsnapped value and within applicable zone.
     (
      IsInertiaFromImpulse
      ?
      // Impulse mode.
      (
       nextSnap == impIgn
       ?
       (
        // Next snapped value is ignored. Pick the previous snapped value if any, else the ignored value.
        (impIgn == first ? first * scale : (impIgn - interval) * scale)
       )
       :
       // Pick next snapped value.
       nextSnap * scale
      )
      :
      // Regular mode. Pick next snapped value.
      nextSnap * scale
     )
     :
     // prevSnap value is closer to unsnapped value.
     (
      IsInertiaFromImpulse
      ?
      // Impulse mode.
      (
       prevSnap == impIgn
       ?
       // Previous snapped value is ignored. Pick the next snapped value if any, else the ignored value.
       (impIgn + interval <= effectiveEnd ? (impIgn + interval) * scale : impIgn * scale)
       :
       (
        prevSnap < first i.e. fracTarget < -0.5
        ?
        // Pick next snapped value as previous snapped value is outside applicable zone.
        nextSnap * scale
        :
        // Pick previous snapped value as it is within applicable zone.
        prevSnap * scale
       )
      )
      :
      // Regular mode.
      (
       prevSnap < first i.e. fracTarget < -0.5
       ?
       // Pick next snapped value as previous snapped value is outside applicable zone.
       nextSnap * scale
       :
       // Pick previous snapped value as it is within applicable zone.
       prevSnap * scale
      )
     )
    */

    winrt::hstring isInertiaFromImpulseExpression = GetIsInertiaFromImpulseExpression(s_interactionTracker.data());
    winrt::hstring expression = StringUtil::FormatString(
        L"((Abs(T.%2!s!/T.Scale-(Floor((T.%2!s!/T.Scale-P)/V)*V+P))>=Abs(T.%2!s!/T.Scale-(Ceil((T.%2!s!/T.Scale-P)/V)*V+P)))&&((Ceil((T.%2!s!/T.Scale-P)/V)*V+P)<=(%1!s!?iE:E)))?(%1!s!?((Ceil((T.%2!s!/T.Scale-P)/V)*V+P)==M?((M==P?P*T.Scale:(M-V)*T.Scale)):(Ceil((T.%2!s!/T.Scale-P)/V)*V+P)*T.Scale):(Ceil((T.%2!s!/T.Scale-P)/V)*V+P)*T.Scale):(%1!s!?((Floor((T.%2!s!/T.Scale-P)/V)*V+P)==M?(M+V<=(%1!s!?iE:E)?(M+V)*T.Scale:M*T.Scale):(T.%2!s!/T.Scale<P-0.5*V?(Ceil((T.%2!s!/T.Scale-P)/V)*V+P)*T.Scale:(Floor((T.%2!s!/T.Scale-P)/V)*V+P)*T.Scale)):(T.%2!s!/T.Scale<P-0.5*V?(Ceil((T.%2!s!/T.Scale-P)/V)*V+P)*T.Scale:(Floor((T.%2!s!/T.Scale-P)/V)*V+P)*T.Scale))",
        isInertiaFromImpulseExpression.data(),
        target.data());

    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, expression.c_str());

    auto restingPointExpressionAnimation = interactionTracker.Compositor().CreateExpressionAnimation(expression);

    SetScalarParameter(restingPointExpressionAnimation, s_interval, static_cast<float>(m_interval));
    SetScalarParameter(restingPointExpressionAnimation, s_end, static_cast<float>(ActualEnd()));
    SetScalarParameter(restingPointExpressionAnimation, s_first, static_cast<float>(DetermineFirstRepeatedSnapPointValue()));
    restingPointExpressionAnimation.SetReferenceParameter(s_interactionTracker, interactionTracker);

    UpdateRestingPointExpressionAnimationForImpulse(
        restingPointExpressionAnimation,
        ignoredValue,
        actualImpulseApplicableZone);

    UpdateExpressionAnimationForImpulse(
        restingPointExpressionAnimation,
        isInertiaFromImpulse);

    return restingPointExpressionAnimation;
}

winrt::ExpressionAnimation RepeatedScrollSnapPoint::CreateConditionalExpression(
    std::tuple<double, double> actualApplicableZone,
    std::tuple<double, double> actualImpulseApplicableZone,
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse)
{
    MUX_ASSERT(std::get<0>(actualApplicableZone) == ActualStart());
    MUX_ASSERT(std::get<1>(actualApplicableZone) == ActualEnd());

    /*
    fracTarget = (target / scale - first) / interval       // Unsnapped value in fractional unscaled intervals from first snapping value
    prevSnap = ((Floor(fracTarget) * interval) + first)    // First unscaled snapped value before unsnapped value
    nextSnap = ((Ceil(fracTarget) * interval) + first)     // First unscaled snapped value after unsnapped value
    effectiveEnd = (IsInertiaFromImpulse ? impEnd : end)   // Regular or impulse upper bound of unscaled applicable zone

    Expression:
    (
     (!IsInertiaFromImpulse && target / scale >= start && target / scale <= end)       // If we are within the start and end in non-impulse mode
     ||
     (IsInertiaFromImpulse && target / scale >= impStart && target / scale <= impEnd)  // or we are within the impulse start and end in impulse mode
    )
    &&                                                                                 // and...
    (                                                                                  // The location of the repeated snap point just before the natural resting point
     (prevSnap + appRange >= target / scale)                                           // Plus the applicable range is greater than the natural resting point
     ||                                                                                // or...
     (                                                                                 // The location of the repeated snap point just after the natural resting point
      (nextSnap - appRange <= target / scale) &&                                       // Minus the applicable range is less than the natural resting point.
      (nextSnap <= effectiveEnd)                                                       // And the snap point after the natural resting point is less than or equal to the effective end value
     )
    )
    */

    winrt::hstring isInertiaFromImpulseExpression = GetIsInertiaFromImpulseExpression(s_interactionTracker.data());
    winrt::hstring expression = StringUtil::FormatString(
        L"((!%1!s!&&T.%2!s!/T.Scale>=S&&T.%2!s!/T.Scale<=E)||(%1!s!&&T.%2!s!/T.Scale>=iS&&T.%2!s!/T.Scale<=iE))&&(((Floor((T.%2!s!/T.Scale-P)/V)*V)+P+aR>=T.%2!s!/T.Scale)||(((Ceil((T.%2!s!/T.Scale-P)/V)*V)+P-aR<=T.%2!s!/T.Scale)&&((Ceil((T.%2!s!/T.Scale-P)/V)*V)+P<=(%1!s!?iE:E))))",
        isInertiaFromImpulseExpression.data(),
        target.data());

    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, expression.c_str());

    auto conditionExpressionAnimation = interactionTracker.Compositor().CreateExpressionAnimation(expression);

    SetScalarParameter(conditionExpressionAnimation, s_interval, static_cast<float>(m_interval));
    SetScalarParameter(conditionExpressionAnimation, s_first, static_cast<float>(DetermineFirstRepeatedSnapPointValue()));
    SetScalarParameter(conditionExpressionAnimation, s_start, static_cast<float>(ActualStart()));
    SetScalarParameter(conditionExpressionAnimation, s_end, static_cast<float>(ActualEnd()));
    SetScalarParameter(conditionExpressionAnimation, s_applicableRange, static_cast<float>(m_specifiedApplicableRange));
    conditionExpressionAnimation.SetReferenceParameter(s_interactionTracker, interactionTracker);

    UpdateConditionalExpressionAnimationForImpulse(
        conditionExpressionAnimation,
        actualImpulseApplicableZone);

    UpdateExpressionAnimationForImpulse(
        conditionExpressionAnimation,
        isInertiaFromImpulse);

    return conditionExpressionAnimation;
}

void RepeatedScrollSnapPoint::UpdateConditionalExpressionAnimationForImpulse(
    winrt::ExpressionAnimation const& conditionExpressionAnimation,
    std::tuple<double, double> actualImpulseApplicableZone) const
{
    SetScalarParameter(conditionExpressionAnimation, s_impulseStart, static_cast<float>(std::get<0>(actualImpulseApplicableZone)));
    SetScalarParameter(conditionExpressionAnimation, s_impulseEnd, static_cast<float>(std::get<1>(actualImpulseApplicableZone)));
}

void RepeatedScrollSnapPoint::UpdateRestingPointExpressionAnimationForImpulse(
    winrt::ExpressionAnimation const& restingValueExpressionAnimation,
    double ignoredValue,
    std::tuple<double, double> actualImpulseApplicableZone) const
{
    SetScalarParameter(restingValueExpressionAnimation, s_impulseEnd, static_cast<float>(std::get<1>(actualImpulseApplicableZone)));
    SetScalarParameter(restingValueExpressionAnimation, s_impulseIgnoredValue, static_cast<float>(ignoredValue));
}

SnapPointSortPredicate RepeatedScrollSnapPoint::SortPredicate() const
{
    // Repeated snap points should be sorted after irregular snap points, so give it a tertiary sort value of 1 (irregular snap points get 0)
    return SnapPointSortPredicate{ ActualStart(), ActualEnd(), 1 };
}

std::tuple<double, double> RepeatedScrollSnapPoint::DetermineActualApplicableZone(
    const SnapPointBase* previousSnapPoint,
    const SnapPointBase* nextSnapPoint)
{
    std::tuple<double, double> actualApplicableZoneReturned = std::make_tuple(
        DetermineMinActualApplicableZone(previousSnapPoint),
        DetermineMaxActualApplicableZone(nextSnapPoint));

    // Influence() will not have thrown if either of the adjacent snap points are also repeated snap points which have the same start and end, however this is not allowed.
    // We only need to check the nextSnapPoint because of the symmetry in the algorithm.
    if (nextSnapPoint && *static_cast<SnapPointBase*>(this) == (nextSnapPoint))
    {
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }

    return actualApplicableZoneReturned;
}

std::tuple<double, double> RepeatedScrollSnapPoint::DetermineActualImpulseApplicableZone(
    const SnapPointBase* previousSnapPoint,
    const SnapPointBase* nextSnapPoint,
    double currentIgnoredValue,
    double previousIgnoredValue,
    double nextIgnoredValue)
{
    return std::make_tuple(
        DetermineMinActualImpulseApplicableZone(
            previousSnapPoint,
            currentIgnoredValue,
            previousIgnoredValue),
        DetermineMaxActualImpulseApplicableZone(
            nextSnapPoint,
            currentIgnoredValue,
            nextIgnoredValue));
}

double RepeatedScrollSnapPoint::ActualOffset() const
{
    return m_offset + m_alignmentAdjustment;
}

double RepeatedScrollSnapPoint::ActualStart() const
{
    return m_start + m_alignmentAdjustment;
}

double RepeatedScrollSnapPoint::ActualEnd() const
{
    return m_end + m_alignmentAdjustment;
}

double RepeatedScrollSnapPoint::DetermineFirstRepeatedSnapPointValue() const
{
    const double actualOffset = ActualOffset();
    const double actualStart = ActualStart();

    MUX_ASSERT(actualOffset >= actualStart);
    MUX_ASSERT(m_interval > 0.0);

    return actualOffset - std::floor((actualOffset - actualStart) / m_interval) * m_interval;
}

double RepeatedScrollSnapPoint::DetermineLastRepeatedSnapPointValue() const
{
    const double actualOffset = ActualOffset();
    const double actualEnd = ActualEnd();

    MUX_ASSERT(actualOffset <= m_end);
    MUX_ASSERT(m_interval > 0.0);

    return actualOffset + std::floor((actualEnd - actualOffset) / m_interval) * m_interval;
}

double RepeatedScrollSnapPoint::DetermineMinActualApplicableZone(
    const SnapPointBase* previousSnapPoint) const
{
    const double actualStart = ActualStart();

    // The Influence() method of repeated snap points has a check to ensure the value does not fall within its range.
    // This call will ensure that we are not in the range of the previous snap point if it is.
    if (previousSnapPoint)
    {
        previousSnapPoint->Influence(actualStart);
    }
    return actualStart;
}

double RepeatedScrollSnapPoint::DetermineMinActualImpulseApplicableZone(
    const SnapPointBase* previousSnapPoint,
    double currentIgnoredValue,
    double previousIgnoredValue) const
{
    if (previousSnapPoint)
    {
        if (currentIgnoredValue == DetermineFirstRepeatedSnapPointValue())
        {
            return currentIgnoredValue;
        }

        if (!isnan(previousIgnoredValue))
        {
            return previousSnapPoint->ImpulseInfluence(ActualStart(), previousIgnoredValue);
        }
    }
    return ActualStart();
}

double RepeatedScrollSnapPoint::DetermineMaxActualApplicableZone(
    const SnapPointBase* nextSnapPoint) const
{
    const double actualEnd = ActualEnd();

    // The Influence() method of repeated snap points has a check to ensure the value does not fall within its range.
    // This call will ensure that we are not in the range of the next snap point if it is.
    if (nextSnapPoint)
    {
        nextSnapPoint->Influence(actualEnd);
    }
    return actualEnd;
}

double RepeatedScrollSnapPoint::DetermineMaxActualImpulseApplicableZone(
    const SnapPointBase* nextSnapPoint,
    double currentIgnoredValue,
    double nextIgnoredValue) const
{
    if (nextSnapPoint)
    {
        if (currentIgnoredValue == DetermineLastRepeatedSnapPointValue())
        {
            return currentIgnoredValue;
        }

        if (!isnan(nextIgnoredValue))
        {
            return nextSnapPoint->ImpulseInfluence(ActualEnd(), nextIgnoredValue);
        }
    }
    return ActualEnd();
}

void RepeatedScrollSnapPoint::ValidateConstructorParameters(
#ifdef ApplicableRangeType
    bool applicableRangeToo,
    double applicableRange,
#endif
    double offset,
    double interval,
    double start,
    double end) const
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

double RepeatedScrollSnapPoint::Influence(double edgeOfMidpoint) const
{
    const double actualStart = ActualStart();
    const double actualEnd = ActualEnd();

    if (edgeOfMidpoint <= actualStart)
    {
        return actualStart;
    }
    else if (edgeOfMidpoint >= actualEnd)
    {
        return actualEnd;
    }
    else
    {
        // Snap points are not allowed within the bounds (Start thru End) of repeated snap points
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
    return 0.0;
}

double RepeatedScrollSnapPoint::ImpulseInfluence(double edgeOfMidpoint, double ignoredValue) const
{
    if (edgeOfMidpoint <= ActualStart())
    {
        if (ignoredValue == DetermineFirstRepeatedSnapPointValue())
        {
            return ignoredValue;
        }
        return ActualStart();
    }
    else if (edgeOfMidpoint >= ActualEnd())
    {
        if (ignoredValue == DetermineLastRepeatedSnapPointValue())
        {
            return ignoredValue;
        }
        return ActualEnd();
    }
    else
    {
        MUX_ASSERT(false);
        return 0.0;
    }
}

void RepeatedScrollSnapPoint::Combine(
    int& combinationCount,
    winrt::SnapPointBase const& snapPoint) const
{
    // Snap points are not allowed within the bounds (Start thru End) of repeated snap points
    // TODO: Provide custom error message
    throw winrt::hresult_error(E_INVALIDARG);
}

int RepeatedScrollSnapPoint::SnapCount() const
{
    return static_cast<int>((m_end - m_start) / m_interval);
}

double RepeatedScrollSnapPoint::Evaluate(
    std::tuple<double, double> actualApplicableZone,
    double value) const
{
    if (value >= ActualStart() && value <= ActualEnd())
    {
        const double firstSnapPointValue = DetermineFirstRepeatedSnapPointValue();
        const double passedSnapPoints = std::floor((value - firstSnapPointValue) / m_interval);
        const double previousSnapPointValue = (passedSnapPoints * m_interval) + firstSnapPointValue;
        const double nextSnapPointValue = previousSnapPointValue + m_interval;

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

/////////////////////////////////////////////////////////////////////
/////////////////       Zoom Snap Points      ///////////////////////
/////////////////////////////////////////////////////////////////////

// Required for Modern Idl bug, should never be called.
ZoomSnapPointBase::ZoomSnapPointBase()
{
    // throw (ERROR_CALL_NOT_IMPLEMENTED);
}

bool ZoomSnapPointBase::OnUpdateViewport(double newViewport)
{
    return false;
}

/////////////////////////////////////////////////////////////////////
//////////////     Irregular Zoom Snap Points    ////////////////////
/////////////////////////////////////////////////////////////////////
#include "ZoomSnapPoint.properties.cpp"

ZoomSnapPoint::ZoomSnapPoint(
    double snapPointValue)
{
    m_value = snapPointValue;
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

    m_value = snapPointValue;
    m_specifiedApplicableRange = applicableRange;
    m_actualApplicableZone = std::tuple<double, double>{ snapPointValue - applicableRange, snapPointValue + applicableRange };
    m_applicableRangeType = winrt::SnapPointApplicableRangeType::Optional;
}
#endif

double ZoomSnapPoint::Value()
{
    return m_value;
}

// For zoom snap points scale == L"1.0".
winrt::ExpressionAnimation ZoomSnapPoint::CreateRestingPointExpression(
    double ignoredValue,
    std::tuple<double, double> actualImpulseApplicableZone,
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    auto restingPointExpressionAnimation = interactionTracker.Compositor().CreateExpressionAnimation(s_snapPointValue);

    SetScalarParameter(restingPointExpressionAnimation, s_snapPointValue, static_cast<float>(m_value));

    UpdateExpressionAnimationForImpulse(
        restingPointExpressionAnimation,
        isInertiaFromImpulse);

    return restingPointExpressionAnimation;
}

// For zoom snap points scale == L"1.0".
winrt::ExpressionAnimation ZoomSnapPoint::CreateConditionalExpression(
    std::tuple<double, double> actualApplicableZone,
    std::tuple<double, double> actualImpulseApplicableZone,
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse)
{
    winrt::hstring isInertiaFromImpulseExpression = GetIsInertiaFromImpulseExpression(L"this.Target");
    winrt::hstring targetExpression = GetTargetExpression(target);
    winrt::hstring expression = StringUtil::FormatString(
        L"%1!s!?(%2!s!>=%5!s!&&%2!s!<=%6!s!):(%2!s!>=%3!s!&&%2!s!<=%4!s!)",
        isInertiaFromImpulseExpression.data(),
        targetExpression.data(),
        s_minApplicableValue.data(),
        s_maxApplicableValue.data(),
        s_minImpulseApplicableValue.data(),
        s_maxImpulseApplicableValue.data());

    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, expression.c_str());

    auto conditionExpressionAnimation = interactionTracker.Compositor().CreateExpressionAnimation(expression);

    SetScalarParameter(conditionExpressionAnimation, s_minApplicableValue, static_cast<float>(std::get<0>(actualApplicableZone)));
    SetScalarParameter(conditionExpressionAnimation, s_maxApplicableValue, static_cast<float>(std::get<1>(actualApplicableZone)));

    UpdateConditionalExpressionAnimationForImpulse(
        conditionExpressionAnimation,
        actualImpulseApplicableZone);

    UpdateExpressionAnimationForImpulse(
        conditionExpressionAnimation,
        isInertiaFromImpulse);

    return conditionExpressionAnimation;
}

void ZoomSnapPoint::UpdateConditionalExpressionAnimationForImpulse(
    winrt::ExpressionAnimation const& conditionExpressionAnimation,
    std::tuple<double, double> actualImpulseApplicableZone) const
{
    SetScalarParameter(conditionExpressionAnimation, s_minImpulseApplicableValue, static_cast<float>(std::get<0>(actualImpulseApplicableZone)));
    SetScalarParameter(conditionExpressionAnimation, s_maxImpulseApplicableValue, static_cast<float>(std::get<1>(actualImpulseApplicableZone)));
}

void ZoomSnapPoint::UpdateRestingPointExpressionAnimationForImpulse(
    winrt::ExpressionAnimation const& restingValueExpressionAnimation,
    double ignoredValue,
    std::tuple<double, double> actualImpulseApplicableZone) const
{
    // An irregular snap point like ZoomSnapPoint is either completely ignored in impulse mode or not ignored at all, unlike repeated snap points
    // which can be partially ignored. Its conditional expression depends on the impulse mode, whereas its resting point expression does not,
    // thus this method has no job to do.
}

SnapPointSortPredicate ZoomSnapPoint::SortPredicate() const
{
    // Irregular snap point should be sorted before repeated snap points so it gives a tertiary sort value of 0 (repeated snap points get 1)
    return SnapPointSortPredicate{ m_value, m_value, 0 };
}

std::tuple<double, double> ZoomSnapPoint::DetermineActualApplicableZone(
    const SnapPointBase* previousSnapPoint,
    const SnapPointBase* nextSnapPoint)
{
    return std::make_tuple(
        DetermineMinActualApplicableZone(previousSnapPoint),
        DetermineMaxActualApplicableZone(nextSnapPoint));
}

std::tuple<double, double> ZoomSnapPoint::DetermineActualImpulseApplicableZone(
    const SnapPointBase* previousSnapPoint,
    const SnapPointBase* nextSnapPoint,
    double currentIgnoredValue,
    double previousIgnoredValue,
    double nextIgnoredValue)
{
    return std::make_tuple(
        DetermineMinActualImpulseApplicableZone(
            previousSnapPoint,
            currentIgnoredValue,
            previousIgnoredValue),
        DetermineMaxActualImpulseApplicableZone(
            nextSnapPoint,
            currentIgnoredValue,
            nextIgnoredValue));
}

double ZoomSnapPoint::DetermineMinActualApplicableZone(
    const SnapPointBase* previousSnapPoint) const
{
    // If we are not passed a previousSnapPoint it means we are the first in the list, see if we expand to negative Infinity or stay put.
    if (!previousSnapPoint)
    {
#ifdef ApplicableRangeType
        if (applicableRangeType != winrt::SnapPointApplicableRangeType::Optional)
        {
            return -INFINITY;
        }
        else
        {
            return m_value - m_specifiedApplicableRange;
        }
#else
        return -INFINITY;
#endif
    }
    // If we are passed a previousSnapPoint then we need to account for its influence on us.
    else
    {
        const double previousMaxInfluence = previousSnapPoint->Influence(m_value);

#ifdef ApplicableRangeType
        switch (m_applicableRangeType)
        {
        case winrt::SnapPointApplicableRangeType::Optional:
            return std::max(previousMaxInfluence, m_value - m_specifiedApplicableRange);
        case winrt::SnapPointApplicableRangeType::Mandatory:
            return previousMaxInfluence;
        default:
            MUX_ASSERT(false);
            return 0.0;
        }
#else
        return previousMaxInfluence;
#endif
    }
}

double ZoomSnapPoint::DetermineMinActualImpulseApplicableZone(
    const SnapPointBase* previousSnapPoint,
    double currentIgnoredValue,
    double previousIgnoredValue) const
{
    if (!previousSnapPoint)
    {
        return -INFINITY;
    }
    else
    {
        const double previousMaxInfluence = previousSnapPoint->ImpulseInfluence(m_value, previousIgnoredValue);

        if (isnan(currentIgnoredValue))
        {
            return previousMaxInfluence;
        }
        else
        {
            return std::max(previousMaxInfluence, m_value);
        }
    }
}

double ZoomSnapPoint::DetermineMaxActualApplicableZone(
    const SnapPointBase* nextSnapPoint) const
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
            return m_value + m_specifiedApplicableRange;
        }
#else
        return INFINITY;
#endif
    }
    // If we are passed a nextSnapPoint then we need to account for its influence on us.
    else
    {
        const double nextMinInfluence = nextSnapPoint->Influence(m_value);

#ifdef ApplicableRangeType
        switch (m_applicableRangeType)
        {
        case winrt::SnapPointApplicableRangeType::Optional:
            return std::min(m_value + m_specifiedApplicableRange, nextMinInfluence);
        case winrt::SnapPointApplicableRangeType::Mandatory:
            return nextMinInfluence;
        default:
            MUX_ASSERT(false);
            return 0.0;
        }
#else
        return nextMinInfluence;
#endif
    }
}

double ZoomSnapPoint::DetermineMaxActualImpulseApplicableZone(
    const SnapPointBase* nextSnapPoint,
    double currentIgnoredValue,
    double nextIgnoredValue) const
{
    if (!nextSnapPoint)
    {
        return INFINITY;
    }
    else
    {
        const double nextMinInfluence = nextSnapPoint->ImpulseInfluence(m_value, nextIgnoredValue);

        if (isnan(currentIgnoredValue))
        {
            return nextMinInfluence;
        }
        else
        {
            return std::min(m_value, nextMinInfluence);
        }
    }
}

double ZoomSnapPoint::Influence(double edgeOfMidpoint) const
{
    const double midPoint = (m_value + edgeOfMidpoint) / 2;

#ifdef ApplicableRangeType
    switch (m_applicableRangeType)
    {
    case winrt::SnapPointApplicableRangeType::Optional:
        if (m_value <= edgeOfMidpoint)
        {
            return std::min(m_value + m_specifiedApplicableRange, midPoint);
        }
        else
        {
            return std::max(m_value - m_specifiedApplicableRange, midPoint);
        }
    case winrt::SnapPointApplicableRangeType::Mandatory:
        return midPoint;
    default:
        MUX_ASSERT(false);
        return 0.0;
    }
#else
    return midPoint;
#endif
}

double ZoomSnapPoint::ImpulseInfluence(double edgeOfMidpoint, double ignoredValue) const
{
    const double midPoint = (m_value + edgeOfMidpoint) / 2.0;

    if (isnan(ignoredValue))
    {
        return midPoint;
    }
    else
    {
        if (m_value <= edgeOfMidpoint)
        {
            return std::min(m_value, midPoint);
        }
        else
        {
            return std::max(m_value, midPoint);
        }
    }
}

void ZoomSnapPoint::Combine(
    int& combinationCount,
    winrt::SnapPointBase const& snapPoint) const
{
    auto snapPointAsIrregular = snapPoint.try_as<winrt::ZoomSnapPoint>();
    if (snapPointAsIrregular)
    {
#ifdef ApplicableRangeType
        //TODO: The m_specifiedApplicableRange field is never expected to change after creation. A correction will be needed here.
        m_specifiedApplicableRange = std::max(snapPointAsIrregular.ApplicableRange(), m_specifiedApplicableRange);
#else
        MUX_ASSERT(m_specifiedApplicableRange == INFINITY);
#endif
        combinationCount++;
    }
    else
    {
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

int ZoomSnapPoint::SnapCount() const
{
    return 1;
}

double ZoomSnapPoint::Evaluate(
    std::tuple<double, double> actualApplicableZone,
    double value) const
{
    if (value >= std::get<0>(actualApplicableZone) && value <= std::get<1>(actualApplicableZone))
    {
        return m_value;
    }
    return value;
}

/////////////////////////////////////////////////////////////////////
/////////////////    Repeated Snap Points    ////////////////////////
/////////////////////////////////////////////////////////////////////
#include "RepeatedZoomSnapPoint.properties.cpp"

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
    m_start = start;
    m_end = end;
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
    m_start = start;
    m_end = end;
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
    return m_start;
}

double RepeatedZoomSnapPoint::End()
{
    return m_end;
}

// For zoom snap points scale == L"1.0".
winrt::ExpressionAnimation RepeatedZoomSnapPoint::CreateRestingPointExpression(
    double ignoredValue,
    std::tuple<double, double> actualImpulseApplicableZone,
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse)
{
    /*
    fracTarget = (target - first) / interval               // Unsnapped value in fractional intervals from first snapping value
    prevSnap = ((Floor(fracTarget) * interval) + first)    // First snapped value before unsnapped value
    nextSnap = ((Ceil(fracTarget) * interval) + first)     // First snapped value after unsnapped value
    effectiveEnd = (IsInertiaFromImpulse ? impEnd : end)   // Regular or impulse upper bound of applicable zone

    Expression:
     ((Abs(target - prevSnap) >= Abs(target - nextSnap)) && (nextSnap <= effectiveEnd))
     ?
     // nextSnap value is closer to unsnapped value and within applicable zone.
     (
      IsInertiaFromImpulse
      ?
      // Impulse mode.
      (
       nextSnap == impIgn
       ?
       (
        // Next snapped value is ignored. Pick the previous snapped value if any, else the ignored value.
        (impIgn == first ? first : impIgn - interval)
       )
       :
       // Pick next snapped value.
       nextSnap
      )
      :
      // Regular mode. Pick next snapped value.
      nextSnap
     )
     :
     // prevSnap value is closer to unsnapped value.
     (
      IsInertiaFromImpulse
      ?
      // Impulse mode.
      (
       prevSnap == impIgn
       ?
       // Previous snapped value is ignored. Pick the next snapped value if any, else the ignored value.
       (impIgn + interval <= effectiveEnd ? impIgn + interval : impIgn)
       :
       (
        prevSnap < first i.e. fracTarget < -0.5
        ?
        // Pick next snapped value as previous snapped value is outside applicable zone.
        nextSnap
        :
        // Pick previous snapped value as it is within applicable zone.
        prevSnap
       )
      )
      :
      // Regular mode.
      (
       prevSnap < first i.e. fracTarget < -0.5
       ?
       // Pick next snapped value as previous snapped value is outside applicable zone.
       nextSnap
       :
       // Pick previous snapped value as it is within applicable zone.
       prevSnap
      )
     )
    */

    winrt::hstring isInertiaFromImpulseExpression = GetIsInertiaFromImpulseExpression(s_interactionTracker.data());
    winrt::hstring expression = StringUtil::FormatString(
        L"((Abs(T.%2!s!-(Floor((T.%2!s!-P)/V)*V+P))>=Abs(T.%2!s!-(Ceil((T.%2!s!-P)/V)*V+P)))&&(Ceil((T.%2!s!-P)/V)*V+P<=(%1!s!?iE:E)))?(%1!s!?(Ceil((T.%2!s!-P)/V)*V+P==M?(M==P?P:M-V):Ceil((T.%2!s!-P)/V)*V+P):Ceil((T.%2!s!-P)/V)*V+P):(%1!s!?(Floor((T.%2!s!-P)/V)*V+P==M?(M+V<=(%1!s!?iE:E)?(M+V):M):(T.%2!s!<P-0.5*V?Ceil((T.%2!s!-P)/V)*V+P:Floor((T.%2!s!-P)/V)*V+P)):(T.%2!s!<P-0.5*V?Ceil((T.%2!s!-P)/V)*V+P:Floor((T.%2!s!-P)/V)*V+P))",
        isInertiaFromImpulseExpression.data(),
        target.data());

    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, expression.c_str());

    auto restingPointExpressionAnimation = interactionTracker.Compositor().CreateExpressionAnimation(expression);

    SetScalarParameter(restingPointExpressionAnimation, s_interval, static_cast<float>(m_interval));
    SetScalarParameter(restingPointExpressionAnimation, s_end, static_cast<float>(m_end));
    SetScalarParameter(restingPointExpressionAnimation, s_first, static_cast<float>(DetermineFirstRepeatedSnapPointValue()));
    restingPointExpressionAnimation.SetReferenceParameter(s_interactionTracker, interactionTracker);

    UpdateRestingPointExpressionAnimationForImpulse(
        restingPointExpressionAnimation,
        ignoredValue,
        actualImpulseApplicableZone);

    UpdateExpressionAnimationForImpulse(
        restingPointExpressionAnimation,
        isInertiaFromImpulse);

    return restingPointExpressionAnimation;
}

// For zoom snap points scale == L"1.0".
winrt::ExpressionAnimation RepeatedZoomSnapPoint::CreateConditionalExpression(
    std::tuple<double, double> actualApplicableZone,
    std::tuple<double, double> actualImpulseApplicableZone,
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse)
{
    MUX_ASSERT(std::get<0>(actualApplicableZone) == m_start);
    MUX_ASSERT(std::get<1>(actualApplicableZone) == m_end);

    /*
    fracTarget = (target - first) / interval               // Unsnapped value in fractional intervals from first snapping value
    prevSnap = ((Floor(fracTarget) * interval) + first)    // First snapped value before unsnapped value
    nextSnap = ((Ceil(fracTarget) * interval) + first)     // First snapped value after unsnapped value
    effectiveEnd = (IsInertiaFromImpulse ? impEnd : end)   // Regular or impulse upper bound of applicable zone

    Expression:
    (
     (!IsInertiaFromImpulse && target >= start && target <= end)       // If we are within the start and end in non-impulse mode
     ||
     (IsInertiaFromImpulse && target >= impStart && target <= impEnd)  // or we are within the impulse start and end in impulse mode
    )
    &&                                                                 // and...
    (                                                                  // The location of the repeated snap point just before the natural resting point
     (prevSnap + appRange >= target)                                   // Plus the applicable range is greater than the natural resting point
     ||                                                                // or...
     (                                                                 // The location of the repeated snap point just after the natural resting point
      (nextSnap - appRange <= target) &&                               // Minus the applicable range is less than the natural resting point.
      (nextSnap <= effectiveEnd)                                       // And the snap point after the natural resting point is less than or equal to the effective end value
     )
    )
    */

    winrt::hstring isInertiaFromImpulseExpression = GetIsInertiaFromImpulseExpression(s_interactionTracker.data());
    winrt::hstring expression = StringUtil::FormatString(
        L"((!%1!s!&&T.%2!s!>=S&&T.%2!s!<=E)||(%1!s!&&T.%2!s!>=iS&&T.%2!s!<=iE))&&((Floor((T.%2!s!-P)/V)*V+P+aR>=T.%2!s!)||((Ceil((T.%2!s!-P)/V)*V+P-aR<=T.%2!s!)&&(Ceil((T.%2!s!-P)/V)*V+P<=(%1!s!?iE:E))))",
        isInertiaFromImpulseExpression.data(),
        target.data());

    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, expression.c_str());

    auto conditionExpressionAnimation = interactionTracker.Compositor().CreateExpressionAnimation(expression);

    SetScalarParameter(conditionExpressionAnimation, s_interval, static_cast<float>(m_interval));
    SetScalarParameter(conditionExpressionAnimation, s_first, static_cast<float>(DetermineFirstRepeatedSnapPointValue()));
    SetScalarParameter(conditionExpressionAnimation, s_start, static_cast<float>(m_start));
    SetScalarParameter(conditionExpressionAnimation, s_end, static_cast<float>(m_end));
    SetScalarParameter(conditionExpressionAnimation, s_applicableRange, static_cast<float>(m_specifiedApplicableRange));
    conditionExpressionAnimation.SetReferenceParameter(s_interactionTracker, interactionTracker);

    UpdateConditionalExpressionAnimationForImpulse(
        conditionExpressionAnimation,
        actualImpulseApplicableZone);

    UpdateExpressionAnimationForImpulse(
        conditionExpressionAnimation,
        isInertiaFromImpulse);

    return conditionExpressionAnimation;
}

void RepeatedZoomSnapPoint::UpdateConditionalExpressionAnimationForImpulse(
    winrt::ExpressionAnimation const& conditionExpressionAnimation,
    std::tuple<double, double> actualImpulseApplicableZone) const
{
    SetScalarParameter(conditionExpressionAnimation, s_impulseStart, static_cast<float>(std::get<0>(actualImpulseApplicableZone)));
    SetScalarParameter(conditionExpressionAnimation, s_impulseEnd, static_cast<float>(std::get<1>(actualImpulseApplicableZone)));
}

void RepeatedZoomSnapPoint::UpdateRestingPointExpressionAnimationForImpulse(
    winrt::ExpressionAnimation const& restingValueExpressionAnimation,
    double ignoredValue,
    std::tuple<double, double> actualImpulseApplicableZone) const
{
    SetScalarParameter(restingValueExpressionAnimation, s_impulseEnd, static_cast<float>(std::get<1>(actualImpulseApplicableZone)));
    SetScalarParameter(restingValueExpressionAnimation, s_impulseIgnoredValue, static_cast<float>(ignoredValue));
}

SnapPointSortPredicate RepeatedZoomSnapPoint::SortPredicate() const
{
    // Repeated snap points should be sorted after irregular snap points, so give it a tertiary sort value of 1 (irregular snap points get 0)
    return SnapPointSortPredicate{ m_start, m_end, 1 };
}

std::tuple<double, double> RepeatedZoomSnapPoint::DetermineActualApplicableZone(
    const SnapPointBase* previousSnapPoint,
    const SnapPointBase* nextSnapPoint)
{
    std::tuple<double, double> actualApplicableZoneReturned = std::make_tuple(
        DetermineMinActualApplicableZone(previousSnapPoint),
        DetermineMaxActualApplicableZone(nextSnapPoint));

    // Influence() will not have thrown if either of the adjacent snap points are also repeated snap points which have the same start and end, however this is not allowed.
    // We only need to check the nextSnapPoint because of the symmetry in the algorithm.
    if (nextSnapPoint && *static_cast<SnapPointBase*>(this) == (nextSnapPoint))
    {
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }

    return actualApplicableZoneReturned;
}

std::tuple<double, double> RepeatedZoomSnapPoint::DetermineActualImpulseApplicableZone(
    const SnapPointBase* previousSnapPoint,
    const SnapPointBase* nextSnapPoint,
    double currentIgnoredValue,
    double previousIgnoredValue,
    double nextIgnoredValue)
{
    return std::make_tuple(
        DetermineMinActualImpulseApplicableZone(
            previousSnapPoint,
            currentIgnoredValue,
            previousIgnoredValue),
        DetermineMaxActualImpulseApplicableZone(
            nextSnapPoint,
            currentIgnoredValue,
            nextIgnoredValue));
}

double RepeatedZoomSnapPoint::DetermineFirstRepeatedSnapPointValue() const
{
    MUX_ASSERT(m_offset >= m_start);
    MUX_ASSERT(m_interval > 0.0);

    return m_offset - std::floor((m_offset - m_start) / m_interval) * m_interval;
}

double RepeatedZoomSnapPoint::DetermineLastRepeatedSnapPointValue() const
{
    MUX_ASSERT(m_offset <= m_end);
    MUX_ASSERT(m_interval > 0.0);

    return m_offset + std::floor((m_end - m_offset) / m_interval) * m_interval;
}

double RepeatedZoomSnapPoint::DetermineMinActualApplicableZone(
    const SnapPointBase* previousSnapPoint) const
{
    // The Influence() method of repeated snap points has a check to ensure the value does not fall within its range.
    // This call will ensure that we are not in the range of the previous snap point if it is.
    if (previousSnapPoint)
    {
        previousSnapPoint->Influence(m_start);
    }
    return m_start;
}

double RepeatedZoomSnapPoint::DetermineMinActualImpulseApplicableZone(
    const SnapPointBase* previousSnapPoint,
    double currentIgnoredValue,
    double previousIgnoredValue) const
{
    if (previousSnapPoint)
    {
        if (currentIgnoredValue == DetermineFirstRepeatedSnapPointValue())
        {
            return currentIgnoredValue;
        }

        if (!isnan(previousIgnoredValue))
        {
            return previousSnapPoint->ImpulseInfluence(m_start, previousIgnoredValue);
        }
    }
    return m_start;
}

double RepeatedZoomSnapPoint::DetermineMaxActualApplicableZone(
    const SnapPointBase* nextSnapPoint) const
{
    // The Influence() method of repeated snap points has a check to ensure the value does not fall within its range.
    // This call will ensure that we are not in the range of the next snap point if it is.
    if (nextSnapPoint)
    {
        nextSnapPoint->Influence(m_end);
    }
    return m_end;
}

double RepeatedZoomSnapPoint::DetermineMaxActualImpulseApplicableZone(
    const SnapPointBase* nextSnapPoint,
    double currentIgnoredValue,
    double nextIgnoredValue) const
{
    if (nextSnapPoint)
    {
        if (currentIgnoredValue == DetermineLastRepeatedSnapPointValue())
        {
            return currentIgnoredValue;
        }

        if (!isnan(nextIgnoredValue))
        {
            return nextSnapPoint->ImpulseInfluence(m_end, nextIgnoredValue);
        }
    }
    return m_end;
}

void RepeatedZoomSnapPoint::ValidateConstructorParameters(
#ifdef ApplicableRangeType
    bool applicableRangeToo,
    double applicableRange,
#endif
    double offset,
    double interval,
    double start,
    double end) const
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

double RepeatedZoomSnapPoint::Influence(double edgeOfMidpoint) const
{
    if (edgeOfMidpoint <= m_start)
    {
        return m_start;
    }
    else if (edgeOfMidpoint >= m_end)
    {
        return m_end;
    }
    else
    {
        // Snap points are not allowed within the bounds (Start thru End) of repeated snap points
        // TODO: Provide custom error message
        throw winrt::hresult_error(E_INVALIDARG);
    }
    return 0.0;
}

double RepeatedZoomSnapPoint::ImpulseInfluence(double edgeOfMidpoint, double ignoredValue) const
{
    if (edgeOfMidpoint <= m_start)
    {
        if (ignoredValue == DetermineFirstRepeatedSnapPointValue())
        {
            return ignoredValue;
        }
        return m_start;
    }
    else if (edgeOfMidpoint >= m_end)
    {
        if (ignoredValue == DetermineLastRepeatedSnapPointValue())
        {
            return ignoredValue;
        }
        return m_end;
    }
    else
    {
        MUX_ASSERT(false);
        return 0.0;
    }
}

void RepeatedZoomSnapPoint::Combine(
    int& combinationCount,
    winrt::SnapPointBase const& snapPoint) const
{
    // Snap points are not allowed within the bounds (Start thru End) of repeated snap points
    // TODO: Provide custom error message
    throw winrt::hresult_error(E_INVALIDARG);
}

int RepeatedZoomSnapPoint::SnapCount() const
{
    return static_cast<int>((m_end - m_start) / m_interval);
}

double RepeatedZoomSnapPoint::Evaluate(
    std::tuple<double, double> actualApplicableZone,
    double value) const
{
    if (value >= m_start && value <= m_end)
    {
        const double firstSnapPointValue = DetermineFirstRepeatedSnapPointValue();
        const double passedSnapPoints = std::floor((value - firstSnapPointValue) / m_interval);
        const double previousSnapPointValue = (passedSnapPoints * m_interval) + firstSnapPointValue;
        const double nextSnapPointValue = previousSnapPointValue + m_interval;

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
