// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// UIAPatternProvider, default implementations of the pattern providers
// Wraps and calls into managed code

#include "precomp.h"
#include "AutomationCValue.h"

//------------------------------------------------------------------------------------------------------------------------
//
// The functions below call into Managed code, the function prototype is
// pfn(NativePointer, Enum for Pattern, Method Number for Pattern, cArgs, pArgs, pReturnValue)
// The "Method Number for Pattern" is just a magic integer that matches on the managed side to know which function to call
//
//-------------------------------------------------------------------------------------------------------------------------

IUIAWrapper* CUIAPatternProvider::GetUIAWrapper()
{
    return m_pUIAWrapper;
}

void CUIAPatternProvider::SetUIAWrapper(IUIAWrapper* pUIAWrapper)
{
    ASSERT(m_pUIAWrapper == NULL);
    ASSERT(pUIAWrapper);
    m_pUIAWrapper = pUIAWrapper;
}

// AutomationPeer::BoxArrayOfHStrings creates an array of CValue strings from HSTRINGs.
// This marshals that structure to an array of xstring_ptrs.
HRESULT CUIAPatternProvider::UnboxArrayOfStrings(_In_ const Automation::CValue &source, _Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) xstring_ptr** ppstrRetVal)
{
    HRESULT hr = S_OK;
    xstring_ptr* pStrArray = nullptr;
    XUINT32 cStrArray = 0;
    Automation::CValue* pValueArray = nullptr;
    XUINT32 cValueArray = 0;

    pValueArray = static_cast<Automation::CValue*>(source.m_pvValue);
    cValueArray = source.GetArrayElementCount();

    if (cValueArray)
    {
        pStrArray = new xstring_ptr[cValueArray];
        for (XUINT32 i = 0; i < cValueArray; ++i)
        {
            pStrArray[cStrArray++] = pValueArray[i].AsString();
        }

        *cRetVal = cStrArray;
        *ppstrRetVal = pStrArray;
    }

    delete[] pValueArray;
    RRETURN(hr);//RRETURN_REMOVAL
}

HRESULT CManagedInvokeProvider::Invoke()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIInvoke, 0, 0, NULL, NULL));

    return S_OK;
}

HRESULT CManagedDockProvider::get_DockPosition(_Out_ UIAXcp::DockPosition *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIDock, 0, 0, NULL, &val));
    *pRetVal = (UIAXcp::DockPosition)(val.m_nValue);

    return S_OK;
}

HRESULT CManagedDockProvider::SetDockPosition(_In_ UIAXcp::DockPosition dockPosition)
{
    Automation::CValue val;

    val.SetEnum(dockPosition);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIDock, 1, 1, &val, NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedExpandCollapseProvider::Collapse
//
//  Synopsis:
//      CManagedExpandCollapseProvider for Collapse
//
//------------------------------------------------------------------------
HRESULT CManagedExpandCollapseProvider::Collapse()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIExpandCollapse, 0, 0, NULL, NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedExpandCollapseProvider::Expand
//
//  Synopsis:
//      CManagedExpandCollapseProvider for Expand
//
//------------------------------------------------------------------------
HRESULT CManagedExpandCollapseProvider::Expand()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIExpandCollapse, 1, 0, NULL, NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedExpandCollapseProvider::get_ExpandCollapseState
//
//  Synopsis:
//      CManagedExpandCollapseProvider for getting ExpandCollapseState
//
//------------------------------------------------------------------------
HRESULT CManagedExpandCollapseProvider::get_ExpandCollapseState(_Out_ UIAXcp::ExpandCollapseState *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIExpandCollapse, 2, 0, NULL, &val));
    *pRetVal = (UIAXcp::ExpandCollapseState)(val.m_nValue);

    return S_OK;
}

HRESULT CManagedGridItemProvider::get_Column(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIGridItem, 0, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

HRESULT CManagedGridItemProvider::get_ColumnSpan(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIGridItem, 1, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

HRESULT CManagedGridItemProvider::get_ContainingGrid(_Out_ CAutomationPeer **pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIGridItem, 2, 0, NULL, &val));
    *pRetVal = do_pointer_cast<CAutomationPeer>(val.m_pdoValue);

    return S_OK;
}

HRESULT CManagedGridItemProvider::get_Row(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIGridItem, 3, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

HRESULT CManagedGridItemProvider::get_RowSpan(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIGridItem, 4, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

HRESULT CManagedGridProvider::get_ColumnCount(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIGrid, 0, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

HRESULT CManagedGridProvider::GetItem(_In_ XINT32 row, _In_ XINT32 column, _Out_ CAutomationPeer **pRetVal)
{
    Automation::CValue val;
    Automation::CValue params[2];

    params[0].SetSigned(row);
    params[1].SetSigned(column);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIGrid, 1, 2, params, &val));
    *pRetVal = do_pointer_cast<CAutomationPeer>(val.m_pdoValue);

    return S_OK;
}

HRESULT CManagedGridProvider::get_RowCount(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIGrid, 2, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

HRESULT CManagedMultipleViewProvider::get_CurrentView(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIMultipleView, 0, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

HRESULT CManagedMultipleViewProvider::GetSupportedViews(_Out_ XUINT32 *cRetVal, _Out_writes_(*cRetVal) XINT32** pRetVal)
{
    // First fetch the length
    Automation::CValue val;
    XINT32 length = 0;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIMultipleView, 1, 0, NULL, &val));
    length = val.m_iValue;

    if ( length > 0 )
    {
        // Sync with CValue max count
        if (length > MAX_VALUE_COUNT)
        {
            length = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        XINT32 *pIntArray = new XINT32[length];
        val.m_pvValue = pIntArray;
        val.SetArrayElementCount(length);

        IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIMultipleView, 2, 0, NULL, &val));

        // Ensure the returned array count
        length = val.GetArrayElementCount();

        *cRetVal = length;
        *pRetVal = pIntArray;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }

    return S_OK;
}

HRESULT CManagedMultipleViewProvider::GetViewName(_In_ XINT32 viewId, _Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue param;
    Automation::CValue val;

    param.SetSigned(viewId);

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIMultipleView, 4, 1, &param, &val));

    *pstrRetVal = val.AsString();

    return S_OK;
}

