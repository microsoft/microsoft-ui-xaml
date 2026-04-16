// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ConnectedAnimationTests.h"

#include <XamlTailored.h>
#include <FileLoader.h>
#include <TestEvent.h>
#include "MUX-ETWEvents.h"
#include "ETWWaiterProxy.h"
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <WUCRenderingScopeGuard.h>
#include <CustomTypeMetadataProvider.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <NavigationThemeTransitionTestPage.xaml.h>
#include <mindebug.h>

using namespace Platform;
using namespace Private::Foundation::CustomTypes;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace MockDComp;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace ::Windows::Foundation::Numerics;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

// RAII class for setting and restoring the Connected Animation Service Duration
class ConnectedAnimationServiceDurationHelper
{
public:
    ConnectedAnimationServiceDurationHelper(bool ignoreMinimumDuration = false)
    {
        m_minimumDuration.Duration = 0;

        if (!ignoreMinimumDuration)
        {
            // It is sometimes beneficial that we slow down the animation when running a test so that we can visually
            // see what is happening with the animations.  This registry entry will set the minimum animation duration
            // in milliseconds
            HKEY hkXaml = NULL;

            // It's possible the WinUI registry key doesn't exist at all. That's just fine.
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, XAML_ROOT_KEY, 0, KEY_READ, &hkXaml) == ERROR_SUCCESS)
            {
                DWORD data = 0;
                DWORD dwSize = sizeof(DWORD);

                // It's also possible the key for our feature doesn't exist at all. That's just fine too.
                if (RegQueryValueEx(hkXaml, L"ConnectedAnimationMinimumDuration", 0, NULL, reinterpret_cast<LPBYTE>(&data), &dwSize) == ERROR_SUCCESS)
                {
                    m_minimumDuration.Duration = data * 10000; // Convert from milliseconds to 100-nanoseconds
                }
                RegCloseKey(hkXaml);
            }
        }
        RunOnUIThread([&]()
        {
            xaml_animation::ConnectedAnimationService^ service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
            m_originalDuration = service->DefaultDuration;
            if (m_originalDuration.Duration < m_minimumDuration.Duration)
            {
                service->DefaultDuration = m_minimumDuration;
            }
        });
    }

    ~ConnectedAnimationServiceDurationHelper()
    {
        ResetDuration();
    }

    void ResetDuration()
    {
        SetDuration(m_originalDuration);
    }

    void SetDuration(::Windows::Foundation::TimeSpan duration)
    {
        RunOnUIThread([&]()
        {
            xaml_animation::ConnectedAnimationService::GetForCurrentView()->DefaultDuration = m_minimumDuration.Duration < duration.Duration ? duration : m_minimumDuration;
        });
    }

    void WaitForCompletionEvent(Microsoft::UI::Xaml::Tests::Common::Event* completionEvent)
    {
        if (m_minimumDuration.Duration <= m_originalDuration.Duration)
        {
            completionEvent->WaitForDefault();
        }
        else
        {
            // We have overridden the animation speed, so make sure the we give the animation enough
            // time to complete.
            completionEvent->WaitFor(std::chrono::milliseconds((m_minimumDuration.Duration / 10000) + 5000));
        }
    }

private:
    ::Windows::Foundation::TimeSpan m_originalDuration;
    ::Windows::Foundation::TimeSpan m_minimumDuration;
};

// This class is used to track the completions of multiple animations.
class CompletionHelper
{

private:
    int m_waitCount = 0;
    std::vector<ConnectedAnimation^> m_animations;
    std::vector<::Windows::Foundation::EventRegistrationToken> m_eventTokens;
    std::shared_ptr<Event> m_completedEvent;
public:
    CompletionHelper()
    {
    }

    ~CompletionHelper()
    {
        TriggerCompletedEvent();
    }

    void AddAnimation(ConnectedAnimation^ animation)
    {
        VERIFY_IS_NOT_NULL(animation);
        RunOnUIThread([&]()
        {
            m_waitCount++;
            m_animations.push_back(animation);
            m_eventTokens.push_back(animation->Completed +=
                ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([&](xaml_animation::ConnectedAnimation^ sender, Object^ e)
            {
                if (--m_waitCount == 0)
                {
                    TriggerCompletedEvent();
                }
            }));
        });
    }

    void SetCompletionEvent(std::shared_ptr<Event> completedEvent)
    {
        RunOnUIThread([&]()
        {
            m_completedEvent = completedEvent;
            if (m_waitCount == 0)
            {
                TriggerCompletedEvent();
            }
        });
    }

private:
    void TriggerCompletedEvent()
    {
        RunOnUIThread([&]()
        {
            for (unsigned int i = 0; i < m_animations.size(); i++)
            {
                m_animations[i]->Completed -= m_eventTokens[i];
            }
            m_animations.clear();
            m_eventTokens.clear();

        });

        if (m_completedEvent != nullptr)
        {
            m_completedEvent->Set();
            m_completedEvent = nullptr;
        }
    }
};


Platform::String^ ConnectedAnimationTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\animation\\";
}

bool ConnectedAnimationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ConnectedAnimationTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());


    // We, by default, disable the default connected animation configuration for these tests.  This allows us
    // to reduce the number of masters we have to change/maintain as the default (currently gravity) animations
    // get tweaked.
    LOG_OUTPUT(L"Disabling the default connected animation configuration");
    m_featureDisableDefaultConnectedAnimationConfiguration.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableDefaultConnectedAnimationConfiguration, true);

    // Ignore whether machine that is running the test has global animations turned off.
    LOG_OUTPUT(L"Enabling Global Animations");
    m_featureEnableGlobalAnimations.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableGlobalAnimations, true);

    return true;
}

bool ConnectedAnimationTests::TestCleanup()
{
    LOG_OUTPUT(L"Re-enabling the default connected animation configuration");
    m_featureDisableDefaultConnectedAnimationConfiguration.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableDefaultConnectedAnimationConfiguration, false);
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ConnectedAnimationTests::BasicAPI()
{
    TestCleanupWrapper cleanup;
    ConnectedAnimationServiceDurationHelper durationHelper;

    xaml_controls::Grid^ rootPanel = CreateTestPageContent();

    RunOnUIThread([&]()
    {
        xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);
        xaml_controls::TextBlock^ destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));
        VERIFY_IS_NOT_NULL(destination);

        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Attempt to retrieve a non-prepared animation");
        animation = service->GetAnimation(L"Test");
        VERIFY_IS_NULL(animation);

        LOG_OUTPUT(L"Prepare Animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Release the service and the animation");
        animation = nullptr;
        service = nullptr;

        LOG_OUTPUT(L"Re-retrieve the animation and service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        animation = service->GetAnimation(L"Test");

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);

        // Try to start an already started animation, the result will be false.
        result = animation->TryStart(destination);
        VERIFY_IS_FALSE(result);

        LOG_OUTPUT(L"Release the service");
        service = nullptr;

        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
        animation = nullptr;

        LOG_OUTPUT(L"Repeat with same elements");

        LOG_OUTPUT(L"Reuse Source element (Prepare)");
        animation = xaml_animation::ConnectedAnimationService::GetForCurrentView()->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Reuse Destination element (Start)");
        result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);

        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
        animation = nullptr;
    });
}

void ConnectedAnimationTests::SnapshotTransforms()
{
    ConnectedAnimationServiceDurationHelper durationHelper;

    TextBlock^ textBlock = nullptr; // This is the source UIElement for connected animation.
    TextBlock^ snapshot = nullptr; // This text block simulates the snapshot.
    Grid^ rootGrid = nullptr; // This is the root where the "snapshot" will be attached.
    Grid^ snapshotHostGrid = nullptr;
    MatrixTransform^ transformToRoot = nullptr;
    MatrixTransform^ parentTransformToRoot = nullptr;
    float zoomFactor = 1.25f;
    double parentScaleFactorX;
    double parentScaleFactorY;
    ScaleTransform^ snapshotScaleTransform = nullptr;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = nullptr;
    });

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(800, 600), zoomFactor);

    Panel^ panel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "VisualSnapshotTransform.xaml"));
    RunOnUIThread([&]()
    {
        textBlock = safe_cast<TextBlock^>(panel->FindName(L"tb2"));
        VERIFY_IS_NOT_NULL(textBlock);
        rootGrid = safe_cast<Grid^>(panel->FindName(L"root"));
        VERIFY_IS_NOT_NULL(rootGrid);
        TestServices::WindowHelper->WindowContent = panel;
    });

    TestServices::WindowHelper->WaitForIdle();

    // Take a DComp dump with the source element.
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Source");

    TestServices::WindowHelper->WaitForIdle();

    // Record transforms.
    RunOnUIThread([&]()
    {
        transformToRoot = static_cast<MatrixTransform^>(textBlock->TransformToVisual(nullptr));
        parentTransformToRoot = static_cast<MatrixTransform^>(static_cast<UIElement^>(textBlock->Parent)->TransformToVisual(nullptr));
        parentScaleFactorX = parentTransformToRoot->Matrix.M11;
        parentScaleFactorY = parentTransformToRoot->Matrix.M22;
    });

    // Simulate snapshot for textBlock
    RunOnUIThread([&]()
    {
        textBlock->Opacity = 0.0f; // Hide the source UIElement.
        snapshot = ref new TextBlock();
        snapshot->Text = textBlock->Text;
        snapshot->Foreground = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        snapshotScaleTransform = ref new ScaleTransform();
        snapshotScaleTransform->ScaleX = static_cast<ScaleTransform^>(textBlock->RenderTransform)->ScaleX * zoomFactor * parentScaleFactorX;
        snapshotScaleTransform->ScaleY = static_cast<ScaleTransform^>(textBlock->RenderTransform)->ScaleY * zoomFactor * parentScaleFactorY;
        snapshot->RenderTransform = snapshotScaleTransform;
    });

    // Insert the snapshot to the root and apply proper transform to place it to the same location as the textBlock.
    RunOnUIThread([&]()
    {
        auto snapshotTransform = ref new MatrixTransform();
        auto matrix = snapshotTransform->Matrix;

        snapshotHostGrid = ref new Grid();
        rootGrid->Children->Append(snapshotHostGrid);
        snapshotHostGrid->Children->Append(snapshot);

        matrix.M11 = 1/zoomFactor;
        matrix.M22 = 1/zoomFactor;
        matrix.M12 = 1.0f / (parentScaleFactorX * zoomFactor);
        matrix.M21 = -1.0f / (parentScaleFactorX * zoomFactor);
        matrix.OffsetX = transformToRoot->Matrix.OffsetX;
        matrix.OffsetY = transformToRoot->Matrix.OffsetY;

        snapshotTransform->Matrix = matrix;
        snapshotHostGrid->RenderTransform = snapshotTransform;
    });

    TestServices::WindowHelper->WaitForIdle();

    // Take a DComp dump with the snapshot element.
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Snapshot");

    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::IsolateElementVisuals()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    xaml_controls::Grid^ rootPanel = CreateTestPageContent();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Idle");

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        // Set the duration to an extremely long time so that nothing actually animates
        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        service->DefaultDuration = ts1;

        xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SourceIsolated");

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service and service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        animation = service->GetAnimation(L"Test");

        xaml_controls::TextBlock^ destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));
        VERIFY_IS_NOT_NULL(destination);

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"BothIsolated");

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service and service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        animation = service->GetAnimation(L"Test");

        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "IdleWithRoot");

}

