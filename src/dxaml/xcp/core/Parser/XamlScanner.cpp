// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Reads the XAML to parse from an XML reader and tokenizes it into
//      primitive markup elements to be used by the XamlPullParser.

#include "precomp.h"
#include "ReaderString.h"
#include "XamlUnknownXmlNamespace.h"
#include <WinReader.h>

// Initializes a new instance of the XamlScanner class.  It takes the same
// XamlParserContext used to initialize the XamlPullParser (which the two share
// to manage XML namespace declarations, among other things), the XML reader
// containing the XAML to parse, and settings used to specify the set of
// features supported when parsing.  After constructing the XamlScanner, you
// should call Init() to setup necessary internal state.
XamlScanner::XamlScanner(
    const std::shared_ptr<XamlParserContext>& spParserContext,
    std::unique_ptr<CWinReader>&& xmlReader,
    const XamlTextReaderSettings& settings)
    : m_CurrentNode(XamlLineInfo())
    , m_ParserContext(spParserContext)
    , m_Attributes(spParserContext, settings)
    , m_XmlReader(std::move(xmlReader))
    , m_ReaderSettings(settings)
    , m_bFoundFirstStartElement(FALSE)
{}

XamlScanner::~XamlScanner()
{}

// Initialize the internal state of the XamlScanner.
_Check_return_ HRESULT XamlScanner::Init()
{
    m_AccumulatedText = std::make_shared<XamlText>();
    IFC_RETURN(m_AccumulatedText->Initialize(STARTING_STRING_BUILDER_SIZE));

    return S_OK;
}

// Read the next XamlScannerNode from the XML reader.  It returns S_OK when a
// node was read, S_FALSE when there are no more nodes to read, or an error if
// it found malformed XAML.  The XamlScannerNode isn't provided directly, but
// its properties can be accessed through the various getters on XamlScanner.
_Check_return_ HRESULT XamlScanner::Read()
{
    HRESULT hr = S_OK;

    IFC(LoadQueue());
    if (hr == S_FALSE)
    {
        goto Cleanup;
    }

    IFC(m_ScannerQueue.front(m_CurrentNode));
    IFC(m_ScannerQueue.pop());

Cleanup:
    RRETURN(hr);
}

// Get the type of the next node to be read.  It uses the same node queue as
// XamlScanner::Read and its return values indicate the same conditions.
_Check_return_ HRESULT XamlScanner::Peek(
    _Out_ XamlScannerNode::ScannerNodeType& sntNextNodeType,
    _Out_ std::shared_ptr<XamlType>& spNextType)
{
    sntNextNodeType = XamlScannerNode::sntNone;

    IFC_RETURN(LoadQueue());
    if (m_ScannerQueue.size() > 0)
    {
        XamlScannerNode next;

        // ZPTD: No long need to front like this.
        IFC_RETURN(m_ScannerQueue.front(next));
        sntNextNodeType = next.get_NodeType();
        // You can't call get_Type on things that don't have types.
        if ((sntNextNodeType == XamlScannerNode::sntEmptyElement)
                || (sntNextNodeType == XamlScannerNode::sntElement))
        {
            spNextType = next.get_Type();
        }
        else
        {
            spNextType.reset();
        }

    }

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::GetErrorService(std::shared_ptr<ParserErrorReporter>& outErrorService)
{
    std::shared_ptr<XamlSchemaContext> schemaContext;

    IFC_RETURN(m_ParserContext->get_SchemaContext(schemaContext));
    IFC_RETURN(schemaContext->GetErrorService(outErrorService));

    return S_OK;

}

_Check_return_ HRESULT XamlScanner::ReportXmlLiteError(HRESULT errorHR)
{
    // Maps XmlLite errors to new parser error codes.

    HRESULT hr = S_OK;

    // For now it's a pass through.
    IFC(ReportError(errorHR));

Cleanup:
    RRETURN(errorHR);
}

_Check_return_ HRESULT XamlScanner::ReportError(XUINT32 errorCode)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, m_WorkingLineInfo.LineNumber(), m_WorkingLineInfo.LinePosition()));

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, m_WorkingLineInfo.LineNumber(), m_WorkingLineInfo.LinePosition(), inssParam1));

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1, _In_ const xstring_ptr& inssParam2)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, m_WorkingLineInfo.LineNumber(), m_WorkingLineInfo.LinePosition(), inssParam1, inssParam2));

    return S_OK;
}

