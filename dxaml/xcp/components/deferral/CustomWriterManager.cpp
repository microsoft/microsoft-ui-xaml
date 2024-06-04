// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ObjectWriterContext.h>
#include "ICustomWriter.h"
#include <CustomWriterManager.h>
#include <CustomWriterRuntimeData.h>
#include "ResourceDictionaryCustomWriter.h"
#include "VisualStateGroupCollectionCustomWriter.h"
#include "StyleCustomWriter.h"
#include "DeferredElementCustomWriter.h"
#include <ObjectWriter.h>
#include <SubObjectWriterResult.h>
#include <DependencyLocator.h>
#include <XamlNode.h>
#include <XamlNodeType.h>
#include <ObjectWriterSettings.h>
#include <SavedContext.h>
#include <XamlQualifiedObject.h>
#include <XamlPredicateHelpers.h>

CustomWriterManager::CustomWriterManager(std::shared_ptr<ObjectWriterContext>& context)
    : m_context(context)
{}

CustomWriterManager::~CustomWriterManager()
{}

HRESULT CustomWriterManager::Initialize()
{
    ASSERT(m_customWriterActivators.empty());

    m_customWriterActivators.emplace_back(new VisualStateGroupCollectionCustomWriterActivator(m_context));
    m_customWriterActivators.emplace_back(new StyleCustomWriterActivator(m_context));
    m_customWriterActivators.emplace_back(new ResourceDictionaryCustomWriterActivator(m_context));
    m_customWriterActivators.emplace_back(new DeferredElementCustomWriterActivator(m_context));

    return S_OK;
}

_Check_return_ HRESULT
CustomWriterManager::TryCustomWriterActivation(
    _In_ ActivatorTrigger trigger,
    _Out_ std::shared_ptr<ObjectWriter>* pWriter)
{
    m_customWriterActivatorState.SetTrigger(trigger);

    for (auto& customWriterActivator : m_customWriterActivators)
    {
        bool shouldActivate = false;
        IFC_RETURN(customWriterActivator->ShouldActivateCustomWriter(m_customWriterActivatorState, &shouldActivate));
        if (shouldActivate)
        {
            std::shared_ptr<ObjectWriter> spNodeCollectingWriter;
            IFC_RETURN(customWriterActivator->CreateNodeStreamCollectingWriter(m_customWriterActivatorState, &spNodeCollectingWriter));

            if (m_customWriterActivatorState.GetTrigger() == ActivatorTrigger::ObjectStart)
            {
                // Need to keep the collecting writer from trying to create a CustomWriter to handle this node, which
                // would result in us getting into an infinite loop. Once it's been fed this node, it's safe to
                // let it create a CustomWriter.
                spNodeCollectingWriter->SetSkipProcessingStartObjectByCustomWriter(true);
            }

            m_currentActiveNodeStreamCollectingWriter = spNodeCollectingWriter;

            std::unique_ptr<ICustomWriter> writer;
            IFC_RETURN(customWriterActivator->CreateCustomWriter(this, m_customWriterActivatorState, &writer));
            m_activeCustomWriters.emplace_back(
                std::move(writer),
                spNodeCollectingWriter);

            *pWriter = spNodeCollectingWriter;
            return S_OK;
        }
    }

    *pWriter = std::shared_ptr<ObjectWriter>();
    return S_OK;
}

