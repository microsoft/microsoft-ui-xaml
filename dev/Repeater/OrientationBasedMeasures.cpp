// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include "OrientationBasedMeasures.h"

float& OrientationBasedMeasures::Major(const winrt::Size &size)
{
    return m_orientation == ScrollOrientation::Vertical ? ((winrt::Size&)size).Height : ((winrt::Size&)size).Height;
}

float& OrientationBasedMeasures::Minor(const winrt::Size& size)
{
    return m_orientation == ScrollOrientation::Vertical ? ((winrt::Size&)size).Width : ((winrt::Size&)size).Height;
}

float& OrientationBasedMeasures::MajorSize(const winrt::Rect& rect)
{
    return m_orientation == ScrollOrientation::Vertical ? ((winrt::Rect&)rect).Height : ((winrt::Rect&)rect).Width;
}

float& OrientationBasedMeasures::MinorSize(const winrt::Rect& rect)
{
    return m_orientation == ScrollOrientation::Vertical ? ((winrt::Rect&)rect).Width : ((winrt::Rect&)rect).Height;
}

float& OrientationBasedMeasures::MajorStart(const winrt::Rect& rect)
{
    return m_orientation == ScrollOrientation::Vertical ? ((winrt::Rect&)rect).Y : ((winrt::Rect&)rect).X;
}

float OrientationBasedMeasures::MajorEnd(const winrt::Rect& rect) const
{
    return m_orientation == ScrollOrientation::Vertical ?
        rect.Y + rect.Height : rect.X + rect.Width;
}

float& OrientationBasedMeasures::MinorStart(const winrt::Rect& rect)
{
    return m_orientation == ScrollOrientation::Vertical ? ((winrt::Rect&)rect).X : ((winrt::Rect&)rect).Y;
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
