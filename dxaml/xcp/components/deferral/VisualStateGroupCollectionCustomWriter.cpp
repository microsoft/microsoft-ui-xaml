// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "VisualStateGroupCollectionCustomWriter.h"
#include "VisualStateGroupCollectionCustomRuntimeData.h"
#include <ICustomWriterCallbacks.h>
#include "VisualTransitionTableOptimizedLookup.h"
#include <XamlProperty.h>
#include <XamlQualifiedObject.h>
#include <XamlSchemaContext.h>
#include <XamlType.h>
#include <CString.h>
#include <ObjectWriter.h>
#include <ObjectWriterContext.h>
#include "MetadataAPI.h"
#include <string>
#include <StringConversions.h>
#include <ObjectWriterErrorService.h>
#include <XamlPredicateHelpers.h>

//#define VSMLOG(...) VSMLOG(__VA_ARGS__)
#define VSMLOG(...)

VisualStateGroupCollectionCustomWriter::VisualStateGroupCollectionCustomWriter(
    ICustomWriterCallbacks* callbacks,
    std::shared_ptr<XamlSchemaContext> context,
    std::shared_ptr<ObjectWriterContext> objectWriterContext)
    : m_customWriterCallbacks(callbacks)
    , m_stackDepth(0)
    , m_context(context)
    , m_objectWriterContext(objectWriterContext)
    , m_nameCapturer(context.get())
{
    m_runtimeData = std::make_unique<VisualStateGroupCollectionCustomRuntimeData>();
    m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualStateGroupCollection));
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::Initialize()
{
    // Save away the entire collection here for use in the "we have to bail out and
    // undefer everything" case.
    IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&m_runtimeData->m_entireCollectionToken));
    VSMLOG(L"[VSGCCW]: Created. Beginning custom writer actions for VSGC.");
    return S_OK;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::Create(
    _In_ ICustomWriterCallbacks* callbacks,
    _In_ std::shared_ptr<XamlSchemaContext> context,
    _In_ std::shared_ptr<ObjectWriterContext> objectWriterContext,
    _Out_ std::unique_ptr<ICustomWriter>* pResult)
{
    auto pWriter = std::unique_ptr<VisualStateGroupCollectionCustomWriter>(new VisualStateGroupCollectionCustomWriter(callbacks, context, objectWriterContext));
    IFC_RETURN(pWriter->Initialize());
    *pResult = std::move(pWriter);
    return S_OK;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::WriteObject(
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ bool bFromMember,
    _Out_ bool* pResult)
{
    switch (m_pendingOperations.back().second)
    {
        case Operation::WritingVisualStateGroupCollection:
            if (spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::VisualStateGroup)))
            {
                VSMLOG(L"[VSGCCW]: Beginning capture of deferred VisualStateGroup.");
                m_runtimeData->m_visualStateGroups.push_back(VisualStateGroupEssence());
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&m_runtimeData->m_visualStateGroups.back().m_deferredSelf));
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualStateGroup));
            }
            break;

        case Operation::WritingVisualStateStoryboard:
            // Anything within a Storyboard is fair game for now. We should consider the
            // case of custom attached properties with changed notifiers, and consider building
            // a generic "detect if creating this object has third-party-observable side-effects"
            // handler.
            if (spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::Storyboard)))
            {
                VSMLOG(L"[VSGCCW]: Beginning capture of deferred Storyboard.");
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&m_runtimeData->m_visualStates.back().m_deferredStoryboardToken));
                m_runtimeData->m_visualStates.back().m_hasStoryboard = true;
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualStateStoryboardContent));
            }
            break;

        case Operation::WritingVisualStateStoryboardContent:
            if (spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::Style)))
            {
                VSMLOG(L"[VSGCCW]: Beginning capture of deferred Style.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingStyle));
            }
            break;

        case Operation::WritingVisualStateGroup:
            if (spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::VisualState)))
            {
                VSMLOG(L"[VSGCCW]: Beginning capture of visual state.");
                m_runtimeData->m_visualStates.push_back(VisualStateEssence());
                m_runtimeData->m_visualStateToGroupMap.push_back(
                    static_cast<unsigned int>(m_runtimeData->m_visualStateGroups.size() - 1));
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualState));
            }
            else if (spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::VisualTransition)))
            {
                VSMLOG(L"[VSGCCW]: Beginning capture of visual transition.");
                m_runtimeData->m_visualTransitions.push_back(VisualTransitionEssence());
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&m_runtimeData->m_visualTransitions.back().m_deferredTransitionToken));
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualTransition));
            }
            break;

        case Operation::WritingVisualStateSetters:
            if (spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::Setter)))
            {
                VSMLOG(L"[VSGCCW]: Beginning capture of deferred visual state setters.");
                StreamOffsetToken token;
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                m_runtimeData->m_visualStates.back().m_deferredPropertySetterTokens.push_back(token);
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualStateSetterContent));
            }
            break;
        case Operation::WritingVisualStateStateTriggers:
            if (spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::StateTriggerCollection)))
            {
                VSMLOG(L"[VSGCCW]: Beginning capture of StateTriggerCollection.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingStateTriggerCollection));

                // Add StateTriggerCollection start offset
                StreamOffsetToken token;
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                m_runtimeData->m_visualStates.back().m_stateTriggerCollectionTokens.push_back(token);
            }
            else if (spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::StaticResource)))
            {
                VSMLOG(L"[VSGCCW]: Beginning capture of StateTriggerCollection StaticResource.");
                StreamOffsetToken token;
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                m_runtimeData->m_visualStates.back().m_staticResourceTriggerTokens.push_back(token);
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingStaticResourceTrigger));
            }
            break;

        case Operation::WritingStateTriggerCollection:
            if (spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::AdaptiveTrigger)))
            {
                VSMLOG(L"[VSGCCW]: Beginning capture of AdaptiveTrigger.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingAdaptiveTrigger));

                // Add AdaptiveTrigger start
                m_runtimeData->m_visualStates.back().m_stateTriggerValues.push_back(std::vector<int>());
            }
            else if (DirectUI::MetadataAPI::IsAssignableFrom<KnownTypeIndex::StateTriggerBase>(spXamlType->get_TypeToken().GetHandle()))
            {
                LOG(L"[VSGCCW]: Beginning capture of StateTriggerBase-derived StateTrigger (ExtensibleTrigger).");

                // Add StateTriggerBase start offset
                StreamOffsetToken token;
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                m_runtimeData->m_visualStates.back().m_extensibleStateTriggerTokens.push_back(token);

                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingStateTrigger));
            }
            else if (spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::StaticResource)))
            {
                VSMLOG(L"[VSGCCW]: Beginning capture of StateTriggerCollection StaticResource.");
                StreamOffsetToken token;
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                m_runtimeData->m_visualStates.back().m_staticResourceTriggerTokens.push_back(token);
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingStaticResourceTrigger));
            }
            break;
    }

    m_stackDepth++;

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::WriteEndObject(_Out_ bool* pResult)
{
    ASSERT(m_stackDepth > 0);
    --m_stackDepth;

    switch (m_pendingOperations.back().second)
    {
        case Operation::WritingVisualTransition:
            m_runtimeData->m_visualTransitionLookup->RecordTransitionGroup(
                static_cast<unsigned int>(m_runtimeData->m_visualStateGroups.size() - 1),
                static_cast<unsigned int>(m_runtimeData->m_visualTransitions.size() - 1),
                m_runtimeData->m_visualTransitions.back());
        case Operation::WritingVisualStateGroupCollection:
        case Operation::WritingVisualStateGroup:
        case Operation::WritingVisualStateStoryboardContent:
        case Operation::WritingVisualState:
        case Operation::WritingVisualStateSetterContent:
        case Operation::WritingStateTriggerCollection:
        case Operation::WritingAdaptiveTrigger:
        case Operation::WritingStaticResourceTrigger:
        case Operation::WritingStyle:
            if (m_pendingOperations.back().first == m_stackDepth)
            {
                m_pendingOperations.pop_back();
                VSMLOG(L"[VSGCCW]: Finishing capture of object.");
            }
            break;
        case Operation::WritingVisualStateStateTriggers:
            {
                if (m_pendingOperations.back().first == m_stackDepth)
                {
                    m_pendingOperations.pop_back();
                    VSMLOG(L"[VSGCCW]: Finishing capture of StateTriggerCollection object.");
                }
                StreamOffsetToken token;
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                m_runtimeData->m_visualStates.back().m_stateTriggerCollectionTokens.push_back(token);
            }
            break;
        case Operation::WritingStateTrigger:
            if (m_pendingOperations.back().first == m_stackDepth)
            {
                m_pendingOperations.pop_back();
                VSMLOG(L"[VSGCCW]: Finishing capture of object.");
            }
            break;
    }

    // If this is the last pending operation after this call returns the CustomWriter
    // will be destroyed.
    if (m_pendingOperations.empty())
    {
        m_runtimeData->m_seenNameDirectives = std::move(m_nameCapturer.GetCapturedNameList());
        m_runtimeData->m_visualTransitionLookup->BuildLookupTable(m_runtimeData->m_visualTransitions);
        IFC_RETURN(m_customWriterCallbacks->SetCustomWriterRuntimeData(
            std::unique_ptr<CustomWriterRuntimeData>(m_runtimeData.release())));
    }

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::WriteMember(
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _Out_ bool* pResult)
{
    IFC_RETURN(m_nameCapturer.WriteMember(spProperty.get()));

    std::shared_ptr<DirectiveProperty> nameDirective;
    IFC_RETURN(m_context->get_X_NameProperty(nameDirective));

    switch (m_pendingOperations.back().second)
    {
        case Operation::WritingVisualStateGroup:
            if (XamlProperty::AreEqual(spProperty, nameDirective) ||
                spProperty->get_PropertyToken().Equals(XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::DependencyObject_Name)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of VisualStateGroup name.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualStateGroupName));
            }
            else if (spProperty->get_PropertyToken().Equals(
                XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualStateGroup_Transitions)))
            {
                VSMLOG(L"[VSGCCW]: VisualStateGroup has dynamicly created timelines.");
                m_runtimeData->m_visualStateGroups.back().m_hasDynamicTimelines = true;
            }
            // The following are all the other tokens we expect to find in the node stream but we don't take explicit
            // action on. If we don't match any of these conditions then we have an unexpected node stream we can't
            // account for.
            else
            {
                MarkAbortIfUnexpectedToken(
                    std::array<KnownPropertyIndex, 1> {{ KnownPropertyIndex::VisualStateGroup_States }},
                    std::array<XamlDirectives, 1> {{ XamlDirectives::xdItems }},
                    spProperty.get());
            }
            break;

        case Operation::WritingVisualState:
            if (XamlProperty::AreEqual(spProperty, nameDirective) ||
                spProperty->get_PropertyToken().Equals(XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::DependencyObject_Name)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of VisualState name.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualStateName));
            }
            else if (spProperty->get_PropertyToken().Equals(
                        XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualState_Storyboard)) ||
                    spProperty->get_PropertyToken().Equals(
                        XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualState___DeferredStoryboard))
                )
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of VisualState deferred Storyboard.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualStateStoryboard));
            }
            else if (spProperty->get_PropertyToken().Equals(
                XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualState_Setters)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of VisualState deferred Setters.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualStateSetters));
            }
            else if (spProperty->get_PropertyToken().Equals(
                        XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualState_StateTriggers))
                )
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of VisualState StateTriggers.");

                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualStateStateTriggers));
            }
            // The following are all the other tokens we expect to find in the node stream but we don't take explicit
            // action on. If we don't match any of these conditions then we have an unexpected node stream we can't
            // account for.
            else
            {
                MarkAbortIfUnexpectedToken(
                    std::array<KnownPropertyIndex, 0> {{ }},
                    std::array<XamlDirectives, 1> {{ XamlDirectives::xdItems }},
                        spProperty.get());
            }
            break;

        case Operation::WritingVisualTransition:
            if (spProperty->get_PropertyToken().Equals(
                XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualTransition_From)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of VisualTransition From name.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualTransitionFromName));
            }
            else if (spProperty->get_PropertyToken().Equals(
                XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualTransition_To)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of VisualTransition To name.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualTransitionToName));
            }
            else if (spProperty->get_PropertyToken().Equals(
                XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualTransition_Storyboard)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of VisualTransition storyboard.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingVisualTransitionStoryboard));
            }
            else
            {
                MarkAbortIfUnexpectedToken(
                    std::array<KnownPropertyIndex, 3> { {
                        KnownPropertyIndex::VisualTransition_Storyboard,
                        KnownPropertyIndex::VisualTransition_GeneratedDuration,
                        KnownPropertyIndex::VisualTransition_GeneratedEasingFunction } },
                    std::array<XamlDirectives, 2> { { XamlDirectives::xdItems, XamlDirectives::xdName } },
                    spProperty.get());
            }
            break;

        case Operation::WritingAdaptiveTrigger:
            if (spProperty->get_PropertyToken().Equals(
                XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::AdaptiveTrigger_MinWindowWidth)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of AdaptiveTrigger MinWindowWidth.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingAdaptiveTriggerMinWindowWidth));

                m_runtimeData->m_visualStates.back().m_stateTriggerValues.back().push_back(static_cast<int>(QualifierFlags::Width));
            }
            else if (spProperty->get_PropertyToken().Equals(
                XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::AdaptiveTrigger_MinWindowHeight)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of AdaptiveTrigger MinWindowHeight.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingAdaptiveTriggerMinWindowHeight));

                m_runtimeData->m_visualStates.back().m_stateTriggerValues.back().push_back(static_cast<int>(QualifierFlags::Height));
            }
            else
            {
                m_runtimeData->m_unexpectedTokensDetected = true;
                IFC_RETURN(WriteUnexpectedTokenDebug(spProperty.get()));
            }
            break;

        case Operation::WritingStateTrigger:
            VSMLOG(L"[VSGCCW]: Beginning member write of StateTrigger member.");
            m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingStateTriggerMember));
            break;

        case Operation::WritingVisualStateStateTriggers:
            if (spProperty->get_PropertyToken().Equals(
                XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualState_StateTriggers)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of VisualState.StateTriggers member.");
                StreamOffsetToken token;
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                m_runtimeData->m_visualStates.back().m_extensibleStateTriggerTokens.push_back(token);
            }
            else if (spProperty->get_PropertyToken().Equals(
                XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::StaticResource_ResourceKey)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of VisualState.StateTrigger StaticResource_ResourceKey member.");
            }
            break;

        case Operation::WritingStaticResourceTrigger:
            if (spProperty->get_PropertyToken().Equals(
                XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::StaticResource_ResourceKey)))
            {
                VSMLOG(L"[VSGCCW]: Beginning member write of unknown VisualState.StateTriggers member.");
                m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingStaticResourceTriggerResourceKey));
            }
            break;

        case Operation::WritingVisualStateSetterContent:
        {
            // Check for invalid use of Setter.Property in a VSM Setter. We only need to examine the property being set
            // if the current depth is one greater than the depth when the VSM Setter object was encountered.
            if (   m_stackDepth == (m_pendingOperations.back().first + 1)
                && spProperty->get_PropertyToken().Equals(
                    XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::Setter_Property)))
            {
                std::unique_ptr<ObjectWriterErrorService> spErrorService(
                    new ObjectWriterErrorService(m_objectWriterContext, true));

                IFC_RETURN(spErrorService->ReportInvalidUseOfSetter_Property(GetLineInfo()));
            }
        }
        break;
    }

    m_stackDepth++;
    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::WriteEndMember(_Out_ bool* pResult)
{
    IFC_RETURN(m_nameCapturer.WriteEndMember());

    ASSERT(m_stackDepth > 0);
    --m_stackDepth;

    switch (m_pendingOperations.back().second)
    {
        case Operation::WritingVisualStateGroupName:
        case Operation::WritingVisualStateName:
        case Operation::WritingVisualStateSetters:
        case Operation::WritingVisualStateStoryboard:
        case Operation::WritingStateTriggerCollection:
        case Operation::WritingVisualTransitionFromName:
        case Operation::WritingVisualTransitionToName:
        case Operation::WritingVisualTransitionStoryboard:
        case Operation::WritingAdaptiveTriggerMinWindowWidth:
        case Operation::WritingAdaptiveTriggerMinWindowHeight:
        case Operation::WritingStaticResourceTriggerResourceKey:
        case Operation::WritingStateTriggerMember:
            if (m_pendingOperations.back().first == m_stackDepth)
            {
                VSMLOG(L"[VSGCCW]: Completed write member operation.");
                m_pendingOperations.pop_back();
            }
            break;
        case Operation::WritingVisualStateStateTriggers:
            {
                if (m_pendingOperations.back().first == m_stackDepth)
                {
                    m_pendingOperations.pop_back();
                    VSMLOG(L"[VSGCCW]: Finishing capture of StateTriggerCollection object.");
                }
                StreamOffsetToken token;
                IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                m_runtimeData->m_visualStates.back().m_stateTriggerCollectionTokens.push_back(token);
            }
            break;
    }

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::WriteConditionalScope(
    _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& /* unused */,
    _Out_ bool* pResult)
{
    // TODO: just bail out for now if we get a ConditionalScope node.
    // The main difficulty is that VSGCW creates a StreamOffsetMarker for the
    // individual components of a VisualState (e.g. the Storyboard and VisualTransitions),
    // but not one for the VisualState itself. Thus, it would be a bit of effort to
    // add logic so that a conditionally declared VisualState will act appropriately
    // (if the associated condition evaluates to false at runtime, then that VisualState
    // effectively needs to not exist).
    // So instead, we'll just fault in the entire VSGC at runtime and let the usual
    // ObjectWriter logic handle the conditional XAML behavior.
    m_runtimeData->m_unexpectedTokensDetected = true;

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::WriteEndConditionalScope(_Out_ bool* pResult)
{
    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::WriteValue(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _Out_ bool* pResult)
{
    IFC_RETURN(m_nameCapturer.WriteValue(spValue.get()));

    auto currentOperation = m_pendingOperations.back().second;
    if (currentOperation == Operation::WritingVisualStateGroupName
        || currentOperation == Operation::WritingVisualStateName
        || currentOperation == Operation::WritingVisualTransitionFromName
        || currentOperation == Operation::WritingVisualTransitionToName
        || currentOperation == Operation::WritingAdaptiveTriggerMinWindowWidth
        || currentOperation == Operation::WritingAdaptiveTriggerMinWindowHeight
        || currentOperation == Operation::WritingStaticResourceTriggerResourceKey
        || currentOperation == Operation::WritingStateTriggerMember
        || currentOperation == Operation::WritingVisualStateStateTriggers)
    {
        xstring_ptr name;

        if (spValue->GetValue().GetType() == ValueType::valueString)
        {
            name = spValue->GetValue().AsString();

        }
        else if (spValue->GetTypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::String)) &&
            spValue->GetDependencyObject() != nullptr)
        {
            name = static_cast<CString*>(spValue->GetDependencyObject())->m_strString;
        }

        if (currentOperation == Operation::WritingVisualStateGroupName)
        {
            m_runtimeData->m_visualStateGroups.back().m_name = name;
        }
        else if (currentOperation == Operation::WritingVisualStateName)
        {
            m_runtimeData->m_visualStates.back().m_name = name;
        }
        else if (currentOperation == Operation::WritingVisualTransitionFromName)
        {
            m_runtimeData->m_visualTransitions.back().m_fromState = name;
        }
        else if (currentOperation == Operation::WritingVisualTransitionToName)
        {
            m_runtimeData->m_visualTransitions.back().m_toState = name;
        }
        else if (currentOperation == Operation::WritingAdaptiveTriggerMinWindowWidth ||
                 currentOperation == Operation::WritingAdaptiveTriggerMinWindowHeight)
        {
            // Try to convert the value into an unsigned integer and store it in the VisualStateEssence.
            // This is a perf optimization to allow creation of qualifiers without creating an associated DO-based
            // AdaptiveTrigger instance at runtime.
            XUINT32 triggerValue = 0;
            UINT32 cString = 0;
            const WCHAR* pString = name.GetBufferAndCount(&cString);
            if (SUCCEEDED(UnsignedFromDecimalString(cString, pString, &cString, &pString, &triggerValue)))
            {
                m_runtimeData->m_visualStates.back().m_stateTriggerValues.back().push_back(triggerValue);
            }
            else
            {
                // Fault-In VSGCollection if MinWindowWidth/Height is set to a non-convertible value such
                // as a markup-extension. This markup-extension will be resolved at runtime.
                //
                // If 'm_unexpectedTokensDetected' is 'true' the standard load path will be used
                // which will result in the creation of VisualStateGroupCollection and all of its children
                // at runtime. By created the VSGC  we ensure that all markup-extensions (including bindings)
                // will be honored.  In the optimized path, the values saved into the VisualStateEssences
                // are used  to lazy-load the triggers, preventing evaluation of markup-extensions.
                // (An unconvertible value is now an expected condition so no additional logging is required.)
                m_runtimeData->m_unexpectedTokensDetected = true;
            }
        }
    }

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::WriteNamespace(
    _In_ const xstring_ptr& spPrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace,
    _Out_ bool* pResult)
{
    VSMLOG(L"[VSGCCW]: WriteNamespace passed in.");
    *pResult = true;
    return S_OK;
}

#pragma endregion

// ICustomWriter
bool VisualStateGroupCollectionCustomWriter::IsCustomWriterFinished() const
{
    return m_pendingOperations.empty();
}

// Called by the CustomerWriterManager to get permission before creating a new custom writer.
bool VisualStateGroupCollectionCustomWriter::ShouldAllowNewCustomWriter(_In_ const std::shared_ptr<XamlType>& spXamlType)
{
    // Return false if the new type is a style or we're already writing a style.
    return (m_pendingOperations.empty() || m_pendingOperations.back().second != Operation::WritingStyle) &&
           (spXamlType == nullptr || !spXamlType->get_TypeToken().Equals(XamlTypeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::Style)));
}

bool VisualStateGroupCollectionCustomWriter::DoesEqual(_In_ XamlProperty* xamlProperty, _In_ KnownPropertyIndex index)
{
    return xamlProperty->get_PropertyToken().Equals(XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative,
        index));
}