HRESULT CManagedMultipleViewProvider::SetCurrentView(_In_ XINT32 viewId)
{
    Automation::CValue val;

    val.SetSigned(viewId);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIMultipleView, 5, 1, &val, NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedRangeValueProvider::IsReadOnly
//
//  Synopsis:
//      CManagedRangeValueProvider for IsReadOnly
//
//------------------------------------------------------------------------
HRESULT CManagedRangeValueProvider::get_IsReadOnly(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIRangeValue, 0, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedRangeValueProvider::LargeChange
//
//  Synopsis:
//      CManagedRangeValueProvider for LargeChange
//
//------------------------------------------------------------------------
HRESULT CManagedRangeValueProvider::get_LargeChange(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIRangeValue, 1, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedRangeValueProvider::Maximum
//
//  Synopsis:
//      CManagedRangeValueProvider for Maximum
//
//------------------------------------------------------------------------
HRESULT CManagedRangeValueProvider::get_Maximum(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIRangeValue, 2, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedRangeValueProvider::Minimum
//
//  Synopsis:
//      CManagedRangeValueProvider for Minimum
//
//------------------------------------------------------------------------
HRESULT CManagedRangeValueProvider::get_Minimum(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIRangeValue, 3, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedRangeValueProvider::SetValue
//
//  Synopsis:
//      CManagedRangeValueProvider for SetValue
//
//------------------------------------------------------------------------
HRESULT CManagedRangeValueProvider::SetValue(_In_ XFLOAT value)
{
    Automation::CValue param;
    
    param.SetFloat(value);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIRangeValue, 4, 1, &param, NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedRangeValueProvider::SmallChange
//
//  Synopsis:
//      CManagedRangeValueProvider for SmallChange
//
//------------------------------------------------------------------------
HRESULT CManagedRangeValueProvider::get_SmallChange(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIRangeValue, 5, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedRangeValueProvider::get_Value
//
//  Synopsis:
//      CManagedRangeValueProvider for get_Value
//
//------------------------------------------------------------------------
HRESULT CManagedRangeValueProvider::get_Value(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIRangeValue, 6, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));

    return S_OK;
}

HRESULT CManagedScrollItemProvider::ScrollIntoView()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIScrollItem, 0, 0, NULL, NULL));

    return S_OK;
}

HRESULT CManagedScrollProvider::get_HorizontallyScrollable(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIScroll, 0, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedScrollProvider::get_HorizontalScrollPercent(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIScroll, 1, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));

    return S_OK;
}

HRESULT CManagedScrollProvider::get_HorizontalViewSize(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIScroll, 2, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));

    return S_OK;
}

HRESULT CManagedScrollProvider::Scroll(_In_ UIAXcp::ScrollAmount horizontalAmount, _In_ UIAXcp::ScrollAmount verticalAmount)
{
    Automation::CValue params[2];
    params[0].SetEnum(horizontalAmount);
    params[1].SetEnum(verticalAmount);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIScroll, 3, 2, params, NULL));

    return S_OK;
}

HRESULT CManagedScrollProvider::SetScrollPercent(_In_ XFLOAT horizontalPercent, _In_ XFLOAT verticalPercent)
{
    Automation::CValue params[2];
    params[0].SetFloat(horizontalPercent);
    params[1].SetFloat(verticalPercent);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIScroll, 4, 2, params, NULL));

    return S_OK;
}

HRESULT CManagedScrollProvider::get_VerticallyScrollable(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIScroll, 5, 0, NULL, &val));
    *pRetVal = val.m_nValue;
    return S_OK;
}

HRESULT CManagedScrollProvider::get_VerticalScrollPercent(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIScroll, 6, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));
    return S_OK;
}

HRESULT CManagedScrollProvider::get_VerticalViewSize(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIScroll, 7, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));
    return S_OK;
}

HRESULT CManagedSelectionItemProvider::AddToSelection()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISelectionItem, 0, 0, NULL, NULL));
    return S_OK;
}

HRESULT CManagedSelectionItemProvider::get_IsSelected(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISelectionItem, 1, 0, NULL, &val));
    *pRetVal = val.m_nValue;
    return S_OK;
}

HRESULT CManagedSelectionItemProvider::RemoveFromSelection()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISelectionItem, 2, 0, NULL, NULL));
    return S_OK;
}

HRESULT CManagedSelectionItemProvider::Select()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISelectionItem, 3, 0, NULL, NULL));
    return S_OK;
}

HRESULT CManagedSelectionItemProvider::get_SelectionContainer(_Out_ CAutomationPeer **pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISelectionItem, 4, 0, NULL, &val));
    *pRetVal = do_pointer_cast<CAutomationPeer>(val.m_pdoValue);
    return S_OK;
}

HRESULT CManagedSelectionProvider::get_CanSelectMultiple(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISelection, 0, 0, NULL, &val));
    *pRetVal = val.m_nValue;
    return S_OK;
}

HRESULT CManagedSelectionProvider::GetSelection(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer*** pRetVal)
{
    HRESULT hr = S_OK;
    CDependencyObject **pDOArray = NULL;
    CAutomationPeer **pAPArray = NULL;

    // First fetch the length
    Automation::CValue val;
    XINT32 length = 0;
    IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISelection, 1, 0, NULL, &val));
    length = val.m_iValue;

    if ( length > 0 )
    {
        // Sync with CValue max count
        if (length > MAX_VALUE_COUNT)
        {
            length = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        pDOArray = new CDependencyObject*[length];
        pAPArray = new CAutomationPeer*[length];
        val.m_pvValue = pDOArray;
        val.SetArrayElementCount(length);
        IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISelection, 2, 0, NULL, &val));

        // Ensure the returned array count
        length = val.GetArrayElementCount();

        for (XINT32 i = 0; i < length; i++)
        {
            pAPArray[i] = do_pointer_cast<CAutomationPeer>(pDOArray[i]);
        }

        *cRetVal = length;
        *pRetVal = pAPArray;
        pAPArray = NULL;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }

Cleanup:
    delete [] pDOArray;
    delete [] pAPArray;
    RRETURN(hr);
}

