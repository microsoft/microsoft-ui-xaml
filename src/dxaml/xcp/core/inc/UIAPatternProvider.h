// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//
//      UIAPatternProvider, default implementations of the pattern providers
//      Wraps and calls into managed code
//

namespace Automation
{
    class CValue;
}

class CUIAPatternProvider : public IUIAProvider
{
public:
    CUIAPatternProvider(_In_ CAutomationPeer *pAP) : m_cRef(1)
    {
        m_pUIAWrapper = NULL;
        m_pInteropObject = NULL;
        m_pAP = pAP;
        m_bInteropSameAsPeer = FALSE;
        ASSERT(m_pAP);
    }

    virtual ~CUIAPatternProvider()
    {
        if (m_pUIAWrapper)
        {
            m_pUIAWrapper->Invalidate();
        }

        // If we have an interop object, we need to un-peg it; it was pegged when
        // CAutomationPeer::GetPattern obtained it.

        if( m_pInteropObject != NULL )
        {
            m_pInteropObject->UnpegManagedPeerNoRef();
        }

        if(!m_bInteropSameAsPeer)
        {
            ReleaseInterface(m_pInteropObject);
        }
    }

    XUINT32 AddRef() final { return ++m_cRef; }

    XUINT32 Release() final
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

    IUIAWrapper *GetUIAWrapper() final;
    void SetUIAWrapper(IUIAWrapper* pUIAWrapper) final;

    void InvalidateUIAWrapper() final
    {
        m_pUIAWrapper = NULL;
    }

    void* GetPatternInterface() override
    {
        return (void*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIInvoke;
    }

protected:
    static _Check_return_ HRESULT UnboxArrayOfStrings(_In_ const Automation::CValue &source, _Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) xstring_ptr** ppstrRetVal);

public:
    CDependencyObject *m_pInteropObject;
    bool m_bInteropSameAsPeer;
protected:
    IUIAWrapper *m_pUIAWrapper;
    XUINT32 m_cRef;
    CAutomationPeer *m_pAP;
};

class CManagedInvokeProvider : public CUIAPatternProvider, public IUIAInvokeProvider
{
public:
    CManagedInvokeProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedInvokeProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAInvokeProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIInvoke;
    }

    HRESULT Invoke() override;
};

class CManagedDockProvider : public CUIAPatternProvider, public IUIADockProvider
{
public:
    CManagedDockProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedDockProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIADockProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIDock;
    }

    HRESULT get_DockPosition(_Out_ UIAXcp::DockPosition *pRetVal) override;
    HRESULT SetDockPosition(_In_ UIAXcp::DockPosition dockPosition) override;
};

class CManagedExpandCollapseProvider : public CUIAPatternProvider, public IUIAExpandCollapseProvider
{
public:
    CManagedExpandCollapseProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedExpandCollapseProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAExpandCollapseProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIExpandCollapse;
    }

    HRESULT Collapse() override;
    HRESULT Expand() override;
    HRESULT get_ExpandCollapseState(_Out_ UIAXcp::ExpandCollapseState *pRetVal) override;
};

class CManagedGridItemProvider : public CUIAPatternProvider, public IUIAGridItemProvider
{
public:
    CManagedGridItemProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedGridItemProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAGridItemProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIGridItem;
    }

    HRESULT get_Column(_Out_ XINT32 *pRetVal) override;
    HRESULT get_ColumnSpan(_Out_ XINT32 *pRetVal) override;
    HRESULT get_ContainingGrid(_Out_ CAutomationPeer **pRetVal) override;
    HRESULT get_Row(_Out_ XINT32 *pRetVal) override;
    HRESULT get_RowSpan(_Out_ XINT32 *pRetVal) override;
};