HRESULT CustomWriterManager::WriteObject(
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ bool bFromMember,
    _Out_ bool& handled)
{
    handled = false;
    std::shared_ptr<ObjectWriter> spNewlyCreatedCollectingWriter;

    m_customWriterActivatorState.SwapInNamespaces(
        m_pendingNamespaces);

    m_customWriterActivatorState.SetObject(
        spXamlType,
        bFromMember);

    // If the latest active writer allows it, try creating a new writer to handle the incoming object type. 
    if (m_activeCustomWriters.size() == 0 || m_activeCustomWriters.back().first->ShouldAllowNewCustomWriter(spXamlType))
    {
        IFC_RETURN(TryCustomWriterActivation(ActivatorTrigger::ObjectStart, &spNewlyCreatedCollectingWriter));
    }

    for (auto& activeCustomWriter : m_activeCustomWriters)
    {
        m_currentActiveNodeStreamCollectingWriter = activeCustomWriter.second;
        activeCustomWriter.first->SetLineInfo(GetLineInfo());
        bool activeCustomWriterHandled = false;
        IFC_RETURN(activeCustomWriter.first->WriteObject(spXamlType, bFromMember, &activeCustomWriterHandled));

        // To avoid needlessly duplicating nodes in multiple substreams, we only write them to one of the collecting
        // writers. Since we're storing active custom writers on a stack, the most recently created collecting writer
        // gets to write the nodes.
        if (activeCustomWriter == m_activeCustomWriters.back())
        {
            m_currentActiveNodeStreamCollectingWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_currentActiveNodeStreamCollectingWriter->WriteObject(spXamlType, bFromMember));
        }

        handled = handled || activeCustomWriterHandled;
    }

    if (spNewlyCreatedCollectingWriter)
    {
        spNewlyCreatedCollectingWriter->SetSkipProcessingStartObjectByCustomWriter(false);
    }

    return S_OK;
}

HRESULT CustomWriterManager::WriteEndObject(_Out_ bool& handled)
{
    handled = false;

    bool customWriterFinished = false;

    for (auto& activeCustomWriter : m_activeCustomWriters)
    {
        m_currentActiveNodeStreamCollectingWriter = activeCustomWriter.second;
        activeCustomWriter.first->SetLineInfo(GetLineInfo());
        bool activeCustomWriterHandled = false;
        IFC_RETURN(activeCustomWriter.first->WriteEndObject(&activeCustomWriterHandled));

        // To avoid needlessly duplicating nodes in multiple substreams, we only write them to one of the collecting
        // writers. Since we're storing active custom writers on a stack, the most recently created collecting writer
        // gets to write the nodes.
        if (activeCustomWriter == m_activeCustomWriters.back())
        {
            IFC_RETURN(m_currentActiveNodeStreamCollectingWriter->WriteEndObject());
        }

        handled = handled || activeCustomWriterHandled;

        if (activeCustomWriter.first->IsCustomWriterFinished())
        {
            // We expect the finished CustomWriter to be the most recently
            // created CustomWriter (i.e. the final one in the list)
            ASSERT(activeCustomWriter.first == (m_activeCustomWriters.back().first));
            customWriterFinished = true;
        }
    }

    // After writing the end object to each of the active ICustomWriters, the most
    // recently created one may have finished writing. In that case we remove it 
    // from the list of active custom writers
    if (customWriterFinished)
    {
        m_activeCustomWriters.pop_back();
    }

    // Set the SubWriterResult if a CustomWriter provided a CustomRuntimeData
    if (IsCustomRuntimeDataAvailable())
    {
        auto nodeList = m_savedNodeStreamCollectingWriter->GetNodeList();
        m_subWriterResult.reset(new SubObjectWriterResult(nodeList));
        m_savedNodeStreamCollectingWriter.reset();
    }

    m_customWriterActivatorState.reset();

    return S_OK;
}

HRESULT CustomWriterManager::WriteMember(
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _Out_ bool& handled)
{
    handled = false;

    m_customWriterActivatorState.SetProperty(
        spProperty);

    for (auto& activeCustomWriter : m_activeCustomWriters)
    {
        m_currentActiveNodeStreamCollectingWriter = activeCustomWriter.second;
        activeCustomWriter.first->SetLineInfo(GetLineInfo());
        bool activeCustomWriterHandled = false;
        IFC_RETURN(activeCustomWriter.first->WriteMember(spProperty, &activeCustomWriterHandled));

        // To avoid needlessly duplicating nodes in multiple substreams, we only write them to one of the collecting
        // writers. Since we're storing active custom writers on a stack, the most recently created collecting writer
        // gets to write the nodes.
        if (activeCustomWriter == m_activeCustomWriters.back())
        {
            m_currentActiveNodeStreamCollectingWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_currentActiveNodeStreamCollectingWriter->WriteMember(spProperty));
        }

        handled = handled || activeCustomWriterHandled;
    }

    return S_OK;
}