HRESULT CManagedSelectionProvider::get_IsSelectionRequired(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISelection, 3, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedTableItemProvider::GetColumnHeaderItems(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer*** pRetVal)
{
    HRESULT hr = S_OK;
    CDependencyObject **pDOArray = NULL;
    CAutomationPeer **pAPArray = NULL;

    // First fetch the length
    Automation::CValue val;
    XINT32 length = 0;
    IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITableItem, 0, 0, NULL, &val));
    length = val.m_iValue;

    if ( length > 0 )
    {
        // Sync with CValue max count
        if (length > MAX_VALUE_COUNT)
        {
            length = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        pDOArray = new CDependencyObject*[length];
        pAPArray = new CAutomationPeer*[length];
        val.m_pvValue = pDOArray;
        val.SetArrayElementCount(length);

        IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITableItem, 1, 0, NULL, &val));

        // Ensure the returned array count
        length = val.GetArrayElementCount();

        for (XINT32 i = 0; i < length; i++)
        {
            pAPArray[i] = do_pointer_cast<CAutomationPeer>(pDOArray[i]);
        }

        *cRetVal = length;
        *pRetVal = pAPArray;
        pAPArray = NULL;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }

Cleanup:
    delete [] pDOArray;
    delete [] pAPArray;
    RRETURN(hr);
}

HRESULT CManagedTableItemProvider::GetRowHeaderItems(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer*** pRetVal)
{
    HRESULT hr = S_OK;
    CDependencyObject **pDOArray = NULL;
    CAutomationPeer **pAPArray = NULL;

    // First fetch the length
    Automation::CValue val;
    XINT32 length = 0;
    IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITableItem, 2, 0, NULL, &val));
    length = val.m_iValue;

    if ( length > 0 )
    {
        // Sync with CValue max count
        if (length > MAX_VALUE_COUNT)
        {
            length = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        pDOArray = new CDependencyObject*[length];
        pAPArray = new CAutomationPeer*[length];
        val.m_pvValue = pDOArray;
        val.SetArrayElementCount(length);

        IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITableItem, 3, 0, NULL, &val));

        // Ensure the returned array count
        length = val.GetArrayElementCount();

        for (XINT32 i = 0; i < length; i++)
        {
            pAPArray[i] = do_pointer_cast<CAutomationPeer>(pDOArray[i]);
        }

        *cRetVal = length;
        *pRetVal = pAPArray;
        pAPArray = NULL;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }

Cleanup:
    delete [] pDOArray;
    delete [] pAPArray;
    RRETURN(hr);
}

HRESULT CManagedTableProvider::GetColumnHeaders(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer*** pRetVal)
{
    HRESULT hr = S_OK;
    CDependencyObject **pDOArray = NULL;
    CAutomationPeer **pAPArray = NULL;

    // First fetch the length
    Automation::CValue val;
    XINT32 length = 0;
    IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITable, 0, 0, NULL, &val));
    length = val.m_iValue;

    if ( length > 0 )
    {
        // Sync with CValue max count
        if (length > MAX_VALUE_COUNT)
        {
            length = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        pDOArray = new CDependencyObject*[length];
        pAPArray = new CAutomationPeer*[length];
        val.m_pvValue = pDOArray;
        val.SetArrayElementCount(length);

        IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITable, 1, 0, NULL, &val));

        // Ensure the returned array count
        length = val.GetArrayElementCount();

        for (XINT32 i = 0; i < length; i++)
        {
            pAPArray[i] = do_pointer_cast<CAutomationPeer>(pDOArray[i]);
        }

        *cRetVal = length;
        *pRetVal = pAPArray;
        pAPArray = NULL;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }
Cleanup:
    delete [] pDOArray;
    delete [] pAPArray;
    RRETURN(hr);
}

HRESULT CManagedTableProvider::GetRowHeaders(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer*** pRetVal)
{
    HRESULT hr = S_OK;
    CDependencyObject **pDOArray = NULL;
    CAutomationPeer **pAPArray = NULL;

    // First fetch the length
    Automation::CValue val;
    XINT32 length = 0;
    IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITable, 2, 0, NULL, &val));
    length = val.m_iValue;

    if ( length > 0 )
    {
        // Sync with CValue max count
        if (length > MAX_VALUE_COUNT)
        {
            length = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        pDOArray = new CDependencyObject*[length];
        pAPArray = new CAutomationPeer*[length];
        val.m_pvValue = pDOArray;
        val.SetArrayElementCount(length);

        IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITable, 3, 0, NULL, &val));

        // Ensure the returned array count
        length = val.GetArrayElementCount();

        for (XINT32 i = 0; i < length; i++)
        {
            pAPArray[i] = do_pointer_cast<CAutomationPeer>(pDOArray[i]);
        }

        *cRetVal = length;
        *pRetVal = pAPArray;
        pAPArray = NULL;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }
Cleanup:
    delete [] pDOArray;
    delete [] pAPArray;
    RRETURN(hr);
}

HRESULT CManagedTableProvider::get_RowOrColumnMajor(_Out_ UIAXcp::RowOrColumnMajor *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITable, 4, 0, NULL, &val));
    *pRetVal = (UIAXcp::RowOrColumnMajor)(val.m_nValue);

    return S_OK;
}

HRESULT CManagedToggleProvider::Toggle()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIToggle, 0, 0, NULL, NULL));

    return S_OK;
}

HRESULT CManagedToggleProvider::get_ToggleState(_Out_ UIAXcp::ToggleState *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIToggle, 1, 0, NULL, &val));
    *pRetVal = (UIAXcp::ToggleState)(val.m_nValue);

    return S_OK;
}

HRESULT CManagedTransformProvider::get_CanMove(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform, 0, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedTransformProvider::get_CanResize(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform, 1, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedTransformProvider::get_CanRotate(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform, 2, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedTransformProvider::Move(_In_ XFLOAT x, _In_ XFLOAT y)
{
    Automation::CValue params[2];
    params[0].SetFloat(x);
    params[1].SetFloat(y);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform, 3, 2, params, NULL));

    return S_OK;
}

HRESULT CManagedTransformProvider::Resize(_In_ XFLOAT width, _In_ XFLOAT height)
{
    Automation::CValue params[2];
    params[0].SetFloat(width);
    params[1].SetFloat(height);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform, 4, 2, params, NULL));

    return S_OK;
}

HRESULT CManagedTransformProvider::Rotate(_In_ XFLOAT degree)
{
    Automation::CValue param;
    param.SetFloat(degree);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform, 5, 1, &param, NULL));

    return S_OK;
}


HRESULT CManagedTransformProvider::get_CanZoom(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform2, 0, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedTransformProvider::get_ZoomLevel(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform2, 1, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));

    return S_OK;
}

HRESULT CManagedTransformProvider::get_ZoomMaximum(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform2, 2, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));

    return S_OK;
}