class CManagedGridProvider : public CUIAPatternProvider, public IUIAGridProvider
{
public:
    CManagedGridProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedGridProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAGridProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIGrid;
    }

    HRESULT get_ColumnCount(_Out_ XINT32 *pRetVal) override;
    HRESULT GetItem(_In_ XINT32 row, _In_ XINT32 column, _Out_ CAutomationPeer **pRetVal) override;
    HRESULT get_RowCount(_Out_ XINT32 *pRetVal) override;
};

class CManagedMultipleViewProvider : public CUIAPatternProvider, public IUIAMultipleViewProvider
{
public:
    CManagedMultipleViewProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedMultipleViewProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAMultipleViewProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIMultipleView;
    }

    HRESULT get_CurrentView(_Out_ XINT32* pRetVal) override;
    HRESULT GetSupportedViews(_Out_ XUINT32 *cRetVal, _Out_writes_(*cRetVal) XINT32** pRetVal) override;
    HRESULT GetViewName(_In_ XINT32 viewId, _Out_ xstring_ptr* pstrRetVal) override;
    HRESULT SetCurrentView(_In_ XINT32 viewId) override;
};

class CManagedRangeValueProvider : public CUIAPatternProvider, public IUIARangeValueProvider
{
public:
    CManagedRangeValueProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedRangeValueProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIARangeValueProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIRangeValue;
    }

    HRESULT get_IsReadOnly(_Out_ XINT32 *pRetVal) override;
    HRESULT get_LargeChange(_Out_ DOUBLE *pRetVal) override;
    HRESULT get_Maximum(_Out_ DOUBLE *pRetVal) override;
    HRESULT get_Minimum(_Out_ DOUBLE *pRetVal) override;
    HRESULT SetValue(_In_ XFLOAT val) override;
    HRESULT get_SmallChange(_Out_ DOUBLE *pRetVal) override;
    HRESULT get_Value(_Out_ DOUBLE *pRetVal) override;
};

class CManagedScrollItemProvider : public CUIAPatternProvider, public IUIAScrollItemProvider
{
public:
    CManagedScrollItemProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedScrollItemProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAScrollItemProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIScrollItem;
    }

    HRESULT ScrollIntoView() override;
};

class CManagedScrollProvider : public CUIAPatternProvider, public IUIAScrollProvider
{
public:
    CManagedScrollProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedScrollProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAScrollProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIScroll;
    }

    HRESULT get_HorizontallyScrollable(_Out_ XINT32 *pRetVal) override;
    HRESULT get_HorizontalScrollPercent(_Out_ DOUBLE *pRetVal) override;
    HRESULT get_HorizontalViewSize(_Out_ DOUBLE *pRetVal) override;
    HRESULT Scroll(_In_ UIAXcp::ScrollAmount horizontalAmount, _In_ UIAXcp::ScrollAmount verticalAmount) override;
    HRESULT SetScrollPercent(_In_ XFLOAT horizontalPercent, _In_ XFLOAT verticalPercent) override;
    HRESULT get_VerticallyScrollable(_Out_ XINT32 *pRetVal) override;
    HRESULT get_VerticalScrollPercent(_Out_ DOUBLE *pRetVal) override;
    HRESULT get_VerticalViewSize(_Out_ DOUBLE *pRetVal) override;
};

class CManagedSelectionItemProvider : public CUIAPatternProvider, public IUIASelectionItemProvider
{
public:
    CManagedSelectionItemProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedSelectionItemProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIASelectionItemProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PISelectionItem;
    }

    HRESULT AddToSelection() override;
    HRESULT get_IsSelected(_Out_ XINT32 *pRetVal) override;
    HRESULT RemoveFromSelection() override;
    HRESULT Select() override;
    HRESULT get_SelectionContainer(_Out_ CAutomationPeer **pRetVal) override;
};

class CManagedSelectionProvider : public CUIAPatternProvider, public IUIASelectionProvider
{
public:
    CManagedSelectionProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedSelectionProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIASelectionProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PISelection;
    }

    HRESULT get_CanSelectMultiple(_Out_ XINT32 *pRetVal) override;
    HRESULT GetSelection(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) override;
    HRESULT get_IsSelectionRequired(_Out_ XINT32 *pRetVal) override;
};

