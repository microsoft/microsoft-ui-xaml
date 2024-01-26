// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "AutomationPeerAnnotationCollection.h"
#include <FocusMgr.h>

#include "FocusObserver.h"
#include <RuntimeEnabledFeatures.h>

CAutomationPeer::~CAutomationPeer()
{
    Deinit();
}

//  Deinitializes the members as either of managed UIElement is gone
//      or when this object dies itself.
void CAutomationPeer::Deinit()
{
    if (m_pPatternsList)
    {
        CXcpList<IUIAProvider>::XCPListNode *pTemp = m_pPatternsList->GetHead();
        while (pTemp)
        {
            IUIAProvider *pData = static_cast<IUIAProvider*>(pTemp->m_pData);
            ReleaseInterface(pData);
            pTemp = pTemp->m_pNext;
        }
        m_pPatternsList->Clean(FALSE);
        delete m_pPatternsList;
        m_pPatternsList = nullptr;
    }

    if (m_pUIAWrapper)
    {
        m_pUIAWrapper->Invalidate();
        m_pUIAWrapper = nullptr;
    }

    auto core = GetContext();

    if (core)
    {
        // During this Deinit call, the tree may have been partly taken apart already and the m_pDO pointer may not be
        // valid. Iterate through all the FocusManagers on the thread to make sure none of them are referencing this
        // AutomationPeer.
        for (const xref_ptr<CContentRoot>& contentRoot : core->GetContentRootCoordinator()->GetContentRoots())
        {
            if (CFocusManager* focusManager = contentRoot->GetFocusManagerNoRef())
            {
                xref_ptr<CAutomationPeer> ap { focusManager->GetFocusedAutomationPeer() };

                if (this == ap.get())
                {
                    focusManager->SetFocusedAutomationPeer(nullptr);
                    break;
                }
            }
        }
    }

    ReleaseInterface(m_pAPParent);

    strCurrentName.Reset();
    strCurrentItemStatus.Reset();

    m_pDO = nullptr;
}

