// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GridTrackInfo.g.h"
#include "GridTrackInfo.properties.h"

class GridTrackInfo :
    public ReferenceTracker<GridTrackInfo, winrt::implementation::GridTrackInfoT>,
    public GridTrackInfoProperties
{
public:
    GridTrackInfo();

    winrt::hstring LineName();
    void LineName(winrt::hstring const& value);

    double Length();
    void Length(double const& value);

    bool Auto();
    void Auto(bool const& value);

    double Fraction();
    void Fraction(double const& value);

    double Percentage();
    void Percentage(double const& value);

private:
    winrt::hstring m_lineName;
    double m_length { };
    bool m_auto { };
    double m_fraction { };
    double m_percentage { };
};
