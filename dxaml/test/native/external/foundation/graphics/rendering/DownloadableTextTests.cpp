// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DownloadableTextTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeaturesEnum.h>
#include <CustomSystemFontCollectionOverride.h>
#include <DWrite_3.h>
#include <stdint.h> // Standard C++ integer types.
#include <WUCRenderingScopeGuard.h>

#pragma warning(disable: 4100) // Unreferenced parameters are perfectly legal C++ and common for formal interfaces, not indicative a real error.
#undef min // Use the true STL min without re-evaluation side-effects.
#undef max // Use the true STL max without re-evaluation side-effects.

// To avoid overly verbose logging for certain cases, but still print errors when they do happen.
#ifndef VERIFY_SUCCEEDED_SILENT
#define VERIFY_SUCCEEDED_SILENT(exp) { auto hr = (exp); if (FAILED(hr)) VERIFY_SUCCEEDED(hr); }
#endif

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        class RefCountBaseImpl
        {
        public:
            unsigned long STDMETHODCALLTYPE AddRef()
            {
                return ::InterlockedIncrement(&m_refCount);
            }

            unsigned long STDMETHODCALLTYPE Release()
            {
                auto newRefCount = ::InterlockedDecrement(&m_refCount);
                if (newRefCount == 0)
                {
                    delete this;
                }
                return newRefCount;
            }

        protected:
            // Force most derived virtual destructor to be called, and
            // prevent stack allocation instead of new.
            virtual ~RefCountBaseImpl()
            {
            }

        private:
            long m_refCount = 0;
        };

        template<typename I>
        class RefCountBase : public RefCountBaseImpl, public I
        {
        public:
            virtual unsigned long STDMETHODCALLTYPE AddRef() override
            {
                return RefCountBaseImpl::AddRef();
            }

            virtual unsigned long STDMETHODCALLTYPE Release() override
            {
                return RefCountBaseImpl::Release();
            }
        };

        // A custom stream that treats a local file is if it was remote.
        class CustomRemoteFileStream : public RefCountBase<IDWriteRemoteFontFileStream>
        {
        public:
            CustomRemoteFileStream()
            {
            }

            ~CustomRemoteFileStream()
            {
                CloseHandle(fileHandle_);
            }

            HRESULT Initialize(
                _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
                uint32_t fontFileReferenceKeySize
                )
            {
                // The unique key is a file path for this custom loader (nul-terminated).
                wchar_t const* filePath = reinterpret_cast<wchar_t const*>(fontFileReferenceKey);
                fileName_ = filePath;

                fileHandle_ = CreateFile(
                    filePath,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, // securityAttributes
                    OPEN_EXISTING, // creationDisposition
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr // templateFile
                    );

                if (fileHandle_ == INVALID_HANDLE_VALUE)
                {
                    return HRESULT_FROM_WIN32(GetLastError());
                }

                // Use GetFileInformationByHandle to obtain both file size and timestamp in a single call.
                BY_HANDLE_FILE_INFORMATION fileInformation = {};
                if (!GetFileInformationByHandle(fileHandle_, &fileInformation))
                {
                    return HRESULT_FROM_WIN32(GetLastError());
                }

                if (fileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    return HRESULT_FROM_WIN32(ERROR_DIRECTORY_NOT_SUPPORTED);
                }

                ULARGE_INTEGER largeInteger;
                largeInteger.HighPart = fileInformation.nFileSizeHigh;
                largeInteger.LowPart = fileInformation.nFileSizeLow;
                fileSize_ = largeInteger.QuadPart;
                largeInteger.HighPart = fileInformation.ftLastWriteTime.dwHighDateTime;
                largeInteger.LowPart = fileInformation.ftLastWriteTime.dwLowDateTime;
                lastWriteTime_ = largeInteger.QuadPart;

                fileData_.resize(size_t(fileSize_));
                fileDataChunkMap_.resize(size_t(fileSize_ + ChunkSizeMask) / ChunkSize); // 1 bit per 64KB chunk

                fileLocality_ = DWRITE_LOCALITY_PARTIAL;

                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE QueryInterface(IID const& iid, _Out_ void** object) override
            {
                *object = nullptr;
                if (iid == __uuidof(IUnknown)
                ||  iid == __uuidof(IDWriteFontFileStream)
                ||  iid == __uuidof(IDWriteRemoteFontFileStream))
                {
                    *object = static_cast<IDWriteRemoteFontFileStream*>(this);
                    AddRef();
                    return S_OK;
                }
                return E_NOINTERFACE;
            }

            HRESULT STDMETHODCALLTYPE ReadFileFragment(
                _Outptr_result_bytebuffer_(fragmentSize) void const** fragmentStart,
                uint64_t fileOffset,
                uint64_t fragmentSize,
                _Out_ void** fragmentContext
                ) override
            {
                *fragmentStart = nullptr;
                *fragmentContext = nullptr;

                auto fileEnd = fileOffset + fragmentSize;
                if (fileEnd < fileOffset || fileEnd > fileSize_)
                {
                    return E_INVALIDARG;
                }

                // Ensure that the chunks are actually local before returning a pointer.
                size_t chunkMapIndex = size_t(fileOffset / ChunkSize);
                size_t chunkMapIndexEnd = size_t((fileOffset + fragmentSize + ChunkSizeMask) / ChunkSize);
                for (; chunkMapIndex != chunkMapIndexEnd; ++chunkMapIndex)
                {
                    if (!fileDataChunkMap_[chunkMapIndex])
                    {
                        return DWRITE_E_REMOTEFONT;
                    }
                }
                *fragmentStart = &fileData_[size_t(fileOffset)];
                return S_OK;
            }

            void STDMETHODCALLTYPE ReleaseFileFragment(
                void* fragmentContext
                ) override
            {
                // No work.
            }

            HRESULT STDMETHODCALLTYPE GetFileSize(
                _Out_ uint64_t* fileSize
                ) override
            {
                *fileSize = static_cast<uint64_t>(fileData_.size());
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE GetLastWriteTime(
                _Out_ uint64_t* lastWriteTime
                ) override
            {
                *lastWriteTime = lastWriteTime_;
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE GetLocalFileSize(
                _Out_ uint64_t* localFileSize
                ) override
            {
                // Accumulate all the present chunks for the total size.
                uint64_t totalFileSize = 0;
                for (auto chunkIsPresent : fileDataChunkMap_)
                {
                    if (chunkIsPresent)
                    {
                        totalFileSize += ChunkSize;
                    }
                }
                *localFileSize = std::min(totalFileSize, fileSize_);
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE GetFileFragmentLocality(
                UINT64 fileOffset,
                UINT64 fragmentSize,
                _Out_ BOOL* isLocal,
                _Out_range_(0, fragmentSize) UINT64* partialSize
                ) override
            {
                *isLocal = false;
                *partialSize = 0;

                // Check the chunk map for missing chunks in the range.
                const size_t chunkMapIndexStart = size_t(fileOffset / ChunkSize);
                const size_t chunkMapIndexEnd = size_t((fileOffset + fragmentSize + ChunkSizeMask) / ChunkSize);

                // An empty range is local by definition.
                if (chunkMapIndexStart == chunkMapIndexEnd)
                {
                    *isLocal = true;
                    return S_OK;
                }

                // Determine whether the first chunk is local.
                size_t chunkMapIndex = chunkMapIndexStart;
                bool isFirstChunkLocal = fileDataChunkMap_[chunkMapIndex++];

                // Advance past subsequent chunks with the same locality as the first.
                while (chunkMapIndex < chunkMapIndexEnd && fileDataChunkMap_[chunkMapIndex] == isFirstChunkLocal)
                {
                    ++chunkMapIndex;
                }

                // Determine the partial size.
                if (chunkMapIndex == chunkMapIndexEnd)
                {
                    // All the chunks had the same locality.
                    *partialSize = fragmentSize;
                }
                else
                {
                    // Compute the size of the chunks with the same locality.
                    *partialSize = (chunkMapIndex - chunkMapIndexStart) * ChunkSize;
                }

                *isLocal = isFirstChunkLocal;

                return S_OK;
            }

            DWRITE_LOCALITY STDMETHODCALLTYPE GetLocality() override
            {
                return fileLocality_;
            }

            DWRITE_FILE_FRAGMENT AlignFragment(DWRITE_FILE_FRAGMENT const& unalignedFragment)
            {
                // Align the potentially unaligned fragment to the nearest whole chunk.
                auto fragmentEnd = unalignedFragment.fileOffset + unalignedFragment.fragmentSize;
                auto alignedFragmentBegin = unalignedFragment.fileOffset & ~ChunkSizeMask;
                auto alignedfragmentEnd = std::min((fragmentEnd + ChunkSizeMask) & ~ChunkSizeMask, fileSize_);
                return DWRITE_FILE_FRAGMENT{ alignedFragmentBegin, alignedfragmentEnd - alignedFragmentBegin };
            }

            HRESULT STDMETHODCALLTYPE BeginDownload(
                _In_ UUID const* downloadOperationID,
                _In_reads_(fragmentCount) DWRITE_FILE_FRAGMENT const* fileFragments,
                UINT32 fragmentCount,
                _COM_Outptr_result_maybenull_ IDWriteAsyncResult** asyncResult
                ) override
            {
                *asyncResult = nullptr;

                if (fileHandle_ == INVALID_HANDLE_VALUE)
                {
                    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                }

                // Read every fragment requested.
                auto* fragmentsEnd = fileFragments + fragmentCount;
                for (auto* fragment = fileFragments; fragment != fragmentsEnd; ++fragment)
                {
                    // Read every chunk within the file fragment, if it has not already been
                    // 'downloaded' before.
                    DWRITE_FILE_FRAGMENT alignedFragment = AlignFragment(*fragment);
                    while (alignedFragment.fragmentSize > 0)
                    {
                        auto chunkIndex = size_t(alignedFragment.fileOffset / ChunkSize);
                        unsigned long bytesToRead = (alignedFragment.fragmentSize < ChunkSize) ? uint32_t(alignedFragment.fragmentSize) : ChunkSize;
                        if (!fileDataChunkMap_[chunkIndex])
                        {
                            unsigned long bytesRead = 0;
                            if (!ReadFile(fileHandle_, OUT &fileData_[size_t(alignedFragment.fileOffset)], bytesToRead, OUT &bytesRead, nullptr))
                            {
                                return HRESULT_FROM_WIN32(GetLastError());
                            }
                            fileDataChunkMap_[chunkIndex] = true;
                        }
                        // Next chunk within the current fragment.
                        alignedFragment.fileOffset += ChunkSize;
                        alignedFragment.fragmentSize -= bytesToRead;
                    }
                }

                // Check whether downloads changed locality.
                if (fileLocality_ != DWRITE_LOCALITY_LOCAL)
                {
                    bool areAllChunksPresent = true;
                    for (auto chunkIsPresent : fileDataChunkMap_)
                    {
                        if (!chunkIsPresent)
                        {
                            areAllChunksPresent = false;
                            break;
                        }
                    }
                    if (areAllChunksPresent)
                    {
                        // Promote it to local hereafter.
                        fileLocality_ = DWRITE_LOCALITY_LOCAL;
                    }
                }

                return S_OK;
            }

            static void SetEnabled(bool isEnabled) { isEnabled_ = isEnabled; }
            static bool GetEnabled() { return isEnabled_; }

        private:
            static uint32_t const ChunkSize = 65536;
            static uint32_t const ChunkSizeMask = 65536-1;

            DWRITE_LOCALITY fileLocality_ = DWRITE_LOCALITY_REMOTE;
            uint64_t lastWriteTime_ = 0;
            uint64_t fileSize_ = 0;
            HANDLE fileHandle_ = INVALID_HANDLE_VALUE;
            std::wstring fileName_; // Just for seeing in the debugger.
            std::vector<uint8_t> fileData_; // Dynamically loaded data read from the file, equal to file size.
            std::vector<bool> fileDataChunkMap_;

        protected:
            static bool isEnabled_;
        };
        bool CustomRemoteFileStream::isEnabled_ = true;


        class CustomRemoteFileLoader : public RefCountBase<IDWriteRemoteFontFileLoader>
        {
            struct RecentFileStream;

        public:
            CustomRemoteFileLoader()
            {
            }

            virtual HRESULT STDMETHODCALLTYPE QueryInterface(IID const& iid, _Out_ void** object) override
            {
                *object = nullptr;
                if (iid == __uuidof(IUnknown)
                ||  iid == __uuidof(IDWriteFontFileLoader)
                ||  iid == __uuidof(IDWriteRemoteFontFileLoader))
                {
                    *object = static_cast<IDWriteRemoteFontFileLoader*>(this);
                    AddRef();
                    return S_OK;
                }
                return E_NOINTERFACE;
            }

            // Create a stream from a key, which comprises the base file name.
            HRESULT STDMETHODCALLTYPE CreateStreamFromKey(
                _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
                uint32_t fontFileReferenceKeySize,
                _COM_Outptr_ IDWriteFontFileStream** fontFileStream
                ) override
            {
                *fontFileStream = nullptr;

                if (!CustomRemoteFileStream::GetEnabled())
                {
                    // If the file stream is disabled for test purposes, always return an error.
                    // This essentially tests the case of a down network.
                    return DWRITE_E_REMOTEFONT;
                }

                uint8_t const* p = static_cast<uint8_t const*>(fontFileReferenceKey);
                for (auto& recentStream : recentFileStreams_)
                {
                    if (recentStream.fontFileReferenceKey.size() == fontFileReferenceKeySize
                    &&  std::equal(recentStream.fontFileReferenceKey.begin(), recentStream.fontFileReferenceKey.end(), p))
                    {
                        auto* s = recentStream.fontFileStream.Get();
                        if (s != nullptr)
                        {
                            s->AddRef();
                            *fontFileStream = s;
                            return S_OK;
                        }
                    }
                }

                Microsoft::WRL::ComPtr<CustomRemoteFileStream> newFileStream = new CustomRemoteFileStream();
                auto hr = newFileStream->Initialize(fontFileReferenceKey, fontFileReferenceKeySize);

                recentFileStreams_.resize(recentFileStreams_.size() + 1);
                recentFileStreams_.back().fontFileReferenceKey.assign(p, p + fontFileReferenceKeySize);
                recentFileStreams_.back().fontFileStream = newFileStream;

                if (SUCCEEDED(hr))
                {
                    *fontFileStream = newFileStream.Detach();
                }

                return hr;
            }

            HRESULT STDMETHODCALLTYPE CreateRemoteStreamFromKey(
                _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
                uint32_t fontFileReferenceKeySize,
                _COM_Outptr_ IDWriteRemoteFontFileStream** fontFileStream
                ) override
            {
                // Just forward the call, since there is no distinction in behavior between the two
                // functions for this interface implementation. You can always create a 'remote'
                // file stream based on a local file.
                return CreateStreamFromKey(
                    fontFileReferenceKey,
                    fontFileReferenceKeySize,
                    reinterpret_cast<IDWriteFontFileStream**>(fontFileStream)
                    );
            }

            HRESULT STDMETHODCALLTYPE GetLocalityFromKey(
                _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
                uint32_t fontFileReferenceKeySize,
                _Out_ DWRITE_LOCALITY* locality
                ) override
            {
                // Just treat all files as partial for this simplistic loader.
                // No files are actually remote anyway, since they are all local files.
                // (do treat them as remote if the stream is disabled for testing though)
                *locality = CustomRemoteFileStream::GetEnabled() ? DWRITE_LOCALITY_PARTIAL : DWRITE_LOCALITY_REMOTE;
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE CreateFontFileReferenceFromUrl(
                IDWriteFactory* factory,
                _In_opt_z_ WCHAR const* baseUrl,
                _In_z_ WCHAR const* fontFileUrl,
                _COM_Outptr_ IDWriteFontFile** fontFile
                ) noexcept override
            {
                *fontFile = nullptr;
                return E_NOTIMPL;
            }

            // Keep track of recent streams. Otherwise we lose all the hydration state information
            // from previous stream creations, and we end up rehydrating font faces every time.
            // Note there is no cap on the number of streams. If this wasn't just test code,
            // you would want to limit it and probably use an LRU.
        private:
            struct RecentFileStream
            {
                std::vector<uint8_t> fontFileReferenceKey;
                Microsoft::WRL::ComPtr<CustomRemoteFileStream> fontFileStream;
            };

            std::vector<RecentFileStream> recentFileStreams_;
        };


        Microsoft::WRL::ComPtr<IDWriteFontFaceReference> MakeLocalFontFaceReference(
            IDWriteFactory3* factory,
            _In_z_ wchar_t const* fileName,
            uint32_t fontFaceIndex
            )
        {
            Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile;
            Microsoft::WRL::ComPtr<IDWriteFontFaceReference> fontFaceReference;

            VERIFY_SUCCEEDED_SILENT(factory->CreateFontFileReference(
                fileName,
                nullptr, // lastWriteTime
                &fontFile
                ));

            VERIFY_SUCCEEDED_SILENT(factory->CreateFontFaceReference(
                fontFile.Get(),
                fontFaceIndex,
                DWRITE_FONT_SIMULATIONS_NONE,
                &fontFaceReference
                ));

            return fontFaceReference;
        }

        Microsoft::WRL::ComPtr<IDWriteFontFaceReference> MakeCustomFontFaceReference(
            IDWriteFactory3* factory,
            IDWriteFontFileLoader* fontFileLoader,
            _In_z_ wchar_t const* fileName,
            uint32_t fontFaceIndex
            )
        {
            Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile;
            Microsoft::WRL::ComPtr<IDWriteFontFaceReference> fontFaceReference;

            VERIFY_SUCCEEDED_SILENT(factory->CreateCustomFontFileReference(
                fileName,
                static_cast<uint32_t>((wcslen(fileName) + 1) * sizeof(*fileName)),
                fontFileLoader,
                &fontFile
                ));

            VERIFY_SUCCEEDED_SILENT(factory->CreateFontFaceReference(
                fontFile.Get(),
                fontFaceIndex,
                DWRITE_FONT_SIMULATIONS_NONE,
                &fontFaceReference
                ));

            return fontFaceReference;
        }

        // Return a wstring for the given locale name.
        void GetLocalizedString(
            IDWriteLocalizedStrings* localizedStrings,
            _In_opt_z_ wchar_t const* localeName,
            _Out_ std::wstring& stringValue
            )
        {
            stringValue.clear();

            if (localizedStrings == nullptr || localizedStrings->GetCount() == 0)
            {
                return; // Just return empty string.
            }

            bool requireExactMatch = (localeName != nullptr) && localeName[0] != '\0';
            uint32_t listIndex = 0;
            BOOL stringExists = false;

            // Try the specific locale first, then en-us, then "", and finally the first index.
            if (localeName == nullptr)
            {
                localeName = L"en-us";
            }
            localizedStrings->FindLocaleName(localeName, OUT &listIndex, OUT &stringExists);
            if (stringExists == false)
            {
                if (requireExactMatch)
                {
                    return; // Empty string
                }
                // Try again with empty string (if we didn't just already try with empty string).
                if (localeName[0] != '\0')
                {
                    localizedStrings->FindLocaleName(L"", OUT &listIndex, OUT &stringExists);
                }
                if (stringExists == false)
                {
                    listIndex = 0; // Take whatever string we can get by using the first index.
                }
            }

            // Get the string length, resize the buffer accordingly, and get the string.
            uint32_t stringLength;
            VERIFY_SUCCEEDED_SILENT(localizedStrings->GetStringLength(
                listIndex,
                OUT &stringLength
                ));

            uint32_t stringBufferSize = stringLength + 1;
            stringValue.resize(stringBufferSize);

            VERIFY_SUCCEEDED_SILENT(localizedStrings->GetString(
                listIndex,
                OUT &stringValue[0],
                stringBufferSize
                ));

            stringValue.resize(stringLength);
        }

        std::wstring GetLocalizedString(_In_ IDWriteLocalizedStrings* localizedStrings)
        {
            std::wstring stringValue;
            GetLocalizedString(localizedStrings, nullptr, OUT stringValue);
            return stringValue;
        }

        bool DoesFontFamilyExist(
            IDWriteFontCollection* fontCollection,
            _In_z_ wchar_t const* fontFamilyName
            )
        {
            uint32_t familyListIndex;
            BOOL exists;
            VERIFY_SUCCEEDED(fontCollection->FindFamilyName(fontFamilyName, OUT &familyListIndex, OUT &exists));
            return !!exists;
        }

        Microsoft::WRL::ComPtr<IDWriteFontCollection1> CreateCustomDownloadableSystemFontCollection(
            IDWriteFactory3* factory,
            IDWriteFontFileLoader* customFontFileLoader,
            _In_z_ wchar_t const* baseFilePath, // including the trailing backslash
            _In_reads_(fontCount) wchar_t const** fontFileNames,
            uint32_t fontCount
            )
        {
            // Create a custom font collection of virtual downloadable fonts.
            // We actually just use the locally installed physical fonts, but treat
            // them as if they were downloadable fonts by abstracting them through
            // the custom remote loader.

            Microsoft::WRL::ComPtr<IDWriteFontCollection1> fontCollection;
            Microsoft::WRL::ComPtr<IDWriteFontSet> customFontSet;
            Microsoft::WRL::ComPtr<IDWriteFontSetBuilder> fontSetBuilder;

            VERIFY_SUCCEEDED(factory->CreateFontSetBuilder(OUT &fontSetBuilder));

            // Get the file path and add a backslash if missing.
            std::wstring filePath;

            // Create another font set that reinterprets the locally installed fonts as downloadable fonts.
            for (uint32_t fontIndex = 0; fontIndex < fontCount; ++fontIndex)
            {
                // Create a font from the given filename. To keep it a little simpler, we'll just pick
                // the first index (so no TTC's beyond the first) and leave out the weight/width/slope.

                auto* fileName = fontFileNames[fontIndex];
                if (wcschr(fileName, '\\') != nullptr)
                {
                    filePath = fileName; // Use the name directly because it has a path.
                }
                else
                {
                    filePath = baseFilePath; // Concatenate the file name to the base path
                    filePath += fileName;
                }
                uint32_t const fontFaceIndex = 0;
                auto fontFaceReference = MakeLocalFontFaceReference(factory, filePath.c_str(), fontFaceIndex);

                Microsoft::WRL::ComPtr<IDWriteFontFace3> fontFace;
                Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile;
                VERIFY_SUCCEEDED_SILENT(fontFaceReference->GetFontFile(OUT &fontFile));
                VERIFY_SUCCEEDED_SILENT(fontFaceReference->CreateFontFace(OUT &fontFace));

                // Get a few of the properties for the FontSet list item to copy over.
                // We only need as many properties as are necessary for font family lookup.
                // Properties like the postscript name are not pertinent in the new collection.

                Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> familyNames, faceNames;
                VERIFY_SUCCEEDED_SILENT(fontFace->GetFamilyNames(OUT &familyNames));
                VERIFY_SUCCEEDED_SILENT(fontFace->GetFaceNames(OUT &faceNames));
                std::wstring familyName = GetLocalizedString(familyNames.Get());
                std::wstring faceName = GetLocalizedString(faceNames.Get());

                DWRITE_FONT_PROPERTY properties[] = {
                    { DWRITE_FONT_PROPERTY_ID_FAMILY_NAME, familyName.c_str(), L"en-us" },
                    { DWRITE_FONT_PROPERTY_ID_FACE_NAME, faceName.c_str(), L"en-us" },
                };

                // Add the custom font reference and associated properties.
                Microsoft::WRL::ComPtr<IDWriteFontFaceReference> newFontFaceReference =
                    MakeCustomFontFaceReference(
                        factory,
                        customFontFileLoader,
                        filePath.c_str(),
                        fontFaceIndex
                        );
                VERIFY_SUCCEEDED_SILENT(fontSetBuilder->AddFontFaceReference(
                    newFontFaceReference.Get(),
                    &properties[0],
                    ARRAYSIZE(properties) // propertiesCount
                    ));
            }

            // Finalize the font set and create a font collection from it.
            VERIFY_SUCCEEDED(fontSetBuilder->CreateFontSet(OUT &customFontSet));
            VERIFY_SUCCEEDED(factory->CreateFontCollectionFromFontSet(customFontSet.Get(), OUT &fontCollection));

            return fontCollection;
        }

        //------------------------------------------------------------------------
        // Test case initialization/cleanup.
        //
        //------------------------------------------------------------------------

        Platform::String^ DownloadableTextTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
        }

        bool DownloadableTextTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool DownloadableTextTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool DownloadableTextTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Renders a few controls using downloadable fonts.
        //
        //------------------------------------------------------------------------
        void DownloadableTextTests::RenderDownloadableFonts()
        {
            {
                // Include a few rarer fonts and one CJK font (note Meiryo and Meiryo UI are both are in same TTC file)
                // Also include the Segoe UI in the fonts (XAML requires Segoe UI as the ultimate font fallback from system font collection), since test is going to override the system font collection.
                Platform::String^ resourcesPath = GetResourcesPath();
                static wchar_t const* fontNames[] = { L"Kootenay", L"Pericles", L"Segoe UI" };
                static wchar_t const* fontFileNames[] = { L"Kootenay.ttf", L"Pericles.ttf", L"segoeui.ttf" };

                Microsoft::WRL::ComPtr<IDWriteFactory3> pDWriteFactory;
                Microsoft::WRL::ComPtr<IDWriteFontDownloadQueue> pDWriteFontDownloadQueue;
                Microsoft::WRL::ComPtr<IDWriteFontCollection1> pSystemFontCollection;
                Microsoft::WRL::ComPtr<IDWriteFontCollection1> pCustomDownloadableFontCollection;

                // Initialize DirectWrite types, including custom font file loader and custom font collection.
                LOG_OUTPUT(L"Initialize DirectWrite factory.");
                VERIFY_SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), reinterpret_cast<IUnknown**>(pDWriteFactory.GetAddressOf())));
                VERIFY_SUCCEEDED(pDWriteFactory->GetFontDownloadQueue(&pDWriteFontDownloadQueue));
                VERIFY_SUCCEEDED(pDWriteFactory->GetSystemFontCollection(/*includeRemoteFonts*/false, &pSystemFontCollection));

                Microsoft::WRL::ComPtr<CustomRemoteFileLoader> customFontFileLoader = new CustomRemoteFileLoader();
                VERIFY_SUCCEEDED(pDWriteFactory->RegisterFontFileLoader(customFontFileLoader.Get()));

                // Set cleanup in case the test fails. Note we retrieved the DWrite objects first because they are needed in cleanup.
                TestCleanupWrapper fontFileLoaderCleanup([&]()
                {
                    // Unregister the font file loader so there are no dangling references to code
                    // that is no longer mapped when the TAEF DLL unloads before the last reference
                    // is held; and explicitly cancel any pending downloads so that DWrite does not
                    // call a DownloadCompleted that is not a valid callback anymore.
                    pDWriteFactory->UnregisterFontFileLoader(customFontFileLoader.Get());
                    pDWriteFontDownloadQueue->CancelDownload();
                });

                // Create a custom collection full of remote fonts (actually local fonts virtualized as remote fonts).
                pCustomDownloadableFontCollection = CreateCustomDownloadableSystemFontCollection(
                    pDWriteFactory.Get(),
                    customFontFileLoader.Get(),
                    resourcesPath->Data(),
                    fontFileNames,
                    ARRAYSIZE(fontFileNames)
                    );

                // Confirm the collection has all the fonts we are interested in. Consider the test
                // blocked if any are missing.
                for (auto fontName : fontNames)
                {
                    if (!DoesFontFamilyExist(pCustomDownloadableFontCollection.Get(), fontName))
                    {
                        WEX::Logging::Log::Result(
                            WEX::Logging::TestResults::Blocked,
                            L"Cannot run download tests because needed test fonts are missing from the custom font collection."
                            );
                        return;
                    }
                }

                LOG_OUTPUT(L"Setting custom font collection with remote fonts disabled.");
                RunOnUIThread([&]()
                {
                    // Clear out the current window content before injecting MockDComp, so that
                    // MockDComp doesn't capture an image for anything currently in the content,
                    // since that will interfere with the expected surface counts.
                    TestServices::WindowHelper->WindowContent = nullptr;
                    // Make XAML use our custom font collection with remote fonts for font
                    // selection. Note that system font fallback still uses the system
                    // font collection.
                    CustomRemoteFileStream::SetEnabled(false);
                });

                CustomSystemFontCollectionOverride fontCollectionOverride1(pCustomDownloadableFontCollection.Get());

                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

                TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

                LOG_OUTPUT(L"Loading text controls that reference missing remote fonts.");
                Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(resourcesPath + L"DownloadableTextTests.xaml"));

                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = root;
                });

                // Draw text controls with the custom font loader disabled to verify fallback.
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "Before");

                // Layout again with enabled remote fonts this time.
                LOG_OUTPUT(L"Reloading controls with remote fonts enabled.");
                RunOnUIThread([&]()
                {
                    CustomRemoteFileStream::SetEnabled(true);
                });

                 CustomSystemFontCollectionOverride fontCollectionOverride2(pCustomDownloadableFontCollection.Get());

                // Draw the controls again, and this time they should use the desired fonts.
                TestServices::WindowHelper->WaitForIdle(); // One tick for first measurement (triggers font downloads, uses fallback)
                LOG_OUTPUT(L"Wait for all pending font downloads to complete.");
                TestServices::WindowHelper->WaitForIdle(); // Wait for the triggered font downloads to complete.
                TestServices::WindowHelper->WaitForIdle(); // And now let the UI settle down with the newly downloaded fonts.
                LOG_OUTPUT(L"Font downloads should be complete now. Draw controls again.");
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "After");
            }
        }
    } }
} } } }
