// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

MeScanner::MeScanner(
            const std::shared_ptr<XamlParserContext>& inContext, 
            _In_ const xstring_ptr& inText, 
            const XamlLineInfo& lineInfo)
    : m_uIndex(static_cast<XUINT32>(-1))
    , m_mtkTokenKind(mtkNone)
    , m_TokenStringState(tssNone)
    , m_LineInfo(lineInfo)
    , m_bHasTrailingWhitespace(FALSE)
    , m_Context(inContext)
    , m_ssInputText(xstring_ptr(inText))
{
}

XUINT32 MeScanner::LineNumber()
{
    return m_LineInfo.LineNumber();
}
XUINT32 MeScanner::LinePosition()
{
    return m_LineInfo.LinePosition();
}
MeScanner::eMeTokenKind MeScanner::TokenKind()
{
    return m_mtkTokenKind;
}
HRESULT MeScanner::get_TokenType(std::shared_ptr<XamlType>& outXamlType)
{
    outXamlType = m_TokenXamlType;
    RRETURN(S_OK);
}
HRESULT MeScanner::get_TokenProperty(std::shared_ptr<XamlProperty>& outXamlProperty)
{
    outXamlProperty = m_TokenProperty;
    RRETURN(S_OK);
}
////HRESULT MeScanner::get_TokenPrefix(_Out_ xstring_ptr* pstrOutPrefix)
////{
////
////}
HRESULT MeScanner::get_TokenText(_Out_ xstring_ptr* pstrOutText)
{
    *pstrOutText = m_ssTokenText;
    RRETURN(S_OK);
}
bool MeScanner::InternalIsAtEndOfInput()
{
    // Only checks the string pointer.
    ASSERT(!m_ssInputText.IsNullOrEmpty());
    return m_uIndex >= m_ssInputText.GetCount();
}
////bool MeScanner::HasTrailingWhitespace()
////{
////    return m_bHasTrailingWhitespace;
////}
HRESULT MeScanner::Read()
{
    xstring_ptr ssString;
    XStringBuilder stringBuilder;

    Advance();
    AdvanceOverWhitespace();

    if (InternalIsAtEndOfInput())
    {
        m_mtkTokenKind = mtkNone;
        // TODO: S_FALSE?
        return S_FALSE;
    }

    switch (CurrentChar())
    {
    case L'{':
        m_mtkTokenKind = mtkOpen;
        m_TokenStringState = tssType;  // types follow '{'
        break;

    case L'}':
        m_mtkTokenKind = mtkClose;
        m_TokenStringState = tssValue;
        break;

    case L'=':
        m_mtkTokenKind = mtkEqualSign;
        m_TokenStringState = tssValue;
        break;

    case L',':
        m_mtkTokenKind = mtkComma;
        m_TokenStringState = tssValue;
        break;

    default:
        IFC_RETURN(ReadString(stringBuilder));
        m_mtkTokenKind = mtkString;

        switch (m_TokenStringState)
        {
        case tssValue:
            break;

        case tssType:
            m_mtkTokenKind = mtkTypeName;
            IFC_RETURN(ResolveTypeName(XSTRING_PTR_EPHEMERAL_FROM_BUILDER(stringBuilder)));
            break;

        case tssProperty:
            m_mtkTokenKind = mtkPropertyName;
            IFC_RETURN(ResolvePropertyName(XSTRING_PTR_EPHEMERAL_FROM_BUILDER(stringBuilder)));
            break;
        default:
            IFC_RETURN(E_UNEXPECTED);
        }

        m_TokenStringState = tssValue;
        IFC_RETURN(RemoveEscapesInPlace(stringBuilder));
        IFC_RETURN(stringBuilder.DetachString(&ssString));
        m_ssTokenText = std::move(ssString);
        break;
    }

    return S_OK;
}

