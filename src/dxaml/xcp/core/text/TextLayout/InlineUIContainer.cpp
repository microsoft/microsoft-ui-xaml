// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

CInlineUIContainer::CInlineUIContainer(_In_ CCoreServices *pCore) : CInline(pCore)
{
    m_pChild = nullptr;
    m_pCachedHost = nullptr;
    m_isChildAttached = FALSE;
    m_childDesiredWidth = 0;
    m_childDesiredHeight = 0;
    m_childBaseline = 0;
}

CInlineUIContainer::~CInlineUIContainer()
{
    IGNOREHR(EnsureDetachedFromHost());
    RemoveLogicalChild(m_pChild);
    ReleaseInterface(m_pChild);
    m_pCachedHost = nullptr;
}

//-------------------------------------------------------------------------
// Summary:
//     Enters the live tree, and adds the child element (if any) to the embedded
//     element host.
//-------------------------------------------------------------------------
_Check_return_ HRESULT CInlineUIContainer::EnterImpl(
    _In_ CDependencyObject *pNamescopeOwner,
         EnterParams        params)
{
    IFC_RETURN(CInline::EnterImpl(pNamescopeOwner, params));

    if (params.fIsLive && m_pChild)
    {
        IFC_RETURN(EnsureAttachedToHost(m_pCachedHost));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
// Summary:
//     Removes the child element (if any) from the embedded element host, and leaves
//     the tree.
//-------------------------------------------------------------------------
_Check_return_ HRESULT CInlineUIContainer::LeaveImpl(
    _In_ CDependencyObject *pNamescopeOwner,
         LeaveParams        params)
{
    if (params.fIsLive && m_pChild)
    {
        IFC_RETURN(EnsureDetachedFromHost());
        ClearCachedHost();
    }

    IFC_RETURN(CInline::LeaveImpl(pNamescopeOwner, params));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Performs any modifications to the visual tree necessary as side effects when the
//      object is being cleared from the tree. The InlineUIContainer needs to detach its
//      child element from the visual tree at this point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineUIContainer::Shutdown()
{
    IFC_RETURN(EnsureDetachedFromHost());
    ClearCachedHost();

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      SetValue override to update the embedded element host when a new child is set.
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineUIContainer::SetValue(_In_ const SetValueParams& args)
{
    // Child is a content property, so it can be passed in more than one way.
    CValue valueChild;

    // This is a three-step process:
    //      1. Remove the current child from the host.
    //      2. Update the current child.
    //      2. Add the new child, if any, to the host.
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::InlineUIContainer_Child && m_pChild)
    {
        // Clear any managed references we're keeping to the old child.
        // (See below after the CInline::SetValue call for details).
        IFC_RETURN(SetPeerReferenceToProperty(args.m_pDP, CValue::Empty()));

        RemoveLogicalChild(m_pChild);
        IFC_RETURN(EnsureDetachedFromHost());
    }

    IFC_RETURN(CInline::SetValue(args));

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::InlineUIContainer_Child && m_pChild)
    {
        // Initially, InlineUIContainer is the parent of its Child property value, which
        // protects it from being GC'ed. However, once we attach to a host, we reparent
        // the Child to another visual element that might not be in the tree
        // (e.g. before the first measure, the RichTextBoxView isn't attached to anything).
        //
        // To protect against that, we set a peer reference to the property value on the
        // managed side.
        valueChild.WrapObjectNoRef(m_pChild);
        IFC_RETURN(SetPeerReferenceToProperty(args.m_pDP, valueChild));

        IFC_RETURN(EnsureAttachedToHost(m_pCachedHost));
        IFC_RETURN(AddLogicalChild(m_pChild));
    }

    return S_OK;
}

_Check_return_ HRESULT CInlineUIContainer::GetRun(
    _In_                              XUINT32                characterPosition,
    _Out_opt_                   const TextFormatting       **ppTextFormatting,
    _Out_opt_                   const InheritedProperties  **ppInheritedProperties,
    _Out_opt_                         TextNestingType       *pNestingType,
    _Out_opt_                         CTextElement         **ppNestedElement,
    _Outptr_result_buffer_(*pcCharacters) const WCHAR      **ppCharacters,
    _Out_                             XUINT32               *pcCharacters
)
{
    // InlineUIContainer only has 2 positions - Open/Close.
    ASSERT(characterPosition <= 2);
    if (characterPosition >= 2)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *ppCharacters = nullptr;
    *pcCharacters = 1;

    if (ppTextFormatting)
    {
        IFC_RETURN(GetTextFormatting(ppTextFormatting));
    }

    if (ppInheritedProperties)
    {
        IFC_RETURN(GetInheritedProperties(ppInheritedProperties));
    }

    if (ppNestedElement)
    {
        *ppNestedElement = this;
    }

    if (pNestingType)
    {
        *pNestingType = ((characterPosition == 0) ? OpenNesting : CloseNesting);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Returns the child of the InlineUIContainer. Can be NULL. The returned
//      reference is *strong*--you're expected to release it.
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineUIContainer::GetChild(_Outptr_ CUIElement** ppChild)
{
    IFCPTR_RETURN(ppChild);

    *ppChild = m_pChild;
    AddRefInterface(m_pChild);

    return S_OK;
}

_Check_return_ HRESULT CInlineUIContainer::EnsureAttachedToHost(IEmbeddedElementHost *pHost)
{
    if (pHost && m_pChild && !m_isChildAttached)
    {
        IFC_RETURN(pHost->AddElement(this));

        m_isChildAttached = TRUE;
        m_pCachedHost = pHost;
    }

    return S_OK;
}

_Check_return_ HRESULT CInlineUIContainer::EnsureDetachedFromHost()
{
    if (m_pChild && m_isChildAttached)
    {
        IFC_RETURN(m_pCachedHost->RemoveElement(this));
        m_isChildAttached = FALSE;
    }

    return S_OK;
}

void CInlineUIContainer::ClearCachedHost()
{
    m_isChildAttached = FALSE;
    m_pCachedHost = nullptr;
}

// Stores measured width, height, and baseline.
void CInlineUIContainer::SetChildLayoutCache(
    _In_ XFLOAT width,
    _In_ XFLOAT height,
    _In_ XFLOAT baseline
    )
{
    m_childDesiredWidth = width;
    m_childDesiredHeight = height;
    m_childBaseline = baseline;
}

// Retrieves measured width, height, and baseline.
void CInlineUIContainer::GetChildLayoutCache(
    _Out_ XFLOAT *pWidth,
    _Out_ XFLOAT *pHeight,
    _Out_ XFLOAT *pBaseline
    ) const
{
    *pWidth = m_childDesiredWidth;
    *pHeight = m_childDesiredHeight;
    *pBaseline = m_childBaseline;
}

// This is duplicated code from CFrameworkElement::AddLogicalChild as CInlineUIContainer is a
// DependencyObject and cannot use the same logic in CFrameworkElement::AddLogicalChild and
// requires a more restricted form of it.
_Check_return_ HRESULT CInlineUIContainer::AddLogicalChild(_In_ CDependencyObject* pNewLogicalChild)
{
    CFrameworkElement* pChild = nullptr;

    pChild = do_pointer_cast<CFrameworkElement>(pNewLogicalChild);
    if (pChild && (pChild->m_pLogicalParent != this))
    {
        const CDependencyProperty* pdp = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Parent);

        //if logical parent is already set, then content should be template bound. otherwise, this is an error.
        if (pChild->IsPropertyDefault(pdp))
        {
            pChild->m_pLogicalParent = this;

            //set the flags to indicate the property value is now set.
            IFC_RETURN(pChild->SetPropertyIsLocal(pdp));
        }
    }

    return S_OK;
}

// This is duplicated code from CFrameworkElement::RemoveLogicalChild as CInlineUIContainer is a
// DependencyObject and cannot use the same logic in CFrameworkElement::RemoveLogicalChild and
// requires a more restricted form of it.
void CInlineUIContainer::RemoveLogicalChild(_Inout_opt_ CDependencyObject* pOldLogicalChild)
{
    const CDependencyProperty* pdp = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Parent);

    CFrameworkElement* pChild = do_pointer_cast<CFrameworkElement>(pOldLogicalChild);

    if (pChild && !pChild->IsPropertyDefault(pdp) && pChild->GetLogicalParentNoRef() == this)
    {
        //set it back to the default value
        pChild->m_pLogicalParent = nullptr;

        //clear the flags to indicate a value is not set for this property
        pChild->SetPropertyIsDefault(pdp);
    }
}
