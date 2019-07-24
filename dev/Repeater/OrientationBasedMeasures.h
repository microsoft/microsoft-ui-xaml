// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class ScrollOrientation
{
    Vertical,
    Horizontal
};

class OrientationBasedMeasures
{
public:
    ScrollOrientation GetScrollOrientation() const { return m_orientation; }
    void SetScrollOrientation(ScrollOrientation value) { m_orientation = value; }

    // Major - Scrolling/virtualizing direction
    // Minor - Opposite direction
    float winrt::Size::* Major() const;
    float winrt::Size::* Minor() const;

    float winrt::Rect::* MajorSize() const;
    float winrt::Rect::* MinorSize() const;
    float winrt::Rect::* MajorStart() const;
    float MajorEnd(const winrt::Rect& rect) const;
    float winrt::Rect::* MinorStart() const;
    float MinorEnd(const winrt::Rect& rect) const;

    winrt::Rect MinorMajorRect(float minor, float major, float minorSize, float majorSize);
    winrt::Point MinorMajorPoint(float minor, float major);
    winrt::Size MinorMajorSize(float minor, float major);

private:
    ScrollOrientation m_orientation { ScrollOrientation::Vertical };
};

