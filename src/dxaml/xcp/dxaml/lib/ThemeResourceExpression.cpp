// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ThemeResource.h"
#include "ThemeResourceExpression.h"
#include "ValueBuffer.h"
#include "CValueUtil.h"

using namespace ::Windows::Internal;
using namespace DirectUI;

// Initializes a new instance of the ThemeResourceExpression class.
ThemeResourceExpression::ThemeResourceExpression():
    m_pCoreThemeResource(nullptr)
{
}

// Destroys an instance of the ThemeResourceExpression class.
ThemeResourceExpression::~ThemeResourceExpression()
{
    ReleaseInterface(m_pCoreThemeResource);
}

// Initializes a new instance of the ThemeResourceExpression class.
_Check_return_ HRESULT ThemeResourceExpression::Create(
    _In_ CThemeResource* pCoreThemeResource,
    _Out_ ThemeResourceExpression** ppExpression)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ThemeResourceExpression> spExpression;
    
    IFCPTR(pCoreThemeResource);
    IFCPTR(ppExpression);

    IFC(ctl::make(&spExpression));

    // Initialize theme resource binding expression.
    spExpression->m_pCoreThemeResource = pCoreThemeResource;
    pCoreThemeResource->AddRef();

    *ppExpression = spExpression.Detach();

Cleanup:
    RRETURN(hr);
}
// Gets a value indicating whether the value can be set.  ThemeResource bindings are
// always removed if a new value is set, so this always returns FALSE.
_Check_return_ HRESULT ThemeResourceExpression::GetCanSetValue(_Out_ bool *pValue)
{
    *pValue = FALSE;
    return S_OK;
}

// Gets a value indicating whether the expression has been associated with a
// target.
bool ThemeResourceExpression::GetIsAssociated()
{
    return false;
}

// Attach the expression to a specific property on the target object.
_Check_return_ HRESULT ThemeResourceExpression::OnAttach(
    _In_ DependencyObject* pTarget,
    _In_ const CDependencyProperty* pTargetProperty)
{
    RRETURN(S_OK);
}

// Detach the expression from the target object.
_Check_return_ HRESULT ThemeResourceExpression::OnDetach()
{
    RRETURN(S_OK);
}

// Get the value of the target object's target property.
_Check_return_ HRESULT ThemeResourceExpression::GetValue(
    _In_ DependencyObject* pObject,
    _In_ const CDependencyProperty* pProperty,
    _Out_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    IFC(GetThemeValueFromCore(m_pCoreThemeResource, pProperty, ppValue));
    
Cleanup:
    RRETURN(hr);
}

// Get last resolved value of the target object's target property.
_Check_return_ HRESULT ThemeResourceExpression::GetLastResolvedThemeValue(
    _Out_ IInspectable** ppValue)
{
    RRETURN(GetLastResolvedThemeValueFromCore(m_pCoreThemeResource, ppValue));
}

// Get property's value from core, by looking up theme dictionary
_Check_return_ HRESULT ThemeResourceExpression::GetThemeValueFromCore(
    _In_ CThemeResource* pCoreThemeResource,
    _In_ const CDependencyProperty* pProperty,
    _Out_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    ValueBuffer valueBuffer(DXamlCore::GetCurrent()->GetHandle());

    IFCPTR(pCoreThemeResource);
    IFCPTR(ppValue);

    // Get property's value from theme dictionary
    IFC(pCoreThemeResource->GetValue(&valueBuffer));

    // Repackage the value so the types match
    IFC(valueBuffer.Repackage(pProperty));

    IFC(CValueBoxer::UnboxObjectValue(&valueBuffer, pProperty->GetPropertyType(), ppValue));

Cleanup:
    RRETURN(hr);
}