class CManagedTableItemProvider : public CUIAPatternProvider, public IUIATableItemProvider
{
public:
    CManagedTableItemProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedTableItemProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIATableItemProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PITableItem;
    }

    HRESULT GetColumnHeaderItems(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) override;
    HRESULT GetRowHeaderItems(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) override;
};

class CManagedTableProvider : public CUIAPatternProvider, public IUIATableProvider
{
public:
    CManagedTableProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedTableProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIATableProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PITable;
    }

    HRESULT GetColumnHeaders(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal)CAutomationPeer ***pRetVal) override;
    HRESULT GetRowHeaders(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal)CAutomationPeer ***pRetVal) override;
    HRESULT get_RowOrColumnMajor(_Out_ UIAXcp::RowOrColumnMajor *pRetVal) override;
};

class CManagedToggleProvider : public CUIAPatternProvider, public IUIAToggleProvider
{
public:
    CManagedToggleProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedToggleProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAToggleProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIToggle;
    }

    HRESULT Toggle() override;
    HRESULT get_ToggleState(_Out_ UIAXcp::ToggleState *pRetVal) override;
};

class CManagedTransformProvider : public CUIAPatternProvider, public IUIATransformProvider
{
public:
    CManagedTransformProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedTransformProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIATransformProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PITransform;
    }

    HRESULT get_CanMove(_Out_ XINT32 *pRetVal) override;
    HRESULT get_CanResize(_Out_ XINT32 *pRetVal) override;
    HRESULT get_CanRotate(_Out_ XINT32 *pRetVal) override;
    HRESULT Move(_In_ XFLOAT x, _In_ XFLOAT y) override;
    HRESULT Resize(_In_ XFLOAT width, _In_ XFLOAT height) override;
    HRESULT Rotate(_In_ XFLOAT degree) override;

    // ITransformProvider2 implementation
    HRESULT get_CanZoom(_Out_ XINT32 *pRetVal) override;
    HRESULT get_ZoomLevel(_Out_ DOUBLE *pRetVal) override;
    HRESULT get_ZoomMaximum(_Out_ DOUBLE *pRetVal) override;
    HRESULT get_ZoomMinimum(_Out_ DOUBLE *pRetVal) override;
    HRESULT Zoom(_In_ XFLOAT zoom) override;
    HRESULT ZoomByUnit(_In_ UIAXcp::ZoomUnit zoomUnit) override;

    HRESULT IsITransformProvider2(_Out_ XINT32 *pIsITransformProvider2) override;
};

class CManagedValueProvider : public CUIAPatternProvider, public IUIAValueProvider
{
public:
    CManagedValueProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedValueProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAValueProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIValue;
    }

    HRESULT get_IsReadOnly(_Out_ XINT32 *pRetVal) override;
    HRESULT SetValue(_In_ const xstring_ptr& strString) override;
    HRESULT get_Value(_Out_ xstring_ptr* pstrRetVal) override;
};

class CManagedWindowProvider : public CUIAPatternProvider, public IUIAWindowProvider
{
public:
    CManagedWindowProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedWindowProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAWindowProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIWindow;
    }

    HRESULT Close() override;
    HRESULT get_IsModal(_Out_ XINT32 *pRetVal) override;
    HRESULT get_IsTopmost(_Out_ XINT32 *pRetVal) override;
    HRESULT get_Maximizable(_Out_ XINT32 *pRetVal) override;
    HRESULT get_Minimizable(_Out_ XINT32 *pRetVal) override;
    HRESULT SetVisualState(_In_ UIAXcp::WindowVisualState state) override;
    HRESULT WaitForInputIdle(_In_ XINT32 milliseconds, _Out_ XINT32 *pRetVal) override;
    HRESULT get_WindowInteractionState(_Out_ UIAXcp::WindowInteractionState *pRetVal) override;
    HRESULT get_WindowVisualState(_Out_ UIAXcp::WindowVisualState *pRetVal) override;
};

