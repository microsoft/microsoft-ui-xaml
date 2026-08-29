// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LsTextLineBreak.h"

using namespace Ptls6;
using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLineBreak::LsTextLineBreak
//
//  Synopsis:
//      Constructor.
//
//---------------------------------------------------------------------------
LsTextLineBreak::LsTextLineBreak(
    _In_ PLSC pLineServicesContext,
        // Pointer to LineServices formatting context.
        // Required to delete wrapped LineServices BreakRecord.
    _In_ PLSBREAKRECLINE pBreakRecord
        // Pointer to LineServices BreakRecord wrapped by this object.
    )
{
    ASSERT(pLineServicesContext != NULL);
    ASSERT(pBreakRecord != NULL);

    m_pBreakRecord = pBreakRecord;
    m_pLineServicesContext = pLineServicesContext;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLineBreak::LsTextLineBreak
//
//  Synopsis:
//      Parameterless constructor if no LS break record exists.
//
//---------------------------------------------------------------------------
LsTextLineBreak::LsTextLineBreak()
{
    m_pBreakRecord = NULL;
    m_pLineServicesContext = NULL;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLineBreak::~LsTextLineBreak
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
LsTextLineBreak::~LsTextLineBreak()
{
    if (m_pBreakRecord)
    {
        LsDestroyBreakRecord(m_pLineServicesContext, m_pBreakRecord);
    }
}
