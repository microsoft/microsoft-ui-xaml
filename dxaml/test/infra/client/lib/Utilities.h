// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RpcClient.h"

#include <Objbase.h> // CoCreateInstance
#include "MockPowerManager.h"

using namespace Microsoft::WRL;

namespace RuntimeFeatureBehavior
{
    enum class RuntimeEnabledFeature;
}

namespace MockDComp
{
    struct FileNameInfo;
}

namespace Private { namespace Infrastructure {

    struct VerificationComparer;

    const size_t NumberOfTestPoolFilters = 8;

    struct CraterJupiterErrorCode
    {
        union ErrorCodeAndErrorFlags {
            DWORD Code;
            struct ErrorFlags {
                bool    Unused                          : 1;
                bool    InvalidOrientation              : 1;
                bool    FinalReleaseQueueNotEmpty       : 1;
                bool    PopupOpen                       : 1;
                bool    NoLongerForeground              : 1;
                bool    CustomSystemFontCollectionInUse : 1;
                bool    UsingNonDefaultFontScale        : 1;
                bool    PhysicalLocationOutsideBounds   : 1;
                int16_t TestPoolDirty                   : NumberOfTestPoolFilters;
            } Flags;
        } Error;
    };

    class Utilities : public Microsoft::WRL::RuntimeClass<test_infra::IUtilities>
    {
        InspectableClass(RuntimeClass_Private_Infrastructure_Utilities, TrustLevel::BaseTrust);

    public:
        IFACEMETHOD(get_IsOneCore)(_Out_ BOOLEAN* pIsOneCore) override;
        IFACEMETHOD(get_IsXBox)(_Out_ BOOLEAN* pIsXBox) override;
        IFACEMETHOD(get_IsDesktop)(_Out_ BOOLEAN* pIsDesktop) override;
        IFACEMETHOD(SetRegKey)(_In_ HSTRING path, _In_ HSTRING name, _In_ DWORD dwValue, _Out_ BOOLEAN* nameExisted, _In_ BOOLEAN currentUser = false) override;
        IFACEMETHOD(DeleteRegKey)(_In_ HSTRING path, _In_ HSTRING name, _In_ BOOLEAN currentUser = false) override;
        IFACEMETHOD(EnableChangingTimeZone)(_In_ BOOLEAN enable) override;
        IFACEMETHOD(SetTimeZone)(_In_ HSTRING timezoneId) override;
        IFACEMETHOD(RunCommandLineWithExitCode)(_In_ HSTRING commandLine, _Out_ DWORD* pExitCode) override;
        IFACEMETHOD(RunCommandLine)(_In_ HSTRING commandLine) override;
        IFACEMETHOD(TerminateProcess)(_In_ HSTRING processName) override;
        IFACEMETHOD(get_IsWPF)(_Out_ BOOLEAN* pIsWPF) override;

        IFACEMETHOD(VerifyMockDCompOutput)(
            mdc::SurfaceComparison surfaceComparison
            ) override;

        IFACEMETHOD(VerifyMockDCompOutputWithOptions)(
            mdc::SurfaceComparison surfaceComparison,
            mdc::SaveToFileOptions xmlOptions
            ) override;

        IFACEMETHOD(VerifyMockDCompOutputWithAnimation)(
            mdc::AnimationComparison animationComparison,
            _In_ HSTRING variation
            ) override;

        IFACEMETHOD(VerifyMockDCompOutputForVariation)(
            mdc::SurfaceComparison surfaceComparison,
            _In_ HSTRING variation
            ) override;

        IFACEMETHOD(VerifyMockDCompOutputForVariationWithOptions)(
            mdc::SurfaceComparison surfaceComparison,
            _In_ HSTRING variation,
            mdc::SaveToFileOptions xmlOptions
            ) override;

        // Verify MockDComp output. Optionally capture a process dump if the
        // verification fails. Please use with care since dumps take up space.
        IFACEMETHOD(VerifyMockDCompOutputForVariationWithDataCollectionOnFailure)(
            mdc::AnimationComparison animationComparison,
            mdc::SurfaceComparison surfaceComparison,
            _In_ HSTRING variation,
            _In_ BOOLEAN captureProcessDumpOnFailure
            ) override;

        IFACEMETHOD(VerifyMockDCompOutputForVariationWithSucceededFlag)(
            mdc::SurfaceComparison surfaceComparison,
            _In_ HSTRING variation,
            BOOLEAN ignoreVerification,
            _Out_ BOOLEAN* succeeded
            ) override;

        // Just save the mock DComp output without verification. Useful in cases
        // where you would like the extra data to debug
        IFACEMETHOD(SaveMockDCompOutput)(
            mdc::AnimationComparison animationComparison,
            mdc::SurfaceComparison surfaceComparison,
            _In_ HSTRING variation
            ) override;

