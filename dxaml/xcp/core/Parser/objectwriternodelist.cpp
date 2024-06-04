// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ObjectWriterNodeList.h"
#include "KeyTime.h"
#include "CustomWriterRuntimeData.h"
#include "SubObjectWriterResult.h"
#include "XamlBinaryFormatStringConverter.h"
#include <CColor.h>
#include <StringConversions.h>

HRESULT ObjectWriterNodeList::AddNode(_In_ ObjectWriterNode&& node)
{
    try
    {
        // if the first node added isn't a PushScope then there was an implicit PushScope by the ObjectWriter.
        // in order to account for this, add one explicitly in the nodelist
        if (m_nodeList.empty() && node.GetNodeType() != ObjectWriterNodeType::PushScope)
        {
            m_nodeList.emplace_back(ObjectWriterNode::MakePushScopeNode(XamlLineInfo()));
        }

        m_nodeList.emplace_back(std::move(node));

        // TODO: optimize the node list inplace
        // we aren't doing this right now while we stabilize
        // because errors in optimization logic
        // will hinder debugging issues with the object writer command list
    }
    CATCH_RETURN();

    return S_OK;
}

ObjectWriterNode ObjectWriterNodeList::PopNode()
{
    ASSERT(m_nodeListOptimized.empty());

    ObjectWriterNode node(std::move(m_nodeList.back()));
    m_nodeList.pop_back();

    return node;
}

const ObjectWriterNode& ObjectWriterNodeList::PeekTopNode() const
{
    ASSERT(m_nodeListOptimized.empty());

    return m_nodeList.back();
}

// optimize the nodes by aggregating operations
HRESULT ObjectWriterNodeList::Optimize()
{
    try
    {
        std::vector<ObjectWriterNode> nodeListOptimized;
        if (!m_nodeList.empty())
        {
            ASSERT(m_nodeListOptimized.empty());

            for (auto& node : m_nodeList)
            {
                nodeListOptimized.push_back(node);
                ProcessOptimizedList(nodeListOptimized);
            }
        }

        m_nodeListOptimized = std::move(nodeListOptimized);
    }
    CATCH_RETURN();

    return S_OK;
}

void ObjectWriterNodeList::ProcessOptimizedList(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    auto stackTopItr = nodeList.rbegin();
    auto& stackTop = *stackTopItr;

    switch (stackTop.GetNodeType())
    {
        case ObjectWriterNodeType::CreateTypeWithInitialValue:
        {
            Optimize_CreateTypeWithInitialValue(nodeList);
        }
        break;

        case ObjectWriterNodeType::CreateType:
        case ObjectWriterNodeType::AddNamespace:
        case ObjectWriterNodeType::GetValue:
        {
            Optimize_AggregatePushScopeOperation(nodeList);
        }
        break;

        case ObjectWriterNodeType::BeginInit:
        {
            Optimize_BeginInit(nodeList);
        }
        break;

        case ObjectWriterNodeType::EndInit:
        {
            Optimize_EndInit(nodeList);
        }
        break;

        case ObjectWriterNodeType::TypeConvertValue:
        {
            Optimize_TypeConvertValue(nodeList);
        }
        break;

        case ObjectWriterNodeType::SetDirectiveProperty:
        {
            Optimize_SetDirectiveProperty(nodeList);
        }
        break;

        case ObjectWriterNodeType::SetDeferredProperty:
        {
            Optimize_SetDeferredProperty(nodeList);
        }
        break;

        case ObjectWriterNodeType::SetValue:
        {
            Optimize_SetValue(nodeList);
        }
        break;

        case ObjectWriterNodeType::PopScope:
        {
            Optimize_PopScope(nodeList);
        }
        break;

        case ObjectWriterNodeType::CheckPeerType:
        {
            Optimize_CheckPeerType(nodeList);
        }
        break;

        case ObjectWriterNodeType::SetName:
        {
            Optimize_SetName(nodeList);
        }
        break;

        case ObjectWriterNodeType::ProvideValue:
        {
            Optimize_ProvideValue(nodeList);
        }
        break;

        case ObjectWriterNodeType::GetResourcePropertyBag:
        {
            Optimize_GetResourcePropertyBag(nodeList);
        }
        break;

        case ObjectWriterNodeType::SetConnectionId:
        {
            Optimize_SetConnectionId(nodeList);
        }
        break;

        default:
            break;
    }
}

// attempts to validate the stack has the matching node types
template<std::size_t EXPECTEDCOUNT>
bool ObjectWriterNodeList::FoundRequiredElements(
    _In_ const std::array<ObjectWriterNodeType, EXPECTEDCOUNT>& expectedStackTopNodeType,
    _In_ const std::vector<ObjectWriterNode>& nodeList)
{
    bool found = false;

    if (expectedStackTopNodeType.size() <= nodeList.size())
    {
        found = std::equal(
            expectedStackTopNodeType.rbegin(),
            expectedStackTopNodeType.rend(),
            nodeList.rbegin(),
            [](const ObjectWriterNodeType& lhs, const ObjectWriterNode& rhs) {
            return lhs == rhs.GetNodeType();
        });
    }

    return found;
}

// attempts to validate and pop a requested number
// of elements if a matching node type criteria is met
template<std::size_t EXPECTEDCOUNT>
bool ObjectWriterNodeList::TryPopRequiredElements(
    _In_ const std::array<ObjectWriterNodeType, EXPECTEDCOUNT>& expectedStackTopNodeType,
    _In_ std::vector<ObjectWriterNode>& nodeList,
    _Out_ std::vector<ObjectWriterNode>& poppedList)
{
    bool found = FoundRequiredElements(expectedStackTopNodeType, nodeList);

    if (found)
    {
        poppedList.clear();
        poppedList.reserve(EXPECTEDCOUNT);
        auto it = nodeList.end() - EXPECTEDCOUNT;
        std::move(it, nodeList.end(), back_inserter(poppedList));
        nodeList.erase(it, nodeList.end());
    }

    return found;
}