void ConnectedAnimationTests::EndToEnd()
{
    EndToEndHelper();
}

void ConnectedAnimationTests::EndToEndCopySource()
{
    EndToEndHelper(SourceCopyOption::CopyWithoutCompNode, 1.0);
}


void ConnectedAnimationTests::EndToEndCopySourceWithCompNode()
{
    EndToEndHelper(SourceCopyOption::CopyWithCompNode, 1.0);
}

void ConnectedAnimationTests::DestinationVisualUpdate()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    xaml_controls::Grid^ rootPanel = CreateTestPageContent(1.0);
    xaml_controls::Grid^ parentGrid;
    xaml_controls::TextBlock^ source;
    xaml_controls::TextBlock^ destination;

    RunOnUIThread([&]()
    {
        source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);

        LOG_OUTPUT(L"Remove the destination from the tree");
        destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));

        parentGrid = safe_cast<xaml_controls::Grid^>(source->Parent);
        unsigned int index;
        VERIFY_IS_TRUE(parentGrid->Children->IndexOf(destination, &index));
        parentGrid->Children->RemoveAt(index);
    });
    TestServices::WindowHelper->WaitForIdle();

    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Updating the UI Tree");

        unsigned int index;
        VERIFY_IS_TRUE(parentGrid->Children->IndexOf(source, &index));
        parentGrid->Children->RemoveAt(index);

        parentGrid->Children->Append(destination);

        LOG_OUTPUT(L"Start Animation");
        caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e)
        {
            LOG_OUTPUT(L"Completed Event Fired");
            caCompletedEvent->Set();
        }));
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });

    // Tick a couple of times to ensure that the animation has started (and the destination snapshot is taken)
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    RunOnUIThread([&]()
    {
        // Update the text content of the destination
        destination->Text = L"New Destination";
    });
    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Wait for Completion Event");
    durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
    LOG_OUTPUT(L"Completed Event found");
}

void ConnectedAnimationTests::CustomAnimations()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;
    ConnectedAnimation^ animation;

    // First pass is to verify the end-to-end running

    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    animation = IntializePageAndCreateCustomAnimation();
    caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e)
    {
        LOG_OUTPUT(L"Completed Event Fired");
        caCompletedEvent->Set();
    }));

    durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
    TestServices::WindowHelper->WaitForIdle();
    animation = nullptr;

    // Second pass is to verify DComp tree

    // Set the duration to an extremely long time so that nothing actually animates
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);
        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        service->DefaultDuration = ts1;
    });

    animation = IntializePageAndCreateCustomAnimation();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        animation->Cancel();
    });
}

ConnectedAnimation^ ConnectedAnimationTests::IntializePageAndCreateCustomAnimation()
{
    xaml_controls::Grid^ rootPanel = CreateTestPageContent(1.0);
    xaml_controls::Grid^ parentGrid;
    xaml_controls::TextBlock^ source;
    xaml_controls::TextBlock^ destination;

    RunOnUIThread([&]()
    {
        source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);

        LOG_OUTPUT(L"Remove the destination from the tree");
        destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));

        parentGrid = safe_cast<xaml_controls::Grid^>(source->Parent);
        unsigned int index;
        VERIFY_IS_TRUE(parentGrid->Children->IndexOf(destination, &index));
        parentGrid->Children->RemoveAt(index);
    });
    TestServices::WindowHelper->WaitForIdle();

    xaml_animation::ConnectedAnimation^ animation;
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);


        unsigned int index;
        VERIFY_IS_TRUE(parentGrid->Children->IndexOf(source, &index));
        parentGrid->Children->RemoveAt(index);

        parentGrid->Children->Append(destination);

        // Just to verify the null parameter code path
        animation->SetAnimationComponent(ConnectedAnimationComponent::OffsetX, nullptr);

        // Create custom animations
        {
            auto customKeyFrameAnimation = ElementCompositionPreview::GetElementVisual(source)->Compositor->CreateScalarKeyFrameAnimation();
            customKeyFrameAnimation->Duration = service->DefaultDuration;
            customKeyFrameAnimation->InsertExpressionKeyFrame(0.0f, "StartingValue");
            customKeyFrameAnimation->InsertExpressionKeyFrame(0.5f, "FinalValue + 25");
            customKeyFrameAnimation->InsertExpressionKeyFrame(1.0f, "FinalValue");
            animation->SetAnimationComponent(ConnectedAnimationComponent::OffsetX, customKeyFrameAnimation);
        }
        {
            auto customKeyFrameAnimation = ElementCompositionPreview::GetElementVisual(source)->Compositor->CreateScalarKeyFrameAnimation();
            customKeyFrameAnimation->Duration = service->DefaultDuration;
            customKeyFrameAnimation->InsertExpressionKeyFrame(0.0f, "StartingValue");
            customKeyFrameAnimation->InsertExpressionKeyFrame(0.5f, "StartingValue - 25");
            customKeyFrameAnimation->InsertExpressionKeyFrame(1.0f, "FinalValue");
            animation->SetAnimationComponent(ConnectedAnimationComponent::OffsetY, customKeyFrameAnimation);
        }
        {
            auto customKeyFrameAnimation = ElementCompositionPreview::GetElementVisual(source)->Compositor->CreateScalarKeyFrameAnimation();
            customKeyFrameAnimation->Duration = service->DefaultDuration;
            customKeyFrameAnimation->InsertExpressionKeyFrame(0.0f, "StartingValue");
            customKeyFrameAnimation->InsertExpressionKeyFrame(0.5f, "FinalValue * 2");
            customKeyFrameAnimation->InsertExpressionKeyFrame(1.0f, "FinalValue");
            animation->SetAnimationComponent(ConnectedAnimationComponent::Scale, customKeyFrameAnimation);
        }
        {
            auto customKeyFrameAnimation = ElementCompositionPreview::GetElementVisual(source)->Compositor->CreateScalarKeyFrameAnimation();
            customKeyFrameAnimation->Duration = service->DefaultDuration;
            customKeyFrameAnimation->InsertExpressionKeyFrame(0.0f, "StartingValue");
            customKeyFrameAnimation->InsertExpressionKeyFrame(0.5f, "FinalValue");
            animation->SetAnimationComponent(ConnectedAnimationComponent::CrossFade, customKeyFrameAnimation);
        }

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });
    return animation;
 }

void ConnectedAnimationTests::RetainElement()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    TextBlock^ sourceElement = nullptr; // source element for connected animation.
    TextBlock^ destinationElement = nullptr; //destination element for connected animation.
    StackPanel^ root = nullptr;
    xaml_animation::ConnectedAnimationService^ service = nullptr;
    xaml_animation::ConnectedAnimation^ animation = nullptr;
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = nullptr;
    });

    TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(800, 600), 1.0f);

    Panel^ panel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "ConnectedAnimationRetainElement.xaml"));
    RunOnUIThread([&]()
    {
        sourceElement = safe_cast<TextBlock^>(panel->FindName(L"source"));
        VERIFY_IS_NOT_NULL(sourceElement);
        destinationElement = safe_cast<TextBlock^>(panel->FindName(L"destination"));
        VERIFY_IS_NOT_NULL(destinationElement);

        root = safe_cast<StackPanel^>(panel->FindName(L"root"));
        VERIFY_IS_NOT_NULL(root);
        TestServices::WindowHelper->WindowContent = panel;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Before");
    TestServices::WindowHelper->WaitForIdle();

    // Record transforms.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", sourceElement);
        VERIFY_IS_NOT_NULL(animation);
        // Now remove sourceParent from the root.
        // The snapshot for source element has not been created, it will retain the sourceParent in root's unloading storage.
        root->Children->RemoveAt(0);

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destinationElement);
        VERIFY_IS_TRUE(result);

        // Ensure that we take the snapshot after a single tick
        TestServices::WindowHelper->SetPostTickCallback(ref new PostTickCallback([]()
        {
            LOG_OUTPUT(L"Running PostTickCallback.");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "AfterSourceRemoval");
            TestServices::WindowHelper->SetPostTickCallback(nullptr);
        }));
    });
    TestServices::WindowHelper->WaitForIdle();

    // Record transforms.
    RunOnUIThread([&]()
    {
        // Now remove the destinationParent from the root.
        // This will de-activate the animation.
        // Note we do not retain the parent for destination and let it go.
        root->Children->RemoveAt(0);
        // Ensure that we take the snapshot after a single tick
        TestServices::WindowHelper->SetPostTickCallback(ref new PostTickCallback([]()
        {
            LOG_OUTPUT(L"Running PostTickCallback.");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "AfterDestRemoval");
            TestServices::WindowHelper->SetPostTickCallback(nullptr);
        }));
    });
    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::HandoffVisualInteraction()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;
    Platform::String^ variation;

    struct
    {
        LPCWSTR variation;
        LPCWSTR description;
    } passInfo[] = {
        { L"Prior", L"Verifying when visuals retrieved prior to animation is prepare/start" },
        { L"Concurrent", L"Verifying when visuals retrieved concurrently with animation prepare/start" },
        { L"During", L"Verifying when visual retrieved during animation" } };


    for (int pass = 0; pass < 3; pass++)
    {
        LOG_OUTPUT(passInfo[pass].description);
        xaml_controls::Grid^ rootPanel = CreateTestPageContent();
        auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
        auto caCompletedEvent = std::make_shared<Event>();
        xaml_controls::TextBlock^ source;
        xaml_controls::TextBlock^ destination;

        RunOnUIThread([&]()
        {
            source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
            VERIFY_IS_NOT_NULL(source);
            destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));
            VERIFY_IS_NOT_NULL(destination);

            if (pass == 0)
            {
                auto visual1 = ElementCompositionPreview::GetElementVisual(source);
                VERIFY_IS_NOT_NULL(visual1);
                auto visual2 = ElementCompositionPreview::GetElementVisual(destination);
                VERIFY_IS_NOT_NULL(visual2);
            }
        });
        TestServices::WindowHelper->WaitForIdle();


        RunOnUIThread([&]()
        {
            if (pass == 1)
            {
                auto visual1 = ElementCompositionPreview::GetElementVisual(source);
                VERIFY_IS_NOT_NULL(visual1);
                auto visual2 = ElementCompositionPreview::GetElementVisual(destination);
                VERIFY_IS_NOT_NULL(visual2);
            }
            xaml_animation::ConnectedAnimationService^ service;
            xaml_animation::ConnectedAnimation^ animation;

            LOG_OUTPUT(L"Retrieve the animation service");
            service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
            VERIFY_IS_NOT_NULL(service);

            LOG_OUTPUT(L"Prepare the animation");
            animation = service->PrepareToAnimate(L"Test", source);
            VERIFY_IS_NOT_NULL(animation);

            // Register the Completed event.
            caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e) { caCompletedEvent->Set(); }));

            LOG_OUTPUT(L"Start Animation");
            auto result = animation->TryStart(destination);
            VERIFY_IS_TRUE(result);
        });

        if (pass == 2)
        {
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            RunOnUIThread([&]()
            {
                auto visual1 = ElementCompositionPreview::GetElementVisual(source);
                VERIFY_IS_NOT_NULL(visual1);
                auto visual2 = ElementCompositionPreview::GetElementVisual(destination);
                VERIFY_IS_NOT_NULL(visual2);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, ref new Platform::String(passInfo[pass].variation));
        TestServices::WindowHelper->WaitForIdle();
    }
}

