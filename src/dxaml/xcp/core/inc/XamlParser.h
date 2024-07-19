// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlBinaryMetadata.h"

class XamlReader;
class XamlWriter;
class XamlSavedContext;
class ObjectWriterSettings;
class ObjectWriter;
class BinaryFormatObjectWriter;
class CParserSettings;
class CCoreServices;

namespace Parser
{
    struct XamlBuffer;
}

class CParser
{
public:
    static
    _Check_return_ HRESULT
    LoadXaml(
        _In_ CCoreServices *pCore,
        _In_ const CParserSettings& parserSettings,
        _In_ const Parser::XamlBuffer& buffer,
        _Outptr_ CDependencyObject **ppDependencyObject,
        _In_ const xstring_ptr_view& strSourceAssemblyName,
        _In_ const std::array<byte, Parser::c_xbfHashSize>& hashForBinaryXaml = { { 0 } }
        );
       
private:
    static
    _Check_return_ HRESULT
    LoadXamlCore(
        _In_ CCoreServices *pCore,
        _In_ const CParserSettings& parserSettings,
        _In_ const Parser::XamlBuffer& buffer,
        _Outptr_ CDependencyObject **ppDependencyObject,
        _In_ const xstring_ptr_view& strSourceAssemblyName,
        _In_ const std::array<byte, Parser::c_xbfHashSize>& hashForBinaryXaml = { { 0 } }
        );

    static
    _Check_return_ HRESULT
    CreateNamescope(
       _In_ CCoreServices *pCore,
       _In_ CDependencyObject *pFrameworkRoot, 
       _In_ const CParserSettings& parserSettings,
       _Out_ xref_ptr<INameScope>& spNameScope );
   
    static 
    _Check_return_ HRESULT
    CreateObjectWriter(
        _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
        _In_ const CParserSettings& parserSettings,
        _In_ bool fEnableEncoding,
        _In_ bool fCheckDuplicateProperty,
        _In_ const xref_ptr<IPALUri>& spBaseUri,
        _In_ const xref_ptr<INameScope> spNameScope,
        _Out_ std::shared_ptr<ObjectWriter>& spObjectWriter,
        _Out_ std::shared_ptr<BinaryFormatObjectWriter>& spBinaryFormatObjectWriter
        );

    static
    _Check_return_ HRESULT
    CreateBinaryFormatObjectWriter(
        _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
        _In_ const CParserSettings& parserSettings,
        _In_ bool fCheckDuplicateProperty,
        _In_ const xref_ptr<IPALUri>& spBaseUri,
        _In_ xref_ptr<INameScope> spNameScope, 
        _In_ const xstring_ptr& strHash, 
        _Out_ std::shared_ptr<BinaryFormatObjectWriter>& spBinaryFormatObjectWriter
        );

    
    static 
    _Check_return_ HRESULT
    GetUniqueName(
        _In_ const xref_ptr<IPALUri>& spBaseUri,
        _Out_ xstring_ptr* pstrUniqueName
        );
};