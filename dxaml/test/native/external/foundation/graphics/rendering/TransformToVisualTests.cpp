// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TransformToVisualTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>
#include <ChangeDpi.h>
#include <Collection.h>
#include <Microsoft.UI.Xaml.media.dxinterop.h>

using namespace Platform;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Windows::Foundation;

using namespace test_infra;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ TransformToVisualTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\general\\";
}

bool TransformToVisualTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool TransformToVisualTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool TransformToVisualTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void TransformToVisualTests::TransformToVisual_Siblings()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ child1;
    Canvas^ child2;
    RunOnUIThread([&]()
    {
        child1 = ref new Canvas();
        Canvas::SetLeft(child1, 50);

        child2 = ref new Canvas();
        Canvas::SetTop(child2, 50);

        Canvas^ root = ref new Canvas();
        root->Children->Append(child1);
        root->Children->Append(child2);

        wh->WindowContent = root;
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        const auto& transform = child1->TransformToVisual(child2);

        wf::Point origin(0,0);
        wf::Point transformed = transform->TransformPoint(origin);

        VERIFY_ARE_EQUAL(50, transformed.X);
        VERIFY_ARE_EQUAL(-50, transformed.Y);
    });
    wh->WaitForIdle();
}

void TransformToVisualTests::BasicTestOnHighDPI()
{
    TestCleanupWrapper cleanup;
    const auto& wh = TestServices::WindowHelper;

    // In some environments, we don't register for all the system events (e.g. window size changes) until we
    // we render our first frame, so make sure we render one before we change the DPI which will wait for
    // the dpi change notifications.
    RunOnUIThread([&]()
    {
        wh->WindowContent = ref new Grid();
    });
    wh->WaitForIdle();

    ChangeDPI changeDPI;    // Runs the test with DPI scale factor of 125%
    wh->WaitForIdle();

    Canvas^ child;
    RunOnUIThread([&]()
    {
        child = ref new Canvas();
        Canvas::SetLeft(child, 50);
        Canvas::SetTop(child, 50);

        Canvas^ root = ref new Canvas();
        root->Children->Append(child);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        const auto& transform = child->TransformToVisual(nullptr);

        wf::Point origin(0,0);
        wf::Point transformed = transform->TransformPoint(origin);

        VERIFY_ARE_EQUAL(50, transformed.X);
        VERIFY_ARE_EQUAL(50, transformed.Y);
    });
}

namespace
{
    bool AreClose(const wf::Point& expected, const wf::Point& actual, float tolerance = 0.05f)
    {
        LOG_OUTPUT(L"Expected (%f, %f), actual (%f, %f)", expected.X, expected.Y, actual.X, actual.Y);
        return std::abs(expected.X - actual.X) <= tolerance && std::abs(expected.Y - actual.Y) <= tolerance;
    }

    // WinRT object identity is the IUnknown pointer; different interface pointers of one object can differ.
    IUnknown* GetIdentityNoRef(Platform::Object^ object)
    {
        IUnknown* identity = nullptr;
        VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(object)->QueryInterface(IID_PPV_ARGS(&identity)));
        identity->Release();
        return identity;
    }

    bool ContainsElement(::Windows::Foundation::Collections::IIterable<UIElement^>^ elements, UIElement^ element)
    {
        IUnknown* expectedIdentity = GetIdentityNoRef(element);
        for (UIElement^ candidate : elements)
        {
            if (GetIdentityNoRef(candidate) == expectedIdentity)
            {
                return true;
            }
        }
        return false;
    }
}