void ConnectedAnimationTests::CompletedEvent()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    xaml_controls::Grid^ rootPanel = CreateTestPageContent();
    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        // Register the Completed event.
        caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e) { caCompletedEvent->Set(); }));

        xaml_controls::TextBlock^ destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));
        VERIFY_IS_NOT_NULL(destination);

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });

    durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::HighDPI()
{
    EndToEndHelper(SourceCopyOption::DontCopy, 1.5f);
}

void ConnectedAnimationTests::VeryHighDPI()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(3000, 3000), 4.0f);
    xaml_controls::Grid^ rootPanel;
    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"<ScrollViewer>"
            L"   <StackPanel x:Name='ParentGrid' HorizontalAlignment='Left' VerticalAlignment='Top'>"
            L"     <Rectangle Width='600' Height='600' Fill='Green' x:Name='SourceElement'/>"
            L"   </StackPanel>"
            L"</ScrollViewer>"
            L"</Grid>"
            ));
        VERIFY_IS_NOT_NULL(rootPanel);
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    ETWWaiterProxy snapshotEtwWaiter;

    Platform::String^ etwValidationString =
        L"@Left=" + 0.0f + L" AND " +
        L"@Top=" + 0.0f + L" AND " +
        L"@Width=" + 2400.0f + L" AND " +
        L"@Height=" + 2400.0f;

    snapshotEtwWaiter.Start(
        WINDOWS_UI_XAML_ETW_PROVIDER,
        DCompSnapshotBoundsInfo_value,
        etwValidationString);

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        // Set the duration to an extremely long time so that nothing actually animates
        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        service->DefaultDuration = ts1;

        Shapes::Rectangle^ source = safe_cast<Shapes::Rectangle^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);
    });

    snapshotEtwWaiter.WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service and service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        animation = service->GetAnimation(L"Test");

        // Because of the snapshot waiter wait, it is possible that the animation has
        // already timed out, so we may not have been able to retrieve the animation.
        if (animation)
        {
            LOG_OUTPUT(L"Cancel Animation");
            animation->Cancel();
        }
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::DefaultAnimationParameters()
{
    TestCleanupWrapper cleanup;
    ConnectedAnimationServiceDurationHelper durationHelper(true /* ignoreMinimumDuration */);
    xaml_controls::Grid^ rootPanel = CreateTestPageContent();

    RunOnUIThread([&]()
    {
        xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);

        xaml_animation::ConnectedAnimationService^ service;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        // Read the default duration from service, it should be 300ms.
        auto duration = service->DefaultDuration;
        VERIFY_ARE_EQUAL(3000000, duration.Duration);

        // Read the default easing function from service, expecting null.
        auto easingFunction = static_cast<CubicBezierEasingFunction^>(service->DefaultEasingFunction);
        VERIFY_IS_NOT_NULL(easingFunction);
        VERIFY_ARE_EQUAL(0.8f, easingFunction->ControlPoint1.x);
        VERIFY_ARE_EQUAL(0.0f, easingFunction->ControlPoint1.y);
        VERIFY_ARE_EQUAL(0.2f, easingFunction->ControlPoint2.x);
        VERIFY_ARE_EQUAL(1.0f, easingFunction->ControlPoint2.y);

        // Set the default duration to be 500ms.
        ::Windows::Foundation::TimeSpan ts1 = { 5000000 };
        service->DefaultDuration = ts1;

        // Read the default duration, it should be 500ms.
        VERIFY_ARE_EQUAL(5000000, service->DefaultDuration.Duration);

        auto visual = ElementCompositionPreview::GetElementVisual(source);

        // Bind Offset.y to compensate vertical scrolling so the object does not move vertically
        auto customEasingFunction = CompositionEasingFunction::CreateLinearEasingFunction(visual->Compositor);

        service->DefaultEasingFunction = customEasingFunction;

        // Read the default easing function, it should be CubicEase
        VERIFY_IS_NOT_NULL(service->DefaultEasingFunction);
        VERIFY_ARE_EQUAL(customEasingFunction, service->DefaultEasingFunction);
    });

    // Tick UI thread and verify the default duration is still 500ms
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    RunOnUIThread([&]()
    {
        auto service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        // Read the default duration from service, it should be 500ms..
        auto duration = service->DefaultDuration;
        VERIFY_ARE_EQUAL(5000000, duration.Duration);
    });
}

void ConnectedAnimationTests::ContentWithOffset()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    TextBlock^ sourceElement = nullptr; // source element for connected animation.
    TextBlock^ destinationElement = nullptr; //destination element for connected animation.
    StackPanel^ root = nullptr;
    xaml_animation::ConnectedAnimationService^ service = nullptr;
    xaml_animation::ConnectedAnimation^ animation = nullptr;

    Panel^ panel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "ConnectedAnimationContentOffset.xaml"));
    RunOnUIThread([&]()
    {
        sourceElement = safe_cast<TextBlock^>(panel->FindName(L"source"));
        VERIFY_IS_NOT_NULL(sourceElement);
        destinationElement = safe_cast<TextBlock^>(panel->FindName(L"destination"));
        VERIFY_IS_NOT_NULL(destinationElement);

        root = safe_cast<StackPanel^>(panel->FindName(L"root"));
        VERIFY_IS_NOT_NULL(root);
        TestServices::WindowHelper->WindowContent = panel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        // Set the duration to an extremely long time so that nothing actually animates,
        // The snapshot will pop up and should be visible.
        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        durationHelper.SetDuration(ts1);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", sourceElement);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destinationElement);
        VERIFY_IS_TRUE(result);
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service and service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        animation = service->GetAnimation(L"Test");

        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
    });

    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::TryStartWithNotLiveElement()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;
    xaml_controls::Grid^ rootPanel = CreateTestPageContent();

    RunOnUIThread([&]()
    {
        xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);
        xaml_controls::TextBlock^ destination = ref new xaml_controls::TextBlock();
        VERIFY_IS_NOT_NULL(destination);

        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare Animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result); // Still return true here, however the animation will not run.
    });
    // Wait for 2 ticks, verify the test doesn't crash.
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::PrepareWithNotLiveElement()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    xaml_controls::Grid^ rootPanel = CreateTestPageContent();

    RunOnUIThread([&]()
    {
        xaml_controls::TextBlock^ source = ref new xaml_controls::TextBlock();
        VERIFY_IS_NOT_NULL(source);

        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare Animation");
        HRESULT hr = S_OK;
        try
        {
            animation = service->PrepareToAnimate(L"Test", source);
        }
        catch (Platform::Exception^ e)
        {
            // If user try to prepare an element that is not in the live tree, we throw E_INVALIDARG error.
            hr = e->HResult;
        }

        VERIFY_ARE_EQUAL(hr, E_INVALIDARG);
    });
    // Wait for 2 ticks, verify the test doesn't crash.
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::CoordinatedAnimations()
{
    TestCoordinatedAnimations(false /* disable Scale */);
}

void ConnectedAnimationTests::CoordinatedAnimationsNoScale()
{
    TestCoordinatedAnimations(true /* disable Scale */);
}

void ConnectedAnimationTests::TestCoordinatedAnimations(bool disableScale)
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    // First pass is to verify end-to-end (and cleanup)
    ConnectedAnimation^ animation = IntializeAndStartCoordinatedAnimation(disableScale);
    caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e)
    {
        LOG_OUTPUT(L"Completed Event Fired");
        caCompletedEvent->Set();
    }));

    durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "AfterAnimation");
    animation = nullptr;

    // Second pass is to verify DComp tree

    // Set the duration to an extremely long time so that nothing actually animates
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);
        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        service->DefaultDuration = ts1;
    });

    animation = IntializeAndStartCoordinatedAnimation(disableScale);
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"InAnimation");
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        animation->Cancel();
    });
    TestServices::WindowHelper->WaitForIdle();
}

