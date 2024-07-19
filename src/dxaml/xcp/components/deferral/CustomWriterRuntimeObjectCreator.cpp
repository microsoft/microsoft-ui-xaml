// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CustomWriterRuntimeObjectCreator.h>
#include <CustomWriterRuntimeContext.h>
#include <StreamOffsetToken.h>

#include <XamlReader.h>
#include <XamlWriter.h>
#include <ObjectWriter.h>
#include <ObjectWriterSettings.h>
#include <IObjectWriterCallbacks.h>
#include <SavedContext.h>
#include <ObjectWriterFrame.h>
#include <ObjectWriterStack.h>
#include <XamlOptimizedNodeList.h>
#include <ParserErrorService.h>
#include <XamlQualifiedObject.h>
#include <INamescope.h>
#include <XamlBinaryFormatSubReader2.h>
#include <ObjectWriterNode.h>
#include <BinaryFormatObjectWriter.h>
#include <XamlBinaryFormatSubReader2.h>
#include <TemplateNamescope.h>
#include <LineInfo.h>
#include <ThemeResource.h>

class CStyle;

CustomWriterRuntimeObjectCreator::CustomWriterRuntimeObjectCreator(
    NameScopeRegistrationMode mode,
    _In_ const CustomWriterRuntimeContext* context)
    : m_context(context)
    , m_pendingFirstNode(false)
    , m_mode(mode)
    , m_restoreIndex(0)
{}

_Check_return_ HRESULT
CustomWriterRuntimeObjectCreator::CreateInstance(
    _Out_ std::shared_ptr<CDependencyObject>* pResult,
    _Out_ xref_ptr<CThemeResource>* resultAsThemeResource)
{
    IFC_RETURN(CreateInstance(StreamOffsetToken(), pResult, resultAsThemeResource));

    return S_OK;
}

_Check_return_ HRESULT
CustomWriterRuntimeObjectCreator::CreateInstance(
    _In_ StreamOffsetToken token,
    _Out_ std::shared_ptr<CDependencyObject>* pResult,
    _Out_ xref_ptr<CThemeResource>* resultAsThemeResource)
{
    IFC_RETURN(EnsureObjectWriter());
    m_pendingFirstNode = true;
    IFC_RETURN(RunObjectWriter(token, nullptr, std::vector<StreamOffsetToken>()));
    std::shared_ptr<XamlQualifiedObject> result = m_writer->get_Result();
    ASSERT(result);

    if (result->GetValue().GetType() == ValueType::valueThemeResource)
    {
        *resultAsThemeResource = result->GetValue().As<valueThemeResource>();
    }
    else
    {
        // Return DependencyObject with ownership back to the caller.
        *pResult = result->GetAndTransferDependencyObjectOwnership();
    }

    return S_OK;
}

_Check_return_ HRESULT CustomWriterRuntimeObjectCreator::ApplyStreamToExistingInstance(
    _In_ StreamOffsetToken token, _In_ CDependencyObject* instance,
    _In_ const std::vector<StreamOffsetToken>& indexRangesToSkip)
{
    IFC_RETURN(EnsureObjectWriter());
    m_pendingFirstNode = true;
    IFC_RETURN(RunObjectWriter(token, instance, indexRangesToSkip));

    return S_OK;
}

