// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlTailored.h>
#include <wincodec.h>
#include "Utilities.h"
#include <TestResource.h>
#include "Hosting.h"

bool _utilities_IsBVT()
{
    return ::Private::Infrastructure::Utilities::IsBVT();
}

#include "TestServices.h"
#include "SmartStackLogger.h"
#include "WindowHelper.h"
#include "ThemingHelper.h"
#include "VisualTreeDumper.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include "IXamlTestHooks-win.h"
#include "VerificationComparer.h"
#include "FileNameGenerator.h"
#include "RenderingScopeGuard.h"
#include "DPISetter.h"
#include "Timeouts.h"
#include "ErrorHandlingHelper.h"
#include <corewindow.h>

#include <vector>

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Private::Infrastructure;

namespace
{
    LPCWSTR GetRawBuffer(HSTRING hstring)
    {
        return ::WindowsGetStringRawBuffer(hstring, nullptr);
    }

    wrl::Wrappers::HString Concat(const wrl::Wrappers::HString& string1, const wchar_t* string2)
    {
        wrl::Wrappers::HString resultHstr;
        FAIL_FAST_IF_FAILED(resultHstr.Set((std::wstring(string1.GetRawBuffer(nullptr)) + std::wstring(string2)).c_str()));
        return resultHstr;
    }

    wrl::Wrappers::HString Concat(const wrl::Wrappers::HString& string1, const wrl::Wrappers::HString& string2)
    {
        return Concat(string1, string2.GetRawBuffer(nullptr));
    }
}

namespace
{
    struct FileNameHelper
    {
        wrl::Wrappers::HString pathAndTest;
        wrl::Wrappers::HStringReference variation;
        wrl::Wrappers::HString extension;

        FileNameHelper() = default;
        FileNameHelper(const FileNameHelper& rhs)
            : variation(rhs.variation)
        {
            LogThrow_IfFailed(pathAndTest.Set(rhs.pathAndTest.Get()));
            LogThrow_IfFailed(extension.Set(rhs.extension.Get()));
        }

        FileNameHelper& operator=(const FileNameHelper& rhs)
        {
            if (this != &rhs)
            {
                LogThrow_IfFailed(pathAndTest.Set(rhs.pathAndTest.Get()));
                variation = rhs.variation;
                LogThrow_IfFailed(extension.Set(rhs.extension.Get()));
            }
            return *this;
        }

        FileNameHelper(const wrl::Wrappers::HString& inPathAndTest,
            const wrl::Wrappers::HStringReference& inVariation,
            const wrl::Wrappers::HString& inExtension)
            : variation(inVariation)
        {
            LogThrow_IfFailed(pathAndTest.Set(inPathAndTest.Get()));
            LogThrow_IfFailed(extension.Set(inExtension.Get()));
        }

        mdc::FileNameInfo ToFileNameInfo() const
        {
            mdc::FileNameInfo fileNameInfo{ pathAndTest.Get(), variation.Get(), extension.Get() };
            return fileNameInfo;
        }
    };

    wrl::Wrappers::HString GetFileName(const FileNameHelper& helper)
    {
        return FileNameGenerator::GetFileName(helper.ToFileNameInfo());
    }

    FileNameHelper PrependPath(const wrl::Wrappers::HString& path, const FileNameHelper& original)
    {
        FileNameHelper result = original;
        result.pathAndTest = Concat(path, result.pathAndTest);
        return result;
    }

    FileNameHelper GenerateFileNameHelper(_In_opt_ HSTRING variation, _In_ HSTRING extension)
    {
        WEX::Common::String testName;
        LogThrow_IfFailed(RuntimeParameters::TryGetValue(L"FullTestName", testName));
        testName.Replace(L"Microsoft::UI::Xaml::Tests::", L"");
        testName.Replace(L"Microsoft.UI.Xaml.Tests.", L"");
        testName.Replace(L"::", L"_");
        testName.Replace(L".", L"_");
        wrl::Wrappers::HString fullTestName;
        LogThrow_IfFailed(fullTestName.Set(testName.GetBuffer()));

        wrl::Wrappers::HString localExtension;
        LogThrow_IfFailed(localExtension.Set(extension));

        FileNameHelper result{ fullTestName, wrl::Wrappers::HStringReference(GetRawBuffer(variation)), localExtension};
        return result;
    }
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {
    namespace ScreenCapture {
        void TakeScreenshot(const wchar_t* variation)
        {
            if (!::Private::Infrastructure::Utilities::TryCaptureDesktopScreenshot(variation))
            {
                LOG_OUTPUT(L"Screenshot capture failed");
            }
        }
    }
} } } } }

namespace Private { namespace Infrastructure {

    template<typename TDelegateInterface, typename TCallback>
    wrl::ComPtr<TDelegateInterface> MakeAgileCallback(TCallback callback) noexcept
    {
        return wrl::Callback<wrl::Implements<wrl::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>, TDelegateInterface, wrl::FtmBase>>(callback);
    }

    HRESULT Utilities::get_IsOneCore(_Out_ BOOLEAN* pIsOneCore)
    {
        COM_START
        {
            LogThrow_IfFailed(Utilities::IsOneCoreStatic(pIsOneCore));
        }
        COM_END
    }

    HRESULT Utilities::get_IsXBox(_Out_ BOOLEAN* pIsXBox)
    {
        COM_START
        {
            LogThrow_IfFailed(Utilities::IsXBoxStatic(pIsXBox));
        }
        COM_END
    }

    HRESULT Utilities::get_IsDesktop(_Out_ BOOLEAN* pIsDesktop)
    {
        COM_START
        {
            LogThrow_IfFailed(Utilities::IsDesktopStatic(pIsDesktop));
        }
        COM_END
    }

    HRESULT Utilities::get_IsWPF(_Out_ BOOLEAN* pIsWPF)
    {
        COM_START
        {
            LogThrow_IfFailed(Utilities::IsWPFStatic(pIsWPF));
        }
        COM_END
    }

    /* static */
    _Check_return_ HRESULT
        Utilities::IsOneCoreStatic(_Out_ BOOLEAN* pIsOneCore)
    {
        RpcClientEnsureConnected();
        return RpcIsOneCore(pIsOneCore);
    }

    // Disable MockDComp on OneCore.
    // Also can be disabled by composition object leak detection, which disables MockDComp during the actual test runs,
    // but enables it in test cleanup so that we can recreate the device and trigger the DComp device final release asserter.
    _Check_return_ HRESULT Utilities::IsMockDCompDisabled(_Out_ BOOLEAN* pIsMockDCompDisabled)
    {
        BOOLEAN isOneCore = FALSE;
        LogThrow_IfFailed(Utilities::IsOneCoreStatic(&isOneCore));
        *pIsMockDCompDisabled = static_cast<BOOLEAN>(isOneCore || (Utilities::IsCompLeakDetectionEnabled() && GetIsMockDCompDisabledForCompLeakDetection()));

        return S_OK;
    }

    _Check_return_ HRESULT
        Utilities::IsXBoxStatic(_Out_ BOOLEAN* pIsXBox)
    {
        RpcClientEnsureConnected();
        return RpcIsXBox(pIsXBox);
    }

    _Check_return_ HRESULT
        Utilities::IsDesktopStatic(_Out_ BOOLEAN* pIsDesktop)
    {
        RpcClientEnsureConnected();
        return RpcIsDesktop(pIsDesktop);
    }

    _Check_return_ HRESULT
        Utilities::IsWPFStatic(_Out_ BOOLEAN* pIsWPF)
    {
        *pIsWPF = FALSE;

        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(GetHostingMode(&hostingMode));

        *pIsWPF = static_cast<BOOLEAN>(hostingMode == Hosting::HostingMode::WPF);

        return S_OK;
    }

