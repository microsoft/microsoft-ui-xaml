// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ResourceDictionaryCustomWriter.h"
#include "ResourceDictionaryCustomRuntimeData.h"
#include <ICustomWriterCallbacks.h>
#include <XamlProperty.h>
#include <XamlQualifiedObject.h>
#include <XamlSchemaContext.h>
#include <XamlTextSyntax.h>
#include <XamlType.h>
#include <ObjectWriterContext.h>
#include <ObjectWriter.h>
#include <MetadataAPI.h>
#include <XamlPredicateHelpers.h>

//#define RDLOG(...) RDRDLOG(__VA_ARGS__)
#define RDLOG(...)

ResourceDictionaryCustomWriter::ResourceDictionaryCustomWriter(
    ICustomWriterCallbacks* callbacks,
    std::shared_ptr<XamlSchemaContext> context,
    std::shared_ptr<ObjectWriterContext> objectWriterContext)
    : m_customWriterCallbacks(callbacks)
    , m_context(context)
    , m_objectWriterContext(objectWriterContext)
{
}

_Check_return_ HRESULT ResourceDictionaryCustomWriter::Initialize()
{
    m_runtimeData = std::unique_ptr<ResourceDictionaryCustomRuntimeData>(
        new ResourceDictionaryCustomRuntimeData());

    IFC_RETURN(m_context->get_X_KeyProperty(m_spXKeyProperty));
    IFC_RETURN(m_context->get_X_NameProperty(m_spXNameProperty));

    RDLOG(L"[RDCW]: Created. Beginning custom writer actions for ResourceDictionary.");
    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomWriter::Create(
    _In_ ICustomWriterCallbacks* callbacks,
    _In_ std::shared_ptr<XamlSchemaContext> context,
    _In_ std::shared_ptr<ObjectWriterContext> objectWriterContext,
    _Out_ std::unique_ptr<ICustomWriter>* ppValue)
{
    auto pValue =
        std::unique_ptr<ResourceDictionaryCustomWriter>(
            new ResourceDictionaryCustomWriter(callbacks, context, objectWriterContext));
    IFC_RETURN(pValue->Initialize());
    *ppValue = std::move(pValue);
    return S_OK;
}

#pragma region ICustomWriter

_Check_return_ HRESULT ResourceDictionaryCustomWriter::WriteObject(
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ bool bFromMember,
    _Out_ bool* pResult)
{
    if (m_stackDepth == 0)
    {
        RDLOG(L"[RDCW]: Beginning capture of resource dictionary.");

        m_operationsStack.push_back(std::make_pair(m_stackDepth, Operation::WritingResourceDictionary));
    }
    else
    {
        auto currentOperation = m_operationsStack.back().second;

        switch (currentOperation)
        {
            case Operation::WritingImplicitItems:
            {
                auto typeToken = spXamlType->get_TypeToken();

                if (typeToken == XamlTypeToken(tpkNative, KnownTypeIndex::ResourceDictionary) ||
                    typeToken == XamlTypeToken(tpkNative, KnownTypeIndex::ColorPaletteResources))
                {
                    RDLOG(L"[RDCW]: Skipping resource of type ResourceDictionary.");
                    m_operationsStack.push_back(std::make_pair(m_stackDepth, Operation::Pass));
                }
                else
                {
                    RDLOG(L"[RDCW]: Beginning capture of deferred resource.");

                    if (!m_hasNamespace)
                    {
                        IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&m_currentResourceOffset));
                        RDLOG(L"[RDCW]: StreamOffsetToken created.");
                    }

                    std::shared_ptr<XamlProperty> spKeyProperty;
                    IFC_RETURN(spXamlType->get_DictionaryKeyProperty(spKeyProperty));
                    m_currentResourceImplicitKey = std::make_pair(spKeyProperty, xstring_ptr::NullString());

                    m_currentResourceXKey = std::make_pair(m_spXKeyProperty, xstring_ptr::NullString());
                    m_currentResourceXName = std::make_pair(m_spXNameProperty, xstring_ptr::NullString());

                    m_operationsStack.push_back(std::make_pair(m_stackDepth, Operation::WritingResource));

                    m_allowNewCustomWriter = false;
                    m_customWriterCallbacks->SetAllowProcessStartObjectByCustomWriter(true);
                }
                break;
            }
            case Operation::Pass:
            {
#if DBG
                xstring_ptr typeName;
                IFC_RETURN(spXamlType->get_Name(&typeName));
                RDLOG(L"[RDCW]: Encountered new object of type %s, but passing.", typeName.GetBuffer());
#else
                RDLOG(L"[RDCW]: Encountered new object, but passing.");
#endif
                break;
            }
        }
    }

    m_stackDepth++;

    *pResult = (m_operationsStack.back().second != Operation::Pass);
    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomWriter::WriteEndObject(_Out_ bool* pResult)
{
    ASSERT(m_stackDepth > 0);
    --m_stackDepth;

    auto currentOperationDepth = m_operationsStack.back().first;
    auto currentOperation = m_operationsStack.back().second;

    switch (currentOperation)
    {
        case Operation::WritingResourceDictionary:
        {
            if (currentOperationDepth == m_stackDepth)
            {
                // If this is the last pending operation after this call returns the CustomWriter
                // will be destroyed.
                m_operationsStack.pop_back();
                m_allowNewCustomWriter = true;
                m_customWriterCallbacks->SetAllowProcessStartObjectByCustomWriter(false);

                RDLOG(L"[RDCW]: Finishing capture of object");
            }
            break;
        }
        case Operation::WritingResource:
        {
            // The resource key precedence rules are as follows:
            // 1) If x:Key property is present, use that as the key, else
            // 2) If x:Name property is present, use that as the key, else
            // Switch to implicit key rules:
            // 3) If the resource type specifies a DictionaryKeyProperty, use that as the key;
            //    currently this is only Style.TargetType.
            xstring_ptr resourceKey;
            bool bIsImplicitKey = false;

            if (!m_currentResourceXKey.second.IsNull())
            {
                resourceKey = m_currentResourceXKey.second;
            }
            else if (!m_currentResourceXName.second.IsNull())
            {
                resourceKey = m_currentResourceXName.second;
            }
            else if (!m_currentResourceImplicitKey.second.IsNull())
            {
                resourceKey = m_currentResourceImplicitKey.second;
                bIsImplicitKey = true;
            }

            if (!resourceKey.IsNull())
            {
                ASSERT(m_runtimeData != nullptr);
                IFC_RETURN(m_runtimeData->AddResourceOffset(
                    resourceKey,
                    m_currentResourceOffset,
                    bIsImplicitKey,
                    !m_currentResourceXName.second.IsNull()));

                ++m_resourceCount;
                m_hasNamespace = false;

                RDLOG(L"[RDCW]: Writing resource with %s %s.", (bIsImplicitKey) ? L"implicit key" : L"key", resourceKey.GetBuffer());
            }

            if (currentOperationDepth == m_stackDepth)
            {
                m_operationsStack.pop_back();
                RDLOG(L"[RDCW]: Finishing capture of resource.");
                m_allowNewCustomWriter = true;
            }

            break;
        }
        case Operation::Pass:
        {
            if (currentOperationDepth == m_stackDepth)
            {
                m_operationsStack.pop_back();
                RDLOG(L"[RDCW]: Finishing skipped object");
                m_allowNewCustomWriter = true;
            }
            break;
        }
    }

    // For a WriteEndXXX operation, we actually want to compare against the operation
    // we (potentially) just popped off the stack, rather than the current top of the stack.
    *pResult = (currentOperation != Operation::Pass);
    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomWriter::WriteMember(
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _Out_ bool* pResult)
{
    auto currentOperation = m_operationsStack.back().second;

    switch (currentOperation)
    {
        case Operation::WritingResource:
        {
            Operation operation = Operation::Miscellaneous;
            // Writing the properties for the resource ... prepare to save away the ones
            // that potentially represent the key
            if (XamlProperty::AreEqual(spProperty, m_currentResourceXKey.first))
            {
                operation = Operation::WritingResourceXKey;
            }
            else if (XamlProperty::AreEqual(spProperty, m_currentResourceXName.first))
            {
                operation = Operation::WritingResourceXName;
            }
            else if (XamlProperty::AreEqual(spProperty, m_currentResourceImplicitKey.first))
            {
                operation = Operation::WritingResourceImplicitKey;
            }

            m_operationsStack.push_back(std::make_pair(m_stackDepth, operation));

            break;
        }
        case Operation::WritingResourceDictionary:
        {
            // For top level properties that aren't the implicit items property,
            // we want to hand back to ObjectWriter the responsibility for processing them
            if (spProperty->IsImplicit() && std::static_pointer_cast<ImplicitProperty>(spProperty)->get_ImplicitPropertyType() == iptItems)
            {
                RDLOG(L"[RDCW]: Beginning member write of implicit items property.");
                m_operationsStack.push_back(std::make_pair(m_stackDepth, Operation::WritingImplicitItems));
            }
            else
            {
#if DBG
                xstring_ptr propName;
                IGNOREHR(spProperty->get_Name(&propName));
                RDLOG(L"[RDCW]: Skipping processing of top level property %s.", propName.GetBuffer());
#else
                RDLOG(L"[RDCW]: Skipping processing of top level property.");
#endif
                m_operationsStack.push_back(std::make_pair(m_stackDepth, Operation::Pass));
            }

            break;
        }
    }

    m_stackDepth++;

    // Because of the way implicit properties are handled by the ObjectWriter, we need to let
    // the WriteMember for the implicit property be handled by the parent ObjectWriter, so the
    // return value is determined by a comparison against Operation::WritingImplicitItems
    // in addition to Operation::Pass.
    *pResult = (m_operationsStack.back().second != Operation::Pass && m_operationsStack.back().second != Operation::WritingImplicitItems);
    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomWriter::WriteEndMember(_Out_ bool* pResult)
{
    ASSERT(m_stackDepth > 0);
    --m_stackDepth;

    auto currentOperationDepth = m_operationsStack.back().first;
    auto currentOperation = m_operationsStack.back().second;

    if (currentOperationDepth == m_stackDepth)
    {
        switch (currentOperation)
        {
            case Operation::Pass:
            {
                RDLOG(L"[RDCW]: Finished skipped top level property. Resuming processing of nodes.");
                break;
            }
            case Operation::WritingImplicitItems:
            {
                // We have finished writing the implicit items collection
                if (m_resourceCount > 0)
                {
                    // if we have only 1 element in the dictionary don't defer it
                    if (m_resourceCount == 1)
                    {
                        m_runtimeData->SetShouldEncodeAsCustomData(false);
                        RDLOG(L"[RDCW]: Only one value in dictionary, not saving CustomWriterRuntimeData.");
                    }
                    IFC_RETURN(m_customWriterCallbacks->SetCustomWriterRuntimeData(
                        std::unique_ptr<CustomWriterRuntimeData>(m_runtimeData.release())));

                    RDLOG(L"[RDCW]: Finished writing implicit items collection, saving CustomWriterRuntimeData.");
                }

                break;
            }
        }
        m_operationsStack.pop_back();
    }

    // For a WriteEndXXX operation, we actually want to compare against the operation
    // we (potentially) just popped off the stack, rather than the current top of the stack.
    //
    // Because of the way implicit properties are handled by the ObjectWriter, we need to let
    // the WriteMember for the implicit property be handled by the parent ObjectWriter, so the
    // return value is determined by a comparison against Operation::WritingImplicitItems
    // in addition to Operation::Pass.
    *pResult = (currentOperation != Operation::Pass && currentOperation != Operation::WritingImplicitItems);
    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomWriter::WriteConditionalScope(
    _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs,
    _Out_ bool* pResult)
{
    RDLOG(L"[RDCW]: WriteConditionalScope passed in.");
    if (m_runtimeData != nullptr)
    {
        m_runtimeData->PushConditionalScope(xamlPredicateAndArgs);
    }

    *pResult = (m_operationsStack.back().second != Operation::Pass);
    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomWriter::WriteEndConditionalScope(_Out_ bool* pResult)
{
    RDLOG(L"[RDCW]: WriteEndConditionalScope passed in.");
    if (m_runtimeData != nullptr)
    {
        m_runtimeData->PopConditionalScope();
    }

    *pResult = (m_operationsStack.back().second != Operation::Pass);
    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomWriter::WriteValue(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _Out_ bool* pResult)
{
    auto currentOperation = m_operationsStack.back().second;

    switch (currentOperation)
    {
        case Operation::WritingResourceXKey:
        {
            m_currentResourceXKey.second = QualifiedObjectToStringKey(spValue);
            break;
        }
        case Operation::WritingResourceXName:
        {
            m_currentResourceXName.second = QualifiedObjectToStringKey(spValue);
            break;
        }
        case Operation::WritingResourceImplicitKey:
        {
            std::shared_ptr<XamlTextSyntax> spTextSyntax;
            std::shared_ptr<XamlType> spType;
            std::shared_ptr<XamlType> spTargetXamlType;
            xstring_ptr implicitKey;

            IFC_RETURN(m_currentResourceImplicitKey.first->get_Type(spType));
            IFC_RETURN(spType->get_TextSyntax(spTextSyntax));

            ASSERT(spTextSyntax->get_TextSyntaxToken() == XamlTypeToken(tpkNative, KnownTypeIndex::TypeName));
            ASSERT(spValue->GetValue().GetType() == valueString);

            IFC_RETURN(m_objectWriterContext->get_MarkupExtensionContext()->ResolveXamlType(spValue->GetValue().AsString(), spTargetXamlType));

            if (spTargetXamlType)
            {
                IFC_RETURN(spTargetXamlType->get_FullName(&implicitKey));
                m_currentResourceImplicitKey.second = implicitKey;
            }

            break;
        }
    }

    *pResult = (m_operationsStack.back().second != Operation::Pass);
    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomWriter::WriteNamespace(
    _In_ const xstring_ptr& spPrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace,
    _Out_ bool* pResult)
{
    auto currentOperation = m_operationsStack.back().second;

    switch (currentOperation)
    {
        case Operation::WritingImplicitItems:
        {
            if (!m_hasNamespace)
            {
                m_hasNamespace = true;
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&m_currentResourceOffset));

                RDLOG(L"[RDCW]: WriteNamespace passed in. StreamOffsetToken created.");
            }
            else
            {
                RDLOG(L"[RDCW]: WriteNamespace passed in. StreamOffsetToken already created.");
            }

            break;
        }
        default:
        {
            RDLOG(L"[RDCW]: WriteNamespace passed in.");
            break;
        }
    }

    *pResult = (m_operationsStack.back().second != Operation::Pass);
    return S_OK;
}

bool ResourceDictionaryCustomWriter::IsCustomWriterFinished() const
{
    return m_operationsStack.size() == 0;
}

// Called by the CustomerWriterManager to get permission before creating a new custom writer.
// We always allow new custom writers except after we start writing a resource or we encounter a style.
bool ResourceDictionaryCustomWriter::ShouldAllowNewCustomWriter(_In_ const std::shared_ptr<XamlType>& spXamlType)
{
    return m_allowNewCustomWriter &&
           (spXamlType == nullptr || !spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::Style)));
}

#pragma endregion

xstring_ptr ResourceDictionaryCustomWriter::QualifiedObjectToStringKey(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    xstring_ptr ssKey;

    // We only support strings as keys to ResourceDictionary.
    IFCFAILFAST(spValue->GetCopyAsString(&ssKey));
    ASSERT(!ssKey.IsNull());

    return ssKey;
}

ResourceDictionaryCustomWriterActivator::ResourceDictionaryCustomWriterActivator(
    _In_ const std::shared_ptr<ObjectWriterContext>& spContext)
    : m_spContext(spContext)
{}

_Check_return_ HRESULT ResourceDictionaryCustomWriterActivator::ShouldActivateCustomWriter(
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ bool* pResult)
{
    using namespace DirectUI;

    if (activatorState.GetTrigger() == ActivatorTrigger::ObjectStart)
    {
        ASSERT(activatorState.GetObject(nullptr));

        auto& typeToken = activatorState.GetObject(nullptr)->get_TypeToken();

        *pResult =
            typeToken.Equals(XamlTypeToken::FromType(DirectUI::MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ResourceDictionary))) ||
            typeToken.Equals(XamlTypeToken::FromType(DirectUI::MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ColorPaletteResources)));
    }
    else
    {
        *pResult = false;
    }

    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomWriterActivator::CreateNodeStreamCollectingWriter(
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ std::shared_ptr<ObjectWriter>* pWriter)
{
    std::shared_ptr<XamlSavedContext> spSavedContext;
    IFC_RETURN(m_spContext->GetSavedContext(spSavedContext));

    ObjectWriterSettings settings;
    settings.set_EnableEncoding(true);
    settings.set_ExpandTemplates(m_spContext->get_ExpandTemplates());

    std::shared_ptr<ObjectWriter> spNodeStreamCollectingWriter;

    IFC_RETURN(ObjectWriter::Create(
        spSavedContext,
        settings,
        spNodeStreamCollectingWriter));

    // Add namespaces
    for (const auto& namespacePair : activatorState.GetNamespaces())
    {
        IFC_RETURN(spNodeStreamCollectingWriter->WriteNamespace(
            namespacePair.first,
            namespacePair.second));
    }

    *pWriter = std::move(spNodeStreamCollectingWriter);
    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomWriterActivator::CreateCustomWriter(
    _In_ ICustomWriterCallbacks* pCallbacks,
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ std::unique_ptr<ICustomWriter>* pWriter)
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    IFC_RETURN(ResourceDictionaryCustomWriter::Create(pCallbacks, spSchemaContext, m_spContext, pWriter));

    return S_OK;
}