// Fill the scanner queue with nodes.
_Check_return_ HRESULT XamlScanner::LoadQueue()
{
    HRESULT hr = S_OK;

    if (m_ScannerQueue.size() == 0)
    {
        IFC(DoXmlRead());

        if (hr == S_FALSE)
        {
            // TODO: Should clear attribute cache here for good measure?
            goto Cleanup;
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        // The error reporter will ignore this error if an error
        // was already recorded, we don't need to check.
        //
        // TODO: Could this be more specific "AG_E_UNKNOWN_SCANNER_ERROR"?
        xstring_ptr ssHrCode;
        // In an error case this string could leak.
        IGNOREHR(xstring_ptr::CreateFromUInt32(hr, &ssHrCode));
        IGNOREHR(ReportError(AG_E_PARSER2_SCANNER_UNKNOWN_ERROR, ssHrCode));
    }

    RRETURN(hr);
}

_Check_return_ HRESULT XamlScanner::DoXmlRead()
{
    HRESULT hr = S_OK;
    while ((m_ScannerQueue.size() == 0) && (hr == S_OK))
    {
        IFC(PullXmlNode());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT XamlScanner::PullXmlNode()
{
    HRESULT hr = S_OK;
    XmlNodeType nodeType = XmlNodeType_None;

    hr = m_XmlReader->Read(&nodeType);

    if (hr == S_FALSE)
    {
        // End of document

    }
    else
    {
        if (FAILED(hr))
        {
            unsigned int lineNumber = 0;
            unsigned int linePosition = 0;

            IGNOREHR(m_XmlReader->GetPosition(&lineNumber, &linePosition));
            m_WorkingLineInfo = XamlLineInfo(lineNumber, linePosition);
            IFC(ReportXmlLiteError(hr));
        }

        switch (nodeType)
        {
        case XmlNodeType_Whitespace:
            IFC(ReadTextOrWhitespace(true));
            break;
        case XmlNodeType_CDATA:
        case XmlNodeType_Text:
            IFC(ReadTextOrWhitespace(false));
            break;
        case XmlNodeType_Element:
            IFC(ReadElement());
            break;
        case XmlNodeType_EndElement:
            IFC(ReadEndElement());
            break;
        case XmlNodeType_DocumentType:
            // The underlying parser no longer fails
            // on a doctype, so now will fail here.
            IFC(ReportXmlLiteError(WC_E_DTDPROHIBITED));
            IFC(static_cast<HRESULT>(WC_E_DTDPROHIBITED));
            break;
        default:
            break;
        };
    }
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT XamlScanner::ReadEndElement()
{
    if (HaveAccumulatedText())
    {
        IFC_RETURN(EnqueueTextNode());
    }

    if (m_ScannerStack.get_CurrentProperty())
    {
        // ZPTD : add a clear method.
        std::shared_ptr<XamlProperty> nullXamlProperty;
        m_ScannerStack.set_CurrentProperty(nullXamlProperty);
    }
    else
    {
        IFC_RETURN(m_ScannerStack.Pop());

        // Sync accumulatedText's preserveSpace flag with the _new_ top of the stack
        IFC_RETURN(m_AccumulatedText->Reset(m_ScannerStack.Depth() > 0 ? m_ScannerStack.get_CurrentXmlSpacePreserve() : FALSE));
    }

    m_WorkingNode.InitEndTag(m_WorkingLineInfo);
    IFC_RETURN(PushNodeToQueue(m_WorkingNode));

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::ReadElement()
{
    bool bIsEmptyTag = true;
    bool bContainsDot = false;
    bool bIsElementIgnored = false;
    xstring_ptr ssPrefix;
    xstring_ptr ssLocalName;

    if (HaveAccumulatedText())
    {
        IFC_RETURN(EnqueueTextNode());
    }

    bIsEmptyTag = m_XmlReader->EmptyElement();
    IFC_RETURN(ReadPrefix(&ssPrefix));
    IFC_RETURN(ReadLocalName(&ssLocalName));
    bContainsDot = (ssLocalName.FindChar(L'.') != xstring_ptr_view::npos);

    if (bContainsDot)
    {
        std::shared_ptr<XamlPropertyName> propertyName;

        IFC_RETURN(XamlPropertyName::Parse(ssPrefix, ssLocalName, propertyName));

        // Ensure the scanner has an item on the stack before we try and read a
        // type from it below
        if (m_ScannerStack.Depth() <= 0)
        {
            IFC_RETURN(ReportError(AG_E_PARSER2_PROPERTY_ELEMENT_AT_ROOT));
            IFC_RETURN(E_FAIL);
        }

        IFC_RETURN(ReadPropertyElement(
                    propertyName,
                    m_ScannerStack.get_CurrentType(),
                    bIsEmptyTag,
                    bIsElementIgnored));
    }
    else
    {
        auto name = std::make_shared<XamlQualifiedName>(ssPrefix, ssLocalName);
        IFC_RETURN(PreProcessAttributes());
        IFC_RETURN(ReadObjectElement(name, bIsEmptyTag, bIsElementIgnored));
    }

    if (bIsElementIgnored)
    {
        return S_OK;
    }

    // Push an endtag onto the stream for empty tags
    // TODO: this is different to the WPF scanner stream, but
    // it simplifies our parser?
    //
    // The scanner node stream is completely internal, but if for
    // some reason we needed to create a compatible scanner node
    // stream, we can just not add this tag here (or anything
    // reading it could ignore or filter any scanner node where
    // type == endtype && empty == true
    if (bIsEmptyTag)
    {
        m_WorkingNode.InitEndTag(m_WorkingLineInfo, TRUE);
        IFC_RETURN(PushNodeToQueue(m_WorkingNode));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::ReadObjectElement(
            const std::shared_ptr<XamlQualifiedName>& inName,
            bool bIsEmptyTag,
            bool& outbIsElementIgnored)
{
    std::shared_ptr<XamlNamespace> xamlNamespace;
    xvector<XamlScannerNode>::iterator attribIteratorEnd;

    // Optionally add the default XML namespaces for Silverlight applications
    // on the start of the first object scanned if the XamlTextReaderSettings
    // specifies it (when XamlTextReaderSettings.get_RequireDefaultNamespace is
    // false)
    if (!m_bFoundFirstStartElement)
    {
        m_bFoundFirstStartElement = TRUE;
        IFC_RETURN(m_ReaderSettings.AddDefaultXmlNamespacePrefixesIfNotRequired(m_ParserContext));

        // TODO: Should we add nodes to the XamlScanner's queue and simulate a namespace declaration, or just initialize them in the XamlParserContext like we're doing now?
    }

    outbIsElementIgnored = FALSE;
    xamlNamespace = m_ParserContext->FindNamespaceByPrefix(inName->get_Prefix());

    if (!xamlNamespace)
    {
        // TODO: I don't think this can ever happen, because
        // xml lite will report it as an error code and we'll error out above.
        IFC_RETURN(ReportUndeclaredNamespacePrefixError(inName->get_Prefix()));
        IFC_RETURN(E_FAIL);
    }
    else
    {
        IFC_RETURN(ReadObjectElement_Object(
                    inName->get_Prefix(),
                    xamlNamespace,
                    inName->get_Name(),
                    bIsEmptyTag,
                    outbIsElementIgnored));

        if (outbIsElementIgnored)
        {
            return S_OK;
        }
    }

    IFC_RETURN(m_Attributes.Reset());
    IFC_RETURN(ProcessAttributes(m_WorkingNode));

    if (m_Attributes.GetBucketSize(XamlSortedAttributes::sakNamespace) > 0)
    {
        // Push the namespaces on first
        attribIteratorEnd = m_Attributes.GetBucketIteratorEnd(XamlSortedAttributes::sakNamespace);
        for (
                    xvector<XamlScannerNode>::iterator it = m_Attributes.GetBucketIteratorBegin(XamlSortedAttributes::sakNamespace);
                    it != attribIteratorEnd;
                    ++it)
        {
            IFC_RETURN(PushNodeToQueue(*it));
        }
    }

    // Push on the element node
    IFC_RETURN(PushNodeToQueue(m_WorkingNode));

    // Now walk through the rest of the buckets.
    for (XUINT32 bucket = XamlSortedAttributes::sakDeferLoadStrategy; bucket < XamlSortedAttributes::sakNumberOfBuckets; bucket++)
    {
        if (m_Attributes.GetBucketSize(static_cast<XamlSortedAttributes::ScannerAttributeKind>(bucket)) > 0)
        {
            bool bIsXmlSpace = (bucket == XamlSortedAttributes::sakXmlSpace);

            attribIteratorEnd = m_Attributes.GetBucketIteratorEnd(static_cast<XamlSortedAttributes::ScannerAttributeKind>(bucket));
            for (
                        xvector<XamlScannerNode>::iterator it = m_Attributes.GetBucketIteratorBegin(static_cast<XamlSortedAttributes::ScannerAttributeKind>(bucket));
                        it != attribIteratorEnd;
                        ++it)
            {
                // Handle xml:space="preserve" (Note: The rest of the parser
                // will treat this directive as a noop.  We just set the
                // IsSpacePreserved flag on XamlText and let it work its way
                // through the rest of the parser.)
                if (bIsXmlSpace)
                {
                    xstring_ptr spText;
                    bool bPreserve = false;
                    bool bIsAccumulatedTextEmpty = false;

                    // Get the value of the xml:space attribute
                    IFC_RETURN(it->get_PropertyAttributeText()->get_Text(&spText));

                    // Save whether or not it's set to "preserve" in the current
                    // element on the scanner stack
                    bPreserve = spText.Equals(XSTRING_PTR_EPHEMERAL(L"preserve"));
                    m_ScannerStack.set_CurrentXmlSpacePreserve(bPreserve);

                    // Make sure the current accumulated text buffer is in sync
                    IFC_RETURN(m_AccumulatedText->get_IsEmpty(bIsAccumulatedTextEmpty));
                    ASSERT(bIsAccumulatedTextEmpty, L"There shouldn't be text in the m_AccumulatedText buffer before xml:space is set.");
                    if (bIsAccumulatedTextEmpty)
                    {
                        IFC_RETURN(m_AccumulatedText->Reset(bPreserve));
                    }
                }

                IFC_RETURN(PushNodeToQueue(*it));
            }
        }
    }

    return S_OK;
}
_Check_return_ HRESULT XamlScanner::ReadObjectElement_Object(
            _In_ const xstring_ptr& inPrefix,
            const std::shared_ptr<XamlNamespace>& inNamespace,
            _In_ const xstring_ptr& inName,
            bool bIsEmptyTag,
            bool& outbIsElementIgnored)
{
    std::shared_ptr<XamlTypeName> xamlTypeName;
    std::shared_ptr<XamlType> xamlType;

    outbIsElementIgnored = FALSE;

    xamlTypeName = std::make_shared<XamlTypeName>(inPrefix, inName);
    IFC_RETURN(m_ParserContext->GetXamlType(xamlTypeName, xamlType));

    if (!xamlType || xamlType->IsUnknown())
    {
        // TODO: This is somewhat redundant since an unknown namespace will
        // return an unknown type and we will recreate it.
        //
        // Need to go back and verify if all the GetXamlType callers can handle
        // getting no type back.
        std::shared_ptr<XamlSchemaContext> schemaContext;

        IFC_RETURN(SkipIgnoredNodes(inPrefix, inNamespace, bIsEmptyTag, outbIsElementIgnored));

        if (outbIsElementIgnored)
        {
            return S_OK;
        }

        IFC_RETURN(m_ParserContext->get_SchemaContext(schemaContext));
        IFC_RETURN(UnknownType::Create(schemaContext, inNamespace, inName, xamlType));
    }

    ASSERT(!!xamlType);

    if (m_ScannerStack.Depth() > 0)
    {
        m_ScannerStack.set_CurrentlyInContent(TRUE);
    }

    if (bIsEmptyTag)
    {
        m_WorkingNode.InitEmptyElement(
                    m_WorkingLineInfo,
                    inPrefix,
                    inNamespace,
                    xamlType);
    }
    else
    {
        m_WorkingNode.InitElement(
                    m_WorkingLineInfo,
                    inPrefix,
                    inNamespace,
                    xamlType);

        IFC_RETURN(m_ScannerStack.Push(xamlType));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::ReadPropertyElement(
            const std::shared_ptr<XamlPropertyName>& inPropertyName,
            const std::shared_ptr<XamlType>& inCurrentType,
            bool bIsEmptyTag,
            bool& outbIsElementIgnored)
{
    outbIsElementIgnored = FALSE;

    // Only the x:Uid attribute is allowed on property elements, so we still
    // scan for attributes but error if anything is found
    IFC_RETURN(PreProcessAttributes());

    xstring_ptr spOwnerPrefix;
    xstring_ptr spOwnerName;
    IFC_RETURN(inPropertyName->get_OwnerPrefix(&spOwnerPrefix));
    IFC_RETURN(inPropertyName->get_OwnerName(&spOwnerName));

    // Owner namespace should be guaranteed to be at least unknownnamespace
    std::shared_ptr<XamlNamespace> spOwnerNamespace = m_ParserContext->FindNamespaceByPrefix(spOwnerPrefix);
    ASSERT(!!spOwnerNamespace);
    auto spOwnerTypeName = std::make_shared<XamlTypeName>(spOwnerPrefix, spOwnerName);
    std::shared_ptr<XamlType> spOwnerType;
    IFC_RETURN(m_ParserContext->GetXamlType(spOwnerTypeName, spOwnerType));

    if (m_ScannerStack.Depth() > 0)
    {
        m_ScannerStack.set_CurrentlyInContent(FALSE);
    }

    if (!spOwnerType || spOwnerType->IsUnknown())
    {
        std::shared_ptr<XamlSchemaContext> schemaContext;

        // We've already resolved the prefix to the namespace so we only
        // need to call skip here
        IFC_RETURN(SkipIgnoredNodes(spOwnerPrefix, spOwnerNamespace, bIsEmptyTag, outbIsElementIgnored));

        if (outbIsElementIgnored)
        {
            return S_OK;
        }

        IFC_RETURN(m_ParserContext->get_SchemaContext(schemaContext));
        IFC_RETURN(UnknownType::Create(schemaContext, spOwnerNamespace, spOwnerName, spOwnerType));
    }

    ASSERT(!!spOwnerType);

    std::shared_ptr<XamlProperty> spProperty;
    bool bIsTypeAssignable = false;
    IFC_RETURN(spOwnerType->IsAssignableFrom(inCurrentType, bIsTypeAssignable));

    if (bIsTypeAssignable)
    {
        IFC_RETURN(m_ParserContext->GetXamlProperty(spOwnerType, inPropertyName->get_Name(), spProperty));
    }

    if (!spProperty)
    {
        IFC_RETURN(m_ParserContext->GetXamlAttachableProperty(spOwnerType, inPropertyName->get_Name(), spProperty));
    }

    if (!spProperty || spProperty->IsUnknown())
    {
        std::shared_ptr<XamlSchemaContext> schemaContext;
        std::shared_ptr<UnknownProperty> unknownProperty;
        std::shared_ptr<XamlNamespace> xamlNamespace;


        // TODO: This case isn't really clear. Is it possible for ssPrefix and spOwnerPrefix to be different?
        // This is intended to find the case where a different prefix is used on the property
        // element than on it's parent element.
        IFC_RETURN(ValidateNamespacePrefixAndSkipIgnoredNodes(
                    inPropertyName->get_Prefix(),
                    xamlNamespace,
                    bIsEmptyTag,
                    outbIsElementIgnored));

        if (outbIsElementIgnored)
        {
            return S_OK;
        }

        IFC_RETURN(m_ParserContext->get_SchemaContext(schemaContext));
        IFC_RETURN(UnknownProperty::Create(
                    schemaContext,
                    inPropertyName->get_Name(),
                    spOwnerType,
                    spOwnerNamespace,
                    TRUE,
                    unknownProperty));

        spProperty = unknownProperty;
    }

    ASSERT(!!spProperty);

    // Reset the attributes, create a node representing this property element,
    // and process the attributes.  We should find no attributes (and the
    // VerifyNoAttributesOnPropertyElement will check this in a few lines),
    // excluding x:Uid which is swallowed by XamlSortedAttributes::Add so it
    // will effectively look like we found no attributes.
    XamlScannerNode node;
    IFC_RETURN(m_Attributes.Reset());
    std::shared_ptr<XamlNamespace> spNamespace = m_ParserContext->FindNamespaceByPrefix(inPropertyName->get_Prefix());
    node.InitPropertyElement(
        m_WorkingLineInfo,
        inPropertyName->get_Prefix(),
        spNamespace,
        spProperty,
        TRUE);
    IFC_RETURN(ProcessAttributes(node));

    if (!bIsEmptyTag)
    {
        m_ScannerStack.set_CurrentProperty(spProperty);
    }

    // Do the check to ensure there are no attributes on the
    // property element.
    IFC_RETURN(VerifyNoAttributesOnPropertyElement());

    m_WorkingNode.InitPropertyElement(
                m_WorkingLineInfo,
                inPropertyName->get_Prefix(),
                spOwnerNamespace,
                spProperty,
                bIsEmptyTag);

    IFC_RETURN(PushNodeToQueue(m_WorkingNode));

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::VerifyNoAttributesOnPropertyElement()
{
    if (m_Attributes.GetSize() > 0)
    {
        // There were attributes.
        IFC_RETURN(ReportError(AG_E_PARSER2_CANT_SET_PROPS_ON_PROP_ELEM));
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::ReadObjectElement_DirectiveProperty(const std::shared_ptr<XamlProperty>& inProperty, XamlScannerNode& node)
{
    // TODO: Not impl
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_ HRESULT XamlScanner::ReadPrefix(_Out_ xstring_ptr* pstrOutPrefix)
{
    ReaderString prefix;
    unsigned int lineNumber = 0;
    unsigned int linePosition = 0;

    // We don't own the buffer we get back here.
    IFC_RETURN(m_XmlReader->GetPrefix(&prefix));
    IGNOREHR(m_XmlReader->GetPosition(&lineNumber, &linePosition));
    m_WorkingLineInfo = XamlLineInfo(lineNumber, linePosition);

    if (prefix.length() == 0)
    {
        // This check is worth it because prefix is likely to be nothing
        // whereas in the general case for names it would never be.
        *pstrOutPrefix = xstring_ptr::EmptyString();
    }
    else
    {
        IFC_RETURN(xstring_ptr::CloneBuffer(prefix, prefix.length(), pstrOutPrefix));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::ReadLocalName(_Out_ xstring_ptr* pstrOutLocalName)
{
    ReaderString localName;

    // We don't own the buffer we get back here.
    IFC_RETURN(m_XmlReader->GetLocalName(&localName));
    // Assumes you always read the line/col with the prefix.
    IFC_RETURN(xstring_ptr::CloneBuffer(localName, localName.length(), pstrOutLocalName));

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::CacheAttributePrefixAndLocalName(_In_ const xstring_ptr& inPrefix, _In_ const xstring_ptr& inName)
{
    return m_AttributePrefixNameCache.push_back(XamlCachedAttribute(inPrefix, inName, m_WorkingLineInfo));
}

void XamlScanner::ClearAttributeNameCache()
{
    m_AttributePrefixNameCache.clear();
}

_Check_return_ HRESULT XamlScanner::GetCachedAttributePrefixAndLocalName(_Out_ xstring_ptr* pstrPrefix, _Out_ xstring_ptr* pstrName)
{
    if (m_AttributePrefixNameCacheIterator != m_AttributePrefixNameCache.end())
    {
        *pstrPrefix = (*m_AttributePrefixNameCacheIterator).m_ssPrefix;
        *pstrName = (*m_AttributePrefixNameCacheIterator).m_ssLocalName;
        m_WorkingLineInfo = (*m_AttributePrefixNameCacheIterator).m_LineInfo;
        return S_OK;
    }

    return E_FAIL;
}

_Check_return_ HRESULT XamlScanner::ReadTextOrWhitespace(bool bIsWhitespace)
{
    xstring_ptr ssText;

    IFC_RETURN(PrivateReadText(&ssText));
    IFC_RETURN(m_AccumulatedText->Paste(ssText, !m_ScannerStack.get_CurrentlyInContent()));

    if (!bIsWhitespace)
    {
        m_ScannerStack.set_CurrentlyInContent(TRUE);
    }

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::PrivateReadText(_Out_ xstring_ptr* pstrOutssText)
{
    ReaderString text;

    IFC_RETURN(PrivateReadTextCore(&text));
    IFC_RETURN(xstring_ptr::CloneBuffer(text, text.length(), pstrOutssText));

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::PrivateReadTextCore(_Inout_ ReaderString *pReaderString)
{
    unsigned int lineNumber = 0;
    unsigned int linePosition = 0;

    IFC_RETURN(m_XmlReader->GetValue(pReaderString));
    IFC_RETURN(m_XmlReader->GetPosition(&lineNumber, &linePosition));
    m_WorkingLineInfo = XamlLineInfo(lineNumber, linePosition);

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::EnqueueTextNode()
{
    bool bIsAllWhitespace = false;
    bool bShouldPreserve = false;

    // Get whether the accumulated text should be initialized to preserve
    // whitespace (which means the element had xml:space="preserve" set)
    bShouldPreserve = m_ScannerStack.get_CurrentXmlSpacePreserve();

    IFC_RETURN(m_AccumulatedText->get_IsWhiteSpaceOnly(bIsAllWhitespace));
    if (!((m_ScannerStack.Depth() == 0) && bIsAllWhitespace))
    {
        m_WorkingNode.InitText(m_WorkingLineInfo, m_AccumulatedText);
        IFC_RETURN(PushNodeToQueue(m_WorkingNode));

        // Clear out the accumulated text for next time.
        m_AccumulatedText = std::make_shared<XamlText>(bShouldPreserve);
        IFC_RETURN(m_AccumulatedText->Initialize(STARTING_STRING_BUILDER_SIZE));
    }
    else
    {
        // TODO: Is this necessary?
        IFC_RETURN(m_AccumulatedText->Reset(bShouldPreserve));
    }

    return S_OK;
}


_Check_return_ HRESULT XamlScanner::ProcessAttributes(const XamlScannerNode& elementNode)
{
    HRESULT hr = S_OK;

    hr = m_XmlReader->FirstAttribute();
    m_AttributePrefixNameCacheIterator = m_AttributePrefixNameCache.begin();

    while (hr == S_OK)
    {
        IFC(ReadAttribute(elementNode));

        hr = m_XmlReader->NextAttribute();
        ++m_AttributePrefixNameCacheIterator;
    }


Cleanup:
    RRETURN(SUCCEEDED(hr) ? S_OK : hr);
}

_Check_return_ HRESULT XamlScanner::PreProcessAttributes()
{
    HRESULT hr = S_OK;

    hr = m_XmlReader->FirstAttribute();
    ClearAttributeNameCache();

    while (hr == S_OK)
    {
        IFC_RETURN(AddNamespaceFromCurrentAttributeIfExists());

        hr = m_XmlReader->NextAttribute();
    }

    // If there were namespace prefixes to ignore we go ahead and ignore them
    // now.
    //
    // Reason this is here is because all namespaces have to be setup first
    // which happens in the preprocess pass. But then the ignoring has to
    // happen before any of the real attributes are parsed.
    //
    // This should only be non-null after pre-processing the first element
    // if it contained an Ignorable attribute.
    for (const auto& candidateIgnorablePrefix : m_candidateIgnorablePrefixes)
    {
        xstring_ptr ssPrefix;
        xstring_ptr ssAttributeText;
        bool bIsMarkupCompatNamespace = false;

        ssPrefix = candidateIgnorablePrefix.m_ssPrefix;
        ssAttributeText = candidateIgnorablePrefix.m_ssAttributeText;

        std::shared_ptr<XamlNamespace> xamlNamespace = m_ParserContext->FindNamespaceByPrefix(ssPrefix);

        IFC_RETURN(m_Attributes.IsMarkupCompatabilityNamespace(
            xamlNamespace,
            bIsMarkupCompatNamespace));

        // This is an actual ignorable prefix and part of the markup
        // compatibility namespace.
        if (bIsMarkupCompatNamespace)
        {
            ASSERT(m_Attributes.IsFirstElement());
            IFC_RETURN(m_Attributes.AddIgnoredNamespaces(ssAttributeText));
        }
    }
    m_candidateIgnorablePrefixes.clear();

    // Now that we've processed the ignored namespaces, we can go ahead and resolve
    // any conditional predicates into concrete types. We can't resolve the predicate
    // type earlier because we need to be able to resolve its xmlns prefix (if there is one)
    // and the prefix may be defined in a later attribute than the attribute defining the
    // conditional namespace.
    for (auto& xamlNamespace : m_unresolvedConditionalNamespaces)
    {
        xamlNamespace->ResolveConditionalPredicate(m_ParserContext);
    }
    m_unresolvedConditionalNamespaces.clear();

    return (SUCCEEDED(hr) ? S_OK : hr);
}

_Check_return_ HRESULT XamlScanner::AddNamespaceFromCurrentAttributeIfExists()
{
    xstring_ptr ssPrefix;
    xstring_ptr ssName;
    xstring_ptr ssAttributeText;

    IFC_RETURN(ReadPrefix(&ssPrefix));
    IFC_RETURN(ReadLocalName(&ssName));
    IFC_RETURN(CacheAttributePrefixAndLocalName(ssPrefix, ssName));

    XamlSortedAttributes::XamlNamespaceKind namespaceKind = XamlSortedAttributes::xnkNotANamespace;
    IFC_RETURN(m_Attributes.IsXmlNamespaceDefinition(ssPrefix, ssName, namespaceKind));

    if (namespaceKind != XamlSortedAttributes::xnkNotANamespace)
    {
        // It is a namespace, so we need to resolve it in the preprocess pass
        // so that things resolve regardless of their order.
        xstring_ptr definingPrefix;
        xstring_ptr uri;
        XamlPropertyNameParser propertyNameParser(ssName);
        std::shared_ptr<XamlPropertyName> propertyName;
        std::shared_ptr<XamlSchemaContext> schemaContext;
        std::shared_ptr<XamlNamespace> xamlNamespace;

        IFC_RETURN(propertyNameParser.ParseXamlPropertyName(ssPrefix, propertyName));
        IFC_RETURN(PrivateReadText(&ssAttributeText));
        IFC_RETURN(m_Attributes.GetXmlNamespaceDefinitionUri(namespaceKind, propertyName, ssAttributeText, &definingPrefix, &uri));
        IFC_RETURN(m_ParserContext->get_SchemaContext(schemaContext));
        IFC_RETURN(schemaContext->GetXamlXmlNamespace(uri, xamlNamespace));

        if (!xamlNamespace)
        {
            auto unknownNamespace = std::make_shared<XamlUnknownXmlNamespace>(schemaContext, uri);
            xamlNamespace = unknownNamespace;
        }

        IFC_RETURN(m_ParserContext->AddNamespacePrefix(definingPrefix, xamlNamespace));

        if (xamlNamespace->IsConditional())
        {
            m_unresolvedConditionalNamespaces.push_back(xamlNamespace);
        }
    }
    else if (m_Attributes.IsFirstElement() && ssName.Equals(XSTRING_PTR_EPHEMERAL(L"Ignorable")))
    {
        // This is a candidate whose prefix and attribute must be processed
        // after all the attributes in the element have been seen.
        IFC_RETURN(PrivateReadText(&ssAttributeText));
        m_candidateIgnorablePrefixes.emplace_back(ssPrefix, ssAttributeText);
    }

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::ReadAttribute(const XamlScannerNode& elementNode)
{
    xstring_ptr ssPrefix;
    xstring_ptr ssName;
    std::shared_ptr<XamlPropertyName> propertyName;
    std::shared_ptr<XamlText> text;

    // These were cached in the pre-process pass.
    IFC_RETURN(GetCachedAttributePrefixAndLocalName(&ssPrefix, &ssName));

    {
        text = std::make_shared<XamlText>(true);
        IFC_RETURN(PrivateReadTextInitializeXamlText(text));

        {
            XamlPropertyNameParser propertyNameParser(ssName);

            IFC_RETURN(propertyNameParser.ParseXamlPropertyName(ssPrefix, propertyName));
            IFC_RETURN(m_Attributes.Add(
                        m_WorkingLineInfo,
                        elementNode,
                        propertyName,
                        text));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::SkipIgnoredElement(bool bIsEmptyTag)
{
    HRESULT hr = S_OK;
    XmlNodeType nodeType = XmlNodeType_None;

    // Start with depth = 1 because we've already eaten the start element
    // that put us in here. Set to one if the current tag is empty, because
    // we've already read it.
    XUINT32 uDepth = bIsEmptyTag ? 0 : 1;

    while (uDepth > 0)
    {
        hr = m_XmlReader->Read(&nodeType);

        if (hr == S_FALSE)
        {
            // End of document - we shouldn't be able to get to the end. If we
            // do the document is unmatched.

            // TODO: Verify if we need a specific error here or
            // whether xmllite handles it.
            IFC(E_UNEXPECTED);
        }
        else
        {
            if (FAILED(hr))
            {
                IFC(ReportXmlLiteError(hr));
                IFC(hr);
            }

            switch (nodeType)
            {
            case XmlNodeType_Whitespace:
            case XmlNodeType_Text:
                break;

            // Only increase the depth of nodes if this element has children.
            case XmlNodeType_Element:
                uDepth += m_XmlReader->EmptyElement() ? 0 : 1;
                break;

            case XmlNodeType_EndElement:
                ASSERT(uDepth > 0);
                uDepth--;
                break;

            case XmlNodeType_DocumentType:
                // The underlying parser no longer fails
                // on a doctype, so now will fail here.
                IFC(ReportXmlLiteError(WC_E_DTDPROHIBITED));
                IFC(static_cast<HRESULT>(WC_E_DTDPROHIBITED));
                break;

            default:
                break;
            };
        }
    }
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT XamlScanner::SkipNodesIfNamespaceIsIgnored(
            const std::shared_ptr<XamlNamespace>& inNamespace,
            bool bIsEmptyTag,
            bool& outbIsIgnored)
{
    outbIsIgnored = FALSE;

    // If a namespace is resolved you can't ignore it
    // and we silently ignore you ignoring it.
    bool bIsNamespaceResolved = inNamespace->get_IsResolved();

    if (!bIsNamespaceResolved)
    {
        xstring_ptr ssUri = inNamespace->get_TargetNamespace();
        if (m_ParserContext->IsNamespaceUriIgnored(ssUri))
        {
            IFC_RETURN(SkipIgnoredElement(bIsEmptyTag));
            outbIsIgnored = TRUE;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::ValidateNamespacePrefixAndSkipIgnoredNodes(
            _In_ const xstring_ptr& inPrefix,
            std::shared_ptr<XamlNamespace>& outNamespace,
            bool bIsEmptyTag,
            bool& outbIsIgnored)
{
    outNamespace = m_ParserContext->FindNamespaceByPrefix(inPrefix);
    IFC_RETURN(SkipIgnoredNodes(inPrefix, outNamespace, bIsEmptyTag, outbIsIgnored));

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::SkipIgnoredNodes(
            _In_ const xstring_ptr& inPrefix,
            const std::shared_ptr<XamlNamespace>& inNamespace,
            bool bIsEmptyTag,
            bool& outbIsIgnored)
{
    if (!inNamespace)
    {
        IFC_RETURN(ReportUndeclaredNamespacePrefixError(inPrefix));
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(SkipNodesIfNamespaceIsIgnored(inNamespace, bIsEmptyTag, outbIsIgnored));

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::PrivateReadTextInitializeXamlText(const std::shared_ptr<XamlText>& inXamlText)
{
    ReaderString text;

    IFC_RETURN(PrivateReadTextCore(&text));
    IFC_RETURN(inXamlText->Initialize(text, text.length()));

    return S_OK;
}

_Check_return_ HRESULT XamlScanner::PushNodeToQueue(const XamlScannerNode& inNode)
{
    RRETURN(m_ScannerQueue.push(inNode));
}

bool XamlScanner::FoundXKeyDirective()
{
    if (!m_Attributes.IsFoundXKeyDirectiveValid())
    {
        bool bFoundXKeyDirective = false;

        // Note that the attributes are reset/flushed on every time we start ReadObjectElement so these attributes belong to the current node
        if (m_Attributes.GetBucketSize(XamlSortedAttributes::sakDirective) > 0)
        {
            xvector<XamlScannerNode>::iterator attribIteratorEnd = m_Attributes.GetBucketIteratorEnd(XamlSortedAttributes::sakDirective);

            // Iterate over directives
            for (
                xvector<XamlScannerNode>::iterator it = m_Attributes.GetBucketIteratorBegin(XamlSortedAttributes::sakDirective);
                it != attribIteratorEnd && !bFoundXKeyDirective;
                ++it)
            {
                bFoundXKeyDirective = (std::static_pointer_cast<DirectiveProperty>(it->get_PropertyAttribute())->get_DirectiveKind() == xdKey);
            }
        }
        m_Attributes.SetFoundXKeyDirective(bFoundXKeyDirective);
    }

    return m_Attributes.GetFoundXKeyDirective();
}