    HRESULT Utilities::SetRegKey(
        _In_ HSTRING path, _In_ HSTRING name, _In_ DWORD dwValue, _Out_ BOOLEAN* nameExisted, _In_ BOOLEAN currentUser)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcSetRegKey(WindowsGetStringRawBuffer(path, nullptr), WindowsGetStringRawBuffer(name, nullptr), dwValue, nameExisted, currentUser));
        }
        COM_END
    }

    HRESULT Utilities::DeleteRegKey(
        _In_ HSTRING path, _In_ HSTRING name, _In_ BOOLEAN currentUser)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcDeleteRegKey(WindowsGetStringRawBuffer(path, nullptr), WindowsGetStringRawBuffer(name, nullptr), currentUser));
        }
        COM_END
    }

    HRESULT Utilities::EnableChangingTimeZone(
        _In_ BOOLEAN enable)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcEnableChangingTimeZone(enable));
        }
        COM_END
    }

    HRESULT Utilities::SetTimeZone(_In_ HSTRING timezoneId)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcSetTimeZone(WindowsGetStringRawBuffer(timezoneId, nullptr)));
        }
        COM_END
    }


    HRESULT Utilities::RunCommandLineWithExitCode(_In_ HSTRING commandLine, _Out_ DWORD* pExitCode)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcRunCommandLine(WindowsGetStringRawBuffer(commandLine, nullptr),pExitCode));
        }
        COM_END
    }

    HRESULT Utilities::RunCommandLine(_In_ HSTRING commandLine)
    {
        DWORD exitCode = 0;
        return Utilities::RunCommandLineWithExitCode(commandLine, &exitCode);
    }

    HRESULT Utilities::VerifyMockDCompOutput(
        mdc::SurfaceComparison surfaceComparison
        )
    {
        return VerifyMockDCompOutputHelper(
            mdc::AnimationComparison_NoComparison,
            surfaceComparison,
            nullptr /* variation */);
    }

    HRESULT Utilities::VerifyMockDCompOutputWithOptions(
        mdc::SurfaceComparison surfaceComparison,
        mdc::SaveToFileOptions xmlOptions
        )
    {
        return VerifyMockDCompOutputHelper(
            mdc::AnimationComparison_NoComparison,
            surfaceComparison,
            nullptr /* variation */,
            xmlOptions);
    }

    HRESULT Utilities::VerifyMockDCompOutputWithAnimation(
        mdc::AnimationComparison animationComparison,
        _In_ HSTRING variation
        )
    {
        return VerifyMockDCompOutputHelper(
            animationComparison,
            mdc::SurfaceComparison_NoComparison,
            variation);
    }

    HRESULT Utilities::VerifyMockDCompOutputForVariationWithSucceededFlag(
        mdc::SurfaceComparison surfaceComparison,
        _In_ HSTRING variation,
        BOOLEAN ignoreVerification,
        _Out_ BOOLEAN* succeeded
        )
    {
        return VerifyMockDCompOutputHelper(
            mdc::AnimationComparison_NoComparison,
            surfaceComparison,
            variation,
            mdc::SaveToFileOptions_None,
            false /* captureProcessDumpOnFailure */,
            !!ignoreVerification,
            succeeded);
    }

    HRESULT Utilities::VerifyMockDCompOutputForVariation(
        mdc::SurfaceComparison surfaceComparison,
        _In_ HSTRING variation
        )
    {
        return VerifyMockDCompOutputHelper(
            mdc::AnimationComparison_NoComparison,
            surfaceComparison,
            variation);
    }

    HRESULT Utilities::VerifyMockDCompOutputForVariationWithOptions(
        mdc::SurfaceComparison surfaceComparison,
        _In_ HSTRING variation,
        mdc::SaveToFileOptions xmlOptions
        )
    {
        return VerifyMockDCompOutputHelper(
            mdc::AnimationComparison_NoComparison,
            surfaceComparison,
            variation,
            xmlOptions);
    }

    HRESULT Utilities::VerifyMockDCompOutputForVariationWithDataCollectionOnFailure(
        mdc::AnimationComparison animationComparison,
        mdc::SurfaceComparison surfaceComparison,
        _In_ HSTRING variation,
        _In_ BOOLEAN captureProcessDumpOnFailure
        )
    {
        return VerifyMockDCompOutputHelper(
            animationComparison,
            surfaceComparison,
            variation,
            mdc::SaveToFileOptions_None,
            captureProcessDumpOnFailure);
    }

    HRESULT Utilities::VerifyMockDCompOutputHelper(
        _In_ mdc::AnimationComparison animationComparison,
        _In_ mdc::SurfaceComparison surfaceComparison,
        _In_ HSTRING variation,
        _In_ mdc::SaveToFileOptions xmlOptions,
        _In_ BOOLEAN captureProcessDumpOnFailure,
        _In_ bool ignoreVerification,
        _Out_opt_ BOOLEAN* succeeded
        )
    {
        COM_START
        {
            BOOLEAN isDisabled = FALSE;
            LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
            if (isDisabled)
            {
                if (succeeded != nullptr)
                {
                    *succeeded = TRUE; // turn off DComp validation on onecore.
                }
            }
            else
            {
                std::vector<WEX::Common::String> surfaceVariations;
                ByteByByteComparer comparer;

                bool success = false;

                // First verification pass checks the mdc output XML files.
                success = DoVerification(
                    L"xml",
                    variation,
                    comparer,
                    [&](const mdc::FileNameInfo& fileNameInfo)
                    {
                        // DComp tree dump needs to happen on the UI thread so that we don't get another render walk to
                        // update the tree as it's being dumped.
                        RunOnUIThread([&] ()
                        {
                            wrl::ComPtr<wfc::IIterable<UINT>> spSurfaceIds;

                            wrl::ComPtr<wfc::IVectorView<IInspectable*>> rootVisuals;
                            WindowHelper::GetAllRootVisualsNoRef(rootVisuals.ReleaseAndGetAddressOf());

                            LogThrow_IfFailed(GetMockDCompDevice2()->SaveToFile(
                                rootVisuals.Get(),
                                fileNameInfo,
                                animationComparison,
                                surfaceComparison,
                                xmlOptions,
                                &spSurfaceIds
                                ));

                            // The string references must be populated here since the file access cannot happen
                            // on the UI thread due to synchronization issues because of the serialized way it is implemented.
                            // Also the spSurfaceIds can only be accessed on the UI thread.  So we parse it into a C++
                            // multi-threading safe data structure and then compare the surfaces after.
                            wrl::ComPtr<wfc::IIterator<UINT>> spIter;
                            LogThrow_IfFailed(spSurfaceIds->First(&spIter));

                            boolean hasCurrent = false;
                            LogThrow_IfFailed(spIter->get_HasCurrent(&hasCurrent));

                            WEX::Common::String surfaceFileId;

                            // NOTE: Image CRC's are output as decimal which is atypical for CRC's (which are typically output as hexadecimal)
                            //       The reason for this is so that the XML will be in decimal which is easier to parse instead of switching
                            //       back and forth depending on if it is in CRC mode or not.
                            while (hasCurrent)
                            {
                                UINT surfaceId = 0;

                                LogThrow_IfFailed(spIter->get_Current(&surfaceId));

                                if (variation == nullptr)
                                {
                                    surfaceFileId.Format(L"%u", surfaceId);
                                }
                                else
                                {
                                    surfaceFileId.Format(L"%s.%u", GetRawBuffer(variation), surfaceId);
                                }

                                surfaceVariations.push_back(surfaceFileId);

                                spIter->MoveNext(&hasCurrent);
                            }
                        });
                    });

                CaptureFailureOutput(
                    success,
                    !!captureProcessDumpOnFailure,
                    true, // captureScreenshotOnFailure
                    variation);

                if (!ignoreVerification)
                {
                    VerifySuccess(success);
                }
                else
                {
                    LOG_OUTPUT(L"!!! doesOutputMatchMaster is %s. However, we are ignoring verification for this call.", (success) ? L"true" : L"false");

                    // If we succeeded, then this run was clean and we should attempt to remove any out files that may have come from previous calls.
                    if (success)
                    {
                        StalingOutFile(wrl::Wrappers::HStringReference(L"xml").Get(), variation);
                    }
                }

                // Second verification pass checks the surfaces.
                for each (WEX::Common::String surfaceVariation in surfaceVariations)
                {
                    wrl::Wrappers::HStringReference strVariation(surfaceVariation.GetBuffer());
                    ImageComparer imageComparer;

                    wrl::Wrappers::HString diffFileNameSuffix;
                    LogThrow_IfFailed(diffFileNameSuffix.Set(L".diff.png"));
                    const auto diffFileNameHelper = GenerateFileNameHelper(strVariation.Get(), diffFileNameSuffix.Get());
                    const wrl::Wrappers::HString strDiffFilename = GetFileName(diffFileNameHelper);
                    const wrl::Wrappers::HString strDiffFilenameWithPath = GetFileName(PrependPath(GetTempFolderPath(), diffFileNameHelper));
                    imageComparer.SetDiffFilePath(wrl::Wrappers::HStringReference(strDiffFilenameWithPath.GetRawBuffer(nullptr)));

                    wrl::Wrappers::HString errorFileNameSuffix;
                    LogThrow_IfFailed(errorFileNameSuffix.Set(L".error.png"));
                    const auto errorFileNameHelper = GenerateFileNameHelper(strVariation.Get(), errorFileNameSuffix.Get());
                    const wrl::Wrappers::HString strErrorFilename = GetFileName(errorFileNameHelper);
                    const wrl::Wrappers::HString strErrorFilenameWithPath = GetFileName(PrependPath(GetTempFolderPath(), errorFileNameHelper));
                    imageComparer.SetErrorFilePath(wrl::Wrappers::HStringReference(strErrorFilenameWithPath.GetRawBuffer(nullptr)));

                    bool surfaceSuccess = false;

                    surfaceSuccess = DoVerification(
                        L"png",
                        strVariation.Get(),
                        imageComparer,
                        [&](const mdc::FileNameInfo& /*fileNameInfo*/)
                        {
                            // No-op since the surface was generated during the first verification pass.
                        });

                    if (!surfaceSuccess)
                    {
                        // Copy to the diff and error files to the output folder
                        wrl::ComPtr<wst::IStorageFile> spDiffFile;
                        wrl::ComPtr<wst::IStorageFile> spErrorFile;

                        spDiffFile = GetStorageFile(strDiffFilenameWithPath.Get());
                        spErrorFile = GetStorageFile(strErrorFilenameWithPath.Get());

                        wrl::ComPtr<wst::IStorageFolder> spOutputFolder;
                        EnsureOutputFolder(&spOutputFolder);

                        if (spDiffFile)
                        {
                            CopyFile(
                                spDiffFile,
                                strDiffFilename.Get(),
                                spOutputFolder);
                        }

                        if (spErrorFile)
                        {
                            CopyFile(
                                spErrorFile,
                                strErrorFilename.Get(),
                                spOutputFolder);
                        }
                    }

                    if (!ignoreVerification)
                    {
                        VerifySuccess(surfaceSuccess);
                    }
                    else
                    {
                        LOG_OUTPUT(L"!!! doesOutputMatchMaster is %s. However, we are ignoring verification for this call.", (surfaceSuccess) ? L"true" : L"false");

                        // If we succeeded, then this run was clean and we should attempt to remove any out files that may have come from previous calls.
                        if (success)
                        {
                            StalingOutFile(wrl::Wrappers::HStringReference(L"png").Get(), strVariation.Get());
                        }
                    }

                    success &= surfaceSuccess;
                }

                if (succeeded != nullptr)
                {
                    *succeeded = success ? TRUE : FALSE;
                }
            }
        }
        COM_END
    }

    HRESULT Utilities::SaveMockDCompOutput(
        mdc::AnimationComparison animationComparison,
        mdc::SurfaceComparison surfaceComparison,
        _In_ HSTRING variation
        )
    {
        COM_START
        {
            wrl::ComPtr<wfc::IIterable<UINT>> spSurfaceIds;

            wrl::Wrappers::HString strOutputFilename = GetFormattedTestName(variation, wrl::Wrappers::HStringReference(L".save.xml").Get());

            WEX::Common::String outputFilenameNoExtension(GetTempFolderPath().GetRawBuffer(nullptr));
            outputFilenameNoExtension.Append(strOutputFilename.GetRawBuffer(nullptr));
            outputFilenameNoExtension.Replace(L".xml", L"");

            wrl::Wrappers::HString strOutputFilenameNoExtension;
            LogThrow_IfFailed(strOutputFilenameNoExtension.Set(outputFilenameNoExtension.GetBuffer()));

            // DComp tree dump needs to happen on the UI thread so that we don't get another render walk to
            // update the tree as it's being dumped.
            RunOnUIThread([&]()
            {
                LogThrow_IfFailed(GetMockDCompDevice()->SaveToFile(
                    strOutputFilenameNoExtension.Get(),
                    animationComparison,
                    surfaceComparison,
                    mdc::SaveToFileOptions_None,
                    &spSurfaceIds
                    ));
            });

            // Create a file name that matches the one that we write out.
            // mockDcompDevice->SaveToFilesWithSurfaces appends .out.xml to the
            // file when it write out.
            wrl::Wrappers::HString strDcompOutFileWithExtension = Concat(strOutputFilenameNoExtension, L".out.xml");

            wrl::ComPtr<wst::IStorageFile> spOutputFile;
            spOutputFile = GetStorageFile(strDcompOutFileWithExtension.Get());
            if (spOutputFile)
            {
                wrl::ComPtr<wst::IStorageFolder> spOutputFolder;
                wrl::ComPtr<wsts::IBuffer> spOutputFileBuffer;

                EnsureOutputFolder(&spOutputFolder);
                ReadFileBuffer(spOutputFile, &spOutputFileBuffer);
                WriteFileBuffer(
                    strOutputFilename.Get(),
                    spOutputFolder,
                    spOutputFileBuffer
                    );
            }
        }
            COM_END
    }

    HRESULT Utilities::VerifyUIElementTreeHelper(
        bool useXmlValidation,
        _In_opt_ HSTRING variation,
        _In_opt_ HSTRING rulesFilename,
        _In_opt_ HSTRING inlineRulesXml,
        bool ignorePopups,
        _In_opt_ xaml::IDependencyObject* root,
        _Out_opt_ BOOLEAN* result)
    {
        COM_START
        {
            std::unique_ptr<VerificationComparer> comparer;
            std::function<void(const mdc::FileNameInfo&)> xmlDumpFn;

            if (!useXmlValidation)
            {
                // Legacy comparison: byte-by-byte.  Once all tests are updated, this code should be removed.

                // IgnorePopups option is not supported in legacy xml validation
                _ASSERT(!ignorePopups);

                comparer.reset(new ByteByByteComparer());

                xmlDumpFn = [&](const mdc::FileNameInfo& fileNameInfo)
                {
                    VisualTreeDumper::DumpToXmlOldFormat(root, FileNameGenerator::GetFileName(fileNameInfo).Get());
                };
            }
            else
            {
                comparer.reset(new VisualTreeXMLComparer());

                xmlDumpFn = [ignorePopups, root](const mdc::FileNameInfo& fileNameInfo)
                {
                    VisualTreeDumper::DumpToXml(root, FileNameGenerator::GetFileName(fileNameInfo).Get(), ignorePopups);
                };

                if (rulesFilename != nullptr)
                {
                    static_cast<VisualTreeXMLComparer*>(comparer.get())->SetRulesFile(wrl::Wrappers::HStringReference(GetRawBuffer(rulesFilename)));
                }
                else if (inlineRulesXml != nullptr)
                {
                    static_cast<VisualTreeXMLComparer*>(comparer.get())->SetRulesInline(wrl::Wrappers::HStringReference(GetRawBuffer(inlineRulesXml)));
                }
            }

            bool success = DoVerification(
                L"xml",
                variation,
                *comparer.get(),
                xmlDumpFn);

            CaptureFailureOutput(
                success,
                false, // captureProcessDumpOnFailure
                true, // captureScreenshotOnFailure
                variation
            );

            VerifySuccess(success);

            if (result != nullptr)
            {
                *result = success ? TRUE : FALSE;
            }
        }
        COM_END
    }

    HRESULT Utilities::VerifyUIElementTree(_Out_ BOOLEAN* result)
    {
        return VerifyUIElementTreeHelper(
            false,      // useXmlValidation
            nullptr,    // variation
            nullptr,    // rulesFilename
            nullptr,    // inlineRulesXml
            false,      // ignorePopups
            nullptr,    // root
            result
            );
    }

    HRESULT Utilities::VerifyUIElementTreeWithRoot(_In_ xaml::IDependencyObject* root, _Out_ BOOLEAN* result)
    {
        return VerifyUIElementTreeHelper(
            false,      // useXmlValidation
            nullptr,    // variation
            nullptr,    // rulesFilename
            nullptr,    // inlineRulesXml
            false,      // ignorePopups
            root,       // root element
            result
            );
    }

    HRESULT Utilities::VerifyUIElementTreeForVariation(_In_opt_ HSTRING variation, _Out_ BOOLEAN* result)
    {
        return VerifyUIElementTreeHelper(
            false,      // useXmlValidation
            variation,
            nullptr,    // rulesFilename
            nullptr,    // inlineRulesXml
            false,      // ignorePopups
            nullptr,    // root
            result
            );
    }

    HRESULT Utilities::VerifyUIElementTreeWithRules(_In_ HSTRING rulesFilename, _Out_ BOOLEAN* result)
    {
        return VerifyUIElementTreeHelper(
            true,           // useXmlValidation
            nullptr,        // variation
            rulesFilename,
            nullptr,        // inlineRulesXml
            false,          // ignorePopups
            nullptr,        // root
            result
            );
    }

    HRESULT Utilities::VerifyUIElementTreeForVariationWithRules(_In_ HSTRING variation, _In_ HSTRING rulesFilename, _Out_ BOOLEAN* result)
    {
        return VerifyUIElementTreeHelper(
            true,           // useXmlValidation
            variation,
            rulesFilename,
            nullptr,        // inlineRulesXml
            false,          // ignorePopups
            nullptr,        // root
            result
            );
    }

    HRESULT Utilities::VerifyUIElementTreeWithRulesInline(_In_ HSTRING inlineRulesXml, _Out_ BOOLEAN* result)
    {
        return VerifyUIElementTreeHelper(
            true,           // useXmlValidation
            nullptr,        // variation
            nullptr,        // rulesFilename
            inlineRulesXml,
            false,          // ignorePopups
            nullptr,        // root
            result
            );
    }

    HRESULT Utilities::VerifyUIElementTreeForVariationWithRulesInline(_In_ HSTRING variation, _In_ HSTRING inlineRulesXml, _Out_ BOOLEAN* result)
    {
        return VerifyUIElementTreeHelper(
            true,           // useXmlValidation
            variation,
            nullptr,        // rulesFilename
            inlineRulesXml,
            false,          // ignorePopups
            nullptr,        // root
            result
            );
    }

    HRESULT Utilities::VerifyUIElementTreeForVariationWithRulesInlineWithIgnorePopupsOption(_In_ HSTRING variation, _In_ HSTRING inlineRulesXml, BOOLEAN ignorePopups, _Out_ BOOLEAN* result)
    {
        return VerifyUIElementTreeHelper(
            true,           // useXmlValidation
            variation,
            nullptr,        // rulesFilename
            inlineRulesXml,
            ignorePopups,
            nullptr,        // root
            result
            );
    }

    HRESULT Utilities::VerifyUIElementTreeContainsOnlyValidColors(_In_ HSTRING variation, _In_ wfc::IVector<unsigned int>* validColors)
    {
        COM_START
        {
            const wrl::Wrappers::HString outputFilename = GetFormattedTestName(variation, wrl::Wrappers::HStringReference(L".out.xml").Get());
            wrl::Wrappers::HString outputFilenameWithPath = Concat(GetTempFolderPath(), outputFilename);

            VisualTreeDumper::DumpToXml(nullptr /* pRoot */, outputFilenameWithPath.Get());

            // A function that returns true if a given color is valid.
            // A Color is considered valid if it is fully transparent or if it is in the list of valid colors.
            auto verifyColor = [&](unsigned int colorToTest)
            {
                // Accept Transparent as a valid color.
                if ((colorToTest & 0xFF000000) == 0)
                {
                    return true;
                }

                unsigned int size;
                validColors->get_Size(&size);
                for (unsigned int i = 0; i < size; i++)
                {
                    unsigned int validColor;
                    validColors->GetAt(i, &validColor);
                    if (colorToTest == validColor)
                    {
                        return true;
                    }
                }

                return false;
            };

            BOOLEAN result = FALSE;
            LogThrow_IfFailed(VerifyOutputFileColors(variation, verifyColor, &result));

            if (!result)
            {
                wrl::ComPtr<wst::IStorageFolder> spOutputFolder;
                EnsureOutputFolder(&spOutputFolder);

                auto outputFile = GetStorageFile(outputFilenameWithPath.Get());

                if (outputFile)
                {
                    CopyFile(
                        outputFile,
                        outputFilename.Get(),
                        spOutputFolder);
                }

                CaptureScreenshot(variation);
            }

        }
        COM_END
    }

    HRESULT Utilities::ResetOptionalChanges()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> testHooks = WindowHelper::GetTestHooks();
                testHooks->ResetOptionalChanges();
            });
        }
        COM_END
    }

    HRESULT Utilities::SetRuntimeEnabledFeatureOverride(_In_ INT feature, _In_ BOOLEAN enabled, _Out_opt_ BOOLEAN* wasPreviouslyEnabled)
    {
        COM_START
        {
            bool wasPreviouslyEnabledBool = SetRuntimeFeatureOverride(static_cast<RuntimeFeatureBehavior::RuntimeEnabledFeature>(feature), !!enabled);
            if (wasPreviouslyEnabled != nullptr)
            {
                *wasPreviouslyEnabled = wasPreviouslyEnabledBool;
            }
        }
        COM_END
    }

    bool Utilities::SetRuntimeFeatureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature feature, bool isEnabled)
    {
        bool wasPreviouslyEnabledBool = false;
        RunOnUIThread([&]()
        {
            wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
            windowTestHooks->SetRuntimeEnabledFeatureOverride(feature, isEnabled, &wasPreviouslyEnabledBool);
        });

        return wasPreviouslyEnabledBool;
    }

    HRESULT Utilities::ClearAllRuntimeEnabledFeatureOverrides()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->ClearAllRuntimeFeatureOverrides();
            });
        }
        COM_END
    }

    HRESULT Utilities::VerifyOutputFileHighContrastColors(_In_ HSTRING variation, _In_ test_infra::HighContrastTheme theme)
    {
        COM_START
        {
            auto verifyColorIsValidForCurrentHighContrastTheme = [&theme](unsigned colorValue)
            {
                return ThemingHelper::IsHighContrastColor(theme, colorValue);
            };
            LogThrow_IfFailed(VerifyOutputFileColors(variation, verifyColorIsValidForCurrentHighContrastTheme, nullptr));
        }
        COM_END
    }

    HRESULT Utilities::VerifyOutputFileColors(_In_ HSTRING variation, _In_ std::function<bool(unsigned)> verifyColor, _Out_ BOOLEAN* result)
    {
        COM_START
        {
            bool verificationFailed = false;
            bool isFirstFailure = true;

            const wrl::Wrappers::HString outputFilename = GetFormattedTestName(variation, wrl::Wrappers::HStringReference(L".out.xml").Get());
            wrl::Wrappers::HString outputFilenameWithPath = Concat(GetTempFolderPath(), outputFilename);

            auto outputFile = GetStorageFile(outputFilenameWithPath.Get());

            wrl::ComPtr<wfc::IVector<HSTRING>> lines;
            ReadFileLines(outputFile, lines.ReleaseAndGetAddressOf());

            unsigned int lineCount = 0;
            LogThrow_IfFailed(lines->get_Size(&lineCount));
            for (unsigned int lineNumber = 0; lineNumber < lineCount; ++lineNumber)
            {
                wrl::Wrappers::HString line;
                LogThrow_IfFailed(lines->GetAt(lineNumber, line.ReleaseAndGetAddressOf()));

                unsigned int bufferLength = 0;
                auto buffer = line.GetRawBuffer(&bufferLength);

                for (unsigned int index = 0; index < bufferLength; ++index)
                {
                    // Color values are preceded by the '#' character
                    if (buffer[index] == L'#')
                    {
                        // Don't parse the '#INF' part of '1.#INF' as a color.
                        if ((index < bufferLength - 1) && buffer[index + 1] != L'I')
                        {
                            unsigned int colorValue = wcstoul(buffer + index + 1, nullptr, 16);

                            bool isValidColor = verifyColor(colorValue);
                            if (!isValidColor)
                            {
                                // We skip the first failure because it corresponds to the root panel's background which
                                // is set to a non high-contrast color such as black or white to make visual
                                // verification easier.
                                if (!isFirstFailure)
                                {
                                    // Work around the fact that TextBlocks initialize their SelectionHighLightColor
                                    // to the accent color but don't update this on high-contrast changes.
                                    bool isAcceptableFailure = (colorValue == ThemingHelper::GetTestAccentColorValue() && std::wstring(buffer).find(L"SelectionHighlightColor") != std::wstring::npos);

                                    if (!isAcceptableFailure)
                                    {
                                        WEX::TestExecution::DisableVerifyExceptions disable;

                                        VERIFY_FAIL();

                                        LOG_OUTPUT(L"!!!! Invalid color on line %d: %s", lineNumber + 1, buffer);

                                        verificationFailed = true;
                                    }
                                }

                                isFirstFailure = false;
                            }
                        }
                    }
                }
            }

            if (result != nullptr)
            {
                *result = verificationFailed ? FALSE: TRUE;
            }
        }
        COM_END
    }

    HRESULT Utilities::CreateLoopingSelector(_Outptr_ IInspectable** ppLoopingSelector)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                LogThrow_IfFailed(windowTestHooks->CreateLoopingSelector(ppLoopingSelector));
            });
        }
        COM_END
    }

    HRESULT Utilities::InjectBackButtonPress(_Out_ BOOLEAN* pHandled)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                LogThrow_IfFailed(windowTestHooks->InjectBackButtonPress(pHandled));
            });
        }
        COM_END
    }

    HRESULT Utilities::ResetMetadata()
    {
        COM_START
        {
            // We need to run this on the UI thread, because ResetMetadata needs access
            // to CCoreServices so that it can clear its schema context.
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                LogThrow_IfFailed(windowTestHooks->ResetMetadata());
            });
        }
        COM_END
    }

    HRESULT Utilities::ClearDefaultLanguageString()
    {
        COM_START
        {
            // We need to run this on the UI thread, because ResetMetadata needs access
            // to CCoreServices so that it can clear its schema context.
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                LogThrow_IfFailed(windowTestHooks->ClearDefaultLanguageString());
            });
        }
        COM_END
    }

    HRESULT Utilities::IsWindowFocused(_Out_ BOOLEAN* isFocused, _In_ xaml::IXamlRoot* xamlRoot)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                LogThrow_IfFailed(windowTestHooks->IsWindowFocused(isFocused, xamlRoot));
            });
        }
            COM_END
    }

    HRESULT Utilities::OverrideTrimImageResourceDelay(_In_ BOOLEAN enabled)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->OverrideTrimImageResourceDelay(!!enabled);
            });
        }
        COM_END
    }

    HRESULT Utilities::ResetAtlasSizeHint()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->ResetAtlasSizeHint();
            });
        }
        COM_END
    }

    HRESULT Utilities::ShrinkApplicationViewVisibleBounds(_In_ BOOLEAN enabled)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->ShrinkApplicationViewVisibleBounds(!!enabled);
            });
        }
        COM_END
    }

    HRESULT Utilities::GetPopupOverlayElement(
        _In_ xaml_primitives::IPopup* popup,
        _Outptr_ xaml::IFrameworkElement** overlayElement
        )
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                LogThrow_IfFailed(WindowHelper::GetTestHooks()->GetPopupOverlayElement(popup, overlayElement));
            });
        }
        COM_END
    }

    HRESULT Utilities::DeletePlatformFamilyCache()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                WindowHelper::GetTestHooks()->DeletePlatformFamilyCache();
            });
        }
        COM_END
    }

    HRESULT Utilities::DeleteResourceDictionaryCaches()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                WindowHelper::GetTestHooks()->DeleteResourceDictionaryCaches();
            });
        }
        COM_END
    }

    HRESULT Utilities::RetrieveOpenBottomCommandBar(_In_ xaml::IXamlRoot* xamlRoot, _Outptr_ xaml_controls::ICommandBar** cmdBar)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<xaml_controls::ICommandBarStaticsPrivate> spCommandBarStaticsPrivate;

                LogThrow_IfFailed(wf::GetActivationFactory(
                    wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_CommandBar).Get(),
                    &spCommandBarStaticsPrivate)
                    );

                LogThrow_IfFailed(spCommandBarStaticsPrivate->GetCurrentBottomCommandBar(xamlRoot, cmdBar));
            });
        }
        COM_END
    }

    HRESULT Utilities::SetMockDCompSwapChainIdentifier(_In_ unsigned __int64 identifierValue, _In_ unsigned int identifierID)
    {
        COM_START
        {
            BOOLEAN isDisabled = FALSE;
            LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
            if (!isDisabled)
            {
                RunOnUIThread([&]()
                {
                    const auto& mockDCompDevice = GetMockDCompDevice();
                    if (mockDCompDevice)
                    {
                        LogThrow_IfFailed(GetMockDCompDevice()->SetMockDCompSwapChainIdentifier(identifierValue, identifierID));
                    }
                });
            }
        }
        COM_END
    }

    HRESULT Utilities::ResetWasCommitInvoked()
    {
        COM_START
        {
            BOOLEAN isDisabled = FALSE;
            LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
            if (!isDisabled)
            {
                RunOnUIThread([&]()
                {
                    LogThrow_IfFailed(GetMockDCompDevice()->ResetWasCommitInvoked());
                });
            }
        }
        COM_END
    }

    HRESULT Utilities::get_WasCommitInvoked(_Out_ BOOLEAN* pWasCommitInvoked)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                *pWasCommitInvoked = false;
                LogThrow_IfFailed(GetMockDCompDevice()->GetWasCommitInvoked(pWasCommitInvoked));
            });
        }
        COM_END
    }

    HRESULT Utilities::VerifyAreSurfaceResourcesOffered(_In_ BOOLEAN value)
    {
        COM_START
        {
            BOOLEAN isDisabled = FALSE;
            LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
            if (!isDisabled)
            {
                RunOnUIThread([&]()
                {
                    //BOOLEAN areOffered = false;
                    //LogThrow_IfFailed(GetMockDCompDevice()->GetAreSurfaceResourcesOffered(&areOffered));
                    // TODO: Update the mock implementation to reflect the per-surfaceFactory offer/reclaim change.
                    //VERIFY_ARE_EQUAL(value, areOffered);
                    value = value;
                });
            }
        }
        COM_END
    }

    HRESULT Utilities::SetMockDCompSurfaceIdMode(_In_ mdc::SurfaceIdMode mode)
    {
        COM_START
        {
            BOOLEAN isDisabled = FALSE;
            LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
            if (!isDisabled)
            {
                RunOnUIThread([&]()
                {
                    GetMockDCompDevice()->SetSurfaceIdMode(mode);
                });
            }
        }
            COM_END
    }

    HRESULT Utilities::ResetMockDCompSurfaceId()
    {
        COM_START
        {
            BOOLEAN isDisabled = FALSE;
            LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
            if (!isDisabled)
            {
                RunOnUIThread([&]()
                {
                    GetMockDCompDevice()->ResetSurfaceId();
                });
            }
        }
            COM_END
    }

    HRESULT Utilities::SimulateAllSurfacesDiscarded()
    {
        COM_START
        {
            BOOLEAN isDisabled = FALSE;
            LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
            if (!isDisabled)
            {
                RunOnUIThread([&]()
                {
                    GetMockDCompDevice()->SimulateAllSurfacesDiscarded();
                });
            }
        }
        COM_END
    }

    HRESULT Utilities::SetReturnDeviceLostWhenCreatingSurfaces(_In_ BOOLEAN returnDeviceLostWhenCreatingSurfaces)
    {
        COM_START
        {
            BOOLEAN isDisabled = FALSE;
            LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
            if (!isDisabled)
            {
                RunOnUIThread([&]()
                {
                    GetMockDCompDevice()->SetReturnDeviceLostWhenCreatingSurfaces(!!returnDeviceLostWhenCreatingSurfaces);
                });
            }
        }
        COM_END
    }

    void Utilities::StalingOutFile(
        _In_ HSTRING fileExtension,
        _In_opt_ HSTRING variation)
    {
        wrl::Wrappers::HString fullExtension;
        LogThrow_IfFailed(fullExtension.Set((std::wstring(L".out") + GetRawBuffer(fileExtension)).c_str()));
        const wrl::Wrappers::HString strOutputFilename = GetFormattedTestName(variation, fullExtension.Get());

        wrl::ComPtr<wst::IStorageFolder> spOutputFolder;

        // This has the side effect of creating Pictures\XamlTAEFOutput even if we don't need it.
        EnsureOutputFolder(&spOutputFolder);

        // Try to get the .out file if there is one
        wrl::ComPtr<wf::IAsyncOperation<wst::StorageFile*>> spGetFileOperation;
        LogThrow_IfFailed(spOutputFolder->GetFileAsync(
            strOutputFilename.Get(),
            &spGetFileOperation
            ));

        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> spGetFileCompleteEvent = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>(L"GetFileCompleted");
        LogThrow_IfFailed(spGetFileOperation->put_Completed(
            MakeAgileCallback<wf::IAsyncOperationCompletedHandler<wst::StorageFile*>>(
                [spGetFileCompleteEvent] (wf::IAsyncOperation<wst::StorageFile*>*, wf::AsyncStatus) -> HRESULT
                {
                    spGetFileCompleteEvent->Set();
                    return S_OK;
                }
                ).Get()
            ));
        spGetFileCompleteEvent->WaitForDefault();

        wrl::ComPtr<wst::IStorageFile> spOutputFile;
        spGetFileOperation->GetResults(&spOutputFile);

        if (spOutputFile == nullptr)
        {
            // Could not find the outfile, meaning one was not created as a result of failure
        }
        else
        {
            LOG_OUTPUT(L"Found an outfile. Since we are currently passing, overwrite this file because it is stale.");

            WriteFileString(
                strOutputFilename.Get(),
                spOutputFolder,
                wrl_wrappers::HStringReference(L"NOTE: Do not check in this file. This file is stale. Although MockDComp verification failed during this test during one or more passes, it eventually succeeded.").Get()
                );
        }
    }

    HRESULT Utilities::ResetMockDCompGraphicsDevice()
    {
        COM_START
        {
            BOOLEAN isDisabled = FALSE;
            LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
            if (!isDisabled)
            {
                RunOnUIThread([&]()
                {
                    GetMockDCompDevice()->ResetGraphicsDevice();
                });
            }
        }
        COM_END
    }

    HRESULT Utilities::SimulateDeviceLostOnOffThreadImageUpload()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> testHooks = WindowHelper::GetTestHooks();
                testHooks->SimulateDeviceLostOnOffThreadImageUpload();
            });
        }
        COM_END
    }

    HRESULT Utilities::SimulateSwallowedDeviceLostOnStartup()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> testHooks = WindowHelper::GetTestHooks();
                testHooks->SimulateSwallowedDeviceLostOnStartup();
            });
        }
        COM_END
    }

    HRESULT Utilities::SetDCompDeviceLeakDetectionEnabled(_In_ BOOLEAN enableLeakDetection)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> testHooks = WindowHelper::GetTestHooks();
                testHooks->SetDCompDeviceLeakDetectionEnabled(!!enableLeakDetection);
            });
        }
        COM_END

    }

    bool Utilities::DoVerification(
        _In_ PCWSTR fileExtension,
        _In_opt_ HSTRING variation,
        _In_ VerificationComparer& comparer,
        std::function<void(const mdc::FileNameInfo&)> outputFileGenerator)
    {
        bool result = false;

        const bool isXmlFile = (std::wstring(fileExtension) == L"xml");
        wrl::Wrappers::HString outputFileNameSuffix;
        LogThrow_IfFailed(outputFileNameSuffix.Set((std::wstring(L".out.") + fileExtension).c_str()));

        const auto outputFileNameHelper = GenerateFileNameHelper(variation, outputFileNameSuffix.Get());

        const wrl::Wrappers::HString strOutputFilename = GetFileName(outputFileNameHelper);
        const wrl::Wrappers::HString strOutputFilenameWithPath = GetFileName(PrependPath(GetTempFolderPath(), outputFileNameHelper));

        // Call the custom function that will generate our output file with the given name.
        // We'll open this file later and compare it with a corresponding baseline file.
        outputFileGenerator(PrependPath(GetTempFolderPath(), outputFileNameHelper).ToFileNameInfo());

        wrl::Wrappers::HString masterFileNameSuffix;
        LogThrow_IfFailed(masterFileNameSuffix.Set((std::wstring(L".master.") + fileExtension).c_str()));
        auto masterFileNameHelper = GenerateFileNameHelper(variation, masterFileNameSuffix.Get());

        wrl::Wrappers::HString strMasterFilename = GetFileName(masterFileNameHelper);
        wrl::Wrappers::HString strMasterFilenameWithPath = GetFileName(PrependPath(GetMastersFolderPath(), masterFileNameHelper));

        wrl::ComPtr<wst::IStorageFile> spOutputFile;
        wrl::ComPtr<wst::IStorageFile> spMasterFile;

        spOutputFile = GetStorageFile(strOutputFilenameWithPath.Get());
        spMasterFile = GetStorageFile(strMasterFilenameWithPath.Get());

        // If neither file exists, then this file was apparently not needed (such as due to this
        // index being for a surface which is no longer referenced).  Everything is good in this
        // case, and we can just exit out now.
        if (!spMasterFile && !spOutputFile)
        {
            return true;
        }

        // If we didn't find a platform-agnostic baseline file, look for a platform-specific one.
        if (!spMasterFile)
        {
            LOG_OUTPUT(L"Platform-agnostic baseline file is missing: %s", strMasterFilenameWithPath.GetRawBuffer(nullptr));
            LOG_OUTPUT(L"Looking for platform-specific baseline file.");

            BOOLEAN isOneCore = FALSE;
            LogThrow_IfFailed(IsOneCoreStatic(&isOneCore));

            if (isOneCore)
            {
                masterFileNameSuffix.Set(L".master.onecore.");
                masterFileNameSuffix = Concat(masterFileNameSuffix, fileExtension);
                masterFileNameHelper = GenerateFileNameHelper(variation, masterFileNameSuffix.Get());
                strMasterFilename = GetFileName(masterFileNameHelper);
                strMasterFilenameWithPath = GetFileName(PrependPath(GetMastersFolderPath(), masterFileNameHelper));
                spMasterFile = GetStorageFile(strMasterFilenameWithPath.Get());

                if (!spMasterFile)
                {
                    LogThrow_IfFailed(masterFileNameSuffix.Set(L".master.clientcore."));
                }
            }
            else
            {
                LogThrow_IfFailed(masterFileNameSuffix.Set(L".master.clientcore."));
            }

            if (!spMasterFile)
            {
                masterFileNameSuffix = Concat(masterFileNameSuffix, fileExtension);
                masterFileNameHelper = GenerateFileNameHelper(variation, masterFileNameSuffix.Get());
                strMasterFilename = GetFileName(masterFileNameHelper);
                strMasterFilenameWithPath = GetFileName(PrependPath(GetMastersFolderPath(), masterFileNameHelper));
                spMasterFile = GetStorageFile(strMasterFilenameWithPath.Get());
            }
        }

        if (spMasterFile || spOutputFile)
        {
            bool doesOutputMatchMaster = false;

            if (spMasterFile && spOutputFile)
            {
                LOG_OUTPUT(L"Comparing files for variation: %s", GetRawBuffer(variation));
                doesOutputMatchMaster = comparer.CompareFiles(
                    wrl::Wrappers::HStringReference(strMasterFilenameWithPath.GetRawBuffer(nullptr)),
                    wrl::Wrappers::HStringReference(strOutputFilenameWithPath.GetRawBuffer(nullptr)));

                if (!doesOutputMatchMaster)
                {
                    LOG_OUTPUT(L"Comparison between output file and baseline file failed.");
                    comparer.OutputAdditionalResults(isXmlFile);
                }
            }
            else if (!spMasterFile)
            {
                LOG_OUTPUT(L"Platform-specific baseline file is missing: %s", strMasterFilenameWithPath.GetRawBuffer(nullptr));
            }
            else
            {
                LOG_OUTPUT(L"Output file is missing: %s", strOutputFilenameWithPath.GetRawBuffer(nullptr));
            }

            // If comparison failed, or one of the files was missing, write out what we have
            // to our output directory.
            if (!doesOutputMatchMaster)
            {
                wrl::ComPtr<wst::IStorageFolder> spOutputFolder;
                EnsureOutputFolder(&spOutputFolder);

                if (spMasterFile)
                {
                    CopyFile(
                        spMasterFile,
                        strMasterFilename.Get(),
                        spOutputFolder);
                }

                if (spOutputFile)
                {
                    CopyFile(
                        spOutputFile,
                        strOutputFilename.Get(),
                        spOutputFolder);
                }
            }

            // Only break for XML baseline file comparisons; skip the break for the surface comparisons.
            if (isXmlFile && IsDebuggerPresent())
            {
                WEX::Common::String value;
                if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"BreakOnVerifyMaster", value)))
                {
                    if (value.CompareNoCase(L"Always") == 0 || !doesOutputMatchMaster)
                    {
                        DebugBreak();
                    }
                }
            }

            result = doesOutputMatchMaster;
        }

        return result;
    }

    void Utilities::VerifySuccess(bool didOutputMatchMaster)
    {
        // We want subsequent validation tests to run even if this one fails,
        // so disable the verify exception.
        WEX::TestExecution::DisableVerifyExceptions disable;

        VERIFY_IS_TRUE(didOutputMatchMaster);
    }

    void Utilities::CaptureFailureOutput(bool didOutputMatchMaster, bool captureProcessDumpOnFailure, bool captureScreenshotOnFailure, _In_opt_ HSTRING variation)
    {
        // If baseline did not match and a process dump was requested on failure, do so now.
        if (!didOutputMatchMaster && captureProcessDumpOnFailure)
        {
            CaptureProcessDump();
        }

        // Capture screenshot if verification failed, with the name of the variation.
        if (!didOutputMatchMaster && captureScreenshotOnFailure)
        {
            CaptureScreenshot(variation);
        }
    }

    void Utilities::CaptureProcessDump()
    {
        NoThrowString tempFile;
        Debug::SaveDump(MiniDumpFormat::Default, tempFile);
        WEX::Logging::Log::File(tempFile);

        // Now that we've told WEX::Logging about the dump file it's successfully copied it to
        // the test output directory. We delete the original.
        ::DeleteFile(tempFile);
    }

    void Utilities::CaptureScreenshot(_In_opt_ HSTRING variation)
    {
        ScreenCapture::TakeScreenshot(GetRawBuffer(variation));
    }

    void Utilities::EnsureOutputFolder(_Outptr_ wst::IStorageFolder **ppOutputFolder)
    {
        wrl::ComPtr<wst::IKnownFoldersStatics> spKnownFolders;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Windows_Storage_KnownFolders).Get(),
            &spKnownFolders
            ));

        // According to http://msdn.microsoft.com/en-us/library/windows/apps/br211423.aspx,
        // there is a "documentsLibrary" capability. However, it doesn't show up in Visual Studio
        // and doesn't appear to work, so we save output to the pictures library instead.
        // The corresponding folder on phone is C:\Data\Users\Public\Pictures\.
        wrl::ComPtr<wst::IStorageFolder> spPicturesFolder;
        LogThrow_IfFailed(spKnownFolders->get_PicturesLibrary(&spPicturesFolder));

        wrl::Wrappers::HStringReference folderName(L"XamlTAEFOutput");
        wrl::ComPtr<wf::IAsyncOperation<wst::StorageFolder*>> spCreateFolderOperation;
        LogThrow_IfFailed(spPicturesFolder->CreateFolderAsync(
            folderName.Get(),
            wst::CreationCollisionOption_OpenIfExists,
            &spCreateFolderOperation
            ));

        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> spCreateFolderCompleteEvent = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>(L"CreateFolderCompleted");
        LogThrow_IfFailed(spCreateFolderOperation->put_Completed(
            MakeAgileCallback<wf::IAsyncOperationCompletedHandler<wst::StorageFolder*>>(
                [spCreateFolderCompleteEvent] (wf::IAsyncOperation<wst::StorageFolder*>*, wf::AsyncStatus) -> HRESULT
                {
                    spCreateFolderCompleteEvent->Set();
                    return S_OK;
                }
                ).Get()
            ));
        spCreateFolderCompleteEvent->WaitForDefault();

        wrl::ComPtr<wst::IStorageFolder> spOutputFolder;
        LogThrow_IfFailed(spCreateFolderOperation->GetResults(&spOutputFolder));
        LogThrow_IfFailed(spOutputFolder.CopyTo(ppOutputFolder));
    }

    HRESULT ReadTestResource(_In_ const wchar_t* resourceName, _Out_ const uint8_t** pData, _Out_ DWORD* pDataLength)
    {
        static HMODULE resourcesModuleHandle = LoadLibraryEx(L"Private.Infrastructure.Resources.dll", NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
        if (resourcesModuleHandle == nullptr)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        HRSRC rc = FindResource(resourcesModuleHandle, resourceName, MAKEINTRESOURCE(TESTRESOURCE));
        if (!rc)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        *pDataLength = SizeofResource(resourcesModuleHandle, rc);
        if (0 == *pDataLength)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        HGLOBAL rcData = LoadResource(resourcesModuleHandle, rc);
        if (!rcData)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        *pData = static_cast<const uint8_t*>(LockResource(rcData));

        return S_OK;
    }

    HRESULT WriteTestResource(wsts::IOutputStream* outputStream, _In_ const uint8_t* data, _In_ DWORD dataLength)
    {
        wrl::ComPtr<wsts::IDataWriterFactory> dataWriterFactory;
        RETURN_IF_FAILED(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Windows_Storage_Streams_DataWriter).Get(),
            &dataWriterFactory));
        wrl::ComPtr<wsts::IDataWriter> dataWriter;
        RETURN_IF_FAILED(dataWriterFactory->CreateDataWriter(outputStream, &dataWriter));
        RETURN_IF_FAILED(dataWriter->WriteBytes(dataLength, const_cast<uint8_t*>(data)));

        wrl::ComPtr<wf::IAsyncOperation<uint32_t>> storeOperation;
        RETURN_IF_FAILED(dataWriter->StoreAsync(&storeOperation));
        auto completeEvent = std::make_shared<Event>(L"WriterCompleted");
        auto callback = MakeAgileCallback<wf::IAsyncOperationCompletedHandler<uint32_t>>(
            [completeEvent](wf::IAsyncOperation<uint32_t>*, wf::AsyncStatus status) -> HRESULT
        {
            completeEvent->Set();
            return status == wf::AsyncStatus::Completed ? S_OK : E_FAIL;
        });
        RETURN_IF_FAILED(storeOperation->put_Completed(callback.Get()));
        completeEvent->WaitForDefault();

        uint32_t bytesStored = 0;
        RETURN_IF_FAILED(storeOperation->GetResults(&bytesStored));
        if (dataLength != bytesStored)
        {
            return E_UNEXPECTED;
        }

        wrl::ComPtr<wf::IClosable> closable;
        RETURN_IF_FAILED(dataWriter.As(&closable));
        RETURN_IF_FAILED(closable->Close());

        return S_OK;
    }

    wrl::ComPtr<wst::IStorageFile> Utilities::GetStorageFile(_In_ HSTRING filenameWithPath)
    {
        wrl::ComPtr<wst::IStorageFileStatics> spStorageFileStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Windows_Storage_StorageFile).Get(),
            &spStorageFileStatics));

        wrl::ComPtr<wf::IAsyncOperation<wst::StorageFile*>> getFileAsyncOp;

        std::wstring resPath(::WindowsGetStringRawBuffer(filenameWithPath, nullptr));
        const static std::wstring mastersMarkerStr(L"\\test\\resources\\masters\\");
        const auto found = resPath.find(mastersMarkerStr);
        const bool isMaster = found != std::wstring::npos;
        if (isMaster)
        {
            auto name = resPath.substr(found+mastersMarkerStr.size());

            const uint8_t* data = nullptr;
            DWORD dataLength = 0;
            if (FAILED(ReadTestResource(name.c_str(), &data, &dataLength)))
            {
                return nullptr;
            }

            auto readCallback = MakeAgileCallback<wst::IStreamedFileDataRequestedHandler>(
                [data, dataLength](wsts::IOutputStream* stream) -> HRESULT
            {
                return WriteTestResource(stream, data, dataLength);
            });
            LogThrow_IfFailed(spStorageFileStatics->CreateStreamedFileAsync(
                wrl::Wrappers::HStringReference(name.c_str()).Get(),
                readCallback.Get(),
                nullptr,
                &getFileAsyncOp));
        }
        else
        {
            LogThrow_IfFailed(spStorageFileStatics->GetFileFromPathAsync(filenameWithPath, &getFileAsyncOp));
        }

        auto operationCompleteEvent = std::make_shared<Event>(L"FileOperationCompleted");
        auto asyncStatus = wf::AsyncStatus::Started;
        auto completeCallback = MakeAgileCallback<wf::IAsyncOperationCompletedHandler<wst::StorageFile*>>(
            [operationCompleteEvent, &asyncStatus] (wf::IAsyncOperation<wst::StorageFile*>*, wf::AsyncStatus status) -> HRESULT
            {
                asyncStatus = status;
                operationCompleteEvent->Set();
                return S_OK;
            });
        LogThrow_IfFailed(getFileAsyncOp->put_Completed(completeCallback.Get()));
        operationCompleteEvent->WaitForDefault();

        if (asyncStatus != wf::AsyncStatus::Completed)
        {
            wrl::ComPtr<wf::IAsyncInfo> spAsyncInfo;
            HRESULT errorHr = S_OK;

            LogThrow_IfFailed(getFileAsyncOp.As(&spAsyncInfo));
            LogThrow_IfFailed(spAsyncInfo->get_ErrorCode(&errorHr));

            // File not found is acceptable.  We'll return a null value
            // in those cases.
            if (errorHr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                LogThrow_IfFailed(errorHr);
            }
            return nullptr;
        }

        wrl::ComPtr<wst::IStorageFile> file;
        LogThrow_IfFailed(getFileAsyncOp->GetResults(&file));
        return file;
    }

    void Utilities::ReadFileBuffer(
        _In_ const wrl::ComPtr<wst::IStorageFile>& spFile,
        _Outptr_ wsts::IBuffer** ppBuffer
        )
    {
        wrl::ComPtr<wst::IFileIOStatics> spFileIOStatics;
        wrl::ComPtr<wf::IAsyncOperation<wsts::IBuffer*>> spReadBufferAsyncOp;
        auto spAsyncOperationCompleteEvent = std::make_shared<Event>(L"ReadFileCompleted");
        wf::AsyncStatus asyncStatus = wf::AsyncStatus::Started;

        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Windows_Storage_FileIO).Get(),
            &spFileIOStatics
            ));

        LogThrow_IfFailed(spFileIOStatics->ReadBufferAsync(spFile.Get(), &spReadBufferAsyncOp));
        LogThrow_IfFailed(spReadBufferAsyncOp->put_Completed(
            MakeAgileCallback<wf::IAsyncOperationCompletedHandler<wsts::IBuffer*>>(
                [spAsyncOperationCompleteEvent, &asyncStatus] (wf::IAsyncOperation<wsts::IBuffer*>*, wf::AsyncStatus status) -> HRESULT
                {
                    asyncStatus = status;
                    spAsyncOperationCompleteEvent->Set();
                    return S_OK;
                }
                ).Get()
            ));
        spAsyncOperationCompleteEvent->WaitForDefault();
        if (asyncStatus == wf::AsyncStatus::Completed)
        {
            LogThrow_IfFailed(spReadBufferAsyncOp->GetResults(ppBuffer));
        }
        else
        {
            wrl::ComPtr<wf::IAsyncInfo> spAsyncInfo;
            HRESULT errorHr = S_OK;

            LogThrow_IfFailed(spReadBufferAsyncOp.As(&spAsyncInfo));
            LogThrow_IfFailed(spAsyncInfo->get_ErrorCode(&errorHr));
            LogThrow_IfFailed(errorHr);
        }
    }

    void Utilities::ReadFileLines(
        _In_ const wrl::ComPtr<wst::IStorageFile>& file,
        _Outptr_ wfc::IVector<HSTRING>** lines
        )
    {
        wrl::ComPtr<wst::IFileIOStatics> fileIOStatics;
        wrl::ComPtr<wf::IAsyncOperation<wfc::IVector<HSTRING>*>> asyncOp;
        auto completeEvent = std::make_shared<Event>(L"ReadFileLinesCompleted");
        auto asyncStatus = wf::AsyncStatus::Started;

        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Windows_Storage_FileIO).Get(),
            &fileIOStatics
            ));

        LogThrow_IfFailed(fileIOStatics->ReadLinesAsync(file.Get(), &asyncOp));
        LogThrow_IfFailed(asyncOp->put_Completed(
            MakeAgileCallback<wf::IAsyncOperationCompletedHandler<wfc::IVector<HSTRING>*>>(
                [completeEvent, &asyncStatus](wf::IAsyncOperation<wfc::IVector<HSTRING>*>*, wf::AsyncStatus status) -> HRESULT
                {
                    asyncStatus = status;
                    completeEvent->Set();
                    return S_OK;
                }).Get()
            ));
        completeEvent->WaitForDefault();

        if (asyncStatus == wf::AsyncStatus::Completed)
        {
            LogThrow_IfFailed(asyncOp->GetResults(lines));
        }
        else
        {
            wrl::ComPtr<wf::IAsyncInfo> asyncInfo;
            HRESULT errorHr = S_OK;

            LogThrow_IfFailed(asyncOp.As(&asyncInfo));
            LogThrow_IfFailed(asyncInfo->get_ErrorCode(&errorHr));
            LogThrow_IfFailed(errorHr);
        }
    }

    wrl::ComPtr<wst::IStorageFile> Utilities::GenerateOutputFile(
        _In_ HSTRING filename,
        _In_ const wrl::ComPtr<wst::IStorageFolder>& spFolder
        )
    {
        wrl::ComPtr<wst::IStorageFile> spOutputFile;
        auto spAsyncOperationCompleteEvent = std::make_shared<Event>(L"GenerateOutputCompleted");
        wf::AsyncStatus asyncStatus = wf::AsyncStatus::Started;
        wrl::ComPtr<wst::IStorageItem> spFolderAsStorageItem;
        wrl::Wrappers::HString strOutputFolderPath;
        wrl::ComPtr<wf::IAsyncOperation<wst::StorageFile*>> spCreateFileOperation;


        LogThrow_IfFailed(spFolder.As(&spFolderAsStorageItem));
        LogThrow_IfFailed(spFolderAsStorageItem->get_Path(strOutputFolderPath.GetAddressOf()));
        LOG_OUTPUT(L"Writing out %s to %s.", GetRawBuffer(filename), strOutputFolderPath.GetRawBuffer(nullptr));

        LogThrow_IfFailed(spFolder->CreateFileAsync(
            filename,
            wst::CreationCollisionOption_ReplaceExisting,
            &spCreateFileOperation
            ));

        LogThrow_IfFailed(spCreateFileOperation->put_Completed(
            MakeAgileCallback<wf::IAsyncOperationCompletedHandler<wst::StorageFile*>>(
                [spAsyncOperationCompleteEvent, &asyncStatus] (wf::IAsyncOperation<wst::StorageFile*>*, wf::AsyncStatus status) -> HRESULT
                {
                    asyncStatus = status;
                    spAsyncOperationCompleteEvent->Set();
                    return S_OK;
                }
                ).Get()
            ));
        spAsyncOperationCompleteEvent->WaitForDefault();
        if (asyncStatus == wf::AsyncStatus::Completed)
        {
            LogThrow_IfFailed(spCreateFileOperation->GetResults(&spOutputFile));
        }
        else
        {
            wrl::ComPtr<wf::IAsyncInfo> spAsyncInfo;
            HRESULT errorHr = S_OK;

            LogThrow_IfFailed(spCreateFileOperation.As(&spAsyncInfo));
            LogThrow_IfFailed(spAsyncInfo->get_ErrorCode(&errorHr));
            LogThrow_IfFailed(errorHr);
        }

        return spOutputFile;
    }

    void Utilities::EnsureFileWriteCompleted(
        _In_ wrl::ComPtr<wf::IAsyncAction> spWriteBufferAsyncAction)
    {
        auto spAsyncOperationCompleteEvent = std::make_shared<Event>(L"FileWriteCompleted");
        wf::AsyncStatus asyncStatus = wf::AsyncStatus::Started;

        LogThrow_IfFailed(spWriteBufferAsyncAction->put_Completed(
            MakeAgileCallback<wf::IAsyncActionCompletedHandler>(
                [spAsyncOperationCompleteEvent, &asyncStatus] (wf::IAsyncAction*, wf::AsyncStatus status) -> HRESULT
                {
                    asyncStatus = status;
                    spAsyncOperationCompleteEvent->Set();
                    return S_OK;
                }
                ).Get()
            ));
        spAsyncOperationCompleteEvent->WaitForDefault();
        if (asyncStatus != wf::AsyncStatus::Completed)
        {
            wrl::ComPtr<wf::IAsyncInfo> spAsyncInfo;
            HRESULT errorHr = S_OK;

            LogThrow_IfFailed(spWriteBufferAsyncAction.As(&spAsyncInfo));
            LogThrow_IfFailed(spAsyncInfo->get_ErrorCode(&errorHr));
            LogThrow_IfFailed(errorHr);
        }
    }

    void Utilities::WriteFileString(
        _In_ HSTRING filename,
        _In_ const wrl::ComPtr<wst::IStorageFolder>& spFolder,
        _In_ HSTRING inString)
    {
        wrl::ComPtr<wst::IStorageFile> spOutputFile;

        spOutputFile = GenerateOutputFile(filename, spFolder);

        wrl::ComPtr<wst::IFileIOStatics> spFileIOStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Windows_Storage_FileIO).Get(),
            &spFileIOStatics
            ));

        wrl::ComPtr<wf::IAsyncAction> spWriteBufferAsyncAction;
        LogThrow_IfFailed(spFileIOStatics->WriteTextAsync(spOutputFile.Get(), inString, &spWriteBufferAsyncAction));

        EnsureFileWriteCompleted(spWriteBufferAsyncAction);
    }

    void Utilities::WriteFileBuffer(
        _In_ HSTRING filename,
        _In_ const wrl::ComPtr<wst::IStorageFolder>& spFolder,
        _In_ const wrl::ComPtr<wsts::IBuffer>& spBuffer)
    {
        wrl::ComPtr<wst::IStorageFile> spOutputFile;

        spOutputFile = GenerateOutputFile(filename, spFolder);

        wrl::ComPtr<wst::IFileIOStatics> spFileIOStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Windows_Storage_FileIO).Get(),
            &spFileIOStatics
            ));

        wrl::ComPtr<wf::IAsyncAction> spWriteBufferAsyncAction;
        LogThrow_IfFailed(spFileIOStatics->WriteBufferAsync(spOutputFile.Get(), spBuffer.Get(), &spWriteBufferAsyncAction));

        EnsureFileWriteCompleted(spWriteBufferAsyncAction);
    }

    wrl::ComPtr<wst::IStorageFile> Utilities::CopyFile(
        _In_ const wrl::ComPtr<wst::IStorageFile>& spSourceFile,
        _In_ HSTRING destinationFilename,
        _In_ const wrl::ComPtr<wst::IStorageFolder>& spDestinationFolder)
    {
        wrl::ComPtr<wf::IAsyncOperation<wst::StorageFile*>> spCopyAsyncOp;
        auto spAsyncOperationCompleteEvent = std::make_shared<Event>(L"CopyFileCompleted");
        wf::AsyncStatus asyncStatus = wf::AsyncStatus::Started;

        LogThrow_IfFailed(spSourceFile->CopyOverload(
            spDestinationFolder.Get(),
            destinationFilename,
            wst::NameCollisionOption_ReplaceExisting,
            &spCopyAsyncOp));

        LogThrow_IfFailed(spCopyAsyncOp->put_Completed(
            MakeAgileCallback<wf::IAsyncOperationCompletedHandler<wst::StorageFile*>>(
                [spAsyncOperationCompleteEvent, &asyncStatus](wf::IAsyncOperation<wst::StorageFile*>*, wf::AsyncStatus status) -> HRESULT
                {
                    asyncStatus = status;
                    spAsyncOperationCompleteEvent->Set();
                    return S_OK;
                }).Get()
            ));

        spAsyncOperationCompleteEvent->WaitForDefault();

        if (asyncStatus != wf::AsyncStatus::Completed)
        {
            wrl::ComPtr<wf::IAsyncInfo> spAsyncInfo;
            HRESULT errorHr = S_OK;

            LogThrow_IfFailed(spCopyAsyncOp.As(&spAsyncInfo));
            LogThrow_IfFailed(spAsyncInfo->get_ErrorCode(&errorHr));
            LogThrow_IfFailed(errorHr);
            return nullptr;
        }

        wrl::ComPtr<wst::IStorageFile> file;
        LogThrow_IfFailed(spCopyAsyncOp->GetResults(&file));
        return file;
    }

    wrl::Wrappers::HString Utilities::GetFormattedTestName(_In_opt_ HSTRING variation, _In_ HSTRING extension)
    {
        return FileNameGenerator::GetFileName(GenerateFileNameHelper(variation, extension).ToFileNameInfo());
    }

    wrl::Wrappers::HString Utilities::GetMastersFolderPath() const
    {
        wrl::Wrappers::HString strMastersFolderPath;

        auto deploymentDir = GetTestDeploymentDir();
        deploymentDir.Append(L"resources\\masters\\");
        deploymentDir = deploymentDir.ToLower();

        LogThrow_IfFailed(strMastersFolderPath.Set(deploymentDir.GetBuffer()));
        return strMastersFolderPath;
    }

    wrl::Wrappers::HString Utilities::GetTempFolderPath()
    {
        wrl::Wrappers::HString strTempFolderPath;

        //On Onecore in absence of domain accounts, GetTempPath returns a global temp location
        //which isn't accessible to the test running inside an AppContainer so we explicitly
        //request for the App's temp folder
        BOOLEAN isOneCore = false;
        LogThrow_IfFailed(Utilities::IsOneCoreStatic(&isOneCore));
        if (isOneCore)
        {
            wrl::ComPtr<wst::IApplicationDataStatics> spAppData;
            wrl::ComPtr<wst::IApplicationData> spCurrentAppData;
            wrl::ComPtr<wst::IStorageFolder> spTempFolder;
            wrl::ComPtr<wst::IStorageItem> spFolderAsStorageItem;

            LogThrow_IfFailed(wf::GetActivationFactory(
                wrl::Wrappers::HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(),
                &spAppData
                ));

            LogThrow_IfFailed(spAppData->get_Current(&spCurrentAppData));
            LogThrow_IfFailed(spCurrentAppData->get_TemporaryFolder(&spTempFolder));
            LogThrow_IfFailed(spTempFolder.As(&spFolderAsStorageItem));
            LogThrow_IfFailed(spFolderAsStorageItem->get_Path(strTempFolderPath.GetAddressOf()));
            //Append a backslash at the end, GetTempPath below does that for other platforms
            strTempFolderPath = Concat(strTempFolderPath, L"\\");
        }
        else
        {
            // GetTempPath returns MAX_PATH+1, plus a terminating null.
            wchar_t tempFolder[MAX_PATH + 2] = {};

            DWORD length = GetTempPath(
                ARRAYSIZE(tempFolder),
                tempFolder
                );
            if (length == 0)
            {
                LogThrow_IfFailed(GetLastError());
            }

            strTempFolderPath.Set(tempFolder);
        }

        return strTempFolderPath;
    }

    wrl::ComPtr<mdc::IMockDCompDevice> Utilities::GetMockDCompDevice()
    {
        wrl::ComPtr<test_infra::ITestServicesStatics> spTestServicesStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
            &spTestServicesStatics
            ));

        wrl::ComPtr<test_infra::IWindowHelper> spWindowHelper;
        LogThrow_IfFailed(spTestServicesStatics->get_WindowHelper(&spWindowHelper));

        wrl::ComPtr<mdc::IMockDCompDevice> spMockDCompDevice;
        LogThrow_IfFailed(spWindowHelper->get_MockDCompDevice(&spMockDCompDevice));

        return spMockDCompDevice;
    }

    wrl::ComPtr<mdc::IMockDCompDevice2> Utilities::GetMockDCompDevice2()
    {
        auto device1 = GetMockDCompDevice();
        wrl::ComPtr<mdc::IMockDCompDevice2> result;
        LogThrow_IfFailed(device1.As(&result));
        return result;
    }

    wrl::Wrappers::HString Utilities::GetOutputFolderPath()
    {
        wrl::Wrappers::HString strTempFolderPath;

        wrl::ComPtr<wst::IStorageFolder> spOutputFolder;
        Utilities::EnsureOutputFolder(&spOutputFolder);

        wrl::ComPtr<wst::IStorageItem> pItem;
        LogThrow_IfFailed(spOutputFolder.As(&pItem));
        LogThrow_IfFailed(pItem->get_Path(strTempFolderPath.ReleaseAndGetAddressOf()));

        return strTempFolderPath;
    }

    void Utilities::CraterJupiter(const CraterJupiterErrorCode& craterJupiterErrorCode)
    {
        auto exitCode = craterJupiterErrorCode.Error.Code;
        if (exitCode!=0)
        {
            const auto errorString = WEX::Common::NoThrowString()
                .Format(
                    L"Test cleanup error code %d has been detected. Cratering Jupiter.",
                    exitCode);
            WEX::Logging::Log::Trace(
                WEX::Logging::ErrorTrace(errorString)
                .WithAttachment(
                    WEX::Logging::LogTraceAttachment::ScreenCapture));
            ::ExitProcess(exitCode);
        }
    }

    bool Utilities::TryCaptureDesktopScreenshot(_In_opt_ const wchar_t* variation)
    {
        WEX::Common::String fullTestName;
        fullTestName = GetOutputFolderPath().GetRawBuffer(nullptr);
        fullTestName.Append(L"\\");
        fullTestName = fullTestName + GetFormattedTestName(nullptr, nullptr).GetRawBuffer(nullptr);
        if(variation)
        {
            fullTestName.Append(L"_");
            fullTestName.Append(variation);
        }
        fullTestName.Append(L".png");

        LOG_OUTPUT(L"Capturing screenshot to %s...", fullTestName.GetBuffer());
        try
        {
            return SUCCEEDED(RpcCaptureScreenshot(fullTestName));
        }
        catch (const WEX::Common::Exception&)
        {
            return false;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    HRESULT Utilities::TerminateProcess(_In_ HSTRING processName)
    {
        UINT32 length = 0;
        return RpcTerminateProcess(WindowsGetStringRawBuffer(processName, &length));
    }

    bool Utilities::s_isBVT = false;

    bool Utilities::IsBVT()
    {
        return s_isBVT;
    }

    void Utilities::RunAsBVT()
    {
        if(IsBVT()) return;

        s_isBVT = true;

        RpcClientEnsureConnected();
        LogThrow_IfFailed(RpcRunAsBVT());
    }

    void Utilities::CheckForBVTMode()
    {
        if(IsBVT()) return;

        String value;
        if (SUCCEEDED(RuntimeParameters::TryGetValue(L"RunAsBVT", value)))
        {
            if (!value.IsEmpty())
            {
                LOG_OUTPUT(L"Activating BVT mode in client process");
                RunAsBVT();
            }
        }
    }

    HRESULT Utilities::CreateRenderingScopeGuard(
            _In_ test_infra::DCompRendering rendering,
            _In_ BOOLEAN resizeWindow,
            _In_ BOOLEAN injectMockDComp,
            _In_ BOOLEAN resetDevice,
            _In_ BOOLEAN resetWindowContent,
            _Outptr_ wf::IClosable** ppGuard)
    {
        Microsoft::WRL::ComPtr<RenderingScopeGuard> spGuard = Microsoft::WRL::Make<RenderingScopeGuard>(
            rendering,
            resizeWindow,
            injectMockDComp,
            resetDevice,
            resetWindowContent);
        Microsoft::WRL::ComPtr<wf::IClosable> spClosable;
        LogThrow_IfFailed(spGuard.As(&spClosable));
        LogThrow_IfFailed(spClosable.CopyTo(ppGuard));
        return S_OK;
    }

    HRESULT Utilities::SetDpi(
        _In_ wgr::DisplayAdapterId displayAdapterId,
        _In_ UINT displayAdapterTargetId,
        _In_ test_infra::DisplayDPIRange dpi,
        _Outptr_ wf::IClosable** ppRestoreDpi)
    {
        COM_START
        {
            LUID luid { displayAdapterId.LowPart, displayAdapterId.HighPart };
            Microsoft::WRL::ComPtr<DPISetter> dpiSetter = Microsoft::WRL::Make<DPISetter>(luid, displayAdapterTargetId, dpi);

            LogThrow_IfFailed(dpiSetter->Init());

            if (dpiSetter->DpiHasChanged())
            {
                Microsoft::WRL::ComPtr<wf::IClosable> closable;
                LogThrow_IfFailed(dpiSetter.As(&closable));
                LogThrow_IfFailed(closable.CopyTo(ppRestoreDpi));
            }
            else
            {
                // The DPI did not change, return nothing as the restorer will do nothing.
                *ppRestoreDpi = nullptr;
            }
        }
        COM_END
    }

    HRESULT Utilities::CaptureScreenAndProcess(_In_ HSTRING variation)
    {
        CaptureProcessDump();
        CaptureScreenshot(variation);
        return S_OK;
    }

    HRESULT Utilities::CaptureScreen(_In_ HSTRING variation)
    {
        CaptureScreenshot(variation);
        return S_OK;
    }
    // Set this constant to determine whether composition object leak detection is enabled in this run.
    // Eventually this should be controllable as argument to the test runner or reg key
    const bool Utilities::sc_enableCompLeakDetection = false;

    bool Utilities::IsCompLeakDetectionEnabled()
    {
        return sc_enableCompLeakDetection;
    }

    // Comp Leak detection disables MockDComp during the actual test runs, but enables it in test cleanup
    // so that we can recreate the device and trigger the DComp device final release asserter
    // Track that state here. Only relevant if sc_enableCompLeakDetection is true.
    bool Utilities::s_isMockDCompDisabledForCompLeakDetection = Utilities::sc_enableCompLeakDetection;

    void Utilities::SetIsMockDCompDisabledForCompLeakDetection(bool value)
    {
        s_isMockDCompDisabledForCompLeakDetection = value;
    }

    bool Utilities::GetIsMockDCompDisabledForCompLeakDetection()
    {
        return s_isMockDCompDisabledForCompLeakDetection;
    }

    HRESULT Utilities::get_IsMockDCompDisabled(_Out_ BOOLEAN* pIsMockDCompDisabled)
    {
        COM_START
        {
            LogThrow_IfFailed(Utilities::IsMockDCompDisabled(pIsMockDCompDisabled));
        }
        COM_END
    }

    HRESULT Utilities::SetMockUIAClientsListening()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->SetMockUIAClientsListening();
            });
        }
        COM_END
    }

    HRESULT Utilities::ClearMockUIAClientsListening()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->ClearMockUIAClientsListening();
            });
        }
        COM_END
    }

    HRESULT Utilities::SetGenericXamlFilePathForMUX(_In_ HSTRING filePath)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->SetGenericXamlFilePathForMUX(filePath);
            });
        }
        COM_END
    }

    HRESULT Utilities::RestoreDefaultFlipViewScrollWheelDelay()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->RestoreDefaultFlipViewScrollWheelDelay();
            });
        }
        COM_END
    }

    HRESULT Utilities::SetFlipViewScrollWheelDelay(_In_ int scrollWheelDelayMS)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->SetFlipViewScrollWheelDelay(scrollWheelDelayMS);
            });
        }
        COM_END
    }

    HRESULT Utilities::SetMockEnergySaverStatus(_In_ wsyp::EnergySaverStatus energyStatus)
    {
        COM_START
        {
            EnsureMockPlatformManager();
            m_mockPowerManager->put_EnergySaverStatus(energyStatus);
        }
        COM_END
    }

    HRESULT Utilities::SetMockPowerSupplyStatus(_In_ wsyp::PowerSupplyStatus powerStatus)
    {
        COM_START
        {
            EnsureMockPlatformManager();
            m_mockPowerManager->put_PowerSupplyStatus(powerStatus);
        }
        COM_END
    }

    HRESULT Utilities::SetMockBatteryStatus(_In_ wsyp::BatteryStatus batteryStatus)
    {
        COM_START
        {
            EnsureMockPlatformManager();
            m_mockPowerManager->put_BatteryStatus(batteryStatus);
        }
        COM_END
    }

    HRESULT Utilities::SetMockRemainingChargePercent(_In_ int value)
    {

        COM_START
        {
            EnsureMockPlatformManager();
            m_mockPowerManager->put_RemainingChargePercent(value);
        }
        COM_END

    }
    HRESULT Utilities::SetMockRemainingDischargeTime(_In_ wf::TimeSpan value)
    {
        COM_START
        {
            EnsureMockPlatformManager();
            m_mockPowerManager->put_RemainingDischargeTime(value);
        }
        COM_END
    }

    HRESULT Utilities::SetApplicationLanguageOverride(_In_ HSTRING languageName)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->SetApplicationLanguageOverride(languageName);
            });
        }
        COM_END
    }

    HRESULT Utilities::ClearApplicationLanguageOverride()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->ClearApplicationLanguageOverride();
            });
        }
        COM_END
    }

    void Utilities::EnsureMockPlatformManager()
    {
        if (!m_mockPowerManager)
        {
            m_mockPowerManager = wrl::Make<MockPowerManager>();
            THROW_IF_NULL_ALLOC(m_mockPowerManager);

            /* MOCK10_REMOVAL
            // typedef MockFunction<HRESULT(HSTRING className, const IID& iid, void** factory)>::Prototype RoGetActivationFactoryPrototype;
            m_mockRoGetActivationFactory.reset(new MockFunction<RoGetActivationFactoryPrototype>(
                RoGetActivationFactory,
                [&](HSTRING className, const IID& iid, void** factory) -> HRESULT
            {
                bool result = wrl::Wrappers::HStringReference(RuntimeClass_Windows_System_Power_PowerManager) == className;
                if (result)
                {
                    return m_mockPowerManager.CopyTo(iid, factory);
                }
                else
                {
                    return RoGetActivationFactory(className, iid, factory);
                }
            }));
            */
        }
    }

    HRESULT Utilities::SimulateDeviceLostOnMetadataParse()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> testHooks = WindowHelper::GetTestHooks();
                testHooks->SimulateDeviceLostOnMetadataParse();
            });
        }
        COM_END
    }

    HRESULT Utilities::SimulateDeviceLostOnCreatingSvgDecoder()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> testHooks = WindowHelper::GetTestHooks();
                testHooks->SimulateDeviceLostOnCreatingSvgDecoder();
            });
        }
        COM_END
    }

    HRESULT Utilities::SetForceDebugSettingsTracingEvents(_In_ BOOLEAN value)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> testHooks = WindowHelper::GetTestHooks();
                testHooks->SetForceDebugSettingsTracingEvents(!!value);
            });
        }
        COM_END
    }
} } // namespace Private::Infrastructure

