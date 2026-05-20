// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomBrushes.h"
#include "BrushTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>
#include "TreeHelper.h"
#include "FeatureFlags.h"

using namespace ::Windows::UI;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ BrushTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\graphics\\rendering\\";
}

bool BrushTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool BrushTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool BrushTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void BrushTests::ZeroSizedGradient()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ZeroSizedGradient.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"StackPanel generated a gradient when it was sized 0x0 - the gradient will be a solid color.");
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        StackPanel^ zero = safe_cast<StackPanel^>(rootCanvas->FindName(L"zero"));
        zero->Width = 200;
        zero->Height = 200;
    });

    LOG_OUTPUT(L"StackPanel now has nonzero size. It needs to generate a real gradient this time.");
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void BrushTests::ChangeSolidColorBrushColor()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    SolidColorBrush^ solidColorBrush;
    RunOnUIThread([&]()
    {
        solidColorBrush = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));

        Canvas^ square = ref new Canvas();
        square->Height = 50;
        square->Width = 50;
        square->Background = solidColorBrush;

        Canvas^ root = ref new Canvas();
        root->Children->Append(square);
        wh->WindowContent = root;
    });

    LOG_OUTPUT(L"> Render with a red brush.");
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"red");

    RunOnUIThread([&]()
    {
        solidColorBrush->Color = Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0xff, 0);
    });

    LOG_OUTPUT(L"> Red brush turns green.");
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"green");
}

void BrushTests::LinearGradientAfterDeviceLoss()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ square;
    RunOnUIThread([&]()
    {
        SolidColorBrush^ solidColorBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

        square = ref new Canvas();
        square->Height = 50;
        square->Width = 50;
        square->Background = solidColorBrush;

        Canvas^ root = ref new Canvas();
        root->Children->Append(square);
        wh->WindowContent = root;
    });

    LOG_OUTPUT(L"> Render with a solid color brush.");
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Simulate device lost and add a gradient brush. Don't crash.");
    RunOnUIThread([&]()
    {
        wh->SimulateDeviceLost();

        GradientStop^ stop1 = ref new GradientStop();
        stop1->Color = Microsoft::UI::Colors::Red;
        stop1->Offset = 0;

        GradientStop^ stop2 = ref new GradientStop();
        stop2->Color = Microsoft::UI::Colors::Green;
        stop2->Offset = 1;

        GradientStopCollection^ stopCollection = ref new GradientStopCollection();
        stopCollection->Append(stop1);
        stopCollection->Append(stop2);

        LinearGradientBrush^ brush = ref new LinearGradientBrush();
        brush->StartPoint = ::Windows::Foundation::Point(0, 0);
        brush->EndPoint = ::Windows::Foundation::Point(1, 0);
        brush->GradientStops = stopCollection;

        square->Background = brush;
    });
    wh->WaitForIdle();
}

Microsoft::UI::Xaml::Controls::Panel^ BrushTests::CreateLinearGradientTestRoot(LinearGradientBrush^ brush)
{
    StackPanel^ root = ref new StackPanel();

    Grid^ grid = ref new Grid();
    grid->Height = 50;
    grid->Width = 200;
    grid->Background = brush;
    root->Children->Append(grid);

    Microsoft::UI::Xaml::Shapes::Ellipse^ ellispe = ref new Microsoft::UI::Xaml::Shapes::Ellipse();
    ellispe->Height = 50;
    ellispe->Width = 200;
    ellispe->Fill = brush;
    root->Children->Append(ellispe);

    TextBlock^ textblock = ref new TextBlock();
    textblock->Text = L"Paris in the Spring. New England in the Fall.";
    textblock->FontSize = 30;
    textblock->Foreground = brush;
    textblock->TextWrapping = Microsoft::UI::Xaml::TextWrapping::Wrap;
    root->Children->Append(textblock);

    RichTextBlock^ richTextBlock = ref new RichTextBlock();
    richTextBlock->Foreground = brush;
    richTextBlock->FontSize = 30;
    richTextBlock->TextWrapping = Microsoft::UI::Xaml::TextWrapping::Wrap;
    root->Children->Append(richTextBlock);

    Paragraph^ paragraph = ref new Paragraph();
    richTextBlock->Blocks->Append(paragraph);

    Run^ textRun1 = ref new Run();
    textRun1->FontSize = 40;
    textRun1->Text = "Paris ";
    paragraph->Inlines->Append(textRun1);

    Run^ textRun2 = ref new Run();
    textRun2->Text = "in the spring. New England in the ";
    paragraph->Inlines->Append(textRun2);

    Run^ textRun3 = ref new Run();
    textRun3->FontSize = 40;
    textRun3->Text = "Fall.";
    paragraph->Inlines->Append(textRun3);

    TestServices::WindowHelper->WindowContent = root;

    return root;
}

#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)

// TODO:  Dynamically enable velocity instead of skipping test
#define VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS if (!Feature_XamlMotionSystemHoldbacks::IsEnabled()) { LOG_OUTPUT(L"Motions holdbacks velocity feature disabled, skipping test"); return; }

