// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LsTextLineBreak.h"
#include "LsTextFormatter.h"

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
    _In_ LsTextFormatter *pTextFormatter,
        // Owner of the LineServices formatting context.
        // Required to delete wrapped LineServices BreakRecord and held with a reference
        // so the context outlives this break record.
    _In_ PLSBREAKRECLINE pBreakRecord
        // Pointer to LineServices BreakRecord wrapped by this object.
    )
{
    ASSERT(pTextFormatter != NULL);
    ASSERT(pBreakRecord != NULL);

    m_pBreakRecord = pBreakRecord;
    // SetInterface assigns and AddRef's in one call, visibly pairing with the
    // ReleaseInterface(m_pTextFormatter) in the destructor so the ref-count balance is
    // easy to verify. Holding a reference keeps the LS context alive until the last
    // cached break record is destroyed.
    SetInterface(m_pTextFormatter, pTextFormatter);
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
    m_pTextFormatter = NULL;
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
    if (m_pTextFormatter != NULL)
    {
        // Invariant: m_pBreakRecord is non-null iff m_pTextFormatter is non-null (both are set
        // together by the two-arg constructor; the parameterless constructor leaves both null and
        // there is no setter). This inner check is therefore defensive - keep it and the invariant
        // in sync if a break-record setter is ever added, so teardown isn't silently keyed off the
        // formatter alone (which would reintroduce a break-record leak).
        if (m_pBreakRecord != NULL)
        {
            LsDestroyBreakRecord(m_pTextFormatter->m_pLsContext, m_pBreakRecord);
        }
        ReleaseInterface(m_pTextFormatter);
    }
}
