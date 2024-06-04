// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Include the XmlParser error code
#include "LineInfo.h"
#include "XamlScannerNode.h"
#include "XamlScannerStack.h"
#include "XamlSortedAttributes.h"
#include "XamlTextReaderSettings.h"

class XamlParserContext;
class ParserErrorContext;
class XamlPropertyName;
class XamlProperty;
class XamlText;
class XamlQualifiedName;
class XamlNamespace;
class XamlParserContext;
class ReaderString;
class CWinReader;

struct XamlCachedAttribute
{
    XamlCachedAttribute(_In_ const xstring_ptr& inssPrefix, _In_ const xstring_ptr& inssLocalName, const XamlLineInfo& inLineInfo)
        : m_ssPrefix(inssPrefix)
        , m_ssLocalName(inssLocalName)
        , m_LineInfo(inLineInfo)
    {

    }
    xstring_ptr m_ssPrefix;
    xstring_ptr m_ssLocalName;
    XamlLineInfo m_LineInfo;
};

// Stores a candidate ignorable attribute prefix and text
struct XamlCandidateIgnorableAttribute
{
    XamlCandidateIgnorableAttribute(_In_ const xstring_ptr& inssPrefix, _In_ const xstring_ptr& inssAttributeText)
        : m_ssPrefix(inssPrefix)
        , m_ssAttributeText(inssAttributeText)
    {

    }
    xstring_ptr m_ssPrefix;
    xstring_ptr m_ssAttributeText;
};

// Reads the XAML to parse from an XML reader and tokenizes it into
// primitive markup elements to be used by the XamlPullParser.
class XamlScanner
{
public:

    // Initializes a new instance of the XamlScanner class.  It takes the same
    // XamlParserContext used to initialize the XamlPullParser (which the two
    // share to manage XML namespace declarations, among other things), the XML
    // reader containing the XAML to parse, and settings used to specify the set
    // of features supported when parsing.  After constructing the XamlScanner,
    // you should call Init() to setup necessary internal state.
    XamlScanner(
                const std::shared_ptr<XamlParserContext>& spParserContext,
                std::unique_ptr<CWinReader>&& xmlReader,
                const XamlTextReaderSettings& settings);

    // Destroy this instance of the XamlScanner.
    ~XamlScanner();

    // Initialize the internal state of the XamlScanner.
    _Check_return_ HRESULT Init();

    // Read the next XamlScannerNode from the XML reader.  It returns S_OK when
    // a node was read, S_FALSE when there are no more nodes to read, or an
    // error if it found malformed XAML.  The XamlScannerNode isn't provided
    // directly, but its properties can be accessed through the various getters
    // on XamlScanner.
    _Check_return_ HRESULT Read();

    // Get the type of the next node to be read.  It uses the same node queue as
    // XamlScanner::Read and its return values indicate the same conditions.
    _Check_return_ HRESULT Peek(
        _Out_ XamlScannerNode::ScannerNodeType& sntNextNodeType,
        _Out_ std::shared_ptr<XamlType>& spNextType);

    const XamlScannerNode& get_CurrentNode()
    {
        ASSERT(m_CurrentNode.get_NodeType() != XamlScannerNode::sntNone);
        return m_CurrentNode;
    }

    bool FoundXKeyDirective();

private:
    // Fill the scanner queue with nodes.
    _Check_return_ HRESULT LoadQueue();

    _Check_return_ HRESULT DoXmlRead();
    _Check_return_ HRESULT PullXmlNode();
    _Check_return_ HRESULT ReadElement();
    _Check_return_ HRESULT ReadAttribute(const XamlScannerNode& elementNode);
    _Check_return_ HRESULT ReadEndElement();
    _Check_return_ HRESULT ReadTextOrWhitespace(bool bIsWhitespace);
    _Check_return_ HRESULT ReadPrefix(_Out_ xstring_ptr* pstrOutPrefix);
    _Check_return_ HRESULT ReadLocalName(_Out_ xstring_ptr* pstrOutLocalName);
    _Check_return_ HRESULT CacheAttributePrefixAndLocalName(_In_ const xstring_ptr& inPrefix, _In_ const xstring_ptr& inName);
    _Check_return_ HRESULT GetCachedAttributePrefixAndLocalName(_Out_ xstring_ptr* pstrOutPrefix, _Out_ xstring_ptr* pstrOutName);
    void ClearAttributeNameCache();
    _Check_return_ HRESULT PushNodeToQueue(const XamlScannerNode& inNode);
    _Check_return_ HRESULT EnqueueTextNode();
    _Check_return_ HRESULT PrivateReadText(_Out_ xstring_ptr* pstrOutssText);
    _Check_return_ HRESULT PrivateReadTextCore(_Inout_ ReaderString *pReaderString);
    _Check_return_ HRESULT PrivateReadTextInitializeXamlText(const std::shared_ptr<XamlText>& inXamlText);