void BrushTests::LinearGradientFacadeProperties()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    const auto& wh = TestServices::WindowHelper;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    LinearGradientBrush^ brush;

    LOG_OUTPUT(L"Creating Test elements");
    RunOnUIThread([&]()
    {
        brush = MakeLinearGradientBrush(0.0f, 1.0f);
        StackPanel^ root = ref new StackPanel();
        CreateLinearGradientTestRoot(brush);
    });
    wh->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"legacy");

    LOG_OUTPUT(L"Converting to Facade mode");
    RunOnUIThread([&]()
    {
        brush->Rotation = 0;
    });
    wh->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"facade");

    LOG_OUTPUT(L"Setting Brush Facade Properties");
    RunOnUIThread([&]()
    {
        brush->Rotation = 45;
        brush->CenterPoint = ::Windows::Foundation::Numerics::float2(50.0f, 100.0f);
        brush->Scale = ::Windows::Foundation::Numerics::float2(2.0f, 3.0f);
        brush->Translation = ::Windows::Foundation::Numerics::float2(25.0f, 75.0f);
        brush->TransformMatrix = ::Windows::Foundation::Numerics::float3x2(.05f, 0.0f, 0.0f, 0.8f, -50.0f, -25.0f);
    });
    wh->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"facadeproperties");
}

void BrushTests::LinearGradientFacadeAnimations()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    const auto& wh = TestServices::WindowHelper;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Panel^ root;
    LinearGradientBrush^ brush;

    LOG_OUTPUT(L"Creating Test elements");
    RunOnUIThread([&]()
    {
        brush = MakeLinearGradientBrush(0.0f, 1.0f);
        root = CreateLinearGradientTestRoot(brush);
        TestServices::WindowHelper->WindowContent = root;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"Creating animations");
    std::vector<Microsoft::UI::Composition::KeyFrameAnimation^> animations;
    RunOnUIThread([&]()
    {
        Compositor^ compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetElementVisual(root)->Compositor;
        // We use an extremely long timespan (10 years) so that the animation won't actually move values
        ::Windows::Foundation::TimeSpan duration = { 60L * 1000L * 10000L };
        {
            auto animation = compositor->CreateScalarKeyFrameAnimation();
            animation->InsertKeyFrame(0.0, 45);
            animation->InsertKeyFrame(1.0, 45);
            animation->Duration = duration;
            animation->Target = "Rotation";
            animations.emplace_back(animation);
        }
        {
            auto animation = compositor->CreateVector2KeyFrameAnimation();
            animation->InsertKeyFrame(0.0, ::Windows::Foundation::Numerics::float2(50.0f, 100.0f));
            animation->InsertKeyFrame(1.0, ::Windows::Foundation::Numerics::float2(50.0f, 100.0f));
            animation->Duration = duration;
            animation->Target = "CenterPoint";
            animations.emplace_back(animation);
        }
        {
            auto animation = compositor->CreateVector2KeyFrameAnimation();
            animation->InsertKeyFrame(0.0, ::Windows::Foundation::Numerics::float2(2.0f, 3.0f));
            animation->InsertKeyFrame(1.0, ::Windows::Foundation::Numerics::float2(2.0f, 3.0f));
            animation->Duration = duration;
            animation->Target = "Scale";
            animations.emplace_back(animation);
        }
        {
            // Animate a subchannel so we test the subchannels of an aliased property name.
            auto animation = compositor->CreateScalarKeyFrameAnimation();
            animation->InsertKeyFrame(0.0, 25.0f);
            animation->InsertKeyFrame(1.0, 25.0f);
            animation->Duration = duration;
            animation->Target = "Translation.x";
            animations.emplace_back(animation);
        }
        {
            auto animation = compositor->CreateVector2KeyFrameAnimation();
            animation->InsertKeyFrame(0.0, ::Windows::Foundation::Numerics::float2(-50.0f, -25.0f));
            animation->InsertKeyFrame(1.0, ::Windows::Foundation::Numerics::float2(-50.0f, -25.0f));
            animation->Duration = duration;
            animation->Target = "TransformMatrix._31_32";
            animations.emplace_back(animation);
        }
    });

    LOG_OUTPUT(L"Starting Animations");
    RunOnUIThread([&]()
    {
        for (KeyFrameAnimation^ animation : animations)
        {
            brush->StartAnimation(animation);
        }
    });

    LOG_OUTPUT(L"Validating animating comp tree");
    wh->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"animating");

    LOG_OUTPUT(L"Setting Facade Properties to cancel animations");
    RunOnUIThread([&]()
    {
        brush->Rotation = 0;
        brush->CenterPoint = ::Windows::Foundation::Numerics::float2(0.0f, 0.0f);
        brush->Scale = ::Windows::Foundation::Numerics::float2(1.0f, 1.0f);
        brush->Translation = ::Windows::Foundation::Numerics::float2(0.0f, 0.0f);
        brush->TransformMatrix = ::Windows::Foundation::Numerics::float3x2(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"Validating reset comp tree");
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"reset");

    LOG_OUTPUT(L"Starting Animations again as a group");
    RunOnUIThread([&]()
    {
        Compositor^ compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetElementVisual(root)->Compositor;
        auto animationGroup = compositor->CreateAnimationGroup();
        for (KeyFrameAnimation^ animation : animations)
        {
            animationGroup->Add(animation);
        }
        brush->StartAnimation(animationGroup);
    });
    LOG_OUTPUT(L"Validating animating comp tree");
    wh->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"animatinggroup");

    LOG_OUTPUT(L"Stopping Animations");
    RunOnUIThread([&]()
    {
        for (KeyFrameAnimation^ animation : animations)
        {
            brush->StopAnimation(animation);
        }
    });
    wh->SynchronouslyTickUIThread(4);

    wh->WaitForIdle();
    LOG_OUTPUT(L"Validating stopped comp tree");
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"stopped");
}

