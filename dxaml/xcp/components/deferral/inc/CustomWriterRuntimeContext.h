// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct XamlQualifiedObject;
class XamlBinaryFormatSubReader2;
class XamlSchemaContext;
class DeferredNodeStreamCollectingWriterResult;
class XamlNamespace;
class ObjectWriterStack;
class INameScope;
class ObjectWriterCallbacksDelegate;

#include <weakref_ptr.h>
#include <INamescope.h>

// Stores all the context necessary to instantiate objects from StreamOffsetTokens.
// Today this will include the XamlOptimizedNodeList, a dictionary of preresolved resource
// dictionary keys, and the needed parser context. Even in XBFv2 this data will likely
// be needed. The XBFv2 decoder will be responsible for creating a similar structure when
// decoding the custom ObjectWriterNode CustomWriter will eventually create and passing
// it to the current instance.
class CustomWriterRuntimeContext
{
public:
    CustomWriterRuntimeContext(
        std::shared_ptr<XamlSchemaContext> schemaContext,
        std::shared_ptr<XamlQualifiedObject> targetType,
        xref_ptr<IPALUri> baseUri,
        xref::weakref_ptr<CDependencyObject> rootInstanceWeakRef,
        std::shared_ptr<XamlBinaryFormatSubReader2> spSubReader,
        std::shared_ptr<ObjectWriterStack> objectWriterStack,
        xref_ptr<INameScope> namescope,
        xstring_ptr xbfHash,
        std::shared_ptr<XamlQualifiedObject> XBindConnector,
        std::shared_ptr<ObjectWriterCallbacksDelegate> objectWriterCallbacks)
        : m_schemaContext(std::move(schemaContext))
        , m_targetType(std::move(targetType))
        , m_baseUri(std::move(baseUri))
        , m_rootInstance(std::move(rootInstanceWeakRef))
        , m_spSubReader(std::move(spSubReader))
        , m_objectWriterStack(std::move(objectWriterStack))
        , m_nameScope(std::move(namescope))
        , m_xbfHash(std::move(xbfHash))
        , m_XBindConnector(std::move(XBindConnector))
        , m_objectWriterCallbacks(std::move(objectWriterCallbacks))
    {}

    std::shared_ptr<XamlQualifiedObject> GetTargetType() const { return m_targetType; }
    std::shared_ptr<XamlSchemaContext> GetSchemaContext() const { return m_schemaContext; }
    xref_ptr<IPALUri> GetBaseUri() const { return m_baseUri; }
    xref_ptr<CDependencyObject> GetRootInstance() const 
    {
        return m_rootInstance.lock();
    }
    std::shared_ptr<XamlBinaryFormatSubReader2> GetReader() const { return m_spSubReader; }
    std::shared_ptr<ObjectWriterStack> GetObjectWriterStack() const { return m_objectWriterStack; }
    const xref_ptr<INameScope> GetNameScope() const { return m_nameScope; }
    const xstring_ptr GetXbfHash() const { return m_xbfHash;  }

    std::shared_ptr<XamlQualifiedObject> GetXBindConnector() const
    {
        return m_XBindConnector;
    }

    std::shared_ptr<ObjectWriterCallbacksDelegate> GetCallbackInterface() const
    {
        return m_objectWriterCallbacks;
    }

private:
    std::shared_ptr<XamlSchemaContext> m_schemaContext;

    // TargetType is the one required ambient property still regularly
    // in use today. If we were deferring a Style this is used for the
    // Setters. If we were deferring a ControlTemplate this is used for
    // the TemplateBindings. 
    std::shared_ptr<XamlQualifiedObject> m_targetType;

    // RootInstance and Connector root for ConnectionId Hookup. This MUST be stored as a weak_ref
    // because it's likely that the root instance contains this element as a child,
    // creating a cycle.
    xref::weakref_ptr<CDependencyObject> m_rootInstance;

    // XBind connector reference returned from code-behind (via ICC2) for compiled binding hookup.
    std::shared_ptr<XamlQualifiedObject> m_XBindConnector;

    // The base uri of what we're parsing. Other places in the system make
    // a distinction between BaseUri and XamlResourceUri. We will ignore that
    // distinction for now, with the caveat that we might need to reintroduce
    // it to support QuirkPropagateUidBaseUrisInDeferredXaml for Win8 app
    // backcompat.
    // DEAD_CODE_REMOVAL ?
    xref_ptr<IPALUri> m_baseUri;

    // Finally the node list itself. Multiple StreamOffsetTokens could
    // have indexes pointing into this list.
    std::shared_ptr<XamlBinaryFormatSubReader2> m_spSubReader;

    // The ObjectWriterStack is kept for dubious reasons. The only concrete reason
    // we're keeping it around is to make nested namespaces available and
    // resource lookup. Ideally we'd create a more reasonable data structure to
    // only store the elements we need here instead of taking the stack, poking
    // out all the instances to avoid circular references, and using it as a
    // husk of its former self.
    std::shared_ptr<ObjectWriterStack> m_objectWriterStack;

    // When we're creating objects within something like a UserControl we need the 
    // namescope to ensure they are available to FindName and other methods that
    // use the runtime name of an element. The specific scenario this is important
    // for is faulting in VSMs in UserControls, where it's important that
    // field-backed elements are set during the UserControl construction, this is
    // implemented using FindName.
    xref_ptr<INameScope> m_nameScope;

    // The callbacks interface is used primarily today to set the TemplateParent. Keep
    // in mind that TemplateParents are not something that can be inferred from the runtime
    // tree, they can only be known at the invocation time of the template expansion: once
    // a template is expanded its elements are just like any others. The TemplateParent is
    // set through this callback delegate interface as it isn't a concept core to parsing, it
    // instead exists as somethign built on top of parsing by the XAML platform. We can't
    // simply rely on GetTemplatedParent of the element that receives the CustomRuntimeData as
    // if the element was the value of a GetValue, read-only property it wasn't actually created
    // by the parser and logically doesn't have a template parent.
    std::shared_ptr<ObjectWriterCallbacksDelegate> m_objectWriterCallbacks;

    xstring_ptr m_xbfHash;
};