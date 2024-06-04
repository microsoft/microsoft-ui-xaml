// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// TODO: Maybe want re-usable with Init instead
MePullParser::MePullParser(
    _In_ IPullParserNodeCallback* pNodeCallback,
    _In_ const std::shared_ptr<XamlParserContext>& inParserContext,
    _In_ const xstring_ptr& inOriginalText,
    _In_ const XamlLineInfo& lineInfo)
        : m_ParserContext(inParserContext)
        , m_OriginalText(xstring_ptr(inOriginalText))
        , m_pEnqueueNodeDelegate(pNodeCallback)
        , m_Scanner(inParserContext, inOriginalText, lineInfo)
{
}

HRESULT MePullParser::Parse()
{
    IFC_RETURN(NextToken());
    IFC_RETURN(P_MarkupExtension());

    if (!m_Scanner.IsAtEndOfInput() || m_Scanner.HasTrailingWhitespace())
    {
        // TODO: This could capture the "extra text" and put it into the message
        IFC_RETURN(ReportError(AG_E_PARSER2_MES_UNEXPECTED_TEXT_AFTER_ME));
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}

HRESULT MePullParser::CheckToken(const MeScanner::eMeTokenKind& inTokenKind)
{
    // TODO: Do something about the rules/errors
    if (m_Scanner.TokenKind() != inTokenKind)
    {
        xstring_ptr ssParam;
        XStringBuilder paramBuilder;

        switch (inTokenKind)
        {
            case MeScanner::mtkOpen:
            case MeScanner::mtkClose:
            case MeScanner::mtkEqualSign:
            case MeScanner::mtkComma:
                // Create a single character string and write the symbol into it.
                IFC_RETURN(paramBuilder.Initialize(1));
                IFC_RETURN(paramBuilder.AppendChar(static_cast<WCHAR>(inTokenKind)));
                IFC_RETURN(paramBuilder.DetachString(&ssParam));
                IFC_RETURN(ReportError(AG_E_PARSER2_MES_EXPECTED_SYMBOL, ssParam));
                IFC_RETURN(E_FAIL);
                break;
            case MeScanner::mtkTypeName:
                IFC_RETURN(ReportError(AG_E_PARSER2_MES_EXPECTED_TYPE));
                IFC_RETURN(E_FAIL);
                break;
            case MeScanner::mtkPropertyName:
                IFC_RETURN(ReportError(AG_E_PARSER2_MES_EXPECTED_PROPERTY));
                IFC_RETURN(E_FAIL);
                break;
            case MeScanner::mtkString:
                // TODO: Is an empty string meaningful? That could accidentally hit this.
                IFC_RETURN(ReportError(AG_E_PARSER2_MES_EXPECTED_VALUE));
                IFC_RETURN(E_FAIL);
                break;
            case MeScanner::mtkNone:
            default:
                IFC_RETURN(xstring_ptr::CreateFromUInt32(static_cast<XUINT32>(inTokenKind), &ssParam));
                IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_MES_UNKNOWN_TOKEN, ssParam));
                IFC_RETURN(E_UNEXPECTED);
                break;

        }
    }

    return S_OK;
}

HRESULT MePullParser::ExpectToken(const MeScanner::eMeTokenKind& inTokenKind)
{
    IFC_RETURN(CheckToken(inTokenKind));
    IFC_RETURN(NextToken());

    return S_OK;
}

HRESULT MePullParser::ExpectTypeName()
{
    IFC_RETURN(CheckToken(MeScanner::mtkTypeName));
    IFC_RETURN(Logic_StartObject());
    IFC_RETURN(NextToken());

    return S_OK;
}

HRESULT MePullParser::ExpectPropertyName()
{
    IFC_RETURN(CheckToken(MeScanner::mtkPropertyName));
    IFC_RETURN(Logic_StartMember());
    IFC_RETURN(NextToken());

    return S_OK;
}