void TransformToVisualTests::ProjectedElementTransformInverse()
{
    TestCleanupWrapper cleanup;
    const auto& wh = TestServices::WindowHelper;

    Grid^ root = nullptr;
    Border^ projected = nullptr;

    RunOnUIThread([&]()
    {
        root = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
            L"      Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'>"
            L"  <Border x:Name='projected' Width='200' Height='100' Margin='50,60,0,0' HorizontalAlignment='Left' VerticalAlignment='Top' Background='Red'>"
            L"    <Border.Projection><PlaneProjection RotationY='40' RotationX='15'/></Border.Projection>"
            L"  </Border>"
            L"</Grid>"));
        projected = safe_cast<Border^>(root->FindName(L"projected"));
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        GeneralTransform^ toRoot = projected->TransformToVisual(root);
        GeneralTransform^ fromRoot = root->TransformToVisual(projected);

        // A transform through a 3D projection can't be represented as a 2D MatrixTransform.
        VERIFY_IS_NULL(dynamic_cast<MatrixTransform^>(toRoot));
        VERIFY_IS_NULL(dynamic_cast<MatrixTransform^>(fromRoot));

        GeneralTransform^ toRootInverse = toRoot->Inverse;
        GeneralTransform^ fromRootInverse = fromRoot->Inverse;
        VERIFY_IS_NOT_NULL(toRootInverse);
        VERIFY_IS_NOT_NULL(fromRootInverse);
        VERIFY_IS_NULL(dynamic_cast<MatrixTransform^>(toRootInverse));

        for (const wf::Point& local : { wf::Point(30, 20), wf::Point(170, 80), wf::Point(100, 50) })
        {
            const wf::Point inRoot = toRoot->TransformPoint(local);

            // The margin alone would put the point at (local.X + 50, local.Y + 60). The projection rotates the element
            // around its center (100, 50) (the default CenterOfRotation), so the center stays at its margin-only
            // position and every other point is moved away from it.
            const wf::Point marginOnly(local.X + 50.0f, local.Y + 60.0f);
            if (local.X == 100.0f && local.Y == 50.0f)
            {
                VERIFY_IS_TRUE(AreClose(marginOnly, inRoot, 0.5f));
            }
            else
            {
                VERIFY_IS_GREATER_THAN(std::abs(inRoot.X - marginOnly.X) + std::abs(inRoot.Y - marginOnly.Y), 5.0f);
            }

            // The inverse maps the point back into the element's space.
            VERIFY_IS_TRUE(AreClose(local, toRootInverse->TransformPoint(inRoot)));

            // The reverse transform maps root points into the element's space, and its inverse is the forward transform.
            VERIFY_IS_TRUE(AreClose(local, fromRoot->TransformPoint(inRoot)));
            VERIFY_IS_TRUE(AreClose(inRoot, fromRootInverse->TransformPoint(local)));

            // Inverting twice gives back the forward transform.
            VERIFY_IS_TRUE(AreClose(inRoot, toRootInverse->Inverse->TransformPoint(local)));
        }
    });
}

void TransformToVisualTests::TransformFromStringAndInverse()
{
    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        auto border = safe_cast<Border^>(xaml_markup::XamlReader::Load(
            L"<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' RenderTransform='2,0,0,4,10,-20'/>"));

        // The Transform type converter creates a MatrixTransform from "M11,M12,M21,M22,OffsetX,OffsetY".
        auto matrixTransform = dynamic_cast<MatrixTransform^>(border->RenderTransform);
        VERIFY_IS_NOT_NULL(matrixTransform);
        const Matrix matrix = matrixTransform->Matrix;
        VERIFY_ARE_EQUAL(2.0, matrix.M11);
        VERIFY_ARE_EQUAL(0.0, matrix.M12);
        VERIFY_ARE_EQUAL(0.0, matrix.M21);
        VERIFY_ARE_EQUAL(4.0, matrix.M22);
        VERIFY_ARE_EQUAL(10.0, matrix.OffsetX);
        VERIFY_ARE_EQUAL(-20.0, matrix.OffsetY);

        const wf::Point transformed = matrixTransform->TransformPoint(wf::Point(3, 5));
        VERIFY_ARE_EQUAL(16.0f, transformed.X);
        VERIFY_ARE_EQUAL(0.0f, transformed.Y);

        // The inverse maps the transformed point back.
        GeneralTransform^ inverse = matrixTransform->Inverse;
        VERIFY_IS_NOT_NULL(inverse);
        const wf::Point original = inverse->TransformPoint(transformed);
        VERIFY_ARE_EQUAL(3.0f, original.X);
        VERIFY_ARE_EQUAL(5.0f, original.Y);

        // A singular transform has no inverse.
        auto singular = ref new ScaleTransform();
        singular->ScaleX = 0;
        singular->ScaleY = 2;

        bool threw = false;
        try
        {
            GeneralTransform^ singularInverse = singular->Inverse;
            (void)singularInverse;
        }
        catch (Platform::Exception^ e)
        {
            threw = true;
            VERIFY_ARE_EQUAL(E_FAIL, e->HResult);
        }
        VERIFY_IS_TRUE(threw);
    });
}

