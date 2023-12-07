// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <collectionbase.h>
#include <DOCollection.h>
#include <CValue.h>
#include <UIElement.h>
#include <double.h>
#include <Point.h>
#include <TextRange.h>
#include <dopointercast.h>
#include <corep.h>

CCollection::~CCollection()
{
}

_Check_return_ HRESULT CCollection::Destroy()
{
    IFC_RETURN(Neat(true));

    return S_OK;
}

#pragma warning(push)
#pragma warning(disable: 6387)
_Check_return_
HRESULT
CCollection::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT hr = S_OK;

// There is only one type of parallel timeline to create so we can ignore the parameters.

   *ppObject = NULL;
    RRETURN(hr);
}
#pragma warning(pop)

_Check_return_
HRESULT
CCollection::IndexOf(CValue& value, _Out_ XINT32 *pIndex)
{
      return E_INVALIDARG;
}

_Check_return_
HRESULT
CCollection::Clear()
{
// Release all the objects in the collection and break their association with
// the collection.

    IFC_RETURN(Destroy());

    IFC_RETURN(OnClear());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CCollection::SetOwner
//
//  Synopsis:
//      Owner of this collection, and if set, will be set as the
//      parent of any items added.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCollection::SetOwner(_In_opt_ CDependencyObject *pNewOwner, _In_ bool fOwnerIsFallback, _In_opt_ RENDERCHANGEDPFN pfnNewOwnerRenderChangedHandler)
{
    auto currentOwner = m_pOwner.lock_noref();
    bool fShouldKeepOwner = pNewOwner && m_pOwner && currentOwner == pNewOwner;

    if (fOwnerIsFallback)
    {
        // we can't clear fallback owner before set
        if (!pNewOwner)
        {
            IFCEXPECT_ASSERT_RETURN(m_pOwner && m_fOwnerIsFallback);
        }
    }
    else
    {
        // Either this is the first time setting the Owner, the owner is
        // ensuring that he is the owner, or their aren't any objects yet
        // so the point is moot.
        //
        // In any case we don't expect that this collection has already
        // gone down the path of temporarily parenting children.
        ASSERT( (!m_pOwner || pNewOwner == NULL || fShouldKeepOwner || GetCount() == 0)
                && !m_fOwnerIsFallback);

        IFCEXPECT_RETURN(!m_fOwnerIsFallback);
    }

    if (!fShouldKeepOwner)
    {
        // Remove relationship from old owner.
        if (m_pfnOwnerRenderChangedHandler && m_pOwner && currentOwner)
        {
            // When the owner is removed, propagate the rendering change to it one last time.

            //
            // Owner changed - Dirties: (None - Context specific)
            //
            m_pfnOwnerRenderChangedHandler(currentOwner, DirtyFlags::None);
        }

        m_pOwner = xref::get_weakref(pNewOwner);

        m_pfnOwnerRenderChangedHandler = pfnNewOwnerRenderChangedHandler;

        // Fire the rendering change for the new owner.
        if (pNewOwner && pfnNewOwnerRenderChangedHandler)
        {
            //
            // Owner changed - Dirties: (None - Context specific)
            //
            pfnNewOwnerRenderChangedHandler(pNewOwner, DirtyFlags::None);
        }
    }

    m_fOwnerIsFallback = pNewOwner && fOwnerIsFallback;

    return S_OK;
}

CDependencyObject* const CCollection::GetOwner() const
{
    return m_pOwner.lock_noref();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This method propagates dirty flags to the parent and owner DOs via their respective
//      stored dirty flag functions.
//
//------------------------------------------------------------------------
void
CCollection::NWPropagateDirtyFlag(DirtyFlags flags)
{
    NW_ASSERT_CAN_PROPAGATE_DIRTY_FLAGS

    // If the collection has an owner, call its dirty flag function.
    CDependencyObject *pOwner = GetOwner();
    if (pOwner && m_pfnOwnerRenderChangedHandler)
    {
        (m_pfnOwnerRenderChangedHandler)(pOwner, flags);
    }

    // Propagate to the parent DO via the base implementation.
    CDependencyObject::NWPropagateDirtyFlag(flags);
}

void CCollection::SetAffectsOwnerMeasure(XUINT32 value)
{
    m_fAffectsOwnerMeasure = value;
}

void CCollection::SetAffectsOwnerArrange(XUINT32 value)
{
    m_fAffectsOwnerArrange = value;
}

_Check_return_
HRESULT
CCollection::GetItemCount(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(0) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
)
{
    CCollection *pCollection = NULL;

    IFC_RETURN(ValidateParams(0 /* cArgsExpected */, pObject, cArgs, ppArgs));

    pCollection = static_cast<CCollection*>(pObject);

    pResult->SetFloat((XFLOAT)pCollection->GetCount());

    return S_OK;
}

_Check_return_ HRESULT CCollection::PreAddToCollection(_In_ CValue& newItem)
{
    if (newItem.GetType() == valueObject)
    {
        CDependencyObject* newDO = newItem.AsObject();

        // verify that the incoming DO is associated with the same thread as this collection
        if (GetCurrentThreadId() != newDO->GetContext()->GetThreadID())
        {
            IFC_RETURN(RPC_E_WRONG_THREAD);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CCollection::Add(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(1) CValue *ppArgs,
    _Out_opt_ CValue *pResult)
{
    CCollection* collection = nullptr;
    XUINT32 index = 0;

    IFC_RETURN(DoPointerCast(collection, pObject));
    IFC_RETURN(collection->FailIfLocked());

    IFC_RETURN(ValidateParams(1 /* cArgsExpected */, pObject, cArgs, ppArgs));
    CValue& arg = ppArgs[0];

    IFC_RETURN(collection->PreAddToCollection(arg));
    IFC_RETURN(collection->Append(arg, &index));   // TODO: The only place that uses the index is CTimelineMarkerCollection::Append.
    IFC_RETURN(collection->OnAddToCollection(arg));

    if (pResult)
    {
        pResult->SetFloat(static_cast<XFLOAT>(index));
    }

    return S_OK;
}


_Check_return_
HRESULT
CCollection::Clear(
    _In_reads_bytes_(sizeof(CCollection))CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_opt_(0) CValue *ppArgs,
    _Out_opt_ CValue *pResult
)
{
    CCollection *pCollection = NULL;

    IFC_RETURN(ValidateParams(0 /* cArgsExpected */, pObject, cArgs, ppArgs));

    IFC_RETURN(static_cast<CCollection *>(pObject)->FailIfLocked());

    pCollection = static_cast<CCollection*>(pObject);

    // Remove all from the collection....
    IFC_RETURN(pCollection->Clear());

    return S_OK;
}

_Check_return_
HRESULT
CCollection::Count(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_opt_(0) CValue *ppArgs,
    _Out_ CValue *pResult
)
{
    IFC_RETURN(ValidateParams(0 /* cArgsExpected */, pObject, cArgs, ppArgs));

    IFC_RETURN(static_cast<CCollection *>(pObject)->FailIfLocked());
    pResult->SetSigned(static_cast<CCollection*>(pObject)->GetCount());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   getItem
//
//  Synopsis: Simplified version for C# calls
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CCollection::GetItem(
_In_ CDependencyObject *pObject,
 _In_ XUINT32 index,
 _Out_ CValue *pResult)

{
    CCollection *pCollection = static_cast<CCollection *>(pObject);
    // Call the real GetItem. This does an AddRef.
    void* pItem = pCollection->GetItemWithAddRef(index);

    // We transfer the reference to the CValue, so that's why we don't call Release here.
    pCollection->ItemToCValue(pResult, pItem, false, true);
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Method:   InsertDO
//
//  Synopsis:
//      Insert a DO into the collection.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCollection::InsertDO(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 nIndex,
    _In_ CValue& value)
{
    CCollection* collection = nullptr;

    IFC_RETURN(DoPointerCast(collection, pObject));

    IFC_RETURN(collection->PreAddToCollection(value));
    IFC_RETURN(collection->Insert(nIndex, value));
    IFC_RETURN(collection->OnAddToCollection(value));

    return S_OK;
}

_Check_return_
HRESULT
CCollection::IndexOf(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(1) CValue *ppArgs,
    _Out_ CValue *pResult
)
{
    XINT32 iIndex;

  // Do parameter validation
    IFC_RETURN(ValidateParams(1 /* cArgsExpected */, pObject, cArgs, ppArgs));

    IFC_RETURN(static_cast<CCollection *>(pObject)->IndexOf(ppArgs[0], &iIndex));

    pResult->SetSigned(iIndex);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ValidateParams
//
//  Synopsis:
// Common Validation code for Static members.  Individual functions may have additional
// validation requirements.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CCollection::ValidateParams(_In_ XUINT32 cArgsExpected,
                                            _In_ CDependencyObject *pObject,
                                            _In_ XUINT32 cArgs,
                                            _In_reads_(cArgs) CValue *ppArgs)
{
    if (cArgsExpected != cArgs
        || !pObject
        || !pObject->OfTypeByIndex<KnownTypeIndex::PresentationFrameworkCollection>()
        )
        return E_INVALIDARG;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetItem
//
//  Synopsis:
//  Sets a collectionItem to CValue setting the appropriate
//  type according to collection type
//  fMAnaged flag shows if the function is called througs
//------------------------------------------------------------------------
void CCollection::ItemToCValue(_In_ CValue *pResult, _In_ void *pItem, bool fDeleteValueObject, bool fManaged)
{
    CDependencyObject* pDO = NULL;
    CREATEPARAMETERS param(GetContext());
    XPOINTF *pPoint = NULL;

    // TODO: This class should not have specializations for specific derived types.
    //                 It should also be updated to support templating the collection element type.

    //set the corresponding value type for value type collections
    switch (GetTypeIndex())
    {
        case KnownTypeIndex::PointCollection:
            pPoint = static_cast<XPOINTF *>(pItem);

            if(fManaged) //wrap it in a DO for JS
            {
                pResult->SetPoint(pPoint);
            }
            else
            {
                param.m_value.SetPoint(pPoint);

                if(SUCCEEDED(CPoint::Create(&pDO, &param)))
                {
                    pResult->SetObjectNoRef(pDO);
                }
            }

            if (fDeleteValueObject)
            {
                delete pPoint;
            }
            break;

        case KnownTypeIndex::DoubleCollection:
            if(pItem)
            {
                XFLOAT *pFloat = static_cast<XFLOAT*>(pItem);

                if(fManaged) //wrap it in a DO for JS
                {
                    pResult->SetFloat(*pFloat);
                }
                else
                {
                    param.m_value.SetFloat(*pFloat);

                    if(SUCCEEDED(CDouble::Create(&pDO, &param)))
                    {
                        pResult->SetObjectNoRef(pDO);
                    }
                }

                if (fDeleteValueObject)
                {
                    delete pFloat;
                }
            }
            break;

        case KnownTypeIndex::TextRangeCollection:
            if (pItem)
            {
                auto textRangeData = static_cast<TextRangeData*>(pItem);

                if (fManaged) //wrap it in a DO for JS
                {
                    pResult->Set<valueTextRange>(*textRangeData);
                }
                else
                {
                    param.m_value.Set<valueTextRange>(*textRangeData);

                    if (SUCCEEDED(CTextRange::Create(&pDO, &param)))
                    {
                        pResult->SetObjectNoRef(pDO);
                    }
                }

                delete textRangeData;
            }
            break;

        default: //this are all CDOCollection descendants
            // The caller already did an addref.
            pResult->SetObjectNoRef(static_cast<CDependencyObject *>(pItem));
            break;
    }
}

_Check_return_ HRESULT CCollection::FailIfLocked()
{
    if (m_cLock > 0)
    {
        IFC_RETURN(this->SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_COLLECTION_MODIFIED_DURING_LAYOUT));
    }

    return S_OK;
}