HRESULT CManagedTransformProvider::get_ZoomMinimum(_Out_ DOUBLE *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform2, 3, 0, NULL, &val));
    IFC_RETURN(val.GetDouble(*pRetVal));

    return S_OK;
}

HRESULT CManagedTransformProvider::Zoom(_In_ XFLOAT zoom)
{
    Automation::CValue param;
    param.SetFloat(zoom);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform2, 4, 1, &param, NULL));

    return S_OK;
}

HRESULT CManagedTransformProvider::ZoomByUnit(_In_ UIAXcp::ZoomUnit zoomUnit)
{
    Automation::CValue param;
    param.SetSigned((XINT32)zoomUnit);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform2, 5, 1, &param, NULL));

    return S_OK;
}

HRESULT CManagedTransformProvider::IsITransformProvider2(_Out_ XINT32 *pIsITransformProvider2)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITransform2, 6/*IsITransformProvider2*/, 0/*countOfParam*/, NULL, &val));
    *pIsITransformProvider2 = val.m_nValue;

    return S_OK;
}


HRESULT CManagedValueProvider::get_IsReadOnly(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIValue, 0, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedValueProvider::SetValue(_In_ const xstring_ptr& strString)
{
    Automation::CValue param;
    param.SetString(strString);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIValue, 1, 1, &param, NULL));

    return S_OK;
}

HRESULT CManagedValueProvider::get_Value(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIValue, 3, 0, NULL, &val));

    *pstrRetVal = val.AsString();

    return S_OK;
}

HRESULT CManagedWindowProvider::Close()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIWindow, 0, 0, NULL, NULL));

    return S_OK;
}

HRESULT CManagedWindowProvider::get_IsModal(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIWindow, 1, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedWindowProvider::get_IsTopmost(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIWindow, 2, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedWindowProvider::get_Maximizable(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIWindow, 3, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedWindowProvider::get_Minimizable(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIWindow, 4, 0, NULL, &val));
    *pRetVal = val.m_nValue;

    return S_OK;
}

HRESULT CManagedWindowProvider::SetVisualState(_In_ UIAXcp::WindowVisualState state)
{
    Automation::CValue val;
    val.SetEnum(state);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIWindow, 5, 1, &val, NULL));

    return S_OK;
}

HRESULT CManagedWindowProvider::WaitForInputIdle(_In_ XINT32 milliseconds, _Out_ XINT32 *pRetVal)
{
    Automation::CValue param;
    Automation::CValue val;
    param.SetSigned(milliseconds);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIWindow, 6, 1, &param, &val));

    return S_OK;
}

HRESULT CManagedWindowProvider::get_WindowInteractionState(_Out_ UIAXcp::WindowInteractionState *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIWindow, 7, 0, NULL, &val));
    *pRetVal = (UIAXcp::WindowInteractionState)val.m_nValue;

    return S_OK;
}

HRESULT CManagedWindowProvider::get_WindowVisualState(_Out_ UIAXcp::WindowVisualState *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIWindow, 8, 0, NULL, &val));
    *pRetVal = (UIAXcp::WindowVisualState)val.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextProvider::GetSelection
//
//  Synopsis:
//
//  Note: Text pattern inferface name is hard coded and tracking Bug#32555.
//------------------------------------------------------------------------

HRESULT CManagedTextProvider::GetSelection(
    _Out_ XUINT32 *cRetVal,
    _Outptr_result_buffer_(*cRetVal) IUIATextRangeProvider*** pRetVal)
{
    HRESULT hr = S_OK;
    CDependencyObject **pDOArray = NULL;
    IUIATextRangeProvider **pTextRangeArray = NULL;

    // First get the count of the array
    Automation::CValue val;
    XINT32 count = 0;
    IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText, 0/*GetSelection*/, 0, NULL, &val));
    count = val.m_iValue;

    if ( count > 0 )
    {
        // Sync with CValue max count
        if (count > MAX_VALUE_COUNT)
        {
            count = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        pDOArray = new CDependencyObject*[count];
        pTextRangeArray = new IUIATextRangeProvider*[count];
        val.m_pvValue = pDOArray;
        val.SetArrayElementCount(count);
        IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText, 1/*GetSelection*/, 0, NULL, &val));

        // Ensure the returned array count
        count = val.GetArrayElementCount();

        for (XINT32 i = 0; i < count; i++)
        {
            pTextRangeArray[i] = m_pAP->GetTextRangePattern(pDOArray[i]);
        }

        *cRetVal = count;
        *pRetVal = pTextRangeArray;
        pTextRangeArray = NULL;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }

Cleanup:
    delete [] pDOArray;
    delete [] pTextRangeArray;

    RRETURN(hr);
}



//------------------------------------------------------------------------
//
//  Method:   CManagedTextProvider::GetVisibleRanges
//
//  Synopsis:
//
//  Note: Text pattern inferface name is hard coded and tracking with Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextProvider::GetVisibleRanges(
    _Out_ XUINT32 *cRetVal,
    _Outptr_result_buffer_(*cRetVal) IUIATextRangeProvider*** pRetVal)
{
    HRESULT hr = S_OK;

    CDependencyObject **pDOArray = NULL;
    IUIATextRangeProvider **pTextRangeArray = NULL;

    // First get the count of the array
    Automation::CValue val;
    XINT32 count = 0;
    IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText, 2/*GetVisibleRanges*/, 0/*countOfParam*/, NULL/*param*/, &val));
    count = val.m_iValue;

    if ( count > 0 )
    {
        // Sync with CValue max count
        if (count > MAX_VALUE_COUNT)
        {
            count = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        pDOArray = new CDependencyObject*[count];
        pTextRangeArray = new IUIATextRangeProvider*[count];
        val.m_pvValue = pDOArray;
        val.SetArrayElementCount(count);

        IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText, 3/*GetVisibleRanges*/, 0/*countOfParam*/, NULL/*param*/, &val));

        // Ensure the returned array count
        count = val.GetArrayElementCount();

        for (XINT32 i = 0; i < count; i++)
        {
            pTextRangeArray[i] = m_pAP->GetTextRangePattern(pDOArray[i]);
        }

        *cRetVal = count;
        *pRetVal = pTextRangeArray;
        pTextRangeArray = NULL;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }

Cleanup:
    delete [] pDOArray;
    delete [] pTextRangeArray;

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   CManagedTextProvider::RangeFromChild
//
//  Synopsis:
//
//  Note: Text pattern inferface name is hard coded and tracking with Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextProvider::RangeFromChild(
    _In_ CAutomationPeer *childElement,
    _Out_ IUIATextRangeProvider **pRetVal)
{
    Automation::CValue val;
    Automation::CValue params;

    params.WrapObjectNoRef((CAutomationPeer*)childElement);

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText, 4/*RangeFromChild*/, 1/*countOfParam*/, &params, &val));
    *pRetVal = m_pAP->GetTextRangePattern(val.m_pdoValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextProvider::RangeFromPoint
//
//  Synopsis:
//
//  Note: Text pattern inferface name is hard coded and tracking with Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextProvider::RangeFromPoint(
    _In_ XPOINTF *pLocation,
    _Out_ IUIATextRangeProvider **pRetVal)
{
    Automation::CValue val;
    Automation::CValue params;
    params.WrapPoint(pLocation);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText, 5/*RangeFromPoint*/, 1/*countOfParam*/, &params, &val));
    *pRetVal = m_pAP->GetTextRangePattern(val.m_pdoValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextProvider::get_DocumentRange
//
//  Synopsis:
//
//  Note: Text pattern inferface name is hard coded and tracking with Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextProvider::get_DocumentRange(
    _Out_ IUIATextRangeProvider **pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText, 6/*get_DocumentRange*/, 0/*countOfParam*/, NULL/*params*/, &val));
    *pRetVal = m_pAP->GetTextRangePattern(val.m_pdoValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextProvider::get_SupportedTextSelection
//
//  Synopsis:
//
//  Note: Text pattern inferface name is hard coded and tracking with Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextProvider::get_SupportedTextSelection(
    _Out_ UIAXcp::SupportedTextSelection *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText, 7/*get_SupportedTextSelection*/, 0/*countOfParam*/, NULL/*params*/, &val));
    *pRetVal = (UIAXcp::SupportedTextSelection)(val.m_nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextProvider::RangeFromAnnotation
//
//  Synopsis: This is defn for Methods belonging to ITextProvider2
//
//------------------------------------------------------------------------
HRESULT CManagedTextProvider::RangeFromAnnotation(
    _In_ CAutomationPeer *pAnnotationElement,
    _Outptr_result_maybenull_ IUIATextRangeProvider **ppRetVal)
{
    Automation::CValue val;
    Automation::CValue params;

    params.WrapObjectNoRef((CAutomationPeer*)pAnnotationElement);

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText2, 0/*RangeFromAnnotation*/, 1/*countOfParam*/, &params, &val));
    *ppRetVal = m_pAP->GetTextRangePattern(val.m_pdoValue);


    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextProvider::GetCaretRange
//
//  Synopsis: This is defn for Methods belonging to ITextProvider2
//
//------------------------------------------------------------------------
HRESULT CManagedTextProvider::GetCaretRange(
    _Out_ XINT32 *pIsActive,
    _Outptr_result_maybenull_ IUIATextRangeProvider **ppRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText2, 1/*GetCaretRange*/, 0/*countOfParam*/, NULL, &val));
    *pIsActive = val.m_nValue;
    *ppRetVal = m_pAP->GetTextRangePattern(val.m_pdoValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextProvider::IsITextProvider2
//
//  Synopsis: This is helper function used to determine if the Managed object is of type ITextProvider2 or not.
//
//------------------------------------------------------------------------
HRESULT CManagedTextProvider::IsITextProvider2(_Out_ XINT32 *pIsITextProvider2)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIText2, 2/*IsITextProvider2*/, 0/*countOfParam*/, NULL, &val));
    *pIsITextProvider2 = val.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::AddToSelection
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::AddToSelection()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 0/*AddToSelection*/, 0/*cParams*/, NULL/*params*/, NULL/*pRetVal*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::Clone
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::Clone(
    _Out_ IUIATextRangeProvider **pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 1/*Clone*/, 0/*cParams*/, NULL/*params*/, &val));
    *pRetVal = m_pAP->GetTextRangePattern(val.m_pdoValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::Compare
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::Compare(
    _In_ IUIATextRangeProvider *pTextRange,
    _Out_ bool *pRetVal)
{
    Automation::CValue val;
    Automation::CValue params;
    params.WrapObjectNoRef((CDependencyObject*)(CDependencyObject*)pTextRange);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 2/*Compare*/, 1/*cParams*/, &params, &val));
    *pRetVal = !!val.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::CompareEndpoints
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::CompareEndpoints(
    _In_ UIAXcp::TextPatternRangeEndpoint endpoint,
    _In_ IUIATextRangeProvider *targetRange,
    _In_ UIAXcp::TextPatternRangeEndpoint targetEndpoint,
    _Out_ int *pRetVal)
{
    Automation::CValue retVal;
    Automation::CValue params[3];
    params[0].SetEnum(endpoint);
    params[1].WrapObjectNoRef((CDependencyObject*)(CDependencyObject*)targetRange);
    params[2].SetEnum(targetEndpoint);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 3/*CompareEndpoints*/, 3/*cParams*/, params, &retVal));
    *pRetVal = retVal.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::ExpandToEnclosingUnit
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::ExpandToEnclosingUnit(
    _In_ UIAXcp::TextUnit unit)
{
    Automation::CValue params;
    params.SetEnum(unit);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 4/*ExpandToEnclosingUnit*/, 1, &params, NULL/*pRetVal*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::FindAttribute
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::FindAttribute(
    _In_ XINT32 attributeId,
    _In_ CValue attributeValue,
    _In_ bool backward,
    _Out_ IUIATextRangeProvider **pRetVal)
{

    Automation::CValue retVal;
    Automation::CValue params[3];
    params[0].SetSigned(attributeId);
    IFC_RETURN(params[1].ConvertFrom(attributeValue));
    params[2].SetSigned(backward);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 5/*FindAttribute*/, 3/*cParams*/, params, &retVal));
    *pRetVal = m_pAP->GetTextRangePattern(retVal.m_pdoValue);
        
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::FindText
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::FindText(
    _In_ const xstring_ptr& strString,
    _In_ bool backward,
    _In_ bool ignoreCase,
    _Out_ IUIATextRangeProvider **pRetVal)
{
    Automation::CValue retVal;
    Automation::CValue params[3];
    params[0].SetString(strString);
    params[1].SetBool(backward);
    params[2].SetBool(ignoreCase);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 6/*FindText*/, 3/*cParams*/, params, &retVal));
    *pRetVal = m_pAP->GetTextRangePattern(retVal.m_pdoValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::GetAttributeValue
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::GetAttributeValue(
    _In_ XINT32 attributeId,
    _Out_ CValue *pRetVal)
{
    Automation::CValue params;
    params.SetSigned(attributeId);
    if (attributeId == 40035) /*UIA_LinkAttributeId*/
    {
        Automation::CValue retVal;
        IUIATextRangeProvider *pTextRangeNoRef = nullptr;
        IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 7/*GetAttributeValue*/, 1/*cParams*/, &params, &retVal));
        pTextRangeNoRef = m_pAP->GetTextRangePattern(retVal.m_pdoValue);
        pRetVal->SetPointer(pTextRangeNoRef);
    }
    else
    {
        Automation::CValue retVal;
        IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 7/*GetAttributeValue*/, 1/*cParams*/, &params, &retVal));
        IFC_RETURN(retVal.ConvertTo(*pRetVal));
    }


    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::GetBoundingRectangles
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::GetBoundingRectangles(
    _Out_ XUINT32 *cRetVal,
    _Out_writes_(*cRetVal) XDOUBLE** pRetVal)
{
    HRESULT hr = S_OK;
    XDOUBLE *pDoubleArray = NULL;

    // First fetch the length
    Automation::CValue val;
    XINT32 length = 0;
    IFC(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 8/*GetBoundingRectangles*/, 0/*cParams*/, NULL/*pParams*/, &val));
    length = val.m_iValue;

    if ( length > 0 )
    {
        // Sync with CValue max count
        if (length > MAX_VALUE_COUNT)
        {
            length = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        pDoubleArray = new XDOUBLE[length];
        val.m_pvValue = pDoubleArray;
        val.SetArrayElementCount(length);

        IFC(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 9/*GetBoundingRectangles*/, 0/*cParams*/, NULL/*pParams*/, &val));

        // Ensure the returned array count
        length = val.GetArrayElementCount();

        *cRetVal = length;
        *pRetVal = pDoubleArray;
        pDoubleArray = NULL;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }

