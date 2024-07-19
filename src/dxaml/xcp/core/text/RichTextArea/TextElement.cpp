// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <AccessKeyInvokedEventArgs.h>
#include <AccessKeyShownEventArgs.h>
#include <AccessKeyHiddenEventArgs.h>
#include <InputServices.h>

using namespace RichTextServices;

//------------------------------------------------------------------------
//  Summary:
//      Destructor
//------------------------------------------------------------------------
CTextElement::~CTextElement()
{
#if DBG
    InheritedProperties::RecordTextPropertyUsage(this);
#endif

    ReleaseInterface(m_pTextFormatting);
}

//------------------------------------------------------------------------
//  Summary:
//      Returns a TextPosition at passed element edge
//---------------------------------------------------------------------------
#pragma warning (push)
// Description: Disable prefast warning 6387: '*ppTextPointerWrapper' might be '0': this does
//              not adhere to the specification for the function 'CTextElement::GetTextPointer'
//              problem occurs in function 'CTextElement::GetTextPointer'
// Reason     : *ppTextPointerWrapper will only be null if this function fails.
#pragma warning (disable : 6387)
_Check_return_ HRESULT CTextElement::GetTextPointer(
    _In_ ElementEdge edge,
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CRichTextBlock *pRichTextBlock = NULL;
    CTextBlock *pTextBlock = NULL;
    CFrameworkElement *pFrameworkElement = NULL;
    ITextContainer *pTextContainer = NULL;
    CPlainTextPosition plainTextPosition;

    IFCEXPECT_RETURN(ppTextPointerWrapper);
    *ppTextPointerWrapper = NULL;
#pragma warning (pop) // disable : 6387

    pFrameworkElement = GetContainingFrameworkElement();

    if (pFrameworkElement)
    {
        pRichTextBlock = do_pointer_cast<CRichTextBlock>(pFrameworkElement);
        pTextBlock = do_pointer_cast<CTextBlock>(pFrameworkElement);

        if (pRichTextBlock != NULL)
        {
            pTextContainer = pRichTextBlock->GetTextContainer();
        }
        else if (pTextBlock != NULL)
        {
            pTextContainer = pTextBlock->GetTextContainer();
        }
        else
        {
            // If we're outside a TextBlock or CRichTextBlock: fail with NotSupportedException.
            IFC_RETURN(E_NOT_SUPPORTED);
        }

        // CRichTextBlock/TextBlock don't use the backing store, so go down the plain text path.
        IFCEXPECT_ASSERT_RETURN(pTextContainer != NULL);
        IFC_RETURN(GetElementEdgePositionFromTextContainer(edge, pTextContainer, &plainTextPosition));
        IFC_RETURN(CTextPointerWrapper::Create(GetContext(), plainTextPosition, ppTextPointerWrapper));
    }
    else
    {
        // If we're outside a TextBlock or CRichTextBlock: fail with NotSupportedException.
        IFC_RETURN(E_NOT_SUPPORTED);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Get Content Start TextPointer
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextElement::GetContentStart(_Outptr_ CTextPointerWrapper **ppTextPointerWrapper)
{
    RRETURN(GetTextPointer(ContentStart, ppTextPointerWrapper));
}

//------------------------------------------------------------------------
//  Summary:
//      Get Content End TextPointer
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextElement::GetContentEnd(_Outptr_ CTextPointerWrapper **ppTextPointerWrapper)
{
    RRETURN(GetTextPointer(ContentEnd, ppTextPointerWrapper));
}

//------------------------------------------------------------------------
//  Summary:
//      Get Element Start TextPointer
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextElement::GetElementStart(_Outptr_ CTextPointerWrapper **ppTextPointerWrapper)
{
    RRETURN(GetTextPointer(ElementStart, ppTextPointerWrapper));
}

//------------------------------------------------------------------------
//  Summary:
//      Get Element End TextPointer
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextElement::GetElementEnd(_Outptr_ CTextPointerWrapper **ppTextPointerWrapper)
{
    RRETURN(GetTextPointer(ElementEnd, ppTextPointerWrapper));
}

//------------------------------------------------------------------------
//  Summary:
//      SetValue override.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextElement::SetValue(_In_ const SetValueParams& args)
{
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::TextElement_Language:
        {
            // validate the XmlLanguage string
            if (!ValidateXmlLanguage(&args.m_value))
            {
                IFC_RETURN(static_cast<HRESULT>(E_DO_INVALID_CONTENT));
            }
            break;
        }
        case KnownPropertyIndex::TextElement_FontFamily:
        {
            if (args.m_value.IsNull() &&
                (GetContext()->IsSettingValueFromManaged(this) || ParserOwnsParent()))
            {
                HRESULT hrToOriginate = E_NER_ARGUMENT_EXCEPTION;
                xephemeral_string_ptr parameters[2];

                ASSERT(parameters[0].IsNull());
                args.m_pDP->GetName().Demote(&parameters[1]);

                IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_PROPERTY_INVALID, 2, parameters));
                IFC_RETURN(hrToOriginate);
            }
            break;
        }
    }

    IFC_RETURN(CDependencyObject::SetValue(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::TextElement_Language:
        {
            ASSERT(m_pTextFormatting != NULL);
            auto core = GetContext();
            IFC_RETURN(m_pTextFormatting->ResolveLanguageString(core));
            IFC_RETURN(m_pTextFormatting->ResolveLanguageListString(core));
            break;
        }
    }

    IFC_RETURN(MarkDirty(args.m_pDP));

    return S_OK;
}



