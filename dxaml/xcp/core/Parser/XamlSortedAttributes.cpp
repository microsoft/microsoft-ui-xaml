// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xamlxmlnamespace.h"
#include "XamlTextReaderSettings.h"

XamlSortedAttributes::XamlSortedAttributes(const std::shared_ptr<XamlParserContext>& inParserContext, const XamlTextReaderSettings& settings)
    : m_ParserContext(inParserContext)
    , m_bFoundXKeyDirective(FALSE)
    , m_bFoundXKeyDirectiveValid(FALSE)
    , m_shouldProcessUid(settings.get_ShouldProcessUid())
{
}

HRESULT XamlSortedAttributes::Reset()
{
    HRESULT hr = S_OK;

    for (XUINT32 i = 0; i < sakNumberOfBuckets; i++)
    {
        m_Buckets[i].clear();
    }
    m_bFoundXKeyDirectiveValid = FALSE;

    RRETURN(hr);
}

bool XamlSortedAttributes::IsFirstElement()
{
    // TODO:
    return m_ParserContext->IsStackEmpty();
}


HRESULT XamlSortedAttributes::AddIgnoredNamespaces(_In_ const xstring_ptr& inNamespacePrefixesToIgnore)
{
    xstring_ptr ssCurrentPrefix;

    XUINT32 upto = 0;
    XUINT32 length = inNamespacePrefixesToIgnore.GetCount();
    const WCHAR* pBuffer = inNamespacePrefixesToIgnore.GetBuffer();
    XUINT32 i = 0;

    while (i < length)
    {
        if (pBuffer[i] == L' ')
        {
            IFC_RETURN(inNamespacePrefixesToIgnore.SubString(upto, i, &ssCurrentPrefix));
            IFC_RETURN(AddIgnoredPrefix(ssCurrentPrefix));

            // Skip any more spaces.
            while ((i < length) && pBuffer[i] == L' ')
            {
                i++;
            }

            upto = i;
        }
        else
        {
            // Skip any NON-spaces.
            i++;
        }
    }

    if (upto != length)
    {
        IFC_RETURN(inNamespacePrefixesToIgnore.SubString(upto, i, &ssCurrentPrefix));
        IFC_RETURN(AddIgnoredPrefix(ssCurrentPrefix));
    }
    return S_OK;
}

HRESULT XamlSortedAttributes::AddIgnoredPrefix(_In_ const xstring_ptr& inPrefixToIgnore)
{
    xstring_ptr ssCurrentUri;
    std::shared_ptr<XamlNamespace> namespaceToIgnore = m_ParserContext->FindNamespaceByPrefix(inPrefixToIgnore);

    // If it didn't resolve it's not an error. But if someone goes to use
    // it later it will be the same error as if it were not in the ignore list.
    if (namespaceToIgnore)
    {
        XamlXmlNamespace *pXmlNs = namespaceToIgnore->AsXamlXmlNamespace();
        bool bIsImplicitlyResolved = false;

        // Determine if this namespace is implicitly resolved
        if (pXmlNs != NULL)
        {
            IFC_RETURN(pXmlNs->GetIsImplicitlyResolved(&bIsImplicitlyResolved));
        }

        // Ignore URI if the URI is not a using-namespace or if allowed by Quirks
        if (!bIsImplicitlyResolved)
        {
            ssCurrentUri = namespaceToIgnore->get_TargetNamespace();
            m_ParserContext->AddIgnoredUri(ssCurrentUri);
        }
    }

    return S_OK;
}

