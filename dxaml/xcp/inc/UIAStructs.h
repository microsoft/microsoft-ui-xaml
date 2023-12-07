// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Interface:  UIAutomation Patterns
//  Synopsis:
//      The interfaces necessary to implement the patterns
//      Compatibility with UIAutomation and WPF

#pragma once

struct IUIAWrapper
{
    virtual void Invalidate() = 0;
};

struct IUIAWindowValidator : IObject
{
    virtual XINT32 IsValid() = 0;
    virtual void Invalidate() = 0;
};

struct IUIAProvider : IObject
{
    virtual void InvalidateUIAWrapper() = 0;
    virtual IUIAWrapper *GetUIAWrapper() = 0;
    virtual void SetUIAWrapper(IUIAWrapper* pUIAWrapper) = 0;
    virtual void* GetPatternInterface() = 0;
    virtual UIAXcp::APPatternInterface GetPatternType() = 0;
};

struct IUIAInvokeProvider
{
    virtual HRESULT Invoke() = 0;
};

struct IUIADockProvider
{
    virtual HRESULT get_DockPosition(_Out_ UIAXcp::DockPosition *pRetVal) = 0;
    virtual HRESULT SetDockPosition(_In_ UIAXcp::DockPosition dockPosition) = 0;
};

struct IUIAExpandCollapseProvider
{
    virtual HRESULT Expand() = 0;
    virtual HRESULT Collapse() = 0;
    virtual HRESULT get_ExpandCollapseState(_Out_ UIAXcp::ExpandCollapseState *pRetVal) = 0;
};

