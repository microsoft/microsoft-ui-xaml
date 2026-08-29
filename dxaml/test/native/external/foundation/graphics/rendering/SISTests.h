// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <Microsoft.UI.Xaml.media.dxinterop.h>
#include <DXGI1_2.h>
#include <Dxgi1_3.h>
#include <D3D11.h>
#include <D2d1_1.h>

using namespace Microsoft::WRL;
using namespace Microsoft::UI::Xaml::Media::Imaging;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

// SIS : Surface Image Source
class SISTests : public WEX::TestClass<SISTests>
{
public:
    BEGIN_TEST_CLASS(SISTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\Microsoft.UI.Xaml.media.dxinterop.idl")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1d91ef47-c885-45e2-a578-7aaf1a1b1296;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(Basics1)
        TEST_METHOD_PROPERTY(L"Description", L"Render a SIS using ISurfaceImageSourceNative")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Basics2)
        TEST_METHOD_PROPERTY(L"Description", L"Render a SIS using ISurfaceImageSourceNativeWithD2D")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Advanced1)
        TEST_METHOD_PROPERTY(L"Description", L"Render two SIS's using ISurfaceImageSourceNativeWithD2D")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NegativeCases)
        TEST_METHOD_PROPERTY(L"Description", L"Validate various expected failure conditions")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SuspendFailureTestOldAPI)
        TEST_METHOD_PROPERTY(L"Description", L"Regression test: validate failure mode on old SIS API while app is suspended")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")  //Disable SIS SuspendFailureTestOldAPI (affected tests) due to Feature_DisableDirectCompositionOfferReclaim
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SuspendFailureTest)
        TEST_METHOD_PROPERTY(L"Description", L"Regression test: validate failure mode while app is suspended")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")  //Disable SIS SuspendFailureTestOldAPI (affected tests) due to Feature_DisableDirectCompositionOfferReclaim
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SuspendFailureTest2)
        TEST_METHOD_PROPERTY(L"Description", L"Regression test: Verify BeginDraw returns E_FAIL while app is suspended")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RegenerateVisual)
        TEST_METHOD_PROPERTY(L"Description", L"Tests that a SIS doesn't regenerate its SpriteVisual unless the surface changes.")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // MockDComp isn't injected on OneCore, so we can't count the number of sprite visuals cleaned up
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Mismatched redraw count
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(OfferReclaimChangeSFReleaseBeforeReclaim)
        TEST_METHOD_PROPERTY(L"Description", L"Test for the offer/reclaim mechanism change, if secondary SF is released after offering, we do not reclaim it.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(OfferInDrawingState)
        TEST_METHOD_PROPERTY(L"Description", L"Test for a bug fix of per-surface-factory offer-reclaim change, we do not offer the SF if corresponding SIS is in drawing state, and reclaim should be successful")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;
    void CreateD3DDevice();
    void EnableMultithreading();
    void CreateD2DDevice();
    void CreateD2DContext();
    void CleanupDevices();
    void Draw(ISurfaceImageSourceNative* pSIS, RECT rect, D2D1::ColorF color);
    void DrawWithD2D(ISurfaceImageSourceNativeWithD2D* pSIS, RECT rect, D2D1::ColorF color);
    void DrawWithD2DSuspend(ISurfaceImageSourceNativeWithD2D* pSIS, RECT rect, D2D1::ColorF color);
    void DrawWithD2DMultiThreaded(ISurfaceImageSourceNativeWithD2D* pSIS, RECT rect, D2D1::ColorF color);
    void FlushWork();
    ComPtr<ISurfaceImageSourceNative> GetSISNative(ISurfaceImageSource^ sis);
    ComPtr<ISurfaceImageSourceNativeWithD2D> GetSISNativeWithD2D(ISurfaceImageSource^ sis);

    unsigned int VerifySpriteVisualsCleanedUp(MockDComp::IMockDCompDevice2^ mockDevice2, unsigned int expected);

private:
    ComPtr<ID3D11Device> m_d3dDevice;
    ComPtr<IDXGIDevice> m_dxgiDevice;
    ComPtr<ID2D1Device> m_d2dDevice;
    ComPtr<ID2D1DeviceContext> m_d2dContext;
};

} } } } } }