        IFACEMETHOD(SetMockDCompSwapChainIdentifier)(_In_ unsigned __int64 identifierValue, _In_ unsigned int identifierID) override;

        // Sets the mode used for surface ID's:
        // AllocationOrder - Order that the surfaces are allocated in
        // XmlOrder - Order that they appear in the MockDComp XML output
        // CRC - Uses the image content to generate a CRC for the ID
        IFACEMETHOD(SetMockDCompSurfaceIdMode)(_In_ mdc::SurfaceIdMode mode);

        IFACEMETHOD(ResetMockDCompSurfaceId)();

        IFACEMETHOD(SimulateAllSurfacesDiscarded)();

        IFACEMETHOD(SetReturnDeviceLostWhenCreatingSurfaces)(_In_ BOOLEAN returnDeviceLostWhenCreatingSurfaces) override;

        IFACEMETHOD(ResetMockDCompGraphicsDevice)();
        IFACEMETHOD(SimulateDeviceLostOnOffThreadImageUpload)();
        IFACEMETHOD(SimulateSwallowedDeviceLostOnStartup)();
        IFACEMETHOD(SimulateDeviceLostOnMetadataParse)();
        IFACEMETHOD(SimulateDeviceLostOnCreatingSvgDecoder)();

        IFACEMETHOD(SetDCompDeviceLeakDetectionEnabled)(_In_ BOOLEAN enableLeakDetection) override;

        IFACEMETHOD(ResetWasCommitInvoked)() override;
        IFACEMETHOD(get_WasCommitInvoked)(_Out_ BOOLEAN* pWasCommitInvoked) override;

        IFACEMETHOD(VerifyAreSurfaceResourcesOffered)(_In_ BOOLEAN value) override;

        // Legacy UIElement tree validation.
        IFACEMETHOD(VerifyUIElementTree)(_Out_ BOOLEAN* result) override;
        IFACEMETHOD(VerifyUIElementTreeWithRoot)(_In_ xaml::IDependencyObject* root, _Out_ BOOLEAN* result) override;
        IFACEMETHOD(VerifyUIElementTreeForVariation)(_In_opt_ HSTRING variation, _Out_ BOOLEAN* result) override;

        // Rule-based UIElement tree validation.
        IFACEMETHOD(VerifyUIElementTreeWithRules)(_In_ HSTRING rulesFilename, _Out_ BOOLEAN* result) override;
        IFACEMETHOD(VerifyUIElementTreeForVariationWithRules)(_In_ HSTRING variation, _In_ HSTRING rulesFilename, _Out_ BOOLEAN* result) override;
        IFACEMETHOD(VerifyUIElementTreeWithRulesInline)(_In_ HSTRING inlineRulesXml, _Out_ BOOLEAN* result) override;
        IFACEMETHOD(VerifyUIElementTreeForVariationWithRulesInline)(_In_ HSTRING variation, _In_ HSTRING inlineRulesXml, _Out_ BOOLEAN* result) override;
        IFACEMETHOD(VerifyUIElementTreeForVariationWithRulesInlineWithIgnorePopupsOption)(_In_ HSTRING variation, _In_ HSTRING inlineRulesXml, BOOLEAN ignorePopups, _Out_ BOOLEAN* result) override;

        // Verifies that the UIElement tree only contains the colors that are specified in the given set.
        IFACEMETHOD(VerifyUIElementTreeContainsOnlyValidColors)(_In_ HSTRING variation, _In_ wfc::IVector<unsigned int>* validColors);

        IFACEMETHOD(ResetOptionalChanges)() override;
        IFACEMETHOD(SetRuntimeEnabledFeatureOverride)(_In_ INT feature, _In_ BOOLEAN enabled, _Out_opt_ BOOLEAN* wasPreviouslyEnabled = nullptr) override;
        IFACEMETHOD(ClearAllRuntimeEnabledFeatureOverrides)() override;
        IFACEMETHOD(OverrideTrimImageResourceDelay)(_In_ BOOLEAN enabled) override;

        IFACEMETHOD(VerifyOutputFileHighContrastColors)(_In_ HSTRING variation, _In_ test_infra::HighContrastTheme theme);

        IFACEMETHOD(RetrieveOpenBottomCommandBar)(_In_ xaml::IXamlRoot* xamlRoot, _Outptr_ xaml_controls::ICommandBar** cmdBar);

        IFACEMETHOD(ResetMetadata)();

        IFACEMETHOD(ClearDefaultLanguageString) ();

        IFACEMETHOD(IsWindowFocused)(_Out_ BOOLEAN* isFocused, _In_ xaml::IXamlRoot* xamlRoot);

