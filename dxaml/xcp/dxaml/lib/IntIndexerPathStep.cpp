// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Implements the class to access an indexer property with an int index

#include "precomp.h"
#include "IntIndexerPathStep.h"
#include "PropertyPath.h"
#include "IndexerPropertyAccess.h"
#include "BindableObservableVectorWrapper.h"
#include "ValidationErrorsObservableVectorWrapper.h"
#include <XStringUtils.h>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

IntIndexerPathStep::~IntIndexerPathStep()
{
    VERIFYHR(SafeRemoveVectorChangedHandler());
    delete[] m_szIndexerName;
}

void IntIndexerPathStep::DisconnectCurrentItem()
{
    if (m_epVectorChangedEventHandler)
    {
        VERIFYHR(m_epVectorChangedEventHandler.DetachEventHandler(m_tpVector.Get()));
    }

    m_tpVector.Clear();
    m_tpVectorView.Clear();

    if (m_tpIndexer)
    {
        VERIFYHR(m_tpIndexer->DisconnectEventHandlers());
        m_tpIndexer.Clear();
    }
}

_Check_return_
HRESULT
IntIndexerPathStep::CollectionViewCurrentChanged()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spItem;

    // Forget the previous vector
    DisconnectCurrentItem();

    IFC(m_tpSourceAsCV->get_CurrentItem(spItem.ReleaseAndGetAddressOf()));

    // Connect to the new vector
    IFC(InitializeFromSource(spItem.Get()));

    // Notify of the change
    IFC(RaiseSourceChanged());

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
IntIndexerPathStep::VectorChanged(_In_ wfc::IVectorChangedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange change;
    UINT32 index = 0;
    bool fChanged = false;

    IFC(pArgs->get_CollectionChange(&change));

    switch (change)
    {
        case wfc::CollectionChange_ItemInserted:
        case wfc::CollectionChange_ItemRemoved:
            IFC(pArgs->get_Index(&index));

            if (index <= m_nIndex)
            {
                fChanged = TRUE;
            }
            break;

        case wfc::CollectionChange_ItemChanged:
            IFC(pArgs->get_Index(&index));

            if (index == m_nIndex)
            {
                fChanged = TRUE;
            }
            break;

        case wfc::CollectionChange_Reset:
            fChanged = TRUE;
            break;
    }

    if (fChanged)
    {
        IFC(RaiseSourceChanged());
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
IntIndexerPathStep::SourceChanged()
{
    RRETURN(RaiseSourceChanged());
}

_Check_return_
HRESULT
IntIndexerPathStep::GetIndexedPropertyName(_Outptr_result_z_ WCHAR **pszPropertyName)
{
    HRESULT hr = S_OK;
    WCHAR szStringIndexer[38] = L"";    // Item[xxxxxxxxxx]
    size_t sizeOfString;

    if (m_szIndexerName == NULL)
    {
        IFCEXPECT(swprintf_s(szStringIndexer, 32, L"Item[%u]", m_nIndex) >= 0);
        sizeOfString = wcslen(szStringIndexer) + 1;
        m_szIndexerName = new WCHAR[sizeOfString];
        IFCEXPECT(wcscpy_s(m_szIndexerName, sizeOfString, szStringIndexer) >= 0);
    }

    *pszPropertyName = m_szIndexerName;

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
IntIndexerPathStep::ReConnect(_In_ IInspectable *pSource)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spInsp;
    ctl::ComPtr<xaml_data::ICollectionView> spSourceAsCV;

    // First disconnect from the current source
    Disconnect();

    // A null source is a NOOP
    if (PropertyValue::IsNullOrEmpty(pSource))
    {
        goto Cleanup;
    }

    IFC(InitializeFromSource(pSource));

    // If we were not able to get either a vector or a vector view from the source then
    // see if it is a collection view and if so connect to it
    if (!m_tpVector && !m_tpVectorView && !m_tpIndexer)
    {
        spSourceAsCV = ctl::query_interface_cast<xaml_data::ICollectionView>(pSource);
        SetPtrValue(m_tpSourceAsCV, spSourceAsCV);
        if (!m_tpSourceAsCV)
        {
            goto Cleanup;
        }

        IFC(AddCurrentChangedEventHandler());

        // Before Phase 2 the current item is suposed to be
        // a PropertyValue
        IFC(m_tpSourceAsCV->get_CurrentItem(spInsp.ReleaseAndGetAddressOf()));

        IFC(InitializeFromSource(spInsp.Get()));
    }

Cleanup:

    if (pSource != NULL && !IsConnected())
    {
        TraceConnectionFailure(pSource);
    }

    RRETURN(hr);
}

void IntIndexerPathStep::TraceGetterFailure()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    const CClassInfo* pTypeInfo = NULL;
    const WCHAR *szTraceString = NULL;
    xstring_ptr strSourceClassName;
    ctl::ComPtr<IInspectable> spSource;
    wrl_wrappers::HString strErrorString;
    ctl::ComPtr<IPropertyPathListener> spListener;

    if (!DebugOutput::IsLoggingForBindingEnabled())
    {
        goto Cleanup;
    }

    IFC(m_tpIndexer->GetSource(spSource.ReleaseAndGetAddressOf()));
    IFC(MetadataAPI::GetFriendlyRuntimeClassName(spSource.Get(), &strSourceClassName));

    IFC(m_tpIndexer->GetType(&pTypeInfo));

    IFC(m_spListener.As(&spListener));
    IFCEXPECT_ASSERT(spListener.Get());
    IFC(spListener.Cast<PropertyPathListener>()->GetTraceString(&szTraceString));

    IFC(DXamlCore::GetCurrentNoCreate()->GetNonLocalizedErrorString(TEXT_BINDINGTRACE_INT_INDEXER_FAILED, strErrorString.GetAddressOf()));

    DebugOutput::LogBindingErrorMessage(StringCchPrintfWWrapper(
        const_cast<WCHAR*>(strErrorString.GetRawBuffer(nullptr)),
        m_nIndex,
        pTypeInfo->GetName().GetBuffer(),
        const_cast<WCHAR*>(strSourceClassName.GetBuffer()),
        szTraceString));

Cleanup:
    return;
}

void IntIndexerPathStep::TraceConnectionFailure(_In_ IInspectable *pSource)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    xstring_ptr strClassName;
    const WCHAR *szTraceString = NULL;
    wrl_wrappers::HString strErrorString;
    ctl::ComPtr<IPropertyPathListener> spListener;

    if (!DebugOutput::IsLoggingForBindingEnabled())
    {
        goto Cleanup;
    }

    IFC(MetadataAPI::GetFriendlyRuntimeClassName(pSource, &strClassName));

    IFC(m_spListener.As(&spListener));
    IFCEXPECT_ASSERT(spListener.Get());
    IFC(spListener.Cast<PropertyPathListener>()->GetTraceString(&szTraceString));

    IFC(DXamlCore::GetCurrentNoCreate()->GetNonLocalizedErrorString(TEXT_BINDINGTRACE_INT_INDEXER_CONNECTION_FAILED, strErrorString.GetAddressOf()));

    DebugOutput::LogBindingErrorMessage(StringCchPrintfWWrapper(
        const_cast<WCHAR*>(strErrorString.GetRawBuffer(nullptr)),
        m_nIndex,
        const_cast<WCHAR*>(strClassName.GetBuffer()),
        szTraceString));

Cleanup:
    return;
}