HRESULT MePullParser::ExpectValue()
{
    std::shared_ptr<XamlType> xamlType;

    IFC_RETURN(CheckToken(MeScanner::mtkString));
    IFC_RETURN(m_Scanner.get_TokenType(xamlType));
    IFC_RETURN(Logic_Text());
    IFC_RETURN(NextToken());

    return S_OK;
}

HRESULT MePullParser::P_MarkupExtension()
{
    IGNORERESULT(m_Scanner.TokenKind());

    // {
    IFC_RETURN(ExpectToken(MeScanner::mtkOpen));

    // { TYPENAME
    IFC_RETURN(ExpectTypeName());

    // { TYPENAME arguments
    IFC_RETURN(P_Arguments());

    // { TYPENAME arguments }
    IFC_RETURN(ExpectToken(MeScanner::mtkClose));
    IFC_RETURN(Logic_EndObject());

    return S_OK;
}

HRESULT MePullParser::P_Arguments()
{
    bool bSeenPropertyName = false;
    bool bSeenAnyPositional = false;
    bool bTrailingComma = false;
    MeScanner::eMeTokenKind peekToken = MeScanner::mtkNone;

    // Loop until a close brace or the None token which indicates no more
    while ( !m_Scanner.IsAtEndOfInput() && ((peekToken = m_Scanner.TokenKind()) != MeScanner::mtkClose))
    {
        bTrailingComma = FALSE;
        switch(peekToken)
        {
        case MeScanner::mtkPropertyName:
////#if 0
////            // TODO: We don't support arbitrary positional
////            // parameters in Silverlight right now.
////            if (bSeenAnyPositional)
////            {
////                IFC(Logic_EndPositionalParameters());
////                bEmitedEndOfPositionals = TRUE;
////            }
////#endif

            // Once there is a property name, positional params aren't
            // valid any more.
            bSeenPropertyName = TRUE;

            // { TYPENAME PROPERTYNAME
            IFC_RETURN(ExpectPropertyName());

            // { TYPENAME PROPERTYNAME=
            IFC_RETURN(ExpectToken(MeScanner::mtkEqualSign));

            // TODO: P_Value?
            // Also I think WPF will accept the ME in a string.
            peekToken = m_Scanner.TokenKind();
            if (peekToken == MeScanner::mtkOpen)
            {
                // { TYPENAME PROPERTYNAME=markup_extension
                IFC_RETURN(P_MarkupExtension());
            }
            else
            {
                // { TYPENAME PROPERTYNAME=VALUE
                IFC_RETURN(ExpectValue());
            }

            IFC_RETURN(Logic_EndMember());
            break;

        case MeScanner::mtkString:
            // { TYPENAME VALUE
////#if 0
////            if (!bAllowPositionalParameters)
////            {
////                // TODO: No guarantees this code path works
////                // if bAllowPositionalParameters is TRUE
////                //
////                // See declaration. This shoudl always error out in
////                // the current version of Silverlight.
////
////                IFC(ReportError(DEFAULT_ERROR));
////            }
////#endif

            if (bSeenPropertyName)
            {
                // Can't take a positional parameter once you've named one.
                IFC_RETURN(ReportError(AG_E_PARSER2_MES_ONLY_ONE_POSITIONAL));
                IFC_RETURN(E_FAIL);
            }
            else
            {

                if (bSeenAnyPositional)
                {
////#if 0
////                    IFC(Logic_StartPositionalParameters());
////#endif
                    // TODO: Silverlight enforced limitation. We only support a single (optional)
                    // positional parameter so error out if we've already seen one.
                    IFC_RETURN(ReportError(AG_E_PARSER2_MES_ONLY_ONE_POSITIONAL));
                    IFC_RETURN(E_FAIL);
                }

                bSeenAnyPositional = TRUE;


                IFC_RETURN(Logic_StartMember());
                IFC_RETURN(ExpectValue());
                IFC_RETURN(Logic_EndMember());
            }
            break;

        case MeScanner::mtkOpen:
            // { TYPENAME markup_extension
            IFC_RETURN(P_MarkupExtension());
            break;

        default:
            xstring_ptr ssParam;
            XStringBuilder paramBuilder;

            // Create a single character string and write the symbol into it.
            IFC_RETURN(paramBuilder.Initialize(1));
            IFC_RETURN(paramBuilder.AppendChar(static_cast<WCHAR>(peekToken)));
            IFC_RETURN(paramBuilder.DetachString(&ssParam));
            IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_MES_UNKNOWN_TOKEN, ssParam));
            IFC_RETURN(E_UNEXPECTED);
            break;
        }

        if (m_Scanner.TokenKind() == MeScanner::mtkComma)
        {
            // { TYPENAME PROPERTYNAME=VALUE,
            // { TYPENAME VALUE,
            // { TYPENAME markup_extension,
            IFC_RETURN(ExpectToken(MeScanner::mtkComma));
            bTrailingComma = TRUE;
        }
    }

    if (bTrailingComma)
    {
        IFC_RETURN(ReportError(AG_E_PARSER2_MES_TRAILING_COMMA));
        IFC_RETURN(E_FAIL);
    }

