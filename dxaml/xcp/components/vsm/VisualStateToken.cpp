// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualStateToken.h"

VisualStateToken::VisualStateToken() 
    : m_value(std::numeric_limits<size_t>::max())
{};

VisualStateToken::VisualStateToken(size_t tokenValue) 
    : m_value(tokenValue)
{};

VisualStateToken::VisualStateToken(_In_ const CVisualState* tokenValue) 
    : m_value(reinterpret_cast<size_t>(tokenValue))
{};

bool VisualStateToken::operator==(const VisualStateToken& t) 
{ 
    return t.m_value == m_value; 
};

bool VisualStateToken::operator!=(const VisualStateToken& t) 
{ 
    return !(*this == t); 
};

bool VisualStateToken::IsEmpty() const
{
    return m_value == std::numeric_limits<size_t>::max();
};