        IFACEMETHOD(CreateLoopingSelector)(_Outptr_ IInspectable** ppLoopingSelector);

        IFACEMETHOD(InjectBackButtonPress)(_Out_ BOOLEAN* pHandled);

        IFACEMETHOD(ResetAtlasSizeHint)();

        IFACEMETHOD(ShrinkApplicationViewVisibleBounds)(_In_ BOOLEAN enabled);

        IFACEMETHOD(GetPopupOverlayElement)(_In_ xaml_primitives::IPopup* popup, _Outptr_ xaml::IFrameworkElement** overlayElement);

        IFACEMETHOD(DeletePlatformFamilyCache)();

        IFACEMETHOD(DeleteResourceDictionaryCaches)();

        IFACEMETHOD(CreateRenderingScopeGuard)(
            _In_ test_infra::DCompRendering rendering,
            _In_ BOOLEAN resizeWindow,
            _In_ BOOLEAN injectMockDComp,
            _In_ BOOLEAN resetDevice,
            _In_ BOOLEAN resetWindowContent,
            _Outptr_ wf::IClosable** ppGuard);

        IFACEMETHOD(CaptureScreenAndProcess)(_In_ HSTRING variation);
        IFACEMETHOD(CaptureScreen)(_In_ HSTRING variation);

        IFACEMETHOD(get_IsMockDCompDisabled)(_Out_ BOOLEAN* pIsMockDCompDisabled) override;

        IFACEMETHOD(SetMockUIAClientsListening)() override;
        IFACEMETHOD(ClearMockUIAClientsListening)() override;

        IFACEMETHOD(SetGenericXamlFilePathForMUX)(_In_ HSTRING filePath);

        IFACEMETHOD(RestoreDefaultFlipViewScrollWheelDelay)();
        IFACEMETHOD(SetFlipViewScrollWheelDelay)(_In_ int scrollWheelDelayMS);

        IFACEMETHOD(SetMockEnergySaverStatus)(_In_ wsyp::EnergySaverStatus energyStatus);
        IFACEMETHOD(SetMockPowerSupplyStatus)(_In_ wsyp::PowerSupplyStatus powerStatus);
        IFACEMETHOD(SetMockBatteryStatus)(_In_ wsyp::BatteryStatus batteryStatus);
        IFACEMETHOD(SetMockRemainingChargePercent)(_In_ int value);
        IFACEMETHOD(SetMockRemainingDischargeTime)(_In_ wf::TimeSpan value);

        IFACEMETHOD(SetApplicationLanguageOverride)(_In_ HSTRING languageName) override;
        IFACEMETHOD(ClearApplicationLanguageOverride)() override;
        IFACEMETHOD(SetDpi)(
            _In_ wgr::DisplayAdapterId displayAdapterId,
            _In_ UINT displayAdapterTargetId,
            _In_ test_infra::DisplayDPIRange dpi,
            _Outptr_ wf::IClosable** ppRestoreDpi) override;

        IFACEMETHOD(SetForceDebugSettingsTracingEvents)(_In_ BOOLEAN value) override;

        //Temporary: We use a reg key set by the tests to detect if we're running on OneCore
        //It only returns true if the Reg Key is set
        static _Check_return_ HRESULT IsOneCoreStatic(_Out_ BOOLEAN* pIsOneCore);

        static _Check_return_ HRESULT IsMockDCompDisabled(_Out_ BOOLEAN* pIsMockDCompDisabled);

        static _Check_return_ HRESULT IsXBoxStatic(_Out_ BOOLEAN* pIsXBox);

        static _Check_return_ HRESULT IsDesktopStatic(_Out_ BOOLEAN* pIsDesktop);

        static _Check_return_ HRESULT IsWPFStatic(_Out_ BOOLEAN* pIsWPF);

        static void CraterJupiter(const CraterJupiterErrorCode& errorCode);
        static bool TryCaptureDesktopScreenshot(_In_opt_ const wchar_t* variation);

        static wrl::ComPtr<wst::IStorageFile> GetStorageFile(
            _In_ HSTRING filenameWithPath);

        static void EnsureOutputFolder(
            _Outptr_ wst::IStorageFolder **ppOutputFolder);

        static void ReadFileBuffer(
            _In_ const wrl::ComPtr<wst::IStorageFile>& spFile,
            _Outptr_ wsts::IBuffer** ppBuffer);

        static void ReadFileLines(
            _In_ const wrl::ComPtr<wst::IStorageFile>& file,
            _Outptr_ wfc::IVector<HSTRING>** lines);

        static void WriteFileBuffer(
            _In_ HSTRING filename,
            _In_ const wrl::ComPtr<wst::IStorageFolder>& spFolder,
            _In_ const wrl::ComPtr<wsts::IBuffer>& spBuffer);