_Check_return_ HRESULT CustomWriterRuntimeObjectCreator::RunObjectWriter(_In_ StreamOffsetToken token, _In_opt_ CDependencyObject* instance,
    _In_ const std::vector<StreamOffsetToken>& indexRangesToSkip)
{
    ASSERT(!(indexRangesToSkip.size() % 2));
    auto nextIndexRangeToSkip = indexRangesToSkip.begin();
    auto reader = GetReaderAndSetIndex(token.GetIndex());
    auto resetReaderGuard = wil::scope_exit([&]()
    {
        // Reset the reader's original position before returning
        RestoreReaderIndex();
    });

    int streamDepth = 0;
    ObjectWriterNode node;

    while (true)
    {
        // In the CustomWriterRuntimeObjectCreator we terminate based on stream depth,
        // not upon reaching the end of the node stream.
        VERIFY(reader->TryRead(node));

        auto nextIndex = reader->get_NextIndex();

        if (node.RequiresNewScope())
        {
            streamDepth++;
        }
        else if (node.RequiresScopeToEnd())
        {
            streamDepth--;
        }

        // For the first node when we're applying this node stream to an existing instance we have to
        // tweak the node stream a little bit to run it through the object writer. If this is a
        // collection we don't send the node to the object writer and instead call SetActiveObject, which
        // stands in for the PushScopeGetValue call.
        // TODO: This doesn't work for non-collection instances today, but the changed needed to make it work
        // are trivial.
        if (m_pendingFirstNode && instance &&
            node.GetNodeType() == ObjectWriterNodeType::PushScopeGetValue)
        {
            auto result = std::make_shared<XamlQualifiedObject>(
                XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, instance->GetTypeIndex()));
            IFC_RETURN(result->SetDependencyObject(instance));
            m_writer->SetActiveObject(std::move(result));
        }
        else if (m_pendingFirstNode && instance)
        {
            ASSERT(false); // TODO: Non-collection node streams.
        }
        else
        {
            IFC_RETURN(m_writer->WriteNode(node));
        }
        m_pendingFirstNode = false;

        if (streamDepth == 0)
        {
            break;
        }
        else
        {
            if(nextIndexRangeToSkip != indexRangesToSkip.end())
            {
                if(nextIndex >= nextIndexRangeToSkip->GetIndex() && nextIndex <= nextIndexRangeToSkip[1].GetIndex())
                {
                    // Skip this token and move next index to the end of the index-range-to-skip
                    reader->set_NextIndex(nextIndexRangeToSkip[1].GetIndex());
                    nextIndex = reader->get_NextIndex();
                    nextIndexRangeToSkip += 2;
                }
            }

            // Feeding the node into the ObjectWriter might cause our reader's position to change (e.g. if there's a template defined in
            // a different part of our XBF stream) since an XBF stream only gets the one reader.
            // Therefore, we need to fix up the reader's current position before performing our next read.
            reader->set_NextIndex(nextIndex);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CustomWriterRuntimeObjectCreator::LookupStaticResourceValue(
    _In_ StreamOffsetToken token,
    _Out_ std::shared_ptr<CDependencyObject>* pResourceValue)
{
    return LookupStaticResourceValue(token, nullptr, KnownPropertyIndex::UnknownType_UnknownProperty, pResourceValue);
}

// Reads the node at the offset as SetValueFromStaticResource, then gets
// the resource value without setting it on an instance (e.g. a Setter property).
_Check_return_ HRESULT
CustomWriterRuntimeObjectCreator::LookupStaticResourceValue(
    _In_ StreamOffsetToken token,
    _In_opt_ CStyle* optimizedStyleParent,
    _In_ KnownPropertyIndex stylePropertyIndex,
    _Out_ std::shared_ptr<CDependencyObject>* pResource)
{
    IFC_RETURN(EnsureObjectWriter());

    auto reader = GetReaderAndSetIndex(token.GetIndex());

    ObjectWriterNode node;

    VERIFY(reader->TryRead(node));

    if (node.GetNodeType() == ObjectWriterNodeType::PushScope)
    {
        // The node stream optimizer will condense a StaticResource reference in a collection into:
        //  PushScope
        //      ProvideStaticResourceValue
        //  If we see PushScope, skip it, so the next node is ProvideStaticResourceValue

        VERIFY(reader->TryRead(node));
    }

    // Instead of the normal processing of SetValueFromStaticResource, which not only gets
    // the resource value, but also sets it on a property, we just get the value.
    ASSERT(node.GetNodeType() == ObjectWriterNodeType::SetValueFromStaticResource ||
    node.GetNodeType() == ObjectWriterNodeType::ProvideStaticResourceValue);
    IFC_RETURN(m_writer->ProvideStaticResourceReference(node, optimizedStyleParent, stylePropertyIndex));

    // Get instance
    std::shared_ptr<XamlQualifiedObject> result = m_writer->get_Result();
    ASSERT(result);

    // Return DependencyObject with ownership back to the caller.
    *pResource = result->GetAndTransferDependencyObjectOwnership();
    return S_OK;
}

_Check_return_ HRESULT
CustomWriterRuntimeObjectCreator::CreateThemeResourceInstance(_In_ StreamOffsetToken token,
    _Out_ std::shared_ptr<XamlQualifiedObject>* pResource)
{
    return CreateThemeResourceInstance(token, nullptr, KnownPropertyIndex::UnknownType_UnknownProperty, pResource);
}

// Reads the nodes starting at the offset until reaching SetValueFromMarkupExtension, then
// gets the value without setting it on anything (e.g. a Setter property).
_Check_return_ HRESULT
CustomWriterRuntimeObjectCreator::CreateThemeResourceInstance(
    _In_ StreamOffsetToken token,
    _In_opt_ CStyle* optimizedStyleParent,
    _In_ KnownPropertyIndex stylePropertyIndex,
    _Out_ std::shared_ptr<XamlQualifiedObject>* pResource)
{
    IFC_RETURN(EnsureObjectWriter());

    auto reader = GetReaderAndSetIndex(token.GetIndex());

    ObjectWriterNode node;
    int streamDepth = 0;
    while (true)
    {
        VERIFY(reader->TryRead(node));

        if (node.RequiresNewScope())
            streamDepth++;
        else if (node.RequiresScopeToEnd())
            streamDepth--;

        if (streamDepth < 0 || (node.GetNodeType() == ObjectWriterNodeType::SetValueFromMarkupExtension) || (node.GetNodeType() == ObjectWriterNodeType::SetValueFromThemeResource))
            break;

        IFC_RETURN(m_writer->WriteNode(node));
    }

    // Instead of the normal processing of SetValueFromMarkupExtension/SetValueFromThemeResource, which not only gets
    // the value, but also sets it on a property, we just get the value.
    ASSERT(node.GetNodeType() == ObjectWriterNodeType::SetValueFromMarkupExtension || node.GetNodeType() == ObjectWriterNodeType::SetValueFromThemeResource);
    if (node.GetNodeType() == ObjectWriterNodeType::SetValueFromMarkupExtension)
    {
        IFC_RETURN(m_writer->ProvideValueForMarkupExtension(node));
    }
    else
    {
        IFC_RETURN(m_writer->ProvideThemeResourceValue(node, optimizedStyleParent, stylePropertyIndex));
    }

    // Get instance
    *pResource = m_writer->get_Result();
    ASSERT(*pResource);

    return S_OK;
}

// Get the reader and set it to the index given by the caller.
// Save the current reader index so that the caller can reset the reader's state when finished.
std::shared_ptr<XamlBinaryFormatSubReader2> CustomWriterRuntimeObjectCreator::GetReaderAndSetIndex(unsigned int nodeIndex)
{
    auto reader = m_context->GetReader();

    m_restoreIndex = reader->get_NextIndex();

    reader->set_NextIndex(nodeIndex);

    return reader;
}

// Reset the reader to its original index.
void CustomWriterRuntimeObjectCreator::RestoreReaderIndex()
{
    m_context->GetReader()->set_NextIndex(m_restoreIndex);
}

_Check_return_ HRESULT CustomWriterRuntimeObjectCreator::EnsureObjectWriter()
{
    if (!m_writer)
    {
        auto savedContext = BuildSavedContext();
        std::shared_ptr<BinaryFormatObjectWriter> writer;

        ObjectWriterSettings settings;
        IFC_RETURN(BuildObjectWriterSettings(&settings));
        IFC_RETURN(BinaryFormatObjectWriter::Create(
            savedContext,
            settings,
            writer));
        m_writer = std::move(writer);
    }

    return S_OK;
}

std::shared_ptr<XamlSavedContext> CustomWriterRuntimeObjectCreator::BuildSavedContext()
{
    return std::make_shared<XamlSavedContext>(
        m_context->GetSchemaContext(),
        m_context->GetObjectWriterStack(),
        m_context->GetBaseUri(),
        xref_ptr<IPALUri>(), // DEPRECATED
        m_context->GetXbfHash());
}

_Check_return_ HRESULT
CustomWriterRuntimeObjectCreator::XamlQOFromCDOHelper(
    _In_ CDependencyObject* cdo,
    _Out_ std::shared_ptr<XamlQualifiedObject>* pQO)
{
    auto qoValue = std::make_shared<XamlQualifiedObject>();
    CValue cValue;

    cValue.SetObjectAddRef(cdo);

    IFC_RETURN(qoValue->SetValue(cValue));

    // Because this QO is only a temporary object for the purposes of taking a weak-ref and
    // handing it to the various parser APIs (which expect a XamlQualifiedObject), we don't
    // want the managed peer to get unpegged when the QO is destroyed.
    qoValue->ClearHasPeggedManagedPeer();

    *pQO = qoValue;
    return S_OK;
}

_Check_return_ HRESULT CustomWriterRuntimeObjectCreator::BuildObjectWriterSettings(_Out_ ObjectWriterSettings* pResult)
{
    ObjectWriterSettings objectWriterSettings;

    objectWriterSettings.set_BaseUri(m_context->GetBaseUri());
    objectWriterSettings.set_ObjectWriterCallbacks(m_context->GetCallbackInterface());

    if (m_mode == NameScopeRegistrationMode::RegisterEntries)
    {
        objectWriterSettings.set_NameScope(m_context->GetNameScope());
    }
    else
    {
        objectWriterSettings.set_NameScope(make_xref<NullNameScopeHelper>());
    }

    std::shared_ptr<XamlQualifiedObject> qo;
    IFC_RETURN(XamlQOFromCDOHelper(m_context->GetRootInstance().get(), &qo));
    objectWriterSettings.set_EventRoot(qo);

    objectWriterSettings.set_XBindConnector(m_context->GetXBindConnector());

    *pResult = objectWriterSettings;
    return S_OK;
}

_Check_return_ HRESULT CustomWriterRuntimeObjectCreator::RegisterTemplateNameScopeEntriesIfNeeded(_In_ CDependencyObject* instance)
{
    // Corner case: Children of this element are sadly NOT registered. Doing
    // that would require a bigger restructure of the Enter walk to
    // not be so terrifyingly rigid and hard to work with. Once the enter walk isn't
    // too terrible to work with we can simple call Enter on the root instance with a flag
    // indicating we wish to perform a deferred template namescope registration here.
    if (instance->IsTemplateNamescopeMember() && !instance->m_strName.IsNullOrEmpty())
    {
        ASSERT(m_context->GetNameScope()->GetNameScopeType() == Jupiter::NameScoping::NameScopeType::TemplateNameScope);
        std::shared_ptr<XamlQualifiedObject> qo;
        IFC_RETURN(XamlQOFromCDOHelper(instance, &qo));
        IFC_RETURN(m_context->GetNameScope()->RegisterName(instance->m_strName, qo));
    }

    return S_OK;
}

XamlLineInfo CustomWriterRuntimeObjectCreator::GetLineInfoForToken(_In_ StreamOffsetToken token)
{
    auto reader = GetReaderAndSetIndex(token.GetIndex());
    auto resetReaderGuard = wil::scope_exit([&]()
    {
        // Reset the reader's original position before returning
        RestoreReaderIndex();
    });

    ObjectWriterNode node;

    VERIFY(reader->TryRead(node));

    return node.GetLineInfo();
}