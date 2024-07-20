// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT CDefinitionBase::ActualSize(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
)
{
    if (!cArgs && !pArgs && pResult)
    {
        auto def = static_cast<CDefinitionBase*>(pObject);
        pResult->SetFloat(def->GetActualSize());
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

float CDefinitionBase::GetActualSize() const
{
    CDependencyObject* parent = GetParentInternal(false);

    return (parent && parent->OfTypeByIndex<KnownTypeIndex::Grid>())
        ? m_measureArrangeSize
        : 0.0f;
}

_Check_return_ HRESULT CRowDefinition::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CDependencyObject::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::RowDefinition_Height:
        case KnownPropertyIndex::RowDefinition_MaxHeight:
        case KnownPropertyIndex::RowDefinition_MinHeight:
        {
            CDependencyObject* pParent = GetParentInternal(false);
            if (pParent && pParent->OfTypeByIndex<KnownTypeIndex::Grid>())
            {
                static_cast<CGrid*>(pParent)->InvalidateDefinitions();
            }
            break;
        }
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT CColumnDefinition::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CDependencyObject::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::ColumnDefinition_Width:
        case KnownPropertyIndex::ColumnDefinition_MaxWidth:
        case KnownPropertyIndex::ColumnDefinition_MinWidth:
        {
            CDependencyObject* pParent = GetParentInternal(false);
            if (pParent && pParent->OfTypeByIndex<KnownTypeIndex::Grid>())
            {
                static_cast<CGrid*>(pParent)->InvalidateDefinitions();
            }
            break;
        }
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT CDefinitionBase::FromString(_In_ CREATEPARAMETERS* pCreate) 
{
    if ((pCreate->m_value).AsString().IsNullOrEmpty())
    {
        IFC_RETURN(E_INVALIDARG);
    }
  
    IFC_RETURN(GridLengthFromString((pCreate->m_value).AsString(), &(this->m_pUserSize)));

    return S_OK;
}

