// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef XAMLBINARYFORMATVALIDATOR_H_
#define XAMLBINARYFORMATVALIDATOR_H_

class XamlBinaryFormatValidator : public XamlReader
{
private:
    // Various terminal/nonterminal symbols the parser might encounter
    enum XamlNodeStreamSymbol
    {
        // catchall
        T_UNKNOWN,

        // Terminals
        T_END_ELEMENT_NODE,
        T_END_OF_ATTRIBUTES_NODE,
        T_END_PROPERTY_NODE,
        T_EOS,
        T_PREFIX_DEFINITION_NODE,
        T_START_ELEMENT_NODE,
        T_START_MULTI_ITEM_PROPERTY_NODE,
        T_START_SINGLE_ITEM_PROPERTY_NODE,
        T_START_XNAME_DIRECTIVE_PROPERTY_NODE,
        T_START_XUID_DIRECTIVE_PROPERTY_NODE,
        T_START_X_DIRECTIVE_PROPERTY_NODE,
        T_TEXTVALUE_NODE,
        T_VALUE_NODE,

        // Non-terminals
        NT_DIRECTIVE_PROPERTY,
        NT_DIRECTIVE_PROPERTY_CONTENT,
        NT_DIRECTIVE_PROPERTIES,
        NT_DOCUMENT,
        NT_ELEMENT,
        NT_MULTI_ITEM_PROPERTY,
        NT_NAME_DIRECTIVE_PROPERTY,
        NT_PROPERTY,
        NT_PROPERTY_CONTENT,
        NT_SINGLE_ITEM_PROPERTY,
        NT_UID_DIRECTIVE_PROPERTY,

        // Sentinel symbols
        S_PREFIX_DEFINITION_NODE_ZERO_OR_MORE,
        S_PROPERTY_CONTENT_ZERO_OR_MORE,
        S_PROPERTY_ZERO_OR_MORE,
        S_DIRECTIVE_PROPERTY_ZERO_OR_MORE,
    };

private:

    _Check_return_ HRESULT ConvertNodeToSymbol(
        _In_ const XamlNode& node, 
        _Out_ XamlNodeStreamSymbol* symbol
        );
    
    // Called when parser encounters an error to set error information on private members
    // Can be useful for debugging, and can be expanded later into full-fledged logging if needed
    _Check_return_ HRESULT SetErrorInfo(
        _In_ const XamlNode& node, 
        _In_ const xstring_ptr& spErrorMessage
        )
    {
        m_errorXamlNode = node;
        m_spErrorMessage = spErrorMessage;
        RRETURN(S_OK);
    }

public:
    XamlBinaryFormatValidator(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ const std::shared_ptr<XamlReader>& spXamlReader
        )
        : m_spXamlSchemaContext(spXamlSchemaContext)
        , m_spXamlReader(spXamlReader)
    {
    }

    //
    // XamlReader Interface Implementation
    //
    _Check_return_ static HRESULT Create(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ const std::shared_ptr<XamlReader>& spXamlReader,
        _Out_ std::shared_ptr<XamlBinaryFormatValidator>& spXamlBinaryFormatValidator
        );

    ~XamlBinaryFormatValidator() override;
    _Check_return_ HRESULT Read() override;
    const XamlNode& CurrentNode() override;
    HRESULT set_NextIndex(XUINT32 uiIndex) override   { ASSERT(FALSE); RRETURN(E_FAIL); }
    HRESULT get_NextIndex(XUINT32 *puiIndex) override { ASSERT(FALSE); RRETURN(E_FAIL); }

    _Check_return_ HRESULT GetSchemaContext(_In_ std::shared_ptr<XamlSchemaContext>& outSchemaContext) override
    {
        outSchemaContext = m_spXamlSchemaContext;
        RRETURN(S_OK);   
    }

private:
    std::shared_ptr<XamlReader>        m_spXamlReader;
    std::shared_ptr<XamlSchemaContext> m_spXamlSchemaContext;
    xstack<XamlNodeStreamSymbol>   m_stack;

    XamlNode                       m_errorXamlNode;
    xstring_ptr          m_spErrorMessage;
};

#endif

