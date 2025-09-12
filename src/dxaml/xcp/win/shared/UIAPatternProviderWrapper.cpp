// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutomationCValue.h"
#include <RootScale.h>

BSTR SysAllocStringFromCValue(_In_ Automation::CValue& value)
{
    xephemeral_string_ptr strValue;
    value.AsEphemeralString(strValue);
    return SysAllocString(strValue.GetBuffer());
}

void CUIAPatternProviderWrapper::Invalidate()
{
    m_pUIAProvider = nullptr;
}

void CUIAPatternProviderWrapper::InvalidateWrapper()
{
    if (m_pUIAProvider)
    {
        m_pUIAProvider->InvalidateUIAWrapper();
    }
}

void CUIAPatternProviderWrapper::SetWrapper()
{
    if (m_pUIAProvider)
    {
        m_pUIAProvider->SetUIAWrapper(this);
    }
}

//------------------------------------------------------------------------
//
//  Method:   CUIAInvokeProviderWrapper::Invoke
//
//  Synopsis:
//      Invoke Function
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAInvokeProviderWrapper::Invoke()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(InvokeImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAInvokeProviderWrapper::InvokeImpl
//
//  Synopsis:
//      Invoke Function
//
//------------------------------------------------------------------------
HRESULT CUIAInvokeProviderWrapper::InvokeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }
    return m_pProvider->Invoke();
}

//------------------------------------------------------------------------
//
//  Method:   CUIADockProviderWrapper::get_DockPosition
//
//  Synopsis:
//      Returns the Position of the Dock
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIADockProviderWrapper::get_DockPosition(_Out_ ::DockPosition *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_DockPositionImpl(0, nullptr, &retCValue));
    *pRetVal = static_cast<::DockPosition>(retCValue.m_nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADockProviderWrapper::get_DockPositionImpl
//
//  Synopsis:
//      Returns the Position of the Dock
//
//------------------------------------------------------------------------
HRESULT CUIADockProviderWrapper::get_DockPositionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    UIAXcp::DockPosition dockPosition;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_DockPosition(&dockPosition));
    pRetVal->SetEnum(dockPosition);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADockProviderWrapper::SetDockPosition
