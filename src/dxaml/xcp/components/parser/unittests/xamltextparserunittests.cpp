// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlTextParserUnitTests.h"
#include "ParserUtilities.h"
#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include <XamlLogging.h>

using namespace DirectUI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    void XamlTextParserUnitTests::Create()
    {
        ParserUtilities parserUtils;

        DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlBuffer, L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'/>");

        auto nodeList = parserUtils.ParseTextXaml(c_strXamlBuffer);
        VERIFY_IS_TRUE(nodeList.size() == 5);

        // xmlns='...'
        VERIFY_IS_TRUE(nodeList[0].get_NodeType() == xntNamespace);

        // xmlns:x='...'
        VERIFY_IS_TRUE(nodeList[1].get_NodeType() == xntNamespace);

        // Grid SO
        VERIFY_IS_TRUE(nodeList[2].get_NodeType() == xntStartObject);
        VERIFY_IS_TRUE(nodeList[2].get_XamlType()->get_TypeToken() == XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Grid)));

        // Grid EO
        VERIFY_IS_TRUE(nodeList[3].get_NodeType() == xntEndOfAttributes);
        VERIFY_IS_TRUE(nodeList[4].get_NodeType() == xntEndObject);
    }

    void XamlTextParserUnitTests::ParseXStringInXaml()
    {
        ParserUtilities parserUtils;

        DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlBuffer,
            L"<x:String xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>stringValue</x:String>");

        auto nodeList = parserUtils.ParseTextXaml(c_strXamlBuffer);
        VERIFY_IS_TRUE(nodeList.size() == 7);

        // xmlns:x='...'
        VERIFY_IS_TRUE(nodeList[0].get_NodeType() == xntNamespace);

        // <x:String>
        VERIFY_IS_TRUE(nodeList[1].get_NodeType() == xntStartObject);
        VERIFY_IS_TRUE(nodeList[1].get_XamlType()->get_TypeToken() == XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::String)));
        VERIFY_IS_TRUE(nodeList[2].get_NodeType() == xntEndOfAttributes);

        // Implicit Initialization Property
        VERIFY_IS_TRUE(nodeList[3].get_NodeType() == xntStartProperty);
        {
            VERIFY_IS_TRUE(nodeList[3].get_Property()->IsImplicit() == true);
            std::shared_ptr<ImplicitProperty> spImplicitProperty = std::static_pointer_cast<ImplicitProperty>(nodeList[3].get_Property());
            VERIFY_IS_TRUE(spImplicitProperty->get_ImplicitPropertyType() == iptInitialization);
        }

        // Text Value
        DECLARE_CONST_STRING_IN_TEST_CODE(c_stringValueText, L"stringValue");
        VERIFY_IS_TRUE(nodeList[4].get_NodeType() == xntText);
        {
            xstring_ptr nodeTextValue;
            VERIFY_SUCCEEDED(nodeList[4].get_Text()->get_Text(&nodeTextValue));
            VERIFY_IS_TRUE(nodeTextValue.Compare(c_stringValueText) == 0);
        }

        // End Implicit Initialization Property
        VERIFY_IS_TRUE(nodeList[5].get_NodeType() == xntEndProperty);

        // </x:String>
        VERIFY_IS_TRUE(nodeList[6].get_NodeType() == xntEndObject);
    }

    void XamlTextParserUnitTests::ParseIgnorableElements()
    {
        ParserUtilities parserUtils;

        DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlBuffer,
        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:mc='http://schemas.openxmlformats.org/markup-compatibility/2006' xmlns:d='http://schemas.microsoft.com/expression/blend/2008' mc:Ignorable='d' d:DesignWidth='640' d:DesignHeight='480' Width='100'><d:DesignGridRow><d:DesignGridRow.Content><d:FooBar Width='100'/></d:DesignGridRow.Content></d:DesignGridRow></Grid>" );

        auto nodeList = parserUtils.ParseTextXaml(c_strXamlBuffer);
        VERIFY_IS_TRUE(nodeList.size() == 9);
    }

    void XamlTextParserUnitTests::VerifyParseFailures()
    {
        ParserUtilities parserUtils;

        DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlBuffer,
        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Width='100/>");

        auto nodeList = parserUtils.ParseTextXaml(c_strXamlBuffer);
        VERIFY_IS_TRUE(nodeList.size() == 0);

        DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlBufferWithMarkupExtension,
        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Width='{StaticResource {}}'/>");

        nodeList = parserUtils.ParseTextXaml(c_strXamlBufferWithMarkupExtension);
        VERIFY_IS_TRUE(nodeList.size() == 2);
    }

} } } } }
