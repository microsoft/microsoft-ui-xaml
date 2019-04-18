// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GridLocation.g.h"
#include "GridLocation.properties.h"

class GridLocation :
    public ReferenceTracker<GridLocation, winrt::implementation::GridLocationT>,
    public GridLocationProperties
{
public:
    GridLocation();

    int Index();
    void Index(int const& value);

    winrt::hstring LineName();
    void LineName(winrt::hstring const& value);

    int Span();
    void Span(int const& value);

private:
    int m_index;
    winrt::hstring m_lineName;
    int m_span;
};