Cleanup:
    delete [] pDoubleArray;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::GetChildren
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::GetChildren(
    _Out_ XINT32 *cRetVal,
    _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal)
{
    HRESULT hr = S_OK;

    CDependencyObject **pDOArray = NULL;
    CAutomationPeer **pAPArray = NULL;

    // First fetch the length
    Automation::CValue val;
    XINT32 length = 0;
    IFC(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 10/*GetChildren*/, 0/*cParams*/, NULL/*pParams*/, &val));
    length = val.m_iValue;

    if ( length > 0 )
    {
        // Sync with CValue max count
        if (length > MAX_VALUE_COUNT)
        {
            length = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        pDOArray = new CDependencyObject*[length];
        pAPArray = new CAutomationPeer*[length];
        val.m_pvValue = pDOArray;
        val.SetArrayElementCount(length);

        IFC(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 11/*GetChildren*/, 0/*cParams*/, NULL/*pParams*/, &val));

        // Ensure the returned array count
        length = val.GetArrayElementCount();

        for (XINT32 i = 0; i < length; i++)
        {
            pAPArray[i] = do_pointer_cast<CAutomationPeer>(pDOArray[i]);
        }

        *cRetVal = length;
        *pRetVal = pAPArray;
        pAPArray = NULL;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }

Cleanup:
    delete [] pDOArray;
    delete [] pAPArray;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::GetEnclosingElement
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::GetEnclosingElement(
    _Out_ CAutomationPeer **pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 12/*GetEnclosingElement*/, 0/*cParams*/, NULL/*pParams*/, &val));
    *pRetVal = do_pointer_cast<CAutomationPeer>(val.m_pdoValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::GetText
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::GetText(
    _In_ int maxLength,
    _Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue param;
    Automation::CValue val;

    param.SetSigned(maxLength);

    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 14/*GetText*/, 1, &param, &val));

    *pstrRetVal = val.AsString();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::Move
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::Move(
    _In_ UIAXcp::TextUnit unit,
    _In_ int count,
    _Out_ int *pRetVal)
{
    Automation::CValue val;
    Automation::CValue params[2];
    params[0].SetEnum(unit);
    params[1].SetSigned(count);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 15/*Move*/, 2, params, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::MoveEndpointByRange
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::MoveEndpointByRange(
    _In_ UIAXcp::TextPatternRangeEndpoint endpoint,
    _In_ IUIATextRangeProvider *targetRange,
    _In_ UIAXcp::TextPatternRangeEndpoint targetEndpoint)
{
    Automation::CValue params[3];
    params[0].SetEnum(endpoint);
    params[1].WrapObjectNoRef((CDependencyObject*)(CDependencyObject*)targetRange);
    params[2].SetEnum(targetEndpoint);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 16/*MoveEndpointByRange*/, 3/*cParams*/, params, NULL/*pRetVal*/));


    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::MoveEndpointByUnit
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::MoveEndpointByUnit(
    _In_ UIAXcp::TextPatternRangeEndpoint endpoint,
    _In_ UIAXcp::TextUnit unit,
    _In_ int count,
    _Out_ int *pRetVal)
{
    Automation::CValue val;
    Automation::CValue params[3];
    params[0].SetEnum(endpoint);
    params[1].SetEnum(unit);
    params[2].SetSigned(count);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 17/*MoveEndpointByUnit*/, 3, params, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::RemoveFromSelection
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::RemoveFromSelection()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 18/*MoveEndpointByUnit*/, 0/*cParams*/, NULL/*params*/, NULL/*pRetVal*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::ScrollIntoView
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::ScrollIntoView(
    _In_ bool alignToTop)
{
    Automation::CValue params;
    params.SetBool(alignToTop);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 19/*ScrollIntoView*/, 1, &params, NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::Select
//
//  Synopsis:
//
//  Note: TextRangeProvider inferface name is hard coded and tracking with
//  Bug#32555.
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::Select()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 20/*Select*/, 0/*cParams*/, NULL/*params*/, NULL/*pRetVal*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::IsITextRangeProvider2
//
//  Synopsis: This is helper function used to determine if the Managed object is of type ITextRangeProvider2 or not.
//
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::IsITextRangeProvider2(_Out_ XINT32 *pIsITextRangeProvider2)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 22/*IsITextRangeProvider2*/, 0/*countOfParam*/, NULL, &val));
    *pIsITextRangeProvider2 = val.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextRangeProvider::ShowContextMenu
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedTextRangeProvider::ShowContextMenu()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIATextRangeInvoke(m_pInteropObject, 21/*ShowContextMenu*/, 0/*cParams*/, NULL/*params*/, NULL/*pRetVal*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedItemContainerProvider::FindItemByProperty
//
//  Synopsis:
//      CManagedItemContainerProvider for finding an item in Items Control
//      based on the search depending up on UIA properties
//
//------------------------------------------------------------------------
HRESULT CManagedItemContainerProvider::FindItemByProperty(
    _In_ CAutomationPeer *pStartAfter,
    _In_ UIAXcp::APAutomationProperties ePropertyType,
    _In_ CValue& val,
    _Out_ CAutomationPeer **ppRetVal)
{
    Automation::CValue retVal;
    Automation::CValue params[3];
    params[0].WrapObjectNoRef(static_cast<CAutomationPeer*>(pStartAfter));
    params[1].SetSigned(ePropertyType);
    IFC_RETURN(params[2].ConvertFrom(val));
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIItemContainer, 0, 3, params, &retVal));

    *ppRetVal = do_pointer_cast<CAutomationPeer>(retVal.m_pdoValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedVirtualizedItemProvider::Realize
//
//  Synopsis:
//      CManagedVirtualizedItemProvider for Realizing already virtualized Item
//
//------------------------------------------------------------------------
HRESULT CManagedVirtualizedItemProvider::Realize()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIVirtualizedItem, 0, 0, NULL, NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextChildProvider::get_TextContainer
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedTextChildProvider::get_TextContainer(_Outptr_result_maybenull_ CAutomationPeer **ppRetVal)
{
    Automation::CValue retVal;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITextChild, 0/*get_TextContainer*/, 0, NULL, &retVal));

    *ppRetVal = do_pointer_cast<CAutomationPeer>(retVal.m_pdoValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedTextChildProvider::get_TextRange
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedTextChildProvider::get_TextRange(_Outptr_result_maybenull_ IUIATextRangeProvider **pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITextChild, 1/*get_TextRange*/, 0/*countOfParam*/, NULL/*params*/, &val));
    *pRetVal = m_pAP->GetTextRangePattern(val.m_pdoValue);

    return S_OK;
}