void TransformToVisualTests::PopupInProjectedParentHitTest()
{
    TestCleanupWrapper cleanup;
    const auto& wh = TestServices::WindowHelper;

    Grid^ root = nullptr;
    Primitives::Popup^ popup = nullptr;
    xaml_shapes::Rectangle^ popupChild = nullptr;

    RunOnUIThread([&]()
    {
        root = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
            L"      Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top' Background='White'>"
            L"  <Border x:Name='host' Width='200' Height='200' Margin='100,100,0,0' HorizontalAlignment='Left' VerticalAlignment='Top' Background='Blue'>"
            L"    <Border.Projection><PlaneProjection RotationY='30' LocalOffsetZ='40'/></Border.Projection>"
            L"    <Popup x:Name='popup' HorizontalOffset='50' VerticalOffset='50'>"
            L"      <Rectangle x:Name='popupChild' Width='100' Height='100' Fill='Red'/>"
            L"    </Popup>"
            L"  </Border>"
            L"</Grid>"));
        popup = safe_cast<Primitives::Popup^>(root->FindName(L"popup"));
        popupChild = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"popupChild"));
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(popup->IsOpen);

        // The popup is positioned under its parent's projection, so its transform to the root is 3D.
        GeneralTransform^ childToRoot = popupChild->TransformToVisual(nullptr);
        VERIFY_IS_NULL(dynamic_cast<MatrixTransform^>(childToRoot));

        // Hit test the content of the XamlRoot. In islands mode a subtree is required; passing the XamlRoot content
        // makes the hit test include the popups of that XamlRoot (whose popup root bounds are regenerated first).
        UIElement^ xamlRootContent = root->XamlRoot->Content;
        VERIFY_IS_NOT_NULL(xamlRootContent);

        const wf::Point projectedCenter = childToRoot->TransformPoint(wf::Point(50, 50));
        LOG_OUTPUT(L"Projected center of the popup child: (%f, %f)", projectedCenter.X, projectedCenter.Y);
        VERIFY_IS_TRUE(ContainsElement(VisualTreeHelper::FindElementsInHostCoordinates(projectedCenter, xamlRootContent), popupChild));

        // Far away from the projected host (and the popup inside it), the popup child is not hit.
        VERIFY_IS_FALSE(ContainsElement(VisualTreeHelper::FindElementsInHostCoordinates(wf::Point(20, 380), xamlRootContent), popupChild));

        popup->IsOpen = false;
    });
    wh->WaitForIdle();
}

