// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ObjectWriter.h"
#include "ObjectWriterRuntimeEncoder.h"
#include "ObjectWriterNode.h"
#include "ObjectWriterNodeList.h"
#include <ObjectWriterContext.h>
#include "TypeTable.g.h"
#include "MetadataAPI.h"
#include "XamlProperty.h"

#include <SubObjectWriterResult.h>
#include <CustomWriterRuntimeData.h>

using namespace DirectUI;

ObjectWriterRuntimeEncoder::ObjectWriterRuntimeEncoder(
    _In_ const std::shared_ptr<ObjectWriterContext>& spContext,
    _In_ const std::shared_ptr<ObjectWriterErrorService>& spErrorService)
    : ObjectWriterCommonRuntime(spContext, spErrorService, true /* validator enabled */, true /* encoding enabled */)
    , m_NextStreamOffsetTokenIndex(0)
    , m_conditionalScopeDepth(0)
{
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::Initialize()
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;

    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    IFC_RETURN(m_spContext->EnsureTargetTypeCaches());
    m_spNodeList = std::make_shared<ObjectWriterNodeList>(spSchemaContext);

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::AddNamespacePrefixImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const xstring_ptr& strNamespacePrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spNamespace)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeAddNamespaceNode(lineInfo, strNamespacePrefix, spNamespace)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::CreateTypeImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeCreateTypeNode(lineInfo, spType)));

    // the objectwriter expects an object by testing exists_Instance() so we
    // need to simulate the fact that we created an object
    IFC_RETURN(GetEmptyXamlQualifiedObject(spInstance));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::GetValueImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spValue)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeGetValueNode(lineInfo, spProperty)));

    // the objectwriter expects an object by testing exists_Instance() so we
    // need to simulate the fact that we created an object
    IFC_RETURN(GetEmptyXamlQualifiedObject(spValue));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::CreateTypeWithInitialValueImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
    _Inout_ std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeCreateTypeWithInitialValueNode(lineInfo, spType)));

    // the objectwriter expects an object by testing exists_Instance() so we
    // need to simulate the fact that we created an object
    IFC_RETURN(GetEmptyXamlQualifiedObject(spInstance));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::TypeConvertValueImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ std::shared_ptr<XamlServiceProviderContext> spContext,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntaxConverter,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _In_ bool fIsPropertyAssignment,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    if (   spTextSyntaxConverter
        && (   spTextSyntaxConverter->get_TextSyntaxToken() == XamlTypeToken(tpkNative, KnownTypeIndex::TypeName)
            || spTextSyntaxConverter->get_TextSyntaxToken() == XamlTypeToken(tpkNative, KnownTypeIndex::DependencyPropertyProxy)))
    {
        ObjectWriterNode node = m_spNodeList->PopNode();
        std::shared_ptr<XamlQualifiedObject> spNodeValue = node.GetValue();

        ASSERT(spNodeValue->GetValue().GetType() == valueString);
        if (spTextSyntaxConverter->get_TextSyntaxToken() == XamlTypeToken(tpkNative, KnownTypeIndex::TypeName))
        {
            std::shared_ptr<XamlType> spXamlType;
            IFC_RETURN(m_spContext->get_MarkupExtensionContext()->ResolveXamlType(spNodeValue->GetValue().AsString(), spXamlType));
            if (spXamlType)
            {
                IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakePushResolvedTypeNode(node.GetLineInfo(), spXamlType)));
            }
            else
            {
                IFC_RETURN(m_spNodeList->AddNode(std::move(node)));
            }
        }
        else if (spTextSyntaxConverter->get_TextSyntaxToken() == XamlTypeToken(tpkNative, KnownTypeIndex::DependencyPropertyProxy))
        {
            std::shared_ptr<XamlType> spTargetType;
            std::shared_ptr<XamlProperty> spResolvedProperty;
            auto frameIterator = m_spContext->get_StackBegin();

            while (frameIterator != m_spContext->get_StackEnd() && !spTargetType)
            {
                ObjectWriterFrame& frame = *frameIterator;

                if (frame.exists_CompressedStack())
                {
                    spTargetType = frame.get_CompressedStack()->get_TargetTypeForEncoder();
                }
                else if (frame.exists_Type() && frame.get_Type()->get_TypeToken() == XamlTypeToken(tpkNative, KnownTypeIndex::ControlTemplate))
                {
                    // default to Control TargetType for a ControlTemplate
                    std::shared_ptr<XamlSchemaContext> spSchemaContext;
                    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
                    IFC_RETURN(spSchemaContext->GetXamlType(XamlTypeToken(tpkNative, KnownTypeIndex::Control), spTargetType));
                }

                frameIterator++;
            }

            IFC_RETURN(m_spContext->get_MarkupExtensionContext()->ResolveDependencyProperty(spNodeValue->GetValue().AsString(), spTargetType, spResolvedProperty));

            if (spResolvedProperty)
            {
                IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakePushResolvedPropertyNode(node.GetLineInfo(), spResolvedProperty)));
            }
            else
            {
                IFC_RETURN(m_spNodeList->AddNode(std::move(node)));
            }
        }
    }

    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeTypeConvertValueNode(lineInfo, spTextSyntaxConverter)));

    spInstance = spValue;

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::SetValueImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    if (XamlProperty::AreEqual(spProperty, m_spContext->GetStyleTargetTypeXamlProperty()) ||
        XamlProperty::AreEqual(spProperty, m_spContext->GetControlTemplateTargetTypeXamlProperty()))
    {
        ObjectWriterNode typeConvertNode = m_spNodeList->PopNode();
        ASSERT(typeConvertNode.GetNodeType() == ObjectWriterNodeType::TypeConvertValue);
        const ObjectWriterNode& pushResolveTypeNode = m_spNodeList->PeekTopNode();
        if (pushResolveTypeNode.GetNodeType() == ObjectWriterNodeType::PushResolvedType)
        {
            m_spContext->Parent().ensure_CompressedStack();
            m_spContext->Parent().get_CompressedStack()->set_TargetTypeForEncoder(pushResolveTypeNode.GetXamlType());
        }
        IFC_RETURN(m_spNodeList->AddNode(std::move(typeConvertNode)));
    }

    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeSetValueNode(lineInfo, spProperty)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::PushConstantImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    std::shared_ptr<XamlQualifiedObject> spConstantValue = spValue;

    // clone it in the node list
    if (spValue->GetValue().GetType() == valueString)
    {
        IFC_RETURN(GetEmptyXamlQualifiedObject(spConstantValue));
        IFC_RETURN(spConstantValue->SetValue(spValue->GetValue()));
    }

    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakePushConstantNode(lineInfo, spConstantValue)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::CheckPeerTypeImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    xstring_ptr strClassName;

    IFC_RETURN(spValue->GetCopyAsString(&strClassName));
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeCheckPeerTypeNode(lineInfo, strClassName)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::AddToCollectionImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spParentPropertyType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _In_ const std::shared_ptr<XamlType>& spValueType)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeAddToCollectionNode(lineInfo)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::AddToDictionaryImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _In_ const std::shared_ptr<XamlType>& spValueType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKey)
{
    if (!spKey)
    {
        IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeAddToDictionaryNode(lineInfo)));
    }
    else
    {
        IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeAddToDictionaryWithKeyNode(lineInfo, spKey)));
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::SetConnectionIdImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeSetConnectionIdNode(lineInfo, spConnectionId)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::GetXBindConnectorImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spReturnConnector)
{
    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::SetNameImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spName,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const xref_ptr<INameScope>& spNamescope)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeSetNameNode(lineInfo, spName)));

    return S_OK;
}


