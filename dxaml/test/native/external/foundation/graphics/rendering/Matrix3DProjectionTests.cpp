// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "Matrix3DProjectionTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool Matrix3DProjectionTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool Matrix3DProjectionTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool Matrix3DProjectionTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

// TODO: Matrix3D_Partial can be componentized, and these can be converted to unit tests.
void Matrix3DProjectionTests::InverseTest()
{
    TestCleanupWrapper cleanup;
    Matrix3D* invertibleMatrix = nullptr;
    Matrix3D* nonInvertibleMatrix = nullptr;

    RunOnUIThread([&]()
    {
        invertibleMatrix = GetInvertibleMatrix();
        bool hasInverse = Matrix3DHelper::GetHasInverse(*invertibleMatrix);
        VERIFY_IS_TRUE(hasInverse);

        nonInvertibleMatrix = GetNonInvertibleMatrix();
        hasInverse = Matrix3DHelper::GetHasInverse(*nonInvertibleMatrix);
        VERIFY_IS_FALSE(hasInverse);

        *invertibleMatrix = Matrix3DHelper::Invert(*invertibleMatrix);
        VERIFY_IS_TRUE(CompareMatrices(invertibleMatrix, GetExpectedInverse()));
    });
}

void Matrix3DProjectionTests::MultiplicationTest()
{
    TestCleanupWrapper cleanup;

    Matrix3D* invertibleMatrix = nullptr;
    Matrix3D* secondInvertibleMatrix = nullptr;
    Matrix3D expectedProduct;

    RunOnUIThread([&]()
    {
        invertibleMatrix = GetInvertibleMatrix();
        secondInvertibleMatrix = GetInvertibleMatrix();
        *secondInvertibleMatrix = Matrix3DHelper::Invert(*secondInvertibleMatrix);

        // M x M^(-1) = I
        expectedProduct = Matrix3DHelper::Identity;

        Matrix3D product = Matrix3DHelper::Multiply(*invertibleMatrix, *secondInvertibleMatrix);
        VERIFY_IS_TRUE(CompareMatrices(&product, &expectedProduct));
    });
}

void Matrix3DProjectionTests::VerifyComplexMatrix3DProjection()
{
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = nullptr;
    });

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 600));

    xaml_controls::Grid^ rootPanel = nullptr;
    xaml_shapes::Rectangle^ rect = nullptr;

    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top' > "
                L"  <Rectangle x:Name='rect' HorizontalAlignment='Left' VerticalAlignment='Top' Width='200' Height='200' Fill='Blue' /> "
                L"</Grid>"));

        rect = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"rect"));
        VERIFY_IS_NOT_NULL(rect);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        const double pi = 3.1415926;
        double fovY = pi / 2.0;
        double translationZ = -rect->ActualHeight / tan(fovY / 2.0);
        double theta = 20.0 * pi / 180.0;

        // Emulate a plane projection transformation
        Matrix3D* centerImageAtOrigin = CreateTranslationTransform(
            -rect->Width / 2.0,
            -rect->Height / 2.0, 0);
        Matrix3D* invertYAxis = CreateScaleTransform(1.0, -1.0, 1.0);
        Matrix3D* rotateAboutY = CreateRotateYTransform(theta);
        Matrix3D* translateAwayFromCamera = CreateTranslationTransform(0, 0, translationZ);
        Matrix3D* perspective = CreatePerspectiveTransform(
            fovY,                                   // field of view
            rootPanel->Width / rootPanel->Height,   // aspect ratio
            1.0,                                    // near plane
            1000.0);                                // far plane
        Matrix3D* viewport = CreateViewportTransform(rootPanel->Width, rootPanel->Height);

        // Multiply the different transforms into a single matrix
        Matrix3D m = Matrix3DHelper::Multiply(*centerImageAtOrigin, *invertYAxis);
        m = Matrix3DHelper::Multiply(m, *rotateAboutY);
        m = Matrix3DHelper::Multiply(m, *translateAwayFromCamera);
        m = Matrix3DHelper::Multiply(m, *perspective);
        m = Matrix3DHelper::Multiply(m, *viewport);

        Matrix3DProjection^ m3dProjection = ref new Matrix3DProjection();
        // Set the projection matrix
        m3dProjection->ProjectionMatrix = m;
        // Set the element's projection to the 3D projection
        rect->Projection = m3dProjection;
    });
    TestServices::WindowHelper->WaitForIdle();

    // Verify that it looks right.
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

Matrix3D* Matrix3DProjectionTests::GetInvertibleMatrix()
{
    auto result = new Matrix3D();
    *result = Matrix3DHelper::FromElements(
        3, 0, 2, -1,
        1, 2, 0, -2,
        4, 0, 6, -3,
        5, 0, 2, 0);
    return result;
}

Matrix3D* Matrix3DProjectionTests::GetExpectedInverse()
{
    auto result = new Matrix3D();
    *result = Matrix3DHelper::FromElements(
         0.6, 0.0, -0.2, 0.0,
        -2.5, 0.5,  0.5, 1.0,
        -1.5, 0.0,  0.5, 0.5,
        -2.2, 0.0,  0.4, 1.0);
    return result;
}

