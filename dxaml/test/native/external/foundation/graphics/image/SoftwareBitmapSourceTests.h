// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CustomWicBitmap.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics { namespace Image {

// Test cases for SoftwareBitmapSource and WriteableBitmap. Both provide ways for the app to provide pixels to be rendered, so the
// test cases are shared. In most test cases there's a boolean switch that determines whether we're using SoftwareBitmapSource or
// WriteableBitmap.
class SoftwareBitmapSourceTests : public WEX::TestClass<SoftwareBitmapSourceTests>
{
public:
    BEGIN_TEST_CLASS(SoftwareBitmapSourceTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"02f4717a-a120-494b-a28e-9a2cbd41ca58;bd1463b3-e5f2-4d54-9394-63a431c53a6e;d04573b8-e899-4822-bb72-9f4743c89d36")

#if defined(_WIN64) // Disable the test on 64-bit build because of floating point differences between x87 and sse instructions
        TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")
#endif

        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(LoadBasicSourceBadData)
        TEST_METHOD_PROPERTY(L"Description", L"Loads an image into the SoftwareBitmapSource with bad data that should trigger invalid arguments.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Close)
        TEST_METHOD_PROPERTY(L"Description", L"Tests explicitly calling Close on a SoftwareBitmapSource.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetBitmapAsyncNull)
        TEST_METHOD_PROPERTY(L"Description", L"Loads an image into the SoftwareBitmapSource/WriteableBitmap and then sets the bitmap to null after.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(SetBitmapAsyncNull2)
        TEST_METHOD_PROPERTY(L"Description", L"Loads an image into the SoftwareBitmapSource/WriteableBitmap and then sets the bitmap to null after.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void SetBitmapAsyncNull(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(LoadImageControl)
        TEST_METHOD_PROPERTY(L"Description", L"Loads an image into the SoftwareBitmapSource/WriteableBitmap asynchronously then attaches it to an Image control.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LoadImageControlAttachFirst)
        TEST_METHOD_PROPERTY(L"Description", L"Attaches the SoftwareBitmapSource/WriteableBitmap to an image control then asynchronously loads the image.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void LoadImageControlAttachFirst(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(NonTiledSegmentedCopy)
        TEST_METHOD_PROPERTY(L"Description", L"Use an image with more than 1MB of pixels and less width/height of 2045 to test iterative copy in the copy algorithm.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NonTiledSegmentedCopyWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Use an image with more than 1MB of pixels and less width/height of 2045 to test iterative copy in the copy algorithm (WUCFull variant).")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void NonTiledSegmentedCopyInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    BEGIN_TEST_METHOD(LoadLargeImage)
        TEST_METHOD_PROPERTY(L"Description", L"Loads a large image into the SoftwareBitmapSource/WriteableBitmap.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LoadLargeImageWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Loads a large image into the SoftwareBitmapSource/WriteableBitmap (WUCFull variant).")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void LoadLargeImageInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    BEGIN_TEST_METHOD(LoadTranslucentImage)
        TEST_METHOD_PROPERTY(L"Description", L"Loads a translucent image into the SoftwareBitmapSource/WriteableBitmap.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BitmapCache)
        TEST_METHOD_PROPERTY(L"Description", L"Loads a SoftwareBitmapSource/WriteableBitmap with an element using BitmapCache.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void BitmapCache(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(LoadImageBrush)
        TEST_METHOD_PROPERTY(L"Description", L"Loads an image into the SoftwareBitmapSource/WriteableBitmap asynchronously then attaches it to an Image brush.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void LoadImageBrush(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(LoadImageBrushAttachFirst)
        TEST_METHOD_PROPERTY(L"Description", L"Attaches the SoftwareBitmapSource/WriteableBitmap to an image brush then asynchronously loads the image.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void LoadImageBrushAttachFirst(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(LoadMultipleImageElements)
        TEST_METHOD_PROPERTY(L"Description", L"Attaches the SoftwareBitmapSource/WriteableBitmap to multiple image elements then asynchronously loads the image.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void LoadMultipleImageElements(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(LoadImageElementAndBrush)
        TEST_METHOD_PROPERTY(L"Description", L"Attaches the SoftwareBitmapSource/WriteableBitmap to an image element and image brush, then asynchronously loads the image.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void LoadImageElementAndBrush(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(LoadImageTwiceNoWait)
        TEST_METHOD_PROPERTY(L"Description", L"Loads an image into the SoftwareBitmapSource/WriteableBitmap without asynchronously waiting.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void LoadImageTwiceNoWait(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(LoadImageTwiceWithWait)
        TEST_METHOD_PROPERTY(L"Description", L"Loads an image into the SoftwareBitmapSource/WriteableBitmap with asynchronously waiting.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void LoadImageTwiceWithWait(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(LeaveTreeDataRestore)
        TEST_METHOD_PROPERTY(L"Description", L"Loads an image into the SoftwareBitmapSource/WriteableBitmap, the image then leaves the tree, has it's hardware data purged and restored when it re-enters the tree.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void LeaveTreeDataRestore(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(DeviceLost)
        TEST_METHOD_PROPERTY(L"Description", L"Loads an image into the SoftwareBitmapSource/WriteableBitmap with a device lost restoration of the content.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    void DeviceLost(bool isSoftwareBitmap);

    BEGIN_TEST_METHOD(WriteableBitmapSetSource)
        TEST_METHOD_PROPERTY(L"Description", L"WriteableBitmap::SetSource(stream)")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TFS_6246592)
        TEST_METHOD_PROPERTY(L"Description", L"Regression test that verifies SoftwareBitmapSource with software rasterization, reloading the software surface after the hardware surface has been purged.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TFS_9742148)
        TEST_METHOD_PROPERTY(L"Description", L"Regression test: Flushes surface updates in-between locks of a surface during asynchronous decoding (SoftwareBitmapSource version).")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetBitmapAsync_DeviceLost)
        TEST_METHOD_PROPERTY(L"Description", L"Loads an image into the SoftwareBitmapSource after a device loss.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

private:

    void SingleImageTestHelper(
        bool isSoftwareBitmap,
        ::Windows::Graphics::Imaging::SoftwareBitmap^ pTestBitmap,
        xaml_imaging::WriteableBitmap^ pWriteableBitmap,
        bool expectFailure = false,
        MockDComp::SurfaceIdMode surfaceIdMode = MockDComp::SurfaceIdMode::AllocationOrder,
        Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering = Microsoft::UI::Xaml::Tests::Common::DCompRendering::WUCCompleteSynchronousCompTree
        );

    void SetBitmapAsync_DeviceLostHelper(bool delay);

    static xaml_imaging::WriteableBitmap^ CreateWriteableBitmap(
        unsigned int width,
        unsigned int height,
        uint32_t colorValue1,
        uint32_t colorValue2
        );

    static void WriteWriteableBitmap(
        xaml_imaging::WriteableBitmap^ bitmap,
        unsigned int width,
        unsigned int height,
        uint32_t colorValue1,
        uint32_t colorValue2
        );

    static ::Windows::Graphics::Imaging::SoftwareBitmap^ CreateCustomSoftwareBitmap(
        Microsoft::WRL::ComPtr<CustomWicBitmap> customWicBitmap
        );

    static ::Windows::Graphics::Imaging::SoftwareBitmap^ CreateCustomSoftwareBitmap(
        bool supportsTransparency,
        unsigned int width,
        unsigned int height,
        uint32_t colorValue1,
        uint32_t colorValue2
        );

    void AllocateDisplayReleaseStress(int iterationCount);

    static const uint32_t OpaqueWhite = 0xFFFFFFFF;
    static const uint32_t OpaqueRed = 0xFFFF0000;
    static const uint32_t OpaqueGreen = 0xFF00FF00;
    static const uint32_t OpaqueBlue = 0xFF0000FF;

    // All colors that use alpha should use Pre-multiplied alpha
    static const uint32_t TranslucentRed = 0x7F7F0000;
    static const uint32_t TranslucentGreen = 0x7F007F00;
    static const uint32_t TranslucentBlue = 0x7F00007F;

    inline Platform::String^ GetResourcesPath() const;
};

} } } } } } }

