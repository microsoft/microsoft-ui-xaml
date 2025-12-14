// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

DEFINE_GUID(GUID_UIATextRangeProviderWrapper, 0x6d14e0c0, 0x585e, 0x490b, 0x84, 0xb2, 0x4e, 0xdf, 0x70, 0x93, 0x05, 0xec);
//------------------------------------------------------------------------
//
//  Abstract:
//
//      UIAPatternProvider Wrappers, for the Jolt core types
//
//------------------------------------------------------------------------

namespace Automation
{
    class CValue;
}

class CUIAPatternProviderWrapper : public IUIAWrapper
{
public:
    CUIAPatternProviderWrapper(_In_ IUIAProvider* pUIAProvider, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds)
    {
        m_pUIAProvider = pUIAProvider;
        m_pWindow = pWindow;
        m_pUIAWindowValidator = m_pWindow->GetUIAWindowValidator();
        pUIAIds = UIAIds;
    }

    virtual ~CUIAPatternProviderWrapper()
    {
        ReleaseInterface(m_pUIAWindowValidator);
        InvalidateWrapper();
    }

    void Invalidate() override;
    void InvalidateWrapper();
    void SetWrapper();

    XINT32 IsPatternValid()
    {
        return m_pUIAProvider != NULL;
    }

    XINT32 IsWindowValid()
    {
        return m_pUIAWindowValidator && m_pUIAWindowValidator->IsValid();
    }

    UIAIdentifiers* GetUIAIds()
    {
        return pUIAIds;
    }