        static void WriteFileString(
            _In_ HSTRING filename,
            _In_ const wrl::ComPtr<wst::IStorageFolder>& spFolder,
            _In_ HSTRING inString);

        static wrl::ComPtr<wst::IStorageFile> CopyFile(
            _In_ const wrl::ComPtr<wst::IStorageFile>& spSourceFile,
            _In_ HSTRING destinationFilename,
            _In_ const wrl::ComPtr<wst::IStorageFolder>& spDestinationFolder);

        static bool IsBVT();

        static void RunAsBVT();

        static void CheckForBVTMode();

        static bool Utilities::IsCompLeakDetectionEnabled();
        static void Utilities::SetIsMockDCompDisabledForCompLeakDetection(bool value);
        static bool Utilities::GetIsMockDCompDisabledForCompLeakDetection();

    private:
        // Compares the output file generated by 'generatedOutputFileFunc' with a baseline file. The baseline file will be
        // looked up based on the name of the current test. If the comparison fails, both files are saved to the Pictures
        // library. If the baseline file is not found, the comparison automatically fails.
        bool DoVerification(
            _In_ PCWSTR fileExtension,
            _In_opt_ HSTRING variation,
            _In_ VerificationComparer& comparer,
            std::function<void(const mdc::FileNameInfo&)> outputFileGenerator);

        void VerifySuccess(bool didOutputMatchMaster);
        void CaptureFailureOutput(bool didOutputMatchMaster, bool captureProcessDumpOnFailure, bool captureScreenshotOnFailure, _In_opt_ HSTRING variation);
        void CaptureProcessDump();
        void CaptureScreenshot(_In_opt_ HSTRING variation);

        HRESULT VerifyMockDCompOutputHelper(
            _In_ mdc::AnimationComparison animationComparison,
            _In_ mdc::SurfaceComparison surfaceComparison,
            _In_ HSTRING variation,
            _In_ mdc::SaveToFileOptions xmlOptions = mdc::SaveToFileOptions_None,
            _In_ BOOLEAN captureProcessDumpOnFailure = false,
            _In_ bool ignoreVerification = false,
            _Out_opt_ BOOLEAN* succeeded = nullptr);

        // Returns the expected file names for the current test. The name will have "Microsoft::UI::Xaml::Tests::" removed,
        // and will have "::" and "." delimiters replaced with underscores.
        HRESULT VerifyUIElementTreeHelper(
            bool useXmlValidation,
            _In_opt_ HSTRING variation,
            _In_opt_ HSTRING rulesFilename,
            _In_opt_ HSTRING inlineRulesXml,
            bool ignorePopups = false,
            _In_opt_ xaml::IDependencyObject* root = nullptr,
            _Out_opt_ BOOLEAN* result = nullptr);

        // Verifies that all of the colors in the output dump file are valid according to the verifyColor function passed as an argument.
        // The verifyColor function should return true for all colors that it considers valid.
        HRESULT VerifyOutputFileColors(_In_ HSTRING variation, _In_ std::function<bool(unsigned)> verifyColor, _Out_ BOOLEAN* result);

        wrl::Wrappers::HString GetMastersFolderPath() const;
        static wrl::Wrappers::HString GetTempFolderPath();
        static wrl::Wrappers::HString GetOutputFolderPath();
        static wrl::Wrappers::HString GetFormattedTestName(_In_opt_ HSTRING variation, _In_ HSTRING extension);


        static wrl::ComPtr<wst::IStorageFile> GenerateOutputFile(
            _In_ HSTRING filename,
            _In_ const wrl::ComPtr<wst::IStorageFolder>& spFolder);

        static void EnsureFileWriteCompleted(
            _In_ wrl::ComPtr<wf::IAsyncAction> spWriteBufferAsyncAction);

        void StalingOutFile(
            _In_ HSTRING fileExtension,
            _In_opt_ HSTRING variation);

        static bool SetRuntimeFeatureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature feature, bool isEnabled);

        static wrl::ComPtr<mdc::IMockDCompDevice> GetMockDCompDevice();
        static wrl::ComPtr<mdc::IMockDCompDevice2> GetMockDCompDevice2();

        void EnsureMockPlatformManager();
        wrl::ComPtr<MockPowerManager> m_mockPowerManager;
        // std::unique_ptr<MockFunction<RoGetActivationFactoryPrototype> > m_mockRoGetActivationFactory;  // MOCK10_REMOVAL
        static bool s_isBVT;

        static const bool sc_enableCompLeakDetection;
        static bool s_isMockDCompDisabledForCompLeakDetection;
    }; // class Utilities

} } // namespace Private::Infrastructure