ConnectedAnimation^ ConnectedAnimationTests::IntializeAndStartCoordinatedAnimation(bool disableScale)
{
    FrameworkElement^ source;
    FrameworkElement^ destination;
    Platform::Collections::Vector<UIElement^>^ coordinatedElements;
    FrameworkElement^ destinationGrid;

    RunOnUIThread([&]()
    {
        Grid^ rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <Rectangle x:Name='Source' Fill='Blue' Height='50' Width='300' HorizontalAlignment='Left' VerticalAlignment='Top'/>"
            L"    <Grid x:Name='DestinationGrid' Height='400' Width='400' HorizontalAlignment='Center' VerticalAlignment='Center' Visibility='Visible'>"
            L"        <Grid.ColumnDefinitions>"
            L"            <ColumnDefinition Width='*'/>"
            L"            <ColumnDefinition Width='*'/>"
            L"            <ColumnDefinition Width='*'/>"
            L"        </Grid.ColumnDefinitions>"
            L"        <Grid.RowDefinitions>"
            L"            <RowDefinition Height='*'/>"
            L"            <RowDefinition Height='*'/>"
            L"            <RowDefinition Height='*'/>"
            L"        </Grid.RowDefinitions>"
            L"        <Rectangle Grid.Row='1' Grid.Column='1' x:Name='Destination' Fill='Gray' HorizontalAlignment='Stretch' VerticalAlignment='Stretch'/>"
            L"        <Ellipse Grid.Row='1' Grid.Column='0' x:Name='Coordinated1' Fill='Yellow' HorizontalAlignment='Stretch' VerticalAlignment='Stretch'/>"
            L"        <Ellipse Grid.Row='1' Grid.Column='2' x:Name='Coordinated2' Fill='Yellow' HorizontalAlignment='Stretch' VerticalAlignment='Stretch'/>"
            L"        <Ellipse Grid.Row='0' Grid.Column='1' x:Name='Coordinated3' Fill='Yellow' HorizontalAlignment='Stretch' VerticalAlignment='Stretch'/>"
            L"        <Ellipse Grid.Row='2' Grid.Column='1' x:Name='Coordinated4' Fill='Yellow' HorizontalAlignment='Stretch' VerticalAlignment='Stretch'/>"
            L"        <TextBlock Grid.Row='1' Grid.Column='1' x:Name='Coordinated5' Foreground='Yellow' FontSize='60' Text='UXP' HorizontalAlignment='Center' VerticalAlignment='Center'/>"
            L"    </Grid>"
            L"</Grid>"
        ));
        VERIFY_IS_NOT_NULL(rootPanel);
        TestServices::WindowHelper->WindowContent = rootPanel;

        source = safe_cast<FrameworkElement^>(rootPanel->FindName(L"Source"));
        VERIFY_IS_NOT_NULL(source);
        destination = safe_cast<FrameworkElement^>(rootPanel->FindName(L"Destination"));
        VERIFY_IS_NOT_NULL(destination);
        destinationGrid = safe_cast<FrameworkElement^>(rootPanel->FindName(L"DestinationGrid"));
        VERIFY_IS_NOT_NULL(destinationGrid);

        coordinatedElements = ref new Platform::Collections::Vector<UIElement^>();
        coordinatedElements->Append(safe_cast<UIElement^>(rootPanel->FindName(L"Coordinated1")));
        VERIFY_IS_NOT_NULL(coordinatedElements->GetAt(0));
        coordinatedElements->Append(safe_cast<UIElement^>(rootPanel->FindName(L"Coordinated2")));
        VERIFY_IS_NOT_NULL(coordinatedElements->GetAt(1));
        coordinatedElements->Append(safe_cast<UIElement^>(rootPanel->FindName(L"Coordinated3")));
        VERIFY_IS_NOT_NULL(coordinatedElements->GetAt(2));
        coordinatedElements->Append(safe_cast<UIElement^>(rootPanel->FindName(L"Coordinated4")));
        VERIFY_IS_NOT_NULL(coordinatedElements->GetAt(3));
        coordinatedElements->Append(safe_cast<UIElement^>(rootPanel->FindName(L"Coordinated5")));
        VERIFY_IS_NOT_NULL(coordinatedElements->GetAt(4));

    });
    TestServices::WindowHelper->WaitForIdle();

    xaml_animation::ConnectedAnimation^ animation;
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        if (disableScale)
        {
            animation->IsScaleAnimationEnabled = false;
        }

        Grid^ parentGrid = safe_cast<xaml_controls::Grid^>(source->Parent);
        unsigned int index;
        VERIFY_IS_TRUE(parentGrid->Children->IndexOf(source, &index));
        parentGrid->Children->RemoveAt(index);
        destinationGrid->Visibility = Visibility::Visible;

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination, coordinatedElements);
        VERIFY_IS_TRUE(result);
    });
    return animation;
}


void ConnectedAnimationTests::CoordinatedAnimationErrors()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    xaml_controls::Grid^ rootPanel = CreateTestPageContent();
    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    xaml_controls::TextBlock^ source;
    xaml_controls::TextBlock^ destination;
    Platform::Collections::Vector<UIElement^>^ coordinatedElements = ref new Platform::Collections::Vector<UIElement^>();


    xaml_animation::ConnectedAnimationService^ service;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootPanel;

        source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);
        destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));
        VERIFY_IS_NOT_NULL(source);
        coordinatedElements->Append(safe_cast<UIElement^>(rootPanel->FindName(L"MiscElement")));
        VERIFY_IS_NOT_NULL(coordinatedElements->GetAt(0));

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        service->DefaultDuration = ts1;
    });

    TestServices::WindowHelper->WaitForIdle();

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Testing cancel after starting");
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Prepare Animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination, coordinatedElements);
        VERIFY_IS_TRUE(result);

        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    // Verify no tree changes occurred due to the coordinated animation.
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "CancelAfterStart");

    LOG_OUTPUT(L"Testing reuse of element");
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Prepare Animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        Platform::Collections::Vector<UIElement^>^ dupCoordinatedElements = ref new Platform::Collections::Vector<UIElement^>(1);
        dupCoordinatedElements->Append(destination);

        LOG_OUTPUT(L"Start animation");
        animation->TryStart(destination, dupCoordinatedElements);
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    // Verify the elements still property participate in the animation
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ElementReuse");

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimation^ animation = service->GetAnimation(L"Test");
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Testing coordinated element not in tree");
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Prepare Animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        Platform::Collections::Vector<UIElement^>^ notLiveElements = ref new Platform::Collections::Vector<UIElement^>(1);
        notLiveElements->Append(ref new TextBlock());

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination, notLiveElements);
        VERIFY_IS_TRUE(result);

        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
    });

    LOG_OUTPUT(L"Testing cancel after animation running");
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        service->DefaultDuration = ts1;

        LOG_OUTPUT(L"Prepare Animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination, coordinatedElements);
        VERIFY_IS_TRUE(result);

        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    // Verify the comp tree hasn't changed
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "CancelAfterRunning");
}

void ConnectedAnimationTests::PrepareElementThenSetOpacity0()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    xaml_controls::Grid^ rootPanel = CreateTestPageContent();

    RunOnUIThread([&]()
    {
        xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);

        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare Animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Set source element opacity to 0, no snapshot will be created, app shouldn't crash");
        source->Opacity = 0.0f;
    });
    // Wait for 2 ticks, verify the test doesn't crash.
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::ElementsClipped()
{
    ClippingHelper(0.5f);
}

void ConnectedAnimationTests::ElementsCompletelyClipped()
{
    ClippingHelper(1.0f);
}

void ConnectedAnimationTests::Timeout()
{
    // We mock the GetTickCount64 method so that we can simulate jumping into the future.
    long tickCountOffset = 0;
    /*  MOCK10_REMOVAL
    typedef Mock10::MockFunction<ULONGLONG WINAPI()>::Prototype GetTickCount64Prototype;
    auto mockGetTickCount64 = Mock10::Mock::Function<GetTickCount64Prototype>(GetTickCount64);
    mockGetTickCount64.Set([&] { return ::GetTickCount64() + tickCountOffset; });
    */

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    xaml_controls::Grid^ rootPanel = CreateTestPageContent();

    RunOnUIThread([&]()
    {
        xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);

        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare Animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);
    });
    TestServices::WindowHelper->WaitForIdle();

    // Jump 5 seconds into the future
    tickCountOffset = 5000;

    // Issue a single tick, which should cancel the animation and return the scene
    // to it normal state.
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ConnectedAnimationTests::RestartBeforeComplete()
{
    TestCleanupWrapper cleanup;
    ConnectedAnimationServiceDurationHelper durationHelper;

    xaml_controls::Grid^ rootPanel = CreateTestPageContent();

    RunOnUIThread([&]()
    {
        xaml_controls::TextBlock^ source1 = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source1);
        xaml_controls::TextBlock^ destination1 = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));
        VERIFY_IS_NOT_NULL(source1);

        xaml_controls::TextBlock^ source2 = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"MiscElement"));
        VERIFY_IS_NOT_NULL(source2);
        xaml_controls::TextBlock^ destination2 = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"MiscElement2"));
        VERIFY_IS_NOT_NULL(source2);

        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation1;
        xaml_animation::ConnectedAnimation^ animation2;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare Animation1");
        animation1 = service->PrepareToAnimate(L"Test1", source1);
        VERIFY_IS_NOT_NULL(animation1);

        LOG_OUTPUT(L"Prepare Animation2");
        animation2 = service->PrepareToAnimate(L"Test2", source2);
        VERIFY_IS_NOT_NULL(animation2);

        LOG_OUTPUT(L"Start Animation1");
        auto result = animation1->TryStart(destination1);
        VERIFY_IS_TRUE(result);

        LOG_OUTPUT(L"Start Animation2");
        result = animation2->TryStart(destination2);
        VERIFY_IS_TRUE(result);

        // Try to prepare/start again.
        LOG_OUTPUT(L"Prepare Animation1");
        animation1 = service->PrepareToAnimate(L"Test1", source1);
        VERIFY_IS_NOT_NULL(animation1);

        LOG_OUTPUT(L"Prepare Animation2");
        animation2 = service->PrepareToAnimate(L"Test2", source2);
        VERIFY_IS_NOT_NULL(animation2);

        LOG_OUTPUT(L"Start Animation1");
        result = animation1->TryStart(destination1);
        VERIFY_IS_TRUE(result);

        LOG_OUTPUT(L"Start Animation2");
        result = animation2->TryStart(destination2);
        VERIFY_IS_TRUE(result);

        LOG_OUTPUT(L"Cancel Animation1");
        animation1->Cancel();
        animation1 = nullptr;

        LOG_OUTPUT(L"Cancel Animation2");
        animation2->Cancel();
        animation2 = nullptr;
    });
}