    CUIAWindow* GetUIAWindow()
    {
        return m_pWindow;
    }

private:
    CUIAWindow *m_pWindow;
    UIAIdentifiers *pUIAIds;
    IUIAWindowValidator *m_pUIAWindowValidator;
    IUIAProvider *m_pUIAProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAInvokeProviderWrapper
//
//  Synopsis:
//      UIA InvokeProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAInvokeProviderWrapper final : public IInvokeProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAInvokeProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAInvokeProvider*)pObj->GetPatternInterface();
    }
    ~CUIAInvokeProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAInvokeProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAInvokeProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAInvokeProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));
    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(IInvokeProvider*)(this);
        }
        else if (riid == __uuidof(IInvokeProvider))
        {
            *ppInterface = (IInvokeProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IInvokeProvider
    HRESULT STDMETHODCALLTYPE Invoke() override;
    HRESULT InvokeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAInvokeProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIADockProviderWrapper
//
//  Synopsis:
//      UIA DockProvider Wrapper
//
//------------------------------------------------------------------------
class CUIADockProviderWrapper final : public IDockProvider, public CUIAPatternProviderWrapper
{
public:
    CUIADockProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIADockProvider*)pObj->GetPatternInterface();
    }
    ~CUIADockProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIADockProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIADockProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIADockProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IDockProvider))
        {
            *ppInterface = (IDockProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IDockProvider
    HRESULT STDMETHODCALLTYPE get_DockPosition(_Out_ DockPosition *pRetVal) override;
    HRESULT get_DockPositionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE SetDockPosition(_In_ DockPosition dockPosition) override;
    HRESULT SetDockPositionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIADockProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAExpandCollapseProviderWrapper
//
//  Synopsis:
//      UIA ExpandCollapseProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAExpandCollapseProviderWrapper final : public IExpandCollapseProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAExpandCollapseProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAExpandCollapseProvider*)pObj->GetPatternInterface();
    }
    ~CUIAExpandCollapseProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAExpandCollapseProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;

        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAExpandCollapseProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAExpandCollapseProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IExpandCollapseProvider))
        {
            *ppInterface = (IExpandCollapseProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    void SetObject(IUIAProvider *pObj)
    {
        m_pProvider = (IUIAExpandCollapseProvider*)pObj;
    }

    // IExpandCollapseProvider
    HRESULT STDMETHODCALLTYPE Collapse() override;
    HRESULT CollapseImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE Expand() override;
    HRESULT ExpandImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_ExpandCollapseState(_Out_ ExpandCollapseState *pRetVal) override;
    HRESULT get_ExpandCollapseStateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAExpandCollapseProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAGridItemProviderWrapper
//
//  Synopsis:
//      UIA GridItemProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAGridItemProviderWrapper final : public IGridItemProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAGridItemProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAGridItemProvider*)pObj->GetPatternInterface();
    }
    ~CUIAGridItemProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAGridItemProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAGridItemProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAGridItemProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));
    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IGridItemProvider))
        {
            *ppInterface = (IGridItemProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IGridItemProvider
    HRESULT STDMETHODCALLTYPE get_Column(_Out_ int *pRetVal) override;
    HRESULT get_ColumnImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_ColumnSpan(_Out_ int *pRetVal) override;
    HRESULT get_ColumnSpanImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_ContainingGrid(_Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal) override;
    HRESULT get_ContainingGridImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_Row(_Out_ int *pRetVal) override;
    HRESULT get_RowImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_RowSpan(_Out_ int *pRetVal) override;
    HRESULT get_RowSpanImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAGridItemProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAGridProviderWrapper
//
//  Synopsis:
//      UIA GridProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAGridProviderWrapper final : public IGridProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAGridProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAGridProvider*)pObj->GetPatternInterface();
    }
    ~CUIAGridProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAGridProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAGridProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAGridProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));
    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IGridProvider))
        {
            *ppInterface = (IGridProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IGridProvider
    HRESULT STDMETHODCALLTYPE get_ColumnCount(_Out_ int *pRetVal) override;
    HRESULT get_ColumnCountImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE GetItem(_In_ int row, _In_ int column, _Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal) override;
    HRESULT GetItemImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_RowCount(_Out_ int *pRetVal) override;
    HRESULT get_RowCountImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAGridProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAMultipleViewProviderWrapper
//
//  Synopsis:
//      UIA MultipleViewProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAMultipleViewProviderWrapper final : public IMultipleViewProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAMultipleViewProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAMultipleViewProvider*)pObj->GetPatternInterface();
    }
    ~CUIAMultipleViewProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAMultipleViewProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAMultipleViewProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAMultipleViewProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));
    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IMultipleViewProvider))
        {
            *ppInterface = (IMultipleViewProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IMultipleViewProvider
    HRESULT STDMETHODCALLTYPE get_CurrentView(_Out_ int *pRetVal) override;
    HRESULT get_CurrentViewImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE GetSupportedViews(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT GetSupportedViewsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE GetViewName(_In_ int viewId, _Out_ BSTR *pRetVal) override;
    HRESULT GetViewNameImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE SetCurrentView(_In_ int viewId) override;
    HRESULT SetCurrentViewImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAMultipleViewProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIARangeValueProviderWrapper
//
//  Synopsis:
//      UIA RangeValueProvider Wrapper
//
//------------------------------------------------------------------------
class CUIARangeValueProviderWrapper final : public IRangeValueProvider, public CUIAPatternProviderWrapper
{
public:
    CUIARangeValueProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIARangeValueProvider*)pObj->GetPatternInterface();
    }
    ~CUIARangeValueProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIARangeValueProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;

        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIARangeValueProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIARangeValueProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IRangeValueProvider))
        {
            *ppInterface = (IRangeValueProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IRangeValueProvider
    HRESULT STDMETHODCALLTYPE get_IsReadOnly(_Out_ BOOL *pRetVal) override;
    HRESULT get_IsReadOnlyImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_LargeChange(_Out_ double *pRetVal) override;
    HRESULT get_LargeChangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_Maximum(_Out_ double *pRetVal) override;
    HRESULT get_MaximumImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_Minimum(_Out_ double *pRetVal) override;
    HRESULT get_MinimumImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE SetValue(_In_ double val) override;
    HRESULT SetValueImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_SmallChange(_Out_ double *pRetVal) override;
    HRESULT get_SmallChangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_Value(_Out_ double *pRetVal) override;
    HRESULT get_ValueImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIARangeValueProvider *m_pProvider;
};


//------------------------------------------------------------------------
//
//  Method:   CUIAScrollItemProviderWrapper
//
//  Synopsis:
//      UIA ScrollItemProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAScrollItemProviderWrapper final : public IScrollItemProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAScrollItemProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAScrollItemProvider*)pObj->GetPatternInterface();
    }
    ~CUIAScrollItemProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAScrollItemProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAScrollItemProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAScrollItemProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));
    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IScrollItemProvider))
        {
            *ppInterface = (IScrollItemProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IScrollItemProvider
    HRESULT STDMETHODCALLTYPE ScrollIntoView() override;
    HRESULT ScrollIntoViewImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAScrollItemProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAScrollProviderWrapper
//
//  Synopsis:
//      UIA ScrollProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAScrollProviderWrapper final : public IScrollProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAScrollProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAScrollProvider*)pObj->GetPatternInterface();
    }
    ~CUIAScrollProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAScrollProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAScrollProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAScrollProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));
    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IScrollProvider))
        {
            *ppInterface = (IScrollProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IScrollProvider
    HRESULT STDMETHODCALLTYPE get_HorizontallyScrollable(_Out_ BOOL *pRetVal) override;
    HRESULT get_HorizontallyScrollableImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_HorizontalScrollPercent(_Out_ double *pRetVal) override;
    HRESULT get_HorizontalScrollPercentImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_HorizontalViewSize(_Out_ double *pRetVal) override;
    HRESULT get_HorizontalViewSizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE Scroll(_In_ ScrollAmount horizontalAmount, _In_ ScrollAmount verticalAmount) override;
    HRESULT ScrollImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE SetScrollPercent(_In_ double horizontalPercent, _In_ double verticalPercent) override;
    HRESULT SetScrollPercentImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_VerticallyScrollable(_Out_ BOOL *pRetVal) override;
    HRESULT get_VerticallyScrollableImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_VerticalScrollPercent(_Out_ double *pRetVal) override;
    HRESULT get_VerticalScrollPercentImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_VerticalViewSize(_Out_ double *pRetVal) override;
    HRESULT get_VerticalViewSizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAScrollProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionItemProviderWrapper
//
//  Synopsis:
//      UIA SelectionItemProvider Wrapper
//
//------------------------------------------------------------------------
class CUIASelectionItemProviderWrapper final : public ISelectionItemProvider, public CUIAPatternProviderWrapper
{
public:
    CUIASelectionItemProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIASelectionItemProvider*)pObj->GetPatternInterface();
    }
    ~CUIASelectionItemProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIASelectionItemProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIASelectionItemProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIASelectionItemProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));
    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ISelectionItemProvider))
        {
            *ppInterface = (ISelectionItemProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // ISelectionItemProvider
    HRESULT STDMETHODCALLTYPE AddToSelection() override;
    HRESULT AddToSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_IsSelected(_Out_ BOOL *pRetVal) override;
    HRESULT get_IsSelectedImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE RemoveFromSelection() override;
    HRESULT RemoveFromSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE Select() override;
    HRESULT SelectImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_SelectionContainer(_Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal) override;
    HRESULT get_SelectionContainerImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);


private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIASelectionItemProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIASelectionProviderWrapper
//
//  Synopsis:
//      UIA SelectionProvider Wrapper
//
//------------------------------------------------------------------------
class CUIASelectionProviderWrapper final : public ISelectionProvider, public CUIAPatternProviderWrapper
{
public:
    CUIASelectionProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIASelectionProvider*)pObj->GetPatternInterface();
    }
    ~CUIASelectionProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIASelectionProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIASelectionProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIASelectionProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));
    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ISelectionProvider))
        {
            *ppInterface = (ISelectionProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // ISelectionProvider
    HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(_Out_ BOOL *pRetVal) override;
    HRESULT get_CanSelectMultipleImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE GetSelection(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT GetSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(_Out_ BOOL *pRetVal) override;
    HRESULT get_IsSelectionRequiredImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIASelectionProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIATableItemProviderWrapper
//
//  Synopsis:
//      UIA TableItemProvider Wrapper
//
//------------------------------------------------------------------------
class CUIATableItemProviderWrapper final : public ITableItemProvider, public CUIAPatternProviderWrapper
{
public:
    CUIATableItemProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIATableItemProvider*)pObj->GetPatternInterface();
    }
    ~CUIATableItemProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIATableItemProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIATableItemProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIATableItemProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));
    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ITableItemProvider))
        {
            *ppInterface = (ITableItemProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // ITableItemProvider
    HRESULT STDMETHODCALLTYPE GetColumnHeaderItems(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT GetColumnHeaderItemsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE GetRowHeaderItems(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT GetRowHeaderItemsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIATableItemProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIATableProviderWrapper
//
//  Synopsis:
//      UIA TableProvider Wrapper
//
//------------------------------------------------------------------------
class CUIATableProviderWrapper final : public ITableProvider, public CUIAPatternProviderWrapper
{
public:
    CUIATableProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIATableProvider*)pObj->GetPatternInterface();
    }
    ~CUIATableProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIATableProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIATableProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIATableProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));
    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ITableProvider))
        {
            *ppInterface = (ITableProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // ITableProvider
    HRESULT STDMETHODCALLTYPE GetColumnHeaders(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT GetColumnHeadersImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE GetRowHeaders(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT GetRowHeadersImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_RowOrColumnMajor(_Out_ RowOrColumnMajor *pRetVal) override;
    HRESULT get_RowOrColumnMajorImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIATableProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAToggleProviderWrapper
//
//  Synopsis:
//      UIA ToggleProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAToggleProviderWrapper final : public IToggleProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAToggleProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAToggleProvider*)pObj->GetPatternInterface();
    }
    ~CUIAToggleProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAToggleProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAToggleProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAToggleProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IToggleProvider))
        {
            *ppInterface = (IToggleProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IToggleProvider
    HRESULT STDMETHODCALLTYPE Toggle() override;
    HRESULT ToggleImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_ToggleState(_Out_ ToggleState *pRetVal) override;
    HRESULT get_ToggleStateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAToggleProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIATransformProviderWrapper
//
//  Synopsis:
//      UIA TransformProvider Wrapper
//
//------------------------------------------------------------------------
class CUIATransformProviderWrapper final : public ITransformProvider2, public CUIAPatternProviderWrapper
{
public:
    CUIATransformProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIATransformProvider*)pObj->GetPatternInterface();
    }
    ~CUIATransformProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIATransformProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIATransformProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIATransformProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        HRESULT hr = S_OK;
        XINT32 isITransformProvider2 = 0;
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ITransformProvider))
        {
            *ppInterface = (ITransformProvider*)(this);
        }
        else if (riid == __uuidof(ITransformProvider2))
        {
            IFC(m_pProvider->IsITransformProvider2(&isITransformProvider2));
            if(isITransformProvider2)
            {
                *ppInterface = static_cast<ITransformProvider2*>(this);
            }
            else
            {
                *ppInterface = NULL;
                IFC(E_NOINTERFACE);
            }
        }
        else
        {
            *ppInterface = NULL;
            IFC(E_NOINTERFACE);
        }

        ((IUnknown*)(*ppInterface))->AddRef();
