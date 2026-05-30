// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "DirectUIXamlQualifiedObject.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Get the value of a XamlQualifiedObject.
_Check_return_ HRESULT DirectUI::XamlQualifiedObject::GetValue(
    _Out_ IInspectable** ppValue,
    _In_ bool fPegNoRef /*= true*/
    ) const
{
    // Get the value out of the object and look it up via its handle
    ctl::ComPtr<IInspectable> spValue;
    IFC_RETURN(CValueBoxer::UnboxObjectValue(&m_value, /* pTargetType */ nullptr, &spValue));
    
    // Ensure that the value is pegged, we wil unpeg it later in LoadFromXaml
    // TODO: Is this really necessary? SetValue is already doing the pegging
    if( fPegNoRef )
    {
        auto spDO = spValue.AsOrNull<xaml::IDependencyObject>();
        if (spDO)
        {
            spDO.Cast<DependencyObject>()->PegNoRef();
        }
    }

    // Handover the reference to the caller
    *ppValue = spValue.Detach();
    return S_OK;
}

// Set the value of a XamlQualifiedObject.
_Check_return_ HRESULT DirectUI::XamlQualifiedObject::SetValue(
    _In_ IInspectable* pValue,
    _In_ const CClassInfo* pValueType,
    bool fNewlyCreated)
{
    BoxerBuffer buffer;

    // Set the type token
    m_typeToken = XamlTypeToken::FromType(pValueType);
    
    m_flags.ClearAllBits();

    // Set the value
    ctl::ComPtr<DependencyObject> spDO;
    IFC_RETURN(CValueBoxer::BoxObjectValue(&m_value, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject), pValue, &buffer, &spDO));

    if (fNewlyCreated)
    {
        if (spDO == nullptr)
        {
            // If we didn't get a MOR, it's likely that the value itself is a DO.
            spDO = ctl::query_interface_cast<xaml::IDependencyObject>(pValue).Cast<DependencyObject>();
        }

        if (spDO != nullptr)
        {
            spDO->PegNoRef();
            m_flags = QualifiedObjectFlags(m_flags | qofHasPeggedManagedPeer);
        }
    }

    return S_OK;
}