bool VisualStateGroupCollectionCustomWriter::DoesEqual(_In_ XamlProperty* xamlProperty, _In_ XamlDirectives directive)
{
    return xamlProperty->get_PropertyToken().GetProviderKind() == XamlTypeInfoProviderKind::tpkParser &&
        xamlProperty->get_PropertyToken().GetDirectiveHandle() == directive;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriter::WriteUnexpectedTokenDebug(_In_ XamlProperty* xamlProperty)
{
    if (xamlProperty->get_PropertyToken().GetProviderKind() == XamlTypeInfoProviderKind::tpkParser)
    {
        // Simply for debuggability.
        VSMLOG(L"[VSGCCW]: WARNING unexpected member detected on VisualState. Setting bail out flag and disabling CustomWriterData. %d",
            xamlProperty->get_PropertyToken().GetDirectiveHandle());
    }
    else
    {
        // Simply for debuggability.
        xstring_ptr propertyName;
        IFC_RETURN(xamlProperty->get_FullName(&propertyName));
        VSMLOG(L"[VSGCCW]: WARNING unexpected member detected on VisualStateGroup. Setting bail out flag and disabling CustomWriterData. %s",
            propertyName.GetBuffer());
    }

    return S_OK;
}

VisualStateGroupCollectionCustomWriterActivator::VisualStateGroupCollectionCustomWriterActivator(
    _In_ const std::shared_ptr<ObjectWriterContext>& spContext)
    : m_spContext(spContext)
{}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriterActivator::ShouldActivateCustomWriter(
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ bool* pResult)
{
#ifdef DBG
    if (activatorState.GetTrigger() == ActivatorTrigger::ObjectStart)
    {
        ASSERT(activatorState.GetObject(nullptr));
    }
#endif

    *pResult = activatorState.GetTrigger() == ActivatorTrigger::ObjectStart &&
        activatorState.GetObject(nullptr)->get_TypeToken().Equals(XamlTypeToken::FromType(DirectUI::MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::VisualStateGroupCollection)));
    return S_OK;
}

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriterActivator::CreateNodeStreamCollectingWriter(
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

_Check_return_ HRESULT VisualStateGroupCollectionCustomWriterActivator::CreateCustomWriter(
    _In_ ICustomWriterCallbacks* pCallbacks,
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ std::unique_ptr<ICustomWriter>* pWriter)
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    IFC_RETURN(VisualStateGroupCollectionCustomWriter::Create(pCallbacks, spSchemaContext, m_spContext, pWriter));
    return S_OK;
}