Cleanup:
        RRETURN(hr);
    }

    // ITransformProvider
    HRESULT STDMETHODCALLTYPE get_CanMove(_Out_ BOOL *pRetVal) override;
    HRESULT get_CanMoveImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_CanResize(_Out_ BOOL *pRetVal) override;
    HRESULT get_CanResizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_CanRotate(_Out_ BOOL *pRetVal) override;
    HRESULT get_CanRotateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE Move(_In_ double x, _In_ double y) override;
    HRESULT MoveImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE Resize(_In_ double width, _In_ double height) override;
    HRESULT ResizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE Rotate(_In_ double degree) override;
    HRESULT RotateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    // ITransformProvider2
    HRESULT STDMETHODCALLTYPE get_CanZoom( _Out_ BOOL *value) override;
    HRESULT get_CanZoomImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_ZoomLevel( _Out_ double *value) override;
    HRESULT STDMETHODCALLTYPE get_ZoomMaximum(_Out_ double *value) override;
    HRESULT STDMETHODCALLTYPE get_ZoomMinimum(_Out_ double *value) override;
    HRESULT STDMETHODCALLTYPE Zoom(_In_ double zoom) override;
    HRESULT ZoomImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE ZoomByUnit(_In_ ZoomUnit zoomUnit) override;
    HRESULT ZoomByUnitImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIATransformProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAValueProviderWrapper
