// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "definitioncollection.h"
#include "grid.h"
#include "dopointercast.h"
#include "StringConversions.h"
#include "GridDefinitions.h"
#include "stack_vector.h"
//------------------------------------------------------------------------
//
//  Method:   CDefinitionCollectionBase::OnCollectionChanged
//
//  Synopsis:
//      Invalidate the owner grid's defintion structure
//------------------------------------------------------------------------

void CDefinitionCollectionBase::OnCollectionChanged()
{
    auto gridOwner = do_pointer_cast<CGrid>(GetOwner());
    if (gridOwner)
    {
        gridOwner->InvalidateDefinitions();
    }
}

//------------------------------------------------------------------------
//
//  Method:   CDefinitionCollectionBase::OnAddToCollection
//
//  Synopsis:
//      Override to act on collection change
//------------------------------------------------------------------------

 _Check_return_ HRESULT CDefinitionCollectionBase::OnAddToCollection(_In_ CDependencyObject *pDO)
{
    IFC_RETURN(CDOCollection::OnAddToCollection(pDO));

    OnCollectionChanged();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDefinitionCollectionBase::OnRemoveFromCollection
//
//  Synopsis:
//      Override to act on collection change
//------------------------------------------------------------------------
 _Check_return_ HRESULT CDefinitionCollectionBase::OnRemoveFromCollection(_In_ CDependencyObject *pDO, _In_ INT32 previousIndex)
{
    IFC_RETURN(CDOCollection::OnRemoveFromCollection(pDO, previousIndex));
    
    OnCollectionChanged();
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDefinitionCollectionBase::OnClear
//
//  Synopsis:
//      Override to act on collection change
//------------------------------------------------------------------------

 _Check_return_ HRESULT CDefinitionCollectionBase::OnClear()
{
    IFC_RETURN(CDOCollection::OnClear());
    
    OnCollectionChanged();

    return S_OK;
}

 _Check_return_ HRESULT CRowDefinitionCollection::FromString(_In_ CREATEPARAMETERS* pCreate)
 {
     Jupiter::stack_vector<xstring_ptr, 8> heightStrings;
     IFC_RETURN(ParseGridDefinitionCollectionInitializationString((pCreate->m_value).AsString(), heightStrings));

     for (const auto& height : heightStrings.m_vector)
     {
         CREATEPARAMETERS cp(pCreate->m_pCore, height);
         xref_ptr<CRowDefinition> definition;
         IFC_RETURN(CRowDefinition::Create((CDependencyObject**)definition.ReleaseAndGetAddressOf(), &cp));

         IFC_RETURN(Append(definition.get()));
     }

     return S_OK;
 }

 _Check_return_ HRESULT CColumnDefinitionCollection::FromString(_In_ CREATEPARAMETERS* pCreate)
 {
    Jupiter::stack_vector<xstring_ptr, 8> widthStrings;
    IFC_RETURN(ParseGridDefinitionCollectionInitializationString((pCreate->m_value).AsString(), widthStrings));

    for (const auto& width : widthStrings.m_vector)
    {
        CREATEPARAMETERS cp(pCreate->m_pCore, width);
        xref_ptr<CColumnDefinition> definition;
        IFC_RETURN(CColumnDefinition::Create((CDependencyObject**)definition.ReleaseAndGetAddressOf(), &cp));

        IFC_RETURN(Append(definition.get()));
     }

     return S_OK;
 }
