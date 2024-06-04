// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

HRESULT XamlScannerStack::Push(const std::shared_ptr<XamlType>& inType)
{
    bool bParentSpacePreserve = false;

    // Get whether the parent was preserving whitespace (unless we're at the top
    // of the stack which then defaults to FALSE)
    if (m_Stack.size() > 0)
    {
        bParentSpacePreserve = get_CurrentXmlSpacePreserve();
    }
    
    IFC_RETURN(m_Stack.push(XamlScannerFrame(inType)));
    m_itTopOfStack = m_Stack.begin();

    // Store whether the parent was preserving whitespace on the child
    set_CurrentXmlSpacePreserve(bParentSpacePreserve);
    
    return S_OK;
}

HRESULT XamlScannerStack::Pop()
{
    IFC_RETURN(m_Stack.pop())
    m_itTopOfStack = m_Stack.begin();

    return S_OK;
}