HRESULT CManagedTextEditProvider::GetActiveComposition(_Outptr_result_maybenull_ IUIATextRangeProvider **ppRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITextEdit, 0/*GetActiveComposition*/, 0/*countOfParam*/, NULL, &val));
    *ppRetVal = m_pAP->GetTextRangePattern(val.m_pdoValue);

    return S_OK;
}

HRESULT CManagedTextEditProvider::GetConversionTarget(_Outptr_result_maybenull_ IUIATextRangeProvider **ppRetVal){

    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PITextEdit, 1/*GetConversionTarget*/, 0/*countOfParam*/, NULL, &val));
    *ppRetVal = m_pAP->GetTextRangePattern(val.m_pdoValue);

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Method:   CManagedAnnotationProvider::get_AnnotationTypeId
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedAnnotationProvider::get_AnnotationTypeId(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIAnnotation, 0, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedAnnotationProvider::get_AnnotationTypeName
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedAnnotationProvider::get_AnnotationTypeName(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIAnnotation, 2, 0, NULL, &val));

    *pstrRetVal = val.AsString();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedAnnotationProvider::get_Author
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedAnnotationProvider::get_Author(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIAnnotation, 4, 0, NULL, &val));

    *pstrRetVal = val.AsString();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedAnnotationProvider::get_DateTime
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedAnnotationProvider::get_DateTime(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIAnnotation, 6, 0, NULL, &val));

    *pstrRetVal = val.AsString();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedAnnotationProvider::get_Target
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedAnnotationProvider::get_Target(_Outptr_result_maybenull_ CAutomationPeer **pRetVal)
{
    Automation::CValue val;

    if(!pRetVal)
    {
        return S_OK;
    }

    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIAnnotation, 7, 0, NULL, &val));
    *pRetVal = (CAutomationPeer*)(do_pointer_cast<CAutomationPeer>(val.m_pdoValue));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedDragProvider::get_IsGrabbed
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedDragProvider::get_IsGrabbed(_Out_ bool* pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIDrag, 0, 0, NULL, &val));
    *pRetVal = val.AsBool();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedDragProvider::get_DropEffect
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedDragProvider::get_DropEffect(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIDrag, 1, 0, NULL, &val));
    *pstrRetVal = val.AsString();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedDragProvider::GetDropEffects
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedDragProvider::get_DropEffects(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) xstring_ptr** ppstrRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIDrag, 2, 0, NULL, &val));
    IFC_RETURN(UnboxArrayOfStrings(val, cRetVal, ppstrRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedDragProvider::GetGrabbedItems
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedDragProvider::GetGrabbedItems(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer*** pRetVal)
{
    CDependencyObject** pDoArray;
    CAutomationPeer** pApArray;
    XUINT32 count;

    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIDrag, 3, 0, NULL, &val));

    pDoArray = static_cast<CDependencyObject**>(val.m_pvValue);
    pApArray = static_cast<CAutomationPeer**>(val.m_pvValue);

    // Adjust the pointers in place
    count = val.GetArrayElementCount();
    for (XUINT32 i = 0; i < count; ++i)
    {
        pApArray[i] = static_cast<CAutomationPeer*>(pDoArray[i]);
    }

    *cRetVal = count;
    *pRetVal = pApArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedDropTargetProvider::get_DropTargetEffect
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedDropTargetProvider::get_DropTargetEffect(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIDropTarget, 0, 0, NULL, &val));
    *pstrRetVal = val.AsString();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedDropTargetProvider::GetDropTargetEffects
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedDropTargetProvider::get_DropTargetEffects(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) xstring_ptr** ppstrRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIDropTarget, 1, 0, NULL, &val));
    IFC_RETURN(UnboxArrayOfStrings(val, cRetVal, ppstrRetVal));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CManagedObjectModelProvider::GetUnderlyingObjectModel
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedObjectModelProvider::GetUnderlyingObjectModel(_Outptr_result_maybenull_ IUnknown ** pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIObjectModel, 0/*GetUnderlyingObjectModel*/, 0/*countOfParam*/, NULL/*params*/, &val));
    *pRetVal = (IUnknown*)(val.m_pvValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedSpreadsheetProvider::GetItemByName
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedSpreadsheetProvider::GetItemByName(_In_ const xstring_ptr& strName, _Outptr_result_maybenull_ CAutomationPeer **ppRetVal)
{
    Automation::CValue param;
    param.SetString(strName);
    Automation::CValue retVal;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISpreadsheet, 0/*GetItemByName*/, 1, &param, &retVal));
    *ppRetVal = (CAutomationPeer*)(do_pointer_cast<CAutomationPeer>(retVal.m_pdoValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedSpreadsheetItemProvider::get_Formula
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedSpreadsheetItemProvider::get_Formula(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISpreadsheetItem, 0, 0, NULL, &val));
    *pstrRetVal = val.AsString();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedSpreadsheetItemProvider::GetAnnotationObjects
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedSpreadsheetItemProvider::GetAnnotationObjects(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) CAutomationPeer ***pRetVal)
{
    HRESULT hr = S_OK;

    CDependencyObject **pDOArray = NULL;
    CAutomationPeer **pAPArray = NULL;

    // First fetch the length
    Automation::CValue val;
    XINT32 length = 0;
    IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISpreadsheetItem, 1/*GetAnnotationObjects*/, 0, NULL, &val));
    length = val.m_iValue;

    if ( length > 0 )
    {
        // Sync with CValue max count
        if (length > MAX_VALUE_COUNT)
        {
            length = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        pDOArray = new CDependencyObject*[length];
        pAPArray = new CAutomationPeer*[length];
        val.m_pvValue = pDOArray;
        val.SetArrayElementCount(length);

        IFC(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISpreadsheetItem, 2/*GetAnnotationObjects*/, 0, NULL, &val));

        // Ensure the returned array count
        IFCEXPECT(length == val.GetArrayElementCount());

        for (XINT32 i = 0; i < length; i++)
        {
            pAPArray[i] = do_pointer_cast<CAutomationPeer>(pDOArray[i]);
        }

        *cRetVal = length;
        *pRetVal = pAPArray;
        pAPArray = NULL;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }

