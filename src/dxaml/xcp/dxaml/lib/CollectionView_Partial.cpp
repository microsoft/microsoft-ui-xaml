// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CollectionView.g.h"
#include "CurrentChangingEventArgs.g.h"
#include "PropertyChangedEventArgs.g.h"

using namespace DirectUI;
using namespace xaml_data;

CollectionView::CollectionView():
    m_currentPosition(0),
    m_isCurrentAfterLast(FALSE),
    m_isCurrentBeforeFirst(FALSE)
{ }

CollectionView::~CollectionView()
{

}

// ICustomPropertyProvider
_Check_return_ HRESULT
CollectionView::get_TypeImpl(_Out_ wxaml_interop::TypeName* pValue)
{
    HRESULT hr = S_OK;
    wxaml_interop::TypeName typeName = { };

    typeName.Kind = wxaml_interop::TypeKind_Metadata;
    IFC(WindowsCreateString(STR_LEN_PAIR(L"Microsoft.UI.Xaml.Data.ICollectionView"), &typeName.Name));

    *pValue = typeName;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CollectionView::GetStringRepresentationImpl(_Out_ HSTRING* returnValue)
{
    RRETURN(WindowsCreateString(STR_LEN_PAIR(L"Microsoft.UI.Xaml.Data.CollectionView"), returnValue));
}

_Check_return_ HRESULT
CollectionView::GetCustomPropertyImpl(_In_ HSTRING name, _Outptr_ xaml_data::ICustomProperty** returnValue)
{
    HRESULT hr = S_OK;

    if (wrl_wrappers::HStringReference(L"CurrentItem") == name)
    {
        hr = CustomProperty::CreateObjectProperty(
            name,
            &CollectionView::GetCurrentItemPropertyValue,
            returnValue);
    }
    else if (wrl_wrappers::HStringReference(L"CollectionGroups") == name)
    {
        hr = CustomProperty::CreateObjectProperty(
            name,
            &CollectionView::GetCollectionGroupsPropertyValue,
            returnValue);
    }
    else if (wrl_wrappers::HStringReference(L"CurrentPosition") == name)
    {
        hr = CustomProperty::CreateInt32Property(
            name,
            &CollectionView::GetCurrentPositionPropertyValue,
            returnValue);
    }
    else if (wrl_wrappers::HStringReference(L"IsCurrentAfterLast") == name)
    {
        hr = CustomProperty::CreateBooleanProperty(
            name,
            &CollectionView::GetIsCurrentAfterLastPropertyValue,
            returnValue);
    }
    else if (wrl_wrappers::HStringReference(L"IsCurrentBeforeFirst") == name)
    {
        hr = CustomProperty::CreateBooleanProperty(
            name,
            &CollectionView::GetIsCurrentBeforeFirstPropertyValue,
            returnValue);
    }
    else if (wrl_wrappers::HStringReference(L"HasMoreItems") == name)
    {
        hr = CustomProperty::CreateBooleanProperty(
            name,
            &CollectionView::GetHasMoreItemsPropertyValue,
            returnValue);
    }
    IFC(hr);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CollectionView::GetCurrentItemPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_data::ICollectionView> spCV;
    ctl::ComPtr<IInspectable> spTarget = pTarget;

    IFC(spTarget.As(&spCV));
    IFC(spCV.Cast<CollectionView>()->get_CurrentItem(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CollectionView::GetCollectionGroupsPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_data::ICollectionView> spCV;
    ctl::ComPtr<IInspectable> spTarget = pTarget;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCollectionGroups;

    IFC(spTarget.As(&spCV));
    IFC(spCV.Cast<CollectionView>()->get_CollectionGroups(&spCollectionGroups));
    *ppValue = spCollectionGroups.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CollectionView::GetCurrentPositionPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_data::ICollectionView> spCV;
    ctl::ComPtr<IInspectable> spTarget = pTarget;
    INT value = 0;

    IFC(spTarget.As(&spCV));
    IFC(spCV.Cast<CollectionView>()->get_CurrentPosition(&value));
    IFC(PropertyValue::CreateFromInt32(value, ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CollectionView::GetIsCurrentAfterLastPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_data::ICollectionView> spCV;
    ctl::ComPtr<IInspectable> spTarget = pTarget;
    BOOLEAN value = 0;

    IFC(spTarget.As(&spCV));
    IFC(spCV.Cast<CollectionView>()->get_IsCurrentAfterLast(&value));
    IFC(PropertyValue::CreateFromBoolean(value, ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CollectionView::GetIsCurrentBeforeFirstPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_data::ICollectionView> spCV;
    ctl::ComPtr<IInspectable> spTarget = pTarget;
    BOOLEAN value = 0;

    IFC(spTarget.As(&spCV));
    IFC(spCV.Cast<CollectionView>()->get_IsCurrentBeforeFirst(&value));
    IFC(PropertyValue::CreateFromBoolean(value, ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CollectionView::GetHasMoreItemsPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_data::ICollectionView> spCV;
    ctl::ComPtr<IInspectable> spTarget = pTarget;
    BOOLEAN value = 0;

    IFC(spTarget.As(&spCV));
    IFC(spCV.Cast<CollectionView>()->get_HasMoreItems(&value));
    IFC(PropertyValue::CreateFromBoolean(value, ppValue));

Cleanup:
    RRETURN(hr);
}

// ICollectionView
_Check_return_
HRESULT
CollectionView::get_CurrentItemImpl(_Outptr_ IInspectable **value)
{
    RRETURN(m_tpCurrentItem.CopyTo(value));
}

_Check_return_
HRESULT
CollectionView::get_CurrentPositionImpl(_Out_ INT *value)
{
    *value = m_currentPosition;
    RRETURN(S_OK);
}

_Check_return_
HRESULT
CollectionView::get_IsCurrentAfterLastImpl(_Out_ BOOLEAN *value)
{
    *value = m_isCurrentAfterLast;
    RRETURN(S_OK);
}

_Check_return_
HRESULT
CollectionView::get_IsCurrentBeforeFirstImpl(_Out_ BOOLEAN *value)
{
    *value = m_isCurrentBeforeFirst;
    RRETURN(S_OK);
}

_Check_return_
HRESULT
CollectionView::MoveCurrentToImpl(
    _In_opt_ IInspectable *pItem,
    _Out_ BOOLEAN *returnValue)
{
    HRESULT hr = S_OK;
    unsigned index = 0;
    BOOLEAN found = false;
    BOOLEAN fOkToChange = false;

    IFC(OkToChangeCurrent(&fOkToChange));
    if (fOkToChange)
    {
        IFC(IndexOf(pItem, &index, &found));
        if (!found)
        {
            // If the value is not found then we go to
            // the position before the first
            // With nothing being the current item
            IFC(SetCurrentIndexAt(-1, returnValue));
            *returnValue = false;
        }
        else
        {
            IFC(SetCurrentIndexAt(index, returnValue));
        }
    }
    else
    {
        *returnValue = false;
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
CollectionView::MoveCurrentToPositionImpl(_In_ INT index, _Out_ BOOLEAN *returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN fOkToChange = false;

    IFC(OkToChangeCurrent(&fOkToChange));

    if (fOkToChange)
    {
        IFC(SetCurrentIndexAt(index, returnValue));
    }
    else
    {
        *returnValue = false;
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
CollectionView::MoveCurrentToFirstImpl(_Out_ BOOLEAN *returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN fOkToChange = false;

    IFC(OkToChangeCurrent(&fOkToChange));

    if (fOkToChange)
    {
        IFC(SetCurrentIndexAt(0, returnValue));
    }
    else
    {
        *returnValue = false;
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
CollectionView::MoveCurrentToLastImpl(_Out_ BOOLEAN *returnValue)
{
    HRESULT hr = S_OK;
    unsigned count = 0;
    BOOLEAN fOkToChange = false;

    IFC(OkToChangeCurrent(&fOkToChange));

    if (fOkToChange)
    {
        IFC(get_Size(&count));

        IFC(SetCurrentIndexAt(count - 1, returnValue));
    }
    else
    {
        *returnValue = false;
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
CollectionView::MoveCurrentToNextImpl(_Out_ BOOLEAN *returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN fOkToChange = false;

    IFC(OkToChangeCurrent(&fOkToChange));
    if (fOkToChange)
    {
        IFC(SetCurrentIndexAt(m_currentPosition + 1, returnValue));
    }
    else
    {
        *returnValue = false;
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
CollectionView::MoveCurrentToPreviousImpl(_Out_ BOOLEAN *returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN fOkToChange = false;

    IFC(OkToChangeCurrent(&fOkToChange));
    if (fOkToChange)
    {
        IFC(SetCurrentIndexAt(m_currentPosition - 1, returnValue));
    }
    else
    {
        *returnValue = false;
    }

Cleanup:

    RRETURN(hr);
}

// Default implemenation for grouping
_Check_return_
HRESULT
CollectionView::get_CollectionGroupsImpl(_Out_ wfc::IObservableVector<IInspectable *> **value)
{
    // No grouping data
    *value = NULL;
    RRETURN(S_OK);
}

// Default implementations for the virtualization methods
_Check_return_
HRESULT
CollectionView::get_HasMoreItemsImpl(_Out_ BOOLEAN *value)
{
    // No more data
    *value = false;
    RRETURN(S_OK);
}

_Check_return_
HRESULT
CollectionView::LoadMoreItemsAsyncImpl(
    _In_ UINT32 count,
    _Outptr_ wf::IAsyncOperation<xaml_data::LoadMoreItemsResult> **operation)
{
    // No loadm ore items implemented by default
    RRETURN(E_NOTIMPL);
}

// IObservableVector<IInspectable *>
IFACEMETHODIMP CollectionView::add_VectorChanged(
    _In_ wfc::VectorChangedEventHandler<IInspectable *> *pHandler,
    _Out_ EventRegistrationToken *token)
{
    HRESULT hr = S_OK;

    ARG_NOTNULL(pHandler, "handler");
    ARG_VALIDRETURNPOINTER(token);

    m_vectorChangedHandlers.AddHandler(pHandler, token);

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP CollectionView::remove_VectorChanged(
    _In_ EventRegistrationToken token)
{
    RRETURN(m_vectorChangedHandlers.RemoveHandler(token));
}

// Private methods
_Check_return_
HRESULT CollectionView::SetCurrentIndexAt(
    _In_ INT newIndex,
    _Out_opt_ BOOLEAN *valid)
{
    HRESULT hr = S_OK;
    unsigned size = 0;
    BOOLEAN result = false;
    BOOLEAN newIsCurrentAfterLast = false;
    BOOLEAN newIsCurrentBeforeFirst = false;
    INT iNewCurrentIndex = 0;
    ctl::ComPtr<IInspectable> spCurrentItem;

    IFC(get_Size(&size));

    if (newIndex < 0)
    {
       iNewCurrentIndex = -1;
    }
    else if (newIndex >= static_cast<INT>(size))
    {
       iNewCurrentIndex = size;
    }
    else
    {
       iNewCurrentIndex = newIndex;
       result = true;
    }

    if (m_currentPosition != iNewCurrentIndex)
    {
        m_currentPosition = iNewCurrentIndex;
        IFC(RaisePropertyChanged(STR_LEN_PAIR(L"CurrentPosition")));
    }

    // If we have a valid index then
    // acquire the new current item
    if (result)
    {
        IFC(GetAt(newIndex, &spCurrentItem));
    }

    // Update the current item, if we didn't acquire the
    // current item, because the index was invalid
    // then this will just set it to null
    if (m_tpCurrentItem.Get() != spCurrentItem.Get())
    {
        SetPtrValue(m_tpCurrentItem, spCurrentItem.Get());
        IFC(RaisePropertyChanged(STR_LEN_PAIR(L"CurrentItem")));
    }

    // Update the IsCurrentAfterLast and IsCurrentBeforeFirst properties
    // accoringly
    newIsCurrentAfterLast = (iNewCurrentIndex >= static_cast<INT>(size));
    if (m_isCurrentAfterLast != newIsCurrentAfterLast)
    {
        m_isCurrentAfterLast = newIsCurrentAfterLast;
        IFC(RaisePropertyChanged(STR_LEN_PAIR(L"IsCurrentAfterLast")));
    }
    newIsCurrentBeforeFirst = (iNewCurrentIndex < 0);
    if (m_isCurrentBeforeFirst != newIsCurrentBeforeFirst)
    {
        m_isCurrentBeforeFirst = newIsCurrentBeforeFirst;
        IFC(RaisePropertyChanged(STR_LEN_PAIR(L"IsCurrentBeforeFirst")));
    }

    IFC(RaiseCurrentChanged());

    if (valid != NULL)
    {
        *valid = result;
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT CollectionView::RaiseVectorChanged(_In_ wfc::IVectorChangedEventArgs *pArgs)
{
    RRETURN(m_vectorChangedHandlers.Raise(this, pArgs));
}

_Check_return_
HRESULT
CollectionView::ProcessCollectionChange(_In_ wfc::IVectorChangedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange change;
    UINT32 changeIndex = 0;
    INT32 iIndex = 0;
    BOOLEAN fCurrentChanged = false;
    BOOLEAN fCurrentDeleted = false;
    UINT32 nNewSize = 0;
    INT32 iCurrentIndex = 0;
    ctl::ComPtr<IInspectable> spCurrentItem;

    IFC(pArgs->get_CollectionChange(&change));
    IFC(RaiseVectorChanged(pArgs));

    iCurrentIndex = m_currentPosition;

    switch (change)
    {
    case wfc::CollectionChange_ItemInserted:
        IFC(pArgs->get_Index(&changeIndex));
        iIndex = static_cast<INT32>(changeIndex);

        if (iIndex <= iCurrentIndex)
        {
            // An item was insterted before the current item
            // we need to update the current index accordingly
            iCurrentIndex++;
            m_currentPosition = iCurrentIndex;
            IFC(RaisePropertyChanged(STR_LEN_PAIR(L"CurrentPosition")));
        }
        break;

    case wfc::CollectionChange_ItemRemoved:
        IFC(pArgs->get_Index(&changeIndex));
        iIndex = static_cast<INT32>(changeIndex);

        if (iIndex < iCurrentIndex)
        {
            // An item was removed before the current item
            // we need to update the current index accordingly
            iCurrentIndex--;
            ASSERT(iCurrentIndex >= 0);
            m_currentPosition = iCurrentIndex;
            IFC(RaisePropertyChanged(STR_LEN_PAIR(L"CurrentPosition")));
        }
        else if (iIndex == iCurrentIndex)
        {
            fCurrentChanged = true;
            fCurrentDeleted = true;
        }
        break;

    case wfc::CollectionChange_ItemChanged:
        IFC(pArgs->get_Index(&changeIndex));
        iIndex = static_cast<INT32>(changeIndex);

        if (iIndex == iCurrentIndex)
        {
            fCurrentChanged = true;
        }
        break;

    case wfc::CollectionChange_Reset:
        if (iCurrentIndex > -1)
        {
            // we had a current item before, we should have one now
            fCurrentChanged = true;
        }
        break;

    default:

        ASSERT(FALSE);
    }

    // We detected that the current item is no longer
    // valid, we will look for a new one
    if (fCurrentChanged)
    {
        BOOLEAN fFound = false;

        // Notify of the fact that the current item is changing
        // but don't allow cancellation
        IFC(RaiseCurrentChanging());

        IFC(get_CurrentItem(&spCurrentItem));

        // We need to determine the new index based on the old
        // current item
        if (spCurrentItem)
        {
            UINT nNewCurrentIndex = 0;

            // In case the current item is deleted we need to maintain
            // the currency as is, or move to the last item
            if (fCurrentDeleted)
            {
                IFC(get_Size(&nNewSize));
                if (iCurrentIndex >= static_cast<INT>(nNewSize))
                {
                    nNewCurrentIndex = nNewSize - 1;
                }
                else
                {
                    nNewCurrentIndex = iCurrentIndex;
                }

                IFC(SetCurrentIndexAt(nNewCurrentIndex, NULL));
            }
            else
            {
                IFC(IndexOf(spCurrentItem.Get(), &nNewCurrentIndex, &fFound));
                if (fFound)
                {
                    // We found the old current item in the new collection
                    // thus adjust the index
                    IFC(SetCurrentIndexAt(nNewCurrentIndex, NULL));
                }
                else
                {
                    // We couldn't find the current item in the new collection
                    // move to the first index
                    IFC(SetCurrentIndexAt(0, NULL));
                }
            }
        }
        else
        {
            // No exising current item means that we will move
            // the current item to the first item
            IFC(SetCurrentIndexAt(0, NULL));
        }
    }

Cleanup:

    RRETURN(hr);
}

void CollectionView::OnReferenceTrackerWalk(INT walkType)
{
    EReferenceTrackerWalkType walkTypeAsEnum = static_cast<EReferenceTrackerWalkType>(walkType);
    m_vectorChangedHandlers.ReferenceTrackerWalk(walkTypeAsEnum);
    CollectionViewGenerated::OnReferenceTrackerWalk(walkType);

    //
    // Forward walk to the event sources
    //

    if (m_spCurrentChangingEventSource)
    {
        m_spCurrentChangingEventSource->ReferenceTrackerWalk(walkTypeAsEnum);
    }

    if (m_spCurrentChangedEventSource)
    {
        m_spCurrentChangedEventSource->ReferenceTrackerWalk(walkTypeAsEnum);
    }

}

_Check_return_
HRESULT
CollectionView::RaisePropertyChanged(
    _In_reads_(nLength) const WCHAR* name,
    _In_ const XUINT32 nLength)
{
    HRESULT hr = S_OK;
    PropertyChangedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<PropertyChangedEventArgs> spArgs;
    wrl_wrappers::HString strPropertyName;

    // Create the args
    IFC(ctl::make(&spArgs));

    IFC(strPropertyName.Set(name, nLength));
    IFC(spArgs->put_PropertyName(strPropertyName));

    // Raise the event
    IFC(GetPropertyChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
CollectionView::RaiseCurrentChanged()
{
    HRESULT hr = S_OK;
    CurrentChangedEventSourceType *pEventSource = NULL;

    IFC(GetCurrentChangedEventSource(&pEventSource));

    IFC(pEventSource->Raise(ctl::as_iinspectable(this), NULL));

Cleanup:

    ctl::release_interface(pEventSource);

    RRETURN(hr);
}

_Check_return_
HRESULT
CollectionView::RaiseCurrentChanging(_In_ ICurrentChangingEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    CurrentChangingEventSourceType *pEventSource = NULL;

    IFC(GetCurrentChangingEventSource(&pEventSource));

    IFC(pEventSource->Raise(ctl::as_iinspectable(this), pArgs));

Cleanup:

    ctl::release_interface(pEventSource);

    RRETURN(hr);
}


_Check_return_
HRESULT
CollectionView::RaiseCurrentChanging()
{
    HRESULT hr = S_OK;
    CurrentChangingEventArgs *pEventArgs = NULL;

    IFC(DirectUI::CurrentChangingEventArgs::CreateInstance(false /* isCancellable */, &pEventArgs));

    IFC(RaiseCurrentChanging(pEventArgs));

Cleanup:

    ctl::release_interface(pEventArgs);

    RRETURN(hr);
}


_Check_return_
HRESULT
CollectionView::OkToChangeCurrent(_Out_ BOOLEAN *pfOkToChange)
{
    HRESULT hr = S_OK;
    CurrentChangingEventArgs *pEventArgs = NULL;
    BOOLEAN fCanceled = false;

    IFC(DirectUI::CurrentChangingEventArgs::CreateInstance(true /* isCancellable */, &pEventArgs));

    IFC(RaiseCurrentChanging(pEventArgs));

    IFC(pEventArgs->get_Cancel(&fCanceled));

    *pfOkToChange = !fCanceled;

Cleanup:

    ctl::release_interface(pEventArgs);

    RRETURN(hr);
}


_Check_return_ HRESULT
CollectionView::GetCurrentChangedEventSource(_Outptr_ CurrentChangedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    if (!m_spCurrentChangedEventSource)
    {
        IFC(ctl::make(&m_spCurrentChangedEventSource));
    }

    IFC(m_spCurrentChangedEventSource.CopyTo(ppEventSource));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CollectionView::GetCurrentChangingEventSource(_Outptr_ CurrentChangingEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    if (!m_spCurrentChangingEventSource)
    {
        IFC(ctl::make(&m_spCurrentChangingEventSource));
    }

    IFC(m_spCurrentChangingEventSource.CopyTo(ppEventSource));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::CollectionView::add_CurrentChanged(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    CurrentChangedEventSourceType* pEventSource = NULL;

    ARG_VALIDRETURNPOINTER(ptToken);
    ARG_NOTNULL(pValue, "value");
    IFC(CheckThread());
    IFC(GetCurrentChangedEventSource(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    ctl::release_interface(pEventSource);
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::CollectionView::remove_CurrentChanged(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    CurrentChangedEventSourceType* pEventSource = NULL;
    wf::IEventHandler<IInspectable*>* pValue = (wf::IEventHandler<IInspectable*>*)tToken.value;

    IFC(CheckThread());
    IFC(GetCurrentChangedEventSource(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

Cleanup:
    ctl::release_interface(pEventSource);
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::CollectionView::add_CurrentChanging(_In_ xaml_data::ICurrentChangingEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    CurrentChangingEventSourceType* pEventSource = NULL;

    ARG_VALIDRETURNPOINTER(ptToken);
    ARG_NOTNULL(pValue, "value");
    IFC(CheckThread());
    IFC(GetCurrentChangingEventSource(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    ctl::release_interface(pEventSource);
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::CollectionView::remove_CurrentChanging(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    CurrentChangingEventSourceType* pEventSource = NULL;
    xaml_data::ICurrentChangingEventHandler* pValue = (xaml_data::ICurrentChangingEventHandler*)tToken.value;

    IFC(CheckThread());
    IFC(GetCurrentChangingEventSource(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

Cleanup:
    ctl::release_interface(pEventSource);
    RRETURN(hr);
}
