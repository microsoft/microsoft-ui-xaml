// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    // RPInvoke callbacks for the XAML parser and a few helpers to implement
    // them.
    class XamlParserCallbacks
    {
    public:
        // Create an instance of the given DXaml type.  This is currently only
        // defined for custom DXaml types derived from DependencyObject and uses
        // TypeInfo::CreateInstance to instantiate the object
        // (including it's peer core instance).
        static _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_CreateInstance(
            _In_ XamlTypeToken tokType,
            _Out_ ::XamlQualifiedObject* pqoInstance);

        // Create an instance of the given DXaml type by parsing a textual
        // representation of the value.  This only DXaml types that have an
        // associated text syntax are custom enums.  This method is more closely
        // tied to the static type tables in TypeInfo.g.h than it should be, but
        // we're making that simplification for now until we can decide our
        // generalized text syntax strategy.
        static _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_CreateFromValue(
            _In_ void* pServiceProviderContext,
            _In_ XamlTypeToken tokTextSyntaxType,
            _In_ ::XamlQualifiedObject* pqoValue,
            _In_ XamlPropertyToken tokProperty,
            _In_ ::XamlQualifiedObject* pqoRootInstance,
            _Out_ ::XamlQualifiedObject* pqoInstance);

        // Get the value of a member on a particular instance.  This currently
        // fails if the member is an event.
        static _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_GetValue(
            _In_ const ::XamlQualifiedObject* pqoInstance,
            _In_ XamlPropertyToken tokProperty,
            _Out_ ::XamlQualifiedObject* pqoValue);

        // Get the ambient value of a member.
        static _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_GetAmbientValue(
            _In_ const ::XamlQualifiedObject* pqoInstance,
            _In_ XamlPropertyToken tokProperty,
            _Out_ ::XamlQualifiedObject* pqoValue);

        // Set the value of a member on a particular instance.  This currently
        // fails if the member is an event.
        static _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_SetValue(
            _In_ ::XamlQualifiedObject* pqoInstance,
            _In_ XamlPropertyToken tokProperty,
            _In_ ::XamlQualifiedObject* pqoValue);

        // Provide a value for a markup extension.
        static _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_CallProvideValue(
            _In_ ::XamlQualifiedObject* pqoMarkupExtension,
            _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            _Out_ ::XamlQualifiedObject* pqoOutValue);

        // Add a value to an instance of a collection.  This currently only
        // works for types derived from PresentationFrameworkCollection.
        static _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_Add(
            _In_ ::XamlQualifiedObject* pqoCollection,
            _In_ ::XamlQualifiedObject* pqoValue);

        // Add a key/value pair to an instance of a dictionary.  This currently
        // only attempts to add to a ResourceDictionary, though it always fails
        // with E_NOTIMPL right now.
        static _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_AddToDictionary(
            _In_ ::XamlQualifiedObject* pqoDictionary,
            _In_ ::XamlQualifiedObject* pqoKey,
            _In_ ::XamlQualifiedObject* pqoValue);

        // The parser has found an x:ConnectionId attribute 
        // and needs to wire up the event based on the connection Id.
        static _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_SetConnectionId(
            _In_ ::XamlQualifiedObject* qoComponentConnector, 
            _In_ ::XamlQualifiedObject* qoConnectionId, 
            _In_ ::XamlQualifiedObject* qoTarget);

        static _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_GetXBindConnector(
            _In_ ::XamlQualifiedObject* qoComponentConnector,
            _In_ ::XamlQualifiedObject* qoConnectionId,
            _In_ ::XamlQualifiedObject* qoTarget,
            _In_ ::XamlQualifiedObject* qoReturnConnector);

        static _Check_return_ HRESULT FrameworkCallbacks_CheckPeerType(
            _In_ CDependencyObject* nativeRoot, 
            _In_ const xstring_ptr& strPeerType, 
            _In_ XINT32 bCheckExact);

    private:
        // Attempt to resolve the name of a type in a given namespace.  This
        // returns a new token corresponding to the type if found and a value
        // indicating whether the token was found (the value is redundant since
        // the token is empty, but the value makes it easier to chain together a
        // series of lookups and short circuit after one is successful).  This 
        // is not a callback.
        static bool TryGetTypeWithSpecifiedNamespaceId(
            _In_ KnownNamespaceIndex nNamespaceIndex,
            _In_ const xstring_ptr& strTypeName,
            _In_ bool bBuiltinTypeOnly,
            _Out_ XamlTypeToken* ptokType);

        static _Check_return_ HRESULT AddXmlnsDefinition(_In_ const xstring_ptr& strXmlNamespace, _In_ const xstring_ptr& strNamespace);
    };
};
