// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WexTestClass.h"
#include "Microsoft.UI.Xaml.coretypes.h"
#include <XbfMetadataProvider.h>
#include "XbfGenTests.h"
#include "SampleXbfMetadataProvider.h"
#include "CMemStream.h"
#include <iostream>
#include "XamlLogging.h"
#include "WaitForDebugger.h"
#include <limits>

struct TargetOSVersion
{
    TargetOSVersion(unsigned short major, unsigned short minor, unsigned short build, unsigned short revision)
    {
        this->major = major;
        this->minor = minor;
        this->build = build;
        this->revision = revision;
    }

    union
    {
        unsigned long long version;
        struct
        {
            unsigned short major;
            unsigned short minor;
            unsigned short build;
            unsigned short revision;
        };
    };
};

WEX::Common::String GetTestDeploymentDir()
{
    WEX::Common::String deploymentDir;
    LogThrow_IfFailed(
        WEX::TestExecution::RuntimeParameters::TryGetValue(WEX::TestExecution::RuntimeParameterConstants::c_szTestDeploymentDir, deploymentDir));
    if(deploymentDir.Right(1) != "\\")
    {
        deploymentDir.Append(L"\\");
    }
    return deploymentDir;
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace XbfGenerator {

    enum XbfGenerationFlags
    {
        Default = 0,
        DisableLineInfo,
    };

    const unsigned int c_checksumSize = 64;

    typedef HRESULT(*PFNWrite)(IStream**, UINT32, byte**, UINT32, IXbfMetadataProvider*, TargetOSVersion&, INT32, IStream**, int*, int*, int*, int*);
    typedef HRESULT(*PFNDump)(IStream*, IStream*, UINT32, int*);

    class DllLibrary {
    public:
        explicit DllLibrary(WEX::Common::String& name)
            : handle(new HMODULE(LoadLibrary(name)),
            [](HMODULE* instance) {
            if (*instance)
            {
                LOG_OUTPUT(L"Unloading Dll");
            }
            FreeLibrary(*instance);
        }) {}

        HMODULE Get() { return *(handle.get()); }

    private:
        std::unique_ptr<HMODULE, std::function<void(HMODULE*)>> handle;
    };

    class Checksum {
    public:
        explicit Checksum(const std::vector<std::array<byte, c_checksumSize>>& checksum)
            :m_numOfChecksums(checksum.size())
        {
            m_checksum = new byte*[m_numOfChecksums];
            for (size_t i = 0; i < m_numOfChecksums; i++)
            {
                m_checksum[i] = new byte[c_checksumSize];
                std::copy_n(checksum[i].begin(), c_checksumSize, m_checksum[i]);
            }
        }

        ~Checksum()
        {
            for (size_t i = 0; i < m_numOfChecksums; i++){
                delete[] m_checksum[i];
            }
            delete[] m_checksum;
        }

        byte** getBuffer()
        {
            return m_checksum;
        }

    private:
        size_t m_numOfChecksums;
        byte** m_checksum;
    };

    WEX::Common::String GetResourcePath(WEX::Common::String inXamlFile)
    {
        WEX::Common::String deploymentDirPath = GetTestDeploymentDir();
        deploymentDirPath += inXamlFile;

        return deploymentDirPath;
    }

    HRESULT CreateStreams(const std::vector<WEX::Common::String>& files, IStream** xamlStream, IStream** xbfStream)
    {
        if (!xamlStream) return E_INVALIDARG;

        for (size_t i = 0; i < files.size(); ++i)
        {
            HANDLE hFile = CreateFile(
                files[i],
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_READONLY,
                NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                LOG_OUTPUT(L"Couldn't Open File");
                return E_FAIL;
            }
            LARGE_INTEGER fileSize = { 0, 0 };
            if (!GetFileSizeEx(hFile, &fileSize))
            {
                LOG_OUTPUT(L"Getting File Size Failed");
                CloseHandle(hFile);
                return E_FAIL;
            }

            std::vector<unsigned char> buffer(fileSize.LowPart + 2);    // Add two for end of file characters

            DWORD numBytesRead = 0;
            if (!ReadFile(hFile, buffer.data(), fileSize.LowPart, &numBytesRead, NULL))
            {
                LOG_OUTPUT(L"File Read failed");
                CloseHandle(hFile);
                return E_FAIL;
            }

            CloseHandle(hFile); //Close the file because we are done with it
            if (fileSize.LowPart != numBytesRead) return E_FAIL;
            ULONG bytesWritten;
            xamlStream[i] = new CMemStream();
            xbfStream[i] = new CMemStream();
            if (!xamlStream[i] || !xbfStream[i]) return E_OUTOFMEMORY;
            if (FAILED(xamlStream[i]->Write(buffer.data(), static_cast<unsigned long>(fileSize.LowPart), &bytesWritten))) return E_FAIL;
        }
        return S_OK;
    }

    void LoadGenerator(
        WEX::Common::String strXbfGeneratorDll,
        std::shared_ptr<DllLibrary>& genXbfDll,
        PFNWrite& callWriteMethodWithChecksum,
        PFNDump& dumpMethod)
    {
        LOG_OUTPUT(L"Calling " + strXbfGeneratorDll);
        std::shared_ptr<DllLibrary> localGenXbfDll;
        localGenXbfDll = std::make_shared<DllLibrary>(strXbfGeneratorDll);

        if (localGenXbfDll->Get() == nullptr)
        {
            // If dll is null at this point, then the test is being run locally, which changes the relative path for GenXbf
            localGenXbfDll = std::make_shared<DllLibrary>(L"..\\xcp\\GenericXaml\\" + strXbfGeneratorDll);
        }
        VERIFY_IS_TRUE(localGenXbfDll->Get() != nullptr);

        genXbfDll = localGenXbfDll;
        callWriteMethodWithChecksum = reinterpret_cast<PFNWrite>(GetProcAddress(localGenXbfDll->Get(), "Write"));
        dumpMethod = reinterpret_cast<PFNDump>(GetProcAddress(localGenXbfDll->Get(), "Dump"));
    }

    void GenerateXbfUsingLoadedDll(
        PFNWrite& callWriteMethod,
        const std::vector<WEX::Common::String>& inXamlFullPath,
        TargetOSVersion inVersion,
        const std::vector<std::array<byte,c_checksumSize>>& inChecksum,
        wrl::ComPtr<IXbfMetadataProvider>& spProvider,
        XbfGenerationFlags generatorFlags,
        std::vector<std::vector<byte>>& outXbfBytes,
        int& outErrorCode,
        int& outErrorLine,
        int& outErrorCol)
    {
        size_t numFiles = inXamlFullPath.size();

        std::vector<wrl::ComPtr<IStream>> spXamlStreams(numFiles);
        std::vector<wrl::ComPtr<IStream>> spXbfStreams(numFiles);

        VERIFY_SUCCEEDED(CreateStreams(inXamlFullPath, spXamlStreams[0].GetAddressOf(), spXbfStreams[0].GetAddressOf()));

        int errorLine = 0, errorCol = 0, errorCode = 0, errorFileIndex = 0;

        if (callWriteMethod == nullptr)
        {
            VERIFY_IS_NOT_NULL(callWriteMethod); // this will fail, will give better output on console to see why test failed
        }

        Checksum checksum(inChecksum);

        HRESULT hr = callWriteMethod(spXamlStreams[0].GetAddressOf(), static_cast<UINT32>(numFiles), checksum.getBuffer(), c_checksumSize, spProvider.Get(), inVersion, generatorFlags, spXbfStreams[0].GetAddressOf(), &errorCode, &errorFileIndex, &errorLine, &errorCol);

        VERIFY_SUCCEEDED(hr, L"Write succeeded");

        if (errorCode == 0)
        {
            // Seek to end of stream to get size
            hr = S_OK;
            ULARGE_INTEGER bufferSize = { 0, 0 };
            LARGE_INTEGER offset = { 0, 0 };

            for (size_t i = 0; i < outXbfBytes.size(); ++i)
            {
                // Initiate early so if we return early, this will fail the test
                hr = spXbfStreams[i]->Seek(offset, STREAM_SEEK_END, &bufferSize);
                if (SUCCEEDED(hr))
                {
                    outXbfBytes[i].resize(bufferSize.LowPart);
                    // Reset the seek to the beginning of the buffer
                    ULARGE_INTEGER bufferPosition = { 0, 0 };
                    hr = spXbfStreams[i]->Seek(offset, STREAM_SEEK_SET, &bufferPosition);
                    if (SUCCEEDED(hr))
                    {   // Get the xaml buffer from the input stream
                        ULONG bytesRead = 0;
                        hr = spXbfStreams[i]->Read(outXbfBytes[i].data(), bufferSize.LowPart, &bytesRead);
                    }
                }
            }
        }

        outErrorCode = errorCode;
        outErrorLine = errorLine;
        outErrorCol = errorCol;
    }

    void GenerateDumpUsingLoadedDll(
        PFNDump& dumpMethod,
        const std::vector<WEX::Common::String>& inXbfFullPath,
        std::vector<std::vector<byte>>& outDumpBytes,
        int& outErrorCode)
    {
        size_t numFiles = inXbfFullPath.size();

        std::vector<wrl::ComPtr<IStream>> spXbfStreams(numFiles);
        std::vector<wrl::ComPtr<IStream>> spDumpStreams(numFiles);

        VERIFY_SUCCEEDED(CreateStreams(inXbfFullPath, spXbfStreams[0].GetAddressOf(), spDumpStreams[0].GetAddressOf()));

        if (dumpMethod == nullptr)
        {
            VERIFY_IS_NOT_NULL(dumpMethod);
        }

        int errorCode;
        HRESULT hr = dumpMethod(spXbfStreams[0].Get(), spDumpStreams[0].Get(), 0, &errorCode);
        VERIFY_SUCCEEDED(hr, L"DumpMethod succeeded.");

        if (errorCode == 0)
        {
            // Seek to end of stream to get size
            hr = S_OK;
            ULARGE_INTEGER bufferSize = { 0, 0 };
            LARGE_INTEGER offset = { 0, 0 };

            for (size_t i = 0; i < outDumpBytes.size(); ++i)
            {
                // Initiate early so if we return early, this will fail the test
                hr = spDumpStreams[0]->Seek(offset, STREAM_SEEK_END, &bufferSize);
                if (SUCCEEDED(hr))
                {
                    outDumpBytes[i].resize(bufferSize.LowPart);
                    // Reset the seek to the beginning of the buffer
                    ULARGE_INTEGER bufferPosition = { 0, 0 };
                    hr = spDumpStreams[0]->Seek(offset, STREAM_SEEK_SET, &bufferPosition);
                    if (SUCCEEDED(hr))
                    {   // Get the xaml buffer from the input stream
                        ULONG bytesRead = 0;
                        hr = spDumpStreams[0]->Read(outDumpBytes[i].data(), bufferSize.LowPart, &bytesRead);
                    }
                }
            }
        }

        outErrorCode = errorCode;
    }

    void GenerateXbfUsingCheckedInDll(
        const std::vector<WEX::Common::String>& inXamlFullPath,
        TargetOSVersion inVersion,
        const std::vector<std::array<byte, c_checksumSize>>& inChecksum,
        wrl::ComPtr<IXbfMetadataProvider>& spProvider,
        XbfGenerationFlags generatorFlags,
        std::vector<std::vector<byte>>& outXbfBytes,
        int& outErrorCode,
        int& outErrorLine,
        int& outErrorCol)
    {
        std::shared_ptr<DllLibrary> genXbfDll;
        PFNWrite callWriteMethod = nullptr;
        PFNDump dumpMethod = nullptr;

        LoadGenerator(L"GenXbfValidation\\GenXbf.dll", genXbfDll, callWriteMethod, dumpMethod);
        GenerateXbfUsingLoadedDll(callWriteMethod, inXamlFullPath, inVersion, inChecksum, spProvider, generatorFlags, outXbfBytes, outErrorCode, outErrorLine, outErrorCol);
    }

    void GenerateXbf(
        const std::vector<WEX::Common::String>& inXamlFullPath,
        TargetOSVersion inVersion,
        const std::vector<std::array<byte,c_checksumSize>>& inChecksum,
        wrl::ComPtr<IXbfMetadataProvider>& spProvider,
        XbfGenerationFlags generatorFlags,
        std::vector<std::vector<byte>>& outXbfBytes,
        int& outErrorCode,
        int& outErrorLine,
        int& outErrorCol)
    {
        std::shared_ptr<DllLibrary> genXbfDll;
        PFNWrite callWriteMethod = nullptr;
        PFNDump dumpMethod = nullptr;

        LoadGenerator(L"GenXbfValidation\\GenXbf.dll", genXbfDll, callWriteMethod, dumpMethod);
        GenerateXbfUsingLoadedDll(callWriteMethod, inXamlFullPath, inVersion, inChecksum, spProvider, generatorFlags, outXbfBytes, outErrorCode, outErrorLine, outErrorCol);
    }

    void GenerateDump(
        const std::vector<WEX::Common::String>& inXbfFullPath,
        std::vector<std::vector<byte>>& outXbfBytes,
        int& outErrorCode)
    {
        std::shared_ptr<DllLibrary> genXbfDll;
        PFNWrite callWriteMethod = nullptr;
        PFNDump dumpMethod = nullptr;

        LoadGenerator(L"GenXbfValidation\\GenXbf.dll", genXbfDll, callWriteMethod, dumpMethod);
        GenerateDumpUsingLoadedDll(dumpMethod, inXbfFullPath, outXbfBytes, outErrorCode);
    }

    std::vector<byte> GetChecksumFromBuffer(std::vector<byte>& inXbfBuffer)
    {
        const int checksumOffset = 68;
        std::vector<byte> checksumBuffer;

        std::copy_n(inXbfBuffer.begin() + checksumOffset, c_checksumSize, std::back_inserter(checksumBuffer));
        return checksumBuffer;
    }

    HRESULT LoadXbfFromDisk(const WEX::Common::String strFilePath, std::vector<byte>& outXbfBytes)
    {
        HANDLE hFile = CreateFile(
            strFilePath,
            GENERIC_READ,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_READONLY,
            nullptr);

        if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE)
        {
            LOG_OUTPUT(L"Couldn't Open File");
            return E_FAIL;
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile, &fileSize))
        {
            LOG_OUTPUT(L"Getting File Size Failed");
            CloseHandle(hFile);
            return E_FAIL;
        }

        std::vector<unsigned char> buffer(fileSize.LowPart);

        DWORD numBytesRead = 0;
        if (!ReadFile(hFile, buffer.data(), fileSize.LowPart, &numBytesRead, nullptr))
        {
            LOG_OUTPUT(L"File Read failed");
            CloseHandle(hFile);
            return E_FAIL;
        }

        CloseHandle(hFile); //Close the file because we are done with it
        if (fileSize.LowPart != numBytesRead) return E_FAIL;
        outXbfBytes = buffer;

        return S_OK;
    }

    HRESULT WriteXbfToDisk(const WEX::Common::String strFilePath, std::vector<byte>& xbfBytes)
    {
        HANDLE hFile = CreateFile(
            strFilePath,
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE)
        {
            LOG_OUTPUT(L"Couldn't Open File");
            return E_FAIL;
        }

        DWORD numBytesWritten = 0;

        if (!WriteFile(hFile, xbfBytes.data(), static_cast<DWORD>(xbfBytes.size()), &numBytesWritten, nullptr))
        {
            LOG_OUTPUT(L"File Write failed");
            CloseHandle(hFile);
            return E_FAIL;
        }

        CloseHandle(hFile); //Close the file because we are done with it
        if (xbfBytes.size() != numBytesWritten) return E_FAIL;

        return S_OK;
    }

    HRESULT CompareGeneratedXbfToMaster(const WEX::Common::String filename)
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(1);
        std::vector<std::array<byte, c_checksumSize>> checksumBytes(1);
        for (auto& checksum : checksumBytes)
        {
            std::fill(checksum.begin(), checksum.end(), static_cast<byte>(0));
        }
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        IFC_RETURN(wrl::MakeAndInitialize<SampleXbfMetadataProvider>(&spMetadataProvider));
        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;
        std::vector<WEX::Common::String> files(1, GetResourcePath(L"resources\\native\\tools\\" + filename + L".xaml"));
        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            return E_FAIL;
        }

        VERIFY_IS_TRUE(outXbfBytes[0].size() > 0);

        std::vector<byte> masterXbfFile(0);
        LOG_IF_FAILED(LoadXbfFromDisk(GetResourcePath(L"resources\\native\\tools\\" + filename + L".xbf"), masterXbfFile));

        HRESULT hr = E_FAIL;
        if (masterXbfFile.size() == outXbfBytes[0].size())
        {
            LOG_OUTPUT(L"Generated xbf file size matches master file size.");

            const int checksumStart = 68;
            const int checksumEnd = checksumStart + c_checksumSize;
            bool xbfFilesAreEqual = std::equal(masterXbfFile.begin(), masterXbfFile.begin() + checksumStart, outXbfBytes[0].begin());
            xbfFilesAreEqual = xbfFilesAreEqual && std::equal(masterXbfFile.begin() + checksumEnd, masterXbfFile.end(), outXbfBytes[0].begin() + checksumEnd);

            if (xbfFilesAreEqual)
            {
                LOG_OUTPUT(L"Generated xbf file matches master file.");
                hr = S_OK;
            }
        }
        else
        {
            WEX::Logging::Log::Warning(WEX::Common::String().Format(L"Generated xbf file size: %d", outXbfBytes[0].size()));
            WEX::Logging::Log::Warning(WEX::Common::String().Format(L"Master xbf file size: %d", masterXbfFile.size()));
        }

        if (hr != S_OK)
        {
            auto generatedXbf = GetResourcePath(L"resources\\native\\tools\\Generated_" + filename + ".xbf");
            LOG_IF_FAILED(WriteXbfToDisk(generatedXbf, outXbfBytes[0]));
            WEX::Logging::Log::Warning(L"Generated XBF written to: " + generatedXbf);
        }

        return hr;
    }

    HRESULT CompareGeneratedDumpToMaster(const WEX::Common::String filename)
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(1);
        int outErrorCode = 0;
        std::vector<WEX::Common::String> files(1, GetResourcePath(L"resources\\native\\tools\\" + filename + L".xbf"));

        GenerateDump(
            files,
            outXbfBytes,
            outErrorCode);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            return E_FAIL;
        }

        VERIFY_IS_TRUE(outXbfBytes[0].size() > 0);

        HRESULT hr = E_FAIL;
        std::vector<byte> masterDumpFile(0);
        LOG_IF_FAILED(LoadXbfFromDisk(GetResourcePath(L"resources\\native\\tools\\" + filename + L".xbf.txt"), masterDumpFile));

        if (masterDumpFile.size() == outXbfBytes[0].size())
        {
            LOG_OUTPUT(L"Generated dump file size matches master file size.");

            bool xbfFilesAreEqual = std::equal(masterDumpFile.begin(), masterDumpFile.end(), outXbfBytes[0].begin());

            if (xbfFilesAreEqual)
            {
                LOG_OUTPUT(L"Generated dump file matches master file.");
                hr = S_OK;
            }
        }
        else
        {
            WEX::Logging::Log::Warning(WEX::Common::String().Format(L"Generated dump file size: %d", outXbfBytes[0].size()));
            WEX::Logging::Log::Warning(WEX::Common::String().Format(L"Master dump file size: %d", masterDumpFile.size()));
        }

        if (hr != S_OK)
        {
            auto generatedDump = GetResourcePath(L"resources\\native\\tools\\Generated_" + filename + ".xbf.txt");
            LOG_IF_FAILED(WriteXbfToDisk(generatedDump, outXbfBytes[0]));
            WEX::Logging::Log::Warning(L"Generated XBF dump written to: " + generatedDump);
        }

        return hr;
    }

    HRESULT CompareGeneratedXbfToCheckedIn(std::vector<WEX::Common::String> files, XbfGenerationFlags flags = XbfGenerationFlags::Default)
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(files.size());
        std::vector<std::array<byte, c_checksumSize>> checksumBytes(files.size());
        for (auto& checksum : checksumBytes)
        {
            std::fill(checksum.begin(), checksum.end(), static_cast<byte>(0));
        }
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        IFC_RETURN(wrl::MakeAndInitialize<SampleXbfMetadataProvider>(&spMetadataProvider));
        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;

        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            flags,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            return E_FAIL;
        }

        VERIFY_IS_TRUE(outXbfBytes[0].size() > 0);


        std::vector<std::vector<byte>> checkedInXbfBytes(files.size());
        std::vector<std::array<byte, c_checksumSize>> checkedInChecksumBytes(files.size());
        for (auto& checksum : checksumBytes)
        {
            std::fill(checksum.begin(), checksum.end(), static_cast<byte>(0));
        }

        GenerateXbfUsingCheckedInDll(
            files,
            OSVersion,
            checkedInChecksumBytes,
            spMetadataProvider,
            flags,
            checkedInXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            return E_FAIL;
        }

        HRESULT hr = E_FAIL;
        if (checkedInXbfBytes[0].size() == outXbfBytes[0].size())
        {
            LOG_OUTPUT(L"Latest xbf file size matches checked-in file size.");

            const int checksumStart = 68;
            const int checksumEnd = checksumStart + c_checksumSize;
            bool xbfFilesAreEqual = std::equal(checkedInXbfBytes[0].begin(), checkedInXbfBytes[0].begin() + checksumStart, outXbfBytes[0].begin());
            xbfFilesAreEqual = xbfFilesAreEqual && std::equal(checkedInXbfBytes[0].begin() + checksumEnd, checkedInXbfBytes[0].end(), outXbfBytes[0].begin() + checksumEnd);

            if (xbfFilesAreEqual)
            {
                LOG_OUTPUT(L"Generated xbf file matches master file.");
                hr = S_OK;
            }
        }
        else
        {
            WEX::Logging::Log::Warning(WEX::Common::String().Format(L"Generated xbf file size: %d", outXbfBytes[0].size()));
            WEX::Logging::Log::Warning(WEX::Common::String().Format(L"Master xbf file size: %d", checkedInXbfBytes[0].size()));
        }

        if (hr != S_OK)
        {
            WEX::Logging::Log::Error(L"XBF's do not match. An update to genxbf.dll in the Tools depot is needed.");
        }

        return hr;
    }

    bool XbfGenerationTests::ClassSetup()
    {
        WaitForDebugger();
        return true;
    }

    void XbfGenerationTests::ValidateWrite()
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(1);
        std::vector<std::array<byte,c_checksumSize>> checksumBytes(1);
        for (size_t i = 0; i < c_checksumSize; i++)
        {
            checksumBytes[0][i] = static_cast<byte>(i);
        }
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        VERIFY_SUCCEEDED(wrl::MakeAndInitialize<SampleXbfMetadataProvider>(&spMetadataProvider));

        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;
        std::vector<WEX::Common::String> files(1,GetResourcePath(L"resources\\sample\\Sample.xaml"));
        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            VERIFY_FAIL();
        }

        VERIFY_IS_TRUE(outXbfBytes[0].size() > 0);
    }

    void XbfGenerationTests::ValidateErrorMessage()
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(1);
        std::vector<std::array<byte,c_checksumSize>> checksumBytes(1);
        for (size_t i = 0; i < c_checksumSize; i++)
        {
            checksumBytes[0][i] = static_cast<byte>(i);
        }
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        VERIFY_SUCCEEDED(wrl::MakeAndInitialize<SampleXbfMetadataProvider>(&spMetadataProvider));
        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;
        std::vector<WEX::Common::String> files(1,GetResourcePath(L"resources\\native\\tools\\XbfError.xaml"));
        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        VERIFY_IS_TRUE(outXbfBytes[0].size() == 0);
        VERIFY_IS_TRUE(outErrorCode == 2529);
        VERIFY_IS_TRUE(outErrorLine == 10);
        VERIFY_IS_TRUE(outErrorCol == 17);
    }

    void XbfGenerationTests::ValidateChecksumPlacement()
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(2);
        std::vector<std::array<byte,c_checksumSize>> checksumBytes(2);
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;

        for (size_t i = 0; i < c_checksumSize; i++)
        {
            checksumBytes[0][i] = static_cast<byte>(i);
            checksumBytes[1][i] = static_cast<byte>(i*2);
        }

        // We can use the same file but we will pass in a separate checksum, since the generator doesn't do anything
        // with the checksum this is ok since it is just stores it. This verifies that we are indeed correctly accepting
        // the array and writing the checksum to the write stream.
        std::vector<WEX::Common::String> files(2,GetResourcePath(L"resources\\sample\\Sample.xaml"));
        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            VERIFY_FAIL();
        }

        // Read the XBF buffer and verify checksum is there
        for (size_t i = 0; i < checksumBytes.size(); i++)
        {
            std::vector<byte> readChecksumBuffer = GetChecksumFromBuffer(outXbfBytes[i]);
            VERIFY_IS_TRUE(std::equal(readChecksumBuffer.begin(), readChecksumBuffer.end(), checksumBytes[i].begin()));
        }
    }

    void XbfGenerationTests::ValidateXbfGenerationOfLargeDouble()
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(1);
        std::vector<std::array<byte,c_checksumSize>> checksumBytes(1);
        for (size_t i = 0; i < c_checksumSize; i++)
        {
            checksumBytes[0][i] = static_cast<byte>(i);
        }
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;
        std::vector<WEX::Common::String> files(1,GetResourcePath(L"resources\\native\\tools\\XbfDictionaryTH882190.xaml"));
        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            VERIFY_FAIL();
        }

        VERIFY_IS_TRUE(outXbfBytes[0].size() > 0);
    }

    void XbfGenerationTests::ValidateXbfGenerationOfPhonePivot()
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(1);
        std::vector<std::array<byte,c_checksumSize>> checksumBytes(1);
        for (size_t i = 0; i < c_checksumSize; i++)
        {
            checksumBytes[0][i] = static_cast<byte>(i);
        }
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        VERIFY_SUCCEEDED(wrl::MakeAndInitialize<SampleXbfMetadataProvider>(&spMetadataProvider));
        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;

        std::vector<WEX::Common::String> files(1,GetResourcePath(L"resources\\native\\tools\\XbfPivotTest.xaml"));
        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            VERIFY_FAIL();
        }

        VERIFY_IS_TRUE(outXbfBytes[0].size() > 0);
    }

    void XbfGenerationTests::ValidateXbfGenerationMetadataReset()
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(1);
        std::vector<std::array<byte,c_checksumSize>> checksumBytes(1);
        for (size_t i = 0; i < c_checksumSize; i++)
        {
            checksumBytes[0][i] = static_cast<byte>(i);
        }
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;
        std::shared_ptr<DllLibrary> genXbfDll;
        PFNWrite callWriteMethodWithChecksum = nullptr;
        PFNDump dumpMethod = nullptr;

        LoadGenerator(L"GenXbfValidation\\GenXbf.dll", genXbfDll, callWriteMethodWithChecksum, dumpMethod);
        std::vector<WEX::Common::String> files(1,GetResourcePath(L"resources\\native\\tools\\XbfPivotTest.xaml"));
        // First attempt to generate the XBF without providing the metadata provider
        GenerateXbfUsingLoadedDll(
            callWriteMethodWithChecksum,
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            VERIFY_IS_TRUE(outErrorLine == 8);
            VERIFY_IS_TRUE(outErrorCol == 16);
        }
        else
        {
            VERIFY_FAIL();
        }

        // Provide the metadata provider which should be able to service the request for the types
        VERIFY_SUCCEEDED(wrl::MakeAndInitialize<SampleXbfMetadataProvider>(&spMetadataProvider));
        GenerateXbfUsingLoadedDll(
            callWriteMethodWithChecksum,
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            VERIFY_FAIL();
        }
        VERIFY_IS_TRUE(outXbfBytes[0].size() > 0);

        // Finally run the generator again without the provider which should fail generation as we should not be caching the metadata information.
        // First attempt to generate the XBF without providing the metadata provider
        spMetadataProvider.Reset();

        GenerateXbfUsingLoadedDll(
            callWriteMethodWithChecksum,
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            VERIFY_IS_TRUE(outErrorLine == 8);
            VERIFY_IS_TRUE(outErrorCol == 16);
        }
        else
        {
            VERIFY_FAIL();
        }
    }

    void XbfGenerationTests::ValidateXbfGenerationOfLooseXamlEvents()
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(1);
        std::vector<std::array<byte,c_checksumSize>> checksumBytes(1);
        for (size_t i = 0; i < c_checksumSize; i++)
        {
            checksumBytes[0][i] = static_cast<byte>(i);
        }
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        VERIFY_SUCCEEDED(wrl::MakeAndInitialize<SampleXbfMetadataProvider>(&spMetadataProvider));
        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;
        std::vector<WEX::Common::String> files (1,GetResourcePath(L"resources\\native\\tools\\XbfLooseXamlEvent.xaml"));
        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            VERIFY_FAIL();
        }

        VERIFY_IS_TRUE(outXbfBytes[0].size() > 0);
    }

    void XbfGenerationTests::ValidateEmptyXbfHash()
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytes(1);
        std::vector<std::array<byte,c_checksumSize>> checksumBytes(1);
        for (auto& checksum : checksumBytes)
        {
            std::fill(checksum.begin(), checksum.end(), static_cast<byte>(0));
        }
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        VERIFY_SUCCEEDED(wrl::MakeAndInitialize<SampleXbfMetadataProvider>(&spMetadataProvider));
        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;

        std::vector<WEX::Common::String> files(1,GetResourcePath(L"resources\\sample\\Sample.xaml"));
        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytes,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            VERIFY_FAIL();
        }

        // Read the XBF buffer and verify checksum is there
        std::vector<byte> readChecksumBuffer = GetChecksumFromBuffer(outXbfBytes[0]);
        std::array<byte,c_checksumSize> emptyHash = {0};
        VERIFY_IS_TRUE(std::equal(readChecksumBuffer.begin(), readChecksumBuffer.end(),emptyHash.begin()));
    }

    // To validate that the disable line info flag works we will do two sepearate generations of the XBF,
    // one with the line info enabled, and the other without. We will then compare the sizes and make sure the
    // one without line info is smaller.
    void XbfGenerationTests::ValidateDisableLineInfoFlag()
    {
        TargetOSVersion OSVersion = { 10, 0, std::numeric_limits<unsigned short>::max(), 0 };
        std::vector<std::vector<byte>> outXbfBytesWithLineInfo(1);
        std::vector<std::array<byte, c_checksumSize>> checksumBytes(1);
        for (auto& checksum : checksumBytes)
        {
            std::fill(checksum.begin(), checksum.end(), static_cast<byte>(0));
        }
        wrl::ComPtr<IXbfMetadataProvider> spMetadataProvider;
        VERIFY_SUCCEEDED(wrl::MakeAndInitialize<SampleXbfMetadataProvider>(&spMetadataProvider));
        int outErrorCode = 0;
        int outErrorLine = 0;
        int outErrorCol = 0;

        std::vector<WEX::Common::String> files(1, GetResourcePath(L"resources\\sample\\Sample.xaml"));
        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::Default,
            outXbfBytesWithLineInfo,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            VERIFY_FAIL();
        }

        // Generate again! This time with line info disabled
        std::vector<std::vector<byte>> outXbfBytesWithoutLineInfo(1);

        GenerateXbf(
            files,
            OSVersion,
            checksumBytes,
            spMetadataProvider,
            XbfGenerationFlags::DisableLineInfo,
            outXbfBytesWithoutLineInfo,
            outErrorCode,
            outErrorLine,
            outErrorCol);

        if (outErrorCode != 0)
        {
            LOG_OUTPUT(L"Error Code: %d", outErrorCode);
            LOG_OUTPUT(L"Error Line: %d", outErrorLine);
            LOG_OUTPUT(L"Error Col: %d", outErrorCol);
            VERIFY_FAIL();
        }

        auto sizeWithLineInfo = outXbfBytesWithLineInfo[0].size();
        auto sizeWithoutLineInfo = outXbfBytesWithoutLineInfo[0].size();

        VERIFY_IS_TRUE(sizeWithLineInfo > sizeWithoutLineInfo);
    }

    void XbfGenerationTests::ValidateXbfGenerationOfResourceDictionary()
    {
        VERIFY_SUCCEEDED(CompareGeneratedXbfToMaster(L"XbfResourceDictionaryTest.v2.1"));
    }

    void XbfGenerationTests::ValidateXbfGenerationOfVSM()
    {
        VERIFY_SUCCEEDED(CompareGeneratedXbfToMaster(L"XbfVsmTest.v2.1"));
    }

    void XbfGenerationTests::ValidateXbfGenerationOfStyles()
    {
        VERIFY_SUCCEEDED(CompareGeneratedXbfToMaster(L"XbfStyleTest.v2.1"));
    }

    void XbfGenerationTests::ValidateXbfDump()
    {
        VERIFY_SUCCEEDED(CompareGeneratedDumpToMaster(L"XbfResourceDictionaryTest.v2.1"));
        VERIFY_SUCCEEDED(CompareGeneratedDumpToMaster(L"CustomTypesPage"));
    }


    void XbfGenerationTests::SanityTest()
    {
        std::vector<WEX::Common::String> files;
        files.push_back(GetResourcePath(L"GenXbfValidation\\generic.xaml"));

        VERIFY_SUCCEEDED(CompareGeneratedXbfToCheckedIn(files, XbfGenerationFlags::DisableLineInfo), L"If this test fails, please update GenXbf.dll in vPack.");
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Tools::XbfGenerator