#endif

ref class MyBrush sealed : public Microsoft::UI::Xaml::Media::XamlCompositionBrushBase
{
public:
    MyBrush()
    {
        m_compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
    }

protected:
    void OnConnected() override
    {
        if (m_brush == nullptr)
        {
            m_brush = m_compositor->CreateColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xFF, 0xFF, 0x00, 0x00));
        }
        this->CompositionBrush = m_brush;
    }

    void OnDisconnected() override
    {
        this->CompositionBrush = nullptr;
        delete m_brush;
        m_brush = nullptr;
    }

    void PopulatePropertyInfoOverride(Platform::String^ propertyName, AnimationPropertyInfo^ animationPropertyInfo) override
    {
        if (propertyName == L"FooBazProperty")
        {
            return m_brush->PopulatePropertyInfo(L"Color", animationPropertyInfo);
        }

        return m_brush->PopulatePropertyInfo(propertyName, animationPropertyInfo);
    }

private:
    Compositor^ m_compositor = nullptr;
    CompositionColorBrush^ m_brush = nullptr;
};

// Gets and sets the Window's SystemBackdrop brush before and after DCompTreeHost's full setup, then resets it.
void BrushTests::UseWindowCompositionSystemBackdrop()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    ::Windows::UI::Composition::Compositor^ compositor;
    ::Windows::UI::Composition::CompositionColorBrush^ compositionColorBrush;
    ::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop^ compositionSupportsSystemBackdrop;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Accessing ICompositionSupportsSystemBackdrop implementation");
        compositionSupportsSystemBackdrop = safe_cast<::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop^>(Window::Current);
        VERIFY_IS_NOT_NULL(compositionSupportsSystemBackdrop);

        auto canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->HorizontalAlignment = xaml::HorizontalAlignment::Left;
        canvas->VerticalAlignment = xaml::VerticalAlignment::Top;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xAA, 0x00, 0xFF, 0x00));

        wh->WindowContent = canvas;

        // Note: We don't have access to the correct Windows.UI.Composition.Compositor to create a brush
        // which can actually draw. But we can create a compositor which should be enough for this test.
        LOG_OUTPUT(L"Creating custom CompositionColorBrush");
        compositor = ref new ::Windows::UI::Composition::Compositor();
        compositionColorBrush = compositor->CreateColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xAA, 0xFF, 0x00, 0x00));

        LOG_OUTPUT(L"Setting SystemBackdrop with custom CompositionColorBrush");
        // Early Window::put_SystemBackdrop call before DCompTreeHost::m_pTargetForHwnd is set causing the brush to be temporarily cached.
        compositionSupportsSystemBackdrop->SystemBackdrop = compositionColorBrush;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Reading back custom CompositionColorBrush");
        auto compositionBrushRead = compositionSupportsSystemBackdrop->SystemBackdrop;
        VERIFY_ARE_EQUAL(compositionBrushRead, compositionColorBrush);

        LOG_OUTPUT(L"Creating second custom CompositionColorBrush");
        compositionColorBrush = compositor->CreateColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xAA, 0x00, 0x00, 0xFF));

        LOG_OUTPUT(L"Setting SystemBackdrop with second custom CompositionColorBrush");
        compositionSupportsSystemBackdrop->SystemBackdrop = compositionColorBrush;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    // TODO: Re-enable this once IXP has completed the fix.
    // Currently, CoreWindowSiteBridge::put_SystemBackdrop throws E_INVALIDARG when setting to nullptr.
    /*RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Resetting SystemBackdrop");
        compositionSupportsSystemBackdrop->SystemBackdrop = nullptr;
    });
    wh->WaitForIdle();*/

    // Note that compared to the couple snapshots above this last snapshot contains an extra visual
    // for the emergency background which was added back when SystemBackdrop was reset.
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

// Gets and resets the Window's already null SystemBackdrop brush.
void BrushTests::UseNullWindowCompositionSystemBackdrop()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    ::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop^ compositionSupportsSystemBackdrop;

    RunOnUIThread([&]()
    {
        auto canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->HorizontalAlignment = xaml::HorizontalAlignment::Left;
        canvas->VerticalAlignment = xaml::VerticalAlignment::Top;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xAA, 0x00, 0xFF, 0x00));

        wh->WindowContent = canvas;

        LOG_OUTPUT(L"Accessing ICompositionSupportsSystemBackdrop implementation");
        compositionSupportsSystemBackdrop = safe_cast<::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop^>(Window::Current);
        VERIFY_IS_NOT_NULL(compositionSupportsSystemBackdrop);

        LOG_OUTPUT(L"Accessing SystemBackdrop");
        // Early Window::get_SystemBackdrop call before DCompTreeHost::m_pTargetForHwnd is set
        auto compositionBrushRead = compositionSupportsSystemBackdrop->SystemBackdrop;
        VERIFY_IS_NULL(compositionBrushRead);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    // TODO: Re-enable this once IXP has completed the fix.
    // Currently, CoreWindowSiteBridge::put_SystemBackdrop throws E_INVALIDARG when setting to nullptr.
    /*RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Resetting SystemBackdrop");
        compositionSupportsSystemBackdrop->SystemBackdrop = nullptr;
    });
    wh->WaitForIdle();*/
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}