//
//  Synopsis:
//      UIA ValueProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAValueProviderWrapper final : public IValueProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAValueProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAValueProvider*)pObj->GetPatternInterface();
    }
    ~CUIAValueProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAValueProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAValueProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAValueProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IValueProvider))
        {
            *ppInterface = (IValueProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IValueProvider
    HRESULT STDMETHODCALLTYPE get_IsReadOnly(_Out_ BOOL *pRetVal) override;
    HRESULT get_IsReadOnlyImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE SetValue(_In_ LPCWSTR val) override;
    HRESULT SetValueImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_Value(_Out_ BSTR *pRetVal) override;
    HRESULT get_ValueImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAValueProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAWindowProviderWrapper
//
//  Synopsis:
//      UIA WindowProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAWindowProviderWrapper final : public IWindowProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAWindowProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAWindowProvider*)pObj->GetPatternInterface();
    }
    ~CUIAWindowProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAWindowProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAWindowProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAWindowProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IWindowProvider))
        {
            *ppInterface = (IWindowProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IWindowProvider
    HRESULT STDMETHODCALLTYPE Close() override;
    HRESULT CloseImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_IsModal(_Out_ BOOL *pRetVal) override;
    HRESULT get_IsModalImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_IsTopmost(_Out_ BOOL *pRetVal) override;
    HRESULT get_IsTopmostImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_CanMaximize(_Out_ BOOL *pRetVal) override;
    HRESULT get_CanMaximizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_CanMinimize(_Out_ BOOL *pRetVal) override;
    HRESULT get_CanMinimizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE SetVisualState(_In_ WindowVisualState state) override;
    HRESULT SetVisualStateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE WaitForInputIdle(_In_ int milliseconds, _Out_ BOOL *pRetVal) override;
    HRESULT WaitForInputIdleImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_WindowInteractionState(_Out_ WindowInteractionState *pRetVal) override;
    HRESULT get_WindowInteractionStateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_WindowVisualState(_Out_ WindowVisualState *pRetVal) override;
    HRESULT get_WindowVisualStateImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    BOOL m_cRef;

    IUIAWindowProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIATextProviderWrapper
//
//  Synopsis:
//      UIA TextProvider Wrapper
//
//------------------------------------------------------------------------
class CUIATextProviderWrapper final : public ITextProvider2, public CUIAPatternProviderWrapper
{
public:
    CUIATextProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIATextProvider*)pObj->GetPatternInterface();
    }
    ~CUIATextProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIATextProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;

        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIATextProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIATextProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));


    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppInterface) override
    {
        HRESULT hr = S_OK;
        XINT32 isITextProvider2 = 0;

        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ITextProvider))
        {
            *ppInterface = (ITextProvider*)(this);
        }
        else if (riid == __uuidof(ITextProvider2))
        {
            IFC(m_pProvider->IsITextProvider2(&isITextProvider2));
            if(isITextProvider2)
            {
                *ppInterface = static_cast<ITextProvider2*>(this);
            }
            else
            {
                *ppInterface = NULL;
                IFC(E_NOINTERFACE);
            }
        }
        else
        {
            *ppInterface = NULL;
            IFC(E_NOINTERFACE);
        }

        ((IUnknown*)(*ppInterface))->AddRef();
