// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Resources.h"
#include "DOPointerCast.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

using namespace Theming;
using namespace Resources;

//------------------------------------------------------------------------
//
//  Method:   SetResourceOwner
//
//  Synopsis: Stores in the weak reference resource dictionary owner and
//            propagates owner value to merged dictionaries
//------------------------------------------------------------------------
void CResourceDictionaryCollection::SetResourceOwner(_In_opt_ CDependencyObject* const pResourceOwner)
{
    XUINT32 nCount = GetCount();

    m_pResourceOwner = pResourceOwner;
    if (nCount > 0)
    {
        for (XUINT32 i = 0; i < nCount; ++i)
        {
            CDependencyObject* pDO = GetItemDOWithAddRef(i);
            checked_cast<CResourceDictionary>(pDO)->SetResourceOwner(pResourceOwner);
            ReleaseInterface(pDO);
        }
    }
}

CDependencyObject* CResourceDictionaryCollection::GetResourceOwnerNoRef() const
{
    return m_pResourceOwner;
}

//------------------------------------------------------------------------
//
//  Method:   HasImplicitStyle
//
//  Synopsis: Returns TRUE if one of merged dictionaries has an implicit style
//------------------------------------------------------------------------
_Check_return_ bool
CResourceDictionaryCollection::HasImplicitStyle()
{
    CDependencyObject * pDO = NULL;

    XUINT32 nCount = GetCount();

    if (nCount > 0)
    {
        for (XUINT32 i = 0; i < nCount; ++i)
        {
            CResourceDictionary * pDictionary = NULL;
            pDO = GetItemDOWithAddRef(i);
            VERIFYHR(DoPointerCast(pDictionary, pDO));
            if(pDictionary && pDictionary->HasImplicitStyle())
            {
                ReleaseInterface(pDO);
                return true;
            }
            ReleaseInterface(pDO);
        }
    }
    return false;
}

//------------------------------------------------------------------------
//
//  Method:   InvalidateImplicitStyles
//
//  Synopsis: Invalidates all implicit styles on all merged dictionaries
//------------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionaryCollection::InvalidateImplicitStyles()
{
    HRESULT hr = S_OK;
    CDependencyObject * pDO = NULL;

    XUINT32 nCount = GetCount();
    if (nCount > 0)
    {
        for (XUINT32 i = 0; i < nCount; ++i)
        {
            CResourceDictionary * pDictionary = NULL;
            pDO = GetItemDOWithAddRef(i);
            IFC(DoPointerCast(pDictionary, pDO));
            if (pDictionary->HasImplicitStyle())
            {
                IFC(pDictionary->InvalidateImplicitStyles(pDictionary));
            }
            ReleaseInterface(pDO);
        }
    }

Cleanup:
    ReleaseInterface(pDO);
    return hr;
}

//------------------------------------------------------------------
//
// Method: OnAddToCollection
//
// Synopsis: When adding to CResourceDictionaryCollection, ensure that the add
// will not create a cycle.
//------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionaryCollection::OnAddToCollection(_In_ CDependencyObject *pDO)
{
    HRESULT hr = S_OK;
    CResourceDictionary* pDictionary = NULL;

    IFC(DoPointerCast(pDictionary, pDO));
    hr = pDictionary->CycleCheckInternal(pDictionary);
    if (FAILED(hr))
    {
        // Break cycle so that we don't leak memory
        IGNOREHR(Remove(pDO));
        ReleaseInterface(pDO);
        IFC(hr);
    }

    if (CResourceDictionary* parentDictionary = do_pointer_cast<CResourceDictionary>(GetParentInternal(false)))
    {
        // If we are adding a resource dictionary, we need to invalidate not-found caches of all resource dictionaries above,
        // since potentially they could have been added.  Clearing is faster and more practical than invalidating individual keys.

        parentDictionary->InvalidateNotFoundCache(true);
    }

    pDictionary->SetResourceOwner(m_pResourceOwner);
    if (pDictionary->HasImplicitStyle())
    {
        IFC(pDictionary->InvalidateImplicitStyles(NULL));
    }

    IFC(CDOCollection::OnAddToCollection(pDO));

Cleanup:
    return hr;
}