Cleanup:
    delete [] pDOArray;
    delete [] pAPArray;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CManagedSpreadsheetItemProvider::GetAnnotationTypes
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedSpreadsheetItemProvider::GetAnnotationTypes(_Out_ XINT32 *cRetVal, _Out_writes_(*cRetVal) UIAXcp::AnnotationType **pRetVal)
{
    // First fetch the length
    Automation::CValue val;
    XINT32 length = 0;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISpreadsheetItem, 3, 0, NULL, &val));
    length = val.m_iValue;

    if ( length > 0 )
    {
        // Sync with CValue max count
        if (length > MAX_VALUE_COUNT)
        {
            length = MAX_VALUE_COUNT;
        }

        // Second fetch the contents
        UIAXcp::AnnotationType *pTypeArray = new UIAXcp::AnnotationType[length];
        val.m_pvValue = pTypeArray;
        val.SetArrayElementCount(length);

        IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISpreadsheetItem, 4, 0, NULL, &val));

        // Ensure the returned array count
        IFCEXPECT_RETURN(length == val.GetArrayElementCount());

        *cRetVal = length;

        /*UXcp::AnnotationType *pTypeArray = new UIAXcp::AnnotationTypes[length];
        for (UINT i=0; i<length; ++i)
        {
            pTypeArray[i] = (UXcp::AnnotationTypes)pIntArray[i];
        }
        delete[] pIntArray;*/
        *pRetVal = pTypeArray;
    }
    else
    {
        *cRetVal = 0;
        *pRetVal = NULL;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedStylesProvider::get_ExtendedProperties
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedStylesProvider::get_ExtendedProperties(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIStyles, 0, 0, NULL, &val));
    *pstrRetVal = val.AsString();

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CManagedStylesProvider::get_FillColor
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedStylesProvider::get_FillColor(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIStyles, 1, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedStylesProvider::get_FillPatternColor
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedStylesProvider::get_FillPatternColor(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIStyles, 2, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedStylesProvider::get_FillPatternColor
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedStylesProvider::get_FillPatternStyle(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIStyles, 3, 0, NULL, &val));
    *pstrRetVal = val.AsString();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedStylesProvider::get_Shape
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedStylesProvider::get_Shape(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIStyles, 4, 0, NULL, &val));
    *pstrRetVal = val.AsString();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedStylesProvider::get_StyleId
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedStylesProvider::get_StyleId(_Out_ XINT32 *pRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIStyles, 5, 0, NULL, &val));
    *pRetVal = val.m_iValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedStylesProvider::get_StyleName
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedStylesProvider::get_StyleName(_Out_ xstring_ptr* pstrRetVal)
{
    Automation::CValue val;
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PIStyles, 6, 0, NULL, &val));
    *pstrRetVal = val.AsString();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedSynchronizedInputProvider::Cancel
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedSynchronizedInputProvider::Cancel()
{
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISynchronizedInput, 0, 0, NULL, NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CManagedSynchronizedInputProvider::StartListening
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CManagedSynchronizedInputProvider::StartListening(_In_ UIAXcp::SynchronizedInputType inputType)
{
    Automation::CValue val;
    val.SetEnum(inputType);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PISynchronizedInput, 1, 1, &val, NULL));

    return S_OK;
}

HRESULT CManagedCustomNavigationProvider::NavigateCustom(_In_ UIAXcp::AutomationNavigationDirection direction, _Outptr_result_maybenull_ CAutomationPeer** ppReturnAP, _Outptr_result_maybenull_ IUnknown** ppReturnIREPSAsUnk)
{
    *ppReturnAP = nullptr;
    *ppReturnIREPSAsUnk = nullptr;

    Automation::CValue val;
    Automation::CValue param;
    param.SetEnum(direction);
    void* pDataArray[2];
    val.m_pvValue = (void*)pDataArray;
    val.SetArrayElementCount(2);
    IFC_RETURN(FxCallbacks::AutomationPeer_UIAPatternInvoke(m_pInteropObject, UIAXcp::PICustomNavigation, 0, 1, &param, &val));

    ::CDependencyObject* pAutomationPeer = static_cast<::CDependencyObject*>(pDataArray[0]);
    if (pAutomationPeer)
    {
        *ppReturnAP = do_pointer_cast<CAutomationPeer>(pAutomationPeer);
    }
    *ppReturnIREPSAsUnk = static_cast<IUnknown*>(pDataArray[1]);

    // We should not have both AP or IREPS return value set
    ASSERT(*ppReturnAP == nullptr || *ppReturnIREPSAsUnk == nullptr);

    return S_OK;
}
