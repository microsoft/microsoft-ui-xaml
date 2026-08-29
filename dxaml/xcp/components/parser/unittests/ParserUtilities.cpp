// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ParserUtilities.h"
#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include <XamlLogging.h>
#include <WexTestClass.h>
#include <MemoryStreamBuffer.h>
#include <WinStream.h>

using namespace DirectUI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    // use the xaml text parser to generate a set of XamlNodes
    std::vector<XamlNode> ParserUtilities::ParseTextXaml(const xstring_ptr& strXamlFragment)
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        std::shared_ptr<XamlTextReader> spXamlTextReader;
        std::vector<XamlNode> xamlNodeList;
        XUINT8* pbResourceBuffer = nullptr;
        XUINT32 cbResourceBuffer = 0;
        HRESULT hr = S_OK;

        pbResourceBuffer = reinterpret_cast<XUINT8*>(const_cast<WCHAR*>(strXamlFragment.GetBuffer()));
        cbResourceBuffer = strXamlFragment.GetCount() * 2;

        // construct schema context with no custom error reporter
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);

        VERIFY_SUCCEEDED(XamlTextReader::Create(
            spSchemaContext,
            XamlTextReaderSettings(false /* requireDefaultNamespace */, true /* shouldProcessUid */, false /*bForceUtf16*/),
            cbResourceBuffer,
            pbResourceBuffer,
            spXamlTextReader));

        std::shared_ptr<XamlSchemaContext> schemaContextFromTextReader;
        VERIFY_SUCCEEDED(spXamlTextReader->GetSchemaContext(schemaContextFromTextReader));
        VERIFY_IS_TRUE(schemaContextFromTextReader == spSchemaContext);

        while ((hr = spXamlTextReader->Read()) == S_OK)
        {
            XamlNode node = spXamlTextReader->CurrentNode();
            xamlNodeList.push_back(node);
        }

        return xamlNodeList;
    }

    std::shared_ptr<XamlSchemaContext> ParserUtilities::GetSchemaContext(const std::shared_ptr<ParserErrorReporter>& customErrorReporter)
    {
        XamlAssemblyToken assemblyToken;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;

        if (customErrorReporter)
        {
            VERIFY_SUCCEEDED(XamlSchemaContext::Create(GetMockParserServices(), customErrorReporter, spSchemaContext));
        }
        else
        {
            VERIFY_SUCCEEDED(XamlSchemaContext::Create(GetMockParserServices(), spSchemaContext));
        }

        return spSchemaContext;
    }

    IParserCoreServices* ParserUtilities::GetMockParserServices()
    {
        static IParserCoreServices *pServices = new MockParserCoreServices();
        return pServices;
    }

    xref_ptr<IPALStream> ParserUtilities::GetPALStreamFromPALMemory(_In_ xref_ptr<IPALMemory>& palMemory)
    {
        xref_ptr<CWinDataStreamBuffer> winDataStreamBuffer;
        xref_ptr<CWinDataStream> winDataStream;

        VERIFY_SUCCEEDED(CWinDataStreamBuffer::CreateFromIPalMemory(palMemory.get(), winDataStreamBuffer.ReleaseAndGetAddressOf()));
        VERIFY_SUCCEEDED(CWinDataStream::Create(winDataStreamBuffer.get(), winDataStream.ReleaseAndGetAddressOf()));

        return winDataStream;
    }

    xref_ptr<IPALMemory> ParserUtilities::GetPALMemoryFromBuffer(_In_ uint32_t cBuffer, _In_ uint8_t* buffer)
    {
        return make_xref<SimpleMemoryBuffer>(buffer, cBuffer);
    }

} } } } }