Cleanup:
        RRETURN(hr);
    }

    // ITextProvider
    HRESULT STDMETHODCALLTYPE GetSelection(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT GetSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE GetVisibleRanges(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT GetVisibleRangesImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE RangeFromChild(_In_ IRawElementProviderSimple *childElement, _Out_ ITextRangeProvider **pRetVal) override;
    HRESULT RangeFromChildImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE RangeFromPoint(_In_ UiaPoint screenLocation, _Out_ ITextRangeProvider **pRetVal) override;
    HRESULT RangeFromPointImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_DocumentRange(_Out_ ITextRangeProvider **pRetVal) override;
    HRESULT get_DocumentRangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_SupportedTextSelection(_Out_ SupportedTextSelection  *pRetVal) override;
    HRESULT get_SupportedTextSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    // ITextProvider2
    HRESULT STDMETHODCALLTYPE RangeFromAnnotation(_In_opt_ IRawElementProviderSimple *annotationElement, _Outptr_result_maybenull_ ITextRangeProvider **pRetVal) override;
    HRESULT RangeFromAnnotationImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE GetCaretRange(_Out_ BOOL *isActive, _Outptr_result_maybenull_ ITextRangeProvider **pRetVal) override;
    HRESULT GetCaretRangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIATextProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIATextRangeProviderWrapper
//
//  Synopsis:
//      UIA TextRangeProvider Wrapper
//
//------------------------------------------------------------------------

class CUIATextRangeProviderWrapper final : public ITextRangeProvider2,
                                            public CUIAPatternProviderWrapper
{
public:
    CUIATextRangeProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIATextRangeProvider*)pObj->GetPatternInterface();
        UiaGetReservedNotSupportedValue(&m_punkNotSupportedValue);
        UiaGetReservedMixedAttributeValue(&m_punkMixedAttribute);
    }
    ~CUIATextRangeProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIATextRangeProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;

        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIATextRangeProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIATextRangeProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));


    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppInterface) override;

    // ITextRangeProvider

    HRESULT STDMETHODCALLTYPE AddToSelection() override;
    HRESULT AddToSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE Clone(_Out_ ITextRangeProvider **pRetVal) override;
    HRESULT CloneImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE Compare(_In_ ITextRangeProvider *textRange, _Out_ BOOL *pRetVal) override;
    HRESULT CompareImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE CompareEndpoints(_In_ TextPatternRangeEndpoint endpoint, _In_ ITextRangeProvider *targetRange, _In_ TextPatternRangeEndpoint targetEndpoint, _Out_ int *pRetVal) override;
    HRESULT CompareEndpointsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE ExpandToEnclosingUnit(_In_ TextUnit unit) override;
    HRESULT ExpandToEnclosingUnitImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE FindAttribute(_In_ int attributeId, _In_ VARIANT value, _In_ BOOL backward, _Out_ ITextRangeProvider **pRetVal) override;
    HRESULT FindAttributeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE FindText(_In_ BSTR text, _In_ BOOL backward, _In_ BOOL ignoreCase, _Out_ ITextRangeProvider **pRetVal) override;
    HRESULT FindTextImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE GetAttributeValue(_In_ int attributeId, _Out_ VARIANT *pRetVal) override;
    HRESULT GetAttributeValueImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE GetBoundingRectangles(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT GetBoundingRectanglesImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE GetChildren(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT GetChildrenImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE GetEnclosingElement(_Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal) override;
    HRESULT GetEnclosingElementImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE GetText(_In_ int maxLength, _Out_ BSTR *pRetVal) override;
    HRESULT GetTextImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE Move(_In_ TextUnit unit, _In_ int count, _Out_ int *pRetVal) override;
    HRESULT MoveImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE MoveEndpointByRange(_In_ TextPatternRangeEndpoint endpoint, _In_ ITextRangeProvider *targetRange, _In_ TextPatternRangeEndpoint targetEndpoint) override;
    HRESULT MoveEndpointByRangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE MoveEndpointByUnit(_In_ TextPatternRangeEndpoint endpoint, _In_ TextUnit unit, _In_ int count, _Out_ int *pRetVal) override;
    HRESULT MoveEndpointByUnitImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE RemoveFromSelection() override;
    HRESULT RemoveFromSelectionImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE ScrollIntoView(_In_ BOOL alignToTop) override;
    HRESULT ScrollIntoViewImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE Select() override;
    HRESULT SelectImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE ShowContextMenu() override;
    HRESULT ShowContextMenuImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIATextRangeProvider *m_pProvider;
    IUnknown *m_punkNotSupportedValue;
    IUnknown *m_punkMixedAttribute;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAItemContainerProviderWrapper
//
//  Synopsis:
//      UIA ItemContainerProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAItemContainerProviderWrapper final : public IItemContainerProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAItemContainerProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAItemContainerProvider*)pObj->GetPatternInterface();
    }
    ~CUIAItemContainerProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAItemContainerProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAItemContainerProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAItemContainerProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IItemContainerProvider))
        {
            *ppInterface = (IItemContainerProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IItemContainerProvider
    HRESULT STDMETHODCALLTYPE FindItemByProperty(_In_ IRawElementProviderSimple *pStartAfter, _In_ PROPERTYID propertyId, _In_ VARIANT value, _Outptr_result_maybenull_ IRawElementProviderSimple **pFound) override;

    HRESULT FindItemByPropertyImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAItemContainerProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAVirtualizedItemProviderWrapper
//
//  Synopsis:
//      UIA VirtualizedItemProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAVirtualizedItemProviderWrapper final : public IVirtualizedItemProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAVirtualizedItemProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAVirtualizedItemProvider*)pObj->GetPatternInterface();
    }
    ~CUIAVirtualizedItemProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAVirtualizedItemProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAVirtualizedItemProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAVirtualizedItemProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IVirtualizedItemProvider))
        {
            *ppInterface = (IVirtualizedItemProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IVirtualizedItemProvider
    HRESULT STDMETHODCALLTYPE Realize() override;
    HRESULT RealizeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAVirtualizedItemProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIATextChildProviderWrapper
//
//  Synopsis:
//      UIA TextChildProvider Wrapper
//
//------------------------------------------------------------------------
class CUIATextChildProviderWrapper final : public ITextChildProvider, public CUIAPatternProviderWrapper
{
public:
    CUIATextChildProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIATextChildProvider*)pObj->GetPatternInterface();
    }
    ~CUIATextChildProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIATextChildProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;

        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIATextChildProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIATextChildProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));


    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppInterface) override
    {
        HRESULT hr = S_OK;
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ITextChildProvider))
        {
            *ppInterface = (ITextChildProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            IFC(E_NOINTERFACE);
        }

        ((IUnknown*)(*ppInterface))->AddRef();
Cleanup:
        RRETURN(hr);
    }

    // ITextChildProvider
    HRESULT STDMETHODCALLTYPE get_TextRange(_Outptr_result_maybenull_ ITextRangeProvider **pRetVal) override;
    HRESULT get_TextRangeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_TextContainer(_Outptr_result_maybenull_ IRawElementProviderSimple **ppTextContainer) override;
    HRESULT get_TextContainerImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIATextChildProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIATextEditProviderWrapper
