// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "BlockNodeBreak.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      BlockNodeBreak::BlockNodeBreak
//
//  Synopsis:
//      Initializes instance of BlockNodeBreak.
//
//---------------------------------------------------------------------------
BlockNodeBreak::BlockNodeBreak(
    _In_ XUINT32 breakIndex
        // Index at which break occurs relative to block start.
    ) :
    m_breakIndex(breakIndex)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      BlockNodeBreak::~BlockNodeBreak
//
//---------------------------------------------------------------------------
BlockNodeBreak::~BlockNodeBreak()
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      BlockNodeBreak::Equals
//
//  Synopsis:
//      Equality operator - used in layout bypass.      
//      Overridden by derived classes based on the contents of their break records/
//  
//  Notes:
//      Base class implementation must be the strictest check lest it causes
//      unintended layout bypass, so this implementation is just reference equals.
//
//---------------------------------------------------------------------------
bool BlockNodeBreak::Equals(
    _In_opt_ BlockNodeBreak *pBreak
    )
{
    return (pBreak == this);
}

//---------------------------------------------------------------------------
//
//  Member:
//      BlockNodeBreak::Equals
//
//  Synopsis:
//      Static helper for equality checking so callers can avoid NULL
//      checks, etc.
//  
//---------------------------------------------------------------------------
bool BlockNodeBreak::Equals(
    _In_opt_ BlockNodeBreak *pFirst,            
    _In_opt_ BlockNodeBreak *pSecond       
    )
{
    return ((pFirst == NULL && pSecond == NULL) ||
            ((pFirst != NULL) &&
             (pSecond != NULL) &&
             pFirst->Equals(pSecond)));
}

