// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CollectionViewSource.g.h"
#include "PropertyPath.g.h"
#include "CollectionViewManager.h"

using namespace DirectUI;
using namespace xaml_data;
using namespace xaml_interop;

DirectUI::CollectionViewSource::CollectionViewSource():
    m_pCVSViewChangedEventSource(NULL)
{ }

DirectUI::CollectionViewSource::~CollectionViewSource()
{
    ctl::release_interface(m_pCVSViewChangedEventSource);
}

_Check_return_ HRESULT DirectUI::CollectionViewSource::GetValue(_In_ const CDependencyProperty* pProperty, _Out_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;

    // If we're looking for the ItemsPath value, go directly through get_ItemsPath so the
    // string automatically gets wrapped in a PropertyPath object.
    if (pProperty->GetIndex() == KnownPropertyIndex::CollectionViewSource_ItemsPath)
    {
        ctl::ComPtr<xaml::IPropertyPath> spItemsPath;
        IFC(get_ItemsPath(&spItemsPath));
        *ppValue = spItemsPath.Detach();
    }
    else
    {
        IFC(CollectionViewSourceGenerated::GetValue(pProperty, ppValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
DirectUI::CollectionViewSource::OnPropertyChanged2(
    _In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(CollectionViewSourceGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::CollectionViewSource_Source:
        {
            ctl::ComPtr<IInspectable> spOldValue, spNewValue;
            IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
            IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
            IFC(OnSourceChanged(spOldValue.Get(), spNewValue.Get()));
        }
        break;

    case KnownPropertyIndex::CollectionViewSource_IsSourceGrouped:
        IFC(OnIsSourceGroupedChanged());
        break;

    case KnownPropertyIndex::CollectionViewSource_ItemsPath:
        IFC(OnItemsPathChanged());
        break;
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
DirectUI::CollectionViewSource::OnSourceChanged(_In_ IInspectable *pOldSource, _In_ IInspectable *pNewSource)
{
    HRESULT hr = S_OK;

    // Validate the source
    if( !IsSourceValid(pNewSource))
    {
        // If it's not valid as is, and this is a CLR application, try to make it valid by injecting a CLR wrapper

        //TODO: does this leak?
        IInspectable* const pWrapper = ReferenceTrackerManager::GetTrackerTarget(pNewSource);
        if( pWrapper != NULL )
        {
            pNewSource = pWrapper;
            if( !IsSourceValid(pNewSource) )
            {
                IFC( E_INVALIDARG );
            }
        }
        else
        {
            IFC( E_INVALIDARG );
        }
    }

    IFC(EnsureView(pNewSource));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
DirectUI::CollectionViewSource::OnIsSourceGroupedChanged()
{
    HRESULT hr = S_OK;
    IInspectable *pSource = NULL;

    IFC(get_Source(&pSource));

    // The source doesn't need to be checked again because it was checked
    // whenever it was set

    IFC(EnsureView(pSource));

Cleanup:

    ReleaseInterface(pSource);

    RRETURN(hr);
}

_Check_return_
HRESULT
DirectUI::CollectionViewSource::OnItemsPathChanged()
{
    HRESULT hr = S_OK;
    IInspectable *pSource = NULL;

    IFC(get_Source(&pSource));

    IFC(EnsureView(pSource));

Cleanup:

    ReleaseInterface(pSource);

    RRETURN(hr);
}

_Check_return_
HRESULT
DirectUI::CollectionViewSource::EnsureView(_In_ IInspectable *pNewSource)
{
    HRESULT hr = S_OK;
    ICollectionView *pView = NULL;
    BOOLEAN valid = false;

    // Only try to create a collection view if we have a source
    // otherwise we will just set the value to NULL
    if (pNewSource != NULL)
    {
        IFC(CollectionViewManager::GetViewRecord(pNewSource, this, &pView));

        // A view should have been created by now, or we would have failed
        // if the source is not supported
        IFCEXPECT(pView);

        // Move the item to the first position on the newly created collection
        // if we have one
        IFC(pView->MoveCurrentToFirst(&valid));
    }

    // Set the read only property
    // Now built-in collection views are DOs which are walkable so
    // storing it as the DP value is all that is needed to ensure
    // that they are walked during GC
    IFC(SetValueInternal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::CollectionViewSource_View),
        pView,
        TRUE /*fAllowReadOnly*/));

    // Notify of the changes
    if (m_pCVSViewChangedEventSource != NULL)
    {
        IFC(m_pCVSViewChangedEventSource->Raise(ctl::as_iinspectable(this), NULL));
    }

Cleanup:
    ReleaseInterface(pView);
    RRETURN(hr);
}

bool DirectUI::CollectionViewSource::IsSourceValid(_In_ IInspectable *pSource)
{
    if ((pSource == NULL ||
        ctl::is<IBindableIterable>(pSource) ||
        ctl::is<wfc::IIterable<IInspectable *>>(pSource)
        ) &&
        !ctl::is<ICollectionView>(pSource))
    {
        return true;
    }
    return false;
}

_Check_return_
HRESULT
DirectUI::CollectionViewSource::GetCVSViewChangedSource(_Outptr_ ICVSViewChangedEventSource **ppSource)
{
    HRESULT hr = S_OK;

    if (m_pCVSViewChangedEventSource == NULL)
    {
        IFC(ctl::ComObject<CVSViewChangedEventSource>::CreateInstance(&m_pCVSViewChangedEventSource));
    }

    *ppSource = m_pCVSViewChangedEventSource;
    ctl::addref_interface(m_pCVSViewChangedEventSource);

Cleanup:

    RRETURN(hr);
}

void
DirectUI::CollectionViewSource::OnReferenceTrackerWalk(INT walkType )
{
    if( m_pCVSViewChangedEventSource )
        m_pCVSViewChangedEventSource->ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));

    CollectionViewSourceGenerated::OnReferenceTrackerWalk( walkType );

}