//
//  Synopsis:
//      UIA TextEditProvider Wrapper
//
//------------------------------------------------------------------------
class CUIATextEditProviderWrapper final : public ITextEditProvider, public CUIAPatternProviderWrapper
{
public:
    CUIATextEditProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = static_cast<IUIATextEditProvider*>(pObj->GetPatternInterface());

        // composing TextWrapper here as TextEdit pattern inherit from text pattern
        // here we are consumung CUIATextProviderWrapper implementation providing the implementaiton of ITextProvider part of ITextEditProvider
        m_spUIAInnerTextProviderWrapper.attach(new CUIATextProviderWrapper(pObj, pWindow, UIAIds));
    }
    ~CUIATextEditProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        xref_ptr<CUIATextEditProviderWrapper> spProviderWrapper;

        spProviderWrapper = static_cast<CUIATextEditProviderWrapper*>(pObj->GetUIAWrapper());
        if (!spProviderWrapper)
        {
            spProviderWrapper.attach(new CUIATextEditProviderWrapper(pObj, pWindow, UIAIds));
            spProviderWrapper->SetWrapper();
        }

        IFC_RETURN(spProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

        return S_OK;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppInterface) override
    {
        HRESULT hr = S_OK;

        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = static_cast<IUnknown*>(this);
        }
        else if (riid == __uuidof(ITextProvider))
        {
            *ppInterface = static_cast<ITextProvider*>(this);
        }
        else if (riid == __uuidof(ITextEditProvider))
        {
            *ppInterface = static_cast<ITextEditProvider*>(this);
        }
        else
        {
            hr = E_NOINTERFACE;
        }

        if (SUCCEEDED(hr))
        {
            ((IUnknown*)(*ppInterface))->AddRef();
        }
        return hr;
    }

    // ITextProvider
    HRESULT STDMETHODCALLTYPE GetSelection(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetVisibleRanges(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE RangeFromChild(_In_ IRawElementProviderSimple *childElement, _Out_ ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE RangeFromPoint(_In_ UiaPoint screenLocation, _Out_ ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_DocumentRange(_Out_ ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_SupportedTextSelection(_Out_ SupportedTextSelection  *pRetVal) override;

    // ITextEditProvider
    HRESULT STDMETHODCALLTYPE GetActiveComposition(_Outptr_result_maybenull_  ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetConversionTarget(_Outptr_result_maybenull_  ITextRangeProvider **pRetVal) override;

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;
    IUIATextEditProvider* m_pProvider;
    xref_ptr<CUIATextProviderWrapper> m_spUIAInnerTextProviderWrapper;
};

//------------------------------------------------------------------------
//
//  Method:   CUIAAnnotationProviderWrapper
//
//  Synopsis:
//      UIA AnnotationProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAAnnotationProviderWrapper final : public IAnnotationProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAAnnotationProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAAnnotationProvider*)pObj->GetPatternInterface();
    }
    ~CUIAAnnotationProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAAnnotationProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;

        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAAnnotationProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAAnnotationProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));


    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppInterface) override
    {
        HRESULT hr = S_OK;
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IAnnotationProvider))
        {
            *ppInterface = (IAnnotationProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            IFC(E_NOINTERFACE);
        }

        ((IUnknown*)(*ppInterface))->AddRef();
Cleanup:
        RRETURN(hr);
    }

    // IAnnotationProvider
    HRESULT STDMETHODCALLTYPE get_AnnotationTypeId(_Out_ int *pRetVal) override;
    HRESULT get_AnnotationTypeIdImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_AnnotationTypeName(_Outptr_result_maybenull_ BSTR *pRetVal) override;
    HRESULT get_AnnotationTypeNameImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_Author(_Outptr_result_maybenull_ BSTR *pRetVal) override;
    HRESULT get_AuthorImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_DateTime(_Outptr_result_maybenull_ BSTR *retVal) override;
    HRESULT get_DateTimeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_Target(_Outptr_result_maybenull_ IRawElementProviderSimple **retVal) override;
    HRESULT get_TargetImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAAnnotationProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIADragProviderWrapper
