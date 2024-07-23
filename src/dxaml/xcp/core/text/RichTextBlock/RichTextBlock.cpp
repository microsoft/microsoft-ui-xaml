// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <functional>

#include "BlockLayoutEngine.h"
#include "DrawingContext.h"
#include "BlockNode.h"
#include "ParagraphNode.h"
#include "BlockNodeBreak.h"
#include "PageNode.h"

#include "TextSelectionManager.h"
#include <math.h>
#include <ContentRenderer.h>
#include <FocusRectManager.h>
#include <TextHighlightRenderer.h>
#include <TextHighlighterCollection.h>
#include "HighlightRegion.h"
#include "application.h"

#include "RichTextBlockView.h"
#include "LinkedRichTextBlockView.h"
#include "RectUtil.h"
#include "RootScale.h"

using namespace Focus;
using namespace RichTextServices;

// RichTextBlock requires custom focus children collection since its focus children
// may be either embedded UElements, which it parents directly during formatting,
// or Hyperlinks, which are nested within its content. To correctly represent
// both and return them to FocusManager for tabbing, etc. a custom focus children
// collection is necessary.
class RichTextBlockFocusableChildrenCollection final : public CDOCollection
{
public:
    RichTextBlockFocusableChildrenCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore, false)
    {}

    bool ShouldParentBePublic() const final { return false; }

    _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject* pObject) final
    {
        if (pObject != nullptr &&
            (CFocusableHelper::IsFocusableDO(pObject) || pObject->OfTypeByIndex<KnownTypeIndex::UIElement>()))
        {
            return S_OK;
        }

        return E_INVALIDARG;
    }
};

CRichTextBlock::CRichTextBlock(_In_ CCoreServices *pCore)
    : CFrameworkElement(pCore)
{
    // Fields unique to RichTextBlock
    m_pBlocks = nullptr;
    m_textIndent = 0.0f;
    m_pFontCollection = nullptr;
    m_pSelectionHighlightColor = nullptr;
    m_pTextFormatter = nullptr;
    m_pLinkedView = nullptr;
    m_focusableChildrenCollection = nullptr;

    m_textWrapping = DirectUI::TextWrapping::Wrap;
    m_isTextSelectionEnabled = true;

    // Fields common to TextBlock and RichTextBlock
    m_eLineHeight = 0.0f; //implies no line height override
    m_textLineBounds = DirectUI::TextLineBounds::Full;
    m_textTrimming = DirectUI::TextTrimming::None;
    m_pFontContext = nullptr;
    m_pBlockLayout = nullptr;
    m_lineStackingStrategy = DirectUI::LineStackingStrategy::MaxHeight;
    m_pSelectionManager = nullptr;
    m_opticalMarginAlignment = DirectUI::OpticalMarginAlignment::None;
    m_textAlignment = DirectUI::TextAlignment::Left;
    m_highContrastAdjustment = Convert(DirectUI::ElementHighContrastAdjustment::None);
    m_applicationHighContrastAdjustment = Convert(DirectUI::ApplicationHighContrastAdjustment::Auto);

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

    m_textReadingOrder = DirectUI::TextReadingOrder::DetectFromContent;

    if (!pCore->IsInBackgroundTask())
    {
        IGNOREHR(SetCursor(MouseCursorIBeam));
    }
    m_isColorFontEnabled = true;
}

CRichTextBlock::~CRichTextBlock()
{
    if (m_pBlocks)
    {
        IGNOREHR(m_pBlocks->Clear());
        VERIFYHR(m_pBlocks->RemoveParent(this));
    }

    ReleaseInterface(m_pBlocks);
    ReleaseInterface(m_pFontCollection);

    if (m_textHighlighters)
    {
        IGNOREHR(m_textHighlighters->Clear());
        VERIFYHR(m_textHighlighters->RemoveParent(this));
    }

    ReleaseInterface(m_textHighlighters);

    if (m_pTextFormatter != nullptr)
    {
        CTextCore *pTextCore = nullptr;
        TextFormatterCache *pTextFormatterCache = nullptr;

        VERIFYHR(GetContext()->GetTextCore(&pTextCore));
        VERIFYHR(pTextCore->GetTextFormatterCache(&pTextFormatterCache));
        pTextFormatterCache->ReleaseTextFormatter(m_pTextFormatter);
    }

    delete m_pFontContext;
    delete m_pBlockLayout;
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
    delete m_pLinkedView;
    VERIFYHR(TextSelectionManager::Destroy(&m_pSelectionManager));
    ReleaseInterface(m_pSelectionHighlightColor);
}

_Check_return_ HRESULT CRichTextBlock::EnterImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    EnterParams params
    )
{
    // First bring this TextBlock into scope.
    IFC_RETURN(CFrameworkElement::EnterImpl(pNamescopeOwner, params));

    if (params.fIsLive)
    {
        // Upon entering the live tree, a parent or ancestor may have changed and inherited properties
        // should be considered dirty.
        InvalidateContent();
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::LeaveImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    LeaveParams params
    )
{
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

        // Ensure this element doesn't have a focus
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

        // Deleting TextSelectionManager here so that Popup can be removed while tree is still alive,
        // If TextSelectionManager is created and not in a live tree, ~CTextBlock will do the deletion work.
        IFC_RETURN(TextSelectionManager::Destroy(&m_pSelectionManager));

        // Upon leaving the live tree, a parent or ancestor may have changed and inherited properties
        // should be considered dirty.
        InvalidateContent();
     }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::RecursiveInvalidateMeasure
//
//------------------------------------------------------------------------
void CRichTextBlock::RecursiveInvalidateMeasure()
{
    InvalidateContentMeasure();
    InvalidateContent();

    CUIElement::RecursiveInvalidateMeasure();
}

CHyperlink* CRichTextBlock::GetCurrentLinkNoRef() const
{
    return m_currentLink.get();
}

//------------------------------------------------------------------------
//
//  Method: CRichTextBlock::GetValue
//
//  Synopsis:
//      Performs any lazy computation required for properties such as
//      ActualWidth, ActualHeight and Text, before calling the standard
//      FrameworkElement GetValue implementation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::GetValue(
    _In_ const CDependencyProperty *pdp,
    _Out_ CValue *pValue
)
{
    // If property query is for Blocks property, we need to create Blocks before calling
    // FrameworkElement::GetValue, so it will populate the value correctly.
    switch (pdp->GetIndex())
    {
        case KnownPropertyIndex::RichTextBlock_Blocks:
        {
            // Create block collection if current value is nullptr.

            if (m_pBlocks == nullptr)
            {
                IFC_RETURN(CreateBlocks());
            }
            break;
        }

        case KnownPropertyIndex::RichTextBlock_SelectionHighlightColor:
        {
            if (m_pSelectionHighlightColor == nullptr)
            {
                IFC_RETURN(CreateSelectionHighlightColor());
            }
            break;
        }

        case KnownPropertyIndex::RichTextBlock_TextHighlighters:
        {
            if (m_textHighlighters == nullptr)
            {
                IFC_RETURN(CreateTextHighlighters());

                // EnsureBlockLayout ensures the RichTextBlockView which is required for the feature and
                // it is pay for play so ensure it now.
                IFC_RETURN(EnsureBlockLayout());
            }
            break;
        }
    }

    IFC_RETURN(CFrameworkElement::GetValue(pdp, pValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::SetValue
//
//  Synopsis: Records a property value change.
//
//  Special handling:
//      1. OverflowTarget copied directly to field bypassing property
//         system which would set up unwanted association/parenting.
//      2. ContentMeasure/Arrange invalidated based on property change.
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::SetValue(_In_ const SetValueParams& args)
{
    bool selectionEnabled = m_isTextSelectionEnabled;
    CRichTextBlockOverflow *pOverflowTargetOldValue = m_pOverflowTarget;
    CValue valueOverflowTarget;

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::RichTextBlock_FontFamily:
        {
            if (args.m_value.IsNull() &&
                (GetContext()->IsSettingValueFromManaged(this) || ParserOwnsParent()))
            {
                HRESULT hrToOriginate = E_NER_ARGUMENT_EXCEPTION;
                xephemeral_string_ptr parameters[2];

                ASSERT(parameters[0].IsNull());
                args.m_pDP->GetName().Demote(&parameters[1]);

                IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_PROPERTY_INVALID, 2, parameters));
                IFC_RETURN(static_cast<HRESULT>(E_NER_ARGUMENT_EXCEPTION));
            }
            break;
        }

        case KnownPropertyIndex::RichTextBlock_LineHeight:
        // Validate LineHeight property setting is > 0.
        {
            if (args.m_value.GetType() == valueFloat || args.m_value.GetType() == valueDouble)
            {
                if (args.m_value.AsDouble() < 0.0f)
                {
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            else
            {
                const CDouble* pDouble = nullptr;
                if (FAILED(DoPointerCast(pDouble, args.m_value)) ||
                    pDouble->m_eValue < 0.0f)
                {
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            break;
        }

        case KnownPropertyIndex::RichTextBlock_MaxLines:
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
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::RichTextBlock_OverflowContentTarget)
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
                if (args.m_value.AsObject()->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
                {
                    m_pOverflowTarget = static_cast<CRichTextBlockOverflow*>(args.m_value.AsObject());
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
                // Since calling InvalidateContentArrange will invalidate all overflows, we just invalidate arrange on the page node directly
                // and render only on this element.
                if (m_pPageNode != nullptr)
                {
                    m_pPageNode->InvalidateArrange();
                }

                // Even though overflow target is marked as AffectsArrange in the SLOM, Arrange is not invalidated by the property system when it changes because the property system is
                // bypassed for setting this property due to multiple associations code. So Arrange on the element needs to be explicitly invalidated here.
                InvalidateArrange();
                CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
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
            case KnownPropertyIndex::FrameworkElement_Language:
            case KnownPropertyIndex::FrameworkElement_FlowDirection:
            case KnownPropertyIndex::RichTextBlock_FontSize:
            case KnownPropertyIndex::RichTextBlock_FontFamily:
            case KnownPropertyIndex::RichTextBlock_FontWeight:
            case KnownPropertyIndex::RichTextBlock_FontStyle:
            case KnownPropertyIndex::RichTextBlock_FontStretch:
            case KnownPropertyIndex::RichTextBlock_CharacterSpacing:
            case KnownPropertyIndex::RichTextBlock_TextReadingOrder:
                InvalidateContent();
                break;

            // TODO: InvalidateMeasure needs to be called on padding changes so that Measure is invalidate on the
            // PageNode, otherwise it may bypass measure by checking size constraint equality without accounting for padding.
            // PageNode/BlockNode's bypass logic should take padding into account.
            case KnownPropertyIndex::RichTextBlock_TextWrapping:
            case KnownPropertyIndex::RichTextBlock_Padding:
            case KnownPropertyIndex::RichTextBlock_LineHeight:
            case KnownPropertyIndex::RichTextBlock_MaxLines:
            case KnownPropertyIndex::RichTextBlock_TextLineBounds:
            case KnownPropertyIndex::RichTextBlock_LineStackingStrategy:
            case KnownPropertyIndex::RichTextBlock_TextAlignment:
            case KnownPropertyIndex::RichTextBlock_OpticalMarginAlignment:
            case KnownPropertyIndex::RichTextBlock_TextTrimming:
            case KnownPropertyIndex::RichTextBlock_TextIndent:
                InvalidateContentMeasure();
                break;

            case KnownPropertyIndex::RichTextBlock_IsColorFontEnabled:
                InvalidateContentArrange();
                break;

            case KnownPropertyIndex::RichTextBlock_Foreground:
                InvalidateRender();
                break;

            case KnownPropertyIndex::RichTextBlock_SelectionHighlightColor:
                UpdateSelectionHighlightColor();
                break;

            case KnownPropertyIndex::RichTextBlock_IsTextSelectionEnabled:
                IFC_RETURN(OnSelectionEnabledChanged(selectionEnabled));
                break;

            case KnownPropertyIndex::Control_IsEnabled:
                if (m_pPageNode)
                {
                    if (m_pPageNode->GetDrawingContext() != nullptr)
                    {
                        m_pPageNode->GetDrawingContext()->SetControlEnabled(args.m_value.AsBool());
                    }
                }
                break;

            case KnownPropertyIndex::UIElement_HighContrastAdjustment:
                IFC_RETURN(OnHighContrastAdjustmentChanged(static_cast<DirectUI::ElementHighContrastAdjustment>(args.m_value.AsEnum())));
                break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::PullInheritedTextFormatting()
{
    HRESULT         hr                    = S_OK;
    TextFormatting *pParentTextFormatting = nullptr;

    IFCEXPECT_ASSERT(m_pTextFormatting != nullptr);
    if (m_pTextFormatting->IsOld())
    {
        // Get the text core properties that we will be inheriting from.
        IFC(GetParentTextFormatting(&pParentTextFormatting));

        // Process each TextElement text core property one by one.

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::RichTextBlock_FontFamily))
        {
            IFC(m_pTextFormatting->SetFontFamily(this, pParentTextFormatting->m_pFontFamily));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::RichTextBlock_Foreground)
            && !m_pTextFormatting->m_freezeForeground)
        {
            IFC(m_pTextFormatting->SetForeground(this, pParentTextFormatting->m_pForeground));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Language))
        {
            m_pTextFormatting->SetLanguageString(pParentTextFormatting->m_strLanguageString);
            m_pTextFormatting->SetResolvedLanguageString(pParentTextFormatting->GetResolvedLanguageStringNoRef());
            m_pTextFormatting->SetResolvedLanguageListString(pParentTextFormatting->GetResolvedLanguageListStringNoRef());
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::RichTextBlock_FontSize))
        {
            m_pTextFormatting->m_eFontSize = pParentTextFormatting->m_eFontSize;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::RichTextBlock_FontWeight))
        {
            m_pTextFormatting->m_nFontWeight = pParentTextFormatting->m_nFontWeight;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::RichTextBlock_FontStyle))
        {
            m_pTextFormatting->m_nFontStyle = pParentTextFormatting->m_nFontStyle;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::RichTextBlock_FontStretch))
        {
            m_pTextFormatting->m_nFontStretch = pParentTextFormatting->m_nFontStretch;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::RichTextBlock_CharacterSpacing))
        {
            m_pTextFormatting->m_nCharacterSpacing = pParentTextFormatting->m_nCharacterSpacing;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::RichTextBlock_TextDecorations))
        {
            m_pTextFormatting->m_nTextDecorations = pParentTextFormatting->m_nTextDecorations;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_FlowDirection))
        {
            m_pTextFormatting->m_nFlowDirection = pParentTextFormatting->m_nFlowDirection;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::RichTextBlock_IsTextScaleFactorEnabled) &&
            IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_IsTextScaleFactorEnabledInternal))
        {
            m_pTextFormatting->m_isTextScaleFactorEnabled = pParentTextFormatting->m_isTextScaleFactorEnabled;
        }

        m_pTextFormatting->SetIsUpToDate();
    }