////#if 0
////    if (bSeenAnyPositional && !bEmitedEndOfPositionals)
////    {
////        IFC(Logic_EndPositionalParameters());
////    }
////#endif
    return S_OK;
}

HRESULT MePullParser::NextToken()
{
    return m_Scanner.Read();
}

XUINT32 MePullParser::LineNumber()
{
    // TODO: Magic offseting into the string?
    return m_Scanner.LineNumber();
}

XUINT32 MePullParser::LinePosition()
{
    // TODO: Magic offseting into the string?
    return m_Scanner.LinePosition();
}

HRESULT MePullParser::EnqueueWorkingNode()
{
    SetLineInfoOnWorkingNode();
    IFC_RETURN(m_pEnqueueNodeDelegate->AcceptNode(m_WorkingNode));

    // TODO: Possibly in debug null out the working node.
    return S_OK;
}

void MePullParser::SetLineInfoOnWorkingNode()
{
    // TODO: Potentially we can do extra line info magic here to
    // offset into the markup extension string.
    m_WorkingNode.set_LineInfo(XamlLineInfo(m_Scanner.LineNumber(), m_Scanner.LinePosition()));
}

HRESULT MePullParser::Logic_StartObject()
{
    // TODO: There is duplication between this and the xamlpull parser.
    // Possibly a common base class here.
    std::shared_ptr<XamlType> xamlType;

    IFC_RETURN(m_Scanner.get_TokenType(xamlType));
    m_ParserContext->PushScope();
    m_ParserContext->set_CurrentType(xamlType);
    // TODO: Want a type for ME objects?
    m_ParserContext->set_CurrentKind(XamlParserFrame::ekElement);
    m_WorkingNode.InitStartObjectNode(xamlType, false, xamlType->IsUnknown());
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}

HRESULT MePullParser::Logic_EndObject()
{
    m_ParserContext->PopScope();
    m_WorkingNode.InitEndObjectNode();
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}

HRESULT MePullParser::Logic_StartMember()
{
    std::shared_ptr<XamlProperty> xamlProperty;

    IFC_RETURN(m_Scanner.get_TokenProperty(xamlProperty));

    if (!xamlProperty)
    {
        // TODO: Compat - Special sauce for Silverlight to
        // manually look up what the first positional
        // parameter looks up to.
        IFC_RETURN(GetPropertyForSilverlightPositionalParameter(xamlProperty));
    }

    m_ParserContext->set_CurrentMember(xamlProperty);
    m_WorkingNode.InitStartMemberNode(xamlProperty);
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}