HRESULT XamlSortedAttributes::Add(
            const XamlLineInfo& inLineInfo,
            const XamlScannerNode& elementNode,
            const std::shared_ptr<XamlPropertyName>& inPropertyName,
            const std::shared_ptr<XamlText>& inAttributeText)
{
    // TODO: Can probably just get prefix, name once here and pass it to the other functions.
    xstring_ptr ssXmlnsDefinitionPrefix;
    xstring_ptr ssXmlnsDefinitionUri;

    ScannerAttributeKind kind = sakProperty;
    XamlSortedAttributes::XamlNamespaceKind namespaceKind = XamlSortedAttributes::xnkNotANamespace;

    ////IFC(inPropertyName->get_Name(ssName));
    ////IFC(inPropertyName->get_Prefix(ssPrefix));

    IFC_RETURN(IsXmlNamespaceDefinition(
                inPropertyName->get_Prefix(),
                inPropertyName->get_Name(),
                namespaceKind));

    if (namespaceKind != XamlSortedAttributes::xnkNotANamespace)
    {
        std::shared_ptr<XamlNamespace> xamlNamespace;
        xstring_ptr ssAttributeString;

        IFC_RETURN(inAttributeText->get_Text(&ssAttributeString));
        kind = sakNamespace;
        // tODO: Can break this out more - don't need the uri again, just want the defining prefix.
        // This function can be broken to two, and we can call the factored one instead.
        IFC_RETURN(GetXmlNamespaceDefinitionUri(
                    namespaceKind,
                    inPropertyName,
                    ssAttributeString,
                    &ssXmlnsDefinitionPrefix,
                    &ssXmlnsDefinitionUri));

        xamlNamespace = m_ParserContext->FindNamespaceByPrefix(ssXmlnsDefinitionPrefix);

        m_WorkingAttributeNode.InitPrefixDefinition(
                    inLineInfo,
                    ssXmlnsDefinitionPrefix,
                    xamlNamespace,
                    inAttributeText);
    }
    else
    {
        std::shared_ptr<XamlProperty> xamlProperty;
        std::shared_ptr<XamlType> spElementType;

        bool bIsMarkupCompatNamespace = false;
        bool bIsIgnored = false;

        if (IsFirstElement())
        {
            std::shared_ptr<XamlNamespace> attributeNamespace = m_ParserContext->FindNamespaceByPrefix(inPropertyName->get_Prefix());

            IFC_RETURN(IsMarkupCompatabilityNamespace(
                        attributeNamespace,
                        bIsMarkupCompatNamespace));

            if (bIsMarkupCompatNamespace && inPropertyName->get_Name().Equals(XSTRING_PTR_EPHEMERAL(L"Ignorable")))
            {
                // This case is already handled in the pre-process phase
                // so do nothing here.
                return S_OK;
            }
        }

        // Only try to get the type of the node if this isn't a property element
        // (because otherwise the value is really null, but get_Type() will
        // return the XamlProperty cast as a XamlType instead).  This is only
        // here to check for XAML like
        //     <Foo>
        //       <Foo.Property Attribute="Not Allowed!!!">
        //          ...
        //       </Foo.Property>
        //     </Foo>
        // This is always an error except in the case of x:Uid, which is special
        // cased below.
        if (elementNode.get_NodeType() != XamlScannerNode::sntPropertyElement)
        {
            spElementType = elementNode.get_Type();
        }

        IFC_RETURN(GetXamlAttributeProperty(
                    inPropertyName,
                    spElementType,
                    elementNode.get_Prefix(),
                    elementNode.get_TypeNamespace(),
                    elementNode.get_LineInfo(),
                    bIsIgnored,
                    xamlProperty));

        if (bIsIgnored)
        {
            return S_OK;
        }

        IFCEXPECT_ASSERT_RETURN(!!xamlProperty);

        if (xamlProperty->IsDirective())
        {
            switch (std::static_pointer_cast<DirectiveProperty>(xamlProperty)->get_DirectiveKind())
            {
                case xdDeferLoadStrategy:
                    kind = sakDeferLoadStrategy;
                    break;

                case xdLoad:
                    kind = sakLoad;
                    break;

                case xdUid:
                    kind = sakUid;

                    // Ignore Uid directives anywhere that we see them (including on
                    // properties like <Button><Button.Content x:Uid="foo"> ...
                    // </Button.Content></Button>).  We specifically don't add them so
                    // that XamlScanner::VerifyNoAttributesOnPropertyElement won't see
                    // any attributes in the collection.
                    if (!m_shouldProcessUid)
                    {
                        return S_OK;
                    }
                    break;

                default:
                    {
                        bool bIsXmlSpace = false;
                        IFC_RETURN(IsXmlSpace(inPropertyName, bIsXmlSpace));
                        kind = (bIsXmlSpace)
                            ? sakXmlSpace
                            : sakDirective;
                    }
                    break;
            }

            m_WorkingAttributeNode.InitDirective(
                        inLineInfo,
                        inPropertyName->get_Prefix(),
                        xamlProperty,
                        inAttributeText);
        }
        else
        {
            if (xamlProperty->IsUnknown())
            {
                kind = sakUnknown;
            }
            else if (xamlProperty->IsEvent())
            {
                kind = sakEvent;
            }
            else
            {
                bool bIsRuntimeName = false;
                IFC_RETURN(IsRuntimeNameProperty(
                            xamlProperty,
                            elementNode.get_Type(),
                            bIsRuntimeName));

                if (bIsRuntimeName)
                {
                    kind = sakName;
                }
                else
                {
                    kind = sakProperty;
                }
            }

            m_WorkingAttributeNode.InitAttribute(
                        inLineInfo,
                        inPropertyName->get_Prefix(),
                        xamlProperty,
                        inAttributeText);
        }
    }

    IFC_RETURN(m_Buckets[kind].push_back(m_WorkingAttributeNode));

    return S_OK;

}