void TransformToVisualTests::NearPlaneClippedProjectionVisibleBounds()
{
    TestCleanupWrapper cleanup;
    const auto& wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(500, 500));

    Grid^ root = nullptr;
    Canvas^ host = nullptr;
    Imaging::VirtualSurfaceImageSource^ vsis = nullptr;

    RunOnUIThread([&]()
    {
        root = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
            L"      Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'>"
            L"  <Canvas x:Name='host' Width='200' Height='200' Margin='100,100,0,0' HorizontalAlignment='Left' VerticalAlignment='Top'/>"
            L"</Grid>"));
        host = safe_cast<Canvas^>(root->FindName(L"host"));

        // The VSIS fills the host exactly, so its visible bounds are the visible part of the host in host coordinates.
        vsis = ref new Imaging::VirtualSurfaceImageSource(200, 200);
        auto brush = ref new ImageBrush();
        brush->ImageSource = vsis;
        host->Background = brush;

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    struct ProjectionCase
    {
        const wchar_t* description;
        Media3D::Matrix3D matrix;
        RECT expectedVisibleBounds;
    };
    std::vector<ProjectionCase> cases;

    RunOnUIThread([&]()
    {
        // The Matrix3DProjection maps host points (x, y, 0, 1) to (x', y', z', w') = (x, y, 0, 1) * M. Geometry with
        // z' < 0 is behind the near plane and is clipped away before dividing by w'; geometry with w' <= 0 makes the
        // projected bounds empty. x' and y' are unchanged here, so the visible part maps back to the same host coordinates.

        // Identity: the whole host is visible.
        cases.push_back({ L"identity", Media3D::Matrix3DHelper::Identity, { 0, 0, 200, 200 } });

        // z' = x - 100: the left half of the host (x < 100) is behind the near plane.
        cases.push_back({ L"left half behind the near plane", Media3D::Matrix3DHelper::FromElements(
            1, 0, 1, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, -100, 1), { 100, 0, 200, 200 } });

        // z' = 100 - x: the right half of the host (x > 100) is behind the near plane.
        cases.push_back({ L"right half behind the near plane", Media3D::Matrix3DHelper::FromElements(
            1, 0, -1, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 100, 1), { 0, 0, 100, 200 } });

        // z' = y - 50: the top quarter of the host (y < 50) is behind the near plane.
        cases.push_back({ L"top quarter behind the near plane", Media3D::Matrix3DHelper::FromElements(
            1, 0, 0, 0,
            0, 1, 1, 0,
            0, 0, 1, 0,
            0, 0, -50, 1), { 0, 50, 200, 200 } });

        // z' = x - y: the triangle x >= y is in front of the near plane. Its corners (0,0) and (200,200) lie exactly on
        // the near plane and are kept, so the visible bounds are the whole host.
        cases.push_back({ L"diagonal, corners on the near plane", Media3D::Matrix3DHelper::FromElements(
            1, 0, 1, 0,
            0, 1, -1, 0,
            0, 0, 1, 0,
            0, 0, 0, 1), { 0, 0, 200, 200 } });

        // z' = x + y - 300: only the corner triangle (200,100), (200,200), (100,200) is in front of the near plane;
        // clipping the quad leaves exactly 3 vertices.
        cases.push_back({ L"corner triangle in front of the near plane", Media3D::Matrix3DHelper::FromElements(
            1, 0, 1, 0,
            0, 1, 1, 0,
            0, 0, 1, 0,
            0, 0, -300, 1), { 100, 100, 200, 200 } });

        // (x', y', w') = (-x, -y, -1): dividing by w' would put the host back at its own position, but w' <= 0 means it
        // is behind the viewer, so nothing is visible.
        cases.push_back({ L"mirrored, negative w", Media3D::Matrix3DHelper::FromElements(
            -1, 0, 0, 0,
            0, -1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, -1), { 0, 0, 0, 0 } });

        // z' = -1: the whole host is behind the near plane.
        cases.push_back({ L"entirely behind the near plane", Media3D::Matrix3DHelper::FromElements(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, -1, 1), { 0, 0, 0, 0 } });

        // w' = -1: non-positive w, nothing is visible.
        cases.push_back({ L"negative w", Media3D::Matrix3DHelper::FromElements(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, -1), { 0, 0, 0, 0 } });
    });

    for (const auto& projectionCase : cases)
    {
        LOG_OUTPUT(L"Projection: %s", projectionCase.description);

        RunOnUIThread([&]()
        {
            auto projection = ref new Matrix3DProjection();
            projection->ProjectionMatrix = projectionCase.matrix;
            host->Projection = projection;
        });
        wh->WaitForIdle();

        RunOnUIThread([&]()
        {
            Microsoft::WRL::ComPtr<IVirtualSurfaceImageSourceNative> vsisNative;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(vsis)->QueryInterface(IID_PPV_ARGS(&vsisNative)));

            RECT visibleBounds = {};
            VERIFY_SUCCEEDED(vsisNative->GetVisibleBounds(&visibleBounds));

            const RECT& expected = projectionCase.expectedVisibleBounds;
            LOG_OUTPUT(L"Visible bounds (%d, %d, %d, %d), expected (%d, %d, %d, %d)",
                visibleBounds.left, visibleBounds.top, visibleBounds.right, visibleBounds.bottom,
                expected.left, expected.top, expected.right, expected.bottom);
            VERIFY_ARE_EQUAL(expected.left, visibleBounds.left);
            VERIFY_ARE_EQUAL(expected.top, visibleBounds.top);
            VERIFY_ARE_EQUAL(expected.right, visibleBounds.right);
            VERIFY_ARE_EQUAL(expected.bottom, visibleBounds.bottom);
        });
    }

    RunOnUIThread([&]()
    {
        host->Background = nullptr;
        vsis = nullptr;
        host = nullptr;
        root = nullptr;
    });
}

