// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "BlockLayoutEngine.h"
#include "DrawingContext.h"
#include "BlockNode.h"
#include "ParagraphNode.h"
#include "BlockNodeBreak.h"
#include "PageNode.h"

#include "TextSelectionManager.h"
#include <math.h>
#include <FocusRectManager.h>
#include <TextHighlightRenderer.h>
#include <TextHighlighterCollection.h>
#include "HighlightRegion.h"

#include "RichTextBlockView.h"
#include "LinkedRichTextBlockView.h"
#include "RectUtil.h"
#include "RootScale.h"

using namespace Focus;
using namespace RichTextServices;

CRichTextBlockOverflow::CRichTextBlockOverflow(_In_ CCoreServices *pCore)
    : CFrameworkElement(pCore)
{
    // Fields unique to RichTextBlockOverflow
    m_pMaster = nullptr;
    m_pPreviousLink = nullptr;

    // Common to RTB and RTBO
    m_pOverflowTarget = nullptr;
    m_pBreak = nullptr;
    m_isBreakValid = FALSE;

    // Fields common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    m_redrawForArrange = false;
    m_padding.left = 0.0f;
    m_padding.top = 0.0f;
    m_padding.right = 0.0f;
    m_padding.bottom = 0.0f;
    m_maxLines = 0; // no max lines set
    m_layoutRoundingHeightAdjustment = 0.0f;
    m_selectionHighlight = nullptr;

    m_pTextView = nullptr;
    m_pPageNode = nullptr;

    if (!pCore->IsInBackgroundTask())
    {
        IGNOREHR(SetCursor(MouseCursorIBeam));
    }
}

CRichTextBlockOverflow::~CRichTextBlockOverflow()
{
    delete m_pPageNode;

    // Linked container disconnect.
    ReleaseInterface(m_pBreak);
    m_isBreakValid = FALSE;

    if (m_pOverflowTarget != nullptr)
    {
        IGNOREHR(RichTextServicesHelper::MapTxErr(m_pOverflowTarget->PreviousLinkDetached(this)));
    }
    ReleaseInterface(m_pOverflowTarget);

    delete m_pTextView;
}

//------------------------------------------------------------------------
//
//  Summary:
//      Gets a value indicating whether this element is laid
//      out from RightToLeft.
//
//  Notes:
//      The overflow element should use the master's FlowDirection to
//      determine this if there is one.
//
//------------------------------------------------------------------------
bool CRichTextBlockOverflow::IsRightToLeft()
{
    if (m_pMaster != nullptr)
    {
        return m_pMaster->IsRightToLeft();
    }
    else
    {
        return CFrameworkElement::IsRightToLeft();
    }
}

