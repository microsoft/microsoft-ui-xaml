// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SnapPointBase.g.h"
#include "ScrollSnapPointBase.g.h"
#include "ScrollSnapPoint.g.h"
#include "RepeatedScrollSnapPoint.g.h"
#include "ZoomSnapPointBase.g.h"
#include "ZoomSnapPoint.g.h"
#include "RepeatedZoomSnapPoint.g.h"

struct ScrollerSnapPointSortPredicate
{
    //Sorting of snap points goes as follows:
    //We first sort on the primary value which for repeated snap points are their actualStart values, and for irregular snap points their values.
    //We then sort by the secondary value which for repeated snap points are their actualEnd values, and for irregular snap points are again their values.
    //We then sort by the tertiary value which for repeated snap points is 1 and for irregular snap points is 0.

    //Since repeated snap points' actualEnd is always greater than or equal to their actualStart the tertiary value is only used if these values are exactly equal
    //for a repeated snap point which also shares a primary (and subsequently secondary) value with an irregular snap point's value.

    //The result of this sorting is irregular snap points will always be sorted before repeated snap points when ambiguity occurs.
    //This allows us to address some corner cases in a more elegant fashion.
    double primary;
    double secondary;
    short tertiary;
};

class SnapPointBase :
    public ReferenceTracker<SnapPointBase, winrt::implementation::SnapPointBaseT, winrt::composable>
{
public:
#ifdef ApplicableRangeType
    double ApplicableRange();
    winrt::SnapPointApplicableRangeType ApplicableRangeType();
#endif

    //Internal
    bool operator< (SnapPointBase* snapPoint);
    bool operator== (SnapPointBase* snapPoint);

    virtual winrt::ExpressionAnimation CreateRestingPointExpression(
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale) = 0;
    virtual winrt::ExpressionAnimation CreateConditionalExpression(
        std::tuple<double, double> actualApplicableZone,
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale) = 0;
    virtual ScrollerSnapPointSortPredicate SortPredicate() = 0;
    virtual std::tuple<double, double> DetermineActualApplicableZone(
        SnapPointBase* previousSnapPoint,
        SnapPointBase* nextSnapPoint) = 0;
    virtual double Influence(double edgeOfMidpoint) const = 0;
    virtual void Combine(
        int& combinationCount,
        winrt::SnapPointBase const& snapPoint) const = 0;
    virtual double Evaluate(
        std::tuple<double, double> actualApplicableZone,
        double value) const = 0;

    // Returns True when this snap point is sensitive to the viewport size and is interested in future updates.
    virtual bool OnUpdateViewport(double newViewport) = 0;

#ifdef _DEBUG
    winrt::Color VisualizationColor();
    void VisualizationColor(winrt::Color color);
#endif // _DEBUG

protected:
    // Needed as work around for Modern Idl inheritance bug
    SnapPointBase();

    winrt::hstring GetTargetExpression(winrt::hstring const& target) const;

    double m_specifiedApplicableRange{ INFINITY };
#ifdef ApplicableRangeType
    // Only mandatory snap points are supported at this point.
    winrt::SnapPointApplicableRangeType m_applicableRangeType{ winrt::SnapPointApplicableRangeType::Mandatory };
#endif
#ifdef _DEBUG
    winrt::Color m_visualizationColor{ winrt::Colors::Black() };
#endif // _DEBUG

    // Maximum difference for snap points to be considered equal
    static constexpr double s_equalityEpsilon{ 0.00001 };
};

class ScrollSnapPointBase :
    public winrt::implementation::ScrollSnapPointBaseT<ScrollSnapPointBase, SnapPointBase>
{
public:
    winrt::ScrollSnapPointsAlignment Alignment();

    bool OnUpdateViewport(double newViewport);

protected:
    // Needed as work around for Modern Idl inheritance bug
    ScrollSnapPointBase();

    winrt::ScrollSnapPointsAlignment m_alignment{ winrt::ScrollSnapPointsAlignment::Near };
    double m_alignmentAdjustment{ 0.0 }; // Non-zero adjustment based on viewport size, when the alignment is Center or Far.
};

class ScrollSnapPoint:
    public winrt::implementation::ScrollSnapPointT<ScrollSnapPoint, ScrollSnapPointBase>
{
public:
    ScrollSnapPoint(
        double snapPointValue,
        winrt::ScrollSnapPointsAlignment alignment = winrt::ScrollSnapPointsAlignment::Near);

#ifdef ApplicableRangeType
    ScrollSnapPoint(
        double snapPointValue,
        double applicableRange,
        winrt::ScrollSnapPointsAlignment alignment = winrt::ScrollSnapPointsAlignment::Near);
#endif

    double Value();

    //Internal
    winrt::ExpressionAnimation CreateRestingPointExpression(
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale);
    winrt::ExpressionAnimation CreateConditionalExpression(
        std::tuple<double, double> actualApplicableZone,
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale);
    ScrollerSnapPointSortPredicate SortPredicate();
    std::tuple<double, double> DetermineActualApplicableZone(
        SnapPointBase* previousSnapPoint,
        SnapPointBase* nextSnapPoint);
    double Influence(double edgeOfMidpoint) const;
    void Combine(
        int& combinationCount,
        winrt::SnapPointBase const& snapPoint) const;
    double Evaluate(
        std::tuple<double, double> actualApplicableZone,
        double value) const;

private:
    double ActualValue() const;
    double DetermineMinActualApplicableZone(
        SnapPointBase* previousSnapPoint) const;
    double DetermineMaxActualApplicableZone(
        SnapPointBase* nextSnapPoint) const;

    double m_value{ 0.0 };
};

