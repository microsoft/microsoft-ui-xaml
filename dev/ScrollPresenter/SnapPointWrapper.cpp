// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SnapPointWrapper.h"

template<typename T>
SnapPointWrapper<T>::SnapPointWrapper(T const& snapPoint)
    : m_snapPoint(snapPoint)
{
}

#ifdef _DEBUG
template<typename T>
SnapPointWrapper<T>::~SnapPointWrapper()
{
}
#endif //_DEBUG

template<typename T>
T SnapPointWrapper<T>::SnapPoint() const
{
    return m_snapPoint;
}

template<typename T>
std::tuple<double, double> SnapPointWrapper<T>::ActualApplicableZone() const
{
    return m_actualApplicableZone;
}

template<typename T>
int SnapPointWrapper<T>::CombinationCount() const
{
    return m_combinationCount;
}

template<typename T>
bool SnapPointWrapper<T>::ResetIgnoredValue()
{
    if (!isnan(m_ignoredValue))
    {
        m_ignoredValue = NAN;
        return true;
    }

    return false;
}

template<typename T>
void SnapPointWrapper<T>::SetIgnoredValue(double ignoredValue)
{
    MUX_ASSERT(!isnan(ignoredValue));

    m_ignoredValue = ignoredValue;
}

template<typename T>
winrt::ExpressionAnimation SnapPointWrapper<T>::CreateRestingPointExpression(
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse)
{
    m_restingValueExpressionAnimation = GetSnapPointFromWrapper(this)->CreateRestingPointExpression(
        m_ignoredValue,
        m_actualImpulseApplicableZone,
        interactionTracker,
        target,
        scale,
        isInertiaFromImpulse);

    return m_restingValueExpressionAnimation;
}

template<typename T>
winrt::ExpressionAnimation SnapPointWrapper<T>::CreateConditionalExpression(
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse)
{
    m_conditionExpressionAnimation = GetSnapPointFromWrapper(this)->CreateConditionalExpression(
        m_actualApplicableZone,
        m_actualImpulseApplicableZone,
        interactionTracker,
        target,
        scale,
        isInertiaFromImpulse);

    return m_conditionExpressionAnimation;
}

// Invoked when the InteractionTracker reaches the Idle State and a new ignored value may have to be set.
template<typename T>
std::tuple<winrt::ExpressionAnimation, winrt::ExpressionAnimation> SnapPointWrapper<T>::GetUpdatedExpressionAnimationsForImpulse()
{
    const SnapPointBase* snapPoint = GetSnapPointFromWrapper(this);

    snapPoint->UpdateConditionalExpressionAnimationForImpulse(
        m_conditionExpressionAnimation,
        m_actualImpulseApplicableZone);
    snapPoint->UpdateRestingPointExpressionAnimationForImpulse(
        m_restingValueExpressionAnimation,
        m_ignoredValue,
        m_actualImpulseApplicableZone);

    return std::make_tuple(m_conditionExpressionAnimation, m_restingValueExpressionAnimation);
}

// Invoked on pre-RS5 versions when ScrollPresenter::m_isInertiaFromImpulse changed
// and the 'iIFI' boolean parameters need to be updated.
template<typename T>
std::tuple<winrt::ExpressionAnimation, winrt::ExpressionAnimation> SnapPointWrapper<T>::GetUpdatedExpressionAnimationsForImpulse(
    bool isInertiaFromImpulse)
{
    MUX_ASSERT(!SharedHelpers::IsRS5OrHigher());

    const SnapPointBase* snapPoint = GetSnapPointFromWrapper(this);

    snapPoint->UpdateExpressionAnimationForImpulse(
        m_conditionExpressionAnimation,
        isInertiaFromImpulse);
    snapPoint->UpdateExpressionAnimationForImpulse(
        m_restingValueExpressionAnimation,
        isInertiaFromImpulse);

    return std::make_tuple(m_conditionExpressionAnimation, m_restingValueExpressionAnimation);
}

