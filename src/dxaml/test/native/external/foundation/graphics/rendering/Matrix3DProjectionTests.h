// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class Matrix3DProjectionTests : public WEX::TestClass<Matrix3DProjectionTests>
{
public:
    BEGIN_TEST_CLASS(Matrix3DProjectionTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    TEST_METHOD(InverseTest)

    TEST_METHOD(MultiplicationTest)

    BEGIN_TEST_METHOD(VerifyComplexMatrix3DProjection)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    Microsoft::UI::Xaml::Media::Media3D::Matrix3D* GetInvertibleMatrix();
    Microsoft::UI::Xaml::Media::Media3D::Matrix3D* GetExpectedInverse();
    Microsoft::UI::Xaml::Media::Media3D::Matrix3D* GetNonInvertibleMatrix();

    Microsoft::UI::Xaml::Media::Media3D::Matrix3D* CreateTranslationTransform(double tx, double ty, double tz);
    Microsoft::UI::Xaml::Media::Media3D::Matrix3D* CreateScaleTransform(double sx, double sy, double sz);
    Microsoft::UI::Xaml::Media::Media3D::Matrix3D* CreateRotateYTransform(double theta);
    Microsoft::UI::Xaml::Media::Media3D::Matrix3D* CreatePerspectiveTransform(double fieldOfViewY, double aspectRatio, double zNearPlane, double zFarPlane);
    Microsoft::UI::Xaml::Media::Media3D::Matrix3D* CreateViewportTransform(double width, double height);

    bool CompareMatrices(Microsoft::UI::Xaml::Media::Media3D::Matrix3D* m1, Microsoft::UI::Xaml::Media::Media3D::Matrix3D* m2);
    bool CompareDoublesWithEpsilon(double a, double b, double epsilon);
};

} } } } } }

