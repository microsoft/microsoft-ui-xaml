// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

        class LoadedImageSurfaceTests : public WEX::TestClass<LoadedImageSurfaceTests>
        {
        public:
            BEGIN_TEST_CLASS(LoadedImageSurfaceTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"02f4717a-a120-494b-a28e-9a2cbd41ca58;bd1463b3-e5f2-4d54-9394-63a431c53a6e;d04573b8-e899-4822-bb72-9f4743c89d36")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CreateFromUri)
                TEST_METHOD_PROPERTY(L"Description", L"Create a LoadedImageSurface from Uri")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateFromUriWithSize)
                TEST_METHOD_PROPERTY(L"Description", L"Create a LoadedImageSurface from Uri specifying desired size small enough for non-virtual surface")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateFromUriWithSizeAndAtlasHint)
                TEST_METHOD_PROPERTY(L"Description", L"Create a LoadedImageSurface from Uri, set atlas hint")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // MockDComp isn't injected on OneCore
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // xaml::Window::Current does not exists on wpf/win32
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateFromUriWithSizeVirtual)
                TEST_METHOD_PROPERTY(L"Description", L"Create a LoadedImageSurface from Uri specifying desired size bigger than virtual threshold")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateFromStream)
                TEST_METHOD_PROPERTY(L"Description", L"Create a LoadedImageSurface from IStream")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateFromStreamWithSize)
                TEST_METHOD_PROPERTY(L"Description", L"Create a LoadedImageSurface from IStream specifying the desired size small enough for non-virtual surface")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateFromStreamWithSizeVirtual)
                TEST_METHOD_PROPERTY(L"Description", L"Create a LoadedImageSurface from IStream specifying the desired size bigger than virtual threshold")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FirstUseAfterLoaded)
                TEST_METHOD_PROPERTY(L"Description", L"Wait until the surface is loaded before using it")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CloseStreamWhileStillLoading)
                TEST_METHOD_PROPERTY(L"Description", L"Close the stream used to create LoadedImageSurface before loading completes")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CloseStreamAfterLoaded)
                TEST_METHOD_PROPERTY(L"Description", L"Close the stream used to create LoadedImageSurface after loading has completed")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CloseCreatedFromStream)
                TEST_METHOD_PROPERTY(L"Description", L"Close LoadedImageSurface created from stream")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CloseCreatedFromUri)
                TEST_METHOD_PROPERTY(L"Description", L"Close LoadedImageSurface created from uri")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CloseAfterLoadCompleted)
                TEST_METHOD_PROPERTY(L"Description", L"Close LoadedImageSurface after LoadCompleted has fired")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DeviceLostRecover)
                TEST_METHOD_PROPERTY(L"Description", L"DeviceLost happens before the first use of LoadedImageSurface")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DeviceLostInUse)
                TEST_METHOD_PROPERTY(L"Description", L"DeviceLost happens while the LoadedImageSurface is in use")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DeviceLostInUseWindowHidden)
                TEST_METHOD_PROPERTY(L"Description", L"DeviceLost happens while the LoadedImageSurface is in use and the window is hidden")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DeviceLostWindowHiddenBeforeLoad)
                TEST_METHOD_PROPERTY(L"Description", L"DeviceLost happens while the window is hidden just before the LoadedImageSurface starts loading")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DeviceLostBeforeLoad)
                TEST_METHOD_PROPERTY(L"Description", L"DeviceLost happens just before the LoadedImageSurface starts loading")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NonExistingUri)
                TEST_METHOD_PROPERTY(L"Description", L"Try loading from non existing URI")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InvalidFormatFromUri)
                TEST_METHOD_PROPERTY(L"Description", L"Try loading corrupted file from a valid URI")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlateauScaleChange)
                TEST_METHOD_PROPERTY(L"Description", L"Ensure the image is redecoded after plateau scale change, loaded from Uri")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlateauScaleChangeDuringDownload)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlateauScaleChangeStream)
                TEST_METHOD_PROPERTY(L"Description", L"Ensure the image is redecoded after plateau scale change, loaded from stream")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ExceedPlateauScaleLimit)
                TEST_METHOD_PROPERTY(L"Description", L"Make sure we do not crash when the plateau scale exceeds the max. Expect the image to be downscaled instead.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlateauScaleChangeMRT)
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TAEF image tests cannot access MRT resources
                TEST_METHOD_PROPERTY(L"Description", L"Ensure proper MRT resource is taken after plateau scale change")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(JumboImageFromUri)
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // There is no reliable way to handle OOM in LoadedImageSurface
                TEST_METHOD_PROPERTY(L"Description", L"Try loading a monster image from URI expecting an OOM")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LowMemoryStream)
                TEST_METHOD_PROPERTY(L"Description", L"Simulate low memory condition when the image was loaded from stream")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreatePairFromSameUri)
                TEST_METHOD_PROPERTY(L"Description", L"Create a pair of LIS's from the same URI")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreatePairFromSameUriWithSize)
                TEST_METHOD_PROPERTY(L"Description", L"Create a pair of LIS's from the same URI with size specified to 256 x 256 (forces non-virtual surfaces)")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreatePairFromSameUriWithAndWithoutSize)
                TEST_METHOD_PROPERTY(L"Description", L"Create a pair of LIS's for the same URI with one specifying size 256 x 256 (non-virtual) and one not specifiying size (virtual)")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreatePairFromDifferentUris)
                TEST_METHOD_PROPERTY(L"Description", L"Create a pair of LIS's from different URIs")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreatePairFromUriAndStream)
                TEST_METHOD_PROPERTY(L"Description", L"Create a pair of LIS's for different images, one from URI and one from stream")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreatePairFromUriWithSizeAndStream)
                TEST_METHOD_PROPERTY(L"Description", L"Create a pair of LIS's for different images, one from URI with size specified and one from stream")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreatePairForSameImageFromUriAndStream)
                TEST_METHOD_PROPERTY(L"Description", L"Create a pair of LIS's for same image, one from URI and one from stream")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UnofferDeviceBeforeImageResize)
                TEST_METHOD_PROPERTY(L"Description", L"Make sure that LIS could successfully resize image when the hardware device is in offered state")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ResizeLargerWhileDecodeIsPending)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Unreliable test
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ResizeSmallerWhileDecodeIsPending)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Unreliable test
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ResizeLargerWhileSurfaceUpdateIsPending)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ResizeSmallerWhileSurfaceUpdateIsPending)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LoadAfterDeviceLostOnStartup)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Messes with the device to simulate a bad startup, AVs in WPF mode from disposing the compositor at a bad time.
            END_TEST_METHOD()

        private:
            inline Platform::String^ GetResourcesPath() const;
            void ResizeWhileDecodeIsPending(float scale1, float scale2, float scale3, float scale4);
            void ResizeWhileSurfaceUpdateIsPending(float scale1, float scale2, float scale3, float scale4);
        };

    } } }
} } } }

