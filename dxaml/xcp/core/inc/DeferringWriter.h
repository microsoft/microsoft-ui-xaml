// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlWriter.h"

class ObjectWriterContext;
class XamlOptimizedNodeList;

namespace Parser
{
struct XamlPredicateAndArgs;
}

// The possible states that the DeferringWriter can be in.
enum DeferringMode
{
    // Waiting for a template to start.  This is the initial state of the
    // DeferringWriter.
    dmOff,

    // We've seen a <Foo.Template> start member and we're in the process of
    // storing all the nodes in the Template.  We'll stay in this state until we
    // see the corresponding </Foo.Template> end member.
    dmTemplateDeferring,

    // We've finished storing a template and are waiting for the node list to be
    // collected and stored in a TemplateContent instance.
    dmTemplateReady,

    // We've seen a <ResourceDictionary x:Key="..."> and the implicit items
    // property is being populated. We'll stay in this state until we
    // see the EndMember on the implicit items property.
    dmResourceDictionaryDeferring,

    // We've finished storing a keyed ResourceDictionary's implicit items
    // property and are waiting for the node list to be collected and stored.
    dmResourceDictionaryReady,
};


// The ObjectWriter has an instance of the DeferringWriter that it calls at the
// beginning of each of its Write methods.  The DeferringWriter effectively acts
// as a proxy that will store all of the XamlNodes that are written between the
// <Foo.Template> start member and </Foo.Template> end member.  It is
// implemented as a basic state machine using the DeferringMode enum.
class DeferringWriter final : public XamlWriter
{
private:
    // The current state of the DeferringWriter.  It defaults to dmOff.
    DeferringMode m_eMode;

    // The list of stored XamlNodes.
    std::shared_ptr<XamlOptimizedNodeList> m_spDeferredList;

    // The XamlWriter used to store nodes in the deferred list.
    std::shared_ptr<XamlWriter> m_spDeferredWriter;

    // The number of objects that we have started - but not finished - storing
    // since we began deferring.  This is used to determine when we've finished
    // deferring and can change to the dmTemplateReady state.  Note that this
    // causes nested templates to be included in the node stream like any other
    // object.
    XINT32 m_iDeferredTreeDepth;

    // The ObjectWriterContext context (only used for obtaining the
    // XamlSchemaContext).
    std::shared_ptr<ObjectWriterContext> m_spContext;

    // A value indicating whether the last node written was deferred.
    bool m_fHandled;

    // Prevent ResourceDictionary deferral
    bool m_fDisableResourceDictionaryDefer;

    // The deferred ResourceDictionary has a key property
    bool m_fIsDictionaryWithKeyProperty;

    // For encoding we purposely unquirk the VisualState__DeferredStoryboard property
    // redirection to disable deferred stroyboards.
    bool m_encoding;

public:
    // Initializes a new instance of the DeferringWriter class.
    DeferringWriter(
        _In_ std::shared_ptr<ObjectWriterContext>& spContext,
        _In_ bool encoding);


    // Destroys an instance of the DeferringWriter class.
    ~DeferringWriter() override {};

    // Gets a value indicating whether the last node written was deferred into
    // the node list.
    bool get_Handled() { return m_fHandled; }

    // Gets the mode indicating whether the DeferringWriter is doing nothing,
    // storing nodes, or has a node list ready.
    DeferringMode get_Mode() { return m_eMode; }

    // Gets the XamlNodeList that was stored while deferring the XamlNodes in a
    // template.  This should only be called when the mode is dmTemplateReady.
    // It will clear the stored list and changed the mode to dmOff.
    _Check_return_ HRESULT CollectTemplateList(
        _Out_ std::shared_ptr<XamlOptimizedNodeList>& spXamlNodeList);

    // Gets the XamlNodeList that was stored while deferring the XamlNodes in a
    // ResourceDictionary's Implicit Items property. This should be called only when
    // then mode is dmResourceDictionaryReady.
    // It will clear the stored list and change the mode to dmOff.
    _Check_return_ HRESULT CollectResourceDictionaryList(
        _Out_ std::shared_ptr<XamlOptimizedNodeList>& spXamlNodeList,
        _Out_ bool *pfIsDictionaryWithKeyProperty);

    // Start writing an object.
    _Check_return_ HRESULT WriteObject(
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ bool fFromMember) override;

    // Finish writing an object.
    _Check_return_ HRESULT WriteEndObject() override;

    // Start writing a member.
    _Check_return_ HRESULT WriteMember(_In_ const std::shared_ptr<XamlProperty>& spProperty) override;

    // Finish writing a member.
    _Check_return_ HRESULT WriteEndMember() override;

    // Start writing a conditional XAML section
    _Check_return_ HRESULT WriteConditionalScope(const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs) override;

    // Finish writing a conditional XAML section
    _Check_return_ HRESULT WriteEndConditionalScope() override;

    // Write the value of a member.
    _Check_return_ HRESULT WriteValue(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) override;

    // Write a namespace.
    _Check_return_ HRESULT WriteNamespace(
        _In_ const xstring_ptr& spPrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace) override;

    // Get the XamlSchemaContext associated with the writer.
    _Check_return_ HRESULT GetSchemaContext(
        _Out_ std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext) const override;

    // Disable deferring of resource dictionary
    void DisableResourceDictionaryDefer() { m_fDisableResourceDictionaryDefer = TRUE; }

    // Enable deferring of resource dictionary
    void EnableResourceDictionaryDefer() { m_fDisableResourceDictionaryDefer = FALSE; }
};