// Get the total number of attributes (opposed to GetBucketSize which only gets
// the number of attributes in a specific category)
XUINT32 XamlSortedAttributes::GetSize()
{
    XUINT32 uiSize = 0;
    for (XUINT32 i = 0; i < sakNumberOfBuckets; i++)
    {
        uiSize += GetBucketSize((ScannerAttributeKind)i);
    }
    return uiSize;
}


HRESULT XamlSortedAttributes::IsRuntimeNameProperty(const std::shared_ptr<XamlProperty>& inProperty, const std::shared_ptr<XamlType>& inTagType, bool& bIsRuntimeName)
{
    // This is supposed to check for x:Name
    std::shared_ptr<XamlSchemaContext> schemaContext;

    IFC_RETURN(m_ParserContext->get_SchemaContext(schemaContext));
    // TODO: Need to get the namespace and look it up.
    bIsRuntimeName = FALSE;

    return S_OK;

}

HRESULT XamlSortedAttributes::IsXmlSpace(
    _In_ const std::shared_ptr<XamlPropertyName>& inName,
    _Out_ bool& bIsXmlSpace)
{
    HRESULT hr = S_OK;

    bIsXmlSpace =
        inName->get_Prefix().Equals(XSTRING_PTR_EPHEMERAL(L"xml")) &&
        inName->get_Name().Equals(XSTRING_PTR_EPHEMERAL(L"space"));

    RRETURN(hr);
}

HRESULT XamlSortedAttributes::IsXmlNamespaceDefinition(_In_ const xstring_ptr& inPrefix, _In_ const xstring_ptr& inName, XamlSortedAttributes::XamlNamespaceKind& namespaceKind)
{
    HRESULT hr = S_OK;

    //TODO: hardcoded string
    // case where:  xmlns:pre="ValueUri"
    if (inPrefix.Equals(XSTRING_PTR_EPHEMERAL(L"xmlns")))
    {
        namespaceKind = xnkDefinedNamespace;
    }
    else if ((inPrefix.GetCount() == 0) && (inName.Equals(XSTRING_PTR_EPHEMERAL(L"xmlns"))))
    {
        namespaceKind = xnkDefaultNamespace;
    }
    else
    {
        namespaceKind = xnkNotANamespace;
    }

    RRETURN(hr);
}

HRESULT XamlSortedAttributes::GetXmlNamespaceDefinitionUri(
                XamlSortedAttributes::XamlNamespaceKind inKind,
                const std::shared_ptr<XamlPropertyName>& inPropertyName,
                _In_ const xstring_ptr& inAttributeValue,
                _Out_ xstring_ptr* pstrOutDefiningPrefix,
                _Out_ xstring_ptr* pstrOutUri)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    bool bIsDotted = false;

    //TODO: hardcoded
    // case where:  xmlns:pre="ValueUri"
    if (inKind == xnkDefinedNamespace)
    {
        *pstrOutUri = inAttributeValue;
        IFC(inPropertyName->get_IsDotted(bIsDotted));
        if (bIsDotted)
        {
            // NB: The original code did this manually, but the function appears to do the same thing?
            IFC(inPropertyName->get_ScopedName(pstrOutDefiningPrefix));
        }
        else
        {
            *pstrOutDefiningPrefix = inPropertyName->get_Name();
        }
    }
    else if (inKind == xnkDefaultNamespace)
    {
        // TODO: Is there as isempty? ^^
        // TODO: Hardcoded - factor to method.
        *pstrOutUri = inAttributeValue;
        *pstrOutDefiningPrefix = xstring_ptr::EmptyString();
    }
    else
    {
        IFC(E_UNEXPECTED);
    }


