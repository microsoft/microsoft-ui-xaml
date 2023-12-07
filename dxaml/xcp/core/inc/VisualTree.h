// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      VisualTree is a class that encapsulates a single visual tree.

#pragma once

class CCoreServices;
class CRootVisual;
class CPopupRoot;
class CUIElement;
class CTransitionRoot;
class CPrintRoot;
class CConnectedAnimationRoot;
class CInputServices;
class CInputManager;
class CFocusManager;
class CLayoutManager;
class CScrollContentControl;
class CContentPresenter;
class CFullWindowMediaRoot;
class CRenderTargetBitmapRoot;
class CGrid;
class CXamlIslandRoot;
class CXamlIslandRootCollection;
class CLayoutTransitionElement;
class IXamlRoot;
class CContentRoot;
class RootScale;
class QualifierContext;

enum class LookupOptions
{
    // Normal lookup
    NoFallback = 0,
    WarningIfNotFound = 1,
    // In the future we may want to add options for falling back to the "main" VisualTree
};

//------------------------------------------------------------------------
//
//  VisualTree contains a single visual tree and is the primary interface
//  for other components to interact with the tree.
//
//------------------------------------------------------------------------
class VisualTree final
{
public:
    XUINT32 AddRef();
    XUINT32 Release();

    _Check_return_ CPopupRoot* GetPopupRoot();
    _Check_return_ CGrid* GetVisualDiagnosticsRoot();
    _Check_return_ CRootVisual *GetRootVisual() const { return m_rootVisual.get(); }
    _Check_return_ CUIElement* GetRootElementNoRef() const { return m_rootElement; }
    _Check_return_ CPrintRoot *GetPrintRoot();
    _Check_return_ CTransitionRoot *GetTransitionRoot();
    _Check_return_ CFullWindowMediaRoot* GetFullWindowMediaRoot();
    _Check_return_ CConnectedAnimationRoot* GetConnectedAnimationRoot();
    _Check_return_ CRenderTargetBitmapRoot *GetRenderTargetBitmapRoot();
    _Check_return_ CUIElement *GetPublicRootVisual() const { return m_publicRootVisual.get(); }
    _Check_return_ CScrollContentControl *GetRootScrollViewer() const { return m_rootScrollViewer.get(); }
    _Check_return_ CLayoutManager *GetLayoutManager() const { return m_layoutManager.get();}
     CXamlIslandRootCollection* GetXamlIslandRootCollection();
     void GetAllVisibleRootsNoRef(_Out_writes_(3) CDependencyObject** roots);

    _Check_return_ HRESULT ResetRoots(_Out_opt_ bool *pReleasePopup = NULL);
    _Check_return_ HRESULT Shutdown();
    _Check_return_ HRESULT SetPublicRootVisual(_In_opt_ CUIElement* pRoot, _In_opt_ CScrollContentControl *pRootScrollViewer, _In_opt_ CContentPresenter *pRootContentPresenter);

    CContentRoot* GetContentRootNoRef();

    CDependencyObject* GetActiveRootVisual() const;
    bool IsBehindFullWindowMediaRoot(_In_opt_ const CDependencyObject* const pDO) const;

    RootScale* GetRootScale() const;

    // http://osgvsowi/17002792: Allow XAML apps to also have XamlIslandRoots (part 2)
    // Remove VisualTree::GetRootOrIslandForElement, and make VisualTree::GetRootForElement understand islands
    static VisualTree* GetForElementNoRef(_In_opt_ CDependencyObject* element, LookupOptions options = LookupOptions::WarningIfNotFound);
    static CUIElement* GetRootOrIslandForElement(_In_ CDependencyObject* element);
    static CRootVisual* GetRootForElement(_In_ const CDependencyObject* pObject);
    static CFocusManager* GetFocusManagerForElement(_In_ CDependencyObject *pObject, LookupOptions options = LookupOptions::WarningIfNotFound);
    static CInputManager* GetInputManagerForElement(_In_ CDependencyObject *pObject);
    static _Ret_maybenull_ CLayoutManager* GetLayoutManagerForElement(_In_ CDependencyObject *pObject);
    static _Check_return_ HRESULT GetPopupRootForElementNoRef(_In_ CDependencyObject *pObject, _Outptr_result_maybenull_ CPopupRoot **pPopupRoot);
    static CTransitionRoot* GetTransitionRootForElement(_In_ CDependencyObject *pObject);
    static CFullWindowMediaRoot* GetFullWindowMediaRootForElement(_In_ CDependencyObject *pObject);
    static CXamlIslandRoot* GetXamlIslandRootForElement(_In_opt_ CDependencyObject *pObject);
    static bool IsElementInWindowedPopup(_In_ CUIElement* element);
    static CContentRoot* GetContentRootForElement( _In_ CDependencyObject* object, LookupOptions options = LookupOptions::WarningIfNotFound );

    static _Check_return_ HRESULT GetUniqueVisualTreeNoRef(
        _In_ CDependencyObject* element,
        _In_opt_ CDependencyObject* positionReferenceElement,
        _In_opt_ VisualTree* explicitTree,
        _Outptr_opt_ VisualTree** uniqueContentRoot);