void TransformToVisualTests::PopupBehindViewerIsNotHitTested()
{
    TestCleanupWrapper cleanup;
    const auto& wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(500, 500));

    Grid^ root = nullptr;
    Border^ host = nullptr;
    Primitives::Popup^ popup = nullptr;
    xaml_shapes::Rectangle^ popupChild = nullptr;

    RunOnUIThread([&]()
    {
        root = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
            L"      Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'>"
            L"  <Border x:Name='host' Width='200' Height='200' Margin='100,100,0,0' HorizontalAlignment='Left' VerticalAlignment='Top'>"
            L"    <Popup x:Name='popup' HorizontalOffset='50' VerticalOffset='50'>"
            L"      <Rectangle x:Name='popupChild' Width='100' Height='100' Fill='Red'/>"
            L"    </Popup>"
            L"  </Border>"
            L"</Grid>"));
        host = safe_cast<Border^>(root->FindName(L"host"));
        popup = safe_cast<Primitives::Popup^>(root->FindName(L"popup"));
        popupChild = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"popupChild"));
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    auto closePopup = wil::scope_exit([&]()
    {
        RunOnUIThread([&]()
        {
            if (popup && popup->IsOpen)
            {
                popup->IsOpen = false;
            }
        });
        wh->WaitForIdle();
    });

    // The popup child covers host coordinates 50..150, i.e. 150..250 in the window. Both projections below map a host
    // point (x, y) to (x, y) after dividing by w, so the center of the popup child is always at (200, 200).
    struct ProjectionCase
    {
        const wchar_t* description;
        Media3D::Matrix3D matrix;
        bool isHit;
    };
    const ProjectionCase cases[] =
    {
        { L"identity", Media3D::Matrix3DHelper::Identity, true },
        // (x', y', z', w') = (-x, -y, 0, -1): w' < 0 for every point, so the popup's parent is behind the viewer and
        // hit testing must not find anything under it.
        { L"mirrored, negative w", Media3D::Matrix3DHelper::FromElements(
            -1, 0, 0, 0,
            0, -1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, -1), false },
        // Back to the identity: the popup child is hit again.
        { L"identity again", Media3D::Matrix3DHelper::Identity, true },
    };

    for (const auto& projectionCase : cases)
    {
        LOG_OUTPUT(L"Projection: %s", projectionCase.description);

        RunOnUIThread([&]()
        {
            auto projection = ref new Matrix3DProjection();
            projection->ProjectionMatrix = projectionCase.matrix;
            host->Projection = projection;
        });
        wh->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(popup->IsOpen);

            // In islands mode a subtree is required; the XamlRoot content includes the popups of that XamlRoot.
            UIElement^ xamlRootContent = root->XamlRoot->Content;
            VERIFY_IS_NOT_NULL(xamlRootContent);

            VERIFY_ARE_EQUAL(projectionCase.isHit,
                ContainsElement(VisualTreeHelper::FindElementsInHostCoordinates(wf::Point(200, 200), xamlRootContent), popupChild));

            // Outside the popup child, it is never hit.
            VERIFY_IS_FALSE(ContainsElement(VisualTreeHelper::FindElementsInHostCoordinates(wf::Point(120, 380), xamlRootContent), popupChild));
        });
    }

    closePopup.reset();

    RunOnUIThread([&]()
    {
        popupChild = nullptr;
        popup = nullptr;
        host = nullptr;
        root = nullptr;
    });
}

} } } } } }