HRESULT MeScanner::ResolveTypeName(_In_ const xstring_ptr_view& inssTypeName)
{
    std::shared_ptr<XamlTypeName> typeName;

    bool bIsMarkupExtension = false;

    if (FAILED(XamlTypeName::Parse(inssTypeName, typeName)))
    {
        xstring_ptr strTypeName;
        IFC_RETURN(inssTypeName.Promote(&strTypeName));
        IFC_RETURN(ReportError(AG_E_PARSER2_INVALID_TYPENAME, m_LineInfo, strTypeName));
        IFC_RETURN(E_UNEXPECTED);
    }

    IFC_RETURN(m_Context->GetXamlType(typeName, m_XamlNamespace, m_TokenXamlType));

    // If we've found the type, it *must* be a MarkupExtension
    if (m_TokenXamlType)
    {
        IFC_RETURN(m_TokenXamlType->IsMarkupExtension(bIsMarkupExtension));
    }

    // If we haven't found the type, or the type that we've found isn't a MarkupExtension,
    // add "Extension" to the type name and look it up again.
    if (m_XamlNamespace && (!m_TokenXamlType || !bIsMarkupExtension))
    {
        if (!typeName->get_Name().EndsWith(XSTRING_PTR_EPHEMERAL(L"Extension"), xstrCompareCaseSensitive))
        {
            xstring_ptr spExtensionName;
            std::shared_ptr<XamlType> spExtensionType;
            IFC_RETURN(m_XamlNamespace->GetTypeExtensionName(typeName->get_Name(), &spExtensionName));
            IFC_RETURN(m_XamlNamespace->GetXamlType(spExtensionName, spExtensionType));

            if (spExtensionType)
            {
                IFC_RETURN(spExtensionType->IsMarkupExtension(bIsMarkupExtension));
                if (bIsMarkupExtension)
                {
                    m_TokenXamlType = spExtensionType;
                }
            }
        }
    }

    if (!m_TokenXamlType)
    {
        // Fallback for x:Null (directive types)
        if (!m_XamlNamespace)
        {
            IFC_RETURN(ReportError(AG_E_PARSER2_UNDECLARED_PREFIX, m_LineInfo, typeName->get_Prefix()));
            IFC_RETURN(E_FAIL);
        }

        IFC_RETURN(m_XamlNamespace->GetDirectiveType(typeName->get_Name(), m_TokenXamlType));

        if (!m_TokenXamlType)
        {
            // TODO: Should this have the fully qualified type name?
            IFC_RETURN(ReportError(AG_E_PARSER2_UNKNOWN_TYPE, m_LineInfo, typeName->get_Name()));
            IFC_RETURN(E_FAIL);
        }
    }

    IFC_RETURN(m_TokenXamlType->IsMarkupExtension(bIsMarkupExtension));
    if (m_TokenXamlType && !bIsMarkupExtension)
    {
        IFC_RETURN(ReportError(AG_E_PARSER2_MES_NOT_A_MARKUP_EXTENSION, m_LineInfo, typeName->get_Name()));
        IFC_RETURN(E_FAIL);
    }

    // When we resolve a new type, get rid of previous property state.
    m_TokenProperty.reset();

    return S_OK;
}
HRESULT MeScanner::ResolvePropertyName(_In_ const xstring_ptr_view& inPropertyName)
{
    bool bIsDotted = false;
    std::shared_ptr<XamlPropertyName> propertyName;
    std::shared_ptr<XamlNamespace> xamlNamespace;
    std::shared_ptr<XamlProperty> xamlProperty;
    xstring_ptr ssOwner;
    std::shared_ptr<XamlType> declaringType;

    IFC_RETURN(XamlPropertyName::Parse(xstring_ptr::NullString(), inPropertyName, propertyName));
    IFC_RETURN(propertyName->get_OwnerName(&ssOwner));
    IFC_RETURN(propertyName->get_IsDotted(bIsDotted));

    // Static or attachable Property n.p
    if (bIsDotted)
    {
        std::shared_ptr<XamlName> ownerName;
        IFC_RETURN(propertyName->get_Owner(ownerName));
        IFC_RETURN(m_Context->GetXamlType(ownerName, declaringType));

        if (!declaringType)
        {
            std::shared_ptr<XamlSchemaContext> schemaContext;
       
            IFC_RETURN(m_Context->get_SchemaContext(schemaContext));
            xamlNamespace = m_Context->FindNamespaceByPrefix(propertyName->get_Prefix());
            IFC_RETURN(UnknownType::Create(schemaContext, xamlNamespace, ssOwner, declaringType));
        }
        

        // First lookup as a normal property
        IFC_RETURN(m_Context->GetXamlProperty(declaringType, propertyName->get_Name(), xamlProperty));

        if (!xamlProperty)
        {
            IFC_RETURN(m_Context->GetXamlAttachableProperty(declaringType, propertyName->get_Name(), xamlProperty));
        }

        if (!xamlProperty)
        {
            xstring_ptr ssTypeName;
            IFC_RETURN(declaringType->get_Name(&ssTypeName));
            IFC_RETURN(ReportError(AG_E_PARSER2_UNKNOWN_ATTACHABLE_PROP_ON_TYPE, m_LineInfo, propertyName->get_Name(), ssTypeName));
            IFC_RETURN(E_FAIL); 
        }
    }
    // Regular property p
    else
    {
        IFC_RETURN(m_Context->GetXamlProperty(
                    m_Context->get_CurrentType(), 
                    propertyName->get_Name(), 
                    xamlProperty));

        if (!xamlProperty)   // Directive x:p
        {
            xamlNamespace = m_Context->FindNamespaceByPrefix(propertyName->get_Prefix());

            // TODO: Why does it do this check?
            if (!xamlNamespace->IsEqual(*m_XamlNamespace))
            {
                std::shared_ptr<XamlSchemaContext> schemaContext;
                std::shared_ptr<DirectiveProperty> directiveProperty;
                
                IFC_RETURN(m_Context->get_SchemaContext(schemaContext));
                IFC_RETURN(schemaContext->GetXamlDirective(xamlNamespace, propertyName->get_Name(), directiveProperty));
                xamlProperty = std::move(directiveProperty);
            }

            if (!xamlProperty)
            {
                xstring_ptr ssTypeName;
                IFC_RETURN(m_Context->get_CurrentType()->get_Name(&ssTypeName));
                IFC_RETURN(ReportError(AG_E_PARSER2_UNKNOWN_PROP_ON_TYPE, m_LineInfo, propertyName->get_Name(), ssTypeName));
                IFC_RETURN(E_FAIL);                    
            }
        }
    }

    if (!xamlProperty)
    {
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorType, L"NullXamlProperty");
        IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_MES_GENERIC, m_LineInfo, c_strErrorType));
        IFC_RETURN(E_UNEXPECTED);
    }

    m_TokenProperty = xamlProperty;
    return S_OK;
}

