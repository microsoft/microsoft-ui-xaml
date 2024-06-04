// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ObjectWriterContext.h>
#include "BinaryFormatObjectWriter.h"
#include "ObjectWriterNode.h"
#include "ObjectWriterRuntime.h"
#include "ObjectWriterErrorService.h"
#include "MetadataAPI.h"
#include "ThemeResource.h"
#include "XamlPredicateHelpers.h"
#include "XamlPredicateService.h"
#include "resources\inc\ResourceLookupLogger.h"

using namespace DirectUI;

_Check_return_ HRESULT BinaryFormatObjectWriter::Create(
    _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
    _In_ const std::shared_ptr<XamlSavedContext>& spSavedContext,
    _In_ const ObjectWriterSettings& settings,
    _Out_ std::shared_ptr<BinaryFormatObjectWriter>& rspObjectWriter)
{
    bool shouldPerformInitialPopScope = true;
    auto spObjectWriter = std::make_shared<BinaryFormatObjectWriter>();
    spObjectWriter->m_spObjectWriterCallbacks = settings.get_ObjectWriterCallbacks();
    spObjectWriter->m_qoRootObjectInstance = settings.get_RootObjectInstance();
    spObjectWriter->m_qoEventRoot = settings.get_EventRoot();
    spObjectWriter->m_qoXBindConnector = settings.get_XBindConnector();
    spObjectWriter->m_qoParentXBindConnector = settings.get_XBindParentConnector();

    // This path will be run if there was no existing
    // object writer context provided for the object writer
    // which means this is an clean object writing setup
    if (spSchemaContext)
    {
        IFC_RETURN(ObjectWriterContext::Create(
            spSchemaContext,
            spObjectWriter->m_qoEventRoot,
            settings.get_ExpandTemplates(),
            spObjectWriter->m_qoParentXBindConnector,
            spObjectWriter->m_spContext));

        // TODO: Move these into interfaces on ObjectWriter like rest of settings?
        spObjectWriter->m_spContext->set_BaseUri(settings.get_BaseUri());
        spObjectWriter->m_spContext->set_XamlResourceUri(settings.get_XamlResourceUri());
        spObjectWriter->m_spContext->set_XbfHash(settings.get_XbfHash());
        spObjectWriter->m_spContext->set_RootNamescope(settings.get_NameScope());
    }
    // We have a saved context provided which should
    // be used as the context for the object writer
    // for initializing its state
    else if (spSavedContext)
    {
        IFC_RETURN(ObjectWriterContext::Create(spSavedContext, spObjectWriter->m_qoEventRoot, settings.get_ExpandTemplates(), spObjectWriter->m_qoParentXBindConnector, spObjectWriter->m_spContext));
        spObjectWriter->m_spContext->set_BaseUri(settings.get_BaseUri());
        spObjectWriter->m_spContext->set_RootNamescope(settings.get_NameScope());
        shouldPerformInitialPopScope = false;
    }
    // Unexpected that we did not get an incoming schema context
    // nor a saved context.
    else
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    // Initialize ObjectWriterRuntime and Error Service

    auto spErrorService = std::make_shared<ObjectWriterErrorService>(spObjectWriter->m_spContext, false);
    spObjectWriter->m_spErrorService = spErrorService;
    auto spRuntime = std::make_shared<ObjectWriterRuntime>(spObjectWriter->m_spContext, spErrorService, false /* validator enabled */);
    spObjectWriter->m_spRuntime = spRuntime;
    if (shouldPerformInitialPopScope)
    {
        IFC_RETURN(spRuntime->PopScope(XamlLineInfo()));
    }

    rspObjectWriter = std::move(spObjectWriter);

    return S_OK;
}

void BinaryFormatObjectWriter::SetActiveObject(_In_ std::shared_ptr<XamlQualifiedObject> object)
{
    m_spContext->PushScope();
    m_spContext->Current().set_Instance(object);
    m_spContext->Current().set_Type(std::shared_ptr<XamlType>());
}

