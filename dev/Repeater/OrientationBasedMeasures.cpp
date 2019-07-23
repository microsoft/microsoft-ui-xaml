// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "OrientationBasedMeasures.h"
#include <pch.h>

float winrt::Size::* OrientationBasedMeasures::Major() const
{
    return m_orientation == ScrollOrientation::Vertical ? &winrt::Size::Height : &winrt::Size::Width;
}

float winrt::Size::* OrientationBasedMeasures::Minor() const
{
    return m_orientation == ScrollOrientation::Vertical ? &winrt::Size::Width : &winrt::Size::Height;
}

float winrt::Rect::* OrientationBasedMeasures::MajorSize() const
{
    return m_orientation == ScrollOrientation::Vertical ? &winrt::Rect::Height : &winrt::Rect::Width;
}

float winrt::Rect::* OrientationBasedMeasures::MinorSize() const
{
    return m_orientation == ScrollOrientation::Vertical ? &winrt::Rect::Width : &winrt::Rect::Height;
}

float winrt::Rect::* OrientationBasedMeasures::MajorStart() const
{
    return m_orientation == ScrollOrientation::Vertical ? &winrt::Rect::Y : &winrt::Rect::X;
}

float OrientationBasedMeasures::MajorEnd(const winrt::Rect& rect) const
{
    return m_orientation == ScrollOrientation::Vertical ?
        rect.Y + rect.Height : rect.X + rect.Width;
}

float winrt::Rect::* OrientationBasedMeasures::MinorStart() const
{
    return m_orientation == ScrollOrientation::Vertical ? &winrt::Rect::X : &winrt::Rect::Y;
}

float OrientationBasedMeasures::MinorEnd(const winrt::Rect& rect) const
{
    return m_orientation == ScrollOrientation::Vertical ?
        rect.X + rect.Width : rect.Y + rect.Height;
}

winrt::Rect OrientationBasedMeasures::MinorMajorRect(float minor, float major, float minorSize, float majorSize)
{
    return m_orientation == ScrollOrientation::Vertical ?
        winrt::Rect{ minor, major, minorSize, majorSize } :
        winrt::Rect{ major, minor, majorSize, minorSize };
}

winrt::Point OrientationBasedMeasures::MinorMajorPoint(float minor, float major)
{
    return m_orientation == ScrollOrientation::Vertical ?
        winrt::Point(minor, major) :
        winrt::Point(major, minor);
}

winrt::Size OrientationBasedMeasures::MinorMajorSize(float minor, float major)
{
    return m_orientation == ScrollOrientation::Vertical ?
        winrt::Size(minor, major) :
        winrt::Size(major, minor);
}