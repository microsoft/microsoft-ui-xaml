// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ObjectWriter.h>
#include <CustomWriterManager.h>
#include <ICustomWriterRuntimeDataReceiver.h>
#include <CustomWriterRuntimeData.h>
#include <SubObjectWriterResult.h>
#include <StreamOffsetToken.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include "XamlPredicateHelpers.h"
#include "XamlPredicateService.h"
#include <ObjectWriterContext.h>
#include "ObjectWriterNode.h"
#include "ObjectWriterRuntime.h"
#include "ObjectWriterRuntimeEncoder.h"
#include "ObjectWriterErrorService.h"
#include "ObjectWriterRuntimeFactory.h"
#include <CollectionInitializationStringParser.h>
#include <xstring_ptr.h>

#include <functional>

using namespace DirectUI;
using namespace RuntimeFeatureBehavior;
using namespace Parser;

// NOTE: there are several validation checks that are disabled if the ObjectWriter
// is in encoding mode and conditional XAML is present, because we simply don't have
// the ability/time to do the static analysis to determine whether or not some combination
// of conditional XAML is going to cause problems at runtime on some hypothetical machine.
// As such, for XAML that is within conditional scopes, that validation is deferred to runtime.
// If somebody complains that the tools no longer validate as well as they used to, well,
// that's By Design(tm).


// The following alter the stack/scope
//
// WriteObject          -   Pushes Scope (when current type exists)
// WriteValue           -   Pushes Scope (always)
//
// WriteEndObject       -   Pops Scope (always)
// WriteEndMember       -   Pops Scope (text node)
// WriteValue           -   Pops Scope (sometimes implicit)
// Logic_CreateFromInitializationValue      - Pops Scope (for text node)
_Check_return_ HRESULT ObjectWriter::Create(
    _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
    _In_ const std::shared_ptr<XamlSavedContext>& spSavedContext,
    _In_ const ObjectWriterSettings& settings,
    _Out_ std::shared_ptr<ObjectWriter>& rspObjectWriter)
{
    auto spObjectWriter = std::make_shared<ObjectWriter>();
    spObjectWriter->m_bIsEncoding = settings.get_EnableEncoding();
    spObjectWriter->m_spObjectWriterCallbacks = settings.get_ObjectWriterCallbacks();

    // Reserve some space for declared namespaces
    // Minimum in a VS blank page template is 5, so double that seems reasonable for
    // a real world app
    spObjectWriter->m_namespacePrefixQueue.reserve(10);

    if (!spObjectWriter->m_bIsEncoding)
    {
        spObjectWriter->m_qoRootObjectInstance = settings.get_RootObjectInstance();
        spObjectWriter->m_qoEventRoot = settings.get_EventRoot();
    }
    spObjectWriter->m_bCheckDuplicateProperty = settings.get_CheckDuplicateProperty();
    spObjectWriter->m_allowCustomWriter = settings.get_AllowCustomWriter();

    // This path will be run if there was no existing
    // object writer context provided for the object writer
    // which means this is an clean object writing setup
    if (spSchemaContext)
    {
        IFC_RETURN(ObjectWriterContext::Create(
            spSchemaContext,
            spObjectWriter->m_qoEventRoot,
            settings.get_ExpandTemplates(),
            settings.get_XBindParentConnector(),
            spObjectWriter->m_spContext));

        // TODO: Move these into interfaces on ObjectWriter like rest of settings?
        spObjectWriter->m_spContext->set_BaseUri(settings.get_BaseUri());
        spObjectWriter->m_spContext->set_XamlResourceUri(settings.get_XamlResourceUri());
        spObjectWriter->m_spContext->set_RootNamescope(settings.get_NameScope());
    }
    // We have a saved context provided which should
    // be used as the context for the object writer
    // for initializing its state
    else if (spSavedContext)
    {
        IFC_RETURN(ObjectWriterContext::Create(spSavedContext, spObjectWriter->m_qoEventRoot, settings.get_ExpandTemplates(), settings.get_XBindParentConnector(), spObjectWriter->m_spContext));

        spObjectWriter->m_spContext->set_BaseUri(settings.get_BaseUri());
        spObjectWriter->m_spContext->set_RootNamescope(settings.get_NameScope());
    }
    // Unexpected that we did not get an incoming schema context
    // nor a saved context.
    else
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    // initialize the deferring writers
    spObjectWriter->m_spDeferringWriter = std::make_shared<DeferringWriter>(spObjectWriter->m_spContext, settings.get_EnableEncoding());
    if (spObjectWriter->m_bIsEncoding)
    {
        spObjectWriter->DisableResourceDictionaryDefer();
    }

    spObjectWriter->m_spCustomWriterManager = std::make_shared<CustomWriterManager>(spObjectWriter->m_spContext);
    IFC_RETURN(spObjectWriter->m_spCustomWriterManager->Initialize());

    spObjectWriter->m_spErrorService = std::make_shared<ObjectWriterErrorService>(spObjectWriter->m_spContext, spObjectWriter->m_bIsEncoding);

    IFC_RETURN(Parser::CreateObjectWriterRuntime(spObjectWriter->m_spContext, spObjectWriter->m_spErrorService, spObjectWriter->m_bIsEncoding, spObjectWriter->m_spRuntime));

    rspObjectWriter = std::move(spObjectWriter);

    return S_OK;
}

bool ObjectWriter::IsCustomWriterActive()
{
    return m_spCustomWriterManager->IsCustomWriterActive();
}

void ObjectWriter::SetActiveObject(_In_ std::shared_ptr<XamlQualifiedObject> object)
{
    m_spContext->PushScope();
    m_spContext->Current().set_Instance(object);
    m_spContext->Current().set_Collection(object);
    m_spContext->Current().set_IsObjectFromMember(TRUE);
    m_spContext->Current().set_Type(std::shared_ptr<XamlType>());
}

_Check_return_ HRESULT
ObjectWriter::GetStreamOffsetToken(_Out_ StreamOffsetToken* pToken)
{
    UINT32 tokenIndex = 0;
    IFC_RETURN(m_spRuntime->GetStreamOffsetToken(&tokenIndex));
    *pToken = StreamOffsetToken(tokenIndex);
    return S_OK;
}

