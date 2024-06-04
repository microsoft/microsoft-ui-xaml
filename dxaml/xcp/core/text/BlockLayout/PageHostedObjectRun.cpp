// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PageHostedObjectRun.h"
#include "ParagraphTextSource.h"
#include "ParagraphNode.h"
#include "BlockLayoutHelpers.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      PageHostedObjectRun::PageHostedObjectRun
//
//  Synopsis:
//      Creates an instance of PageHostedObjectRun class.
//
//---------------------------------------------------------------------------
PageHostedObjectRun::PageHostedObjectRun(
    _In_ CInlineUIContainer *pContainer,
    _In_ XUINT32            characterIndex,
    _In_ TextRunProperties *pProperties)
    :
    ObjectRun(characterIndex, pProperties),
    m_pContainer(pContainer)
{
}

PageHostedObjectRun::~PageHostedObjectRun()
{
}

bool PageHostedObjectRun::HasFixedSize() const
{
    return true;
}

//------------------------------------------------------------------------
//  Summary:
//      Measures the embedded element.
// 
//  Remarks:
//      The position passed in to this function does not represent the final embedded
//      element position, so we do not send it to the host (i.e. no calls to
//      <IEmbeddedElementHost::UpdatePosition> here).
//------------------------------------------------------------------------
Result::Enum PageHostedObjectRun::Format(
    _In_ TextSource *pTextSource, 
    _In_  XFLOAT remainingParagraphWidth,
    _In_  const XPOINTF &currentPosition,
    _Out_ ObjectRunMetrics *pMetrics)
{
    HRESULT               hr    = S_OK;
    IEmbeddedElementHost *pHost = NULL;
    CDependencyObject *pParent = NULL;
    bool measureElement = true;
    CUIElement *pElement = NULL;

    IFC(m_pContainer->GetChild(&pElement));
    pHost = pTextSource->GetEmbeddedElementHost();

    // If the host is not the same as the cached host, element needs to be removed from the
    // cached host and added to the current host. However, in certain cases the host
    // may not support reparenting, e.g. if formatting already took place and the inline object is only 
    // being reformatted during arrange/rendering. 
    if (pHost != m_pContainer->GetCachedHost() &&
        pHost->CanAddElement())
    {
        measureElement = TRUE;
        IFC(m_pContainer->EnsureDetachedFromHost());
        IFC(m_pContainer->EnsureAttachedToHost(pHost));
    }
    else
    {
        // If we are called with a different host that cannot support adding elements, we shouldn't
        // remeasure the element at its current host - it's unnecessary and may actually invalidate the host needlessly.
        // In that case we leave the host as is and skip measuring. If the host is the same as the 
        // element's current host, then we can measure without reparenting. 
        // Essentially we will not perform Measure unless the calling host can (or already does) parent the element.
        if (pHost != m_pContainer->GetCachedHost())
        {
            measureElement = FALSE;
        }
        else
        {
            // In CRichTextBox embedded element hosting, embedded elements may be removed from their 
            // host temporarily during formatting due to undo and certain backing store behavior.
            // So even if the host is the same as cached host on InlineUIContainer, it may not
            // actually be attached to the host. In CRichTextBlock/PageHostedObjectNode code path,
            // this can't happen. The host will set the inline container's cached host to NULL 
            // when it's detached, so if it has a cached host, it's parented.
            IFC(GetEmbeddedElementParent(&pParent));
            ASSERT(pParent != NULL);
        }
    }

    // Host can be NULL if we're being measured outside the tree.
    if (pElement && pHost)
    {
        if (measureElement)
        {
            XFLOAT baseline;
            IFC(pElement->Measure(pHost->GetAvailableMeasureSize()));

            // If the child doesn't fit, it will be removed from the parent RichTextBlock after Measure completes.
            // At that point, layout storage is cleared, so cache desired size to ensure a consistent
            // desired size when called outside of measure (e.g., hit testing) when we cannot
            // re-parent the child just to re-measure it.
            IFC(BlockLayoutHelpers::GetElementBaseline(pElement, &baseline));
            m_pContainer->SetChildLayoutCache(pElement->DesiredSize.width, pElement->DesiredSize.height, baseline);
        }

        m_pContainer->GetChildLayoutCache(&pMetrics->width, &pMetrics->height, &pMetrics->baseline);
    }
    else
    {
        memset(pMetrics, 0, sizeof(ObjectRunMetrics));
    }

Cleanup:
    ReleaseInterface(pElement);
    return TxerrFromXResult(hr);
}

Result::Enum PageHostedObjectRun::ComputeBoundingBox(
    _In_  bool   rightToLeft,
    _In_  bool   sideways,
    _Out_ XRECTF *pBounds)
{
    return Result::NotImplemented;
}


//------------------------------------------------------------------------
//  Summary:
//      Retrieves the embedded element's parent. Used to verify 
//      that the element is correctly parented.
//------------------------------------------------------------------------
Result::Enum PageHostedObjectRun::Arrange(
    _In_ const XPOINTF &position
    )
{
    HRESULT hr = S_OK;
    IEmbeddedElementHost *pHost = m_pContainer->GetCachedHost();
    
    if (pHost)
    {
        IFC(pHost->UpdateElementPosition(m_pContainer, position));
    }

Cleanup:
    return TxerrFromXResult(hr);
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the embedded element's parent. Used to verify 
//      that the element is correctly parented.
//------------------------------------------------------------------------
_Check_return_ HRESULT PageHostedObjectRun::GetEmbeddedElementParent(
    _Outptr_ CDependencyObject **ppParent
    )
{
    HRESULT hr = S_OK;
    CUIElement *pElement = NULL;
    *ppParent = NULL;

    IFC(m_pContainer->GetChild(&pElement));

    if (pElement)
    {
        CRichTextBlock *pRichTextBlock = do_pointer_cast<CRichTextBlock>(pElement->GetParentInternal());
        if (pRichTextBlock)
        {
            *ppParent = pRichTextBlock;
        }
        else
        {
            CRichTextBlockOverflow *pRichTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(pElement->GetParentInternal());
            if (pRichTextBlockOverflow)
            {
                *ppParent = pRichTextBlockOverflow;
            }
        }
    }

Cleanup:
    ReleaseInterface(pElement);
    RRETURN(hr);    
}
