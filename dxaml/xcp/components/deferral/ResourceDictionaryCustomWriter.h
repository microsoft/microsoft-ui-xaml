// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ICustomWriter.h"
#include <StreamOffsetToken.h>
#include <ICustomWriterActivator.h>

class DirectiveProperty;
class ObjectWriterContext;
class ResourceDictionaryCustomRuntimeData;
struct ICustomWriterCallbacks;

namespace Parser
{
struct XamlPredicateAndArgs;
}

// A CustomWriter for deferring ResourceDictionaries. This writer will examine the nodestream
// and pull out the keys of the resources along with their nodestream locations.
class ResourceDictionaryCustomWriter
    : public ICustomWriter
{
private:
    ResourceDictionaryCustomWriter(
        _In_ ICustomWriterCallbacks* callbacks,
        _In_ std::shared_ptr<XamlSchemaContext> context,
        _In_ std::shared_ptr<ObjectWriterContext> objectWriterContext);

    _Check_return_ HRESULT Initialize();

public:
    static _Check_return_ HRESULT Create(
        _In_ ICustomWriterCallbacks* callbacks,
        _In_ std::shared_ptr<XamlSchemaContext> context,
        _In_ std::shared_ptr<ObjectWriterContext> objectWriterContext,
        _Out_ std::unique_ptr<ICustomWriter>* ppValue);

    // ICustomWriter
    _Check_return_ HRESULT WriteObject(
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ bool bFromMember,
        _Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteEndObject(_Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteMember(
        _In_ const std::shared_ptr<XamlProperty>& inProperty,
        _Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteEndMember(_Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteConditionalScope(_In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs, _Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteEndConditionalScope(_Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteValue(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteNamespace(
        _In_ const xstring_ptr& spPrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace,
        _Out_ bool* pResult) override;

    bool IsCustomWriterFinished() const override;

    bool ShouldAllowNewCustomWriter(_In_ const std::shared_ptr<XamlType>& spXamlType) override;

private:
    // Helper method to convert a XamlQualifiedObject into a string key. Currently,
    // only strings or Types can be used as ResourceDictionary keys, so those are the two
    // kinds of XamlQualifiedObjects that this helper supports.
    xstring_ptr QualifiedObjectToStringKey(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue);

    // The current operation is pushed onto the pendingOperations vector below
    // along with the current depth in the node tree. As we encounter Begin/End
    // pairs we maintain a depth to sense how deep we are in the XAML parse tree.
    // As we encounter certain TypeTokens we change which operation we are in. The
    // current operation is the way we store state between XamlWriter calls.
    enum class Operation {
        // Push/Popped at ObjectBegin/ObjectEnd when a ResourceDictionary is encountered. When the
        // top level one of these is popped, that signals that the CustomWriter is finished
        WritingResourceDictionary,

        // Push/Popped at MemberBegin/MemberEnd, sets the CustomWriter
        // up to start saving away the implicit items
        WritingImplicitItems,

        // Push/Popped at ObjectBegin/ObjectEnd as we enter a resource definition. This signals that we should
        // begin looking for resource key-related properties (x:Key and x:Name).
        WritingResource,

        // Push/Popped at MemberBegin/MemberEnd, sets the CustomWriter
        // up to look for a CString value to store as the current resource's x:Key.
        WritingResourceXKey,

        // Push/Popped at MemberBegin/MemberEnd, sets the CustomWriter
        // up to look for a CString value to store as the current resource's x:Name.
        WritingResourceXName,

        // Push/Popped at MemberBegin/MemberEnd, sets the CustomWriter
        // up to look for a CType value to store as the current resource's TargetType (if appropriate;
        // currently this means only Style and DataTemplate resources).
        WritingResourceImplicitKey,

        // Push/Popped when we don't need to specifically process nodes but still need to
        // remember our context (e.g. we don't want nested x:Keys to clobber the top level x:Key)
        Miscellaneous,

        // Push/Popped at MemberBegin/MemberEnd to indicate that we are not currently actively processing
        // nodes (although we still wish to be given nodes to examine). An example of when we would want to do this
        // is when ResourceDictionary.MergedDictionaries is set; we want ObjectWriter to handle the property set,
        // but we're not yet finished with the overall ResourceDictionary
        Pass,
    };

    std::vector<std::pair<unsigned int, Operation>> m_operationsStack;

    bool m_allowNewCustomWriter = true;

    // The StreamOffsetToken that we will record for the resource that is currently being processed
    StreamOffsetToken m_currentResourceOffset;

    // The number of objects deep we are relative to the
    // first StartObject that was pushed into this writer.
    unsigned int m_stackDepth = 0;

    // The number of resources we have encountered. Track this so we know if we shouldn't
    // defer the ResourceDictionary after all.
    // The reason for this is that our built-in controls' styles exist in single-item 
    // ResourceDictionaries (a legacy of an early optimization, before ResourceDictionary
    // deferral existed) and the argument is that the overhead of deferral is unjustified
    // for that scenario.
    std::size_t m_resourceCount = 0;

    // Bool indicating whether or not the resource has a namespace. If it does, then that means
    // we created a StreamOffsetToken when the namespace node was encountered and thus
    // shouldn't create one when the StartObject node is encountered
    bool m_hasNamespace = false;

    // Property information we need to store about the current resource in order to reason
    // about what its key is
    std::pair<std::shared_ptr<DirectiveProperty>, xstring_ptr> m_currentResourceXKey;
    std::pair<std::shared_ptr<DirectiveProperty>, xstring_ptr> m_currentResourceXName;
    std::pair<std::shared_ptr<XamlProperty>, xstring_ptr> m_currentResourceImplicitKey;

    std::shared_ptr<DirectiveProperty> m_spXKeyProperty;
    std::shared_ptr<DirectiveProperty> m_spXNameProperty;

    ICustomWriterCallbacks* m_customWriterCallbacks;
    std::unique_ptr<ResourceDictionaryCustomRuntimeData> m_runtimeData;
    std::shared_ptr<XamlSchemaContext> m_context;
    std::shared_ptr<ObjectWriterContext> m_objectWriterContext;
};

class ResourceDictionaryCustomWriterActivator
    : public ICustomWriterActivator
{
public:
    ResourceDictionaryCustomWriterActivator(
        _In_ const std::shared_ptr<ObjectWriterContext>& spContext);

    _Check_return_ HRESULT ShouldActivateCustomWriter(
        _In_ const CustomWriterActivatorState& activatorState,
        _Out_ bool* pResult) override;

    _Check_return_ HRESULT CreateNodeStreamCollectingWriter(
        _In_ const CustomWriterActivatorState& activatorState,
        _Out_ std::shared_ptr<ObjectWriter>* pWriter) override;

    _Check_return_ HRESULT CreateCustomWriter(
        _In_ ICustomWriterCallbacks* pCallbacks,
        _In_ const CustomWriterActivatorState& activatorState,
        _Out_ std::unique_ptr<ICustomWriter>* pWriter) override;

private:
    std::shared_ptr<ObjectWriterContext> m_spContext;
};