_Check_return_
HRESULT
IntIndexerPathStep::InitializeFromSource(_In_ IInspectable *pRawSource)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_data::ICustomPropertyProvider> spProvider;
    wxaml_interop::TypeName sTypeName = {0};
    wrl_wrappers::HString strTypeName;
    ctl::ComPtr<IInspectable> spIndex;
    bool fInitialized = false;
    ctl::ComPtr<IInspectable> spSource;
    ctl::ComPtr<IInspectable> spWrapper;
    spSource.Attach(ValueWeakReference::get_value_as<IInspectable>(pRawSource));

    // Try to look for a vector in the source
    IFC(InitializeFromVector(spSource.Get(), &fInitialized));
    if (!fInitialized)
    {
        // We couldn't find a vector interface in the source, if this is
        // a CLR app try to wrap it in a CLR wrapper to see if we have something
        spWrapper.Attach(ReferenceTrackerManager::GetTrackerTarget(spSource.Get()));
        if (spWrapper)
        {
            IFC(InitializeFromVector(spWrapper.Get(), &fInitialized));
        }
    }

    // We coudn't find a vector, we will look for an integer indexer instead
    if (!fInitialized && (spProvider = spSource.AsOrNull<xaml_data::ICustomPropertyProvider>()))
    {
        ctl::ComPtr<PropertyAccess> spIndexer;

        // The source at this point is neither a vector or a vector view, default to try
        // to get a custom indexer of type int
        IFC(strTypeName.Set(STR_LEN_PAIR(L"Int32")));
        sTypeName.Name = strTypeName.Get();
        sTypeName.Kind = wxaml_interop::TypeKind_Primitive;
        IFC(PropertyValue::CreateFromInt32(m_nIndex, &spIndex));
        IFC(IndexerPropertyAccess::CreateInstance(
                this,
                spProvider.Get(), sTypeName,
                spIndex.Get(),
                m_fListenToChanges,
                &spIndexer));
        SetPtrValue(m_tpIndexer, spIndexer);
    }

    if (m_fListenToChanges)
    {
        IFC(AddVectorChangedHandler());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
IntIndexerPathStep::InitializeFromVector(_In_ IInspectable *pSource, _Out_ bool *pfResult)
{
    ctl::ComPtr<IInspectable> source(pSource);
    // We should be clean at this point
    ASSERT(!m_tpVector && !m_tpVectorView && !m_tpIndexer);

    // Not initialzed yet
    *pfResult = FALSE;

    // Get the vector from the property value
    // if the source is not a vector, then nothing to do
    if (auto spBindableVector = source.AsOrNull<xaml_interop::IBindableVector>())
    {
        ctl::ComPtr<wfc::IVector<IInspectable *>> bindableVectorWrapper;
        if (auto spINCC = source.AsOrNull<xaml_interop::INotifyCollectionChanged>())
        {
            IFC_RETURN(BindableObservableVectorWrapper::CreateInstance(spBindableVector.Get(), spINCC.Get(), bindableVectorWrapper.ReleaseAndGetAddressOf()));
        }
        else if (auto spObservableBindable = source.AsOrNull<xaml_interop::IBindableObservableVector>())
        {
            IFC_RETURN(BindableObservableVectorWrapper::CreateInstance(spObservableBindable.Get(), bindableVectorWrapper.ReleaseAndGetAddressOf()));
        }
        else
        {
            bindableVectorWrapper = reinterpret_cast<wfc::IVector<IInspectable *>*>(spBindableVector.Get());
        }

        SetPtrValue(m_tpVector, bindableVectorWrapper);
        *pfResult = TRUE;
    }
    else if (auto spVector = source.AsOrNull<wfc::IVector<IInspectable *>>())
    {
        SetPtrValue(m_tpVector, spVector);
        *pfResult = TRUE;
    }
    // The source could also be a vector view, account for that
    else if (auto spVectorView = source.AsOrNull<wfc::IVectorView<IInspectable *>>())
    {
        SetPtrValue(m_tpVectorView, spVectorView);
        *pfResult = TRUE;
    }
    else if (auto spBindableView = source.AsOrNull<xaml_interop::IBindableVectorView>())
    {
        SetPtrValue(m_tpVectorView, reinterpret_cast<wfc::IVectorView<IInspectable *>*>(spBindableView.Get()));
        *pfResult = TRUE;
    }
    else if (auto validationErrorsCollection = source.AsOrNull<ValidationErrorsCollection>())
    {
        // We want to create our own special wrapper to ensures that Text="{Binding Path=(Validation.Errors)[0].ErrorMessage}"
        // works as expected
        SetPtrValue(m_tpVector, ValidationErrorsObservableVectorWrapper::CreateInstanceAsVector(validationErrorsCollection.Get()));
        *pfResult = TRUE;
    }

    return S_OK;
}


_Check_return_
HRESULT
IntIndexerPathStep::AddVectorChangedHandler()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spObservableVector;

    spObservableVector = m_tpVector.AsOrNull<wfc::IObservableVector<IInspectable *>>();
    if (spObservableVector)
    {
        IFC(m_epVectorChangedEventHandler.AttachEventHandler(spObservableVector.Get(),
            [this](wfc::IObservableVector<IInspectable *> *sender, wfc::IVectorChangedEventArgs *args) {
                return VectorChanged(args);
            }));
    }

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
IntIndexerPathStep::SafeRemoveVectorChangedHandler()
{
    HRESULT hr = S_OK;

    if (m_epVectorChangedEventHandler)
    {
        auto spVector = m_tpVector.GetSafeReference();

        if (spVector)
        {
            IFC(m_epVectorChangedEventHandler.DetachEventHandler(spVector.Get()));
        }
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
IntIndexerPathStep::GetVectorSize(_Out_ XUINT32 *pnSize)
{
    HRESULT hr = S_OK;

    IFCEXPECT(m_tpVector || m_tpVectorView);

    if (m_tpVector)
    {
        IFC(m_tpVector->get_Size(pnSize));
    }
    else if (m_tpVectorView)
    {
        IFC(m_tpVectorView->get_Size(pnSize));
    }
    else
    {
        IFC(E_UNEXPECTED);
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
IntIndexerPathStep::GetValueAtIndex(_Out_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue *pIndex = NULL;

    IFCEXPECT(m_tpVector || m_tpVectorView || m_tpIndexer);

    if (m_tpVector)
    {
        IFC(m_tpVector->GetAt(m_nIndex, ppValue));
    }
    else if (m_tpVectorView)
    {
        IFC(m_tpVectorView->GetAt(m_nIndex, ppValue));
    }
    else
    {
        IFC(m_tpIndexer->GetValue(ppValue));
    }

Cleanup:

    ReleaseInterface(pIndex);

    RRETURN(hr);
}

_Check_return_
HRESULT
IntIndexerPathStep::SetValueAtIndex(_Out_ IInspectable *pValue)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue *pIndex = NULL;

    IFCEXPECT((m_tpVector || m_tpIndexer) && !m_tpVectorView);

    if (m_tpVector)
    {
        IFC(m_tpVector->SetAt(m_nIndex, pValue));
    }
    else
    {
        IFC(m_tpIndexer->SetValue(pValue));
    }

Cleanup:

    ReleaseInterface(pIndex);

    RRETURN(hr);
}


bool IntIndexerPathStep::IsConnected()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    bool fReturn = false;
    XUINT32 nSize = 0;

    // No vector or indexer means we're not connected
    if (!m_tpVector && !m_tpVectorView && !m_tpIndexer)
    {
        goto Cleanup;
    }

    // Check if we're accessing the data through an indexer or through IVector
    if (m_tpIndexer)
    {
        // If we have an indexer, nothing else to check, we're connected
        fReturn = TRUE;
    }
    else
    {
        // We're connected as long as 1) there's a vector
        // and 2) the index is within range
        IFC(GetVectorSize(&nSize));
        fReturn = m_nIndex < nSize ? TRUE : FALSE;
    }

Cleanup:

    return fReturn;
}


_Check_return_
HRESULT
IntIndexerPathStep::GetValue(_Out_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;

    // If we're not connected nothing to do
    if (!IsConnected())
    {
        *ppValue = NULL;
        goto Cleanup;
    }

    // Read the value
    IFC(GetValueAtIndex(ppValue));

Cleanup:

    if (FAILED(hr))
    {
        TraceGetterFailure();
    }

    RRETURN(hr);
}

_Check_return_
HRESULT
IntIndexerPathStep::SetValue(_In_  IInspectable *pValue)
{
    HRESULT hr = S_OK;

    // Set value can only work if we're connected and the caller must know this
    IFCEXPECT(IsConnected());

    // Write the value
    IFC(SetValueAtIndex(pValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
IntIndexerPathStep::GetType(_Outptr_ const CClassInfo **ppType)
{
    // If we have an indexer then get the type of it directly
    if (m_tpIndexer)
    {
        return m_tpIndexer->GetType(ppType);
    }

    // Since we operate only with IVector<IInspectable>/IVectorView<IInspectable> the
    // type will always be of object
    *ppType = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Object);
    RRETURN(S_OK);
}
_Check_return_
HRESULT
IntIndexerPathStep::GetSourceType(_Outptr_ const CClassInfo **ppType)
{
    HRESULT hr = S_OK;
    IFCEXPECT(m_tpIndexer);
    IFC(m_tpIndexer->GetSourceType(ppType));
Cleanup:
    return hr;
}