class CManagedTextRangeProvider : public CUIAPatternProvider, public IUIATextRangeProvider
{
public:
    CManagedTextRangeProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedTextRangeProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIATextRangeProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PITextRange;
    }

    CDependencyObject* GetTextRangeProviderObject() override
    {
        return m_pInteropObject;
    }

    CAutomationPeer* GetAutomationPeer() override
    {
        return m_pAP;
    }

    IUIAProvider* GetUIAProvider() override
    {
        return this;
    }

    HRESULT AddToSelection() override;
    HRESULT Clone(_Out_ IUIATextRangeProvider **pRetVal) override;
    HRESULT Compare(_In_ IUIATextRangeProvider *range, _Out_ bool *pRetVal) override;
    HRESULT CompareEndpoints(_In_ UIAXcp::TextPatternRangeEndpoint endpoint, _In_ IUIATextRangeProvider *targetRange, _In_ UIAXcp::TextPatternRangeEndpoint targetEndpoint, _Out_ int *pRetVal) override;
    HRESULT ExpandToEnclosingUnit(_In_ UIAXcp::TextUnit unit) override;
    HRESULT FindAttribute(_In_ XINT32 attributeId, _In_ CValue val, _In_ bool backward, _Out_ IUIATextRangeProvider **pRetVal) override;
    HRESULT FindText(_In_ const xstring_ptr& strString, _In_ bool backward, _In_ bool ignoreCase, _Out_ IUIATextRangeProvider **pRetVal) override;
    HRESULT GetAttributeValue(_In_ XINT32 attributeId, _Out_ CValue *pRetVal) override;
    HRESULT GetBoundingRectangles(_Out_ XUINT32 *cRetVal, _Out_writes_(*cRetVal) XDOUBLE** pRetVal) override;
    HRESULT GetChildren(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) override;
    HRESULT GetEnclosingElement(_Out_ CAutomationPeer **pRetVal) override;
    HRESULT GetText(_In_ int maxLength, _Out_ xstring_ptr* pstrRetVal) override;
    HRESULT Move(_In_ UIAXcp::TextUnit unit, _In_ int count, _Out_ int *pRetVal) override;
    HRESULT MoveEndpointByRange(_In_ UIAXcp::TextPatternRangeEndpoint endpoint, _In_ IUIATextRangeProvider *targetRange, _In_ UIAXcp::TextPatternRangeEndpoint targetEndpoint) override;
    HRESULT MoveEndpointByUnit(_In_ UIAXcp::TextPatternRangeEndpoint endpoint, _In_ UIAXcp::TextUnit unit, _In_ int count, _Out_ int *pRetVal) override;
    HRESULT RemoveFromSelection() override;
    HRESULT ScrollIntoView(_In_ bool alignToTop) override;
    HRESULT Select() override;

    //For ITextProvider2
    HRESULT ShowContextMenu() override;

    HRESULT IsITextRangeProvider2(_Out_ XINT32 *pIsITextRangeProvider2) override;

};

