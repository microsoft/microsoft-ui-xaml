// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

CTransformGroup::~CTransformGroup()
{
    ReleaseInterface(m_pChild);
}

_Check_return_ HRESULT CTransformGroup::GetTransformValue(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    if (pObject)
    {
        CMILMatrix transGroupMatrix;
        CMatrix *pMatrix = NULL;

        CTransformGroup* pTransformGroup = static_cast<CTransformGroup*>(pObject);
        CREATEPARAMETERS cp(pTransformGroup->GetContext());

        pTransformGroup->GetTransform( &transGroupMatrix );
        IFC_RETURN(CMatrix::Create((CDependencyObject **)&pMatrix, &cp));
        pMatrix->m_matrix = transGroupMatrix;

        pResult->SetObjectNoRef(pMatrix);
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

CTransformGroup::CTransformGroup(_In_ const CTransformGroup& original, _Out_ HRESULT& hr) : CTransform(original,hr)
{
    CREATEPARAMETERS            cp(GetContext());
    CTransformCollection       *pCollectionClone = NULL;
    CNoParentShareableDependencyObject *pChildTransform = NULL;
    CNoParentShareableDependencyObject *pChildTransformClone = NULL;

    m_pChild = NULL;

    IFC(hr);

    if (original.m_pChild)
    {
        IFC(CreateDO(&pCollectionClone, &cp));

        for (XUINT32 i = 0; i < original.m_pChild->GetCount(); i++)
        {
            // Can't use DoPointerCast as GetItemWithAddRef() returns void*
            pChildTransform = reinterpret_cast<CNoParentShareableDependencyObject*>(original.m_pChild->GetItemWithAddRef(i));
            IFC(pChildTransform->Clone(&pChildTransformClone));
            ReleaseInterface(pChildTransform);

            IFC(pCollectionClone->Append(pChildTransformClone));
            ReleaseInterface(pChildTransformClone);
        }

        IFC(SetValueByKnownIndex(KnownPropertyIndex::TransformGroup_Children, pCollectionClone)); // Adds ref to collection
    }

    // The clone will only need to recalculate its cached matrix if the original did.
    m_groupMatrix = original.m_groupMatrix;
    m_uiCacheUpdatedFrame = original.m_uiCacheUpdatedFrame;
    m_fNWTransformsDirty = original.m_fNWTransformsDirty;

Cleanup:
    ReleaseInterface(pCollectionClone);
    ReleaseInterface(pChildTransform);
    ReleaseInterface(pChildTransformClone);
}

// Update the cached transform we have by multiplying the other transforms in
void CTransformGroup::UpdateCachedTransform()
{
    m_groupMatrix.SetToIdentity();

    if (m_pChild)
    {
        for (auto& child : *m_pChild)
        {
            if (child)
            {
                CMILMatrix childMatrix;

                static_cast<CTransform*>(child)->GetTransform(&childMatrix);
                m_groupMatrix.Append(childMatrix);
            }
        }
    }

    // Mark the cached transform as being up-to-date.
     m_fNWTransformsDirty = FALSE;
}