_Check_return_ HRESULT BinaryFormatObjectWriter::WriteNode(_In_ const ObjectWriterNode& inNode)
{
    auto nodeType = inNode.GetNodeType();

    IFC_RETURN(PushScopeIfRequired(inNode));

    if (nodeType == ObjectWriterNodeType::BeginConditionalScope)
    {
        IFC_RETURN(BeginConditionalScope(inNode));
    }
    else if (nodeType == ObjectWriterNodeType::EndConditionalScope)
    {
        IFC_RETURN(EndConditionalScope(inNode));
    }
    else if (!IsSkippingForConditionalScope())
    {
        switch (nodeType)
        {
            case ObjectWriterNodeType::AddNamespace:
            case ObjectWriterNodeType::PushScopeAddNamespace:
            {
                IFC_RETURN(AddNamespacePrefix(inNode));
            }
            break;

            case ObjectWriterNodeType::SetDeferredProperty:
            {
                IFC_RETURN(SetDeferredPropertyOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::SetCustomRuntimeData:
            {
                IFC_RETURN(SetCustomRuntimeDataOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::CreateType:
            {
                IFC_RETURN(CreateInstanceFromType(inNode));
            }
            break;

            case ObjectWriterNodeType::PushScopeCreateTypeBeginInit:
            case ObjectWriterNodeType::CreateTypeBeginInit:
            {
                IFC_RETURN(CreateInstanceFromType(inNode));
                IFC_RETURN(BeginInitOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::BeginInit:
            {
                IFC_RETURN(BeginInitOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::EndInitPopScope:
            case ObjectWriterNodeType::EndInit:
            {
                IFC_RETURN(EndInitOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::GetValue:
            case ObjectWriterNodeType::PushScopeGetValue:
            {
                IFC_RETURN(GetValueFromMemberOnParentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::SetName:
            {
                IFC_RETURN(SetNameOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::SetConnectionId:
            {
                IFC_RETURN(SetConnectionIdOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::PushConstant:
            {
                // TODO: Are there any valid cases for an optimized stream to have an orphan PushConstant?
                IFC_RETURN(PushConstant(inNode));
            }
            break;

            case ObjectWriterNodeType::SetValue:
            {
                IFC_RETURN(SetValueOnCurrentInstance(inNode, m_qoLastInstance));
            }
            break;

            case ObjectWriterNodeType::SetValueConstant:
            {
                IFC_RETURN(SetValueOnCurrentInstance(inNode, inNode.GetValue()));
            }
            break;

            case ObjectWriterNodeType::SetValueTypeConvertedConstant:
            {
                std::shared_ptr<XamlQualifiedObject> typeConvertedValue;
                std::shared_ptr<XamlType> propertyType;
                IFC_RETURN(inNode.GetXamlProperty()->get_Type(propertyType));
                IFC_RETURN(TypeConvertValue(inNode, propertyType, inNode.GetTypeConverter(), false, typeConvertedValue));
                IFC_RETURN(SetValueOnCurrentInstance(inNode, typeConvertedValue));
            }
            break;

            case ObjectWriterNodeType::SetValueTypeConvertedResolvedType:
            {
                std::shared_ptr<XamlQualifiedObject> typeConvertedValue;
                IFC_RETURN(TypeConvertResolvedType(inNode, m_spContext->Current().get_Type(), inNode.GetTypeConverter(), false, typeConvertedValue));
                IFC_RETURN(SetValueOnCurrentInstance(inNode, typeConvertedValue));
            }
            break;

            case ObjectWriterNodeType::SetValueTypeConvertedResolvedProperty:
            {
                std::shared_ptr<XamlQualifiedObject> typeConvertedValue;
                IFC_RETURN(TypeConvertResolvedProperty(inNode, m_spContext->Current().get_Type(), inNode.GetTypeConverter(), false, typeConvertedValue));
                IFC_RETURN(SetValueOnCurrentInstance(inNode, typeConvertedValue));
            }
            break;

            case ObjectWriterNodeType::PushScopeCreateTypeWithConstantBeginInit:
            case ObjectWriterNodeType::CreateTypeWithConstantBeginInit:
            {
                std::shared_ptr<XamlQualifiedObject> typeConvertedValue;
                std::shared_ptr<XamlTextSyntax> typeConverter;
                IFC_RETURN(inNode.GetXamlType()->get_TextSyntax(typeConverter));

                // type convert but also create the instance
                IFC_RETURN(TypeConvertValue(inNode, inNode.GetXamlType(), typeConverter, true, typeConvertedValue));

                // the type convert when set to createValueTypeAsObject will push the type converted object into the parser context
                IFC_RETURN(BeginInitOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstantBeginInit:
            case ObjectWriterNodeType::CreateTypeWithTypeConvertedConstantBeginInit:
            {
                std::shared_ptr<XamlQualifiedObject> typeConvertedValue;

                // type convert but also create the instance
                IFC_RETURN(TypeConvertValue(inNode, inNode.GetXamlType(), inNode.GetTypeConverter(), true, typeConvertedValue));

                // the type convert when set to createValueTypeAsObject will push the type converted object into the parser context
                IFC_RETURN(BeginInitOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::AddToCollection:
            {
                IFC_RETURN(AddToCollectionOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::AddToDictionary:
            case ObjectWriterNodeType::AddToDictionaryWithKey:
            {
                IFC_RETURN(AddToDictionaryOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::ProvideStaticResourceValue:
            {
                m_spContext->Current().clear_Member();
                IFC_RETURN(m_spRuntime->PushScope(inNode.GetLineInfo()));
                IFC_RETURN(ProvideStaticResourceReference(inNode));
                IFC_RETURN(m_spRuntime->PopScope(inNode.GetLineInfo()));
            }
            break;

            case ObjectWriterNodeType::ProvideThemeResourceValue:
            {
                m_spContext->Current().clear_Member();
                IFC_RETURN(m_spRuntime->PushScope(inNode.GetLineInfo()));
                IFC_RETURN(ProvideThemeResourceValue(inNode));
                IFC_RETURN(m_spRuntime->PopScope(inNode.GetLineInfo()));
            }
            break;

            case ObjectWriterNodeType::SetValueFromStaticResource:
            {
                if (m_spContext->IsInConditionalScope(false))
                {
                    IFC_RETURN(m_spErrorService->ValidateIsKnown(inNode.GetXamlProperty(), inNode.GetLineInfo()));
                }

                // set the markup extension target property context
                m_spContext->Current().set_Member(inNode.GetXamlProperty());
                IFC_RETURN(m_spRuntime->PushScope(inNode.GetLineInfo()));
                IFC_RETURN(ProvideStaticResourceReference(inNode));
                IFC_RETURN(m_spRuntime->PopScope(inNode.GetLineInfo()));
                m_spContext->Current().clear_Member();
                IFC_RETURN(SetValueOnCurrentInstance(inNode, m_qoLastInstance));
            }
            break;

            case ObjectWriterNodeType::SetValueFromTemplateBinding:
            {
                IFC_RETURN(SetTemplateBindingOnCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::EndInitProvideValuePopScope:
            {
                IFC_RETURN(EndInitOnCurrentInstance(inNode));
                IFC_RETURN(m_spRuntime->PushScope(inNode.GetLineInfo()));
                IFC_RETURN(ProvideValueForMarkupExtension(inNode));
                IFC_RETURN(m_spRuntime->PopScope(inNode.GetLineInfo()));

                m_spContext->Current().set_Instance(m_qoLastInstance);
                m_spContext->Current().set_Type(m_qoLastType);
            }
            break;

            case ObjectWriterNodeType::SetValueFromMarkupExtension:
            {
                if (m_spContext->IsInConditionalScope(false))
                {
                    IFC_RETURN(m_spErrorService->ValidateIsKnown(inNode.GetXamlProperty(), inNode.GetLineInfo()));
                }

                // set the markup extension target property context
                m_spContext->Current().set_Member(inNode.GetXamlProperty());
                IFC_RETURN(m_spRuntime->PushScope(inNode.GetLineInfo()));
                IFC_RETURN(ProvideValueForMarkupExtension(inNode));
                IFC_RETURN(m_spRuntime->PopScope(inNode.GetLineInfo()));
                m_spContext->Current().clear_Member();
                IFC_RETURN(SetValueOnCurrentInstance(inNode, m_qoLastInstance));
            }
            break;

            case ObjectWriterNodeType::SetValueFromThemeResource:
            {
                if (m_spContext->IsInConditionalScope(false))
                {
                    IFC_RETURN(m_spErrorService->ValidateIsKnown(inNode.GetXamlProperty(), inNode.GetLineInfo()));
                }

                // set the markup extension target property context
                m_spContext->Current().set_Member(inNode.GetXamlProperty());
                IFC_RETURN(m_spRuntime->PushScope(inNode.GetLineInfo()));
                IFC_RETURN(ProvideThemeResourceValue(inNode));
                IFC_RETURN(m_spRuntime->PopScope(inNode.GetLineInfo()));
                m_spContext->Current().clear_Member();
                IFC_RETURN(SetValueOnCurrentInstance(inNode, m_qoLastInstance));
            }
            break;

            case ObjectWriterNodeType::CheckPeerType:
            {
                // TODO: Handle these cases.
            }
            break;

            case ObjectWriterNodeType::GetResourcePropertyBag:
            {
                IFC_RETURN(GetResourcePropertyBagForCurrentInstance(inNode));
            }
            break;

            case ObjectWriterNodeType::PushScope:
            case ObjectWriterNodeType::PopScope:
            {
                // noop, handled by PushScopeIfRequired/PopScopeIfRequired()
            }
            break;

            default:
            {
                // Unhandled situation - investigate
                ASSERT(false);
            }
            break;
        }
    }

    IFC_RETURN(PopScopeIfRequired(inNode));

    return S_OK;
}

// adds a new parser scope if requested by the node type
_Check_return_ HRESULT BinaryFormatObjectWriter::PushScopeIfRequired(_In_ const ObjectWriterNode& node)
{
    if (node.RequiresNewScope())
    {
        IFC_RETURN(m_spRuntime->PushScope(node.GetLineInfo()));
    }

    return S_OK;
}

// removes a parser scope if requested by the node type
_Check_return_ HRESULT BinaryFormatObjectWriter::PopScopeIfRequired(_In_ const ObjectWriterNode& node)
{
    if (node.RequiresScopeToEnd())
    {
        m_qoLastInstance = m_spContext->Current().get_Instance();
        m_qoLastType = m_spContext->Current().get_Type();

        IFC_RETURN(m_spRuntime->PopScope(node.GetLineInfo()));
    }

    return S_OK;
}

// adds a namespace prefix on the parser context stack
_Check_return_ HRESULT BinaryFormatObjectWriter::AddNamespacePrefix(_In_ const ObjectWriterNode& node)
{
    IFC_RETURN(m_spRuntime->AddNamespacePrefix(node.GetLineInfo(), node.GetStringValue(), node.GetNamespace()));

    return S_OK;
}

// create a new type instance and place it on the parser context stack
// if there was a provided root instance, the first frame will use the
// existing root instance instead of creating a new type.
_Check_return_ HRESULT BinaryFormatObjectWriter::CreateInstanceFromType(_In_ const ObjectWriterNode& node)
{
    std::shared_ptr<XamlQualifiedObject> outInstance;

    if (m_spContext->IsInConditionalScope(false))
    {
        IFC_RETURN(m_spErrorService->ValidateIsKnown(node.GetXamlType(), node.GetLineInfo()));
    }

    // no special handling for non root objects
    if (m_rootObjectCreated)
    {
        IFC_RETURN(m_spRuntime->CreateType(node.GetLineInfo(), node.GetXamlType(), m_spObjectWriterCallbacks, m_qoRootObjectInstance, outInstance));
    }
    else
    {
        // check if an existing root was provided
        if (m_qoRootObjectInstance)
        {
            ASSERT(m_spContext->get_LiveDepth() == 1);
            // handle the existing framework root object instance case
            CDependencyObject* pDO = m_qoRootObjectInstance->GetDependencyObject();
            // adjust the token type for a resource dictionary root
            if (pDO && pDO->OfTypeByIndex<KnownTypeIndex::ResourceDictionary>())
            {
                if (pDO->GetTypeIndex() == KnownTypeIndex::ColorPaletteResources)
                {
                    m_qoRootObjectInstance->SetTypeToken(XamlTypeToken(tpkNative, KnownTypeIndex::ColorPaletteResources));
                }
                else
                {
                    m_qoRootObjectInstance->SetTypeToken(XamlTypeToken(tpkNative, KnownTypeIndex::ResourceDictionary));
                }
            }
            outInstance = m_qoRootObjectInstance;
        }
        else
        {
            IFC_RETURN(m_spRuntime->CreateType(node.GetLineInfo(), node.GetXamlType(), m_spObjectWriterCallbacks, m_qoRootObjectInstance, outInstance));
        }

        m_rootObjectCreated = outInstance;

        if( m_rootObjectCreated && m_spContext->get_RootNamescope() )
        {
            // If the name scope helper doesn't have a namescope owner yet, use this one.  (This establishes the root.)
            m_spContext->get_RootNamescope()->EnsureNamescopeOwner(outInstance);
        }
    }

    m_spContext->Current().set_Type(node.GetXamlType());
    m_spContext->Current().set_Instance(outInstance);

    m_qoLastInstance = std::move(outInstance);
    m_qoLastType = node.GetXamlType();

    return S_OK;
}

// initialize phase for the current instance
_Check_return_ HRESULT BinaryFormatObjectWriter::BeginInitOnCurrentInstance(_In_ const ObjectWriterNode& node)
{
    IFC_RETURN(m_spRuntime->BeginInit(node.GetLineInfo(), m_spContext->Current().get_Type(), m_spContext->Current().get_Instance()));

    return S_OK;
}

// signal the end of parse for the instance
_Check_return_ HRESULT BinaryFormatObjectWriter::EndInitOnCurrentInstance(_In_ const ObjectWriterNode& node)
{
    ASSERT(m_spContext->Current().exists_Instance());

    if (m_spContext->Current().exists_ReplacementPropertyValues())
    {
        // Set a flag so we don't trigger the duplicate property assignment check
        m_replacingPropertyValues = true;
        auto scopeGuard = wil::scope_exit([this]
        {
            this->m_replacingPropertyValues = false;
        });

        SP_MapPropertyToQO spRemainingPropertiesMap = m_spContext->Current().get_ReplacementPropertyValues();

        for (MapPropertyToQO::iterator iterRemainingProperties = spRemainingPropertiesMap->begin();
            iterRemainingProperties != spRemainingPropertiesMap->end();
            ++iterRemainingProperties)
        {
            std::shared_ptr<XamlProperty> spReplacementProperty;
            std::shared_ptr<XamlTextSyntax> spConverter;
            std::shared_ptr<XamlQualifiedObject> spValue;

            spReplacementProperty = iterRemainingProperties->first;
            IFC_RETURN(spReplacementProperty->get_TextSyntax(spConverter));
            spValue = iterRemainingProperties->second;

            ObjectWriterNode setReplacementValueNode = ObjectWriterNode::MakeSetValueTypeConvertedConstantNode(
                node.GetLineInfo(),
                spReplacementProperty,
                spConverter,
                spValue);

            IFC_RETURN(WriteNode(setReplacementValueNode));
        }
    }

    IFC_RETURN(m_spRuntime->EndInit(node.GetLineInfo(), m_spContext->Current().get_Type(), m_spContext->Current().get_Instance()));

    return S_OK;
}

// fetch the set of localized properties for the current instance
_Check_return_ HRESULT BinaryFormatObjectWriter::GetResourcePropertyBagForCurrentInstance(_In_ const ObjectWriterNode& node)
{
    SP_MapPropertyToQO spPropertyBag;

    ASSERT(m_spContext->Current().exists_Instance());

    IFC_RETURN(m_spRuntime->GetResourcePropertyBag(
        node.GetLineInfo(),
        m_spContext->get_MarkupExtensionContext(),
        node.GetValue(),
        m_spContext->Current().get_Type(),
        m_spContext->Current().get_Instance(),
        spPropertyBag));

    if (spPropertyBag)
    {
        m_spContext->Current().set_ReplacementPropertyValues(spPropertyBag);
    }

    return S_OK;
}


// retrieves an instance from a member on the parent instance
// and makes the retrieved instance the current parser context
_Check_return_ HRESULT BinaryFormatObjectWriter::GetValueFromMemberOnParentInstance(_In_ const ObjectWriterNode& node)
{
    if (m_spContext->IsInConditionalScope(false))
    {
        IFC_RETURN(m_spErrorService->ValidateIsKnown(node.GetXamlProperty(), node.GetLineInfo()));
    }

    std::shared_ptr<XamlQualifiedObject> outInstance;
    std::shared_ptr<XamlType> instanceType;
    std::shared_ptr<XamlSchemaContext> schemaContext;
    IFC_RETURN(m_spContext->get_SchemaContext(schemaContext));

    IFC_RETURN(m_spRuntime->GetValue(node.GetLineInfo(), m_spContext->Parent().get_Instance(), node.GetXamlProperty(), outInstance));
    IFC_RETURN(schemaContext->GetXamlType(outInstance->GetTypeToken(), instanceType));

    m_spContext->Current().set_Instance(outInstance);
    m_spContext->Current().set_Member(node.GetXamlProperty());
    m_spContext->Current().set_Type(instanceType);

    m_qoLastInstance = outInstance;
    m_qoLastType = std::move(instanceType);

    return S_OK;
}

// register name in the namescope table and also
// set the value on the remapped property for x:Name
_Check_return_ HRESULT BinaryFormatObjectWriter::SetNameOnCurrentInstance(_In_ const ObjectWriterNode& node)
{
    std::shared_ptr<XamlProperty> spRemappedProperty;
    IFC_RETURN(m_spRuntime->SetName(node.GetLineInfo(), node.GetValue(), m_spContext->Current().get_Instance(), m_spContext->get_RootNamescope()));
    IFC_RETURN(m_spContext->Current().get_Type()->get_RuntimeNameProperty(spRemappedProperty));
    if (spRemappedProperty)
    {
        IFC_RETURN(PerformDuplicatePropertyAssignmentCheck(spRemappedProperty, node.GetLineInfo()));

        IFC_RETURN(m_spRuntime->SetValue(node.GetLineInfo(), m_spContext->Current().get_Type(), m_spContext->Current().get_Instance(), spRemappedProperty, node.GetValue()));
    }

    TraceSetNameOnCurrentInstanceEnd();

    return S_OK;
}

// calls the component connector with the current instance as the target object
_Check_return_ HRESULT BinaryFormatObjectWriter::SetConnectionIdOnCurrentInstance(_In_ const ObjectWriterNode& node)
{
    TraceSetConnectionIDBegin();
    ASSERT(m_rootObjectCreated);

    std::shared_ptr<XamlQualifiedObject> currentInstance(m_spContext->Current().get_Instance());

    IFC_RETURN(m_spRuntime->SetConnectionId(
        node.GetLineInfo(),
        m_qoEventRoot,
        node.GetValue(),
        currentInstance));

    if (currentInstance.get()->GetValue() == m_rootObjectCreated.get()->GetValue())
    {
        if (!m_qoXBindConnector || m_qoXBindConnector->IsEmpty())
        {
            // Try to get XBind connector for root element

            std::shared_ptr<XamlQualifiedObject> returnedComponentConnector = std::make_shared<XamlQualifiedObject>();

            if (m_qoParentXBindConnector && !m_qoParentXBindConnector->IsEmpty())
            {
                IFC_RETURN(m_spRuntime->GetXBindConnector(
                    node.GetLineInfo(),
                    m_qoParentXBindConnector,
                    node.GetValue(),
                    currentInstance,
                    returnedComponentConnector));
            }

            if (!returnedComponentConnector ||
                returnedComponentConnector->IsEmpty())
            {
                IFC_RETURN(m_spRuntime->GetXBindConnector(
                    node.GetLineInfo(),
                    m_qoEventRoot,
                    node.GetValue(),
                    currentInstance,
                    returnedComponentConnector));
            }

            if (returnedComponentConnector &&
                !returnedComponentConnector->IsEmpty())
            {
                m_qoXBindConnector = returnedComponentConnector;
            }
        }
    }

    if (m_qoXBindConnector &&
        !m_qoXBindConnector->IsEmpty())
    {
        // Call into XBind connector

        IFC_RETURN(m_spRuntime->SetConnectionId(
            node.GetLineInfo(),
            m_qoXBindConnector,
            node.GetValue(),
            currentInstance));
    }
    TraceSetConnectionIDEnd();

    return S_OK;
}

// pushes a constant into the current parser context stack
_Check_return_ HRESULT BinaryFormatObjectWriter::PushConstant(_In_ const ObjectWriterNode& node)
{
    IFC_RETURN(m_spRuntime->PushConstant(node.GetLineInfo(), node.GetValue()));
    m_qoLastInstance = node.GetValue();
    m_qoLastType.reset();

    return S_OK;
}

// set member property on the current instance with the provided value
_Check_return_ HRESULT BinaryFormatObjectWriter::SetValueOnCurrentInstance(_In_ const ObjectWriterNode& node, _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    TraceSetValueOnCurrentInstanceBegin();

    if (m_spContext->IsInConditionalScope(false))
    {
        IFC_RETURN(m_spErrorService->ValidateIsKnown(node.GetXamlProperty(), node.GetLineInfo()));
    }
    if (!m_replacingPropertyValues)
    {
        IFC_RETURN(PerformDuplicatePropertyAssignmentCheck(node.GetXamlProperty(), node.GetLineInfo()));
    }

    IFC_RETURN(m_spRuntime->SetValue(node.GetLineInfo(), m_spContext->Current().get_Type(), m_spContext->Current().get_Instance(), node.GetXamlProperty(), spValue));

    if (EventEnabledSetValueOnCurrentInstanceEnd())
    {
        xstring_ptr typeFullName, propertyFullName;
        std::shared_ptr<XamlType> propertyType;
        IFC_RETURN(node.GetXamlProperty()->get_Type(propertyType));
        propertyType->get_FullName(&propertyFullName);
        m_spContext->Current().get_Type()->get_FullName(&typeFullName);

        TraceSetValueOnCurrentInstanceEnd(typeFullName.GetBuffer(), propertyFullName.GetBuffer());
    }

    return S_OK;
}

// type convert a value
// if createVAlueTypeAsObject is true then we create DOs for
// value type objects like int,bool etc.
// this is required for cases where they exist in resource dictionaries
// as <x:Int32>10</x:Int32>
_Check_return_ HRESULT BinaryFormatObjectWriter::TypeConvertValue(
    _In_ const ObjectWriterNode& node,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlTextSyntax>& spConverter,
    _In_ const bool createValueTypeAsObject,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spConvertedInstance)
{
    if (m_spContext->IsInConditionalScope(false))
    {
        IFC_RETURN(m_spErrorService->ValidateIsKnown(spType, node.GetLineInfo()));
    }

    IFC_RETURN(m_spRuntime->TypeConvertValue(
            node.GetLineInfo(),
            m_spContext->get_MarkupExtensionContext(),
            spType,
            spConverter,
            node.GetValue(),
            createValueTypeAsObject,
            spConvertedInstance));

    m_qoLastInstance = spConvertedInstance;
    m_qoLastType = spType;

    if (createValueTypeAsObject)
    {
        m_spContext->Current().set_Instance(spConvertedInstance);
        m_spContext->Current().set_Type(spType);
    }

    return S_OK;
}

// type convert a resolved type (as a CType)
_Check_return_ HRESULT BinaryFormatObjectWriter::TypeConvertResolvedType(
    _In_ const ObjectWriterNode& node,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlTextSyntax>& spConverter,
    _In_ const bool createValueTypeAsObject,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spConvertedInstance)
{
    xref_ptr<CType> spResolvedType;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    ASSERT(createValueTypeAsObject == false);
    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    CCoreServices* const core = spSchemaContext->GetCore();
    IFC_RETURN(spResolvedType.init(new CType(core)));
    IFC_RETURN(spResolvedType->FromXamlType(node.GetXamlTypeProxy()));

    IFC_RETURN(XamlQualifiedObject::CreateNoAddRef(
        core,
        XamlTypeToken(tpkNative, KnownTypeIndex::TypeName),
        spResolvedType.detach(),
        spConvertedInstance));

    m_qoLastInstance = spConvertedInstance;
    m_qoLastType = spType;

    return S_OK;
}

// type convert a resolved property (as a CDependencyPropertyProxy)
_Check_return_ HRESULT BinaryFormatObjectWriter::TypeConvertResolvedProperty(
    _In_ const ObjectWriterNode& node,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlTextSyntax>& spConverter,
    _In_ const bool createValueTypeAsObject,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spConvertedInstance)
{
    xref_ptr<CDependencyPropertyProxy> spResolvedProperty;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    ASSERT(createValueTypeAsObject == false);
    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    CCoreServices* const core = spSchemaContext->GetCore();
    IFC_RETURN(spResolvedProperty.init(new CDependencyPropertyProxy(core)));
    IFC_RETURN(spResolvedProperty->FromXamlProperty(node.GetXamlPropertyProxy()));

    IFC_RETURN(XamlQualifiedObject::CreateNoAddRef(
        core,
        XamlTypeToken(tpkNative, KnownTypeIndex::DependencyPropertyProxy),
        spResolvedProperty.detach(),
        spConvertedInstance));

    m_qoLastInstance = spConvertedInstance;
    m_qoLastType = spType;

    return S_OK;
}

// add the last instance on the parser context to a collection
_Check_return_ HRESULT BinaryFormatObjectWriter::AddToCollectionOnCurrentInstance(_In_ const ObjectWriterNode& node)
{
    TraceAddToCollectionOnCurrentInstanceBegin();

    IFC_RETURN(m_spErrorService->ValidateFrameForCollectionAdd(m_spContext->Current(), node.GetLineInfo()));
    IFC_RETURN(m_spRuntime->AddToCollection(node.GetLineInfo(), m_spContext->Current().get_Type(), m_spContext->Current().get_Instance(), m_qoLastInstance, m_qoLastType));

    TraceAddToCollectionOnCurrentInstanceEnd();

    return S_OK;
}

// add the last instance on the parser context to a collection
_Check_return_ HRESULT BinaryFormatObjectWriter::SetTemplateBindingOnCurrentInstance(_In_ const ObjectWriterNode& node)
{
    if (m_spContext->IsInConditionalScope(false))
    {
        IFC_RETURN(m_spErrorService->ValidateIsKnown(node.GetXamlProperty(), node.GetLineInfo()));
    }

    const CDependencyProperty* sourceProperty = DirectUI::MetadataAPI::GetPropertyBaseByIndex(node.GetXamlProperty()->get_PropertyToken().GetHandle())->AsOrNull<CDependencyProperty>();
    const CDependencyProperty* targetProperty = DirectUI::MetadataAPI::GetPropertyBaseByIndex(node.GetXamlPropertyProxy()->get_PropertyToken().GetHandle())->AsOrNull<CDependencyProperty>();

    if (!sourceProperty || !targetProperty)
    {
        // We don't support template binding on anything that isn't a dependency property or custom CLR properties, so no-op
        XAML_FAIL_FAST();
    }

    CDependencyObject* target = m_spContext->Current().get_Instance()->GetDependencyObject();
    ASSERT(target);

    IFC_RETURN(target->SetTemplateBinding(sourceProperty, targetProperty));

    return S_OK;
}

// add a keyed item to the dictionary instance
// if there was no key provided, perform a runtime search to
// find a matching key property
_Check_return_ HRESULT BinaryFormatObjectWriter::AddToDictionaryOnCurrentInstance(_In_ const ObjectWriterNode& node)
{
    std::shared_ptr<XamlQualifiedObject> spValue = node.GetValue();
    std::shared_ptr<XamlProperty> spKeyProperty;

    // if there was no provided key property, look for it
    // by retrieving members
    if (!spValue)
    {
        XamlQualifiedObject value;
        IFC_RETURN(m_qoLastType->get_DictionaryKeyProperty(spKeyProperty));
        if (spKeyProperty)
        {
            IGNOREHR(spKeyProperty->GetValue(*m_qoLastInstance, value));
        }

        if (value.IsUnset())
        {
            IFC_RETURN(m_qoLastType->get_RuntimeNameProperty(spKeyProperty));
            if (spKeyProperty)
            {
                IGNOREHR(spKeyProperty->GetValue(*m_qoLastInstance, value));
            }
        }
        spValue = std::make_shared<XamlQualifiedObject>(std::move(value));
    }

    ASSERT(spValue);

    IFC_RETURN(m_spRuntime->AddToDictionary(node.GetLineInfo(), m_spContext->Current().get_Instance(), m_qoLastInstance, m_qoLastType, spValue));

    return S_OK;
}

// looks up a static resource reference and provides the value on the parser m_qoLastInstance
_Check_return_ HRESULT BinaryFormatObjectWriter::ProvideStaticResourceReference(_In_ const ObjectWriterNode& node, _In_opt_ CStyle* optimizedStyleParent, _In_ KnownPropertyIndex stylePropertyIndex)
{
    TraceProvideStaticResourceReferenceBegin();
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    CDependencyObject *pObjectNoRef = nullptr;

    auto traceGuard = wil::scope_exit([&]
    {
        if (EventEnabledProvideStaticResourceReferenceEnd())
        {
            if (!pObjectNoRef)
            {
                TraceProvideStaticResourceReferenceEnd(L"Failed to find Resource");
            }
            else
            {
                xstring_ptr fullName;
                m_qoLastType->get_FullName(&fullName);
                TraceProvideStaticResourceReferenceEnd(fullName.GetBuffer());
            }
        }
    });

    auto& staticResourceValue = node.GetValue()->GetValue();
    ASSERT(staticResourceValue.GetType() == valueString);
    auto staticResourceKey = staticResourceValue.AsString();

    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    CCoreServices* const core = spSchemaContext->GetCore();

    IFC_RETURN(CStaticResourceExtension::LookupResourceNoRef(staticResourceKey, m_spContext->get_MarkupExtensionContext(), core, &pObjectNoRef, TRUE, optimizedStyleParent, stylePropertyIndex));
    // If we can't find an object that corresponds to the key, raise an error
    // like "Cannot find a Resource with the Name/Key %0".
    if (!pObjectNoRef)
    {
        // Rerun the search with logging enabled
        xstring_ptr traceMessage;
        {
            auto cleanupGuard = wil::scope_exit([&]
            {
                TRACE_HR_NORETURN(core->GetResourceLookupLogger()->Stop(staticResourceKey, traceMessage));
            });

            xstring_ptr currentUri;
            if (const auto baseUri = m_spContext->get_BaseUri())
            {
                IGNOREHR(UriXStringGetters::GetPath(baseUri, &currentUri));
            }

            IFC_RETURN(core->GetResourceLookupLogger()->Start(staticResourceKey, currentUri));
            IFC_RETURN(CStaticResourceExtension::LookupResourceNoRef(staticResourceKey, m_spContext->get_MarkupExtensionContext(), core, &pObjectNoRef, TRUE, optimizedStyleParent, stylePropertyIndex));
        }

        // Record the message in an error context
        std::vector<std::wstring> extraInfo;
        extraInfo.push_back(std::wstring(traceMessage.GetBuffer()));

        HRESULT xr = CErrorService::OriginateInvalidOperationError(
            core,
            AG_E_PARSER_FAILED_RESOURCE_FIND,
            staticResourceKey);
        IFC_RETURN_EXTRA_INFO(m_spErrorService->WrapErrorWithParserErrorAndRethrow(xr, node.GetLineInfo()), &extraInfo);
    }

    // TODO: This is pretty terrible way to pack the object - revisit.
    {
        std::shared_ptr<XamlQualifiedObject> qoValue;
        std::shared_ptr<XamlType> spXamlType;
        if (!do_pointer_cast<CNullKeyedResource>(pObjectNoRef))
        {
            XamlTypeToken typeToken;

            IFC_RETURN(XamlNativeRuntime::GetTypeTokenForDO(pObjectNoRef, typeToken));
            // TODO: move this setting of the TypeToken some where central.
            qoValue = std::make_shared<XamlQualifiedObject>(typeToken);
            IFC_RETURN(qoValue->SetDependencyObject(pObjectNoRef));

            // Resources are special, in that Poco objects are actually stored on the managed side. If this
            // is a Poco object, it will consequently be a ManagedObjectReference and pegged.  So we need
            // to set the corresponding flag in the XQO.

            if (pObjectNoRef->OfTypeByIndex<KnownTypeIndex::ExternalObjectReference>())
            {
                qoValue->SetHasPeggedManagedPeer();
            }

            IFC_RETURN(spSchemaContext->GetXamlType(typeToken, spXamlType));
        }
        else
        {
            // The resource key resolving to a CNullKeyedResource indicates that the resource is
            // null-valued. Get a XamlQualifiedObject representing 'null'
            qoValue = spSchemaContext->get_NullKeyedResourceXQO();

            IFC_RETURN(spSchemaContext->GetXamlTypeFromStableXbfIndex(
                Parser::StableXbfTypeIndex::NullKeyedResource,
                spXamlType));
        }

        m_qoLastInstance = qoValue;
        m_qoLastType = spXamlType;
    }

    return S_OK;
}

// looks up a static resource reference and provides the value on the parser m_qoLastInstance
_Check_return_ HRESULT BinaryFormatObjectWriter::ProvideThemeResourceValue(_In_ const ObjectWriterNode& node, _In_opt_ CStyle* optimizedStyleParent, _In_ KnownPropertyIndex stylePropertyIndex)
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    std::shared_ptr<XamlQualifiedObject> qoValue;
    xref_ptr<CThemeResource> spThemeResource;
    auto& themeResourceValue = node.GetValue()->GetValue();
    ASSERT(themeResourceValue.GetType() == valueString);
    auto themeResourceKey = themeResourceValue.AsString();

    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    CCoreServices* const core = spSchemaContext->GetCore();

    IFC_RETURN(CThemeResource::LookupResource(themeResourceKey, m_spContext->get_MarkupExtensionContext(), core, TRUE, spThemeResource.ReleaseAndGetAddressOf(), optimizedStyleParent, stylePropertyIndex));
    // If we can't find an object that corresponds to the key, raise an error
    // like "Cannot find a Resource with the Name/Key %0".
    if (!spThemeResource)
    {
        // Rerun the search with logging enabled
        xstring_ptr traceMessage;
        {
            auto cleanupGuard = wil::scope_exit([&]
            {
                TRACE_HR_NORETURN(core->GetResourceLookupLogger()->Stop(themeResourceKey, traceMessage));
            });

            xstring_ptr currentUri;
            if (const auto baseUri = m_spContext->get_BaseUri())
            {
                IGNOREHR(UriXStringGetters::GetPath(baseUri, &currentUri));
            }

            IFC_RETURN(core->GetResourceLookupLogger()->Start(themeResourceKey, currentUri));
            IFC_RETURN(CThemeResource::LookupResource(themeResourceKey, m_spContext->get_MarkupExtensionContext(), core, TRUE, spThemeResource.ReleaseAndGetAddressOf(), optimizedStyleParent, stylePropertyIndex));
        }

        // Record the message in an error context
        std::vector<std::wstring> extraInfo;
        extraInfo.push_back(std::wstring(traceMessage.GetBuffer()));

        HRESULT hr = CErrorService::OriginateInvalidOperationError(
            core,
            AG_E_PARSER_FAILED_RESOURCE_FIND,
            themeResourceKey);
        IFC_RETURN_EXTRA_INFO(m_spErrorService->WrapErrorWithParserErrorAndRethrow(hr, node.GetLineInfo()), &extraInfo);
    }

    XamlTypeToken typeToken = XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ThemeResource));
    std::shared_ptr<XamlType> spXamlType;

    qoValue = std::make_shared<XamlQualifiedObject>(typeToken);
    qoValue.get()->GetValue().SetThemeResourceNoRef(spThemeResource.detach());

    IFC_RETURN(spSchemaContext->GetXamlType(typeToken, spXamlType));
    m_qoLastInstance = qoValue;
    m_qoLastType = spXamlType;

    return S_OK;
}

// calls the markup extension provide value method to retrieve the value.
_Check_return_ HRESULT BinaryFormatObjectWriter::ProvideValueForMarkupExtension(_In_ const ObjectWriterNode& node)
{
    std::shared_ptr<XamlQualifiedObject> spValue;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    std::shared_ptr<XamlType> spXamlType;
    IFC_RETURN(m_spRuntime->ProvideValue(node.GetLineInfo(), m_spContext->get_MarkupExtensionContext(), m_qoLastInstance, spValue));
    if (spValue->GetValue() != m_qoLastInstance->GetValue())
    {
        IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetXamlType(spValue->GetTypeToken(), spXamlType));
        m_qoLastInstance = spValue;
        m_qoLastType = spXamlType;
    }
    else
    {
        ASSERT(m_qoLastInstance->GetHasPeggedManagedPeer());

        // The instance is the value. Make sure that when qoValue goes out of scope, it doesn't
        // unpeg the instance.
        spValue->ClearHasPeggedManagedPeer();
    }

    return S_OK;
}

// sets up and populates a deferred property on the current instance
_Check_return_ HRESULT BinaryFormatObjectWriter::SetCustomRuntimeDataOnCurrentInstance(_In_ const ObjectWriterNode& node)
{
    TraceSetCustomRuntimeDataOnCurrentInstanceBegin();

    IFC_RETURN(m_spRuntime->SetCustomRuntimeData(
        node.GetLineInfo(),
        m_spContext->Current().get_Type(),
        m_spContext->Current().get_Instance(),
        node.GetCustomWriterData(),
        node.GetCustomWriterNodeStream(),
        m_spContext->get_RootNamescope(),
        m_qoXBindConnector,
        m_spObjectWriterCallbacks));

    TraceSetCustomRuntimeDataOnCurrentInstanceEnd();
    return S_OK;
}

// sets up and populates a deferred property on the current instance
_Check_return_ HRESULT BinaryFormatObjectWriter::SetDeferredPropertyOnCurrentInstance(_In_ const ObjectWriterNode& node)
{
    if (m_spContext->IsInConditionalScope(false))
    {
        IFC_RETURN(m_spErrorService->ValidateIsKnown(node.GetXamlProperty(), node.GetLineInfo()));
    }
    IFC_RETURN(PerformDuplicatePropertyAssignmentCheck(node.GetXamlProperty(), node.GetLineInfo()));

    // TODO: The interface required for SetDeferredProperty doesn't match the ObjectWriterRuntime
    // because this interface would like to send a XamlReader rather than the nodelist.
    //
    // Also reconcile with CustomWriter logic.
    CTemplateContent *pTemplateContent = nullptr;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    std::shared_ptr<XamlQualifiedObject> spTemplateContent;
    std::shared_ptr<XamlQualifiedObject> spInstance;
    std::shared_ptr<XamlType> spType;

    spInstance = m_spContext->Current().get_Instance();
    spType = m_spContext->Current().get_Type();

    if (!node.GetValue())
    {
        // we don't have a cached node value so do the runtime work involved in creating a placeholder
        // CTempateContent and then transferring over the context data and node list to it.
        std::shared_ptr<XamlSavedContext> spSavedContext;
        XamlTypeToken sTemplateContentTypeToken(tpkNative, KnownTypeIndex::TemplateContent);
        IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
        CCoreServices* const core = spSchemaContext->GetCore();

        // prepare a CTemplateContent
        {
            CREATEPARAMETERS cp(core);

            IFC_RETURN(CTemplateContent::Create((CDependencyObject**)&pTemplateContent, &cp));
            pTemplateContent->SetIsParsing(TRUE);
        }

        // save the stack
        IFC_RETURN(m_spContext->GetSavedContext(spSavedContext));

        // prepare for runtime context capture
        IFC_RETURN(m_spRuntime->PushScope(node.GetLineInfo()));
        ASSERT(node.GetSubReader());
        IFC_RETURN(pTemplateContent->RecordXaml(m_spContext->get_MarkupExtensionContext(), std::shared_ptr<XamlOptimizedNodeList>(), spSavedContext, node.GetSubReader(), node.GetListOfReferences()));

        IFC_RETURN(m_spRuntime->PopScope(node.GetLineInfo()));

        IFC_RETURN(XamlQualifiedObject::CreateNoAddRef(core, sTemplateContentTypeToken, pTemplateContent, spTemplateContent));
        pTemplateContent = nullptr;
    }
    else
    {
        spTemplateContent = node.GetValue();
    }

    // set the deferred template on the template property
    IFC_RETURN(m_spRuntime->SetValue(
        node.GetLineInfo(),
        m_spContext->Current().get_Type(),
        m_spContext->Current().get_Instance(),
        node.GetXamlProperty(),
        spTemplateContent));

    m_qoLastInstance = spTemplateContent;

    //TODO: Re-enable ETW events
    //IFC(RaiseTemplateOwnerEventImplHelper(m_spContext->Current().get_Instance()));

    return S_OK;
}

std::shared_ptr<XamlQualifiedObject> BinaryFormatObjectWriter::get_Result() const
{
    //Note that in the case that the document didn't parser correctly because of an error
    //m_qoLastInstance may not be m_spContext->get_RootInstance
    return m_qoLastInstance;
}

_Check_return_ HRESULT BinaryFormatObjectWriter::BeginConditionalScope(_In_ const ObjectWriterNode& node)
{
    try
    {
        auto xamlPredicateAndArgs = node.GetXamlPredicateAndArgs();
        auto result = Parser::XamlPredicateService::EvaluatePredicate(xamlPredicateAndArgs->PredicateType, xamlPredicateAndArgs->Arguments);
        m_spContext->Current().PushConditionalScopeNode(!result);

        THROW_IF_FAILED(m_spRuntime->BeginConditionalScope(node.GetLineInfo(), xamlPredicateAndArgs));
    }
    CATCH_RETURN();

    return S_OK;
}

_Check_return_ HRESULT BinaryFormatObjectWriter::EndConditionalScope(_In_ const ObjectWriterNode& node)
{
    m_spContext->Current().PopConditionalScopeNode();

    IFC_RETURN(m_spRuntime->EndConditionalScope(node.GetLineInfo()));

    return S_OK;
}

_Check_return_ HRESULT BinaryFormatObjectWriter::PerformDuplicatePropertyAssignmentCheck(
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const XamlLineInfo& lineInfo)
{
    auto& currentFrame = m_spContext->Current();

    // report error if we are setting a duplicate property
    IFC_RETURN(m_spErrorService->ValidateFrameForPropertySet(currentFrame, spProperty, lineInfo));

    // notify the frame of the property assignment
    currentFrame.NotifyPropertyAssigned(spProperty);

    return S_OK;
}

bool BinaryFormatObjectWriter::IsSkippingForConditionalScope() const
{
    return m_spContext->IsInConditionalScope(true);
}