// aggregate operations which will create a new stack frame
//
// * addnamespace - creates a frame to hold the namespaces for the scope
// * createtype   - creates a frame to hold the instance type
// * getvalue     - creates a frame to hold the retrieved instance type
//
// the scope creation becomes implicit as part of the instruction type for addnamespace, createtype, getvalue
void ObjectWriterNodeList::Optimize_AggregatePushScopeOperation(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::AddNamespace } }, nodeList, poppedList))
    {
        // Transform:
        //     PushScope
        //       AddNamespace
        // To:
        //     PushScopeAddNamespace
        auto& addNamespaceNode = poppedList[1];
        auto& pushScopeNode = poppedList[0];

        // Retain line information from the AddNamespace node
        auto lineInfo = pushScopeNode.GetLineInfo();
        auto& strNamespacePrefix = addNamespaceNode.GetStringValue();
        auto& spNamespace = addNamespaceNode.GetNamespace();

        nodeList.push_back(ObjectWriterNode::MakePushScopeAddNamespaceNode(
            lineInfo,
            strNamespacePrefix,
            spNamespace));
    }
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::CreateType } }, nodeList, poppedList))
    {
        // Transform:
        //     PushScope
        //       CreateType
        // To:
        //     PushScopeCreateType
        auto& createTypeNode = poppedList[1];
        auto& pushScopeNode = poppedList[0];

        // Retain line information from the CreateType node
        auto lineInfo = pushScopeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();

        nodeList.push_back(ObjectWriterNode::MakePushScopeCreateTypeNode(lineInfo, spType));
    }
    else if (TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::BeginConditionalScope, ObjectWriterNodeType::CreateType } }, nodeList, poppedList))
    {
        // Transform:
        //     PushScope
        //       BeginConditionalScope
        //       CreateType
        // To:
        //     PushScopeCreateType
        //       BeginConditionalScope
        //
        // Required because the CreateType/BeginInit node pair is not added to the nodelist until a WriteMember (or WriteEndObject)
        // is encountered. If the WriteMember was preceded by a WriteConditionalScope, then a BeginConditionalScope node
        // will be added to the nodelist *before* the CreateType/BeginInit node pair, even though it properly belongs after them.
        // This transformation (combined with one when the BeginInit node is reached) will migrate the BeginConditionalScope node
        // to the proper position.
        auto& createTypeNode = poppedList[2];
        auto& BeginConditionalScopeNode = poppedList[1];
        auto& pushScopeNode = poppedList[0];

        // Retain line information from the CreateType node
        auto lineInfo = pushScopeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();

        nodeList.push_back(ObjectWriterNode::MakePushScopeCreateTypeNode(lineInfo, spType));
        nodeList.push_back(std::move(BeginConditionalScopeNode));
    }
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::GetValue } }, nodeList, poppedList))
    {
        // Transform:
        //     PushScope
        //       GetValue
        // To:
        //     PushScopeGetValue
        auto& getValueNode = poppedList[1];
        auto& pushScopeNode = poppedList[0];

        // Retain line information from the GetValue node
        auto lineInfo = pushScopeNode.GetLineInfo();
        auto& spProperty = getValueNode.GetXamlProperty();

        nodeList.push_back(ObjectWriterNode::MakePushScopeGetValueNode(lineInfo, spProperty));
    }
    else
    {
        if (!FoundRequiredElements<2>({ { ObjectWriterNodeType::CheckPeerType, ObjectWriterNodeType::CreateType } }, nodeList) &&
            !FoundRequiredElements<2>({ { ObjectWriterNodeType::AddNamespace, ObjectWriterNodeType::CreateType } }, nodeList) &&
            !FoundRequiredElements<2>({ { ObjectWriterNodeType::PushScopeAddNamespace, ObjectWriterNodeType::CreateType } }, nodeList) &&
            !FoundRequiredElements<2>({ { ObjectWriterNodeType::AddNamespace, ObjectWriterNodeType::AddNamespace } }, nodeList) &&
            !FoundRequiredElements<2>({ { ObjectWriterNodeType::PushScopeAddNamespace, ObjectWriterNodeType::AddNamespace } }, nodeList))
        {
            DebugFailure(nodeList);
        }
    }
}

