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
    float& Major(const winrt::Size& size);
    float& Minor(const winrt::Size& size);

    float& MajorSize(const winrt::Rect& rect);
    float& MinorSize(const winrt::Rect& rect);
    float& MajorStart(const winrt::Rect& rect);
    float MajorEnd(const winrt::Rect& rect) const;
    float& MinorStart(const winrt::Rect& rect);
    float MinorEnd(const winrt::Rect& rect) const;

    winrt::Rect MinorMajorRect(float minor, float major, float minorSize, float majorSize);
    winrt::Point MinorMajorPoint(float minor, float major);
    winrt::Size MinorMajorSize(float minor, float major);

private:
    ScrollOrientation m_orientation { ScrollOrientation::Vertical };
};

