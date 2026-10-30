// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlTextParserUnitTests.h"
#include "ParserUtilities.h"
#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include <XamlLogging.h>
#include <ObjectWriter.h>
#include <ObjectWriterSettings.h>
#include <ObjectWriterNode.h>
#include <ObjectWriterNodeList.h>
#include <XamlPredicateHelpers.h>

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

    void XamlTextParserUnitTests::ParseNamedXStringInXaml()
    {
        // x:Name must remain a directive even when its namespace also owns the string element.
        ParserUtilities parserUtils;
        const WCHAR* fragments[] =
        {
            L"<x:String xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='named'>stringValue</x:String>",
            L"<x:String xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:ConnectionId='2' x:Key='key' x:Name='named'>stringValue</x:String>",
            L"<s:String xmlns:s='http://schemas.microsoft.com/winfx/2006/xaml' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='named'>stringValue</s:String>"
        };

        for (const WCHAR* fragment : fragments)
        {
            xstring_ptr markup;
            VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(fragment, static_cast<XUINT32>(wcslen(fragment)), &markup));
            auto nodes = parserUtils.ParseTextXaml(markup);
            bool hasNameDirective = false;
            bool hasInitialization = false;
            bool hasStringValue = false;

            for (const auto& node : nodes)
            {
                if (node.get_NodeType() == xntStartProperty)
                {
                    const auto& property = node.get_Property();
                    if (property->IsDirective())
                    {
                        hasNameDirective |= std::static_pointer_cast<DirectiveProperty>(property)->get_DirectiveKind() == xdName;
                    }
                    else if (property->IsImplicit())
                    {
                        hasInitialization |= std::static_pointer_cast<ImplicitProperty>(property)->get_ImplicitPropertyType() == iptInitialization;
                    }
                }
                else if (node.get_NodeType() == xntText)
                {
                    xstring_ptr text;
                    VERIFY_SUCCEEDED(node.get_Text()->get_Text(&text));
                    hasStringValue |= text.Equals(XSTRING_PTR_EPHEMERAL(L"stringValue"));
                }
            }

            VERIFY_IS_TRUE(hasNameDirective);
            VERIFY_IS_TRUE(hasInitialization);
            VERIFY_IS_TRUE(hasStringValue);
            VERIFY_IS_TRUE(nodes.back().get_NodeType() == xntEndObject);
        }
    }

    void XamlTextParserUnitTests::ParseEmptyNamedXStringInXaml()
    {
        // An absent text initializer must not prevent the name directive from being parsed.
        ParserUtilities parserUtils;
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlBuffer,
            L"<x:String xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='named'/>");
        auto nodes = parserUtils.ParseTextXaml(c_strXamlBuffer);
        bool hasNameDirective = false;

        for (const auto& node : nodes)
        {
            if (node.get_NodeType() == xntStartProperty && node.get_Property()->IsDirective())
            {
                hasNameDirective |= std::static_pointer_cast<DirectiveProperty>(node.get_Property())->get_DirectiveKind() == xdName;
            }
        }

        VERIFY_IS_TRUE(hasNameDirective);
        VERIFY_IS_TRUE(nodes.back().get_NodeType() == xntEndObject);
    }

    void XamlTextParserUnitTests::EncodeNamedObjectBeforeConditionalProperty()
    {
        ParserUtilities parserUtils;
        std::shared_ptr<ParserErrorReporter> errorReporter;
        auto schema = parserUtils.GetSchemaContext(errorReporter);
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlBuffer,
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L" xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L" xmlns:bogus_contract='http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(83589481-75C4-465C-AB94-3182F13778BF,1)'>"
            L"<TextBlock xml:lang='en-US' bogus_contract:Text='ignored'/>"
            L"<TextBlock bogus_contract:Name='cardinal' x:Name='chop' xml:lang='en-US'/>"
            L"</StackPanel>");
        std::shared_ptr<XamlTextReader> reader;
        VERIFY_SUCCEEDED(XamlTextReader::Create(
            schema,
            XamlTextReaderSettings(false, true, false),
            c_strXamlBuffer.GetCount() * sizeof(WCHAR),
            reinterpret_cast<XUINT8*>(const_cast<WCHAR*>(c_strXamlBuffer.GetBuffer())),
            reader));

        ObjectWriterSettings settings;
        settings.set_EnableEncoding(true);
        std::shared_ptr<ObjectWriter> writer;
        VERIFY_SUCCEEDED(ObjectWriter::Create(schema, settings, writer));
        HRESULT hr;
        while ((hr = reader->Read()) == S_OK)
        {
            VERIFY_SUCCEEDED(writer->WriteNode(reader->CurrentNode()));
        }
        VERIFY_ARE_EQUAL(S_FALSE, hr);

        auto nodeList = writer->GetNodeList();
        VERIFY_SUCCEEDED(nodeList->Optimize());

        unsigned int conditionalDepth = 0;
        bool unconditionalNameRegistered = false;
        bool conditionalNameRegistered = false;
        bool textBlockInitialized = false;
        unsigned int languageValuesRecorded = 0;
        for (const auto& node : nodeList->GetNodeList())
        {
            if (node.GetNodeType() == ObjectWriterNodeType::BeginConditionalScope)
            {
                ++conditionalDepth;
            }
            else if (node.GetNodeType() == ObjectWriterNodeType::EndConditionalScope)
            {
                VERIFY_IS_TRUE(conditionalDepth > 0);
                --conditionalDepth;
            }
            else if (node.GetNodeType() == ObjectWriterNodeType::CreateTypeBeginInit ||
                node.GetNodeType() == ObjectWriterNodeType::PushScopeCreateTypeBeginInit ||
                node.GetNodeType() == ObjectWriterNodeType::CreateType ||
                node.GetNodeType() == ObjectWriterNodeType::PushScopeCreateType)
            {
                if (node.GetXamlType()->get_TypeToken() == XamlTypeToken(tpkNative, KnownTypeIndex::TextBlock))
                {
                    textBlockInitialized = node.GetNodeType() == ObjectWriterNodeType::CreateTypeBeginInit ||
                        node.GetNodeType() == ObjectWriterNodeType::PushScopeCreateTypeBeginInit;
                }
            }
            else if ((node.GetNodeType() == ObjectWriterNodeType::SetValue ||
                node.GetNodeType() == ObjectWriterNodeType::SetValueConstant) &&
                node.GetXamlProperty()->get_PropertyToken() == XamlPropertyToken(tpkNative, KnownPropertyIndex::FrameworkElement_Language))
            {
                VERIFY_IS_TRUE(node.GetNodeType() == ObjectWriterNodeType::SetValueConstant);
                VERIFY_IS_TRUE(node.GetValue()->GetValue().GetType() == valueString);
                VERIFY_IS_TRUE(node.GetValue()->GetValue().AsString().Equals(XSTRING_PTR_EPHEMERAL(L"en-US")));
                VERIFY_ARE_EQUAL(0u, conditionalDepth);
                VERIFY_IS_TRUE(textBlockInitialized);
                ++languageValuesRecorded;
            }
            else if (node.GetNodeType() == ObjectWriterNodeType::SetName)
            {
                const auto& name = node.GetValue()->GetValue().AsString();
                if (name.Equals(XSTRING_PTR_EPHEMERAL(L"chop")))
                {
                    VERIFY_ARE_EQUAL(0u, conditionalDepth);
                    unconditionalNameRegistered = true;
                }
                else if (name.Equals(XSTRING_PTR_EPHEMERAL(L"cardinal")))
                {
                    VERIFY_ARE_EQUAL(1u, conditionalDepth);
                    conditionalNameRegistered = true;
                }
            }
        }
        VERIFY_ARE_EQUAL(0u, conditionalDepth);
        VERIFY_IS_TRUE(unconditionalNameRegistered);
        VERIFY_IS_TRUE(conditionalNameRegistered);
        VERIFY_ARE_EQUAL(2u, languageValuesRecorded);
    }

    void XamlTextParserUnitTests::EncodeDeferredTextBlockLanguage()
    {
        ParserUtilities parserUtils;
        std::shared_ptr<ParserErrorReporter> errorReporter;
        auto schema = parserUtils.GetSchemaContext(errorReporter);
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlBuffer,
            L"<TextBlock xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xml:lang='en-US'/>");
        std::shared_ptr<XamlTextReader> reader;
        VERIFY_SUCCEEDED(XamlTextReader::Create(
            schema,
            XamlTextReaderSettings(false, true, false),
            c_strXamlBuffer.GetCount() * sizeof(WCHAR),
            reinterpret_cast<XUINT8*>(const_cast<WCHAR*>(c_strXamlBuffer.GetBuffer())),
            reader));
        ObjectWriterSettings settings;
        settings.set_EnableEncoding(true);
        std::shared_ptr<ObjectWriter> writer;
        VERIFY_SUCCEEDED(ObjectWriter::Create(schema, settings, writer));
        HRESULT hr;
        while ((hr = reader->Read()) == S_OK)
        {
            if (reader->CurrentNode().get_NodeType() == xntEndObject)
            {
                for (const auto& node : writer->GetNodeList()->GetNodeList())
                {
                    VERIFY_IS_TRUE(node.GetNodeType() != ObjectWriterNodeType::CreateType &&
                        node.GetNodeType() != ObjectWriterNodeType::CreateTypeWithInitialValue);
                }
            }
            VERIFY_SUCCEEDED(writer->WriteNode(reader->CurrentNode()));
        }
        VERIFY_ARE_EQUAL(S_FALSE, hr);
        auto nodeList = writer->GetNodeList();
        VERIFY_SUCCEEDED(nodeList->Optimize());

        unsigned int initializations = 0;
        bool languageAssigned = false;
        for (const auto& node : nodeList->GetNodeList())
        {
            if (node.GetNodeType() == ObjectWriterNodeType::CreateTypeBeginInit ||
                node.GetNodeType() == ObjectWriterNodeType::PushScopeCreateTypeBeginInit)
            {
                VERIFY_IS_TRUE(node.GetXamlType()->get_TypeToken() == XamlTypeToken(tpkNative, KnownTypeIndex::TextBlock));
                ++initializations;
            }
            else if ((node.GetNodeType() == ObjectWriterNodeType::SetValue ||
                node.GetNodeType() == ObjectWriterNodeType::SetValueConstant) &&
                node.GetXamlProperty()->get_PropertyToken() == XamlPropertyToken(tpkNative, KnownPropertyIndex::FrameworkElement_Language))
            {
                VERIFY_ARE_EQUAL(1u, initializations);
                VERIFY_IS_TRUE(node.GetNodeType() == ObjectWriterNodeType::SetValueConstant);
                VERIFY_IS_TRUE(node.GetValue()->GetValue().GetType() == valueString);
                VERIFY_IS_TRUE(node.GetValue()->GetValue().AsString().Equals(XSTRING_PTR_EPHEMERAL(L"en-US")));
                languageAssigned = true;
            }
        }
        VERIFY_ARE_EQUAL(1u, initializations);
        VERIFY_IS_TRUE(languageAssigned);
    }

    void XamlTextParserUnitTests::EncodeSavedDirectivesOnContainer()
    {
        ParserUtilities parserUtils;
        std::shared_ptr<ParserErrorReporter> errorReporter;
        auto schema = parserUtils.GetSchemaContext(errorReporter);
        const WCHAR* fragments[] =
        {
            L"<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xml:lang='en-US'><Border.Child><TextBlock/></Border.Child></Border>",
            L"<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='container'><Border.Child><TextBlock/></Border.Child></Border>",
            L"<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='container' xml:lang='en-US'><Border.Child><TextBlock/></Border.Child></Border>"
        };

        for (const WCHAR* fragment : fragments)
        {
            xstring_ptr markup;
            VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(fragment, static_cast<XUINT32>(wcslen(fragment)), &markup));
            std::shared_ptr<XamlTextReader> reader;
            VERIFY_SUCCEEDED(XamlTextReader::Create(
                schema,
                XamlTextReaderSettings(false, true, false),
                markup.GetCount() * sizeof(WCHAR),
                reinterpret_cast<XUINT8*>(const_cast<WCHAR*>(markup.GetBuffer())),
                reader));
            ObjectWriterSettings settings;
            settings.set_EnableEncoding(true);
            std::shared_ptr<ObjectWriter> writer;
            VERIFY_SUCCEEDED(ObjectWriter::Create(schema, settings, writer));
            HRESULT hr;
            while ((hr = reader->Read()) == S_OK)
            {
                VERIFY_SUCCEEDED(writer->WriteNode(reader->CurrentNode()));
            }
            VERIFY_ARE_EQUAL(S_FALSE, hr);

            auto nodeList = writer->GetNodeList();
            unsigned int childAssignments = 0;
            for (const auto& node : nodeList->GetNodeList())
            {
                if (node.GetNodeType() == ObjectWriterNodeType::SetValue &&
                    node.GetXamlProperty()->get_PropertyToken() == XamlPropertyToken(tpkNative, KnownPropertyIndex::Border_Child))
                {
                    ++childAssignments;
                }
            }
            VERIFY_ARE_EQUAL(1u, childAssignments);
            VERIFY_SUCCEEDED(nodeList->Optimize());
        }
    }

    void XamlTextParserUnitTests::EncodeNamedStringConnectsInitializedValue()
    {
        // Exercise the textual connection IDs produced by the XAML reader.
        VerifyStringConnection(false);
    }

    void XamlTextParserUnitTests::EncodeNamedStringConnectsConvertedId()
    {
        // Also accept connection IDs converted before they reach the object writer.
        VerifyStringConnection(true);
    }

    void XamlTextParserUnitTests::EncodeNamedStringBeforeConditionalProperty()
    {
        for (bool numericConnection : { false, true })
        {
            VerifyStringConnection(numericConnection, true);
        }
    }

    void XamlTextParserUnitTests::EncodeConditionalNamedStringBeforeConditionalProperty()
    {
        VerifyStringConnection(false, true, true);
    }

    void XamlTextParserUnitTests::VerifyStringConnection(bool numericConnection, bool conditionalMember, bool conditionalObject)
    {
        // Connection callbacks and name registration must observe one fully initialized string.
        ParserUtilities parserUtils;
        std::shared_ptr<ParserErrorReporter> errorReporter;
        auto schema = parserUtils.GetSchemaContext(errorReporter);
        std::shared_ptr<XamlType> stringType;
        std::shared_ptr<XamlNamespace> xamlNamespace;
        std::shared_ptr<DirectiveProperty> connectionProperty;
        std::shared_ptr<DirectiveProperty> nameProperty;
        std::shared_ptr<ImplicitProperty> initializationProperty;
        VERIFY_SUCCEEDED(schema->get_StringXamlType(stringType));
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strMarkupNamespace, L"http://schemas.microsoft.com/winfx/2006/xaml");
        VERIFY_SUCCEEDED(schema->GetXamlXmlNamespace(c_strMarkupNamespace, xamlNamespace));
        VERIFY_SUCCEEDED(schema->get_X_ConnectionIdProperty(connectionProperty));
        VERIFY_SUCCEEDED(schema->get_X_NameProperty(nameProperty));
        VERIFY_SUCCEEDED(schema->get_InitializationProperty(initializationProperty));

        std::shared_ptr<::Parser::XamlPredicateAndArgs> predicate;
        std::shared_ptr<UnknownProperty> conditionalProperty;
        std::shared_ptr<XamlType> containerType;
        std::shared_ptr<XamlProperty> contentProperty;
        if (conditionalMember || conditionalObject)
        {
            std::shared_ptr<XamlType> predicateType;
            VERIFY_SUCCEEDED(schema->GetXamlType(XamlTypeToken(tpkNative, KnownTypeIndex::IsApiContractPresent), predicateType));
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strArguments, L"Windows.Foundation.UniversalApiContract,999");
            predicate = std::make_shared<::Parser::XamlPredicateAndArgs>(predicateType, c_strArguments);
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strFutureProperty, L"FutureProperty");
            VERIFY_SUCCEEDED(UnknownProperty::Create(schema, c_strFutureProperty, stringType, xamlNamespace, FALSE, conditionalProperty));
        }
        if (conditionalObject)
        {
            VERIFY_SUCCEEDED(schema->GetXamlType(XamlTypeToken(tpkNative, KnownTypeIndex::ContentControl), containerType));
            VERIFY_SUCCEEDED(schema->GetXamlProperty(XamlPropertyToken(tpkNative, KnownPropertyIndex::ContentControl_Content), contentProperty));
        }

        for (bool empty : { false, true })
        {
            ObjectWriterSettings settings;
            settings.set_EnableEncoding(true);
            std::shared_ptr<ObjectWriter> writer;
            VERIFY_SUCCEEDED(ObjectWriter::Create(schema, settings, writer));
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strPrefix, L"x");
            VERIFY_SUCCEEDED(writer->WriteNamespace(c_strPrefix, xamlNamespace));
            if (conditionalObject)
            {
                VERIFY_SUCCEEDED(writer->WriteObject(containerType, false));
                VERIFY_SUCCEEDED(writer->WriteMember(contentProperty));
                VERIFY_SUCCEEDED(writer->WriteConditionalScope(predicate));
            }
            VERIFY_SUCCEEDED(writer->WriteObject(stringType, false));
            // Model the reader's text-valued directives separately from preconverted IDs.
            auto writeProperty = [&](const std::shared_ptr<XamlProperty>& property, const xstring_ptr_view& text)
            {
                auto value = std::make_shared<XamlQualifiedObject>();
                xstring_ptr valueText;
                VERIFY_SUCCEEDED(text.Promote(&valueText));
                VERIFY_SUCCEEDED(value->CreateFromXStringPtr(valueText, value));
                VERIFY_SUCCEEDED(writer->WriteMember(property));
                VERIFY_SUCCEEDED(writer->WriteValue(value));
                VERIFY_SUCCEEDED(writer->WriteEndMember());
            };
            if (numericConnection)
            {
                auto connection = std::make_shared<XamlQualifiedObject>(XamlTypeToken(tpkNative, KnownTypeIndex::Int32));
                CValue value;
                value.SetSigned(2);
                VERIFY_SUCCEEDED(connection->SetValue(value));
                VERIFY_SUCCEEDED(writer->WriteMember(connectionProperty));
                VERIFY_SUCCEEDED(writer->WriteValue(connection));
                VERIFY_SUCCEEDED(writer->WriteEndMember());
            }
            else
            {
                writeProperty(connectionProperty, XSTRING_PTR_EPHEMERAL(L"2"));
            }
            writeProperty(nameProperty, XSTRING_PTR_EPHEMERAL(L"named"));
            if (!empty)
            {
                writeProperty(initializationProperty, XSTRING_PTR_EPHEMERAL(L"stringValue"));
            }
            if (conditionalMember)
            {
                VERIFY_SUCCEEDED(writer->WriteConditionalScope(predicate));
                writeProperty(conditionalProperty, XSTRING_PTR_EPHEMERAL(L"ignored"));
                VERIFY_SUCCEEDED(writer->WriteEndConditionalScope());
            }
            VERIFY_SUCCEEDED(writer->WriteEndObject());
            if (conditionalObject)
            {
                VERIFY_SUCCEEDED(writer->WriteEndConditionalScope());
                VERIFY_SUCCEEDED(writer->WriteEndMember());
                VERIFY_SUCCEEDED(writer->WriteEndObject());
            }

            // Check construction order before the optimizer combines the creation nodes.
            auto nodeList = writer->GetNodeList();
            unsigned int creations = 0;
            const unsigned int objectConditionalDepth = conditionalObject ? 1u : 0u;
            unsigned int conditionalDepth = 0;
            bool initialized = false;
            bool connected = false;
            for (const auto& node : nodeList->GetNodeList())
            {
                if (node.GetNodeType() == ObjectWriterNodeType::BeginConditionalScope)
                {
                    ++conditionalDepth;
                }
                else if (node.GetNodeType() == ObjectWriterNodeType::EndConditionalScope)
                {
                    VERIFY_IS_TRUE(conditionalDepth > 0);
                    --conditionalDepth;
                }
                else if (node.GetNodeType() == ObjectWriterNodeType::CreateType ||
                    node.GetNodeType() == ObjectWriterNodeType::CreateTypeWithInitialValue)
                {
                    if (node.GetXamlType()->get_TypeToken() != stringType->get_TypeToken())
                    {
                        continue;
                    }
                    VERIFY_ARE_EQUAL(objectConditionalDepth, conditionalDepth);
                    VERIFY_IS_TRUE(empty ? node.GetNodeType() == ObjectWriterNodeType::CreateType :
                        node.GetNodeType() == ObjectWriterNodeType::CreateTypeWithInitialValue);
                    ++creations;
                    VERIFY_IS_FALSE(connected);
                }
                else if (node.GetNodeType() == ObjectWriterNodeType::BeginInit && creations > 0)
                {
                    VERIFY_ARE_EQUAL(objectConditionalDepth, conditionalDepth);
                    initialized = true;
                }
                else if (node.GetNodeType() == ObjectWriterNodeType::SetName)
                {
                    VERIFY_ARE_EQUAL(objectConditionalDepth, conditionalDepth);
                    VERIFY_ARE_EQUAL(!empty, initialized);
                }
                else if (node.GetNodeType() == ObjectWriterNodeType::SetConnectionId)
                {
                    VERIFY_ARE_EQUAL(objectConditionalDepth, conditionalDepth);
                    VERIFY_ARE_EQUAL(1u, creations);
                    VERIFY_IS_TRUE(initialized);
                    connected = true;
                }
            }
            VERIFY_ARE_EQUAL(0u, conditionalDepth);
            VERIFY_ARE_EQUAL(1u, creations);
            VERIFY_IS_TRUE(connected);
            VERIFY_SUCCEEDED(nodeList->Optimize());

            initialized = false;
            bool named = false;
            connected = false;
            // Optimization must preserve initialization, connection IDs, and names.
            for (const auto& node : nodeList->GetNodeList())
            {
                if (node.GetNodeType() == ObjectWriterNodeType::BeginConditionalScope)
                {
                    ++conditionalDepth;
                }
                else if (node.GetNodeType() == ObjectWriterNodeType::EndConditionalScope)
                {
                    VERIFY_IS_TRUE(conditionalDepth > 0);
                    --conditionalDepth;
                }
                else if (node.GetNodeType() == ObjectWriterNodeType::CreateTypeWithConstantBeginInit ||
                    node.GetNodeType() == ObjectWriterNodeType::PushScopeCreateTypeWithConstantBeginInit ||
                    node.GetNodeType() == ObjectWriterNodeType::CreateTypeWithTypeConvertedConstantBeginInit ||
                    node.GetNodeType() == ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstantBeginInit)
                {
                    if (node.GetXamlType()->get_TypeToken() != stringType->get_TypeToken())
                    {
                        continue;
                    }
                    VERIFY_ARE_EQUAL(objectConditionalDepth, conditionalDepth);
                    VERIFY_IS_FALSE(empty);
                    VERIFY_IS_TRUE(node.GetXamlType()->get_TypeToken() == stringType->get_TypeToken());
                    const auto& value = node.GetValue()->GetValue().AsString();
                    VERIFY_IS_FALSE(value.IsNull());
                    VERIFY_IS_TRUE(value.Equals(XSTRING_PTR_EPHEMERAL(L"stringValue")));
                    initialized = true;
                }
                else if (node.GetNodeType() == ObjectWriterNodeType::CreateTypeBeginInit ||
                    node.GetNodeType() == ObjectWriterNodeType::PushScopeCreateTypeBeginInit)
                {
                    if (node.GetXamlType()->get_TypeToken() != stringType->get_TypeToken())
                    {
                        continue;
                    }
                    VERIFY_ARE_EQUAL(objectConditionalDepth, conditionalDepth);
                    VERIFY_IS_TRUE(empty);
                    initialized = true;
                }
                else if (node.GetNodeType() == ObjectWriterNodeType::SetConnectionId)
                {
                    VERIFY_ARE_EQUAL(objectConditionalDepth, conditionalDepth);
                    VERIFY_IS_TRUE(initialized);
                    VERIFY_ARE_EQUAL(2, node.GetValue()->GetValue().AsSigned());
                    connected = true;
                }
                else if (node.GetNodeType() == ObjectWriterNodeType::SetName)
                {
                    VERIFY_ARE_EQUAL(objectConditionalDepth, conditionalDepth);
                    VERIFY_IS_TRUE(initialized);
                    VERIFY_IS_TRUE(node.GetValue()->GetValue().AsString().Equals(XSTRING_PTR_EPHEMERAL(L"named")));
                    named = true;
                }
            }
            VERIFY_ARE_EQUAL(0u, conditionalDepth);
            VERIFY_IS_TRUE(initialized);
            VERIFY_IS_TRUE(connected);
            VERIFY_IS_TRUE(named);
        }
    }

    void XamlTextParserUnitTests::EncodeUnknownConditionalTypeWithConnectionId()
    {
        // Conditional unknown types must retain constructor-first connection ordering.
        ParserUtilities parserUtils;
        std::shared_ptr<ParserErrorReporter> errorReporter;
        auto schema = parserUtils.GetSchemaContext(errorReporter);
        std::shared_ptr<XamlNamespace> xamlNamespace;
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strNamespace, L"http://schemas.microsoft.com/winfx/2006/xaml/presentation");
        VERIFY_SUCCEEDED(schema->GetXamlXmlNamespace(c_strNamespace, xamlNamespace));
        std::shared_ptr<XamlType> containerType;
        VERIFY_SUCCEEDED(schema->GetXamlType(XamlTypeToken(tpkNative, KnownTypeIndex::ContentControl), containerType));
        std::shared_ptr<XamlProperty> contentProperty;
        VERIFY_SUCCEEDED(schema->GetXamlProperty(XamlPropertyToken(tpkNative, KnownPropertyIndex::ContentControl_Content), contentProperty));
        std::shared_ptr<XamlType> unknownType;
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strFutureControl, L"FutureControl");
        VERIFY_SUCCEEDED(UnknownType::Create(schema, xamlNamespace, c_strFutureControl, unknownType));
        std::shared_ptr<XamlType> predicateType;
        VERIFY_SUCCEEDED(schema->GetXamlType(XamlTypeToken(tpkNative, KnownTypeIndex::IsApiContractPresent), predicateType));
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strArguments, L"Windows.Foundation.UniversalApiContract,999");
        auto predicate = std::make_shared<::Parser::XamlPredicateAndArgs>(predicateType, c_strArguments);
        std::shared_ptr<DirectiveProperty> connectionProperty;
        VERIFY_SUCCEEDED(schema->get_X_ConnectionIdProperty(connectionProperty));
        auto connection = std::make_shared<XamlQualifiedObject>(XamlTypeToken(tpkNative, KnownTypeIndex::Int32));
        CValue value;
        value.SetSigned(2);
        VERIFY_SUCCEEDED(connection->SetValue(value));

        ObjectWriterSettings settings;
        settings.set_EnableEncoding(true);
        std::shared_ptr<ObjectWriter> writer;
        VERIFY_SUCCEEDED(ObjectWriter::Create(schema, settings, writer));
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strPrefix, L"p");
        VERIFY_SUCCEEDED(writer->WriteNamespace(c_strPrefix, xamlNamespace));
        VERIFY_SUCCEEDED(writer->WriteObject(containerType, false));
        VERIFY_SUCCEEDED(writer->WriteMember(contentProperty));
        VERIFY_SUCCEEDED(writer->WriteConditionalScope(predicate));
        VERIFY_SUCCEEDED(writer->WriteObject(unknownType, false));
        VERIFY_SUCCEEDED(writer->WriteMember(connectionProperty));
        VERIFY_SUCCEEDED(writer->WriteValue(connection));
        VERIFY_SUCCEEDED(writer->WriteEndMember());
        VERIFY_SUCCEEDED(writer->WriteEndObject());
        VERIFY_SUCCEEDED(writer->WriteEndConditionalScope());
        VERIFY_SUCCEEDED(writer->WriteEndMember());
        VERIFY_SUCCEEDED(writer->WriteEndObject());
        auto nodeList = writer->GetNodeList();
        VERIFY_SUCCEEDED(nodeList->Optimize());

        bool created = false;
        bool connected = false;
        for (const auto& node : nodeList->GetNodeList())
        {
            if (node.GetNodeType() == ObjectWriterNodeType::PushScopeCreateTypeBeginInit)
            {
                created |= node.GetXamlType() == unknownType;
            }
            else if (node.GetNodeType() == ObjectWriterNodeType::SetConnectionId)
            {
                VERIFY_IS_TRUE(created);
                VERIFY_ARE_EQUAL(2, node.GetValue()->GetValue().AsSigned());
                connected = true;
            }
        }
        VERIFY_IS_TRUE(created);
        VERIFY_IS_TRUE(connected);
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
