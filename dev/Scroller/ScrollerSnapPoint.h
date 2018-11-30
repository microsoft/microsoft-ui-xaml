// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollerSnapPointBase.g.h"
#include "ScrollerSnapPointIrregular.g.h"
#include "ScrollerSnapPointRegular.g.h"

struct ScrollerSnapPointSortPredicate
{
    //Sorting of snap points goes as follows:
    //We first sort on the primary value which for regular snap points are their actualStart values, and for irregular snap points their values.
    //We then sort by the secondary value which for regular snap points are their actualEnd values, and for irregular snap points are again their values.
    //We then sort by the tertiary value which for regular snap points is 1 and for irregular snap points is 0.

    //Since regular snap points' actualEnd is always greater than or equal to their actualStart the tertiary value is only used if these values are exactly equal
    //for a regular snap point which also shares a primary (and subsequently secondary) value with an irregular snap point's value.

    //The result of this sorting is irregular snap points will always be sorted before regular snap points when ambiguity occurs.
    //This allows us to address some corner cases in a more elegant fashion.
    double primary;
    double secondary;
    short tertiary;
};

class ScrollerSnapPointBase :
    public ReferenceTracker<ScrollerSnapPointBase, winrt::implementation::ScrollerSnapPointBaseT, winrt::composable>
{
public:
    winrt::ScrollerSnapPointAlignment Alignment();
    void Alignment(winrt::ScrollerSnapPointAlignment alignment);
    double SpecifiedApplicableRange();
    winrt::ScrollerSnapPointApplicableRangeType ApplicableRangeType();

    //Internal
    bool operator< (ScrollerSnapPointBase* snapPoint);
    bool operator== (ScrollerSnapPointBase* snapPoint);

    virtual winrt::ExpressionAnimation CreateRestingPointExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale) = 0;
    virtual winrt::ExpressionAnimation CreateConditionalExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale) = 0;
    virtual ScrollerSnapPointSortPredicate SortPredicate() = 0;
    virtual void DetermineActualApplicableZone(ScrollerSnapPointBase* previousSnapPoint, ScrollerSnapPointBase* nextSnapPoint) = 0;
    virtual double Influence(double edgeOfMidpoint) = 0;
    virtual void Combine(winrt::ScrollerSnapPointBase snapPoint) = 0;
    virtual double Evaluate(double value) = 0;
    int CombinationCount();
#ifdef _DEBUG
    winrt::Color VisualizationColor();
    void VisualizationColor(winrt::Color color);
#endif // _DEBUG
    std::tuple<double, double> ActualApplicableZone();
    void ActualApplicableZone(std::tuple<double, double> range);

protected:
    //Needed as work around for Modern Idl inheritance bug
    ScrollerSnapPointBase();

    winrt::hstring GetTargetExpression(winrt::hstring target) const;

    winrt::ScrollerSnapPointAlignment m_alignment{ winrt::ScrollerSnapPointAlignment::Near };
    double m_specifiedApplicableRange{ INFINITY };
    std::tuple<double, double> m_actualApplicableZone{ -INFINITY, INFINITY };
    winrt::ScrollerSnapPointApplicableRangeType m_applicableRangeType{ winrt::ScrollerSnapPointApplicableRangeType::Mandatory };
    int m_combinationCount{ 0 };
#ifdef _DEBUG
    winrt::Color m_visualizationColor{ winrt::Colors::Black() };
#endif // _DEBUG

    //Maximum difference for snap points to be considered equal
    static constexpr double s_equalityEpsilon{ 0.00001 };
};

class ScrollerSnapPointIrregular:
    public winrt::implementation::ScrollerSnapPointIrregularT<ScrollerSnapPointIrregular, ScrollerSnapPointBase>
{
public:
    ScrollerSnapPointIrregular(double snapPointValue,
        winrt::ScrollerSnapPointAlignment alignment = winrt::ScrollerSnapPointAlignment::Near);

    ScrollerSnapPointIrregular(double snapPointValue,
        double applicableRange,
        winrt::ScrollerSnapPointAlignment alignment = winrt::ScrollerSnapPointAlignment::Near,
        winrt::ScrollerSnapPointApplicableRangeType applicableRangeType = winrt::ScrollerSnapPointApplicableRangeType::Optional);

    double Value();

    //Internal
    winrt::ExpressionAnimation CreateRestingPointExpression(winrt::Compositor compositor, winrt::hstring, winrt::hstring scale);
    winrt::ExpressionAnimation CreateConditionalExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale);
    ScrollerSnapPointSortPredicate SortPredicate();
    void DetermineActualApplicableZone(ScrollerSnapPointBase* previousSnapPoint, ScrollerSnapPointBase* nextSnapPoint);
    double Influence(double edgeOfMidpoint);
    void Combine(winrt::ScrollerSnapPointBase snapPoint);
    double Evaluate(double value);
private:
    double DetermineMinActualApplicableZone(ScrollerSnapPointBase* previousSnapPoint);
    double DetermineMaxActualApplicableZone(ScrollerSnapPointBase* nextSnapPoint);

    double m_specifiedValue{ 0.0 };
    double m_actualValue{ 0.0 };
};

class ScrollerSnapPointRegular :
    public winrt::implementation::ScrollerSnapPointRegularT<ScrollerSnapPointRegular, ScrollerSnapPointBase>
{
public:
    ScrollerSnapPointRegular(double offset, double interval, double start, double end,
        winrt::ScrollerSnapPointAlignment alignment = winrt::ScrollerSnapPointAlignment::Near);

    ScrollerSnapPointRegular(double offset, double interval, double start, double end,
        double applicableRange,
        winrt::ScrollerSnapPointAlignment alignment = winrt::ScrollerSnapPointAlignment::Near,
        winrt::ScrollerSnapPointApplicableRangeType applicableRangeType = winrt::ScrollerSnapPointApplicableRangeType::Optional);

    double Offset();
    double Interval();
    double Start();
    double End();

    //Internal
    winrt::ExpressionAnimation CreateRestingPointExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale);
    winrt::ExpressionAnimation CreateConditionalExpression(winrt::Compositor compositor, winrt::hstring target, winrt::hstring scale);
    ScrollerSnapPointSortPredicate SortPredicate();
    void DetermineActualApplicableZone(ScrollerSnapPointBase* previousSnapPoint, ScrollerSnapPointBase* nextSnapPoint);
    double Influence(double edgeOfMidpoint);
    void Combine(winrt::ScrollerSnapPointBase snapPoint);
    double Evaluate(double value);

    double ActualStart();
    void ActualStart(double start);
    double ActualEnd();
    void ActualEnd(double end);

private:
    double DetermineFirstRegularSnapPointValue();
    double DetermineMinActualApplicableZone(ScrollerSnapPointBase* previousSnapPoint);
    double DetermineMaxActualApplicableZone(ScrollerSnapPointBase* nextSnapPoint);
    void ValidateConstructorParameters(
        double offset,
        double interval,
        double start,
        double end,
        double applicableRange,
        bool applicableRangeTypeToo);

    double m_offset{ 0.0f };
    double m_interval{ 0.0f };
    double m_specifiedStart{ 0.0f };
    double m_actualStart{ 0.0f };
    double m_specifiedEnd{ 0.0f };
    double m_actualEnd{ 0.0f };
};

struct winrtProjectionComparator
{
    inline bool operator()(winrt::ScrollerSnapPointBase left, winrt::ScrollerSnapPointBase right) const
    {
        return *winrt::get_self<ScrollerSnapPointBase>(left) < winrt::get_self<ScrollerSnapPointBase>(right);
    }
};