//------------------------------------------------------------------------
//
//  Synopsis:
//      Pulls all non-locally set inherited properties from the parent.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextElement::PullInheritedTextFormatting()
{
    HRESULT         hr                    = S_OK;
    TextFormatting *pParentTextFormatting = NULL;

    IFCEXPECT_ASSERT(m_pTextFormatting != NULL);
    if (m_pTextFormatting->IsOld())
    {
        // Get the text core properties that we will be inheriting from.
        IFC(GetParentTextFormatting(&pParentTextFormatting));

        // Process each TextElement text core property one by one.

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextElement_FontFamily))
        {
            IFC(m_pTextFormatting->SetFontFamily(this, pParentTextFormatting->m_pFontFamily));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextElement_Foreground)
            && !m_pTextFormatting->m_freezeForeground)
        {
            IFC(m_pTextFormatting->SetForeground(this, pParentTextFormatting->m_pForeground));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextElement_Language))
        {
            m_pTextFormatting->SetLanguageString(pParentTextFormatting->m_strLanguageString);
            m_pTextFormatting->SetResolvedLanguageString(pParentTextFormatting->GetResolvedLanguageStringNoRef());
            m_pTextFormatting->SetResolvedLanguageListString(pParentTextFormatting->GetResolvedLanguageListStringNoRef());
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextElement_FontSize))
        {
            m_pTextFormatting->m_eFontSize = pParentTextFormatting->m_eFontSize;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextElement_FontWeight))
        {
            m_pTextFormatting->m_nFontWeight = pParentTextFormatting->m_nFontWeight;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextElement_FontStyle))
        {
            m_pTextFormatting->m_nFontStyle = pParentTextFormatting->m_nFontStyle;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextElement_FontStretch))
        {
            m_pTextFormatting->m_nFontStretch = pParentTextFormatting->m_nFontStretch;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextElement_CharacterSpacing))
        {
            m_pTextFormatting->m_nCharacterSpacing = pParentTextFormatting->m_nCharacterSpacing;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextElement_TextDecorations))
        {
            m_pTextFormatting->m_nTextDecorations = pParentTextFormatting->m_nTextDecorations;
        }

        // Runs have the FlowDirection property
        if (!OfTypeByIndex<KnownTypeIndex::Run>()
            || IsPropertyDefaultByIndex(KnownPropertyIndex::Run_FlowDirection))
        {
            m_pTextFormatting->m_nFlowDirection = pParentTextFormatting->m_nFlowDirection;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextElement_IsTextScaleFactorEnabled))
        {
            m_pTextFormatting->m_isTextScaleFactorEnabled = pParentTextFormatting->m_isTextScaleFactorEnabled;
        }

        m_pTextFormatting->SetIsUpToDate();
    }