HRESULT MePullParser::Logic_EndMember()
{
    // TODO: Could we have a clear_CurrentMember function on the context?
    m_ParserContext->set_CurrentMember(std::shared_ptr<XamlProperty>());
    m_WorkingNode.InitEndMemberNode();
    IFC_RETURN(EnqueueWorkingNode());
    return S_OK;
}

HRESULT MePullParser::Logic_Text()
{
    xstring_ptr ssText;
    std::shared_ptr<XamlText> xamlText;

    IFC_RETURN(m_Scanner.get_TokenText(&ssText));

    // TODO: Is it correct to set whitespace preserve here?
    xamlText = std::make_shared<XamlText>(true);
    IFC_RETURN(xamlText->Initialize(ssText));
    m_WorkingNode.InitTextValueNode(xamlText);
    IFC_RETURN(EnqueueWorkingNode());

    return S_OK;
}

HRESULT MePullParser::GetPropertyForSilverlightPositionalParameter(std::shared_ptr<XamlProperty>& outXamlProperty)
{
    std::shared_ptr<XamlType> xamlType;
    xstring_ptr ssTypeName;

    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strResourceKey, L"ResourceKey");
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strPath, L"Path");
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strProperty, L"Property");
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strMode, L"Mode");

    IFC_RETURN(m_Scanner.get_TokenType(xamlType));
    IFC_RETURN(xamlType->get_Name(&ssTypeName));

    if (ssTypeName.Equals(STR_LEN_PAIR(L"StaticResource")))
    {
        IFC_RETURN(xamlType->GetProperty(c_strResourceKey, outXamlProperty));
    }
    else if (ssTypeName.Equals(STR_LEN_PAIR(L"CustomResource")))
    {
        IFC_RETURN(xamlType->GetProperty(c_strResourceKey, outXamlProperty));
    }
    else if (ssTypeName.Equals(STR_LEN_PAIR(L"ThemeResource")))
    {
        IFC_RETURN(xamlType->GetProperty(c_strResourceKey, outXamlProperty));
    }
    else if (ssTypeName.Equals(STR_LEN_PAIR(L"Binding")))
    {
        IFC_RETURN(xamlType->GetProperty(c_strPath, outXamlProperty));
    }
    else if (ssTypeName.Equals(STR_LEN_PAIR(L"TemplateBinding")))
    {
        IFC_RETURN(xamlType->GetProperty(c_strProperty, outXamlProperty));
    }
    else if (ssTypeName.Equals(STR_LEN_PAIR(L"RelativeSource")))
    {
        IFC_RETURN(xamlType->GetProperty(c_strMode, outXamlProperty));
    }
    else
    {
        // This shouldn't be able to happen because the scanner
        // will have already checked that it is a valid ME type.
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strUnknownPositionalParameter, L"UnknownPositionalParameter");
        IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_MES_GENERIC, c_strUnknownPositionalParameter));
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

HRESULT MePullParser::Logic_StartPositionalParameters()
{
    return E_NOTIMPL;
}

HRESULT MePullParser::Logic_EndPositionalParameters()
{
    return E_NOTIMPL;
}

HRESULT MePullParser::GetErrorService(std::shared_ptr<ParserErrorReporter>& outErrorService)
{
    std::shared_ptr<XamlSchemaContext> schemaContext;

    IFC_RETURN(GetSchemaContext(schemaContext));
    IFC_RETURN(schemaContext->GetErrorService(outErrorService));

    return S_OK;

}

HRESULT MePullParser::ReportError(XUINT32 errorCode)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, m_Scanner.LineNumber(), m_Scanner.LinePosition()));

    return S_OK;
}

HRESULT MePullParser::ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, m_Scanner.LineNumber(), m_Scanner.LinePosition(), inssParam1));

    return S_OK;
}

HRESULT MePullParser::ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1, _In_ const xstring_ptr& inssParam2)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, m_Scanner.LineNumber(), m_Scanner.LinePosition(), inssParam1, inssParam2));

    return S_OK;
}