_Check_return_ HRESULT ObjectWriterRuntimeEncoder::GetResourcePropertyBagImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKey,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spObject,
    _Out_ SP_MapPropertyToQO& spPropertyBag)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeGetResourcePropertyBagNode(lineInfo, spKey)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::SetDirectivePropertyImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<DirectiveProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeSetDirectivePropertyNode(lineInfo, spProperty)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::ProvideValueImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spMarkupExtension,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spValue)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeProvideValueNode(lineInfo)));

    spValue = spMarkupExtension;

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::BeginInitImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeBeginInitNode(lineInfo)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::EndInitImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeEndInitNode(lineInfo)));

    return S_OK;

}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::SetCustomRuntimeDataImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>&,
    _In_ const std::shared_ptr<XamlQualifiedObject>&,
    _In_ std::shared_ptr<CustomWriterRuntimeData> customWriterData,
    _In_ std::shared_ptr<SubObjectWriterResult> customWriterNodeStream,
    _In_ xref_ptr<INameScope> nameScope,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spXBindConnector,
    const std::shared_ptr<ObjectWriterCallbacksDelegate>& callback)
{
    UNREFERENCED_PARAMETER(nameScope);
    UNREFERENCED_PARAMETER(spXBindConnector);

    IFC_RETURN(customWriterData->PrepareStream(customWriterNodeStream));
    if (customWriterData->ShouldEncodeAsCustomData())
    {
        IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeSetCustomRuntimeData(lineInfo, customWriterData, customWriterNodeStream)));
    }
    else
    {
        for (auto&& node : customWriterNodeStream->GetNodeList()->GetNodeList())
        {
            IFC_RETURN(m_spNodeList->AddNode(std::move(node)));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::DeferResourceDictionaryItemsImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ ObjectWriter* pObjectWriter,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spCollection,
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _In_ const bool fIsDictionaryWithKeyProperty,
    _Out_ bool& hasDeferred)
{
    // TODO
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeSetResourceDictionaryItems(lineInfo)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::DeferTemplateContentImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spTemplateContent)
{
    HRESULT hr = S_OK;

    std::shared_ptr<ObjectWriterNodeList> spObjectNodeList;
    std::shared_ptr<XamlSavedContext> spSavedContext;
    std::shared_ptr<XamlReader> spReader;
    std::shared_ptr<ObjectWriter> spObjectWriter;
    ObjectWriterSettings objectWriterSettings;
    std::vector<std::pair<bool,xstring_ptr>> vecResourceList;

    ASSERT(spType);
    ASSERT(spProperty);
    ASSERT(spNodeList);

    IFC(m_spContext->GetSavedContext(spSavedContext));
    IFC(spNodeList->get_Reader(spReader));

    objectWriterSettings.set_EnableEncoding(true);

    IFC(ObjectWriter::Create(spSavedContext, objectWriterSettings, spObjectWriter));
    while ((hr = spReader->Read()) == S_OK)
    {
        IFC(spObjectWriter->WriteNode(spReader->CurrentNode()));
    }

    // collect resource reference list
    IFC(GetListOfReferencedResources(spNodeList, vecResourceList));

    spObjectNodeList = spObjectWriter->GetNodeList();
    IFC(spObjectNodeList->Optimize());

    IFC(m_spNodeList->AddNode(ObjectWriterNode::MakeSetDeferredPropertyNode(lineInfo, spProperty, spObjectNodeList, std::move(vecResourceList))));

Cleanup:
    return hr;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::GetListOfReferencedResources(
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _Out_ std::vector<std::pair<bool,xstring_ptr>>& spResourceList)
{
    HRESULT hr = S_OK;
    XamlTypeToken staticResourceToken(tpkNative, KnownTypeIndex::StaticResource);
    XamlTypeToken themeResourceToken(tpkNative, KnownTypeIndex::ThemeResource);

    std::shared_ptr<XamlReader> spReader;
    IFC_RETURN(spNodeList->get_Reader(spReader));
    while ((hr = spReader->Read()) == S_OK)
    {
        if (spReader->CurrentNode().get_NodeType() == XamlNodeType::xntStartObject)
        {
            XamlTypeToken xamlTypeToken = spReader->CurrentNode().get_XamlType()->get_TypeToken();

            if (xamlTypeToken == staticResourceToken || xamlTypeToken == themeResourceToken)
            {
                // We need to find the ResourceKey property, which means skipping over any directives
                // (these may be present if the Static/ThemeResource was specified using object-element syntax)
                // We'll read until we've encountered (and consumed) the matching XamlNodeType::xntEndObject.
                int depth = 1;
                while ((hr = spReader->Read()) == S_OK && depth > 0)
                {
                    switch (spReader->CurrentNode().get_NodeType())
                    {
                        case XamlNodeType::xntStartObject:
                        {
                            ++depth;
                        }
                        break;

                        case XamlNodeType::xntEndObject:
                        {
                            --depth;
                        }
                        break;

                        case XamlNodeType::xntStartProperty:
                        {
                            ++depth;
                            if (!spReader->CurrentNode().get_Property()->IsDirective())
                            {
                                // StaticResourceExtension & ThemeResourceExtension have only one property.
                                ASSERT(spReader->CurrentNode().get_Property()->get_PropertyToken().GetHandle() == KnownPropertyIndex::StaticResource_ResourceKey ||
                                    spReader->CurrentNode().get_Property()->get_PropertyToken().GetHandle() == KnownPropertyIndex::ThemeResource_ResourceKey);
                                bool isStaticResource = (spReader->CurrentNode().get_Property()->get_PropertyToken().GetHandle() == KnownPropertyIndex::StaticResource_ResourceKey);

                                if ((hr = spReader->Read()) == S_OK)
                                {
                                    XamlNodeType nodeType = spReader->CurrentNode().get_NodeType();

                                    if (nodeType == XamlNodeType::xntValue)
                                    {
                                        xstring_ptr strKey;

                                        // lookup the string from CValue or CString
                                        if (spReader->CurrentNode().get_Value()->GetValue().GetType() == valueString)
                                        {
                                            strKey = spReader->CurrentNode().get_Value()->GetValue().AsString();

                                            std::pair<bool, xstring_ptr> resourceInfo = std::make_pair(isStaticResource, std::move(strKey));
                                            if (std::find(spResourceList.begin(), spResourceList.end(), resourceInfo) == spResourceList.end())
                                            {
                                                spResourceList.emplace_back(std::move(resourceInfo));
                                            }
                                        }
                                        else
                                        {
                                            ASSERT(false);
                                        }
                                    }
                                    else
                                    {
                                        // complex case
                                        // TODO: when do we hit this case?
                                    }
                                }
                                ASSERT(hr != S_FALSE);
                            }
                        }
                        break;

                        case XamlNodeType::xntEndProperty:
                        {
                            --depth;
                        }
                        break;
                    }
                }
                ASSERT(hr != S_FALSE);
                IFC_RETURN(hr);
            }
        }
    }
    IFC_RETURN(hr);

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::PushScopeImpl(
    _In_ const XamlLineInfo& lineInfo)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakePushScopeNode(lineInfo)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::PopScopeImpl(
    _In_ const XamlLineInfo& lineInfo)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakePopScopeNode(lineInfo)));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::GetStreamOffsetTokenImpl(
    _Out_ UINT32 *pTokenIndex)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeStreamOffsetMarker(m_NextStreamOffsetTokenIndex)));
    *pTokenIndex = m_NextStreamOffsetTokenIndex++;

    return S_OK;
}