struct IUIAGridItemProvider
{
    virtual HRESULT get_Column(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_ColumnSpan(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_ContainingGrid(_Out_ CAutomationPeer **pRetVal) = 0;
    virtual HRESULT get_Row(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_RowSpan(_Out_ XINT32 *pRetVal) = 0;
};

struct IUIAGridProvider
{
    virtual HRESULT get_ColumnCount(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT GetItem(_In_ XINT32 row, _In_ XINT32 column, _Out_ CAutomationPeer **pRetVal) = 0;
    virtual HRESULT get_RowCount(_Out_ XINT32 *pRetVal) = 0;
};

struct IUIAMultipleViewProvider
{
    virtual HRESULT get_CurrentView(_Out_ XINT32* pRetVal) = 0;
    virtual HRESULT GetSupportedViews(_Out_ XUINT32 *cRetVal, _Out_writes_(*cRetVal) XINT32** pRetVal) = 0;
    virtual HRESULT GetViewName(_In_ XINT32 viewId, _Out_ xstring_ptr* pstrRetVal) = 0;
    virtual HRESULT SetCurrentView(_In_ XINT32 viewId) = 0;
};

struct IUIARangeValueProvider
{
    virtual HRESULT get_IsReadOnly(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_LargeChange(_Out_ DOUBLE *pRetVal) = 0;
    virtual HRESULT get_Maximum(_Out_ DOUBLE *pRetVal) = 0;
    virtual HRESULT get_Minimum(_Out_ DOUBLE *pRetVal) = 0;
    virtual HRESULT SetValue(_In_ XFLOAT val) = 0;
    virtual HRESULT get_SmallChange(_Out_ DOUBLE *pRetVal) = 0;
    virtual HRESULT get_Value(_Out_ DOUBLE *pRetVal) = 0;
};

struct IUIAScrollItemProvider
{
    virtual HRESULT ScrollIntoView() = 0;
};

struct IUIAScrollProvider
{
    virtual HRESULT get_HorizontallyScrollable(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_HorizontalScrollPercent(_Out_ DOUBLE *pRetVal) = 0;
    virtual HRESULT get_HorizontalViewSize(_Out_ DOUBLE *pRetVal) = 0;
    virtual HRESULT Scroll(_In_ UIAXcp::ScrollAmount horizontalAmount, _In_ UIAXcp::ScrollAmount verticalAmount) = 0;
    virtual HRESULT SetScrollPercent(_In_ XFLOAT horizontalPercent, _In_ XFLOAT verticalPercent) = 0;
    virtual HRESULT get_VerticallyScrollable(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_VerticalScrollPercent(_Out_ DOUBLE *pRetVal) = 0;
    virtual HRESULT get_VerticalViewSize(_Out_ DOUBLE *pRetVal) = 0;
};

struct IUIASelectionItemProvider
{
    virtual HRESULT AddToSelection() = 0;
    virtual HRESULT get_IsSelected(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT RemoveFromSelection() = 0;
    virtual HRESULT Select() = 0;
    virtual HRESULT get_SelectionContainer(_Out_ CAutomationPeer **pRetVal) = 0;
};

struct IUIASelectionProvider
{
    virtual HRESULT get_CanSelectMultiple(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT GetSelection(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) = 0;
    virtual HRESULT get_IsSelectionRequired(_Out_ XINT32 *pRetVal) = 0;
};

struct IUIATableItemProvider
{
    virtual HRESULT GetColumnHeaderItems(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) = 0;
    virtual HRESULT GetRowHeaderItems(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) = 0;
};

struct IUIATableProvider
{
    virtual HRESULT GetColumnHeaders(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) = 0;
    virtual HRESULT GetRowHeaders(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) = 0;
    virtual HRESULT get_RowOrColumnMajor(_Out_ UIAXcp::RowOrColumnMajor *pRetVal) = 0;
};

struct IUIAToggleProvider
{
    virtual HRESULT Toggle() = 0;
    virtual HRESULT get_ToggleState(_Out_ UIAXcp::ToggleState *pRetVal) = 0;
};

struct IUIATransformProvider
{
    virtual HRESULT get_CanMove(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_CanResize(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_CanRotate(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT Move(_In_ XFLOAT x, _In_ XFLOAT y) = 0;
    virtual HRESULT Resize(_In_ XFLOAT width, _In_ XFLOAT height) = 0;
    virtual HRESULT Rotate(_In_ XFLOAT degree) = 0;

    // ITransformProvider2 implementation
    virtual HRESULT get_CanZoom(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_ZoomLevel(_Out_ DOUBLE *pRetVal) = 0;
    virtual HRESULT get_ZoomMaximum(_Out_ DOUBLE *pRetVal) = 0;
    virtual HRESULT get_ZoomMinimum(_Out_ DOUBLE *pRetVal) = 0;
    virtual HRESULT Zoom(_In_ XFLOAT zoom) = 0;
    virtual HRESULT ZoomByUnit(_In_ UIAXcp::ZoomUnit zoomUnit) = 0;

    virtual HRESULT IsITransformProvider2(_Out_ XINT32 *pIsITransformProvider2) = 0;
};

struct IUIAValueProvider
{
    virtual HRESULT get_IsReadOnly(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT SetValue(_In_ const xstring_ptr& strString) = 0;
    virtual HRESULT get_Value(_Out_ xstring_ptr* pstrRetVal) = 0;
};

struct IUIAWindowProvider
{
    virtual HRESULT Close() = 0;
    virtual HRESULT get_IsModal(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_IsTopmost(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_Maximizable(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_Minimizable(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT SetVisualState(_In_ UIAXcp::WindowVisualState state) = 0;
    virtual HRESULT WaitForInputIdle(_In_ XINT32 milliseconds, _Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_WindowInteractionState(_Out_ UIAXcp::WindowInteractionState *pRetVal) = 0;
    virtual HRESULT get_WindowVisualState(_Out_ UIAXcp::WindowVisualState *pRetVal) = 0;

};

struct IUIATextRangeProvider
{
    virtual HRESULT AddToSelection() = 0;
    virtual HRESULT Clone(_Out_ IUIATextRangeProvider **pRetVal) = 0;
    virtual HRESULT Compare(_In_ IUIATextRangeProvider *pTextRange, _Out_ bool *pRetVal) = 0;
    virtual HRESULT CompareEndpoints(_In_ UIAXcp::TextPatternRangeEndpoint endpoint, _In_ IUIATextRangeProvider *targetRange, _In_ UIAXcp::TextPatternRangeEndpoint targetEndpoint, _Out_ int *pRetVal) = 0;
    virtual HRESULT ExpandToEnclosingUnit(_In_ UIAXcp::TextUnit unit) = 0;
    virtual HRESULT FindAttribute(_In_ XINT32 attributeId, _In_ CValue val, _In_ bool backward, _Out_ IUIATextRangeProvider **pRetVal) = 0;
    virtual HRESULT FindText(_In_ const xstring_ptr& strString, _In_ bool backward, _In_ bool ignoreCase, _Out_ IUIATextRangeProvider **pRetVal) = 0;
    virtual HRESULT GetAttributeValue(_In_ XINT32 attributeId, _Out_ CValue *pRetVal) = 0;
    virtual HRESULT GetBoundingRectangles(_Out_ XUINT32 *cRetVal, _Out_writes_(*cRetVal) XDOUBLE** pRetVal) = 0;
    virtual HRESULT GetChildren(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) = 0;
    virtual HRESULT GetEnclosingElement(_Out_ CAutomationPeer **pRetVal) = 0;
    virtual HRESULT GetText(_In_ int maxLength, _Out_ xstring_ptr* pstrRetVal) = 0;
    virtual HRESULT Move(_In_ UIAXcp::TextUnit unit, _In_ int count, _Out_ int *pRetVal) = 0;
    virtual HRESULT MoveEndpointByRange(_In_ UIAXcp::TextPatternRangeEndpoint endpoint, _In_ IUIATextRangeProvider *targetRange, _In_ UIAXcp::TextPatternRangeEndpoint targetEndpoint) = 0;
    virtual HRESULT MoveEndpointByUnit(_In_ UIAXcp::TextPatternRangeEndpoint endpoint, _In_ UIAXcp::TextUnit unit, _In_ int count, _Out_ int *pRetVal) = 0;
    virtual HRESULT RemoveFromSelection() = 0;
    virtual HRESULT ScrollIntoView(_In_ bool alignToTop) = 0;
    virtual HRESULT Select() = 0;

    // ITextProvider2 implementation
    virtual HRESULT ShowContextMenu() = 0;

    virtual HRESULT IsITextRangeProvider2(_Out_ XINT32 *pIsITextRangeProvider2) = 0;

    virtual CDependencyObject* GetTextRangeProviderObject() = 0;
    virtual CAutomationPeer* GetAutomationPeer() = 0;

    virtual IUIAProvider* GetUIAProvider() = 0;
};

struct IUIATextProvider
{
    virtual HRESULT GetSelection(_Out_ XUINT32 *cRetVal, _Outptr_result_buffer_(*cRetVal) IUIATextRangeProvider*** pRetVal) = 0;
    virtual HRESULT GetVisibleRanges(_Out_ XUINT32 *cRetVal, _Outptr_result_buffer_(*cRetVal) IUIATextRangeProvider*** pRetVal) = 0;
    virtual HRESULT RangeFromChild(_In_ CAutomationPeer *childElement, _Out_ IUIATextRangeProvider **pRetVal) = 0;
    virtual HRESULT RangeFromPoint(_In_ XPOINTF *pLocation, _Out_ IUIATextRangeProvider **pRetVal) = 0;
    virtual HRESULT get_DocumentRange(_Out_ IUIATextRangeProvider **pRetVal) = 0;
    virtual HRESULT get_SupportedTextSelection(_Out_ UIAXcp::SupportedTextSelection  *pRetVal) = 0;

    // ITextProvider2 implementation
    virtual HRESULT RangeFromAnnotation(_In_ CAutomationPeer *annotationElement, _Outptr_result_maybenull_ IUIATextRangeProvider **pRetVal) = 0;
    virtual HRESULT GetCaretRange(_Out_ XINT32 *pIsActive, _Outptr_result_maybenull_ IUIATextRangeProvider **pRetVal) = 0;

    virtual HRESULT IsITextProvider2(_Out_ XINT32 *pIsITextProvider2) = 0;

    virtual CAutomationPeer* GetAutomationPeer() = 0;
};

struct IUIATextEditProvider
{
    virtual HRESULT GetActiveComposition(_Outptr_result_maybenull_ IUIATextRangeProvider **pRetVal) = 0;
    virtual HRESULT GetConversionTarget(_Outptr_result_maybenull_ IUIATextRangeProvider **pRetVal) = 0;
};

struct IUIAItemContainerProvider
{
    virtual HRESULT FindItemByProperty(_In_ CAutomationPeer *pStartAfter, UIAXcp::APAutomationProperties ePropertyType, _In_ CValue& val, _Out_ CAutomationPeer **ppRetVal) = 0;
};

struct IUIAVirtualizedItemProvider
{
    virtual HRESULT Realize() = 0;
};

struct IUIATextChildProvider
{
    virtual HRESULT get_TextContainer(_Outptr_result_maybenull_ CAutomationPeer **ppTextContainer) = 0;
    virtual HRESULT get_TextRange(_Outptr_result_maybenull_ IUIATextRangeProvider **pRetVal) = 0;
};

struct IUIAAnnotationProvider
{
    virtual HRESULT get_AnnotationTypeId(_Out_ XINT32 *pRetVal) = 0;
    virtual HRESULT get_AnnotationTypeName(_Out_ xstring_ptr* pstrRetVal) = 0;
    virtual HRESULT get_Author(_Out_ xstring_ptr* pstrRetVal) = 0;
    virtual HRESULT get_DateTime(_Out_ xstring_ptr* pstrRetVal) = 0;
    virtual HRESULT get_Target(_Outptr_result_maybenull_ CAutomationPeer **pRetVal) = 0;
};

struct IUIADragProvider
{
    virtual HRESULT get_IsGrabbed(_Out_ bool* pRetVal) = 0;
    virtual HRESULT get_DropEffect(_Out_ xstring_ptr* pstrRetVal) = 0;
    virtual HRESULT get_DropEffects(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) xstring_ptr** ppstrRetVal) = 0;
    virtual HRESULT GetGrabbedItems(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer*** pRetVal) = 0;
};

struct IUIADropTargetProvider
{
    virtual HRESULT get_DropTargetEffect(_Out_ xstring_ptr* pstrRetVal) = 0;
    virtual HRESULT get_DropTargetEffects(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) xstring_ptr** ppstrRetVal) = 0;
};

struct IUIAObjectModelProvider
{
    virtual HRESULT GetUnderlyingObjectModel(_Outptr_result_maybenull_ IUnknown ** pRetVal) = 0;
};

struct IUIASpreadsheetProvider
{
    virtual HRESULT GetItemByName(_In_ const xstring_ptr& strName, _Outptr_result_maybenull_ CAutomationPeer **pElement) = 0;
};

struct IUIASpreadsheetItemProvider
{
    virtual HRESULT get_Formula(_Out_ xstring_ptr* pstrRetVal) = 0;
    virtual HRESULT GetAnnotationObjects(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal) = 0;
    virtual HRESULT GetAnnotationTypes(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) UIAXcp::AnnotationType **pRetVal) = 0;
};

struct IUIAStylesProvider
{
    virtual HRESULT get_ExtendedProperties(_Out_ xstring_ptr* pstrRetVal) = 0;
    virtual HRESULT get_FillColor(_Out_ XINT32 *cRetVal) = 0;
    virtual HRESULT get_FillPatternColor(_Out_ XINT32 *cRetVal) = 0;
    virtual HRESULT get_FillPatternStyle(_Out_ xstring_ptr* pstrRetVal) =0;
    virtual HRESULT get_Shape(_Out_ xstring_ptr* pstrRetVal) = 0;
    virtual HRESULT get_StyleId(_Out_ XINT32 *cRetVal) = 0;
    virtual HRESULT get_StyleName(_Out_ xstring_ptr* pstrRetVal) = 0;
};

struct IUIASynchronizedInputProvider
{
    virtual HRESULT Cancel() = 0;
    virtual HRESULT StartListening(_In_ UIAXcp::SynchronizedInputType inputType) = 0;
};

struct IUIACustomNavigationProvider
{
    virtual HRESULT NavigateCustom(UIAXcp::AutomationNavigationDirection direction, _Outptr_result_maybenull_ CAutomationPeer** ppReturnAP, _Outptr_result_maybenull_ IUnknown** ppReturnIREPSAsUnk) = 0;
};