    _Check_return_ HRESULT ReadPropertyElement(
                const std::shared_ptr<XamlPropertyName>& inPropertyName,
                const std::shared_ptr<XamlType>& inCurrentType,
                bool bIsEmptyTag,
                bool& outbIsElementIgnored);

    _Check_return_ HRESULT VerifyNoAttributesOnPropertyElement();
    _Check_return_ HRESULT ReadObjectElement(
                const std::shared_ptr<XamlQualifiedName>& name,
                bool bIsEmptyTag,
                bool& outbIsElementIgnored);
    _Check_return_ HRESULT ReadObjectElement_DirectiveProperty(
                const std::shared_ptr<XamlProperty>& inProperty,
                XamlScannerNode& node);
    _Check_return_ HRESULT ReadObjectElement_Object(
                _In_ const xstring_ptr& inPrefix,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                _In_ const xstring_ptr& inName,
                bool bIsEmptyTag,
                bool& outbIsElementIgnored);
    _Check_return_ HRESULT ProcessAttributes(const XamlScannerNode& elementNode);
    _Check_return_ HRESULT PreProcessAttributes();
    _Check_return_ HRESULT AddNamespaceFromCurrentAttributeIfExists();
    _Check_return_ HRESULT SkipNodesIfNamespaceIsIgnored(const std::shared_ptr<XamlNamespace>& inNamespace, bool bIsEmptyTag, bool& outbIsIgnored);
    _Check_return_ HRESULT SkipIgnoredElement(bool bIsEmptyTag);
    _Check_return_ HRESULT ValidateNamespacePrefixAndSkipIgnoredNodes(_In_ const xstring_ptr& inPrefix, std::shared_ptr<XamlNamespace>& outNamespace, bool bIsEmptyTag, bool& outbIsIgnored);
    _Check_return_ HRESULT SkipIgnoredNodes(
                _In_ const xstring_ptr& inPrefix,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                bool bIsEmptyTag,
                bool& outbIsIgnored);

    // Error Functions
    _Check_return_ HRESULT GetErrorService(std::shared_ptr<ParserErrorReporter>& outErrorService);
    _Check_return_ HRESULT ReportXmlLiteError(HRESULT errorHR);
    _Check_return_ HRESULT ReportError(XUINT32 errorCode);
    _Check_return_ HRESULT ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1);
    _Check_return_ HRESULT ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1, _In_ const xstring_ptr& inssParam2);
    _Check_return_ HRESULT ReportUndeclaredNamespacePrefixError(_In_ const xstring_ptr& inssPrefix)
    {
        if (!inssPrefix.IsNullOrEmpty())
        {
            RRETURN(ReportError(AG_E_PARSER2_UNDECLARED_PREFIX, inssPrefix));
        }
        else
        {
            RRETURN(ReportError(AG_E_PARSER2_NO_DEFAULT_NAMESPACE));
        }
    }


    bool HaveAccumulatedText()
    {
        bool bIsEmpty = false;
        VERIFYHR(m_AccumulatedText->get_IsEmpty(bIsEmpty));
        return !bIsEmpty;
    }

private:
    static const XUINT32 STARTING_STRING_BUILDER_SIZE = 16;

    std::shared_ptr<XamlParserContext> m_ParserContext;
    std::shared_ptr<XamlText> m_AccumulatedText;
    // Specifies a set of features to support when parsing XAML.
    XamlTextReaderSettings m_ReaderSettings;
    std::vector<XamlCandidateIgnorableAttribute> m_candidateIgnorablePrefixes;
    std::vector<std::shared_ptr<XamlNamespace>> m_unresolvedConditionalNamespaces;

    XamlLineInfo m_WorkingLineInfo;

    XamlScannerStack m_ScannerStack;
    xqueue<XamlScannerNode> m_ScannerQueue;
    xvector<XamlCachedAttribute> m_AttributePrefixNameCache;
    xvector<XamlCachedAttribute>::iterator m_AttributePrefixNameCacheIterator;

    // The current node is the one that external entities query against
    XamlScannerNode m_CurrentNode;
    // The working node is the one that we use while we are in the progress
    // of adding a node to the queue.
    XamlScannerNode m_WorkingNode;
    XamlSortedAttributes m_Attributes;

    std::unique_ptr<CWinReader> m_XmlReader;

    // A value indicating whether the XamlScanner has already found the first
    // start of an element.  This flag is used to automatically include default
    // namespaces via XamlTextReaderSettings before the first time the scanner
    // needs to find a namespace.  The default value is false.
    bool m_bFoundFirstStartElement;
};

