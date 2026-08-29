// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//
// A settings class for the Parser.
// 
// Describe parser settings that apply to the Parse Context when calling LoadXaml
// 
// NOTE: 
// Please avoid adding parameters to CParser::LoadXaml and add them here instead.
//
class CParserSettings
{
private:
    bool m_bForceUtf16;
    bool m_bRequireDefaultNamespace;
    bool m_bExpandTemplatesDuringParse;
    bool m_bUseXamlResourceUri;
    bool m_bCreateNamescope;

    IPALUri *m_pXamlResourceUri;
    CDependencyObject *m_pExistingFrameworkRoot;

public:
    CParserSettings() 
    : m_bForceUtf16(false)
    , m_bRequireDefaultNamespace(false)
    , m_bExpandTemplatesDuringParse(false)
    , m_pExistingFrameworkRoot(nullptr)
    , m_pXamlResourceUri(nullptr)
    , m_bUseXamlResourceUri(FALSE)
    , m_bCreateNamescope(true)
    {     
    }

    void set_IsUtf16Encoded(const bool value)    { m_bForceUtf16 = value; }
    const bool get_IsUtf16Encoded() const       { return m_bForceUtf16; }

    void set_RequireDefaultNamespace(const bool value) { m_bRequireDefaultNamespace = value; }
    const bool get_RequireDefaultNamespace() const    { return m_bRequireDefaultNamespace; }

    void set_ExpandTemplatesDuringParse(const bool value) { m_bExpandTemplatesDuringParse = value; }
    const bool get_ExpandTemplatesDuringParse() const    { return m_bExpandTemplatesDuringParse; }

    void set_XamlResourceUri(IPALUri *pValue) { m_pXamlResourceUri = pValue; }
    IPALUri* get_XamlResourceUri() const      { return m_pXamlResourceUri; }

    void set_ExistingFrameworkRoot(CDependencyObject *pValue) { m_pExistingFrameworkRoot = pValue; }
    CDependencyObject* get_ExistingFrameworkRoot() const      { return m_pExistingFrameworkRoot; }

    void set_UseXamlResourceUri(const bool value) { m_bUseXamlResourceUri = value; }
    const bool get_UseXamlResourceUri() const    { return m_bUseXamlResourceUri; }
    void set_CreateNamescope(const bool value) { m_bCreateNamescope = value; }
    const bool get_CreateNamescope() const    { return m_bCreateNamescope; }

};
