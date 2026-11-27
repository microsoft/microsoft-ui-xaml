// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class TransformToVisualTests : public WEX::TestClass<TransformToVisualTests>
{
public:
    BEGIN_TEST_CLASS(TransformToVisualTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)

    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    TEST_METHOD(TransformToVisual_Siblings)

    TEST_METHOD(BasicTestOnHighDPI)

    BEGIN_TEST_METHOD(ProjectedElementTransformInverse)
        TEST_METHOD_PROPERTY(L"Description", L"TransformToVisual through a PlaneProjection returns a non-matrix transform whose Inverse maps transformed points back.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TransformFromStringAndInverse)
        TEST_METHOD_PROPERTY(L"Description", L"A Transform-typed property set from a matrix string in markup creates a MatrixTransform; Transform.Inverse inverts it and fails for a singular transform.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PopupInProjectedParentHitTest)
        TEST_METHOD_PROPERTY(L"Description", L"The child of an open Popup whose parent has a PlaneProjection is hit-testable at its projected position.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NearPlaneClippedProjectionVisibleBounds)
        TEST_METHOD_PROPERTY(L"Description", L"The visible bounds of a VirtualSurfaceImageSource on an element with a Matrix3DProjection exclude the parts behind the near plane, and are empty for non-positive w.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PopupBehindViewerIsNotHitTested)
        TEST_METHOD_PROPERTY(L"Description", L"The child of an open Popup whose parent's Matrix3DProjection puts it behind the viewer (negative w) is not hit-testable, while the same position is hit with an identity projection.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ProjectedTextBlockAlphaMask)
        TEST_METHOD_PROPERTY(L"Description", L"TextBlock.GetAlphaMask returns a mask for a TextBlock inside a translated parent that has a PlaneProjection.")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;
};

} } } } } }
