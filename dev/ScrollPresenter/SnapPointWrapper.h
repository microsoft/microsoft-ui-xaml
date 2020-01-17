// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SnapPoint.h"

// The SnapPointWrapper class has a SnapPointBase member that can be shared among multiple
// HorizontalSnapPoints, VerticalSnapPoints and ZoomSnapPoints collections.
// It also includes all the data that is specific to that snap point and a particular collection.

template <typename T>
class SnapPointWrapper
{
public:
    SnapPointWrapper(T const& snapPoint);
#ifdef _DEBUG
    ~SnapPointWrapper();
#endif //_DEBUG

    T SnapPoint() const;
    std::tuple<double, double> ActualApplicableZone() const;
    int CombinationCount() const;
    bool ResetIgnoredValue();
    void SetIgnoredValue(double ignoredValue);

    winrt::ExpressionAnimation CreateRestingPointExpression(
        winrt::InteractionTracker const& interactionTracker,
        winrt::hstring const& target,
        winrt::hstring const& scale,
        bool isInertiaFromImpulse);
    winrt::ExpressionAnimation CreateConditionalExpression(
        winrt::InteractionTracker const& interactionTracker,
        winrt::hstring const& target,
        winrt::hstring const& scale,
        bool isInertiaFromImpulse);
    std::tuple<winrt::ExpressionAnimation, winrt::ExpressionAnimation> GetUpdatedExpressionAnimationsForImpulse();
    std::tuple<winrt::ExpressionAnimation, winrt::ExpressionAnimation> GetUpdatedExpressionAnimationsForImpulse(
        bool isInertiaFromImpulse);
    void DetermineActualApplicableZone(
        SnapPointWrapper<T>* previousSnapPointWrapper,
        SnapPointWrapper<T>* nextSnapPointWrapper,
        bool forImpulseOnly);
    void Combine(SnapPointWrapper<T>* snapPointWrapper);
    double Evaluate(double value) const;
    bool SnapsAt(double value) const;

    static SnapPointBase* GetSnapPointFromWrapper(std::shared_ptr<SnapPointWrapper<T>> snapPointWrapper);

private:
	static SnapPointBase* GetSnapPointFromWrapper(const SnapPointWrapper<T>* snapPointWrapper);

private:
    T m_snapPoint;
    std::tuple<double, double> m_actualApplicableZone{ -INFINITY, INFINITY };
    std::tuple<double, double> m_actualImpulseApplicableZone{ -INFINITY, INFINITY };
    int m_combinationCount{ 0 };
    double m_ignoredValue{ NAN }; // Ignored snapping value when inertia is triggered by an impulse
    winrt::ExpressionAnimation m_conditionExpressionAnimation{ nullptr };
    winrt::ExpressionAnimation m_restingValueExpressionAnimation{ nullptr };
};

template <typename T>
struct SnapPointWrapperComparator
{
    inline bool operator()(std::shared_ptr<SnapPointWrapper<T>> left, std::shared_ptr<SnapPointWrapper<T>> right) const
    {
        winrt::SnapPointBase winrtLeftSnapPoint = left->SnapPoint().as<winrt::SnapPointBase>();
        winrt::SnapPointBase winrtRightSnapPoint = right->SnapPoint().as<winrt::SnapPointBase>();
        SnapPointBase* leftSnapPoint = winrt::get_self<SnapPointBase>(winrtLeftSnapPoint);
        SnapPointBase* rightSnapPoint = winrt::get_self<SnapPointBase>(winrtRightSnapPoint);

        return *leftSnapPoint < rightSnapPoint;
    }
};