Cleanup:
    ReleaseInterface(pParentTextFormatting);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Mark state as dirty when an inherited property changes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextElement::MarkInheritedPropertyDirty(
    _In_ const CDependencyProperty* pdp,
    _In_ const CValue* pValue)
{
    IFC_RETURN(CDependencyObject::MarkInheritedPropertyDirty(pdp, pValue));

    IFC_RETURN(MarkDirty(pdp));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the element or one of its properties changes. Marks the parent
//      collection, if any, as dirty.
//
//  Notes:
//      Takes ownership of pLocalValueRecord on success only.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT CTextElement::MarkDirty(
    _In_opt_ const CDependencyProperty *pdp
    )
{
    CDependencyObject *pParent = GetParentInternal(false);

    // Propagate the change up if we have a text element collection as parent.
    if (CTextElementCollection *pCollection = do_pointer_cast<CTextElementCollection>(pParent))
    {
        IFC_RETURN(pCollection->MarkDirty(pdp));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      CDependencyObject override -- causes the object and its "children"
//      to enter scope.
//
//      This override is a work around for the default behavior, which
//      sets TextElement collection parents as internal.  We need them to
//      to be public to route events.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextElement::EnterImpl(
    _In_ CDependencyObject *pNamescopeOwner,
         EnterParams        params)
{
    HRESULT hr = S_OK;
    CDependencyObject *pParent = NULL;
    RENDERCHANGEDPFN pfnRenderChangedHandler = NULL;
    bool fPegged = false;

    IFC(CDependencyObject::EnterImpl(pNamescopeOwner, params));

    pParent = GetParentInternal(false /* bPublicOnly */);

    // Ensure that a managed peer cannot be GC'd while we reset the parent.
    if (HasManagedPeer() && ParticipatesInManagedTree())
    {
        IFC(PegManagedPeer());
        fPegged = TRUE;
    }

    // Reset the fPublic bit.  We want it true.
    // NB: We take the somewhat round-about approach of resetting the
    // parent because DO doesn't expose the public bit directly (and
    // DO doesn't want to, there are dependencies that would incur a
    // complexity/fragility cost).

    pfnRenderChangedHandler = NWGetRenderChangedHandlerInternal();

    IFC(RemoveParent(pParent));
    IFC(AddParent(pParent, TRUE /* fPublic */, pfnRenderChangedHandler));

Cleanup:
    if (fPegged)
    {
        UnpegManagedPeer();
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the parent RichTextBox/CRichTextBlock/TextBlock.
//------------------------------------------------------------------------
CFrameworkElement *CTextElement::GetContainingFrameworkElement()
{
    CDependencyObject *pParent = GetParentInternal(false);

    while (pParent != NULL &&
        !pParent->OfTypeByIndex<KnownTypeIndex::RichTextBlock>() &&
        !pParent->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        pParent = pParent->GetParentInternal(false);
    }

    return do_pointer_cast<CFrameworkElement>(pParent);
}

_Check_return_ HRESULT CTextElement::GetElementEdgePositionFromTextContainer(
    _In_ ElementEdge edge,
    _In_ ITextContainer *pTextContainer,
    _Out_ CPlainTextPosition *pPlainTextPosition
    )
{
    CPlainTextPosition plainTextPosition;
    bool found = false;
    XUINT32 offset = 0;
    TextGravity gravity;

    IFC_RETURN(pTextContainer->GetElementEdgeOffset(
        this,
        edge,
        &offset,
        &found));

    // Element must be found within the container otherwise we wouldn't have matched it to the control.
    IFCEXPECT_ASSERT_RETURN(found);

    switch (edge)
    {
    case ContentStart:
    case ElementEnd:
        gravity = LineForwardCharacterBackward;
        break;
    case ContentEnd:
    case ElementStart:
        gravity = LineForwardCharacterForward;
        break;
    default:
        ASSERT(false);
        return E_FAIL;
        break;
    }

    plainTextPosition = CPlainTextPosition(pTextContainer, offset, gravity);
    *pPlainTextPosition = plainTextPosition;

    return S_OK;
}

_Check_return_ HRESULT CTextElement::GetOffsetForEdge(
    _In_ ElementEdge edge,
    _Out_ XUINT32 *pOffset
    )
{
    XUINT32 positionCount;
    XUINT32 offset = 0;

    GetPositionCount(&positionCount);
    XCP_FAULT_ON_FAILURE(positionCount >= 2);

    switch (edge)
    {
    case ContentStart:
        offset = 1;
        break;
    case ElementStart:
        offset = 0;
        break;
    case ContentEnd:
        offset = positionCount - 1;
        break;
    case ElementEnd:
        offset = positionCount;
        break;
    default:
        ASSERT(false);
        return E_FAIL;
        break;
    }

    *pOffset = offset;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   OnCreateAutomationPeerInternal
//
//  Synopsis:  Creates an CAutomationPeer by checking each override
//
//------------------------------------------------------------------------
CAutomationPeer*
CTextElement::OnCreateAutomationPeerInternal()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    CAutomationPeer* pAP = NULL;

    // Start participating because peer could keep a reference to
    // the created automation peer.
    IFC(SetParticipatesInManagedTreeDefault());

    // Ensure the having managed peer for automation peer usage
    IFC(EnsurePeer());

    if (HasManagedPeer())
    {
        IFC(FxCallbacks::TextElement_OnCreateAutomationPeer(this, &pAP));
    }

    AddRefInterface(pAP);

Cleanup:
    return pAP;
}

bool CTextElement::RaiseAccessKeyInvoked()
{
    if (ShouldRaiseEvent(KnownEventIndex::TextElement_AccessKeyInvoked))
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        ASSERT(pEventManager);

        xref_ptr<CAccessKeyInvokedEventArgs> spArgs;
        spArgs.init(new CAccessKeyInvokedEventArgs());

        pEventManager->Raise(
            EventHandle(KnownEventIndex::TextElement_AccessKeyInvoked),
            TRUE   /* bRefire */,
            this    /* pSender */,
            spArgs  /* pArgs */,
            TRUE    /* fRaiseSync */);

        return spArgs->GetHandled();
    }

    return false;
}

void CTextElement::RaiseAccessKeyShown(_In_z_ const wchar_t* strPressedKeys)
{
    GetContext()->GetInputServices()->GetKeyTipManager().ShowAutoKeyTipForElement(this, strPressedKeys);

    if (ShouldRaiseEvent(KnownEventIndex::TextElement_AccessKeyDisplayRequested))
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        ASSERT(pEventManager);

        xref_ptr<CAccessKeyDisplayRequestedEventArgs> spArgs;
        IFCFAILFAST(spArgs.init(new CAccessKeyDisplayRequestedEventArgs(strPressedKeys)));

        pEventManager->Raise(
            EventHandle(KnownEventIndex::TextElement_AccessKeyDisplayRequested),
            TRUE   /* bRefire */,
            this    /* pSender */,
            spArgs  /* pArgs */,
            FALSE    /* fRaiseSync */);
    }
}

void CTextElement::RaiseAccessKeyHidden()
{
    GetContext()->GetInputServices()->GetKeyTipManager().HideAutoKeyTipForElement(this);

    if (ShouldRaiseEvent(KnownEventIndex::TextElement_AccessKeyDisplayDismissed))
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        ASSERT(pEventManager);

        xref_ptr<CAccessKeyDisplayDismissedEventArgs> spArgs;
        IFCFAILFAST(spArgs.init(new CAccessKeyDisplayDismissedEventArgs()));

        pEventManager->Raise(
            EventHandle(KnownEventIndex::TextElement_AccessKeyDisplayDismissed),
            TRUE   /* bRefire */,
            this    /* pSender */,
            spArgs  /* pArgs */,
            FALSE    /* fRaiseSync */);
    }
}

_Check_return_ HRESULT CTextElement::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CDependencyObject::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::TextElement_AccessKey:
        {
            const auto contentRoot = VisualTree::GetContentRootForElement(this);
            const auto& akExport = contentRoot->GetAKExport();
            //Only add the element to the Scope if we are in AK mode
            if (akExport.IsActive())
            {
                IFC_RETURN(akExport.AddElementToAKMode(this));
            }

            break;
        }
    }

    return S_OK;
}