// aggregate the initialization with the creation of type
void ObjectWriterNodeList::Optimize_BeginInit(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushScopeCreateType, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList))
    {
        // Transform:
        //     PushScopeCreateType
        //       BeginInit
        // To:
        //     PushScopeCreateTypeBeginInit
        auto& createTypeNode = poppedList[0];

        // Retain line information from the creation node
        auto lineInfo = createTypeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();

        nodeList.push_back(ObjectWriterNode::MakePushScopeCreateTypeBeginInitNode(lineInfo, spType));
    }
    else if (TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScopeCreateType, ObjectWriterNodeType::BeginConditionalScope, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList))
    {
        // Transform:
        //     PushScopeCreateType
        //       BeginConditionalScope
        //       BeginInit
        // To:
        //     PushScopeCreateTypeBeginInit
        //       BeginConditionalScope
        //
        // Required because the CreateType/BeginInit node pair is not added to the nodelist until a WriteMember (or WriteEndObject)
        // is encountered. If the WriteMember was preceded by a WriteConditionalScope, then a BeginConditionalScope node
        // will be added to the nodelist *before* the CreateType/BeginInit node pair, even though it properly belongs after them.
        // This transformation will migrate the BeginConditionalScope node to the proper position.
        auto& createTypeNode = poppedList[0];
        auto& BeginConditionalScopeNode = poppedList[1];

        // Retain line information from the creation node
        auto lineInfo = createTypeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();

        nodeList.push_back(ObjectWriterNode::MakePushScopeCreateTypeBeginInitNode(lineInfo, spType));
        nodeList.push_back(std::move(BeginConditionalScopeNode));
    }
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::CreateType, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList))
    {
        // Transform:
        //     CreateType
        //     BeginInit
        // To:
        //     CreateTypeBeginInit
        auto& createTypeNode = poppedList[0];

        // Retain line information from the creation node
        auto lineInfo = createTypeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();

        nodeList.push_back(ObjectWriterNode::MakeCreateTypeBeginInitNode(lineInfo, spType));
    }
    else if (TryPopRequiredElements<3>({ { ObjectWriterNodeType::AddNamespace, ObjectWriterNodeType::CreateTypeWithConstant, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList) ||
             TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScopeAddNamespace, ObjectWriterNodeType::CreateTypeWithConstant, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList) ||
             TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::CreateTypeWithConstant, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList))
    {
        // Transform:
        //     PushScope
        //       CreateTypeWithConstant
        //       BeginInit
        // To:
        //     PushScopeCreateTypeWithConstantBeginInit
        // (or)
        //     [PushScope]AddNamespace
        //     CreateTypeWithConstant
        //     BeginInit
        // To:
        //     [PushScope]AddNamespace
        //     CreateTypeWithConstantBeginInit
        auto& firstNode = poppedList[0];
        auto& createTypeNode = poppedList[1];

        // Retain line information from the creation node
        auto lineInfo = createTypeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();
        auto& spValue = createTypeNode.GetValue();

        if (firstNode.GetNodeType() == ObjectWriterNodeType::PushScope)
        {
            nodeList.push_back(ObjectWriterNode::MakePushScopeCreateTypeWithConstantBeginInitNode(lineInfo, spType, spValue));
        }
        else
        {
            nodeList.push_back(firstNode);
            nodeList.push_back(ObjectWriterNode::MakeCreateTypeWithConstantBeginInitNode(lineInfo, spType, spValue));
        }
    }
    else if (TryPopRequiredElements<3>({ { ObjectWriterNodeType::AddNamespace, ObjectWriterNodeType::CreateTypeWithTypeConvertedConstant, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList) ||
             TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScopeAddNamespace, ObjectWriterNodeType::CreateTypeWithTypeConvertedConstant, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList) ||
             TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::CreateTypeWithTypeConvertedConstant, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList))
    {
        // Transform:
        //     PushScope
        //       CreateTypeWithTypeConvertedConstant
        //       BeginInit
        // To:
        //     PushScopeCreateTypeWithTypeConvertedConstantBeginInit
        // (or)
        //     [PushScope]AddNamespace
        //     CreateTypeWithTypeConvertedConstant
        //     BeginInit
        // To:
        //     [PushScope]AddNamespace
        //     CreateTypeWithTypeConvertedConstantBeginInit
        auto& firstNode = poppedList[0];
        auto& createTypeNode = poppedList[1];

        // Retain line information from the creation node
        auto lineInfo = createTypeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();
        auto& spValue = createTypeNode.GetValue();
        auto& spTypeConverter = createTypeNode.GetTypeConverter();

        if (firstNode.GetNodeType() == ObjectWriterNodeType::PushScope)
        {
            nodeList.push_back(ObjectWriterNode::MakePushScopeCreateTypeWithTypeConvertedConstantBeginInitNode(lineInfo, spType, spTypeConverter, spValue));
        }
        else
        {
            nodeList.push_back(firstNode);
            nodeList.push_back(ObjectWriterNode::MakeCreateTypeWithTypeConvertedConstantBeginInitNode(lineInfo, spType, spTypeConverter, spValue));
        }
    }
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::CreateTypeWithTypeConvertedConstant, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList))
    {
        // Transform:
        //     CreateTypeWithTypeConvertedConstant
        //       BeginInit
        // To:
        //     CreateTypeWithTypeConvertedConstantBeginInit
        auto& createTypeNode = poppedList[0];

        // Retain line information from the creation node
        auto lineInfo = createTypeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();
        auto& spValue = createTypeNode.GetValue();
        auto& spTypeConverter = createTypeNode.GetTypeConverter();

        nodeList.push_back(ObjectWriterNode::MakeCreateTypeWithTypeConvertedConstantBeginInitNode(lineInfo, spType, spTypeConverter, spValue));
    }
    else if (TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScopeCreateType, ObjectWriterNodeType::SetName, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList))
    {
        // Transform:
        //     PushScopeCreateType
        //         SetName
        //         BeginInit
        // To:
        //     PushScopeCreateTypeBeginInit
        //         SetName
        auto& createTypeNode = poppedList[0];
        auto& setNameNode = poppedList[1];

        // Retain line information from the creation node
        auto lineInfo = createTypeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();

        nodeList.push_back(ObjectWriterNode::MakePushScopeCreateTypeBeginInitNode(lineInfo, spType));
        nodeList.push_back(std::move(setNameNode));
    }
    else if (TryPopRequiredElements<4>({ {
                ObjectWriterNodeType::PushScopeCreateType,
                ObjectWriterNodeType::BeginConditionalScope,
                ObjectWriterNodeType::SetName,
                ObjectWriterNodeType::BeginInit } },
            nodeList, poppedList))
    {
        // Transform:
        //     PushScopeCreateType
        //       BeginConditionalScope
        //       SetName
        //       BeginInit
        // To:
        //     PushScopeCreateTypeBeginInit
        //       SetName
        //       BeginConditionalScope
        //
        // Required because the CreateType/SetName/BeginInit node triplet is not added to the nodelist until a WriteMember (or WriteEndObject)
        // is encountered. If the WriteMember was preceded by a WriteConditionalScope, then a BeginConditionalScope node
        // will be added to the nodelist *before* the CreateType/SetName/BeginInit node triplet, even though it properly belongs after them.
        // This transformation will migrate the BeginConditionalScope node to the proper position.
        auto& createTypeNode = poppedList[0];
        auto& BeginConditionalScopeNode = poppedList[1];
        auto& setNameNode = poppedList[2];

        // Retain line information from the creation node
        auto lineInfo = createTypeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();

        nodeList.push_back(ObjectWriterNode::MakePushScopeCreateTypeBeginInitNode(lineInfo, spType));
        nodeList.push_back(std::move(setNameNode));
        nodeList.push_back(std::move(BeginConditionalScopeNode));
    }
    else if (TryPopRequiredElements<3>({ { ObjectWriterNodeType::CreateType, ObjectWriterNodeType::SetName, ObjectWriterNodeType::BeginInit } }, nodeList, poppedList))
    {
        // Transform:
        //     CreateType
        //     SetName
        //     BeginInit
        // To:
        //     CreateTypeBeginInit
        //     SetName
        auto& createTypeNode = poppedList[0];
        auto& setNameNode = poppedList[1];

        // Retain line information from the creation node
        auto lineInfo = createTypeNode.GetLineInfo();
        auto& spType = createTypeNode.GetXamlType();

        nodeList.push_back(ObjectWriterNode::MakeCreateTypeBeginInitNode(lineInfo, spType));
        nodeList.push_back(std::move(setNameNode));
    }
    else
    {
        if (!FoundRequiredElements<2>({ { ObjectWriterNodeType::AddNamespace, ObjectWriterNodeType::BeginInit } }, nodeList))
        {
            DebugFailure(nodeList);
        }
    }
}

// remove duplicated endinit
void ObjectWriterNodeList::Optimize_EndInit(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     EndInit
    //     EndInit
    // To:
    //     <null>
    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::EndInit, ObjectWriterNodeType::EndInit } }, nodeList, poppedList))
    {
        auto& endInitNode = poppedList[0];
        nodeList.push_back(std::move(endInitNode));
    }
    else
    {
        //DebugFailure(nodeList)
    }
}

// optimize the type conversion by either getting rid of redundant typeconversion when the constant would
// be sufficient for setvalue or by performing an intermediate conversion from string to better type
// suited for the setvalue
void ObjectWriterNodeList::Optimize_TypeConvertValue(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     PushConstant (string)
    //     TypeConvertValue
    // To:
    // Case I:
    //     PushConstant
    //
    // Case II:
    //     PushConstant (type)
    //     TypeConvertValue
    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushConstant, ObjectWriterNodeType::TypeConvertValue } }, nodeList, poppedList))
    {
        auto& constantNode = poppedList[0];
        auto& typeConvertNode = poppedList[1];

        XamlTypeToken typeToken;
        if (typeConvertNode.GetTypeConverter())
        {
            typeToken = typeConvertNode.GetTypeConverter()->get_TextSyntaxToken();
        }
        auto& constantValue = constantNode.GetValue()->GetValue();

        CCoreServices* pCore = m_spSchemaContext->GetCore();

        XamlBinaryFormatStringConverter converter(pCore);

        // For unknown types (e.g. a property on a condtionally declared object is potentially
        // unknown because the declaring type might be unresolvable right now, or the property itself
        // is unresolvable), just skip the type conversion attempt
        if (   typeToken.GetHandle() != KnownTypeIndex::UnknownType
            && converter.TryConvertValue(typeToken, constantValue))
        {
            nodeList.push_back(constantNode);

            // Keep the type convert node for values that still need conversion at run time.
            if (typeToken.GetHandle() == KnownTypeIndex::KeyTime || typeToken.GetHandle() == KnownTypeIndex::Brush)
            {
                nodeList.push_back(typeConvertNode);
            }
        }
        else
        {
            // roll back since we failed to apply our type conversion logic
            // this will propagate back at runtime as a parse exception
            // TODO: Consider actually letting this be a Generation Phase error.
            // However, there are unfathomable but true cases when code
            // paths actually recover gracefully during such parse errors, so
            // we should be careful if we decide not to keep the runtime exception.
            nodeList.push_back(constantNode);
            nodeList.push_back(typeConvertNode);
        }
    }
    else
    {
        if (!FoundRequiredElements<2>({ { ObjectWriterNodeType::PushResolvedType, ObjectWriterNodeType::TypeConvertValue } }, nodeList) &&
            !FoundRequiredElements<2>({ { ObjectWriterNodeType::PushResolvedProperty, ObjectWriterNodeType::TypeConvertValue } }, nodeList))
        {
            DebugFailure(nodeList);
        }
    }

}