class CManagedTextProvider : public CUIAPatternProvider, public IUIATextProvider
{
public:
    CManagedTextProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedTextProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIATextProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIText;
    }

    CAutomationPeer* GetAutomationPeer() override
    {
        return m_pAP;
    }

    HRESULT GetSelection(_Out_ XUINT32 *cRetVal, _Outptr_result_buffer_(*cRetVal) IUIATextRangeProvider*** pRetVal) override;
    HRESULT GetVisibleRanges(_Out_ XUINT32 *cRetVal, _Outptr_result_buffer_(*cRetVal) IUIATextRangeProvider*** pRetVal) override;
    HRESULT RangeFromChild(_In_ CAutomationPeer *childElement, _Out_ IUIATextRangeProvider **pRetVal) override;
    HRESULT RangeFromPoint(_In_ XPOINTF *pLocation, _Out_ IUIATextRangeProvider **pRetVal) override;
    HRESULT get_DocumentRange(_Out_ IUIATextRangeProvider **pRetVal) override;
    HRESULT get_SupportedTextSelection (_Out_ UIAXcp::SupportedTextSelection *pRetVal) override;

    //For ITextProvider2
    HRESULT RangeFromAnnotation(_In_ CAutomationPeer *pAnnotationElement,  _Outptr_result_maybenull_ IUIATextRangeProvider **ppRetVal) override;
    HRESULT GetCaretRange(_Out_ XINT32 *pIsActive, _Outptr_result_maybenull_ IUIATextRangeProvider **ppRetVal) override;

    HRESULT IsITextProvider2(_Out_ XINT32 *pIsITextProvider2) override;
};



class CManagedTextEditProvider : public CManagedTextProvider, public IUIATextEditProvider
{
public:
    CManagedTextEditProvider(_In_ CAutomationPeer *pAP) : CManagedTextProvider(pAP)
    {
    }

    ~CManagedTextEditProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIATextEditProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PITextEdit;
    }

    HRESULT GetActiveComposition(_Outptr_result_maybenull_ IUIATextRangeProvider **pRetVal) override;
    HRESULT GetConversionTarget(_Outptr_result_maybenull_ IUIATextRangeProvider **pRetVal) override;
};


class CManagedItemContainerProvider : public CUIAPatternProvider, public IUIAItemContainerProvider
{
public:
    CManagedItemContainerProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedItemContainerProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAItemContainerProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIItemContainer;
    }

    HRESULT FindItemByProperty(_In_ CAutomationPeer *pStartAfter, _In_ UIAXcp::APAutomationProperties ePropertyType, _In_ CValue& val, _Out_ CAutomationPeer **ppRetVal) override;
};

class CManagedVirtualizedItemProvider : public CUIAPatternProvider, public IUIAVirtualizedItemProvider
{
public:
    CManagedVirtualizedItemProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedVirtualizedItemProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAVirtualizedItemProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIVirtualizedItem;
    }

    HRESULT Realize() override;
};

class CManagedTextChildProvider : public CUIAPatternProvider, public IUIATextChildProvider
{
public:
    CManagedTextChildProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedTextChildProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIATextChildProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PITextChild;
    }

    HRESULT get_TextContainer(_Outptr_result_maybenull_ CAutomationPeer **ppRetVal) override;
    HRESULT get_TextRange(_Outptr_result_maybenull_ IUIATextRangeProvider **ppRetVal) override;
};

class CManagedAnnotationProvider : public CUIAPatternProvider, public IUIAAnnotationProvider
{
public:
    CManagedAnnotationProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedAnnotationProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAAnnotationProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIAnnotation;
    }

    HRESULT get_AnnotationTypeId(_Out_ XINT32 *pRetVal) override;
    HRESULT get_AnnotationTypeName(_Out_ xstring_ptr* pstrRetVal) override;
    HRESULT get_Author(_Out_ xstring_ptr* pstrRetVal) override;
    HRESULT get_DateTime(_Out_ xstring_ptr* pstrRetVal) override;
    HRESULT get_Target(_Outptr_result_maybenull_ CAutomationPeer **pRetVal) override;
};

class CManagedDragProvider : public CUIAPatternProvider, public IUIADragProvider
{
public:
    CManagedDragProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedDragProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIADragProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIDrag;
    }

    HRESULT get_IsGrabbed(_Out_ bool* pRetVal) override;
    HRESULT get_DropEffect(_Out_ xstring_ptr* pstrRetVal) override;
    HRESULT get_DropEffects(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) xstring_ptr** ppstrRetVal) override;
    HRESULT GetGrabbedItems(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer*** pRetVal) override;
};

