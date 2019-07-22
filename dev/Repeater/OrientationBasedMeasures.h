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
    [[nodiscard]] ScrollOrientation GetScrollOrientation() const { return m_orientation; }
    void SetScrollOrientation(ScrollOrientation value) { m_orientation = value; }

    // Major - Scrolling/virtualizing direction
    // Minor - Opposite direction
    [[nodiscard]] float winrt::Size::* Major() const;
    [[nodiscard]] float winrt::Size::* Minor() const;

    [[nodiscard]] float winrt::Rect::* MajorSize() const;
    [[nodiscard]] float winrt::Rect::* MinorSize() const;
    [[nodiscard]] float winrt::Rect::* MajorStart() const;
    [[nodiscard]] float MajorEnd(const winrt::Rect& rect) const;
    [[nodiscard]] float winrt::Rect::* MinorStart() const;
    [[nodiscard]] float MinorEnd(const winrt::Rect& rect) const;

    winrt::Rect MinorMajorRect(float minor, float major, float minorSize, float majorSize);
    winrt::Point MinorMajorPoint(float minor, float major);
    winrt::Size MinorMajorSize(float minor, float major);

private:
    ScrollOrientation m_orientation { ScrollOrientation::Vertical };
};