// the constant value is the same as the parameter on check peer type
// and is redundant so we can remove it
void ObjectWriterNodeList::Optimize_CheckPeerType(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     PushConstant (string)
    //     CheckPeerType
    // To:
    //     CheckPeerType
    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushConstant, ObjectWriterNodeType::CheckPeerType } }, nodeList, poppedList))
    {
        auto& checkPeerTypeNode = poppedList[1];
        ASSERT(poppedList[0].GetValue()->GetValue().GetType() == valueString);

        nodeList.push_back(std::move(checkPeerTypeNode));
    }
    else
    {
        DebugFailure(nodeList);
    }
}

// the constant value is the same as the parameter on set connection id
// and is redundant so we can remove it
void ObjectWriterNodeList::Optimize_SetConnectionId(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     PushConstant (int)
    //     SetConnectionId
    // To:
    //     SetConnectionId
    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushConstant, ObjectWriterNodeType::SetConnectionId } }, nodeList, poppedList))
    {
        auto& constantNode = poppedList[0];
        auto& setConnectionIdNode = poppedList[1];
        auto& constantValue = constantNode.GetValue();
        auto& lineInfo = setConnectionIdNode.GetLineInfo();
        ASSERT(constantValue->GetValue().GetType() == valueSigned);

        nodeList.push_back(ObjectWriterNode::MakeSetConnectionIdNode(lineInfo, constantValue));
    }
    else
    {
        DebugFailure(nodeList);
    }
}

// the constant value is the same as the parameter on get resource property bag
// and is redundant so we can remove it
void ObjectWriterNodeList::Optimize_GetResourcePropertyBag(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     PushConstant (string)
    //     GetResourcePropertyBag
    // To:
    //     GetResourcePropertyBag
    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushConstant, ObjectWriterNodeType::GetResourcePropertyBag } }, nodeList, poppedList))
    {
        auto& getResourcePropertyBagNode = poppedList[1];
        ASSERT(poppedList[0].GetValue()->GetValue().GetType() == valueString);

        nodeList.push_back(std::move(getResourcePropertyBagNode));
    }
    else
    {
        DebugFailure(nodeList);
    }
}

void ObjectWriterNodeList::Optimize_SetName(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     PushConstant (string)
    //     SetName
    // To:
    //     SetName
    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushConstant, ObjectWriterNodeType::SetName } }, nodeList, poppedList))
    {
        // the constant value is the same as the parameter on set name
        // and is redundant so we can remove it
        auto& setNameNode = poppedList[1];
        ASSERT(poppedList[0].GetValue()->GetValue().GetType() == valueString);

        nodeList.push_back(std::move(setNameNode));
    }
    // Transform:
    //     SetValue (Name Property)
    //     SetName
    // To:
    //     SetName
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::SetValue, ObjectWriterNodeType::SetName } }, nodeList, poppedList) ||
        TryPopRequiredElements<2>({ { ObjectWriterNodeType::SetValueConstant, ObjectWriterNodeType::SetName } }, nodeList, poppedList))
    {
        auto& setValueNode = poppedList[0];
        auto& setNameNode = poppedList[1];

        // SetName can be coalesced with SetValue(Name) property,
        // but first make sure this is truly Name Property
        std::shared_ptr<XamlProperty> spProperty = setValueNode.GetXamlProperty();
        ASSERT(spProperty->get_PropertyToken() == XamlPropertyToken(tpkNative, KnownPropertyIndex::DependencyObject_Name));

        nodeList.push_back(std::move(setNameNode));
    }
    else
    {
        if (!FoundRequiredElements<2>({ { ObjectWriterNodeType::PushScopeCreateType, ObjectWriterNodeType::SetName } }, nodeList))
        {
            DebugFailure(nodeList);
        }
    }
}

// the x:Key and x:Name property get redundantly added during
// objectwriting phase. Since they will have a specific SetName or AddToDictionary
// node associated when they are required, we can remove the directive property sets
void ObjectWriterNodeList::Optimize_SetDirectiveProperty(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     PushConstant (string)
    //     SetDirectiveProperty
    // To:
    //     CheckPeerType (if x:Class)
    //  OR <null>
    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushConstant, ObjectWriterNodeType::SetDirectiveProperty } }, nodeList, poppedList))
    {
        auto& constantNode = poppedList[0];
        auto& directiveNode = poppedList[1];
        auto directiveProperty = std::static_pointer_cast<DirectiveProperty>(directiveNode.GetXamlProperty());

        ASSERT(directiveProperty->IsDirective());
        ASSERT(constantNode.GetValue()->GetValue().GetType() == valueString);
        ASSERT(directiveProperty->get_DirectiveKind() == XamlDirectives::xdKey
            || directiveProperty->get_DirectiveKind() == XamlDirectives::xdName
            || directiveProperty->get_DirectiveKind() == XamlDirectives::xdClass
            || directiveProperty->get_DirectiveKind() == XamlDirectives::xdDeferLoadStrategy
            || directiveProperty->get_DirectiveKind() == XamlDirectives::xdLoad
            || directiveProperty->get_DirectiveKind() == XamlDirectives::xdFieldModifier
            || directiveProperty->get_DirectiveKind() == XamlDirectives::xdClassModifier
            || directiveProperty->get_DirectiveKind() == XamlDirectives::xdSpace);

        if (directiveProperty->get_DirectiveKind() == XamlDirectives::xdClass)
        {
            xstring_ptr strClassName;
            auto& lineInfo = directiveNode.GetLineInfo();

            THROW_IF_FAILED(constantNode.GetValue()->GetCopyAsString(&strClassName));

            nodeList.push_back(ObjectWriterNode::MakeCheckPeerTypeNode(lineInfo, strClassName));
        }

    }
    // Transform:
    //     SetName
    //     SetDirectiveProperty
    // To:
    //     SetName
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::SetName, ObjectWriterNodeType::SetDirectiveProperty } }, nodeList, poppedList))
    {
        auto& setNameNode = poppedList[0];
        auto& directiveNode = poppedList[1];
        auto directiveProperty = std::static_pointer_cast<DirectiveProperty>(directiveNode.GetXamlProperty());

        ASSERT(directiveProperty->IsDirective());
        ASSERT(directiveProperty->get_DirectiveKind() == XamlDirectives::xdName);

        nodeList.push_back(setNameNode);
    }
    else
    {
        DebugFailure(nodeList);
    }
}