HRESULT MeScanner::ReadString(_Inout_ XStringBuilder& sb)
{
    // TODO: Ported mostly directly from the C#
    // This whole thing could be done better than appending
    // characters to a string builder like this.
    const WCHAR nullChar = L'\0';
    bool bEscaped = false;
    WCHAR quoteChar = nullChar;
    bool bAtStart = true;
    bool bWasQuoted = false;
    WCHAR ch = nullChar;

    IFC_RETURN(sb.Initialize(24));

    while(!InternalIsAtEndOfInput())
    {
        ch = CurrentChar();

        // handle escaping and quoting first.
        if(bEscaped)
        {
            IFC_RETURN(sb.AppendChar('\\'));
            IFC_RETURN(sb.AppendChar(ch));
            bEscaped = FALSE;
        }
        else if (quoteChar != nullChar)
        {
            // TODO: this is dubious!
            // Why do they do this?
            if (ch != quoteChar)
            {
                IFC_RETURN(sb.AppendChar(ch));
            }
            else
            {
                ch = CurrentChar();
                quoteChar = nullChar;
                break;  // we are done.
            }
        }
        else
        {
            bool bDone = false;
            switch (ch)
            {
            case L' ':
                if (m_TokenStringState == tssType)
                {
                    bDone = TRUE;  // we are done.
                    break;
                }
                IFC_RETURN(sb.AppendChar(ch));
                break;

            case L'{':
            case L'}':
            case L',':
                bDone = TRUE;  // we are done.
                break;

            case L'=':
                m_TokenStringState = tssProperty;
                bDone = TRUE;  // we are done.
                break;

            case L'\\':
                bEscaped = TRUE;
                break;

            case L'\'':
            case L'"':
                if (!bAtStart)
                {
                    IFC_RETURN(ReportError(AG_E_PARSER2_MES_UNEXPECTED_QUOTE, m_LineInfo));
                    IFC_RETURN(E_FAIL);
                    ////throw new XamlParseException(this, SR.Get(SRID.QuoteCharactersOutOfPlace));
                }
                quoteChar = ch;
                bWasQuoted = TRUE;
                break;

            default:  // All other character (including whitespace)
                IFC_RETURN(sb.AppendChar(ch));
                break;
            }

            if (bDone)
            {
                PushBack();
                break;  // we are done.
            }
        }
        bAtStart = FALSE;
        Advance();
    }

    if (quoteChar != nullChar)
    {
        IFC_RETURN(ReportError(AG_E_PARSER2_MES_UNCLOSED_QUOTE, m_LineInfo));
        IFC_RETURN(E_FAIL);
        ////throw new XamlParseException(this, SR.Get(SRID.UnclosedQuote));
    }

    if (!bWasQuoted)
    {
        IFC_RETURN(sb.TrimWhitespace());
    }

    if ((m_TokenStringState == tssValue) && MeScanner::HasMarkupExtensionEscaping(sb.GetCount(), sb.GetBuffer()))
    {
        IFC_RETURN(sb.ShiftLeft(2));
    }

    return S_OK;
}

