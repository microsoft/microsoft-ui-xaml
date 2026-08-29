// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlTypeTokens.h"

XamlTypeNamespaceToken XamlTypeNamespaceToken::FromNamespace(_In_ const CNamespaceInfo* pNamespace)
{
    return XamlTypeNamespaceToken(pNamespace->IsBuiltinNamespace() ? tpkNative : tpkManaged, pNamespace->GetIndex());
}

XamlTypeToken XamlTypeToken::FromType(_In_ const CClassInfo* pType)
{
    return XamlTypeToken((pType->RequiresPeerActivation() || pType->IsInterface()) ? tpkManaged : tpkNative, pType->GetIndex());
}

XamlPropertyToken XamlPropertyToken::FromProperty(_In_ const CPropertyBase* pProperty)
{
    // Route custom DPs through the managed provider *for now* to make sure we continue using FX TemplateBindings, because 
    // CDependencyObject::SetValue doesn't know how to repackage IInspectable values yet.
    // Also route properties of interface types through the managed provider *for now* because they generally use EORs.
    return XamlPropertyToken((
        (pProperty->Is<CCustomProperty>() || pProperty->Is<CCustomDependencyProperty>() || pProperty->GetPropertyType()->IsInterface())
            ? tpkManaged
            : tpkNative),
        pProperty->GetIndex());
}