Matrix3D* Matrix3DProjectionTests::GetNonInvertibleMatrix()
{
    auto result = new Matrix3D();
    *result = Matrix3DHelper::FromElements(
        1, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
    return result;
}

Microsoft::UI::Xaml::Media::Media3D::Matrix3D* Matrix3DProjectionTests::CreateTranslationTransform(double tx, double ty, double tz)
{
    Matrix3D* m = new Matrix3D();

    m->M11 = 1.0; m->M12 = 0.0; m->M13 = 0.0; m->M14 = 0.0;
    m->M21 = 0.0; m->M22 = 1.0; m->M23 = 0.0; m->M24 = 0.0;
    m->M31 = 0.0; m->M32 = 0.0; m->M33 = 1.0; m->M34 = 0.0;
    m->OffsetX = tx; m->OffsetY = ty; m->OffsetZ = tz; m->M44 = 1.0;

    return m;
}

Microsoft::UI::Xaml::Media::Media3D::Matrix3D* Matrix3DProjectionTests::CreateScaleTransform(double sx, double sy, double sz)
{
    Matrix3D* m = new Matrix3D();

    m->M11 = sx; m->M12 = 0.0; m->M13 = 0.0; m->M14 = 0.0;
    m->M21 = 0.0; m->M22 = sy; m->M23 = 0.0; m->M24 = 0.0;
    m->M31 = 0.0; m->M32 = 0.0; m->M33 = sz; m->M34 = 0.0;
    m->OffsetX = 0.0; m->OffsetY = 0.0; m->OffsetZ = 0.0; m->M44 = 1.0;

    return m;
}

Microsoft::UI::Xaml::Media::Media3D::Matrix3D* Matrix3DProjectionTests::CreateRotateYTransform(double theta)
{
    double localSin = sin(theta);
    double localCos = cos(theta);

    Matrix3D* m = new Matrix3D();

    m->M11 = localCos; m->M12 = 0.0; m->M13 = -localSin; m->M14 = 0.0;
    m->M21 = 0.0; m->M22 = 1.0; m->M23 = 0.0; m->M24 = 0.0;
    m->M31 = localSin; m->M32 = 0.0; m->M33 = localCos; m->M34 = 0.0;
    m->OffsetX = 0.0; m->OffsetY = 0.0; m->OffsetZ = 0.0; m->M44 = 1.0;

    return m;
}

Microsoft::UI::Xaml::Media::Media3D::Matrix3D* Matrix3DProjectionTests::CreatePerspectiveTransform(double fieldOfViewY, double aspectRatio, double zNearPlane, double zFarPlane)
{
    double height = 1.0 / tan(fieldOfViewY / 2.0);
    double width = height / aspectRatio;
    double d = zNearPlane - zFarPlane;

    Matrix3D* m = new Matrix3D();
    m->M11 = width; m->M12 = 0; m->M13 = 0; m->M14 = 0;
    m->M21 = 0; m->M22 = height; m->M23 = 0; m->M24 = 0;
    m->M31 = 0; m->M32 = 0; m->M33 = zFarPlane / d; m->M34 = -1;
    m->OffsetX = 0; m->OffsetY = 0; m->OffsetZ = zNearPlane * zFarPlane / d; m->M44 = 0;

    return m;
}

Microsoft::UI::Xaml::Media::Media3D::Matrix3D* Matrix3DProjectionTests::CreateViewportTransform(double width, double height)
{
    Matrix3D* m = new Matrix3D();

    m->M11 = width / 2.0; m->M12 = 0.0; m->M13 = 0.0; m->M14 = 0.0;
    m->M21 = 0.0; m->M22 = -height / 2.0; m->M23 = 0.0; m->M24 = 0.0;
    m->M31 = 0.0; m->M32 = 0.0; m->M33 = 1.0; m->M34 = 0.0;
    m->OffsetX = width / 2.0; m->OffsetY = height / 2.0; m->OffsetZ = 0.0; m->M44 = 1.0;

    return m;
}

bool Matrix3DProjectionTests::CompareMatrices(Microsoft::UI::Xaml::Media::Media3D::Matrix3D* m1, Microsoft::UI::Xaml::Media::Media3D::Matrix3D* m2)
{
    // Due to very small floating point differences when inverting/multiplying, we can't just use strict equality when determining if these Matrices are equivalent.
    double epsilon = 0.0001;
    return CompareDoublesWithEpsilon(m1->M11,m2->M11,epsilon) &&
    CompareDoublesWithEpsilon(m1->M12,m2->M12,epsilon) &&
    CompareDoublesWithEpsilon(m1->M13,m2->M13,epsilon) &&
    CompareDoublesWithEpsilon(m1->M14,m2->M14,epsilon) &&
    CompareDoublesWithEpsilon(m1->M21,m2->M21,epsilon) &&
    CompareDoublesWithEpsilon(m1->M22,m2->M22,epsilon) &&
    CompareDoublesWithEpsilon(m1->M23,m2->M23,epsilon) &&
    CompareDoublesWithEpsilon(m1->M24,m2->M24,epsilon) &&
    CompareDoublesWithEpsilon(m1->M31,m2->M31,epsilon) &&
    CompareDoublesWithEpsilon(m1->M32,m2->M32,epsilon) &&
    CompareDoublesWithEpsilon(m1->M33,m2->M33,epsilon) &&
    CompareDoublesWithEpsilon(m1->M34,m2->M34,epsilon) &&
    CompareDoublesWithEpsilon(m1->OffsetX,m2->OffsetX,epsilon) &&
    CompareDoublesWithEpsilon(m1->OffsetY,m2->OffsetY,epsilon) &&
    CompareDoublesWithEpsilon(m1->OffsetZ,m2->OffsetZ,epsilon) &&
    CompareDoublesWithEpsilon(m1->M44,m2->M44,epsilon);
}

bool Matrix3DProjectionTests::CompareDoublesWithEpsilon(double a, double b, double epsilon)
{
    return (a - b < epsilon) && (b - a < epsilon);
}

} } } } } }
