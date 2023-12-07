// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//
// PInvokes Stub
//
//   The only PInvoke from DXaml Layer is for registering the namespaces for
//   DXaml types.
//
namespace CoreImports
{
_Check_return_ HRESULT XamlSchemaContext_AddAssemblyXmlnsDefinition(
    _In_ CCoreServices* pCore,
    _In_ XamlAssemblyToken tAssembly,
    _In_ const xstring_ptr& strXmlNamespace,
    _In_ XamlTypeNamespaceToken tTypeNamespace,
    _In_ const xstring_ptr& strTypeNamespace)
{
    HRESULT hr = S_OK;

    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;

    spXamlSchemaContext = g_pCore->GetSchemaContext();
    IFCEXPECT(spXamlSchemaContext);

    IFC(spXamlSchemaContext->AddAssemblyXmlnsDefinition(tAssembly, strXmlNamespace, tTypeNamespace, strTypeNamespace));

Cleanup:
    RRETURN(hr);
}
}