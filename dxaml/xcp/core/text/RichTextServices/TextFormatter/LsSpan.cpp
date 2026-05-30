// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines LsSpan represented by a plsspan value dispatched to LS.

#include "precomp.h"
#include "LsSpan.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      LsSpan::LsSpan
//
//  Synopsis:
//      Constructor.
//
//---------------------------------------------------------------------------
LsSpan::LsSpan(
    _In_ XUINT32 characterIndex, 
    _In_ LsSpanType::Enum type,
    _In_opt_ LsSpan *pParent
    )
{
    m_characterIndex    = characterIndex;
    m_type              = type;
    XCP_WEAK(&m_pParent);
    m_pParent           = pParent;
    m_pFirstChild       = NULL;
    m_pNextSibling      = NULL;
    m_length            = -1;

    // Update the parent/child/sibling relationship in the tree of spans.
    if (m_pParent != NULL)
    {
        ASSERT(m_pParent->m_characterIndex <= m_characterIndex);
        LsSpan *pLastChild = m_pParent->m_pFirstChild;
        if (pLastChild == NULL)
        {
            m_pParent->m_pFirstChild = this;
        }
        else
        {
            while (pLastChild->m_pNextSibling != NULL)
            {
                pLastChild = pLastChild->m_pNextSibling;
            }

            ASSERT(pLastChild->m_characterIndex <= m_characterIndex);
            pLastChild->m_pNextSibling = this;
        }
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      GetBidiLevel
//
//  Synopsis:
//      Gets bidi embedding level of this span, less Bidi base offset.
//
//  Notes:
//      Spans may be created also to represent objects not related to Bidi
//      handling. In such case, this call may move up the parent tree.
//
//---------------------------------------------------------------------------
XUINT8 LsSpan::GetBidiLevel() const
{

    if (m_type > LsSpanType::BidiBase)
    {
        // Bidi span, return level.
        return  (XUINT8)(m_type - LsSpanType::BidiBase);
    }

    // If not, look up parent. Spans intended as master spans should override this logic.
    ASSERT(m_pParent != NULL);
    return m_pParent->GetBidiLevel();
}

//-------------------------------------------------------------------
//
//  Member:
//      LsSpan::SetLength
//
//  Synopsis:
//      Sets the span's length.
//
//  Notes:
//      A span's length is only known at the time it is closed,
//      so this cannot be passed to ctor.
//  
//-------------------------------------------------------------------
void LsSpan::SetLength(
    _In_ XINT32 length
        // Span's length.
    )
{
    m_length = length;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsSpan::GetOpenChildSpan
//
//  Synopsis:
//      Gets the first child span open at the current cp. If no child span is
//      open, returns null.
//
//  Notes
//      This method is necessary to keep LS run cache data current in case 
//      a new line is started without the previous line's break record.
//      In such a case, if the same run cache is used it may pass span closing
//      data even though LS does not have the correct master span,
//      since this information is normally obtained through the break 
//      record.
//
//---------------------------------------------------------------------------
void LsSpan::GetOpenChildSpan(
    _In_ XUINT32 cpCurrent, 
        // Current cp.
    _Outptr_ LsSpan **ppChildSpan
        // Open child span, if it exists.
        ) const
{
    *ppChildSpan = NULL;

    // This span must be open at the given cp, or there is no chance a child will be.
    // In the case of paragraph spans, the cp may be matched to the span's length, i.e. the
    // the span may close on this CP. This is possible in the case of the previous line breaking
    // just before EOP character and the new line starts at EOP. But this can happen only 
    // for paragraph spans, not for reverse/ embedded objects.
    ASSERT((m_characterIndex <= cpCurrent)                  &&
           (m_length == -1                                  || 
            m_characterIndex + m_length > cpCurrent        ||
           (m_characterIndex + m_length == cpCurrent &&
            m_type == LsSpanType::Paragraph)
            ));

    LsSpan *pChildSpan = m_pFirstChild;

    while (pChildSpan != NULL)
    {
        if ((pChildSpan->m_characterIndex <= cpCurrent)
            && (pChildSpan->m_length == -1
            || (pChildSpan->m_characterIndex + pChildSpan->m_length) > cpCurrent))
        {
            (*ppChildSpan) = pChildSpan;
            break;
        }
        pChildSpan = pChildSpan->m_pNextSibling;
    }
}