void ConnectedAnimationTests::DisabledFromSettings()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_controls::Grid^ rootPanel = CreateTestPageContent();
    xaml_controls::TextBlock^ source;
    xaml_controls::TextBlock^ destination;
    xaml_animation::ConnectedAnimationService^ service;
    xaml_animation::ConnectedAnimation^ animation;
    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();
    ::Windows::Foundation::EventRegistrationToken renderingEventToken;
    ConnectedAnimationServiceDurationHelper durationHelper;

    bool wasAnimationDisabled = false;
    TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations), true, &wasAnimationDisabled);
    TestCleanupWrapper cleanup([&]()
    {
        TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations), wasAnimationDisabled, nullptr);
    });

    int count = 0;
    {
        RunOnUIThread([&]()
        {
            xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
            VERIFY_IS_NOT_NULL(source);
            xaml_controls::TextBlock^ destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));
            VERIFY_IS_NOT_NULL(destination);

            LOG_OUTPUT(L"Retrieve the animation service");
            service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
            VERIFY_IS_NOT_NULL(service);

            LOG_OUTPUT(L"Prepare Animation");
            animation = service->PrepareToAnimate(L"Test", source);
            VERIFY_IS_NOT_NULL(animation);

            // Register the Completed event.
            caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e) { caCompletedEvent->Set(); }));

            LOG_OUTPUT(L"Start Animation");
            auto result = animation->TryStart(destination);
            // When animations are disabled from Settings, we should still run connected animation but it will complete immediately.
            VERIFY_IS_TRUE(result);

            // Register for the Composition Target Rendering event.  This will get called at the beginning
            // of each tick and allow us to perform a render action, modifying the tree.
            auto onRendering = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
            {
                if (count == 1) // validate at the beginning of the second tick.
                {
                    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
                    Microsoft::UI::Xaml::Media::CompositionTarget::Rendering::remove(renderingEventToken);
                }
                count++;
            });
            renderingEventToken = Microsoft::UI::Xaml::Media::CompositionTarget::Rendering::add(onRendering);
        });
        // Note due to the very short animation duration, we cannot do the following:
        // RunOnUIThread([&]() { /* asdf */ });
        // The UI thread could be ticking here
        // TestServices::WindowHelper->SynchronouslyTickUIThread(1);  // UI thread ticks 1 more time
        // The UI thread could tick here too
        // TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        // that guarantees at least 1 frame
        // We want exactly 1 frame.
        durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
        TestServices::WindowHelper->WaitForIdle();
    }
}
void ConnectedAnimationTests::ContentThemeTransition()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    Shapes::Rectangle^ sourceElement = nullptr; // source element for connected animation.
    Shapes::Rectangle^ destinationElement = nullptr; //destination element for connected animation.
    xaml_animation::ConnectedAnimationService^ service = nullptr;
    xaml_animation::ConnectedAnimation^ animation = nullptr;
    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    Panel^ panel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "ConnectedAnimationWithContentThemeTransition.xaml"));
    RunOnUIThread([&]()
    {
        sourceElement = safe_cast<Shapes::Rectangle^>(panel->FindName(L"source"));
        VERIFY_IS_NOT_NULL(sourceElement);
        destinationElement = safe_cast<Shapes::Rectangle^>(panel->FindName(L"destination"));
        VERIFY_IS_NOT_NULL(destinationElement);

        TestServices::WindowHelper->WindowContent = panel;
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        // Set the duration to an extremely long time so that nothing actually animates,
        // The snapshot will pop up and should be visible.
        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        durationHelper.SetDuration(ts1);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", sourceElement);
        VERIFY_IS_NOT_NULL(animation);

        caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e) { caCompletedEvent->Set(); }));

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destinationElement);
        VERIFY_IS_TRUE(result);
    });

    TestServices::WindowHelper->WaitForIdle();
    // Wait for 0.6 second, content theme transition animation should have completed, verify the DComp tree and the destination element should still be hidden.
    caCompletedEvent->WaitForNoThrow(std::chrono::milliseconds(600) /* timeout */);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service and cancel.");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        animation = service->GetAnimation(L"Test");

        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
    });

    TestServices::WindowHelper->WaitForIdle();

}
void ConnectedAnimationTests::SuppressScaleAnimation()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));
    xaml_controls::Grid^ rootPanel;
    xaml_controls::Grid^ source;
    xaml_controls::Grid^ destination;

    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Blue'"
            L"   ScrollViewer.VerticalScrollMode='Disabled' ScrollViewer.VerticalScrollBarVisibility='Hidden'>"
            L"   <Grid x:Name='SourceElement' Margin='10,10,10,10' Height='50' Width='50' HorizontalAlignment='Left' VerticalAlignment='Top' Background='Yellow'/>"
            L"   <Grid x:Name='DestinationElement' Margin='10,10,10,10' Height='200' Width='200' HorizontalAlignment='Right' VerticalAlignment='Bottom' Background='Green'/>"
            L"</Grid>"
        ));
        VERIFY_IS_NOT_NULL(rootPanel);
        TestServices::WindowHelper->WindowContent = rootPanel;

        source = safe_cast<xaml_controls::Grid^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);
        destination = safe_cast<xaml_controls::Grid^>(rootPanel->FindName(L"DestinationElement"));
        VERIFY_IS_NOT_NULL(destination);

    });

    TestServices::WindowHelper->WaitForIdle();

    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    // First pass is to verify the end-to-end running
    ConnectedAnimation^ animation;
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Suppress the scale animation");
        VERIFY_IS_TRUE(animation->IsScaleAnimationEnabled);
        animation->IsScaleAnimationEnabled = false;
        VERIFY_IS_FALSE(animation->IsScaleAnimationEnabled);

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });

    caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e)
    {
        LOG_OUTPUT(L"Completed Event Fired");
        caCompletedEvent->Set();
    }));


    durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
    TestServices::WindowHelper->WaitForIdle();

    animation = nullptr;

    // Second pass is to verify DComp tree

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);
        // Set the duration to an extremely long time so that nothing actually animates
        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        service->DefaultDuration = ts1;

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Suppress the scale animation");
        animation->IsScaleAnimationEnabled = false;

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        animation->Cancel();
    });
}

xaml_controls::Grid^ ConnectedAnimationTests::CreateTestPageContent(float scale)
{
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(400, 300), scale);
    xaml_controls::Grid^ rootPanel;
    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Blue'"
            L"   ScrollViewer.VerticalScrollMode='Disabled' ScrollViewer.VerticalScrollBarVisibility='Hidden'>"
            L"   <Grid x:Name='ParentGrid' Margin='10,10,10,10'>"
            L"     <TextBlock x:Name='SourceElement' FontSize='15' Text='Source Element' HorizontalAlignment='Left' VerticalAlignment='Bottom'/>"
            L"     <TextBlock x:Name='MiscElement' FontSize='15' Text='Miscellaneous Element' HorizontalAlignment='Left' VerticalAlignment='Center'/>"
            L"     <TextBlock x:Name='MiscElement2' FontSize='15' Text='Miscellaneous Element' HorizontalAlignment='Right' VerticalAlignment='Center'/>"
            L"     <TextBlock x:Name='DestinationElement' FontSize='30' Text='Destination Element' HorizontalAlignment='Right' VerticalAlignment='Top'/>"
            L"   </Grid>"
            L"</Grid>"
            ));
        VERIFY_IS_NOT_NULL(rootPanel);
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    return rootPanel;
}

void ConnectedAnimationTests::EndToEndHelper(SourceCopyOption sourceCopyOption, float scale)
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    xaml_controls::Grid^ rootPanel = CreateTestPageContent(scale);
    xaml_controls::Grid^ parentGrid;
    xaml_controls::TextBlock^ source;
    xaml_controls::TextBlock^ destination;
    Microsoft::UI::Composition::Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        if (scale == 1.0f)
        {
            source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        }
        else
        {
           // SourceElement is pushed outside the render area in high DPI scenario, use another element instead.
            source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"MiscElement"));
        }
        VERIFY_IS_NOT_NULL(source);

        LOG_OUTPUT(L"Remove the destination from the tree");
        destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));

        // If we are copying a source element that already has a comp node then we need to
        // make sure it does have one by requesting a handoff visual.
        if (sourceCopyOption == SourceCopyOption::CopyWithCompNode)
        {
            handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(source));
            VERIFY_IS_NOT_NULL(handOffVisual);
        }

        parentGrid = safe_cast<xaml_controls::Grid^>(source->Parent);
        unsigned int index;
        VERIFY_IS_TRUE(parentGrid->Children->IndexOf(destination, &index));
        parentGrid->Children->RemoveAt(index);
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Before");

    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Updating the UI Tree");

        if (sourceCopyOption == SourceCopyOption::DontCopy)
        {
            // If we aren't animating a copy of the source, then remove it from the tree.
            unsigned int index;
            VERIFY_IS_TRUE(parentGrid->Children->IndexOf(source, &index));
            parentGrid->Children->RemoveAt(index);
        }
        parentGrid->Children->Append(destination);

        LOG_OUTPUT(L"Retrieve the animation service and animation");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        animation = service->GetAnimation(L"Test");

        LOG_OUTPUT(L"Start Animation");
        caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e)
        {
            LOG_OUTPUT(L"Completed Event Fired");
            caCompletedEvent->Set();
        }));
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });

    LOG_OUTPUT(L"Wait for Completion Event");
    durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
    LOG_OUTPUT(L"Completed Event found");
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "After");
}

