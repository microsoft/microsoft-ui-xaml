// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "WinReaderUnitTests.h"
#include <ReaderString.h>
#include <WinReader.h>

#define LOG_OUTPUT(fmt, ...) WEX::Logging::Log::Comment(WEX::Common::String().Format(fmt, __VA_ARGS__))

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Framework {            

    const wchar_t* simpleXml =
        L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
        L"  <Button.Template>"
        L"      <ControlTemplate>"
        L"          <TextBlock x:Name='bindingTarget'/>"
        L"      </ControlTemplate>"
        L"  </Button.Template>"
        L"</Button>";

    void WinReaderUnitTests::ValidateCreation()
    {
        auto createdReader = XmlReaderWrapper::CreateLegacyXmlReaderWrapper();
        VERIFY_IS_NOT_NULL(createdReader.get());
    }

    void WinReaderUnitTests::ValidateReadBasicNodeTypes()
    {
        auto createdReader = XmlReaderWrapper::CreateLegacyXmlReaderWrapper();
        VERIFY_SUCCEEDED(createdReader->SetInput(static_cast<unsigned int>(wcslen(simpleXml)) * 2, 
            reinterpret_cast<const uint8_t*>(simpleXml), true));

        std::array<XmlNodeType, 12> expectedNodes 
        {
            XmlNodeType_Element,
            XmlNodeType_Whitespace,
            XmlNodeType_Element,
            XmlNodeType_Whitespace,
            XmlNodeType_Element,
            XmlNodeType_Whitespace,
            XmlNodeType_Element,
            XmlNodeType_Whitespace,
            XmlNodeType_EndElement,
            XmlNodeType_Whitespace,
            XmlNodeType_EndElement,
            XmlNodeType_EndElement
        };

        for (auto expectedNode : expectedNodes)
        {
            XmlNodeType node;
            VERIFY_SUCCEEDED(createdReader->Read(&node));
            VERIFY_ARE_EQUAL(expectedNode, node);
        }
    }

    void WinReaderUnitTests::ValidateGetNamespaceUri()
    {
        auto createdReader = XmlReaderWrapper::CreateLegacyXmlReaderWrapper();
        VERIFY_SUCCEEDED(createdReader->SetInput(static_cast<unsigned int>(wcslen(simpleXml)) * 2, 
            reinterpret_cast<const uint8_t*>(simpleXml), true));

        XmlNodeType node;
        VERIFY_SUCCEEDED(createdReader->Read(&node));

        ReaderString string;
        VERIFY_SUCCEEDED(createdReader->GetNamespaceUri(&string));
        VERIFY_IS_TRUE(wcscmp(L"http://schemas.microsoft.com/winfx/2006/xaml/presentation", string.Get()) == 0);
    }

    void WinReaderUnitTests::ValidateGetLocalName()
    {
        auto createdReader = XmlReaderWrapper::CreateLegacyXmlReaderWrapper();
        VERIFY_SUCCEEDED(createdReader->SetInput(static_cast<unsigned int>(wcslen(simpleXml)) * 2, 
            reinterpret_cast<const uint8_t*>(simpleXml), true));

        std::array<const wchar_t*, 4> expectedStrings
        {
            L"Button",
            L"Button.Template",
            L"ControlTemplate",
            L"TextBlock"
        };

        XmlNodeType node;
        auto expectedStringIter = expectedStrings.begin();
        do
        {
            VERIFY_SUCCEEDED(createdReader->Read(&node));

            if (node == XmlNodeType_Element)
            {
                ReaderString string;
                VERIFY_SUCCEEDED(createdReader->GetLocalName(&string));
                VERIFY_IS_TRUE(wcscmp(*expectedStringIter, string.Get()) == 0);
                expectedStringIter++;
            }
        } while (node != XmlNodeType_None);

        VERIFY_IS_TRUE(expectedStringIter == expectedStrings.end());
    }

    }
}}}}