_Check_return_ HRESULT
ObjectWriterRuntimeEncoder::BeginConditionalScopeImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs)
{
    m_conditionalScopeDepth++;

    // Disable validation because XAML inside conditional scopes is not guaranteed to
    // be used at runtime
    m_isValidatorEnabled = false;

    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeBeginConditionalScope(lineInfo, xamlPredicateAndArgs)));

    return S_OK;
}

_Check_return_ HRESULT
ObjectWriterRuntimeEncoder::EndConditionalScopeImpl(
    _In_ const XamlLineInfo& lineInfo)
{
    IFC_RETURN(m_spNodeList->AddNode(ObjectWriterNode::MakeEndConditionalScope(lineInfo)));

    ASSERT(m_conditionalScopeDepth > 0);
    --m_conditionalScopeDepth;

    // Re-enable validation if we are no longer inside a conditional XAML scope
    if (m_conditionalScopeDepth == 0)
    {
        m_isValidatorEnabled = true;
    }

    return S_OK;
}

std::shared_ptr<ObjectWriterNodeList> ObjectWriterRuntimeEncoder::GetNodeList()
{
    return m_spNodeList;
}

_Check_return_ HRESULT ObjectWriterRuntimeEncoder::GetEmptyXamlQualifiedObject(
    _Out_ std::shared_ptr<XamlQualifiedObject>& spObject)
{
    CValue emptyObject;
    emptyObject.WrapObjectNoRef(nullptr);
    auto qo = std::make_shared<XamlQualifiedObject>();
    IFC_RETURN(qo->SetValue(emptyObject));
    spObject = std::move(qo);
    return S_OK;
}