HRESULT CustomWriterManager::WriteEndMember(_Out_ bool& handled)
{
    handled = false;

    for (auto& activeCustomWriter : m_activeCustomWriters)
    {
        m_currentActiveNodeStreamCollectingWriter = activeCustomWriter.second;
        activeCustomWriter.first->SetLineInfo(GetLineInfo());
        bool activeCustomWriterHandled = false;
        IFC_RETURN(activeCustomWriter.first->WriteEndMember(&activeCustomWriterHandled));

        // To avoid needlessly duplicating nodes in multiple substreams, we only write them to one of the collecting
        // writers. Since we're storing active custom writers on a stack, the most recently created collecting writer
        // gets to write the nodes.
        if (activeCustomWriter == m_activeCustomWriters.back())
        {
            IFC_RETURN(m_currentActiveNodeStreamCollectingWriter->WriteEndMember());
        }

        handled = handled || activeCustomWriterHandled;
    }

    // If the latest active writer allows it, try creating a new writer in response to the current node.
    if (m_activeCustomWriters.size() == 0 || m_activeCustomWriters.back().first->ShouldAllowNewCustomWriter(nullptr))
    {
        std::shared_ptr<ObjectWriter> writer;
        IFC_RETURN(TryCustomWriterActivation(ActivatorTrigger::MemberEnd, &writer));
    }

    // Set the SubWriterResult if a CustomWriter provided a CustomRuntimeData
    if (IsCustomRuntimeDataAvailable())
    {
        auto nodeList = m_savedNodeStreamCollectingWriter->GetNodeList();
        m_subWriterResult.reset(new SubObjectWriterResult(nodeList));
        m_savedNodeStreamCollectingWriter.reset();
    }

    m_customWriterActivatorState.ResetProperty();

    return S_OK;
}

HRESULT CustomWriterManager::WriteConditionalScope(
    _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs,
    _Out_ bool& handled)
{
    handled = false;

    for (auto& activeCustomWriter : m_activeCustomWriters)
    {
        m_currentActiveNodeStreamCollectingWriter = activeCustomWriter.second;
        activeCustomWriter.first->SetLineInfo(GetLineInfo());
        bool activeCustomWriterHandled = false;
        IFC_RETURN(activeCustomWriter.first->WriteConditionalScope(xamlPredicateAndArgs, &activeCustomWriterHandled));

        // To avoid needlessly duplicating nodes in multiple substreams, we only write them to one of the collecting
        // writers. Since we're storing active custom writers on a stack, the most recently created collecting writer
        // gets to write the nodes.
        if (activeCustomWriter == m_activeCustomWriters.back())
        {
            m_currentActiveNodeStreamCollectingWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_currentActiveNodeStreamCollectingWriter->WriteConditionalScope(xamlPredicateAndArgs));
        }

        handled = handled || activeCustomWriterHandled;
    }

    return S_OK;
}

HRESULT CustomWriterManager::WriteEndConditionalScope(_Out_ bool& handled)
{
    handled = false;

    for (auto& activeCustomWriter : m_activeCustomWriters)
    {
        m_currentActiveNodeStreamCollectingWriter = activeCustomWriter.second;
        activeCustomWriter.first->SetLineInfo(GetLineInfo());
        bool activeCustomWriterHandled = false;
        IFC_RETURN(activeCustomWriter.first->WriteEndConditionalScope(&activeCustomWriterHandled));

        // To avoid needlessly duplicating nodes in multiple substreams, we only write them to one of the collecting
        // writers. Since we're storing active custom writers on a stack, the most recently created collecting writer
        // gets to write the nodes.
        if (activeCustomWriter == m_activeCustomWriters.back())
        {
            m_currentActiveNodeStreamCollectingWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_currentActiveNodeStreamCollectingWriter->WriteEndConditionalScope());
        }

        handled = handled || activeCustomWriterHandled;
    }

    return S_OK;
}

