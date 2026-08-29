// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlBinaryFormatReader;
class XamlBinaryFormatReader2;
class XamlBinaryMetadataReader;
class XamlBinaryFormatWriter;
class XamlBinaryMetadataStore;
struct IPALMemory;

namespace Parser
{
    enum class XamlBufferType : uint8_t;

    _Check_return_ HRESULT
        CreateXamlBinaryFileReader(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ IPALMemory* pFileStream,
        _In_ XamlBufferType bufferType,
        _Out_ std::shared_ptr<XamlReader>& spReader,
        _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spVersion2Reader);
}

