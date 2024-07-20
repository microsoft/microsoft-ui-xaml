// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ObjectWriterContext.h>

HRESULT XamlSavedContext::Create( 
            _In_ const std::shared_ptr<ObjectWriterContext>& spObjectWriterContext, 
            _In_ std::shared_ptr< ObjectWriterStack >& spObjectWriterStack, 
            _Out_ std::shared_ptr<XamlSavedContext>& rspXamlSavedContext )
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    IFC_RETURN(spObjectWriterContext->get_SchemaContext(spXamlSchemaContext));

    rspXamlSavedContext = std::make_shared<XamlSavedContext>(
        spXamlSchemaContext, 
        spObjectWriterStack, 
        spObjectWriterContext->get_BaseUri(),
        spObjectWriterContext->get_XamlResourceUri(), 
        spObjectWriterContext->get_XbfHash());
    
    return S_OK;
}