    _Check_return_ HRESULT AttachElement(_In_ CDependencyObject* element);

    xref_ptr<CLayoutTransitionElement> AddTestLTE(
        _In_ CUIElement *lteTarget,
        _In_ CUIElement *lteParent,
        bool parentIsRootVisual,
        bool parentIsPopupRoot,
        bool isAbsolutelyPositioned);

    void RemoveTestLTE(_In_ CUIElement *lte);

    void ClearTestLTEs();

    void CloseAllPopupsForTreeReset();

    IInspectable* GetOrCreateXamlRootNoRef();
    IInspectable* GetXamlRootNoRef() const;

    // Utility functions for understanding the context in which the VisualTree is running
    // Nany interact pretty closely with the XamlRoot public API
    double GetRasterizationScale() const;
    wf::Size GetSize() const;
    bool GetIsVisible() const;
    wf::Rect GetVisibleBounds() const;

    POINT GetScreenOffset() const;

    std::shared_ptr<QualifierContext> GetQualifierContext();

    POINT ClientLogicalToScreenPhysical(const wf::Point& pt) const;
    wf::Point ScreenPhysicalToClientLogical(const POINT& pt) const;

    VisualTree(_In_ CCoreServices *pCore, _In_ XUINT32 backgroundColor, _In_opt_ CUIElement* rootElement, _In_ CContentRoot& coreContentRoot);
    virtual ~VisualTree();

    xref::details::control_block* EnsureControlBlock();

private:
    static VisualTree* GetVisualTreeViaTreeWalkNoRef(_In_ CDependencyObject* element, LookupOptions options = LookupOptions::WarningIfNotFound);

    _Check_return_ HRESULT EnsurePopupRoot();
    _Check_return_ HRESULT EnsureVisualDiagnosticsRoot();
    _Check_return_ HRESULT EnsurePrintRoot();
    _Check_return_ HRESULT EnsureTransitionRoot();
    _Check_return_ HRESULT EnsureFullWindowMediaRoot();
    _Check_return_ HRESULT EnsureRenderTargetBitmapRoot();
    _Check_return_ HRESULT EnsureConnectedAnimationRoot();
    void EnsureXamlIslandRootCollection();

    _Check_return_ HRESULT AddRoot(_In_ CUIElement *pRoot);
    _Check_return_ HRESULT RemoveRoot(_In_ CUIElement *pRoot);
    _Check_return_ HRESULT AddVisualRootToRootScrollViewer(_In_ CUIElement *pVisualRoot);
    _Check_return_ HRESULT RemoveVisualRootFromRootScrollViewer(_In_ CUIElement *pVisualRoot);
    _Check_return_ HRESULT AddRootScrollViewer(_In_ CScrollContentControl *pRootScrollViewer);
    _Check_return_ HRESULT RemoveRootScrollViewer(_In_ CScrollContentControl *pRootScrollViewer);

    CUIElement* GetNamescopeOwnerForRoot(_In_ CUIElement *pRoot);

    bool ShouldFinalShutdown() const;
    void FinalShutdown();

    XUINT32 m_ref = 1;

    CCoreServices* m_pCoreNoRef = nullptr;
    CContentRoot& m_coreContentRoot;

    // m_rootElement is the parent of the roots.  For XAML app window content, this is the m_rootVisual.
    // For XamlIslandRoot content, it's the CXamlIslandRoot.
    CUIElement* m_rootElement;
    xref_ptr<CRootVisual> m_rootVisual;
    xref_ptr<CPopupRoot> m_popupRoot;
    xref_ptr<CGrid> m_visualDiagnosticsRoot;
    xref_ptr<CUIElement> m_publicRootVisual;
    xref_ptr<CScrollContentControl> m_rootScrollViewer;
    xref_ptr<CContentPresenter> m_rootContentPresenter;
    xref_ptr<CPrintRoot> m_printRoot;
    xref_ptr<CTransitionRoot> m_transitionRoot;
    xref_ptr<CFullWindowMediaRoot> m_fullWindowMediaRoot;
    xref_ptr<CRenderTargetBitmapRoot> m_renderTargetBitmapRoot;
    xref_ptr<CConnectedAnimationRoot> m_connectedAnimationRoot;
    xref_ptr<CXamlIslandRootCollection> m_xamlIslandRootCollection;
    std::shared_ptr<CLayoutManager> m_layoutManager;
    std::shared_ptr<RootScale> m_rootScale;

    // A vector of LTEs created with a test hook. These should be cleaned up by the test with ClearTestLTEs.
    std::vector<xref_ptr<CLayoutTransitionElement>> m_testLTEs;

    // This is effectively the public API wrapper for this type
    wrl::ComPtr<IInspectable> m_xamlRoot;

    std::shared_ptr<QualifierContext> m_pQualifierContext{ nullptr };

    // True if the VisualTree is serving as the main visual tree for a XAML app.  Otherwise, it's for a XamlIslandRoot
    bool IsMainVisualTree() { return m_rootVisual != nullptr; }

    bool m_bIsRootScrollViewerAddedToRoot : 1;
    bool m_shutdownInProgress : 1;
    bool m_isShutdown : 1;
};