void ObjectWriterNodeList::Optimize_SetDeferredProperty(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    // std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     SetValue (deferredProperty)
    //     SetDeferredProperty (deferredProperty)
    // To:
    //     SetDeferredProperty (deferredProperty)
    //if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::SetValue, ObjectWriterNodeType::SetDeferredProperty } }, nodeList, poppedList))
    //{
    //    auto& setValueNode = poppedList[0];
    //    auto& setDeferredPropertyNode = poppedList[1];

    //    ASSERT(setValueNode.GetXamlProperty()->get_PropertyToken() == setDeferredPropertyNode.GetXamlProperty()->get_PropertyToken());
    //    nodeList.push_back(setDeferredPropertyNode);
    //}
    //else
    //{
    //    DebugFailure(nodeList);
    //}
}

void ObjectWriterNodeList::Optimize_PopScope(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     PushScope
    //     PopScope
    // To:
    //     <null>
    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::PopScope } }, nodeList, poppedList))
    {
        // empty scope, get rid of it.
    }
    // Transform:
    //         EndInit
    //     PopScope
    // To:
    //     EndInitPopScope
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::EndInit, ObjectWriterNodeType::PopScope } }, nodeList, poppedList))
    {
        auto& endInitNode = poppedList[0];

        // preserve the line info to match the endinit node
        auto lineInfo = endInitNode.GetLineInfo();

        nodeList.push_back(ObjectWriterNode::MakeEndInitPopScopeNode(lineInfo));
    }
    // Transform:
    //         EndInit
    //         AddToCollection/AddToDictionary/SetValue
    //     PopScope
    // To:
    //         EndInitPopScope
    //     AddToCollection/AddToDictionary/SetValue
    else if (
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::EndInit, ObjectWriterNodeType::AddToCollection, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::EndInit, ObjectWriterNodeType::AddToDictionary, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::EndInit, ObjectWriterNodeType::AddToDictionaryWithKey, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::EndInit, ObjectWriterNodeType::SetValue, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::EndInit, ObjectWriterNodeType::SetValueFromMarkupExtension, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::EndInit, ObjectWriterNodeType::ProvideValue, ObjectWriterNodeType::PopScope } }, nodeList, poppedList))
    {
        auto& endInitNode = poppedList[0];
        auto& operationNode = poppedList[1];

        // preserve the line info to match the endinit node
        auto lineInfo = endInitNode.GetLineInfo();

        nodeList.push_back(ObjectWriterNode::MakeEndInitPopScopeNode(lineInfo));
        nodeList.push_back(std::move(operationNode));
    }
    // Transform:
    //     PushScope
    //         PushConstant/SetName/SetConnectionId/GetResourcePropertyBag/SetValueConstant/SetValueTypeConvertedConstant/CheckPeerType/ProvideStaticResourceValue/ProvideThemeResourceValue
    //     PopScope
    // To:
    //     PushConstant/SetName/SetConnectionId/GetResourcePropertyBag/SetValueConstant/SetValueTypeConvertedConstant/CheckPeerType/ProvideStaticResourceValue/ProvideThemeResourceValue
    else if (
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::PushConstant, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::SetName, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::SetConnectionId, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::GetResourcePropertyBag, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::SetValueConstant, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::SetValueTypeConvertedConstant, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::SetValueTypeConvertedResolvedType, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::SetValueTypeConvertedResolvedProperty, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::CheckPeerType, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::SetValueFromStaticResource, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::SetValueFromThemeResource, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::SetValueFromTemplateBinding, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::ProvideTemplateBindingValue, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::ProvideStaticResourceValue, ObjectWriterNodeType::PopScope } }, nodeList, poppedList) ||
        TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushScope, ObjectWriterNodeType::ProvideThemeResourceValue, ObjectWriterNodeType::PopScope } }, nodeList, poppedList))
    {
        auto& operationNode = poppedList[1];
        nodeList.push_back(std::move(operationNode));
    }
    // Transform:
    //         EndInit
    //         ProvideValue/ProvideStaticResourceValue/ProvideThemeResourceValue
    //         SetValue
    //     PopScope
    // To:
    //         EndInitPopScope
    //     ProvideValue/ProvideStaticResourceValue/ProvideThemeResourceValue
    //     SetValue
    else if (
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::EndInit,
            ObjectWriterNodeType::ProvideValue,
            ObjectWriterNodeType::SetValue,
            ObjectWriterNodeType::PopScope
        } }, nodeList, poppedList)
        ||
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::EndInit,
            ObjectWriterNodeType::ProvideTemplateBindingValue,
            ObjectWriterNodeType::SetValue,
            ObjectWriterNodeType::PopScope
        } }, nodeList, poppedList)
        ||
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::EndInit,
            ObjectWriterNodeType::ProvideStaticResourceValue,
            ObjectWriterNodeType::SetValue,
            ObjectWriterNodeType::PopScope
        } }, nodeList, poppedList)
        ||
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::EndInit,
            ObjectWriterNodeType::ProvideThemeResourceValue,
            ObjectWriterNodeType::SetValue,
            ObjectWriterNodeType::PopScope
        } }, nodeList, poppedList))
    {
        auto& endInitNode = poppedList[0];
        auto& provideValueNode = poppedList[1];
        auto& setValueNode = poppedList[2];

        // preserve the line info to match the endinit node
        auto lineInfo = endInitNode.GetLineInfo();

        nodeList.push_back(ObjectWriterNode::MakeEndInitPopScopeNode(lineInfo));
        nodeList.push_back(std::move(provideValueNode));
        nodeList.push_back(std::move(setValueNode));
    }
    // Transform: [MarkupExtension-provided object as keyed resource]
    //         EndInit
    //         ProvideValue
    //         AddToDictionaryWithKey
    //     PopScope
    // To:
    //         EndInitProvideValuePopScope
    //     AddToDictionaryWithKey
    else if (
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::EndInit,
            ObjectWriterNodeType::ProvideValue,
            ObjectWriterNodeType::AddToDictionaryWithKey,
            ObjectWriterNodeType::PopScope
        } }, nodeList, poppedList))
    {
        auto& endInitNode = poppedList[0];
        auto& addToDictionaryNode = poppedList[2];

        // preserve the line info to match the endinit node
        auto lineInfo = endInitNode.GetLineInfo();

        nodeList.push_back(ObjectWriterNode::MakeEndInitProvideValuePopScopeNode(lineInfo));
        nodeList.push_back(std::move(addToDictionaryNode));
    }

    // Transform:
    //         EndInit
    //         SetValue
    //         SetCustomRuntimeData
    //     PopScope
    // To:
    //         EndInitPopScope
    //     SetValue
    //     SetCustomRuntimeData
    else if (
        TryPopRequiredElements<4>({ {
        ObjectWriterNodeType::EndInit,
        ObjectWriterNodeType::SetValue,
        ObjectWriterNodeType::SetCustomRuntimeData,
        ObjectWriterNodeType::PopScope} }, nodeList, poppedList))
    {
        auto& endInitNode = poppedList[0];
        auto& setValueNode = poppedList[1];
        auto& setCustomRuntimeDataNode = poppedList[2];

        // preserve the line info to match the endinit node
        auto lineInfo = endInitNode.GetLineInfo();

        nodeList.push_back(ObjectWriterNode::MakeEndInitPopScopeNode(lineInfo));
        nodeList.push_back(std::move(setValueNode));
        nodeList.push_back(std::move(setCustomRuntimeDataNode));
    }
    // Transform:
    //     PushScope
    //         PushConstant
    //         TypeConvertValue
    //     PopScope
    // To:
    //     PushConstant
    //     TypeConvertValue
    else if (
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::PushScope,
            ObjectWriterNodeType::PushConstant,
            ObjectWriterNodeType::TypeConvertValue,
            ObjectWriterNodeType::PopScope
        } }, nodeList, poppedList))
    {
        auto& pushConstantNode = poppedList[1];
        auto& typeConvertNode = poppedList[2];

        nodeList.push_back(std::move(pushConstantNode));
        nodeList.push_back(std::move(typeConvertNode));
    }
    // Transform:
    //     PushScope
    //         ProvideStaticResourceReference/ProvideThemeResourceReference
    //         AddToDictionary
    //     PopScope
    // To:
    //     ProvideStaticResourceReference/ProvideThemeResourceReference
    //     AddToDictionary
    else if (
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::PushScope,
            ObjectWriterNodeType::ProvideStaticResourceValue,
            ObjectWriterNodeType::AddToDictionary,
            ObjectWriterNodeType::PopScope
        } }, nodeList, poppedList)
        ||
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::PushScope,
            ObjectWriterNodeType::ProvideStaticResourceValue,
            ObjectWriterNodeType::AddToDictionaryWithKey,
            ObjectWriterNodeType::PopScope
        } }, nodeList, poppedList)
        ||
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::PushScope,
            ObjectWriterNodeType::ProvideThemeResourceValue,
            ObjectWriterNodeType::AddToDictionary,
            ObjectWriterNodeType::PopScope
        } }, nodeList, poppedList)
        ||
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::PushScope,
            ObjectWriterNodeType::ProvideThemeResourceValue,
            ObjectWriterNodeType::AddToDictionaryWithKey,
            ObjectWriterNodeType::PopScope
        } }, nodeList, poppedList))
    {
        auto& provideResourceReferenceNode = poppedList[1];
        auto& addToDictionaryNode = poppedList[2];

        nodeList.push_back(std::move(provideResourceReferenceNode));
        nodeList.push_back(std::move(addToDictionaryNode));
    }

    // Transform: [Inline Collection Text Append]
    //     PushScope
    //         PushConstant (string)
    //         AddToCollection
    //     PopScope
    // To:
    //     PushConstant (string)
    //     AddToCollection
    else if (
        TryPopRequiredElements<4>({ {
        ObjectWriterNodeType::PushScope,
        ObjectWriterNodeType::PushConstant,
        ObjectWriterNodeType::AddToCollection,
        ObjectWriterNodeType::PopScope
    } }, nodeList, poppedList))
    {
        auto& pushConstantNode = poppedList[1];
        auto& addToCollectionNode = poppedList[2];

        nodeList.push_back(std::move(pushConstantNode));
        nodeList.push_back(std::move(addToCollectionNode));
    }
    else
    {
        //DebugFailure(nodeList);
    }
}