//------------------------------------------------------------------------
//
//  Method:   Initialize
//
//------------------------------------------------------------------------
HRESULT CAutomationPeer::InitInstance()
{
    m_pPatternsList = new CXcpList<IUIAProvider>();

    return S_OK;//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   GetUIAWrapper
//
//  Synopsis: Returns the wrapper object, it is used validate an AutomationPeer
//            within a given wrapper
//
//
//------------------------------------------------------------------------

IUIAWrapper* CAutomationPeer::GetUIAWrapper()
{
    return m_pUIAWrapper;
}

//------------------------------------------------------------------------
//
//  Method:   SetUIAWrapper
//
//  Synopsis: When CUIAWrapper gets created for a given AutomationPeer, it caches
//            itself here for reuse. It's associated as weak ref and uses validation
//            pattern back and forth to make sure the pointer is live.
//
//
//------------------------------------------------------------------------

void CAutomationPeer::SetUIAWrapper(IUIAWrapper* pUIAWrapper)
{
    ASSERT(m_pUIAWrapper == nullptr);
    ASSERT(pUIAWrapper);
    m_pUIAWrapper = pUIAWrapper;
}

//------------------------------------------------------------------------
//
//  Method:   InvalidateUIAWrapper
//
//  Synopsis: When CUIAWrapper is destructed it lets AutomationPeer know about that to clean appropriately.
//
//
//------------------------------------------------------------------------

void CAutomationPeer::InvalidateUIAWrapper()
{
    m_pUIAWrapper = nullptr;

    // Notify managed AP that no UIA client object is referred in order to allow manage AP to let it's owners clean itself.
    // It can happen that the destructor will be called after our CCoreServices has been destroyed. This occurs when external
    // clients are holding a reference to AutomationPeer objects. Hence, the extra check for GetIsCoreServicesReady().
    if (CCoreServices::GetIsCoreServicesReady())
    {
        IGNOREHR(FxCallbacks::AutomationPeer_NotifyNoUIAClientObjectForAP(this));
    }
}

//------------------------------------------------------------------------
//
//  Method:   InvalidateOwner
//
//  Synopsis: When DO is destructed it lets AutomationPeer know to clean itself up.
//
//
//------------------------------------------------------------------------

void CAutomationPeer::InvalidateOwner()
{
    m_pDO = nullptr;
}

//------------------------------------------------------------------------
//
//  Method:   NotifyManagedUIElementIsDead
//
//  Synopsis: Calls Deinitializes managed UIElement is gone.
//
//
//------------------------------------------------------------------------
void CAutomationPeer::NotifyManagedUIElementIsDead()
{
    Deinit();
}

//------------------------------------------------------------------------
//
//  Method:   GetAPEventsSource
//
//  Synopsis:
//      Returns the EventsSource corresponding to current AutomationPeer, EventsSource is the real peer which goes to client and
//      should be considered source of events for raised on this peer.
//
//------------------------------------------------------------------------
CAutomationPeer* CAutomationPeer::GetAPEventsSource()
{
    HRESULT hr = S_OK;
    UIAXcp::APAutomationControlType apControlType;

    IFC(GetAutomationControlType(&apControlType));
    if(apControlType == UIAXcp::ACTListItem || apControlType == UIAXcp::ACTTabItem || apControlType == UIAXcp::ACTTreeItem)
    {
        // Ignore if we don't have a managed peer.
        CorePeggedPtr<CAutomationPeer> autoPeg;
        autoPeg.TrySet( this );
        if( autoPeg )
        {
            ASSERT( HasManagedPeer() );

            xref_ptr<IUnknown> spUnkParentNativeNode;
            xref_ptr<CAutomationPeer> spAPParent;
            IFC(Navigate(UIAXcp::AutomationNavigationDirection_Parent, spAPParent.ReleaseAndGetAddressOf(), spUnkParentNativeNode.ReleaseAndGetAddressOf()));

            if (spAPParent)
            {
                IFC(FxCallbacks::AutomationPeer_GenerateAutomationPeerEventsSource(this, static_cast<CAutomationPeer*>(spAPParent.get())));
                if (m_pAPEventsSource)
                {
                    // This code path exists for one reason: for ListViewItems we need to make sure that
                    // their DataAutomationPeers, which are the EventsSource* for the actual FrameworkElement
                    // derived AutomationPeers, return the correct parent. In List/ListItem peer implementations
                    // we hide all the controls between the ListViewItem and the ListView control. Instead of having
                    // something that looks like this:
                    // ListViewItem(ListItem)AP -> ScrollViewer(Pane)AP -> ListView(List)AP
                    // you see something like this:
                    // ListViewItem(ListItem)AP -> ListView(List)AP
                    //
                    // This is accomplished in two ways:
                    // - For internal framework code there exists an override in the DXAML FrameworkElement
                    //   implementation: GetLogicalParentForAPProtected. We don't expose this publically. It
                    //   is called from GetAPTemplatedParentFirst and dumped into pAPParent above.
                    // - For external code implementing the List/ListView AP pattern (have mercy) there's an
                    //   API on AutomationPeer called SetParent. This API sets m_pAPParent.
                    //
                    // We make sure here that if m_pAPParent is already set, we leave it alone and don't override it.
                    // That indicates external code is trying to set the parent on a data automation peer.
                    // If not we perform this call, which does the same thing, but for internal automation peers.
                    //
                    // * EventsSource is a little bit of a misnomer. If an automation peer has an EventsSource set
                    //   essentially all UIAWrapper invocations will replace a pointer to that automation peer with
                    //   a pointer to the EventsSource AP when returning pointers to client code.
                    if (!m_pAPEventsSource->HasParent())
                    {
                        m_pAPEventsSource->SetAPParent(spAPParent.get());
                    }
                }
            }
        }
    }

Cleanup:
    if ( hr != S_OK )
    {
        return nullptr;
    }
    return m_pAPEventsSource;
}

//------------------------------------------------------------------------
//
//  Method:   GetAPParent
//
//  Synopsis:
//      1. APParent is null for the first layer of the APs directly connected with XAML UIA root (CUIAWindow)
//      2. APParent also gets set when GetChildren is called, the returned child object then set their parent
//         as the caller AP
//
//------------------------------------------------------------------------

CAutomationPeer* CAutomationPeer::GetAPParent()
{
    return m_pAPParent;
}

//------------------------------------------------------------------------
//
//  Method:   GetAPParent
//
//  Synopsis:
//      Returns the logical automation peer parent.  If we don't currently have an m_pAPParent set, then
//      we will attempt to walk the logical tree to get the parent.
//
//------------------------------------------------------------------------
CAutomationPeer* CAutomationPeer::GetLogicalAPParent()
{
    if (!m_pDO)
    {
        return nullptr;
    }

    if (m_pAPParent)
    {
        return m_pAPParent;
    }

    CAutomationPeer* pAP = nullptr;
    CDependencyObject *pParent = m_pDO;

    // Keep finding the logical AP parent
    while (pAP == nullptr)
    {
        // if parent is Popup associated PopupRoot is returned by GetParent which needs special handling to find the logical parent.
        // GetPopupAssociatedAutomationPeer does that special handling of cases wherein parent is popup.
        pAP = static_cast<CUIElement*>(pParent)->GetPopupAssociatedAutomationPeer();
        if (!pAP)
        {
            CDependencyObject *pLogicalParent = nullptr;
            IGNOREHR(FxCallbacks::FrameworkElement_GetLogicalParentForAP(static_cast<CDependencyObject *>(pParent), &pLogicalParent));
            if (pLogicalParent)
            {
                pParent = pLogicalParent;
            }
            else
            {
                pParent = pParent->GetParentInternal(false /* public parent only */);
                if (pParent && pParent->OfTypeByIndex<KnownTypeIndex::XamlIslandRootCollection>())
                {
                    // XamlIslandRootCollection is invisible to automation -- the parent walk stops at the XamlIslandRoot.
                    pParent = nullptr;
                }
            }

            // Framework callback added a ref, so we need to release before walking to the next parent
            ReleaseInterface(pLogicalParent);

            if (pParent)
            {
                pAP = pParent->OnCreateAutomationPeer();
            }
            else
            {
                break;
            }
        }
    }

    return pAP;
}

// In current design CUIAWrapper deals with weak ref of CAPs, this was done to make sure XAML tree is not leaking when
// UIACore keeps provider nodes alive for far too long. As those issues have been fixed this design should be reconsidered.
HRESULT CAutomationPeer::Navigate(
     _In_ UIAXcp::AutomationNavigationDirection direction,
     _Outptr_result_maybenull_ CAutomationPeer** ppReturnAP,
     _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk)
{
    CorePeggedPtr<CAutomationPeer> autoPeg;

    *ppReturnAP = nullptr;
    *ppReturnIREPFAsUnk = nullptr;

    // Ignore if we don't have a managed peer. ensuring from getting this object neutered.
    autoPeg.TrySet(this);
    if (autoPeg)
    {
        xref_ptr<CDependencyObject> spResultAPAsDO;
        xref_ptr<IUnknown> spUnkNativeNode;

        ASSERT(HasManagedPeer());

        IFC_RETURN(FxCallbacks::AutomationPeer_Navigate(this, direction, spResultAPAsDO.ReleaseAndGetAddressOf(), spUnkNativeNode.ReleaseAndGetAddressOf()));

        // we are using two types of return values for APs or IREPS to make sure the whole contract is strongly typed instead of using void**,
        // that means the target is either AP or IREPS it can't be both.
        ASSERT(spResultAPAsDO == nullptr || spUnkNativeNode == nullptr);

        // We are looking for siblings and if they come out to be null there are 2 cases possible:
        //      1. Sibling is actually Null
        //      2. When this node is directly connected to CUIAWindow, Navigate to Parent will be null.
        //         In this case to find the sibling we need to get children of the root and then do our search.
        if ((spResultAPAsDO == nullptr && spUnkNativeNode == nullptr) && (direction == UIAXcp::AutomationNavigationDirection_NextSibling || direction == UIAXcp::AutomationNavigationDirection_PreviousSibling))
        {
            CAutomationPeer **pChildren = nullptr;
            xref_ptr<CAutomationPeer> spChild;
            XINT32 index = 0;
            XINT32 cChildren = 0;

            xref_ptr<CDependencyObject> spParentAPAsDO;
            xref_ptr<IUnknown> spUnkParentNativeNode;

            IFC_RETURN(FxCallbacks::AutomationPeer_Navigate(this, UIAXcp::AutomationNavigationDirection_Parent, spParentAPAsDO.ReleaseAndGetAddressOf(), spUnkParentNativeNode.ReleaseAndGetAddressOf()));

            // This is a second case where we look for accessible children of the root window and search that for current node.
            if (spParentAPAsDO == nullptr && spUnkParentNativeNode == nullptr)
            {
                cChildren = GetRootChildren(&pChildren);

                if (cChildren != 0 && pChildren != nullptr)
                {
                    // In Core layer we match runtime ids as they are unique in this context to find the
                    // current element among the children. In this case previous sibling is index - 1 and
                    // next sibling is index + 1.
                    while (index < cChildren)
                    {
                        if (pChildren[index]->GetRuntimeId() == GetRuntimeId())
                        {
                            break;
                        }
                        index++;
                    }

                    if (direction == UIAXcp::AutomationNavigationDirection_NextSibling && (index < cChildren - 1))
                    {
                        spChild = static_cast<CAutomationPeer*>(pChildren[index + 1]);
                    }
                    else if (direction == UIAXcp::AutomationNavigationDirection_PreviousSibling && (index > 0) && (index < cChildren))
                    {
                        spChild = static_cast<CAutomationPeer*>(pChildren[index - 1]);
                    }
                }

                *ppReturnAP = spChild.detach();

                // This is not a leak here as there are no IFCs.
                delete [] pChildren;
            }
        }
        else
        {
            // Navigate deals with both AutomationPeer as well as native UIA nodes.
            // We use two return values for each of them for making sure that contract
            // between layers is strongly typed.
            *ppReturnAP = do_pointer_cast<CAutomationPeer>(spResultAPAsDO.detach());

            *ppReturnIREPFAsUnk = spUnkNativeNode.detach();
        }
    }
    else
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetChildren
//
//  Synopsis:
//      Returns the count and an array containing the Children of this AP
//      Calls out to managed code to ask
//
//------------------------------------------------------------------------
XINT32 CAutomationPeer::GetChildren(CAutomationPeer*** pppReturnAP)
{
    HRESULT hr = S_OK;
    XINT32 iReturnCount = 0;
    XINT32 iObjectsCount = 0;
    CDependencyObject** pReturnDO = nullptr;
    CAutomationPeer** pReturnAP = nullptr;
    CorePeggedPtr<CAutomationPeer> autoPeg;

    // Ignore if we don't have a managed peer.
    autoPeg.TrySet( this );
    if ( autoPeg )
    {
        ASSERT( HasManagedPeer() );

        // method#0 will return Children count only.
        IFC(FxCallbacks::AutomationPeer_GetAutomationPeerChildren(this, GET_AUTOMATION_PEER_CHILDREN_COUNT, &iReturnCount, &pReturnDO));
        if (iReturnCount < 0)
        {
            // managed side return -1;
            IFC(E_FAIL);
        }
        else if (iReturnCount == 0)
        {
            // no children in the AP children collection.
            goto Cleanup;
        }

        pReturnDO = new CDependencyObject*[iReturnCount];

        // Assigned the allocated AP array count to compare it with
        // the children count that will fill into the allocated AP array.
        iObjectsCount = iReturnCount;

        // method#1 should return the same amount of objects as method#0
        // it returns the real objects array instead of just count.
        IFC(FxCallbacks::AutomationPeer_GetAutomationPeerChildren(this, GET_AUTOMATION_PEER_CHILDREN_CHILDREN, &iObjectsCount, &pReturnDO));
        IFCEXPECT_ASSERT(iObjectsCount == iReturnCount);
        IFCPTR(pReturnDO);

        pReturnAP = (CAutomationPeer**)(pReturnDO);
        for (XINT32 i = 0; i < iReturnCount; i++)
        {
            // cast all pointers to CAutomationPeer from IDO
            pReturnAP[i] = (CAutomationPeer*)(do_pointer_cast<CAutomationPeer>(pReturnDO[i]));
        }

        // We don't need to set the parents for this set of children because they were set in the DXAML layer.
        // see AutomationPeer::GetChildrenImpl

        *pppReturnAP = pReturnAP;
    }
    else
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

Cleanup:
    if (FAILED(hr))
    {
        iReturnCount = -1;
        delete [] pReturnDO;
    }
    return iReturnCount;
}

//------------------------------------------------------------------------
//
//  Method:   GetRootChildren
//
//  Synopsis:
//      Returns the count and an array containing the Children of the root
//
//------------------------------------------------------------------------
XINT32 CAutomationPeer::GetRootChildren(CAutomationPeer*** pppReturnAP)
{
    return GetRootChildrenCore(pppReturnAP);
}

//------------------------------------------------------------------------
//
//  Method:   GetPattern
//
//  Synopsis:
//      Queries to see if a certain pattern is supported
//      Calls out to managed code to ask
//
//------------------------------------------------------------------------
IUIAProvider* CAutomationPeer::GetPattern(_In_ UIAXcp::APPatternInterface ePattern)
{
    HRESULT hr = S_OK;
    CDependencyObject* pInteropObject = nullptr;
    IUIAProvider *pProvider = nullptr;
    bool isMOR = false;
    bool isCachedPattern = false;
    CorePeggedPtr<CAutomationPeer> autoPeg;

    // Ignore if we don't have a managed peer.
    autoPeg.TrySet( this );
    if( autoPeg )
    {
        ASSERT( HasManagedPeer() );
        IFC(FxCallbacks::AutomationPeer_GetPattern(this, &pInteropObject, ePattern));
    }
    else
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    if ( pInteropObject )
    {
        isMOR = static_cast<CDependencyObject*>(pInteropObject)->GetTypeIndex() == DependencyObjectTraits<CManagedObjectReference>::Index;

        // If it's MOR then there's no need to find cached pattern as it will always be a new object returned hence won't be in cache.
        if( !isMOR )
        {
            isCachedPattern = FindCachedPattern(ePattern, pInteropObject, &pProvider);
        }

        if( !pProvider )
        {
            switch (ePattern)
            {
                case UIAXcp::PIInvoke:
                {
                   pProvider = new CManagedInvokeProvider(this);
                   break;
                }
                case UIAXcp::PIDock:
                {
                    pProvider = new CManagedDockProvider(this);
                    break;
                }
                case UIAXcp::PIExpandCollapse:
                {
                    pProvider = new CManagedExpandCollapseProvider(this);
                    break;
                }
                case UIAXcp::PIGridItem:
                {
                    pProvider = new CManagedGridItemProvider(this);
                    break;
                }
                case UIAXcp::PIGrid:
                {
                    pProvider = new CManagedGridProvider(this);
                    break;
                }
                case UIAXcp::PIMultipleView:
                {
                    pProvider = new CManagedMultipleViewProvider(this);
                    break;
                }
                case UIAXcp::PIRangeValue:
                {
                    pProvider = new CManagedRangeValueProvider(this);
                    break;
                }
                case UIAXcp::PIScrollItem:
                {
                    pProvider = new CManagedScrollItemProvider(this);
                    break;
                }
                case UIAXcp::PIScroll:
                {
                    pProvider = new CManagedScrollProvider(this);
                    break;
                }
                case UIAXcp::PISelectionItem:
                {
                    pProvider = new CManagedSelectionItemProvider(this);
                    break;
                }
                case UIAXcp::PISelection:
                {
                    pProvider = new CManagedSelectionProvider(this);
                    break;
                }
                case UIAXcp::PITableItem:
                {
                    pProvider = new CManagedTableItemProvider(this);
                    break;
                }
                case UIAXcp::PITable:
                {
                    pProvider = new CManagedTableProvider(this);
                    break;
                }
                case UIAXcp::PIToggle:
                {
                    pProvider = new CManagedToggleProvider(this);
                    break;
                }
                case UIAXcp::PITransform:
                case UIAXcp::PITransform2:
                {
                    pProvider = new CManagedTransformProvider(this);
                    break;
                }
                case UIAXcp::PIValue:
                {
                    pProvider = new CManagedValueProvider(this);
                    break;
                }
                case UIAXcp::PIWindow:
                {
                    pProvider = new CManagedWindowProvider(this);
                    break;
                }
                case UIAXcp::PIText:
                case UIAXcp::PIText2:
                {
                    pProvider = new CManagedTextProvider(this);
                    break;
                }
                case UIAXcp::PIItemContainer:
                {
                    pProvider = new CManagedItemContainerProvider(this);
                    break;
                }
                case UIAXcp::PIVirtualizedItem:
                {
                    pProvider = new CManagedVirtualizedItemProvider(this);
                    break;
                }
                case UIAXcp::PITextChild:
                {
                    pProvider = new CManagedTextChildProvider(this);
                    break;
                }
                case UIAXcp::PIAnnotation:
                {
                    pProvider = new CManagedAnnotationProvider(this);
                    break;
                }
                case UIAXcp::PIDrag:
                {
                    pProvider = new CManagedDragProvider(this);
                    break;
                }
                case UIAXcp::PIDropTarget:
                {
                    pProvider = new CManagedDropTargetProvider(this);
                    break;
                }
                case UIAXcp::PIObjectModel:
                {
                    pProvider = new CManagedObjectModelProvider(this);
                    break;
                }
                case UIAXcp::PISpreadsheet:
                {
                    pProvider = new CManagedSpreadsheetProvider(this);
                    break;
                }
                case UIAXcp::PISpreadsheetItem:
                {
                    pProvider = new CManagedSpreadsheetItemProvider(this);
                    break;
                }
                case UIAXcp::PIStyles:
                {
                    pProvider = new CManagedStylesProvider(this);
                    break;
                }
                case UIAXcp::PISynchronizedInput:
                {
                    pProvider = new CManagedSynchronizedInputProvider(this);
                    break;
                }
                case UIAXcp::PITextEdit:
                {
                    pProvider = new CManagedTextEditProvider(this);
                    break;
                }
                case UIAXcp::PICustomNavigation:
                {
                    pProvider = new CManagedCustomNavigationProvider(this);
                    break;
                }
                default:
                {
                    IFC(E_FAIL);
                }
            };
            ((CUIAPatternProvider*)(pProvider))->m_pInteropObject = pInteropObject;

            // If pInteropObject is basically this AutomationPeer itself(which is the case mostly)
            // by addrefing for within Pattern we will create a circular dependency which we must avoid to protect the leak.
            // "below check" adds reference only if it's indepdent object which implements the pattern.
            if(static_cast<CAutomationPeer*>(pInteropObject) == this)
            {
                ((CUIAPatternProvider*)(pProvider))->m_bInteropSameAsPeer = TRUE;
            }
            else
            {
                AddRefInterface(pInteropObject);
            }
        }
    }

    // Add the core object corresponding to the pattern to pattern list, if it's not already in there.
    if ( pProvider && m_pPatternsList && !isCachedPattern )
    {
        m_pPatternsList->Add(pProvider);
    }

Cleanup:
    if ( hr != S_OK )
    {
        return nullptr;
    }
    return pProvider;
}

//------------------------------------------------------------------------
//
//  Method:   FindCachedPattern
//
//  Synopsis: Checks if the patternobject already exist in there for a particular pattern type, if yes then returns it.
//
//------------------------------------------------------------------------
bool CAutomationPeer::FindCachedPattern(_In_ UIAXcp::APPatternInterface patternType,
        _In_ CDependencyObject* pPatternObject,
        _Outptr_ IUIAProvider** ppPatternProvider)
{
    HRESULT hr = S_OK;
    bool found = false;

    IFCPTR(ppPatternProvider);
    *ppPatternProvider = nullptr;

    if ( m_pPatternsList )
    {
        CXcpList<IUIAProvider>::XCPListNode *pTemp = m_pPatternsList->GetHead();
        while ( pTemp )
        {
            IUIAProvider *pData = static_cast<IUIAProvider*>(pTemp->m_pData);
            if( pData && pData->GetPatternType() == patternType && ((CUIAPatternProvider*)(pData))->m_pInteropObject == pPatternObject )
            {
                *ppPatternProvider = pData;
                found = TRUE;
                break;
            }
            pTemp = pTemp->m_pNext;
        }
    }

Cleanup:
    if( hr != S_OK )
    {
        return false;
    }

    return found;
}

//------------------------------------------------------------------------
//
//  Method:   GetTextRangePattern
//
//  Synopsis:
//
//------------------------------------------------------------------------
IUIATextRangeProvider* CAutomationPeer::GetTextRangePattern(_In_ CDependencyObject *pInteropObject)
{
    CManagedTextRangeProvider *pTextRangeProvider = nullptr;
    IUIAProvider *pProvider = nullptr;
    bool isMOR = false;
    bool isCachedPattern = false;

    if (pInteropObject)
    {
        isMOR = static_cast<CDependencyObject*>(pInteropObject)->GetTypeIndex() == DependencyObjectTraits<CManagedObjectReference>::Index;

        // If it's MOR then there's no need to find cached pattern as it will always be a new object returned hence won't be in cache.
        if( !isMOR )
        {
            isCachedPattern = FindCachedPattern(UIAXcp::PITextRange, pInteropObject, &pProvider);
            pTextRangeProvider = static_cast<CManagedTextRangeProvider*>(pProvider);
        }

        if(!pTextRangeProvider)
        {
            pTextRangeProvider = new CManagedTextRangeProvider(this);
            pTextRangeProvider->m_pInteropObject = static_cast<CDependencyObject*>(pInteropObject);

            // If pInteropObject is basically this AutomationPeer itself(which is the case mostly)
            // by addrefing for within Pattern we will create a circular dependency which we must avoid to protect the leak.
            // "below check" adds reference only if it's indepdent object which implements the pattern.
            if(static_cast<CAutomationPeer*>(pInteropObject) == this)
            {
                ((CUIAPatternProvider*)(pProvider))->m_bInteropSameAsPeer = TRUE;
            }
            else
            {
                AddRefInterface(pInteropObject);
            }
        }
    }

    if (pTextRangeProvider && m_pPatternsList && !isCachedPattern)
    {
        m_pPatternsList->Add(pTextRangeProvider);
    }

    return pTextRangeProvider;
}

//------------------------------------------------------------------------
//
//  Method:   GetAcceleratorKey
//
//  Synopsis:
//      Returns Accelerator Key
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetAcceleratorKey(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APAcceleratorKeyProperty, pstrRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetAccessKey
//
//  Synopsis:
//      Returns Access Key
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetAccessKey(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APAccessKeyProperty, pstrRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetAutomationControlType
//
//  Synopsis:
//      Returns Type of Control
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetAutomationControlType(_Out_ UIAXcp::APAutomationControlType *pRetVal)
{
    XINT32 nValue = 0;

    IFC_RETURN(GetAutomationPeerIntValueFromManaged(UIAXcp::APAutomationControlTypeProperty, &nValue));
    *pRetVal = static_cast<UIAXcp::APAutomationControlType>(nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetAutomationId
//
//  Synopsis:
//      Returns Access Key
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetAutomationId(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APAutomationIdProperty, pstrRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetBoundingRectangle
//
//  Synopsis:
//      Returns Bounding Rectangle
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetBoundingRectangle(_Out_ XRECTF* pRetVal)
{
    return GetAutomationPeerRectValueFromManaged(UIAXcp::APBoundingRectangleProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetClassName
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetClassName(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APClassNameProperty, pstrRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetClickablePoint
//
//  Synopsis:
//      Returns Access Key
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetClickablePoint(_Out_ XPOINTF* pRetVal)
{
    return GetAutomationPeerPointValueFromManaged(UIAXcp::APClickablePointProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetHelpText
//
//  Synopsis:
//      Returns help text
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetHelpText(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APHelpTextProperty, pstrRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetItemStatus
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetItemStatus(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APItemStatusProperty, pstrRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetItemType
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetItemType(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APItemTypeProperty, pstrRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetLabeledBy
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetLabeledBy(_Outptr_ CAutomationPeer** pRetVal)
{
    return GetAutomationPeerAPValueFromManaged(UIAXcp::APLabeledByProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetLocalizedControlType
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetLocalizedControlType(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APLocalizedControlTypeProperty, pstrRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetName
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetName(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APNameProperty, pstrRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   GetOrientation
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetOrientation(_Out_ UIAXcp::OrientationType* pRetVal)
{
    XINT32 nValue = 0;

    IFC_RETURN(GetAutomationPeerIntValueFromManaged(UIAXcp::APOrientationProperty, &nValue));
    *pRetVal = static_cast<UIAXcp::OrientationType>(nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetLiveSetting
//
//  Synopsis:
//      Return Live Setting Property. The property corresponds to LiveRegionChanged event
//      which is raised when you want unfocusable elements to be read by Narrator.
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetLiveSetting(_Out_ UIAXcp::LiveSetting* pRetVal)
{
    XINT32 nValue = 0;

    IFC_RETURN(GetAutomationPeerIntValueFromManaged(UIAXcp::APLiveSettingProperty, &nValue));
    *pRetVal = static_cast<UIAXcp::LiveSetting>(nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetControlledPeers
//
//  Synopsis:
//      Return ControlledPeers data.
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::GetControlledPeers(_Outptr_ CAutomationPeerCollection** ppRetVal)
{
    xref_ptr<CDependencyObject> spDO = nullptr;

    *ppRetVal = nullptr;

    IFC_RETURN(GetAutomationPeerDOValueFromManaged(UIAXcp::APControlledPeersProperty, spDO.ReleaseAndGetAddressOf()));
    *ppRetVal = do_pointer_cast<CAutomationPeerCollection>(spDO.detach());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   HasKeyboardFocus
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::HasKeyboardFocus(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APHasKeyboardFocusProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   IsContentElement
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::IsContentElement(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APIsContentElementProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   IsControlElement
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::IsControlElement(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APIsControlElementProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   IsEnabled
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::IsEnabled(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APIsEnabledProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   IsKeyboardFocusable
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::IsKeyboardFocusable(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APIsKeyboardFocusableProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   IsOffscreen
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::IsOffscreen(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APIsOffscreenProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   IsPassword
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::IsPassword(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APIsPasswordProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   IsRequiredForForm
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::IsRequiredForForm(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APIsRequiredForFormProperty, pRetVal);
}

//------------------------------------------------------------------------
//
//  Method:   SetFocus
//
//  Synopsis:
//      Sets the Focus to current Element
//
//------------------------------------------------------------------------
void CAutomationPeer::SetFocus()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    CorePeggedPtr<CAutomationPeer> autoPeg;

    // See if we have a managed peer
    autoPeg.TrySet( this );
    if( autoPeg )
    {
        ASSERT( HasManagedPeer() );
        IFC(FxCallbacks::AutomationPeer_CallAutomationPeerMethod(this, 0 /* SetFocus */));
    }
    else
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }
Cleanup:
    return;
}

//------------------------------------------------------------------------
//
//  Method:   InvalidatePeer
//
//  Synopsis:
//      Return name of the class
//
//------------------------------------------------------------------------

void CAutomationPeer::InvalidatePeer()
{
    CAutomationPeerEventArgs *pArgs = nullptr;

    // Special property for async event callbacks
    EventHandle hEvent(KnownEventIndex::DependencyObject_RaiseAsyncCallback);

     // Create the DO that represents the event args
    pArgs = new CAutomationPeerEventArgs();

    pArgs->m_pAP = this;
    pArgs->m_pAP->PegManagedPeerNoRef();

    // Raise the request
    GetContext()->GetEventManager()->ThreadSafeRaise(hEvent, FALSE, nullptr, (CEventArgs**)&pArgs);
    ReleaseInterface(pArgs);
}

HRESULT CAutomationPeer::ShowContextMenu()
{
    CorePeggedPtr<CAutomationPeer> autoPeg;

    // See if we have a managed peer
    autoPeg.TrySet( this );
    if( autoPeg )
    {
        ASSERT( HasManagedPeer() );
        IFC_RETURN(FxCallbacks::AutomationPeer_CallAutomationPeerMethod(this, 1 /* ShowContextMenu*/));
    }
    else
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    return S_OK;
}

HRESULT CAutomationPeer::ShowContextMenuHelper()
{
    return S_OK;
}

// CUIAWindow starts hit testing from the root and uses XAML hit-testing and handles other exceptional cases to finally
// reach the correct AP which contains the point. But there are cases involving custom APs where they can host further
// accessible controls within this area which are not known to XAML. In that case we want to give the target AP a chance
// to return any containing accessible object that actually contains the point.
HRESULT CAutomationPeer::GetElementFromPoint(
    _In_ XPOINTF *pLocation,
    _Outptr_result_maybenull_ CAutomationPeer** ppReturnAP,
    _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk)
{
    xref_ptr<CDependencyObject> spResultAPAsDO;
    xref_ptr<IUnknown> spUnkNativeNode;
    CorePeggedPtr<CAutomationPeer> autoPeg;

    *ppReturnAP = nullptr;
    *ppReturnIREPFAsUnk = nullptr;

    // Ignore if we don't have a managed peer.
    autoPeg.TrySet(this);
    if (autoPeg)
    {
        CValue param;
        param.WrapPoint(pLocation);
        ASSERT(HasManagedPeer());

        IFC_RETURN(FxCallbacks::AutomationPeer_GetElementFromPoint(this, param, spResultAPAsDO.ReleaseAndGetAddressOf(), spUnkNativeNode.ReleaseAndGetAddressOf()));

        // we are using two types of return values for APs or IREPS to make sure the whole contract is strongly typed instead of using void**,
        // that means the target is either AP or IREPS it can't be both.
        ASSERT(spResultAPAsDO == nullptr || spUnkNativeNode == nullptr);
    }
    else
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    // GetElementFromPoint deals with both AutomationPeer as well as native UIA nodes.
    // We use two return values for each of them for making sure that contract
    // between layers is strongly typed.
    *ppReturnAP = do_pointer_cast<CAutomationPeer>(spResultAPAsDO.detach());
    *ppReturnIREPFAsUnk = spUnkNativeNode.detach();

    return S_OK;
}

// CUIAWindow uses XAML's Focus manager to reach the correct AP containing focus.
// But there are cases involving custom APs where they can host further accessible
// controls which are not known to XAML. In that case we want to give the target AP
// a chance to return any containing accessible object that actually contains the focus.
HRESULT CAutomationPeer::GetFocusedElement(_Outptr_result_maybenull_ CAutomationPeer** ppReturnAP, _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk)
{
    xref_ptr<CDependencyObject> spResultAPAsDO;
    xref_ptr<IUnknown> spUnkNativeNode;
    CorePeggedPtr<CAutomationPeer> autoPeg;

    *ppReturnAP = nullptr;
    *ppReturnIREPFAsUnk = nullptr;

    // Ignore if we don't have a managed peer.
    autoPeg.TrySet(this);
    if (autoPeg)
    {
        ASSERT(HasManagedPeer());
        IFC_RETURN(FxCallbacks::AutomationPeer_GetFocusedElement(this, spResultAPAsDO.ReleaseAndGetAddressOf(), spUnkNativeNode.ReleaseAndGetAddressOf()));

        // we are using two types of return values for APs or IREPS to make sure the whole contract is strongly typed instead of using void**,
        // that means the target is either AP or IREPS it can't be both.
        ASSERT(spResultAPAsDO == nullptr || spUnkNativeNode == nullptr);
    }
    else
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    // GetFocusedElement deals with both AutomationPeer as well as native UIA nodes.
    // We use two return values for each of them for making sure that contract
    // between layers is strongly typed.
    *ppReturnAP = do_pointer_cast<CAutomationPeer>(spResultAPAsDO.detach());
    *ppReturnIREPFAsUnk = spUnkNativeNode.detach();

    return S_OK;
}

//======================================================================================================
//
//  The "Core" methods below are the standard implementation for a blank AutomationPeer.
//  Elements which wish to have custom returns should override the below functions
//
//======================================================================================================

//------------------------------------------------------------------------
//
//  Method:   GetRootChildrenCore
//
//  Synopsis:
//      Walks the root DO tree to return the children APs
//
//------------------------------------------------------------------------
XINT32 CAutomationPeer::GetRootChildrenCore(CAutomationPeer*** pppReturnAP)
{
    return 0;
}

HRESULT CAutomationPeer::GetAutomationIdHelper(_Out_ xstring_ptr* pstrRetVal)
{
    // If AutomationId is not set, FrameworkElementAP and HyperlinkAP uses the element names as AutomationID.
    if (m_pDO != nullptr && pstrRetVal->IsNullOrEmpty() && !m_pDO->m_strName.IsNullOrEmpty())
    {
        *pstrRetVal = m_pDO->m_strName;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   HasKeyboardFocusCore
//
//  Synopsis:
//      Return if this AP has Focus or not, as SetFocusCore now considers enabling SetFocusCore for core CAutomationPeer
//      which doesn't have corresponding UIElement, HasKeyBoardFocusCore reflects that as well.
//
//------------------------------------------------------------------------

HRESULT CAutomationPeer::HasKeyboardFocusHelper(_Out_ BOOLEAN* pRetVal)
{
    xref_ptr<CAutomationPeer> ap;
    XINT32 isKeyboardFocusable = FALSE;

    *pRetVal = FALSE;

    IFC_RETURN(IsKeyboardFocusable(&isKeyboardFocusable));
    if(isKeyboardFocusable)
    {
        if (CFocusManager* focusManager = GetFocusManagerNoRef())
        {
            ap = focusManager->GetFocusedAutomationPeer();

            if(this == static_cast<CAutomationPeer*>(ap.get()))
            {
                *pRetVal = TRUE;
            }
        }
    }

    return S_OK;
}

// Core layer helper overrides for the cases when property is required to be evaluated in core layer instead of framework layer.

HRESULT CAutomationPeer::IsEnabledHelper(_Out_ BOOLEAN* pRetVal)
{
    *pRetVal = TRUE;

    return S_OK;
}

HRESULT CAutomationPeer::IsKeyboardFocusableHelper(_Out_ BOOLEAN* pRetVal)
{
    *pRetVal = FALSE;

    return S_OK;
}

HRESULT CAutomationPeer::IsOffscreenHelper(bool ignoreClippingOnScrollContentPresenters, _Out_ BOOLEAN* pRetVal)
{
    *pRetVal = FALSE;
    return S_OK;
}

HRESULT CAutomationPeer::GetCultureHelper(_Out_ int* returnValue)
{
    IFC_RETURN(ThrowElementNotAvailableError());

    CValue culture;
    IFC_RETURN(m_pDO->GetValueByIndex(KnownPropertyIndex::AutomationProperties_Culture, &culture));
    *returnValue = culture.As<valueSigned>();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetFocusCore
//
//  Synopsis:
//      If this is Focusable and not the current focused element, then we should be able to treat it as focused.
//      It basically supports the cases for Application Devs while they create Custom AutomationPeer for non-UIElement controls.
//      It also enables showing Soft keyboard during IHM scenarios in such cases as needed.
//
//------------------------------------------------------------------------
HRESULT CAutomationPeer::SetFocusHelper()
{
    XINT32 isKeyboardFocusable = FALSE;

    IFC_RETURN(IsKeyboardFocusable(&isKeyboardFocusable));
    if(isKeyboardFocusable)
    {
        if (CFocusManager* focusManager = GetFocusManagerNoRef())
        {
            xref_ptr<CAutomationPeer> ap { focusManager->GetFocusedAutomationPeer() };

            if(this != static_cast<CAutomationPeer*>(ap.get()))
            {
                if ( S_OK == GetContext()->UIAClientsAreListening(UIAXcp::AEAutomationFocusChanged) )
                {
                    ap = this;

                    if(this->GetAPEventsSource())
                    {
                        ap = this->GetAPEventsSource();
                    }

                    focusManager->SetFocusedAutomationPeer(this);
                    ap->RaiseAutomationEvent(UIAXcp::AEAutomationFocusChanged);
                }
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetAutomationFocusHelper
//
//  Synopsis:
//     We want this method to set automation focus, but not keyboard focus, to the element that this is an automation peer of.
//     AutomationPeer's override of SetFocusHelper() happens to do exactly this, so we'll just use its implementation.
//     (We can't just call SetFocusHelper(), because would likely be overridden by CFrameworkElementAutomationPeer.)
//     The motivation behind this is, for example, Pivot, where keyboard focus is given to the header panel to enable
//     keyboarding scenarios, but we want to act as though individual PivotItems have focus for the purposes of what we
//     report to UIA clients, since they require automation focus to be given to elements before they can read them
//     and report their contents.
//
//------------------------------------------------------------------------
void CAutomationPeer::SetAutomationFocusHelper()
{
    CAutomationPeer::SetFocusHelper();
}

bool CAutomationPeer::HasParent()
{
    return m_pAPParent != nullptr;
}

bool CAutomationPeer::ListenerExists(_In_ UIAXcp::APAutomationEvents eventId)
{
    HRESULT hr = S_OK;
    hr = GetContext()->UIAClientsAreListening(eventId);
    if ( hr == S_OK )
    {
        return true;
    }
    return false;
}

void CAutomationPeer::RaiseAutomationEvent(_In_ UIAXcp::APAutomationEvents eventId)
{
    if (ListenerExists(eventId))
    {
        IGNOREHR(GetContext()->UIARaiseAutomationEvent(this, eventId));
    }
}

void CAutomationPeer::RaisePropertyChangedEvent(_In_ UIAXcp::APAutomationProperties eAutomationProperty,
                                                _In_ const CValue& oldValue,
                                                _In_ const CValue& newValue)
{
    IGNOREHR(GetContext()->UIARaiseAutomationPropertyChangedEvent(this, eAutomationProperty, oldValue, newValue));
}

void CAutomationPeer::RaiseTextEditTextChangedEvent(_In_ UIAXcp::AutomationTextEditChangeType automationTextEditType, _In_ CValue *pChangedData)
{
    IGNOREHR(GetContext()->UIARaiseTextEditTextChangedEvent(this, automationTextEditType, pChangedData));
}

void CAutomationPeer::RaiseNotificationEvent(
    UIAXcp::AutomationNotificationKind notificationKind,
    UIAXcp::AutomationNotificationProcessing notificationProcessing,
    _In_opt_ xstring_ptr displayString,
    _In_ xstring_ptr activityId)
{
    bool reentrancyChecksEnabled = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableReentrancyChecks);
    if (reentrancyChecksEnabled)
    {
        if (ListenerExists(UIAXcp::AESelectionItemPatternOnElementSelected))
        {
            VERIFYHR(GetContext()->UIARaiseNotificationEvent(this, notificationKind, notificationProcessing, displayString, activityId));
        }
    }
    else
    {
        VERIFYHR(GetContext()->UIARaiseNotificationEvent(this, notificationKind, notificationProcessing, displayString, activityId));
    }
}
//------------------------------------------------------------------------
//
//  Method:   GetAutomationPeerStringValueFromManaged
//
//  Synopsis:
//      Calls into managed to retrieve an xstring_ptr
//
//------------------------------------------------------------------------
HRESULT CAutomationPeer::GetAutomationPeerStringValueFromManaged(
    _In_ UIAXcp::APAutomationProperties app,
    _Out_ xstring_ptr* pstrString)
{
    HRESULT hr = S_OK;
    WCHAR pStringDefault[DEFAULT_AP_STRING_SIZE] = {0};
    WCHAR* pString = nullptr;
    XINT32 cString = DEFAULT_AP_STRING_SIZE-1;                            // size = 128, should account for the majority of cases

    CorePeggedPtr<CAutomationPeer> autoPeg;

    // Ignore if we don't have a managed peer.
    autoPeg.TrySet( this );
    if( autoPeg )
    {
        ASSERT( HasManagedPeer() );

        hr = FxCallbacks::AutomationPeer_GetAutomationPeerStringValue(this, app, pStringDefault, &cString);

        if ( FAILED(hr) )
        {
            // quirking this behavior by keeping 2048 char limit check for value for RC, cString < MAX_AP_STRING_SIZE
            if (cString >= DEFAULT_AP_STRING_SIZE)
            {
                pString = new WCHAR[cString +1];
                IFC(FxCallbacks::AutomationPeer_GetAutomationPeerStringValue(this, app, pString, &cString));
            }
            else
            {
                IFC(hr);
            }
            pString[cString] = '\0';

            IFC(xstring_ptr::CloneBuffer(pString, cString, pstrString));
        }
        else
        {
            IFC(xstring_ptr::CloneBuffer(pStringDefault, cString, pstrString));
        }

    }
    else
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

Cleanup:
    delete [] pString;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   GetAutomationPeerIntValueFromManaged
//
//  Synopsis:
//      Calls into managed to retrieve an Int
//
//------------------------------------------------------------------------
HRESULT CAutomationPeer::GetAutomationPeerIntValueFromManaged(
    _In_ UIAXcp::APAutomationProperties app,
    _Out_ XINT32* pRetVal)
{
    CorePeggedPtr<CAutomationPeer> autoPeg;

    // Ignore if we don't have a managed peer.
    autoPeg.TrySet( this );
    if( autoPeg )
    {
        ASSERT( HasManagedPeer() );

        IFC_RETURN(FxCallbacks::AutomationPeer_GetAutomationPeerIntValue(this, app, pRetVal));
    }
    else
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetAutomationPeerPointValueFromManaged
//
//  Synopsis:
//      Calls into managed to retrieve a XPOINTF
//
//------------------------------------------------------------------------
HRESULT CAutomationPeer::GetAutomationPeerPointValueFromManaged(
    _In_ UIAXcp::APAutomationProperties app,
    _Out_ XPOINTF* pPointF)
{
    CorePeggedPtr<CAutomationPeer> autoPeg;

    // Ignore if we don't have a managed peer.
    autoPeg.TrySet( this );
    if ( autoPeg )
    {
        ASSERT( HasManagedPeer() );

        IFC_RETURN(FxCallbacks::AutomationPeer_GetAutomationPeerPointValue(this, app, pPointF));
    }
    else
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetAutomationPeerRectValueFromManaged
//
//  Synopsis:
//      Calls into managed to retrieve a XRECTF
//
//------------------------------------------------------------------------
HRESULT CAutomationPeer::GetAutomationPeerRectValueFromManaged(
    _In_ UIAXcp::APAutomationProperties app,
    _Out_ XRECTF* pRectF)
{
    CorePeggedPtr<CAutomationPeer> autoPeg;

    // Ignore if we don't have a managed peer.
    autoPeg.TrySet( this );
    if ( autoPeg )
    {
        ASSERT( HasManagedPeer() );

        IFC_RETURN(FxCallbacks::AutomationPeer_GetAutomationPeerRectValue(this, app, pRectF));
    }
    else
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetAutomationPeerAPValueFromManaged
//
//  Synopsis:
//      Calls into managed to retrieve a automation peer
//
//------------------------------------------------------------------------
HRESULT CAutomationPeer::GetAutomationPeerAPValueFromManaged(
    _In_ UIAXcp::APAutomationProperties app,
    _Out_ CAutomationPeer** ppAP)
{
    CorePeggedPtr<CAutomationPeer> autoPeg;

    // Ignore if we don't have a managed peer.
    autoPeg.TrySet( this );
    if ( autoPeg )
    {
        ASSERT( HasManagedPeer() );
        CDependencyObject* pAP = nullptr;
        IFC_RETURN(FxCallbacks::AutomationPeer_GetAutomationPeerAPValue(this, app, &pAP));
        *ppAP = do_pointer_cast<CAutomationPeer>(pAP);
    }
    else
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetAutomationPeerDOValueFromManaged
//
//  Synopsis:
//      Calls into managed to retrieve a DependencyObject.
//
//------------------------------------------------------------------------
HRESULT CAutomationPeer::GetAutomationPeerDOValueFromManaged(
    _In_ UIAXcp::APAutomationProperties app,
    _Outptr_ CDependencyObject** ppDO)
{
    *ppDO = nullptr;
    CorePeggedPtr<CAutomationPeer> autoPeg;

    // Ignore if we don't have a managed peer.
    autoPeg.TrySet( this );
    if( autoPeg )
    {
        ASSERT( HasManagedPeer() );
        IFC_RETURN(FxCallbacks::AutomationPeer_GetAutomationPeerDOValue(this, app, ppDO));
    }
    else
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RaiseAutomaticPropertyChanges
//
//  Synopsis:
//      Raises events for each of the properties that are automagic
//
//------------------------------------------------------------------------
void CAutomationPeer::RaiseAutomaticPropertyChanges(bool firePropertyChangedEvents)
{
    HRESULT hr = S_OK;  // WARNING_IGNORES_FAILURES
    CValue oldValue;
    CValue newValue;
    XINT32 bIsEnabled = FALSE;
    XINT32 bIsOffscreen = TRUE;
    xstring_ptr strName;
    xstring_ptr strItemStatus;

    IFC(IsEnabled(&bIsEnabled));
    IFC(IsOffscreen(&bIsOffscreen));
    IFC(GetName(&strName));
    IFC(GetItemStatus(&strItemStatus));

    if (firePropertyChangedEvents)
    {
        if (!!nCurrentIsEnabled != !!bIsEnabled)
        {
            oldValue.SetBool(!!nCurrentIsEnabled);
            newValue.SetBool(!!bIsEnabled);
            RaisePropertyChangedEvent(UIAXcp::APIsEnabledProperty, oldValue, newValue);
            nCurrentIsEnabled = !!bIsEnabled;
        }

        if (!!nCurrentIsOffscreen != !!bIsOffscreen)
        {
            oldValue.SetBool(!!nCurrentIsOffscreen);
            newValue.SetBool(!!bIsOffscreen);
            RaisePropertyChangedEvent(UIAXcp::APIsOffscreenProperty, oldValue, newValue);
            nCurrentIsOffscreen = !!bIsOffscreen;
        }

        if ((!strCurrentName.IsNull() && !strCurrentName.Equals(strName)) || (strCurrentName.IsNull() && !strName.IsNull()))
        {
            oldValue.SetString(strCurrentName);
            newValue.SetString(strName);
            RaisePropertyChangedEvent(UIAXcp::APNameProperty, oldValue, newValue);
            if (!strName.IsNull())
            {
                strCurrentName = strName;
            }
        }

        if ((!strCurrentItemStatus.IsNull() && !strCurrentItemStatus.Equals(strItemStatus)) || (strCurrentItemStatus.IsNull() && !strItemStatus.IsNull()))
        {
            oldValue.SetString(strCurrentItemStatus);
            newValue.SetString(strItemStatus);
            RaisePropertyChangedEvent(UIAXcp::APItemStatusProperty, oldValue, newValue);
            if (!strItemStatus.IsNull())
            {
                strCurrentItemStatus = strItemStatus;
            }
        }
    }
    else
    {
        nCurrentIsEnabled = !!bIsEnabled;
        nCurrentIsOffscreen = !!bIsOffscreen;

        if (!strName.IsNull())
        {
            strCurrentName = strName;
        }

        if (!strItemStatus.IsNull())
        {
            strCurrentItemStatus = strItemStatus;
        }
    }

Cleanup:
    return;
}

HRESULT CAutomationPeer::GetPositionInSet(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APPositionInSetProperty, pRetVal);
}

HRESULT CAutomationPeer::GetSizeOfSet(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APSizeOfSetProperty, pRetVal);
}

HRESULT CAutomationPeer::GetLevel(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APLevelProperty, pRetVal);
}

HRESULT CAutomationPeer::GetAnnotations(_Outptr_ CAutomationPeerAnnotationCollection** ppRetVal)
{
    xref_ptr<CDependencyObject> spDO = nullptr;
    IFC_RETURN(GetAutomationPeerDOValueFromManaged(UIAXcp::APAnnotationsProperty, spDO.ReleaseAndGetAddressOf()));
    IFC_RETURN(DoPointerCast(*ppRetVal, spDO.detach()));
    return S_OK;
}

HRESULT CAutomationPeer::GetLandmarkType(_Out_ UIAXcp::AutomationLandmarkType* pRetVal)
{
    XINT32 nValue = 0;

    IFC_RETURN(GetAutomationPeerIntValueFromManaged(UIAXcp::APLandmarkTypeProperty, &nValue));
    *pRetVal = static_cast<UIAXcp::AutomationLandmarkType>(nValue);

    return S_OK;
}

HRESULT CAutomationPeer::GetLocalizedLandmarkType(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APLocalizedLandmarkTypeProperty, pstrRetVal);
}

HRESULT CAutomationPeer::IsPeripheral(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APIsPeripheralProperty, pRetVal);
}

HRESULT CAutomationPeer::IsDataValidForForm(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APIsDataValidForFormProperty, pRetVal);
}

HRESULT CAutomationPeer::GetFullDescription(_Out_ xstring_ptr* pstrRetVal)
{
    return GetAutomationPeerStringValueFromManaged(UIAXcp::APFullDescriptionProperty, pstrRetVal);
}

HRESULT CAutomationPeer::GetDescribedBy(_Outptr_ CAutomationPeerCollection **ppRetVal)
{
    xref_ptr<CDependencyObject> spDO = nullptr;
    IFC_RETURN(GetAutomationPeerDOValueFromManaged(UIAXcp::APDescribedByProperty, spDO.ReleaseAndGetAddressOf()));
    IFC_RETURN(DoPointerCast(*ppRetVal, spDO.detach()));
    return S_OK;
}

HRESULT CAutomationPeer::GetFlowsTo(_Outptr_ CAutomationPeerCollection **ppRetVal)
{
    xref_ptr<CDependencyObject> spDO = nullptr;
    IFC_RETURN(GetAutomationPeerDOValueFromManaged(UIAXcp::APFlowsToProperty, spDO.ReleaseAndGetAddressOf()));
    IFC_RETURN(DoPointerCast(*ppRetVal, spDO.detach()));
    return S_OK;
}

HRESULT CAutomationPeer::GetFlowsFrom(_Outptr_ CAutomationPeerCollection **ppRetVal)
{
    xref_ptr<CDependencyObject> spDO = nullptr;
    IFC_RETURN(GetAutomationPeerDOValueFromManaged(UIAXcp::APFlowsFromProperty, spDO.ReleaseAndGetAddressOf()));
    IFC_RETURN(DoPointerCast(*ppRetVal, spDO.detach()));
    return S_OK;
}

HRESULT CAutomationPeer::GetCulture(_Out_ XINT32* pRetVal)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APCultureProperty, pRetVal);
}

_Check_return_ HRESULT CAutomationPeer::ThrowElementNotAvailableError()
{
    return (m_pDO ? S_OK : static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
}

int CAutomationPeer::GetRuntimeId()
{
    if (m_runtimeId == 0)
    {
        m_runtimeId = GetContext()->GetNextRuntimeId();
    }

    return m_runtimeId;
}

_Check_return_ HRESULT CAutomationPeer::GetHeadingLevel(_Out_ UIAXcp::AutomationHeadingLevel* pRetVal)
{
    XINT32 nValue = 0;

    IFC_RETURN(GetAutomationPeerIntValueFromManaged(UIAXcp::APHeadingLevelProperty, &nValue));
    *pRetVal = static_cast<UIAXcp::AutomationHeadingLevel>(nValue);

    return S_OK;
}

_Check_return_ HRESULT CAutomationPeer::IsDialog(_Out_ XINT32* returnValue)
{
    return GetAutomationPeerIntValueFromManaged(UIAXcp::APIsDialogProperty, returnValue);
}

// Find the root for the given AutomationPeer.
// Since AutomationPeers don't enter the live tree, we need to look at a few pointers to figure out which
// tree of XAML the peer belongs to.
CDependencyObject* CAutomationPeer::GetRootNoRef() const
{
    // Note: m_pDO usually refers to the element peer of the AutomationPeer, but not always. In some cases m_pDO points to the parent.
    // See for example ListViewItemDataAutomationPeerFactory::CreateInstanceWithParentAndItemImpl, where we pass the parent as the
    // 2nd parameter to ActivateInstance, which is then assigned to m_pDO.
    // In order to deal with this case, we first follow the m_pDO pointer as long as it's pointing to an AutomationPeer.  When
    // we hit a non-automation peer, we're back to the live XAML element tree, and return the root for that element.
    // http://osgvsowi/14707451 -- clean this up.
    const CAutomationPeer* node = this;

    while (true)
    {
        const CAutomationPeer* nextNodeAsAP = do_pointer_cast<CAutomationPeer>(node->m_pDO);
        if (nextNodeAsAP)
        {
            node = nextNodeAsAP;
        }
        else
        {
            break;
        }
    }

    if (node->m_pDO == nullptr && m_pAPParent != nullptr)
    {
        // In some cases (such as LoopingSelector), we may not have a pointer to a DependencyObject in the live tree.
        // If we have an AP parent, let's try following that chain to the root instead.
        return m_pAPParent->GetRootNoRef();
    }

    return GetContext()->GetRootForElement(node->m_pDO);
}

CFocusManager* CAutomationPeer::GetFocusManagerNoRef() const
{
    if (GetContext())
    {
        if (CDependencyObject* root = GetRootNoRef())
        {
            return VisualTree::GetFocusManagerForElement(root);
        }
    }
    return nullptr;
}