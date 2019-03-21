// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollerSnapPoint.h"

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
    winrt::ExpressionAnimation CreateRestingPointExpression(
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale) const;
    winrt::ExpressionAnimation CreateConditionalExpression(
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale) const;
    void DetermineActualApplicableZone(
        SnapPointBase* previousSnapPoint,
        SnapPointBase* nextSnapPoint);
    void Combine(SnapPointWrapper<T>* snapPointWrapper);
    double Evaluate(double value) const;

private:
    T m_snapPoint;
    std::tuple<double, double> m_actualApplicableZone{ -INFINITY, INFINITY };
    int m_combinationCount{ 0 };
};

template <typename T>
struct SnapPointWrapperComparator
{
    inline bool operator()(std::shared_ptr<SnapPointWrapper<T>> left, std::shared_ptr<SnapPointWrapper<T>> right) const
    {
        winrt::SnapPointBase winrtLeftSnapPoint = safe_cast<winrt::SnapPointBase>(left->SnapPoint());
        winrt::SnapPointBase winrtRightSnapPoint = safe_cast<winrt::SnapPointBase>(right->SnapPoint());
        SnapPointBase* leftSnapPoint = winrt::get_self<SnapPointBase>(winrtLeftSnapPoint);
        SnapPointBase* rightSnapPoint = winrt::get_self<SnapPointBase>(winrtRightSnapPoint);

        return *leftSnapPoint < rightSnapPoint;
    }
};