void ObjectWriterNodeList::Optimize_SetValue(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     PushConstant
    //     SetValue
    // To:
    //     SetValueConstant
    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushConstant, ObjectWriterNodeType::SetValue } }, nodeList, poppedList))
    {
        auto& constantNode = poppedList[0];
        auto& setValueNode = poppedList[1];

        // loss of line information as we are losing track of the source location of
        // the source value and using the property set line information instead
        auto lineInfo = setValueNode.GetLineInfo();
        auto& spProperty = setValueNode.GetXamlProperty();
        auto& spValue = constantNode.GetValue();

        nodeList.push_back(ObjectWriterNode::MakeSetValueConstantNode(lineInfo, spProperty, spValue));
    }
    // Transform:
    //     SetName
    //     SetValue (Name Property)
    // To:
    //     SetName
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::SetName, ObjectWriterNodeType::SetValue } }, nodeList, poppedList))
    {
        auto& setNameNode = poppedList[0];
        auto& setValueNode = poppedList[1];

        // SetName can be coalesced with SetValue(Name) property,
        // but first make sure this is truly Name Property
        std::shared_ptr<XamlProperty> spProperty = setValueNode.GetXamlProperty();
        if (spProperty->get_PropertyToken() == XamlPropertyToken(tpkNative, KnownPropertyIndex::DependencyObject_Name))
        {
            nodeList.push_back(std::move(setNameNode));
        }
        else
        {
            // roll back
            nodeList.push_back(std::move(setNameNode));
            nodeList.push_back(std::move(setValueNode));
        }
    }
    // Transform:
    //     ProvideStaticResourceValue
    //     SetValue
    // To:
    //     SetValueFromStaticResource
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::ProvideStaticResourceValue, ObjectWriterNodeType::SetValue } }, nodeList, poppedList))
    {
        auto& provideStaticResourceNode = poppedList[0];
        auto& setValueNode = poppedList[1];

        // loss of line information as we are losing track of the source location of
        // the source value and using the property set line information instead
        auto lineInfo = setValueNode.GetLineInfo();
        auto& spProperty = setValueNode.GetXamlProperty();
        auto& spValue = provideStaticResourceNode.GetValue();
        ASSERT(spValue->GetValue().GetType() == valueString);
        nodeList.push_back(ObjectWriterNode::MakeSetValueFromStaticResourceNode(lineInfo, spProperty, spValue));
    }
    // Transform:
    //     ProvideThemeResourceValue
    //     SetValue
    // To:
    //     SetValueFromThemeResource
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::ProvideThemeResourceValue, ObjectWriterNodeType::SetValue } }, nodeList, poppedList))
    {
        auto& provideThemeResourceNode = poppedList[0];
        auto& setValueNode = poppedList[1];

        // loss of line information as we are losing track of the source location of
        // the source value and using the property set line information instead
        auto lineInfo = setValueNode.GetLineInfo();
        auto& spProperty = setValueNode.GetXamlProperty();
        auto& spValue = provideThemeResourceNode.GetValue();
        ASSERT(spValue->GetValue().GetType() == valueString);
        nodeList.push_back(ObjectWriterNode::MakeSetValueFromThemeResourceNode(lineInfo, spProperty, spValue));
    }
    // Transform:
    //     ProvideTemplateBindingValue
    //     SetValue
    // To:
    //     SetValueFromTemplateBinding
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::ProvideTemplateBindingValue, ObjectWriterNodeType::SetValue } }, nodeList, poppedList))
    {
        auto& provideTemplateBindingNode = poppedList[0];
        auto& setValueNode = poppedList[1];

        // loss of line information as we are losing track of the source location of
        // the source value and using the property set line information instead
        auto lineInfo = setValueNode.GetLineInfo();
        auto& spProperty = setValueNode.GetXamlProperty();
        auto& spPropertyProxy = provideTemplateBindingNode.GetXamlProperty();

        // Check if the property that the TemplateBinding will be set on is a simple property.
        // If so, raise an error (simple properties do not support many of the traditional DP services, like
        // template binding)
        if (DirectUI::MetadataAPI::GetPropertyBaseByIndex(spProperty->get_PropertyToken().GetHandle())->Is<CSimpleProperty>())
        {
            xstring_ptr propertyName;
            THROW_IF_FAILED(spProperty->get_FullName(&propertyName));

            std::shared_ptr<ParserErrorReporter> errorService;
            THROW_IF_FAILED(m_spSchemaContext->GetErrorService(errorService));
            THROW_IF_FAILED(errorService->SetError(AG_E_PARSER2_TEMPLATEBINDING_NOT_ALLOWED_ON_PROPERTY, lineInfo.LineNumber(), lineInfo.LinePosition(), propertyName));
            THROW_HR(static_cast<HRESULT>(E_FAIL));
        }

        ASSERT(spPropertyProxy);
        nodeList.push_back(ObjectWriterNode::MakeSetValueFromTemplateBindingNode(lineInfo, spProperty, spPropertyProxy));
    }
    // Transform:
    //     ProvideValue
    //     SetValue
    // To:
    //     SetValueFromMarkupExtension
    else if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::ProvideValue, ObjectWriterNodeType::SetValue } }, nodeList, poppedList))
    {
        auto& setValueNode = poppedList[1];

        // loss of line information as we are losing track of the source location of
        // the source value and using the property set line information instead
        auto lineInfo = setValueNode.GetLineInfo();
        auto& spProperty = setValueNode.GetXamlProperty();
        nodeList.push_back(ObjectWriterNode::MakeSetValueFromMarkupExtensionNode(lineInfo, spProperty));
    }
    // Transform:
    //     PushConstant
    //     TypeConvert
    //     SetValue
    // To:
    //     SetValueTypeConvertedConstant
    else if (TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushConstant, ObjectWriterNodeType::TypeConvertValue, ObjectWriterNodeType::SetValue } }, nodeList, poppedList))
    {
        auto& constantNode = poppedList[0];
        auto& typeConvertValueNode = poppedList[1];
        auto& setValueNode = poppedList[2];

        // loss of line information as we are losing track of the source location of
        // the source value and using the property set line information instead
        auto lineInfo = setValueNode.GetLineInfo();
        auto& spProperty = setValueNode.GetXamlProperty();
        auto& spTypeConverter = typeConvertValueNode.GetTypeConverter();
        auto& spValue = constantNode.GetValue();

        // add the setvalue+typeconvert+pushconstant node
        nodeList.push_back(ObjectWriterNode::MakeSetValueTypeConvertedConstantNode(lineInfo, spProperty, spTypeConverter, spValue));
    }
    // Transform:
    //     PushResolvedType
    //     TypeConvert
    //     SetValue
    // To:
    //     SetValueTypeConvertedResolvedType
    else if (TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushResolvedType, ObjectWriterNodeType::TypeConvertValue, ObjectWriterNodeType::SetValue } }, nodeList, poppedList))
    {
        auto& proxyNode = poppedList[0];
        auto& typeConvertValueNode = poppedList[1];
        auto& setValueNode = poppedList[2];

        // loss of line information as we are losing track of the source location of
        // the source value and using the property set line information instead
        auto lineInfo = setValueNode.GetLineInfo();
        auto& spProperty = setValueNode.GetXamlProperty();
        auto& spTypeConverter = typeConvertValueNode.GetTypeConverter();
        auto& spProxy = proxyNode.GetXamlType();

        // add the setvalue+typeconvert+pushconstant node
        nodeList.push_back(ObjectWriterNode::MakeSetValueTypeConvertedResolvedTypeNode(lineInfo, spProperty, spTypeConverter, spProxy));
    }
    // Transform:
    //     PushResolvedProperty
    //     TypeConvert
    //     SetValue
    // To:
    //     SetValueTypeConvertedResolvedProperty
    else if (TryPopRequiredElements<3>({ { ObjectWriterNodeType::PushResolvedProperty, ObjectWriterNodeType::TypeConvertValue, ObjectWriterNodeType::SetValue } }, nodeList, poppedList))
    {
        auto& proxyNode = poppedList[0];
        auto& typeConvertValueNode = poppedList[1];
        auto& setValueNode = poppedList[2];

        // loss of line information as we are losing track of the source location of
        // the source value and using the property set line information instead
        auto lineInfo = setValueNode.GetLineInfo();
        auto& spProperty = setValueNode.GetXamlProperty();
        auto& spTypeConverter = typeConvertValueNode.GetTypeConverter();
        auto& spProxy = proxyNode.GetXamlProperty();

        // add the setvalue+typeconvert+pushconstant node
        nodeList.push_back(ObjectWriterNode::MakeSetValueTypeConvertedResolvedPropertyNode(lineInfo, spProperty, spTypeConverter, spProxy));
    }
    else
    {
        //DebugFailure(nodeList);
    }
}