//
//  Synopsis:
//      UIA DragProvider Wrapper
//
//------------------------------------------------------------------------
class CUIADragProviderWrapper final : public IDragProvider, public CUIAPatternProviderWrapper
{
public:
    CUIADragProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIADragProvider*)pObj->GetPatternInterface();
    }
    ~CUIADragProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIADragProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIADragProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIADragProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IDragProvider))
        {
            *ppInterface = (IDragProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IDragProvider
    HRESULT STDMETHODCALLTYPE get_IsGrabbed(_Out_ BOOL *pRetVal) override;
    HRESULT get_IsGrabbedImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_DropEffect(_Outptr_result_maybenull_ BSTR *pRetVal) override;
    HRESULT get_DropEffectImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE get_DropEffects(_Outptr_result_maybenull_ SAFEARRAY * *pRetVal) override;
    HRESULT get_DropEffectsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

    HRESULT STDMETHODCALLTYPE GetGrabbedItems(_Outptr_result_maybenull_ SAFEARRAY * *pRetVal) override;
    HRESULT GetGrabbedItemsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIADragProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIADropTargetProviderWrapper
//
//  Synopsis:
//      UIA DropTargetProvider Wrapper
//
//------------------------------------------------------------------------
class CUIADropTargetProviderWrapper final : public IDropTargetProvider, public CUIAPatternProviderWrapper
{
public:
    CUIADropTargetProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIADropTargetProvider*)pObj->GetPatternInterface();
    }
    ~CUIADropTargetProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIADropTargetProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIADropTargetProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIADropTargetProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IDropTargetProvider))
        {
            *ppInterface = (IDropTargetProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IDropTargetProvider
    HRESULT STDMETHODCALLTYPE get_DropTargetEffect( _Outptr_result_maybenull_ BSTR *pRetVal) override;
    HRESULT get_DropTargetEffectImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_DropTargetEffects( _Outptr_result_maybenull_ SAFEARRAY * *pRetVal) override;
    HRESULT get_DropTargetEffectsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIADropTargetProvider *m_pProvider;
};


//------------------------------------------------------------------------
//
//  Method:   CUIAObjectModelProvider
//
//  Synopsis:
//      UIA ObjectModelProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAObjectModelProviderWrapper final : public IObjectModelProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAObjectModelProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAObjectModelProvider*)pObj->GetPatternInterface();
    }
    ~CUIAObjectModelProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAObjectModelProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAObjectModelProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAObjectModelProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IObjectModelProvider))
        {
            *ppInterface = (IObjectModelProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IObjectModelProvider
    HRESULT STDMETHODCALLTYPE GetUnderlyingObjectModel(_Outptr_result_maybenull_ IUnknown **returnValue) override;
    HRESULT GetUnderlyingObjectModelImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAObjectModelProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIASpreadsheetProvider
//
//  Synopsis:
//      UIA SpreadsheetProvider Wrapper
//
//------------------------------------------------------------------------
class CUIASpreadsheetProviderWrapper final : public ISpreadsheetProvider, public CUIAPatternProviderWrapper
{
public:
    CUIASpreadsheetProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIASpreadsheetProvider*)pObj->GetPatternInterface();
    }
    ~CUIASpreadsheetProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIASpreadsheetProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIASpreadsheetProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIASpreadsheetProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ISpreadsheetProvider))
        {
            *ppInterface = (ISpreadsheetProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // ISpreadsheetProvider
    HRESULT STDMETHODCALLTYPE GetItemByName(_In_ LPCWSTR name, _Outptr_result_maybenull_ IRawElementProviderSimple **returnValue) override;
    HRESULT GetItemByNameImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIASpreadsheetProvider *m_pProvider;
};


//------------------------------------------------------------------------
//
//  Method:   CUIASpreadsheetItemProvider
//
//  Synopsis:
//      UIA SpreadsheetItemProvider Wrapper
//
//------------------------------------------------------------------------
class CUIASpreadsheetItemProviderWrapper final : public ISpreadsheetItemProvider, public CUIAPatternProviderWrapper
{
public:
    CUIASpreadsheetItemProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIASpreadsheetItemProvider*)pObj->GetPatternInterface();
    }
    ~CUIASpreadsheetItemProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIASpreadsheetItemProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIASpreadsheetItemProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIASpreadsheetItemProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ISpreadsheetItemProvider))
        {
            *ppInterface = (ISpreadsheetItemProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // ISpreadsheetItemProvider
    HRESULT STDMETHODCALLTYPE get_Formula(_Outptr_result_maybenull_ BSTR *pValue) override;
    HRESULT get_FormulaImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE GetAnnotationObjects(_Outptr_result_maybenull_ SAFEARRAY **returnValue) override;
    HRESULT GetAnnotationObjectsImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE GetAnnotationTypes(_Outptr_result_maybenull_ SAFEARRAY **returnValue) override;
    HRESULT GetAnnotationTypesImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIASpreadsheetItemProvider *m_pProvider;
};


//------------------------------------------------------------------------
//
//  Method:   CUIAStylesProviderWrapper
//
//  Synopsis:
//      UIA StylesProvider Wrapper
//
//------------------------------------------------------------------------
class CUIAStylesProviderWrapper final : public IStylesProvider, public CUIAPatternProviderWrapper
{
public:
    CUIAStylesProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIAStylesProvider*)pObj->GetPatternInterface();
    }
    ~CUIAStylesProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIAStylesProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIAStylesProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIAStylesProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(IStylesProvider))
        {
            *ppInterface = (IStylesProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // IStylesProvider
    HRESULT STDMETHODCALLTYPE get_ExtendedProperties(_Outptr_result_maybenull_ BSTR *pRetVal) override;
    HRESULT get_ExtendedPropertiesImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_FillColor(_Out_ int *value) override;
    HRESULT get_FillColorImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_FillPatternColor(_Out_ int *value) override;
    HRESULT get_FillPatternColorImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_FillPatternStyle(_Outptr_result_maybenull_ BSTR *retVal) override;
    HRESULT get_FillPatternStyleImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_Shape(_Outptr_result_maybenull_ BSTR *value) override;
    HRESULT get_ShapeImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_StyleId(_Out_ int *value) override;
    HRESULT get_StyleIdImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE get_StyleName(_Outptr_result_maybenull_ BSTR *value) override;
    HRESULT get_StyleNameImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIAStylesProvider *m_pProvider;
};


//------------------------------------------------------------------------
//
//  Method:   CUIASynchronizedInputProviderWrapper
//
//  Synopsis:
//      UIA SynchronizedInputProvider Wrapper
//
//------------------------------------------------------------------------
class CUIASynchronizedInputProviderWrapper final : public ISynchronizedInputProvider, public CUIAPatternProviderWrapper
{
public:
    CUIASynchronizedInputProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIASynchronizedInputProvider*)pObj->GetPatternInterface();
    }
    ~CUIASynchronizedInputProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        CUIASynchronizedInputProviderWrapper *pProviderWrapper = NULL;
        HRESULT hr = S_OK;
        IFCPTR(pObj);
        IFCPTR(UIAIds);
        IFCPTR(pWindow);
        pProviderWrapper = static_cast<CUIASynchronizedInputProviderWrapper*>(pObj->GetUIAWrapper());
        if(!pProviderWrapper)
        {
            pProviderWrapper = new CUIASynchronizedInputProviderWrapper(pObj, pWindow, UIAIds);
            pProviderWrapper->SetWrapper();
        }
        else
        {
            (pProviderWrapper)->AddRef();
        }
        IFC(pProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

    Cleanup:
        ReleaseInterface(pProviderWrapper);
        return hr;
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ISynchronizedInputProvider))
        {
            *ppInterface = (ISynchronizedInputProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // ISynchronizedInputProvider
    HRESULT STDMETHODCALLTYPE Cancel(void) override;
    HRESULT CancelImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);
    HRESULT STDMETHODCALLTYPE StartListening(SynchronizedInputType inputType) override;
    HRESULT StartListeningImpl(_In_ XUINT32 cValue, _In_reads_(cValue) Automation::CValue* pValue, _Out_ Automation::CValue* pRetVal);

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIASynchronizedInputProvider *m_pProvider;
};