void ConnectedAnimationTests::ClippingHelper(float percentage)
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    Shapes::Ellipse^ source;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));
    xaml_controls::Grid^ rootPanel;

    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"   <StackPanel>"
            L"     <Ellipse x:Name='source' Width='100' Height='100' Fill='Green' />"
            L"     <Ellipse x:Name='destination' Margin='-100,0,0,0' Width='200' Height='200' HorizontalAlignment='Left' Fill='Blue'/>"
            L"   </StackPanel>"
            L"</Grid>"
            ));
        VERIFY_IS_NOT_NULL(rootPanel);
        source = safe_cast<Shapes::Ellipse^>(rootPanel->FindName(L"source"));
        VERIFY_IS_NOT_NULL(source);
        source->Margin = Thickness({0,-(percentage * 100),0,0});
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        // Set the duration to an extremely long time so that nothing actually animates
        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        service->DefaultDuration = ts1;

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        Shapes::Ellipse^ destination = safe_cast<Shapes::Ellipse^>(rootPanel->FindName(L"destination"));
        VERIFY_IS_NOT_NULL(destination);

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service and service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        animation = service->GetAnimation(L"Test");

        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
    });

    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::ConnectedPlusImplicit()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    Canvas^ sourceParent = nullptr;
    Canvas^ destinationParent = nullptr;
    TextBlock^ sourceElement = nullptr; // source element for connected animation.
    TextBlock^ destinationElement = nullptr; //destination element for connected animation.
    StackPanel^ root = nullptr;
    Visual^ handOffVisualOld = nullptr;
    Visual^ handOffVisualNew = nullptr;
    ScalarKeyFrameAnimation^ hideAnimation = nullptr;
    ScalarKeyFrameAnimation^ showAnimation = nullptr;
    xaml_animation::ConnectedAnimationService^ service = nullptr;
    xaml_animation::ConnectedAnimation^ animation = nullptr;
    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    Panel^ panel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "ConnectedPlusImplicit.xaml"));
    RunOnUIThread([&]()
    {
        sourceElement = safe_cast<TextBlock^>(panel->FindName(L"source"));
        VERIFY_IS_NOT_NULL(sourceElement);
        destinationElement = safe_cast<TextBlock^>(panel->FindName(L"destination"));
        VERIFY_IS_NOT_NULL(destinationElement);

        root = safe_cast<StackPanel^>(panel->FindName(L"root"));
        VERIFY_IS_NOT_NULL(root);

        sourceParent = safe_cast<Canvas^>(root->FindName(L"sourceParent"));
        destinationParent = safe_cast<Canvas^>(root->FindName(L"destinationParent"));
        root->Children->RemoveAt(1);
        TestServices::WindowHelper->WindowContent = panel;

        LOG_OUTPUT(L"Getting Handoff Visual");
        handOffVisualOld = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(sourceParent));
        handOffVisualNew = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(destinationParent));

        LOG_OUTPUT(L"Creating Implicit Animations");
        auto compositor = handOffVisualOld->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        LinearEasingFunction^ ease;
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(sourceParent, hideAnimation);
        ElementCompositionPreview::SetImplicitShowAnimation(destinationParent, showAnimation);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        ::Windows::Foundation::TimeSpan ts1 = { 10000000000L };   // 1000 seconds
        service->DefaultDuration = ts1;

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", sourceElement);
        VERIFY_IS_NOT_NULL(animation);
        // Now remove sourceParent from the root.
        // The snapshot for source element has not been created, it will retain the sourceParent in root's unloading storage.
        root->Children->RemoveAt(0);
        root->Children->Append(destinationParent);
        auto result = animation->TryStart(destinationElement);
        VERIFY_IS_TRUE(result);
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        animation->Cancel();
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating implicit animations");
        handOffVisualOld->StopAnimationGroup(hideAnimation);
        handOffVisualNew->StopAnimationGroup(showAnimation);
    });

    TestServices::WindowHelper->WaitForImplicitShowHideComplete();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void ConnectedAnimationTests::ConnectedPlusThemeTransition()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;
    Frame^ rootFrame;
    Page^ p1;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    xaml_animation::ConnectedAnimationService^ service = nullptr;
    xaml_animation::ConnectedAnimation^ animation = nullptr;

    RunOnUIThread([&]()
    {
        p1 = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        p1->Transitions = ref new xaml_animation::TransitionCollection();
        p1->Transitions->Append(ref new xaml_animation::ContentThemeTransition());
        r1 = ref new xaml_shapes::Rectangle();
        r1->Width = 200;
        r1->Height = 200;
        r1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);
        p1->Content = r1;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);
        ::Windows::Foundation::TimeSpan ts1 = { 10000000000L };   // 1000 seconds
        service->DefaultDuration = ts1;
        animation = service->PrepareToAnimate(L"Test", r1);
        VERIFY_IS_NOT_NULL(animation);
        rootFrame = p1->Frame;
        p1->Frame->Navigate(::Windows::UI::Xaml::Interop::TypeName(p1->GetType()));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        r2 = ref new xaml_shapes::Rectangle();
        r2->Width = 200;
        r2->Height = 200;
        r2->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        auto p2 = safe_cast<Page^>(rootFrame->Content);
        p2->Content = r2;
        auto result = animation->TryStart(r2);
        VERIFY_IS_TRUE(result);
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        animation->Cancel();
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void ConnectedAnimationTests::ConnectedPlusThemeTransition2()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;
    Frame^ rootFrame;
    Page^ p1;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    xaml_animation::ConnectedAnimationService^ service = nullptr;
    xaml_animation::ConnectedAnimation^ animation = nullptr;

    RunOnUIThread([&]()
    {
        p1 = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        p1->Transitions = ref new xaml_animation::TransitionCollection();
        p1->Transitions->Append(ref new xaml_animation::NavigationThemeTransition());
        r1 = ref new xaml_shapes::Rectangle();
        r1->Width = 200;
        r1->Height = 200;
        r1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);
        p1->Content = r1;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);
        ::Windows::Foundation::TimeSpan ts1 = { 10000000000L };   // 1000 seconds
        service->DefaultDuration = ts1;
        animation = service->PrepareToAnimate(L"Test", r1);
        VERIFY_IS_NOT_NULL(animation);
        rootFrame = p1->Frame;
        p1->Frame->Navigate(::Windows::UI::Xaml::Interop::TypeName(p1->GetType()));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        r2 = ref new xaml_shapes::Rectangle();
        r2->Width = 200;
        r2->Height = 200;
        r2->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        auto p2 = safe_cast<Page^>(rootFrame->Content);
        p2->Content = r2;
        auto result = animation->TryStart(r2);
        VERIFY_IS_TRUE(result);
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        animation->Cancel();
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}


void ConnectedAnimationTests::ConnectedPlusThemeTransition3()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;
    Frame^ rootFrame;
    Page^ p1;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    xaml_animation::ConnectedAnimationService^ service = nullptr;
    xaml_animation::ConnectedAnimation^ animation = nullptr;

    RunOnUIThread([&]()
    {
        // We are currently using SetupSimulatedAppPage here to get the structure, but that doesn't give use the
        // content we need and the hooks.  So we immediately navigate to a more custom page using the
        // Navigation Theme Transition Test page.  It would be desirable to roll the flexibility of this into
        // the SimulatedAppPage structure.
        p1 = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        NavigationThemeTransitionTestPageConfiguration config;
        config.PageInitialization = [&](Microsoft::UI::Xaml::Controls::Page^ page)
        {
            // Find our child element and move it to the left
            r1 = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(page->Content);
            r1->VerticalAlignment = xaml::VerticalAlignment::Center;
            r1->HorizontalAlignment = xaml::HorizontalAlignment::Left;
        };
        config.NavigatingFrom = [&](Microsoft::UI::Xaml::Controls::Page^ page, Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs^ e)
        {
            // Prepare the connected animation.
            LOG_OUTPUT(L"Preparing connected Animation");
            service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
            VERIFY_IS_NOT_NULL(service);
            ::Windows::Foundation::TimeSpan ts1 = { 10000000000L };   // 1000 seconds
            service->DefaultDuration = ts1;
            animation = service->PrepareToAnimate(L"Test", r1);
            VERIFY_IS_NOT_NULL(animation);
        };
        LOG_OUTPUT(L"Navigating from main page to the source page");
        p1->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, ref new xaml_animation::SuppressNavigationTransitionInfo());

    });
    TestServices::WindowHelper->WaitForIdle();

     RunOnUIThread([&]()
    {
        NavigationThemeTransitionTestPageConfiguration config;
        config.PageInitialization = [&](Microsoft::UI::Xaml::Controls::Page^ page)
            {
            // Find our child element and move it to the right
            r2 = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(page->Content);
            r2->VerticalAlignment = xaml::VerticalAlignment::Center;
            r2->HorizontalAlignment = xaml::HorizontalAlignment::Right;
            r2->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        };
        config.NavigatedTo = [&](Microsoft::UI::Xaml::Controls::Page^ page, Microsoft::UI::Xaml::Navigation::NavigationEventArgs^ e)
        {
            auto result = animation->TryStart(r2);
            VERIFY_IS_TRUE(result);
        };

        // Note, the navigation transition used here must not delay the start of the "to page" animation, since the purpose
        // of this test is to determine if we compute the target location of the destination element instead of the
        // actual location.
        LOG_OUTPUT(L"Navigating from main page to the target page with Slide (from bottom) transition");
        p1->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, ref new xaml_animation::SlideNavigationTransitionInfo());
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        animation->Cancel();
    });
    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::TextAnimations()
{
    int testAnimationFilterIndex = -1;  // This value can be changed to get the test to run a single animation

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;
    CompletionHelper completionHelper;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
    xaml_controls::Grid^ rootPanel;

    LPCWSTR leftNames[] = { L"NormalScale", L"CombinedScale", L"CenterAligned", L"LocalFlip", L"LocalRTL", L"UnclippedText" };
    LPCWSTR rightNames[] = { L"ParentScale", L"LocalScale", L"RightAligned", L"ParentFlip", L"ParentRTL", L"ClippedText" };
    static_assert(_countof(leftNames) == _countof(rightNames), "Name arrays must be of the same size");

    Panel^ panel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "ConnectedAnimationText.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = panel;
    });

    LOG_OUTPUT(L"Animating from left to right");
    StartAnimations(_countof(leftNames), leftNames, rightNames, false /*suppressScaleAnimation*/, L"ForwardAnimate", testAnimationFilterIndex);
    LOG_OUTPUT(L"Animating from right to left");
    StartAnimations(_countof(leftNames), rightNames, leftNames, false /*suppressScaleAnimation*/, L"ReverseAnimate", testAnimationFilterIndex);
    LOG_OUTPUT(L"Animating from left to right no scaling");
    StartAnimations(_countof(leftNames), leftNames, rightNames, true /*suppressScaleAnimation*/, L"ForwardAnimateSuppressScaling", testAnimationFilterIndex);
    LOG_OUTPUT(L"Animating from right to left no scaling");
    StartAnimations(_countof(leftNames), rightNames, leftNames, true /*suppressScaleAnimation*/, L"ReverseAnimateSuppressScaling", testAnimationFilterIndex);
}