void ObjectWriterNodeList::Optimize_CreateTypeWithInitialValue(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     PushConstant
    //     CreateTypeWithInitialValue
    // To:
    //     CreateTypeWithConstant
    if (TryPopRequiredElements<2>({ { ObjectWriterNodeType::PushConstant, ObjectWriterNodeType::CreateTypeWithInitialValue } }, nodeList, poppedList))
    {
        auto& pushConstantNode = poppedList[0];
        auto& createTypeWithValueNode = poppedList[1];

        // loss of line information as we are losing track of the source location of
        // the source value and using the property set line information instead
        auto lineInfo = createTypeWithValueNode.GetLineInfo();
        auto& spType = createTypeWithValueNode.GetXamlType();
        auto& spValue = pushConstantNode.GetValue();

        nodeList.push_back(ObjectWriterNode::MakeCreateTypeWithConstantNode(lineInfo, spType, spValue));
    }
    // Transform:
    //     PushConstant
    //     TypeConvert
    //     CreateTypeWithInitialValue
    // To:
    //     CreateTypeWithTypeConvertedConstant
    else if (TryPopRequiredElements<3>({ {
        ObjectWriterNodeType::PushConstant,
        ObjectWriterNodeType::TypeConvertValue,
        ObjectWriterNodeType::CreateTypeWithInitialValue
        } }, nodeList, poppedList))
    {
        auto& pushConstantNode = poppedList[0];
        auto& typeConvertNode = poppedList[1];
        auto& createTypeWithValueNode = poppedList[2];

        // loss of line information as we are losing track of the source location of
        // the source value and using the property set line information instead
        auto lineInfo = createTypeWithValueNode.GetLineInfo();
        auto& spType = createTypeWithValueNode.GetXamlType();
        auto& spValue = pushConstantNode.GetValue();
        auto& spConverter = typeConvertNode.GetTypeConverter();

        nodeList.push_back(ObjectWriterNode::MakeCreateTypeWithTypeConvertedConstantNode(lineInfo, spType, spConverter, spValue));
    }
    else
    {
        DebugFailure(nodeList);
    }
}

