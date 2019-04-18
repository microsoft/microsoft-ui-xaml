// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "GridLocation.h"

GridLocation::GridLocation()
{
}

int GridLocation::Index()
{
    return m_index;
}

void GridLocation::Index(int const& value)
{
    m_index = value;
}

winrt::hstring GridLocation::LineName()
{
    return m_lineName;
}

void GridLocation::LineName(winrt::hstring const& value)
{
    m_lineName = value;
}

int GridLocation::Span()
{
    return m_span;
}

void GridLocation::Span(int const& value)
{
    m_span = value;
}
