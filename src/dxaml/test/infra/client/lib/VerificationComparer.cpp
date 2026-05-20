// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include <memory.h>
#include <robuffer.h>

#include "VerificationComparer.h"
#include "Utilities.h"
#include "FileDiff.h"
#include "XamlTailored.h"

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Private {
    namespace Infrastructure {

        VerificationComparer::~VerificationComparer() { }

        bool ByteByByteComparer::CompareFiles(
            _In_ const wrl::Wrappers::HStringReference& s1,
            _In_ const wrl::Wrappers::HStringReference& s2)
        {
            m_file1 = s1.GetRawBuffer(nullptr);
            m_file2 = s2.GetRawBuffer(nullptr);

            wrl::ComPtr<wst::IStorageFile> spMasterFile = Utilities::GetStorageFile(s1.Get());
            wrl::ComPtr<wst::IStorageFile> spOutputFile = Utilities::GetStorageFile(s2.Get());

            wrl::ComPtr<wsts::IBuffer> spMasterFileBuffer;
            wrl::ComPtr<wsts::IBuffer> spOutputFileBuffer;

            Utilities::ReadFileBuffer(spMasterFile, &spMasterFileBuffer);
            Utilities::ReadFileBuffer(spOutputFile, &spOutputFileBuffer);

            unsigned int masterLength = 0;
            unsigned int outputLength = 0;

            wrl::ComPtr<::Windows::Storage::Streams::IBufferByteAccess> spOutputBufferByteAccess;
            wrl::ComPtr<::Windows::Storage::Streams::IBufferByteAccess> spMasterBufferByteAccess;

            byte* pMasterBuffer = nullptr;
            byte* pOutputBuffer = nullptr;

            LogThrow_IfFailed(spMasterFileBuffer->get_Length(&masterLength));
            LogThrow_IfFailed(spOutputFileBuffer->get_Length(&outputLength));

            LogThrow_IfFailed(spMasterFileBuffer.As(&spMasterBufferByteAccess));
            LogThrow_IfFailed(spOutputFileBuffer.As(&spOutputBufferByteAccess));

            LogThrow_IfFailed(spMasterBufferByteAccess->Buffer(&pMasterBuffer));
            LogThrow_IfFailed(spOutputBufferByteAccess->Buffer(&pOutputBuffer));

            bool doesOutputMatchMaster = (masterLength == outputLength);

            for (unsigned int byteIndex = 0; byteIndex < masterLength && doesOutputMatchMaster; ++byteIndex)
            {
                doesOutputMatchMaster = (pMasterBuffer[byteIndex] == pOutputBuffer[byteIndex]);
            }

            return doesOutputMatchMaster;
        }

        void ByteByByteComparer::OutputAdditionalResults(
            bool isXmlFile)
        {
            if (isXmlFile)
            {
                LOG_OUTPUT(L"Generating human-readable diff output of ASCII text files.");
                FileDiff::FileDiff fileDiff;
                fileDiff.Diff(m_file1.GetBuffer(), m_file2.GetBuffer());
            }
        }

        void ImageComparer::SetDiffFilePath(
            _In_ const wrl::Wrappers::HStringReference& diffFilePath)
        {
            m_diffFilePath = diffFilePath.GetRawBuffer(nullptr);
        }

        void ImageComparer::SetErrorFilePath(
            _In_ const wrl::Wrappers::HStringReference& errorFilePath)
        {
            m_errorFilePath = errorFilePath.GetRawBuffer(nullptr);
        }

        bool ImageComparer::CompareFiles(
            _In_ const wrl::Wrappers::HStringReference& s1,
            _In_ const wrl::Wrappers::HStringReference& s2)
        {
            auto wicBitmapSource1 = LoadWicBitmapSource(s1);
            auto wicBitmapSource2 = LoadWicBitmapSource(s2);

            auto width1 = 0u;
            auto height1 = 0u;
            LogThrow_IfFailed(wicBitmapSource1->GetSize(&width1, &height1));

            auto width2 = 0u;
            auto height2 = 0u;
            LogThrow_IfFailed(wicBitmapSource2->GetSize(&width2, &height2));

            if ((width1 != width2) || (height1 != height2))
            {
                return false;
            }

            auto pixelCount = width1 * height1;
            auto bufferSize = pixelCount * 4;
            auto stride = width1 * 4;

            auto buffer1 = wil::make_unique_failfast<uint8_t[]>(bufferSize);
            auto buffer2 = wil::make_unique_failfast<uint8_t[]>(bufferSize);

            LogThrow_IfFailed(wicBitmapSource1->CopyPixels(nullptr, stride, bufferSize, buffer1.get()));
            LogThrow_IfFailed(wicBitmapSource2->CopyPixels(nullptr, stride, bufferSize, buffer2.get()));

            auto pixel1 = reinterpret_cast<uint32_t*>(buffer1.get());
            auto pixel2 = reinterpret_cast<uint32_t*>(buffer2.get());
            uint32_t pixelDifferenceCount = 0;
            double squareDifferenceAccumulator = 0.0;

            m_diffBuffer.SetSize(width1, height1);
            m_errorBuffer.SetSize(width1, height1);

            for (auto y = 0u; y < height1; y++)
            {
                for (auto x = 0u; x < width1; x++)
                {
                    if (*pixel1 != *pixel2)
                    {
                        if (pixelDifferenceCount == 0)
                        {
                            LOG_OUTPUT(L"First pixel mismatch at (%lu, %lu): Expected = %08lx, Actual = 0x%08lx", x, y, *pixel1, *pixel2);
                        }
                        pixelDifferenceCount++;

                        uint8_t* pixel1Components = reinterpret_cast<uint8_t*>(pixel1);
                        uint8_t* pixel2Components = reinterpret_cast<uint8_t*>(pixel2);
                        double bDiff = abs(static_cast<double>(pixel1Components[0]) - static_cast<double>(pixel2Components[0]));
                        double gDiff = abs(static_cast<double>(pixel1Components[1]) - static_cast<double>(pixel2Components[1]));
                        double rDiff = abs(static_cast<double>(pixel1Components[2]) - static_cast<double>(pixel2Components[2]));
                        double aDiff = abs(static_cast<double>(pixel1Components[3]) - static_cast<double>(pixel2Components[3]));
                        double diff = bDiff + gDiff + rDiff + aDiff;
                        squareDifferenceAccumulator += diff * diff;

                        m_diffBuffer.SetPixel(x, y, static_cast<uint8_t>(aDiff), static_cast<uint8_t>(rDiff), static_cast<uint8_t>(gDiff), static_cast<uint8_t>(bDiff));
                        m_errorBuffer.SetPixel(x, y, 0xFFFF0000);
                    }

                    pixel1++;
                    pixel2++;
                }
            }

            if (pixelDifferenceCount == 0)
            {
                LOG_OUTPUT(L"Images are an exact match.");
            }
            else
            {
                double rmsError = sqrt(squareDifferenceAccumulator / pixelCount);

                LOG_WARNING(L"Images do not match:");
                LOG_OUTPUT(L"    Width = %lu", width1);
                LOG_OUTPUT(L"    Height = %lu", height1);
                LOG_OUTPUT(L"    # Diff Pixels = %lu", pixelDifferenceCount);
                LOG_OUTPUT(L"    RMS Error = %lf", rmsError);
                LOG_OUTPUT(L"    Image1 = %s", s1.GetRawBuffer(nullptr));
                LOG_OUTPUT(L"    Image2 = %s", s2.GetRawBuffer(nullptr));
            }

            return (pixelDifferenceCount == 0);
        }

        void ImageComparer::OutputAdditionalResults(
            bool)
        {
            if (m_diffBuffer.IsInitialized())
            {
                m_diffBuffer.SaveToFile(m_diffFilePath.GetBuffer());
            }

            if (m_errorBuffer.IsInitialized())
            {
                m_errorBuffer.SaveToFile(m_errorFilePath.GetBuffer());
            }
        }

        HRESULT ReadTestResource(_In_ const wchar_t* resourceName, _Out_ const uint8_t** pData, _Out_ DWORD* pDataLength);

        wrl::ComPtr<IWICBitmapSource> ImageComparer::LoadWicBitmapSource(const wrl::Wrappers::HStringReference& file)
        {
            if (m_wicImagingFactory == nullptr)
            {
                // Setup WIC and write the data to a file
                LogThrow_IfFailed(CoCreateInstance(
                    CLSID_WICImagingFactory,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&m_wicImagingFactory)
                    ));
            }

            wrl::ComPtr<IWICBitmapDecoder> wicBitmapDecoder;

            std::wstring fileName(file.GetRawBuffer(nullptr));
            const static std::wstring mastersMarkerStr(L"\\test\\resources\\masters\\");
            const auto found = fileName.find(mastersMarkerStr);
            const bool isMaster = found != std::wstring::npos;
            if (isMaster)
            {
                std::wstring name = fileName.substr(found + mastersMarkerStr.size());
                const uint8_t* data = nullptr;
                DWORD dataLength = 0;
                LogThrow_IfFailed(ReadTestResource(name.c_str(), &data, &dataLength));

                wrl::ComPtr<IWICStream> stream;
                LogThrow_IfFailed(m_wicImagingFactory->CreateStream(&stream));
                LogThrow_IfFailed(stream->InitializeFromMemory(const_cast<WICInProcPointer>(data), dataLength));
                LogThrow_IfFailed(m_wicImagingFactory->CreateDecoderFromStream(
                    stream.Get(),
                    nullptr,
                    WICDecodeMetadataCacheOnDemand,
                    &wicBitmapDecoder));
            }
            else
            {
                // Create a decoder for the given image file
                LogThrow_IfFailed(m_wicImagingFactory->CreateDecoderFromFilename(
                    fileName.c_str(),
                    nullptr,
                    GENERIC_READ,
                    WICDecodeMetadataCacheOnDemand,
                    &wicBitmapDecoder));
            }

            // Retrieve the requested frame of the image from the decoder
            wrl::ComPtr<IWICBitmapFrameDecode> wicBitmapFrameDecode;
            LogThrow_IfFailed(wicBitmapDecoder->GetFrame(0, &wicBitmapFrameDecode));

            UINT width = 0;
            UINT height = 0;
            LogThrow_IfFailed(wicBitmapFrameDecode->GetSize(&width, &height));

            wrl::ComPtr<IWICFormatConverter> wicFormatConverter;
            LogThrow_IfFailed(m_wicImagingFactory->CreateFormatConverter(&wicFormatConverter));
            LogThrow_IfFailed(wicFormatConverter->Initialize(
                wicBitmapFrameDecode.Get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                NULL,
                0.f,
                WICBitmapPaletteTypeCustom
                ));

            return wicFormatConverter;
        }

        VisualTreeXMLComparer::VisualTreeXMLComparer()
            : m_rulesAreInline(false)
        { }

        void VisualTreeXMLComparer::SetRulesFile(
            _In_ const wrl::Wrappers::HStringReference& rulesFilename)
        {
            m_rulesAreInline = false;
            m_rules = rulesFilename.GetRawBuffer(nullptr);
        }

        void VisualTreeXMLComparer::SetRulesInline(
            _In_ const wrl::Wrappers::HStringReference& rulesXml)
        {
            m_rulesAreInline = true;
            m_rules = rulesXml.GetRawBuffer(nullptr);
        }

        bool VisualTreeXMLComparer::CompareFiles(
            _In_ const wrl::Wrappers::HStringReference& s1,
            _In_ const wrl::Wrappers::HStringReference& s2)
        {
            // Child process output redirection loosely based on: https://support.microsoft.com/en-us/kb/190351

            SECURITY_ATTRIBUTES securityAttributes = { 0 };
            securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
            securityAttributes.bInheritHandle = TRUE;
            securityAttributes.lpSecurityDescriptor = nullptr;

            wil::unique_handle outReadHandle;
            wil::unique_handle outWriteHandle;

            CreateOutputStreamPipe(
                &outReadHandle,
                &outWriteHandle,
                &securityAttributes);

            STARTUPINFO startupInfo = { 0 };
            startupInfo.cb = sizeof(STARTUPINFO);
            startupInfo.dwFlags |= STARTF_USESTDHANDLES;
            startupInfo.hStdOutput = outWriteHandle.get();
            startupInfo.hStdError = outWriteHandle.get();

            WEX::Common::String cmdLine = CreateCommandLine(s1, s2);

            PROCESS_INFORMATION processInformation = { 0 };

            Throw::LastErrorIf(
                ::CreateProcess(
                    nullptr,               // lpApplicationName
                    cmdLine.GetBuffer(),   // lpCommandLine
                    &securityAttributes,   // lpProcessAttributes
                    &securityAttributes,   // lpThreadAttributes
                    TRUE,                  // bInheritHandles
                    CREATE_NO_WINDOW,      // dwCreationFlags
                    nullptr,               // lpEnvironment
                    nullptr,               // lpCurrentDirectory
                    &startupInfo,
                    &processInformation) == 0,
                L"CreateProcess failed");

            wil::unique_handle childProcess(processInformation.hProcess);
            ::CloseHandle(processInformation.hThread);

            // Close write end, so ReadFile will not hang.
            outWriteHandle.reset();

            RedirectStreamOutputToLog(outReadHandle.get());

            Throw::LastErrorIf(
                ::WaitForSingleObject(
                    childProcess.get(),
                    INFINITE) == WAIT_FAILED,
                L"WaitForSingleObject failed");

            unsigned long exitCode = 0;

            Throw::LastErrorIf(
                !::GetExitCodeProcess(
                    childProcess.get(),
                    &exitCode),
                L"GetExitCodeProcess failed");

            // If tool returns code indicating failed validation (e.g. not success nor differences in files) throw exception.
            // See enum ExitCode in dxaml/test/tools/XmlValidation/XmlValidation/Program.cs for possible values.

            Throw::IfFalse(
                exitCode == 0 || exitCode == 27182,
                E_FAIL);

            return exitCode == 0;
        }

        void VisualTreeXMLComparer::OutputAdditionalResults(bool) { }

        void VisualTreeXMLComparer::RedirectStreamOutputToLog(
            _In_ HANDLE handle)
        {
            CHAR buffer[257];
            DWORD bytesRead;
            WEX::Common::String lastLine;

            for (;;)
            {
                if (!::ReadFile(
                        handle,
                        buffer,
                        sizeof(buffer) - 1,
                        &bytesRead,
                        nullptr) || bytesRead == 0)
                {
                    DWORD lastError = ::GetLastError();

                    if (lastError == ERROR_BROKEN_PIPE)
                    {
                        // pipe done - normal exit path.
                        if (!lastLine.IsEmpty())
                        {
                            LOG_OUTPUT(lastLine);
                        }
                        break;
                    }
                    else
                    {
                        Throw::Exception(
                            HRESULT_FROM_WIN32(lastError),
                            L"Error occurred reading from output stream");
                    }
                }

                // Break output into lines since logging calls are line-based

                CHAR* bufferLineStart = buffer;
                CHAR* bufferLineEnd = buffer;
                CHAR* bufferEnd = buffer + bytesRead;

                for (;;)
                {
                    if (bufferLineEnd == bufferEnd)
                    {
                        *bufferLineEnd = '\0';
                        lastLine.Append(bufferLineStart);
                        break;
                    }
                    else if (*bufferLineEnd == '\n')
                    {
                        *bufferLineEnd = '\0';
                        LOG_OUTPUT(lastLine + WEX::Common::String(bufferLineStart));
                        lastLine.Empty();
                        bufferLineStart = ++bufferLineEnd;
                    }
                    else
                    {
                        ++bufferLineEnd;
                    }
                }
            }
        }

        void VisualTreeXMLComparer::CreateOutputStreamPipe(
            _Out_ PHANDLE readHandle,
            _Out_ PHANDLE writeHandle,
            _In_ SECURITY_ATTRIBUTES* securityAttributes)
        {
            wil::unique_handle outReadHandle;
            wil::unique_handle outReadHandleTemp;
            wil::unique_handle outWriteHandle;

            Throw::LastErrorIf(
                ::CreatePipe(
                    &outReadHandleTemp,
                    &outWriteHandle,
                    securityAttributes,
                    0) == 0,
                L"CreatePipe failed");

            Throw::LastErrorIf(
                ::DuplicateHandle(
                    ::GetCurrentProcess(),
                    outReadHandleTemp.get(),
                    ::GetCurrentProcess(),
                    &outReadHandle,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS) == 0,
                L"DuplicateHandle failed");

            *readHandle = outReadHandle.release();
            *writeHandle = outWriteHandle.release();
        }

        WEX::Common::String VisualTreeXMLComparer::CreateCommandLine(
            _In_ const wrl::Wrappers::HStringReference& s1,
            _In_ const wrl::Wrappers::HStringReference& s2)
        {
            WEX::Common::String cmdLine;

            // Execute Test.NET host

            WEX::Common::String validationToolPath = GetTestDeploymentDir();

            WEX::Common::String validationTool = validationToolPath;
            validationTool.Append(L"XmlValidation.exe");

            cmdLine.Format(
                L"%s %s %s",
                validationTool.GetBuffer(),
                s1.GetRawBuffer(nullptr),
                s2.GetRawBuffer(nullptr));

            if (!m_rules.IsEmpty())
            {
                if (m_rulesAreInline)
                {
                    cmdLine += WEX::Common::String().Format(L" -i \"%s\"", m_rules.GetBuffer());
                }
                else
                {
                    cmdLine += WEX::Common::String().Format(L" -f %s", m_rules.GetBuffer());
                }
            }

            return cmdLine;
        }
    }
};
