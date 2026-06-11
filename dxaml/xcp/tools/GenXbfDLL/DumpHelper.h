// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct IStream;
class XamlBinaryFormatReader2;

class DumpHelper
{
public:
    static HRESULT DumpXbfStream(
        _In_  IStream *pXbfStream,
        _In_  IStream *pOutStream,
        _Out_ UINT32  *puiErrorCode);

private:
    static HRESULT CreateXbf2Reader(
        _In_ IStream *pXbfStream,
        _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spVersion2Reader);
};