//
//  Synopsis:
//      Sets the Position of the Dock
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIADockProviderWrapper::SetDockPosition(_In_ ::DockPosition dockPosition)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;
    value.SetEnum(dockPosition);
    IFC_RETURN(SetDockPositionImpl(1, &value, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADockProviderWrapper::SetDockPositionImpl
//
//  Synopsis:
//      Sets the Position of the Dock
//
//------------------------------------------------------------------------
HRESULT CUIADockProviderWrapper::SetDockPositionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1)
    {
        return E_INVALIDARG;
    }

    IFCPTR_RETURN(pValue);
    auto dockPosition = static_cast<UIAXcp::DockPosition>(pValue->m_nValue);

    IFC_RETURN(m_pProvider->SetDockPosition(dockPosition));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAExpandCollapseProviderWrapper::Collapse
//
//  Synopsis:
//      UIA ExpandCollapseProvider Wrapper for Collapse
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAExpandCollapseProviderWrapper::Collapse()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(CollapseImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAExpandCollapseProviderWrapper::CollapseImpl
//
//  Synopsis:
//      UIA ExpandCollapseProvider Wrapper for CollapseImpl
//
//------------------------------------------------------------------------
HRESULT CUIAExpandCollapseProviderWrapper::CollapseImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->Collapse();
}

//------------------------------------------------------------------------
//
//  Method:   CUIAExpandCollapseProviderWrapper::Expand
//
//  Synopsis:
//      UIA ExpandCollapseProvider Wrapper for Expand
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAExpandCollapseProviderWrapper::Expand()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(ExpandImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAExpandCollapseProviderWrapper::ExpandImpl
//
//  Synopsis:
//      UIA ExpandCollapseProvider Wrapper for ExpandImpl
//
//------------------------------------------------------------------------
HRESULT CUIAExpandCollapseProviderWrapper::ExpandImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->Expand();
}

//------------------------------------------------------------------------
//
//  Method:   CUIAExpandCollapseProviderWrapper::get_ExpandCollapseState
//
//  Synopsis:
//      UIA ExpandCollapseProvider Wrapper
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAExpandCollapseProviderWrapper::get_ExpandCollapseState(_Out_ ::ExpandCollapseState *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_ExpandCollapseStateImpl(0, NULL, &retCValue));
    *pRetVal = static_cast<::ExpandCollapseState>(retCValue.m_nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAExpandCollapseProviderWrapper::get_ExpandCollapseStateImpl
//
//  Synopsis:
//      UIA ExpandCollapserovider Wrapper for getting ExpandCollapseState
//
//------------------------------------------------------------------------
HRESULT CUIAExpandCollapseProviderWrapper::get_ExpandCollapseStateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    UIAXcp::ExpandCollapseState expandCollapseState;

    IFC_RETURN(m_pProvider->get_ExpandCollapseState(&expandCollapseState));
    pRetVal->SetEnum(expandCollapseState);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper::get_Column
//
//  Synopsis:
//      Returns the column specified
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAGridItemProviderWrapper::get_Column(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_ColumnImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.m_iValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper::get_ColumnImpl
//
//  Synopsis:
//      Returns the column specified
//
//------------------------------------------------------------------------
HRESULT CUIAGridItemProviderWrapper::get_ColumnImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 column;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_Column(&column));
    pRetVal->SetSigned(column);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper::get_ColumnSpan
//
//  Synopsis:
//      Returns the amount column spanned
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAGridItemProviderWrapper::get_ColumnSpan(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_ColumnSpanImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.m_iValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper::get_ColumnSpanImpl
//
//  Synopsis:
//      Returns the amount column spanned
//
//------------------------------------------------------------------------
HRESULT CUIAGridItemProviderWrapper::get_ColumnSpanImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 column;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_ColumnSpan(&column));
    pRetVal->SetSigned(column);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper::get_ContainingGrid
//
//  Synopsis:
//      Returns the grid that this item is in
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAGridItemProviderWrapper::get_ContainingGrid(_Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_ContainingGridImpl(0, nullptr, &retCValue));

    if (retCValue.m_pvValue)
    {
        xref_ptr<CUIAWrapper> wrapper;
        IFC_RETURN(GetUIAWindow()->CreateProviderForAP(static_cast<CAutomationPeer*>(retCValue.AsObject()), wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(pRetVal)));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper::get_ContainingGridImpl
//
//  Synopsis:
//      Returns the grid that this item is in
//
//------------------------------------------------------------------------
HRESULT CUIAGridItemProviderWrapper::get_ContainingGridImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    CAutomationPeer* pAP = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_ContainingGrid(&pAP));
    pRetVal->WrapObjectNoRef(pAP);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper::get_Row
//
//  Synopsis:
//      Returns the row specified
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAGridItemProviderWrapper::get_Row(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_RowImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsSigned();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper::get_RowImpl
//
//  Synopsis:
//      Returns the row specified
//
//------------------------------------------------------------------------
HRESULT CUIAGridItemProviderWrapper::get_RowImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 row;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_Row(&row));
    pRetVal->SetSigned(row);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper::get_RowSpan
//
//  Synopsis:
//      Returns the rows this item spans over
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAGridItemProviderWrapper::get_RowSpan(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_RowSpanImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsSigned();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper::get_RowSpanImpl
//
//  Synopsis:
//      Returns the rows this item spans over
//
//------------------------------------------------------------------------
HRESULT CUIAGridItemProviderWrapper::get_RowSpanImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 row;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_RowSpan(&row));
    pRetVal->SetSigned(row);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridProviderWrapper::get_ColumnCount
//
//  Synopsis:
//      Returns the number of columns in the grid
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAGridProviderWrapper::get_ColumnCount(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_ColumnCountImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsSigned();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridProviderWrapper::get_ColumnCountImpl
//
//  Synopsis:
//      Returns the number of columns in the grid
//
//------------------------------------------------------------------------
HRESULT CUIAGridProviderWrapper::get_ColumnCountImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 column;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_ColumnCount(&column));
    pRetVal->SetSigned(column);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridProviderWrapper::GetItem
//
//  Synopsis:
//      Returns the item specified in the grid
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAGridProviderWrapper::GetItem(_In_ int row, _In_ int column, _Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value[2] = {};

    value[0].SetSigned(row);
    value[1].SetSigned(column);

    Automation::CValue retCValue;

    IFC_RETURN(GetItemImpl(2, value, &retCValue));

    if (retCValue.m_pvValue)
    {
        xref_ptr<CUIAWrapper> wrapper;
        IFC_RETURN(GetUIAWindow()->CreateProviderForAP((CAutomationPeer*)(retCValue.m_pvValue), wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(pRetVal)));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridProviderWrapper::GetItemImpl
//
//  Synopsis:
//      Returns the item specified in the grid
//
//------------------------------------------------------------------------
HRESULT CUIAGridProviderWrapper::GetItemImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    CAutomationPeer* pAP = nullptr;

    if (cValue != 2 || pValue == nullptr)
    {
        return E_INVALIDARG;
    }
    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->GetItem(pValue[0].m_iValue, pValue[1].m_iValue, &pAP));
    pRetVal->m_pvValue = pAP;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridProviderWrapper::get_RowCount
//
//  Synopsis:
//      Returns the number of rows in the grid
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAGridProviderWrapper::get_RowCount(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_RowCountImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsSigned();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAGridProviderWrapper::get_RowCountImpl
//
//  Synopsis:
//      Returns the number of rows in the grid
//
//------------------------------------------------------------------------
HRESULT CUIAGridProviderWrapper::get_RowCountImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 row;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_RowCount(&row));
    pRetVal->SetSigned(row);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAMultipleViewProviderWrapper::get_CurrentView
//
//  Synopsis:
//      Return the id for the current view
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAMultipleViewProviderWrapper::get_CurrentView(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_CurrentViewImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsSigned();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAMultipleViewProviderWrapper::get_CurrentViewImpl
//
//  Synopsis:
//      Return the id for the current view
//
//------------------------------------------------------------------------
HRESULT CUIAMultipleViewProviderWrapper::get_CurrentViewImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 viewId;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_CurrentView(&viewId));
    pRetVal->SetSigned(viewId);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAMultipleViewProviderWrapper::GetSupportedViews
//
//  Synopsis:
//      Return the id for the current view
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAMultipleViewProviderWrapper::GetSupportedViews(_Out_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    XINT32* pIntArray = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetSupportedViewsImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pIntArray = static_cast<XINT32*>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_I4, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(&(pIntArray[i]))));
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pIntArray;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAMultipleViewProviderWrapper::GetSupportedViewsImpl
//
//  Synopsis:
//      Return the id for the current view
//
//------------------------------------------------------------------------
HRESULT CUIAMultipleViewProviderWrapper::GetSupportedViewsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XUINT32 cRetVal = 0;
    XINT32* pIntArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetSupportedViews(&cRetVal, &pIntArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pIntArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAMultipleViewProviderWrapper::GetViewName
//
//  Synopsis:
//      Return the name of the view with id
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAMultipleViewProviderWrapper::GetViewName(_In_ int viewId, _Out_ BSTR *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    Automation::CValue value;

    value.SetSigned(viewId);

    IFC_RETURN(GetViewNameImpl(1, &value, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAMultipleViewProviderWrapper::GetViewNameImpl
//
//  Synopsis:
//      Return the name of the view with id
//
//------------------------------------------------------------------------
HRESULT CUIAMultipleViewProviderWrapper::GetViewNameImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetViewName(pValue[0].m_iValue, &strString));
    pRetVal->SetString(strString);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAMultipleViewProviderWrapper::SetCurrentView
//
//  Synopsis:
//      Sets the current view
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAMultipleViewProviderWrapper::SetCurrentView(_In_ int viewId)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;
    value.SetSigned(viewId);

    IFC_RETURN(SetCurrentViewImpl(1, &value, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAMultipleViewProviderWrapper::SetCurrentView
//
//  Synopsis:
//      Sets the current view
//
//------------------------------------------------------------------------
HRESULT CUIAMultipleViewProviderWrapper::SetCurrentViewImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pValue);

    IFC_RETURN(m_pProvider->SetCurrentView(pValue->m_nValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::IsReadOnly
//
//  Synopsis:
//      Gets a value that specifies whether the value of a control is read
//      only.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIARangeValueProviderWrapper::get_IsReadOnly(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_IsReadOnlyImpl(0, nullptr, &retCValue));
    *pRetVal = (retCValue.m_nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_IsReadOnlyImpl
//
//  Synopsis:
//      CUIARangeValueProviderWrapper for get_IsReadOnlyImpl
//
//------------------------------------------------------------------------
HRESULT CUIARangeValueProviderWrapper::get_IsReadOnlyImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 readonly;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_IsReadOnly(&readonly));
    pRetVal->SetSigned(readonly);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_LargeChange
//
//  Synopsis:
//      Gets the value that is added to or subtracted from the
//      IRangeValueProvider::Value property when a large change is made,
//      such as with the PAGE DOWN key
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIARangeValueProviderWrapper::get_LargeChange(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_LargeChangeImpl(0, nullptr, &retCValue));
    IFC_RETURN(retCValue.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_LargeChangeImpl
//
//  Synopsis:
//      Gets the value that is added to or subtracted from the
//      IRangeValueProvider::Value property when a large change is made,
//      such as with the PAGE DOWN key
//
//------------------------------------------------------------------------
HRESULT CUIARangeValueProviderWrapper::get_LargeChangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    DOUBLE largeChange;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_LargeChange(&largeChange));
    pRetVal->SetDouble(largeChange);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_Maximum
//
//  Synopsis:
//      Gets the maximum range value supported by the control.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIARangeValueProviderWrapper::get_Maximum(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_MaximumImpl(0, nullptr, &retCValue));
    IFC_RETURN(retCValue.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_MaximumImpl
//
//  Synopsis:
//      Gets the maximum range value supported by the control.
//
//------------------------------------------------------------------------
HRESULT CUIARangeValueProviderWrapper::get_MaximumImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    DOUBLE maximum;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_Maximum(&maximum));
    pRetVal->SetDouble(maximum);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_Minimum
//
//  Synopsis:
//      Gets the minimum range value supported by the control.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIARangeValueProviderWrapper::get_Minimum(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_MinimumImpl(0, nullptr, &retCValue));
    IFC_RETURN(retCValue.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_MinimumImpl
//
//  Synopsis:
//      Gets the minimum range value supported by the control.
//
//------------------------------------------------------------------------
HRESULT CUIARangeValueProviderWrapper::get_MinimumImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    DOUBLE minimum;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_Minimum(&minimum));
    pRetVal->SetDouble(minimum);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::SetValue
//
//  Synopsis:
//      Sets the value of the control.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIARangeValueProviderWrapper::SetValue(_In_ double val)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;
    value.SetFloat((float)val);

    IFC_RETURN(SetValueImpl(1, &value, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::SetValueImpl
//
//  Synopsis:
//      Gets the minimum range value supported by the control.
//
//------------------------------------------------------------------------
HRESULT CUIARangeValueProviderWrapper::SetValueImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1 || pValue == nullptr)
    {
        return E_INVALIDARG;
    }

    IFC_RETURN(m_pProvider->SetValue(pValue[0].m_eValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_SmallChange
//
//  Synopsis:
//      Gets the value that is added to or subtracted from the
//      IRangeValueProvider::Value property when a small change is made,
//      such as with an arrow key.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIARangeValueProviderWrapper::get_SmallChange(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_SmallChangeImpl(0, nullptr, &retCValue));
    IFC_RETURN(retCValue.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_SmallChangeImpl
//
//  Synopsis:
//      Gets the value that is added to or subtracted from the
//      IRangeValueProvider::Value property when a small change is made,
//      such as with an arrow key.
//
//------------------------------------------------------------------------
HRESULT CUIARangeValueProviderWrapper::get_SmallChangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    DOUBLE smallChange;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_SmallChange(&smallChange));
    pRetVal->SetDouble(smallChange);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_Value
//
//  Synopsis:
//      Gets the value of the control.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIARangeValueProviderWrapper::get_Value(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_ValueImpl(0, nullptr, &retCValue));
    IFC_RETURN(retCValue.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper::get_ValueImpl
//
//  Synopsis:
//      Gets the value of the control.
//
//------------------------------------------------------------------------
HRESULT CUIARangeValueProviderWrapper::get_ValueImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    DOUBLE value;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_Value(&value));
    pRetVal->SetDouble(value);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollItemProviderWrapper::ScrollIntoView
//
//  Synopsis:
//      Scrolls current item into view
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAScrollItemProviderWrapper::ScrollIntoView()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(ScrollIntoViewImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollItemProviderWrapper::ScrollIntoViewImpl
//
//  Synopsis:
//      Scrolls current item into view
//
//------------------------------------------------------------------------
HRESULT CUIAScrollItemProviderWrapper::ScrollIntoViewImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->ScrollIntoView();
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_HorizontallyScrollable
//
//  Synopsis:
//      Returns whether the control can be scrolled horizontally
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAScrollProviderWrapper::get_HorizontallyScrollable(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_HorizontallyScrollableImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_HorizontallyScrollableImpl
//
//  Synopsis:
//      Returns whether the control can be scrolled horizontally
//
//------------------------------------------------------------------------
HRESULT CUIAScrollProviderWrapper::get_HorizontallyScrollableImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 scrollable;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_HorizontallyScrollable(&scrollable));
    pRetVal->SetBool(!!scrollable);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_HorizontalScrollPercent
//
//  Synopsis:
//      Returns the scroll percentage horizontally
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAScrollProviderWrapper::get_HorizontalScrollPercent(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_HorizontalScrollPercentImpl(0, nullptr, &retCValue));
    IFC_RETURN(retCValue.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_HorizontalScrollPercentImpl
//
//  Synopsis:
//      Returns the scroll percentage horizontally
//
//------------------------------------------------------------------------
HRESULT CUIAScrollProviderWrapper::get_HorizontalScrollPercentImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    DOUBLE percent;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_HorizontalScrollPercent(&percent));
    pRetVal->SetDouble(percent);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_HorizontalViewSize
//
//  Synopsis:
//      Returns the scroll size horizontally
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAScrollProviderWrapper::get_HorizontalViewSize(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_HorizontalViewSizeImpl(0, nullptr, &retCValue));
    IFC_RETURN(retCValue.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_HorizontalViewSizeImpl
//
//  Synopsis:
//      Returns the scroll size horizontally
//
//------------------------------------------------------------------------
HRESULT CUIAScrollProviderWrapper::get_HorizontalViewSizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    DOUBLE size;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_HorizontalViewSize(&size));
    pRetVal->SetDouble(size);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::Scroll
//
//  Synopsis:
//      Scroll by the amount specified
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAScrollProviderWrapper::Scroll(_In_::ScrollAmount horizontalAmount, _In_ ::ScrollAmount verticalAmount)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue values[2] = {};
    values[0].SetEnum(horizontalAmount);
    values[1].SetEnum(verticalAmount);

    IFC_RETURN(ScrollImpl(_countof(values), values, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::ScrollImpl
//
//  Synopsis:
//      Scroll by the amount specified
//
//------------------------------------------------------------------------
HRESULT CUIAScrollProviderWrapper::ScrollImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 2 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(m_pProvider->Scroll((UIAXcp::ScrollAmount)(pValue[0].m_nValue), (UIAXcp::ScrollAmount)(pValue[1].m_nValue)));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::SetScrollPercent
//
//  Synopsis:
//      Set the percent of the scroll
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAScrollProviderWrapper::SetScrollPercent(_In_ double horizontalPercent, _In_ double verticalPercent)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue values[2] = {};
    values[0].SetFloat((float)horizontalPercent);
    values[1].SetFloat((float)verticalPercent);

    IFC_RETURN(SetScrollPercentImpl(_countof(values), values, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::SetScrollPercentImpl
//
//  Synopsis:
//      Set the percent of the scroll
//
//------------------------------------------------------------------------
HRESULT CUIAScrollProviderWrapper::SetScrollPercentImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 2 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(m_pProvider->SetScrollPercent(pValue[0].m_eValue, pValue[1].m_eValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_VerticallyScrollable
//
//  Synopsis:
//      Returns whether the control is scrollable vertically
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAScrollProviderWrapper::get_VerticallyScrollable(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_VerticallyScrollableImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_VerticallyScrollableImpl
//
//  Synopsis:
//      Returns whether the control is scrollable vertically
//
//------------------------------------------------------------------------
HRESULT CUIAScrollProviderWrapper::get_VerticallyScrollableImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 scrollable;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_VerticallyScrollable(&scrollable));
    pRetVal->SetBool(!!scrollable);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_VerticalScrollPercent
//
//  Synopsis:
//      Returns the scroll percentage vertically
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAScrollProviderWrapper::get_VerticalScrollPercent(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_VerticalScrollPercentImpl(0, nullptr, &retCValue));
    IFC_RETURN(retCValue.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_VerticalScrollPercentImpl
//
//  Synopsis:
//      Returns the scroll percentage vertically
//
//------------------------------------------------------------------------
HRESULT CUIAScrollProviderWrapper::get_VerticalScrollPercentImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    DOUBLE percent;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_VerticalScrollPercent(&percent));
    pRetVal->SetDouble(percent);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_VerticalViewSize
//
//  Synopsis:
//      Returns the scroll size vertically
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAScrollProviderWrapper::get_VerticalViewSize(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_VerticalViewSizeImpl(0, nullptr, &retCValue));
    IFC_RETURN(retCValue.GetDouble(*pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper::get_VerticalViewSize
//
//  Synopsis:
//      Returns the scroll size vertically
//
//------------------------------------------------------------------------
HRESULT CUIAScrollProviderWrapper::get_VerticalViewSizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    DOUBLE size;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_VerticalViewSize(&size));
    pRetVal->SetDouble(size);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper::AddToSelection
//
//  Synopsis:
//      Adds the item to a selection
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIASelectionItemProviderWrapper::AddToSelection()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(AddToSelectionImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper::AddToSelectionImpl
//
//  Synopsis:
//      Adds the item to a selection
//
//------------------------------------------------------------------------
HRESULT CUIASelectionItemProviderWrapper::AddToSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->AddToSelection();
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper::get_IsSelected
//
//  Synopsis:
//      Returns if this item is selected
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIASelectionItemProviderWrapper::get_IsSelected(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_IsSelectedImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper::get_IsSelectedImpl
//
//  Synopsis:
//      Returns if this item is selected
//
//------------------------------------------------------------------------
HRESULT CUIASelectionItemProviderWrapper::get_IsSelectedImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 isSelected;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_IsSelected(&isSelected));
    pRetVal->SetBool(!!isSelected);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper::RemoveFromSelection
//
//  Synopsis:
//      Removes the item from selection
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIASelectionItemProviderWrapper::RemoveFromSelection()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(RemoveFromSelectionImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper::RemoveFromSelectionImpl
//
//  Synopsis:
//      Removes the item from selection
//
//------------------------------------------------------------------------
HRESULT CUIASelectionItemProviderWrapper::RemoveFromSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->RemoveFromSelection();
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper::Select
//
//  Synopsis:
//      Select the item and deselect all others
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIASelectionItemProviderWrapper::Select()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(SelectImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper::SelectImpl
//
//  Synopsis:
//      Select the item and deselect all others
//
//------------------------------------------------------------------------
HRESULT CUIASelectionItemProviderWrapper::SelectImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->Select();
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper::get_SelectionContainer
//
//  Synopsis:
//      Returns the container for this selection
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIASelectionItemProviderWrapper::get_SelectionContainer(_Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal)
{
    xref_ptr<CUIAWrapper> wrapper;
    IRawElementProviderSimple* pFrag = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_SelectionContainerImpl(0, nullptr, &retCValue));

    if (retCValue.m_pvValue)
    {
        IFC_RETURN(GetUIAWindow()->CreateProviderForAP((CAutomationPeer*)(retCValue.m_pvValue), wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(&pFrag)));
    }

    *pRetVal = pFrag;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper::get_SelectionContainerImpl
//
//  Synopsis:
//      Returns the container for this selection
//
//------------------------------------------------------------------------
HRESULT CUIASelectionItemProviderWrapper::get_SelectionContainerImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    CAutomationPeer* pAP = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_SelectionContainer(&pAP));
    pRetVal->m_pvValue = pAP;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionProviderWrapper::get_CanSelectMultiple
//
//  Synopsis:
//      Returns if it can select multiple items
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIASelectionProviderWrapper::get_CanSelectMultiple(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_CanSelectMultipleImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionProviderWrapper::get_CanSelectMultipleImpl
//
//  Synopsis:
//      Returns if it can select multiple items
//
//------------------------------------------------------------------------
HRESULT CUIASelectionProviderWrapper::get_CanSelectMultipleImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 canSelectMultiple;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_CanSelectMultiple(&canSelectMultiple));
    pRetVal->SetBool(!!canSelectMultiple);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionProviderWrapper::GetSelection
//
//  Synopsis:
//      Returns the current selection of items
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIASelectionProviderWrapper::GetSelection(_Out_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    CAutomationPeer** pAPArray = nullptr;
    CUIAWrapper* pWrapper = nullptr;
    IUnknown* pFrag = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetSelectionImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pAPArray = static_cast<CAutomationPeer**>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFC(GetUIAWindow()->CreateProviderForAP(pAPArray[i], &pWrapper));
        IFC(pWrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pFrag)));
        ReleaseInterfaceNoNULL(pWrapper);
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(pFrag)));
        ReleaseInterface(pWrapper);
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pAPArray;
    ReleaseInterface(pWrapper);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionProviderWrapper::GetSelectionImpl
//
//  Synopsis:
//      Returns the current selection of items
//
//------------------------------------------------------------------------
HRESULT CUIASelectionProviderWrapper::GetSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    CAutomationPeer** pAPArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetSelection(&cRetVal, &pAPArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pAPArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionProviderWrapper::get_IsSelectionRequired
//
//  Synopsis:
//      Returns if something must be selected
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIASelectionProviderWrapper::get_IsSelectionRequired(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_IsSelectionRequiredImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionProviderWrapper::get_IsSelectionRequiredImpl
//
//  Synopsis:
//      Returns if something must be selected
//
//------------------------------------------------------------------------
HRESULT CUIASelectionProviderWrapper::get_IsSelectionRequiredImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 isSelectionRequired;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_IsSelectionRequired(&isSelectionRequired));
    pRetVal->SetBool(!!isSelectionRequired);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATableItemProviderWrapper::GetColumnHeaderItems
//
//  Synopsis:
//      Returns the column headers
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATableItemProviderWrapper::GetColumnHeaderItems(_Out_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    CAutomationPeer** pAPArray = nullptr;
    CUIAWrapper* pWrapper = nullptr;
    IUnknown* pFrag = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetColumnHeaderItemsImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pAPArray = static_cast<CAutomationPeer**>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFC(GetUIAWindow()->CreateProviderForAP(pAPArray[i], &pWrapper));
        IFC(pWrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pFrag)));
        ReleaseInterfaceNoNULL(pWrapper);
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(pFrag)));
        ReleaseInterface(pWrapper);
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pAPArray;
    ReleaseInterface(pWrapper);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATableItemProviderWrapper::GetColumnHeaderItemsImpl
//
//  Synopsis:
//      Returns the column headers
//
//------------------------------------------------------------------------
HRESULT CUIATableItemProviderWrapper::GetColumnHeaderItemsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    CAutomationPeer** pAPArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetColumnHeaderItems(&cRetVal, &pAPArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pAPArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATableItemProviderWrapper::GetRowHeaderItems
//
//  Synopsis:
//      Returns the row headers
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATableItemProviderWrapper::GetRowHeaderItems(_Out_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    CAutomationPeer** pAPArray = nullptr;
    CUIAWrapper* pWrapper = nullptr;
    IUnknown* pFrag = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetRowHeaderItemsImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pAPArray = static_cast<CAutomationPeer**>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFC(GetUIAWindow()->CreateProviderForAP(pAPArray[i], &pWrapper));
        IFC(pWrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pFrag)));
        ReleaseInterfaceNoNULL(pWrapper);
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(pFrag)));
        ReleaseInterface(pWrapper);
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pAPArray;
    ReleaseInterface(pWrapper);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATableItemProviderWrapper::GetRowHeaderItemsImpl
//
//  Synopsis:
//      Returns the row headers
//
//------------------------------------------------------------------------
HRESULT CUIATableItemProviderWrapper::GetRowHeaderItemsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    CAutomationPeer** pAPArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetRowHeaderItems(&cRetVal, &pAPArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pAPArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATableProviderWrapper::GetColumnHeaders
//
//  Synopsis:
//      Returns the column headers
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATableProviderWrapper::GetColumnHeaders(_Out_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    CAutomationPeer** pAPArray = nullptr;
    CUIAWrapper* pWrapper = nullptr;
    IUnknown* pFrag = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetColumnHeadersImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pAPArray = static_cast<CAutomationPeer**>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, length));
    IFCOOM(safeArray.release());
    for (LONG i = 0; i < length; i++)
    {
        IFC(GetUIAWindow()->CreateProviderForAP(pAPArray[i], &pWrapper));
        IFC(pWrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pFrag)));
        ReleaseInterfaceNoNULL(pWrapper);
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(pFrag)));
        ReleaseInterface(pWrapper);
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pAPArray;
    ReleaseInterface(pWrapper);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATableProviderWrapper::GetColumnHeadersImpl
//
//  Synopsis:
//      Returns the column headers
//
//------------------------------------------------------------------------
HRESULT CUIATableProviderWrapper::GetColumnHeadersImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    CAutomationPeer** pAPArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetColumnHeaders(&cRetVal, &pAPArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pAPArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATableProviderWrapper::GetRowHeaders
//
//  Synopsis:
//      Returns the row headers
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATableProviderWrapper::GetRowHeaders(_Out_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    CAutomationPeer** pAPArray = nullptr;
    CUIAWrapper* pWrapper = nullptr;
    IUnknown* pFrag = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetRowHeadersImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pAPArray = static_cast<CAutomationPeer**>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFC(GetUIAWindow()->CreateProviderForAP(pAPArray[i], &pWrapper));
        IFC(pWrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pFrag)));
        ReleaseInterfaceNoNULL(pWrapper);
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(pFrag)));
        ReleaseInterface(pWrapper);
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pAPArray;
    ReleaseInterface(pWrapper);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATableProviderWrapper::GetRowHeadersImpl
//
//  Synopsis:
//      Returns the row headers
//
//------------------------------------------------------------------------
HRESULT CUIATableProviderWrapper::GetRowHeadersImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    CAutomationPeer** pAPArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetRowHeaders(&cRetVal, &pAPArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pAPArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATableProviderWrapper::get_RowOrColumnMajor
//
//  Synopsis:
//      Returns if it's row or column major
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATableProviderWrapper::get_RowOrColumnMajor(_Out_ ::RowOrColumnMajor *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_RowOrColumnMajorImpl(0, nullptr, &retCValue));
    *pRetVal = (::RowOrColumnMajor)(retCValue.m_nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATableProviderWrapper::get_RowOrColumnMajorImpl
//
//  Synopsis:
//      Returns if it's row or column major
//
//------------------------------------------------------------------------
HRESULT CUIATableProviderWrapper::get_RowOrColumnMajorImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    UIAXcp::RowOrColumnMajor rowOrColumnMajor;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_RowOrColumnMajor(&rowOrColumnMajor));
    pRetVal->SetEnum(rowOrColumnMajor);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAToggleProviderWrapper::Toggle
//
//  Synopsis:
//      Toggles the control
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAToggleProviderWrapper::Toggle()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(ToggleImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAToggleProviderWrapper::ToggleImpl
//
//  Synopsis:
//      Toggles the control
//
//------------------------------------------------------------------------
HRESULT CUIAToggleProviderWrapper::ToggleImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }
    return m_pProvider->Toggle();
}

//------------------------------------------------------------------------
//
//  Method:   CUIAToggleProviderWrapper::get_ToggleState
//
//  Synopsis:
//      Returns the toggle state
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAToggleProviderWrapper::get_ToggleState(_Out_ ::ToggleState *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_ToggleStateImpl(0, nullptr, &retCValue));
    *pRetVal = (::ToggleState)(retCValue.m_nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAToggleProviderWrapper::get_ToggleStateImpl
//
//  Synopsis:
//      Returns the toggle state
//
//------------------------------------------------------------------------
HRESULT CUIAToggleProviderWrapper::get_ToggleStateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    UIAXcp::ToggleState toggleState;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_ToggleState(&toggleState));
    pRetVal->SetEnum(toggleState);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::get_CanMove
//
//  Synopsis:
//      Gets a value that specifies whether the value of a control can move.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::get_CanMove(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_CanMoveImpl(0, nullptr, &retCValue));
    *pRetVal = (retCValue.m_nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::get_CanMove
//
//  Synopsis:
//      Gets a value that specifies whether the value of a control can move.
//
//------------------------------------------------------------------------
HRESULT CUIATransformProviderWrapper::get_CanMoveImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 canMove;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_CanMove(&canMove));
    pRetVal->SetSigned(canMove);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::CanResize
//
//  Synopsis:
//      Gets a value that specifies whether the value of a control can resize.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::get_CanResize(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_CanResizeImpl(0, nullptr, &retCValue));
    *pRetVal = (retCValue.m_nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::CanResize
//
//  Synopsis:
//      Gets a value that specifies whether the value of a control can resize.
//
//------------------------------------------------------------------------
HRESULT CUIATransformProviderWrapper::get_CanResizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 canResize;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_CanResize(&canResize));
    pRetVal->SetSigned(canResize);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::CanRotate
//
//  Synopsis:
//      Gets a value that specifies whether the value of a control can rotate.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::get_CanRotate(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_CanRotateImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.m_nValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::CanRotate
//
//  Synopsis:
//      Gets a value that specifies whether the value of a control can rotate.
//
//------------------------------------------------------------------------
HRESULT CUIATransformProviderWrapper::get_CanRotateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 canRotate;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_CanRotate(&canRotate));
    pRetVal->SetSigned(canRotate);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::Move
//
//  Synopsis:
//      Moves the control by specified amount
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::Move(_In_ double x, _In_ double y)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue values[2] = {};

    values[0].SetFloat((float)x);
    values[1].SetFloat((float)y);

    IFC_RETURN(MoveImpl(_countof(values), values, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::MoveImpl
//
//  Synopsis:
//      Moves the control by specified amount
//
//------------------------------------------------------------------------
HRESULT CUIATransformProviderWrapper::MoveImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 2 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(m_pProvider->Move(pValue[0].m_eValue, pValue[1].m_eValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::Resize
//
//  Synopsis:
//      Resizes the control by specified amount
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::Resize(_In_ double width, _In_ double height)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue values[2] = {};
    values[0].SetFloat((float)width);
    values[1].SetFloat((float)height);

    IFC_RETURN(ResizeImpl(_countof(values), values, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::ResizeImpl
//
//  Synopsis:
//      Resizes the control by specified amount
//
//------------------------------------------------------------------------
HRESULT CUIATransformProviderWrapper::ResizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 2 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(m_pProvider->Resize(pValue[0].m_eValue, pValue[1].m_eValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::Rotate
//
//  Synopsis:
//      Rotates the control by specified amount
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::Rotate(_In_ double degree)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;
    value.SetFloat((float)degree);

    IFC_RETURN(RotateImpl(1, &value, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::RotateImpl
//
//  Synopsis:
//      Rotates the control by specified amount
//
//------------------------------------------------------------------------
HRESULT CUIATransformProviderWrapper::RotateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(m_pProvider->Rotate(pValue[0].m_eValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::get_CanZoom
//
//  Synopsis:
//      Gets a value that specifies whether the value of a control can zoom.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::get_CanZoom(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_CanZoomImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsBool();

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::get_CanZoomImpl
//
//  Synopsis:
//      Gets a value that specifies whether the value of a control can zoom.
//
//------------------------------------------------------------------------
HRESULT CUIATransformProviderWrapper::get_CanZoomImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 canMove;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_CanZoom(&canMove));
    pRetVal->SetBool(!!canMove);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::get_ZoomLevel
//
//  Synopsis:
//      Gets a value that specifies the value zoom.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::get_ZoomLevel(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid() || !IsPatternValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(m_pProvider->get_ZoomLevel(pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::get_ZoomMaximum
//
//  Synopsis:
//      Gets a value that specifies the value max zoom.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::get_ZoomMaximum(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid() || !IsPatternValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(m_pProvider->get_ZoomMaximum(pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::get_ZoomMinimum
//
//  Synopsis:
//      Gets a value that specifies the value min zoom.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::get_ZoomMinimum(_Out_ double *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    if (!IsWindowValid() || !IsPatternValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(m_pProvider->get_ZoomMinimum(pRetVal));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::Zoom
//
//  Synopsis:
//      Zoom the control by specified amount
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::Zoom(_In_ double zoom)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue cValue;
    cValue.SetFloat((float)zoom);

    IFC_RETURN(ZoomImpl(1, &cValue, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::ZoomImpl
//
//  Synopsis:
//      Zoom the control by specified amount
//
//------------------------------------------------------------------------
HRESULT CUIATransformProviderWrapper::ZoomImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(m_pProvider->Zoom(pValue->m_eValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::ZoomByUnit
//
//  Synopsis:
//      Zoom the control by specified unit
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATransformProviderWrapper::ZoomByUnit(_In_ ::ZoomUnit zoomUnit)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue cValue;
    cValue.SetEnum(zoomUnit);

    IFC_RETURN(ZoomByUnitImpl(1, &cValue, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper::ZoomByUnitImpl
//
//  Synopsis:
//      Zoom the control by specified unit
//
//------------------------------------------------------------------------
HRESULT CUIATransformProviderWrapper::ZoomByUnitImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(m_pProvider->ZoomByUnit((UIAXcp::ZoomUnit)(pValue->m_nValue)));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAValueProviderWrapper::get_IsReadOnly
//
//  Synopsis:
//      Returns if this is a read only value
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAValueProviderWrapper::get_IsReadOnly(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_IsReadOnlyImpl(0, nullptr, &retCValue));

    *pRetVal = (retCValue.m_nValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAValueProviderWrapper::get_IsReadOnlyImpl
//
//  Synopsis:
//      Returns if this is a read only value
//
//------------------------------------------------------------------------
HRESULT CUIAValueProviderWrapper::get_IsReadOnlyImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 readonly;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_IsReadOnly(&readonly));
    pRetVal->SetSigned(readonly);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAValueProviderWrapper::SetValue
//
//  Synopsis:
//      Sets the value
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAValueProviderWrapper::SetValue(_In_ LPCWSTR val)
{
    Automation::CValue value;
    xstring_ptr strString;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    {
        XINT32 length = SysStringLen((BSTR)val);

        XStringBuilder stringBuilder;

        IFC_RETURN(stringBuilder.Initialize(length));

        IFC_RETURN(stringBuilder.Append(val, length));

        // Note: XStringBuilder strings are always null-terminated
        IFC_RETURN(stringBuilder.DetachString(&strString));
    }

    value.SetString(strString);

    IFC_RETURN(SetValueImpl(1, &value, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAValueProviderWrapper::SetValueImpl
//
//  Synopsis:
//      Sets the value
//
//------------------------------------------------------------------------
HRESULT CUIAValueProviderWrapper::SetValueImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IFCPTR_RETURN(pValue);
    IFC_RETURN(m_pProvider->SetValue(pValue->AsString()));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAValueProviderWrapper::get_Value
//
//  Synopsis:
//      Gets the value
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAValueProviderWrapper::get_Value(_Out_ BSTR *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_ValueImpl(0, nullptr, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAValueProviderWrapper::get_ValueImpl
//
//  Synopsis:
//      Gets the value
//
//------------------------------------------------------------------------
HRESULT CUIAValueProviderWrapper::get_ValueImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_Value(&strString));
    pRetVal->SetString(strString);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::Close
//
//  Synopsis:
//      Closes the window
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindowProviderWrapper::Close()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(CloseImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::CloseImpl
//
//  Synopsis:
//      Closes the window
//
//------------------------------------------------------------------------
HRESULT CUIAWindowProviderWrapper::CloseImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }
    return m_pProvider->Close();
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_IsModal
//
//  Synopsis:
//      Returns if the window is modal
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindowProviderWrapper::get_IsModal(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_IsModalImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsBool();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_IsModalImpl
//
//  Synopsis:
//      Returns if the window is modal
//
//------------------------------------------------------------------------
HRESULT CUIAWindowProviderWrapper::get_IsModalImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 isModal;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_IsModal(&isModal));
    pRetVal->SetBool(!!isModal);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_IsTopmost
//
//  Synopsis:
//      Returns if the window is top most
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindowProviderWrapper::get_IsTopmost(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_IsTopmostImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsBool();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_IsTopmostImpl
//
//  Synopsis:
//      Returns if the window is top most
//
//------------------------------------------------------------------------
HRESULT CUIAWindowProviderWrapper::get_IsTopmostImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 isTopmost;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_IsTopmost(&isTopmost));
    pRetVal->SetBool(!!isTopmost);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_CanMaximize
//
//  Synopsis:
//      Returns if the window can be maximized
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindowProviderWrapper::get_CanMaximize(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_CanMaximizeImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsBool();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_CanMaximizeImpl
//
//  Synopsis:
//      Returns if the window can be maximized
//
//------------------------------------------------------------------------
HRESULT CUIAWindowProviderWrapper::get_CanMaximizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 CanMaximize;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_Maximizable(&CanMaximize));
    pRetVal->SetBool(!!CanMaximize);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_CanMinimize
//
//  Synopsis:
//      Returns if the window can be minimized
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindowProviderWrapper::get_CanMinimize(_Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_CanMinimizeImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsBool();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_CanMinimizeImpl
//
//  Synopsis:
//      Returns if the window can be minimized
//
//------------------------------------------------------------------------
HRESULT CUIAWindowProviderWrapper::get_CanMinimizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 CanMinimize;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_Minimizable(&CanMinimize));
    pRetVal->SetBool(!!CanMinimize);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::SetVisualState
//
//  Synopsis:
//      Sets the visual state of the window
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindowProviderWrapper::SetVisualState(_In_ ::WindowVisualState WindowVisualState)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;

    value.SetEnum(WindowVisualState);

    IFC_RETURN(SetVisualStateImpl(1, &value, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::SetVisualStateImpl
//
//  Synopsis:
//      Sets the visual state of the window
//
//------------------------------------------------------------------------
HRESULT CUIAWindowProviderWrapper::SetVisualStateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pValue);
    IFC_RETURN(m_pProvider->SetVisualState((UIAXcp::WindowVisualState)(pValue->m_nValue)));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::WaitForInputIdle
//
//  Synopsis:
//      Forces the window to suspend and wait for input
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindowProviderWrapper::WaitForInputIdle(_In_ int milliseconds, _Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;
    value.SetSigned(milliseconds);

    Automation::CValue retCValue;
    IFC_RETURN(WaitForInputIdleImpl(1, &value, &retCValue));

    *pRetVal = retCValue.AsBool();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::WaitForInputIdleImpl
//
//  Synopsis:
//      Forces the window to suspend and wait for input
//
//------------------------------------------------------------------------
HRESULT CUIAWindowProviderWrapper::WaitForInputIdleImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 retVal;

    if (cValue != 1)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pValue);

    IFC_RETURN(m_pProvider->WaitForInputIdle(pValue->m_nValue, &retVal));
    pRetVal->SetBool(!!retVal);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_WindowInteractionState
//
//  Synopsis:
//      Returns the window interaction state
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindowProviderWrapper::get_WindowInteractionState(_Out_ ::WindowInteractionState *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_WindowInteractionStateImpl(0, nullptr, &retCValue));
    *pRetVal = static_cast<::WindowInteractionState>(retCValue.AsEnum());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_WindowInteractionStateImpl
//
//  Synopsis:
//      Returns the window interaction state
//
//------------------------------------------------------------------------
HRESULT CUIAWindowProviderWrapper::get_WindowInteractionStateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    UIAXcp::WindowInteractionState WindowInteractionState;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_WindowInteractionState(&WindowInteractionState));
    pRetVal->SetEnum(WindowInteractionState);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_WindowVisualState
//
//  Synopsis:
//      Returns the window visual state
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindowProviderWrapper::get_WindowVisualState(_Out_ ::WindowVisualState *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_WindowVisualStateImpl(0, nullptr, &retCValue));
    *pRetVal = static_cast<::WindowVisualState>(retCValue.AsEnum());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper::get_WindowVisualStateImpl
//
//  Synopsis:
//      Returns the window visual state
//
//------------------------------------------------------------------------
HRESULT CUIAWindowProviderWrapper::get_WindowVisualStateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    UIAXcp::WindowVisualState WindowVisualState;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_WindowVisualState(&WindowVisualState));
    pRetVal->SetEnum(WindowVisualState);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::GetSelection
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextProviderWrapper::GetSelection(_Out_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    IUnknown* pWrapper = nullptr;
    IUIATextRangeProvider** pUIATextRangeArray = nullptr;
    ITextRangeProvider* pTextRangeProvider = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    retCValue.m_pvValue = nullptr;
    XINT32 length = 0;

    IFC(GetSelectionImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pUIATextRangeArray = static_cast<IUIATextRangeProvider**>(retCValue.m_pvValue);

    // Create the safe array for ITextRangeProvider interface
    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFCPTR(pUIATextRangeArray[i]);
        // Create the UIATextRangeProviderWrapper object to interact with UIAutomation
        IFC(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeArray[i]->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                &pWrapper));
        IFC(pWrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(&pTextRangeProvider)));
        ReleaseInterfaceNoNULL(pWrapper);
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(pTextRangeProvider)));
        ReleaseInterface(pWrapper);
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pUIATextRangeArray;
    ReleaseInterface(pWrapper);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::GetSelectionImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextProviderWrapper::GetSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XUINT32 cRetVal = 0;
    IUIATextRangeProvider** pTextRangeArray = nullptr;

    IFCPTR_RETURN(pRetVal);

    // Call the provider GetSelection() functionality
    IFC_RETURN(m_pProvider->GetSelection(&cRetVal, &pTextRangeArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pTextRangeArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::GetVisibleRanges
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextProviderWrapper::GetVisibleRanges(_Out_ SAFEARRAY** pRetVal)
{
    HRESULT hr = S_OK;

    IUnknown* pWrapper = nullptr;
    IUIATextRangeProvider** pTextRangeArray = nullptr;
    ITextRangeProvider* pTextRangeProvider = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetVisibleRangesImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pTextRangeArray = static_cast<IUIATextRangeProvider**>(retCValue.m_pvValue);

    // Create the safe array for ITextRangeProvider interface
    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFCPTR(pTextRangeArray[i]);
        IFC(CUIATextRangeProviderWrapper::Create(
                pTextRangeArray[i]->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                &pWrapper));
        IFC(pWrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(&pTextRangeProvider)));
        ReleaseInterfaceNoNULL(pWrapper);
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(pTextRangeProvider)));
        ReleaseInterface(pWrapper);
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pTextRangeArray;
    ReleaseInterface(pWrapper);
    return hr;

}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::GetVisibleRangesImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextProviderWrapper::GetVisibleRangesImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XUINT32 cRetVal = 0;
    IUIATextRangeProvider** pTextRangeArray = nullptr;

    IFCPTR_RETURN(pRetVal);

    // Call the provider GetVisibleRanges() functionality
    IFC_RETURN(m_pProvider->GetVisibleRanges(&cRetVal, &pTextRangeArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pTextRangeArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::RangeFromChild
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextProviderWrapper::RangeFromChild(
    _In_ IRawElementProviderSimple *childElement,
    _Out_ ITextRangeProvider **pRetVal)
{
    xref_ptr<IUnknown> unkWrapper;
    ITextRangeProvider *pTextRangeProvider = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;

    Automation::CValue retCValue;
    CUIAWrapper *pElementWrapper = (CUIAWrapper*)childElement;

    // Casting childElement to CUIAWrapper is not safe.
    // Check if it's really a CUIAWrapper and not a CUIAHostWindow (otherwise it will crash)
    xref_ptr<CUIAWrapper> wrapper;
    IFC_RETURN(childElement->QueryInterface(__uuidof(CUIAWrapper), reinterpret_cast<void**>(wrapper.ReleaseAndGetAddressOf())));

    // This is not a safe cast and will crash if it's not a CUIWrapper
    value.WrapObjectNoRef(static_cast<CDependencyObject*>((CDependencyObject*)pElementWrapper->GetAP()));

    IFC_RETURN(RangeFromChildImpl(1, &value, &retCValue));

    // Create the UIATextRangeProviderWrapper to communicate with UIAutomation
    if (IUIATextRangeProvider *pUIATextRangeProvider = (IUIATextRangeProvider*)retCValue.m_pvValue)
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeProvider->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                unkWrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(unkWrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(&pTextRangeProvider)));
    }

    *pRetVal = pTextRangeProvider;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::RangeFromChildImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextProviderWrapper::RangeFromChildImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IUIATextRangeProvider* pTextRange = nullptr;

    if (cValue != 1 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pRetVal);

    // Call RangeFromChild() from TextPattern through UIAPatternProvider
    IFC_RETURN(m_pProvider->RangeFromChild((CAutomationPeer*)pValue[0].m_pdoValue, &pTextRange));
    pRetVal->m_pvValue = pTextRange;

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::RangeFromPoint
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextProviderWrapper::RangeFromPoint(_In_ UiaPoint screenLocation, _Out_ ITextRangeProvider **pRetVal)
{
    XPOINTF pointLocation;
    xref_ptr<IUnknown> wrapper;
    ITextRangeProvider *pTextRangeProvider = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;
    Automation::CValue retCValue;

    POINT ptClient;
    ptClient.x = (LONG)screenLocation.x;
    ptClient.y = (LONG)screenLocation.y;

    // Transform to the client coordinate point from the screen point
    VERIFY(GetUIAWindow()->TransformScreenToClient(GetUIAWindow()->GetHostEnvironmentInfo(), &ptClient));

    pointLocation.x = (XFLOAT)ptClient.x;
    pointLocation.y = (XFLOAT)ptClient.y;

    // As Automation::CValue doesn't own the Point, call WrapPoint instead of SetPoint.
    value.WrapPoint(&pointLocation);

    IFC_RETURN(RangeFromPointImpl(1, &value, &retCValue));

    // Create UIATextRangeProviderWrapper for UIAutomation
    if (IUIATextRangeProvider *pUIATextRangeProvider = (IUIATextRangeProvider*)retCValue.m_pvValue)
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeProvider->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(&pTextRangeProvider)));
    }

    *pRetVal = pTextRangeProvider;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::RangeFromPointImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextProviderWrapper::RangeFromPointImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IUIATextRangeProvider* pTextRange = nullptr;

    if (cValue != 1 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pRetVal);

    // Call RangeFromPoint()
    IFC_RETURN(m_pProvider->RangeFromPoint((XPOINTF *)pValue[0].m_peValue, &pTextRange));
    pRetVal->m_pvValue = pTextRange;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::get_DocumentRange
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextProviderWrapper::get_DocumentRange(_Out_ ITextRangeProvider **pRetVal)
{
    xref_ptr<IUnknown> wrapper;
    IUIATextRangeProvider *pUIATextRangeProvider = nullptr;
    ITextRangeProvider *pTextRangeProvider = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_DocumentRangeImpl(0, nullptr, &retCValue));

    pUIATextRangeProvider = (IUIATextRangeProvider*)retCValue.m_pvValue;
    if (pUIATextRangeProvider)
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeProvider->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(&pTextRangeProvider)));
    }

    *pRetVal = pTextRangeProvider;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::get_DocumentRangeImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextProviderWrapper::get_DocumentRangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IUIATextRangeProvider* pTextRange = nullptr;

    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->get_DocumentRange(&pTextRange));
    pRetVal->m_pvValue = pTextRange;

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::get_SupportedTextSelection
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextProviderWrapper::get_SupportedTextSelection(_Out_ ::SupportedTextSelection  *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_SupportedTextSelectionImpl(0, nullptr, &retCValue));
    *pRetVal = static_cast<::SupportedTextSelection>(retCValue.AsEnum());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::get_SupportedTextSelectionImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextProviderWrapper::get_SupportedTextSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    UIAXcp::SupportedTextSelection supportedTextSelection;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_SupportedTextSelection(&supportedTextSelection));
    pRetVal->SetEnum(supportedTextSelection);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::RangeFromAnnotation
//
//  Synopsis:
//      Finds the TextRange from Annotation
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextProviderWrapper::RangeFromAnnotation(
    _In_opt_ IRawElementProviderSimple *annotationElement,
    _Outptr_result_maybenull_ ITextRangeProvider **pRetVal)
{
    xref_ptr<IUnknown> unkWrapper;
    ITextRangeProvider *pTextRangeProvider = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;
    Automation::CValue retCValue;
    CUIAWrapper *pElementWrapper = (CUIAWrapper*)annotationElement;

    // Casting annotationElement to CUIAWrapper may not be safe
    xref_ptr<CUIAWrapper> wrapper;
    IFC_RETURN(annotationElement->QueryInterface(__uuidof(CUIAWrapper), reinterpret_cast<void**>(wrapper.ReleaseAndGetAddressOf())));

    value.WrapObjectNoRef(static_cast<CDependencyObject*>((CDependencyObject*)pElementWrapper->GetAP()));

    IFC_RETURN(RangeFromAnnotationImpl(1, &value, &retCValue));

    // Create the UIATextRangeProviderWrapper to communicate with UIAutomation
    if (IUIATextRangeProvider *pUIATextRangeProvider = (IUIATextRangeProvider*)retCValue.m_pvValue)
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeProvider->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                unkWrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(unkWrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(&pTextRangeProvider)));
    }

    *pRetVal = pTextRangeProvider;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::RangeFromAnnotationImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextProviderWrapper::RangeFromAnnotationImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IUIATextRangeProvider* pTextRange = nullptr;

    if (cValue != 1 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pRetVal);

    // Call RangeFromAnnotation() from TextPattern2 through UIAPatternProvider
    IFC_RETURN(m_pProvider->RangeFromAnnotation((CAutomationPeer*)pValue[0].m_pdoValue, &pTextRange));
    pRetVal->m_pvValue = pTextRange;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::GetCaretRange
//
//  Synopsis:
//      Finds the Caret Range
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextProviderWrapper::GetCaretRange(
    _Out_ BOOL *isActive,
    _Outptr_result_maybenull_ ITextRangeProvider **pRetVal)
{
    xref_ptr<IUnknown> wrapper;
    ITextRangeProvider *pTextRangeProvider = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(GetCaretRangeImpl(0, nullptr, &retCValue));

    // Create the UIATextRangeProviderWrapper to communicate with UIAutomation
    if (IUIATextRangeProvider *pUIATextRangeProvider = (IUIATextRangeProvider*)retCValue.m_pvValue)
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeProvider->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(&pTextRangeProvider)));
    }

    *pRetVal = pTextRangeProvider;
    *isActive = retCValue.AsBool();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper::GetCaretRangeImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextProviderWrapper::GetCaretRangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 isActive;
    IUIATextRangeProvider* pTextRange = nullptr;

    IFCPTR_RETURN(pRetVal);

    // Call GetCaretRange() from TextPattern2 through UIAPatternProvider
    IFC_RETURN(m_pProvider->GetCaretRange(&isActive, &pTextRange));
    pRetVal->SetBool(!!isActive);
    pRetVal->m_pvValue = pTextRange;

    return S_OK;
}

HRESULT CUIATextRangeProviderWrapper::QueryInterface(REFIID riid, void** ppInterface)
{
    XINT32 isITextRangeProvider2 = 0;
    if (riid == __uuidof(IUnknown))
    {
        *ppInterface = static_cast<ITextRangeProvider2*>(this);
    }
    else if (riid == GUID_UIATextRangeProviderWrapper)
    {
        *ppInterface = static_cast<ITextRangeProvider2*>(this);
    }
    else if (riid == __uuidof(ITextRangeProvider))
    {
        *ppInterface = static_cast<ITextRangeProvider*>(this);
    }
    else if (riid == __uuidof(ITextRangeProvider2))
    {
        IFC_RETURN(m_pProvider->IsITextRangeProvider2(&isITextRangeProvider2));
        if (isITextRangeProvider2)
        {
            *ppInterface = static_cast<ITextRangeProvider2*>(this);
        }
        else
        {
            *ppInterface = nullptr;
            return E_NOINTERFACE;
        }
    }
    else
    {
        *ppInterface = nullptr;
        return E_NOINTERFACE;
    }

    static_cast<IUnknown*>(*ppInterface)->AddRef();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::AddToSelection
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::AddToSelection()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(AddToSelectionImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::AddToSelectionImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::AddToSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->AddToSelection();
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::Clone
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::Clone(_Out_ ITextRangeProvider **pRetVal)
{
    xref_ptr<IUnknown> wrapper;
    ITextRangeProvider *pTextRangeProvider = nullptr;
    IUIATextRangeProvider *pUIATextRangeProvider = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(CloneImpl(0, nullptr, &retCValue));

    pUIATextRangeProvider = (IUIATextRangeProvider*)retCValue.m_pvValue;
    if (pUIATextRangeProvider)
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeProvider->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(&pTextRangeProvider)));
    }

    *pRetVal = pTextRangeProvider;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::CloneImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::CloneImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IUIATextRangeProvider *pTextRange = nullptr;

    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->Clone(&pTextRange));
    pRetVal->m_pvValue = pTextRange;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::Compare
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::Compare(
    _In_ ITextRangeProvider *textRange,
    _Out_ BOOL *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    Automation::CValue value;

    wrl::ComPtr<CUIATextRangeProviderWrapper> spProviderWrapper;
    textRange->QueryInterface(GUID_UIATextRangeProviderWrapper, reinterpret_cast<void**>(spProviderWrapper.ReleaseAndGetAddressOf()));
    if (!spProviderWrapper || !(spProviderWrapper->IsPatternValid()))
    {
        *pRetVal = FALSE;
        return E_INVALIDARG;
    }

    value.WrapObjectNoRef(static_cast<CDependencyObject*>((spProviderWrapper.Get()->m_pProvider->GetTextRangeProviderObject())));

    IFC_RETURN(CompareImpl(1, &value, &retCValue));
    *pRetVal = retCValue.AsBool();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::CompareImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::CompareImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    bool isSameTextRange;

    if (cValue != 1 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->Compare((IUIATextRangeProvider*)pValue[0].m_pdoValue, &isSameTextRange));
    pRetVal->SetBool(isSameTextRange);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::CompareEndpoints
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::CompareEndpoints(
    _In_ ::TextPatternRangeEndpoint endpoint,
    _In_ ITextRangeProvider *targetRange,
    _In_ ::TextPatternRangeEndpoint targetEndpoint,
    _Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    wrl::ComPtr<CUIATextRangeProviderWrapper> spProviderWrapper;
    targetRange->QueryInterface(GUID_UIATextRangeProviderWrapper, reinterpret_cast<void**>(spProviderWrapper.ReleaseAndGetAddressOf()));
    if (!spProviderWrapper || !(spProviderWrapper->IsPatternValid()))
    {
        return E_INVALIDARG;
    }

    Automation::CValue values[3] = {};
    Automation::CValue retCValue;

    values[0].SetEnum(endpoint);
    values[1].WrapObjectNoRef((CDependencyObject*)(CDependencyObject *)(spProviderWrapper.Get()->m_pProvider->GetTextRangeProviderObject()));
    values[2].SetEnum(targetEndpoint);

    IFC_RETURN(CompareEndpointsImpl(3, values, &retCValue));
    *pRetVal = retCValue.AsSigned();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::CompareEndpointsImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::CompareEndpointsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 compareResult;

    if (cValue != 3 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->CompareEndpoints(
        (UIAXcp::TextPatternRangeEndpoint)pValue[0].m_nValue,
        (IUIATextRangeProvider*)pValue[1].m_pdoValue,
        (UIAXcp::TextPatternRangeEndpoint)pValue[2].m_nValue,
        &compareResult));

    pRetVal->SetSigned(compareResult);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::ExpandToEnclosingUnit
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::ExpandToEnclosingUnit(_In_ ::TextUnit unit)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;
    value.SetEnum(unit);

    IFC_RETURN(ExpandToEnclosingUnitImpl(1, &value, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::ExpandToEnclosingUnitImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::ExpandToEnclosingUnitImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pValue);
    IFC_RETURN(m_pProvider->ExpandToEnclosingUnit((UIAXcp::TextUnit)(pValue->m_nValue)));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::FindAttribute
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::FindAttribute(
    _In_ int attributeId,
    _In_ VARIANT value,
    _In_ BOOL backward,
    _Out_ ITextRangeProvider **pRetVal)
{
    SuspendFailFastOnStowedException suspender(E_NOTIMPL);

    xref_ptr<IUnknown> spWrapper;
    xref_ptr<ITextRangeProvider> spTextRangeProvider;
    CValue temp;

    // UIA is not translating the following objects to provider (XAML) wrappers
    // This needs to be fixed in UIA before supporting these
    if (attributeId == UIA_AnnotationObjectsAttributeId ||
        attributeId == UIA_LinkAttributeId)
    {
        return E_NOT_SUPPORTED;
    }

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    Automation::CValue values[3] = {};

    values[0].SetSigned(attributeId);

    IFC_RETURN(GetUIAWindow()->VariantToCValue(&value, &temp));
    IFC_RETURN(values[1].ConvertFrom(temp));

    values[2].SetBool(backward);

    IFC_RETURN(FindAttributeImpl(_countof(values), values, &retCValue));

    if (IUIATextRangeProvider *pUIATextRangeProvider = static_cast<IUIATextRangeProvider*>(retCValue.m_pvValue))
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeProvider->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                spWrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(spWrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(spTextRangeProvider.ReleaseAndGetAddressOf())));
    }

    *pRetVal = spTextRangeProvider.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::FindAttributeImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::FindAttributeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IUIATextRangeProvider *pTextRange = nullptr;
    CValue temp;

    if (cValue != 3 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(pValue[1].ConvertTo(temp));

    IFC_RETURN(m_pProvider->FindAttribute(
        pValue[0].m_iValue,
        temp,
        !!pValue[2].m_nValue,
        &pTextRange));

    pRetVal->m_pvValue = pTextRange;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::FindText
//
//  Synopsis:
//
//------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable: 26007) // supperessing the bstr conversion to WCHAR* warning for buffer. OACR doesn't understand BSTR well enough.
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::FindText(
    _In_ BSTR text,
    _In_ BOOL backward,
    _In_ BOOL ignoreCase,
    _Out_ ITextRangeProvider **pRetVal)
{
    SuspendFailFastOnStowedException suspender(E_NOT_SUPPORTED);

    xstring_ptr strString;
    xref_ptr<IUnknown> wrapper;
    ITextRangeProvider *pTextRangeProvider = nullptr;
    IUIATextRangeProvider *pUIATextRangeProvider = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    Automation::CValue values[3] = {};

    IFC_RETURN(xstring_ptr::CloneBuffer(text, SysStringLen(text), &strString));

    values[0].SetString(strString);
    values[1].SetBool(backward);
    values[2].SetBool(ignoreCase);

    IFC_RETURN(FindTextImpl(_countof(values), values, &retCValue));

    pUIATextRangeProvider = (IUIATextRangeProvider*)retCValue.m_pvValue;
    if (pUIATextRangeProvider)
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeProvider->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(&pTextRangeProvider)));
    }

    *pRetVal = pTextRangeProvider;

    return S_OK;
}
#pragma warning(pop)

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::FindTextImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::FindTextImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IUIATextRangeProvider* pTextRange = nullptr;

    if (cValue != 3 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFCPTR_RETURN(pRetVal);
    IFCPTR_RETURN(pValue);

    IFC_RETURN(m_pProvider->FindText(pValue[0].AsString(), !!pValue[1].m_nValue, !!pValue[2].m_nValue, &pTextRange));

    pRetVal->m_pvValue = pTextRange;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::GetAttributeValue
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::GetAttributeValue(_In_ int attributeId, _Out_ VARIANT *pRetVal)
{
    SuspendFailFastOnStowedException suspender(E_NOT_SUPPORTED);

    HRESULT hr = S_OK;
    Automation::CValue retCValue;
    Automation::CValue value;

    xref_ptr<IUnknown> spWrapper;
    xref_ptr<ITextRangeProvider> spTextRangeProvider;
    xref_ptr<CAutomationPeerCollection> spAutomationPeerCollection;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    value.SetSigned(attributeId);

    IFC(GetAttributeValueImpl(1, &value, &retCValue));
    if (attributeId == UIA_AnnotationObjectsAttributeId)
    {
        spAutomationPeerCollection.attach(static_cast<CAutomationPeerCollection*>(retCValue.GetValuePointer()));
        IFC(GetUIAWindow()->ConvertToVariant(spAutomationPeerCollection.get(), pRetVal));
    }
    else if (attributeId == UIA_LinkAttributeId)
    {
        CValue temp;
        IUIATextRangeProvider *pUIATextRangeProvider = static_cast<IUIATextRangeProvider*>(retCValue.m_pvValue);
        if (pUIATextRangeProvider)
        {
            IFC(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeProvider->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                spWrapper.ReleaseAndGetAddressOf()));
            IFC(spWrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(spTextRangeProvider.ReleaseAndGetAddressOf())));
        }
        value.SetIUnknownAddRef(spTextRangeProvider.detach());
        IFC(value.ConvertTo(temp));
        IFC(GetUIAWindow()->CValueToVariant(temp, pRetVal));
    }
    else
    {
        CValue temp;
        IFC(retCValue.ConvertTo(temp));
        IFC(GetUIAWindow()->CValueToVariant(temp, pRetVal));
    }

Cleanup:
    if (hr == E_NOT_SUPPORTED)
    {
        pRetVal->vt = VT_UNKNOWN;
        pRetVal->punkVal = m_punkNotSupportedValue;
        hr = S_OK;
    }
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::GetAttributeValueImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::GetAttributeValueImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    CValue temp;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetAttributeValue(pValue[0].m_iValue, &temp));
    IFC_RETURN(pRetVal->ConvertFrom(temp));

    // There's no way to represent MixedAttribute IUnkown using IInspectable, so we set the
    // type to IUnknown and value to nullptr via Empty IInspectable in DXAML layer, please see
    // TextRangeAdapter::GetAttributeValue and once callback is over we fill in the real Mixed
    // attribute in the Automation::CValue.
    if (pRetVal->m_type == valueIUnknown)
    {
        pRetVal->SetIUnknownAddRef(m_punkMixedAttribute);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::GetBoundingRectangles
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::GetBoundingRectangles(_Out_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    XDOUBLE* pDoubleArray = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetBoundingRectanglesImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pDoubleArray = static_cast<XDOUBLE*>(retCValue.m_pvValue);

    // Transform X,Y,width,height to the screen coordinate point from the client coordinate
    for (int i = 0; i < length; i+=4)
    {
        XRECTF rect;
        rect.X = static_cast<XFLOAT>(pDoubleArray[i]);
        rect.Y = static_cast<XFLOAT>(pDoubleArray[i + 1]);
        rect.Width = static_cast<XFLOAT>(pDoubleArray[i + 2]);
        rect.Height = static_cast<XFLOAT>(pDoubleArray[i + 3]);

        // Transform to the screen coordinate point from the client point
        VERIFY(GetUIAWindow()->TransformClientToScreen(GetUIAWindow()->GetHostEnvironmentInfo(), &rect));

        pDoubleArray[i] = rect.X;
        pDoubleArray[i + 1] = rect.Y;
        pDoubleArray[i + 2] = rect.Width;
        pDoubleArray[i + 3] = rect.Height;
    }

    safeArray.reset(SafeArrayCreateVector(VT_R8, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(&(pDoubleArray[i]))));
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pDoubleArray;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::GetBoundingRectanglesImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::GetBoundingRectanglesImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XUINT32 cRetVal = 0;
    XDOUBLE* pDoubleArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetBoundingRectangles(&cRetVal, &pDoubleArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pDoubleArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::GetChildren
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::GetChildren(_Out_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    CAutomationPeer** pAPArray = nullptr;
    CUIAWrapper* pWrapper = nullptr;
    IUnknown* pFrag = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetChildrenImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pAPArray = static_cast<CAutomationPeer**>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFC(GetUIAWindow()->CreateProviderForAP(pAPArray[i], &pWrapper));
        IFC(pWrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pFrag)));
        ReleaseInterfaceNoNULL(pWrapper);
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(pFrag)));
        ReleaseInterface(pWrapper);
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pAPArray;
    ReleaseInterface(pWrapper);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::GetChildrenImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::GetChildrenImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    CAutomationPeer** pAPArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetChildren(&cRetVal, &pAPArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pAPArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::GetEnclosingElement
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::GetEnclosingElement(_Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal)
{
    *pRetVal = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(GetEnclosingElementImpl(0, nullptr, &retCValue));

    if (retCValue.m_pvValue)
    {
        xref_ptr<CUIAWrapper> wrapper;
        IFC_RETURN(GetUIAWindow()->CreateProviderForAP((CAutomationPeer*)(retCValue.m_pvValue), wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(pRetVal)));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::GetEnclosingElementImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::GetEnclosingElementImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    CAutomationPeer* pAP = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetEnclosingElement(&pAP));
    pRetVal->m_pvValue = pAP;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::GetText
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::GetText(_In_ int maxLength, _Out_ BSTR *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    Automation::CValue value;

    value.SetSigned(maxLength);

    IFC_RETURN(GetTextImpl(1, &value, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::GetTextImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::GetTextImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetText(pValue[0].m_iValue, &strString));
    pRetVal->SetString(strString);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::Move
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::Move(
    _In_ ::TextUnit unit,
    _In_ int count,
    _Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue values[2] = {};
    values[0].SetEnum(unit);
    values[1].SetSigned(count);

    Automation::CValue retCValue;
    IFC_RETURN(MoveImpl(_countof(values), values, &retCValue));

    *pRetVal = retCValue.AsSigned();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::MoveImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::MoveImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 movedUnit;

    if (cValue != 2 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->Move(
        (UIAXcp::TextUnit) pValue[0].m_nValue,
        pValue[1].m_nValue,
        &movedUnit));

    pRetVal->SetSigned(movedUnit);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::MoveEndpointByRange
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::MoveEndpointByRange(
    _In_ ::TextPatternRangeEndpoint endpoint,
    _In_ ITextRangeProvider *targetRange,
    _In_ ::TextPatternRangeEndpoint targetEndpoint)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }
    wrl::ComPtr<CUIATextRangeProviderWrapper> spProviderWrapper;
    targetRange->QueryInterface(GUID_UIATextRangeProviderWrapper, reinterpret_cast<void**>(spProviderWrapper.ReleaseAndGetAddressOf()));
    if (!spProviderWrapper || !(spProviderWrapper->IsPatternValid()))
    {
        return E_INVALIDARG;
    }

    Automation::CValue values[3] = {};

    values[0].SetEnum(endpoint);
    values[1].WrapObjectNoRef((CDependencyObject*)(CDependencyObject *)(spProviderWrapper.Get()->m_pProvider->GetTextRangeProviderObject()));
    values[2].SetEnum(targetEndpoint);

    IFC_RETURN(MoveEndpointByRangeImpl(_countof(values), values, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::MoveEndpointByRangeImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::MoveEndpointByRangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 3 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(m_pProvider->MoveEndpointByRange(
        (UIAXcp::TextPatternRangeEndpoint) pValue[0].m_nValue,
        (IUIATextRangeProvider*)pValue[1].m_pdoValue,
        (UIAXcp::TextPatternRangeEndpoint) pValue[2].m_nValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::MoveEndpointByUnit
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::MoveEndpointByUnit(
    _In_ ::TextPatternRangeEndpoint endpoint,
    _In_ ::TextUnit unit,
    _In_ int count,
    _Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue values[3] = {};
    values[0].SetEnum(endpoint);
    values[1].SetEnum(unit);
    values[2].SetSigned(count);

    Automation::CValue retCValue;
    IFC_RETURN(MoveEndpointByUnitImpl(_countof(values), values, &retCValue));

    *pRetVal = retCValue.AsSigned();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::MoveEndpointByUnitImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::MoveEndpointByUnitImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 movedUnit;

    if (cValue != 3 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->MoveEndpointByUnit(
        (UIAXcp::TextPatternRangeEndpoint) pValue[0].m_nValue,
        (UIAXcp::TextUnit) pValue[1].m_nValue,
        pValue[2].m_nValue,
        &movedUnit));

    pRetVal->SetSigned(movedUnit);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::RemoveFromSelection
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::RemoveFromSelection()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(RemoveFromSelectionImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::RemoveFromSelectionImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::RemoveFromSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->RemoveFromSelection();
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::ScrollIntoView
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::ScrollIntoView(_In_ BOOL alignToTop)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;

    value.SetBool(alignToTop);
    IFC_RETURN(ScrollIntoViewImpl(1, &value, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::ScrollIntoViewImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::ScrollIntoViewImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pValue);

    IFC_RETURN(m_pProvider->ScrollIntoView(!!pValue->m_nValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::Select
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::Select()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(SelectImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::ShowContextMenuImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::ShowContextMenuImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->ShowContextMenu();
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::ShowContextMenu
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextRangeProviderWrapper::ShowContextMenu()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    return ShowContextMenuImpl(0, nullptr, nullptr);
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper::SelectImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextRangeProviderWrapper::SelectImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->Select();
}

//------------------------------------------------------------------------
//
//  Method:   CUIAItemContainerProviderWrapper::FindItemByProperty
//
//  Synopsis:
//      Finds an Item depending upon the Property matching
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAItemContainerProviderWrapper::FindItemByProperty(
    _In_ IRawElementProviderSimple *pStartAfter,
    _In_ XINT32 propertyId,
    _In_ VARIANT val,
    _Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal)
{
    xref_ptr<CUIAWrapper> wrapper;
    IRawElementProviderSimple* pFrag = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    Automation::CValue values[3] = {};

    CUIAWrapper *pElementWrapper = (CUIAWrapper*)pStartAfter;
    if (pElementWrapper)
    {
        values[0].WrapObjectNoRef((CDependencyObject*)(CDependencyObject *)reinterpret_cast<CDependencyObject*>(pElementWrapper->GetAP()));
    }
    else
    {
        values[0].SetNull();
    }
    UIAXcp::APAutomationProperties ePropertyType = GetUIAWindow()->ConvertIdToAPAutomationProperties(propertyId);
    values[1].SetSigned(ePropertyType);
    if (ePropertyType == UIAXcp::APAutomationControlTypeProperty)
    {
        IFCEXPECT_RETURN(val.vt == VT_I4);
        UIAXcp::APAutomationControlType eControlType = GetUIAWindow()->ConvertIdToAPAutomationControlType(val.intVal);
        values[2].SetSigned(eControlType);
    }
    else
    {
        CValue temp;
        IFC_RETURN(GetUIAWindow()->VariantToCValue(&val, &temp));
        IFC_RETURN(values[2].ConvertFrom(temp));
    }

    IFC_RETURN(FindItemByPropertyImpl(_countof(values), values, &retCValue));

    if (retCValue.m_pvValue)
    {
        IFC_RETURN(GetUIAWindow()->CreateProviderForAP((CAutomationPeer*)(retCValue.m_pvValue), wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(&pFrag)));
        *pRetVal = pFrag;
    }
    else
    {
        *pRetVal = nullptr;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAItemContainerProviderWrapper::FindItemByPropertyImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIAItemContainerProviderWrapper::FindItemByPropertyImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    CAutomationPeer* pAP = nullptr;
    CValue temp;

    if (cValue != 3 || pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(pValue[2].ConvertTo(temp));

    if (pValue[0].m_type == valueObject)
    {
        IFC_RETURN(m_pProvider->FindItemByProperty(
        (CAutomationPeer*)pValue[0].m_pdoValue,
        (UIAXcp::APAutomationProperties) pValue[1].m_iValue,
        temp,
        &pAP));
    }
    else
    {
        IFC_RETURN(m_pProvider->FindItemByProperty(
        nullptr,
        (UIAXcp::APAutomationProperties) pValue[1].m_iValue,
        temp,
        &pAP));
    }

    pRetVal->m_pvValue = pAP;

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CUIAVirtualizedItemProviderWrapper::Realize
//
//  Synopsis:
//      Realizes a current item into view if it's virtualized
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAVirtualizedItemProviderWrapper::Realize()
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(RealizeImpl(0, nullptr, nullptr));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAVirtualizedItemProviderWrapper::RealizeImpl
//
//  Synopsis:
//
//
//------------------------------------------------------------------------
HRESULT CUIAVirtualizedItemProviderWrapper::RealizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    return m_pProvider->Realize();
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextChildProviderWrapper::get_TextContainer
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextChildProviderWrapper::get_TextContainer(_Outptr_result_maybenull_ IRawElementProviderSimple **ppRetVal)
{
    *ppRetVal = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_TextContainerImpl(0, nullptr, &retCValue));

    if (retCValue.m_pvValue)
    {
        xref_ptr<CUIAWrapper> wrapper;
        IFC_RETURN(GetUIAWindow()->CreateProviderForAP((CAutomationPeer*)(retCValue.m_pvValue), wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(&ppRetVal)));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextChildProviderWrapper::get_TextContainerImpl
//
//  Synopsis: Gets the TextContainer for the pattern.
//
//------------------------------------------------------------------------
HRESULT CUIATextChildProviderWrapper::get_TextContainerImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    CAutomationPeer* pAP = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_TextContainer(&pAP));
    pRetVal->m_pvValue = pAP;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextChildProviderWrapper::get_TextRange
//
//  Synopsis: Gets the corresponding TextRange for the TextChild
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIATextChildProviderWrapper::get_TextRange(_Outptr_result_maybenull_ ITextRangeProvider **pRetVal)
{
    xref_ptr<IUnknown> wrapper;
    IUIATextRangeProvider *pUIATextRangeProvider = nullptr;
    ITextRangeProvider *pTextRangeProvider = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_TextRangeImpl(0, nullptr, &retCValue));

    pUIATextRangeProvider = (IUIATextRangeProvider*)retCValue.m_pvValue;
    if (pUIATextRangeProvider)
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
                pUIATextRangeProvider->GetUIAProvider(),
                GetUIAWindow(),
                GetUIAIds(),
                wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(&pTextRangeProvider)));
    }

    *pRetVal = pTextRangeProvider;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIATextChildProviderWrapper::get_TextRangeImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIATextChildProviderWrapper::get_TextRangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IUIATextRangeProvider* pTextRange = nullptr;

    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->get_TextRange(&pTextRange));
    pRetVal->m_pvValue = pTextRange;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CUIATextEditProviderWrapper::GetSelection(_Out_ SAFEARRAY **pRetVal)
{
    return m_spUIAInnerTextProviderWrapper->GetSelection(pRetVal);
}

HRESULT STDMETHODCALLTYPE CUIATextEditProviderWrapper::GetVisibleRanges(_Out_ SAFEARRAY **pRetVal)
{
    return m_spUIAInnerTextProviderWrapper->GetVisibleRanges(pRetVal);
}

HRESULT STDMETHODCALLTYPE CUIATextEditProviderWrapper::RangeFromChild(_In_ IRawElementProviderSimple *childElement, _Out_ ITextRangeProvider **pRetVal)
{
    return m_spUIAInnerTextProviderWrapper->RangeFromChild(childElement, pRetVal);
}

HRESULT STDMETHODCALLTYPE CUIATextEditProviderWrapper::RangeFromPoint(_In_ UiaPoint screenLocation, _Out_ ITextRangeProvider **pRetVal)
{
    return m_spUIAInnerTextProviderWrapper->RangeFromPoint(screenLocation, pRetVal);
}

HRESULT STDMETHODCALLTYPE CUIATextEditProviderWrapper::get_DocumentRange(_Out_ ITextRangeProvider **pRetVal)
{
    return m_spUIAInnerTextProviderWrapper->get_DocumentRange(pRetVal);
}

HRESULT STDMETHODCALLTYPE CUIATextEditProviderWrapper::get_SupportedTextSelection(_Out_ SupportedTextSelection  *pRetVal)
{
    return m_spUIAInnerTextProviderWrapper->get_SupportedTextSelection(pRetVal);
}

HRESULT CUIATextEditProviderWrapper::GetActiveComposition(_Outptr_result_maybenull_ ITextRangeProvider **pRetVal)
{
    xref_ptr<IUnknown> spWrapper;
    xref_ptr<ITextRangeProvider> spTextRangeProvider;
    IUIATextRangeProvider* pUIATextRangeProviderNoRef = nullptr;

    IFCCHECK_RETURN(IsWindowValid());
    IFCCHECK_RETURN(IsPatternValid());

    IFC_RETURN(m_pProvider->GetActiveComposition(&pUIATextRangeProviderNoRef));
    if (pUIATextRangeProviderNoRef)
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
            pUIATextRangeProviderNoRef->GetUIAProvider(),
            GetUIAWindow(),
            GetUIAIds(),
            spWrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(spWrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(spTextRangeProvider.ReleaseAndGetAddressOf())));
    }
    *pRetVal = spTextRangeProvider.detach();
    return S_OK;
}


HRESULT CUIATextEditProviderWrapper::GetConversionTarget(_Outptr_result_maybenull_ ITextRangeProvider **pRetVal)
{
    xref_ptr<IUnknown> spWrapper;
    xref_ptr<ITextRangeProvider> spTextRangeProvider;
    IUIATextRangeProvider* pUIATextRangeProviderNoRef = nullptr;

    IFCCHECK_RETURN(IsWindowValid());
    IFCCHECK_RETURN(IsPatternValid());

    IFC_RETURN(m_pProvider->GetConversionTarget(&pUIATextRangeProviderNoRef));
    if (pUIATextRangeProviderNoRef)
    {
        IFC_RETURN(CUIATextRangeProviderWrapper::Create(
            pUIATextRangeProviderNoRef->GetUIAProvider(),
            GetUIAWindow(),
            GetUIAIds(),
            spWrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(spWrapper->QueryInterface(__uuidof(ITextRangeProvider), reinterpret_cast<void**>(spTextRangeProvider.ReleaseAndGetAddressOf())));
    }
    *pRetVal = spTextRangeProvider.detach();
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper::get_AnnotationTypeId
//
//  Synopsis:
//      Returns the Annotation Type Id
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAAnnotationProviderWrapper::get_AnnotationTypeId(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFCPTR_RETURN(pRetVal);

    Automation::CValue retCValue;

    IFC_RETURN(get_AnnotationTypeIdImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsSigned();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper::get_AnnotationTypeIdImpl
//
//  Synopsis:
//      Returns the Annotation Type Id
//
//------------------------------------------------------------------------
HRESULT CUIAAnnotationProviderWrapper::get_AnnotationTypeIdImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 typeId;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_AnnotationTypeId(&typeId));
    pRetVal->SetSigned(typeId);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper::get_AnnotationTypeName
//
//  Synopsis:
//      Gets the annotation type name.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAAnnotationProviderWrapper::get_AnnotationTypeName(_Outptr_result_maybenull_ BSTR *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    if (!pRetVal)
    {
        return S_OK;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_AnnotationTypeNameImpl(0, nullptr, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper::get_AnnotationTypeName
//
//  Synopsis:
//      Gets the annotation type name.
//
//------------------------------------------------------------------------
HRESULT CUIAAnnotationProviderWrapper::get_AnnotationTypeNameImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_AnnotationTypeName(&strString));
    pRetVal->SetString(strString);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper::get_Author
//
//  Synopsis:
//      Gets the Author name.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAAnnotationProviderWrapper::get_Author(_Outptr_result_maybenull_ BSTR *pRetVal)
{
    Automation::CValue retCValue;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    if (!pRetVal)
    {
        return S_OK;
    }

    IFC_RETURN(get_AuthorImpl(0, nullptr, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper::get_AuthorImpl
//
//  Synopsis:
//      Gets the Author name.
//
//------------------------------------------------------------------------
HRESULT CUIAAnnotationProviderWrapper::get_AuthorImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_Author(&strString));
    pRetVal->SetString(strString);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper::get_DateTime
//
//  Synopsis:
//      Gets the Date Time.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAAnnotationProviderWrapper::get_DateTime(_Outptr_result_maybenull_ BSTR *pRetVal)
{
    Automation::CValue retCValue;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    if (!pRetVal)
    {
        return S_OK;
    }

    IFC_RETURN(get_DateTimeImpl(0, nullptr, &retCValue));
    *pRetVal = SysAllocStringFromCValue(retCValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper::get_DateTimeImpl
//
//  Synopsis:
//      Gets the Date Time.
//
//------------------------------------------------------------------------
HRESULT CUIAAnnotationProviderWrapper::get_DateTimeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    xstring_ptr strString;

    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->get_DateTime(&strString));

    pRetVal->SetString(strString);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper::get_Target
//
//  Synopsis:
//      Returns the Target of Annotation
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAAnnotationProviderWrapper::get_Target(_Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal)
{
    *pRetVal = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    if (!pRetVal)
    {
        return S_OK;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_TargetImpl(0, nullptr, &retCValue));

    if (retCValue.m_pvValue)
    {
        xref_ptr<CUIAWrapper> wrapper;
        IFC_RETURN(GetUIAWindow()->CreateProviderForAP((CAutomationPeer*)(retCValue.m_pvValue), wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(pRetVal)));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper::get_TargetImpl
//
//  Synopsis:
//      Returns the Target of Annotation
//
//------------------------------------------------------------------------
HRESULT CUIAAnnotationProviderWrapper::get_TargetImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    CAutomationPeer* pAP = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_Target(&pAP));
    pRetVal->m_pvValue = pAP;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADragProviderWrapper::get_IsGrabbed
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIADragProviderWrapper::get_IsGrabbed(_Out_ BOOL *pRetVal)
{
    IFCPTR_RETURN(pRetVal);
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_IsGrabbedImpl(0, nullptr, &retCValue));

    *pRetVal = retCValue.AsBool();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADragProviderWrapper::get_IsGrabbedImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIADragProviderWrapper::get_IsGrabbedImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    bool isGrabbed = false;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_IsGrabbed(&isGrabbed));
    pRetVal->SetBool(isGrabbed);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADragProviderWrapper::get_DropEffect
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIADragProviderWrapper::get_DropEffect(_Outptr_result_maybenull_ BSTR *pRetVal)
{
    if (!pRetVal)
    {
        return S_OK;
    }

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_DropEffectImpl(0, nullptr, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADragProviderWrapper::get_DropEffectImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIADragProviderWrapper::get_DropEffectImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_DropEffect(&strString));

    pRetVal->SetString(strString);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADragProviderWrapper::get_DropEffects
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIADragProviderWrapper::get_DropEffects(_Outptr_result_maybenull_ SAFEARRAY** pRetVal)
{
    HRESULT hr = S_OK;

    XINT32 length = 0;
    xstring_ptr* pstrArray = nullptr;
    BSTR bstr = nullptr;
    unique_safearray safeArray = nullptr;

    if (!pRetVal)
    {
        return S_OK;
    }

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    retCValue.m_pvValue = nullptr;

    IFC(get_DropEffectsImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pstrArray = static_cast<xstring_ptr*>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_BSTR, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        bstr = SysAllocString(pstrArray[i].GetBuffer());
        IFCOOM(bstr);
        IFC(SafeArrayPutElement(safeArray.get(), &i, bstr));
        bstr = nullptr;
    }
    *pRetVal = safeArray.release();

Cleanup:
    SysFreeString(bstr);
    delete[] pstrArray;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADragProviderWrapper::get_DropEffectsImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIADragProviderWrapper::get_DropEffectsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    xstring_ptr* pstrArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_DropEffects(&cRetVal, &pstrArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pstrArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADragProviderWrapper::GetGrabbedItems
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIADragProviderWrapper::GetGrabbedItems(_Outptr_result_maybenull_ SAFEARRAY * *pRetVal)
{
    HRESULT hr = S_OK;

    CAutomationPeer** pAPArray = nullptr;
    CUIAWrapper* pWrapper = nullptr;
    IUnknown* pFrag = nullptr;
    unique_safearray safeArray = nullptr;

    if (!pRetVal)
    {
        return S_OK;
    }

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetGrabbedItemsImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pAPArray = static_cast<CAutomationPeer**>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFC(GetUIAWindow()->CreateProviderForAP(pAPArray[i], &pWrapper));
        IFC(pWrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pFrag)));
        ReleaseInterfaceNoNULL(pWrapper);
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(pFrag)));
        ReleaseInterface(pWrapper);
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pAPArray;
    ReleaseInterface(pWrapper);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADragProviderWrapper::GetGrabbedItemsImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIADragProviderWrapper::GetGrabbedItemsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    CAutomationPeer** pAPArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetGrabbedItems(&cRetVal, &pAPArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pAPArray;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADropTargetProviderWrapper::get_DropTargetEffect
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIADropTargetProviderWrapper::get_DropTargetEffect( _Outptr_result_maybenull_ BSTR *pRetVal)
{
    Automation::CValue retCValue;

    if (!pRetVal)
    {
        return S_OK;
    }

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(get_DropTargetEffectImpl(0, nullptr, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADropTargetProviderWrapper::get_DropTargetEffectImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIADropTargetProviderWrapper::get_DropTargetEffectImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);

    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(m_pProvider->get_DropTargetEffect(&strString));

    pRetVal->SetString(strString);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADropTargetProviderWrapper::get_DropTargetEffects
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIADropTargetProviderWrapper::get_DropTargetEffects( _Outptr_result_maybenull_ SAFEARRAY * *pRetVal)
{
    HRESULT hr = S_OK;

    Automation::CValue retCValue;
    XINT32 length = 0;
    xstring_ptr* pstrArray = nullptr;
    BSTR bstr = nullptr;
    unique_safearray safeArray = nullptr;

    if (!pRetVal)
    {
        goto Cleanup;
    }

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    IFC(get_DropTargetEffectsImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pstrArray = static_cast<xstring_ptr*>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_BSTR, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        bstr = SysAllocString(pstrArray[i].GetBuffer());
        IFCOOM(bstr);
        IFC(SafeArrayPutElement(safeArray.get(), &i, bstr));
        bstr = nullptr;
    }
    *pRetVal = safeArray.release();

Cleanup:
    SysFreeString(bstr);
    delete[] pstrArray;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CUIADropTargetProviderWrapper::get_DropTargetEffectsImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CUIADropTargetProviderWrapper::get_DropTargetEffectsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    xstring_ptr* pstrArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_DropTargetEffects(&cRetVal, &pstrArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pstrArray;

    return S_OK;
}



HRESULT STDMETHODCALLTYPE CUIAObjectModelProviderWrapper::GetUnderlyingObjectModel(_Outptr_result_maybenull_ IUnknown **pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    if (!pRetVal)
    {
        return S_OK;
    }

    Automation::CValue retCValue;
    IFC_RETURN(GetUnderlyingObjectModelImpl(0, nullptr, &retCValue));

    *pRetVal = (IUnknown*)retCValue.m_pvValue;
    return S_OK;
}

HRESULT CUIAObjectModelProviderWrapper::GetUnderlyingObjectModelImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IUnknown* pUnknown = nullptr;

    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->GetUnderlyingObjectModel(&pUnknown));
    pRetVal->m_pvValue = pUnknown;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIASpreadsheetProviderWrapper::GetItemByName(_In_ LPCWSTR name, _Outptr_result_maybenull_ IRawElementProviderSimple **ppRetVal)
{
    *ppRetVal = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    // Why is it ok to cast an LPCWSTR to a BSTR???
    xstring_ptr strString;
    IFC_RETURN(xstring_ptr::CloneBuffer(name, SysStringLen((BSTR)name), &strString));

    Automation::CValue value;
    value.SetString(strString);

    Automation::CValue retCValue;

    IFC_RETURN(GetItemByNameImpl(1, &value, &retCValue));

    if (retCValue.m_pvValue)
    {
        xref_ptr<CUIAWrapper> wrapper;
        IFC_RETURN(GetUIAWindow()->CreateProviderForAP((CAutomationPeer*)(retCValue.m_pvValue), wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(ppRetVal)));
    }

    return S_OK;
}

HRESULT CUIASpreadsheetProviderWrapper::GetItemByNameImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    CAutomationPeer* pAP = nullptr;

    IFCPTR_RETURN(pValue);
    IFCPTR_RETURN(pRetVal);

    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(m_pProvider->GetItemByName(pValue->AsString(), &pAP));

    pRetVal->m_pvValue = pAP;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIASpreadsheetItemProviderWrapper::get_Formula(_Outptr_result_maybenull_ BSTR *pRetVal)
{
    Automation::CValue retCValue;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    if (!pRetVal)
    {
        return S_OK;
    }

    IFC_RETURN(get_FormulaImpl(0, nullptr, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);

    return S_OK;
}

HRESULT CUIASpreadsheetItemProviderWrapper::get_FormulaImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);

    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(m_pProvider->get_Formula(&strString));

    pRetVal->SetString(strString);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIASpreadsheetItemProviderWrapper::GetAnnotationObjects(_Outptr_result_maybenull_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    CAutomationPeer** pAPArray = nullptr;
    CUIAWrapper* pWrapper = nullptr;
    IUnknown* pFrag = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetAnnotationObjectsImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pAPArray = static_cast<CAutomationPeer**>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFC(GetUIAWindow()->CreateProviderForAP(pAPArray[i], &pWrapper));
        IFC(pWrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pFrag)));
        ReleaseInterfaceNoNULL(pWrapper);
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(pFrag)));
        ReleaseInterface(pWrapper);
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pAPArray;
    ReleaseInterface(pWrapper);
    return hr;
}

HRESULT CUIASpreadsheetItemProviderWrapper::GetAnnotationObjectsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    CAutomationPeer** pAPArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetAnnotationObjects(&cRetVal, &pAPArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pAPArray;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIASpreadsheetItemProviderWrapper::GetAnnotationTypes(_Outptr_result_maybenull_ SAFEARRAY **pRetVal)
{
    HRESULT hr = S_OK;

    UIAXcp::AnnotationType* pTypeArray = nullptr;
    unique_safearray safeArray = nullptr;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;
    XINT32 length = 0;

    retCValue.m_pvValue = nullptr;

    IFC(GetAnnotationTypesImpl(0, nullptr, &retCValue));

    length = static_cast<XINT32>(retCValue.GetArrayElementCount());
    pTypeArray = static_cast<UIAXcp::AnnotationType*>(retCValue.m_pvValue);
    safeArray.reset(SafeArrayCreateVector(VT_I4, 0, length));
    IFCOOM(safeArray.get());
    for (LONG i = 0; i < length; i++)
    {
        IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(&(pTypeArray[i]))));
    }
    *pRetVal = safeArray.release();

Cleanup:
    delete[] pTypeArray;
    return hr;
}

HRESULT CUIASpreadsheetItemProviderWrapper::GetAnnotationTypesImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 cRetVal = 0;
    UIAXcp::AnnotationType* pTypeArray = nullptr;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->GetAnnotationTypes(&cRetVal, &pTypeArray));
    pRetVal->SetArrayElementCount(cRetVal);
    pRetVal->m_pvValue = pTypeArray;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CUIAStylesProviderWrapper::get_ExtendedProperties(_Outptr_result_maybenull_ BSTR *pRetVal)
{
    Automation::CValue retCValue;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    if (!pRetVal)
    {
        return S_OK;
    }

    IFC_RETURN(get_ExtendedPropertiesImpl(0, nullptr, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);

    return S_OK;
}

HRESULT CUIAStylesProviderWrapper::get_ExtendedPropertiesImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);

    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(m_pProvider->get_ExtendedProperties(&strString));

    pRetVal->SetString(strString);

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CUIAStylesProviderWrapper::get_FillColor(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_FillColorImpl(0, nullptr, &retCValue));
    *pRetVal = (retCValue.m_iValue);

    return S_OK;
}

HRESULT CUIAStylesProviderWrapper::get_FillColorImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 color;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_FillColor(&color));
    pRetVal->SetSigned(color);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAStylesProviderWrapper::get_FillPatternColor(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_FillPatternColorImpl(0, nullptr, &retCValue));
    *pRetVal = (retCValue.m_iValue);

    return S_OK;
}

HRESULT CUIAStylesProviderWrapper::get_FillPatternColorImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 color;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_FillPatternColor(&color));
    pRetVal->SetSigned(color);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAStylesProviderWrapper::get_FillPatternStyle(_Outptr_result_maybenull_ BSTR *pRetVal)
{
    Automation::CValue retCValue;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    if (!pRetVal)
    {
        return S_OK;
    }

    IFC_RETURN(get_FillPatternStyleImpl(0, nullptr, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);

    return S_OK;
}

HRESULT CUIAStylesProviderWrapper::get_FillPatternStyleImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);

    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(m_pProvider->get_FillPatternStyle(&strString));

    pRetVal->SetString(strString);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAStylesProviderWrapper::get_Shape(_Outptr_result_maybenull_ BSTR *pRetVal)
{
    Automation::CValue retCValue;

    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    if (!pRetVal)
    {
        return S_OK;
    }

    IFC_RETURN(get_ShapeImpl(0, nullptr, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);

    return S_OK;
}

HRESULT CUIAStylesProviderWrapper::get_ShapeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    xstring_ptr strString;

    IFCPTR_RETURN(pRetVal);

    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IFC_RETURN(m_pProvider->get_Shape(&strString));

    pRetVal->SetString(strString);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAStylesProviderWrapper::get_StyleId(_Out_ int *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue retCValue;

    IFC_RETURN(get_StyleIdImpl(0, nullptr, &retCValue));
    *pRetVal = retCValue.AsSigned();

    return S_OK;
}

HRESULT CUIAStylesProviderWrapper::get_StyleIdImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    XINT32 style;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(m_pProvider->get_StyleId(&style));
    pRetVal->SetSigned(style);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAStylesProviderWrapper::get_StyleName(_Outptr_result_maybenull_ BSTR *pRetVal)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    if (!pRetVal)
    {
        return S_OK;
    }

    Automation::CValue retCValue;
    IFC_RETURN(get_StyleNameImpl(0, nullptr, &retCValue));

    *pRetVal = SysAllocStringFromCValue(retCValue);
    return S_OK;
}

HRESULT CUIAStylesProviderWrapper::get_StyleNameImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    xstring_ptr strString;

    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    IFCPTR_RETURN(pRetVal);

    IFC_RETURN(m_pProvider->get_StyleName(&strString));

    pRetVal->SetString(strString);

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CUIASynchronizedInputProviderWrapper::Cancel(void)
{
    return CancelImpl(0, nullptr, nullptr);
}

HRESULT CUIASynchronizedInputProviderWrapper::CancelImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }
    return m_pProvider->Cancel();
}

HRESULT STDMETHODCALLTYPE CUIASynchronizedInputProviderWrapper::StartListening(::SynchronizedInputType inputType)
{
    if (!IsWindowValid())
    {
        return E_FAIL;
    }

    Automation::CValue value;

    value.SetEnum(inputType);

    IFC_RETURN(StartListeningImpl(1, &value, nullptr));
    return S_OK;
}

HRESULT CUIASynchronizedInputProviderWrapper::StartListeningImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal)
{
    if (!IsPatternValid())
    {
        return E_FAIL;
    }

    if (cValue != 1)
    {
        return E_INVALIDARG;
    }
    IFCPTR_RETURN(pValue);
    IFC_RETURN(m_pProvider->StartListening((UIAXcp::SynchronizedInputType)(pValue->m_nValue)));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIACustomNavigationProviderWrapper::Navigate(_In_ NavigateDirection direction, _Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal)
{
    xref_ptr<CAutomationPeer> spAP;
    xref_ptr<IUnknown> spUnkNativeNode;
    xref_ptr<IRawElementProviderSimple> spIREPS;

    IFCCHECK_RETURN(IsWindowValid());
    IFCCHECK_RETURN(IsPatternValid());

    IFC_RETURN(m_pProvider->NavigateCustom((UIAXcp::AutomationNavigationDirection)direction, spAP.ReleaseAndGetAddressOf(), spUnkNativeNode.ReleaseAndGetAddressOf()));
    if (spUnkNativeNode)
    {
        IFC_RETURN(spUnkNativeNode->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(spIREPS.ReleaseAndGetAddressOf())));
    }
    else if (spAP)
    {
        xref_ptr<CUIAWrapper> spWrapper;
        xref_ptr<CAutomationPeer> spAPEventsSource;
        spAPEventsSource = static_cast<CAutomationPeer*>(spAP->GetAPEventsSource());
        if (spAPEventsSource)
        {
            spAP = spAPEventsSource;
        }
        IFC_RETURN(GetUIAWindow()->CreateProviderForAP(spAP, spWrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(spWrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(spIREPS.ReleaseAndGetAddressOf())));
    }
    *pRetVal = spIREPS.detach();
    return S_OK;
}