Cleanup:
    ReleaseInterface(pParentTextFormatting);
    return hr;
}

void CRichTextBlock::OnChildDesiredSizeChanged(_In_ CUIElement* pElement)
{
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->OnChildDesiredSizeChanged(pElement);
    }
    m_isBreakValid = FALSE;
    CFrameworkElement::OnChildDesiredSizeChanged(pElement);
}

_Check_return_ HRESULT CRichTextBlock::GetActualWidth(_Out_ float* pWidth)
{
    *pWidth = m_pPageNode ? m_pPageNode->GetDesiredSize().width : 0.0f;
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::GetActualHeight(_Out_ float* pHeight)
{
    *pHeight = m_pPageNode ? m_pPageNode->GetDesiredSize().height : 0.0f;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetTextFormatter
//
//  Synopsis: Get TextFormatter instance.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::GetTextFormatter(
    _Outptr_ TextFormatter **ppTextFormatter
    )
{
    // RichTextBlock maintains one formatter per element since it doesn't face memory pressure like TextBlock where there are so
    // many instances each one cannot maintain a formatter for its entire lifetime. So there's no need to acquire one here.
    ASSERT(m_pTextFormatter != nullptr);
    *ppTextFormatter = m_pTextFormatter;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ReleaseTextFormatter
//
//  Synopsis: Create a new TextFormatter instance.
//
//------------------------------------------------------------------------

void CRichTextBlock::ReleaseTextFormatter(
    _In_ TextFormatter *pTextFormatter
    )
{
    // RichTextBlock maintains one formatter per element since it doesn't face memory pressure like TextBlock where there are so
    // many instances each one cannot maintain a formatter for its entire lifetime. So there's no need to release anything here,
    // the instance formatter will be released in dtor.
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::GetBaselineOffset
//
//  Synopsis: Gets distance to baseline of first line from top of control.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::GetBaselineOffset(_Out_ XFLOAT *pBaselineOffset)
{
    *pBaselineOffset = 0;
    if (m_pPageNode != nullptr)
    {
        *pBaselineOffset  = m_pPageNode->GetBaselineAlignmentOffset();
    }
    return S_OK;
}

uint32_t CRichTextBlock::GetContentStartPosition() const
{
    // CRichTextBlock always starts at 0.
    return 0;
}

uint32_t CRichTextBlock::GetContentLength() const
{
    // If there is a valid measured page, use its length unless there is no overflow target for this element.
    // If there's no overflow target the element is considered to "contain" all the content up to the text container's end,
    // even if it's not visible or doesn't fit.
    if (m_pPageNode != nullptr &&
        !m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty() &&
        m_pOverflowTarget != nullptr)
    {
        return m_pPageNode->GetContentLength();
    }
    else if (m_pBlocks != nullptr)
    {
        // If PageNode does not exist or is not valid, layout is either invalid or has
        // never taken place. Since CRichTextBlock is the owner of content, it should return
        // the full length of the text container.
        uint32_t containerLength = 0;
        m_pBlocks->GetTextContainer()->GetPositionCount(&containerLength);
        return containerLength;
    }
    return 0;
}

//------------------------------------------------------------------------
//    Summary:
//      Returns the ITextView that this object exposes publicly for querying.
//      This is the standalone TextView if there are no overflow targets,
//      or the linked view if this is the master in a linked chain.
//------------------------------------------------------------------------
ITextView *CRichTextBlock::GetTextView() const
{
    if (m_pLinkedView != nullptr)
    {
        return m_pLinkedView;
    }
    return m_pTextView;
}

//------------------------------------------------------------------------
//    Summary:
//      Called when the BlockCollection is dirtied.
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::OnContentChanged(
    _In_opt_ const CDependencyProperty* dp
    )
{
    bool shouldInvalidateContent = true;
    bool shouldClearCachedHyperlinks = true;

    if (dp != nullptr)
    {
        if (IsForegroundPropertyIndex(dp->GetIndex()) ||
            dp->GetIndex() == KnownPropertyIndex::Hyperlink_FocusState)
        {
            shouldInvalidateContent = false;
            shouldClearCachedHyperlinks = false;
            InvalidateRender();
        }
        else if (dp->GetIndex() == KnownPropertyIndex::Block_Margin
            || dp->GetIndex() == KnownPropertyIndex::Paragraph_TextIndent)
        {
            shouldInvalidateContent = false;
            shouldClearCachedHyperlinks = false;
            InvalidateContentMeasure();
        }
        else if (dp->GetIndex() == KnownPropertyIndex::TextElement_TextDecorations)
        {
            //
            // For a tap landing on a Hyperlink inside a RichTextBlock, the sequence of events is OnPointerPressed,
            // OnPointerReleased, and OnTapped. The Hyperlink is only invoked in OnTapped, but CRichTextBlock::OnTapped
            // requires not only that the pointer hit tests to a Hyperlink, but also that the Hyperlink matches our
            // cached m_pressedHyperlink which was calculated in OnPointerPressed.
            //
            // This turns out to be a fragile mechanism though, because after calculating m_pressedHyperlink,
            // OnPointerPressed immediately goes to the Hyperlink's UpdateForegroundColor, which calls UpdateUnderline.
            // CHyperlink::UpdateUnderline can mark its TextElement_TextDecorations property dirty, which propagates
            // dirtiness up to the tree until it reaches here. If we handle it naively, we'll detect the text decoration
            // change as a content-invalidating change, which invalidates the page layout but also *resets the
            // m_pressedHyperlink that was just calculated*. When OnTapped comes in later it will never find a
            // m_pressedHyperlink that matches the hit test, which then means Hyperlinks in RichTextBlocks can never be
            // tapped.
            //
            // So we can't handle this naively. Instead we have to explicitly watch for the TextElement_TextDecorations
            // change and preserve the m_pressedHyperlink that we just calculated. Conceptually this makes sense - if an
            // underline or a strikethrough somewhere changed, that shouldn't affect what Hyperlink was just pressed.
            // There might be an edge case where a tap happens to land on where an underline would have been drawn, but
            // taps aren't that precise to begin so we're willing to tolerate a 1px error.
            //
            // Also note: CTextBlock::OnContentChanged has a similar shouldInvalidateContent check, but it doesn't clear
            // m_pressedHyperlink even if the content is invalidated. These different behaviors go back to at least 2016
            // in the old os repo.
            //
            shouldClearCachedHyperlinks = false;
        }
    }

    if (shouldInvalidateContent)
    {
        InvalidateContent(shouldClearCachedHyperlinks);
        if (shouldClearCachedHyperlinks)
        {
            m_pressedHyperlink.reset();
            m_currentLink.reset();
        }

        // Clear selection, since content has changed selection start/end may no longer be valid.
        if (IsSelectionEnabled() &&
            m_pSelectionManager->GetTextSelection() != nullptr)
        {
            IFC_RETURN(m_pSelectionManager->GetTextSelection()->Select(0, 0, LineForwardCharacterBackward));
        }

        // Delete cached collection of focusable children on any content changes.
        // It will get repopulated on the next usage.
        if (m_focusableChildrenCollection != nullptr)
        {
            m_focusableChildrenCollection.reset();
        }
    }

    return S_OK;
}

RichTextServices::ILinkedTextContainer *CRichTextBlock::GetPrevious() const
{
    // CRichTextBlock is always a content owner and doesn't accept overflow,
    // there is no previous link.
    return nullptr;
}

RichTextServices::ILinkedTextContainer *CRichTextBlock::GetNext() const
{
    return m_pOverflowTarget;
}

RichTextServices::TextBreak *CRichTextBlock::GetBreak() const
{
    // In CRichTextBlock linking implementation, another container should only request the break if it is valid.
    ASSERT(m_isBreakValid == TRUE);
    return m_pBreak;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRichTextBlock::PreviousBreakUpdated
//
//  Synopsis:
//      Handler for break updated from previous link.
//
//---------------------------------------------------------------------------
RichTextServices::Result::Enum CRichTextBlock::PreviousBreakUpdated(
    _In_ RichTextServices::ILinkedTextContainer *pPrevious
    )
{
    // CRichTextBlock is always the first link, no previous break exists and since this method is
    // called by the previous link to inform the next link of a break update, it is not expected to be called here.
    return Result::InvalidOperation;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRichTextBlock::PreviousLinkAttached
//
//  Synopsis:
//      Called when an CRichTextBlock attaches itself as the previous link to this
//      object.
//
//---------------------------------------------------------------------------
RichTextServices::Result::Enum CRichTextBlock::PreviousLinkAttached(
    _In_ RichTextServices::ILinkedTextContainer *pPrevious
    )
{
    // CRichTextBlock is always the first link, no previous link can attach itself.
    return Result::InvalidOperation;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRichTextBlock::NextLinkDetached
//
//  Synopsis:
//      Called when this control's next link detaches itself.
//
//---------------------------------------------------------------------------
RichTextServices::Result::Enum CRichTextBlock::NextLinkDetached(
    _In_ RichTextServices::ILinkedTextContainer *pNext
    )
{
    Result::Enum txhr = Result::Success;

    // No need to invalidate, etc. here, removing the next link doesn't affect previous container.
    ASSERT(m_pOverflowTarget != nullptr &&
           m_pOverflowTarget == pNext);

    // Delete the linked view and go back to being a standalone CRichTextBlock. Notify selection manager of the change.
    if (m_pSelectionManager != nullptr)
    {
        IFC_FROM_HRESULT_RTS(m_pSelectionManager->TextViewChanged(m_pLinkedView, m_pTextView));
    }
    delete m_pLinkedView;
    m_pLinkedView = nullptr;

    ReleaseInterface(m_pOverflowTarget);

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRichTextBlock::PreviousLinkDetached
//
//  Synopsis:
//      Called when this control's previous link detaches itself.
//
//---------------------------------------------------------------------------
RichTextServices::Result::Enum CRichTextBlock::PreviousLinkDetached(
    _In_ RichTextServices::ILinkedTextContainer *pPrevious
    )
{
    // CRichTextBlock is always the first link, no previous link can attach/detach itself.
    return Result::InvalidOperation;
}

_Check_return_ HRESULT CRichTextBlock::HitTestLink(
    _In_ XPOINTF *ptPointer,
    _Outptr_ CHyperlink **ppLink
    )
{
    CHyperlink *pLink = nullptr;

    *ppLink = nullptr;

    IFC_RETURN(CRichTextBlock::HitTestLink(
        m_pBlocks->GetTextContainer(),
        this,
        ptPointer,
        m_pTextView,
        &pLink));
    *ppLink = pLink;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::OnSelectionChanged
//
//  Synopsis: Selection changed notification called by TextSelectionManager
//            when selection has changed. Invalidates render or arrange
//            on this element depending on theme if it is affected by
//            the change and notifies all overflows.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::OnSelectionChanged(
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
        selected = true;
    }
    else if ((previousSelectionStartOffset < contentEnd &&
              previousSelectionEndOffset >= contentStart))
    {
        // Element is not selected now, but was previously.
        unSelected = true;
    }

    if (selected || unSelected)
    {
        // Element is at least partially selected/unselected, and should be re-rendered.
        InvalidateSelectionRender();
    }

    // Invalidate content arrange on subsequent overflow elements.
    IFC_RETURN(CRichTextBlockOverflow::NotifyAllOverflowContentSelectionChanged(
        m_pOverflowTarget,
        previousSelectionStartOffset,
        previousSelectionEndOffset,
        newSelectionStartOffset,
        newSelectionEndOffset
        ));

    IFC_RETURN(RaiseSelectionChangedEvent());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::OnSelectionVisibilityChanged
//
//  Synopsis: Selection visibility changed notification called by TextSelectionManager
//            when selection has changed. Invalidates render on this element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::OnSelectionVisibilityChanged(
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

    // Invalidate content arrange on subsequent overflow elements.
    IFC_RETURN(CRichTextBlockOverflow::NotifyAllOverflowContentSelectionVisibilityChanged(
        m_pOverflowTarget,
        selectionStartOffset,
        selectionEndOffset
        ));

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::GetPlainText(
    _In_ CFrameworkElement *pElement,
    _Out_ xstring_ptr* pstrText
    )
{
    HRESULT hr = S_OK;
    CRichTextBlock *pRichTextBlock = nullptr;
    CRichTextBlockOverflow *pRichTextBlockOverflow = nullptr;
    ITextContainer *pTextContainer = nullptr;
    uint32_t contentStart = 0;
    uint32_t contentLength = 0;
    const WCHAR *pCharacters = nullptr;
    uint32_t textLength = 0;
    bool takeOwnership = false;

    IFCEXPECT_ASSERT(pElement->OfTypeByIndex<KnownTypeIndex::RichTextBlock>() ||
        pElement->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>());

    if (pElement->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        pRichTextBlock = static_cast<CRichTextBlock *>(pElement);
        contentStart = pRichTextBlock->GetContentStartPosition();
        contentLength = pRichTextBlock->GetContentLength();
    }
    else
    {
        pRichTextBlockOverflow = static_cast<CRichTextBlockOverflow *>(pElement);
        pRichTextBlock = pRichTextBlockOverflow->GetMaster();
        contentStart = pRichTextBlockOverflow->GetContentStartPosition();
        contentLength = pRichTextBlockOverflow->GetContentLength();
    }

    if (pRichTextBlock != nullptr &&
        pRichTextBlock->m_pBlocks != nullptr)
    {
        pTextContainer = pRichTextBlock->m_pBlocks->GetTextContainer();
    }

    if (contentLength > 0)
    {
        IFC(pTextContainer->GetText(
            contentStart,
            contentStart + contentLength,
            TRUE /*insertNewlines*/,
            &textLength,
            &pCharacters,
            &takeOwnership));
    }

    IFC(xstring_ptr::CloneBuffer(pCharacters, textLength, pstrText));

    if (!takeOwnership && textLength > 0)
    {
        // There should be no case where RTB text container will return a buffer
        // that is still owns.
        ASSERT(FALSE);
    }

Cleanup:
    if (takeOwnership)
    {
        delete [] pCharacters;
    }

    return hr;
}

_Check_return_ HRESULT CRichTextBlock::SelectAll()
{
    if (GetSelectionManager() != nullptr)
    {
        IFC_RETURN(GetSelectionManager()->SelectAll());
    }
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::HasOverflowContent(
    _In_ CDependencyObject *pObject,
    _In_ uint32_t cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock*>(pObject);

    if (!pRichTextBlock ||
        cArgs != 0 ||
        !pResult)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    pResult->SetBool(!!pRichTextBlock->HasOverflowContent());

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::GetSelectedText(
    _In_ CDependencyObject *pObject,
    _In_ uint32_t cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    xstring_ptr strString;
    CRichTextBlock *pRichTextBlock = do_pointer_cast<CRichTextBlock>(pObject);

    if (!pRichTextBlock || cArgs != 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    pResult->Unset();

    if (pRichTextBlock->GetSelectionManager() != nullptr)
    {
        IFC_RETURN(pRichTextBlock->GetSelectionManager()->GetSelectedText(&strString));
    }

    if (!strString.IsNull())
    {
        pResult->SetString(strString);
    }
    else
    {
        // This property is not allowed to return nullptr value, return empty.
        pResult->SetString(xstring_ptr::EmptyString());
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Summary: Called by JupiterWindowMobile when Phone copy button is pressed
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::CopySelectedText()
{
    if (IsSelectionEnabled())
    {
        IFC_RETURN(m_pSelectionManager->CopySelectionToClipboard());
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::GetContentStart(
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CPlainTextPosition textPosition;

    uint32_t contentStartPosition = 0;

    *ppTextPointerWrapper = nullptr;

    // For CRichTextBlock, blocks collection is implicitly created - so it should
    // always have a ContentStart/End value. It's OK to create the blocks collection implicitly here
    // if it hasn't yet been created.
    if (GetTextContainer() == nullptr)
    {
        IFC_RETURN(CreateBlocks());
    }
    ASSERT(GetTextContainer() != nullptr);
    contentStartPosition = GetContentStartPosition();

    // CRichTextBlock's ContentStart always has backward gravity since it's the first content position in the collection.
    textPosition = CPlainTextPosition(GetTextContainer(), contentStartPosition, LineForwardCharacterBackward);
    IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::GetContentEnd(
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CPlainTextPosition textPosition;
    uint32_t contentEndPosition = 0;
    TextGravity gravity = LineForwardCharacterBackward;

    *ppTextPointerWrapper = nullptr;

    // For CRichTextBlock, blocks collection is implicitly created - so it should
    // always have a ContentStart/End value. It's OK to create the blocks collection implicitly here
    // if it hasn't yet been created.
    if (GetTextContainer() == nullptr)
    {
        IFC_RETURN(CreateBlocks());
    }
    ASSERT(GetTextContainer() != nullptr);

    // ContentEnd position is always the position just after the end of content. If CRichTextBlock has a break,
    // its gravity is backward since the forward gravity position will be in the next link. If there is no break the gravity is forward
    // since it's the end of all content.
    contentEndPosition = GetContentStartPosition() + GetContentLength();
    if (m_pBreak == nullptr)
    {
        gravity = LineForwardCharacterForward;
    }

    textPosition = CPlainTextPosition(GetTextContainer(), contentEndPosition, gravity);
    IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::GetSelectionStart(
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CPlainTextPosition textPosition;
    CTextPosition startTextPosition;

    *ppTextPointerWrapper = nullptr;

    if (IsSelectionEnabled() &&
        m_pSelectionManager->GetTextSelection() != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->GetTextSelection()->GetStartTextPosition(&startTextPosition));
        textPosition = CPlainTextPosition(startTextPosition.GetPlainPosition());
        IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::GetSelectionEnd(
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CPlainTextPosition textPosition;
    CTextPosition endTextPosition;

    *ppTextPointerWrapper = nullptr;

    if (IsSelectionEnabled() &&
        m_pSelectionManager->GetTextSelection() != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->GetTextSelection()->GetEndTextPosition(&endTextPosition));
        textPosition = CPlainTextPosition(endTextPosition.GetPlainPosition());
        IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::Select(
    _In_ CTextPointerWrapper *pAnchorPosition,
    _In_ CTextPointerWrapper *pMovingPosition
    )
{
    if (pAnchorPosition->IsValid() &&
        pMovingPosition->IsValid())
    {
        if (IsSelectionEnabled())
        {
            IFC_RETURN(m_pSelectionManager->Select(
                pAnchorPosition->GetPlainTextPosition(),
                pMovingPosition->GetPlainTextPosition()));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Public hit-test API to get a TextPosition from point
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::GetTextPositionFromPoint(
    _In_ XPOINTF point,
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper)
{
    CPlainTextPosition textPosition;
    uint32_t position = 0;
    TextGravity gravity = LineForwardCharacterForward;

    *ppTextPointerWrapper = nullptr;

    // Use this element's standalone view to query the pixel position.
    if (m_pTextView != nullptr)
    {
        // No coordinate transformation is necessary - this API is assumed to be called w/ element-relative coordinates.
        IFC_RETURN(m_pTextView->PixelPositionToTextPosition(
            point,
            FALSE, // Recognise hits after newline.
            &position,
            &gravity));

        if (GetTextContainer() != nullptr)
        {
            textPosition = CPlainTextPosition(GetTextContainer(), position, gravity);
            IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::MeasureOverride
//
//  Synopsis: Returns the desired size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::MeasureOverride(
    XSIZEF availableSize,
    XSIZEF& desiredSize
    )
{
    xref_ptr<RichTextBlockBreak> pBreak;
    BlockNodeBreak *pOldPageBreak  = nullptr;
    CValue highContrastAdjustment;

    // Ensure any embedded UIElements are measured. If an embedded UIElement is dirty for measure, the
    // RichTextBlock will be put on the measure path. Typically, this doesn't result in MeasureOverride being
    // called - instead CUIElement::Measure will skip down to the dirty children and just Measure them. However,
    // in situations where the RichTextBlock itself is also dirty for measure, the base implementation will not
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

    IFC_RETURN(EnsureBlockLayout());

    // Always use the RichTextBlockBreak object to retrive the old page break. If the PageNode has not been
    // deleted it should be exactly the same as the the page node's break.
    if (m_pBreak != nullptr)
    {
        pOldPageBreak = m_pBreak->GetBlockBreak();
    }

    if (m_pPageNode != nullptr)
    {
        IFC_RETURN(m_pPageNode->Measure(availableSize, m_maxLines, 0.0f, FALSE, FALSE, TRUE, nullptr, nullptr));
        desiredSize = m_pPageNode->GetDesiredSize();

        // If PageNode bypasses Measure, its break will be exactly the same,
        // and there's no need to notify overflow elements, etc. in this case.
        // If it's not the same update the container's break and set it, notifying overflows.
        // We use ref equals here and not BlockNodeBreak::Equals because we want RichTextBlockBreak to contain
        // the page node's actual break, not someting that's semantically the same. If the PageNode
        // didn't bypass measure and produced a new break that evaluates the same using BlockNodeBreak::Equals,
        // we'll still replace our break, but that's OK - block layout should detect this on bypass checks.
        // TODO: Consider using Equals and just swapping out break objects if it's the same,
        // without updating break on overflows which will invalidate their Measure, etc. If the break really is semantically
        // equal it won't matter too much since any overflow page nodes will check for that during bypass checks, so it
        // may not be worth it.
        if (m_pPageNode->GetBreak() != nullptr)
        {
            if (m_pPageNode->GetBreak() != pOldPageBreak)
            {
                pBreak = make_xref<RichTextBlockBreak>(m_pPageNode->GetBreak());
                IFC_RETURN(SetBreak(pBreak.get()));
            }
        }
        else
        {
            IFC_RETURN(SetBreak(nullptr));
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

    // ElementHighContrastAdjustment is an inherited property so we don't get a Property change when the element is added to the tree, we
    // check the property here to make sure it is current.
    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::UIElement_HighContrastAdjustment, &highContrastAdjustment));
    IFC_RETURN(OnHighContrastAdjustmentChanged(static_cast<DirectUI::ElementHighContrastAdjustment>(highContrastAdjustment.AsEnum())));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::ArrangeOverride
//
//  Synopsis: Returns the final render size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::ArrangeOverride(
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

        // Arrange and Render the page.
        IFC_RETURN(m_pPageNode->Arrange(finalSize));
        renderSize = m_pPageNode->GetRenderSize();

        std::shared_ptr<HighlightRegion> selection;
        if (m_pTextView != nullptr &&
            IsSelectionEnabled() &&
            m_pSelectionManager->IsSelectionVisible())
        {
            IFC_RETURN(m_pSelectionManager->GetSelectionHighlightRegion(UseHighContrastSelection(GetContext()), selection));
        }

        // Set DrawingContext properties to handle foreground color when BackPlate is enabled. This must be set every ArrangeOverride because
        // elements inside the DrawingContext can change.
        m_pPageNode->GetDrawingContext()->SetControlEnabled(IsEnabled());
        m_pPageNode->GetDrawingContext()->SetBackPlateConfiguration(IsHighContrastAdjustmentActive(), false /*useHyperlinkForeground*/);

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
_Check_return_ HRESULT CRichTextBlock::UpdateIsTextTrimmed()
{
    auto last_isTextTrimmed = m_isTextTrimmed;

    if (HasOverflowContent())
    {
        m_isTextTrimmed = true;
    }
    else
    {
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
    }

    if (last_isTextTrimmed != m_isTextTrimmed)
    {
        RaiseIsTextTrimmedChangedEvent();

        CValue oldValue;
        oldValue.Set<valueBool>(last_isTextTrimmed);
        CValue newValue;
        newValue.Set<valueBool>(m_isTextTrimmed);
        IFC_RETURN(NotifyPropertyChanged(
            PropertyChangedParams(
                DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RichTextBlock_IsTextTrimmed),
                oldValue,
                newValue
            )));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::RaiseIsTextTrimmedChangedEvent
//
//  Synopsis: Raises event for IsTextTrimmedChanged.
//
//------------------------------------------------------------------------
void CRichTextBlock::RaiseIsTextTrimmedChangedEvent()
{
    CEventManager *const eventManager = GetContext()->GetEventManager();
    // Create the DO that represents the event args.
    xref_ptr<CIsTextTrimmedChangedEventArgs> args;
    args.init(new CIsTextTrimmedChangedEventArgs());
    // Raise event.
    eventManager->Raise(EventHandle(KnownEventIndex::RichTextBlock_IsTextTrimmedChanged), true, this, args);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Mark state as dirty when an inherited property changes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::MarkInheritedPropertyDirty(
    _In_ const CDependencyProperty* pdp,
    _In_ const CValue* pValue)
{
    IFC_RETURN(CFrameworkElement::MarkInheritedPropertyDirty(pdp, pValue));

    if (!IsForegroundPropertyIndex(pdp->GetIndex()))
    {
        InvalidateContent();
    }
    else
    {
        InvalidateRender();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  NotifyThemeChangedCore
//
//  Notify collection and its brush that the theme has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::NotifyThemeChangedCore(
    _In_ Theming::Theme theme,
    _In_ bool fForceRefresh)
{
    IFC_RETURN(CFrameworkElement::NotifyThemeChangedCore(theme, fForceRefresh));

    // Theme change may have changed high contrast mode and we disable color fonts
    // in high contrast.  Translation of color fonts happens during arrange so we
    // need to re-run arrange to re-apply that decision.
    if (m_isColorFontEnabled)
    {
        InvalidateContentArrange();
    }

    IFC_RETURN(EnsureBlockLayout());
    UpdateBackPlateForegroundOverride();

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::NotifyApplicationHighContrastAdjustmentChanged()
{
    DirectUI::ApplicationHighContrastAdjustment applicationHighContrastAdjustment;
    IFC_RETURN(CApplication::GetApplicationHighContrastAdjustment(&applicationHighContrastAdjustment));

    IFC_RETURN(OnApplicationHighContrastAdjustmentChanged(applicationHighContrastAdjustment));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::EnsureFontContext
//
//  Synopsis: Makes sure the m_pFontContext is both present and up to date.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::EnsureFontContext()
{
    CTextCore           *pTextCore   = nullptr;
    xref_ptr<IFontCollection> pFontCollection(m_pFontCollection);

    if (pFontCollection == nullptr)
    {
        IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
        IFontAndScriptServices *pFontAndScriptServices = nullptr;
        IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));
        IFC_RETURN(pFontAndScriptServices->GetSystemFontCollection(pFontCollection.ReleaseAndGetAddressOf()));
    }

    if ((m_pFontContext != nullptr) && (m_pFontContext->GetBaseUri() == nullptr))
    {
        // our font context doesn't have a BaseUri, it might have been prepared while the element was not yet in the tree
        m_pFontContext->SetBaseUri(GetBaseUri());
    }

    if (m_pFontContext == nullptr)
    {
        m_pFontContext = new CFontContext(pFontCollection.get(), GetBaseUri());
    }
    else
    {
        m_pFontContext->SetFontSource(pFontCollection.get());
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::CreateTextSelectionManager()
{
    IFC_RETURN(TextSelectionManager::Create(this, m_pBlocks->GetTextContainer(), &m_pSelectionManager));

    UpdateSelectionHighlightColor();

    // Set the selection manager with the linked view if one exists.
    if (m_pLinkedView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->TextViewChanged(nullptr, m_pLinkedView));
    }
    else if (m_pTextView != nullptr) // set to local view if exists
    {
        IFC_RETURN(m_pSelectionManager->TextViewChanged(nullptr, m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::EnsureBlockLayout()
{
    HRESULT hr = S_OK;
    BlockNode *pBlockNode = nullptr;

    IFC(EnsureTextFormattingForRead());
    IFC(EnsureInheritedPropertiesForRead());

    // RichTextBlock should format one line of content  at its default properties even if there are no blocks in the collection.
    // PageNode will work fine with an empty Blocks collection, and since this collection is created implicitly on any access, its
    // fine to create it here.
    if (m_pBlocks == nullptr)
    {
        IFC(CreateBlocks());
    }

    if (m_pBlockLayout == nullptr)
    {
        IFC(EnsureFontContext());

        if (m_pTextFormatter == nullptr)
        {
            CTextCore *pTextCore = nullptr;
            RichTextServices::TextFormatterCache *pTextFormatterCache;

            IFC(GetContext()->GetTextCore(&pTextCore));
            IFC(pTextCore->GetTextFormatterCache(&pTextFormatterCache));
            pTextFormatterCache->AcquireTextFormatter(&m_pTextFormatter);
        }

        m_pBlockLayout = new BlockLayoutEngine(this);
    }

    // Since IsTextSelectionEnabled=TRUE by default on CRichTextBlock, TextSelectionManager needs to be
    // created once in the default case. It can otherwise be deleted/created on property
    // change notifications.
    // Create TextSelectionManager. Check if m_pTextView is nullptr because even thought SelectionManager will
    // accept a linked view if there is one, this is the master object and there's no meaning for a linked
    // view without a local TextView for this object.
    if (m_pSelectionManager == nullptr &&
        (m_isTextSelectionEnabled || IsHighContrastAdjustmentActive()))
    {
        IFC(CreateTextSelectionManager());
    }

    if (m_pPageNode == nullptr)
    {
        IFC(m_pBlockLayout->CreatePageNode(m_pBlocks, this, &pBlockNode));
        m_pPageNode = static_cast<PageNode *>(pBlockNode);
        pBlockNode = nullptr;
    }

    // If there is a valid PageNode, there is content that will be laid out. Create a TextView.
    if (m_pPageNode != nullptr &&
        m_pTextView == nullptr)
    {
        m_pTextView = new RichTextBlockView(static_cast<PageNode *>(m_pPageNode));

        // If there is no linked view, TextSelectionManager needs to be set to the local view.
        if (m_pSelectionManager != nullptr &&
            m_pLinkedView == nullptr)
        {
            IFC(m_pSelectionManager->TextViewChanged(nullptr, m_pTextView));
        }
    }

Cleanup:
    delete pBlockNode;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::CreateBlocks
//
//  Synopsis: Creates the blocks property, using SetValue to ensure
//            that the property system knows the property is not defaulted.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::CreateBlocks()
{
    HRESULT hr = S_OK;
    CBlockCollection *pBlocks = nullptr;
    CREATEPARAMETERS createParameters(GetContext());
    CValue value;

    IFCEXPECT(m_pBlocks == nullptr);

    IFC(CreateDO(&pBlocks, &createParameters));
    value.WrapObjectNoRef(pBlocks);

    IFC(CFrameworkElement::SetValueByKnownIndex(KnownPropertyIndex::RichTextBlock_Blocks, value));

Cleanup:
    ReleaseInterface(pBlocks);
    return hr;
}

_Check_return_ HRESULT CRichTextBlock::CreateTextHighlighters()
{
    ASSERT(m_textHighlighters == nullptr);

    xref_ptr<CTextHighlighterCollection> textHighlighterCollection;

    CREATEPARAMETERS createParameters(GetContext());
    IFC_RETURN(CreateDO(textHighlighterCollection.ReleaseAndGetAddressOf(), &createParameters));
    IFC_RETURN(textHighlighterCollection->SetOwner(this));

    CValue value;
    value.WrapObjectNoRef(textHighlighterCollection.get());

    // This will also set the m_textHighlighters property via metadata table offset
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::RichTextBlock_TextHighlighters, value));

    return S_OK;
}

//------------------------------------------------------------------------
//
//
//  Synopsis: Creates the SelectionHighlightColor property, using SetValue to ensure
//            that the property system knows the property is not defaulted.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::CreateSelectionHighlightColor()
{
    HRESULT hr = S_OK;
    CSolidColorBrush *pHighlightColorBrush  = nullptr;
    CREATEPARAMETERS createParameters(GetContext());
    CValue value;

    IFCEXPECT(m_pSelectionHighlightColor == nullptr);

    IFC(CreateDO(&pHighlightColorBrush, &createParameters));
    value.SetColor(GetDefaultSelectionHighlightColor());
    IFC(CFrameworkElement::SetValueByKnownIndex(KnownPropertyIndex::RichTextBlock_SelectionHighlightColor, value));

Cleanup:
    ReleaseInterface(pHighlightColorBrush);
    return hr;
}

//------------------------------------------------------------------------
//
//
//  Synopsis: Update selection manager's highlight selection color,
//  use the default color if the SelectionHighlightColor property is set to nullptr
//
//------------------------------------------------------------------------
void CRichTextBlock::UpdateSelectionHighlightColor()
{
    uint32_t selectionHighlightColor;
    if (m_pSelectionManager)
    {
        if (m_pSelectionHighlightColor)
        {
            selectionHighlightColor = m_pSelectionHighlightColor->m_rgb;
        }
        else
        {
            selectionHighlightColor = GetDefaultSelectionHighlightColor();
        }
        IFCFAILFAST(m_pSelectionManager->SetSelectionHighlightColor(selectionHighlightColor));
        InvalidateRender();
    }
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::InvalidateFontSize
//
//  Synopsis: Invalidates the FontSize property.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::InvalidateFontSize()
{
    const CDependencyProperty *pFontSizeProperty = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RichTextBlock_FontSize);
    CValue value;

    IFC_RETURN(GetValueInherited(pFontSizeProperty, &value));
    IFC_RETURN(MarkInheritedPropertyDirty(pFontSizeProperty, &value));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::InvalidateContent
//
//  Synopsis: Invalidates layout and stored information about content,
//            e.g. run caches.
//
//------------------------------------------------------------------------
void CRichTextBlock::InvalidateContent(const bool clearCachedLinks)
{
    // Invalidate content for the block layout engine, which invalidates all layout as well.
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->InvalidateContent();
    }

    m_isBreakValid = FALSE;

    InvalidateMeasure();
    InvalidateRender();

    // Invalidate content on subsequent overflow elements.
    CRichTextBlockOverflow::InvalidateAllOverflowContent(m_pOverflowTarget, clearCachedLinks);
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::InvalidateContentMeasure
//
//  Synopsis: Invalidate layout for all content, including
//            arrange/render info.
//
//------------------------------------------------------------------------
void CRichTextBlock::InvalidateContentMeasure()
{
    // Invalidate measure for the block layout engine, which invalidates arrange as well.
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->InvalidateMeasure();
    }

    m_isBreakValid = FALSE;

    InvalidateMeasure();
    InvalidateRender();

    // Invalidate content measure on subsequent overflow elements.
    CRichTextBlockOverflow::InvalidateAllOverflowContentMeasure(m_pOverflowTarget);
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::InvalidateContentArrange
//
//  Synopsis: Invalidate arrange and render data for content.
//
//------------------------------------------------------------------------
void CRichTextBlock::InvalidateContentArrange()
{
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->InvalidateArrange();
    }

    InvalidateArrange();
    InvalidateRender();

    // Invalidate content arrange on subsequent overflow elements.
    CRichTextBlockOverflow::InvalidateAllOverflowContentArrange(m_pOverflowTarget);
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::InvalidateRender
//
//  Synopsis: Invalidates rendering data and releases render cache.
//
//------------------------------------------------------------------------
void CRichTextBlock::InvalidateRender()
{
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);

    // Invalidate render on subsequent overflow elements.
    CRichTextBlockOverflow::InvalidateAllOverflowRender(m_pOverflowTarget);
}

void  CRichTextBlock::InvalidateSelectionRender()
{
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
}

//------------------------------------------------------------------------
//
//  Method:    CRichTextBlock::OnCreateAutomationPeerImpl
//
//  Synopsis:  Creates and returns CAutomationPeer associated with this
//             CRichTextBlock
//
//------------------------------------------------------------------------
CAutomationPeer* CRichTextBlock::OnCreateAutomationPeerImpl()
{
    // Automation peer is implemented in managed code in Silverlight.
    return nullptr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRichTextBlock::ResolveNextLink
//
//  Synopsis:
//      Connects the next link and notifies it that a previous link has attached
//      itself.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::ResolveNextLink(
    )
{
    // Clears the previous link, resets to local backing store and informs the next link of the backing store switch.
    HRESULT hr = S_OK;

    if (m_pOverflowTarget != nullptr)
    {
        // Create a linked view. At this stage we should not have a linked view - if overflow target was changed it should
        // have been reset when the previous target detached.
        // If there is no linked view, create one and notify selection manager of the change.
        if (m_pLinkedView == nullptr)
        {
            m_pLinkedView = new LinkedRichTextBlockView(this);
            if (m_pSelectionManager != nullptr)
            {
                IFC(m_pSelectionManager->TextViewChanged(m_pTextView, m_pLinkedView));
            }
        }
        IFC(RichTextServicesHelper::MapTxErr(m_pOverflowTarget->PreviousLinkAttached(this)));
    }
    else
    {
        // If overflow target was set to nullptr, delete the linked view and go back to being a standalone CRichTextBlock.
        if (m_pSelectionManager != nullptr)
        {
            IFC(m_pSelectionManager->TextViewChanged(m_pLinkedView, m_pTextView));
        }
        delete m_pLinkedView;
        m_pLinkedView = nullptr;
    }

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRichTextBlock::InvalidateNextLink
//
//  Synopsis:
//      Invalidates information stored about the next link.
//      Called when the next link changes or is detached.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::InvalidateNextLink()
{
    // Notify the next link, that the previous link has been detached.
    if (m_pOverflowTarget != nullptr)
    {
        IFC_RETURN(RichTextServicesHelper::MapTxErr(m_pOverflowTarget->PreviousLinkDetached(this)));
    }

    ReleaseInterface(m_pOverflowTarget);
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::SetBreak(
    _In_ RichTextBlockBreak *pBreak
    )
{
    bool hasBreak = (nullptr != m_pBreak);
    bool hasNewBreak = (nullptr != pBreak);

    ReplaceInterface(m_pBreak, pBreak);
    m_isBreakValid = TRUE;

    if (hasBreak != hasNewBreak)
    {
        // Since HasOverflowContent returns (m_pBreak != nullptr), fire a property
        // changed event if the value of m_pBreak has changed between null and
        // non-null
        CValue value;
        IFC_RETURN(HasOverflowContent(this, 0, /* pArgs */ nullptr, /* pValueOuter */ nullptr, &value));
        IFC_RETURN(NotifyPropertyChanged(
            PropertyChangedParams(
                DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RichTextBlock_HasOverflowContent),
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

_Check_return_ HRESULT CRichTextBlock::UpdateSelectionAndHighlightRegions(_Out_ bool *redrawForHighlightRegions)
{
    *redrawForHighlightRegions = false;
    std::shared_ptr<HighlightRegion> selection = nullptr;
    m_highlightRegions.clear();

    // Find text selection.
    if (m_pSelectionManager != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->GetSelectionHighlightRegion(UseHighContrastSelection(GetContext()), selection));
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

// return contained hyplink Text Element if it is currently focused and focus rect should be drawn
CTextElement* CRichTextBlock::GetTextElementForFocusRect()
{
    if (m_pTextView != nullptr
        && !CFocusRectManager::AreHighVisibilityFocusRectsEnabled())
    {
        CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(this);
        if (pFocusManager)
        {
            CTextElement* pTextElement = pFocusManager->GetTextElementForFocusRectCandidate();

            if (pTextElement && pTextElement->GetContainingFrameworkElement() == this)
            {
                return pTextElement;
            }
        }
    }
    return nullptr;
}

_Check_return_ HRESULT CRichTextBlock::D2DPreChildrenRenderVirtual(
    _In_ const SharedRenderParams& sharedRP,
    _In_ const D2DRenderParams& d2dRP
    )
{
    if (m_pPageNode != nullptr &&
        !m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        ASSERT(m_pPageNode->GetDrawingContext() != nullptr);

        if (m_pSelectionManager && m_pTextView)
        {
            IFC_RETURN(m_pSelectionManager->D2DRender(
                d2dRP,
                m_pTextView,
                UseHighContrastSelection(GetContext()),
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
_Check_return_ HRESULT CRichTextBlock::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams
    )
{
    IFC_RETURN(D2DEnsureResources(cp, sharedPrintParams.pCurrentTransform));
    IFC_RETURN(D2DPreChildrenRenderVirtual(sharedPrintParams, printParams));

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::D2DEnsureResources(
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

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::RaiseSelectionChangedEvent
//
//  Synopsis: Raises routed event for SelectionChanged.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::RaiseSelectionChangedEvent()
{
    CEventManager *pEventManager = nullptr;
    xref_ptr<CRoutedEventArgs> pArgs;
    CValue value;

    auto core = GetContext();
    // Parser setting selection properties should not fire SelectionChanged event.
    if (!ParserOwnsParent())
    {
        IFCEXPECT_ASSERT_RETURN(core);

        pEventManager = core->GetEventManager();
        if (pEventManager)
        {
            pArgs = make_xref<CRoutedEventArgs>();

            // Raise event.
            pEventManager->RaiseRoutedEvent(EventHandle(KnownEventIndex::RichTextBlock_SelectionChanged), this, pArgs.get());
            if (core->UIAClientsAreListening(UIAXcp::AETextPatternOnTextSelectionChanged) == S_OK)
            {
                if(m_pAP == nullptr)
                {
                    OnCreateAutomationPeer();
                }
                else
                {
                    m_pAP->RaiseAutomationEvent(UIAXcp::AETextPatternOnTextSelectionChanged);
                }
            }
        }
    }

    // Raise a property changed event for the SelectedText property
    IFC_RETURN(GetSelectedText(this, 0, /* pArgs */ nullptr, /* pValueOuter */ nullptr, &value));
    IFC_RETURN(NotifyPropertyChanged(
        PropertyChangedParams(
            DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RichTextBlock_SelectedText),
            CValue(),
            value)));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::OnSelectionEnabledChanged
//
//  Synopsis: Creates/deletes TextSelectionManager if selection is enabled/
//            disabled.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::OnSelectionEnabledChanged(bool oldValue)
{
    if (oldValue != m_isTextSelectionEnabled)
    {
        MouseCursor mouseCursor;

        if (oldValue)
        {
            bool raiseEvent = false;
            mouseCursor = MouseCursorDefault;

            IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(GetSelectionFlyoutNoRef()));

            // If current selection is non-empty, raise selection changed event.
            if (IsSelectionEnabled() &&
                m_pSelectionManager->GetTextSelection() != nullptr &&
                !(m_pSelectionManager->GetTextSelection()->IsEmpty()))
            {
                raiseEvent = true;
            }

            // Selection went from enabled to disabled. Delete selection manager if BackPlate doesn't need it.
            if (!IsHighContrastAdjustmentEnabled())
            {
                IFC_RETURN(TextSelectionManager::Destroy(&m_pSelectionManager));
            }
            else
            {
                IFC_RETURN(m_pSelectionManager->ReleaseGrippers());
            }

            if (raiseEvent)
            {
                IFC_RETURN(RaiseSelectionChangedEvent());
            }
        }
        else
        {
            mouseCursor = MouseCursorIBeam;

            // Selection went from disabled to enabled. Create selection manager if it doesn't exist.
            if (m_pSelectionManager == nullptr)
            {
                if (m_pBlocks != nullptr)
                {
                    IFC_RETURN(CreateTextSelectionManager());
                }
            }
        }


        IFC_RETURN(SetCursor(mouseCursor));
        CRichTextBlockOverflow *pRichTextBlockOverflow = m_pOverflowTarget;
        while (pRichTextBlockOverflow)
        {
            IFC_RETURN(pRichTextBlockOverflow->SetCursor(mouseCursor));
            pRichTextBlockOverflow = pRichTextBlockOverflow->m_pOverflowTarget;
        }

        InvalidateRender();
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::OnHighContrastAdjustmentChanged(
    _In_ DirectUI::ElementHighContrastAdjustment newValue
    )
{
    if (Convert(newValue) != m_highContrastAdjustment)
    {
        m_highContrastAdjustment = Convert(newValue);

        // ApplicationHighContrastAdjustment is not propagated through the tree on app start up, we have to make this check to ensure our default value
        // matches the value set in the application XAML.
        if (m_highContrastAdjustment == Convert(DirectUI::ElementHighContrastAdjustment::Application))
        {
            DirectUI::ApplicationHighContrastAdjustment applicationHighContrastAdjustment;
            IFC_RETURN(CApplication::GetApplicationHighContrastAdjustment(&applicationHighContrastAdjustment));

            m_applicationHighContrastAdjustment = Convert(applicationHighContrastAdjustment);
        }

        IFC_RETURN(UpdateHighContrastAdjustments());
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::OnApplicationHighContrastAdjustmentChanged(
    _In_ DirectUI::ApplicationHighContrastAdjustment newValue
)
{
    if (Convert(newValue) != m_applicationHighContrastAdjustment)
    {
        m_applicationHighContrastAdjustment = Convert(newValue);

        IFC_RETURN(UpdateHighContrastAdjustments());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::UpdateHighContrastAdjustments
//
//  Synopsis: Creates/deletes TextSelectionManager if HighContrastAdjustments are enabled/
//            disabled.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::UpdateHighContrastAdjustments()
{
    if (!IsHighContrastAdjustmentEnabled())
    {
        // BackPlate was set to disabled. Delete selection manager if selection doesn't need it.
        if (m_pSelectionManager && !m_isTextSelectionEnabled)
        {
            IFC_RETURN(TextSelectionManager::Destroy(&m_pSelectionManager));
        }
    }
    else
    {
        // BackPlate was enabled. Create selection manager if it doesn't exist.
        IFC_RETURN(EnsureBackPlateDependencies());
    }

    UpdateBackPlateForegroundOverride();
    InvalidateContentArrange();

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::EnsureBackPlateDependencies()
{
    // Create dependencies for the first time only when the BackPlate is active and HighCotrast is enabled.
    if (IsHighContrastAdjustmentActive())
    {
        if (m_pSelectionManager == nullptr)
        {
            if (m_pBlocks != nullptr)
            {
                IFC_RETURN(CreateTextSelectionManager());
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::GenerateContentBounds(
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
_Check_return_ HRESULT CRichTextBlock::HitTestLocalInternal(
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
_Check_return_ HRESULT CRichTextBlock::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    XRECTF_RB innerBounds =  { };

    IFC_RETURN(GetInnerBounds(&innerBounds));

    *pHit = target.IntersectsRect(innerBounds);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Overridden to remove inlined UIElements from the tree.
//      Due to ordering issues when Clear-ing an ancestor's child collection, side effects
//      that modify the tree need to occur during Shutdown instead of just during Leave.
//
//------------------------------------------------------------------------
void
CRichTextBlock::Shutdown()
{
    if (m_pBlocks)
    {
        uint32_t numBlocks = m_pBlocks->GetCount();
        for (uint32_t i = 0; i < numBlocks; i++)
        {
            CDependencyObject *pBlock = static_cast<CDependencyObject*>(m_pBlocks->GetItemWithAddRef(i));
            CParagraph *pParagraphNoRef = do_pointer_cast<CParagraph>(pBlock);
            if (pParagraphNoRef)
            {
                VERIFYHR(pParagraphNoRef->Shutdown());
            }

            ReleaseInterface(pBlock);
        }
    }

    CFrameworkElement::Shutdown();
}

_Check_return_ XUINT32 CRichTextBlock::GetLinkAPChildrenCount()
{
    if (!m_pBlocks)
        return 0;

    unsigned int linkAPCount = 0;

    const uint32_t numBlocks = m_pBlocks->GetCount();
    for (uint32_t i = 0; i < numBlocks; i++)
    {
        CDependencyObject *pBlock = static_cast<CDependencyObject*>(m_pBlocks->GetItemImpl(i));
        auto inlines = static_cast<CParagraph*>(pBlock)->GetInlineCollection();
        linkAPCount += GetLinkAPChildrenCountHelper(inlines);
    }
    return linkAPCount;
}

_Check_return_ XUINT32 CRichTextBlock::GetLinkAPChildrenCountHelper(_In_opt_ CInlineCollection* inlines)
{
    if (inlines == nullptr)
        return 0;

    unsigned int linkAPCount = 0;

    auto& inlineCollection = inlines->GetCollection();
    for (const auto& in : inlineCollection)
    {
        if (in->OfTypeByIndex<KnownTypeIndex::Hyperlink>())
        {
            linkAPCount++;
        }
        else if (in->OfTypeByIndex<KnownTypeIndex::Span>())
        {
            linkAPCount += GetLinkAPChildrenCountHelper(static_cast<CSpan*>(in)->GetInlineCollection());
        }
    }
    return linkAPCount;
}

//------------------------------------------------------------------------
//
//  Method:  GetFocusableChildrenHelper
//
//  Summary: Navigates through the TextElement collection and retrieves
//           list of focusable element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::GetFocusableChildrenHelper(
    _In_ CDOCollection *pFocusChildren,
    _In_ CTextElementCollection *pCollection
    )
{
    HRESULT hr = S_OK;
    CDependencyObject *pObject = nullptr;

    for (uint32_t i = 0, count = pCollection->GetCount(); i < count; i++)
    {
        pObject = pCollection->GetItemDOWithAddRef(i);
        if (pObject)
        {
            if (CFocusableHelper::IsFocusableDO(pObject))
            {
                IFC(pFocusChildren->Append(pObject));
            }
            else if (pObject->OfTypeByIndex<KnownTypeIndex::InlineUIContainer>())
            {
                CInlineUIContainer *pInlineUIContainer = static_cast<CInlineUIContainer *>(pObject);
                if (pInlineUIContainer->m_pChild != nullptr)
                {
                    IFC(pFocusChildren->Append(pInlineUIContainer->m_pChild));
                }
            }
            else if (pObject->OfTypeByIndex<KnownTypeIndex::Paragraph>())
            {
                CParagraph *pParagraph = static_cast<CParagraph *>(pObject);
                if (pParagraph->m_pInlines != nullptr)
                {
                    IFC(GetFocusableChildrenHelper(pFocusChildren, pParagraph->m_pInlines));
                }
            }
            else if (pObject->OfTypeByIndex<KnownTypeIndex::Span>())
            {
                CSpan *pSpan = static_cast<CSpan *>(pObject);
                if (pSpan->m_pInlines != nullptr)
                {
                    IFC(GetFocusableChildrenHelper(pFocusChildren, pSpan->m_pInlines));
                }
            }
        }
        ReleaseInterface(pObject);
    }

Cleanup:
    ReleaseInterface(pObject);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:  GetFocusableChildren
//
//  Summary: Builds a collection of elements for the focus manager to
//           discover and work with for tab navigation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::GetFocusableChildren(
    _Outptr_result_maybenull_ CDOCollection **ppFocusableChildren
    )
{
    IFCEXPECT_ASSERT_RETURN(ppFocusableChildren);
    *ppFocusableChildren = nullptr;

    if (m_pBlocks != nullptr)
    {
        if (m_focusableChildrenCollection == nullptr)
        {
            m_focusableChildrenCollection = make_xref<RichTextBlockFocusableChildrenCollection>(GetContext());
            IFC_RETURN(GetFocusableChildrenHelper(m_focusableChildrenCollection.get(), m_pBlocks));
        }

        if (m_focusableChildrenCollection->GetCount() != 0)
        {
            *ppFocusableChildren = m_focusableChildrenCollection.get();
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  HitTestLink
//
//  Summary: Hittest content and retrieve a Hyperlink, if one has been hit.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::HitTestLink(
    _In_ ITextContainer *pTextContainer,
    _In_ CUIElement *pSender,
    _In_ XPOINTF  *ptPointer,
    _In_ ITextView *pTextView,
    _Outptr_ CHyperlink **ppLink
    )
{
    HRESULT hr = S_OK;
    uint32_t hittestOffset;
    TextGravity hittestGravity;
    CTextElement *pTextElement;
    CDependencyObject *pElement;
    ITransformer *pTransformer = nullptr;
    XPOINTF localPoint = {0};

    *ppLink = nullptr;

    // Retrieve the containing element for this position from TextContainer.
    IFC(pSender->TransformToRoot(&pTransformer));
    IFC(pTransformer->ReverseTransform(ptPointer, &localPoint, 1));
    IFC(pTextView->PixelPositionToTextPosition(localPoint, FALSE, &hittestOffset, &hittestGravity));
    IFC(pTextContainer->GetContainingElement(hittestOffset, &pTextElement));

    pElement = pTextElement;
    while (pElement != nullptr &&
        !pElement->OfTypeByIndex<KnownTypeIndex::Hyperlink>() &&
        !pElement->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
    {
        pElement = pElement->GetParentInternal(false);
    }

    if (pElement != nullptr &&
        (pElement->OfTypeByIndex<KnownTypeIndex::Hyperlink>()))
    {
        *ppLink = static_cast<CHyperlink *>(pElement);
    }

Cleanup:
    ReleaseInterface(pTransformer);
    return hr;
}

// Render focus rectangles for the currently focused TextElement.
_Check_return_ HRESULT CRichTextBlock::HWRenderFocusRects(
    _In_ IContentRenderer* pContentRenderer,
    _In_ CTextElement *pFocusedElement,
    _In_ ITextView *pTextView
    )
{
    HRESULT hr = S_OK;
    uint32_t cFocusRects = 0;
    XRECTF *pFocusRects = nullptr;

    IFC(RichTextBlockView::GetBoundsCollectionForElement(pTextView, pFocusedElement, &cFocusRects, &pFocusRects));

    if (cFocusRects > 0)
    {
        for (uint32_t i = 0; i < cFocusRects; i++)
        {
            XRECTF rect = pFocusRects[i];

            if (!IsEmptyRectF(rect))
            {
                CMILMatrix transformToRoot = pContentRenderer->GetTransformToRoot()->Get2DTransformToRoot(pContentRenderer->GetUIElement());

                IFC(CTextBoxHelpers::GetPixelSnappedRectangle(
                    &transformToRoot,
                    FocusRectangle,
                    0.0f,
                    &rect));

                // Find the colors from generic.xaml
                xref_ptr<CDependencyObject> pFocusBrushDO = nullptr;
                CBrush* pFocusHighBrushNoRef = nullptr;

                IFC(pFocusedElement->GetContext()->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"SystemControlForegroundBaseHighBrush"), pFocusBrushDO.ReleaseAndGetAddressOf()));
                if (pFocusBrushDO)
                {
                    pFocusHighBrushNoRef = static_cast<CBrush*>(pFocusBrushDO.get());

                    IFC(pContentRenderer->RenderFocusRectangle(
                        pFocusedElement->GetContext(),
                        rect,
                        false, /* isContinuous */
                        0.5f,
                        1.0f,
                        pFocusHighBrushNoRef
                        ));
                }

                CBrush* pFocusHighAltBrushNoRef = nullptr;

                IFC(pFocusedElement->GetContext()->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"SystemControlForegroundAltHighBrush"), pFocusBrushDO.ReleaseAndGetAddressOf()));
                if (pFocusBrushDO)
                {
                    pFocusHighAltBrushNoRef = static_cast<CBrush*>(pFocusBrushDO.get());

                    IFC(pContentRenderer->RenderFocusRectangle(
                        pFocusedElement->GetContext(),
                        rect,
                        false, /* isContinuous */
                        1.5f,
                        1.0f,
                        pFocusHighAltBrushNoRef
                        ));
                }
            }
        }
    }

Cleanup:
    delete [] pFocusRects;

    return hr;
}

//------------------------------------------------------------------------
//
//  Method:  CRichTextBlock::GetTextElementBoundRect
//
//  Summary: Calculate bounding rectangles for the specifed TextElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::GetTextElementBoundRect(
    _In_ CTextElement *pElement,
    _Out_ XRECTF *pRectFocus,
    _In_ bool ignoreClip
    )
{
    return CRichTextBlock::GetTextElementBoundRect(pElement, m_pTextView, this, pRectFocus, ignoreClip);
}


//------------------------------------------------------------------------
//
//  Method:  CRichTextBlock::GetTextElementBoundRect
//
//  Summary: Calculate bounding rectangles for the specifed TextElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::GetTextElementBoundRect(
        _In_ CTextElement *pElement,
        _In_ ITextView *pTextView,
        _In_ CUIElement *pTextControl,
        _Out_ XRECTF *pRectBound,
        _In_ bool ignoreClip
    )
{
    HRESULT hr = S_OK;
    uint32_t cBoundRects = 0;
    XRECTF *pBoundRects = nullptr;
    CTextPointerWrapper *pContentStart = nullptr;
    CTextPointerWrapper *pContentEnd = nullptr;
    XINT32 contentStartOffset;
    XINT32 contentEndOffset;

    IFC(pElement->GetContentStart(&pContentStart));
    IFC(pElement->GetContentEnd(&pContentEnd));
    IFC(pContentStart->GetOffset(&contentStartOffset));
    IFC(pContentEnd->GetOffset(&contentEndOffset));

    //  There is no bounding box API for TextElement
    //  Hence we ask the parent container UIElement to calculate the bounds
    IFC(pTextView->TextRangeToTextBounds(
        contentStartOffset,
        contentEndOffset,
        &cBoundRects,
        &pBoundRects));

    if (cBoundRects > 0)
    {
        //  since there can be multiple bounding rects as they wrap we need to unioning them together
        *pRectBound = pBoundRects[0];
        for (uint32_t i = 1; i < cBoundRects; i++)
        {
            UnionRectF(pRectBound, &pBoundRects[i]);
        }

        XRECTF_RB rtGlobalBounds = { };
        IFC(pTextControl->TransformToWorldSpace(&(ToXRectFRB(*pRectBound)), &rtGlobalBounds, ignoreClip, false /* ignoreClippingOnScrollContentPresenters */, false /* useTargetInformation */));
        *pRectBound = ToXRectF(rtGlobalBounds);
    }

Cleanup:
    delete [] pBoundRects;
    ReleaseInterface(pContentStart);
    ReleaseInterface(pContentEnd);
    return hr;
}

void CRichTextBlock::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
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
_Check_return_ HRESULT CRichTextBlock::ArrangeCore(XRECTF finalRect)
{
    HRESULT hr = CFrameworkElement::ArrangeCore(finalRect);
    if (m_pSelectionManager != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->UpdateGripperPositions());
    }
    return hr;
}

HRESULT CRichTextBlock::OnPointerCaptureLost(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(__super::OnPointerCaptureLost(pEventArgs));
    if (m_pressedHyperlink)
    {
       IFC_RETURN(m_pressedHyperlink.get()->UpdateForegroundColor(HYPERLINK_NORMAL));
    }

    if (m_currentLink)
    {
       IFC_RETURN(m_currentLink.get()->UpdateForegroundColor(HYPERLINK_NORMAL));
    }

    return S_OK;
}

// Checks is HighContrast Theme is active and HighContrastAdjustment is enabled.
bool CRichTextBlock::IsHighContrastAdjustmentActive() const
{
    return UseHighContrastSelection(GetContext()) && IsHighContrastAdjustmentEnabled();
}

bool CRichTextBlock::IsHighContrastAdjustmentEnabled() const
{
    if (m_highContrastAdjustment == Convert(DirectUI::ElementHighContrastAdjustment::Auto))
    {
        return true;
    }
    else if (m_highContrastAdjustment == Convert(DirectUI::ElementHighContrastAdjustment::Application) &&
             m_applicationHighContrastAdjustment == Convert(DirectUI::ApplicationHighContrastAdjustment::Auto))
    {
        return true;
    }

    return false;
}

// Sets DrawingContext properties to handle foreground color when BackPlate is enabled.
void CRichTextBlock::UpdateBackPlateForegroundOverride()
{
    if (m_pPageNode)
    {
        if (m_pPageNode->GetDrawingContext() != nullptr)
        {
            m_pPageNode->GetDrawingContext()->SetBackPlateConfiguration(IsHighContrastAdjustmentActive(), false /*useHyperlinkForeground*/);
        }
    }
}

bool CRichTextBlock::IsSelectionEnabled() const
{
    return m_pSelectionManager != nullptr && m_isTextSelectionEnabled;
}

_Check_return_ HRESULT CRichTextBlock::RenderHighlighterForegroundCallback(
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
_Check_return_ HRESULT CRichTextBlock::OnPointerMoved(
    _In_ CEventArgs* pEventArgs
)
{
    ITextView *pView = m_pTextView;

    // check whether the pointer is over a link
    if (m_pBlocks != nullptr && m_pTextView != nullptr)
    {
        CPointerEventArgs *pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
        if (!pPointerArgs->m_bHandled)
        {
            CHyperlink *pLink = nullptr;
            // If the event doesn't come from the sender, we shouldn't be doing anything.
            if (pPointerArgs->m_pSource == this)
            {
                IFC_RETURN(CRichTextBlock::HitTestLink(
                    m_pBlocks->GetTextContainer(),
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
                if (m_isTextSelectionEnabled)
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

    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        if (m_pLinkedView != nullptr)
        {
            m_pLinkedView->SetInputContextView(m_pTextView);
            pView = m_pLinkedView;
        }

        IFC_RETURN(m_pSelectionManager->OnPointerMoved(
            this,
            pEventArgs,
            pView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::OnPointerExited(
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

_Check_return_ HRESULT CRichTextBlock::OnPointerPressed(
    _In_ CEventArgs* pEventArgs
)
{
    ITextView *pView = m_pTextView;

    if (m_pBlocks != nullptr && m_pTextView != nullptr)
    {
        bool processSelection = true;
        CPointerEventArgs *pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
        if (!pPointerArgs->m_bHandled)
        {
            // Hyperlink clicks are prioritized above selection.
            if (pPointerArgs->m_pPointer->m_pointerDeviceType != DirectUI::PointerDeviceType::Mouse ||
                pPointerArgs->m_pPointer->m_bLeftButtonPressed)
            {
                CHyperlink *pLink = nullptr;

                // If the event doesn't come from the sender, we shouldn't be doing anything.
                if (pPointerArgs->m_pSource == this)
                {
                    IFC_RETURN(HitTestLink(
                        m_pBlocks->GetTextContainer(),
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
        }

        // As of Win8 selection was processed even if event was marked handled. With the addition of Hyperlink in WinBlue
        // we process Hyperlink only if the event is not marked handled. To keep selection consistent with Win8 shipped behavior,
        // we process selection if a) event was marked handled b) event was not marked handled but no hyperlink navigation needs to
        // take place.
        if (processSelection &&
            IsSelectionEnabled() &&
            m_pTextView != nullptr)
        {
            if (m_pLinkedView != nullptr)
            {
                m_pLinkedView->SetInputContextView(m_pTextView);
                pView = m_pLinkedView;
            }

            IFC_RETURN(m_pSelectionManager->OnPointerPressed(
                this,
                pEventArgs,
                pView));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CRichTextBlock::OnPointerReleased
//
//  Synopsis: Event handler for OnPointerReleased event on TextBlock.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::OnPointerReleased(
    _In_ CEventArgs* pEventArgs
)
{
    if (m_pBlocks != nullptr && m_pTextView != nullptr)
    {
        CPointerEventArgs *pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
        if (!pPointerArgs->m_bHandled)
        {
            CHyperlink *pLink = nullptr;
            // If the event doesn't come from the sender, we shouldn't be doing anything.
            if (pPointerArgs->m_pSource == this)
            {
                IFC_RETURN(CRichTextBlock::HitTestLink(
                    m_pBlocks->GetTextContainer(),
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

    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        ITextView *pView = m_pTextView;

        if (m_pLinkedView != nullptr)
        {
            m_pLinkedView->SetInputContextView(m_pTextView);
            pView = m_pLinkedView;
        }

        IFC_RETURN(m_pSelectionManager->OnPointerReleased(
            this,
            pEventArgs,
            pView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::OnGotFocus(
    _In_ CEventArgs* pEventArgs
)
{
    ITextView *pView = m_pTextView;

    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        if (m_pLinkedView != nullptr)
        {
            m_pLinkedView->SetInputContextView(m_pTextView);
            pView = m_pLinkedView;
        }

        IFC_RETURN(m_pSelectionManager->OnGotFocus(
            this,
            pEventArgs,
            pView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::OnLostFocus(
    _In_ CEventArgs* pEventArgs
)
{
    ITextView *pView = m_pTextView;

    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        if (m_pLinkedView != nullptr)
        {
            m_pLinkedView->SetInputContextView(m_pTextView);
            pView = m_pLinkedView;
        }

        IFC_RETURN(m_pSelectionManager->OnLostFocus(
            this,
            pEventArgs,
            pView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::OnHolding(
    _In_ CEventArgs* pEventArgs
)
{
    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->OnHolding(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::OnTapped(
    _In_ CEventArgs* pEventArgs
)
{
    HRESULT hr = S_OK;
    ITextView *pView = m_pTextView;

    if (m_pBlocks != nullptr && m_pTextView != nullptr)
    {
        CTappedEventArgs *pPointerArgs = static_cast<CTappedEventArgs*>(pEventArgs);
        bool processSelection = true;

        if (!pPointerArgs->m_bHandled)
        {
            CHyperlink *pLink = nullptr;

            // If the event doesn't come from the sender, we shouldn't be doing anything.
            if (pPointerArgs->m_pSource == this)
            {
                // Hittest the content to find out if we are over a Hyperlink element.
                IFC(HitTestLink(
                    m_pBlocks->GetTextContainer(),
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
                processSelection = false;
                IFC(pLink->Navigate());
                pPointerArgs->m_bHandled = TRUE;
            }
        }

        // As of Win8 selection was processed even if event was marked handled. With the addition of Hyperlink in WinBlue
        // we process Hyperlink only if the event is not marked handled. To keep selection consistent with Win8 shipped behavior,
        // we process selection if a) event was marked handled b) event was not marked handled but no hyperlink navigation needs to
        // take place.
        if (IsSelectionEnabled() && processSelection)
        {
            if (m_pLinkedView != nullptr)
            {
                m_pLinkedView->SetInputContextView(m_pTextView);
                pView = m_pLinkedView;
            }

            IFC(m_pSelectionManager->OnTapped(
                this,
                pEventArgs,
                pView));
        }
    }

Cleanup:
    m_pressedHyperlink.reset();
    return hr;
}

_Check_return_ HRESULT CRichTextBlock::OnRightTapped(
    _In_ CEventArgs* pEventArgs
)
{
    ITextView *pView = m_pTextView;

    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        if (m_pLinkedView != nullptr)
        {
            m_pLinkedView->SetInputContextView(m_pTextView);
            pView = m_pLinkedView;
        }

        IFC_RETURN(m_pSelectionManager->OnRightTapped(
            this,
            pEventArgs,
            pView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::OnDoubleTapped(
    _In_ CEventArgs* pEventArgs
)
{
    ITextView *pView = m_pTextView;

    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        if (m_pLinkedView != nullptr)
        {
            m_pLinkedView->SetInputContextView(m_pTextView);
            pView = m_pLinkedView;
        }

        IFC_RETURN(m_pSelectionManager->OnDoubleTapped(
            this,
            pEventArgs,
            pView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::OnKeyUp(
    _In_ CEventArgs* pEventArgs
)
{
    ITextView *pView = m_pTextView;

    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        if (m_pLinkedView != nullptr)
        {
            m_pLinkedView->SetInputContextView(m_pTextView);
            pView = m_pLinkedView;
        }

        IFC_RETURN(m_pSelectionManager->OnKeyUp(
            this,
            pEventArgs,
            pView));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::OnKeyDown(
    _In_ CEventArgs* pEventArgs
)
{
    ITextView *pView = m_pTextView;

    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        if (m_pLinkedView != nullptr)
        {
            m_pLinkedView->SetInputContextView(m_pTextView);
            pView = m_pLinkedView;
        }

        IFC_RETURN(m_pSelectionManager->OnKeyDown(
            this,
            pEventArgs,
            pView));
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
_Check_return_ HRESULT CRichTextBlock::HWRenderSelection(
    _In_ IContentRenderer* pContentRenderer,
    _In_ bool isHighContrast,
    _In_ uint32_t highlightRectCount,
    _In_reads_opt_(highlightRectCount) XRECTF *pHighlightRects,
    _In_ bool isBackPlate
    )
{
    if (m_pSelectionManager != nullptr &&
        m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->HWRender(pContentRenderer, m_pTextView, isHighContrast, highlightRectCount, pHighlightRects, isBackPlate));
    }

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::HWRenderContent(
    _In_ IContentRenderer* pContentRenderer
)
{
    HRESULT hr = S_OK;
    XRECTF *pBackPlateHighlightRects = nullptr;

    if (m_pPageNode != nullptr &&
        !m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        uint32_t cBackPlateHighlightRects = 0;
        ASSERT(m_pPageNode->GetDrawingContext() != nullptr);

        // Page node should be rendered at this time so that glyph runs can be split to show selection
        // foreground per theme colors.
        if (m_pSelectionManager != nullptr &&
            m_pTextView != nullptr)
        {

            // Find the BackPlate selection rectangles.
            if (pBackPlateHighlightRects == nullptr)
            {
                if (IsHighContrastAdjustmentActive())
                {
                    IFC(m_pSelectionManager->GetBackPlateSelectionHighlightRects(
                        m_pTextView,
                        &cBackPlateHighlightRects,
                        &pBackPlateHighlightRects));
                }
            }
        }

        // Render BackPlate.
        if (IsHighContrastAdjustmentActive() &&
            cBackPlateHighlightRects > 0)
        {
            IFC(HWRenderSelection(
                pContentRenderer,
                UseHighContrastSelection(GetContext()), // Is high contrast.
                cBackPlateHighlightRects,
                pBackPlateHighlightRects,
                true));
        }

        auto foregroundRenderingCallback = std::bind(
            &CRichTextBlock::RenderHighlighterForegroundCallback,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3);

        bool redrawForHighlightRegions = false;
        IFC(UpdateSelectionAndHighlightRegions(&redrawForHighlightRegions));

        // Render highlight and selection
        IFC(TextHighlightRenderer::HWRenderCollection(
            GetContext(),
            m_textHighlighters,
            m_highlightRegions,
            m_pTextView,
            pContentRenderer,
            foregroundRenderingCallback));

        if (redrawForHighlightRegions || m_redrawForArrange ||
            (m_textHighlighters != nullptr))
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

    delete[] pBackPlateHighlightRects;

    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders post children content in PC rendering walk.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock::HWPostChildrenRender(
    _In_ IContentRenderer* pContentRenderer
    )
{
    CTextElement *pFocusedElement = GetTextElementForFocusRect();
    if (pFocusedElement)
    {
        IFC_RETURN(HWRenderFocusRects(pContentRenderer, pFocusedElement, m_pTextView));
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
void CRichTextBlock::GetIndependentlyAnimatedBrushes(
    _Outptr_ CSolidColorBrush **ppFillBrush,
    _Outptr_ CSolidColorBrush **ppStrokeBrush
    )
{
    if (m_pTextFormatting->m_pForeground != nullptr &&
        m_pTextFormatting->m_pForeground->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
    {
        SetInterface(*ppFillBrush, static_cast<CSolidColorBrush *>(m_pTextFormatting->m_pForeground));
    }
    if (m_pSelectionManager != nullptr && m_pSelectionManager->GetSelectionBackgroundBrush() != nullptr)
    {
        SetInterface(*ppStrokeBrush, m_pSelectionManager->GetSelectionBackgroundBrush());
    }
}

CFlyoutBase* CRichTextBlock::GetSelectionFlyoutNoRef() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::RichTextBlock_SelectionFlyout, &result));
    return do_pointer_cast<CFlyoutBase>(result.AsObject());
}

_Check_return_ HRESULT CRichTextBlock::FireContextMenuOpeningEventSynchronously(_Out_ bool& handled, const wf::Point& point)
{
    handled = false;

    XPOINTF pointerPosition = { point.X, point.Y };

    xref_ptr<ITransformer> spTransformer;
    IFC_RETURN(TransformToRoot(spTransformer.ReleaseAndGetAddressOf()));
    IFC_RETURN(spTransformer->Transform(&pointerPosition, &pointerPosition, 1 /* count */));

    // Convert to DIPS by adjusting for scale
    const float zoomScale = RootScale::GetRasterizationScaleForElement(this);
    pointerPosition /= zoomScale;

    // We cannot use EventManager here due to reentrancy so raising through the peer directly
    IFC_RETURN(FxCallbacks::RichTextBlock_OnContextMenuOpeningHandler(static_cast<CDependencyObject*>(this), pointerPosition.x, pointerPosition.y, handled));

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::UpdateSelectionFlyoutVisibility()
{
    IFC_RETURN(m_pSelectionManager->UpdateSelectionFlyoutVisibility());
    return S_OK;
}