void ConnectedAnimationTests::ImageAnimations()
{
    int testAnimationFilterIndex = -1;  // This value can be changed to get the test to run a single animation

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;
    CompletionHelper completionHelper;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
    xaml_controls::Grid^ rootPanel;

    LPCWSTR leftNames[] = { L"NormalScale", L"CombinedScale", L"CenterAligned", L"LocalFlip", L"LocalRTL", L"UnclippedImage" };
    LPCWSTR rightNames[] = { L"ParentScale", L"LocalScale", L"RightAligned", L"ParentFlip", L"ParentRTL", L"ClippedImage" };
    static_assert(_countof(leftNames) == _countof(rightNames), "Name arrays must be of the same size");

    Panel^ panel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "ConnectedAnimationImage.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = panel;
    });

    LOG_OUTPUT(L"Animating from left to right");
    StartAnimations(_countof(leftNames), leftNames, rightNames, false /*suppressScaleAnimation*/, L"ForwardAnimate", testAnimationFilterIndex);
    LOG_OUTPUT(L"Animating from right to left");
    StartAnimations(_countof(leftNames), rightNames, leftNames, false /*suppressScaleAnimation*/, L"ReverseAnimate", testAnimationFilterIndex);
    LOG_OUTPUT(L"Animating from left to right no scaling");
    StartAnimations(_countof(leftNames), leftNames, rightNames, true /*suppressScaleAnimation*/, L"ForwardAnimateSuppressScaling", testAnimationFilterIndex);
    LOG_OUTPUT(L"Animating from right to left no scaling");
    StartAnimations(_countof(leftNames), rightNames, leftNames, true /*suppressScaleAnimation*/, L"ReverseAnimateSuppressScaling", testAnimationFilterIndex);
}

void ConnectedAnimationTests::TextRTLAnimations()
{
    int testAnimationFilterIndex = -1;  // This value can be changed to get the test to run a single animation

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;
    CompletionHelper completionHelper;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
    xaml_controls::Grid^ rootPanel;

    LPCWSTR leftNames[] = { L"ParentRTLParentScale", L"ParentRTLLocalScale", L"ParentRTLCombinedScale", L"ParentRTLParentFlip", L"ParentRTLLocalFlip", L"ParentRTLLocalLTR" };
    LPCWSTR rightNames[] = { L"LocalRTLParentScale", L"LocalRTLLocalScale", L"LocalRTLCombinedScale", L"LocalRTLParentFlip", L"LocalRTLLocalFlip", L"ParentRTLLocalLTRScale" };
    static_assert(_countof(leftNames) == _countof(rightNames), "Name arrays must be of the same size");

    Panel^ panel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "ConnectedAnimationTextRTL.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = panel;
    });

    LOG_OUTPUT(L"Animating from left to right");
    StartAnimations(_countof(leftNames), leftNames, rightNames, false /*suppressScaleAnimation*/, L"ForwardAnimate", testAnimationFilterIndex);
    LOG_OUTPUT(L"Animating from right to left");
    StartAnimations(_countof(leftNames), rightNames, leftNames, false /*suppressScaleAnimation*/, L"ReverseAnimate", testAnimationFilterIndex);
    LOG_OUTPUT(L"Animating from left to right no scaling");
    StartAnimations(_countof(leftNames), leftNames, rightNames, true /*suppressScaleAnimation*/, L"ForwardAnimateSuppressScaling", testAnimationFilterIndex);
    LOG_OUTPUT(L"Animating from right to left no scaling");
    StartAnimations(_countof(leftNames), rightNames, leftNames, true /*suppressScaleAnimation*/, L"ReverseAnimateSuppressScaling", testAnimationFilterIndex);
}

void  ConnectedAnimationTests::StartAnimations(unsigned int nameCount, _In_count_(nameCount) LPCWSTR* sourceNames, _In_count_(nameCount) LPCWSTR* destinationNames, bool suppressScaleAnimation, _In_ LPCWSTR filetag, int filterIndex)
{
    ConnectedAnimationServiceDurationHelper durationHelper;
    CompletionHelper completionHelper;

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        Panel^ rootPanel = safe_cast<Panel^>(TestServices::WindowHelper->WindowContent);
        xaml_animation::ConnectedAnimationService^ service = xaml_animation::ConnectedAnimationService::GetForCurrentView();

        for (int i = 0; i < static_cast<int>(nameCount); i++)
        {
            if (filterIndex >= 0 && filterIndex != i) continue;

            LOG_OUTPUT(L"Prepare to animate %s", sourceNames[i]);
            UIElement^ source = safe_cast<UIElement^>(rootPanel->FindName(ref new Platform::String(sourceNames[i])));
            VERIFY_IS_NOT_NULL(source);

            ConnectedAnimation^ animation = service->PrepareToAnimate(ref new Platform::String(sourceNames[i]), source);
            VERIFY_IS_NOT_NULL(animation);
            animation->IsScaleAnimationEnabled = !suppressScaleAnimation;

            completionHelper.AddAnimation(animation);
        }
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        Panel^ rootPanel = safe_cast<Panel^>(TestServices::WindowHelper->WindowContent);
        xaml_animation::ConnectedAnimationService^ service = xaml_animation::ConnectedAnimationService::GetForCurrentView();

        for (int i = 0; i < static_cast<int>(nameCount); i++)
        {
            if (filterIndex >= 0 && filterIndex != i) continue;

            LOG_OUTPUT(L"Start animation %s to %s", sourceNames[i], destinationNames[i]);

            ConnectedAnimation^ animation = service->GetAnimation(ref new Platform::String(sourceNames[i]));

            UIElement^ destination = safe_cast<UIElement^>(rootPanel->FindName(ref new Platform::String(destinationNames[i])));
            VERIFY_IS_NOT_NULL(destination);

            VERIFY_IS_TRUE(animation->TryStart(destination));
        }
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, ref new Platform::String(filetag));
    TestServices::WindowHelper->WaitForIdle();

    // Let the animations run and wait for completion
    {
        std::shared_ptr<Event> completionEvent = std::make_shared<Event>();
        completionHelper.SetCompletionEvent(completionEvent);
        durationHelper.WaitForCompletionEvent(completionEvent.get());
    }
    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::MultipleDeletes()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));
    xaml_controls::Grid^ rootPanel;
    xaml_controls::Grid^ sourcePanel;
    xaml_controls::Grid^ destinationPanel;

    RunOnUIThread([&]()
    {
        rootPanel = ref new Grid();

        sourcePanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"   <Grid x:Name='WrapperElement' HorizontalAlignment='Left' VerticalAlignment='Top'>"
            L"      <Grid x:Name='SourceElement' Margin='10,10,10,10' Height='50' Width='50' Background='Yellow'/>"
            L"   </Grid>"
            L"</Grid>"
        ));
        destinationPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"   <Grid x:Name='DestinationElement' Margin='10,10,10,10' Height='200' Width='200' HorizontalAlignment='Right' VerticalAlignment='Bottom' Background='Green'/>"
            L"</Grid>"
        ));
        rootPanel->Children->Append(sourcePanel);
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    ConnectedAnimation^ animation;
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_controls::Grid^ source = safe_cast<xaml_controls::Grid^>(sourcePanel->FindName(L"SourceElement"));
        xaml_controls::Grid^ destination = safe_cast<xaml_controls::Grid^>(destinationPanel->FindName(L"DestinationElement"));
        VERIFY_IS_NOT_NULL(source);
        VERIFY_IS_NOT_NULL(destination);

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        LOG_OUTPUT(L"Remove the source panel and add the destination");
        rootPanel->Children->Clear();
        rootPanel->Children->Append(destinationPanel);

        LOG_OUTPUT(L"Remove the wrapper panel");
        sourcePanel->Children->Clear();

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });

    caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e)
    {
        LOG_OUTPUT(L"Completed Event Fired");
        caCompletedEvent->Set();
    }));

    durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
    TestServices::WindowHelper->WaitForIdle();

    // Verifying that there are no extra visual hanging around since that would indicate that we leaked
    // something.
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ConnectedAnimationTests::ScrollIntoView()
{

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ConnectedAnimationServiceDurationHelper durationHelper;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));
    xaml_controls::Grid^ rootPanel;
    xaml_controls::Grid^ sourcePanel;
    xaml_controls::Grid^ destinationPanel;

    RunOnUIThread([&]()
    {
        rootPanel = ref new Grid();

        sourcePanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Margin='20,20,20,20'>"
            L"   <TextBlock x:Name='SourceElement' Text='Source Text' FontSize='50' HorizontalAlignment='Right' VerticalAlignment='Top'/>"
            L"</Grid>"
        ));
        destinationPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"   <ScrollViewer x:Name='Viewer' VerticalScrollMode='Auto' VerticalScrollBarVisibility='Hidden'>"
            L"      <StackPanel Margin='20,10,20,10' HorizontalAlignment='Right' VerticalAlignment='Top'>"
            L"         <Border Height='200' HorizontalAlignment='Stretch' Background='Blue'/>"
            L"         <Border Height='200' HorizontalAlignment='Stretch' Background='Green'/>"
            L"         <Border Height='200' HorizontalAlignment='Stretch' Background='Yellow'/>"
            L"         <Border Height='200' HorizontalAlignment='Stretch' Background='Orange'/>"
            L"         <Border Height='200' HorizontalAlignment='Stretch' Background='Red'/>"
            L"         <Border Height='200' HorizontalAlignment='Stretch' Background='Purple'/>"
            L"         <TextBlock x:Name='DestinationElement' Text='Destination Text' FontSize='50' HorizontalAlignment='Left'/>"
            L"      </StackPanel>"
            L"   </ScrollViewer>"
            L"</Grid>"
        ));
        rootPanel->Children->Append(sourcePanel);
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    auto loadedEvent = std::make_shared<Event>();
    auto viewChangeComplete = std::make_shared<Event>();
    auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, Loaded);
    auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

    ConnectedAnimation^ animation;
    RunOnUIThread([&]()
    {
        xaml_animation::ConnectedAnimationService^ service;
        xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(sourcePanel->FindName(L"SourceElement"));
        xaml_controls::TextBlock^ destination = safe_cast<xaml_controls::TextBlock^>(destinationPanel->FindName(L"DestinationElement"));
        xaml_controls::ScrollViewer^ viewer = safe_cast<xaml_controls::ScrollViewer^>(destinationPanel->FindName(L"Viewer"));
        VERIFY_IS_NOT_NULL(source);
        VERIFY_IS_NOT_NULL(destination);
        VERIFY_IS_NOT_NULL(viewer);

        viewChangedRegistration.Attach(viewer, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
            [&viewChangeComplete](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
        {
            // The animation is complete, we can look at the DComp tree now.
            if (!args->IsIntermediate)
            {
                LOG_OUTPUT(L"Manipulation Complete");
                viewChangeComplete->Set();
            }
        }));

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        // Set the duration to an extremely long time so that nothing actually animates
        LOG_OUTPUT(L"Setting default duration to be 24 hours");
        ::Windows::Foundation::TimeSpan ts1 = { 864000000000 };
        service->DefaultDuration = ts1;

        LOG_OUTPUT(L"Prepare the animation");
        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        // There is currently an issue with BringIntoView where the element must already be active in the tree
        // before StartBringIntoView will schedule the event needed to actually bring it into view.  The issue
        // arises if the element is beneath a control that needs template expansion (such as a ScrollViewer).
        // In the case, the element doesn't actually enter the tree until layout runs and the template is
        // expanded, so the BringIntoView becomes a no-op.  To work around this for this test, we will
        // explicitly wait for the element to load before calling bring into view and starting the
        // animation.  This is not optimal in real life, because if the element is already in view, it will
        // flash before the connected animation begins.
        loadedRegistration.Attach(destination,
                ref new xaml::RoutedEventHandler([loadedEvent, destination, animation](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"destination loaded");
            loadedEvent->Set();

            LOG_OUTPUT(L"Bring destination into view");
            destination->StartBringIntoView();

            LOG_OUTPUT(L"Start Animation");
            auto result = animation->TryStart(destination);
            VERIFY_IS_TRUE(result);
        }));

        LOG_OUTPUT(L"Remove the source panel and add the destination");
        rootPanel->Children->Clear();
        rootPanel->Children->Append(destinationPanel);
    });

    LOG_OUTPUT(L"Waiting for load event");
    loadedEvent->WaitForDefault();

    LOG_OUTPUT(L"Waiting for manipulation completed event");
    viewChangeComplete->WaitForDefault();

    // Verify the output
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        animation->Cancel();
    });

    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::BasicConnectedAnimationConfiguration()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_animation::BasicConnectedAnimationConfiguration^ configuration;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Basic Configuration");
        configuration = ref new xaml_animation::BasicConnectedAnimationConfiguration();
    });
    ProcessConfigurationTest(configuration);
}

