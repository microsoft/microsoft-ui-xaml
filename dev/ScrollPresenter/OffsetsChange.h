// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChange.h"

class OffsetsChange : public ViewChange
{
public:
    OffsetsChange(
        double zoomedHorizontalOffset,
        double zoomedVerticalOffset,
        ScrollPresenterViewKind offsetsKind,
        winrt::IInspectable const& options);
    ~OffsetsChange();

    double ZoomedHorizontalOffset() const
    {
        return m_zoomedHorizontalOffset;
    }

    double ZoomedVerticalOffset() const
    {
        return m_zoomedVerticalOffset;
    }

    void ZoomedHorizontalOffset(double zoomedHorizontalOffset);
    void ZoomedVerticalOffset(double zoomedVerticalOffset);

private:
    double m_zoomedHorizontalOffset{};
    double m_zoomedVerticalOffset{};
};