//------------------------------------------------------------------------
//
//  Method:   CUIACustomNavigationProviderWrapper
//
//  Synopsis:
//      UIA CustomNavigationProvider Wrapper
//
//------------------------------------------------------------------------
class CUIACustomNavigationProviderWrapper final : public ICustomNavigationProvider, public CUIAPatternProviderWrapper
{
public:
    CUIACustomNavigationProviderWrapper(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds) : m_cRef(1),
        CUIAPatternProviderWrapper(pObj, pWindow, UIAIds)
    {
        m_pProvider = (IUIACustomNavigationProvider*)pObj->GetPatternInterface();
    }
    ~CUIACustomNavigationProviderWrapper() override
    {
    }
    static HRESULT Create(_In_ IUIAProvider* pObj, _In_ CUIAWindow *pWindow, _In_ UIAIdentifiers *UIAIds, _Out_ IUnknown** ppWrapper)
    {
        xref_ptr<CUIACustomNavigationProviderWrapper> spProviderWrapper;
        spProviderWrapper = static_cast<CUIACustomNavigationProviderWrapper*>(pObj->GetUIAWrapper());
        if (!spProviderWrapper)
        {
            spProviderWrapper.attach(new CUIACustomNavigationProviderWrapper(pObj, pWindow, UIAIds));
            spProviderWrapper->SetWrapper();
        }

        IFC_RETURN(spProviderWrapper->QueryInterface(__uuidof(IUnknown), (void**)ppWrapper));

        RRETURN(S_OK);
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRef; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = (IUnknown*)(this);
        }
        else if (riid == __uuidof(ICustomNavigationProvider))
        {
            *ppInterface = (ICustomNavigationProvider*)(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown*)(*ppInterface))->AddRef();
        return S_OK;
    }

    // ICustomNavigationProvider
    HRESULT STDMETHODCALLTYPE Navigate(_In_ NavigateDirection direction, _Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal) override;

private:
    // Ref Counter for this COM object
    XINT32 m_cRef;

    IUIACustomNavigationProvider *m_pProvider;
};
