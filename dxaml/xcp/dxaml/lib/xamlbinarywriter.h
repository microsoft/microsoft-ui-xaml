// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef XAMLBINARYWRITER_H__
#define XAMLBINARYWRITER_H__

namespace DirectUI
{
    class XamlBinaryWriterFactory
        :
        public xaml_markup::IXamlBinaryWriterStatics,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(XamlBinaryWriterFactory, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(XamlBinaryWriterFactory, xaml_markup::IXamlBinaryWriterStatics)
        END_INTERFACE_MAP(XamlBinaryWriterFactory, ctl::AbstractActivationFactory)

    public:

        // Properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.
        IFACEMETHOD(Write)(_In_ wfc::IVector<wsts::IRandomAccessStream*>* inputStreams, _In_ wfc::IVector<wsts::IRandomAccessStream*>* outputStreams, _In_ xaml_markup::IXamlMetadataProvider* xamlMetadataProvider, _Out_ xaml_markup::XamlBinaryWriterErrorInformation* returnValue) override;

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;


    private:

        // Customized static properties.

        // Custom static methods.
        virtual _Check_return_ HRESULT WriteImpl(_In_ wfc::IVector<wsts::IRandomAccessStream*>* inputStreams, _In_ wfc::IVector<wsts::IRandomAccessStream*>* outputStreams, _In_ xaml_markup::IXamlMetadataProvider* xamlMetadataProvider, _Out_ xaml_markup::XamlBinaryWriterErrorInformation* returnValue);

        virtual _Check_return_ HRESULT ReadBytesFromInputStream(
                _In_                                 wsts::IInputStream  *pInputStream,
                _In_                                 XUINT32                                   cBufferSize,
                _Out_                                XUINT32                                  *pcBytesRead,
                _Outptr_result_bytebuffer_((*pcBytesRead))   XBYTE                                   **ppBuffer);

        virtual _Check_return_ HRESULT WriteBytesToOutputStream(
                _In_                     wsts::IOutputStream *pOutputStream,
                _In_                     XUINT32                                   cBufferSize,
                _In_reads_bytes_(cBufferSize) XBYTE                                    *pBuffer);
    };
}

#endif
