// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// TODO: add a way for the TypeInfoProvider to create a 
// XamlAssembly and populate the xmlns/type-ns pairs

#include "XamlTypeTokens.h"

class XamlTypeNamespace;
class XamlSchemaContext;
class XamlAssembly;

typedef xstring_ptr StringPtr;
typedef xvector<StringPtr> StringList;
typedef std::shared_ptr<StringList> StringListPtr;

class XamlAssembly
{
public:
    XamlAssembly(
        const std::shared_ptr<XamlSchemaContext>& inSchemaContext,
        _In_ const xstring_ptr& inAssemblyName, 
        XamlAssemblyToken inXamlAssemblyToken
        )
    {
        m_spSchemaContext = inSchemaContext;
        m_spAssemblyName = inAssemblyName;
        m_XamlAssemblyToken = inXamlAssemblyToken;
    }
    
    _Check_return_ HRESULT GetTypeNamespace_ForBinaryXaml(
        const std::shared_ptr<XamlAssembly>& spAssembly,
        _In_ const xstring_ptr& inTypeNamespace,
        std::shared_ptr<XamlTypeNamespace>& outXamlTypeNamespace
        );
        
    // GetTypeInfoProviderKind
    //
    // Each Assembly will have only one TypeInfoProviderKind
    XamlTypeInfoProviderKind GetTypeInfoProviderKind()
    {
        return m_XamlAssemblyToken.GetProviderKind();
    }

    _Check_return_ HRESULT get_Name(_Out_ xstring_ptr* pstrOutName);
    
    const XamlAssemblyToken& get_AssemblyToken() const
    {
        return m_XamlAssemblyToken;
    }
    
        
private:

    _Check_return_ HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext);

    xstring_ptr m_spAssemblyName;
    
    XamlAssemblyToken m_XamlAssemblyToken;
    std::weak_ptr<XamlSchemaContext> m_spSchemaContext;

    // from system.xaml XmlNsInfo
    //
    // internal Dictionary<string, IList<string>> ClrToXmlNs { get; private set; }
    // internal Dictionary<string, string> OldToNewNs { get; private set; }
    // internal Dictionary<string, string> Prefixes { get; private set; }
};


