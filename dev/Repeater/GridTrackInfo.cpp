// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "GridTrackInfo.h"

GridTrackInfo::GridTrackInfo()
{
}

winrt::hstring GridTrackInfo::LineName()
{
    return m_lineName;
}

void GridTrackInfo::LineName(winrt::hstring const& value)
{
    m_lineName = value;
}

double GridTrackInfo::Length()
{
    return m_length;
}

void GridTrackInfo::Length(double const& value)
{
    m_length = value;
}

bool GridTrackInfo::Auto()
{
    return m_auto;
}

void GridTrackInfo::Auto(bool const& value)
{
    m_auto = value;
}

double GridTrackInfo::Fraction()
{
    return m_fraction;
}

void GridTrackInfo::Fraction(double const& value)
{
    m_fraction = value;
}

double GridTrackInfo::Percentage()
{
    return m_percentage;
}

void GridTrackInfo::Percentage(double const& value)
{
    m_percentage = value;
}