HRESULT CustomWriterManager::WriteValue(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _Out_ bool& handled)
{
    handled = false;

    m_customWriterActivatorState.SetValue(
         spValue);

    for (auto& activeCustomWriter : m_activeCustomWriters)
    {
        m_currentActiveNodeStreamCollectingWriter = activeCustomWriter.second;
        activeCustomWriter.first->SetLineInfo(GetLineInfo());
        bool activeCustomWriterHandled = false;
        IFC_RETURN(activeCustomWriter.first->WriteValue(spValue, &activeCustomWriterHandled));

        // To avoid needlessly duplicating nodes in multiple substreams, we only write them to one of the collecting
        // writers. Since we're storing active custom writers on a stack, the most recently created collecting writer
        // gets to write the nodes.
        if (activeCustomWriter == m_activeCustomWriters.back())
        {
            m_currentActiveNodeStreamCollectingWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_currentActiveNodeStreamCollectingWriter->WriteValue(spValue));
        }

        handled = handled || activeCustomWriterHandled;
    }

    return S_OK;
}

HRESULT CustomWriterManager::WriteNamespace(
    _In_ const xstring_ptr& spPrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace,
    _Out_ bool& handled)
{
    handled = false;

    // Cache namespaces we've seen, so thay are part of state info passed to custom writer activator.
    m_pendingNamespaces.emplace_back(
        spPrefix,
        spXamlNamespace);

    for (auto& activeCustomWriter : m_activeCustomWriters)
    {
        m_currentActiveNodeStreamCollectingWriter = activeCustomWriter.second;
        activeCustomWriter.first->SetLineInfo(GetLineInfo());
        bool activeCustomWriterHandled = false;
        IFC_RETURN(activeCustomWriter.first->WriteNamespace(spPrefix, spXamlNamespace, &activeCustomWriterHandled));

        // To avoid needlessly duplicating nodes in multiple substreams, we only write them to one of the collecting
        // writers. Since we're storing active custom writers on a stack, the most recently created collecting writer
        // gets to write the nodes.
        if (activeCustomWriter == m_activeCustomWriters.back())
        {
            m_currentActiveNodeStreamCollectingWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_currentActiveNodeStreamCollectingWriter->WriteNamespace(spPrefix, spXamlNamespace));
        }

        handled = handled || activeCustomWriterHandled;
    }

    return S_OK;
}

#pragma region ICustomWriterCallbacks
_Check_return_ HRESULT CustomWriterManager::CreateStreamOffsetToken(_Out_ StreamOffsetToken* pToken)
{
    ASSERT(m_currentActiveNodeStreamCollectingWriter);
    IFC_RETURN(m_currentActiveNodeStreamCollectingWriter->GetStreamOffsetToken(pToken));
    return S_OK;
}

_Check_return_ HRESULT CustomWriterManager::SetCustomWriterRuntimeData(std::unique_ptr<CustomWriterRuntimeData> runtimeData)
{
    // The parent ObjectWriter should've already gotten and cleared the previous CustomWriterRuntimeData
    ASSERT(!IsCustomRuntimeDataAvailable());

    m_savedNodeStreamCollectingWriter = m_currentActiveNodeStreamCollectingWriter;
    m_savedRuntimeData = std::move(runtimeData);

    return S_OK;
}

void CustomWriterManager::SetAllowProcessStartObjectByCustomWriter(bool allow)
{
    ASSERT(m_currentActiveNodeStreamCollectingWriter);
    m_currentActiveNodeStreamCollectingWriter->SetSkipProcessingStartObjectByCustomWriter(!allow);
}

#pragma endregion

bool CustomWriterManager::IsCustomWriterActive() const
{
    return m_activeCustomWriters.size() > 0;
}

bool CustomWriterManager::IsCustomRuntimeDataAvailable() const
{
    return !!m_savedRuntimeData;
}

std::unique_ptr<CustomWriterRuntimeData> CustomWriterManager::GetAndClearCustomRuntimeData()
{
    return std::move(m_savedRuntimeData);
}

std::unique_ptr<SubObjectWriterResult> CustomWriterManager::GetAndClearSubWriterResult()
{
    return std::move(m_subWriterResult);
}

int CustomWriterManager::GetActiveCustomWriterCount() const
{
    return static_cast<int>(m_activeCustomWriters.size());
}

void CustomWriterManager::SetLineInfo(const XamlLineInfo& lineInfo)
{
    m_lineInfo = lineInfo;
}

XamlLineInfo CustomWriterManager::GetLineInfo() const
{
    return m_lineInfo;
}