Cleanup:
    if (FAILED(hr))
    {
        pstrOutDefiningPrefix->Reset();
        pstrOutUri->Reset();
    }
    RRETURN(S_OK);
}

HRESULT XamlSortedAttributes::GetXamlAttributeProperty(
            const std::shared_ptr<XamlPropertyName>& inPropertyName,
            const std::shared_ptr<XamlType>& inTagType,
            _In_ const xstring_ptr& spOwnerPrefix,
            const std::shared_ptr<XamlNamespace>& inOwnerNamespace,
            const XamlLineInfo& lineInfo,
            bool& outbIsIgnored,
            std::shared_ptr<XamlProperty>& outProperty)
{
    // Note that inTagType will be null in the case that we have an attribute
    // specified on a property element like:
    //     <Foo>
    //       <Foo.Property Attribute="Not Allowed!">
    //         ...
    //       </Foo.Property>
    //     </Foo>
    // In this case we will only return an UnknownProperty or a
    // DirectiveProperty (because x:Uid is valid in this scenario).

    // TODO: This function needs refactoring.
    bool bIsDotted = false;
    bool bNamespaceIsOwnerNamespace = false;
    bool bIsNamespaceResolved = false;
    std::shared_ptr<XamlProperty> xamlProperty;
    std::shared_ptr<XamlSchemaContext> schemaContext;
    std::shared_ptr<XamlNamespace> xamlNamespace;
    std::shared_ptr<XamlName> xamlOwnerName;
    xstring_ptr ssOwnerName;

    outbIsIgnored = FALSE;
    IFC_RETURN(inPropertyName->get_OwnerName(&ssOwnerName));
    IFC_RETURN(inPropertyName->get_Owner(xamlOwnerName));
    IFC_RETURN(inPropertyName->get_IsDotted(bIsDotted));
    IFC_RETURN(m_ParserContext->get_SchemaContext(schemaContext));

    if ((inPropertyName->get_Prefix().GetCount() == 0) && !bIsDotted || inPropertyName->get_Prefix().Equals(spOwnerPrefix))
    {
        xamlNamespace = inOwnerNamespace;
        bNamespaceIsOwnerNamespace = TRUE;
    }
    else
    {
        xamlNamespace = m_ParserContext->FindNamespaceByPrefix(inPropertyName->get_Prefix());
        // In some cases multiple prefixes can be mapped to the same namespace
        // so now that we resolved it we need to check again.
        bNamespaceIsOwnerNamespace = xamlNamespace->IsEqual(*inOwnerNamespace);
    }

    if (!xamlNamespace)
    {
        // TODO: Line number
        IFC_RETURN(ReportError(AG_E_PARSER2_UNDECLARED_PREFIX, inPropertyName->get_Prefix(), lineInfo));
        IFC_RETURN(E_FAIL);
    }

    // TODO: This code is duplicated in the scanner.
    // If a namespace is resolved you can't ignore it
    // and we silently ignore you ignoring it.
    bIsNamespaceResolved = xamlNamespace->get_IsResolved();

    if (!bIsNamespaceResolved)
    {
        xstring_ptr ssUri = xamlNamespace->get_TargetNamespace();
        if (m_ParserContext->IsNamespaceUriIgnored(ssUri))
        {
            outbIsIgnored = TRUE;
            return S_OK;
        }
    }


    if (bIsDotted)
    {
        std::shared_ptr<XamlType> attachedOwnerType;

        bool bIsAssignableFrom = false;

        IFC_RETURN(m_ParserContext->GetXamlType(xamlOwnerName, attachedOwnerType));

        if (!attachedOwnerType)
        {
            IFC_RETURN(UnknownType::Create(schemaContext, xamlNamespace, ssOwnerName, attachedOwnerType));
        }

        if (inTagType)
        {
            IFC_RETURN(attachedOwnerType->IsAssignableFrom(inTagType, bIsAssignableFrom));
        }

        if (bIsAssignableFrom)
        {
            IFC_RETURN(m_ParserContext->GetXamlProperty(attachedOwnerType, inPropertyName->get_Name(), xamlProperty));
        }

        if (!xamlProperty)
        {
            IFC_RETURN(m_ParserContext->GetXamlAttachableProperty(attachedOwnerType, inPropertyName->get_Name(), xamlProperty));

            if (!xamlProperty)
            {
                std::shared_ptr<UnknownProperty> unknownProperty;
                IFC_RETURN(UnknownProperty::Create(schemaContext, inPropertyName->get_Name(), attachedOwnerType, xamlNamespace, TRUE, unknownProperty));
                xamlProperty = unknownProperty;
            }
        }
    }
    else
    {
        if (bNamespaceIsOwnerNamespace && inTagType)
        {
            IFC_RETURN(m_ParserContext->GetXamlProperty(inTagType, inPropertyName->get_Name(), xamlProperty));

            // REVIEW: GetXamlAttachableProperty() is basically the same thing as GetXamlProperty(), but it
            // validates that the retrieved XamlProperty is an attached property. Since the following call is only if
            // the previous call returned false, it seems redundant...
            if (!xamlProperty)
            {
                IFC_RETURN(m_ParserContext->GetXamlAttachableProperty(inTagType, inPropertyName->get_Name(), xamlProperty));
            }
        }

        if (!xamlProperty)
        {
            // Check if it's a processing attribute e.g. x:Key, x:Name, x:Class, etc.
            std::shared_ptr<DirectiveProperty> directiveProperty;
            IFC_RETURN(schemaContext->GetXamlDirective(xamlNamespace, inPropertyName->get_Name(), directiveProperty));

            if (directiveProperty && xamlNamespace->IsConditional())
            {
                // Conditionally declared directives are not permitted, as this opens
                // a nice can of worms vis-�-vis the implementation of such.
                // NOTE: this limitation might be interesting if we introduce a new directive property,
                // but we'll cross that bridge when we reach it.
                IFC_RETURN(ReportError(AG_E_PARSER2_DIRECTIVE_CANNOT_BE_CONDITIONAL, lineInfo));
                IFC_RETURN(E_FAIL);
            }
            else
            {
                xamlProperty = std::move(directiveProperty);
            }
        }

        if (!xamlProperty)
        {
            std::shared_ptr<UnknownProperty> unknownProperty;
            IFC_RETURN(UnknownProperty::Create(schemaContext, inPropertyName->get_Name(), inTagType, xamlNamespace, FALSE, unknownProperty));
            xamlProperty = unknownProperty;
        }
    }

    ASSERT(!!xamlProperty);
    outProperty = xamlProperty;


    return S_OK;
}