//------------------------------------------------------------------
//
// Method: OnRemoveFromCollection
//
// Synopsis: When removing from CResourceDictionaryCollection we should
//           invalidate all existing implicit styles on that entry which
//           we are removing
//------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionaryCollection::OnRemoveFromCollection(_In_ CDependencyObject *pDO, _In_ XINT32 iPreviousIndex)
{
    CResourceDictionary * pDictionary = NULL;

    IFC_RETURN(DoPointerCast(pDictionary, pDO));
    if (pDictionary->HasImplicitStyle())
    {
        IFC_RETURN(pDictionary->InvalidateImplicitStyles(pDictionary));
    }
    pDictionary->SetResourceOwner(nullptr);

    IFC_RETURN(CDOCollection::OnRemoveFromCollection(pDO, iPreviousIndex));

    return S_OK;
}

//------------------------------------------------------------------
//
// Method:   Clear
//
// Synopsis: When we clear CResourceDictionaryCollection we should invalidate
//           all implicit styles on merged dictionaries
//
// Note: since we are appending elements to a temporary collection,
//       we are changing parents. This causes base implementation of Clear
//       to miss some side effects. See ClearNoInvalidationOfImplicitStyles.
//------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionaryCollection::Clear()
{
    HRESULT hr = S_OK;
    CDependencyObject * pObject = NULL;
    CResourceDictionaryCollection * pOldDictionaries = NULL;
    bool fPeggedOldDictionary = false;

    XUINT32 nCount = GetCount();
    if (nCount > 0)
    {
        CREATEPARAMETERS cp(GetContext());

        IFC(CResourceDictionaryCollection::Create(&pObject, &cp));
        IFC(DoPointerCast(pOldDictionaries, pObject));

        // Peg the temporary dictionary collection while the we append entries into it.
        // Without the peg, each Append() will cause the temp
        // ResourceDictionaryCollection peer to be recreated, firing ASSERT(HasEverHadManagedPeer())
        IFC(pOldDictionaries->PegManagedPeer(false, &fPeggedOldDictionary));

        for (XUINT32 i = 0; i < nCount; ++i)
        {
            CResourceDictionary * pDictionary = NULL;
            pObject = GetItemDOWithAddRef(i);
            IFC(DoPointerCast(pDictionary, pObject));
            IFC(pOldDictionaries->Append(pDictionary));
            ReleaseInterface(pObject);
        }
    }

    IFC(CDOCollection::Clear());

    if (pOldDictionaries)
    {
        IFC(pOldDictionaries->InvalidateImplicitStyles());
        // has sideeffects that were skipped in the clear of this dictionary because it was not the parent.
        // The dtor of pOldDictionaries will not run on the Cleanup because there is a managed peer holding on to it
        // We rely on having cleared the elements though, so the method we are calling here will ensure these
        // side-effects.
        IFC(pOldDictionaries->ClearNoInvalidationOfImplicitStyles());
    }

Cleanup:
    if (fPeggedOldDictionary)
    {
        pOldDictionaries->UnpegManagedPeer();
    }
    ReleaseInterface(pOldDictionaries);
    ReleaseInterface(pObject);
    return hr;
}

//------------------------------------------------------------------
// Synopsis: Regular clear method will invalidate implicitstyles
// which makes the elements miss some sideeffects of clear. The
// temporary collection needs to be cleared.
//------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionaryCollection::ClearNoInvalidationOfImplicitStyles()
{
    RRETURN(CDOCollection::Clear());
}

KnownTypeIndex CResourceDictionaryCollection::GetTypeIndex() const
{
    return DependencyObjectTraits<CResourceDictionaryCollection>::Index;
}