// Get last resolved value of the target object's target property.
_Check_return_ HRESULT ThemeResourceExpression::GetLastResolvedThemeValueFromCore(
    _In_ CThemeResource* pCoreThemeResource,
    _Out_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    const CClassInfo* pType = NULL;
    CValue valueFromCore;
    KnownTypeIndex valueTypeIndex = KnownTypeIndex::UnknownType;

    IFCPTR(pCoreThemeResource);
    IFCPTR(ppValue);

    IFC(pCoreThemeResource->GetLastResolvedThemeValue(&valueFromCore));

    // If we got the type of the value in the core, resolve the corresponding TypeInfo.
    valueTypeIndex = CValueUtil::GetTypeIndex(valueFromCore);

    if (valueTypeIndex != KnownTypeIndex::UnknownType)
    {
        pType = MetadataAPI::GetClassInfoByIndex(valueTypeIndex);
    }
    
    IFC(CValueBoxer::UnboxObjectValue(&valueFromCore, pType, ppValue));

Cleanup:
    RRETURN(hr);
}


// Set/Clear a theme resource binding. pCoreThemeResourceExtension is NULL to clear.
_Check_return_ HRESULT ThemeResourceExpression::SetThemeResourceBinding2(
    _In_ CDependencyObject* pdo,
    _In_ KnownPropertyIndex nPropertyID,
    _In_ ::BaseValueSource baseValueSource,
    _In_opt_ CThemeResource* pCoreThemeResource)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> spTarget;
    const CDependencyProperty* pTargetProperty = nullptr;
    ctl::ComPtr<ThemeResourceExpression> spExpression;
    ctl::ComPtr<IInspectable> spValue;
    bool bWasFrozen = false;

    // Use TryGetPeer, in case peer is shutting down, which can occur, for example,
    // if an animated value is set during the DO's shutdown. Core calls EnsurePeer
    // before calling this, so peer will be created when needed.
    IFC(DXamlCore::GetCurrent()->TryGetPeer(pdo, &spTarget));        
    if (!spTarget)
    {
        goto Cleanup;
    }        

    // Lookup the DP.
    pTargetProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyID);
    
    if (pCoreThemeResource)
    {
        // Create the ThemeResourceExpression.
        IFC(ThemeResourceExpression::Create(pCoreThemeResource, &spExpression));

        IFC(CoreImports::DependencyObject_Unfreeze(pdo, &bWasFrozen));

        // Assign the expression to the target DP.
        IFC(spTarget->SetValueExpression(pTargetProperty, spExpression.Get(), baseValueSource));
    }
    else
    {
        IFC(spTarget->ClearCorePropertyThemeResourceExpression(pTargetProperty));
    }
    
Cleanup:
    if (bWasFrozen)
    {
        IGNOREHR(CoreImports::DependencyObject_Freeze(pdo));
    }

    RRETURN(hr);
}

// Return the CThemeResource from a ThemeResourceExpression if one exists. Otherwise 
// return NULL.
_Check_return_ HRESULT
ThemeResourceExpression::GetThemeResourceNoRef(
    _In_ CDependencyObject* pdo,
    _In_ KnownPropertyIndex nPropertyID,
    _Out_ CThemeResource** ppThemeResourceNoRef)
{
    ctl::ComPtr<DependencyObject> spTarget;

    *ppThemeResourceNoRef = nullptr;

    // Get the managed peer.
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pdo, &spTarget));

    // If we don't have a peer, we don't have an expression to work with.
    if (spTarget)
    {
        // Get the effective value entry if one exists.
        EffectiveValueEntry* pValueEntry = spTarget->TryGetEffectiveValueEntry(nPropertyID);

        if (pValueEntry && pValueEntry->IsExpression())
        {
            ctl::ComPtr<IInspectable> spExpressionInsp = pValueEntry->GetBaseValue();
            ctl::ComPtr<IThemeResourceExpression> spExpression = spExpressionInsp.AsOrNull<IThemeResourceExpression>();

            if (spExpression)
            {
                *ppThemeResourceNoRef = spExpression.Cast<ThemeResourceExpression>()->m_pCoreThemeResource;
            }
        }
    }

    return S_OK;
}