template<typename T>
void SnapPointWrapper<T>::DetermineActualApplicableZone(
    const SnapPointWrapper<T>* previousSnapPointWrapper,
    const SnapPointWrapper<T>* nextSnapPointWrapper,
    bool forImpulseOnly)
{
    SnapPointBase* snapPoint = GetSnapPointFromWrapper(this);
    const SnapPointBase* previousSnapPoint = GetSnapPointFromWrapper(previousSnapPointWrapper);
    const SnapPointBase* nextSnapPoint = GetSnapPointFromWrapper(nextSnapPointWrapper);
    const double previousIgnoredValue = previousSnapPointWrapper ? previousSnapPointWrapper->m_ignoredValue : NAN;
    const double nextIgnoredValue = nextSnapPointWrapper ? nextSnapPointWrapper->m_ignoredValue : NAN;

    if (!forImpulseOnly)
    {
        m_actualApplicableZone = snapPoint->DetermineActualApplicableZone(
            previousSnapPoint,
            nextSnapPoint);
    }

    m_actualImpulseApplicableZone = snapPoint->DetermineActualImpulseApplicableZone(
        previousSnapPoint,
        nextSnapPoint,
        m_ignoredValue,
        previousIgnoredValue,
        nextIgnoredValue);
}

template<typename T>
void SnapPointWrapper<T>::Combine(const SnapPointWrapper<T>* snapPointWrapper)
{
    GetSnapPointFromWrapper(this)->Combine(m_combinationCount, snapPointWrapper->SnapPoint());
}

template<typename T>
double SnapPointWrapper<T>::Evaluate(double value) const
{
    return GetSnapPointFromWrapper(this)->Evaluate(m_actualApplicableZone, value);
}

template<typename T>
bool SnapPointWrapper<T>::SnapsAt(double value) const
{
    return GetSnapPointFromWrapper(this)->SnapsAt(m_actualApplicableZone, value);
}

template<typename T>
SnapPointBase* SnapPointWrapper<T>::GetSnapPointFromWrapper(std::shared_ptr<SnapPointWrapper<T>> snapPointWrapper)
{
    return GetSnapPointFromWrapper(snapPointWrapper.get());
}

template<typename T>
SnapPointBase* SnapPointWrapper<T>::GetSnapPointFromWrapper(const SnapPointWrapper<T>* snapPointWrapper)
{
    if (snapPointWrapper)
    {
        winrt::SnapPointBase winrtPreviousSnapPoint = snapPointWrapper->SnapPoint().as<winrt::SnapPointBase>();
        return winrt::get_self<SnapPointBase>(winrtPreviousSnapPoint);
    }
    return nullptr;
}

template SnapPointWrapper<winrt::ScrollSnapPointBase>::SnapPointWrapper(winrt::ScrollSnapPointBase const& snapPoint);
template SnapPointWrapper<winrt::ZoomSnapPointBase>::SnapPointWrapper(winrt::ZoomSnapPointBase const& snapPoint);

#ifdef _DEBUG
template SnapPointWrapper<winrt::ScrollSnapPointBase>::~SnapPointWrapper();
template SnapPointWrapper<winrt::ZoomSnapPointBase>::~SnapPointWrapper();
#endif //_DEBUG

template winrt::ScrollSnapPointBase SnapPointWrapper<winrt::ScrollSnapPointBase>::SnapPoint() const;
template winrt::ZoomSnapPointBase SnapPointWrapper<winrt::ZoomSnapPointBase>::SnapPoint() const;

template std::tuple<double, double> SnapPointWrapper<winrt::ScrollSnapPointBase>::ActualApplicableZone() const;
template std::tuple<double, double> SnapPointWrapper<winrt::ZoomSnapPointBase>::ActualApplicableZone() const;

template int SnapPointWrapper<winrt::ScrollSnapPointBase>::CombinationCount() const;
template int SnapPointWrapper<winrt::ZoomSnapPointBase>::CombinationCount() const;

