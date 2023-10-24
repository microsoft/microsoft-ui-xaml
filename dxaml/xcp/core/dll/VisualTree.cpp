// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ConnectedAnimationRoot.h"
#include <ContentRoot.h>
#include <CoreWindowRootScale.h>
#include "DCompTreeHost.h"
#include "ErrorHelper.h"
#include "FxCallbacks.h"
#include <QualifierContext.h>
#include "ToolTipService_Partial.h"
#include "XamlIslandRoot.h"
#include <XamlIslandRootScale.h>
#include <XamlOneCoreTransforms.h>

// Marked noinline so we can easily breakpoint
// When we hit this warning, it represents code that will not work correctly in DesktopWindowXamlSource or AppWindows
// Task 19408294: VisualTreeNotFoundWarning -- fix the places in code where we fall back to using the main tree because we can't find the right VisualTree
__declspec(noinline) void VisualTreeNotFoundWarning()
{
#if DBG
    static bool gaveWarning = false;
    if (!gaveWarning)
    {
        RAWTRACE(TraceAlways, L"VisualTreeNotFoundWarning: Could not find VisualTree for element");
        gaveWarning = true;
    }
#endif
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Shutdown this visual tree.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::Shutdown()
{
    m_shutdownInProgress = true;
    auto inShutdownGuard = wil::scope_exit([&]
    {
        m_shutdownInProgress = false;
    });

    m_rootElement = nullptr;

    if (m_xamlRoot)
    {
        FxCallbacks::XamlRoot_UpdatePeg(m_xamlRoot.Get(), false);
        m_xamlRoot = nullptr;
    }

    if (m_rootVisual)
    {
        m_rootVisual->Shutdown();
    }

    if (m_popupRoot)
    {
        m_popupRoot->UnpegManagedPeer();
        m_popupRoot.reset();
    }

    if (m_printRoot)
    {
        m_printRoot->UnpegManagedPeer();
        m_printRoot.reset();
    }

    if (m_transitionRoot)
    {
        m_transitionRoot->UnpegManagedPeer();
        m_transitionRoot.reset();
    }

    if (m_fullWindowMediaRoot)
    {
        m_fullWindowMediaRoot->UnpegManagedPeer();
        m_fullWindowMediaRoot.reset();
    }

    if (m_renderTargetBitmapRoot)
    {
        m_renderTargetBitmapRoot->UnpegManagedPeer();
        m_renderTargetBitmapRoot.reset();
    }

    if (m_connectedAnimationRoot)
    {
        m_connectedAnimationRoot->UnpegManagedPeer();
        m_connectedAnimationRoot.reset();
    }

    if (m_xamlIslandRootCollection)
    {
        m_xamlIslandRootCollection->UnpegManagedPeer();
        m_xamlIslandRootCollection.reset();
    }

    if (m_visualDiagnosticsRoot)
    {
        m_visualDiagnosticsRoot->UnpegManagedPeer();
        m_visualDiagnosticsRoot.reset();
    }

    // For XamlIslands, this object has a reference on the composition island, which keeps a MockDComp island alive
    // during test cleanup.
    if (m_rootScale)
    {
        m_rootScale.reset();
    }

    if (ShouldFinalShutdown())
    {
        FinalShutdown();
    }

    m_isShutdown = true;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Replace the existing popup root (if any) with the provided one
//
//------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::EnsurePopupRoot()
{
    if (!m_popupRoot)
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);
        IFC_RETURN(CreateDO(m_popupRoot.ReleaseAndGetAddressOf(), &cp));

        IFC_RETURN(m_popupRoot->SetZIndex(POPUP_ZINDEX));

        // Protect the root.
        IFC_RETURN(m_popupRoot->EnsurePeerAndTryPeg());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Ensure that the visual diagnostics root exists.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT VisualTree::EnsureVisualDiagnosticsRoot()
{
    if (!m_visualDiagnosticsRoot)
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);
        IFC_RETURN(CreateDO(m_visualDiagnosticsRoot.ReleaseAndGetAddressOf(), &cp));
        IFC_RETURN(m_visualDiagnosticsRoot->SetZIndex(VISUALDIAGNOSTICSROOT_ZINDEX));
        // Protect the root.
        IFC_RETURN(m_visualDiagnosticsRoot->EnsurePeerAndTryPeg());
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Ensure that the connected animation root exists.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::EnsureConnectedAnimationRoot()
{
    if (m_connectedAnimationRoot == nullptr)
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);
        IFC_RETURN(CreateDO(m_connectedAnimationRoot.ReleaseAndGetAddressOf(), &cp));

        IFC_RETURN(m_connectedAnimationRoot->SetZIndex(CONNECTEDANIMATIONROOT_ZINDEX));

        // Protect the root.
        IFC_RETURN(m_connectedAnimationRoot->EnsurePeerAndTryPeg());
    }

    return S_OK;
}

// Find the XamlIslandRootCollection in the tree, if there is one
CXamlIslandRootCollection* VisualTree::GetXamlIslandRootCollection()
{
    if (!m_shutdownInProgress)
    {
        EnsureXamlIslandRootCollection();
    }
    return m_xamlIslandRootCollection.get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Replace the existing print root (if any) with the provided one
//
//------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::EnsurePrintRoot()
{
    if (IsMainVisualTree() && !m_printRoot)
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);
        IFC_RETURN(CreateDO(m_printRoot.ReleaseAndGetAddressOf(), &cp));

        m_printRoot->SetIsStandardNameScopeOwner(TRUE);
        m_printRoot->SetIsStandardNameScopeMember(FALSE);
        m_printRoot->SetOpacityLocal(0.0f);

        // Protect the root.
        IFC_RETURN(m_printRoot->EnsurePeerAndTryPeg());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Replace the existing transition root (if any) with the provided one
//
//------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::EnsureTransitionRoot()
{
    if (!m_transitionRoot)
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);
        IFC_RETURN(CreateDO(m_transitionRoot.ReleaseAndGetAddressOf(), &cp));

        IFC_RETURN(m_transitionRoot->SetZIndex(TRANSITIONROOT_ZINDEX));

        // Protect the root.
        IFC_RETURN(m_transitionRoot->EnsurePeerAndTryPeg());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Replace the existing full window media root (if any) with the provided one
//
//------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::EnsureFullWindowMediaRoot()
{
    if (IsMainVisualTree() && !m_fullWindowMediaRoot)
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);
        IFC_RETURN(CreateDO(m_fullWindowMediaRoot.ReleaseAndGetAddressOf(), &cp));

        IFC_RETURN(m_fullWindowMediaRoot->SetZIndex(FULLWINDOWMEDIAROOT_ZINDEX));

        // Protect the root.
        IFC_RETURN(m_fullWindowMediaRoot->EnsurePeerAndTryPeg());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Makes sure that a root for RenderTargetBitmap source elements
//      is created.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::EnsureRenderTargetBitmapRoot()
{
    if (IsMainVisualTree() && !m_renderTargetBitmapRoot)
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);
        IFC_RETURN(CreateDO(m_renderTargetBitmapRoot.ReleaseAndGetAddressOf(), &cp));

        // Protect the root.
        IFC_RETURN(m_renderTargetBitmapRoot->EnsurePeerAndTryPeg());
    }

    return S_OK;
}

