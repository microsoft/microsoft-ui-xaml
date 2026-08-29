// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ObjectWriterContext;
class ObjectWriterStack;
class XamlSchemaContext;

#include <INamescope.h>

class XamlSavedContext 
{
private:
    std::weak_ptr<XamlSchemaContext> m_spSchemaContext;
    std::shared_ptr<ObjectWriterStack> m_spObjectWriterStack;
    xref_ptr<IPALUri> m_spBaseUri;
    xref_ptr<IPALUri> m_spXamlResourceUri;
    xstring_ptr m_xbfHash;
    
public:

    XamlSavedContext(
            _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext, 
            _In_ const std::shared_ptr<ObjectWriterStack>& spObjectWriterStack, 
            _In_ const xref_ptr<IPALUri>& spBaseUri,
            _In_ const xref_ptr<IPALUri>& spXamlResourceUri, 
            _In_ const xstring_ptr& strXbfHash)
            : m_spSchemaContext(spXamlSchemaContext)
            , m_spObjectWriterStack(spObjectWriterStack)
            , m_spBaseUri(spBaseUri)
            , m_spXamlResourceUri(spXamlResourceUri)
            , m_xbfHash(strXbfHash)
    {
    }

    ~XamlSavedContext()
    {
    }
    static HRESULT Create(
                _In_ const std::shared_ptr<ObjectWriterContext>& spObjectWriterContext, 
                _In_ std::shared_ptr<ObjectWriterStack>& spObjectWriterStack, 
                _Out_ std::shared_ptr<XamlSavedContext>& rspXamlSavedContext);

    HRESULT get_SchemaContext(_Out_ std::shared_ptr<XamlSchemaContext>& spSchemaContext )
    {
        spSchemaContext = m_spSchemaContext.lock();
        if (!spSchemaContext)
        {
            IFC_RETURN(E_FAIL);
        }
        return S_OK;
    }

    HRESULT get_Stack(_Out_ std::shared_ptr<ObjectWriterStack>& rspObjectWriterStack)
    {
        rspObjectWriterStack = m_spObjectWriterStack;
        return S_OK;
    }

    HRESULT get_BaseUri(_Out_ xref_ptr<IPALUri>& rspBaseUri)
    {
        rspBaseUri = m_spBaseUri;
        return S_OK;
    }

    HRESULT get_XamlResourceUri(_Out_ xref_ptr<IPALUri>& rspXamlResourceUri)
    {
        rspXamlResourceUri = m_spXamlResourceUri;
        return S_OK;
    }

    const xstring_ptr& get_XbfHash() const
    {
        return m_xbfHash;
    }
};