_Check_return_ HRESULT CRichTextBlockOverflow::EnterImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    EnterParams params
    )
{
    IFC_RETURN(CFrameworkElement::EnterImpl(pNamescopeOwner, params));

    if (params.fIsLive)
    {
        InvalidateAllOverflowContentMeasure(this);
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::LeaveImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    LeaveParams params
    )
{
    if (m_pMaster != nullptr && m_pMaster->GetSelectionManager() != nullptr)
    {
        m_pMaster->GetSelectionManager()->RemoveCaretFromTextObject(this);
    }

    if (IsFocused() && GetContext())
    {
        CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(this);

        if(pFocusManager)
        {
            // Set the focus on the next focusable element.
            // If we remove the currently focused element from the live tree, inside a GettingFocus or LosingFocus handler,
            // we failfast. This is being tracked by Bug 9840123
            // Use InputActivationBehavior::NoActivate because removing this element from the tree shouldn't steal activation from another window/island.
            IFCFAILFAST(pFocusManager->SetFocusOnNextFocusableElement(GetFocusState(), !params.fVisualTreeBeingReset, InputActivationBehavior::NoActivate));
        }

        // Ensure this control doesn't have a focus
        UpdateFocusState(DirectUI::FocusState::Unfocused);
    }

    IFC_RETURN(CFrameworkElement::LeaveImpl(pNamescopeOwner, params));

    if (params.fIsLive)
    {
        // Cleanup all the device related resources.
        if (m_pPageNode != nullptr)
        {
            m_pPageNode->CleanupRealizations();
        }

        InvalidateAllOverflowContentMeasure(this);
    }

    return S_OK;
}

CHyperlink* CRichTextBlockOverflow::GetCurrentLinkNoRef() const
{
    return m_currentLink.get();
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::SetValue
//
//  Synopsis: Records a property value change.
//
//  Special handling:
//      1. OverflowTarget copied directly to field bypassing property
//         system which would set up unwanted association/parenting.
//      2. ContentMeasure/Arrange invalidated based on property change.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::SetValue(_In_ const SetValueParams& args)
{
    CRichTextBlockOverflow *pOverflowTargetOldValue = m_pOverflowTarget;
    CValue valueOverflowTarget;

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::UIElement_IsTabStop:
        // RichTextBlock handles tab focus, explicitly disallow tab related APIs for RichTextBlockOverflow
        ::RoOriginateError(E_INVALIDARG, wrl_wrappers::HStringReference(L"RichTextBlockOverflow does not support the IsTabStop property.  The content source RichTextBlock should be used instead.").Get());
        IFC_RETURN(E_INVALIDARG);
        break;
    case KnownPropertyIndex::UIElement_TabIndex:
        // RichTextBlock handles tab focus, explicitly disallow tab related APIs for RichTextBlockOverflow
        ::RoOriginateError(E_INVALIDARG, wrl_wrappers::HStringReference(L"RichTextBlockOverflow does not support the TabIndex property.  The content source RichTextBlock should be used instead.").Get());
        IFC_RETURN(E_INVALIDARG);
        break;

    case KnownPropertyIndex::RichTextBlockOverflow_MaxLines:
        {
            if (args.m_value.GetType() == valueSigned)
            {
                if (args.m_value.AsSigned() < 0)
                {
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            else
            {
                const CInt32* pInt32 = nullptr;
                if (FAILED(DoPointerCast(pInt32, args.m_value)) ||
                    pInt32->m_iValue < 0)
                {
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            break;
        }
    }

    // SetValue for OverflowContentTarget is a special case since we need the local field to be written, but not pass through the property system, otherwise the multiple association code
    // will not allow the property value to be set. We don't want to allow multiple associations for the target element since that will affect its parenting, so we bypass the SetValue call on
    // DO and just write the field ourselves here.
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::RichTextBlockOverflow_OverflowContentTarget)
    {
        if (args.m_value.GetType() == valueNull || args.m_value.GetType() == valueObject)
        {
            IFC_RETURN(InvalidateNextLink());

            if (args.m_value.GetType() == valueNull)
            {
                m_pOverflowTarget = nullptr;
            }
            else
            {
                // Check for a cycle and error out if one is detected.
                if (args.m_value.AsObject()->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>() &&
                    ValidateNextLink(do_pointer_cast<CRichTextBlockOverflow>(args.m_value.AsObject())))
                {
                    m_pOverflowTarget = do_pointer_cast<CRichTextBlockOverflow>(args.m_value.AsObject());
                    AddRefInterface(m_pOverflowTarget);
                }
                else
                {
                    IFC_RETURN(E_INVALIDARG);
                }
            }

            // SetPeerReferenceToProperty is normally done as part of DO::SetValue.
            // Since we cannot call DO::SetValue, SetPeerReferenceToProperty needs to be called here.
            valueOverflowTarget.WrapObjectNoRef(m_pOverflowTarget);
            IFC_RETURN(SetPeerReferenceToProperty(args.m_pDP, valueOverflowTarget));

            IFC_RETURN(ResolveNextLink());

            if ((m_pOverflowTarget != nullptr && pOverflowTargetOldValue == nullptr) ||
                (m_pOverflowTarget == nullptr && pOverflowTargetOldValue != nullptr))
            {
                // If switching overflow target from non-null to null or vice versa, it may affect rendering of
                // paragraph ellipsis if TextTrimming is on. Invalidate arrange/render so that ellipsis rendering can be updated.
                // There is no need to invalidate all subsequent overflows since only trimming rendering for this element is affected.
                // Breaks are never affected.
                InvalidateContentArrange();

                // Even though overflow target is marked as AffectsArrange in the SLOM, Arrange is not invalidated by the property system when it changes because the property system is
                // bypassed for setting this property due to multiple associations code. So Arrange on the element needs to be explicitly invalidated here.
                InvalidateArrange();
            }
        }
        else
        {
            IFC_RETURN(E_INVALIDARG);
        }
    }
    else
    {
        IFC_RETURN(CFrameworkElement::SetValue(args));

        // Handle property settings
        switch (args.m_pDP->GetIndex())
        {
        case KnownPropertyIndex::RichTextBlockOverflow_Padding:
        case KnownPropertyIndex::RichTextBlockOverflow_MaxLines:
            // Invalidate measure of this and all subsequent overflows.
            CRichTextBlockOverflow::InvalidateAllOverflowContentMeasure(this);
            break;
        }
    }

    return S_OK;
}

void CRichTextBlockOverflow::OnChildDesiredSizeChanged(_In_ CUIElement* pElement)
{
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->OnChildDesiredSizeChanged(pElement);
    }
    m_isBreakValid = FALSE;
    CFrameworkElement::OnChildDesiredSizeChanged(pElement);
}

_Check_return_ HRESULT CRichTextBlockOverflow::GetActualWidth(_Out_ float* pWidth)
{
    *pWidth = m_pPageNode ? m_pPageNode->GetDesiredSize().width : 0.0f;
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::GetActualHeight(_Out_ float* pHeight)
{
    *pHeight = m_pPageNode ? m_pPageNode->GetDesiredSize().height : 0.0f;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::GetBaselineOffset
//
//  Synopsis: Gets distance to baseline of first line from top of control.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::GetBaselineOffset(_Out_ XFLOAT *pBaselineOffset)
{
    *pBaselineOffset = 0;
    if (m_pPageNode != nullptr)
    {
        *pBaselineOffset  = m_pPageNode->GetBaselineAlignmentOffset();
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Summary: Called by JupiterWindowMobile when Phone copy button is pressed
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::CopySelectedText()
{
    if (m_pMaster != nullptr &&
        m_pMaster->GetSelectionManager() != nullptr)
    {
        IFC_RETURN(m_pMaster->GetSelectionManager()->CopySelectionToClipboard());
    }
    return S_OK;
}

uint32_t CRichTextBlockOverflow::GetContentStartPosition() const
{
    if (m_pPageNode != nullptr &&
        !m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        ASSERT(m_pMaster != nullptr);
        return m_pPageNode->GetStartPosition();
    }
    return 0;
}

uint32_t CRichTextBlockOverflow::GetContentLength() const
{
    if (m_pPageNode != nullptr &&
        !m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        // If there is no overflow target, the content length should extend up to the end of the TextContainer. However,
        // this is true only if there is a master (which we can assume if there's a PageNode). We require PageNode's Measure
        // to be valid since PageNode will be deleted in MeasureCore if there is no content overflowing to this element.
        // Having a measured PageNode means that it is valid and has content, so layout metrics can be reliably accessed.
        ASSERT(m_pMaster != nullptr);
        if (m_pOverflowTarget == nullptr)
        {
            if (m_pMaster->GetTextContainer() != nullptr)
            {
                uint32_t contentStart = GetContentStartPosition();
                uint32_t containerLength = 0;
                m_pMaster->GetTextContainer()->GetPositionCount(&containerLength);
                ASSERT(containerLength >= contentStart);
                return (containerLength - contentStart);
            }
        }
        else
        {
            // If there is an overflow target, return only the content that fits on the page.
            return m_pPageNode->GetContentLength();
        }
    }

    // If PageNode is not valid or we cannot get the text container's length, assume 0 length for this element.
    return 0;
}

RichTextServices::ILinkedTextContainer *CRichTextBlockOverflow::GetPrevious() const
{
    return m_pPreviousLink;
}

RichTextServices::ILinkedTextContainer *CRichTextBlockOverflow::GetNext() const
{
    return m_pOverflowTarget;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRichTextBlockOverflow::GetBreak
//
//---------------------------------------------------------------------------
RichTextServices::TextBreak *CRichTextBlockOverflow::GetBreak() const
{
    // In CRichTextBlock linking implementation, another container should only request the break if it is valid.
    ASSERT(m_isBreakValid == TRUE);
    return m_pBreak;
}

RichTextServices::Result::Enum CRichTextBlockOverflow::PreviousBreakUpdated(
    _In_ RichTextServices::ILinkedTextContainer *pPrevious
    )
{
    ASSERT(m_pPreviousLink != nullptr &&
           m_pPreviousLink == pPrevious);

    InvalidateMeasure();
    return Result::Success;
}

RichTextServices::Result::Enum CRichTextBlockOverflow::PreviousLinkAttached(
    _In_ RichTextServices::ILinkedTextContainer *pPrevious
    )
{
    Result::Enum txhr = Result::Success;

    // If there's already a previous link, notify it that this element is detaching itself.
    if (m_pPreviousLink != nullptr)
    {
        IFCTEXT(m_pPreviousLink->NextLinkDetached(this));
    }

    // Set this and all subsequent masters to NULL, they'll be re evaluated when measured.
    ResetAllOverflowMasters(this);

    m_pPreviousLink = pPrevious;

    InvalidateMeasure();

Cleanup:
    return txhr;
}

RichTextServices::Result::Enum CRichTextBlockOverflow::NextLinkDetached(
    _In_ RichTextServices::ILinkedTextContainer *pNext
    )
{
    Result::Enum txhr = Result::Success;

    ASSERT(m_pOverflowTarget != nullptr &&
           m_pOverflowTarget == pNext);

    ReleaseInterface(m_pOverflowTarget);

    return txhr;
}

RichTextServices::Result::Enum CRichTextBlockOverflow::PreviousLinkDetached(
    _In_ RichTextServices::ILinkedTextContainer *pPrevious
    )
{
    // Even though we don't expose next link publicly, we may need to call this e.g. if previous link
    // is deleted.
    Result::Enum txhr = Result::Success;

    ASSERT(m_pPreviousLink != nullptr &&
           m_pPreviousLink == pPrevious);

    // Set this and all subsequent masters to NULL, they'll be re evaluated when measured.
    ResetAllOverflowMasters(this);

    m_pPreviousLink = nullptr;

    InvalidateMeasure();

    return txhr;
}

//------------------------------------------------------------------------
//
//  Method:  CRichTextBlockOverflow::GetTextElementBoundRect
//
//  Summary: Calculate bounding rectangles for the specifed TextElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::GetTextElementBoundRect(
    _In_ CTextElement *pElement,
    _Out_ XRECTF *pRectFocus,
    _In_ bool ignoreClip
    )
{
    IFC_RETURN(CRichTextBlock::GetTextElementBoundRect(pElement, m_pTextView, this, pRectFocus, ignoreClip));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  CRichTextBlockOverflow::GetTextElementBoundRect
//
//  Summary: Calculate bounding rectangles for the specifed TextElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::HitTestLink(
    _In_ XPOINTF *ptPointer,
    _Outptr_ CHyperlink **ppLink
    )
{
    CHyperlink *pLink = nullptr;
    *ppLink = nullptr;
    IFC_RETURN(CRichTextBlock::HitTestLink(
        m_pMaster->m_pBlocks->GetTextContainer(),
        this,
        ptPointer,
        m_pTextView,
        &pLink));
    *ppLink = pLink;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::InvalidateAllOverflowContent
//
//  Synopsis: Invalidates content on all subsequent overflow elements,
//            starting at pFirst
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::InvalidateAllOverflowContent(_In_opt_ CRichTextBlockOverflow *pFirst, const bool clearCachedLinks)
{
    CRichTextBlockOverflow *pOverflow;
    for (pOverflow = pFirst; pOverflow != nullptr; pOverflow = pOverflow->m_pOverflowTarget)
    {
        pOverflow->InvalidateContent(clearCachedLinks);
    }
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::InvalidateAllOverflowContentMeasure
//
//  Synopsis: Invalidates content measure on all subsequent overflow elements,
//            starting at pFirst
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::InvalidateAllOverflowContentMeasure(_In_opt_ CRichTextBlockOverflow *pFirst)
{
    CRichTextBlockOverflow *pOverflow;
    for (pOverflow = pFirst; pOverflow != nullptr; pOverflow = pOverflow->m_pOverflowTarget)
    {
        pOverflow->InvalidateContentMeasure();
    }
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::InvalidateAllOverflowContentArrange
//
//  Synopsis: Invalidates content arrange on all subsequent overflow elements,
//            starting at pFirst
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::InvalidateAllOverflowContentArrange(_In_opt_ CRichTextBlockOverflow *pFirst)
{
    CRichTextBlockOverflow *pOverflow;
    for (pOverflow = pFirst; pOverflow != nullptr; pOverflow = pOverflow->m_pOverflowTarget)
    {
        pOverflow->InvalidateContentArrange();
    }
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::InvalidateAllOverflowRender
//
//  Synopsis: Invalidates render on all subsequent overflow elements,
//            starting at pFirst
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::InvalidateAllOverflowRender(_In_opt_ CRichTextBlockOverflow *pFirst)
{
    CRichTextBlockOverflow *pOverflow;
    for (pOverflow = pFirst; pOverflow != nullptr; pOverflow = pOverflow->m_pOverflowTarget)
    {
        pOverflow->InvalidateRender();
    }
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::ResetAllOverflowMasters
//
//  Synopsis: Resets master on all subsequent overflow elements,
//            starting at pFirst
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::ResetAllOverflowMasters(_In_opt_ CRichTextBlockOverflow *pFirst)
{
    CRichTextBlockOverflow *pOverflow;
    for (pOverflow = pFirst; pOverflow != nullptr; pOverflow = pOverflow->m_pOverflowTarget)
    {
        pOverflow->ResetMaster();
    }
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::NotifyAllOverflowContentSelectionChanged
//
//  Synopsis: Notifies all overflow elements that selection has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::NotifyAllOverflowContentSelectionChanged(
    _In_opt_ CRichTextBlockOverflow *pFirst,
    _In_ uint32_t previousSelectionStartOffset,
    _In_ uint32_t previousSelectionEndOffset,
    _In_ uint32_t newSelectionStartOffset,
    _In_ uint32_t newSelectionEndOffset
    )
{
    CRichTextBlockOverflow *pOverflow;
    for (pOverflow = pFirst; pOverflow != nullptr; pOverflow = pOverflow->m_pOverflowTarget)
    {
        IFC_RETURN(pOverflow->OnSelectionChanged(
            previousSelectionStartOffset,
            previousSelectionEndOffset,
            newSelectionStartOffset,
            newSelectionEndOffset));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::NotifyAllOverflowContentSelectionVisibilityChanged
//
//  Synopsis: Notifies all overflow elements that selection visibility has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::NotifyAllOverflowContentSelectionVisibilityChanged(
    _In_opt_ CRichTextBlockOverflow *pFirst,
    _In_ uint32_t selectionStartOffset,
    _In_ uint32_t selectionEndOffset
    )
{
    CRichTextBlockOverflow *pOverflow;
    for (pOverflow = pFirst; pOverflow != nullptr; pOverflow = pOverflow->m_pOverflowTarget)
    {
        IFC_RETURN(pOverflow->OnSelectionVisibilityChanged(
            selectionStartOffset,
            selectionEndOffset));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::HasOverflowContent
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::HasOverflowContent(
    _In_ CDependencyObject *pObject,
    _In_ uint32_t cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    CRichTextBlockOverflow *pRichTextBlockOverflow = nullptr;

    pRichTextBlockOverflow = static_cast<CRichTextBlockOverflow *>(pObject);

    if (!pRichTextBlockOverflow ||
        cArgs != 0 ||
        !pResult)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    pResult->SetBool(!!pRichTextBlockOverflow->HasOverflowContent());
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::GetContentStart(
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CPlainTextPosition textPosition;
    uint32_t contentStartPosition = 0;

    *ppTextPointerWrapper = nullptr;

    // If there is no PageNode, return NULL for ContentStart/End. It means this element was either not laid out yet
    // or has no content overflowed from its master and overflow elements don't have their own content.
    if (m_pPageNode != nullptr)
    {
        // If the master has no TextContainer it would not overflow to this element, so we don't need create implicit
        // TextContainer for the master here. It's enough to return NULL if container is NULL.
        if (m_pMaster != NULL &&
            m_pMaster->GetTextContainer() != nullptr)
        {
            // CRichTextBlockOverflow's ContentStart always has forward gravity since backward gravity would belong in the previous link.
            contentStartPosition = GetContentStartPosition();
            textPosition = CPlainTextPosition(m_pMaster->GetTextContainer(), contentStartPosition, LineForwardCharacterForward);
            IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::GetContentEnd(
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CPlainTextPosition textPosition;
    uint32_t contentEndPosition = 0;
    TextGravity gravity = LineForwardCharacterBackward;

    *ppTextPointerWrapper = nullptr;

    // If there is no PageNode, return NULL for ContentStart/End. It means this element was either not laid out yet
    // or has no content overflowed from its master and overflow elements don't have their own content.
    if (m_pPageNode != nullptr)
    {
        // If the master has no TextContainer it would not overflow to this element, so we don't need create implicit
        // TextContainer for the master here. It's enough to return NULL if container is NULL.
        if (m_pMaster != NULL &&
            m_pMaster->GetTextContainer() != nullptr)
        {
            // ContentEnd position is always the position just after the end of content. If CRichTextBlock has a break,
            // its gravity is backward since the forward gravity position will be in the next link. If there is no break the gravity is forward
            // since it's the end of all content.
            contentEndPosition = GetContentStartPosition() + GetContentLength();
            if (m_pBreak == nullptr)
            {
                gravity = LineForwardCharacterForward;
            }

            textPosition = CPlainTextPosition(m_pMaster->GetTextContainer(), contentEndPosition, gravity);
            IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Public hit-test API to get a TextPosition from point
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::GetTextPositionFromPoint(
    _In_ XPOINTF point,
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper)
{
    CPlainTextPosition textPosition;
    uint32_t position = 0;
    TextGravity gravity = LineForwardCharacterForward;

    *ppTextPointerWrapper = nullptr;

    // Use this element's standalone view to query the pixel position.
    if (m_pMaster != nullptr &&
        m_pTextView != nullptr)
    {
        // No coordinate transformation is necessary - this API is assumed to be called w/ element-relative coordinates.
        IFC_RETURN(m_pTextView->PixelPositionToTextPosition(
            point,
            FALSE, // Recognise hits after newline.
            &position,
            &gravity));

        if (m_pMaster->GetTextContainer() != nullptr)
        {
            textPosition = CPlainTextPosition(m_pMaster->GetTextContainer(), position, gravity);
            IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::MeasureOverride
//
//  Synopsis: Returns the desired size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::MeasureOverride(
    XSIZEF availableSize,
    XSIZEF& desiredSize
    )
{
    desiredSize.width = 0.0f;
    desiredSize.height = 0.0f;
    xref_ptr<RichTextBlockBreak> pBreak;
    BlockNodeBreak *pOldPageBreak  = nullptr;

    // Ensure any embedded UIElements are measured. If an embedded UIElement is dirty for measure, the
    // RichTextBlockOverflow will be put on the measure path. Typically, this doesn't result in MeasureOverride being
    // called - instead CUIElement::Measure will skip down to the dirty children and just Measure them. However,
    // in situations where the RichTextBlockOverflow itself is also dirty for measure, the base implementation will not
    // walk to the children - it's up to this override to do so.
    auto children = GetUnsortedChildren();
    UINT32 childrenCount = children.GetCount();
    for (uint32_t i = 0; i < childrenCount; i++)
    {
        // Children are measured with their previous constraints, which come from the BlockNode host.
        // If the constraints change, the blocks will be reformatted (in Measure or Arrange) and the child
        // will be re-measured there via the InlineUIContainer hosting it.
        // If none of the text constaints changed, but the UIElement did, it needs to be measured here.
        // If the size changes, OnChildDesiredSizeChanged will invalidate the block layout for reformatting.
        CUIElement *pChildNoRef = children[i];
        if (pChildNoRef->GetRequiresMeasure())
        {
            IFC_RETURN(pChildNoRef->EnsureLayoutStorage());
            if (pChildNoRef->OfTypeByIndex<KnownTypeIndex::CaretBrowsingCaret>())
            {
                // The CaretBrowsingCaret just needs to be measured with the current available size
                IFC_RETURN(pChildNoRef->Measure(availableSize));
            }
            else
            {
                IFC_RETURN(pChildNoRef->Measure(pChildNoRef->PreviousConstraint));
            }
        }
    }

    IFC_RETURN(SetupLinkedBlockLayout());

    // Always use the RichTextBlockBreak object to retrive the old page break. If the PageNode has not been
    // deleted it should be exactly the same as the the page node's break.
    if (m_pBreak != nullptr)
    {
        pOldPageBreak = m_pBreak->GetBlockBreak();
    }

    if (m_pPageNode != nullptr)
    {
        // We shouldn't be able to create a page node without a previous link.
        ASSERT(m_pPreviousLink != nullptr);

        if (IsPreviousBreakValid())
        {
            RichTextBlockBreak *pPreviousBreak = GetPreviousBreak();
            if (pPreviousBreak)
            {
                // No RichTextBlockBreak should exist without a block break from the BlockLayoutEngine.
                ASSERT(pPreviousBreak->GetBlockBreak() != nullptr);
                pOldPageBreak = m_pPageNode->GetBreak();
                IFC_RETURN(m_pPageNode->Measure(availableSize, m_maxLines, 0.0f, FALSE, FALSE, TRUE, pPreviousBreak->GetBlockBreak(), nullptr));
                desiredSize = m_pPageNode->GetDesiredSize();

                // If PageNode bypasses Measure, its break will be exactly the same,
                // and there's no need to notify overflow elements, etc. in this case.
                // If it's not the same update the container's break and set it, notifying overflows.
                // We use ref equals here and not BlockNodeBreak::Equals because we want RichTextBlockBreak to contain
                // the page node's actual break, not someting that's semantically the same. If the PageNode
                // didn't bypass measure and produced a new break that evaluates the same using BlockNodeBreak::Equals,
                // we'll still replace our break, but that's OK - block layout should detect this on bypass checks.
                if (m_pPageNode->GetBreak() != nullptr)
                {
                    if (m_pPageNode->GetBreak() != pOldPageBreak)
                    {
                        // Create break for this container.
                        pBreak = make_xref<RichTextBlockBreak>(m_pPageNode->GetBreak());
                        IFC_RETURN(SetBreak(pBreak.get()));
                    }
                }
                else
                {
                    if (pOldPageBreak != nullptr)
                    {
                        IFC_RETURN(SetBreak(nullptr));
                    }
                }
            }
            else
            {
                // There's no content here - delete the page node.
                // This should not introduce too much overhead given that the page node will only be deleted/reallocated if
                // the master is reset or if content reduces to the point where nothing overflows to it. This is not really
                // a critical path.
                delete m_pPageNode;
                delete m_pTextView;
                m_pPageNode = nullptr;
                m_pTextView = nullptr;
                IFC_RETURN(SetBreak(nullptr));
            }
        }
    }

    if (GetUseLayoutRounding())
    {
        // In order to prevent text clipping as a result of layout rounding at
        // scales other than 1.0x, the ceiling of the rescaled size is used.
        const float plateauScale = RootScale::GetRasterizationScaleForElement(this);
        XSIZEF pageNodeSize = desiredSize;
        desiredSize.width = XcpCeiling(pageNodeSize.width * plateauScale) / plateauScale;
        desiredSize.height = XcpCeiling(pageNodeSize.height * plateauScale) / plateauScale;

        // LsTextLine is not aware of layoutround and uses baseline height to place the rendered text.
        // However, because the height of the *block is potentionally layoutround-ed up, we should adjust the
        // placement of text by the difference.  Horizontal adjustment is not of concern since
        // LsTextLine uses arranged size which is already layoutround-ed.
        m_layoutRoundingHeightAdjustment = desiredSize.height - pageNodeSize.height;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::ArrangeOverride
//
//  Synopsis: Returns the final render size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::ArrangeOverride(
    XSIZEF finalSize,
    XSIZEF& newFinalSize
    )
{
    XSIZEF renderSize = {0.0f, 0.0f};

    if (m_pPageNode != nullptr &&
        !m_pPageNode->IsMeasureDirty())
    {
        ASSERT(!IsInfiniteF(finalSize.width));
        ASSERT(!IsInfiniteF(finalSize.height));

        IFC_RETURN(m_pPageNode->Arrange(finalSize));
        renderSize = m_pPageNode->GetRenderSize();

        std::shared_ptr<HighlightRegion> selection;
        if (m_pMaster != nullptr &&
            m_pTextView != nullptr &&
            m_pMaster->GetSelectionManager() != nullptr &&
            m_pMaster->GetSelectionManager()->IsSelectionVisible())
        {
            m_pMaster->GetSelectionManager()->GetSelectionHighlightRegion(UseHighContrastSelection(GetContext()), selection);
        }

        // If no selection, no additional processing needs to be done for
        // selection rendering. Proceed with regular text rendering.
        if (!selection)
        {
            // Render page node which will communicate render data for text to the drawing context.
            // There's no need to force re-rendering here, since Arrange was just called on PageNode and Arrange bypass
            // rules apply to render in this case.
            IFC_RETURN(m_pPageNode->Draw(FALSE));
            CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
        }
        else
        {
            m_redrawForArrange = true;  // Force redraw during render pass
        }


        IFC_RETURN(UpdateIsTextTrimmed());
    }

    // If we lose the pageNode and text was previously trimmed, we need to
    // account for it not being trimmed any more
    if (m_pPageNode == NULL && m_isTextTrimmed)
    {
        m_isTextTrimmed = false;
        RaiseIsTextTrimmedChangedEvent();

        CValue oldValue;
        oldValue.Set<valueBool>(true);
        CValue newValue;
        newValue.Set<valueBool>(m_isTextTrimmed);
        IFC_RETURN(NotifyPropertyChanged(
            PropertyChangedParams(
                DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RichTextBlockOverflow_IsTextTrimmed),
                oldValue,
                newValue
            )));
    }

    // We want to return the larger of render size returned by Arrange and
    // finalSize passed by the parent. This is done to make sure that content is
    // positioned correctely if the actual width/height is smaller than finalSize,
    // or clipped correctly if the width is greater than finalSize.
    newFinalSize.width  = MAX(finalSize.width, renderSize.width);
    newFinalSize.height = MAX(finalSize.height, renderSize.height);

    // Forcefully arrange the CaretBrowsingCaret at (0,0) as we'll use a Translation to position it
    auto children = GetUnsortedChildren();
    UINT32 childrenCount = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < childrenCount; childIndex++)
    {
        CUIElement* currentChild = children[childIndex];
        if (currentChild->OfTypeByIndex<KnownTypeIndex::CaretBrowsingCaret>())
        {
            XRECTF arrangeRect;
            arrangeRect.X = 0;
            arrangeRect.Y = 0;
            arrangeRect.Width = currentChild->GetLayoutStorage()->m_desiredSize.width;
            arrangeRect.Height = currentChild->GetLayoutStorage()->m_desiredSize.height;
            IFC_RETURN(currentChild->Arrange(arrangeRect));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  UpdateIsTextTrimmed
//
//  Synopsis: Update whether the text of the block has been trimmed or
//            not and raise an event if the state of trimming has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::UpdateIsTextTrimmed()
{
    auto last_isTextTrimmed = m_isTextTrimmed;

    bool isTrimmed = false;

    ParagraphNode *blockChild = static_cast<ParagraphNode *>(m_pPageNode->GetFirstChild());
    while (blockChild != nullptr)
    {
        isTrimmed = blockChild->GetHasTrimmedLine();
        if (isTrimmed)
        {
            break;
        }
        blockChild = static_cast<ParagraphNode *>(blockChild->GetNext());
    }
    m_isTextTrimmed = isTrimmed;

    if (last_isTextTrimmed != m_isTextTrimmed)
    {
        RaiseIsTextTrimmedChangedEvent();

        CValue oldValue;
        oldValue.Set<valueBool>(last_isTextTrimmed);
        CValue newValue;
        newValue.Set<valueBool>(m_isTextTrimmed);
        IFC_RETURN(NotifyPropertyChanged(
            PropertyChangedParams(
                DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RichTextBlockOverflow_IsTextTrimmed),
                oldValue,
                newValue
            )));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::RaiseIsTextTrimmedChangedEvent
//
//  Synopsis: Raises event for IsTextTrimmedChanged.
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::RaiseIsTextTrimmedChangedEvent()
{
    CEventManager *const eventManager = GetContext()->GetEventManager();
    // Create the DO that represents the event args.
    xref_ptr<CIsTextTrimmedChangedEventArgs> args;
    args.init(new CIsTextTrimmedChangedEventArgs());
    // Raise event.
    eventManager->Raise(EventHandle(KnownEventIndex::RichTextBlockOverflow_IsTextTrimmedChanged), true, this, args);
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::InvalidateContent
//
//  Synopsis: Invalidates layout and stored information about content,
//            e.g. run caches.
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::InvalidateContent(const bool clearCachedLinks)
{
    // Invalidate content for the block layout engine, which invalidates all layout as well.
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->InvalidateContent();
    }

    if (clearCachedLinks)
    {
        m_currentLink.reset();
        m_pressedHyperlink.reset();
    }

    InvalidateMeasure();
    InvalidateRender();
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::InvalidateContentMeasure
//
//  Synopsis: Invalidate layout for all content, including
//            arrange/render info.
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::InvalidateContentMeasure()
{
    // Invalidate measure for the block layout engine, which invalidates arrange as well.
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->InvalidateMeasure();
    }

    m_isBreakValid = FALSE;

    InvalidateMeasure();
    InvalidateRender();
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::InvalidateContentArrange
//
//  Synopsis: Invalidate arrange and render data for content.
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::InvalidateContentArrange()
{
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->InvalidateArrange();
    }

    InvalidateArrange();
    InvalidateRender();
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::InvalidateRender
//
//  Synopsis: Invalidates rendering data and releases render cache.
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::InvalidateRender()
{
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
}

//------------------------------------------------------------------------
//
//  Method:    CRichTextBlockOverflow::OnCreateAutomationPeerImpl
//
//  Synopsis:  Creates and returns CAutomationPeer associated with this
//             CRichTextBlockOverflow
//
//------------------------------------------------------------------------
CAutomationPeer* CRichTextBlockOverflow::OnCreateAutomationPeerImpl()
{
    // Automation peer is implemented in managed code in Silverlight.
    return nullptr;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::ResetMaster
//
//  Synopsis: Resets master and deletes existing page node, since it was
//            created using the previous master's BLE.
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::ResetMaster()
{
    m_pMaster = nullptr;
    delete m_pPageNode;
    delete m_pTextView;
    m_pPageNode = nullptr;
    m_pTextView = nullptr;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::IsPreviousBreakValid
//
//  Synopsis: Queries the previous link to determine whether it has a valid break record.
//
//------------------------------------------------------------------------
bool CRichTextBlockOverflow::IsPreviousBreakValid() const
{
    if (m_pPreviousLink != nullptr)
    {
        return m_pPreviousLink->IsBreakValid();
    }
    return false;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::GetPreviousBreak
//
//  Synopsis: Obtains break record from the previous link.
//
//------------------------------------------------------------------------
RichTextBlockBreak *CRichTextBlockOverflow::GetPreviousBreak() const
{
    if (m_pPreviousLink != nullptr)
    {
        return static_cast<RichTextBlockBreak *>(m_pPreviousLink->GetBreak());
    }
    return nullptr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRichTextBlockOverflow::SetBreak
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::SetBreak(
    _In_ RichTextBlockBreak *pBreak
    )
{
    bool hasBreak = (m_pBreak != nullptr);
    bool hasNewBreak = (pBreak != nullptr);

    ReplaceInterface(m_pBreak, pBreak);
    m_isBreakValid = TRUE;

    if (hasBreak != hasNewBreak)
    {
        // Since HasOverflowContent returns (m_pBreak != NULL), fire a property
        // changed event if the value of m_pBreak has changed between null and
        // non-null
        CValue value;
        IFC_RETURN(HasOverflowContent(this, 0, /* pArgs */ nullptr, /* pValueOuter */ nullptr, &value));
        IFC_RETURN(NotifyPropertyChanged(
            PropertyChangedParams(
            DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RichTextBlockOverflow_HasOverflowContent),
            CValue(),
            value)));
    }

    // Notify the next link that the break has been updated.
    if (m_pOverflowTarget != nullptr)
    {
        IFC_RETURN(RichTextServicesHelper::MapTxErr(m_pOverflowTarget->PreviousBreakUpdated(this)));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::SetupLinkedBlockLayout
//
//  Synopsis: Queries the master for the block layout engine necessary for
//            layout.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::SetupLinkedBlockLayout()
{
    HRESULT hr = S_OK;
    BlockNode *pBlockNode = nullptr;

    // If there is no existing page node, use the master's block layout engine to create one.
    // If there is a page node, assume that it is valid to use i.e. that it was created by the
    // current master's block layout. It's up to linking logic elsewhere in this class to
    // delete a page node if the master is reset.
    if (m_pPageNode == nullptr)
    {
        if (m_pMaster == nullptr &&
            m_pPreviousLink != nullptr)
        {
            // The previous link either has or is a master. If it is not a master and hasn't
            // resolved its master yet, it will need to do so to Measure, at which point it will
            // notify this link of break updated. So don't ever look further than the previous link for the master.
            if (m_pPreviousLink->IsMaster())
            {
                m_pMaster = static_cast<CRichTextBlock *>(m_pPreviousLink);
            }
            else
            {
                m_pMaster = (static_cast<CRichTextBlockOverflow *>(m_pPreviousLink))->GetMaster();
            }
        }

        if (m_pMaster != nullptr &&
            m_pMaster->GetBlockLayoutEngine() != nullptr &&
            m_pMaster->m_pBlocks != nullptr)
        {
            IFC(m_pMaster->GetBlockLayoutEngine()->CreatePageNode(m_pMaster->m_pBlocks, this, &pBlockNode));
            m_pPageNode = static_cast<PageNode *>(pBlockNode);
            pBlockNode = nullptr;

            IFC(SetCursor(m_pMaster->m_eMouseCursor));
        }
    }

    // If there is a valid PageNode, there is content that will be laid out. Create a TextView.
    if (m_pPageNode != nullptr &&
        m_pTextView == nullptr)
    {
        m_pTextView = new RichTextBlockView(static_cast<PageNode *>(m_pPageNode));
    }

Cleanup:
    delete pBlockNode;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::ValidateNextLink
//
//  Synopsis: Checks for cycles in linked overflow elements.
//
//------------------------------------------------------------------------
bool CRichTextBlockOverflow::ValidateNextLink(
    _In_ RichTextServices::ILinkedTextContainer *pNewNext
    )
{
    bool cycleDetected = false;

    ILinkedTextContainer *pPrevious = this;
    while (pPrevious != nullptr)
    {
        if (pPrevious == pNewNext)
        {
            cycleDetected = TRUE;
            break;
        }
        pPrevious = pPrevious->GetPrevious();
    }

    return !cycleDetected;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRichTextBlockOverflow::ResolveNextLink
//
//  Synopsis:
//      Connects the next link and notifies it that a previous link has attached
//      itself.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::ResolveNextLink()
{
    // Clears the previous link, resets to local backing store and informs the next link of the backing store switch.
    if (m_pOverflowTarget != nullptr)
    {
        IFC_RETURN(RichTextServicesHelper::MapTxErr(m_pOverflowTarget->PreviousLinkAttached(this)));
    }
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRichTextBlockOverflow::InvalidateNextLink
//
//  Synopsis:
//      Invalidates information stored about the next link.
//      Called when the next link changes or is detached.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::InvalidateNextLink()
{
    // Notify the next link, that the previous link has been detached.
    if (m_pOverflowTarget != nullptr)
    {
        IFC_RETURN(RichTextServicesHelper::MapTxErr(m_pOverflowTarget->PreviousLinkDetached(this)));
    }

    ReleaseInterface(m_pOverflowTarget);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::OnSelectionChanged
//
//  Synopsis: Selection changed notification called by TextSelectionManager
//            when selection has changed. Invalidates render or arrange
//            on this element depending on theme if it is affected by
//            the change and notifies all overflows.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::OnSelectionChanged(
    _In_ uint32_t previousSelectionStartOffset,
    _In_ uint32_t previousSelectionEndOffset,
    _In_ uint32_t newSelectionStartOffset,
    _In_ uint32_t newSelectionEndOffset
    )
{
    bool selected = false;
    bool unSelected = false;

    // Determine if this element is affected by selection changing, either by:
    // 1. Previously being part of selection, and now being un-selected.
    // 2. Either newly containing part of selection or having existing selected portion change.
    // We don't try to optimize here by keeping track of what part of the element was selected and comparing with that -
    // if selection changed, and the element was selected/unselected, we'll invalidate.
    uint32_t contentStart = GetContentStartPosition();
    uint32_t contentEnd = contentStart + GetContentLength();

    if (newSelectionStartOffset < contentEnd &&
        newSelectionEndOffset >= contentStart)
    {
        // Element is at least partially selected, and should have selection rects re-drawn.
        selected = TRUE;
    }
    else if ((previousSelectionStartOffset < contentEnd &&
              previousSelectionEndOffset >= contentStart))
    {
        // Element is not selected now, but was previously.
        unSelected = TRUE;
    }

    if (selected || unSelected)
    {
        InvalidateSelectionRender();
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlockOverflow::OnSelectionVisibilityChanged
//
//  Synopsis: Selection visibility changed notification called by TextSelectionManager
//            when selection has changed. Invalidates render on this element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::OnSelectionVisibilityChanged(
    _In_ uint32_t selectionStartOffset,
    _In_ uint32_t selectionEndOffset
    )
{
    // Determine if this element is affected by selection changing, either by:
    // 1. Previously being part of selection, and now being un-selected.
    // 2. Either newly containing part of selection or having existing selected portion change.
    // We don't try to optimize here by keeping track of what part of the element was selected and comparing with that -
    // if selection changed, and the element was selected/unselected, we'll invalidate.
    uint32_t contentStart = GetContentStartPosition();
    uint32_t contentEnd = contentStart + GetContentLength();

    if (selectionStartOffset < contentEnd &&
        selectionEndOffset >= contentStart)
    {
        // Element is at least partially selected, and should be re-rendered.
        InvalidateSelectionRender();
    }
    return S_OK;
}

void  CRichTextBlockOverflow::InvalidateSelectionRender()
{
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
}

_Check_return_ HRESULT CRichTextBlockOverflow::D2DPreChildrenRenderVirtual(
    _In_ const SharedRenderParams& sharedRP,
    _In_ const D2DRenderParams& d2dRP
    )
{
    if (m_pPageNode != nullptr &&
        !m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        ASSERT(m_pPageNode->GetDrawingContext() != nullptr);

        if (m_pMaster && m_pMaster->GetSelectionManager() && m_pTextView)
        {
            IFC_RETURN(m_pMaster->GetSelectionManager()->D2DRender(
                d2dRP,
                m_pTextView,
                TRUE,
                0,
                nullptr));
        }

        IFC_RETURN(m_pPageNode->GetDrawingContext()->D2DRenderContent(
            sharedRP,
            d2dRP,
            d2dRP.m_forceOpaque ? 1.0f : GetOpacityCombined()));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: PrintPreChildrenPrintVirtual
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams
    )
{
    IFC_RETURN(SetupLinkedBlockLayout());
    IFC_RETURN(D2DEnsureResources(cp, sharedPrintParams.pCurrentTransform));
    IFC_RETURN(D2DPreChildrenRenderVirtual(sharedPrintParams, printParams));

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::D2DEnsureResources(
    _In_ const D2DPrecomputeParams &cp,
    _In_ const CMILMatrix *pMyAccumulatedTransform
    )
{
    if (m_pPageNode != nullptr &&
        !m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        ASSERT(m_pPageNode->GetDrawingContext() != nullptr);

        // Render page node which will communicate render data for text to the drawing context.
        // The Draw call was skipped in Arrange.
        // If element is in the live tree, we use whatever render data we already have.
        IFC_RETURN(m_pPageNode->Draw(FALSE));

        // If UIElement is marked dirty for rendering, invalidate render walk caches accumulated
        // in the drawing context.
        // Example of such situation is when Brush is changed inside, without replacing entire Brush object.
        // In such case all the render walk caches (textures/edge stores) need to be regenerated.
        if (NWIsContentDirty())
        {
            m_pPageNode->GetDrawingContext()->InvalidateRenderCache();
        }

        IFC_RETURN(m_pPageNode->GetDrawingContext()->D2DEnsureResources(cp, pMyAccumulatedTransform));
    }
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::UpdateSelectionAndHighlightRegions(_Out_ bool *redrawForHighlightRegions)
{
    *redrawForHighlightRegions = false;
    std::shared_ptr<HighlightRegion> selection = nullptr;
    m_highlightRegions.clear();

    // Find text selection.
    if (m_pMaster->GetSelectionManager() != nullptr)
    {
        IFC_RETURN(m_pMaster->GetSelectionManager()->GetSelectionHighlightRegion(UseHighContrastSelection(GetContext()), selection));
        if (selection != nullptr)
        {
            m_highlightRegions.push_back(*selection);
        }
    }

    if (!TextSelectionManager::AreSelectionHighlightOffsetsEqual(m_selectionHighlight, selection))
    {
        // Redraw is needed
        // Save offsets, so 'this' will maintain selection range memory ownership
        m_selectionHighlight = selection;
        // If selection has changed we must force re-rendering even if Arrange was bypassed.
        *redrawForHighlightRegions = true;
    }

    return S_OK;
}

_Check_return_ XUINT32 CRichTextBlockOverflow::GetLinkAPChildrenCount()
{
    unsigned int linkAPCount = 0;

    if (m_pMaster)
    {
        linkAPCount = m_pMaster->GetLinkAPChildrenCount();
    }

    return linkAPCount;
}

xref_ptr<CFlyoutBase> CRichTextBlockOverflow::GetContextFlyout() const
{
    if (auto flyout = CFrameworkElement::GetContextFlyout())
    {
        return flyout;
    }
    else if (m_pMaster)
    {
        return m_pMaster->GetContextFlyout();
    }
    else
    {
        return nullptr;
    }
}

// return contained hyplink Text Element if it is currently focused and focus rect should be drawn
CTextElement* CRichTextBlockOverflow::GetTextElementForFocusRect()
{
    if (m_pMaster != nullptr
        && m_pTextView != nullptr
        && !CFocusRectManager::AreHighVisibilityFocusRectsEnabled())
    {
        CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(this);
        if (pFocusManager)
        {
            CTextElement* pTextElement = pFocusManager->GetTextElementForFocusRectCandidate();

            if (pTextElement && pTextElement->GetContainingFrameworkElement() == m_pMaster)
            {
                return pTextElement;
            }
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    pBounds->left = 0.0f;
    pBounds->top = 0.0f;
    pBounds->right = CFrameworkElement::GetActualWidth();
    pBounds->bottom = CFrameworkElement::GetActualHeight();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a point intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    XRECTF_RB innerBounds =  { };

    IFC_RETURN(GetInnerBounds(&innerBounds));

    *pHit = DoesRectContainPoint(innerBounds, target);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a polygon intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    XRECTF_RB innerBounds =  { };

    IFC_RETURN(GetInnerBounds(&innerBounds));

    *pHit = target.IntersectsRect(innerBounds);

    return S_OK;
}

void CRichTextBlockOverflow::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CFrameworkElement::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->CleanupRealizations();
    }
}

//-----------------------------------------------------------------------------
//
//  Synopsis:
//      CUIElement override to update the gripper positions. This is required
//      when the rearrage occurs, for example upon a screen orientation change.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::ArrangeCore(XRECTF finalRect)
{
    HRESULT hr = CFrameworkElement::ArrangeCore(finalRect);
    if ((m_pMaster != nullptr) &&
        (m_pMaster->GetSelectionManager() != nullptr))
    {
        IFC_RETURN(m_pMaster->GetSelectionManager()->UpdateGripperPositions());
    }
    return hr;
}

_Check_return_ HRESULT CRichTextBlockOverflow::RenderHighlighterForegroundCallback(
    _In_ CSolidColorBrush* foregroundBrush,
    _In_ uint32_t highlightRectCount,
    _In_reads_(highlightRectCount) XRECTF* highlightRects
    )
{
    ASSERT(m_pPageNode->GetDrawingContext());

    XPOINTF pageOffset = { 0.0f, 0.0f };
    uint32_t rectsProcessed = 0;
    m_pPageNode->GetDrawingContext()->AppendForegroundHighlightInfo(
        highlightRectCount,
        highlightRects,
        foregroundBrush,
        0, // Index in rect array at which DrawingContext should start processing rects.
        pageOffset, // Offset of page in element's space.
        &rectsProcessed); // Number of rectangles processed by the page.

    // All highlight rects within local text view should be within the page.
    ASSERT(rectsProcessed == highlightRectCount);

    return S_OK;
}

//------------------------------------------------------------------------
//
// Event handlers
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::OnPointerMoved(
    _In_ CEventArgs* pEventArgs
)
{
    LinkedRichTextBlockView *pLinkedView = nullptr;

    // check whether the pointer is over a HyperLink
    if (m_pMaster != nullptr && m_pMaster->m_pBlocks != nullptr && m_pTextView != nullptr)
    {
        CPointerEventArgs *pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
        if (!pPointerArgs->m_bHandled)
        {
            CHyperlink *pLink = nullptr;
            // If the event doesn't come from the sender, we shouldn't be doing anything.
            if (pPointerArgs->m_pSource == this)
            {
                IFC_RETURN(CRichTextBlock::HitTestLink(
                    m_pMaster->m_pBlocks->GetTextContainer(),
                    this,
                    &pPointerArgs->GetGlobalPoint(),
                    m_pTextView,
                    &pLink));
            }

            auto currentLink = GetCurrentLinkNoRef();
            if (currentLink != pLink)
            {
                if (currentLink != nullptr)
                {
                    // If we go from one hyperlink to another, we need to reset the old one to normal color.
                    IFC_RETURN(currentLink->OnPointerExited(pPointerArgs));
                }

                if (pLink != nullptr)
                {
                    IFC_RETURN(pLink->OnPointerEntered(pPointerArgs));
                }
            }

            if (pLink != nullptr)
            {
                m_currentLink = pLink;
                // There is a hyperlink, we need to update the cursor to hand shape.
                VERIFYHR(SetCursor(MouseCursorHand));

                // Moved but pressed.
                auto currentHyperlink = m_currentLink.get();
                if (pPointerArgs->m_pPointer->m_pointerDeviceType != DirectUI::PointerDeviceType::Mouse ||
                    (pPointerArgs->m_pPointer->m_bLeftButtonPressed && (currentHyperlink == m_pressedHyperlink)))
                {
                    IFC_RETURN(currentHyperlink->UpdateForegroundColor(HYPERLINK_PRESSED));
                }
                // Just point over.
                else
                {
                    IFC_RETURN(currentHyperlink->UpdateForegroundColor(HYPERLINK_POINTOVER));
                }
            }
            else
            {
                if (m_pMaster->m_isTextSelectionEnabled)
                {
                    VERIFYHR(SetCursor(MouseCursorIBeam));
                }
                else
                {
                    VERIFYHR(SetCursor(MouseCursorDefault));
                }
            }
        }
    }

    if (m_pMaster != nullptr &&
        m_pMaster->GetTextView() != nullptr &&
        m_pMaster->GetSelectionManager() != nullptr &&
        m_pTextView != nullptr)
    {
        pLinkedView = static_cast<LinkedRichTextBlockView *>(m_pMaster->GetTextView());
        ASSERT(pLinkedView != nullptr);
        pLinkedView->SetInputContextView(m_pTextView);
        IFC_RETURN(m_pMaster->GetSelectionManager()->OnPointerMoved(
            this,
            pEventArgs,
            pLinkedView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnPointerExited(
    _In_ CEventArgs* pEventArgs
)
{
    auto currentLink = GetCurrentLinkNoRef();
    if (currentLink != nullptr)
    {
        IFC_RETURN(currentLink->OnPointerExited(static_cast<CPointerEventArgs *>(pEventArgs)));
        m_currentLink.reset();
    }

    m_pressedHyperlink.reset();
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnPointerPressed(
    _In_ CEventArgs* pEventArgs
)
{
    if (m_pMaster != nullptr && m_pMaster->m_pBlocks != nullptr && m_pTextView != nullptr)
    {
        CPointerEventArgs *pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
        if (!pPointerArgs->m_bHandled)
        {
            bool processSelection = true;

            // Hyperlink clicks are prioritized above selection.
            if (pPointerArgs->m_pPointer->m_pointerDeviceType != DirectUI::PointerDeviceType::Mouse ||
                pPointerArgs->m_pPointer->m_bLeftButtonPressed)
            {
                CHyperlink *pLink = nullptr;

                // If the event doesn't come from the sender, we shouldn't be doing anything.
                if (pPointerArgs->m_pSource == this)
                {
                    IFC_RETURN(CRichTextBlock::HitTestLink(
                        m_pMaster->m_pBlocks->GetTextContainer(),
                        this,
                        &pPointerArgs->GetGlobalPoint(),
                        m_pTextView,
                        &pLink));
                }

                if (pLink != nullptr)
                {
                    processSelection = false;

                    CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(this);
                    const FocusMovementResult result = pFocusManager->SetFocusedElement(FocusMovement(pLink, DirectUI::FocusNavigationDirection::None, DirectUI::FocusState::Pointer));
                    IFC_RETURN(result.GetHResult());

                    m_pressedHyperlink = pLink;

                    IFC_RETURN(m_pressedHyperlink->UpdateForegroundColor(HYPERLINK_PRESSED));
                    pPointerArgs->m_bHandled = TRUE;
                }
                if (pLink != nullptr)
                {
                    m_currentLink = pLink;
                }
            }

            if (processSelection &&
                m_pMaster->GetTextView() != nullptr &&
                m_pMaster->GetSelectionManager() != nullptr)
            {
                LinkedRichTextBlockView *pLinkedView = static_cast<LinkedRichTextBlockView *>(m_pMaster->GetTextView());
                ASSERT(pLinkedView != nullptr);
                pLinkedView->SetInputContextView(m_pTextView);
                IFC_RETURN(m_pMaster->GetSelectionManager()->OnPointerPressed(
                    this,
                    pEventArgs,
                    pLinkedView));
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnPointerReleased(
    _In_ CEventArgs* pEventArgs
)
{
    if (m_pMaster != nullptr && m_pMaster->m_pBlocks != nullptr && m_pTextView != nullptr)
    {
        CPointerEventArgs *pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
        if (!pPointerArgs->m_bHandled)
        {
            CHyperlink *pLink = nullptr;
            // If the event doesn't come from the sender, we shouldn't be doing anything.
            if (pPointerArgs->m_pSource == this)
            {
                IFC_RETURN(CRichTextBlock::HitTestLink(
                    m_pMaster->m_pBlocks->GetTextContainer(),
                    this,
                    &pPointerArgs->GetGlobalPoint(),
                    m_pTextView,
                    &pLink));

                // If we go from one link to another, we need to reset the old one to normal color.
                auto currentLink = GetCurrentLinkNoRef();
                if (currentLink != pLink)
                {
                    if (currentLink != nullptr)
                    {
                        IFC_RETURN(currentLink->OnPointerExited(pPointerArgs));
                    }

                    if (pLink != nullptr)
                    {
                        IFC_RETURN(pLink->OnPointerEntered(pPointerArgs));
                    }
                }

                if (pLink != nullptr)
                {
                    m_currentLink = pLink;

                    // Not pressed, point over for mouse.
                    if (pPointerArgs->m_pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Mouse
                        && !pPointerArgs->m_pPointer->m_bLeftButtonPressed)
                    {
                        IFC_RETURN(pLink->UpdateForegroundColor(HYPERLINK_POINTOVER));
                    }
                    // When touch release, set color to normal.
                    else
                    {
                        IFC_RETURN(pLink->OnPointerExited(pPointerArgs));
                    }
                }
                else
                {
                    m_currentLink.reset();
                }
            }
        }
    }

    if (m_pMaster != nullptr &&
        m_pMaster->GetTextView() != nullptr &&
        m_pMaster->GetSelectionManager() != nullptr &&
        m_pTextView != nullptr)
    {
        LinkedRichTextBlockView *pLinkedView = static_cast<LinkedRichTextBlockView *>(m_pMaster->GetTextView());
        ASSERT(pLinkedView != nullptr);
        pLinkedView->SetInputContextView(m_pTextView);
        IFC_RETURN(m_pMaster->GetSelectionManager()->OnPointerReleased(
            this,
            pEventArgs,
            pLinkedView));
    }
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnGotFocus(
    _In_ CEventArgs* pEventArgs
)
{
    LinkedRichTextBlockView *pLinkedView = nullptr;

    if (m_pMaster != nullptr &&
        m_pMaster->GetTextView() != nullptr &&
        m_pMaster->GetSelectionManager() != nullptr &&
        m_pTextView != nullptr)
    {
        pLinkedView = static_cast<LinkedRichTextBlockView *>(m_pMaster->GetTextView());
        ASSERT(pLinkedView != nullptr);
        pLinkedView->SetInputContextView(m_pTextView);
        IFC_RETURN(m_pMaster->GetSelectionManager()->OnGotFocus(
            this,
            pEventArgs,
            pLinkedView));
    }
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnLostFocus(
    _In_ CEventArgs* pEventArgs
)
{
    LinkedRichTextBlockView *pLinkedView = nullptr;

    if (m_pMaster != nullptr &&
        m_pMaster->GetTextView() != nullptr &&
        m_pMaster->GetSelectionManager() != nullptr &&
        m_pTextView != nullptr)
    {
        pLinkedView = static_cast<LinkedRichTextBlockView *>(m_pMaster->GetTextView());
        ASSERT(pLinkedView != nullptr);
        pLinkedView->SetInputContextView(m_pTextView);
        IFC_RETURN(m_pMaster->GetSelectionManager()->OnLostFocus(
            this,
            pEventArgs,
            pLinkedView));
    }
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnHolding(
    _In_ CEventArgs* pEventArgs
)
{
    if (m_pMaster != nullptr &&
        m_pMaster->GetTextView() != nullptr &&
        m_pMaster->GetSelectionManager() != nullptr &&
        m_pMaster->IsSelectionEnabled() &&
        m_pTextView != nullptr
        )
    {
        IFC_RETURN(m_pMaster->GetSelectionManager()->OnHolding(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnTapped(
    _In_ CEventArgs* pEventArgs
)
{
    HRESULT hr = S_OK;

    if (m_pMaster != nullptr && m_pMaster->m_pBlocks != nullptr && m_pTextView != nullptr)
    {
        CTappedEventArgs *pPointerArgs = static_cast<CTappedEventArgs*>(pEventArgs);
        bool processSelection = true;
        if (!pPointerArgs->m_bHandled)
        {
            CHyperlink *pLink = nullptr;

            if (pPointerArgs->m_pSource == this)
            {
                // Hittest the content to find out if we are over a Hyperlink element.
                IFC(CRichTextBlock::HitTestLink(
                    m_pMaster->m_pBlocks->GetTextContainer(),
                    this,
                    &pPointerArgs->GetGlobalPoint(),
                    m_pTextView,
                    &pLink));
            }

            // Hyperlink clicks are prioritized above selection.
            // Do Hyperlink navigation only if the Hyperlink is the same as
            // the one for which press was initiated.
            if (pLink != nullptr && m_pressedHyperlink == pLink)
            {
                processSelection = FALSE;
                IFC(pLink->Navigate());
                pPointerArgs->m_bHandled = TRUE;
            }
        }
        if (m_pMaster->GetSelectionManager() != nullptr &&
            m_pMaster->GetTextView() != nullptr &&
            processSelection)
        {
            LinkedRichTextBlockView *pLinkedView = static_cast<LinkedRichTextBlockView *>(m_pMaster->GetTextView());
            ASSERT(pLinkedView != nullptr);
            pLinkedView->SetInputContextView(m_pTextView);
            IFC(m_pMaster->GetSelectionManager()->OnTapped(
                this,
                pEventArgs,
                pLinkedView));
        }
    }

Cleanup:
    m_pressedHyperlink.reset();
    return hr;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnRightTapped(
    _In_ CEventArgs* pEventArgs
)
{
    LinkedRichTextBlockView *pLinkedView = nullptr;

    if (m_pMaster != nullptr &&
        m_pMaster->GetTextView() != nullptr &&
        m_pMaster->GetSelectionManager() != nullptr &&
        m_pTextView != nullptr)
    {
        pLinkedView = static_cast<LinkedRichTextBlockView *>(m_pMaster->GetTextView());
        ASSERT(pLinkedView != nullptr);
        pLinkedView->SetInputContextView(m_pTextView);
        IFC_RETURN(m_pMaster->GetSelectionManager()->OnRightTapped(
            this,
            pEventArgs,
            pLinkedView));
    }
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnDoubleTapped(
    _In_ CEventArgs* pEventArgs
)
{
    LinkedRichTextBlockView *pLinkedView = nullptr;

    if (m_pMaster != nullptr &&
        m_pMaster->GetTextView() != nullptr &&
        m_pMaster->GetSelectionManager() != nullptr &&
        m_pTextView != nullptr)
    {
        pLinkedView = static_cast<LinkedRichTextBlockView *>(m_pMaster->GetTextView());
        ASSERT(pLinkedView != nullptr);
        pLinkedView->SetInputContextView(m_pTextView);
        IFC_RETURN(m_pMaster->GetSelectionManager()->OnDoubleTapped(
            this,
            pEventArgs,
            pLinkedView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnKeyUp(
    _In_ CEventArgs* pEventArgs
)
{
    LinkedRichTextBlockView *pLinkedView = nullptr;

    if (m_pMaster != nullptr &&
        m_pMaster->GetTextView() != nullptr &&
        m_pMaster->GetSelectionManager() != nullptr &&
        m_pTextView != NULL)
    {
        pLinkedView = static_cast<LinkedRichTextBlockView *>(m_pMaster->GetTextView());
        ASSERT(pLinkedView != nullptr);
        pLinkedView->SetInputContextView(m_pTextView);
        IFC_RETURN(m_pMaster->GetSelectionManager()->OnKeyUp(
            this,
            pEventArgs,
            pLinkedView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::OnKeyDown(
    _In_ CEventArgs* pEventArgs
)
{
    LinkedRichTextBlockView *pLinkedView = nullptr;

    if (m_pMaster != nullptr &&
        m_pMaster->GetTextView() != nullptr &&
        m_pMaster->GetSelectionManager() != nullptr &&
        m_pTextView != nullptr)
    {
        pLinkedView = static_cast<LinkedRichTextBlockView *>(m_pMaster->GetTextView());
        ASSERT(pLinkedView != nullptr);
        pLinkedView->SetInputContextView(m_pTextView);
        IFC_RETURN(m_pMaster->GetSelectionManager()->OnKeyDown(
            this,
            pEventArgs,
            pLinkedView));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   HWRenderSelection
//
//  Synopsis:
//      Calls TextSelectionManager to render selection, indicating if
//      current theme is high contrast so appropriate highlight color
//      can be used.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow::HWRenderSelection(
    _In_ IContentRenderer* pContentRenderer,
    _In_ bool isHighContrast,
    _In_ uint32_t highlightRectCount,
    _In_reads_opt_(highlightRectCount) XRECTF *pHighlightRects
    )
{
    if (m_pMaster != nullptr &&
        m_pMaster->GetSelectionManager() != NULL &&
        m_pTextView != NULL)
    {
        IFC_RETURN(m_pMaster->GetSelectionManager()->HWRender(pContentRenderer, m_pTextView, isHighContrast, highlightRectCount, pHighlightRects));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockOverflow::HWRenderContent(
    _In_ IContentRenderer* pContentRenderer
)
{
    HRESULT hr = S_OK;

    if (m_pPageNode != nullptr &&
        !m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        ASSERT(m_pPageNode->GetDrawingContext() != nullptr);
        bool redrawForHighlightRegions = false;

        if (m_pMaster != nullptr &&
            m_pTextView != nullptr)
        {
            auto foregroundRenderingCallback = std::bind(
                &CRichTextBlockOverflow::RenderHighlighterForegroundCallback,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3);

            IFC(UpdateSelectionAndHighlightRegions(&redrawForHighlightRegions));

            IFC(TextHighlightRenderer::HWRenderCollection(
                GetContext(),
                m_pMaster->m_textHighlighters,
                m_highlightRegions,
                m_pTextView,
                pContentRenderer,
                foregroundRenderingCallback));
        }
        else
        {
            m_selectionHighlight = nullptr;
        }

        if (redrawForHighlightRegions || m_redrawForArrange ||
            ((m_pMaster != nullptr) &&
            (m_pMaster->m_textHighlighters != nullptr)))
        {
            // Render page node which will communicate render data for text to the drawing context.
            IFC(m_pPageNode->Draw(TRUE));
            m_redrawForArrange = false;
        }

        // If UIElement is marked dirty for rendering, invalidate render walk caches accumulated
        // in the drawing context.
        // Example of such situation is when Brush is changed inside, without replacing entire Brush object.
        // In such case all the render walk caches (textures/edge stores) need to be regenerated.
        if (NWIsContentDirty())
        {
            m_pPageNode->GetDrawingContext()->InvalidateRenderCache();
        }

        IFC(m_pPageNode->GetDrawingContext()->HWRenderContent(pContentRenderer));

    }

Cleanup:
    if (m_pPageNode != nullptr)
    {
        if (m_pPageNode->GetDrawingContext() != nullptr)
        {
            m_pPageNode->GetDrawingContext()->ClearForegroundHighlightInfo();
        }
    }

    return hr;
}

_Check_return_ HRESULT CRichTextBlockOverflow::HWPostChildrenRender(
    _In_ IContentRenderer* pContentRenderer
)
{
    CTextElement *pFocusedElement = GetTextElementForFocusRect();
    if (pFocusedElement)
    {
        IFC_RETURN(CRichTextBlock::HWRenderFocusRects(pContentRenderer, pFocusedElement, m_pTextView));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns independently animated brushes for rendering with PC.
//      The fill brush is the foreground brush. The stroke brush is the
//      selection brush.
//
//------------------------------------------------------------------------
void CRichTextBlockOverflow::GetIndependentlyAnimatedBrushes(
    _Outptr_ CSolidColorBrush **ppFillBrush,
    _Outptr_ CSolidColorBrush **ppStrokeBrush
)
{
    if (m_pTextFormatting->m_pForeground != NULL && m_pTextFormatting->m_pForeground->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
    {
        SetInterface(*ppFillBrush, static_cast<CSolidColorBrush *>(m_pTextFormatting->m_pForeground));
    }
    if (m_pMaster != NULL && m_pMaster->GetSelectionManager() != nullptr &&
        m_pMaster->GetSelectionManager()->GetSelectionBackgroundBrush() != nullptr)
    {
        SetInterface(*ppStrokeBrush, m_pMaster->GetSelectionManager()->GetSelectionBackgroundBrush());
    }
}
