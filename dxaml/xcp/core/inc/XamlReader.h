// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlNode.h"
#include "XamlSchemaContext.h"

class XamlNode;
class XamlSchemaContext;

class IGetIndexedNodeDelegate
{
public:
    virtual HRESULT GetNodeByIndex(XUINT32 inIndex, XamlNode& outNode) = 0;
    virtual const XamlNode& UnsafeGetNodeByIndex(XUINT32 inIndex) = 0;
};

class XamlReader
{
public:
    virtual ~XamlReader() {}
    virtual HRESULT Read() = 0;
    virtual const XamlNode& CurrentNode() = 0;
    virtual HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext) = 0;
    virtual HRESULT set_NextIndex(XUINT32 uiIndex)   = 0;
    virtual HRESULT get_NextIndex(XUINT32 *puiIndex) = 0;
};

class ReaderDelegate : public XamlReader
{
public:
    ReaderDelegate(const std::shared_ptr<XamlSchemaContext>& inSchemaContext, const std::shared_ptr<IGetIndexedNodeDelegate>& inGetNodeDelegate, XUINT32 uNodeCount);
    ~ReaderDelegate() override;
    HRESULT Read() override;
    const XamlNode& CurrentNode() override;
    HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext) override;

private:
    std::weak_ptr<XamlSchemaContext> m_SchemaContext;
    std::shared_ptr<IGetIndexedNodeDelegate> m_GetNodeDelegate;
    XUINT32 m_uNodeCount;
    XUINT32 m_uUptoIndex;
};