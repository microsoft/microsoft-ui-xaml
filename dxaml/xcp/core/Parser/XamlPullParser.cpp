// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Reads the XAML to parse from the XamlScanner one token at a time and
//      parses it into a stream of XamlNodes that can be serialized by a
//      XamlWriter (to an object graph, etc.).

#include "precomp.h"

#include "XamlPredicateHelpers.h"

// Initializes a new instance of the XamlPullParser class.
XamlPullParser::XamlPullParser(
            _In_ const std::shared_ptr<XamlParserContext>& inContext,
            _In_ const std::shared_ptr<XamlScanner>& inScanner)
    : m_ParserContext(inContext)
    , m_Scanner(inScanner)
    , m_Validator(inContext)
{
}

// Get the next XamlNode in the parse stream.  Parse should be called until
// exhaustion.  It returns S_OK if it successfully parsed the next node, S_FALSE
// if it has finished parsing, or an error code if it encountered a parse error.
_Check_return_ HRESULT XamlPullParser::Parse(
    // Reference to the XamlNode that was just parsed, unless there was an error
    _Out_opt_ XamlNode& outNode)
{
    HRESULT hr = S_OK;
    XamlNodeType parserNodeType = XamlNodeType::xntNone;

    // Continue reading from the scanner until we have enough XamlScannerNodes
    // to create a XamlNode or the scanner has exhausted the input.  Note that
    // we don't have to read from the scanner at all if previous calls to Parse
    // added more than one node to the queue.
    while ((hr == S_OK) && (m_NodeQueue.size() == 0))
    {
        IFC(m_Scanner->Read());
        if (hr == S_OK)
        {
            XamlParserState::fParseActionDelegate action = nullptr;
            XamlParserState::fParseActionDelegate postTransitionAction = nullptr;

            // Transition to the next parse state based on the token just read
            IFC(m_ParserState.Transition(
                        m_Scanner->get_CurrentNode().get_NodeType(),
                        &action,
                        &postTransitionAction));
            // Perform any necessary operations after transitioning from one parse state to
            // another, but before the action associated with the transition has been invoked.
            if (postTransitionAction != nullptr)
            {
                IFC(postTransitionAction(this));
            }
            IFC(action(this));

            // No codepaths should cause an S_FALSE to be leaked, as doing so would cause odd behavior.
            // Fail in retail, and ASSERT() in chk.
            IFCEXPECT_ASSERT(hr != S_FALSE);
            hr = S_OK;
        }
    }

    // Add the EndOfStream node when we've exhausted the input
    if (hr == S_FALSE)
    {
        IFC(Logic_EndOfStream());
    }

    // Provide the next node from the queue in the outNode parameter
    if (m_NodeQueue.size() > 0)
    {
        IFC(m_NodeQueue.front(outNode));
        IFC(m_NodeQueue.pop());

        // Get the node type so we can synchronize hr with EndOFStream nodes
        parserNodeType = outNode.get_NodeType();

        // Validate that the XamlNode is safe to return
        IFC(m_Validator.ShowNode(outNode));
    }

Cleanup:
    // Make sure we return S_FALSE when we return the EndOfStream node (we can
    // have hr out of sync when an EndOfStream XamlNode is added to the queue
    // but not returned until a later call to Parse)
    if ((hr == S_OK) && (parserNodeType == XamlNodeType::xntEndOfStream))
    {
        ASSERT(m_NodeQueue.size() == 0);
        hr = S_FALSE;
    }

    // TODO: Internal parser error.

    RRETURN(hr);
}

XamlParserState::ParseState XamlPullParser::CurrentState()
{
    return m_ParserState.CurrentState();
}

XamlParserState::ParseState XamlPullParser::PreviousState()
{
    return m_ParserState.PreviousState();
}

void XamlPullParser::SetLineInfoOnWorkingNode()
{
    SetLineInfoOnWorkingNode(m_Scanner->get_CurrentNode().get_LineInfo());
}

void XamlPullParser::SetLineInfoOnWorkingNode(const XamlLineInfo& inLineInfo)
{
    m_WorkingNode.set_LineInfo(inLineInfo);
}

_Check_return_ HRESULT XamlPullParser::EnqueueWorkingNode()
{
    SetLineInfoOnWorkingNode();
    RRETURN(InternalEnqueueNode(m_WorkingNode));
}

_Check_return_ HRESULT XamlPullParser::EnqueueWorkingNode(const XamlLineInfo& inLineInfo)
{
    SetLineInfoOnWorkingNode(inLineInfo);
    RRETURN(InternalEnqueueNode(m_WorkingNode));
}

_Check_return_ HRESULT XamlPullParser::AcceptNode(const XamlNode& node)
{
    // This function is here, for the MEParser to call back into
    // so that it pushes directly onto the main parse queue.
    //
    // Since we currently parse the ME in one pass and generate
    // all it's nodes. (Currently for simpilicity, and they are generally
    // small).
    //
    // This avoids the ME parser having a queue too and then enqueuing onto
    // that, then dequeueing and reenqueuing on to the main parser queue.
    //

    return InternalEnqueueNode(node);
}

_Check_return_ HRESULT XamlPullParser::InternalEnqueueNode(const XamlNode& node)
{
    // Non-virtual implementation.
     return m_NodeQueue.push(node);
    // TODO: Possibly in debug null out the working node.
}

_Check_return_ HRESULT XamlPullParser::Logic_PrefixDefinition()
{
    RRETURN(m_PendingNamespaces.push_back(PrefixNamespacePair(
                    m_Scanner->get_CurrentNode().get_Prefix(),
                    m_Scanner->get_CurrentNode().get_TypeNamespace(),
                    m_Scanner->get_CurrentNode().get_LineInfo())));
}