template bool SnapPointWrapper<winrt::ScrollSnapPointBase>::ResetIgnoredValue();
template bool SnapPointWrapper<winrt::ZoomSnapPointBase>::ResetIgnoredValue();

template void SnapPointWrapper<winrt::ScrollSnapPointBase>::SetIgnoredValue(double ignoredValue);
template void SnapPointWrapper<winrt::ZoomSnapPointBase>::SetIgnoredValue(double ignoredValue);

template winrt::ExpressionAnimation SnapPointWrapper<winrt::ScrollSnapPointBase>::CreateRestingPointExpression(
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse);
template winrt::ExpressionAnimation SnapPointWrapper<winrt::ZoomSnapPointBase>::CreateRestingPointExpression(
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse);

template winrt::ExpressionAnimation SnapPointWrapper<winrt::ScrollSnapPointBase>::CreateConditionalExpression(
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse);
template winrt::ExpressionAnimation SnapPointWrapper<winrt::ZoomSnapPointBase>::CreateConditionalExpression(
    winrt::InteractionTracker const& interactionTracker,
    winrt::hstring const& target,
    winrt::hstring const& scale,
    bool isInertiaFromImpulse);

template std::tuple<winrt::ExpressionAnimation, winrt::ExpressionAnimation> SnapPointWrapper<winrt::ScrollSnapPointBase>::GetUpdatedExpressionAnimationsForImpulse();
template std::tuple<winrt::ExpressionAnimation, winrt::ExpressionAnimation> SnapPointWrapper<winrt::ScrollSnapPointBase>::GetUpdatedExpressionAnimationsForImpulse(
    bool isInertiaFromImpulse);
template std::tuple<winrt::ExpressionAnimation, winrt::ExpressionAnimation> SnapPointWrapper<winrt::ZoomSnapPointBase>::GetUpdatedExpressionAnimationsForImpulse();
template std::tuple<winrt::ExpressionAnimation, winrt::ExpressionAnimation> SnapPointWrapper<winrt::ZoomSnapPointBase>::GetUpdatedExpressionAnimationsForImpulse(
    bool isInertiaFromImpulse);

template void SnapPointWrapper<winrt::ScrollSnapPointBase>::DetermineActualApplicableZone(
    const SnapPointWrapper<winrt::ScrollSnapPointBase>* previousSnapPoint,
    const SnapPointWrapper<winrt::ScrollSnapPointBase>* nextSnapPoint,
    bool forImpulseOnly);
template void SnapPointWrapper<winrt::ZoomSnapPointBase>::DetermineActualApplicableZone(
    const SnapPointWrapper<winrt::ZoomSnapPointBase>* previousSnapPoint,
    const SnapPointWrapper<winrt::ZoomSnapPointBase>* nextSnapPoint,
    bool forImpulseOnly);

template void SnapPointWrapper<winrt::ScrollSnapPointBase>::Combine(const SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper);
template void SnapPointWrapper<winrt::ZoomSnapPointBase>::Combine(const SnapPointWrapper<winrt::ZoomSnapPointBase>* snapPointWrapper);

template double SnapPointWrapper<winrt::ScrollSnapPointBase>::Evaluate(double value) const;
template double SnapPointWrapper<winrt::ZoomSnapPointBase>::Evaluate(double value) const;

template bool SnapPointWrapper<winrt::ScrollSnapPointBase>::SnapsAt(double value) const;
template bool SnapPointWrapper<winrt::ZoomSnapPointBase>::SnapsAt(double value) const;

template SnapPointBase* SnapPointWrapper<winrt::ScrollSnapPointBase>::GetSnapPointFromWrapper(std::shared_ptr<SnapPointWrapper<winrt::ScrollSnapPointBase>> snapPointWrapper);
template SnapPointBase* SnapPointWrapper<winrt::ZoomSnapPointBase>::GetSnapPointFromWrapper(std::shared_ptr<SnapPointWrapper<winrt::ZoomSnapPointBase>> snapPointWrapper);
