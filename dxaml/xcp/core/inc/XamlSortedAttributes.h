// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlParserContext;
class XamlPropertyName;
class XamlNamespace;
class ParserErrorReporter;
class XamlScannerNode;
class XamlText;

class XamlSortedAttributes
{
public:

    // The ordering is relevant here, since they are used as indicies and
    // looped.
    enum ScannerAttributeKind
    {
        sakNamespace,

        sakDeferLoadStrategy,
        sakLoad,
        // x:Uid must be processed before all attributes
        // so that the resource values to be replaced can be
        // understood before we get to the properties that could
        // have their values replaced.
        sakUid,

        sakName,
        sakDirective,
        sakXmlSpace,
        sakEvent,
        sakUseStrictProperty,
        sakProperty,
        sakUnknown,
        sakNumberOfBuckets
    };

    enum XamlNamespaceKind
    {
        xnkNotANamespace,
        xnkDefaultNamespace,
        xnkDefinedNamespace
    };

    XamlSortedAttributes(const std::shared_ptr<XamlParserContext>& inParserContext, const XamlTextReaderSettings& settings);
    ~XamlSortedAttributes() = default;

    HRESULT Reset();
    HRESULT Add(
            const XamlLineInfo& inLineInfo,
            const XamlScannerNode& elementNode,
            const std::shared_ptr<XamlPropertyName>& inPropertyName,
            const std::shared_ptr<XamlText>& inAttributeText);
    ScannerAttributeKind IdentifyNode(const XamlScannerNode& node);

    XUINT32 GetBucketSize(ScannerAttributeKind kind)
    {
        return m_Buckets[kind].size();
    }

    XUINT32 GetSize();

    xvector<XamlScannerNode>::iterator GetBucketIteratorBegin(ScannerAttributeKind kind)
    {
        return m_Buckets[kind].begin();
    }

    xvector<XamlScannerNode>::iterator GetBucketIteratorEnd(ScannerAttributeKind kind)
    {
        return m_Buckets[kind].end();
    }

    HRESULT IsXmlNamespaceDefinition(
                _In_ const xstring_ptr& inPrefix,
                _In_ const xstring_ptr& inName,
                XamlSortedAttributes::XamlNamespaceKind& namespaceKind);

    HRESULT GetXmlNamespaceDefinitionUri(
                XamlSortedAttributes::XamlNamespaceKind inKind,
                const std::shared_ptr<XamlPropertyName>& inPropertyName,
                _In_ const xstring_ptr& inAttributeValue,
                _Out_ xstring_ptr* pstrOutDefiningPrefix,
                _Out_ xstring_ptr* pstrOutUri);

    bool IsFirstElement();
    HRESULT IsMarkupCompatabilityNamespace(const std::shared_ptr<XamlNamespace>& inNamespace, bool& outbIsMarkupCompatibility);
    HRESULT AddIgnoredNamespaces(_In_ const xstring_ptr& inNamespacePrefixesToIgnore);
    void SetFoundXKeyDirective(bool bFound) { m_bFoundXKeyDirective = bFound; m_bFoundXKeyDirectiveValid = TRUE; }
    bool GetFoundXKeyDirective() { ASSERT(m_bFoundXKeyDirectiveValid); return m_bFoundXKeyDirective; }
    bool IsFoundXKeyDirectiveValid() { return m_bFoundXKeyDirectiveValid; }

private:
    HRESULT IsXmlSpace(_In_ const std::shared_ptr<XamlPropertyName>& name, _Out_ bool& bIsXmlSpace);
    HRESULT AddIgnoredPrefix(_In_ const xstring_ptr& inPrefixToIgnore);
    HRESULT IsRuntimeNameProperty(const std::shared_ptr<XamlProperty>& inProperty, const std::shared_ptr<XamlType>& inTagType, bool& bIsRuntimeName);
    HRESULT GetXamlAttributeProperty(
                const std::shared_ptr<XamlPropertyName>& inPropertyName,
                const std::shared_ptr<XamlType>& inTagType,
                _In_ const xstring_ptr& spOwnerPrefix,
                const std::shared_ptr<XamlNamespace>& inOwnerNamespace,
                const XamlLineInfo& lineInfo,
                bool& outbIsIgnored,
                std::shared_ptr<XamlProperty>& outProperty);

    HRESULT GetErrorService(std::shared_ptr<ParserErrorReporter>& outErrorService);
    HRESULT ReportError(XUINT32 errorCode, const XamlLineInfo& lineInfo);
    HRESULT ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1, const XamlLineInfo& lineInfo);
    HRESULT ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1, _In_ const xstring_ptr& inssParam2, const XamlLineInfo& lineInfo);

private:
    XamlScannerNode m_WorkingAttributeNode;
    xvector<XamlScannerNode> m_Buckets[sakNumberOfBuckets];
    std::shared_ptr<XamlParserContext> m_ParserContext;
    bool m_shouldProcessUid:1;
    bool m_bFoundXKeyDirective:1;
    bool m_bFoundXKeyDirectiveValid:1;
};

