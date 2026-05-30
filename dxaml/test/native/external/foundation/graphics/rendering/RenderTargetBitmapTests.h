// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WUCRenderingScopeGuard.h>
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class RenderTargetBitmapTests : public WEX::TestClass<RenderTargetBitmapTests>
{
public:
    BEGIN_TEST_CLASS(RenderTargetBitmapTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(BasicRenderTargetBitmapWithDebugDevice)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises BasicRenderTargetBitmap with Debug D3D device")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"Ignore", L"True") // TODO: Re-enable D3D Debug Layer tests
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BasicRenderTargetBitmapWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap.RenderAsync for a simple visual tree and consumes the result in an Image.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PopupChildRTBWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap.RenderAsync for a Popup child visual tree and consumes the result in an Image.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Crash - failed to assign Popup.Child
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PopupRTBWUCFull)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap.RenderAsync for a Popup parent visual tree and consumes the result in an Image.")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderScrollViewerBitmapWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap.RenderAsync for a ScrollViewer and consumes the result in an Image.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PanAfterRenderScrollViewerBitmapWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap.RenderAsync for a ScrollViewer and then pans it.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderTransformRTBWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap for simple combinations of render transforms and verifies RTB surfaces.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RTLElementWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap.RenderAsync for an RTL element")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RTLwithinRTLWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap.RenderAsync for an RTL element located within an RTL parent")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LTRwithinRTLWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap.RenderAsync for an LTR element located within RTL parent")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderToSizeWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap asking a specific size.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScaledRenderWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap after having changed the current scale factor and checks the effective rendered size.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Testing private API: XamlRenderingBackgroundTask.SetScalePercentage this is not supported in Island mode
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BorderDiscardMaskOnResizeWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap while changing a Border from rendering with a mask to rendering without a mask and back.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BorderDiscardMaskOnResizeBitmapCacheWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap while changing a Border from rendering with a mask to rendering without a mask and back.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"UAP:WaitForXamlWindowActivation", L"false")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(XamlCompositionBrushRTBWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap.RenderAsync for rectangles filled with XamlCompositionBrush.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AncestorOpacity0WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap on an element that has 0 opacity on an ancestor element")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AncestorClippedWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap on child of element with 0x0 clip")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AncestorScaledTo0WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap on child of element with ScaleTransform scaled down to 0")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LTEOpacity0WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap on target of an LTE that's been culled due to 0 opacity")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderSameElementWUCFull)
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // Test must use a fresh XAML state for repro.
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap on an element multiple times.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TiledTextWUCFull)
        // TODO:  This test does not verify surfaces.
        TEST_METHOD_PROPERTY(L"Description", L"Invokes RenderTargetBitmap on a text element that must tile it's content.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Ignore", L"True")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RTBFirstFailWUCFull)
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // Test must use a fresh XAML state for repro.
        TEST_METHOD_PROPERTY(L"Description", L"Test whereby the very first RTB causes a FailRender.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RTB0SizeWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Tests RTB of an element with 0x0 for width x height.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LineTest)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PathTest)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EllipseTest)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RasterizationScale)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    void BasicRenderTargetBitmapInternal();
    void PopupChildRTBInternal();
    void PopupRTBInternal();
    void RenderScrollViewerBitmapInternal();
    void PanAfterRenderScrollViewerBitmapInternal();
    void RenderTransformRTBInternal();
    void RTLElementInternal();
    void RTLwithinRTLInternal();
    void LTRwithinRTLInternal();
    void RenderToSizeInternal();
    void ScaledRenderInternal();
    void BorderDiscardMaskOnResizeInternal(Microsoft::UI::Xaml::Media::CacheMode^ cacheMode);
    void XamlCompositionBrushRTBInternal(bool expectCaptureAsync);
    void AncestorOpacity0Internal();
    void AncestorClippedInternal();
    void AncestorScaledTo0Internal(bool verifyIsTransparent);
    void LTEOpacity0Internal();
    void RenderSameElementInternal();
    void TiledTextInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering rendering);
    void RTB0SizeInternal();

    // TODO: Should add the following tests in the future:
    //                 - TiledImages
    //                 - More complex transforms (Translation + Scale + Rotation)
    //                 - Downscaling quality check

    static bool CheckPixelValue(
        Platform::Array<byte>^ imageData,
        int x,
        int y,
        int pixelStride,
        uint32_t expectedPixelValue);

    inline Platform::String^ GetResourcesPath() const;
    void StackPanelTestHelper(
        Platform::String^ fileName,
        bool expectRTL = false,
        bool useXCB = false,
        bool expectCaptureAsync = false,
        int renderCallCount = 1,
        bool verifyIsTransparent = false);

    unsigned int PixelIndex(unsigned int x, unsigned int y, unsigned int w);
    uint32_t RgbaToUint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void TestShapeInternal(Platform::String^ fileName, std::function<void(Platform::Array<uint32_t>^, int, int)> verifyFunc);
};
} } } } } }