HRESULT XamlSortedAttributes::IsMarkupCompatabilityNamespace(
            const std::shared_ptr<XamlNamespace>& inNamespace,
            bool& outbIsMarkupCompatibility)
{
    outbIsMarkupCompatibility = FALSE;

    if (inNamespace)
    {
        xstring_ptr ssNamespaceUri = inNamespace->get_TargetNamespace();
        // TODO: Hardcoded string
        // TODO: Case sensitive?
        outbIsMarkupCompatibility = ssNamespaceUri.Equals(XSTRING_PTR_EPHEMERAL(L"http://schemas.openxmlformats.org/markup-compatibility/2006"));
    }

    return S_OK;
}

HRESULT XamlSortedAttributes::GetErrorService(std::shared_ptr<ParserErrorReporter>& outErrorService)
{
    std::shared_ptr<XamlSchemaContext> schemaContext;

    IFC_RETURN(m_ParserContext->get_SchemaContext(schemaContext));
    IFC_RETURN(schemaContext->GetErrorService(outErrorService));

    return S_OK;
}

HRESULT XamlSortedAttributes::ReportError(XUINT32 errorCode, const XamlLineInfo& lineInfo)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, lineInfo.LineNumber(), lineInfo.LinePosition()));

    return S_OK;
}

HRESULT XamlSortedAttributes::ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1, const XamlLineInfo& lineInfo)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, lineInfo.LineNumber(), lineInfo.LinePosition(), inssParam1));

    return S_OK;
}

HRESULT XamlSortedAttributes::ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1, _In_ const xstring_ptr& inssParam2, const XamlLineInfo& lineInfo)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, lineInfo.LineNumber(), lineInfo.LinePosition(), inssParam1, inssParam2));

    return S_OK;
}


