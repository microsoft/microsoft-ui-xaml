// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>
#include "ImageTestEngine.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

        class ImageTests : public WEX::TestClass<ImageTests>
        {
        public:
            BEGIN_TEST_CLASS(ImageTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"02f4717a-a120-494b-a28e-9a2cbd41ca58;bd1463b3-e5f2-4d54-9394-63a431c53a6e;d04573b8-e899-4822-bb72-9f4743c89d36")

#if defined(_WIN64) // Disable the test on 64-bit build because of floating point differences between x87 and sse instructions
                TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")
#endif
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(SimpleImageElement)
                TEST_METHOD_PROPERTY(L"Description", L"Renders a simple image element.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultiImagesSameUriFirstFrame)
                TEST_METHOD_PROPERTY(L"Description", L"Renders a multiple image elements from XAML with the same URI, but sets it in the first frame.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultiImageSameUriDiffSizeFirstFrame)
                TEST_METHOD_PROPERTY(L"Description", L"Renders a multiple image elements from XAML with the same URI (different render sizes), but sets it in the first frame.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SimpleImageElementRelativePath)
                TEST_METHOD_PROPERTY(L"Description", L"Renders a simple image element using a relative path to reference the image.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ExplicitXaml)
                TEST_METHOD_PROPERTY(L"Description", L"Renders a simple image element using a relative path to reference the image with explicit xaml syntax.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlateauScaleChange)
                TEST_METHOD_PROPERTY(L"Description", L"Renders a simple image element and changes the plateau scale.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(WriteableBitmapInvalidate)
                TEST_METHOD_PROPERTY(L"Description", L"Renders a simple image using a WriteableBitmap with a call to WriteableBitmap::Invalid.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(WriteableBitmapNoInvalidate)
                TEST_METHOD_PROPERTY(L"Description", L"Renders a simple image using a WriteableBitmap without a call to WriteableBitmap::Invalid.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BaseImageTestEngineWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Runs the image test engine with all default values and a single image.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SimpleCRCCheck)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Validates image loading with a CRC check of the output.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BmpImage)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Uses a BMP test image.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PngImage)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Uses a PNG test image.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(JxrImage)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Uses an hdr JXR test image.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(JxrImageSDR)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] An hdr JXR test image is decoded into an SDR surface.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeHDRMode)
                TEST_METHOD_PROPERTY(L"Description", L"JXR test image should redecode on display HDR mode switch.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StretchMode)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Uses the same image with all different stretch modes simultaneously.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StretchModeNoneLarger)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Uses the Stretch::None with a larger element than the image itself.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DecodeToRenderSizeOff)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image with DecodeToRenderSize feature disabled.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultipleImages)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads multiple different images.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultipleImagesSameUri)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads multiple images with the same URI to test caching behavior.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultipleImagesSameUriIgnoreCache)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads multiple images with the same URI to test caching behavior but ignores caching.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BitmapCacheWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image with the BitmapCache API.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LoadStreamSync)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image as a stream with SetSource (synchronous).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LoadStreamAsync)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image as a stream with SetSourceAsync.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LoadStreamAsync2)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image as a stream with SetSourceAsync. Calls SetSourceAsync 2 times quickly without waiting.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StreamCleanupAndRestore)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image as a stream and attempts to cleanup the hardware resources and verifies they are decoded to element size.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // MockDComp crash
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Opacity50)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image with 50% opacity.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Opacity0)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image with 0% opacity.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DecodePixelWidth)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image and uses the DecodePixelWidth API to control decoding size.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DecodePixelHeight)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image and uses the DecodePixelHeight API to control decoding size.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DecodePixelWidthAndHeightWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image and uses the DecodePixelWidth/DecodePixelHeight API to control decoding size.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // MockDComp crash
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DecodePixelTypeLogical)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image and uses the DecodePixelType API to control decoding size.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            // TODO:
            //       Consider adding image test engine support for altering the properties of the images and element.
            //       This should allow for more tests that change things dynamically but requires some special consideration for
            //       timing and waiting for decode operations to complete which can get finnicky in terms of test stability.
            //       Especially since the ImageOpened will not fire for subsequent decodes, ETW events will have to be used.
            //       Cool tests to add:
            //       - Changing the DecodePixelWidth/Height/Type

            BEGIN_TEST_METHOD(BorderElement)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image within a Border element background.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EllipseShapeWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image as a brush for an Ellipse shape.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NineGrid)
                TEST_METHOD_PROPERTY(L"Description", L"[ImageTestEngine] Loads an image and uses a NineGrid for image scaling.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            TEST_METHOD(NineGridNoSource)

            BEGIN_TEST_METHOD(ImageElementCastingSource)
                TEST_METHOD_PROPERTY(L"Description", L"Gets the casting source from an image element.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VeryLargeImageSize)
                TEST_METHOD_PROPERTY(L"Description", L"Loads a very large image and verifies ImageOpened in reasonable time (< 2sec).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VeryLargeImageSizeNineGridWUCFull)
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // No Dcomp/MockDComp virtual surface support yet
                TEST_METHOD_PROPERTY(L"Description", L"Loads a very large image and verifies ImageOpened in reasonable time (< 2sec).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VeryLarge_4640x168_NineGrid)
                TEST_METHOD_PROPERTY(L"Description", L"Loads a very large image (4640x168) with a ninegrid and verifies ImageOpened in reasonable time (< 2sec).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VeryLarge_4640x504_NineGridWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Loads a very large image (4640x504) with a ninegrid and verifies ImageOpened in reasonable time (< 2sec).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VeryLarge_168x3744_NineGrid)
                TEST_METHOD_PROPERTY(L"Description", L"Loads a very large image (168x3744) with a ninegrid and verifies ImageOpened in reasonable time (< 2sec).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VeryLarge_504x3725_NineGrid)
                TEST_METHOD_PROPERTY(L"Description", L"Loads a very large image (504x3725) with a ninegrid and verifies ImageOpened in reasonable time (< 2sec).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VeryLarge_4640x168_Hardware_And_BitmapCacheWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Loads a very large image (4640x168), renders it in hardware and in BitmapCache.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InvalidImageInImageControl)
                TEST_METHOD_PROPERTY(L"Description", L"Attempts to load an invalid image and ensure that ImageFailed gets fired and ImageOpened does not get fired (in an Image control).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NotExistingImageInImageControl)
                TEST_METHOD_PROPERTY(L"Description", L"Attempts to load a not existing image and ensure that ImageFailed gets fired and ImageOpened does not get fired (in an Image control).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NotExistingImageInImageControlMsAppx)
                TEST_METHOD_PROPERTY(L"Description", L"Attempts to load a not existing ms-appx resource and ensure that ImageFailed gets fired and ImageOpened does not get fired (in an Image control).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InvalidImageInImageBrush)
                TEST_METHOD_PROPERTY(L"Description", L"Attempts to load an invalid image and ensure that ImageFailed gets fired and ImageOpened does not get fired (in an Image brush).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NotExistingImageInImageBrush)
                TEST_METHOD_PROPERTY(L"Description", L"Attempts to load a not existing image and ensure that ImageFailed gets fired and ImageOpened does not get fired (in an Image brush).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NotExistingImageInImageBrushMsAppx)
                TEST_METHOD_PROPERTY(L"Description", L"Attempts to load a not existing ms-appx resource and ensure that ImageFailed gets fired and ImageOpened does not get fired (in an Image brush).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InvalidImageClearsPreviousOne)
                TEST_METHOD_PROPERTY(L"Description", L"Loading an invalid image should clear the previously successfully loaded one")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NotExistingImageClearsPreviousOne)
                TEST_METHOD_PROPERTY(L"Description", L"Loading from a not existing image Uri should clear the previously successfully loaded one")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NullUriClearsPreviousImage)
                TEST_METHOD_PROPERTY(L"Description", L"Setting the null uri should clear the previously loaded loaded image")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AutomationPeerDefault)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a XAML tree with an image element that has no automation peer set and ensures the correct automation results.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AutomationPeerProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a XAML tree with an image element that has automation peer properties set and ensures the correct automation results.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DownloadProgressUriSource)
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DownloadProgressStreamSource)
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DownloadProgressStreamSourceAsync)
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DontReloadImageFromStream)
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ToggleUriSource)
                TEST_METHOD_PROPERTY(L"Description", L"Set Uri to A, then change to B, then set back to A")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RasterizationScale)
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            TEST_METHOD(DecodeToRenderSizeUnderCollapsedSubtree)

        private:
            void BaseImageTestEngineInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering rendering);
            void DecodePixelWidthAndHeightInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering rendering);
            void EllipseShapeInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering rendering);
            void VeryLargeImageSizeNineGridInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering rendering);
            void VeryLarge_4640x504_NineGridInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering rendering);

            inline Platform::String^ GetResourcesPath() const;

            xaml_imaging::WriteableBitmap^ CreateSolidColorWriteableBitmap(int width, int height, uint32 colorValue);

            void MultipleImagesTestHelper(Platform::String^ pXamlPath, Platform::String^ pImagePath, MockDComp::SurfaceIdMode imageCrcMode);

            void BitmapCache(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

            void VeryLarge_4640x168_Hardware_And_BitmapCache(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

            void DownloadProgressTestHelper(TestImageEnums::LoadApi loadApi);
            void LoadHelper(xaml_imaging::BitmapImage ^bitmapImage, TestImageEnums::LoadApi loadApi);
        };

    } } }
} } } }

