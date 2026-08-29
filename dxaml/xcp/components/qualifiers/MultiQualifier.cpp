// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MultiQualifier.h"
#include <numeric>

// Determines if this qualifier meets its qualifying conditions
_Check_return_ bool MultiQualifier::IsQualified()
{
    if(m_qualifiers.size() == 0) return false;

    // All child qualifiers must meet all their respective qualifying conditions
    auto iter = 
        std::find_if_not(begin(m_qualifiers), end(m_qualifiers), 
                [](std::shared_ptr<IQualifier>& q) { return q->IsQualified(); });

    return (iter == end(m_qualifiers));
}

// Returns the score of a specific context dimension
_Check_return_ XINT32 MultiQualifier::Score(_In_ QualifierFlags qualifierFlags)
{
    // Return the first value that is qualified (not -1)
    XINT32 qualifierValue = -1;
    for(auto& q : m_qualifiers)
    {
        qualifierValue = q->Score(qualifierFlags);
        if(qualifierValue != -1) 
        {
            break;
        }
    }

    return qualifierValue;
}

// Evaluates qualifying conditions
void MultiQualifier::Evaluate(_In_ QualifierContext* pQualifierContext)
{
    std::for_each(begin(m_qualifiers), end(m_qualifiers), [&](std::shared_ptr<IQualifier>& q) { q->Evaluate(pQualifierContext); }); 
};

// Returns all context changes qualifiers need to regsiter for 
_Check_return_ QualifierFlags MultiQualifier::Flags()
{
    QualifierFlags flags = std::accumulate(begin(m_qualifiers), end(m_qualifiers), QualifierFlags::None,
            [](QualifierFlags f, std::shared_ptr<IQualifier>& q) { return (f | q->Flags()); });

    return flags;
}

// Adds a child qualifier
void MultiQualifier::Add(_In_ std::shared_ptr<IQualifier> pQualifier)
{
    m_qualifiers.push_back(pQualifier);
};

