// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ThumbIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>

#undef GetCurrentTime
#include <Microsoft.UI.Xaml.h>

#include <cmath>
#include <cstring>
#include <limits>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace abi_primitives = ABI::Microsoft::UI::Xaml::Controls::Primitives;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace Thumb {

    // An app-defined class deriving from the unsealed DragCompletedEventArgs runtime class. Constructing it
    // activates DragCompletedEventArgs as the composed inner object of this outer object.
    ref class DerivedDragCompletedEventArgs sealed : public xaml_primitives::DragCompletedEventArgs
    {
    public:
        DerivedDragCompletedEventArgs(double horizontalChange, double verticalChange, bool canceled)
            : xaml_primitives::DragCompletedEventArgs(horizontalChange, verticalChange, canceled)
        {
        }
    };

    // A plain app object used as the controlling outer object in ABI factory calls.
    ref class AggregationOuter sealed
    {
    };

    namespace
    {
        // Maximum difference between the requested pointer offset and the reported change. Injected pointer
        // positions are rounded to whole physical pixels at both ends of the drag.
        const double c_pointerRoundingTolerance = 1.5;

        // DragDelta values are accumulated as doubles from float positions.
        const double c_accumulationTolerance = 0.01;

        struct DragObservations
        {
            int startedCount = 0;
            int deltaCount = 0;
            double deltaSumX = 0.0;
            double deltaSumY = 0.0;
            int completedCount = 0;
            int deltaCountAtCompletion = -1;
            double completedHorizontalChange = 0.0;
            double completedVerticalChange = 0.0;
            bool completedCanceled = false;
            bool completedSenderIsThumb = false;
            bool isDraggingAfterRelease = true;
            std::shared_ptr<Event> completedEvent = std::make_shared<Event>();
        };

        uintptr_t GetIdentity(Platform::Object^ object)
        {
            wrl::ComPtr<IUnknown> identity;
            if (object == nullptr || FAILED(reinterpret_cast<IInspectable*>(object)->QueryInterface(IID_PPV_ARGS(&identity))))
            {
                return 0;
            }

            return reinterpret_cast<uintptr_t>(identity.Get());
        }

        uintptr_t GetIdentity(IUnknown* object)
        {
            wrl::ComPtr<IUnknown> identity;
            if (object == nullptr || FAILED(object->QueryInterface(IID_PPV_ARGS(&identity))))
            {
                return 0;
            }

            return reinterpret_cast<uintptr_t>(identity.Get());
        }

        // Returns a guard that releases the left mouse button over the Thumb if the test leaves the pressed interval
        // early (a failed wait or verification), so the button is not left pressed for the following tests. It must
        // not throw while the stack unwinds. Call release() on it after the normal MouseButtonUp.
        auto ReleaseMouseOnExit(xaml_primitives::Thumb^ thumb)
        {
            return wil::scope_exit([thumb]()
            {
                try
                {
                    TestServices::InputHelper->MouseButtonUp(thumb, 0, 0, MouseButton::Left);
                }
                catch (...)
                {
                }
            });
        }

        uint64_t GetBits(double value)
        {
            uint64_t bits = 0;
            static_assert(sizeof(bits) == sizeof(value), "double must be 64 bits");
            std::memcpy(&bits, &value, sizeof(bits));
            return bits;
        }

        // Verifies that a double came back unchanged. NaN is compared by class because the contract is that it
        // stays NaN; every other value, including the sign of zero, must be bit-for-bit identical.
        void VerifyDoublePreserved(double expected, double actual, const wchar_t* axis)
        {
            if (std::isnan(expected))
            {
                VERIFY_IS_TRUE(std::isnan(actual), WEX::Common::String().Format(L"%s: expected NaN, got %g", axis, actual));
            }
            else
            {
                VERIFY_ARE_EQUAL(GetBits(expected), GetBits(actual),
                    WEX::Common::String().Format(L"%s: expected %g (bits 0x%016llx), got %g (bits 0x%016llx)",
                        axis, expected, GetBits(expected), actual, GetBits(actual)));
            }
        }

        xaml_primitives::Thumb^ CreateThumbInCanvas()
        {
            xaml_primitives::Thumb^ thumb = nullptr;

            RunOnUIThread([&]()
            {
                auto canvas = ref new xaml_controls::Canvas();
                canvas->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::White);

                thumb = ref new xaml_primitives::Thumb();
                thumb->Width = 80;
                thumb->Height = 80;
                thumb->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
                xaml_controls::Canvas::SetLeft(thumb, 100);
                xaml_controls::Canvas::SetTop(thumb, 100);

                canvas->Children->Append(thumb);
                TestServices::WindowHelper->WindowContent = canvas;
            });
            TestServices::WindowHelper->WaitForIdle();

            return thumb;
        }

        void AttachDragHandlers(
            xaml_primitives::Thumb^ thumb,
            const std::shared_ptr<DragObservations>& observations,
            SafeEventRegistrationType(xaml_primitives::Thumb, DragStarted)& startedRegistration,
            SafeEventRegistrationType(xaml_primitives::Thumb, DragDelta)& deltaRegistration,
            SafeEventRegistrationType(xaml_primitives::Thumb, DragCompleted)& completedRegistration)
        {
            uintptr_t thumbIdentity = 0;
            RunOnUIThread([&]()
            {
                thumbIdentity = GetIdentity(thumb);
            });
            VERIFY_ARE_NOT_EQUAL(static_cast<uintptr_t>(0), thumbIdentity);

            startedRegistration.Attach(thumb, ref new xaml_primitives::DragStartedEventHandler(
                [observations](Platform::Object^, xaml_primitives::DragStartedEventArgs^)
                {
                    ++observations->startedCount;
                }));

            deltaRegistration.Attach(thumb, ref new xaml_primitives::DragDeltaEventHandler(
                [observations](Platform::Object^, xaml_primitives::DragDeltaEventArgs^ args)
                {
                    ++observations->deltaCount;
                    observations->deltaSumX += args->HorizontalChange;
                    observations->deltaSumY += args->VerticalChange;
                }));

            completedRegistration.Attach(thumb, ref new xaml_primitives::DragCompletedEventHandler(
                [observations, thumbIdentity](Platform::Object^ sender, xaml_primitives::DragCompletedEventArgs^ args)
                {
                    ++observations->completedCount;
                    observations->deltaCountAtCompletion = observations->deltaCount;
                    observations->completedHorizontalChange = args->HorizontalChange;
                    observations->completedVerticalChange = args->VerticalChange;
                    observations->completedCanceled = args->Canceled;
                    observations->completedSenderIsThumb = (GetIdentity(sender) == thumbIdentity);
                    observations->completedEvent->Set();
                }));
        }

        // Verifies the DragCompleted event raised for a drag that moved the pointer by about (expectedX, expectedY).
        void VerifyCompletedChanges(const DragObservations& observations, double expectedX, double expectedY)
        {
            LOG_OUTPUT(L"DragCompleted: HorizontalChange=%f VerticalChange=%f Canceled=%d; DragDelta count=%d sum=(%f, %f); expected about (%f, %f)",
                observations.completedHorizontalChange, observations.completedVerticalChange, observations.completedCanceled,
                observations.deltaCount, observations.deltaSumX, observations.deltaSumY, expectedX, expectedY);

            VERIFY_IS_TRUE(observations.completedSenderIsThumb);

            // The completed changes are the total displacement of the drag, which is the sum of every DragDelta.
            VERIFY_IS_LESS_THAN(std::abs(observations.completedHorizontalChange - observations.deltaSumX), c_accumulationTolerance);
            VERIFY_IS_LESS_THAN(std::abs(observations.completedVerticalChange - observations.deltaSumY), c_accumulationTolerance);

            // ...and matches the pointer displacement on each axis, including its sign.
            VERIFY_IS_LESS_THAN(std::abs(observations.completedHorizontalChange - expectedX), c_pointerRoundingTolerance);
            VERIFY_IS_LESS_THAN(std::abs(observations.completedVerticalChange - expectedY), c_pointerRoundingTolerance);
        }

        // Presses the left mouse button on the center of a Thumb, moves it through the given offsets from that
        // center and releases it. Validates the drag state while the button is pressed and returns the events
        // observed for the whole drag, after its handlers are detached.
        std::shared_ptr<DragObservations> PerformMouseDrag(const std::vector<std::pair<int, int>>& offsetsFromCenter)
        {
            auto thumb = CreateThumbInCanvas();
            auto observations = std::make_shared<DragObservations>();
            auto startedRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, DragStarted);
            auto deltaRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, DragDelta);
            auto completedRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, DragCompleted);
            AttachDragHandlers(thumb, observations, startedRegistration, deltaRegistration, completedRegistration);

            TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
            auto mouseReleaseGuard = ReleaseMouseOnExit(thumb);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(thumb->IsDragging);
                VERIFY_ARE_EQUAL(1, observations->startedCount);
                VERIFY_ARE_EQUAL(0, observations->completedCount);
            });

            for (const auto& offset : offsetsFromCenter)
            {
                LOG_OUTPUT(L"Moving the pointer to (%d, %d) from the Thumb's center.", offset.first, offset.second);
                TestServices::InputHelper->MoveMouse(thumb, offset.first, offset.second);
                TestServices::WindowHelper->WaitForIdle();
            }

            RunOnUIThread([&]()
            {
                // Moving does not complete the drag.
                VERIFY_IS_TRUE(thumb->IsDragging);
                VERIFY_IS_GREATER_THAN_OR_EQUAL(observations->deltaCount, 1);
                VERIFY_ARE_EQUAL(0, observations->completedCount);
            });

            const auto& last = offsetsFromCenter.back();
            TestServices::InputHelper->MouseButtonUp(thumb, last.first, last.second, MouseButton::Left);
            mouseReleaseGuard.release();
            observations->completedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                observations->isDraggingAfterRelease = thumb->IsDragging;
            });

            return observations;
        }
    }

    bool ThumbIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ThumbIntegrationTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ThumbIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ThumbIntegrationTests::DragCompletedEventArgsConstructorRoundTripsValues()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto canceledArgs = ref new xaml_primitives::DragCompletedEventArgs(12.5, -7.25, true);
            auto completedArgs = ref new xaml_primitives::DragCompletedEventArgs(-3.0, 640.75, false);

            VERIFY_ARE_EQUAL(12.5, canceledArgs->HorizontalChange);
            VERIFY_ARE_EQUAL(-7.25, canceledArgs->VerticalChange);
            VERIFY_IS_TRUE(canceledArgs->Canceled);

            VERIFY_ARE_EQUAL(-3.0, completedArgs->HorizontalChange);
            VERIFY_ARE_EQUAL(640.75, completedArgs->VerticalChange);
            VERIFY_IS_FALSE(completedArgs->Canceled);

            // Each instance keeps its own values.
            VERIFY_ARE_EQUAL(12.5, canceledArgs->HorizontalChange);
            VERIFY_ARE_EQUAL(-7.25, canceledArgs->VerticalChange);
        });
    }

    void ThumbIntegrationTests::DragCompletedEventArgsConstructorPreservesBoundaryDoubles()
    {
        TestCleanupWrapper cleanup;

        const double boundaryValues[] =
        {
            0.0,
            -0.0,
            (std::numeric_limits<double>::max)(),
            -(std::numeric_limits<double>::max)(),
            (std::numeric_limits<double>::min)(),
            std::numeric_limits<double>::denorm_min(),
            -std::numeric_limits<double>::denorm_min(),
            std::numeric_limits<double>::infinity(),
            -std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::quiet_NaN(),
        };

        RunOnUIThread([&]()
        {
            for (size_t i = 0; i < ARRAYSIZE(boundaryValues); ++i)
            {
                const double boundary = boundaryValues[i];
                // A distinct ordinary value on the other axis shows the axes are stored independently.
                const double other = 3.5 + static_cast<double>(i);
                const bool canceled = (i % 2) == 0;

                LOG_OUTPUT(L"Boundary value %g (bits 0x%016llx) with Canceled=%d", boundary, GetBits(boundary), canceled);

                auto horizontalArgs = ref new xaml_primitives::DragCompletedEventArgs(boundary, other, canceled);
                VerifyDoublePreserved(boundary, horizontalArgs->HorizontalChange, L"HorizontalChange");
                VerifyDoublePreserved(other, horizontalArgs->VerticalChange, L"VerticalChange");
                VERIFY_ARE_EQUAL(canceled, horizontalArgs->Canceled);

                auto verticalArgs = ref new xaml_primitives::DragCompletedEventArgs(other, boundary, !canceled);
                VerifyDoublePreserved(other, verticalArgs->HorizontalChange, L"HorizontalChange");
                VerifyDoublePreserved(boundary, verticalArgs->VerticalChange, L"VerticalChange");
                VERIFY_ARE_EQUAL(!canceled, verticalArgs->Canceled);
            }
        });
    }

    void ThumbIntegrationTests::DragCompletedEventArgsSupportsDerivedClass()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto first = ref new DerivedDragCompletedEventArgs(-15.5, 33.25, true);
            auto second = ref new DerivedDragCompletedEventArgs(0.25, -1.0, false);

            xaml_primitives::DragCompletedEventArgs^ firstAsBase = first;
            VERIFY_ARE_EQUAL(-15.5, firstAsBase->HorizontalChange);
            VERIFY_ARE_EQUAL(33.25, firstAsBase->VerticalChange);
            VERIFY_IS_TRUE(firstAsBase->Canceled);

            xaml_primitives::DragCompletedEventArgs^ secondAsBase = second;
            VERIFY_ARE_EQUAL(0.25, secondAsBase->HorizontalChange);
            VERIFY_ARE_EQUAL(-1.0, secondAsBase->VerticalChange);
            VERIFY_IS_FALSE(secondAsBase->Canceled);

            // The composed DragCompletedEventArgs belongs to the derived object: casting back through the base
            // projection returns the same derived object.
            auto firstRoundTrip = dynamic_cast<DerivedDragCompletedEventArgs^>(static_cast<Platform::Object^>(firstAsBase));
            VERIFY_IS_NOT_NULL(firstRoundTrip);
            VERIFY_ARE_EQUAL(GetIdentity(first), GetIdentity(firstRoundTrip));
            VERIFY_ARE_NOT_EQUAL(GetIdentity(first), GetIdentity(second));

            // Both derived types are routed event args.
            VERIFY_IS_NOT_NULL(dynamic_cast<xaml::RoutedEventArgs^>(static_cast<Platform::Object^>(second)));
        });
    }

    void ThumbIntegrationTests::DragCompletedEventArgsFactoryValidatesAbiArguments()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            wrl::ComPtr<abi_primitives::IDragCompletedEventArgsFactory> factory;
            VERIFY_SUCCEEDED(::RoGetActivationFactory(
                wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.Primitives.DragCompletedEventArgs").Get(),
                IID_PPV_ARGS(&factory)));
            VERIFY_IS_NOT_NULL(factory.Get());

            LOG_OUTPUT(L"A null instance out-pointer is rejected.");
            {
                wrl::ComPtr<IInspectable> inner;
                VERIFY_ARE_EQUAL(E_INVALIDARG, factory->CreateInstanceWithHorizontalChangeVerticalChangeAndCanceled(
                    1.0, 2.0, TRUE, nullptr, &inner, nullptr));
                VERIFY_IS_NULL(inner.Get());
            }

            LOG_OUTPUT(L"An outer object without an inner out-pointer is rejected.");
            {
                auto outer = ref new AggregationOuter();
                wrl::ComPtr<abi_primitives::IDragCompletedEventArgs> instance;
                VERIFY_ARE_EQUAL(E_UNEXPECTED, factory->CreateInstanceWithHorizontalChangeVerticalChangeAndCanceled(
                    1.0, 2.0, TRUE, reinterpret_cast<IInspectable*>(outer), nullptr, &instance));
                VERIFY_IS_NULL(instance.Get());
            }

            LOG_OUTPUT(L"Without an outer object, the returned inner object is the instance.");
            wrl::ComPtr<abi_primitives::IDragCompletedEventArgs> instance;
            {
                wrl::ComPtr<IInspectable> inner;
                VERIFY_SUCCEEDED(factory->CreateInstanceWithHorizontalChangeVerticalChangeAndCanceled(
                    3.5, -4.5, FALSE, nullptr, &inner, &instance));
                VERIFY_IS_NOT_NULL(inner.Get());
                VERIFY_IS_NOT_NULL(instance.Get());
                VERIFY_ARE_EQUAL(GetIdentity(inner.Get()), GetIdentity(instance.Get()));

                wrl::ComPtr<abi_primitives::IDragCompletedEventArgs> innerAsArgs;
                VERIFY_SUCCEEDED(inner.As(&innerAsArgs));
                DOUBLE innerHorizontalChange = 0.0;
                VERIFY_SUCCEEDED(innerAsArgs->get_HorizontalChange(&innerHorizontalChange));
                VERIFY_ARE_EQUAL(3.5, innerHorizontalChange);
            }

            DOUBLE horizontalChange = 0.0;
            DOUBLE verticalChange = 0.0;
            BOOLEAN canceled = TRUE;
            VERIFY_SUCCEEDED(instance->get_HorizontalChange(&horizontalChange));
            VERIFY_SUCCEEDED(instance->get_VerticalChange(&verticalChange));
            VERIFY_SUCCEEDED(instance->get_Canceled(&canceled));
            VERIFY_ARE_EQUAL(3.5, horizontalChange);
            VERIFY_ARE_EQUAL(-4.5, verticalChange);
            VERIFY_ARE_EQUAL(static_cast<BOOLEAN>(FALSE), canceled);

            LOG_OUTPUT(L"Getters reject null out-pointers.");
            VERIFY_ARE_EQUAL(E_INVALIDARG, instance->get_HorizontalChange(nullptr));
            VERIFY_ARE_EQUAL(E_INVALIDARG, instance->get_VerticalChange(nullptr));
            VERIFY_ARE_EQUAL(E_INVALIDARG, instance->get_Canceled(nullptr));

            LOG_OUTPUT(L"An instance created without an inner out-pointer is equivalent.");
            {
                wrl::ComPtr<abi_primitives::IDragCompletedEventArgs> plainInstance;
                VERIFY_SUCCEEDED(factory->CreateInstanceWithHorizontalChangeVerticalChangeAndCanceled(
                    -0.5, 9.0, TRUE, nullptr, nullptr, &plainInstance));
                VERIFY_IS_NOT_NULL(plainInstance.Get());
                VERIFY_ARE_NOT_EQUAL(GetIdentity(plainInstance.Get()), GetIdentity(instance.Get()));
                VERIFY_SUCCEEDED(plainInstance->get_HorizontalChange(&horizontalChange));
                VERIFY_SUCCEEDED(plainInstance->get_VerticalChange(&verticalChange));
                VERIFY_SUCCEEDED(plainInstance->get_Canceled(&canceled));
                VERIFY_ARE_EQUAL(-0.5, horizontalChange);
                VERIFY_ARE_EQUAL(9.0, verticalChange);
                VERIFY_ARE_EQUAL(static_cast<BOOLEAN>(TRUE), canceled);
            }
        });
    }

    void ThumbIntegrationTests::DragCompletedEventArgsRejectsCrossThreadAccess()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::DragCompletedEventArgs^ args = nullptr;
        wrl::ComPtr<abi_primitives::IDragCompletedEventArgs> abiArgs;
        DWORD uiThreadId = 0;

        RunOnUIThread([&]()
        {
            uiThreadId = ::GetCurrentThreadId();
            args = ref new xaml_primitives::DragCompletedEventArgs(21.5, -8.25, true);
            VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(args)->QueryInterface(IID_PPV_ARGS(&abiArgs)));
        });

        // The test body runs on a thread other than the UI thread that created the args.
        VERIFY_ARE_NOT_EQUAL(uiThreadId, ::GetCurrentThreadId());

        VERIFY_THROWS_SPECIFIC_WINRT((void)args->HorizontalChange, Platform::Exception^,
            [](Platform::Exception^ ex) { return ex->HResult == RPC_E_WRONG_THREAD; });
        VERIFY_THROWS_SPECIFIC_WINRT((void)args->VerticalChange, Platform::Exception^,
            [](Platform::Exception^ ex) { return ex->HResult == RPC_E_WRONG_THREAD; });
        VERIFY_THROWS_SPECIFIC_WINRT((void)args->Canceled, Platform::Exception^,
            [](Platform::Exception^ ex) { return ex->HResult == RPC_E_WRONG_THREAD; });

        // At the ABI, a failed get does not leave the caller's previous value in the out-parameter.
        DOUBLE horizontalChange = 42.0;
        DOUBLE verticalChange = 42.0;
        VERIFY_ARE_EQUAL(RPC_E_WRONG_THREAD, abiArgs->get_HorizontalChange(&horizontalChange));
        VERIFY_ARE_EQUAL(RPC_E_WRONG_THREAD, abiArgs->get_VerticalChange(&verticalChange));
        VERIFY_ARE_EQUAL(0.0, horizontalChange);
        VERIFY_ARE_EQUAL(0.0, verticalChange);

        // The failed cross-thread reads leave the object intact for its own thread.
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(21.5, args->HorizontalChange);
            VERIFY_ARE_EQUAL(-8.25, args->VerticalChange);
            VERIFY_IS_TRUE(args->Canceled);

            abiArgs.Reset();
            args = nullptr;
        });
    }

    void ThumbIntegrationTests::DragCompletedReportsNetDisplacementOfMouseDrag()
    {
        TestCleanupWrapper cleanup;

        // Right/down, then partly back left: the net change (25, 30) differs from the last DragDelta (-15, 20)
        // and from the summed absolute movement (55, 30).
        LOG_OUTPUT(L"Drag right and down.");
        auto rightDown = PerformMouseDrag({ { 40, 10 }, { 25, 30 } });
        VERIFY_IS_FALSE(rightDown->isDraggingAfterRelease);
        VERIFY_ARE_EQUAL(1, rightDown->startedCount);
        VERIFY_ARE_EQUAL(1, rightDown->completedCount);
        // DragCompleted is raised after every DragDelta of the drag.
        VERIFY_ARE_EQUAL(rightDown->deltaCount, rightDown->deltaCountAtCompletion);
        VERIFY_IS_FALSE(rightDown->completedCanceled);
        VerifyCompletedChanges(*rightDown, 25, 30);

        LOG_OUTPUT(L"Drag left and up.");
        auto leftUp = PerformMouseDrag({ { -30, -45 }, { -50, -20 } });
        VERIFY_IS_FALSE(leftUp->isDraggingAfterRelease);
        VERIFY_ARE_EQUAL(1, leftUp->startedCount);
        VERIFY_ARE_EQUAL(1, leftUp->completedCount);
        VERIFY_ARE_EQUAL(leftUp->deltaCount, leftUp->deltaCountAtCompletion);
        VERIFY_IS_FALSE(leftUp->completedCanceled);
        VerifyCompletedChanges(*leftUp, -50, -20);
    }

    void ThumbIntegrationTests::CancelDragRaisesCanceledDragCompleted()
    {
        TestCleanupWrapper cleanup;

        auto thumb = CreateThumbInCanvas();
        auto observations = std::make_shared<DragObservations>();
        auto startedRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, DragStarted);
        auto deltaRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, DragDelta);
        auto completedRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, DragCompleted);
        AttachDragHandlers(thumb, observations, startedRegistration, deltaRegistration, completedRegistration);

        LOG_OUTPUT(L"CancelDrag without a drag in progress does nothing.");
        RunOnUIThread([&]()
        {
            thumb->CancelDrag();
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(0, observations->completedCount);
        });

        TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
        auto mouseReleaseGuard = ReleaseMouseOnExit(thumb);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MoveMouse(thumb, 35, -25);
        TestServices::WindowHelper->WaitForIdle();

        int deltaCountAtCancel = 0;
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(1, observations->startedCount);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(observations->deltaCount, 1);
            VERIFY_ARE_EQUAL(0, observations->completedCount);

            LOG_OUTPUT(L"Cancel the drag while the mouse button is still pressed.");
            thumb->CancelDrag();

            // CancelDrag ends the drag and raises DragCompleted before it returns.
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(1, observations->completedCount);
            VERIFY_IS_TRUE(observations->completedCanceled);
            VERIFY_ARE_EQUAL(observations->deltaCount, observations->deltaCountAtCompletion);
            VerifyCompletedChanges(*observations, 35, -25);

            deltaCountAtCancel = observations->deltaCount;
        });

        LOG_OUTPUT(L"Moving and releasing the pointer after the cancel does not resume or complete the drag again.");
        TestServices::InputHelper->MoveMouse(thumb, 60, 45);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MouseButtonUp(thumb, 60, 45, MouseButton::Left);
        mouseReleaseGuard.release();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(1, observations->startedCount);
            VERIFY_ARE_EQUAL(deltaCountAtCancel, observations->deltaCount);
            VERIFY_ARE_EQUAL(1, observations->completedCount);

            thumb->CancelDrag();
            VERIFY_ARE_EQUAL(1, observations->completedCount);
        });
    }

    void ThumbIntegrationTests::DisablingThumbDuringDragCompletesWithoutCancel()
    {
        TestCleanupWrapper cleanup;

        auto thumb = CreateThumbInCanvas();
        auto observations = std::make_shared<DragObservations>();
        auto startedRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, DragStarted);
        auto deltaRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, DragDelta);
        auto completedRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, DragCompleted);
        AttachDragHandlers(thumb, observations, startedRegistration, deltaRegistration, completedRegistration);

        TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
        auto mouseReleaseGuard = ReleaseMouseOnExit(thumb);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MoveMouse(thumb, -20, 30);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(thumb->IsDragging);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(observations->deltaCount, 1);
            VERIFY_ARE_EQUAL(0, observations->completedCount);

            LOG_OUTPUT(L"Disable the Thumb while the mouse button is still pressed.");
            thumb->IsEnabled = false;
        });
        observations->completedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(1, observations->completedCount);
            VERIFY_IS_FALSE(observations->completedCanceled);
            VERIFY_ARE_EQUAL(observations->deltaCount, observations->deltaCountAtCompletion);
            VerifyCompletedChanges(*observations, -20, 30);
        });

        LOG_OUTPUT(L"Releasing the pointer on the disabled Thumb does not complete the drag again.");
        TestServices::InputHelper->MouseButtonUp(thumb, -20, 30, MouseButton::Left);
        mouseReleaseGuard.release();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, observations->startedCount);
            VERIFY_ARE_EQUAL(1, observations->completedCount);
        });
    }

} } } } } } }