WCHAR MeScanner::CurrentChar()
{
    ASSERT(!m_ssInputText.IsNullOrEmpty());
    ASSERT(!InternalIsAtEndOfInput());

    return m_ssInputText.GetBuffer()[m_uIndex];
}
bool MeScanner::Advance()
{
    ASSERT(!m_ssInputText.IsNullOrEmpty());

    ++m_uIndex;
    if (InternalIsAtEndOfInput())
    {
        m_uIndex = m_ssInputText.GetCount();
        return false;
    }

    return true;
}
bool MeScanner::IsWhitespaceChar(const WCHAR& ch)
{
    // TODO: This is duplicated in the xamlscanner.
    // TODO: Also we have a constant xiswhite lookup.
    // TODO: Also in string builder.
    return (ch == L' ') || (ch == L'\n') || (ch == L'\r') || (ch == L'\t');

}
void MeScanner::AdvanceOverWhitespace()
{
    bool bSawWhitespace = false;

    while(!InternalIsAtEndOfInput() && IsWhitespaceChar(CurrentChar()))
    {
        bSawWhitespace = TRUE;
        Advance();
    }

    // We normally skip whitespace, but you aren't allowed to have trailing whitespace.
    if (bSawWhitespace && InternalIsAtEndOfInput())
    {
        m_bHasTrailingWhitespace = TRUE;
    }

}
void MeScanner::PushBack()
{
    ASSERT(m_uIndex != 0);

    // TODO: Does this need to be enforced more?
    m_uIndex--;
}

HRESULT MeScanner::RemoveEscapesInPlace(_Inout_ XStringBuilder& stringBuilder)
{
    UINT32 ichFound = 0;
    while (XStringBuilder::npos != (ichFound = stringBuilder.FindChar(L'\\', ichFound)))
    {
        IFC_RETURN(stringBuilder.Remove(ichFound, 1));

        //
        // if at this point ichFound == rspString->GetCount(), then the found value
        // was at the end of the string:
        // 
        // For the string "012\":
        //      After FindChar:   GetCount() == 4, ichFound == 3
        //      After Remove:     GetCount() == 3, ichFound == 3
        // 
        if (ichFound == stringBuilder.GetCount())
        {
            break;
        }

        //  increment ichFound so that we start one past the newly unescaped
        //  character.  If we didn't do this, then we'd be removing all of the 
        //  back-slashes rather than just the back-slashes immediately preceding
        //  other characters.
        ichFound++;
    }
    return S_OK;
}

HRESULT MeScanner::GetErrorService(std::shared_ptr<ParserErrorReporter>& outErrorService)
{
    std::shared_ptr<XamlSchemaContext> schemaContext;
    
    IFC_RETURN(GetSchemaContext(schemaContext));
    IFC_RETURN(schemaContext->GetErrorService(outErrorService));

    return S_OK;

}

HRESULT MeScanner::ReportError(XUINT32 errorCode, const XamlLineInfo& inLineInfo)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, inLineInfo.LineNumber(), inLineInfo.LinePosition()));

    return S_OK;
}

HRESULT MeScanner::ReportError(XUINT32 errorCode, const XamlLineInfo& inLineInfo, _In_ const xstring_ptr& inssParam1)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, inLineInfo.LineNumber(), inLineInfo.LinePosition(), inssParam1));

    return S_OK;
}

HRESULT MeScanner::ReportError(XUINT32 errorCode, const XamlLineInfo& inLineInfo, _In_ const xstring_ptr& inssParam1, _In_ const xstring_ptr& inssParam2)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, inLineInfo.LineNumber(), inLineInfo.LinePosition(), inssParam1, inssParam2));

    return S_OK;
}