void ConnectedAnimationTests::DirectConnectedAnimationConfiguration()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_animation::DirectConnectedAnimationConfiguration^ configuration;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Direct Configuration");
        configuration = ref new xaml_animation::DirectConnectedAnimationConfiguration();
    });
    ProcessConfigurationTest(configuration);
}

void ConnectedAnimationTests::GravityConnectedAnimationConfiguration()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_animation::GravityConnectedAnimationConfiguration^ configuration;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Gravity Configuration");
        configuration = ref new xaml_animation::GravityConnectedAnimationConfiguration();
        VERIFY_IS_TRUE(configuration->IsShadowEnabled);
    });
    ProcessConfigurationTest(configuration, L"");
}

void ConnectedAnimationTests::GravityConnectedAnimationConfigurationNoShadow()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_animation::GravityConnectedAnimationConfiguration^ configuration;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Gravity Configuration");
        configuration = ref new xaml_animation::GravityConnectedAnimationConfiguration();
        LOG_OUTPUT(L"Disabling shadow");

        configuration->IsShadowEnabled = false;
        VERIFY_IS_FALSE(configuration->IsShadowEnabled);
    });
    ProcessConfigurationTest(configuration, "NoShadow");
}

void ConnectedAnimationTests::GravityConnectedAnimationConfigurationSameSize()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_animation::GravityConnectedAnimationConfiguration^ configuration;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Gravity Configuration");
        configuration = ref new xaml_animation::GravityConnectedAnimationConfiguration();
    });
    ProcessConfigurationTest(configuration, L"", true);
}

void ConnectedAnimationTests::DefaultConnectedAnimationConfiguration()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    // Disable the default disablement of the configuration so we can test whether we picked up the real default.
    LOG_OUTPUT(L"Re-enabling the default connected animation configuration");
    m_featureDisableDefaultConnectedAnimationConfiguration.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableDefaultConnectedAnimationConfiguration, false);

    ProcessConfigurationTest(nullptr, L"");

}
void ConnectedAnimationTests::ProcessConfigurationTest(_In_ xaml_animation::ConnectedAnimationConfiguration^ configuration, _In_opt_ Platform::String^ outputSuffix, bool makeSame)
{
    ConnectedAnimationServiceDurationHelper durationHelper;
    xaml_controls::Grid^ rootPanel = CreateTestPageContent();
    xaml_animation::ConnectedAnimation^ animation;

    if (makeSame)
    {
        RunOnUIThread([&]()
        {
            xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
            VERIFY_IS_NOT_NULL(source);
            xaml_controls::TextBlock^ destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));
            VERIFY_IS_NOT_NULL(destination);
            source->FontSize = destination->FontSize;
            source->Text = destination->Text;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    RunOnUIThread([&]()
    {
        xaml_controls::TextBlock^ source = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"SourceElement"));
        VERIFY_IS_NOT_NULL(source);
        xaml_controls::TextBlock^ destination = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"DestinationElement"));
        VERIFY_IS_NOT_NULL(destination);

        xaml_animation::ConnectedAnimationService^ service;
        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        if (configuration != nullptr)
        {
            animation->Configuration = configuration;
        }

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, outputSuffix);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Cancel Animation");
        animation->Cancel();
        animation = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();
}

void ConnectedAnimationTests::AnimateFromContentDialog()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    ConnectedAnimationServiceDurationHelper durationHelper;
    xaml_controls::Grid^ rootPanel;
    xaml_controls::ContentDialog^ contentDialog;

    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <ContentDialog x:Name='Dialog' Padding='20,20,20,20'>"
            L"        <Rectangle x:Name='SourceElement' Height='100' Width='100' Fill='Red'/>"
            L"    </ContentDialog>"
            L"    <Rectangle x:Name='DestinationElement' Height='200' Width='200' Fill='Pink' Visibility='Collapsed' HorizontalAlignment='Left' VerticalAlignment='Top'/>"
            L"</Grid>"
        ));
        VERIFY_IS_NOT_NULL(rootPanel);
        contentDialog = safe_cast<xaml_controls::ContentDialog^>(rootPanel->FindName(L"Dialog"));
        VERIFY_IS_NOT_NULL(contentDialog);
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        contentDialog->ShowAsync(Microsoft::UI::Xaml::Controls::ContentDialogPlacement::Popup);
    });
    TestServices::WindowHelper->WaitForIdle();

    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        Shapes::Rectangle^ source = safe_cast<Shapes::Rectangle^>(rootPanel->FindName(L"SourceElement"));

        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        contentDialog->Hide();

        Shapes::Rectangle^ destination = safe_cast<Shapes::Rectangle^>(rootPanel->FindName(L"DestinationElement"));
        destination->Visibility = Visibility::Visible;

        caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e)
        {
            LOG_OUTPUT(L"Completed Event Fired");
            caCompletedEvent->Set();
        }));

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });
    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Wait for Completion Event");
    durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
}

void ConnectedAnimationTests::AnimateFromPopup()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    ConnectedAnimationServiceDurationHelper durationHelper;
    xaml_controls::Grid^ rootPanel;
    xaml_controls::Primitives::Popup^ popup;

    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"   <Popup x:Name='Popup' HorizontalAlignment='Center' VerticalAlignment='Center' IsOpen='True'>"
            L"        <Rectangle x:Name='SourceElement' Height='100' Width='100' Fill='Red'/>"
            L"    </Popup>"
            L"    <Rectangle x:Name='DestinationElement' Height='200' Width='200' Fill='Pink' Visibility='Collapsed' HorizontalAlignment='Left' VerticalAlignment='Top'/>"
            L"</Grid>"
        ));
        VERIFY_IS_NOT_NULL(rootPanel);
        popup = safe_cast<xaml_controls::Primitives::Popup^>(rootPanel->FindName(L"Popup"));
        VERIFY_IS_NOT_NULL(popup);
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        Shapes::Rectangle^ source = safe_cast<Shapes::Rectangle^>(rootPanel->FindName(L"SourceElement"));

        xaml_animation::ConnectedAnimationService^ service;
        xaml_animation::ConnectedAnimation^ animation;

        LOG_OUTPUT(L"Retrieve the animation service");
        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);

        animation = service->PrepareToAnimate(L"Test", source);
        VERIFY_IS_NOT_NULL(animation);

        popup->IsOpen = false;

        Shapes::Rectangle^ destination = safe_cast<Shapes::Rectangle^>(rootPanel->FindName(L"DestinationElement"));
        destination->Visibility = Visibility::Visible;

        caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Object^ e)
        {
            LOG_OUTPUT(L"Completed Event Fired");
            caCompletedEvent->Set();
        }));

        LOG_OUTPUT(L"Start Animation");
        auto result = animation->TryStart(destination);
        VERIFY_IS_TRUE(result);
    });
    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Wait for Completion Event");
    durationHelper.WaitForCompletionEvent(caCompletedEvent.get());
}

} } } } } }
