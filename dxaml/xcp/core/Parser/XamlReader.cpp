// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

ReaderDelegate::ReaderDelegate(const std::shared_ptr<XamlSchemaContext>& inSchemaContext, const std::shared_ptr<IGetIndexedNodeDelegate>& inGetNodeDelegate, XUINT32 uNodeCount)
    : m_uNodeCount(uNodeCount)
    , m_SchemaContext(inSchemaContext)
    , m_GetNodeDelegate(inGetNodeDelegate)
    , m_uUptoIndex(static_cast<XUINT32>(-1))
{

}

ReaderDelegate::~ReaderDelegate()
{

}

const XamlNode& ReaderDelegate::CurrentNode()
{
    // Only call this function
    return m_GetNodeDelegate->UnsafeGetNodeByIndex(m_uUptoIndex);
}

HRESULT ReaderDelegate::Read()
{
    // NOTE: The index starts at -1.
    m_uUptoIndex++;
    if (m_uUptoIndex < m_uNodeCount)
    {
        RRETURN(S_OK);
    }
    else
    {
        RRETURN(S_FALSE);
    }
}

HRESULT ReaderDelegate::GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_SchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}


