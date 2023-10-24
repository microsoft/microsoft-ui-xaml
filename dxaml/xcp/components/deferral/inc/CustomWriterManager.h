// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlWriter.h>
#include "ICustomWriterCallbacks.h"
#include "ICustomWriterActivator.h"

class ObjectWriterContext;
class CustomWriterRuntimeData;
class ICustomWriter;
class SubObjectWriterResult;
class ObjectWriter;

namespace Parser
{
struct XamlPredicateAndArgs;
}
    

// A manager to be used by ObjectWriter to allow certain types to
// perform custom operations at parse time. This manager is used to
// ensure ObjectWriter never needs specific knowledge of the types it's
// processing. Examples of such types are ResourceDictionary and VisualStateGroupCollection. 
//
// The manager will also handle the operations that are generic to all CustomWriters,
// including providing a service to the CustomWriter that allows it to request
// a StreamOffsetToken while opaquely handling the creation of a sub ObjectWriter,
// storing of key names to preresolve, and capturing the correct context to be
// returned in a future ObjectWriterResult.
class CustomWriterManager
    : public ICustomWriterCallbacks
{
public:
    CustomWriterManager(std::shared_ptr<ObjectWriterContext>& context);
    ~CustomWriterManager();

    _Check_return_ HRESULT Initialize();

    _Check_return_ HRESULT WriteObject(
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ bool bFromMember,
        _Out_ bool& handled);
    
    _Check_return_ HRESULT WriteEndObject(_Out_ bool& handled);
    
    _Check_return_ HRESULT WriteMember(
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _Out_ bool& handled);
    
    _Check_return_ HRESULT WriteEndMember(_Out_ bool& handled);
    
    _Check_return_ HRESULT WriteConditionalScope(
        _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs,
        _Out_ bool& handled);
    _Check_return_ HRESULT WriteEndConditionalScope(_Out_ bool& handled);

    _Check_return_ HRESULT WriteValue(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _Out_ bool& handled);
    
    _Check_return_ HRESULT WriteNamespace(
        _In_ const xstring_ptr& spPrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace,
        _Out_ bool& handled);

    // Returns true if a custom writer is active.
    // This indicates that the ObjectWriter
    // should not perform the standard logical runtime operation.
    bool IsCustomWriterActive() const;

    // When a CustomWriter is finished it likely has created and handled to this manager
    // a CustomRuntimeData object. This interface allows the owning ObjectWriter to retrieve
    // that data and either serialize it to XBFv2 or pass it to the created instance of
    // the object assosicated with the CustomWriter.
    bool IsCustomRuntimeDataAvailable() const;
    std::unique_ptr<CustomWriterRuntimeData> GetAndClearCustomRuntimeData();

    // If while a custom writer was active it deferred creation of subobjects it will
    // create a DeferredWriterResult, which contains all the parser-specific information
    // needed to encode the deferred objects into XBFv2 and construct them later.
    std::unique_ptr<SubObjectWriterResult> GetAndClearSubWriterResult();

    // Returns the number of active custom writers. Used by ObjectWriter::WriteObject() so that
    // it can determine if a new custom writer was activated.
    int GetActiveCustomWriterCount() const;

    void SetLineInfo(const XamlLineInfo& lineInfo);
    XamlLineInfo GetLineInfo() const;

    // ICustomWriterCallbacks
    _Check_return_ HRESULT CreateStreamOffsetToken(_Out_ StreamOffsetToken* pToken) override;
    _Check_return_ HRESULT SetCustomWriterRuntimeData(std::unique_ptr<CustomWriterRuntimeData>  runtimeData) override;
    void SetAllowProcessStartObjectByCustomWriter(bool allow) override;

private:
    // Runs through activators querying if they are interested in starting a custom writer.
    // Currently only one (the first one) wins, so priority is decided by order in m_customWriterActivators vector.
    _Check_return_ HRESULT TryCustomWriterActivation(
        _In_ ActivatorTrigger trigger,
        _Out_ std::shared_ptr<ObjectWriter>* pWriter);

    std::vector<std::unique_ptr<ICustomWriterActivator>> m_customWriterActivators;

    CustomWriterActivatorState m_customWriterActivatorState;
    std::vector<CustomWriterActivatorState::NamespacePair> m_pendingNamespaces;

    std::vector<std::pair<std::unique_ptr<ICustomWriter>, std::shared_ptr<ObjectWriter>>> m_activeCustomWriters;
    std::shared_ptr<ObjectWriter> m_currentActiveNodeStreamCollectingWriter;
    std::unique_ptr<CustomWriterRuntimeData> m_savedRuntimeData;
    std::shared_ptr<ObjectWriter> m_savedNodeStreamCollectingWriter;
    std::unique_ptr<SubObjectWriterResult> m_subWriterResult;
    std::shared_ptr<ObjectWriterContext> m_context;

    XamlLineInfo m_lineInfo;
};
