// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ParserAPI.h>

CContentControl::~CContentControl()
{
    RemoveLogicalChild(m_content.AsObject());
    ReleaseInterface(m_pContentTemplate);
    ReleaseInterface(m_pSelectedContentTemplate);
    ReleaseInterface(m_pContentTransitions);
    ReleaseInterface(m_pListViewBaseItemChrome);
}

CContentControl::CContentControl(_In_ CCoreServices *pCore)
    : CControl(pCore)
    , m_pContentTemplate(nullptr)
    , m_pSelectedContentTemplate(nullptr)
    , m_bInOnApplyTemplate(false)
    , m_bContentIsLogical(true)
    , m_bContentIsTemplateboundManaged(false)
    , m_pContentTransitions(nullptr)
    , m_pListViewBaseItemChrome(nullptr)
{
    m_content.SetNull();
}

//------------------------------------------------------------------------
//
//  Method: SetValue
//
//  Synopsis:
//      Detect changes in key values.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CContentControl::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::ContentControl_Content)
    {
        // CDependencyObject::SetValue will fail if we attempt to set the content property to itself.
        if (args.m_value.GetType() != valueObject || args.m_value.GetType() != m_content.GetType() || args.m_value.AsObject() != m_content.AsObject())
        {
            // Let the base do most of the work.
            IFC_RETURN(CControl::SetValue(args));
        }
    }
    else if (args.m_pDP->GetIndex() == KnownPropertyIndex::ContentControl_ContentTemplate)
    {
        // we don't need to invalidate control if old and new content templates are null.
        if ((args.m_value.GetType() != valueObject && args.m_value.GetType() != valueNull) || args.m_value.AsObject() != m_pContentTemplate)
        {
            xref_ptr<CDataTemplate> oldContentTemplate;

            auto cleanupGuard = wil::scope_exit([&]
            {
                if (oldContentTemplate)
                {
                    oldContentTemplate->UnpegManagedPeer();
                }
            });

            if (m_pContentTemplate)
            {
                // Momentarily pegging the DataTemplate's DXaml peer so it does not get discarded before the CContentPresenter::OnContentTemplateChanged notification completes.
                IFC_RETURN(m_pContentTemplate->PegManagedPeer());
                oldContentTemplate = m_pContentTemplate;
            }

            // Let the base do most of the work.
            IFC_RETURN(CControl::SetValue(args));
            IFC_RETURN(Invalidate(m_pTemplate == NULL));
        }
    }
    else if (args.m_pDP->GetIndex() == KnownPropertyIndex::ContentControl_SelectedContentTemplate)
    {
        // we don't need to invalidate control if old and new content templates are null.
        if ((args.m_value.GetType() != valueObject && args.m_value.GetType() != valueNull) || args.m_value.AsObject() != m_pSelectedContentTemplate)
        {
            // Let the base do most of the work.
            IFC_RETURN(CControl::SetValue(args));
            if (!m_pContentTemplate)
            {
                IFC_RETURN(Invalidate(m_pTemplate == NULL));
            }
        }
    }
    else
    {
        // The template property needs special handling if the content is a UI element
        if (args.m_pDP->GetIndex() == KnownPropertyIndex::Control_Template)
        {
            auto pOldTemplate = GetTemplate();

            // If template has not changed, don't unapply current template, for
            // better perf and to prevent duplicate OnApplyTemplate calls.
            if (args.m_value.AsObject() != pOldTemplate)
            {
                // If we're changing the template and the content is a UI element
                // then we need to ensure that the UI element is not associated with anything
                CUIElement *pUI = do_pointer_cast<CUIElement>(m_content.AsObject());
                if (pUI)
                {
                    CUIElement *pParent = pUI->GetUIElementParentInternal();
                    if (pParent != NULL)
                    {
                        IFC_RETURN(pParent->RemoveChild(pUI));
                    }
                }
            }
        }
        else if (args.m_pDP->GetIndex() == KnownPropertyIndex::ContentControl_ContentTransitions)
        {
            // when this elements receives a transition meant for its child,
            // proactively ensure storage on that child so it starts to
            // take snapshots.
            if (args.m_value.AsObject() || args.m_value.AsIInspectable())
            {
                IFC_RETURN(CUIElement::EnsureLayoutTransitionStorage(m_content.AsObject(), NULL, TRUE));
            }
            // lifetime of transitioncollection is manually handled since it has no parent
            IFC_RETURN(SetPeerReferenceToProperty(args.m_pDP, args.m_value));
        }
        IFC_RETURN(CControl::SetValue(args));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method: EnterImpl
//
//  Synopsis:
//      Causes the object and its "children" to enter scope. If bLive,
//      then the object can now respond to OM requests and perform actions
//      like downloads and animation.
//
//      Derived classes are expected to first call <base>::EnterImpl, and
//      then call Enter on any "children".
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CContentControl::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    HRESULT hr;
    CDependencyObject *pContent = m_content.AsObject();

    // Work on the content
    if (pContent && !params.fSkipNameRegistration)
    {
        EnterParams newParams(params);
        newParams.fIsLive = FALSE;
        IFC(pContent->Enter(pNamescopeOwner, newParams));
    }

    // Call the base class to enter all of this object's properties
    IFC(CControl::EnterImpl(pNamescopeOwner, params));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   LeaveImpl
//
//  Synopsis:
//      Called when a DependencyObject leaves scope.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CContentControl::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    HRESULT hr;
    CDependencyObject *pContent = m_content.AsObject();

// Work on the children
    if (pContent && !params.fSkipNameRegistration)
    {
        LeaveParams newParams(params);
        newParams.fIsLive = FALSE;
        IFC(pContent->Leave(pNamescopeOwner, newParams));
    }

// Call the base class to leave all of this object's properties

    IFC(CControl::LeaveImpl(pNamescopeOwner, params));


Cleanup:
    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Function:   Content()
//
//  Synopsis:   This is the Content property getter and setter method.  This
//              is really no different than a normal property but we can't
//              use the normal property machinery because that would end up
//              adding the children to the live tree twice.  Once under the
//              Content property and once under the visual children.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CContentControl::Content(
    _In_ CDependencyObject* pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue* pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue* pResult)
{
    HRESULT hr;
    CContentControl* pThis;
    CDependencyObject* pOldContent = NULL;
    CDependencyObject* pNewContent = NULL;
    CValue oldForManaged;
    CValue newForManaged;
    EnterParams enterParams;

    IFC(DoPointerCast(pThis, pObject));
    IFC(pThis && cArgs <= 1 ? S_OK : E_INVALIDARG);

    if (cArgs == 0)
    {
        IFCEXPECT(pResult);
        IFC(pResult->CopyConverted(pThis->m_content));
    }
    else
    {
        // The Setter
        CUIElement* pOldContentAsElement = nullptr;
        IFCEXPECT(pArgs);

        // Create a copy of the old and new value to be send up to managed
        IFC(oldForManaged.CopyConverted(pThis->m_content));
        IFC(newForManaged.CopyConverted(*pArgs));

        //update the logical parent for old and new content
        pOldContent = pThis->m_content.AsObject();
        AddRefInterface(pOldContent);   // We need to have our own reference to the old content for later
        if (pOldContent)
        {
            pOldContentAsElement = do_pointer_cast<CUIElement>(pOldContent);
            pThis->RemoveLogicalChild(pOldContent);
        }

        pNewContent = pArgs->AsObject();

        if(pNewContent && pThis->IsContentLogical())
        {
            IFC(pThis->AddLogicalChild(pNewContent, pThis->IsContentTemplateboundManaged()));
        }

        // Determine if we need to invalidate the visual children.  This will create
        //  a new data template and is unnecessary cost if we can reuse it.
        // We can reuse the same template if both old and new content are non-UIElements,
        //  so we invalidate if either the old or the new are UIElement.
        // We also can reuse template if we have ContentControl's template set before.

        // note: we invalidate while m_content is still pointing to oldcontent. This allows unloading to occur correctly.
        IFC(pThis->Invalidate(
            (pThis->m_pTemplate == NULL) &&
            (pOldContentAsElement || (pNewContent && pNewContent->OfTypeByIndex<KnownTypeIndex::UIElement>()))
            ));

        // Release existing memory/reference as applicable.
        pThis->m_content.ReleaseAndReset();

        // Wholly contained CValue: copied directly.  (Color, Enum, Double, Int.)
        // CValue with pointer to memory: make our own copy.  (String, Point, Rect.)
        // CValue with DO: AddRef().
        IFC(pThis->m_content.CopyConverted(*pArgs));

        CDependencyObject *pNamescopeOwner = pThis->GetStandardNameScopeOwner();

        if (pNamescopeOwner)
        {
            if (pOldContent)
            {
                CUIElementCollection* pChildrenCollection = static_cast<CUIElementCollection*>(pThis->GetChildren());

                // Check if old content is indeed transitioning
                bool deferRemovalOfOldContent = pOldContentAsElement && pChildrenCollection
                                              && pChildrenCollection->IsUnloadingElement(pOldContentAsElement);

                if (!deferRemovalOfOldContent)
                {
                    // can do immediate cleanup
                    IFC(pThis->DeferredContentRemovalLogic(pOldContent));
                }
                else
                {
                    // need to defer this
                    UnloadCleanup* pRemoveLogicToExecute = pChildrenCollection->m_pUnloadingStorage->Get(pOldContentAsElement);
                    *pRemoveLogicToExecute = (UnloadCleanup) (*pRemoveLogicToExecute | UC_ContentControlLeave);
                    pThis->AddRef();    // take a ref, which will be released on the unload action of the child
                }
            }

            // Call Enter on the pNewContent.
            // We do want to Enter and avoid calling EnterImpl (because we do that later
            // by calling AddChild) hence, setting fIsLive to FALSE.
            if (pNewContent)
            {
                enterParams.fIsLive = FALSE;
                enterParams.fSkipNameRegistration = pThis->IsTemplateNamescopeMember();
                enterParams.fCoercedIsEnabled = pThis->IsEnabled();
                enterParams.visualTree = VisualTree::GetForElementNoRef(pThis, LookupOptions::NoFallback);
                IFC(pNewContent->Enter(pNamescopeOwner, enterParams));
            }
        }

        // Notify the managed code layer of this change.
        IFC(FxCallbacks::ContentControl_OnContentChanged(pThis, &oldForManaged, &newForManaged, pValueOuter));
    }

Cleanup:
    // No need to release pNewContent because it is the same reference as the
    // one stored in pThis->m_content
    ReleaseInterface(pOldContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   DeferredContentRemovalLogic()
//
//  Synopsis:   When unloading the content of this contentcontrol, the
//              leave walk needs to occur when that element is indeed
//              unloading.
//
//-------------------------------------------------------------------------
 _Check_return_ HRESULT CContentControl::DeferredContentRemovalLogic(_In_ CDependencyObject* pTarget)
{
    LeaveParams leaveParams;

    CDependencyObject *pNamescopeOwner = GetStandardNameScopeOwner();
    leaveParams.fIsLive = FALSE;
    leaveParams.fSkipNameRegistration = IsTemplateNamescopeMember();
    leaveParams.fCoercedIsEnabled = IsEnabled();
    leaveParams.fVisualTreeBeingReset = FALSE;
    IFC_RETURN(pTarget->Leave(pNamescopeOwner, leaveParams));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method: Invalidate
//
//  Synopsis:
//      Remove all the visual children and mark the node as layout dirty.
//      During the next pass all the visual children will be recreated.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CContentControl::Invalidate(XUINT32 bClearChildren)
{
    if (bClearChildren && GetChildren())
    {
        IFC_RETURN(GetChildren()->Clear());
        // only clear the suggested cp if we actually removed all children!
        m_pTemplatePresenter.reset();
    }


    InvalidateMeasure();

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CContentControl::CompareForCircularReference()
//
//  Synopsis:   Called during template instantiation for all the templated
//  parents of an element. Should return TRUE if a cycle is detected.
//
//  Note: see comments on CFrameworkElements implementation.
//-------------------------------------------------------------------------
bool CContentControl::CompareForCircularReference(_In_ CFrameworkElement *pTreeChild)
{
    CContentControl* pTreeChildContentControl = do_pointer_cast<CContentControl>(pTreeChild);

    if (!pTreeChildContentControl)
        return false;   // although we do not check for this in base implementation, we can expect same types

    return CControl::CompareForCircularReference(pTreeChild) &&
       pTreeChildContentControl->m_pContentTemplate == m_pContentTemplate;
}

_Check_return_ HRESULT CContentControl::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    HRESULT hr;

    IFC(CControl::ApplyTemplate(fAddedVisuals));

    if (m_bInOnApplyTemplate)
        goto Cleanup;

    m_bInOnApplyTemplate = TRUE;

    if (!GetFirstChildNoAddRef() && !m_content.IsNull())
    {
        IFC(CreateDefaultVisuals(this, m_content.AsObject(), xstring_ptr::NullString()));
        fAddedVisuals = GetFirstChildNoAddRef() != NULL;
    }

Cleanup:
    m_bInOnApplyTemplate = FALSE;
    RRETURN(hr);
}

DECLARE_CONST_XSTRING_PTR_STORAGE(c_strTextTemplate1Storage,
L"<ControlTemplate "
    L"xmlns=\"http://schemas.microsoft.com/client/2007\" "
    L"TargetType=\"");

DECLARE_CONST_XSTRING_PTR_STORAGE(c_strTextTemplate2Storage, L"\">"
    L"<Grid "
        L"Background=\"{TemplateBinding Background}\">"
        L"<TextBlock "
            L"Text=\"{Binding");

DECLARE_CONST_XSTRING_PTR_STORAGE(c_strTextTemplate3Storage, L"}\" "
            L"HorizontalAlignment=\"Left\" "
            L"VerticalAlignment=\"Top\" "
            L"TextAlignment=\"Left\" "
            L"TextWrapping=\"NoWrap\" "
            L"/>"
    L"</Grid>"
L"</ControlTemplate>");

//-------------------------------------------------------------------------
//
//  Function:   CContentControl::CreateDefaultContent()
//
//  Synopsis:   Create the default visual tree.
//              ContentPresenter uses this function as well.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CContentControl::CreateDefaultVisuals(_In_ CContentControl* pParent, _In_ CDependencyObject* pContent, _In_ const xstring_ptr& strBindingPath)
{
    CUIElement* pUI = NULL;

    IFCEXPECT_RETURN(pParent);

    // If the content is a UIElement then show it.
    pUI = do_pointer_cast<CUIElement>(pContent);
    if (pUI)
    {
        if (pUI->IsAssociated() && !pUI->DoesAllowMultipleAssociation())
        {
            IFC_RETURN(E_INVALIDARG);
        }

        IFC_RETURN(pParent->AddChild(pUI));
    }
    else
    {
        bool fAddedVisuals;

        ReleaseInterface(pParent->m_pTemplate);
        IFC_RETURN(CreateDefaultTemplate(&pParent->m_pTemplate, pParent, strBindingPath));
        IFC_RETURN(pParent->ApplyTemplate(fAddedVisuals));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CContentControl::CreateDefaultTemplate()
//
//  Synopsis:   Create a control template for the default visual tree.
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CContentControl::CreateDefaultTemplate(_Outptr_ CControlTemplate** ppTemplate, _In_ CDependencyObject* pTemplatedParent, _In_ const xstring_ptr& strBindingPath)
{
    HRESULT hr;
    CDependencyObject* pDO = NULL;
    XStringBuilder templateStringBuilder;
    xstring_ptr strName;
    XUINT32 cch = 0;
    Parser::XamlBuffer buffer;
    const xstring_ptr c_strTextTemplate1 = XSTRING_PTR_FROM_STORAGE(c_strTextTemplate1Storage);
    const xstring_ptr c_strTextTemplate2 = XSTRING_PTR_FROM_STORAGE(c_strTextTemplate2Storage);
    const xstring_ptr c_strTextTemplate3 = XSTRING_PTR_FROM_STORAGE(c_strTextTemplate3Storage);

    IFCEXPECT(ppTemplate);
    *ppTemplate = NULL;
    IFCEXPECT(pTemplatedParent);

    strName = pTemplatedParent->GetClassName();
    cch = c_strTextTemplate1.GetCount() + c_strTextTemplate2.GetCount() + c_strTextTemplate3.GetCount() + strName.GetCount();

    // If we are adding the binding string we also add a space.
    cch += strBindingPath.IsNull() ? 0 : strBindingPath.GetCount();

    IFC(templateStringBuilder.Initialize(cch));

    IFC(templateStringBuilder.Append(c_strTextTemplate1));
    IFC(templateStringBuilder.Append(strName));
    IFC(templateStringBuilder.Append(c_strTextTemplate2));

    if (!strBindingPath.IsNull())
    {
        IFC(templateStringBuilder.AppendChar(L' '));
        IFC(templateStringBuilder.Append(strBindingPath));
    }

    IFC(templateStringBuilder.Append(c_strTextTemplate3));

    buffer.m_count = templateStringBuilder.GetCount() * sizeof(WCHAR);
    buffer.m_buffer = reinterpret_cast<const XUINT8*>(templateStringBuilder.GetBuffer());
    buffer.m_bufferType = Parser::XamlBufferType::Text;
    IFC(pTemplatedParent->GetContext()->ParseXaml(
        buffer,
        false, // bForceUtf16
        false, // bCreatePermanentNamescope
        false, // bRequiresDefaultNamespace
        &pDO));
    IFC(DoPointerCast(*ppTemplate, pDO));
    pDO = NULL;

Cleanup:
    ReleaseInterface(pDO);
    RRETURN(hr);
}

// gets called when a contentpresenter in the template of a contentcontrol is used. It will call back
// offering its content to compare to the cc's content. If they are the same, we consider that
// contentpresenter our templateroot.
void CContentControl::ConsiderContentPresenterForContentTemplateRoot(_In_ CContentPresenter* pCandidate, _In_ CValue& value)
{
    if (m_content == value || value.IsNull() && m_content.IsNull())
    {
        m_pTemplatePresenter = xref::get_weakref(pCandidate);
    }
}

xref_ptr<CUIElement> CContentControl::GetContentTemplateRoot() const
{
    xref_ptr<CUIElement> pTemplateRoot;

    auto pTemplatePresenter = m_pTemplatePresenter.lock();
    if (pTemplatePresenter)
    {
        // Using GetTemplateChildNoRef instead of GetFirstChildNoRef because starting with the Cobalt release,
        // when the inner content presenter is a CListViewBaseItemChrome, the template root may be the second
        // child because of the potential backplate positioned in the first slot.
        pTemplateRoot.reset(pTemplatePresenter->GetTemplateChildNoRef());
    }
    return pTemplateRoot;
}

// Sets the Grid/ListViewItem-specific chrome. If we add CSelectorItem, move this and the associated field
// there. This method takes and releases refs as required. Passing in NULL clears the chrome.
_Check_return_ HRESULT
CContentControl::SetGridViewItemChrome(_In_opt_ CListViewBaseItemChrome* pChrome)
{
    if (m_pListViewBaseItemChrome)
    {
        IFC_RETURN(m_pListViewBaseItemChrome->SetChromedListViewBaseItem(NULL));
        ReleaseInterface(m_pListViewBaseItemChrome);
    }

    if (pChrome)
    {
        m_pListViewBaseItemChrome = pChrome;
        AddRefInterface(m_pListViewBaseItemChrome);
    }

    RRETURN(S_OK);
}

// Gets the Grid/ListViewItem-specific chrome.
CListViewBaseItemChrome*
CContentControl::GetGridViewItemChromeNoRef()
{
    return m_pListViewBaseItemChrome;
}

// Fetches the child TextBlock of the default template if we are using the default template; null otherwise.
xref_ptr<CTextBlock> CContentControl::GetTextBlockChildOfDefaultTemplate(_In_ bool fAllowNullContent)
{
    xref_ptr<CTextBlock> textBlock;

    // Make sure we are indeed using the default template (i.e. content is non-null and is not a UIElement).
    if (fAllowNullContent || (!m_content.IsNull() && !do_pointer_cast<CUIElement>(m_content.AsObject())))
    {
        CUIElementCollection* children = this->GetChildren();
        if (children)
        {
            auto cKids = children->GetCount();
            if (cKids >= 1)
            {
                xref_ptr<CUIElement> child;
                child.attach(do_pointer_cast<CUIElement>(children->GetItemDOWithAddRef(0)));
                if (child)
                {
                    // The TextBlock can now be the first child of the ContentControl
                    if (child->GetTypeIndex() == KnownTypeIndex::TextBlock)
                    {
                        textBlock = static_sp_cast<CTextBlock>(std::move(child));
                    }
                    else
                    {
                        // Old template with the Grid
                        children = child->GetChildren();
                        if (children)
                        {
                            cKids = children->GetCount();
                            if (cKids == 1)
                            {
                                child.attach(do_pointer_cast<CUIElement>(children->GetItemDOWithAddRef(0)));
                                if (child && child->GetTypeIndex() == KnownTypeIndex::TextBlock)
                                {
                                    textBlock = static_sp_cast<CTextBlock>(std::move(child));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return textBlock;
}

CTextBlock *CContentControl::GetTextBlockNoRef()
{
    return GetTextBlockChildOfDefaultTemplate(false /* fAllowNullContent */);
}