void BrushTests::PopulatePropertyInfoOverride()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    MyBrush^ myBrush;
    Compositor^ compositor;
    CompositionColorBrush^ colorBrush;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        myBrush = ref new MyBrush();
        canvas->Background = myBrush;

        wh->WindowContent = canvas;

        compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
        colorBrush = compositor->CreateColorBrush();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;
        animation = compositor->CreateExpressionAnimation(L"brush.FooBazProperty");
        animation->SetExpressionReferenceParameter(L"brush", myBrush);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        colorBrush->StartAnimation(L"Color", animation);
        scopedBatch->End();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        colorBrush->StopAnimation(L"Color");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        ::Windows::UI::Color color = colorBrush->Color;
        LOG_OUTPUT(L"Color is [%d,%d,%d,%d]", color.A, color.R, color.G, color.B);
        VERIFY_ARE_EQUAL(color.A, 255);
        VERIFY_ARE_EQUAL(color.R, 255);
        VERIFY_ARE_EQUAL(color.G, 0);
        VERIFY_ARE_EQUAL(color.B, 0);
    });
    wh->WaitForIdle();
}

void BrushTests::XamlCompositionBrushChangeBrush()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    SeahawksColorsBrush^ xcb = nullptr;

    LOG_OUTPUT(L"Use original brush - blue.");
    RunOnUIThread([&]()
    {
        xcb = ref new SeahawksColorsBrush();

        Canvas^ canvas1 = ref new Canvas();
        canvas1->Width = 400;
        canvas1->Height = 300;
        canvas1->Background = xcb;

        wh->WindowContent = canvas1;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"original");

    LOG_OUTPUT(L"Use alternate brush - green.");
    RunOnUIThread([&]()
    {
        xcb->UseAlternateColor  = true;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"alternate");
}

void BrushTests::XamlCompositionBrushFallbackColor()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    RunOnUIThread([&]()
    {
        XcbPurpleBrush^ xcb = ref new XcbPurpleBrush();
        xcb->FallbackColor = Microsoft::UI::Colors::Red;

        Canvas^ canvas1 = ref new Canvas();
        canvas1->CompositeMode = ElementCompositeMode::SourceOver;
        canvas1->CacheMode = ref new BitmapCache(); // This is now a no-op, but left in to check that it doesn't cause problems when set
        canvas1->Width = 400;
        canvas1->Height = 300;
        canvas1->Background = xcb;

        wh->WindowContent = canvas1;
    });

    wh->WaitForIdle();

    LOG_OUTPUT(L"Use FallbackColor - red.");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}