void VisualTree::EnsureXamlIslandRootCollection()
{
    if (IsMainVisualTree() && !m_xamlIslandRootCollection)
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);
        IFCFAILFAST(CreateDO(m_xamlIslandRootCollection.ReleaseAndGetAddressOf(), &cp));

        // Protect the root.
        IFCFAILFAST(m_xamlIslandRootCollection->EnsurePeerAndTryPeg());
        IFCFAILFAST(m_xamlIslandRootCollection->EnsureChildrenCollection());
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the main popup root for this visual tree.
//
//------------------------------------------------------------------------
CPopupRoot* VisualTree::GetPopupRoot()
{
    if (!(m_shutdownInProgress || m_isShutdown))
    {
        VERIFYHR(EnsurePopupRoot());
    }
    return m_popupRoot;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the visual diagnostics root for this visual tree
//
//------------------------------------------------------------------------
CGrid* VisualTree::GetVisualDiagnosticsRoot()
{
    if (!m_shutdownInProgress)
    {
        VERIFYHR(EnsureVisualDiagnosticsRoot());
    }
    return m_visualDiagnosticsRoot.get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the connected animation root for this visual tree
//
//------------------------------------------------------------------------
CConnectedAnimationRoot* VisualTree::GetConnectedAnimationRoot()
{
    if (!m_shutdownInProgress)
    {
        VERIFYHR(EnsureConnectedAnimationRoot());
    }
    return m_connectedAnimationRoot.get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the main transition root for this visual tree.
//
//------------------------------------------------------------------------
CTransitionRoot* VisualTree::GetTransitionRoot()
{
    if (!m_shutdownInProgress)
    {
        VERIFYHR(EnsureTransitionRoot());
    }
    return m_transitionRoot.get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the main print root for this visual tree.
//
//------------------------------------------------------------------------
CPrintRoot* VisualTree::GetPrintRoot()
{
    if (!m_shutdownInProgress)
    {
        VERIFYHR(EnsurePrintRoot());
    }
    return m_printRoot.get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the full window media root for this visual tree.
//
//------------------------------------------------------------------------
CFullWindowMediaRoot* VisualTree::GetFullWindowMediaRoot()
{
    if (!m_shutdownInProgress)
    {
        VERIFYHR(EnsureFullWindowMediaRoot());
    }
    return m_fullWindowMediaRoot.get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the main rendertargetbitmap root for this visual tree.
//
//------------------------------------------------------------------------
CRenderTargetBitmapRoot* VisualTree::GetRenderTargetBitmapRoot()
{
    if (m_pCoreNoRef->IsInBackgroundTask() && !m_shutdownInProgress)
    {
        VERIFYHR(EnsureRenderTargetBitmapRoot());
    }
    return m_renderTargetBitmapRoot.get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the public root visual for this visual tree
//
//------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::SetPublicRootVisual(
    _In_opt_ CUIElement* pRoot,
    _In_opt_ CScrollContentControl* pRootScrollViewer,
    _In_opt_ CContentPresenter* pRootContentPresenter
    )
{
    HRESULT hr = S_OK;

    // NOTE: This doesn't check for the root scroll viewer changing independently of the root visual.
    if (pRoot == m_publicRootVisual.get())
    {
        goto Cleanup;
    }

    TracePutRootVisualBegin();

    // If the public root visual changes, we need to remove and re-add the existing roots so that
    // they use the new public visual as namescope owner if needed, but we don't want to release them.
    //
    // Note: This needs to be done even if the public root visual is null. The app can explicitly set
    // a null root, which will cause us to create the popup/print/transition/media root elements. When
    // the app sets a non-null root later, we have to make sure to not add those elements a second time.
    //
    IFC(ResetRoots(nullptr));

    m_publicRootVisual.reset(pRoot);
    m_rootScrollViewer.reset(pRootScrollViewer);
    m_rootContentPresenter.reset(pRootContentPresenter);

    IFC(EnsureVisualDiagnosticsRoot());
    EnsureXamlIslandRootCollection();
    IFC(EnsureConnectedAnimationRoot());
    IFC(EnsurePopupRoot());
    IFC(EnsurePrintRoot());
    IFC(EnsureTransitionRoot());
    IFC(EnsureFullWindowMediaRoot());

    if (m_pCoreNoRef->IsInBackgroundTask())
    {
        IFC(EnsureRenderTargetBitmapRoot());
    }

    if (pRoot)
    {
        // A visual set as the root of the tree implicitly becomes a permanent
        // namescope owner, and will always have a name store.
        pRoot->SetIsStandardNameScopeOwner(TRUE);
        pRoot->SetIsStandardNameScopeMember(FALSE);
    }

    // Re-enter the roots with the new public root's namescope.
    IFC(AddRoot(m_visualDiagnosticsRoot.get()));
    IFC(AddRoot(m_xamlIslandRootCollection));
    IFC(AddRoot(m_connectedAnimationRoot.get()));
    IFC(AddRoot(m_popupRoot.get()));
    IFC(AddRoot(m_printRoot.get()));
    IFC(AddRoot(m_transitionRoot.get()));
    IFC(AddRoot(m_fullWindowMediaRoot.get()));

    if (m_pCoreNoRef->IsInBackgroundTask())
    {
        IFC(AddRoot(m_renderTargetBitmapRoot.get()));
    }

    if (pRoot)
    {
        if (m_rootScrollViewer)
        {
            // A visual set as the root SV of the tree implicitly becomes a permanent
            // namescope owner, and will always have a name store.
            m_rootScrollViewer->SetIsStandardNameScopeOwner(TRUE);
            m_rootScrollViewer->SetIsStandardNameScopeMember(FALSE);

            // Add the visual root as the child of the root ScrollViwer
            IFC(AddVisualRootToRootScrollViewer(pRoot));

            // Add the root ScrollViewer to the hidden root visual
            IFC(AddRootScrollViewer(m_rootScrollViewer.get()));

            m_bIsRootScrollViewerAddedToRoot = true;
        }
        else
        {
            IFC(AddRoot(pRoot));
        }

        IFC(m_pCoreNoRef->RaisePendingLoadedRequests());
    }

    GetContentRootNoRef()->AddPendingXamlRootChangedEvent(CContentRoot::ChangeType::Content);

Cleanup:
    // If anything failed during setting public root visual, reset the roots of visual tree.
    // This ensures we don't have visual tree in incorrect state.
    if (FAILED(hr))
    {
        HRESULT recordHr = S_OK;
        RECORDFAILURE(ResetRoots(nullptr));
    }

    TracePutRootVisualEnd();

    return hr;
}

CContentRoot* VisualTree::GetContentRootNoRef()
{
    return &m_coreContentRoot;
}

std::shared_ptr<QualifierContext> VisualTree::GetQualifierContext()
{
    if (!m_pQualifierContext)
    {
        m_pQualifierContext = std::make_shared<QualifierContext>();
    }

    return m_pQualifierContext;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance of the VisualTree class.
//
//------------------------------------------------------------------------
VisualTree::VisualTree(_In_ CCoreServices *pCore, _In_ XUINT32 backgroundColor, _In_opt_ CUIElement* rootElement, _In_ CContentRoot& coreContentRoot)
    : m_pCoreNoRef(pCore)
    , m_bIsRootScrollViewerAddedToRoot(false)
    , m_shutdownInProgress(false)
    , m_isShutdown(false)
    , m_coreContentRoot(coreContentRoot)
{
    XCP_WEAK(&m_pCoreNoRef);

    if (rootElement)
    {
        m_rootElement = rootElement;
    }
    else
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);
        IFCFAILFAST(CreateDO(m_rootVisual.ReleaseAndGetAddressOf(), &cp));

        // Mark the element as the root of the render walk.
        m_rootVisual->SetRequiresComposition(
            CompositionRequirement::RootElement,
            IndependentAnimationType::None
            );

        IFCFAILFAST(m_rootVisual->SetAssociatedVisualTree(this));

        // Mark the root as used to prevent circular dependencies
        m_rootVisual->SetAssociated(true, nullptr /*Association owner needed only for shareable, non-parent aware DOs */);

        IFCFAILFAST(m_rootVisual->SetBackgroundColor(backgroundColor));

        m_layoutManager = std::make_shared<CLayoutManager>(m_pCoreNoRef, this);
        m_rootElement = m_rootVisual;
    }

    if (m_coreContentRoot.GetType() == CContentRoot::Type::CoreWindow)
    {
        const auto config = XamlOneCoreTransforms::IsEnabled() ? RootScaleConfig::ParentApply : RootScaleConfig::ParentInvert;
        m_rootScale = std::make_shared<CoreWindowRootScale>(config, m_pCoreNoRef, this);
    }
    else if (m_coreContentRoot.GetType() == CContentRoot::Type::XamlIsland)
    {
        m_rootScale = std::make_shared<XamlIslandRootScale>(m_pCoreNoRef, this);
    }
    else
    {
        XAML_FAIL_FAST();
    }
}

VisualTree::~VisualTree()
{
    FinalShutdown();
}

XUINT32 VisualTree::AddRef()
{
    m_ref++;
    return m_coreContentRoot.AddRef();
}

XUINT32 VisualTree::Release()
{
    m_ref--;

    if (ShouldFinalShutdown())
    {
        FinalShutdown();
    }

    return m_coreContentRoot.Release();

    // Note: While technically this leaks (there's no "delete this" here) it doesn't practically cause any problems.
    // VisualTree is inlined into CContentRoot and will be deleted when the CContentRoot is deleted.
}

bool VisualTree::ShouldFinalShutdown() const
{
    // For legacy reasons, we need to ensure that the RootScrollViewer, RootContentPresenter, and
    // RootVisual are kept alive, even after resetting and shutting down the tree. However, because
    // the ContentRoot can now keep the VisualTree alive longer than before, we now need to be
    // careful not to leak these objects. Therefore, we will always reset these objects once there is
    // no dependency on the VisualTree.
    return m_ref == 1 && (m_isShutdown || m_shutdownInProgress);
}

void VisualTree::FinalShutdown()
{
    m_layoutManager.reset();
    m_rootContentPresenter.reset();
    m_rootScrollViewer.reset();

    if (m_rootVisual)
    {
        m_rootVisual->CleanupCompositionResources();
        IGNOREHR(m_rootVisual->SetAssociatedVisualTree(nullptr));

        m_rootVisual.reset();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static helper function that encapsulates walking up the tree
//      to get the CRootVisual.  This function returns NULL if walking
//      up the parent chain does not reach a CRootVisual
//
//------------------------------------------------------------------------
CRootVisual* VisualTree::GetRootForElement(_In_ const CDependencyObject* pObject)
{
    return pObject->GetContext()->GetMainRootVisual();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static helper function that encapsulates getting the CFocusManager
//      that is associated with the VisualTree containing the provided
//      object.  If the object is not contained within a VisualTree's tree
//      then this method falls back to returning the CFocusManager
//      associated with the core's main VisualTree.
//
//------------------------------------------------------------------------
_Check_return_ CFocusManager*
VisualTree::GetFocusManagerForElement(_In_ CDependencyObject *pObject, LookupOptions options)
{
    if (CContentRoot* contentRoot = GetContentRootForElement(pObject, options))
    {
        return contentRoot->GetFocusManagerNoRef();
    }

    if (CContentRoot* contentRoot = pObject->GetContext()->GetContentRootCoordinator()->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot())
    {
        return contentRoot->GetFocusManagerNoRef();
    }
    return nullptr;
}

_Check_return_ CInputManager*
VisualTree::GetInputManagerForElement(_In_ CDependencyObject *pObject)
{
    if (CContentRoot* contentRoot = GetContentRootForElement(pObject))
    {
        return &contentRoot->GetInputManager();
    }

    return nullptr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static helper function that encapsulates getting the CLayoutManager
//      that is associated with the VisualTree containing the provided
//      object.  If the object is not contained within a VisualTree's tree
//      then this method falls back to returning the CLayoutManager
//      associated with the core's main VisualTree.
//
//------------------------------------------------------------------------
_Ret_maybenull_ CLayoutManager*
VisualTree::GetLayoutManagerForElement(_In_ CDependencyObject *pObject)
{
    CLayoutManager *pLayoutManager = nullptr;
    const CRootVisual *pRoot = VisualTree::GetRootForElement(pObject);

    if (pRoot)
    {
        pLayoutManager = pRoot->GetAssociatedLayoutManager();
    }
    else if (pObject->GetContext())
    {
        pLayoutManager = pObject->GetContext()->GetMainLayoutManager();
    }

    return pLayoutManager;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static helper function that encapsulates getting the CPopupRoot
//      that is associated with the provided object. If the object is not
//      contained within a VisualTree's tree then this method checks if
//      the object is a CPopup, or if the root of its visual tree is the
//      logical child of a CPopup. If so, it gets the CPopupRoot associated
//      with that CPopup. Otherwise, this method falls back to returning
//      the CPopupRoot of the core's main VisuaTree.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
VisualTree::GetPopupRootForElementNoRef(
    _In_ CDependencyObject *pObject,
    _Outptr_result_maybenull_ CPopupRoot **ppPopupRoot)
{
    IFCPTR_RETURN(ppPopupRoot);
    IFCPTR_RETURN(pObject);

    CCoreServices* core = pObject->GetContext();

    CPopup* popup = do_pointer_cast<CPopup>(pObject);
    if (popup)
    {
        // The PopupRoot may be disconnected from its parent by this point.
        if (CUIElement* child = popup->GetChildNoRef())
        {
            if (CPopupRoot* popupRoot = do_pointer_cast<CPopupRoot>(child->GetParentInternal(false /*publicParentOnly*/)))
            {
                *ppPopupRoot = popupRoot;
                return S_OK;
            }
        }
    }

    if (VisualTree* visualTree = GetForElementNoRef(pObject))
    {
        if (CPopupRoot* popupRoot = visualTree->GetPopupRoot())
        {
            *ppPopupRoot = popupRoot;
            return S_OK;
        }
    }

    if (popup)
    {
        // If this is an unparented popup, then we don't have a parent, and there's no ancestor chain leading up to a
        // popup root. We can try one last thing - since the popup is open, some popup root knows about it. Walk every
        // XamlIslandRoot and check whether its popup root contains this open popup.
        VisualTree* mainVisualTree = core->GetMainVisualTree();

        if (mainVisualTree != nullptr)
        {
            CXamlIslandRootCollection* xamlIslandRootCollection = mainVisualTree->GetXamlIslandRootCollection();

            if (xamlIslandRootCollection != nullptr)
            {
                CDOCollection* xamlIslandRoots = xamlIslandRootCollection->GetChildren();

                if (xamlIslandRoots != nullptr)
                {
                    for (CDependencyObject* xamlIslandRootDO : (*xamlIslandRoots))
                    {
                        CXamlIslandRoot* xamlIslandRoot = do_pointer_cast<CXamlIslandRoot>(xamlIslandRootDO);
                        ASSERT(xamlIslandRoot != nullptr);

                        CPopupRoot* popupRoot = xamlIslandRoot->GetPopupRootNoRef();

                        if (popupRoot != nullptr && (popupRoot->ContainsOpenOrUnloadingPopup(popup)))
                        {
                            *ppPopupRoot = popupRoot;
                            return S_OK;
                        }
                    }
                }
            }
        }
    }

    // Note: This explicitly checks that an island exists. If all islands have been torn down then we don't actually go
    // into this block and instead we'll return the default "main" popup root.
    if (core->HasXamlIslands() && core->GetInitializationType() == InitializationType::IslandsOnly)
    {
        // We failed to find a XamlIslandRoot. Rather than defaulting to the CRootVisual's popup root, return null here.
        // We don't want to use the CRootVisual's popup root because the entire CRootVisual tree is non-live when Xaml islands
        // are involved. If we use that popup root, we'll walk up that subtree when rendering the popup, fail to find a comp
        // node, and crash. If we return a null popup root, then the popup will just not render.
        *ppPopupRoot = nullptr;
        return S_OK;
    }

    *ppPopupRoot = core->GetMainPopupRoot();

    return S_OK; // RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static helper function that encapsulates getting the CTransitionRoot
//      that is associated with the VisualTree containing the provided
//      object.
//
//------------------------------------------------------------------------
/*static*/ CTransitionRoot* VisualTree::GetTransitionRootForElement(_In_ CDependencyObject *pObject)
{
    CXamlIslandRoot* xamlIslandRootNoRef = GetXamlIslandRootForElement(pObject);
    if (xamlIslandRootNoRef)
    {
        CTransitionRoot* transitionRoot = xamlIslandRootNoRef->GetTransitionRootNoRef();
        if(transitionRoot)
        {
            return transitionRoot;
        }
    }

    const CRootVisual* pRootVisual = VisualTree::GetRootForElement(pObject);
    if (pRootVisual != nullptr)
    {
        VisualTree *pVisualTree = pRootVisual->GetAssociatedVisualTree();
        return pVisualTree->GetTransitionRoot();
    }

    return nullptr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static helper function that encapsulates getting the
//      CFullWindowMediaRoot that is associated with the VisualTree
//      containing the provided object.
//
//------------------------------------------------------------------------
/*static*/ CFullWindowMediaRoot* VisualTree::GetFullWindowMediaRootForElement(_In_ CDependencyObject *pObject)
{
    const CRootVisual* pRootVisual = VisualTree::GetRootForElement(pObject);
    if (pRootVisual != NULL)
    {
        VisualTree *pVisualTree = pRootVisual->GetAssociatedVisualTree();
        return pVisualTree->GetFullWindowMediaRoot();
    }

    return nullptr;
}

CContentRoot* VisualTree::GetContentRootForElement(_In_ CDependencyObject* object, LookupOptions options)
{
    if (VisualTree* visualTree = GetForElementNoRef(object, options))
    {
        return visualTree->GetContentRootNoRef();
    }

    if (CContentRoot* contentRoot = object->GetContext()->GetContentRootCoordinator()->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot())
    {
        return contentRoot;
    }

    return nullptr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Adds the given root to the implicit root visual, and potentially 'Enter' it into
//      the tree.
//
//  Notes:
//      A root is entered into the tree if we can retrieve a namescope owner for it
//      using GetNamescopeOwnerForRoot.
//
//      A precondition of this function is that the root being entered is already set
//      in its corresponding member variable (e.g. m_popupRoot, m_publicRootVisual, etc.)
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::AddRoot(_In_ CUIElement *pRoot)
{
    if (pRoot)
    {
        CDependencyObject *pNamescopeOwner = GetNamescopeOwnerForRoot(pRoot);
        const bool isXamlIslandRootCollection = (pRoot == m_xamlIslandRootCollection.get());

        // The XamlIslandRootCollection is not a part of any VisualTree -- it's parent
        // CRootVisual defines a VisualTree, and it's children, CXamlIslandRoots will
        // define their own VisualTrees.  During development, we've seen it's helpful
        // to make sure the CRootVisual VisualTree pointer doesn't propagate down -- so
        // we set the VisualTree param null here as we enter.

        EnterParams enterParams(
            /*isLive*/                TRUE,
            /*skipNameRegistration*/  TRUE,
            /*coercedIsEnabled*/      TRUE,
            /*useLayoutRounding*/     EnterParams::UseLayoutRoundingDefault,
            /*visualTree*/            isXamlIslandRootCollection ? nullptr : this
        );

        IFCEXPECT_ASSERT_RETURN(pRoot != nullptr);
        IFC_RETURN(m_rootElement->AddChild(pRoot));

        if (IsMainVisualTree() && pNamescopeOwner)
        {
            IFC_RETURN(pRoot->Enter(pNamescopeOwner, enterParams));
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Adds the visual root to the root ScrollViewer
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::AddVisualRootToRootScrollViewer(_In_ CUIElement *pVisualRoot)
{
    CUIElement *pContentElement = nullptr;
    CValue cValue;

    IFCEXPECT_ASSERT_RETURN(pVisualRoot != nullptr);
    IFCEXPECT_ASSERT_RETURN(m_rootScrollViewer != nullptr);

    if (m_rootContentPresenter)
    {
        // Set the root ContentPresenter as the child of RootScrollViewer.
        IFC_RETURN(m_rootScrollViewer->AddChild(m_rootContentPresenter.get()));

        // Set the root ScrollContentPresenter's content property that is the content of RootScrollViewer.
        ASSERT(m_rootScrollViewer->m_content.AsObject());
        cValue.WrapObjectNoRef(m_rootScrollViewer->m_content.AsObject());
        IFC_RETURN(m_rootContentPresenter->SetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_Content, cValue));

        // Set the templated parent to the content presenter with the RootSV.
        IFC_RETURN(m_rootContentPresenter->SetTemplatedParent(m_rootScrollViewer.get()));
    }

    pContentElement = do_pointer_cast<CUIElement>(m_rootScrollViewer->m_content.AsObject());
    IFCEXPECT_ASSERT_RETURN(pContentElement != nullptr);

    if (m_rootContentPresenter)
    {
        // Add the root ScrollViewer's content to the root ScrollContentPresenter.
        IFC_RETURN(m_rootContentPresenter->AddChild(pContentElement));
    }

    // *** Notes on Public Root as Projected Shadow Receiver ***
    // Xaml's ThemeShadow design flattens the public root into a single shadow receiver. This requires a Visual from the public root's compnode.
    // There are two possibilities for public root:
    //   1. RootScrollViewer-hosted content (normal case), and
    //      Here we make use of the Border element hosting the actual public root in the RSV. We use this instead of the public root itself since
    //      the border already has a compnode due to being a DManip target - this avoids an extra compnode for shadows in the common case.
    //      For completeness and to address potential corner cases, mark the border as requiring compnode for shadows while it is in the tree.
    //   2. Canvas root (legacy design)
    //      This comes up if Window.Content is set to a Canvas - we don't create an RSV and parent the Canvas directly to CRootVisual
    //      In this case, we require a compnode on the Canvas as a projected shadow receiver.

    // Require compnode for RSV-based public root (specifically the Border containing the content) as shadow receiver
    if (!pContentElement->OfTypeByIndex<KnownTypeIndex::Canvas>())
    {
        IFC_RETURN(pContentElement->SetRequiresComposition(CompositionRequirement::ProjectedShadowDefaultReceiver, IndependentAnimationType::None));
    }

    // Add the visual root as the child of the root ScrollViewer's content
    IFC_RETURN(pContentElement->AddChild(pVisualRoot));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Remove the visual root from the root ScrollViewer
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::RemoveVisualRootFromRootScrollViewer(_In_ CUIElement *pVisualRoot)
{
    CUIElement *pContentElement = nullptr;

    IFCEXPECT_ASSERT_RETURN(pVisualRoot != nullptr);
    IFCEXPECT_ASSERT_RETURN(m_rootScrollViewer != nullptr);

    pContentElement = do_pointer_cast<CUIElement>(m_rootScrollViewer->m_content.AsObject());
    IFCEXPECT_ASSERT_RETURN(pContentElement != NULL);

    if (m_rootContentPresenter)
    {
        // Remove the root content presenter from the root ScrollViewer.
        IFC_RETURN(m_rootScrollViewer->RemoveChild(m_rootContentPresenter.get()));

        // Unset compnode requirement as shadow receiver for for RSV-based public root (specifically the Border containing the content)
        if (!pContentElement->OfTypeByIndex<KnownTypeIndex::Canvas>())
        {
            pContentElement->UnsetRequiresComposition(CompositionRequirement::ProjectedShadowDefaultReceiver, IndependentAnimationType::None);
        }

        // Remove the content element from the root content presenter.
        IFC_RETURN(m_rootContentPresenter->RemoveChild(pContentElement));
    }

    // Remove the visual root from the root ScrollViewer's content
    IFC_RETURN(pContentElement->RemoveChild(pVisualRoot));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the given root from the implicit root visual, and potentially 'Leave' it
//      from the tree.
//
//  Notes:
//      A root is leaves the tree if we can retrieve a namescope owner for it
//      using GetNamescopeOwnerForRoot.
//
//      A precondition of this function is that the root being entered is already set
//      in its corresponding member variable (e.g. m_popupRoot, m_publicRootVisual, etc.)
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::RemoveRoot(_In_ CUIElement *pRoot)
{
    CUIElement *pPublicRoot = GetPublicRootVisual();
    CDependencyObject *pNamescopeOwner = GetNamescopeOwnerForRoot(pRoot);

    LeaveParams leaveParams(
        /*fIsLive*/               pPublicRoot? pPublicRoot->IsActive() : FALSE,
        /*fSkipNameRegistration*/ TRUE,
        /*fCoercedIsEnabled*/     TRUE,
        /*fVisualTreeBeingReset*/ TRUE
    );

    if (pNamescopeOwner)
    {
        IFC_RETURN(pRoot->Leave(pNamescopeOwner, leaveParams));
    }
    IFC_RETURN(m_rootElement->RemoveChild(pRoot));

    // Ensure that incremental PC render data is cleaned up.
    // This would normally happen during a 'live' Leave, but the Leave call here doesn't always happen,
    // isn't always live, and there's no live Leave generated by the RemoveChild call either since m_rootVisual
    // itself is never live.
    pRoot->LeavePCSceneRecursive();

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Add the root ScrollViewer to the hidden RootVisual
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::AddRootScrollViewer(_In_ CScrollContentControl *pRootScrollViewer)
{
    IFCEXPECT_ASSERT_RETURN(pRootScrollViewer);
    IFC_RETURN(AddRoot(pRootScrollViewer));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Remove the visual root to the root ScrollViewer
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::RemoveRootScrollViewer(_In_ CScrollContentControl *pRootScrollViewer)
{
    LeaveParams leaveParams(
        /*fIsLive*/               pRootScrollViewer ? pRootScrollViewer->IsActive() : FALSE,
        /*fSkipNameRegistration*/ TRUE,
        /*fCoercedIsEnabled*/     TRUE,
        /*fVisualTreeBeingReset*/ TRUE
    );
    IFCEXPECT_ASSERT_RETURN(pRootScrollViewer != nullptr);
    IFC_RETURN(pRootScrollViewer->Leave(pRootScrollViewer, leaveParams));
    IFC_RETURN(m_rootElement->RemoveChild(pRootScrollViewer));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the namescope owner to use for Enter/Leave calls when adding/removing
//      the given root.
//
//  Returns:
//      The namescope owner, or NULL if Enter/Leave should be skipped on AddRoot/RemoveRoot.
//
//  Notes:
//      A precondition of this function is that the root being entered is already set
//      in its corresponding member variable (e.g. m_popupRoot, m_publicRootVisual, etc.)
//
//---------------------------------------------------------------------------
CUIElement* VisualTree::GetNamescopeOwnerForRoot(_In_ CUIElement *pRoot)
{
    // The following roots use themselves as namescope owners
    if (pRoot == m_printRoot.get())
    {
        return pRoot;
    }

    // The rest use the public root visual
    return m_publicRootVisual.get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resets the roots of this visual tree by removing them (and potentially calling
//      Leave. See RemoveRoot).
//
//  Notes:
//      If provided, pReleasePopup will be set TRUE if a popup root was encountered.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT VisualTree::ResetRoots( _Out_opt_ bool *pReleasePopup)
{
    HRESULT recordHr = S_OK;

    if (pReleasePopup)
    {
        *pReleasePopup = (m_popupRoot != nullptr);
    }

    if (m_connectedAnimationRoot != nullptr)
    {
        RECORDFAILURE(RemoveRoot(m_connectedAnimationRoot.get()));
    }

    if (m_visualDiagnosticsRoot)
    {
        RECORDFAILURE(RemoveRoot(m_visualDiagnosticsRoot.get()));
    }

    if (m_popupRoot && m_rootElement)
    {
        // The popup root does not always get cleared so it needs to be checked in the collection before attempting to remove
        CUIElementCollection* pCollection = static_cast<CUIElementCollection*>(m_rootElement->GetChildren());
        if (pCollection)
        {
            XINT32 iIndex = -1;
            IFC_RETURN(pCollection->IndexOf(m_popupRoot, &iIndex));
            if (iIndex >= 0)
            {
                RECORDFAILURE(RemoveRoot(m_popupRoot.get()));
            }
        }
    }

    if (m_printRoot)
    {
        RECORDFAILURE(RemoveRoot(m_printRoot.get()));
    }

    if (m_transitionRoot)
    {
        RECORDFAILURE(RemoveRoot(m_transitionRoot.get()));
    }

    if (m_fullWindowMediaRoot)
    {
        RECORDFAILURE(RemoveRoot(m_fullWindowMediaRoot.get()));
    }

    if (m_renderTargetBitmapRoot)
    {
        RECORDFAILURE(RemoveRoot(m_renderTargetBitmapRoot.get()));
    }

    if (m_xamlIslandRootCollection)
    {
        RECORDFAILURE(RemoveRoot(m_xamlIslandRootCollection.get()));
    }

    if (m_publicRootVisual)
    {
        // ToolTipService attaches handlers to the public root of the main window and each Xaml island. Clean up its
        // bookkeeping now that the root is going away.
        DirectUI::ToolTipService::OnPublicRootRemoved(m_publicRootVisual.get());
    }

    if (m_rootScrollViewer && m_bIsRootScrollViewerAddedToRoot)
    {
        // Remove both root ScrollViewer and visual root from the tree
        RECORDFAILURE(RemoveRootScrollViewer(m_rootScrollViewer.get()));
        RECORDFAILURE(RemoveVisualRootFromRootScrollViewer(m_publicRootVisual.get()));

        // The public visual root is always released immediately.
        // But we keep the root ScrollViewer reference to reuse it
        // for new public visual root.
        m_publicRootVisual.reset();

        m_bIsRootScrollViewerAddedToRoot = false;
    }
    else
    {
        // Public root visual is always removed last
        if (m_publicRootVisual)
        {
            RECORDFAILURE(RemoveRoot(m_publicRootVisual.get()));

            // The public root visual is always released, regardless of 'resetRoots'.
            m_publicRootVisual.reset();
        }
    }

    IFC_RETURN(recordHr);

    return S_OK;
}

void VisualTree::GetAllVisibleRootsNoRef(_Out_writes_(3) CDependencyObject** roots)
{
    auto contentRoot = GetContentRootNoRef();
    if (auto xamlIslandRoot = contentRoot->GetXamlIslandRootNoRef())
    {
        roots[0] = xamlIslandRoot;
    }
    else
    {
        roots[0] = GetRootVisual();
    }
    roots[1] = GetPopupRoot();
    roots[2] = GetFullWindowMediaRoot();
}

CDependencyObject* VisualTree::GetActiveRootVisual() const
{
    CDependencyObject* pCandidateRoot = m_fullWindowMediaRoot.get();
    if (pCandidateRoot != nullptr)
    {
        if (static_cast<CUIElement*>(pCandidateRoot)->IsCollapsed())
        {
            pCandidateRoot = nullptr;
        }
    }

    if (pCandidateRoot == nullptr)
    {
        pCandidateRoot = this->GetPublicRootVisual();
    }

    return pCandidateRoot;
}

bool VisualTree::IsBehindFullWindowMediaRoot(_In_opt_ const CDependencyObject* const pDO) const
{
    if (pDO == nullptr)
    {
        return false;
    }

    CDependencyObject *pActiveRoot = this->GetActiveRootVisual();
    if (pActiveRoot == nullptr)
    {
        return false;
    }

    if (pActiveRoot == m_fullWindowMediaRoot.get())
    {
        CDependencyObject *pPublicRoot = this->GetPublicRootVisual();
        const CDependencyObject *pParent = pDO;
        while (pParent)
        {
            if (pParent == pPublicRoot)
            {
                return true;
            }
            else if (pParent == pActiveRoot)
            {
                return false;
            }
            pParent = pParent->GetParent();
        }
    }

    return false;
}

CXamlIslandRoot* VisualTree::GetXamlIslandRootForElement(_In_opt_ CDependencyObject* pObject)
{
    if (!pObject || !pObject->GetContext()->HasXamlIslands())
    {
        return nullptr;
    }
    if (VisualTree* visualTree = GetForElementNoRef(pObject))
    {
        return do_pointer_cast<CXamlIslandRoot>(visualTree->m_rootElement);
    }
    return nullptr;
}

#if DBG
// Walk up the visual tree and ensure that all the parent nodes are part of the expected VisualTree
void DebugValidateVisualTree(_In_ CDependencyObject* element, _In_ VisualTree* expectedVisualTree)
{
    if (element->GetTypeIndex() == KnownTypeIndex::XamlIsland)
    {
        return;
    }
    else if (element->GetTypeIndex() == KnownTypeIndex::RootVisual)
    {
        return;
    }

    if (VisualTree* visualTree = element->GetVisualTree())
    {
        FAIL_FAST_ASSERT(visualTree == expectedVisualTree);
    }

    if (auto candidate = element->GetParentInternal(false))
    {
        DebugValidateVisualTree(candidate, expectedVisualTree);
    }

    if (CPopup* popup = do_pointer_cast<CPopup>(element))
    {
        if (CDependencyObject* owner = popup->GetToolTipOwner())
        {
            DebugValidateVisualTree(owner, expectedVisualTree);
        }

        if (CFlyoutBase* flyout = popup->GetAssociatedFlyoutNoRef())
        {
            DebugValidateVisualTree(flyout, expectedVisualTree);
        }

        if (popup->IsOpen())
        {
            if (CUIElement* popupChild = popup->GetChildNoRef())
            {
                DebugValidateVisualTree(popupChild, expectedVisualTree);
            }
        }
    }

}
#endif

/*static*/ VisualTree* VisualTree::GetForElementNoRef(_In_opt_ CDependencyObject* element, LookupOptions options)
{
    if (!element)
    {
        return nullptr;
    }

    if (VisualTree* visualTree = element->GetVisualTree())
    {
        return visualTree;
    }

    VisualTree* result = GetVisualTreeViaTreeWalkNoRef(element, options);
    if (result)
    {
        // We found the right VisualTree -- we might as well remember it!
        element->SetVisualTree(result);
    }
#if DBG
    // http://osgvsowi/19548424: Enable this VisualTree validator and debug the problems.
    // DebugValidateVisualTree(element, result);
#endif
    return result;
}

/*static*/ VisualTree* VisualTree::GetVisualTreeViaTreeWalkNoRef(_In_ CDependencyObject* element, LookupOptions options)
{
    CDependencyObject* currentAncestor = element;
    VisualTree* result = nullptr;

    // We cap the tree walk because we've found at least one case (Skype app -- 19h1) where there's a cycle in the tree.
    int iterationsLeft = 1000;

    // Walk up the tree to find either the XamlIslandRoot or RootVisual
    while (currentAncestor && iterationsLeft != 0)
    {
        --iterationsLeft;
        if (VisualTree* visualTree = currentAncestor->GetVisualTree())
        {
            return visualTree;
        }

        if (currentAncestor->GetTypeIndex() == KnownTypeIndex::XamlIsland)
        {
            return static_cast<CXamlIslandRoot*>(currentAncestor)->GetVisualTreeNoRef();
        }
        else if (currentAncestor->GetTypeIndex() == KnownTypeIndex::RootVisual)
        {
            return static_cast<CRootVisual*>(currentAncestor)->GetAssociatedVisualTree();
        }

        CDependencyObject* nextAncestor = nullptr;
        if (currentAncestor->DoesAllowMultipleAssociation() && currentAncestor->GetParentCount() > 1)
        {
            // We cannot travese up a tree through a multiply associated element.  Our goal is to support DOs being
            // shared between XAML trees.  We've seen cases where we traverse up the tree through CSetter objects,
            // so for now we allow the traversal if there's one unique parent.  TODO: This could be fragile?  Allowing
            // the traversal to happen when the parent count is 1 means that if this element gets another parent later,
            // we're now in an inconsistent state.
            // Bug 19548424: Investigate places where an element entering the tree doesn't have a unique VisualTree ptr
        }
        else
        {
            nextAncestor = currentAncestor->GetParentInternal(false /* public parent only */);
        }

        //
        // We have a few tricks to figure out which VisualTree an element may be associated with.
        // There is now a cached weak VisualTree pointer on each DO that we update when we do a live
        // enter, so we may be able to remove some of these lookups.  Let's investigate this with bug 19548424.
        //

        if (CPopup* popup = do_pointer_cast<CPopup>(currentAncestor))
        {
            if (CDependencyObject* owner = popup->GetToolTipOwner())
            {
                nextAncestor = owner;
            }

            if (CFlyoutBase* flyout = popup->GetAssociatedFlyoutNoRef())
            {
                if (VisualTree* visualTree = VisualTree::GetForElementNoRef(flyout))
                {
                    return visualTree;
                }
            }

            if (nextAncestor == nullptr && popup->IsOpen())
            {
                // If the popup is open, its child will be parented to a PopupRoot.  We
                // can quickly fine the root from there.
                CUIElement* popupChild = popup->GetChildNoRef();
                if (popupChild)
                {
                    nextAncestor = popupChild->GetParentInternal(false /* public parent only */);
                }
            }
        }

        if (nextAncestor == nullptr)
        {
            if (CCollection* collection = do_pointer_cast<CCollection>(currentAncestor))
            {
                nextAncestor = collection->GetOwner();
            }
        }

        // If the next visual parent is null, check the logical parent. Popups are connected to their child with
        // a logical link. Only allow the walk up if the logical parent is a popup.
        if (nextAncestor == nullptr)
        {
            if (CUIElement* currentUIE = do_pointer_cast<CUIElement>(currentAncestor))
            {
                CPopup* nextAncestorPopup = do_pointer_cast<CPopup>(currentUIE->GetLogicalParentNoRef());
                nextAncestor = nextAncestorPopup;
            }
        }

        // If this is a hyperlink's automation peer, check if the owner is in the tree.
        if (nextAncestor == nullptr)
        {
            if (CHyperlinkAutomationPeer* currentHyperlinkAP = do_pointer_cast<CHyperlinkAutomationPeer>(currentAncestor))
            {
                CHyperlink* currentHyperlink = do_pointer_cast<CHyperlink>(currentHyperlinkAP->GetDONoRef());
                nextAncestor = currentHyperlink;
            }
        }

        currentAncestor = nextAncestor;
    }
    if (!result)
    {
        if (options == LookupOptions::WarningIfNotFound)
        {
            // We didn't find anything
            VisualTreeNotFoundWarning();
        }
    }
    return result;
}

/*static*/ CUIElement* VisualTree::GetRootOrIslandForElement(_In_ CDependencyObject* element)
{
    CUIElement* root = GetXamlIslandRootForElement(element);
    if (root == nullptr)
    {
        return GetRootForElement(element);
    }

    return root;
}

/*static*/ bool VisualTree::IsElementInWindowedPopup(_In_ CUIElement* element)
{
    xref_ptr<CPopup> popup;
    IFCFAILFAST(CPopupRoot::GetOpenPopupForElement(element, popup.ReleaseAndGetAddressOf()));
    return popup && popup->IsWindowed();
}


xref_ptr<CLayoutTransitionElement> VisualTree::AddTestLTE(
    _In_ CUIElement *lteTarget,
    _In_ CUIElement *lteParent,
    bool parentIsRootVisual,
    bool parentIsPopupRoot,
    bool isAbsolutelyPositioned)
{
    CTransitionRoot* transitionRootNoRef;
    if (parentIsRootVisual)
    {
        IFCFAILFAST(EnsureTransitionRoot());
        transitionRootNoRef = m_transitionRoot;
    }
    else if (parentIsPopupRoot)
    {
        IFCFAILFAST(EnsurePopupRoot());
        transitionRootNoRef = m_popupRoot->GetLocalTransitionRoot(true);
    }
    else
    {
        transitionRootNoRef = lteParent->GetLocalTransitionRoot(true);
    }

    xref_ptr<CLayoutTransitionElement> lte;
    IFCFAILFAST(CLayoutTransitionElement::Create(
        lteTarget,
        isAbsolutelyPositioned,
        lte.ReleaseAndGetAddressOf()));
    lte->AttachTransition(transitionRootNoRef);

    m_testLTEs.push_back(lte);

    return lte;
}

void VisualTree::RemoveTestLTE(_In_ CUIElement *lte)
{
    CLayoutTransitionElement* lteNoRef = static_cast<CLayoutTransitionElement*>(lte);
    for (unsigned int i = 0; i < m_testLTEs.size(); i++)
    {
        if (m_testLTEs[i] == lteNoRef)
        {
            lteNoRef->DetachTransition();
            m_testLTEs.erase(m_testLTEs.begin() + i);
            break;
        }
    }
}

void VisualTree::ClearTestLTEs()
{
    for (xref_ptr<CLayoutTransitionElement> lte : m_testLTEs)
    {
        lte->DetachTransition();
    }
    m_testLTEs.clear();
}

void VisualTree::CloseAllPopupsForTreeReset()
{
    if (m_popupRoot)
    {
        m_popupRoot->CloseAllPopupsForTreeReset();
    }
}

IInspectable* VisualTree::GetOrCreateXamlRootNoRef()
{
    if (m_xamlRoot == nullptr)
    {
        m_xamlRoot.Attach(FxCallbacks::XamlRoot_Create(this));
        // Although we hold a reference to the xamlroot here, the object is not being pegged like we expect
        // and we must peg it explicitly here.
        FxCallbacks::XamlRoot_UpdatePeg(m_xamlRoot.Get(), true);
    }
    return GetXamlRootNoRef();
}

IInspectable* VisualTree::GetXamlRootNoRef() const
{
    return m_xamlRoot.Get();
}

RootScale* VisualTree::GetRootScale() const
{
    return m_rootScale.get();
}

double VisualTree::GetRasterizationScale() const
{
    if (auto rootScale = GetRootScale())
    {
        return rootScale->GetEffectiveRasterizationScale();
    }
    else
    {
        return 1.0f;
    }
}

wf::Size VisualTree::GetSize() const
{
    if (m_isShutdown || m_shutdownInProgress)
    {
        // In a shutdown state, return (0,0) as a default.
        return {0.0f, 0.0f};
    }
    else if (CXamlIslandRoot* xamlIsland = do_pointer_cast<CXamlIslandRoot>(m_rootElement))
    {
        return xamlIsland->GetSize();
    }
    else if (CRootVisual* rootVisual = do_pointer_cast<CRootVisual>(m_rootElement))
    {
        XSIZE_LOGICAL logicalSize = {};
        rootVisual->GetContext()->GetWindowSize(rootVisual, nullptr /*physicalSize*/, &logicalSize);
        return { static_cast<float>(logicalSize.Width), static_cast<float>(logicalSize.Height) };
    }

    XAML_FAIL_FAST();
    return {};
}

bool VisualTree::GetIsVisible() const
{
    if (m_isShutdown || m_shutdownInProgress)
    {
        // In a shutdown state, return false as a default.
        return false;
    }
    else if (CXamlIslandRoot* xamlIsland = do_pointer_cast<CXamlIslandRoot>(m_rootElement))
    {
        return xamlIsland->IsVisible();
    }
    else if (CRootVisual* rootVisual = do_pointer_cast<CRootVisual>(m_rootElement))
    {
        return rootVisual->GetContext()->IsXamlVisible();
    }

    XAML_FAIL_FAST();
    return false;
}

wf::Rect VisualTree::GetVisibleBounds() const
{
    // TODO: Do we want to move the DXamlCore_GetVisibleContentBoundsForElement logic to here?
    wf::Rect visibleBounds = {};
    if (m_rootElement)
    {
        IFCFAILFAST(FxCallbacks::DXamlCore_GetVisibleContentBoundsForElement(m_rootElement, &visibleBounds));
    }
    return visibleBounds;
}

POINT VisualTree::GetScreenOffset() const
{
    if (m_isShutdown || m_shutdownInProgress)
    {
        // In a shutdown state, return (0,0) as a default.
        return {0, 0};
    }
    if (CXamlIslandRoot* xamlIsland = do_pointer_cast<CXamlIslandRoot>(m_rootElement))
    {
        return xamlIsland->GetScreenOffset();
    }
    else if (CRootVisual* rootVisual = do_pointer_cast<CRootVisual>(m_rootElement))
    {
        XamlOneCoreTransforms::FailFastIfEnabled(); // Due to GetWindowRect

        const HWND hwndJupiter = static_cast<HWND>(rootVisual->GetContext()->GetHostSite()->GetXcpControlWindow());

        RECT jupiterWindowRect = {};
        ::GetWindowRect(hwndJupiter, &jupiterWindowRect);
        return { jupiterWindowRect.left, jupiterWindowRect.top };
    }

    XAML_FAIL_FAST();
    return {};
}

POINT VisualTree::ClientLogicalToScreenPhysical(const wf::Point& pt) const
{
    const POINT screenOffset = GetScreenOffset();
    const float scale = static_cast<float>(GetRasterizationScale());
    return {
        lround(static_cast<float>(screenOffset.x) + (pt.X * scale)),
        lround(static_cast<float>(screenOffset.y) + (pt.Y * scale))};
}

wf::Point VisualTree::ScreenPhysicalToClientLogical(const POINT& pt) const
{
    const POINT screenOffset = GetScreenOffset();
    const float scale = static_cast<float>(GetRasterizationScale());
    return {
        static_cast<float>(pt.x - screenOffset.x) / scale,
        static_cast<float>(pt.y - screenOffset.y) / scale};
}

// Helper function for types like Popup, Flyout, and ContentDialog to figure out which VisualTree
// it's attached to.  If there is no clear, unambiguous answer, this function returns an error.
_Check_return_ HRESULT VisualTree::GetUniqueVisualTreeNoRef(
    _In_ CDependencyObject* element,
    _In_opt_ CDependencyObject* positionReferenceElement,
    _In_opt_ VisualTree* explicitTree,
    _Outptr_opt_ VisualTree** uniqueVisualTree)
{
    VisualTree* result = nullptr;

    result = VisualTree::GetForElementNoRef(element);
    if (!result)
    {
        // For a flyout that's on a button via e.g. Button.Flyout, we need to follow the mentor
        // pointer to the Button and find the tree it belongs to.
        result = VisualTree::GetForElementNoRef(element->GetMentor());
    }

    if (positionReferenceElement)
    {
        VisualTree* referenceTree = VisualTree::GetForElementNoRef(positionReferenceElement);
        if (!result)
        {
            result = referenceTree;
        }
        else if (result != referenceTree)
        {
            IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_XAMLROOT_AMBIGUOUS));
        }
    }

    if (explicitTree)
    {
        if (!result)
        {
            result = explicitTree;
        }
        else if (result != explicitTree)
        {
            IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_XAMLROOT_AMBIGUOUS));
        }
    }

    if (!result)
    {
        CCoreServices* core = element->GetContext();
        if (core->GetInitializationType() == InitializationType::IslandsOnly)
        {
            // If we started in an "islands-only way" (e.g., DesktopWindowXamlSource) there in no XAML content
            // attached to the main window.  In this case, we can't fall back to the main visual tree -- this is
            // an error.
            IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_XAMLROOT_AMBIGUOUS));
        }
        // The answer is unclear, so fall back to the MainVisualTree (the XAML content on the CoreWindow)
        result = core->GetMainVisualTree();
    }

    if (uniqueVisualTree)
    {
        *uniqueVisualTree = result;
    }
    return S_OK;
}

_Check_return_ HRESULT VisualTree::AttachElement(_In_ CDependencyObject* element)
{
    VisualTree* uniqueTree = nullptr;

    IFC_RETURN(VisualTree::GetUniqueVisualTreeNoRef(
        element,
        nullptr /*positionReferenceElement*/,
        this /*explicitTree*/,
        &uniqueTree));

    IFCEXPECT_RETURN(this == uniqueTree);
    element->SetVisualTree(this);

    return S_OK;
}

xref::details::control_block* VisualTree::EnsureControlBlock()
{
    return m_coreContentRoot.EnsureControlBlock();
}
