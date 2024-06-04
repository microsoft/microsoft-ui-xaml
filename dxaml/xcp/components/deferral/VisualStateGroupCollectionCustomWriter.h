// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ICustomWriter.h"
#include <ICustomWriterActivator.h>
#include <XamlWriter.h>
#include <StreamOffsetToken.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>
#include <QualifierFlags.h>

#include "NameDirectiveCapturingWriter.h"

struct ICustomWriterCallbacks;
class VisualStateGroupCollectionCustomRuntimeData;
class XamlSchemaContext;

namespace Parser
{
struct XamlPredicateAndArgs;
}

// A CustomWriter for deferring VisualStateGroupCollections. This writer will examine the nodestream
// and pull out the names of the VisualStates and VisualStateGroups.
class VisualStateGroupCollectionCustomWriter
    : public ICustomWriter
{
private:
    VisualStateGroupCollectionCustomWriter(
        _In_ ICustomWriterCallbacks* callbacks,
        _In_ std::shared_ptr<XamlSchemaContext> context,
        _In_ std::shared_ptr<ObjectWriterContext> objectWriterContext);

    _Check_return_ HRESULT Initialize();

public:
    static _Check_return_ HRESULT Create(
        _In_ ICustomWriterCallbacks* callbacks,
        _In_ std::shared_ptr<XamlSchemaContext> context,
        _In_ std::shared_ptr<ObjectWriterContext> objectWriterContext,
        _Out_ std::unique_ptr<ICustomWriter>* ppResult);

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

    ICustomWriterCallbacks* m_customWriterCallbacks;
    std::unique_ptr<VisualStateGroupCollectionCustomRuntimeData> m_runtimeData;
    std::shared_ptr<XamlSchemaContext> m_context;
    std::shared_ptr<ObjectWriterContext> m_objectWriterContext;

    Jupiter::Deferral::NameDirectiveCapturingWriter m_nameCapturer;

    template <size_t itemSize, size_t directiveCount>
    void MarkAbortIfUnexpectedToken(_In_ std::array<KnownPropertyIndex, itemSize> expectedProperties, 
        _In_ std::array<XamlDirectives, directiveCount> expectedDirectives, _In_ XamlProperty* xamlProperty);
    static bool DoesEqual(_In_ XamlProperty* xamlProperty, _In_ KnownPropertyIndex index);
    static bool DoesEqual(_In_ XamlProperty* xamlProperty, _In_ XamlDirectives directive);
    static _Check_return_ HRESULT WriteUnexpectedTokenDebug(_In_ XamlProperty* xamlProperty);

    // The current operation is pushed onto the pendingOperations vector below
    // along with the current depth in the node tree. As we encounter Begin/End
    // pairs we maintain a depth to sense how deep we are in the XAML parse tree.
    // As we encounter certain TypeTokens we change which operation we are in. The
    // current operation is the way we store state between XamlWriter calls.
    enum class Operation {
        // Push/Popped at ObjectBeing/ObjectEnd. This is the top-level operation. Once this
        // CustomWriter pops this operation it will signal to the manager that it needs to be
        // destroyed.
        WritingVisualStateGroupCollection,

        // Push/Popped at ObjectBegin/ObjectEnd as we center a VSG. This signals that we should
        // begin looking for VSG-specific properties and nodes.
        WritingVisualStateGroup,

        // Push/Popped at MemberBegin/MemberEnd, sets the CustomWriter
        // up to look for a CString value to store as the current VisualStateGroup's name.
        WritingVisualStateGroupName,

        // Push/Popped at ObjectBegin/ObjectEnd, signals that we're now inside the
        // node stream of a VisualState and that we should start looking for either
        // a Storyboard or a VisualState Name property.
        WritingVisualState,

        // Push/Poppped at MemberBegin/MemberEnd, sets the CustomWriter
        // up to look for a CString value to store as the current VisualState's name.
        WritingVisualStateName,

        // Push/Popped at ObjectBegin/ObjectEnd, signals that we're now inside the
        // node stream of a VisualTransition and that we should start looking for either
        // a VisualTransition Name property.
        WritingVisualTransition,

        // Push/Poppped at MemberBegin/MemberEnd, sets the CustomWriter
        // up to look for a CString value to store as the current VisualTransition's from name.
        WritingVisualTransitionFromName,

        // Push/Poppped at MemberBegin/MemberEnd, sets the CustomWriter
        // up to look for a CString value to store as the current VisualTransition's to name.
        WritingVisualTransitionToName,

        // Push/Popped at MemberBegin/MemberEnd, sets the CustomWriter
        // up to ignore the tokens assosicated with the storyboard.
        WritingVisualTransitionStoryboard,

        // Push/Popped at MemberBegin/MemberEnd, indicates that the CustomWriter is currently
        // deferring the nodestream to collect the storyboard and will wait until it sees the 
        // MemberEnd tag at the correct depth and then leave this operation.
        WritingVisualStateStoryboard,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of the storyboard
        // content, a single level deeper than the WritingVisualStateStoryboard operation.
        WritingVisualStateStoryboardContent,

        // Push/Popped at MemberBegin/MemberEnd, indicates that the CustomWriter is currently
        // deferring the nodestream to collect the setters collection and will wait until it sees the 
        // MemberEnd tag at the correct depth and then leave this operation.
        WritingVisualStateSetters,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of the content
        // of an individual setter, a single level deeper than the WritingVisualStateSetters operation.
        WritingVisualStateSetterContent,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of the 
        // VisualState StateTriggers.
        WritingVisualStateStateTriggers,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of the 
        // VisualState.StateTriggers. 
        WritingStateTriggerCollection,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of a 
        // VisualState.StateTriggers StateTriggerBase-derived class. 
        WritingStateTrigger,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of the 
        // VisualState.StateTriggers StateTriggerBased-derived class instance
        // (extensible trigger) member. 
        WritingStateTriggerMember,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of the 
        // VisualState.StateTriggers AdaptiveTrigger. 
        WritingAdaptiveTrigger,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of the 
        // VisualState.StateTriggers AdaptiveTrigger MinWindowWidth property. 
        WritingAdaptiveTriggerMinWindowWidth,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of the 
        // VisualState.StateTriggers AdaptiveTrigger MinWindowHeight property. 
        WritingAdaptiveTriggerMinWindowHeight,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of the 
        // VisualState.StateTriggers AdaptiveTrigger MinWindowHeight property. 
        WritingStaticResourceTrigger,

        // Pushed/Popped at ObjectBegin/ObjectEnd, is the actual deferral of the 
        // VisualState.StateTriggers AdaptiveTrigger MinWindowHeight property. 
        WritingStaticResourceTriggerResourceKey,

        // Pushed/Popped at ObjectBegin/ObjectEnd, sets the CustomWriter
        // up to ignore everything in a Style, and let the StyleCustomWriter
        // handle it.
        WritingStyle
    };

    std::vector<std::pair<unsigned int, Operation>> m_pendingOperations;

    // The number of objects deep we are relevative to the
    // first StartObject that was pushed into this writer.
    unsigned int m_stackDepth;
};

template <size_t itemSize, size_t directiveCount>
void VisualStateGroupCollectionCustomWriter::MarkAbortIfUnexpectedToken(
    _In_ std::array<KnownPropertyIndex, itemSize> properties,
    _In_ std::array<XamlDirectives, directiveCount> directives, XamlProperty* xamlProperty)
{
    if (std::find_if(properties.begin(), properties.end(), [&] (KnownPropertyIndex index) {
        return DoesEqual(xamlProperty, index);
    }) == properties.end() &&
        std::find_if(directives.begin(), directives.end(), [&](XamlDirectives directive) {
        return DoesEqual(xamlProperty, directive);
    }) == directives.end())
    {
        m_runtimeData->m_unexpectedTokensDetected = true;
        WriteUnexpectedTokenDebug(xamlProperty);
    }
}

class VisualStateGroupCollectionCustomWriterActivator
    : public ICustomWriterActivator
{
public:
    VisualStateGroupCollectionCustomWriterActivator(
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