void BrushTests::XamlCompositionBrushTransforms()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    XcbImageBrush^ xcb = nullptr;
    Canvas^ canvas1 = nullptr;
    xaml_shapes::Rectangle^ rectangle1 = nullptr;

    RunOnUIThread([&]()
    {
        xcb = ref new XcbImageBrush();
        canvas1 = ref new Canvas();

        rectangle1 = ref new xaml_shapes::Rectangle();
        rectangle1->Height = 150;            // Natural height of barcelona.jpg
        rectangle1->Width = 266;             // Natural width of barcelona.jpg
        rectangle1->Fill = xcb;

        canvas1->Children->Append(rectangle1);

        wh->WindowContent = canvas1;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"original");

    LOG_OUTPUT(L"Setting RenderTransform on canvas filled with XCBB");
    RunOnUIThread([&]()
    {
        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        scale->ScaleY = 2;
        rectangle1->RenderTransform = scale;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"TransformElement");

    LOG_OUTPUT(L"Setting XCBB.Tranform (to a scale) - should throw");
    RunOnUIThread([&]()
    {
        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        scale->ScaleY = 2;

        bool testPass = false;
        try
        {
            xcb->Transform = scale;
        }
        catch(Platform::Exception^ e)
        {
            VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
            testPass = true;
        }
        VERIFY_IS_TRUE(testPass);
    });

    wh->WaitForIdle();

    LOG_OUTPUT(L"Setting XCBB.RelativeTransform (to a translation) - should throw");
    RunOnUIThread([&]()
    {
        TranslateTransform^ translate = ref new TranslateTransform();
        translate->X = 100;
        translate->Y = -100;

        bool testPass = false;
        try
        {
            xcb->RelativeTransform = translate;
        }
        catch(Platform::Exception^ e)
        {
            VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
            testPass = true;
        }
        VERIFY_IS_TRUE(testPass);
    });

    wh->WaitForIdle();
}

void BrushTests::XamlCompositionBrushNullBrush()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    XcbNullableBrush^ xcb = nullptr;

    RunOnUIThread([&]()
    {
        xcb = ref new XcbNullableBrush();
        xcb->FallbackColor = Microsoft::UI::Colors::Red;

        Canvas^ canvas1 = ref new Canvas();
        canvas1->Width = 400;
        canvas1->Height = 300;
        canvas1->Background = xcb;

        wh->WindowContent = canvas1;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"original");

    LOG_OUTPUT(L"Nulling out the XCBB");
    RunOnUIThread([&]()
    {
        xcb->NullBrush();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"null");
}


void BrushTests::XamlCompositionBrushShapesAndControls()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XcbbApplications.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    XcbPurpleBrush^ xcbPurpleBrush = nullptr;
    XcbImageBrush^ xcbImageBrush = nullptr;
    SeahawksColorsBrush^ xcbSeahawksColorsBrush = nullptr;
    SeahawksColorsBrush^ xcbSeahawksColorsBrush2 = nullptr;
    xaml_shapes::Ellipse^ ellipse1 = nullptr;
    TextBlock^ textBlock1 = nullptr;
    Border^ border1 = nullptr;
    Button^ button1 = nullptr;
    ListViewItem^ lvi1 = nullptr;
    ListViewItem^ lvi2 = nullptr;

    RunOnUIThread([&]()
    {
        xcbPurpleBrush = ref new XcbPurpleBrush();
        xcbImageBrush = ref new XcbImageBrush();
        xcbSeahawksColorsBrush = ref new SeahawksColorsBrush();
        xcbSeahawksColorsBrush2 = ref new SeahawksColorsBrush();

        ellipse1 = safe_cast<xaml_shapes::Ellipse^>(rootCanvas->FindName(L"Ellipse1"));
        ellipse1->Fill = xcbImageBrush;
        ellipse1->Stroke = xcbPurpleBrush;

        // Share xcbPurpleBrush between ellipse and textblock ...
        textBlock1 = safe_cast<TextBlock^>(rootCanvas->FindName(L"TextBlock1"));
        textBlock1->Foreground = xcbPurpleBrush;

        // Share xcbImageBrush between ellipse and border ...
        // Share xcbPurpleBrush between ellipse, textblock, and border ...
        border1 = safe_cast<Border^>(rootCanvas->FindName(L"Border1"));
        border1->Background = xcbImageBrush;
        border1->BorderBrush = xcbSeahawksColorsBrush;

        xcbSeahawksColorsBrush2->UseAlternateColor = true;
        button1 = safe_cast<Button^>(rootCanvas->FindName(L"Button1"));
        button1->Background = xcbSeahawksColorsBrush;
        button1->BorderBrush = xcbSeahawksColorsBrush2;

        lvi1 = safe_cast<ListViewItem^>(rootCanvas->FindName(L"ListViewItem1"));
        lvi1->BorderThickness = ThicknessHelper::FromUniformLength(3);
        lvi1->BorderBrush = xcbPurpleBrush;

        lvi2 = safe_cast<ListViewItem^>(rootCanvas->FindName(L"ListViewItem2"));
        lvi2->BorderThickness = ThicknessHelper::FromUniformLength(3);
        lvi2->BorderBrush = MakeLinearGradientBrush(0.0f, 1.0f);
    });

    LOG_OUTPUT(L"Use and share a collection of several XCBB's to paint various Xaml elements");
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}


void BrushTests::XamlCompositionBrushListViewItem_PlateauScale()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    wh->SetWindowSizeOverrideWithWindowScale(wf::Size(400, 300), 2.0f);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XcbbApplications.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    RunOnUIThread([&]()
    {
        auto xcbPurpleBrush = ref new XcbPurpleBrush();

        auto ellipse1 = safe_cast<xaml_shapes::Ellipse^>(rootCanvas->FindName(L"Ellipse1"));
        ellipse1->Visibility = Visibility::Collapsed;

        auto textBlock1 = safe_cast<TextBlock^>(rootCanvas->FindName(L"TextBlock1"));
        textBlock1->Visibility = Visibility::Collapsed;

        auto border1 = safe_cast<Border^>(rootCanvas->FindName(L"Border1"));
        border1->Visibility = Visibility::Collapsed;

        auto button1 = safe_cast<Button^>(rootCanvas->FindName(L"Button1"));
        button1->Visibility = Visibility::Collapsed;

        auto lvi1 = safe_cast<ListViewItem^>(rootCanvas->FindName(L"ListViewItem1"));
        lvi1->BorderThickness = ThicknessHelper::FromUniformLength(3);
        lvi1->BorderBrush = xcbPurpleBrush;

        auto lvi2 = safe_cast<ListViewItem^>(rootCanvas->FindName(L"ListViewItem2"));
        lvi2->Visibility = Visibility::Collapsed;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void BrushTests::XamlCompositionBrushAnimation()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    XcbPurpleBrush^ xcbPurpleBrush = nullptr;
    XcbImageBrush^ xcbImageBrush = nullptr;
    ScaleTransform^ scale = nullptr;
    Storyboard^ storyboard = nullptr;
    Canvas^ canvas1 = nullptr;
    xaml_shapes::Rectangle^ rectangle1 = nullptr;

    RunOnUIThread([&]()
    {
        xcbPurpleBrush = ref new XcbPurpleBrush();
        xcbImageBrush = ref new XcbImageBrush();

        scale = ref new ScaleTransform();

        canvas1 = ref new Canvas();
        rectangle1 = ref new xaml_shapes::Rectangle();
        rectangle1->Height = 100;
        rectangle1->Width = 100;
        rectangle1->StrokeThickness = 5;
        rectangle1->Stroke = xcbPurpleBrush;
        rectangle1->Fill = xcbImageBrush;
        rectangle1->RenderTransform = scale;
        canvas1->Children->Append(rectangle1);

        ::Windows::Foundation::TimeSpan span;
        span.Duration = 10000000L;    // 1 second

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 1.0;
        da->To = 3.0;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, scale);
        Storyboard::SetTargetProperty(da, L"ScaleX");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        wh->WindowContent = canvas1;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"original");

    LOG_OUTPUT(L"Start animation of rectangle painted with XCBB's");
    RunOnUIThread([&]()
    {
        storyboard->Begin();
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Animated");
}

void BrushTests::XamlCompositionBrushOnConnected()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    XcbPurpleBrush^ xcbb = nullptr;
    Canvas^ root = nullptr;
    xaml_shapes::Rectangle^ rect = nullptr;
    xaml_shapes::Rectangle^ rect2 = nullptr;

    RunOnUIThread([&]()
    {
        xcbb = ref new XcbPurpleBrush();

        rect = ref new xaml_shapes::Rectangle();
        rect->Height = 100;
        rect->Width = 100;
        rect->Fill = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));

        rect2 = ref new xaml_shapes::Rectangle();
        rect2->Height = 100;
        rect2->Width = 100;
        rect2->Fill = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));

        root = ref new Canvas();
        root->Children->Append(rect);
        root->Children->Append(rect2);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L">>> [Add to a ResourceDictionary, then remove from the ResourceDictionary].");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Add XCBB to a ResourceDictionary.");
        root->Resources->Insert(L"brush", xcbb);
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnConnected should not be called when added to a ResourceDictionary.");
    VERIFY_IS_FALSE(xcbb->IsConnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Remove XCBB from the ResourceDictionary.");
        root->Resources->Clear();
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnDisconnected should not be called when OnConnected hasn't been called.");
    VERIFY_IS_FALSE(xcbb->IsDisconnectedCalled());
    xcbb->ResetCallFlags();

    LOG_OUTPUT(L">>> [Add to a ResourceDictionary and two elements, then remove from the ResourceDictionary and two elements].");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Add XCBB to a ResourceDictionary.");
        root->Resources->Insert(L"brush", xcbb);
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnConnected should not be called when added to a ResourceDictionary, if it's not already attached to an element.");
    VERIFY_IS_FALSE(xcbb->IsConnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Add XCBB to an element.");
        rect->Fill = xcbb;
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnConnected should be called when first added to an element.");
    VERIFY_IS_TRUE(xcbb->IsConnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Add XCBB to a second element.");
        rect2->Fill = xcbb;
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnConnected should not be called when added to another element.");
    VERIFY_IS_FALSE(xcbb->IsConnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Remove XCBB from the ResourceDictionary.");
        root->Resources->Clear();
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnDisconnected should not be called when still connected to an element.");
    VERIFY_IS_FALSE(xcbb->IsDisconnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Remove XCBB from one element.");
        rect->Fill = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnDisconnected should not be called when still connected to an element.");
    VERIFY_IS_FALSE(xcbb->IsDisconnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Remove XCBB from the second element.");
        rect2->Fill = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnDisconnected should be called when disconnected from the last parent.");
    VERIFY_IS_TRUE(xcbb->IsDisconnectedCalled());
    xcbb->ResetCallFlags();

    LOG_OUTPUT(L">>> [Add to two elements and two ResourceDictionaries, then remove from two elements and two ResourceDictionaries].");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Add XCBB to an element.");
        rect->Fill = xcbb;
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnConnected should be called when added to an element.");
    VERIFY_IS_TRUE(xcbb->IsConnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Add XCBB to a ResourceDictionary.");
        root->Resources->Insert(L"brush", xcbb);
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnConnected should not be called when added to a ResourceDictionary.");
    VERIFY_IS_FALSE(xcbb->IsConnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Add XCBB to a second element.");
        rect2->Fill = xcbb;
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnConnected should not be called when added to another element.");
    VERIFY_IS_FALSE(xcbb->IsConnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Add XCBB to a second ResourceDictionary.");
        rect2->Resources->Insert(L"brush", xcbb);
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnConnected should not be called when added to a ResourceDictionary.");
    VERIFY_IS_FALSE(xcbb->IsConnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Remove XCBB from one element.");
        rect2->Fill = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnDisconnected should not be called when still connected to an element.");
    VERIFY_IS_FALSE(xcbb->IsDisconnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Remove XCBB from the second element.");
        rect->Fill = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnDisconnected should not be immediately called when disconnected from the last element.");
    VERIFY_IS_FALSE(xcbb->IsDisconnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Remove XCBB from one ResourceDictionary.");
        root->Resources->Clear();
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnDisconnected should be called after having no more UIElement parents.");
    VERIFY_IS_TRUE(xcbb->IsDisconnectedCalled());
    xcbb->ResetCallFlags();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Remove XCBB from the second ResourceDictionary.");
        rect2->Resources->Clear();
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> XCBB::OnDisconnected should not be called after having been called before.");
    VERIFY_IS_FALSE(xcbb->IsDisconnectedCalled());
    xcbb->ResetCallFlags();
}

void BrushTests::SolidColorBrushTransition()
{
    const auto& windowHelper = TestServices::WindowHelper;
    const auto& utilities = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Grid^ grid = nullptr;
    Grid^ gridRoundedCorner = nullptr;
    Border^ border = nullptr;
    Border^ borderRoundedCorner = nullptr;
    ContentPresenter^ contentPresenter = nullptr;
    ContentPresenter^ contentPresenterRoundedCorner = nullptr;
    Border^ borderRoundedCornerGoingTransparent = nullptr;
    Border^ borderRoundedCornerComingFromTransparent = nullptr;
    SolidColorBrush^ blueBrush = nullptr;

    RunOnUIThread([&]()
    {
        blueBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

        grid = ref new Grid();
        grid->Height = 50;
        grid->Width = 50;
        grid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        gridRoundedCorner = ref new Grid();
        gridRoundedCorner->Height = 50;
        gridRoundedCorner->Width = 50;
        gridRoundedCorner->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        gridRoundedCorner->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(5);

        border = ref new Border();
        border->Height = 75;
        border->Width = 75;
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        borderRoundedCorner = ref new Border();
        borderRoundedCorner->Height = 75;
        borderRoundedCorner->Width = 75;
        borderRoundedCorner->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        borderRoundedCorner->BorderBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        borderRoundedCorner->BorderThickness = xaml::Thickness({ 3,3,3,3 });
        borderRoundedCorner->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(5);

        contentPresenter = ref new ContentPresenter();
        contentPresenter->Height = 100;
        contentPresenter->Width = 100;
        contentPresenter->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        contentPresenterRoundedCorner = ref new ContentPresenter();
        contentPresenterRoundedCorner->Height = 100;
        contentPresenterRoundedCorner->Width = 100;
        contentPresenterRoundedCorner->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        contentPresenterRoundedCorner->BorderBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        contentPresenterRoundedCorner->BorderThickness = xaml::Thickness({ 3,3,3,3 });
        contentPresenterRoundedCorner->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(5);

        borderRoundedCornerGoingTransparent = ref new Border();
        borderRoundedCornerGoingTransparent->Height = 125;
        borderRoundedCornerGoingTransparent->Width = 125;
        borderRoundedCornerGoingTransparent->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        borderRoundedCornerGoingTransparent->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(5);

        borderRoundedCornerComingFromTransparent = ref new Border();
        borderRoundedCornerComingFromTransparent->Height = 150;
        borderRoundedCornerComingFromTransparent->Width = 150;
        borderRoundedCornerComingFromTransparent->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0x0, 0xff, 0, 0));
        borderRoundedCornerComingFromTransparent->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(5);

        Canvas^ blueCanvas = ref new Canvas();
        blueCanvas->Height = 25;
        blueCanvas->Width = 25;
        blueCanvas->Background = blueBrush;

        Canvas^ root = ref new Canvas();
        root->Children->Append(contentPresenter);
        root->Children->Append(contentPresenterRoundedCorner);
        root->Children->Append(border);
        root->Children->Append(borderRoundedCorner);
        root->Children->Append(grid);
        root->Children->Append(gridRoundedCorner);
        root->Children->Append(borderRoundedCornerGoingTransparent);
        root->Children->Append(borderRoundedCornerComingFromTransparent);
        root->Children->Append(blueCanvas);
        windowHelper->WindowContent = root;
    });
    windowHelper->SynchronouslyTickUIThread(2);
    LOG_OUTPUT(L"> No BrushTransition, so this should be a hard cut.");
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"red");

    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding BrushTransition, so this should be animated.");
        BrushTransition^ defaultDuration = ref new BrushTransition();
        grid->BackgroundTransition = defaultDuration;
        grid->Background = blueBrush;
        gridRoundedCorner->BackgroundTransition = defaultDuration;
        gridRoundedCorner->Background = blueBrush;

        LOG_OUTPUT(L"> Casting BrushTransition to IInspectable.");
        IInspectable* inspectable = reinterpret_cast<IInspectable*>(defaultDuration);

        {
            LOG_OUTPUT(L"  > QI to DependencyObject. This should fail.");
            void* dependencyObject = nullptr;
            HRESULT castHR = inspectable->QueryInterface(__uuidof(DependencyObject^), &dependencyObject);
            VERIFY_ARE_EQUAL(E_NOINTERFACE, castHR);
            VERIFY_IS_NULL(dependencyObject);
        }

        {
            LOG_OUTPUT(L"  > QI to IDependencyObject. This should fail.");
            void* dependencyObject = nullptr;
            HRESULT castHR = inspectable->QueryInterface(__uuidof(IDependencyObject^), &dependencyObject);
            VERIFY_ARE_EQUAL(E_NOINTERFACE, castHR);
            VERIFY_IS_NULL(dependencyObject);
        }

        BrushTransition^ explicitDuration = ref new BrushTransition();
        explicitDuration->Duration = { 1000L * 10000L };
        border->BackgroundTransition = explicitDuration;
        border->Background = blueBrush;
        borderRoundedCorner->BackgroundTransition = explicitDuration;
        borderRoundedCorner->Background = blueBrush;
        borderRoundedCornerGoingTransparent->BackgroundTransition = explicitDuration;
        borderRoundedCornerGoingTransparent->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0x0, 0x0, 0xff, 0));
        borderRoundedCornerComingFromTransparent->BackgroundTransition = explicitDuration;
        borderRoundedCornerComingFromTransparent->Background = blueBrush;

        // Change the duration. It shouldn't affect border's animation above - that one is already in flight.
        explicitDuration->Duration = { 500L * 10000L };
        contentPresenter->BackgroundTransition = explicitDuration;
        contentPresenter->Background = blueBrush;
        contentPresenterRoundedCorner->BackgroundTransition = explicitDuration;
        contentPresenterRoundedCorner->Background = blueBrush;
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"blue");
}

void BrushTests::ListViewItemPresenterBrushTransition()
{
    const auto& windowHelper = TestServices::WindowHelper;
    const auto& utilities = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListViewBrushTransition.xaml"));
    ListView^ listView = nullptr;
    ListViewItem^ lvi = nullptr;
    ListViewItemPresenter^ lvip = nullptr;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting window content to ListView.");
        windowHelper->WindowContent = root;
    });
    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Looking for ListView.");
        listView = safe_cast<ListView^>(root->FindName(L"listView"));
        VERIFY_IS_NOT_NULL(listView);
        LOG_OUTPUT(L"> ListView found.");

        LOG_OUTPUT(L"> Looking for ListViewItem.");
        lvi = TreeHelper::GetVisualChildByType<ListViewItem>(listView);
        VERIFY_IS_NOT_NULL(lvi);
        LOG_OUTPUT(L"> ListViewItem found.");

        LOG_OUTPUT(L"> Looking for ListViewItemPresenter.");
        lvip = TreeHelper::GetVisualChildByType<ListViewItemPresenter>(lvi);
        VERIFY_IS_NOT_NULL(lvip);
        LOG_OUTPUT(L"> ListViewItemPresenter found.");
    });
    windowHelper->WaitForIdle();

    LOG_OUTPUT(L"> Changing states. There's no BrushTransition, so this should be a hard cut.");

    RunOnUIThread([&]()
    {
        VisualStateManager::GoToState(lvi, "PointerOver", false);
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Red");

    RunOnUIThread([&]()
    {
        VisualStateManager::GoToState(lvi, "Pressed", false);
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Orange");

    RunOnUIThread([&]()
    {
        VisualStateManager::GoToState(lvi, "Normal", false);
    });
    windowHelper->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding a BrushTransition to the ListViewItemPresenter.");
        BrushTransition^ explicitDuration = ref new BrushTransition();
        explicitDuration->Duration = { 500L * 10000L };
        lvip->BackgroundTransition = explicitDuration;
    });
    windowHelper->WaitForIdle();

    LOG_OUTPUT(L"> Changing states. This should play brush transitions.");

    RunOnUIThread([&]()
    {
        VisualStateManager::GoToState(lvi, "PointerOver", false);
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ToRed");
    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VisualStateManager::GoToState(lvi, "Pressed", false);
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ToOrange");
    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VisualStateManager::GoToState(lvi, "Selected", false);
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ToYellow");
    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VisualStateManager::GoToState(lvi, "PointerOverSelected", false);
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ToGreen");
    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VisualStateManager::GoToState(lvi, "PressedSelected", false);
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ToBlue");
    windowHelper->WaitForIdle();

    LOG_OUTPUT(L"> Changing brushes on a ListViewItemPresenter. This should play brush transitions.");

    RunOnUIThread([&]()
    {
        lvip->SelectedPressedBackground = ref new SolidColorBrush(Microsoft::UI::Colors::Purple);
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ToPurple");
}

void BrushTests::BrushTransition_Handoff()
{
    const auto& windowHelper = TestServices::WindowHelper;
    const auto& utilities = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ canvas = nullptr;
    SolidColorBrush^ greenBrush = nullptr;
    SolidColorBrush^ yellowBrush = nullptr;
    SolidColorBrush^ redBrush = nullptr;
    BrushTransition^ brushTransition = nullptr;

    RunOnUIThread([&]()
    {
        greenBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        yellowBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);
        redBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds

        brushTransition = ref new BrushTransition();
        brushTransition->Duration = span;

        canvas = ref new Canvas();
        canvas->Height = 50;
        canvas->Width = 50;
        canvas->Background = greenBrush;

        Canvas^ root = ref new Canvas();
        root->Children->Append(canvas);
        windowHelper->WindowContent = root;
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"g");

    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding BrushTransition, so this should be animated.");

        canvas->BackgroundTransition = brushTransition;
        canvas->Background = yellowBrush;
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"gy");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Going to red. This should be a handoff.");

        canvas->Background = redBrush;
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"r");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing BrushTransition, then hard cut back to green. This should clear out any running brush animations.");

        canvas->BackgroundTransition = nullptr;
        canvas->Background = greenBrush;
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"g");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding BrushTransition and switching brushes twice in a row. This should be an explicit animation that skips the intermediate brush.");

        canvas->BackgroundTransition = brushTransition;
        canvas->Background = yellowBrush;
        canvas->Background = redBrush;
    });
    windowHelper->SynchronouslyTickUIThread(2);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"gr");
}

void BrushTests::SolidColorBrushTransition_SetStaticValueWhenComplete()
{
    const auto& windowHelper = TestServices::WindowHelper;
    const auto& utilities = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ canvas = nullptr;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Height = 25;
        canvas->Width = 25;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        Canvas^ root = ref new Canvas();
        root->Children->Append(canvas);
        windowHelper->WindowContent = root;
    });
    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Change the duration to 1ms. This should complete immediately, and revert to a static blue color.
        BrushTransition^ explicitDuration = ref new BrushTransition();
        explicitDuration->Duration = { 10L };
        canvas->BackgroundTransition = explicitDuration;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
    });
    windowHelper->SynchronouslyTickUIThread(5);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

LinearGradientBrush^ BrushTests::MakeLinearGradientBrush(float startX, float endX)
{
    GradientStop^ stop1 = ref new GradientStop();
    stop1->Color = Microsoft::UI::Colors::Green;
    stop1->Offset = 0.2;

    GradientStop^ stop2 = ref new GradientStop();
    stop2->Color = Microsoft::UI::Colors::Red;
    stop2->Offset = 0.8;

    GradientStopCollection^ stopCollection = ref new GradientStopCollection();
    stopCollection->Append(stop1);
    stopCollection->Append(stop2);

    LinearGradientBrush^ brush = ref new LinearGradientBrush();
    brush->StartPoint = ::Windows::Foundation::Point(startX, 0);
    brush->EndPoint = ::Windows::Foundation::Point(endX, 0);
    brush->GradientStops = stopCollection;
    return brush;
}
} } } } } }