class CManagedDropTargetProvider : public CUIAPatternProvider, public IUIADropTargetProvider
{
public:
    CManagedDropTargetProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    // Destructor
    ~CManagedDropTargetProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIADropTargetProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIDropTarget;
    }

    HRESULT get_DropTargetEffect(_Out_ xstring_ptr* pstrRetVal) override;
    HRESULT get_DropTargetEffects(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) xstring_ptr** ppstrRetVal) override;
};


class CManagedObjectModelProvider : public CUIAPatternProvider, public IUIAObjectModelProvider
{
public:
    CManagedObjectModelProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    ~CManagedObjectModelProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAObjectModelProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIObjectModel;
    }

    HRESULT GetUnderlyingObjectModel(_Outptr_result_maybenull_ IUnknown ** pRetVal) override;
};

class CManagedSpreadsheetProvider : public CUIAPatternProvider, public IUIASpreadsheetProvider
{
public:
    CManagedSpreadsheetProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    ~CManagedSpreadsheetProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIASpreadsheetProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PISpreadsheet;
    }

    HRESULT GetItemByName(_In_ const xstring_ptr& strName, _Outptr_result_maybenull_ CAutomationPeer **ppRetVal) override;
};

class CManagedSpreadsheetItemProvider : public CUIAPatternProvider, public IUIASpreadsheetItemProvider
{
public:
    CManagedSpreadsheetItemProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    ~CManagedSpreadsheetItemProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIASpreadsheetItemProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PISpreadsheetItem;
    }

    HRESULT get_Formula(_Out_ xstring_ptr* pstrRetVal) override;
    HRESULT GetAnnotationObjects(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) override;
    HRESULT GetAnnotationTypes(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) UIAXcp::AnnotationType **pRetVal) override;
};

class CManagedStylesProvider : public CUIAPatternProvider, public IUIAStylesProvider
{
public:
    CManagedStylesProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    ~CManagedStylesProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIAStylesProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PIStyles;
    }

    HRESULT get_ExtendedProperties(_Out_ xstring_ptr* pstrRetVal) override;
    HRESULT get_FillColor(_Out_ XINT32 *pRetVal) override;
    HRESULT get_FillPatternColor(_Out_ XINT32 *pRetVal) override;
    HRESULT get_FillPatternStyle(_Out_ xstring_ptr* pstrRetVal) override;
    HRESULT get_Shape(_Out_ xstring_ptr* pstrRetVal) override;
    HRESULT get_StyleId(_Out_ XINT32 *pRetVal) override;
    HRESULT get_StyleName(_Out_ xstring_ptr* pstrRetVal) override;
};

class CManagedSynchronizedInputProvider : public CUIAPatternProvider, public IUIASynchronizedInputProvider
{
public:
    CManagedSynchronizedInputProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    ~CManagedSynchronizedInputProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIASynchronizedInputProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PISynchronizedInput;
    }

    HRESULT Cancel() override;
    HRESULT StartListening(_In_ UIAXcp::SynchronizedInputType inputType) override;
};

class CManagedCustomNavigationProvider : public CUIAPatternProvider, public IUIACustomNavigationProvider
{
public:
    CManagedCustomNavigationProvider(_In_ CAutomationPeer *pAP) : CUIAPatternProvider(pAP)
    {
    }

    ~CManagedCustomNavigationProvider() override
    {
    }

    void* GetPatternInterface() override
    {
        return (void*)(IUIACustomNavigationProvider*)this;
    }

    UIAXcp::APPatternInterface GetPatternType() override
    {
        return UIAXcp::PICustomNavigation;
    }

    HRESULT NavigateCustom(_In_ UIAXcp::AutomationNavigationDirection direction, _Outptr_result_maybenull_ CAutomationPeer** ppReturnAP, _Outptr_result_maybenull_ IUnknown** ppReturnIREPSAsUnk) override;
};
