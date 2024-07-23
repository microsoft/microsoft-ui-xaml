// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Given that the XAML parser returns a stream of XamlNodes intead of a
//      parse tree, we need to store the context of parents and ancestors
//      necsesary to resolve namespaces, type info, etc.  The context works by
//      adding and remove frames as we begin and finish parsing the elements
//      they correspond to.

#include "precomp.h"

XamlParserContext::XamlParserContext(const std::shared_ptr<XamlSchemaContext>& inSchemaContext)
    : XamlContext(inSchemaContext)
{
}

HRESULT XamlParserContext::Create(
            _In_ const std::shared_ptr<XamlSchemaContext>& inSchemaContext, 
            _In_ std::shared_ptr<XamlParserContext>& outParserContext)
{
    outParserContext = std::make_shared<XamlParserContext>(inSchemaContext);
    // ZPTD: Consider giving different initial size.

    outParserContext->m_PrescopeNamespaces = std::make_shared<containers::vector_map<xstring_ptr, std::shared_ptr<XamlNamespace>>>();
    IFC_RETURN(outParserContext->PopulateDefaultNamespaces());

    return S_OK;
}

void XamlParserContext::AddIgnoredUri(_In_ const xstring_ptr& inUriToIgnore)
{
    m_IgnoredNamespaceUris.insert({ inUriToIgnore, false });
}

bool XamlParserContext::IsNamespaceUriIgnored(_In_ const xstring_ptr& uri)
{
    return m_IgnoredNamespaceUris.find(uri) != m_IgnoredNamespaceUris.end();
}

HRESULT XamlParserContext::AddNamespacePrefix(
            _In_ const xstring_ptr& inPrefix, 
            _In_ const std::shared_ptr<XamlNamespace>& inXamlNamespace)
{
    // TODO: We allow adding a null namespace here.
    auto result = m_PrescopeNamespaces->insert({ inPrefix, inXamlNamespace });

    if (result.second) RRETURN(S_OK);

    auto itResult = result.first;
    itResult->second = inXamlNamespace;

    RRETURN(S_FALSE);
}

std::shared_ptr<XamlNamespace> XamlParserContext::FindNamespaceByPrefix(
            _In_ const xstring_ptr& inPrefix)
{
    std::shared_ptr<XamlNamespace> outNamespace;

    auto itNamespace = m_PrescopeNamespaces->find(inPrefix);
    if (itNamespace == m_PrescopeNamespaces->end())
    {
        for (const auto& frame : m_parserStack)
        {
            outNamespace = frame.GetNamespaceByPrefix(inPrefix);
            if (outNamespace)
            {
                break;
            }
        }
    }
    else
    {
        outNamespace = itNamespace->second;
    }

    return outNamespace;
}

void XamlParserContext::PushScope(bool fPushNamescopes /* = true */)
{
    XamlParserFrame frame;
    if (fPushNamescopes)
    {
        // Put the namespaces into the new frame, then reset them for the next scope.
        frame.SetNamespaces(m_PrescopeNamespaces);
        m_PrescopeNamespaces = std::make_shared<tNamespaceMap::element_type>();
    }
    else
    {
        frame.SetNamespaces(std::make_shared<tNamespaceMap::element_type>());
    }
    m_parserStack.push_back(frame);
}