_Check_return_ HRESULT XamlPullParser::Logic_EndOfAttributes()
{
    m_WorkingNode.InitEndOfAttributesNode();
    IFC_RETURN(EnqueueWorkingNode());
    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_EndOfStream()
{
    m_WorkingNode.InitEndOfStreamNode();
    IFC_RETURN(EnqueueWorkingNode());
    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_EndElement()
{
    IFC_RETURN(PopImplicitNodesIfRequired());

    switch (m_ParserContext->get_CurrentKind())
    {
    case XamlParserFrame::ekPropertyElement:
        m_ParserContext->set_CurrentInPropertyElement(false);
        RRETURN(Logic_EndMember());
    case XamlParserFrame::ekEmptyElement:
    case XamlParserFrame::ekElement:
        RRETURN(Logic_EndObject());
    default:
        IFC_RETURN(E_UNEXPECTED);

    }

    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_EndObject()
{
    // Save the type away before popping the scope.
    std::shared_ptr<XamlType> xamlType(m_ParserContext->get_CurrentType());

    // ZPTD: Can the context perhaps push the type to the parent with this temporary?

    m_WorkingNode.InitEndObjectNode();
    IFC_RETURN(EnqueueWorkingNode());

    if (m_ParserContext->get_CurrentInConditionalElement())
    {
        m_ParserContext->set_CurrentInConditionalElement(false);

        m_WorkingNode.InitEndConditionalScopeNode();
        IFC_RETURN(EnqueueWorkingNode());
    }

    m_ParserContext->PopScope();

    if (!m_ParserContext->IsStackEmpty())
    {
        m_ParserContext->set_CurrentPreviousChildType(xamlType);
    }

    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_EndMember()
{
    // TODO: Does this need a flag to say if it's an attrib or prop element?

    // ZPTD: Use a clear_Foo method instead.
    m_ParserContext->set_CurrentMember(std::shared_ptr<XamlProperty>());
    // TODO: Any other things need to be set?
    m_ParserContext->set_CurrentInItemsProperty(false);
    m_ParserContext->set_CurrentInInitProperty(false);
    m_ParserContext->set_CurrentInImplicitContent(false);
    m_WorkingNode.InitEndMemberNode();
    IFC_RETURN(EnqueueWorkingNode());

    if (m_ParserContext->get_CurrentInConditionalProperty())
    {
        m_ParserContext->set_CurrentInConditionalProperty(false);

        m_WorkingNode.InitEndConditionalScopeNode();
        IFC_RETURN(EnqueueWorkingNode());
    }

    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_StartElement()
{
    return Logic_StartObject(
        m_Scanner->get_CurrentNode().get_Type(),
        m_Scanner->get_CurrentNode().get_Prefix(),
        XamlParserFrame::ekElement);
}

_Check_return_ HRESULT XamlPullParser::Logic_StartEmptyElement()
{
    return Logic_StartObject(
        m_Scanner->get_CurrentNode().get_Type(),
        m_Scanner->get_CurrentNode().get_Prefix(),
        XamlParserFrame::ekEmptyElement);
}

_Check_return_ HRESULT XamlPullParser::Logic_StartPropertyElement()
{
    // TODO: Logic for items and init properties needed here.

    if (m_ParserContext->get_CurrentInPropertyElement())
    {
        std::shared_ptr<ParserErrorReporter> errorReporter;

        IFC_RETURN(m_ParserContext->GetErrorService(errorReporter));
        IFC_RETURN(errorReporter->SetError(
                    AG_E_PARSER2_NESTED_PROP_ELEM,
                    m_Scanner->get_CurrentNode().get_LineInfo().LineNumber(),
                    m_Scanner->get_CurrentNode().get_LineInfo().LinePosition()));
        IFC_RETURN(E_FAIL);
    }

    // Properties can appear both above and below an element's content like:
    //      <Element>
    //          <Element.Property1>...</Element.Property1>
    //          <Child />
    //          <Child />
    //          <Element.Property2>...</Element.Property2>
    //      </Element>
    // When properties appear below the content, we need to close the XamlNodes
    // implicitly created to contain child items so the rest of the properties
    // are correctly associated with the element.
    IFC_RETURN(PopImplicitNodesIfRequired());

    ASSERT(!!m_Scanner->get_CurrentNode().get_PropertyElement());
    m_ParserContext->set_CurrentInPropertyElement(true);
    IFC_RETURN(Logic_StartMember(
        m_Scanner->get_CurrentNode().get_PropertyElement(),
        m_Scanner->get_CurrentNode().get_Prefix()));


    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_Attribute()
{
    RRETURN(Logic_AttributeOrDirective(FALSE));
}

_Check_return_ HRESULT XamlPullParser::Logic_Directive()
{
    RRETURN(Logic_AttributeOrDirective(TRUE));
}

_Check_return_ HRESULT XamlPullParser::Logic_AttributeOrDirective(bool bIsDirective)
{
    bool bLooksLikeMarkupExtension = false;

    if (!bIsDirective)
    {
        m_ParserContext->set_CurrentSeenRealProperties(true);
    }

    IFC_RETURN(Logic_StartMember(
        m_Scanner->get_CurrentNode().get_PropertyAttribute(),
        m_Scanner->get_CurrentNode().get_Prefix()));
    IFC_RETURN(m_Scanner->get_CurrentNode().get_PropertyAttributeText()->get_LooksLikeAMarkupExtension(bLooksLikeMarkupExtension))

    if (bLooksLikeMarkupExtension)
    {
        xstring_ptr ssText;

        // NOTE: This doesn't need to be get_AttributeText (though it wouldn't hurt)
        // because we already determined it "LooksLikeMarkupExtension" which means
        // implicitly it didn't look like "{}....."
        IFC_RETURN(m_Scanner->get_CurrentNode().get_PropertyAttributeText()->get_Text(&ssText));

        {
            MePullParser meParser(
                        this,
                        m_ParserContext,
                        ssText,
                        m_WorkingNode.get_LineInfo());

            IFC_RETURN(meParser.Parse());
        }
    }
    else
    {
        // Unescape if it's an attribute.
        if (!bIsDirective)
        {
            IFC_RETURN(m_Scanner->get_CurrentNode().get_PropertyAttributeText()->UnescapeMEEscapingIfRequired());
        }

        IFC_RETURN(Logic_AttributeValueNode(m_Scanner->get_CurrentNode().get_PropertyAttributeText()));
    }

    IFC_RETURN(Logic_EndMember());


    return S_OK;
}

// Parse the literal text content between the start tag and end tag of an
// element (i.e., <TextBlock>This is Text Content.</TextBlock>).  This method
// includes logic from the System.Xaml parser's P_ElementContent.
//
// The literal text may be the representation an initialization property like
// <sys:Int32>42</sys:Int32> which just invokes the type's text syntax, a
// content property like <Button>My Content</Button> which implicitly sets
// the property, or an items property like
//   <ListBox>
//     First Item
//     Second Item
//     Third Item
//   </ListBox>
// which implicitly adds each item to the collection.
//
// Note that <TextBlock>Some Text</TextBlock> is actually using a content
// property with a collection type where the text content will be wrapped using
// the collection's defined content wrapper (i.e., TextBlock's ContentProperty
// is the Inlines collection which has a ContentWrapper type of Run which has
// the ContentProperty Text of type string).
_Check_return_ HRESULT XamlPullParser::Logic_TextContent()
{
    if (m_Scanner->get_CurrentNode().get_NodeType() == XamlScannerNode::sntText)
    {
        bool bIsEmpty = false;                 // Whether the text is empty

        bool bIsTextInitialization = false;    // Whether we are performing an implicit text initialization
        bool bIsContentProperty = false;       // Whether we are creating an implicit content property

        bool bIsSpacePreserved = false;        // Whether the text explicitly requires whitespace preservation
        xstring_ptr spTrimmedText;              // The trimmed text
        bool bInImplicitContent = m_ParserContext->get_CurrentInImplicitContent();

        // TODO: Compat: The System.Xaml parser stores any prefix definitions until any necessary preamble can be emitted

        // If we can determine the text content is empty or ignorable whitespace
        // then we'll ignore it and bail out early
        IFC_RETURN(m_Scanner->get_CurrentNode().get_TextContent()->get_IsEmpty(bIsEmpty));
        if (!bIsEmpty)
        {
            // TODO: checking IsEmpty should be enough here.
            IFC_RETURN(Logic_IsDiscardableWhitespace(m_Scanner->get_CurrentNode().get_TextContent(), bIsEmpty));
        }
        if (bIsEmpty)
        {
            return S_OK;
        }

        // Get the necessary state from the parser's context to determine how we
        // should classify the text content

        // First we emit any implicit preamble nodes that may be necessary for
        // the current type's text initialization, content property, or items
        // property.
        if (!m_ParserContext->get_CurrentInItemsProperty()
                    && !m_ParserContext->get_CurrentInUnknownContent())
        {
            std::shared_ptr<XamlProperty> spContentProperty;
            std::shared_ptr<XamlTextSyntax> spTextSyntax;   // Text syntax for the current type
            bool bCanAcceptString = false;             // Whether a content property can accept a string

            IFC_RETURN(m_ParserContext->get_CurrentType()->get_ContentProperty(spContentProperty));
            // ZPTD: This boolean logic is redundant

            // For text nodes, we first check if it's a content property, then
            // text initialization, then an items property, and finally resort
            // to an unknown content property if all else failed
            if (!m_ParserContext->get_CurrentInPropertyElement() && spContentProperty)
            {
                IFC_RETURN(CanAcceptString(spContentProperty, bCanAcceptString));
                if (bCanAcceptString)
                {
                    bIsContentProperty = TRUE;
                }
            }

            if (!m_ParserContext->get_CurrentInPropertyElement() && !bIsContentProperty)
            {
                if (!m_ParserContext->get_CurrentSeenRealProperties())
                {
                    IFC_RETURN(m_ParserContext->get_CurrentType()->get_TextSyntax(spTextSyntax));
                    if (spTextSyntax)
                    {
                        // If we haven't processed any properties and have text
                        // syntax available, then we can consider this text
                        // initialization (i.e. <sys:Int32>42</sys:Int32>)
                        bIsTextInitialization = TRUE;
                    }
                }
            }

            if (!m_ParserContext->get_CurrentInPropertyElement()
                        && !bIsTextInitialization && !bIsContentProperty)
            {
                bool bIsCollection = false;
                bool bIsDictionary = false;
                IFC_RETURN(m_ParserContext->get_CurrentType()->IsCollection(bIsCollection));
                IFC_RETURN(m_ParserContext->get_CurrentType()->IsDictionary(bIsDictionary));

                if (bIsCollection || bIsDictionary)
                {
                    // If this is the first item in a collection, emit an
                    // implicit items collection property
                    IFC_RETURN(Logic_StartItemsProperty(m_ParserContext->get_CurrentType()));
                }
                else
                {
                    // If all else fails, we have a unknown content property
                    bIsContentProperty = TRUE;
                }
            }

            // Emit the implicit content property (but not more than one for
            // multiple contiguous unknown content objects and values)
            if (bIsContentProperty
                        && !m_ParserContext->get_CurrentInUnknownContent())
            {
                IFC_RETURN(Logic_StartContentProperty(spContentProperty));
                IFC_RETURN(Logic_CheckForStartGetCollectionFromMember());
            }
        }

        // TODO: Compat: The System.Xaml parser emits any stored prefix definitions before the element is emitted

        // If whitespace doesn't need to be preserved
        IFC_RETURN(m_Scanner->get_CurrentNode().get_TextContent()->get_IsSpacePreserved(bIsSpacePreserved));
        if (!bIsSpacePreserved)
        {
            // Trim the whitespace from the text
            IFC_RETURN(Logic_ApplyFinalTextTrimming(m_Scanner->get_CurrentNode().get_TextContent(), &spTrimmedText));
            if (spTrimmedText.GetCount() == 0)
            {
                return S_OK;
            }
            else
            {
                // Use the trimmed text as the text content
                IFC_RETURN(m_Scanner->get_CurrentNode().get_TextContent()->set_Text(spTrimmedText));
            }
        }

        if (bIsTextInitialization)
        {
            IFC_RETURN(Logic_StartInitProperty(m_ParserContext->get_CurrentType()));
        }

        // TODO: Compat: The System.Xaml parser emits the necessary nodes for an XData preamble

        IFC_RETURN(Logic_TextValueNode(m_Scanner->get_CurrentNode().get_TextContent()));

        // TODO: Compat: The System.Xaml parser emits the necessary nodes to close the XData preamble

        // TODO: The System.Xaml parser does this for anything known that's not in a collection
        // We must close the implicit preamble created in the case of text initialization or implicit content property for simple text values
        if (bIsTextInitialization ||
            (bInImplicitContent == FALSE &&                     // We were not in implicit content
            m_ParserContext->get_CurrentInImplicitContent() &&  // We entered implicit content - this is what tells us that an implicit content property was created
            !m_ParserContext->get_CurrentInItemsProperty() &&   // We aren't in a collection or dictionary
            !m_ParserContext->get_CurrentInPropertyElement()))  // We aren't in an explicit content property
        {
            IFC_RETURN(Logic_EndMember());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_TextValueNode(const std::shared_ptr<XamlText>& inXamlText)
{
    m_WorkingNode.InitTextValueNode(inXamlText);
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_AttributeValueNode(const std::shared_ptr<XamlText>& inXamlText)
{
    m_WorkingNode.InitTextValueNode(inXamlText);
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}
_Check_return_ HRESULT XamlPullParser::Logic_CheckForStartGetCollectionFromMember()
{
    std::shared_ptr<XamlType> currentPropertyType;
    bool bIsCollection = false;
    bool bIsDictionary = false;

    IFC_RETURN(m_ParserContext->get_CurrentMember()->get_Type(currentPropertyType));

    if (currentPropertyType)
    {
        IFC_RETURN(currentPropertyType->IsCollection(bIsCollection));
        IFC_RETURN(currentPropertyType->IsDictionary(bIsDictionary));

        if (bIsCollection || bIsDictionary)
        {
            bool fEmitPreamble = false;

            // If the collection property is Readonly then "retrieve" the collection.
            if (m_ParserContext->get_CurrentMember()->IsReadOnly())
            {
                fEmitPreamble = TRUE;
            }
            // OR if the Value isn't assignable to the Collection emit the preable.
            else
            {
                if (m_Scanner->get_CurrentNode().get_NodeType() == XamlScannerNode::sntText)
                {
                    fEmitPreamble = TRUE;
                }
                else
                {
                    bool fIsAssignableFromValue = false;

                    IFC_RETURN(currentPropertyType->IsAssignableFrom(
                                m_Scanner->get_CurrentNode().get_Type(),
                                fIsAssignableFromValue));

                    if (!fIsAssignableFromValue)
                    {
                        // UNLESS: the Value is a Markup extension, then it is assumed that
                        // the ProvidValue type will be AssignableFrom.
                        if (m_Scanner->get_CurrentNode().get_Type())
                        {
                            bool bIsMarkupExtension = false;

                            if (!m_Scanner->get_CurrentNode().get_Type()->IsUnknown())
                            {
                                IFC_RETURN(m_Scanner->get_CurrentNode().get_Type()->IsMarkupExtension(bIsMarkupExtension));

                                if (!bIsMarkupExtension)
                                {
                                    fEmitPreamble = TRUE;
                                }
                                else
                                {
                                    std::shared_ptr<XamlSchemaContext> spSchemaContext;

                                    IFC_RETURN(m_ParserContext->get_SchemaContext(spSchemaContext));

                                    // The presence of x:Key signals the addition to the collection
                                    fEmitPreamble = m_Scanner->FoundXKeyDirective();
                                }
                            }
                        }
                    }
                }
            }

            if (fEmitPreamble)
            {
                IFC_RETURN(Logic_StartGetObjectFromMember(currentPropertyType));
                IFC_RETURN(Logic_StartItemsProperty(currentPropertyType));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_StartGetObjectFromMember(const std::shared_ptr<XamlType>& inPropertyType)
{
    // Remember the element kind of the real node, so we
    // can push it down through the implicit node.
    // TODO: looks like this isn't correct.  verify and remove the comment
    // IFC(m_ParserContext->get_CurrentKind(elementKind));

    // The current namescope belongs to the ObjectFromMember that we're about to parse,
    // but the context we're about to save belongs to the implicit node, so we don't actually
    // want to push the namescope with the rest of the current context
    m_ParserContext->PushScope(false);

    // TODO: see comment above
    // IFC(m_ParserContext->set_CurrentKind(elementKind));
    m_ParserContext->set_CurrentKind(XamlParserFrame::ekElement);
    m_ParserContext->set_CurrentType(inPropertyType);
    m_ParserContext->set_CurrentInCollectionFromMember(true);
    m_WorkingNode.InitStartObjectNode(
        inPropertyType,
        true,
        inPropertyType->IsUnknown());
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}

// Perform any final leading or trailing whitespace trimming of literal text
// depending on the elements around it.
_Check_return_ HRESULT XamlPullParser::Logic_ApplyFinalTextTrimming(
    _In_ const std::shared_ptr<XamlText>& spText,
    _Out_ xstring_ptr* pstrTrimmed)
{
    xstring_ptr spOriginal;   // The original text
    bool bIsSpacePreserved = false;    // Whether whitespace is preserved in the string
    bool bTrimStart = false;           // Whether we'll trim leading whitespace
    bool bTrimEnd = false;             // Whether we'll trim trailing whitespace

    IFC_RETURN(spText->get_IsSpacePreserved(bIsSpacePreserved));
    if (!bIsSpacePreserved)
    {
        XamlScannerNode::ScannerNodeType sntNextNodeType = XamlScannerNode::sntNone;
        std::shared_ptr<XamlType> spNextType;
        bool bTrimSurroundingWhitespace = false;

        // Peek at the next node type
        IFC_RETURN(m_Scanner->Peek(sntNextNodeType, spNextType));

        // Trim trailing space from the text if it's the last bit of content.
        // EndElement, and EndPropertyElement, and PropertyElement all end
        // literal content.
        // TODO: Compat: System.XAML also includes EmptyPropertyElement which we don't have
        if (sntNextNodeType == XamlScannerNode::sntEndTag ||
            sntNextNodeType == XamlScannerNode::sntPropertyElement)
        {
            bTrimEnd = TRUE;
        }

        // If the text is the first thing (before any element or the previous
        // element should TrimSurroundingWhitespace), then trim the leading
        // whitespace.
        if (m_ParserContext->get_CurrentPreviousChildType())
        {
            IFC_RETURN(m_ParserContext->get_CurrentPreviousChildType()->TrimSurroundingWhitespace(bTrimSurroundingWhitespace));
        }

        if (!m_ParserContext->get_CurrentPreviousChildType() || bTrimSurroundingWhitespace)
        {
            bTrimStart = TRUE;
        }

        // If the next element wants to TrimSurroundingWhitespace, then trim the
        // trailing whitespace of the text.
        if (sntNextNodeType == XamlScannerNode::sntElement ||
            sntNextNodeType == XamlScannerNode::sntEmptyElement)
        {
            IFC_RETURN(spNextType->TrimSurroundingWhitespace(bTrimSurroundingWhitespace))
            if (spNextType && bTrimSurroundingWhitespace)
            {
                bTrimEnd = TRUE;
            }
        }
    }

    // Trim the text
    IFC_RETURN(spText->get_Text(&spOriginal));
    IFC_RETURN(XamlText::TrimWhitespace(spOriginal, bTrimStart, bTrimEnd, pstrTrimmed));

    return S_OK;
}


_Check_return_ HRESULT XamlPullParser::Logic_StartContentProperty(const std::shared_ptr<XamlProperty>& inContentProperty)
{
    std::shared_ptr<XamlProperty> propertyToSet;

    if (!inContentProperty)
    {
        std::shared_ptr<UnknownProperty> unknownProperty;

        // TODO: hardcoded strings
        // ZPTD: Is this necessary?
        std::shared_ptr<XamlSchemaContext> schemaContext;

        IFC_RETURN(m_ParserContext->get_SchemaContext(schemaContext));

        // Mark the current parser frame as being in an uknown content property
        m_ParserContext->set_CurrentInUnknownContent(true);

        // TODO: This may not be the correct create function.
        // TODO: Is setting attachable to FALSE the right thing?
        IFC_RETURN(UnknownProperty::Create(
                    schemaContext,
                    c_strUnknownContent,
                    m_ParserContext->get_CurrentType(),
                    FALSE,
                    unknownProperty));
        propertyToSet = unknownProperty;
    }
    else
    {
        propertyToSet = inContentProperty;
    }

    m_ParserContext->set_CurrentInImplicitContent(true);
    m_ParserContext->set_CurrentMember(propertyToSet);

    m_WorkingNode.InitStartMemberNode(propertyToSet);
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}

// TODO: no use for the param?  remove it if not.
_Check_return_ HRESULT XamlPullParser::Logic_StartInitProperty(const std::shared_ptr<XamlType>& inType)
{
    std::shared_ptr<ImplicitProperty> spInitProperty;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;

    IFC_RETURN(m_ParserContext->get_SchemaContext(spSchemaContext));
    IFC_RETURN(spSchemaContext->get_InitializationProperty(spInitProperty));

    m_ParserContext->set_CurrentInInitProperty(true);
    m_ParserContext->set_CurrentMember(spInitProperty);

    m_WorkingNode.InitStartMemberNode(spInitProperty);

    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}


_Check_return_ HRESULT XamlPullParser::Logic_StartItemsProperty(const std::shared_ptr<XamlType>& inCollectionType)
{
    std::shared_ptr<ImplicitProperty> itemsProperty;
    std::shared_ptr<XamlSchemaContext> schemaContext;

    IFC_RETURN(m_ParserContext->get_SchemaContext(schemaContext));
    IFC_RETURN(schemaContext->get_ItemsProperty(inCollectionType, itemsProperty));

    m_ParserContext->set_CurrentInItemsProperty(true);
    m_ParserContext->set_CurrentMember(itemsProperty);

    m_WorkingNode.InitStartMemberNode(itemsProperty);

    // TODO: There is a note here that says no line info for implicit properties.
    // EnqueueWorkingNode will set the line number. Need to review whether this matters
    // or pass a bool, or make sure it refers to the parent location.
    //
    // Also, there is an inconsistency with the prefix defs. This location may have to
    // pass a flag to say "suppress outputing saved prefix definitions", since they
    // belong to the first item in the collection and we need to push out this implicit
    // property in front.
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_StartMember(
    _In_ const std::shared_ptr<XamlProperty>& inXamlProperty,
    _In_ const xstring_ptr& xmlnsPrefix)
{
    bool bSeenRealProperties = m_ParserContext->get_CurrentSeenRealProperties();

    m_ParserContext->set_CurrentMember(inXamlProperty);

    // TODO: Review this flag 'bSeenRealProperties' and determine whether
    // or not we should be overwriting the previous value like this.
    // ZPTD: BUG?? This looks wrong, it's calculating the OR but not using it?
    bSeenRealProperties |= (!inXamlProperty->IsDirective());
    m_ParserContext->set_CurrentSeenRealProperties(!inXamlProperty->IsDirective());

    // Check if the namespace mapped to the prefix was declared conditionally and, if so,
    // retrieve the conditional predicate so it can be stored on the resulting XamlNode
    std::shared_ptr<Parser::XamlPredicateAndArgs> conditionalPredicate;
    std::shared_ptr<XamlNamespace> xamlNamespace = m_ParserContext->FindNamespaceByPrefix(xmlnsPrefix);
    if (xamlNamespace->IsConditional())
    {
        conditionalPredicate = xamlNamespace->get_XamlPredicateAndArgs();
        m_ParserContext->set_CurrentInConditionalProperty(true);

        m_WorkingNode.InitStartConditionalScopeNode(conditionalPredicate);
        IFC_RETURN(EnqueueWorkingNode());
    }

    m_WorkingNode.InitStartMemberNode(inXamlProperty);
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   EmitImplicitPreambleNodesIfRequired
//
//  Synopsis:
//      Alters the structure of the parse stack and emits nodes depending
//      on a few criteria
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlPullParser::EmitImplicitPreambleNodesIfRequired(
            _In_ const std::shared_ptr<XamlType>& inXamlType)
{
    if (!m_ParserContext->IsStackEmpty())
    {
        bool bIsElementContent = false;

        IFC_RETURN(IsInElementContent(bIsElementContent));
        if (bIsElementContent)
        {
            // Logic from P_ElementContent()
            if (!m_ParserContext->get_CurrentInItemsProperty())
            {
                bool bIsCollection = false;
                bool bIsDictionary = false;

                IFC_RETURN(m_ParserContext->get_CurrentType()->IsCollection(bIsCollection));
                IFC_RETURN(m_ParserContext->get_CurrentType()->IsDictionary(bIsDictionary));

                if (bIsCollection || bIsDictionary)
                {
                    //yield return Logic_StartItemsProperty(currentType);
                    IFC_RETURN(Logic_StartItemsProperty(m_ParserContext->get_CurrentType()));
                }
                else
                {
                    std::shared_ptr<XamlProperty> spContentProperty;
                    IFC_RETURN(m_ParserContext->get_CurrentType()->get_ContentProperty(spContentProperty));

                    if (!spContentProperty)
                    {
                        std::shared_ptr<ParserErrorReporter> errorReporter;
                        xstring_ptr spTypeName;
                        xstring_ptr spContentTypeName;

                        if (!m_ParserContext->get_CurrentType()->IsUnknown())
                        {
                            IFC_RETURN(m_ParserContext->get_CurrentType()->get_Name(&spTypeName));
                        }

                        if (!inXamlType->IsUnknown())
                        {
                            IFC_RETURN(inXamlType->get_Name(&spContentTypeName));
                        }

                        IFC_RETURN(m_ParserContext->GetErrorService(errorReporter));
                        IFC_RETURN(errorReporter->SetError(
                            AG_E_PARSER_INVALID_CONTENT,
                            m_Scanner->get_CurrentNode().get_LineInfo().LineNumber(),
                            m_Scanner->get_CurrentNode().get_LineInfo().LinePosition(),
                            spTypeName,
                            spContentTypeName
                        ));

                        // TODO: This should report an error if content property is null and
                        // it's content inside another element.
                        IFC_RETURN(E_FAIL);
                    }

                    IFC_RETURN(Logic_StartContentProperty(spContentProperty));
                    IFC_RETURN(Logic_CheckForStartGetCollectionFromMember());
                }
            }
        }
        else
        {
            // Logic from P_PropertyContent
            if (!m_ParserContext->get_CurrentInCollectionFromMember())
            {
                IFC_RETURN(Logic_CheckForStartGetCollectionFromMember());
            }
        }
    }

    return S_OK;
}


_Check_return_ HRESULT XamlPullParser::EmitPendingNamespaceNodes()
{
    for (xvector<PrefixNamespacePair>::iterator it = m_PendingNamespaces.begin(); it != m_PendingNamespaces.end(); ++it)
    {
        m_WorkingNode.InitAddNamespaceNode((*it).m_Prefix, (*it).m_Namespace);
        IFC_RETURN(EnqueueWorkingNode((*it).m_LineInfo));
    }

    m_PendingNamespaces.clear();
    return S_OK;

}
_Check_return_ HRESULT XamlPullParser::Logic_StartObject(
    _In_ const std::shared_ptr<XamlType>& inXamlType,
    _In_ const xstring_ptr& xmlnsPrefix,
    _In_ const XamlParserFrame::ElementKind& inElementKind)
{
    bool bIsPropertyElement = false;

    if (!m_ParserContext->IsStackEmpty())
    {
        IFC_RETURN(EmitImplicitPreambleNodesIfRequired(inXamlType));
    }

    // These have to be after any pre-amble nodes, but before
    // the object node.
    IFC_RETURN(EmitPendingNamespaceNodes());

    // Remember this before we push the scope.
    bIsPropertyElement = m_ParserContext->get_CurrentInPropertyElement();
    m_ParserContext->PushScope();

    // Let the next node know it's parent is a property, so it can
    // tell what type of content it is.
    m_ParserContext->set_ParentIsPropertyElement(bIsPropertyElement);
    m_ParserContext->set_CurrentType(inXamlType);
    m_ParserContext->set_CurrentKind(inElementKind);

    // Check if the namespace mapped to the prefix was declared conditionally and, if so,
    // emit a XamlNode containing the associated conditional predicate
    std::shared_ptr<Parser::XamlPredicateAndArgs> conditionalPredicate;
    std::shared_ptr<XamlNamespace> xamlNamespace = m_ParserContext->FindNamespaceByPrefix(xmlnsPrefix);
    if (xamlNamespace->IsConditional())
    {
        conditionalPredicate = xamlNamespace->get_XamlPredicateAndArgs();
        m_ParserContext->set_CurrentInConditionalElement(true);

        m_WorkingNode.InitStartConditionalScopeNode(conditionalPredicate);
        IFC_RETURN(EnqueueWorkingNode());
    }

    m_WorkingNode.InitStartObjectNode(
        inXamlType,
        false,
        inXamlType->IsUnknown());
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}

// Gets a value indicating whether whitespace is discardable at this phase in
// the parsing.  Here we discard whitespace between property elements but keep
// it between object elements for collections that accept it.  Discarding
// trailing whitespace in collections cannot be decided here. See
// Logic__ApplyFinalTextTrimming.
_Check_return_ HRESULT XamlPullParser::Logic_IsDiscardableWhitespace(
    _In_ const std::shared_ptr<XamlText>& spText,
    _Out_ bool& bIsDiscardable)
{
    // Note: Some of the conventions in this method aren't
    // standard (like the repeated goto Cleanup jumps).  This was done to
    // maintain as much parity with the WPF implementation of
    // Logic_IsDiscardableWhitespace as possible because of the complex logic
    // below.

    bool bIsWhitespaceOnly = false;

    IFC_RETURN(spText->get_IsWhiteSpaceOnly(bIsWhitespaceOnly));
    if (!bIsWhitespaceOnly)
    {
        // Obviously we can't discard text that isn't all whitespace
        bIsDiscardable = FALSE;
        return S_OK;
    }
    else
    {
        std::shared_ptr<XamlProperty> spCurrentMember(m_ParserContext->get_CurrentMember());

        if (spCurrentMember && spCurrentMember->IsUnknown())
        {
            // Treat unknown members as whitespace significant collections in
            // order to preserve as much information as possible
            bIsDiscardable = FALSE;
            return S_OK;
        }
        else if (m_ParserContext->get_CurrentInItemsProperty())
        {
            std::shared_ptr<XamlType> spCollectionType;
            bool bIsWhitespaceSignificant = false;

            spCollectionType = m_ParserContext->get_CurrentType();

            if (spCollectionType)
            {
                IFC_RETURN(spCollectionType->IsWhitespaceSignificantCollection(bIsWhitespaceSignificant));
            }

            // If we're in a collection, then the collection type indicates
            // whether whitespace can be discarded via the (currently hardcoded)
            // WhitespaceSignificantCollection attribute.
            bIsDiscardable = spCollectionType && !bIsWhitespaceSignificant;
            return S_OK;
        }
        else
        {
            XamlScannerNode::ScannerNodeType sntNextNodeType = XamlScannerNode::sntNone;
            std::shared_ptr<XamlType> spNextType;
            std::shared_ptr<XamlTextSyntax> spCurrentTypeTextSyntax;
            bool bIsSpacePreserved = false;

            IFC_RETURN(m_Scanner->Peek(sntNextNodeType, spNextType));
            IFC_RETURN(spText->get_IsSpacePreserved(bIsSpacePreserved));
            IFC_RETURN(m_ParserContext->get_CurrentType()->get_TextSyntax(spCurrentTypeTextSyntax));

            // Whitepsace by itself does not start content.  The
            // whitespace between the start element and the first property
            // element is not content, but the whitespace between start
            // element and the first child element (i.e., other content) is
            // content.
            if (sntNextNodeType == XamlScannerNode::sntElement)
            {
                std::shared_ptr<XamlType> spCurrentMemberType;
                bool bIsCurrentMemberWhitespaceSignificant = false;
                bool bIsCurrentTypeWhitespaceSignificant = false;


                // If there's no current member, then we're implicitly in the
                // content property and should use that as the member
                if (!spCurrentMember)
                {
                    IFC_RETURN(m_ParserContext->get_CurrentType()->get_ContentProperty(spCurrentMember));
                }

                if (spCurrentMember)
                {
                    IFC_RETURN(spCurrentMember->get_Type(spCurrentMemberType));
                }

                // ZPTD: This boolean logic looks wrong.

                // Whitespace is only significant if our enclosing type demands
                // it via the WhitespaceSignificantCollection attribute.
                if (spCurrentMemberType)
                {
                    IFC_RETURN(spCurrentMemberType->IsWhitespaceSignificantCollection(bIsCurrentMemberWhitespaceSignificant));
                }

                ASSERT(!!m_ParserContext->get_CurrentType());
                IFC_RETURN(m_ParserContext->get_CurrentType()->IsWhitespaceSignificantCollection(bIsCurrentTypeWhitespaceSignificant));

                if (spCurrentMember && bIsCurrentMemberWhitespaceSignificant)
                {
                    bIsDiscardable = FALSE;
                    return S_OK;
                }
                else if (!spCurrentMember && bIsCurrentTypeWhitespaceSignificant)
                {
                    bIsDiscardable = FALSE;
                    return S_OK;
                }
            }
            // Whitespace can also start content if space is preserved and it's
            // at the end of an element and ...
            else if (bIsSpacePreserved && sntNextNodeType == XamlScannerNode::sntEndTag)
            {
                std::shared_ptr<XamlProperty> spContentProperty;

                IFC_RETURN(m_ParserContext->get_CurrentType()->get_ContentProperty(spContentProperty));

                // ... it's by itself in a property element with no other
                // children
                if (spCurrentMember)
                {
                    if (!m_ParserContext->get_CurrentPreviousChildType())
                    {
                        bIsDiscardable = FALSE;
                        return S_OK;
                    }
                }
                // ... it's in an element with a string content property
                else if (spContentProperty)
                {
                    std::shared_ptr<XamlType> spContentPropertyType;
                    std::shared_ptr<XamlSchemaContext> spSchemaContext;
                    std::shared_ptr<XamlType> spStringType;
                    bool bIsWhitespaceSignificant = false;

                    IFC_RETURN(spContentProperty->get_Type(spContentPropertyType));
                    IFC_RETURN(m_ParserContext->get_SchemaContext(spSchemaContext));
                    IFC_RETURN(spSchemaContext->get_StringXamlType(spStringType));

                    if (XamlType::AreEqual(spContentPropertyType, spStringType))
                    {
                        bIsDiscardable = FALSE;
                        return S_OK;
                    }

                    IFC_RETURN(spContentPropertyType->IsWhitespaceSignificantCollection(bIsWhitespaceSignificant));
                    if (bIsWhitespaceSignificant)
                    {
                        bIsDiscardable = FALSE;
                        return S_OK;
                    }
                }
                // ... it's in a type-convertible element
                else if (spCurrentTypeTextSyntax && spCurrentTypeTextSyntax->get_TextSyntaxToken().GetProviderKind() != tpkUnknown) // TODO: System.XAML also checks  XamlParserContext::get_CurrentForcedToUseConstructor which we don't implement
                {
                    bIsDiscardable = FALSE;
                    return S_OK;
                }
            }
        }
    }

    // Otherwise, we can discard the whitespace
    bIsDiscardable = TRUE;

    return S_OK;
}

// Determine whether a content property can be set from a string.
_Check_return_ HRESULT XamlPullParser::CanAcceptString(
    _In_ const std::shared_ptr<XamlProperty>& spXamlProperty,
    _Out_ bool& bCanAcceptString)
{
    bool bIsCollection = false;

    std::shared_ptr<XamlType> spPropertyType;
    std::shared_ptr<XamlTextSyntax> spPropertyTextSyntax;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    std::shared_ptr<XamlTextSyntax> spDefaultStringTextSyntax;

    bCanAcceptString = FALSE;

    IFC_RETURN(spXamlProperty->get_Type(spPropertyType));
    IFC_RETURN(spXamlProperty->get_TextSyntax(spPropertyTextSyntax));
    IFC_RETURN(m_ParserContext->get_SchemaContext(spSchemaContext));
    IFC_RETURN(spSchemaContext->get_StringSyntax(spDefaultStringTextSyntax));
    IFC_RETURN(spPropertyType->IsCollection(bIsCollection));

    // TODO: Compat: Check for the default Object Text Syntax as well
    if (spPropertyTextSyntax == spDefaultStringTextSyntax)
    {
        bCanAcceptString = TRUE;
    }
    else if (bIsCollection)
    {
        // If the property type is a collection, we'll check to see if it has
        // a ContentWrapper defined.  The ContentWrapper is a type used to wrap
        // strings so they can be added to the strongly typed collection.  The
        // ContentWrapper type's Content property should be of type string.
        std::shared_ptr<XamlType> spWrapperType;
        IFC_RETURN(spPropertyType->get_ContentWrapper(spWrapperType));
        if (spWrapperType)
        {
            std::shared_ptr<XamlProperty> spWrapperContentProperty;
            IFC_RETURN(spWrapperType->get_ContentProperty(spWrapperContentProperty));
            if (spWrapperContentProperty)
            {
                std::shared_ptr<XamlType>spWrapperContentPropertyType;

                IFC_RETURN(spWrapperContentProperty->get_Type(spWrapperContentPropertyType));
                if (spWrapperContentPropertyType)
                {
                    bool bIsString = false;

                    IFC_RETURN(spWrapperContentPropertyType->IsString(bIsString));

                    if (bIsString)
                    {
                        bCanAcceptString = TRUE;
                    }
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_EndCollectionFromMember()
{
    IFC_RETURN(Logic_EndObject());
    ////IFC(Logic_EndContentProperty());

    return S_OK;
}

_Check_return_ HRESULT XamlPullParser::Logic_EndItemsProperty()
{
    RRETURN(Logic_EndMember());
}

_Check_return_ HRESULT XamlPullParser::Logic_EndContentProperty()
{
    RRETURN(Logic_EndMember());
}

_Check_return_ HRESULT XamlPullParser::PopImplicitNodesIfRequired()
{
    if (m_ParserContext->get_CurrentInItemsProperty())
    {
        IFC_RETURN(Logic_EndItemsProperty());
    }

    if (m_ParserContext->get_CurrentInCollectionFromMember())
    {
        IFC_RETURN(Logic_EndCollectionFromMember());
    }

    if (m_ParserContext->get_CurrentInImplicitContent())
    {
        IFC_RETURN(Logic_EndContentProperty());
    }

    return S_OK;
}