void ObjectWriterNodeList::Optimize_ProvideValue(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
    std::vector<ObjectWriterNode> poppedList;

    // Transform:
    //     PushScopeCreateTypeBeginInit (StaticResource)
    //         SetValueConstant (StaticResource.ResourceKey)
    //         EndInit
    //         ProvideValue
    // To:
    //     PushScope
    //       ProvideStaticResourceValue
    if (
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::PushScopeCreateTypeBeginInit,
            ObjectWriterNodeType::SetValueConstant,
            ObjectWriterNodeType::EndInit,
            ObjectWriterNodeType::ProvideValue
        } }, nodeList, poppedList))
    {
        auto& createTypeNode = poppedList[0];
        auto& setValueConstantNode = poppedList[1];
        auto& endInitNode = poppedList[2];
        auto& provideValueNode = poppedList[3];

        if (createTypeNode.GetXamlType()->get_TypeToken() == XamlTypeToken(tpkNative, KnownTypeIndex::StaticResource) &&
            setValueConstantNode.GetXamlProperty()->get_PropertyToken() == XamlPropertyToken(tpkNative, KnownPropertyIndex::StaticResource_ResourceKey) &&
            setValueConstantNode.GetValue()->GetValue().GetType() == valueString)
        {
            // use the line information from the create static resource key type node
            auto lineInfo = createTypeNode.GetLineInfo();
            auto& spValue = setValueConstantNode.GetValue();

            nodeList.push_back(ObjectWriterNode::MakePushScopeNode(lineInfo));
            nodeList.push_back(ObjectWriterNode::MakeProvideStaticResourceValueNode(lineInfo, spValue));
        }
        else if (createTypeNode.GetXamlType()->get_TypeToken() == XamlTypeToken(tpkNative, KnownTypeIndex::ThemeResource) &&
            setValueConstantNode.GetXamlProperty()->get_PropertyToken() == XamlPropertyToken(tpkNative, KnownPropertyIndex::ThemeResource_ResourceKey) &&
            setValueConstantNode.GetValue()->GetValue().GetType() == valueString)
        {
            // use the line information from the create theme resource key type node
            auto lineInfo = createTypeNode.GetLineInfo();
            auto& spValue = setValueConstantNode.GetValue();

            nodeList.push_back(ObjectWriterNode::MakePushScopeNode(lineInfo));
            nodeList.push_back(ObjectWriterNode::MakeProvideThemeResourceValueNode(lineInfo, spValue));
        }
        else
        {
            // roll back
            nodeList.push_back(std::move(createTypeNode));
            nodeList.push_back(std::move(setValueConstantNode));
            nodeList.push_back(std::move(endInitNode));
            nodeList.push_back(std::move(provideValueNode));
        }
    }
    // Transform:
    //     PushScopeCreateTypeBeginInit (StaticResource)
    //         SetValueTypeConvertedResolvedProperty
    //         EndInit
    //         ProvideValue
    // To:
    //     PushScope
    //       ProvideTemplateBindingValue
    else if (
        TryPopRequiredElements<4>({ {
            ObjectWriterNodeType::PushScopeCreateTypeBeginInit,
            ObjectWriterNodeType::SetValueTypeConvertedResolvedProperty,
            ObjectWriterNodeType::EndInit,
            ObjectWriterNodeType::ProvideValue
        } }, nodeList, poppedList))
    {
        auto& createTypeNode = poppedList[0];
        auto& setValueNode = poppedList[1];
        auto& endInitNode = poppedList[2];
        auto& provideValueNode = poppedList[3];

        if (createTypeNode.GetXamlType()->get_TypeToken() == XamlTypeToken(tpkNative, KnownTypeIndex::TemplateBinding) &&
            setValueNode.GetXamlProperty()->get_PropertyToken() == XamlPropertyToken(tpkNative, KnownPropertyIndex::TemplateBinding_Property) &&
            setValueNode.GetXamlPropertyProxy())
        {
            // use the line information from the create template binding type node
            auto lineInfo = createTypeNode.GetLineInfo();
            auto& spProxy = setValueNode.GetXamlPropertyProxy();

            nodeList.push_back(ObjectWriterNode::MakePushScopeNode(lineInfo));
            nodeList.push_back(ObjectWriterNode::MakeProvideTemplateBindingValueNode(lineInfo, spProxy));
        }
        else
        {
            // roll back
            nodeList.push_back(std::move(createTypeNode));
            nodeList.push_back(std::move(setValueNode));
            nodeList.push_back(std::move(endInitNode));
            nodeList.push_back(std::move(provideValueNode));
        }
    }
    else
    {
        //DebugFailure(nodeList);
    }
}


void ObjectWriterNodeList::DebugFailure(
    _In_ std::vector<ObjectWriterNode>& nodeList)
{
#if DBG
    xstring_ptr strIndent = xstring_ptr::EmptyString();
    Dump(nodeList, strIndent);
#endif

    ASSERT(false);
}

std::vector<ObjectWriterNode>& ObjectWriterNodeList::GetNodeList()
{
    if (!m_nodeListOptimized.empty())
    {
        return m_nodeListOptimized;
    }
    else
    {
        return m_nodeList;
    }
}

std::shared_ptr<XamlSchemaContext> ObjectWriterNodeList::GetSchemaContext()
{
    return m_spSchemaContext;
}

#if DBG

HRESULT ObjectWriterNodeList::Dump()
{
    xstring_ptr strIndent = xstring_ptr::EmptyString();
    RRETURN(Dump(GetNodeList(), strIndent));
}

// dump the node list tree in a pretty formatting.
// since this doesn't validate that the node list
// has been closed, the resulting tree may
// not be complete
HRESULT ObjectWriterNodeList::Dump(
    _In_ std::vector<ObjectWriterNode>& nodeList, xstring_ptr& strIndent)
{
    for (auto& node : nodeList)
    {
        xstring_ptr nodeDescription;
        xstring_ptr lineInfoString;
        if (node.RequiresScopeToEnd())
        {
            if (strIndent.GetCount() >= 2)
            {
                IFC_RETURN(strIndent.SubString(0, strIndent.GetCount() - 2, &strIndent));
            }
        }

        if (node.ProvidesCustomBinaryData())
        {
            IFC_RETURN(Dump(node.GetNodeList()->GetNodeList(), strIndent));
        }

        IFC_RETURN(node.GetLineInfoAsString(lineInfoString));
        IFC_RETURN(node.ToString(nodeDescription));
        LOG(L"Encoding:%s %s [%s]", strIndent.GetBuffer(), nodeDescription.GetBuffer(), lineInfoString.GetBuffer());

        if (node.RequiresNewScope())
        {
            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strBlankIndent, L"  ");

            XStringBuilder strBuilder;
            IFC_RETURN(strBuilder.Initialize());
            IFC_RETURN(strBuilder.Append(strIndent));
            IFC_RETURN(strBuilder.Append(c_strBlankIndent));
            IFC_RETURN(strBuilder.DetachString(&strIndent));
        }
    }

    return S_OK;
}

#endif