// Determines whether processing should be deferred or skipped and if
// not, passes processing on to WriteObjectCore
_Check_return_ HRESULT ObjectWriter::WriteObject(
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ bool fromMember)
{
    bool actionDeferred = false;

    if (IsSkippingForConditionalScope())
    {
        return S_OK;
    }

    if (!ShouldSkipForResourceReplacement())
    {
        if (!(m_bIsEncoding && m_spContext->IsInConditionalScope(false)))
        {
            // report error if we have an unresolved type
            // Don't do validation if we are encoding and there is a XamlPredicate,
            // since it will be conditionally resolved at run time
            IFC_RETURN(m_spErrorService->ValidateIsKnown(spXamlType, GetLineInfo()));
        }

        int previousActiveCustomWriterCount = m_spCustomWriterManager->GetActiveCustomWriterCount();

        m_spDeferringWriter->SetLineInfo(GetLineInfo());
        m_spCustomWriterManager->SetLineInfo(GetLineInfo());

        auto deferredWriterFunc = [&]
        {
            return m_spDeferringWriter->WriteObject(spXamlType, fromMember);
        };

        auto customWriterFunc = [&](bool& handled)
        {
            if (m_skipCustomWriterProcessingOfStartObject)
            {
                handled = false;
                return S_OK;
            }
            else
            {
                return m_spCustomWriterManager->WriteObject(spXamlType, fromMember, handled);
            }
        };

        IFC_RETURN(ForwardActionToChildrenWriters(
            std::ref(deferredWriterFunc),
            std::ref(customWriterFunc),
            actionDeferred));

        // Unlike with the other WriteXXX operations, we need to adjust the value of actionDeferred
        // based on whether or not a new CustomWriter was created to handle this node. This is because this
        // ObjectWriter still needs to write out the initial WriteObject node, even if a custom writer was
        // created to handle it.
        // Since actionDeferred hides the component values (deferring writer status and CustomWriter
        // status), we need to infer the original component values from it, specifically whether or not
        // a new CustomWriter was created to handle the WriteObject, in order to derive the correct
        // adjusted value of actionDeferred.
        bool adjustedActionDeferred = false;
        if (!!m_spDeferringWriter->get_Handled())
        {
            // If the deferring writer handled the node, then ForwardActionToChildrenWriters will always
            // set actionDeferred to true.
            adjustedActionDeferred = true;
        }
        else
        {
            // If the deferring writer didn't handle the node, then ForwardActionToChildrenWriters will only set
            // actionDeferred to true if both the CustomWriter was active and it handled the node. We thus
            // need to also check if a CustomWriter was created to handle this node.
            int currentActiveCustomWriterCount = m_spCustomWriterManager->GetActiveCustomWriterCount();
            adjustedActionDeferred = actionDeferred && (previousActiveCustomWriterCount == currentActiveCustomWriterCount);
        }

        actionDeferred = adjustedActionDeferred;
    }

    if (!actionDeferred)
    {
        UpdateSkipDepthForResourceReplacement(XamlNodeType::xntStartObject);
        if (!ShouldSkipForResourceReplacement())
        {
            IFC_RETURN(WriteObjectCore(spXamlType, fromMember));
        }
    }
    else
    {
        // If the WriteObject was handled by a child writer, then this ObjectWriter
        // will never get a chance to clear the namespace prefix queue, so empty it now
        m_namespacePrefixQueue.clear();
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::WriteObjectCore(
    const std::shared_ptr<XamlType>& spXamlType,
    bool fromMember)
{
    //The first node (T, SO, or EP) after an EndObject should NULL out m_qoLastInstance.
    m_qoLastInstance.reset();

    // If the current scope already has a value for Type, then that means this isn't the root object
    // and we need to push a new scope
    if (m_spContext->Current().exists_Type())
    {
        IFC_RETURN(m_spRuntime->PushScope(GetLineInfo()));
    }
    else
    {
        // If there is no root then we need to cache the line info because we are currently writting the root object.
        // The creation will be deferred until we write a member so we lose the context otherwise.
        m_rootLineInfo = GetLineInfo();
    }

    // Now process the queued namespace prefixes
    for (const auto& queuedNamespace : m_namespacePrefixQueue)
    {
        IFC_RETURN(m_spRuntime->AddNamespacePrefix(
            std::get<0>(queuedNamespace) /* line info */,
            std::get<1>(queuedNamespace) /* prefix */,
            std::get<2>(queuedNamespace) /* XamlNamespace */));
    }
    m_namespacePrefixQueue.clear();

    m_spContext->Current().set_Type(spXamlType);

    // Don't create the Root Instance if we were given one in the settings.
    // This is an important scenario when a XamlObject loads a XamlDefinition of itself
    // in it's constructor.  The instance is already created (that is how we got into
    // the constructor), now don't create the first StartObject use the existing instance.
    if ((m_spContext->get_LiveDepth() == 1) && m_qoRootObjectInstance)
    {
        IFC_RETURN(Logic_SetupRootInstance(m_qoRootObjectInstance));
    }

    if (fromMember)
    {
        std::shared_ptr<XamlQualifiedObject> qoInst;
        bool bIsCollection = false;
        bool bIsDictionary = false;

        m_spContext->Current().set_IsObjectFromMember(TRUE);

        IFC_RETURN(m_spRuntime->GetValue(
            GetLineInfo(),
            m_spContext->Parent().get_Instance(),
            m_spContext->Parent().get_Member(),
            qoInst));
        m_spContext->Current().set_Instance(qoInst);

        IFC_RETURN(spXamlType->IsCollection(bIsCollection));
        IFC_RETURN(spXamlType->IsDictionary(bIsDictionary));
        if (bIsCollection || bIsDictionary)
        {
            m_spContext->Current().set_Collection(qoInst);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::Logic_SetupRootInstance(
    _In_ std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance
    )
{
    CDependencyObject* pDO = spRootObjectInstance->GetDependencyObject();

    // ********************************************
    // TODO: This is only a partial and temporary fix for this case.
    // The general problem is detecting if the passed in object is a collection or
    // dictionary. This only fixed the resource dictionary the root case.
    if (pDO && pDO->OfTypeByIndex<KnownTypeIndex::ResourceDictionary>())
    {
        if (pDO->GetTypeIndex() == KnownTypeIndex::ColorPaletteResources)
        {
            spRootObjectInstance->SetTypeToken(XamlTypeToken(tpkNative, KnownTypeIndex::ColorPaletteResources));
        }
        else
        {
            spRootObjectInstance->SetTypeToken(XamlTypeToken(tpkNative, KnownTypeIndex::ResourceDictionary));
        }
    }
    // ********************************************

    // We just let it be set, but we will verify later when
    // we see some properties, because we need to see x:Class before
    // we can be sure this is legit.
    m_spContext->Current().set_Instance(spRootObjectInstance);
    m_spContext->Current().set_Collection(spRootObjectInstance);

    IFC_RETURN(m_spRuntime->BeginInit(
        GetLineInfo(),
        m_spContext->Current().get_Type(),
        m_spContext->Current().get_Instance()));

    return S_OK;
}

// Determines whether processing should be deferred or skipped and if
// not, passes processing on to WriteEndObjectCore
_Check_return_ HRESULT ObjectWriter::WriteEndObject()
{
    if (IsSkippingForConditionalScope())
    {
        return S_OK;
    }

    bool actionDeferred = false;

    if (!ShouldSkipForResourceReplacement())
    {
        auto deferredWriterFunc = [&]
        {
            return m_spDeferringWriter->WriteEndObject();
        };

        auto customWriterFunc = [&](bool& handled)
        {
            return m_spCustomWriterManager->WriteEndObject(handled);
        };

        IFC_RETURN(ForwardActionToChildrenWriters(
            std::ref(deferredWriterFunc),
            std::ref(customWriterFunc),
            actionDeferred));
    }

    if (!actionDeferred)
    {
        UpdateSkipDepthForResourceReplacement(XamlNodeType::xntEndObject);
        if (!ShouldSkipForResourceReplacement())
        {
            IFC_RETURN(ApplyRemainingResourceProperties());
            IFC_RETURN(WriteEndObjectCore());
        }
    }

    return S_OK;
}


_Check_return_ HRESULT ObjectWriter::WriteEndObjectCore()
{
    // Imagine the following XAML that gets loaded in via ResourceDictionary.Source:
    // <ResourceDictionary xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    //                     xmlns:x = "http://schemas.microsoft.com/winfx/2006/xaml">
    //   <ResourceDictionary.MergedDictionaries>
    //     <ResourceDictionary Source="Dictionary2.xaml" />
    //   </ResourceDictionary.MergedDictionaries>
    //
    //   <SolidColorBrush x:Key="MergedPinkBrush"
    //                    Color="Pink" />
    // </ResourceDictionary>
    //
    // The node stream looks like:
    // WriteObject
    //   WriteMember
    //     etc.
    // WriteEndObject
    //
    // A ResourceDictionaryCustomWriter will be created to handle it. At the time of creation during WriteObject,
    // the ObjectWriterStack will have a single empty frame. Simultaneously, an ObjectWriter (the collecting writer)
    // is created to collect nodes from the RDCW; since it is passed in our current saved content, it will have
    // the same stack (with a single empty frame) and a saved depth of 1.
    // Now the final WriteEndObject node is processed. The collecting writer will be passed the WriteEndObject node,
    // causing this method (WriteEndObjectCore) to be invoked. Because the saved depth is 1 and the stack now has only
    // one frame on it, the live depth is 0, but this is in fact a perfectly legal situation to be in.
    ASSERT(m_spContext->get_LiveDepth() >= 0);

    if (!m_spContext->Current().get_IsObjectFromMember())
    {
        // We defer creation in WriteObject because we might have args for
        // Create From InitText, or Create with parameters.
        // But, If we got to End Object and still haven't Created it,
        // then create it now.
        if (!m_spContext->Current().exists_Instance())
        {
            IFC_RETURN(Logic_CreateWithCtor());
        }

        // Check if it's necessary to set CustomRuntimeData.
        // This needs to happen before EndInit in order to ensure it gets assigned
        // to the correct instance later.
        IFC_RETURN(Logic_CheckAndSetCustomRuntimeData());

        // calling EndInit() before Logic_DoAssignmentToParentProperty()
        // is differend from System.Xaml's implementation.  We need to do this to
        // clear the "ParserParentLock"
        IFC_RETURN(m_spRuntime->EndInit(
            GetLineInfo(),
            m_spContext->Current().get_Type(),
            m_spContext->Current().get_Instance()));

        // FUTURE: We're going to get in trouble in the future if we add new markup
        // extension types to the XAML framework, since conditional XAML allows
        // use of new types that are unknown to the min version of GenXbf and we need to call the
        // ProvideValue logic for markup extensions. For now, though, this is OK
        // because we can resolve all of the possible markup extension types.
        //
        // For custom markup extensions, they can be provided via one of two ways:
        // 1) part of the app package, in which case it never makes sense for them to *not* be resolvable
        // 2) part of a framework package, but those are still a long-term plan with no public
        //    availability, and because of strict version dependence, again a custom markup extension
        //    would never not be resolvable.
        bool isMarkupExtension = false;
        auto currentType = m_spContext->Current().get_Type();
        if (!currentType->IsUnknown())
        {
            IFC_RETURN(currentType->IsMarkupExtension(isMarkupExtension));
        }
        else
        {
            // If the current instance's type is Unknown, then we had better be inside a
            // conditional XAML scope and encoding (since that is the only circumstance
            // under which an unknown type is permitted)
            ASSERT(m_bIsEncoding && m_spContext->IsInConditionalScope(false));
        }

        if (isMarkupExtension)
        {
            if (m_spContext->get_LiveDepth() > 1)
            {
                IFC_RETURN(Logic_AssignProvidedValue());
            }
            else
            {
                IFC_RETURN(Logic_ProvideValue());
            }
        }
        else
        {
            if (m_spContext->get_LiveDepth() > 1)
            {
                IFC_RETURN(Logic_DoAssignmentToParentProperty());
            }
        }
    }
    else
    {
        IFC_RETURN(Logic_CheckAndSetCustomRuntimeData());
    }

    m_qoLastInstance = m_spContext->Current().get_Instance();

    IFC_RETURN(m_spRuntime->PopScope(GetLineInfo()));

    return S_OK;
}

// Determines whether processing should be deferred or skipped and if
// not, passes processing on to WriteMemberCore
_Check_return_ HRESULT
ObjectWriter::WriteMember(_In_ const std::shared_ptr<XamlProperty>& spProperty)
{
    if (IsSkippingForConditionalScope())
    {
        return S_OK;
    }

    std::shared_ptr<XamlProperty> spTargetProperty = spProperty;

    if (m_bIsEncoding)
    {
        // before XBFv2 we would switch the VisualState.Storyboard property to the __DeferredStoryboard property in order to
        // be able to defer it using the CTemplateContent mechanism. With XBFv2+, we want the deferral to take place through VSM
        // so we revert the property shift that occurred in the NativeTypeInfoProvider::GetContentProperty that switched
        // the VisualState.Storyboard property to the VisualState.__DeferredStoryboard property.
        if (spProperty->get_PropertyToken() == XamlPropertyToken(tpkNative, KnownPropertyIndex::VisualState___DeferredStoryboard))
        {
           std::shared_ptr<XamlSchemaContext> spSchemaContext;
           IFC_RETURN(GetSchemaContext(spSchemaContext));
           IFC_RETURN(spSchemaContext->GetXamlProperty(
               XamlPropertyToken(tpkNative, KnownPropertyIndex::VisualState_Storyboard),
               XamlTypeToken(tpkNative, KnownTypeIndex::Storyboard),
               spTargetProperty));
        }
    }
    else
    {
        // If we're not encoding XBF, then we want to switch VisualState.Setters to VisualState.__DeferredSetters in order to
        // save away the nodes representing the SetterBaseCollection into a TemplateContent.  The setters node stream is
        // stored in a CTemplateContent object until first use.  CVisualState::GetValue has a check for VisualState_Setters
        // which calls TryDelayCreateLegacyPropertySetters, a function that inflates the CTemplateContent instance
        // holding the node stream to the setters, if the CTemplateContent object has not already been inflated. This is a legacy
        // deferral mechanism. Here we exclude the case where the template is being expanded on parse because the parser will
        // try to add incompatible types to a parent collection (see bug 2916828.)
        if (spProperty->get_PropertyToken() == XamlPropertyToken(tpkNative, KnownPropertyIndex::VisualState_Setters) && !(m_spContext->get_ExpandTemplates()))
        {
            std::shared_ptr<XamlSchemaContext> spSchemaContext;
            IFC_RETURN(GetSchemaContext(spSchemaContext));
            IFC_RETURN(spSchemaContext->GetXamlProperty(
                XamlPropertyToken(tpkNative, KnownPropertyIndex::VisualState___DeferredSetters),
                XamlTypeToken(tpkNative, KnownTypeIndex::TemplateContent),
                spTargetProperty));
        }
    }

    bool actionDeferred = false;

    if (!ShouldSkipForResourceReplacement())
    {
        m_spCustomWriterManager->SetLineInfo(GetLineInfo());

        auto deferredWriterFunc = [&]
        {
            return m_spDeferringWriter->WriteMember(spTargetProperty);
        };

        auto customWriterFunc = [&](bool& handled)
        {
            return m_spCustomWriterManager->WriteMember(spTargetProperty, handled);
        };

        IFC_RETURN(ForwardActionToChildrenWriters(
            std::ref(deferredWriterFunc),
            std::ref(customWriterFunc),
            actionDeferred));
    }

    if (!actionDeferred)
    {
        if (!(m_bIsEncoding && m_spContext->IsInConditionalScope(false)))
        {
            // report error if we have an unresolved property
            // Don't do validation if we are encoding and there is a XamlPredicate,
            // since it will be conditionally resolved at run time
            IFC_RETURN(m_spErrorService->ValidateIsKnown(spTargetProperty, GetLineInfo()));
        }

        // Replacement Resource Properties handling
        UpdateSkipDepthForResourceReplacement(XamlNodeType::xntStartProperty);

        // Check if a replacement values exists for this property in x:uid resources
        IFC_RETURN(InitiatePropertyReplacementIfNeeded(spTargetProperty));

        // Skip Processing due to Resource value replacement
        if (!ShouldSkipForResourceReplacement())
        {
            bool originalCheckDuplicateProperty = m_bCheckDuplicateProperty;
            auto scopeguard = wil::scope_exit([originalCheckDuplicateProperty, this]
            {
                this->m_bCheckDuplicateProperty = originalCheckDuplicateProperty;
            });

            if (m_bIsEncoding && m_spContext->Current().get_HasConditionalScopeToSkip(false))
            {
                m_bCheckDuplicateProperty = false;
            }

            IFC_RETURN(WriteMemberCore(spTargetProperty));
        }
    }

    return S_OK;
}


_Check_return_ HRESULT ObjectWriter::WriteMemberCore(_In_ const std::shared_ptr<XamlProperty>& spProperty)
{
    // Imagine the following XAML that gets loaded in via ResourceDictionary.Source:
    // <ResourceDictionary xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    //                     xmlns:x = "http://schemas.microsoft.com/winfx/2006/xaml">
    //   <ResourceDictionary.MergedDictionaries>
    //     <ResourceDictionary Source="Dictionary2.xaml" />
    //   </ResourceDictionary.MergedDictionaries>
    //
    //   <SolidColorBrush x:Key="MergedPinkBrush"
    //                    Color="Pink" />
    // </ResourceDictionary>
    //
    // The node stream looks like:
    // WriteObject
    //   WriteMember
    //     etc.
    //
    // A ResourceDictionaryCustomWriter will be created to handle it. At the time of creation during WriteObject,
    // the ObjectWriterStack will have a single empty frame. Simultaneously, an ObjectWriter (the collecting writer)
    // is created to collect nodes from the RDCW; since it is passed in our current saved content, it will have
    // the same stack (with a single empty frame) and a saved depth of 1.
    // Now the WriteMember node is processed. The collecting writer will be passed the WriteMember node, causing this
    // method (WriteMemberCore) to be invoked. Because the saved depth is 1 and the stack has only one frame on it,
    // the live depth is 0, but this is in fact a perfectly legal situation to be in.
    ASSERT(m_spContext->get_LiveDepth() >= 0);

    m_spContext->Current().set_Member(spProperty);

    IFC_RETURN(Logic_DuplicatePropertyCheck(spProperty));

    // If we haven't created the object yet then consider creating it now.
    // We need an object instance to set property values on.
    if (!m_spContext->Current().exists_Instance())
    {
        bool fShouldWriteMemberCreateWithCtor = false;
        IFC_RETURN(Logic_ShouldWriteMemberCreateWithCtor(/* out */ fShouldWriteMemberCreateWithCtor));
        if (fShouldWriteMemberCreateWithCtor)
        {
            IFC_RETURN(Logic_CreateWithCtor());
        }
    }

    return S_OK;
}


// Determines whether processing should be deferred or skipped and if
// not, passes processing on to WriteEndMemberCore
_Check_return_ HRESULT ObjectWriter::WriteEndMember()
{
    if (IsSkippingForConditionalScope())
    {
        return S_OK;
    }

    bool actionDeferred = false;

    if (!ShouldSkipForResourceReplacement())
    {
        int previousActiveCustomWriterCount = m_spCustomWriterManager->GetActiveCustomWriterCount();

        auto deferredWriterFunc = [&]
        {
            return m_spDeferringWriter->WriteEndMember();
        };

        auto customWriterFunc = [&](bool& handled)
        {
            return m_spCustomWriterManager->WriteEndMember(handled);
        };

        IFC_RETURN(ForwardActionToChildrenWriters(
            std::ref(deferredWriterFunc),
            std::ref(customWriterFunc),
            actionDeferred));

        bool adjustedActionDeferred = false;

        // Unlike with the other WriteXXX operations, we need to adjust the value of actionDeferred
        // based on whether or not a new CustomWriter was created to handle this node. This is because this
        // ObjectWriter still needs to write out the WriteEndMember node, even if a custom writer was
        // created to handle it.
        // Since actionDeferred hides the component values (deferring writer status and CustomWriter
        // status), we need to infer the original component values from it, specifically whether or not
        // a new CustomWriter was created to handle the WriteObject, in order to derive the correct
        // adjusted value of actionDeferred.
        if (!!m_spDeferringWriter->get_Handled())
        {
            // If the deferring writer handled the node, then ForwardActionToChildrenWriters will always
            // set actionDeferred to true.
            adjustedActionDeferred = true;
        }
        else
        {
            // If the deferring writer didn't handle the node, then ForwardActionToChildrenWriters will only set
            // actionDeferred to true if both the CustomWriter was active and it handled the node. We thus
            // need to also check if a CustomWriter was created to handle this node.
            int currentActiveCustomWriterCount = m_spCustomWriterManager->GetActiveCustomWriterCount();
            adjustedActionDeferred = actionDeferred && (previousActiveCustomWriterCount == currentActiveCustomWriterCount);
        }

        actionDeferred = adjustedActionDeferred;

        if (m_spDeferringWriter->get_Handled())
        {
            std::shared_ptr<XamlOptimizedNodeList> spXamlNodeList;

            switch (m_spDeferringWriter->get_Mode())
            {
                case dmResourceDictionaryReady:
                {

                    bool fIsDictionaryWithKeyProperty = false;
                    IFC_RETURN(m_spDeferringWriter->CollectResourceDictionaryList(spXamlNodeList, &fIsDictionaryWithKeyProperty));
                    IFC_RETURN(Logic_SetResourceDictionaryItemsProperty(spXamlNodeList, fIsDictionaryWithKeyProperty));
                }
                break;

                case dmTemplateReady:
                {
                    IFC_RETURN(m_spDeferringWriter->CollectTemplateList(spXamlNodeList));
                    IFC_RETURN(Logic_SetTemplateProperty(spXamlNodeList));

                    if (m_spContext->get_ExpandTemplates())
                    {
                        IFC_RETURN(ExpandTemplateForInitialValidation(spXamlNodeList));
                    }
                }
                break;
            }
        }
    }

    if (!actionDeferred)
    {
        // Replacement Resource Properties handling
        UpdateSkipDepthForResourceReplacement(XamlNodeType::xntEndProperty);

        if (!ShouldSkipForResourceReplacement())
        {
            IFC_RETURN(WriteEndMemberCore());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::WriteEndMemberCore()
{
    std::shared_ptr<XamlProperty> spProperty;

    // In the Text value case we will be on the text frame
    // and the property is in the parent.
    if (!m_spContext->Current().exists_Type())
    {
        spProperty = m_spContext->Parent().get_Member();
    }
    // In the Object value case we pop'ed and assigned the value already.
    else
    {
        spProperty = m_spContext->Current().get_Member();
    }

    if (spProperty->IsImplicit())
    {
        std::shared_ptr<ImplicitProperty> spImplicitProperty = std::static_pointer_cast<ImplicitProperty>(spProperty);
        switch (spImplicitProperty->get_ImplicitPropertyType())
        {
        case iptInitialization:
            // ***************************** This call changes the scope ************************** //
            IFC_RETURN(Logic_CreateFromInitializationValue());
            break;
        case iptItems:
            m_spContext->Current().clear_Collection();
            break;
        }
    }
    else
    {
        // CurrentType == NULL means this is TEXT.  (a'la XML TEXT)
        // Which is different than a String.  <x:String>text</x:String>
        // Would have a CurrentType of String.
        // Therefore we TypeConvert TEXT but not String.
        if (!m_spContext->Current().exists_Type() || m_spContext->Current().exists_Value())
        {
            std::shared_ptr<XamlSchemaContext> spSchemaContext;
            std::shared_ptr<XamlType> spStringType;

            IFC_RETURN(GetSchemaContext(spSchemaContext));
            IFC_RETURN(spSchemaContext->get_StringXamlType(spStringType));

            // if we have a value that's not string - the string could be in the CValue
            // or in a CString, then this is not a situation where we need to do type-conversion.
            // in addition if the value has been optimized for setvalue through the
            // xbf path then we still go through Logic_CreatePropertyValue() which will
            // set the value directly.
            if (m_spContext->Current().exists_Value() &&
                !((m_spContext->Current().get_Value()->GetIsXbfOptimized())) &&
                (m_spContext->Current().get_Value()->GetTypeToken() != spStringType->get_TypeToken() ||
                m_spContext->Current().get_Value()->GetDependencyObject() != NULL))
            {
                m_spContext->Current().set_Instance(m_spContext->Current().get_Value());
            }
            else // m_spContext->Current().get_Value() is a string
            {
                std::shared_ptr<XamlProperty> member = m_spContext->Parent().get_Member();
                std::shared_ptr<XamlType> memberType;
                IFC_RETURN(member->get_Type(memberType));
                
                if (!member->IsUnknown() && memberType != nullptr && !memberType->IsUnknown())
                {
                    bool isCollection;
                    IFC_RETURN(memberType->IsCollection(isCollection));

                    // get type token of collection member
                    std::shared_ptr<XamlTextSyntax> memberTextSyntax;
                    XamlTypeToken textSyntaxToken;

                    IFC_RETURN(m_spContext->Parent().get_Member()->get_TextSyntax(memberTextSyntax));
                    textSyntaxToken = memberTextSyntax->get_TextSyntaxToken();

                    if ((isCollection && m_spContext->Current().exists_Value() && m_spContext->Current().get_Value()->GetValue().GetType() == valueString) // member is a Collection type, value exists and is a string
                        && (textSyntaxToken.GetHandle() == KnownTypeIndex::UnknownType)) // collection member does not have typeconverter, create from initialization syntax
                    {
                        IFC_RETURN(Logic_InitializeCollectionFromString(memberType));
                        m_spContext->Current().clear_Member();
                        return S_OK;
                    }
                    else
                    {
                        IFC_RETURN(Logic_CreatePropertyValueFromText());
                    }
                }
                else
                {
                    IFC_RETURN(Logic_CreatePropertyValueFromText());
                }
            }

            m_qoLastInstance = m_spContext->Current().get_Instance();
            IFC_RETURN(Logic_DoAssignmentToParentProperty());
            IFC_RETURN(m_spRuntime->PopScope(GetLineInfo()));  // Text Node Scope
        }
    }

    if (m_bIsEncoding &&
        !m_spContext->Current().exists_Instance() &&
        spProperty->IsDirective())
    {
        auto directive = std::static_pointer_cast<DirectiveProperty>(spProperty);

        if (directive->get_DirectiveKind() == xdDeferLoadStrategy ||
            directive->get_DirectiveKind() == xdLoad)
        {
            // If this is x:DeferLoadStrategy or x:Load directive during encoding, then create object right away.
            // For loose XAML, don't change anything.  Directive will be set, but no other action is taken on it.
            IFC_RETURN(Logic_CreateWithCtor());
        }
    }

    // setting the CurrentProperty to a NULL shared_ptr
    m_spContext->Current().clear_Member();

    // It's possible for a CustomWriter to save out its CustomWriterRuntimeData inside WriteEndMember rather than
    // the usual WriteEndObject; ResourceDictionaryCustomWriter does this because only part of the ResourceDictionary
    // (the implicit items collection) is deferred, so the SetCustomRuntimeData node needs to be emitted as soon as
    // possible in order to maintain the original order in which properties are set.
    // Because of the way objects are created (creation may be deferred until WriteEndObject), we want to write out the
    // SetCustomRuntimeData node only if the object has already been created (otherwise there's no object for the node
    // to be associated with)
    if (m_spContext->Current().exists_Instance())
    {
        IFC_RETURN(Logic_CheckAndSetCustomRuntimeData());
    }

    return S_OK;
}

_Check_return_ HRESULT
ObjectWriter::Logic_InitializeCollectionFromString(_In_ const std::shared_ptr<XamlType>& memberType)
{
    // save collection initialization string as xstring_ptr
    xstring_ptr initializationString = m_spContext->Current().get_Value()->GetValue().AsString();
    IFC_RETURN(m_spRuntime->PopScope(GetLineInfo())); // Text Node Scope

    // push frame onto stack to hold collection
    IFC_RETURN(m_spRuntime->PushScope(GetLineInfo()));

    // get collection as member from Parent and set as current instance
    std::shared_ptr<XamlQualifiedObject> collectionInstance;

    IFC_RETURN(m_spRuntime->GetValue(
        GetLineInfo(),
        m_spContext->Parent().get_Instance(),
        m_spContext->Parent().get_Member(),
        collectionInstance));

    m_spContext->Current().set_IsObjectFromMember(true);

    // set the type and value of the current instance
    m_spContext->Current().set_Type(memberType);
    m_spContext->Current().set_Instance(collectionInstance);
    m_spContext->Current().set_Collection(collectionInstance);

    // get collection item type
    std::shared_ptr<XamlType> collectionItemType;
    IFC_RETURN(memberType->get_CollectionItemType(collectionItemType));
    
    // check if collection item type has typeconverter
    std::shared_ptr<XamlTextSyntax> collectionItemTypeTextSyntax;

    IFC_RETURN(collectionItemType->get_TextSyntax(collectionItemTypeTextSyntax));
    if (collectionItemTypeTextSyntax->get_TextSyntaxToken().GetHandle() == KnownTypeIndex::UnknownType)
    {
        xstring_ptr collectionTypeName;
        xstring_ptr collectionItemTypeName;
        IFC_RETURN(memberType->get_FullName(&collectionTypeName));
        IFC_RETURN(collectionItemType->get_FullName(&collectionItemTypeName));

        // Raise error "Failed to initialize a '%0' from string as item type '%1' has no typeconverter."
        IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_OW_COLLECTION_ITEM_TYPE_MISSING_TYPE_CONVERTER, GetLineInfo(), collectionTypeName, collectionItemTypeName));
        IFC_RETURN(E_INVALIDARG);
    }

    // parse initialization string
    Jupiter::stack_vector<xstring_ptr, 8> parsedStrings;

    if (FAILED(CollectionInitializationStringParser::ParseInitializationString(initializationString, parsedStrings)))
    {
        // Raise error "Failed to parse initialization string '%0'."
        IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_INVALID_INITIALIZATION_STRING_TO_PARSE, GetLineInfo(), initializationString));
        IFC_RETURN(E_INVALIDARG);
    }

    for (xstring_ptr rawObjectString : parsedStrings.m_vector) 
    {
        // create object from objectString and append
        if (rawObjectString.IsNullOrEmpty())
        {
            xstring_ptr collectionItemTypeName;
            IFC_RETURN(collectionItemType->get_FullName(&collectionItemTypeName));

            // Raise error "Failed to create a '%1' from the text '%0'."
            IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_OW_TYPE_CONVERSION_FAILED, GetLineInfo(), rawObjectString, collectionItemTypeName));
            IFC_RETURN(E_INVALIDARG);
        }
        IFC_RETURN(TypeConvertStringAndAddToCollectionOnCurrentInstance(collectionItemType, collectionItemTypeTextSyntax, rawObjectString));
    }
    IFC_RETURN(m_spRuntime->PopScope(GetLineInfo())); // pop frame holding collection

    return S_OK;
}

_Check_return_ HRESULT
ObjectWriter::TypeConvertStringAndAddToCollectionOnCurrentInstance(
    _In_ const std::shared_ptr<XamlType>& collectionItemType,
    _In_ const std::shared_ptr<XamlTextSyntax>& collectionItemTypeTextSyntax,
    _In_ xstring_ptr& trimmedObjectString)
{
    auto trimmedStringAsQO = std::make_shared<XamlQualifiedObject>();

    IFC_RETURN(trimmedStringAsQO->CreateFromXStringPtr(trimmedObjectString, trimmedStringAsQO));

    IFC_RETURN(m_spRuntime->PushScope(GetLineInfo())); // push frame to hold instance of new object to add

    std::shared_ptr<XamlServiceProviderContext> collectionItemTypeTextSyntaxContext = m_spContext->get_TextSyntaxContext();

    IFC_RETURN(m_spRuntime->PushConstant(GetLineInfo(), trimmedStringAsQO));
    std::shared_ptr<XamlQualifiedObject> typeConvertedValue;

    IFC_RETURN(m_spRuntime->TypeConvertValue(
        GetLineInfo(),
        collectionItemTypeTextSyntaxContext,
        collectionItemType,
        collectionItemTypeTextSyntax,
        trimmedStringAsQO,
        false,
        typeConvertedValue));

    IFC_RETURN(m_spRuntime->CreateTypeWithInitialValue(GetLineInfo(), collectionItemType, m_spObjectWriterCallbacks, m_qoRootObjectInstance, typeConvertedValue));

    m_spContext->Current().set_Type(collectionItemType);
    m_spContext->Current().set_Instance(typeConvertedValue);

    IFC_RETURN(m_spRuntime->BeginInit(
        GetLineInfo(),
        m_spContext->Current().get_Type(),
        m_spContext->Current().get_Instance()));
    IFC_RETURN(Logic_ApplyCurrentSavedDirectives());
    IFC_RETURN(m_spRuntime->EndInit(
        GetLineInfo(),
        m_spContext->Current().get_Type(),
        m_spContext->Current().get_Instance()));

    if (!m_bIsEncoding)
    {
        IFC_RETURN(m_spErrorService->ValidateFrameForCollectionAdd(m_spContext->Parent(), GetLineInfo()));
    }

    IFC_RETURN(m_spRuntime->AddToCollection(
        GetLineInfo(),
        m_spContext->Parent().get_Type(),
        m_spContext->Parent().get_Collection(),
        m_spContext->Current().get_Instance(),
        m_spContext->Current().get_Type()));
    
    IFC_RETURN(m_spRuntime->PopScope(GetLineInfo())); // pop frame holding instance of added object

    return S_OK;
}

_Check_return_ HRESULT
ObjectWriter::WriteConditionalScope(const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs)
{
    if (!ShouldSkipForResourceReplacement())
    {
        bool actionDeferred = false;

        m_spCustomWriterManager->SetLineInfo(GetLineInfo());
        auto deferredWriterFunc = [&]
        {
            return m_spDeferringWriter->WriteConditionalScope(xamlPredicateAndArgs);
        };

        auto customWriterFunc = [&](bool& handled)
        {
            return m_spCustomWriterManager->WriteConditionalScope(xamlPredicateAndArgs, handled);
        };

        IFC_RETURN(ForwardActionToChildrenWriters(
            std::ref(deferredWriterFunc),
            std::ref(customWriterFunc),
            actionDeferred));

        if (!actionDeferred)
        {
            try
            {
                if (!m_bIsEncoding)
                {
                    auto result = Parser::XamlPredicateService::EvaluatePredicate(xamlPredicateAndArgs->PredicateType, xamlPredicateAndArgs->Arguments);
                    m_spContext->Current().PushConditionalScopeNode(!result);
                }
                else
                {
                    // When encoding, we pretend that all predicates evaluate to true (i.e. shouldn't skip)
                    m_spContext->Current().PushConditionalScopeNode(false);
                }

                THROW_IF_FAILED(m_spRuntime->BeginConditionalScope(GetLineInfo(), xamlPredicateAndArgs));
            }
            CATCH_RETURN();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ObjectWriter::WriteEndConditionalScope()
{
    if (!ShouldSkipForResourceReplacement())
    {
        bool actionDeferred = false;

        m_spCustomWriterManager->SetLineInfo(GetLineInfo());
        auto deferredWriterFunc = [&]
        {
            return m_spDeferringWriter->WriteEndConditionalScope();
        };

        auto customWriterFunc = [&](bool& handled)
        {
            return m_spCustomWriterManager->WriteEndConditionalScope(handled);
        };

        IFC_RETURN(ForwardActionToChildrenWriters(
            std::ref(deferredWriterFunc),
            std::ref(customWriterFunc),
            actionDeferred));

        if (!actionDeferred)
        {
            m_spContext->Current().PopConditionalScopeNode();

            IFC_RETURN(m_spRuntime->EndConditionalScope(GetLineInfo()));
        }
    }

    return S_OK;
}

// Determines whether processing should be deferred or skipped and if
// not, passes processing on to WriteNamespaceCore
_Check_return_ HRESULT ObjectWriter::WriteNamespace(
    _In_ const xstring_ptr& spPrefix,
    const std::shared_ptr<XamlNamespace>& spXamlNamespace)
{
    if (IsSkippingForConditionalScope())
    {
        return S_OK;
    }

    if (!ShouldSkipForResourceReplacement())
    {
        bool actionDeferred = false;

        m_spCustomWriterManager->SetLineInfo(GetLineInfo());
        auto deferredWriterFunc = [&]
        {
            return m_spDeferringWriter->WriteNamespace(spPrefix, spXamlNamespace);
        };

        auto customWriterFunc = [&](bool& handled)
        {
            return m_spCustomWriterManager->WriteNamespace(spPrefix, spXamlNamespace, handled);
        };

        IFC_RETURN(ForwardActionToChildrenWriters(
            std::ref(deferredWriterFunc),
            std::ref(customWriterFunc),
            actionDeferred));

        if (!actionDeferred)
        {
            IFC_RETURN(WriteNamespaceCore(spPrefix, spXamlNamespace));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ObjectWriter::WriteNamespaceCore(
    _In_ const xstring_ptr& spPrefix,
    const std::shared_ptr<XamlNamespace>& spXamlNamespace)
{
    // Namespace definitions are always associated with (and followed by) a WriteObject
    // We'll queue up the namespaces until that WriteObject is encountered, at which point
    // we'll PushScope and then copy over the queue to the new scope
    m_namespacePrefixQueue.emplace_back(GetLineInfo(), spPrefix, spXamlNamespace);

    return S_OK;
}



// Determines whether processing should be deferred or skipped and if
// not, passes processing on to WriteValueCore
_Check_return_ HRESULT
ObjectWriter::WriteValue(_In_ const std::shared_ptr<XamlQualifiedObject>& value)
{
    if (IsSkippingForConditionalScope())
    {
        return S_OK;
    }

    if (!ShouldSkipForResourceReplacement())
    {
        bool actionDeferred = false;

        m_spDeferringWriter->SetLineInfo(GetLineInfo());
        m_spCustomWriterManager->SetLineInfo(GetLineInfo());

        auto deferredWriterFunc = [&]
        {
            return m_spDeferringWriter->WriteValue(value);
        };

        auto customWriterFunc = [&](bool& handled)
        {
            return m_spCustomWriterManager->WriteValue(value, handled);
        };

        IFC_RETURN(ForwardActionToChildrenWriters(
            std::ref(deferredWriterFunc),
            std::ref(customWriterFunc),
            actionDeferred));

        if (!actionDeferred)
        {
            IFC_RETURN(WriteValueCore(value));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ObjectWriter::WriteValueCore(_In_ const std::shared_ptr<XamlQualifiedObject>& value)
{
    //The first node (T, SO, or EP) after an EndObject should NULL out m_qoLastInstance.
    m_qoLastInstance.reset();

    IFC_RETURN(m_spRuntime->PushScope(GetLineInfo()));
    IFC_RETURN(m_spRuntime->PushConstant(GetLineInfo(), value));

    // report error if we are setting a value on a non existent parent property
    IFC_RETURN(m_spErrorService->ValidateFrameForWriteValue( m_spContext->Parent(), GetLineInfo()))

    // There are two cases.
    // 1) Text on a regular property.
    //    Processed on EndMember.  (not here)
    // 2) Text on an _items implicit property.
    //    Processed Item by item right here.
    if (m_spContext->Parent().get_Member()->IsImplicit())
    {
        std::shared_ptr<ImplicitProperty> spImplicitProperty = std::static_pointer_cast<ImplicitProperty>(m_spContext->Parent().get_Member());
        if (spImplicitProperty->get_ImplicitPropertyType() == iptItems)
        {
            std::shared_ptr<XamlType> spType;
            std::shared_ptr<XamlSchemaContext> spSchemaContext;

            IFC_RETURN(GetSchemaContext(spSchemaContext));

            // Set the type and value of the current instance
            IFC_RETURN(spSchemaContext->GetXamlType(value->GetTypeToken(), spType));
            m_spContext->Current().set_Type(spType);
            m_spContext->Current().set_Instance(value);

            IFC_RETURN(Logic_DoAssignmentToParentCollection());
            IFC_RETURN(m_spRuntime->PopScope(GetLineInfo()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ObjectWriter::ForwardActionToChildrenWriters(
    const std::function<HRESULT()>& deferringWriterAction,
    const std::function<HRESULT(bool&)>& customWriterManagerAction,
    _Out_ bool& isDeferred)
{
    isDeferred = false;
    bool customWriterHandled = false;
    int previousActiveCustomWriterCount = m_spCustomWriterManager->GetActiveCustomWriterCount();

    if (m_spDeferringWriter->get_Handled())
    {
        IFC_RETURN(deferringWriterAction());
    }
    else if (m_spCustomWriterManager->IsCustomWriterActive())
    {
        IFC_RETURN(customWriterManagerAction(customWriterHandled));
    }
    else
    {
        IFC_RETURN(deferringWriterAction());
        if (m_allowCustomWriter)
        {
            IFC_RETURN(customWriterManagerAction(customWriterHandled));
        }
    }

    bool deferringWriterActive = !!m_spDeferringWriter->get_Handled();

    bool activeCustomWriterCountDecreased = previousActiveCustomWriterCount > m_spCustomWriterManager->GetActiveCustomWriterCount();
    bool customWriterManagerActive = !activeCustomWriterCountDecreased && customWriterHandled;

    isDeferred = deferringWriterActive || customWriterManagerActive;

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::Logic_ShouldWriteMemberCreateWithCtor(_Out_ bool& fOut)
{
    // Start out assuming regular property on an normal type
    fOut = true;

    // We need to create the object instance to hold "real" properties.  But we might
    // need to wait for parameters for Initialization Text or with Ctor args.
    // So don't create with default constructor unless we know it can't be
    // Initialization Text or PositionalParameters.
    //
    if (m_spContext->Current().get_Member()->IsDirective())
    {
        XamlDirectives directiveKind = std::static_pointer_cast<DirectiveProperty>(m_spContext->Current().get_Member())->get_DirectiveKind();

        if (directiveKind == xdConnectionId || directiveKind == xdUid)
        {
            fOut = true;
        }
        else
        {
            fOut = false;
        }
        return S_OK;
    }

    if (m_spContext->Current().get_IsObjectFromMember())
    {
        fOut = false;
        return S_OK;
    }

    if (m_spContext->Current().get_Member()->IsImplicit())
    {
        std::shared_ptr<ImplicitProperty> spImplicitProperty = std::static_pointer_cast<ImplicitProperty>(m_spContext->Current().get_Member());

        // report error if the implicit property was unknown
        IFC_RETURN(m_spErrorService->ValidateIsKnown(spImplicitProperty, GetLineInfo()));

        switch (spImplicitProperty->get_ImplicitPropertyType())
        {
            case iptInitialization:
            {
                fOut = false;
                break;
            }
            case iptItems:
            {
                fOut = true;
                break;
            }
            default:
            {
                // can't really get here if we did the correct validation
                break;
            }
        }
        return S_OK;
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::Logic_CreateWithCtor()
{
    bool bIsDictionary = false;
    bool bIsCollection = false;
    std::shared_ptr<XamlQualifiedObject> qoInst;

    // If the depth is greater than one then this is not the root, so we should get the line info
    // of the current context, otherwise we should get the cached line info.
    XamlLineInfo objectLineInfo = m_spContext->get_Depth() > 1 ? GetLineInfo() : m_rootLineInfo;

    // report error if we are constructing a type which is fromMember
    IFC_RETURN(m_spErrorService->ValidateFrameForConstructType(m_spContext->Current(), objectLineInfo));

    ASSERT(m_spContext->Current().exists_Type());

    IFC_RETURN(m_spRuntime->CreateType(objectLineInfo, m_spContext->Current().get_Type(), m_spObjectWriterCallbacks, m_qoRootObjectInstance, qoInst));
    m_spContext->Current().set_Instance(qoInst);

    if(m_spContext->get_RootNamescope() && qoInst->GetDependencyObject())
    {
        // If the name scope helper doesn't have a namescope owner yet, use this one.  (This establishes the root.)
        // (Consider factoring this from BinaryFormatObjectWriter and ObjectWriter into ObjectWriterRuntime.)
        m_spContext->get_RootNamescope()->EnsureNamescopeOwner(qoInst);
    }

    IFC_RETURN(Logic_ApplyCurrentSavedDirectives());

    if (!(m_spContext->Current().get_Type()->IsUnknown()))
    {
        IFC_RETURN(m_spContext->Current().get_Type()->IsCollection(bIsCollection));
        IFC_RETURN(m_spContext->Current().get_Type()->IsDictionary(bIsDictionary));
    }
    else
    {
        // If the current instance's type is Unknown, then we had better be inside a
        // conditional XAML scope and encoding (since that is the only circumstance
        // under which an unknown type is permitted)
        ASSERT(m_bIsEncoding && m_spContext->IsInConditionalScope(false));
    }

    if (bIsCollection || bIsDictionary)
    {
        m_spContext->Current().set_Collection(qoInst);
    }

    IFC_RETURN(m_spRuntime->BeginInit(
        objectLineInfo,
        m_spContext->Current().get_Type(),
        m_spContext->Current().get_Instance()));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::Logic_DuplicatePropertyCheck(_In_ const std::shared_ptr<XamlProperty>& spProperty)
{
    if (m_bCheckDuplicateProperty)
    {
        auto& currentFrame = m_spContext->Current();

        // report error if we are setting a duplicate property
        IFC_RETURN(m_spErrorService->ValidateFrameForPropertySet(currentFrame, spProperty, GetLineInfo()));

        // notify the frame of the property assignment
        currentFrame.NotifyPropertyAssigned(spProperty);
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::Logic_ApplyCurrentSavedDirectives()
{
    if (m_spContext->Current().exists_Directives())
    {
        std::shared_ptr<DirectiveProperty> spNameProperty;
        std::shared_ptr<DirectiveProperty> spKeyProperty;

        IFC_RETURN(get_X_NameProperty(spNameProperty));
        IFC_RETURN(get_X_KeyProperty(spKeyProperty));

        auto spSavedDirectives = m_spContext->Current().get_Directives();

        for (auto iterSavedDirectives = spSavedDirectives->begin();
            iterSavedDirectives != spSavedDirectives->end();
            ++iterSavedDirectives)
        {
            auto entry = *iterSavedDirectives;

            if (!XamlProperty::AreEqual(spKeyProperty, entry.first))
            {
                std::shared_ptr<XamlProperty> spRemappedProperty;

                // RemapDirectiveToProperty will return S_FALSE if it can't map
                if (S_OK == RemapDirectiveToProperty(
                        m_spContext->Current().get_Type(),
                        entry.first,
                        spRemappedProperty))
                {
                    IFC_RETURN(Logic_DuplicatePropertyCheck(spRemappedProperty));
                    IFC_RETURN(m_spRuntime->SetValue(
                        GetLineInfo(),
                        m_spContext->Current().get_Type(),
                        m_spContext->Current().get_Instance(),
                        spRemappedProperty,
                        entry.second));
                }
            }

            // Register x:Name associations in namescopes
            if (XamlProperty::AreEqual(spNameProperty, entry.first))
            {
                IFC_RETURN(Logic_RegisterName_OnCurrent(entry.second));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::Logic_ProvideValue()
{
    std::shared_ptr<XamlQualifiedObject> qoInstance;
    std::shared_ptr<XamlQualifiedObject> qoValue;
    std::shared_ptr<XamlType> spXamlType;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;

    qoInstance = m_spContext->Current().get_Instance();
    IFC_RETURN(m_spRuntime->ProvideValue(
        GetLineInfo(),
        m_spContext->get_MarkupExtensionContext(),
        m_spContext->Current().get_Instance(),
        qoValue));

    // Some markup extensions return themselves, such as Binding. Avoid doing any work. This way
    // we don't have to deal with properly carrying over the PegNoRef from one XQO to the other.
    if (qoValue->GetValue() != qoInstance->GetValue())
    {
        IFC_RETURN(GetSchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetXamlType(qoValue->GetTypeToken(), spXamlType));
        m_spContext->Current().set_Type(spXamlType);
        m_spContext->Current().set_Instance(qoValue);
    }
    else
    {
        ASSERT(qoInstance->GetHasPeggedManagedPeer());

        // The instance is the value. Make sure that when qoValue goes out of scope, it doesn't
        // unpeg the instance.
        qoValue->ClearHasPeggedManagedPeer();
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::Logic_AssignProvidedValue()
{
    bool skipProvideValueCall = false;
    bool memberExistsOnParent = m_spContext->Parent().exists_Member();

    // If we're assigning a custom markup extension to a property that is of type
    // MarkupExtension, then skip the call to MarkupExtension::ProvideValue().
    if (memberExistsOnParent)
    {
        auto memberOnParent = m_spContext->Parent().get_Member();
        if (!(memberOnParent->IsImplicit() || memberOnParent->IsDirective() || memberOnParent->IsUnknown()))
        {
            std::shared_ptr<XamlType> propertyType;
            IFC_RETURN(memberOnParent->get_Type(propertyType));
            if (DirectUI::MetadataAPI::IsAssignableFrom<KnownTypeIndex::MarkupExtension>(propertyType->get_TypeToken().GetHandle()))
            {
                skipProvideValueCall = true;
            }
        }
    }

    if (!skipProvideValueCall)
    {
        IFC_RETURN(Logic_ProvideValue());
    }

    // Checking that the ME isn't the Root of the XAML Document.
    if (memberExistsOnParent)
    {
        IFC_RETURN(Logic_DoAssignmentToParentProperty());
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::RemapDirectiveToProperty(
    _In_ const std::shared_ptr<XamlType>& inParentType,
    _In_ const std::shared_ptr<DirectiveProperty>& inDirectiveProperty,
    _Out_ std::shared_ptr<XamlProperty>& outRemappedProperty)
{
    HRESULT hr = S_OK;
    std::shared_ptr<XamlProperty> spRemappedProperty;
    XamlDirectives directiveKind = inDirectiveProperty->get_DirectiveKind();

    outRemappedProperty = spRemappedProperty;

    if (directiveKind == xdName)
    {
        IFC(inParentType->get_RuntimeNameProperty(spRemappedProperty));
    }
    else if (directiveKind == xdLang)
    {
        IFC(inParentType->get_XmlLangProperty(spRemappedProperty));
    }

    if (!spRemappedProperty)
    {
        hr = S_FALSE;
    }
    else
    {
        outRemappedProperty = spRemappedProperty;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ObjectWriter::Logic_DoAssignmentToParentProperty()
{
    HRESULT hr = S_OK;

    if (m_spContext->Parent().get_Member()->IsImplicit())
    {
        IFC(Logic_DoAssignmentToParentCollection());
    }
    else
    {
        if (m_spContext->Parent().exists_Instance())
        {
            if (!m_spContext->Current().get_IsObjectFromMember())
            {
                // Note: Changed to handle directives. Will remap the directive to a property
                // on the actual type.
                //
                // In the case of x:Class it must be the first node AND an existing root object
                // must have been passed in.
                //
                // TODO: is this x:Class handling in the correct place?
                if (m_spContext->Parent().get_Member()->IsDirective())
                {
                    auto directiveProperty = std::static_pointer_cast<DirectiveProperty>(m_spContext->Parent().get_Member());
                    auto directiveKind = directiveProperty->get_DirectiveKind();
                    switch (directiveKind)
                    {
                        case xdClassModifier:
                        case xdFieldModifier:
                        {
                            // Ignore ClassModifier and FieldModifier because
                            // they mean nothing at runtime (note that we would
                            // also ignore Uid here if we didn't strip all
                            // instances out in the XamlScanner via
                            // XamlSortedAttributes::Add)
                            break;
                        }
                        case xdClass:
                        {
                            // report error if x:Class is incorrectly present
                            IFC(m_spErrorService->ValidateXClassMustBeOnRoot(m_spContext->get_LiveDepth(), m_qoRootObjectInstance, GetLineInfo()));

                            if (   m_spContext->get_LiveDepth() == 2
                                && (m_qoRootObjectInstance || m_bIsEncoding))
                            {
                                // Make sure that setting what we did at the root was kosher.
                                // We know x:Class will be the first possible attribute we see.
                                IFC(m_spRuntime->CheckPeerType(
                                    GetLineInfo(),
                                    m_qoRootObjectInstance,
                                    m_spContext->Current().get_Instance()));
                                goto Cleanup;
                            }
                            break;
                        }
                        case xdConnectionId:
                        {
                            // In XBFv1 scenarios, IComponentConnector2 should not be called.

                            IFC(m_spRuntime->SetConnectionId(
                                GetLineInfo(),
                                m_spContext->get_RootInstance(),
                                m_spContext->Current().get_Instance(),
                                m_spContext->Parent().get_Instance()));
                            break;
                        }
                        case xdUid:
                        {
                            IFC(GetResourcePropertyBag());
                            break;
                        }
                        default:
                        {
                            // Attempt to remap it.
                            std::shared_ptr<DirectiveProperty> spKeyProperty;
                            IFC(get_X_KeyProperty(spKeyProperty));
                            if (!XamlProperty::AreEqual(spKeyProperty, m_spContext->Parent().get_Member()))
                            {
                                std::shared_ptr<XamlProperty> spRemappedProperty;

                                if (directiveKind == xdName)
                                {
                                    IFC(Logic_RegisterName_OnParent(m_spContext->Current().get_Instance()));

                                    //
                                    // Add x:Name to map of directives for case where parent instance
                                    // already exists (eg if ConnectionID is set on the element)
                                    //
                                    IFC(m_spRuntime->SetDirectiveProperty(
                                        GetLineInfo(),
                                        directiveProperty,
                                        m_spContext->Current().get_Instance()));
                                }


                                //RemapDirectiveToProperty will return S_FALSE if it can't map
                                hr = RemapDirectiveToProperty(
                                    m_spContext->Parent().get_Type(),
                                    directiveProperty,
                                    spRemappedProperty);
                                if (hr == S_OK)
                                {
                                    IFC(Logic_DuplicatePropertyCheck(spRemappedProperty));
                                    IFC(m_spRuntime->SetValue(
                                        GetLineInfo(),
                                        m_spContext->Parent().get_Type(),
                                        m_spContext->Parent().get_Instance(),
                                        spRemappedProperty,
                                        m_spContext->Current().get_Instance()));
                                }
                            }
                            //
                            // Add x:Key to map of directives for case where parent instance
                            // already exists (eg if ConnectionID is set on the element)
                            //
                            else
                            {
                                if (directiveKind == xdKey)
                                {
                                    IFC(m_spRuntime->SetDirectiveProperty(
                                        GetLineInfo(),
                                        directiveProperty,
                                        m_spContext->Current().get_Instance()));
                                }
                            }
                        }
                    }
                }
                else if (   !m_spContext->Parent().get_Member()->IsUnknown()
                         && m_spContext->Parent().get_Member()->IsEvent())
                {
                    // Silently skip events.
                    goto Cleanup;
                }
                else
                {
                    std::shared_ptr<XamlProperty> spRuntimeNameProperty;

                    IFC(m_spRuntime->SetValue(
                        GetLineInfo(),
                        m_spContext->Parent().get_Type(),
                        m_spContext->Parent().get_Instance(),
                        m_spContext->Parent().get_Member(),
                        m_spContext->Current().get_Instance()));

                    if (!m_spContext->Parent().get_Member()->IsUnknown())
                    {
                        // If the property maps to the RuntimeNameProperty, then
                        // we need to register the value of the property as a
                        // name association (just like the x:Name directive is
                        // registered in Logic_ApplyCurrentSavedDirectives) in
                        // the parent's namescope.
                        IFC(m_spContext->Parent().get_Type()->get_RuntimeNameProperty(spRuntimeNameProperty));
                        if (XamlProperty::AreEqual(m_spContext->Parent().get_Member(), spRuntimeNameProperty))
                        {
                            IFC(Logic_RegisterName_OnParent(m_spContext->Current().get_Instance()));
                        }
                    }
                }
            }
        }
        else
        {
            if (m_spContext->Parent().get_Member()->IsDirective())
            {
                auto directiveProperty = std::static_pointer_cast<DirectiveProperty>(m_spContext->Parent().get_Member());
                // report error if this is a duplicate directive
                IFC(m_spErrorService->ValidateFrameForPropertySet(m_spContext->Parent(), directiveProperty, GetLineInfo()));

                if (directiveProperty->get_DirectiveKind() == xdClass)
                {
                    // report error if x:Class is incorrectly present
                    IFC(m_spErrorService->ValidateXClassMustBeOnRoot(m_spContext->get_LiveDepth(), m_qoRootObjectInstance, GetLineInfo()));
                }

                IFC(m_spRuntime->SetDirectiveProperty(GetLineInfo(), directiveProperty, m_spContext->Current().get_Instance()));
            }
            else
            {
                // report error as we are in a bad object writer state
                IFC(m_spErrorService->ReportBadObjectWriterState(GetLineInfo()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

std::shared_ptr<XamlQualifiedObject> ObjectWriter::get_Result() const
{
    //Note that in the case that the document didn't parser correctly because of an error
    //m_qoLastInstance may not be m_spContext->get_RootInstance
    return m_qoLastInstance;
}

// TODO: put these directive properties in one place so that they can
// be accessed by the parser?
_Check_return_ HRESULT ObjectWriter::get_X_KeyProperty(_Out_ std::shared_ptr<DirectiveProperty>& rspOut)
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;

    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    IFC_RETURN(spSchemaContext->get_X_KeyProperty(rspOut));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::get_X_NameProperty(_Out_ std::shared_ptr<DirectiveProperty>& rspOut)
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;

    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    IFC_RETURN(spSchemaContext->get_X_NameProperty(rspOut));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::Logic_CreateFromInitializationValue()
{
    std::shared_ptr<XamlTextSyntax> spTextSyntax;
    std::shared_ptr<XamlQualifiedObject> qoInstance;
    std::shared_ptr<XamlType> spParentType = m_spContext->Parent().get_Type();

    // report error if type is not known
    IFC_RETURN(m_spErrorService->ValidateIsKnown(spParentType, GetLineInfo()));

    IFC_RETURN(spParentType->get_TextSyntax(spTextSyntax));

    {
        std::shared_ptr<XamlServiceProviderContext> spTextSyntaxContext;

        spTextSyntaxContext = m_spContext->get_TextSyntaxContext();

        IFC_RETURN(m_spRuntime->TypeConvertValue(
            GetLineInfo(),
            spTextSyntaxContext,
            spParentType,
            spTextSyntax,
            m_spContext->Current().get_Value(),
            false,
            qoInstance));

        if(m_spContext->get_RootNamescope() && qoInstance->GetDependencyObject())
        {
            // If the name scope helper doesn't have a namescope owner yet, use this one.  (This establishes the root.)
            m_spContext->get_RootNamescope()->EnsureNamescopeOwner(qoInstance);
        }
    }


    // *********** Scope changes here! ******************* //
    // Pop off the Text Frame.
    IFC_RETURN(m_spRuntime->PopScope(GetLineInfo()));

    IFC_RETURN(m_spRuntime->CreateTypeWithInitialValue(GetLineInfo(), spParentType, m_spObjectWriterCallbacks, m_qoRootObjectInstance, qoInstance));
    m_spContext->Current().set_Instance(qoInstance);

    {
        bool bIsCollection = false;
        bool bIsDictionary = false;

        // NB: Using the cached spParentType from the start of the function
        // because the scope was popped.
        IFC_RETURN(spParentType->IsCollection(bIsCollection));
        IFC_RETURN(spParentType->IsDictionary(bIsDictionary));

        if (bIsCollection || bIsDictionary)
        {
            m_spContext->Current().set_Collection(qoInstance);
        }

        IFC_RETURN(m_spRuntime->BeginInit(
            GetLineInfo(),
            m_spContext->Current().get_Type(),
            m_spContext->Current().get_Instance()));
        IFC_RETURN(Logic_ApplyCurrentSavedDirectives());
        IFC_RETURN(m_spRuntime->EndInit(
            GetLineInfo(),
            m_spContext->Current().get_Type(),
            m_spContext->Current().get_Instance()));
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::Logic_CreatePropertyValueFromText()
{
    auto qoInstance = m_spContext->Current().get_Value();
    auto property = m_spContext->Parent().get_Member();
    std::shared_ptr<XamlType> spPropertyType;
    IFC_RETURN(property->get_Type(spPropertyType));

    bool shouldSkipTypeConversion = false;
    // If the property being set is a simple property and a non-primitive type, skip the type
    // conversion here and let the code-genned setter handle it.
    // The reason is that since the setters are strongly typed, we may as well let
    // the compiler tell us if we've forgotten a type converter by explicitly invoking it
    // rather than generically dispatching by property type.
    if (   !property->IsUnknown() && !property->IsDirective() && !property->IsImplicit() && !property->IsEvent()
        && MetadataAPI::GetPropertyBaseByIndex(property->get_PropertyToken().GetHandle())->Is<CSimpleProperty>())
    {
        switch (spPropertyType->get_TypeToken().GetHandle())
        {
            case KnownTypeIndex::Matrix4x4:
            case KnownTypeIndex::Vector3:
            {
                shouldSkipTypeConversion = true;
            }
            break;
        }
    }

    if (!shouldSkipTypeConversion)
    {
        std::shared_ptr<XamlTextSyntax> spTextSyntax;
        std::shared_ptr<XamlServiceProviderContext> spTextSyntaxContext;

        // The text syntax isn't utilized for XBF, and we won't be able to retrieve it
        // for an unknown property anyway (scenario: conditional XAML), so just skip.
        if (!(m_bIsEncoding && property->IsUnknown()))
        {
            // I moved this inside the parent if
            // because it fails on directive properties, and it isn't used
            // in the other branches.

            // this looks at the spProperty
            // AND the type of the spProperty
            IFC_RETURN(property->get_TextSyntax(spTextSyntax));
            spTextSyntaxContext = m_spContext->get_TextSyntaxContext();
        }

        IFC_RETURN(m_spRuntime->TypeConvertValue(
            GetLineInfo(),
            spTextSyntaxContext,
            spPropertyType,
            spTextSyntax,
            m_spContext->Current().get_Value(),
            true,
            qoInstance));
    }

    m_spContext->Current().set_Instance(qoInstance);

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::Logic_DoAssignmentToParentCollection()
{
    std::shared_ptr<XamlType> spParentPropertyType;
    std::shared_ptr<XamlQualifiedObject> qoKey;

    bool bIsDictionary = false;
    bool bIsCollection = false;

    // This method should only have been called if we're adding a value to the
    // collection retrieved from the implicit items property
    ASSERT(m_spContext->Parent().get_Member()->IsImplicit());

    // Ensure the collection to add to isn't null
    IFC_RETURN(m_spErrorService->ValidateFrameForCollectionAdd(m_spContext->Parent(), GetLineInfo()));

    spParentPropertyType = m_spContext->Parent().get_Type();
    if (spParentPropertyType)
    {
        IFC_RETURN(spParentPropertyType->IsDictionary(bIsDictionary));
        IFC_RETURN(spParentPropertyType->IsCollection(bIsCollection));
    }

    // Always check for a key if the type implements dictionary.
    if (bIsDictionary)
    {
        if (m_spContext->Current().exists_Directives())
        {
            std::shared_ptr<DirectiveProperty> spKeyProperty;
            // Look for the "x:Key" directive
            IFC_RETURN(get_X_KeyProperty(spKeyProperty));
            IFC_RETURN(m_spContext->Current().tryget_Directive(spKeyProperty, qoKey));

            if (!qoKey)
            {
                // if we couldn't find that, then look for the "x:Name" directive
                IFC_RETURN(get_X_NameProperty(spKeyProperty));
                IFC_RETURN(m_spContext->Current().tryget_Directive(spKeyProperty, qoKey));
            }
        }

        // Look for the designated "key" property
        if (!qoKey)
        {
            std::shared_ptr<XamlProperty> spKeyProperty;
            // lookup the key property for the type.
            IFC_RETURN(m_spContext->Current().get_Type()->get_DictionaryKeyProperty(spKeyProperty));
            if (spKeyProperty)
            {
                if (!m_bIsEncoding)
                {
                    XamlQualifiedObject tempKey;
                    // We allow this to fail since we retry for name.
                    IGNOREHR(spKeyProperty->GetValue(
                        *m_spContext->Current().get_Instance(),
                        tempKey));
                    if (!tempKey.IsUnset())
                    {
                        qoKey = std::make_shared<XamlQualifiedObject>(std::move(tempKey));
                    }
                }
            }

            if (!qoKey)
            {
                // TODO: This is not wpf compat.
                IFC_RETURN(m_spContext->Current().get_Type()->get_RuntimeNameProperty(spKeyProperty));
                if (spKeyProperty)
                {
                    if (!m_bIsEncoding)
                    {
                        XamlQualifiedObject tempKey;
                        // REVIEW: this looks like a copy/paste of above (just checking the
                        // runtime name property instead of dictionary key property). Do we still
                        // want to ignore the result?
                        IGNOREHR(spKeyProperty->GetValue(
                            *m_spContext->Current().get_Instance(),
                            tempKey));
                        if (!tempKey.IsUnset())
                        {
                            qoKey = std::make_shared<XamlQualifiedObject>(std::move(tempKey));
                        }
                    }
                }
            }

            if (!m_bIsEncoding && !qoKey)
            {
                if (bIsCollection)
                {
                    // In rare situations we have a parent property type that is
                    // both an IList and an IDictionary. If no key is found
                    // then revert to treating as a simple list.
                    bIsDictionary = FALSE;
                }
                else
                {
                    // This error applies when there is a dictionary but not a
                    // list. It is the typical case.
                    IFC_RETURN(m_spErrorService->ReportDictionaryItemMustHaveKey(GetLineInfo()));
                }
            }
        }
    }

    // TODO: need a more elegant way to handle the
    // type of the __items ImplicitProperty's type (which in System.Xaml
    // is typeof(Object).
    if (!bIsDictionary)
    {
        IFC_RETURN(m_spRuntime->AddToCollection(
            GetLineInfo(),
            spParentPropertyType,
            m_spContext->Parent().get_Collection(),
            m_spContext->Current().get_Instance(),
            m_spContext->Current().get_Type()));
    }
    else
    {
        IFC_RETURN(m_spRuntime->AddToDictionary(
            GetLineInfo(),
            m_spContext->Parent().get_Collection(),
            m_spContext->Current().get_Instance(),
            m_spContext->Current().get_Type(),
            qoKey));
    }

    return S_OK;
}

// Create an association between the current instance and a name in a template's
// namescope.
_Check_return_ HRESULT ObjectWriter::Logic_RegisterName_OnCurrent(_In_ const std::shared_ptr<XamlQualifiedObject>& qoName)
{
    RRETURN(m_spRuntime->SetName(
        GetLineInfo(),
        qoName,
        m_spContext->Current().get_Instance(),
        m_spContext->get_RootNamescope()));
}

// Create an association between the parent instance and a name in a template's
// namescope
_Check_return_ HRESULT ObjectWriter::Logic_RegisterName_OnParent(_In_ const std::shared_ptr<XamlQualifiedObject>& qoName)
{
    RRETURN(m_spRuntime->SetName(
        GetLineInfo(),
        qoName,
        m_spContext->Parent().get_Instance(),
        m_spContext->get_RootNamescope()));
}

_Check_return_ HRESULT ObjectWriter::Logic_SetResourceDictionaryItemsProperty(_In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList, _In_ bool fIsDictionaryWithKeyProperty)
{
    HRESULT hr = S_OK;
    bool hasDeferred = false;
    std::shared_ptr<XamlQualifiedObject> spCollection;

    if (!m_spContext->Current().exists_Instance())
    {
        IFC(Logic_CreateWithCtor());
    }
    IFCEXPECT(m_spContext->Current().exists_Instance());

    spCollection = m_spContext->Current().get_Collection();

    // attempt to defer the dictionary items
    IFC(m_spRuntime->DeferResourceDictionaryItems(
        GetLineInfo(),
        this,
        spCollection,
        spNodeList,
        fIsDictionaryWithKeyProperty,
        hasDeferred));

    if (hasDeferred)
    {
        // disable nested dictionary deferral
        if (!fIsDictionaryWithKeyProperty)
        {
            DisableResourceDictionaryDefer();
        }
    }
    else
    {
        std::shared_ptr<XamlReader> spReader;

        // fetch the reader for the deferred list
        IFC(spNodeList->get_Reader(spReader));

        // playback the stream
        while ((hr = spReader->Read()) == S_OK)
        {
            IFC(this->WriteNode(spReader->CurrentNode()));
        }
        // scope will be popped by WriteEndMemberCore()
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ObjectWriter::Logic_SetTemplateProperty(_In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList)
{
    std::shared_ptr<XamlQualifiedObject> spTemplateContent;
    std::shared_ptr<XamlType> spXamlType;
    std::shared_ptr<XamlProperty> spXamlProperty;
    std::shared_ptr<XamlQualifiedObject> spInstance;

    IFCEXPECT_RETURN(m_spContext->Current().exists_Member() && m_spContext->Current().exists_Instance());

    spXamlType = m_spContext->Current().get_Type();
    spXamlProperty = m_spContext->Current().get_Member();
    spInstance = m_spContext->Current().get_Instance();

    IFC_RETURN(m_spRuntime->DeferTemplateContent(
        GetLineInfo(),
        spXamlType,
        spXamlProperty,
        spInstance,
        spNodeList,
        spTemplateContent));

    m_qoLastInstance = spTemplateContent;

    return S_OK;
}

// Checks to see if there's a CustomRuntimeData available, and if so, tells the
// runtime to set it.
_Check_return_ HRESULT ObjectWriter::Logic_CheckAndSetCustomRuntimeData()
{
    if (m_spCustomWriterManager->IsCustomRuntimeDataAvailable())
    {
        auto currentInstance = m_spContext->Current().get_Instance() ?
            m_spContext->Current().get_Instance() : m_spContext->Current().get_Collection();

        IFC_RETURN(m_spRuntime->SetCustomRuntimeData(
            GetLineInfo(),
            m_spContext->Current().get_Type(),
            currentInstance,
            m_spCustomWriterManager->GetAndClearCustomRuntimeData(),
            m_spCustomWriterManager->GetAndClearSubWriterResult(),
            m_spContext->get_RootNamescope(),
            std::shared_ptr<XamlQualifiedObject>(),
            m_spObjectWriterCallbacks));
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriter::ExpandTemplateForInitialValidation(_In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList)
{
    std::shared_ptr<XamlSavedContext> spSavedContext;
    std::shared_ptr<ObjectWriter> spTemplateObjectWriter;
    std::shared_ptr<XamlReader> spNodeReader;

    ObjectWriterSettings settings;
    settings.set_ExpandTemplates(true);

    IFC_RETURN(m_spContext->GetSavedContext(spSavedContext));
    IFC_RETURN(ObjectWriter::Create(spSavedContext, settings, spTemplateObjectWriter));
    IFC_RETURN(spNodeList->get_Reader(spNodeReader));

    while (spNodeReader->Read() == S_OK)
    {
        IFC_RETURN(spTemplateObjectWriter->WriteNode(spNodeReader->CurrentNode()));
    }

    // Assume that the returned root is pegged, and ensure that the flag is set.  This, in turn,
    // will ensure that the root is un-pegged.  For most Xaml parsing, we need to return the
    // root pegged to the managed caller, so that the managed caller can un-peg it.
    // But in this case, since we only expanded the template for validation and are throwing
    // it away, we don't want to leak the tree (see bug 85233).

    std::shared_ptr<XamlQualifiedObject> qoResult = spTemplateObjectWriter->get_Result();
    qoResult->SetHasPeggedManagedPeer();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:   Starts skipping processing of XamlNodes based on whether
//              the current property is one that should be replaced.
//
//  Notes:      This function simulates having seen the WriteMember and WriteValue
//              nodes for the replacement value, but not for the WriteEndMember.
//              The reason that we don't do it for the WriteEndMember is that
//              we expect get that node at a later time, and when we do, we
//              will just pass it through.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ObjectWriter::InitiatePropertyReplacementIfNeeded(const std::shared_ptr<XamlProperty>& spXamlProperty)
{
    // if we are already skipping, we don't need to *start* skipping
    if (m_skipStage == spsNotSkipping)
    {
        if (m_spContext->Current().exists_ReplacementPropertyValues())
        {
            std::shared_ptr<XamlQualifiedObject> qoValue;
            IFC_RETURN(m_spContext->Current().GetAndRemoveReplacementPropertyValue(spXamlProperty, qoValue));

            // if we found a value.
            if (!!qoValue)
            {
                // start skip
                m_skipStage = spsSkipping;
                m_skipDepth = 1;

                // simulate the processing that would have occurred
                // if the replacement value were encountered in the actual
                // XamlNode stream.
                IFC_RETURN(WriteMemberCore(spXamlProperty));
                IFC_RETURN(WriteValueCore(qoValue));
            }

        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Acquires the property bag for the current
//
//  Notes:
//      Assumes the following about the structure of the context stack:
//      --  m_spContext->Current()->get_Instance() contains the key.
//      --  m_spContext->Parent().get_Instance() contains the object that will
//          have the resource values set.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ObjectWriter::GetResourcePropertyBag()
{
    SP_MapPropertyToQO spPropertyBag;

    IFC_RETURN(m_spRuntime->GetResourcePropertyBag(
        GetLineInfo(),
        m_spContext->get_MarkupExtensionContext(),
        m_spContext->Current().get_Instance(),
        m_spContext->Parent().get_Type(),
        m_spContext->Parent().get_Instance(),
        spPropertyBag));

    if (spPropertyBag)
    {
        m_spContext->Parent().set_ReplacementPropertyValues(spPropertyBag);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update the skip depth based on the current operation, and
//      indicate whether the we should continue skipping
//
//  Notes:
//
//------------------------------------------------------------------------
void ObjectWriter::UpdateSkipDepthForResourceReplacement(
    XamlNodeType operationType)
{
    if (m_skipStage != spsNotSkipping)
    {
        // we're skipping so we have to keep track of the depth.
        switch (operationType)
        {
            case XamlNodeType::xntStartObject:
            case XamlNodeType::xntStartProperty:
            {
                m_skipDepth++;
                break;
            }
            case XamlNodeType::xntEndObject:
            case XamlNodeType::xntEndProperty:
            {
                m_skipDepth--;
                break;
            }
            default:
            {
                // do nothing
            }
        }
        if (m_skipDepth == 0)
        {
            m_skipStage = spsNotSkipping;
        }
    }
}

_Check_return_ HRESULT ObjectWriter::ApplyRemainingResourceProperties()
{
    if (m_spContext->Current().exists_ReplacementPropertyValues())
    {
        SP_MapPropertyToQO spRemainingPropertiesMap = m_spContext->Current().get_ReplacementPropertyValues();

        for (MapPropertyToQO::iterator iterRemainingProperties = spRemainingPropertiesMap->begin();
            iterRemainingProperties != spRemainingPropertiesMap->end();
            ++iterRemainingProperties)
        {
            IFC_RETURN(WriteMemberCore(iterRemainingProperties->first));
            IFC_RETURN(WriteValueCore(iterRemainingProperties->second));
            IFC_RETURN(WriteEndMemberCore());
        }

    }

    return S_OK;

}

// This is called by WriteNode when it sees that an error has occurred.  It
// should not be called by anyone else.
_Check_return_ HRESULT ObjectWriter::ProcessError()
{
    // Only try to report an unhandled error if we're the top level parse (which
    // we can tell by checking whether there are previous object writer
    // contexts available)
    if (!m_spContext->HasPreviousObjectWriterContext())
    {
        IErrorService *pErrorService = NULL;
        std::shared_ptr<ParserErrorReporter> spParserErrorReporter;
        IError* pLastError = NULL;
        IFC_RETURN(m_spErrorService->GetReportedError(spParserErrorReporter, &pErrorService, &pLastError));
        if (spParserErrorReporter && !pLastError)
        {
            // Report an unhandled error
            // TODO: Use a more specific error message once we're no longer under UI Freeze
            IFC_RETURN(spParserErrorReporter->SetError(AG_E_UNKNOWN_ERROR, GetLineInfo().LineNumber(), GetLineInfo().LinePosition()));
        }
    }

    return S_OK;
}

// This method exists solely to support CDeferredKeys::PlayXaml()
_Check_return_ HRESULT ObjectWriter::ForceAssignmentToParentCollection(_In_ const std::shared_ptr<XamlQualifiedObject>& parentCollection)
{
    m_spContext->Parent().set_Instance(parentCollection);
    m_spContext->Parent().set_Collection(parentCollection);

    if (!m_spContext->Current().exists_Instance())
    {
        IFC_RETURN(Logic_CreateWithCtor());
    }

    IFC_RETURN(m_spRuntime->EndInit(
        GetLineInfo(),
        m_spContext->Current().get_Type(),
        m_spContext->Current().get_Instance()));

    // Ensure markup extension instances are added as resolved values.
    // With the following example, looking for "ButtonBackground" in the
    // dictionary at run time should return the brush.
    // <ResourceDictionary>
    //     <Color x:Key="SystemBaseLowColor">#33FFFFFF</Color>
    //     <SolidColorBrush x:Key="SystemControlBackgroundBaseLowBrush" Color="{StaticResource SystemBaseLowColor}" />
    //     <StaticResource x:Key="ButtonBackground" ResourceKey="SystemControlBackgroundBaseLowBrush" />
    // </ResourceDictionary>
    bool bIsMarkupExtension = false;
    IFC_RETURN(m_spContext->Current().get_Type()->IsMarkupExtension(bIsMarkupExtension));
    if (bIsMarkupExtension)
    {
        IFC_RETURN(Logic_ProvideValue());
    }

    IFC_RETURN(Logic_DoAssignmentToParentCollection());

    m_spContext->Parent().clear_Instance();
    m_spContext->Parent().clear_Collection();

    return S_OK;
}

void ObjectWriter::DisableResourceDictionaryDefer()
{
    if (m_spDeferringWriter)
    {
        m_spDeferringWriter->DisableResourceDictionaryDefer();
    }
}

void ObjectWriter::EnableResourceDictionaryDefer()
{
    if (m_spDeferringWriter)
    {
        m_spDeferringWriter->EnableResourceDictionaryDefer();
    }
}

std::shared_ptr<ObjectWriterNodeList> ObjectWriter::GetNodeList()
{
    return m_spRuntime->GetNodeList();
}

_Check_return_ HRESULT ObjectWriter::GetSchemaContext(_Out_ std::shared_ptr<XamlSchemaContext>& rspOut) const
{
    // TODO: public?
    RRETURN(m_spContext->get_SchemaContext(rspOut));
}

bool ObjectWriter::IsSkippingForConditionalScope() const
{
    return !m_bIsEncoding && m_spContext->IsInConditionalScope(true);
}
