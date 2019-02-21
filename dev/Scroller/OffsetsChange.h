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
        ScrollerViewKind offsetsKind,
        winrt::ScrollOptions const& options);
    ~OffsetsChange();

    double GetZoomedHorizontalOffset() const
    {
        return m_zoomedHorizontalOffset;
    }

    double GetZoomedVerticalOffset() const
    {
        return m_zoomedVerticalOffset;
    }

private:
    double m_zoomedHorizontalOffset{};
    double m_zoomedVerticalOffset{};
};

