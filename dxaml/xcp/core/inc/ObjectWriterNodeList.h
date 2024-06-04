// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ObjectWriterNode.h"

class XamlSchemaContext;

// Nodes representing the commands from the ObjectWriter.
// Each node information is self contained is a description
// of the operation and schema/value parameters that is used
// to reconstruct the operation.
class ObjectWriterNodeList
{
public:
    ObjectWriterNodeList(_In_ std::shared_ptr<XamlSchemaContext> spSchemaContext) 
        : m_spSchemaContext(spSchemaContext)
    {
    }
    
    _Check_return_ HRESULT AddNode(
        _In_ ObjectWriterNode&& node
        );

    ObjectWriterNode PopNode();
    
    const ObjectWriterNode& PeekTopNode() const;

    _Check_return_ HRESULT Optimize();
 
#if DBG
    _Check_return_ HRESULT Dump(); 
#endif

    std::shared_ptr<XamlSchemaContext> GetSchemaContext();
 
    std::vector<ObjectWriterNode>& GetNodeList();   

private:
    void ProcessOptimizedList(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_AggregatePushScopeOperation(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_BeginInit(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_CheckPeerType(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_CreateTypeWithInitialValue(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_SetName(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_EndInit(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_TypeConvertValue(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_SetDirectiveProperty(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_SetDeferredProperty(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_SetValue(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_PopScope(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_ProvideValue(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_SetConnectionId(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void Optimize_GetResourcePropertyBag(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    void DebugFailure(
        _In_ std::vector<ObjectWriterNode>& nodeList);

    template<std::size_t EXPECTEDCOUNT>
    bool TryPopRequiredElements(
        _In_ const std::array<ObjectWriterNodeType, EXPECTEDCOUNT>& expectedStackTopNodeType,
        _In_ std::vector<ObjectWriterNode>& nodeList,
        _Out_ std::vector<ObjectWriterNode>& poppedList);

    template<std::size_t EXPECTEDCOUNT>
    bool FoundRequiredElements(
        _In_ const std::array<ObjectWriterNodeType, EXPECTEDCOUNT>& expectedStackTopNodeType,
        _In_ const std::vector<ObjectWriterNode>& nodeList);

#if DBG
    _Check_return_ HRESULT Dump(
        _In_ std::vector<ObjectWriterNode>& nodeList, xstring_ptr& strIndent);
#endif

    std::shared_ptr<XamlSchemaContext> m_spSchemaContext;
    std::vector<ObjectWriterNode> m_nodeList;
    std::vector<ObjectWriterNode> m_nodeListOptimized;
};