class RepeatedScrollSnapPoint :
    public winrt::implementation::RepeatedScrollSnapPointT<RepeatedScrollSnapPoint, ScrollSnapPointBase>
{
public:
    RepeatedScrollSnapPoint(
        double offset,
        double interval,
        double start,
        double end,
        winrt::ScrollSnapPointsAlignment alignment = winrt::ScrollSnapPointsAlignment::Near);

#ifdef ApplicableRangeType
    RepeatedScrollSnapPoint(
        double offset,
        double interval,
        double start,
        double end,
        double applicableRange,
        winrt::ScrollSnapPointsAlignment alignment = winrt::ScrollSnapPointsAlignment::Near);
#endif

    double Offset();
    double Interval();
    double Start();
    double End();

    //Internal
    winrt::ExpressionAnimation CreateRestingPointExpression(
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale);
    winrt::ExpressionAnimation CreateConditionalExpression(
        std::tuple<double, double> actualApplicableZone,
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale);
    ScrollerSnapPointSortPredicate SortPredicate();
    std::tuple<double, double> DetermineActualApplicableZone(
        SnapPointBase* previousSnapPoint,
        SnapPointBase* nextSnapPoint);
    double Influence(double edgeOfMidpoint) const;
    void Combine(
        int& combinationCount,
        winrt::SnapPointBase const& snapPoint) const;
    double Evaluate(
        std::tuple<double, double> actualApplicableZone,
        double value) const;

private:
    double ActualOffset() const;
    double ActualStart() const;
    double ActualEnd() const;
    double DetermineFirstRepeatedSnapPointValue() const;
    double DetermineMinActualApplicableZone(
        SnapPointBase* previousSnapPoint) const;
    double DetermineMaxActualApplicableZone(
        SnapPointBase* nextSnapPoint) const;
    void ValidateConstructorParameters(
#ifdef ApplicableRangeType
        bool applicableRangeToo,
        double applicableRange,
#endif
        double offset,
        double interval,
        double start,
        double end) const;

    double m_offset{ 0.0f };
    double m_interval{ 0.0f };
    double m_start{ 0.0f };
    double m_end{ 0.0f };
};

class ZoomSnapPointBase :
    public winrt::implementation::ZoomSnapPointBaseT<ZoomSnapPointBase, SnapPointBase>
{
    bool OnUpdateViewport(double newViewport);

protected:
    // Needed as work around for Modern Idl inheritance bug
    ZoomSnapPointBase();
};

class ZoomSnapPoint :
    public winrt::implementation::ZoomSnapPointT<ZoomSnapPoint, ZoomSnapPointBase>
{
public:
    ZoomSnapPoint(
        double snapPointValue);

#ifdef ApplicableRangeType
    ZoomSnapPoint(
        double snapPointValue,
        double applicableRange);
#endif

    double Value();

    //Internal
    winrt::ExpressionAnimation CreateRestingPointExpression(
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale);
    winrt::ExpressionAnimation CreateConditionalExpression(
        std::tuple<double, double> actualApplicableZone,
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale);
    ScrollerSnapPointSortPredicate SortPredicate();
    std::tuple<double, double> DetermineActualApplicableZone(
        SnapPointBase* previousSnapPoint,
        SnapPointBase* nextSnapPoint);
    double Influence(double edgeOfMidpoint) const;
    void Combine(
        int& combinationCount,
        winrt::SnapPointBase const& snapPoint) const;
    double Evaluate(
        std::tuple<double, double> actualApplicableZone,
        double value) const;

private:
    double DetermineMinActualApplicableZone(
        SnapPointBase* previousSnapPoint) const;
    double DetermineMaxActualApplicableZone(
        SnapPointBase* nextSnapPoint) const;

    double m_value{ 0.0 };
};

class RepeatedZoomSnapPoint :
    public winrt::implementation::RepeatedZoomSnapPointT<RepeatedZoomSnapPoint, ZoomSnapPointBase>
{
public:
    RepeatedZoomSnapPoint(
        double offset,
        double interval,
        double start,
        double end);

#ifdef ApplicableRangeType
    RepeatedZoomSnapPoint(
        double offset,
        double interval,
        double start,
        double end,
        double applicableRange);
#endif

    double Offset();
    double Interval();
    double Start();
    double End();

    //Internal
    winrt::ExpressionAnimation CreateRestingPointExpression(
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale);
    winrt::ExpressionAnimation CreateConditionalExpression(
        std::tuple<double, double> actualApplicableZone,
        winrt::Compositor const& compositor,
        winrt::hstring const& target,
        winrt::hstring const& scale);
    ScrollerSnapPointSortPredicate SortPredicate();
    std::tuple<double, double> DetermineActualApplicableZone(
        SnapPointBase* previousSnapPoint,
        SnapPointBase* nextSnapPoint);
    double Influence(double edgeOfMidpoint) const;
    void Combine(
        int& combinationCount,
        winrt::SnapPointBase const& snapPoint) const;
    double Evaluate(
        std::tuple<double, double> actualApplicableZone,
        double value) const;

private:
    double DetermineFirstRepeatedSnapPointValue() const;
    double DetermineMinActualApplicableZone(
        SnapPointBase* previousSnapPoint) const;
    double DetermineMaxActualApplicableZone(
        SnapPointBase* nextSnapPoint) const;
    void ValidateConstructorParameters(
#ifdef ApplicableRangeType
        bool applicableRangeToo,
        double applicableRange,
#endif
        double offset,
        double interval,
        double start,
        double end) const;

    double m_offset{ 0.0f };
    double m_interval{ 0.0f };
    double m_start{ 0.0f };
    double m_end{ 0.0f };
};
