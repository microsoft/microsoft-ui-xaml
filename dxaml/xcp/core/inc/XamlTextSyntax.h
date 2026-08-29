// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// TODO: I'm not fully convinced that we need this class.
class XamlTextSyntax
{
private:
    XamlTypeToken m_sTypeConverterTypeToken;

public:

    XamlTextSyntax(XamlTypeToken sTypeConverterTypeToken)
        : m_sTypeConverterTypeToken(sTypeConverterTypeToken)
    {
    }

    const XamlTypeToken& get_TextSyntaxToken() const
    {
        return m_sTypeConverterTypeToken;
    }

    virtual ~XamlTextSyntax() {}

    // omitted:
    // HRESULT get_Name(_Out_ xstring_ptr* pstrOut);

    // TODO: add these static common Syntaxes to the XamlSchemaContext.
    static HRESULT get_NoSyntax(_Out_ std::shared_ptr<XamlTextSyntax>& outTextSyntax);
    static HRESULT get_StringSyntax(_Out_ std::shared_ptr<XamlTextSyntax>& outTextSyntax);
    static HRESULT get_EventSyntax(_Out_ std::shared_ptr<XamlTextSyntax>& outTextSyntax);
    static HRESULT get_Int32Syntax(_Out_ std::shared_ptr<XamlTextSyntax>& outTextSyntax);

protected:

    // omitted:
    //virtual HRESULT get_NameCore(_Out_ xstring_ptr* pstrOut) = 0;
    //virtual HRESULT set_NameCore(_Out_ xstring_ptr* pstrOut) = 0;

};


