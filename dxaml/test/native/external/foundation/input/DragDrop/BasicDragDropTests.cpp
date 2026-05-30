// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BasicDragDropTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include <fileloader.h>
#include <collection.h>
#include <XamlLogging.h>

using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::ApplicationModel::DataTransfer;
using namespace ::Windows::ApplicationModel::DataTransfer::DragDrop;
using namespace ::Windows::ApplicationModel::DataTransfer::DragDrop::Core;
using namespace ::Windows::Storage;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::Graphics::Imaging;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DragDrop {

        static const wchar_t *ResourceXaml =
L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >\n\
    <DataTemplate x:Key='SimpleTemplate'>\n\
        <Border Height='50' Width='300' Background='Red'>\n\
                            <TextBlock Text='{Binding Text}' Foreground='White' VerticalAlignment='Center'/>\n\
        </Border>\n\
    </DataTemplate>\n\
</ResourceDictionary>";

        // Structure to define a basic test scene.  At some future point we will modify tests to use this common object to define
        // their test scenes rather than all the duplicate code;
        struct DragDropTestScene
        {
        public:
            DragDropTestScene();

            Shapes::Rectangle^ Rectangle;
            Controls::Border^ Border;
        };

        struct DropTargetHelper
        {
        public:
            DropTargetHelper(UIElement^ target);
            ~DropTargetHelper();

            SafeEventRegistrationType(UIElement, DragEnter) DragEnterRegistration  = CreateSafeEventRegistration(UIElement, DragEnter);
            SafeEventRegistrationType(UIElement, DragOver) DragOverRegistration = CreateSafeEventRegistration(UIElement, DragOver);
            SafeEventRegistrationType(UIElement, DragLeave) DragLeaveRegistration = CreateSafeEventRegistration(UIElement, DragLeave);
            SafeEventRegistrationType(UIElement, Drop) DropRegistration = CreateSafeEventRegistration(UIElement, Drop);

            std::shared_ptr<Event> DragEnterEvent = std::make_shared<Event>();
            std::shared_ptr<Event> DragLeaveEvent = std::make_shared<Event>();
            std::shared_ptr<Event> DropEvent = std::make_shared<Event>();

            void Reset();

        private:
            bool m_originalAllowDrop;
            UIElement^ m_target;
        };

        struct DragSourceHelper
        {
        public:
            DragSourceHelper(UIElement^ source, bool manualStart = false);
            ~DragSourceHelper();

            SafeEventRegistrationType(UIElement, PointerPressed) PointerPressedRegistration = CreateSafeEventRegistration(UIElement, PointerPressed);
            SafeEventRegistrationType(UIElement, DragStarting) DragStartingRegistration = CreateSafeEventRegistration(UIElement, DragStarting);
            
            std::shared_ptr<Event> DragStartingEvent = std::make_shared<Event>();

        private:
            bool m_originalCanDrag;
            UIElement^ m_source;

        };

        static const double DefaultSpeed = 0.3;

        ref class TextProperty sealed : public Microsoft::UI::Xaml::Data::ICustomProperty
        {
        public:

            virtual Platform::Object^ GetIndexedValue(Platform::Object^ target, Platform::Object^)
            {
                return GetText(target);
            }
            virtual Platform::Object^ GetValue(Platform::Object^ target)
            {
                return GetText(target);
            }
            virtual void SetIndexedValue(Platform::Object^, Platform::Object^, Platform::Object^)
            {}
            virtual void SetValue(Platform::Object^, Platform::Object^)
            {}
            property bool CanRead
            {
                virtual bool get() { return true; }
            }
            property bool CanWrite
            {
                virtual bool get() { return false; }
            }
            property Platform::String^ Name
            {
                virtual Platform::String^ get() { return ref new Platform::String(L"Text"); }
            }
            property ::Windows::UI::Xaml::Interop::TypeName Type
            {
                virtual ::Windows::UI::Xaml::Interop::TypeName get()
                {
                    return ::Windows::UI::Xaml::Interop::TypeName(Platform::String::typeid);
                }
            }
        private:
            Platform::String^ GetText(Platform::Object^ target);
        };
        ref class Contact sealed : public ::Windows::Foundation::IStringable, public Microsoft::UI::Xaml::Data::ICustomPropertyProvider
        {
        public:
            Contact(unsigned int id) : _id(id)
            {
                wchar_t wsTemp[32];
                StringCchPrintf(wsTemp, ARRAYSIZE(wsTemp), L"Contact #%d", id);
                _text = ref new Platform::String(wsTemp);
            }
            property unsigned int Id
            {
                unsigned int get() { return _id; }
            }
            property Platform::String^ Text
            {
                Platform::String^ get() { return _text; }
            }
            virtual Platform::String^ ToString()
            {
                return _text;
            }
            virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetCustomProperty(Platform::String^ name)
            {
                return ref new TextProperty();

            }
            virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, ::Windows::UI::Xaml::Interop::TypeName type)
            {
                return ref new TextProperty();
            }
            virtual Platform::String^ GetStringRepresentation()
            {
                return _text;
            }
            property ::Windows::UI::Xaml::Interop::TypeName Type
            {
                virtual ::Windows::UI::Xaml::Interop::TypeName get()
                {
                    return ::Windows::UI::Xaml::Interop::TypeName(Contact::typeid);
                }
            }

        private:
            unsigned int _id;
            Platform::String^ _text;
        };

        Platform::String^ TextProperty::GetText(Platform::Object^ target)
        {
            auto contact = dynamic_cast<Contact^>(target);
            return (contact == nullptr) ? nullptr : contact->Text;
        }

        bool BasicDragDropTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool BasicDragDropTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        void RestoreForegroundWindowIfAvailable()
        {
            if (TestServices::Utilities->IsDesktop)
            {
                // Workaround for a known issue
                TestServices::WindowHelper->RestoreForegroundWindow();

            }
        }
        bool BasicDragDropTests::TestCleanup()
        {
            RestoreForegroundWindowIfAvailable();

            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void BasicDragDropTests::DndTestCleanup()
        {
            try
            {
                // Workaround for a known issue
                RestoreForegroundWindowIfAvailable();
            }
            catch (...)
            {
                LOG_OUTPUT(L"Ignoring exception raised in RestoreForegroundWindow");
            }
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        }

        void BasicDragDropTests::PrepareDragTests()
        {
            // During the first drag and drop operation, DataExchangeHost.exe steals focus from the test window.
            // Moreover, on drop, another window might get focus for a brief lapse of time.
            // We suspect this could be a source of instability in dnd tests. Let's run this prep test in its own
            // window. The rest of the suite will run in a different window that won't be subject to the weird
            // behavior described above. This should be removed a week or two from now because it slows down the
            // test suite a little bit.
            CanDefaultMouseDrag();
        }

        //------------------------------------------------------------------------
        // Test case: Drag a Rectangle in a Canvas with the left mouse button.
        //------------------------------------------------------------------------
        void BasicDragDropTests::CanDragUsingCoreDragOperation()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderReleaseEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectPointerPressedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerPressed);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            auto borderPointerReleasedRegistration = CreateSafeEventRegistration(Border, PointerReleased);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectPointerPressedRegistration.Attach(rect, ref new PointerEventHandler([](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerPressed event - Start Drag");

                    CoreDragOperation^ drag = ref new CoreDragOperation();
                    drag->SetPointerId(args->Pointer->PointerId);
                    drag->Data->SetText(ref new Platform::String(L"This is a test"));
                    drag->Data->RequestedOperation = DataPackageOperation::Copy | DataPackageOperation::Move | DataPackageOperation::Link;
                    
                    {
                        void* result = nullptr;

                        const auto hmod = ::GetModuleHandleA("microsoft.inputstatemanager.dll");
                        if (hmod == nullptr)
                        {
                            return;
                        }

                        using LiftedDragStartAsync = HRESULT(*)(_In_ CoreDragOperation^, //IUnknown*,
                            _In_opt_ IInspectable*,
                            _In_ Point,
                            _Outptr_ void**);
                        const auto StartAsyncDrag = reinterpret_cast<LiftedDragStartAsync>(::GetProcAddress(hmod, "LiftedDragStartAsync"));
                        if (StartAsyncDrag)
                        {
                            StartAsyncDrag(drag, nullptr, {}, &result);
                        }
                    }
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount, borderDropEvent](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                    borderDropEvent->Set();
                }));

                borderPointerReleasedRegistration.Attach(border, ref new PointerEventHandler([borderReleaseEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Released event");

                    borderReleaseEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            // Let's force the input helper to initialize before we can do the real test
            TestServices::InputHelper->PressHoldAndPanFromCenter(border, 10, 10, DefaultSpeed, 100);
            if (!borderReleaseEvent->WaitForNoThrow(std::chrono::milliseconds(1000)))
            {
                LOG_WARNING(L"Input not ready, test will most likely fail...");
            }

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            borderDropEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
            });
        }

        void BasicDragDropTests::CanDragUsingStartDragAsync()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectPointerPressedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerPressed);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectPointerPressedRegistration.Attach(rect, ref new PointerEventHandler([](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerPressed event - Start Drag");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);

                    UIElement^ rect = dynamic_cast<UIElement^>(sender);

                    concurrency::create_task(rect->StartDragAsync(args->GetCurrentPoint(rect))).then(
                        [&](DataPackageOperation dragResult)
                    {
                        LOG_OUTPUT(L"StartDragAsync Completed");
                        VERIFY_ARE_EQUAL(DataPackageOperation::Copy, dragResult);
                    });
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    args->Data->SetText(ref new Platform::String(L"This is a test"));
                    args->Data->RequestedOperation = DataPackageOperation::Copy | DataPackageOperation::Move | DataPackageOperation::Link;
                    rectDragStartingCount++;

                    args->Cancel = false;
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);
            });
        }

        void BasicDragDropTests::ValidateDragOperationDeferral()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            EnsureDataExchangeHostStarted();

            auto rectLoadedEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;
            Border^ parentBorder = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            auto parentBorderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto parentBorderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;

            UINT parentBorderDragEnterCount = 0;
            UINT parentBorderDragOverCount = 0;

            bool asyncCompletedDragEnter = false;
            bool asyncCompletedDragOver = false;

            DragOperationDeferral^ dragDeferral = nullptr;
            DragOperationDeferral^ dropDeferral = nullptr;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                canvas->AllowDrop = true;
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                parentBorder = ref new Border();
                parentBorder->Width = 200;
                parentBorder->Height = 60;
                parentBorder->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                parentBorder->AllowDrop = true;
                parentBorder->SetValue(Canvas::LeftProperty, 100.0f);
                parentBorder->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(parentBorder);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                parentBorder->Child = border;

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");

                    args->Data->SetText(L"One Two Three");
                    args->Data->RequestedOperation = DataPackageOperation::Copy | DataPackageOperation::Move | DataPackageOperation::Link;
                    args->Cancel = false;
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");

                    VERIFY_ARE_EQUAL(DataPackageOperation::Move, args->DropResult);
                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount, &asyncCompletedDragEnter](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;

                    // Do not use the deferral. asyncCompleted will be set after the parent is called.
                    concurrency::create_task(args->DataView->GetTextAsync()).then(
                        [&](concurrency::task<Platform::String^> textTask)
                    {
                        asyncCompletedDragEnter = true;
                    });
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount, &asyncCompletedDragOver, &dragDeferral](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);


                    // This deferral does not really validate AcceptedOperation (it will be done on Drop)
                    // but validates the effect on deferrals on event sequencing
                    // Use the deferral. asyncCompleted will be set before the parent is called.
                    dragDeferral = args->GetDeferral();
                    LOG_OUTPUT(L"Got the deferral");

                    args->AcceptedOperation = DataPackageOperation::Copy;

                    concurrency::create_task(args->DataView->GetTextAsync()).then(
                        [&asyncCompletedDragOver,&dragDeferral](Platform::String^ text)
                    {
                        RunOnUIThread([&]()
                        {
                            asyncCompletedDragOver = true;
                            LOG_OUTPUT(L"Completing deferral");
                            dragDeferral->Complete();
                            dragDeferral = nullptr;
                            LOG_OUTPUT(L"Completed deferral");
                        });
                    });
                }));

                parentBorderDragEnterRegistration.Attach(parentBorder, ref new DragEventHandler([&parentBorderDragEnterCount, &borderDragEnterCount, &asyncCompletedDragEnter](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"parentBorder DragEnter event");
                    parentBorderDragEnterCount++;

                    VERIFY_ARE_EQUAL(parentBorderDragEnterCount, parentBorderDragEnterCount);

                }));

                parentBorderDragOverRegistration.Attach(parentBorder, ref new DragEventHandler([&parentBorderDragOverCount, &borderDragOverCount, &asyncCompletedDragOver](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"parentBorder DragOver event");
                    parentBorderDragOverCount++;
                    VERIFY_ARE_EQUAL(parentBorderDragOverCount, borderDragOverCount);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                    VERIFY_IS_TRUE(asyncCompletedDragOver);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&dropDeferral](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");

                    // DragOver returned Copy, Drop returns Move asynchronously
                    // And this is what the source should get
                    dropDeferral = args->GetDeferral();
                    LOG_OUTPUT(L"Got the deferral");
                    args->AcceptedOperation = DataPackageOperation::Move;

                    concurrency::create_task(args->DataView->GetTextAsync()).then(
                        [&dropDeferral](Platform::String^ text)
                    {
                        RunOnUIThread([&]()
                        {
                            // To validate deferral
                            LOG_OUTPUT(L"Completing deferral");
                            dropDeferral->Complete();
                            dropDeferral = nullptr;
                            LOG_OUTPUT(L"Completed deferral");
                        });
                    });
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150/*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();
        }

        void BasicDragDropTests::CanDefaultMouseDrag()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            EnsureDataExchangeHostStarted();

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            auto resetCountFunc = [&]()
            {
                borderDragEnterCount = 0;
                borderDragOverCount = 0;
                borderDragLeaveCount = 0;
                borderDropCount = 0;
                rectDragStartingCount = 0;
                rectDropCompletedCount = 0;
            };

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount, rect, border](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    // As we dragged from the center, we can check GetPosition
                    // Note that we have to account for rounding errors, as
                    // well as the D&D trigger (especially horizontally)
                    // position is in physical pixels...
                    auto position = args->GetPosition(rect);
                    const float scaleFactor = TestServices::WindowHelper->GetCurrentWindowScale();
                    const double scaledX = position.X / scaleFactor;
                    const double scaledY = position.Y / scaleFactor;
                    double deltaX = scaledX - rect->ActualWidth / 2.0;
                    double deltaY = scaledY - rect->ActualHeight / 2.0;
                    LOG_OUTPUT(L"DragStarting, position=%.2f, %.2f, scaled=%.2f, %.2f, middle = %.2f,%.2f",
                        position.X, position.Y,
                        scaledX, scaledY,
                        rect->ActualWidth / 2.0, rect->ActualHeight / 2.0);

                    // We accept deltaX >> 0 as it will depend on the load of the test machine
                    // We accept also a slight variation on Y to take into account rounding errors
                    VERIFY_IS_TRUE((deltaX > -2.0) && (deltaX < 100.0));
                    VERIFY_IS_TRUE((deltaY > -2.0) && (deltaY < 2.0));

                    // Finally, let's try with another UIElement
                    auto borderPosition = args->GetPosition(border);
                    const double scaledborderPositionX = borderPosition.X / scaleFactor;
                    const double scaledborderPositionY = borderPosition.Y / scaleFactor;
                    LOG_OUTPUT(L"DragStarting, position vs border=%.2f, %.2f, scaled=%.2f, %.2f",
                        borderPosition.X, borderPosition.Y,
                        scaledborderPositionX, scaledborderPositionY);
                    deltaX = scaledX - (scaledborderPositionX + (double)border->GetValue(Canvas::LeftProperty));
                    deltaY = scaledY - (scaledborderPositionY + (double)border->GetValue(Canvas::TopProperty));
                    VERIFY_IS_TRUE((deltaX > -2.0) && (deltaX < 2.0));
                    VERIFY_IS_TRUE((deltaY > -2.0) && (deltaY < 2.0));
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                rectPointerReleasedRegistration.Attach(rect, ref new PointerEventHandler([rectPointerReleasedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerReleased event - Mouse lifted");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    rectPointerReleasedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

                resetCountFunc();
                rectDropCompletedEvent->Reset();
            });

            LOG_OUTPUT(L"Dragging border only by 2 pixels.");
            TestServices::InputHelper->DragFromCenter(border, 2 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectPointerReleasedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 0u);
                VERIFY_ARE_EQUAL(borderDragOverCount, 0u);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 0u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 0u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 0u);

                resetCountFunc();
                rectDropCompletedEvent->Reset();
            });

            LOG_OUTPUT(L"Dragging Rectangle again.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

                resetCountFunc();
                rectDropCompletedEvent->Reset();
            });

            LOG_OUTPUT(L"Dragging Rectangle again.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);
            });
        }

        void BasicDragDropTests::CanDefaultTouchDrag()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();
            auto canvasDropCompletedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            auto canvasDragStartingRegistration = CreateSafeEventRegistration(Canvas, DragStarting);
            auto canvasDropCompletedRegistration = CreateSafeEventRegistration(Canvas, DropCompleted);

            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;

            UINT canvasDragStartingCount = 0;
            UINT canvasDropCompletedCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));

                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;
                }));

                canvasDragStartingRegistration.Attach(canvas, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&canvasDragStartingCount](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Canvas DragStarting event");
                    canvasDragStartingCount++;

                }));


                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);

                    rectDropCompletedEvent->Set();
                }));

                canvasDropCompletedRegistration.Attach(canvas, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&canvasDropCompletedCount, canvasDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Canvas DropCompleted event");
                    canvasDropCompletedCount++;
                    canvasDropCompletedEvent->Set();
                }));


                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    Border^ senderAsBorder = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(senderAsBorder);
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    Border^ senderAsBorder = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(senderAsBorder);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    Border^ senderAsBorder = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(senderAsBorder);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                rectPointerReleasedRegistration.Attach(rect, ref new PointerEventHandler([rectPointerReleasedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerReleased event - Mouse lifted");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Touch);
                    rectPointerReleasedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"\nDragging Rectangle by panning without holding first.");
            TestServices::InputHelper->PanFromCenter(rect, 120 /*relX*/, 10 /*relY*/, 0.2 /*velocityFactor*/);
            rectPointerReleasedEvent->WaitForDefault();
            rectPointerReleasedEvent->Reset();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 0u);
                VERIFY_ARE_EQUAL(borderDragOverCount, 0u);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 0u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 0u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 0u);

                borderDragEnterCount = 0;
                borderDragOverCount = 0;
                borderDragLeaveCount = 0;
                borderDropCount = 0;
                rectDragStartingCount = 0;
                rectDropCompletedCount = 0;
            });

            LOG_OUTPUT(L"\nDragging Rectangle.");
            TestServices::InputHelper->PressHoldAndPanFromCenter(rect, 120 /*relX*/, 10 /*relY*/, 0.5 /*velocityFactor*/, 1000 /*holdTime*/);
            rectDropCompletedEvent->WaitForDefault();
            rectDropCompletedEvent->Reset();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

                borderDragEnterCount = 0;
                borderDragOverCount = 0;
                borderDragLeaveCount = 0;
                borderDropCount = 0;
                rectDragStartingCount = 0;
                rectDropCompletedCount = 0;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\nDragging Rectangle by pressing and holding then only panning by 2 pixels.");
            TestServices::InputHelper->PressHoldAndPanFromCenter(rect, 2 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/, 500 /*holdTime*/);
            rectPointerReleasedEvent->WaitForDefault();
            rectPointerReleasedEvent->Reset();

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 0u);
                VERIFY_ARE_EQUAL(borderDragOverCount, 0u);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 0u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 0u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 0u);

                borderDragEnterCount = 0;
                borderDragOverCount = 0;
                borderDragLeaveCount = 0;
                borderDropCount = 0;
                rectDragStartingCount = 0;
                rectDropCompletedCount = 0;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\nRepeat Dragging Rectangle.");
            TestServices::InputHelper->PressHoldAndPanFromCenter(rect, 120 /*relX*/, 10 /*relY*/, 0.4 /*velocityFactor*/, 500 /*holdTime*/);
            rectDropCompletedEvent->WaitForDefault();
            rectDropCompletedEvent->Reset();

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

                borderDragEnterCount = 0;
                borderDragOverCount = 0;
                borderDragLeaveCount = 0;
                borderDropCount = 0;
                rectDragStartingCount = 0;
                rectDropCompletedCount = 0;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rect->CanDrag = false;
            });

            LOG_OUTPUT(L"Dragging Rectangle by pressing, holding and panning again when Rectangle.CanDrag=False shouldn't drag.");
            TestServices::InputHelper->PressHoldAndPanFromCenter(rect, 50 /*relX*/, 0 /*relY*/, 0.4 /*velocityFactor*/, 500 /*holdTime*/);
            LOG_OUTPUT(L"done.");

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 0u);
                VERIFY_ARE_EQUAL(borderDragOverCount, 0u);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 0u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 0u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 0u);

                borderDragEnterCount = 0;
                borderDragOverCount = 0;
                borderDragLeaveCount = 0;
                borderDropCount = 0;
                rectDragStartingCount = 0;
                rectDropCompletedCount = 0;

                canvas->CanDrag = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Dragging Rectangle by pressing, holding and panning again when Rectangle.CanDrag=False and Canvas.CanDrag=True should drag canvas.");
            TestServices::InputHelper->PressHoldAndPanFromCenter(rect, 120 /*relX*/, 10 /*relY*/, 0.4 /*velocityFactor*/, 500 /*holdTime*/);
            canvasDropCompletedEvent->WaitForDefault();
            canvasDropCompletedEvent->Reset();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(canvasDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(canvasDropCompletedCount, 1u);
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 0u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 0u);
            });

        }

        void BasicDragDropTests::CanCancelDrag()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto operationCompletedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectPointerPressedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerPressed);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);

            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                canvas->Children->Append(rect);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));

                rectPointerPressedRegistration.Attach(rect, ref new PointerEventHandler([operationCompletedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerPressed event - Start Drag");

                    auto rect = dynamic_cast<UIElement^>(sender);
                    auto operation = rect->StartDragAsync(args->GetCurrentPoint(nullptr));
                    operation->Completed = ref new AsyncOperationCompletedHandler<DataPackageOperation>([operationCompletedEvent](IAsyncOperation<DataPackageOperation>^ pOperation, AsyncStatus status)
                    {
                        // Check that the status is canceled
                        VERIFY_ARE_EQUAL(AsyncStatus::Canceled, status);

                        operationCompletedEvent->Set();
                    });

                }));


                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    args->Cancel = true;
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;
                }));



                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            operationCompletedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 0u);
            });
        }

        void BasicDragDropTests::CanSetCustomDragVisualWithBitmapUriSource()
        {
            SetCustomDragVisualWithBitmapUriSourceHelper();
        }

        void BasicDragDropTests::CanSetCustomDragVisualWithBitmapUriSourceSmallerSize()
        {
            SetCustomDragVisualWithBitmapUriSourceHelper(true /*smaller decoding size*/);
        }

        void BasicDragDropTests::CanSetCustomDragVisualWithBitmapSetSource()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            EnsureDataExchangeHostStarted();

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;
            BitmapImage^ bitmapImage = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount, rect, border, &bitmapImage](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    args->Data->SetText(L"Hello world");
                    args->Data->RequestedOperation = DataPackageOperation::Copy;
                    auto def = args->GetDeferral();

                    args->DragUI->SetContentFromBitmapImage(bitmapImage); // Note that bitmapImage will be pre-initialized before this handler is called.
                    LOG_OUTPUT(L"BitmapImage SetSourceAsyncCompleted");
                    def->Complete();
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount, &bitmapImage](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                    Point anchor{ -100.0, 150.0f };
                    args->DragUIOverride->SetContentFromBitmapImage(bitmapImage, anchor);
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                rectPointerReleasedRegistration.Attach(rect, ref new PointerEventHandler([rectPointerReleasedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerReleased event - Mouse lifted");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    rectPointerReleasedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            // Initialize the Bitmap Image.  Because this test uses the imagein the DragStarting handler, it is possible that we
            // would get there before we have completed initializing if we attempted to initialize it on drag start.
            // For this reason, we initialize it up front.  Note: There are separate tests for deferrals where situations where
            // an app would be unable to do this ahead of time.
            auto bitmapImageEvent = std::make_shared<Event>();
            RunOnUIThread([&]()
            {
                auto path = GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dragdrop\\pussinboots.jpg";

                concurrency::create_task(::Windows::Storage::StorageFile::GetFileFromPathAsync(path)).then([&](::Windows::Storage::StorageFile^ p)
                {
                    return p->OpenAsync(::Windows::Storage::FileAccessMode::Read);
                }).then([&](IRandomAccessStream^ stream)
                {
                    RunOnUIThread([&]()
                    {
                        bitmapImage = ref new BitmapImage();
                        bitmapImage->SetSource(stream);
                        bitmapImageEvent->Set();
                    });
                });
            });
            bitmapImageEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

            });
        }

        void BasicDragDropTests::CanSetPreparedCustomDragVisualWithBitmapUriSource()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            EnsureDataExchangeHostStarted();

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                BitmapImage^ dragVisual = ref new BitmapImage();
                auto testUri = ref new Uri(GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dragdrop\\pussinboots.jpg");
                dragVisual->UriSource = testUri;

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount, rect, border, dragVisual](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    args->Data->SetText(L"Hello world");
                    args->Data->RequestedOperation = DataPackageOperation::Copy;
                    args->DragUI->SetContentFromBitmapImage(dragVisual);
                    args->Cancel = false;
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                rectPointerReleasedRegistration.Attach(rect, ref new PointerEventHandler([rectPointerReleasedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerReleased event - Mouse lifted");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    rectPointerReleasedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

            });
        }

        // The idea of the test is that the DragVisual is created With SetSourceAsync
        // BEFORE the DragVisual is really used in Drag and Drop
        void BasicDragDropTests::CanSetPreparedCustomDragVisualWithBitmapSetSource()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            EnsureDataExchangeHostStarted();

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();
            auto visualReadyEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;
            BitmapImage^ dragVisual = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount, &dragVisual](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    VERIFY_IS_NOT_NULL(dragVisual);

                    args->Data->SetText(L"Hello world");
                    args->Data->RequestedOperation = DataPackageOperation::Copy;
                    args->DragUI->SetContentFromBitmapImage(dragVisual);
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount, &dragVisual](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    VERIFY_IS_NOT_NULL(dragVisual);
                    args->DragUIOverride->SetContentFromBitmapImage(dragVisual);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                rectPointerReleasedRegistration.Attach(rect, ref new PointerEventHandler([rectPointerReleasedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerReleased event - Mouse lifted");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    rectPointerReleasedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            // Initialize the Bitmap Image.  Because this test uses the imagein the DragStarting handler, it is possible that we
            // would get there before we have completed initializing if we attempted to initialize it on drag start.
            // For this reason, we initialize it up front.  Note: There are separate tests for deferrals where situations where
            // an app would be unable to do this ahead of time.
            RunOnUIThread([&]()
            {
                auto path = GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dragdrop\\pussinboots.jpg";

                concurrency::create_task(::Windows::Storage::StorageFile::GetFileFromPathAsync(path)).then([&](::Windows::Storage::StorageFile^ picture)
                {
                    return picture->OpenAsync(::Windows::Storage::FileAccessMode::Read);
                }).then([&](IRandomAccessStream^ stream)
                {
                    RunOnUIThread([&]()
                    {
                        dragVisual = ref new BitmapImage();
                        dragVisual->SetSource(stream);
                        LOG_OUTPUT(L"DragVisual ready");
                        visualReadyEvent->Set();
                    });
                });
            });
            visualReadyEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

            });
        }

        void BasicDragDropTests::CanSetAllDragVisualSettings()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    args->Data->SetText(L"Hello world");
                    args->Data->RequestedOperation = DataPackageOperation::Copy;
                    auto bitmapImage = ref new BitmapImage();
                    VERIFY_IS_NOT_NULL(bitmapImage);

                    auto testUri = ref new Uri(GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dragdrop\\pussinboots.jpg");
                    VERIFY_IS_NOT_NULL(testUri);

                    bitmapImage->UriSource = testUri;

                    args->DragUI->SetContentFromBitmapImage(bitmapImage);


                    args->Cancel = false;
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    args->DragUIOverride->Caption = ref new Platform::String(L"O Captain! My Captain!");
                    args->DragUIOverride->IsCaptionVisible = true;
                    args->DragUIOverride->IsGlyphVisible = true;
                    args->DragUIOverride->IsContentVisible = false;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                rectPointerReleasedRegistration.Attach(rect, ref new PointerEventHandler([rectPointerReleasedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerReleased event - Mouse lifted");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    rectPointerReleasedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

            });
        }

        void BasicDragDropTests::CanUseSoftwareBitmapAndDeferral()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();
            auto imageDecodedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            SoftwareBitmap^ softwareBitmap = nullptr;
            // Create our software bitmap.  Because this test uses the software bitmap in the
            // DrageEnter handler, it is possible that we would get there before we have completed
            // the decoding of the bitmap if we started the decoding on drag start.
            // For this reason, we decode it up front.
            RunOnUIThread([&]()
            {
                auto path = GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dragdrop\\rubik_160.png";
                concurrency::create_task(::Windows::Storage::StorageFile::GetFileFromPathAsync(path))
                    .then([](::Windows::Storage::StorageFile^ picture)
                {
                    LOG_OUTPUT(L"Opening file");
                    return picture->OpenAsync(::Windows::Storage::FileAccessMode::Read);
                }).then([](IRandomAccessStream^ stream)
                {
                    LOG_OUTPUT(L"Creating decoder");
                    return BitmapDecoder::CreateAsync(stream);
                }).then([](BitmapDecoder^ decoder)
                {
                    LOG_OUTPUT(L"Decoding");
                    return decoder->GetSoftwareBitmapAsync();
                }).then([&softwareBitmap, imageDecodedEvent](SoftwareBitmap^ softbitmap)
                {
                    LOG_OUTPUT(L"Setting software bitmap");
                    softwareBitmap = softbitmap;
                    imageDecodedEvent->Set();
                });
            });
            imageDecodedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount, &softwareBitmap](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    args->Data->SetText(L"Hello world");
                    args->Data->RequestedOperation = DataPackageOperation::Copy;
                    args->DragUI->SetContentFromDataPackage();

                    auto def = args->GetDeferral();
                    LOG_OUTPUT(L"Got Deferral");
                    // Simulate asynchronous operation to get the bitmap image.  We have already
                    // pre-decode the image to ensure that this is done prior to the drag enter
                    // event being trigger, but we still want to process it as deferral.
                    Concurrency::create_task([&softwareBitmap, args, def]() {
                        RunOnUIThread([&softwareBitmap, args, def]()
                        {
                            LOG_OUTPUT(L"Setting Defered Content");
                            args->DragUI->SetContentFromSoftwareBitmap(softwareBitmap);
                            def->Complete();
                        });
                    });
                    args->Cancel = false;
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount, &softwareBitmap](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                    LOG_OUTPUT(L"Setting Content With Anchor");
                    Point anchor = { 10.0f, 25.0f };
                    args->DragUIOverride->SetContentFromSoftwareBitmap(softwareBitmap, anchor);
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                rectPointerReleasedRegistration.Attach(rect, ref new PointerEventHandler([rectPointerReleasedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerReleased event - Mouse lifted");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    rectPointerReleasedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

            });
        }

        void BasicDragDropTests::CanTakeDeferralOnDragStarting()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            bool asyncCallbackCalled = false;
            bool dragOverCalled = false;
            bool dragEnterCalled = false;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 0.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>
                    ([&rectDragStartingCount, &asyncCallbackCalled, &dragOverCalled, &dragEnterCalled](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    args->Data->SetText(L"Hello world");
                    args->Data->RequestedOperation = DataPackageOperation::Copy;

                    auto dragDeferral = args->GetDeferral();
                    LOG_OUTPUT(L"Got Deferral");

                    test_infra::TestServices::WindowHelper->CurrentDispatcher->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
                        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([dragDeferral, args, &asyncCallbackCalled, &dragOverCalled, &dragEnterCalled]() {
                            LOG_OUTPUT(L"Inside Dispatcher Callback\n");

                            // Even though the rectangle was already over the border
                            // These event should not be called before
                            VERIFY_IS_FALSE(dragOverCalled);
                            VERIFY_IS_FALSE(dragEnterCalled);
                            asyncCallbackCalled = true;

                            dragDeferral->Complete();
                            args->Cancel = false;
                            LOG_OUTPUT(L"Deferral completed\n");

                    }));

                    LOG_OUTPUT(L"Exiting Rectangle DragStarting event");
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, &dragEnterCalled, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");

                    rectDropCompletedCount++;

                    // If dragEnter hasn't been called, then we won't have a Data Package Operation.  We will handle the
                    // recovery or erroring out because of the non-call in the mainline of the test.
                    if (dragEnterCalled)
                    {
                        VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);
                    }
                    else
                    {
                        VERIFY_ARE_EQUAL(DataPackageOperation::None, args->DropResult);
                    }

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount, &dragEnterCalled](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;
                    dragEnterCalled = true;
                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount, &dragOverCalled](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;
                    dragOverCalled = true;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                rectPointerReleasedRegistration.Attach(rect, ref new PointerEventHandler([rectPointerReleasedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerReleased event - Mouse lifted");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    rectPointerReleasedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 50 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            // Because there is a delay when launching the DataTransferHost, it is possible that due to the quickness
            // and conciseness (no wavering of pointer input) that occurs with our input simulation, the DataTransferHost
            // may not have been loaded fast enough to for us to get the drag over events before the drop was completed.
            // If this is true, we will want to retry the drag.
            if (!dragEnterCalled)
            {
                LOG_OUTPUT(L"Drag Enter not called.  Try again.");
                rectDragStartingCount = 0;
                rectDropCompletedCount = 0;
                rectDropCompletedEvent->Reset();

                LOG_OUTPUT(L"Redragging Rectangle.");
                TestServices::InputHelper->DragFromCenter(rect, 50 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
                rectDropCompletedEvent->WaitForDefault();

                RestoreForegroundWindowIfAvailable();
            }

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

            });
        }

        void BasicDragDropTests::ProvidesDragUIOverrideOnLeave()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rootLeftEvent = std::make_shared<Event>();

            Grid^ mainGrid = nullptr;
            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rootDragLeaveRegistration = CreateSafeEventRegistration(UIElement, DragLeave);

            RunOnUIThread([&]()
            {
                mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 200;
                rect->Height = 10;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                // Let's find the root of XAML in order for DragLeave event to be raised by Core's LeaveAsync
                UIElement^ root = mainGrid;
                DependencyObject^ parent = VisualTreeHelper::GetParent(mainGrid);
                while (parent != nullptr)
                {
                    LOG_OUTPUT(L"%s", parent->GetType()->ToString()->Data());
                    root = dynamic_cast<UIElement^>(parent);
                    parent = VisualTreeHelper::GetParent(parent);
                }

                VERIFY_IS_NOT_NULL(root);
                root->AllowDrop = true;
                rootDragLeaveRegistration.Attach(root, ref new DragEventHandler([rootLeftEvent](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"DragLeave event on top element");
                    args->DragUIOverride->Clear();
                    args->DragUIOverride->IsCaptionVisible = false;
                    args->DragUIOverride->IsGlyphVisible = false;
                    args->DragUIOverride->Caption = ref new Platform::String(L"Leaving");

                    rootLeftEvent->Set();

                }));

            });

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 0 /*relX*/, 1500 /*relY*/, 1 /*velocityFactor*/);
            rootLeftEvent->WaitForDefault();
        }

        void BasicDragDropTests::DoNotRaiseMultipleDragEnterOnTreeChange()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Grid^ targetGrid = nullptr;
            Border^ childBorder = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto gridDragEnterRegistration = CreateSafeEventRegistration(Grid, DragEnter);
            auto gridDragLeaveRegistration = CreateSafeEventRegistration(Grid, DragLeave);
            auto gridDropRegistration = CreateSafeEventRegistration(Grid, Drop);

            UINT gridDragEnterCount = 0;
            UINT gridDragLeaveCount = 0;
            UINT gridDropCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                targetGrid = ref new Grid();
                targetGrid->Width = 300;
                targetGrid->Height = 60;
                targetGrid->SetValue(Canvas::LeftProperty, 0.0);
                targetGrid->SetValue(Canvas::TopProperty, 20.0);
                targetGrid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                targetGrid->AllowDrop = true;

                childBorder = ref new Border();
                childBorder->HorizontalAlignment = HorizontalAlignment::Stretch;
                childBorder->VerticalAlignment = VerticalAlignment::Stretch;
                childBorder->Background = ref new SolidColorBrush(Microsoft::UI::Colors::LightBlue);
                targetGrid->Children->Append(childBorder);

                canvas->Children->Append(targetGrid);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->SetValue(Canvas::LeftProperty, 120);
                rect->SetValue(Canvas::TopProperty, 100.0);
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedEvent->Set();
                }));

                gridDragEnterRegistration.Attach(targetGrid, ref new DragEventHandler([targetGrid, &gridDragEnterCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    gridDragEnterCount++;

                    targetGrid->Children->Clear();

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));
                gridDragLeaveRegistration.Attach(targetGrid, ref new DragEventHandler([&gridDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    gridDragLeaveCount++;
                }));
                gridDropRegistration.Attach(targetGrid, ref new DragEventHandler([&gridDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    gridDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 0 /*relX*/, -120 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(gridDragEnterCount, 1u);
                VERIFY_ARE_EQUAL(gridDragLeaveCount, 1u);
                VERIFY_ARE_EQUAL(gridDropCount, 0u);

                rectDropCompletedEvent->Reset();
            });


            RestoreForegroundWindowIfAvailable();
        }

        void BasicDragDropTests::CanDoTouchListReordering()
        {
            PerformTouchListReordering(false /* useLegacyPanel */);
        }

        void BasicDragDropTests::CanCancelDragProgrammatically()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectPointerPressedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerPressed);

            bool completed = false;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 200;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->AllowDrop = true;
                canvas->Children->Append(rect);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));

                rectPointerPressedRegistration.Attach(rect, ref new PointerEventHandler([&completed, rectDropCompletedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerPressed event - Start Drag");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);

                    UIElement^ rect = dynamic_cast<UIElement^>(sender);

                    auto operation = rect->StartDragAsync(args->GetCurrentPoint(rect));
                    LOG_OUTPUT(L"Rectangle PointerPressed event - Operation started");

                    operation->Completed = ref new AsyncOperationCompletedHandler<DataPackageOperation>([&completed, rectDropCompletedEvent](IAsyncOperation<DataPackageOperation>^ pOperation, AsyncStatus status)
                    {
                        completed = true;
                        VERIFY_ARE_EQUAL(AsyncStatus::Canceled, status);
                        rectDropCompletedEvent->Set();
                    });

                    operation->Cancel();
                    LOG_OUTPUT(L"Rectangle PointerPressed event - Operation canceled");

                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 50 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(completed);
            });
        }

        void BasicDragDropTests::ClearUIOverridesWhenSwitchingTarget()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();
            auto border2LoadedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;
            Border^ border2 = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto border2LoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto border2DragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);

            UINT borderDragEnterCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT border2DragEnterCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            bool overrideVisualOnDragEnter = true;

            bool defaultGlyphVisible = false;
            bool defaultCaptionVisible = false;
            bool defaultContentVisible = false;
            Platform::String ^defaultCaption = nullptr;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 100;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                border2 = ref new Border();
                border2->Width = 100;
                border2->Height = 60;
                border2->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Pink);
                border2->AllowDrop = true;
                border2->SetValue(Canvas::LeftProperty, 200.0f);
                border2->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border2);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));
                border2LoadedRegistration.Attach(border, ref new RoutedEventHandler([border2LoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border2 Loaded event");
                    border2LoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount, rect, border](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    args->Cancel = false;
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler(
                    [&overrideVisualOnDragEnter, &borderDragEnterCount, &defaultGlyphVisible, &defaultCaptionVisible, &defaultContentVisible, &defaultCaption]
                    (Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;

                    defaultGlyphVisible = args->DragUIOverride->IsGlyphVisible;
                    defaultContentVisible = args->DragUIOverride->IsContentVisible;
                    defaultCaptionVisible = args->DragUIOverride->IsCaptionVisible;
                    defaultCaption = args->DragUIOverride->Caption;

                    if (overrideVisualOnDragEnter)
                    {
                        args->DragUIOverride->Caption = ref new Platform::String(L"Already pinned");
                        args->DragUIOverride->IsCaptionVisible = true; // will allow visual verification during test
                        args->DragUIOverride->IsGlyphVisible = !defaultGlyphVisible;
                        args->DragUIOverride->IsContentVisible = !defaultContentVisible;
                    }
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler(
                    [&overrideVisualOnDragEnter, &borderDragLeaveCount, &defaultGlyphVisible, &defaultCaptionVisible, &defaultContentVisible, &defaultCaption]
                    (Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    // Override the visuals if not done on DragEnter
                    if (!overrideVisualOnDragEnter)
                    {
                        args->DragUIOverride->Caption = ref new Platform::String(L"Already pinned");
                        args->DragUIOverride->IsCaptionVisible = true; // will allow visual verification during test
                        args->DragUIOverride->IsGlyphVisible = !defaultGlyphVisible;
                        args->DragUIOverride->IsContentVisible = !defaultContentVisible;
                    }
                }));

                border2DragEnterRegistration.Attach(border2, ref new DragEventHandler(
                    [&border2DragEnterCount, &defaultGlyphVisible, &defaultCaptionVisible, &defaultContentVisible, &defaultCaption]
                    (Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border2 DragEnter event");
                    border2DragEnterCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;

                    VERIFY_ARE_EQUAL(defaultGlyphVisible, args->DragUIOverride->IsGlyphVisible);
                    VERIFY_ARE_EQUAL(defaultCaptionVisible, args->DragUIOverride->IsCaptionVisible);
                    VERIFY_ARE_EQUAL(defaultContentVisible, args->DragUIOverride->IsContentVisible);
                    if (defaultCaption == nullptr)
                    {
                        VERIFY_IS_NULL(args->DragUIOverride->Caption);
                    }
                    else
                    {
                        VERIFY_ARE_EQUAL(0, wcscmp(defaultCaption->Data(), args->DragUIOverride->Caption->Data()));
                    }
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();
            border2LoadedEvent->WaitForDefault();

            overrideVisualOnDragEnter = true;
            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 250 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();
            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 1u);
                VERIFY_ARE_EQUAL(border2DragEnterCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

                borderDragEnterCount = 0;
                borderDragLeaveCount = 0;
                border2DragEnterCount = 0;
                rectDragStartingCount = 0;
                rectDropCompletedCount = 0;
            });
            TestServices::WindowHelper->WaitForIdle();

            overrideVisualOnDragEnter = false;
            LOG_OUTPUT(L"Dragging Rectangle again, trying to customize visual on DragLeave.");
            TestServices::InputHelper->DragFromCenter(rect, 250 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RestoreForegroundWindowIfAvailable();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 1u);
                VERIFY_ARE_EQUAL(border2DragEnterCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void BasicDragDropTests::DeferralOnEnterShouldNotBreakLeave()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            // Leak: BasicDragDropTests::DeferralOnEnterShouldNotBreakLeave leaks Border (440 bytes)
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);

            UINT borderDragEnterCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            bool completeSynchronously = true;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 100;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount, rect, border](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    args->Cancel = false;
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount, &completeSynchronously](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto deferral = args->GetDeferral();
                    if (completeSynchronously)
                    {
                        args->DragUIOverride->Caption = ref new Platform::String(L"Already Pinned");
                        deferral->Complete();
                    }
                    else
                    {
                        test_infra::TestServices::WindowHelper->CurrentDispatcher->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
                            ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([deferral, args]() {
                                LOG_OUTPUT(L"Inside Dispatcher Callback\n");
                                args->DragUIOverride->Caption = ref new Platform::String(L"Already Pinned");
                                deferral->Complete();
                                LOG_OUTPUT(L"Deferral completed\n");
                        }));
                    }
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            completeSynchronously = true;
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();
            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

                borderDragEnterCount = 0;
                borderDragLeaveCount = 0;
                rectDragStartingCount = 0;
                rectDropCompletedCount = 0;
                rectDropCompletedEvent->Reset();
            });

            LOG_OUTPUT(L"Dragging Rectangle again.");
            completeSynchronously = false;
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();
            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

                borderDragEnterCount = 0;
                borderDragLeaveCount = 0;
                rectDragStartingCount = 0;
                rectDropCompletedCount = 0;
                rectDropCompletedEvent->Reset();
            });


        }

        void BasicDragDropTests::ValidateThatLightDismissPopupDoesNotDismissWhenStartingDragDrop()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            ListView^ listView;
            xaml_primitives::Popup^ popup;
            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

            auto listViewDragItemsCompletedEvent = std::make_shared<Event>();
            auto listViewDragItemsCompletedRegistration = CreateSafeEventRegistration(xaml_controls::ListView, DragItemsCompleted);

            RunOnUIThread([&]()
            {
                ResourceDictionary^ rd = dynamic_cast<ResourceDictionary^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(ref new Platform::String(ResourceXaml)));
                Platform::Collections::Vector<Contact^>^ items = ref new Platform::Collections::Vector<Contact^>();
                for (unsigned int i = 0; i < 20U; i++)
                {
                    items->Append(ref new Contact(i));
                }

                listView = ref new ListView();
                listView->Width = 300;
                listView->Height = 650;
                listView->ItemsSource = items;
                listView->CanReorderItems = true;
                listView->CanDragItems = true;
                listView->AllowDrop = true;
                listView->ItemTemplate = dynamic_cast<DataTemplate^>(rd->Lookup(ref new Platform::String(L"SimpleTemplate")));

                auto root = ref new Grid();
                TestServices::WindowHelper->WindowContent = root;
                loadedRegistration.Attach(root, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Grid loaded.");
                    loadedEvent->Set();
                }));

                popup = ref new xaml_primitives::Popup();
                popup->Child = listView;
                popup->IsLightDismissEnabled = true;
                {
                    auto xamlRoot = root->XamlRoot;
                    if (xamlRoot)
                    {
                        // UAP will return a null content root and does not need this to be set
                        popup->XamlRoot = xamlRoot;
                    }
                }
            });

            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // register for the DragItemsCompleted event
                listViewDragItemsCompletedRegistration.Attach(listView, ref new TypedEventHandler<ListViewBase^, DragItemsCompletedEventArgs^>(
                    [listViewDragItemsCompletedEvent, popup]
                (ListViewBase^, DragItemsCompletedEventArgs^)
                {
                    VERIFY_IS_TRUE(popup->IsOpen);
                    listViewDragItemsCompletedEvent->Set();
                }));

                popup->IsOpen = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Dragging ListViewItem.");
            TestServices::InputHelper->DragFromCenter(listView, 0 /*relX*/, 200 /*relY*/, DefaultSpeed /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();

            listViewDragItemsCompletedEvent->WaitForDefault();
        }

        void BasicDragDropTests::CanSetCanDragOnListViewItem()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            ListView^ listView = nullptr;
            auto listLoadedEvent = std::make_shared<Event>();
            auto dragCompletedEVent = std::make_shared<Event>();
            auto listContainerContentChangingRegistration = CreateSafeEventRegistration(ListView, ContainerContentChanging);
            auto listDragItemsStartingRegistration = CreateSafeEventRegistration(ListView, DragItemsStarting);
            auto listDragItemsCompletedRegistration = CreateSafeEventRegistration(ListView, DragItemsCompleted);

            std::vector<SafeEventRegistration<UIElement, TypedEventHandler<UIElement^, DragStartingEventArgs^>>> dragStartingRegistrations;
            std::vector<SafeEventRegistration<UIElement, TypedEventHandler<UIElement^, DropCompletedEventArgs^>>> dropCompletedRegistrations;

            unsigned int n = 20;
            unsigned int realizedItems = 0;
            Platform::Collections::Vector<Contact^>^ items = ref new Platform::Collections::Vector<Contact^>();
            for (unsigned int i = 0; i < n; i++)
            {
                items->Append(ref new Contact(i));
            }

            int dragStartingCount = 0;
            int dropCompletedCount = 0;
            int dragItemsStartingCount = 0;
            int dragItemsCompletedCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();
                ResourceDictionary^ rd = dynamic_cast<ResourceDictionary^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(ref new Platform::String(ResourceXaml)));
                VERIFY_IS_NOT_NULL(rd);

                listView = ref new ListView();
                listView->Width = 300;
                listView->Height = 650;
                listView->HorizontalAlignment = HorizontalAlignment::Center;
                listView->VerticalAlignment = VerticalAlignment::Center;
                listView->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                listView->ItemsSource = items;
                listView->CanReorderItems = false;
                listView->CanDragItems = true;
                listView->ItemTemplate = dynamic_cast<DataTemplate^>(rd->Lookup(ref new Platform::String(L"SimpleTemplate")));
                mainGrid->Children->Append(listView);

                listContainerContentChangingRegistration.Attach(listView, ref new TypedEventHandler<ListViewBase^, ContainerContentChangingEventArgs^>(
                    [&realizedItems,n,listLoadedEvent,&dragStartingCount, &dropCompletedCount, dragCompletedEVent, &dragStartingRegistrations,&dropCompletedRegistrations](ListViewBase^, ContainerContentChangingEventArgs^args)
                {
                    LOG_OUTPUT(L"listContainerContentChanging: %d", ++realizedItems);

                    auto lvi = dynamic_cast<ListViewItem^>(args->ItemContainer);
                    VERIFY_IS_NOT_NULL(lvi);

                    lvi->CanDrag = true;

                    // We save the registrations at the outer scope to keep the handlers alive for the whole test duration
                    dragStartingRegistrations.push_back(CreateSafeEventRegistration(UIElement, DragStarting));
                    dragStartingRegistrations[dragStartingRegistrations.size()-1].Attach(lvi, ref new TypedEventHandler<UIElement^, DragStartingEventArgs^>([&dragStartingCount](xaml::UIElement^ sender, xaml::DragStartingEventArgs^)
                    {
                        LOG_OUTPUT(L"DragStarting.");
                        dragStartingCount++;
                    }));
                    dropCompletedRegistrations.push_back(CreateSafeEventRegistration(UIElement, DropCompleted));
                    dropCompletedRegistrations[dropCompletedRegistrations.size()-1].Attach(lvi, ref new TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&dropCompletedCount, dragCompletedEVent](xaml::UIElement^ sender, xaml::DropCompletedEventArgs^)
                    {
                        LOG_OUTPUT(L"DropCompleted.");
                        dropCompletedCount++;
                        dragCompletedEVent->Set();
                    }));

                    if (realizedItems == n)
                    {
                        listLoadedEvent->Set();
                    }
                }));

                listDragItemsStartingRegistration.Attach(listView, ref new DragItemsStartingEventHandler([&dragItemsStartingCount](Platform::Object^, DragItemsStartingEventArgs^)
                {
                    LOG_OUTPUT(L"DragItemsStarting.");
                    dragItemsStartingCount++;
                }));
                listDragItemsCompletedRegistration.Attach(listView, ref new TypedEventHandler<ListViewBase^, DragItemsCompletedEventArgs^>([&dragItemsCompletedCount, dragCompletedEVent](ListViewBase^, DragItemsCompletedEventArgs^)
                {
                    LOG_OUTPUT(L"DragItemsCompleted.");
                    dragItemsCompletedCount++;
                    dragCompletedEVent->Set();
                }));
                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            listLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging ListViewItem with Touch");
            TestServices::InputHelper->PressHoldAndPanFromCenter(listView, 0 /*relX*/, 150 /*relY*/, DefaultSpeed /*velocityFactor*/, 1000 /*holdTime*/);
            TestServices::WindowHelper->WaitForIdle();
            dragCompletedEVent->WaitForDefault();
            RestoreForegroundWindowIfAvailable();

            VERIFY_ARE_EQUAL(1, dragStartingCount);
            VERIFY_ARE_EQUAL(1, dropCompletedCount);
            VERIFY_ARE_EQUAL(1, dragItemsStartingCount);
            VERIFY_ARE_EQUAL(1, dragItemsCompletedCount);
            dragStartingCount = 0;
            dropCompletedCount = 0;
            dragItemsStartingCount = 0;
            dragItemsCompletedCount = 0;

            LOG_OUTPUT(L"Dragging ListViewItem with Mouse");
            TestServices::InputHelper->DragFromCenter(listView, 0 /*relX*/, 150 /*relY*/, DefaultSpeed /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();
            dragCompletedEVent->WaitForDefault();
            RestoreForegroundWindowIfAvailable();

            VERIFY_ARE_EQUAL(1, dragStartingCount);
            VERIFY_ARE_EQUAL(1, dropCompletedCount);
            VERIFY_ARE_EQUAL(1, dragItemsStartingCount);
            VERIFY_ARE_EQUAL(1, dragItemsCompletedCount);
            dragStartingCount = 0;
            dropCompletedCount = 0;
            dragItemsStartingCount = 0;
            dragItemsCompletedCount = 0;


            // Now disable CanDragItems on ListView, CanDrag on ListViewItem should take over
            RunOnUIThread([&]()
            {
                listView->CanDragItems = false;
            });

            LOG_OUTPUT(L"Dragging ListViewItem with Touch");
            TestServices::InputHelper->PressHoldAndPanFromCenter(listView, 0 /*relX*/, 150 /*relY*/, DefaultSpeed /*velocityFactor*/, 1000 /*holdTime*/);
            TestServices::WindowHelper->WaitForIdle();
            dragCompletedEVent->WaitForDefault();
            RestoreForegroundWindowIfAvailable();

            // ListView CanDragItems is now false
            VERIFY_ARE_EQUAL(1, dragStartingCount);
            VERIFY_ARE_EQUAL(1, dropCompletedCount);
            VERIFY_ARE_EQUAL(0, dragItemsStartingCount);
            VERIFY_ARE_EQUAL(0, dragItemsCompletedCount);
            dragStartingCount = 0;
            dropCompletedCount = 0;

            LOG_OUTPUT(L"Dragging ListViewItem with Mouse");
            TestServices::InputHelper->DragFromCenter(listView, 0 /*relX*/, 150 /*relY*/, DefaultSpeed /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();
            dragCompletedEVent->WaitForDefault();
            RestoreForegroundWindowIfAvailable();

            // ListView CanDragItems is now false
            VERIFY_ARE_EQUAL(1, dragStartingCount);
            VERIFY_ARE_EQUAL(1, dropCompletedCount);
            VERIFY_ARE_EQUAL(0, dragItemsStartingCount);
            VERIFY_ARE_EQUAL(0, dragItemsCompletedCount);
            dragStartingCount = 0;
            dropCompletedCount = 0;
        }

        void BasicDragDropTests::CanDeleteDraggedElement()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            ::Windows::Foundation::Point listviewCenter;
            ListView^ listView = nullptr;
            auto listLoadedEvent = std::make_shared<Event>();
            auto listContainerContentChanging = CreateSafeEventRegistration(ListView, ContainerContentChanging);
            auto listDragItemsStarting = CreateSafeEventRegistration(ListView, DragItemsStarting);
            auto listDragItemsCompletedRegistration = CreateSafeEventRegistration(ListView, DragItemsCompleted);

            auto dragItemsStartingEvent = std::make_shared<Event>();
            auto dragCompletedEvent = std::make_shared<Event>();

            unsigned int n = 20;
            unsigned int realizedItems = 0;
            auto items = ref new Platform::Collections::Vector<Contact^>();
            for (unsigned int i = 0; i < n; i++)
            {
                items->Append(ref new Contact(i));
            }

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();
                ResourceDictionary^ rd = dynamic_cast<ResourceDictionary^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(ref new Platform::String(ResourceXaml)));
                VERIFY_IS_NOT_NULL(rd);

                listView = ref new ListView();
                listView->Width = 300;
                listView->Height = 650;
                listView->HorizontalAlignment = HorizontalAlignment::Center;
                listView->VerticalAlignment = VerticalAlignment::Center;
                listView->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                listView->ItemsSource = items;
                listView->CanReorderItems = true;
                listView->CanDragItems = true;
                listView->AllowDrop = true;
                listView->ItemTemplate = dynamic_cast<DataTemplate^>(rd->Lookup(ref new Platform::String(L"SimpleTemplate")));
                mainGrid->Children->Append(listView);

                listContainerContentChanging.Attach(listView, ref new TypedEventHandler<ListViewBase^, ContainerContentChangingEventArgs^>([&realizedItems, n, listLoadedEvent, &listviewCenter](ListViewBase^ sender, ContainerContentChangingEventArgs^)
                {
                    LOG_OUTPUT(L"listContainerContentChanging: %d", ++realizedItems);
                    if (realizedItems == n)
                    {
                        listLoadedEvent->Set();
                    }
                }));

                listDragItemsStarting.Attach(listView, ref new DragItemsStartingEventHandler([dragItemsStartingEvent, &items](Platform::Object^, DragItemsStartingEventArgs^ e)
                {
                    LOG_OUTPUT(L"DragItemsStarting...");
                    Window::Current->Dispatcher->RunAsync(
                        ::Windows::UI::Core::CoreDispatcherPriority::Normal,
                        ref new ::Windows::UI::Core::DispatchedHandler([&items]() {
                            LOG_OUTPUT(L"Remove item at index 6");
                            items->RemoveAt(6);
                    }));
                }));
                listDragItemsCompletedRegistration.Attach(listView, ref new TypedEventHandler<ListViewBase^, DragItemsCompletedEventArgs^>([dragCompletedEvent](ListViewBase^, DragItemsCompletedEventArgs^ e)
                {
                    LOG_OUTPUT(L"DragItemsCompleted.");
                    VERIFY_ARE_EQUAL(DataPackageOperation::None, e->DropResult);
                    dragCompletedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            listLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging ListViewItem.");
            TestServices::InputHelper->PressHoldAndPanFromCenter(listView, 0 /*relX*/, 220 /*relY*/, DefaultSpeed / 2 /*velocityFactor*/, 3000 /*holdTime*/);
            LOG_OUTPUT(L"Waiting on dragCompletedEvent.");
            dragCompletedEvent->WaitForDefault();
        }

        void BasicDragDropTests::CanReorderAndDropOnItems()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            ::Windows::Foundation::Point listviewCenter;
            ListView^ listView = nullptr;
            auto listReadyEvent = std::make_shared<Event>();
            auto droppedOnItemEvent = std::make_shared<Event>();
            auto listContainerContentChanging = CreateSafeEventRegistration(ListView, ContainerContentChanging);
            auto listDragItemsStarting = CreateSafeEventRegistration(ListView, DragItemsStarting);

            std::vector<SafeEventRegistration<UIElement, DragEventHandler>> dragEnterRegistrations;
            std::vector<SafeEventRegistration<UIElement, DragEventHandler>> dragOverRegistrations;
            std::vector<SafeEventRegistration<UIElement, DragEventHandler>> dropRegistrations;
            auto dragItemsStartingEvent = std::make_shared<Event>();
            unsigned int n = 20;
            unsigned int realizedItems = 0;
            auto items = ref new Platform::Collections::Vector<Contact^>();
            for (unsigned int i = 0; i < n; i++)
            {
                items->Append(ref new Contact(i));
            }

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();
                ResourceDictionary^ rd = dynamic_cast<ResourceDictionary^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(ref new Platform::String(ResourceXaml)));
                VERIFY_IS_NOT_NULL(rd);

                listView = ref new ListView();
                listView->Width = 300;
                listView->Height = 650;
                listView->HorizontalAlignment = HorizontalAlignment::Center;
                listView->VerticalAlignment = VerticalAlignment::Center;
                listView->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                listView->ItemsSource = items;
                listView->CanReorderItems = true;
                listView->CanDragItems = true;
                listView->AllowDrop = true;
                listView->ItemTemplate = dynamic_cast<DataTemplate^>(rd->Lookup(ref new Platform::String(L"SimpleTemplate")));
                mainGrid->Children->Append(listView);

                listContainerContentChanging.Attach(listView, ref new TypedEventHandler<ListViewBase^, ContainerContentChangingEventArgs^>([
                                                                    &realizedItems, n, listReadyEvent, &listviewCenter,
                                                                    &dragEnterRegistrations,
                                                                    &dragOverRegistrations,
                                                                    &dropRegistrations,
                                                                    droppedOnItemEvent](ListViewBase^ sender, ContainerContentChangingEventArgs^ args)
                {
                    LOG_OUTPUT(L"listContainerContentChanging: %d", ++realizedItems);
                    auto contact = dynamic_cast<Contact^>(args->Item);
                    if (contact != nullptr)
                    {
                        UIElement^ root = args->ItemContainer->ContentTemplateRoot;
                        if (contact->Id >= 8)
                        {
                            LOG_OUTPUT(L"Enabling Drag and Drop.");
                            root->AllowDrop = true;
                            Border^ border = dynamic_cast<Border^>(root);
                            if (border != nullptr)
                            {
                                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
                            }
                            dragEnterRegistrations.push_back(CreateSafeEventRegistration(UIElement, DragEnter));
                            dragEnterRegistrations[dragEnterRegistrations.size()-1].Attach(root, ref new DragEventHandler([](Platform::Object^ sender, DragEventArgs^ e)
                            {
                                LOG_OUTPUT(L"DragEnter on item.");
                                e->Handled = true;
                                e->AcceptedOperation = DataPackageOperation::Move;
                            }));
                            dragOverRegistrations.push_back(CreateSafeEventRegistration(UIElement, DragOver));
                            dragOverRegistrations[dragOverRegistrations.size()-1].Attach(root, ref new DragEventHandler([](Platform::Object^ sender, DragEventArgs^ e)
                            {
                                LOG_OUTPUT(L"DragOver on item.");
                                e->Handled = true;
                                e->AcceptedOperation = DataPackageOperation::Move;
                            }));
                            dropRegistrations.push_back(CreateSafeEventRegistration(UIElement, Drop));
                            dropRegistrations[dropRegistrations.size() - 1].Attach(root, ref new DragEventHandler([droppedOnItemEvent](Platform::Object^ sender, DragEventArgs^ e)
                            {
                                LOG_OUTPUT(L"Drop on item.");
                                e->Handled = true;
                                e->AcceptedOperation = DataPackageOperation::Move;
                                droppedOnItemEvent->Set();
                            }));
                        }
                    }

                    if (realizedItems == n)
                    {
                        // we don't need the containers to be realized to do this
                        // but here is a convenient way to do it only once
                        auto transform = sender->TransformToVisual(nullptr);
                        ::Windows::Foundation::Point listviewCenter;
                        listviewCenter.X = static_cast<float>(sender->ActualWidth / 2.0);
                        listviewCenter.Y = static_cast<float>(sender->ActualHeight / 2.0);
                        listviewCenter = transform->TransformPoint(listviewCenter);

                        listReadyEvent->Set();
                    }
                }));

                listDragItemsStarting.Attach(listView, ref new DragItemsStartingEventHandler([dragItemsStartingEvent](Platform::Object^, DragItemsStartingEventArgs^ e)
                {
                    LOG_OUTPUT(L"DragItemsStarting...");
                    dragItemsStartingEvent->Set();
                    auto n = e->Items->Size;
                    for (unsigned int i = 0; i < n; i++)
                    {
                        auto contact = dynamic_cast<Platform::String^>(e->Items->GetAt(i));
                        if (contact != nullptr)
                        {
                            LOG_OUTPUT(L"%s", contact->Data());
                        }
                    }
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            listReadyEvent->WaitForDefault();
            // Do twice the loop to check that the ListView is in good shape after first D&D
            for (int testrun = 0; testrun < 2; testrun++)
            {
                droppedOnItemEvent->Reset();
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"Dragging ListViewItem.");
                TestServices::InputHelper->PressHoldAndPanFromCenter(listView, 0 /*relX*/, 250 /*relY*/, DefaultSpeed /*velocityFactor*/, 1000 /*holdTime*/);
                LOG_OUTPUT(L"Waiting\n");

                // verify we dropped on the item
                droppedOnItemEvent->WaitForDefault();

                // As the drop occurred on a item with AllowDrop=True and it set Handled=True,
                // ListView should not have been reordered
                for (unsigned int i = 0; i < items->Size; i++)
                {
                    VERIFY_ARE_EQUAL(i, items->GetAt(i)->Id);
                }
            }
        }

        void BasicDragDropTests::CanCollapseDraggedElement()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount, rect, border](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    sender->Visibility = Microsoft::UI::Xaml::Visibility::Collapsed;
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                rectPointerReleasedRegistration.Attach(rect, ref new PointerEventHandler([rectPointerReleasedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerReleased event - Mouse lifted");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    rectPointerReleasedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void BasicDragDropTests::CanReorderWithLegacyPanel()
        {
            PerformTouchListReordering(true /* useLegacyPanel */);
        }

        void BasicDragDropTests::PerformTouchListReordering(bool useLegacyPanel)
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            ListView^ listView = nullptr;
            auto listLoadedEvent = std::make_shared<Event>();
            auto parallelInputCompletedEvent = std::make_shared<Event>();
            auto listLoaded = CreateSafeEventRegistration(ListView, Loaded);
            auto listDragItemsStarting = CreateSafeEventRegistration(ListView, DragItemsStarting);
            auto dragItemsStartingEvent = std::make_shared<Event>();
            unsigned int n = 20;
            auto items = ref new Platform::Collections::Vector<Contact^>();
            for (unsigned int i = 0; i < n; i++)
            {
                items->Append(ref new Contact(i));
            }

            // Controls whether we cancel drag on DragItemsStarting or not.
            bool cancelDrag = false;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();
                ResourceDictionary^ rd = dynamic_cast<ResourceDictionary^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(ref new Platform::String(ResourceXaml)));
                VERIFY_IS_NOT_NULL(rd);

                listView = ref new ListView();
                listView->Width = 300;
                listView->Height = 650;
                listView->HorizontalAlignment = HorizontalAlignment::Center;
                listView->VerticalAlignment = VerticalAlignment::Center;
                listView->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                listView->ItemsSource = items;
                listView->CanReorderItems = true;
                listView->CanDragItems = true;
                listView->AllowDrop = true;
                listView->ItemTemplate = dynamic_cast<DataTemplate^>(rd->Lookup(ref new Platform::String(L"SimpleTemplate")));

                if (useLegacyPanel)
                {
                    LOG_OUTPUT(L"Using VirtualizingStackPanel legacy panel.");
                    listView->ItemsPanel = dynamic_cast<ItemsPanelTemplate^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
L"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >\n\
     <VirtualizingStackPanel />\
</ItemsPanelTemplate>"));
                }

                mainGrid->Children->Append(listView);

                listLoaded.Attach(listView, ref new RoutedEventHandler(([&listLoadedEvent](Platform::Object^ sender, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"List loaded!");
                    listLoadedEvent->Set();
                })));

                listDragItemsStarting.Attach(listView, ref new DragItemsStartingEventHandler([dragItemsStartingEvent, &cancelDrag](Platform::Object^, DragItemsStartingEventArgs^ e)
                {
                    LOG_OUTPUT(L"DragItemsStarting...");
                    dragItemsStartingEvent->Set();
                    auto n = e->Items->Size;
                    for (unsigned int i = 0; i < n; i++)
                    {
                        auto contact = dynamic_cast<Platform::String^>(e->Items->GetAt(i));
                        if (contact != nullptr)
                        {
                            LOG_OUTPUT(L"%s", contact->Data());
                        }
                    }

                    if (cancelDrag)
                    {
                        e->Cancel = true;
                    }
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            listLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // We inject in parallel input from another pointer to validate that this will not affect
            // the current drag operation.
            ::Windows::System::Threading::ThreadPool::RunAsync(ref new ::Windows::System::Threading::WorkItemHandler([dragItemsStartingEvent, listView, parallelInputCompletedEvent](IAsyncAction^)
            {
                dragItemsStartingEvent->WaitForDefault();
                LOG_OUTPUT(L"Mouse click\n");
                TestServices::InputHelper->ClickMouseButton(MouseButton::Left, listView);
                parallelInputCompletedEvent->Set();
            }));

            LOG_OUTPUT(L"Dragging ListViewItem.");
            TestServices::InputHelper->PressHoldAndPanFromCenter(listView, 0 /*relX*/, 220 /*relY*/, DefaultSpeed /*velocityFactor*/, 3000 /*holdTime*/);
            LOG_OUTPUT(L"Waiting\n");
            TestServices::WindowHelper->WaitForIdle();
            parallelInputCompletedEvent->WaitForDefault();

            auto valdidateData = [&]()
            {
                // As we have dragged from the middle: item #6 has been moved (650/50 = 13 visible items)
                // and 220 px down means that it should be at index 10
                // and at indexes between 6 and 9, we are expected former items #7 to #10
                for (unsigned int i = 0; i < items->Size; i++)
                {
                    unsigned int expectedId = i;
                    switch (i)
                    {
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                        expectedId = i + 1;
                        break;
                    case 10:
                        expectedId = 6;
                        break;
                    }
                    VERIFY_ARE_EQUAL(expectedId, items->GetAt(i)->Id);
                };
            };

            valdidateData();

            // Let's try the same drag operation, but this time we will cancel.
            cancelDrag = true;
            LOG_OUTPUT(L"Dragging ListViewItem. Expecting cancellation on DragItemsStarting.");
            TestServices::InputHelper->PressHoldAndPanFromCenter(listView, 0 /*relX*/, 220 /*relY*/, DefaultSpeed /*velocityFactor*/, 3000 /*holdTime*/);
            LOG_OUTPUT(L"Waiting\n");
            TestServices::WindowHelper->WaitForIdle();

            valdidateData();
        }

        void BasicDragDropTests::AllowedOperationsPassThrough()
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            ListView ^root = nullptr;

            ListViewItem    ^lvi1 = nullptr,
                            ^lvi2 = nullptr;

            TextBlock ^tb1 = nullptr,
                      ^tb2 = nullptr;

            ::Windows::ApplicationModel::DataTransfer::DataPackageOperation allowedOperations = ::Windows::ApplicationModel::DataTransfer::DataPackageOperation::None;

            auto lvi1DragStart = std::make_shared<Event>();
            auto lvi1DropCompleted = std::make_shared<Event>();

            auto lvi2DragOver = std::make_shared<Event>();
            auto lvi2DragEnter = std::make_shared<Event>();
            auto lvi2DragLeave = std::make_shared<Event>();

            auto lvi1DragStartingRegistration = CreateSafeEventRegistration(ListViewItem, DragStarting);
            auto lvi1DropCompletedRegistration = CreateSafeEventRegistration(ListViewItem, DropCompleted);

            auto lvi2DragEnterRegistration = CreateSafeEventRegistration(ListViewItem, DragEnter);
            auto lvi2DragOverRegistration = CreateSafeEventRegistration(ListViewItem, DragOver);
            auto lvi2DragLeaveRegistration = CreateSafeEventRegistration(ListViewItem, DragLeave);

            RunOnUIThread([&] {
                root = ref new ListView();
                lvi1 = ref new ListViewItem();
                lvi2 = ref new ListViewItem();

                tb1 = ref new TextBlock();
                tb2 = ref new TextBlock();

                lvi1->Content = tb1;
                lvi1->CanDrag = true;

                lvi2->Content = tb2;
                lvi2->AllowDrop = true;

                tb1->Text = "Dragable text";
                tb1->Margin = xaml::ThicknessHelper::FromUniformLength(25);

                tb2->Text = "Drag Target";
                tb2->Margin = xaml::ThicknessHelper::FromUniformLength(25);

                lvi1DragStartingRegistration.Attach(lvi1, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    args->AllowedOperations = allowedOperations;
                    LOG_OUTPUT(L"AllowedOperations set to %u", args->AllowedOperations);
                    lvi1DragStart->Set();
                }));

                lvi1DropCompletedRegistration.Attach(lvi1, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    lvi1DropCompleted->Set();
                }));

                lvi2DragEnterRegistration.Attach(lvi2, ref new DragEventHandler([&](Platform::Object^ sender, DragEventArgs^ args)
                {
                    VERIFY_ARE_EQUAL(args->AllowedOperations, allowedOperations);
                    lvi2DragEnter->Set();
                }));

                lvi2DragOverRegistration.Attach(lvi2, ref new DragEventHandler([&](Platform::Object^ sender, DragEventArgs^ args)
                {
                    VERIFY_ARE_EQUAL(args->AllowedOperations, allowedOperations);
                    lvi2DragOver->Set();
                }));

                lvi2DragLeaveRegistration.Attach(lvi2, ref new DragEventHandler([&](Platform::Object^ sender, DragEventArgs^ args)
                {
                    VERIFY_ARE_EQUAL(args->AllowedOperations, allowedOperations);
                    lvi2DragLeave->Set();
                }));

                root->Items->Append(lvi1);
                root->Items->Append(lvi2);

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();
            RestoreForegroundWindowIfAvailable();

            // Iterate through the possible DatapackageOperations combinations.  Currently there are only 3 that can be or'd together so any integer 0->7
            for (unsigned int i = 0; i < 8; i++)
            {
                allowedOperations = (::Windows::ApplicationModel::DataTransfer::DataPackageOperation) i;
                LOG_OUTPUT(L"Testing AllowedOperations setting %u", i);
                TestServices::InputHelper->DragFromCenter(lvi1, 0, 150, DefaultSpeed);

                lvi1DragStart->WaitForDefault();
                lvi2DragEnter->WaitForDefault();
                lvi2DragOver->WaitForDefault();
                lvi2DragLeave->WaitForDefault();
                lvi1DropCompleted->WaitForDefault();

                TestServices::WindowHelper->WaitForIdle();

                RestoreForegroundWindowIfAvailable();  // Ensure the test window is on top after the drag.  Sometimes other windows pop up.
                TestServices::WindowHelper->WaitForIdle();

                RestoreForegroundWindowIfAvailable();
                TestServices::WindowHelper->WaitForIdle();
            }
        }
        
        void BasicDragDropTests::CanDragToFromWindowedPopups()
        {
            TestCleanupWrapper cleanup([]()
                {
                    BasicDragDropTests::DndTestCleanup();
                });

            EnsureDataExchangeHostStarted();

            Grid^ mainPanel;

            auto mainPanelLoadedEvent = std::make_shared<Event>();
            auto mainPanelLoadedRegistration = CreateSafeEventRegistration(Grid, Loaded);

            Shapes::Rectangle^ mainDragSource;
            Shapes::Rectangle^ mainDropTarget;

            Shapes::Rectangle^ abovePopupDragSource;
            Shapes::Rectangle^ abovePopupDropTarget;

            Shapes::Rectangle^ belowPopupDragSource;
            Shapes::Rectangle^ belowPopupDropTarget;

            RunOnUIThread([&]()
                {
                    mainPanel = dynamic_cast<Grid^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\n"
                        L"   <StackPanel x:Name='MainStackPanel' Orientation='Horizontal' Spacing='10' HorizontalAlignment='Center' VerticalAlignment='Center' Padding='40,40,40,40'>\n"
                        L"        <Border BorderThickness='1,1,1,1' BorderBrush='Blue' Background='AntiqueWhite' Padding='40,40,40,40'>\n"
                        L"            <StackPanel Orientation='Horizontal' Spacing='50'>\n"
                        L"                <TextBlock Text='Main Page' Foreground='Blue'/>\n"
                        L"                <Rectangle x:Name='MainDragSource' Height='50' Width='100' Fill='LightBlue'/>\n"
                        L"                <Rectangle x:Name='MainDropTarget' Height='50' Width='100' Fill='GoldenRod'/>\n"
                        L"            </StackPanel>\n"
                        L"        </Border>\n"
                        L"    </StackPanel>\n"
                        L"    <Popup x:Name='AbovePopup' ShouldConstrainToRootBounds='false' DesiredPlacement='Top'>\n"
                        L"        <Border BorderThickness='1,1,1,1' BorderBrush='Black' Background='AntiqueWhite' Padding='40,40,40,40'>\n"
                        L"            <StackPanel Orientation='Horizontal' Spacing='50'>\n"
                        L"                <TextBlock Text='Above Popup' Foreground='Blue'/>\n"
                        L"                <Rectangle x:Name='AboveDragSource' Height='50' Width='100' Fill='LightBlue'/>\n"
                        L"                <Rectangle x:Name='AboveDragTarget' Height='50' Width='100' Fill='GoldenRod' />\n"
                        L"            </StackPanel>\n"
                        L"        </Border>\n"
                        L"    </Popup>\n"
                        L"    <Popup x:Name='BelowPopup' ShouldConstrainToRootBounds='false' DesiredPlacement='Bottom'>\n"
                        L"        <Border BorderThickness='1,1,1,1' BorderBrush='Black' Background='AntiqueWhite' Padding='40,40,40,40'>\n"
                        L"            <StackPanel Orientation='Horizontal' Spacing='50'>\n"
                        L"                <TextBlock Text='Below Popup' Foreground='Blue'/>\n"
                        L"                <Rectangle x:Name='BelowDragSource' Height='50' Width='100' Fill='LightBlue'/>\n"
                        L"                <Rectangle x:Name='BelowDragTarget' Height='50' Width='100' Fill='GoldenRod'/>\n"
                        L"            </StackPanel>\n"
                        L"        </Border>\n"
                        L"    </Popup>\n"
                        L"</Grid>"));

                    mainDragSource = dynamic_cast<Shapes::Rectangle^>(mainPanel->FindName(L"MainDragSource"));
                    VERIFY_IS_NOT_NULL(mainDragSource);
                    mainDropTarget = dynamic_cast<Shapes::Rectangle^>(mainPanel->FindName(L"MainDropTarget"));
                    VERIFY_IS_NOT_NULL(mainDropTarget);

                    abovePopupDragSource = dynamic_cast<Shapes::Rectangle^>(mainPanel->FindName(L"AboveDragSource"));
                    VERIFY_IS_NOT_NULL(abovePopupDragSource);
                    abovePopupDropTarget = dynamic_cast<Shapes::Rectangle^>(mainPanel->FindName(L"AboveDragTarget"));
                    VERIFY_IS_NOT_NULL(abovePopupDropTarget);

                    belowPopupDragSource = dynamic_cast<Shapes::Rectangle^>(mainPanel->FindName(L"BelowDragSource"));
                    VERIFY_IS_NOT_NULL(belowPopupDragSource);
                    belowPopupDropTarget = dynamic_cast<Shapes::Rectangle^>(mainPanel->FindName(L"BelowDragTarget"));
                    VERIFY_IS_NOT_NULL(belowPopupDropTarget);

                    mainPanelLoadedRegistration.Attach(mainPanel, ref new RoutedEventHandler([&mainPanelLoadedEvent](Platform::Object^ sender, RoutedEventArgs^ args)
                        {
                            mainPanelLoadedEvent->Set();
                        }));

                    TestServices::WindowHelper->WindowContent = mainPanel;
                });

            mainPanelLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
                {
                    // We need to set the target and open the popups after we have completed a frame so that the popup is
                    // attached to the tree.
                    auto mainStackPanel = dynamic_cast<StackPanel^>(mainPanel->FindName(L"MainStackPanel"));

                    auto abovePopup = dynamic_cast<Primitives::Popup^>(mainPanel->FindName(L"AbovePopup"));
                    VERIFY_IS_NOT_NULL(abovePopup);
                    abovePopup->PlacementTarget = mainStackPanel;
                    abovePopup->IsOpen = true;

                    auto belowPopup = dynamic_cast<Primitives::Popup^>(mainPanel->FindName(L"BelowPopup"));
                    VERIFY_IS_NOT_NULL(belowPopup);
                    belowPopup->PlacementTarget = mainStackPanel;
                    belowPopup->IsOpen = true;
                });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"***** Test Drag Within a Popup *****");
            TestDragDropBetweenElements(abovePopupDragSource, abovePopupDropTarget);

            LOG_OUTPUT(L"***** Test Drag between popups *****");
            TestDragDropBetweenElements(belowPopupDragSource, abovePopupDropTarget);

            LOG_OUTPUT(L"***** Test Drag From Main Scene to Popup *****");
            TestDragDropBetweenElements(mainDragSource, abovePopupDropTarget);

            LOG_OUTPUT(L"***** Test Drag From Popup to Main Scene *****");
            TestDragDropBetweenElements(belowPopupDragSource, mainDropTarget);

            //
            // This is a complex test that will test dragging the pointer across multiple content islands multiple times.
            // This is to try to ensure we hit both the scenario where we get reentrancy of drag events and the scenario
            // where we get the enter event for one island before we get the leave event for the previous one.
            //
            {
                LOG_OUTPUT(L"***** Test Drag Across Multiple Islands and Main *****");

                DragSourceHelper abovePopupDragSourceHelper(abovePopupDragSource);
                DropTargetHelper abovePopupDropTargetHelper(abovePopupDropTarget);
                DropTargetHelper belowPopupDropTargetHelper(belowPopupDropTarget);
                DropTargetHelper mainDropTargetHelper(mainDropTarget);
                TestServices::WindowHelper->WaitForIdle();

                auto sourcePoint = GetCenterPoint(abovePopupDragSource);
                auto abovePopupTargetPoint = GetCenterPoint(abovePopupDropTarget);
                auto mainTargetPoint = GetCenterPoint(mainDropTarget);
                auto belowPopupTargetPoint = GetCenterPoint(belowPopupDropTarget);
                auto idlePoint = GetCenterPoint(belowPopupDragSource);

                // Press the mouse button.
                TestServices::InputHelper->MouseButtonDown(sourcePoint, MouseButton::Left);

                // Make sure we release the mouse button even if we error out
                TestCleanupWrapper cleanupDrag([&mainPanel]()
                    {
                        TestServices::InputHelper->MouseButtonUp(mainPanel, 0, 0, MouseButton::Left);
                    });

                // Move through the three drop targets and then out
                LOG_OUTPUT(L"Drag to the above popup drop target");
                DragBetweenPoints(sourcePoint, abovePopupTargetPoint);
                abovePopupDropTargetHelper.DragEnterEvent->WaitForDefault();

                LOG_OUTPUT(L"Drag to the main drop target");
                DragBetweenPoints(abovePopupTargetPoint, mainTargetPoint);
                abovePopupDropTargetHelper.DragLeaveEvent->WaitForDefault();
                mainDropTargetHelper.DragEnterEvent->WaitForDefault();

                LOG_OUTPUT(L"Drag to the below popup drop target");
                DragBetweenPoints(mainTargetPoint, belowPopupTargetPoint);
                mainDropTargetHelper.DragLeaveEvent->WaitForDefault();
                belowPopupDropTargetHelper.DragEnterEvent->WaitForDefault();

                LOG_OUTPUT(L"Drag to the below popup source (idle point)");
                DragBetweenPoints(belowPopupTargetPoint, idlePoint);
                belowPopupDropTargetHelper.DragLeaveEvent->WaitForDefault();

                // Reset our drop targets and move back through the drop targets in the opposite order.
                abovePopupDropTargetHelper.Reset();
                mainDropTargetHelper.Reset();
                belowPopupDropTargetHelper.Reset();

                LOG_OUTPUT(L"Drag to the below popup drop target");
                DragBetweenPoints(idlePoint, belowPopupTargetPoint);
                belowPopupDropTargetHelper.DragEnterEvent->WaitForDefault();

                LOG_OUTPUT(L"Drag to the main drop target");
                DragBetweenPoints(belowPopupTargetPoint, mainTargetPoint);
                belowPopupDropTargetHelper.DragLeaveEvent->WaitForDefault();
                mainDropTargetHelper.DragEnterEvent->WaitForDefault();

                LOG_OUTPUT(L"Drag to the above popup drop target");
                DragBetweenPoints(mainTargetPoint, abovePopupTargetPoint);
                mainDropTargetHelper.DragLeaveEvent->WaitForDefault();
                abovePopupDropTargetHelper.DragEnterEvent->WaitForDefault();

                LOG_OUTPUT(L"Drag to the above popup source (starting point)");
                DragBetweenPoints(abovePopupTargetPoint, sourcePoint);
                abovePopupDropTargetHelper.DragLeaveEvent->WaitForDefault();
            }
        }
        
        void BasicDragDropTests::SetCustomDragVisualWithBitmapUriSourceHelper(bool smallerDecodeSize)
        {
            TestCleanupWrapper cleanup([]() { BasicDragDropTests::DndTestCleanup(); });

            auto rectLoadedEvent = std::make_shared<Event>();
            auto rectDropCompletedEvent = std::make_shared<Event>();
            auto rectPointerReleasedEvent = std::make_shared<Event>();
            auto borderDropEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Shapes::Rectangle^ rect = nullptr;
            Border^ border = nullptr;

            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto rectDragStartingRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragStarting);
            auto rectDropCompletedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DropCompleted);
            auto rectPointerReleasedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerReleased);

            auto borderLoadedRegistration = CreateSafeEventRegistration(Border, Loaded);
            auto borderDragEnterRegistration = CreateSafeEventRegistration(Border, DragEnter);
            auto borderDragOverRegistration = CreateSafeEventRegistration(Border, DragOver);
            auto borderDragLeaveRegistration = CreateSafeEventRegistration(Border, DragLeave);
            auto borderDropRegistration = CreateSafeEventRegistration(Border, Drop);

            UINT borderDragEnterCount = 0;
            UINT borderDragOverCount = 0;
            UINT borderDragLeaveCount = 0;
            UINT borderDropCount = 0;
            UINT rectDragStartingCount = 0;
            UINT rectDropCompletedCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Shapes::Rectangle();
                rect->Width = 60;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->CanDrag = true;
                canvas->Children->Append(rect);

                border = ref new Border();
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->AllowDrop = true;
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);

                rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));

                rectDragStartingRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&rectDragStartingCount, &smallerDecodeSize](UIElement^ sender, DragStartingEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DragStarting event");
                    rectDragStartingCount++;

                    auto bitmapImage = ref new BitmapImage();
                    VERIFY_IS_NOT_NULL(bitmapImage);

                    if (smallerDecodeSize)
                    {
                        bitmapImage->DecodePixelWidth = 30;
                    }

                    auto testUri = ref new Uri(GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dragdrop\\pussinboots.jpg");
                    VERIFY_IS_NOT_NULL(testUri);

                    bitmapImage->UriSource = testUri;

                    args->Data->SetText(L"Hello world");
                    args->Data->RequestedOperation = DataPackageOperation::Copy;
                    args->DragUI->SetContentFromBitmapImage(bitmapImage);


                    args->Cancel = false;
                }));

                rectDropCompletedRegistration.Attach(rect, ref new wf::TypedEventHandler<UIElement^, DropCompletedEventArgs^>([&rectDropCompletedCount, rectDropCompletedEvent](UIElement^ sender, DropCompletedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle DropCompleted event");
                    rectDropCompletedCount++;

                    VERIFY_ARE_EQUAL(DataPackageOperation::Copy, args->DropResult);

                    rectDropCompletedEvent->Set();
                }));

                borderDragEnterRegistration.Attach(border, ref new DragEventHandler([&borderDragEnterCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragEnter event");
                    borderDragEnterCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragOverRegistration.Attach(border, ref new DragEventHandler([&borderDragOverCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragOver event");
                    borderDragOverCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                borderDragLeaveRegistration.Attach(border, ref new DragEventHandler([&borderDragLeaveCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border DragLeave event");
                    borderDragLeaveCount++;

                    auto border = dynamic_cast<Border^>(sender);
                    VERIFY_IS_NOT_NULL(border);
                }));

                borderDropRegistration.Attach(border, ref new DragEventHandler([&borderDropCount](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    borderDropCount++;

                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                rectPointerReleasedRegistration.Attach(rect, ref new PointerEventHandler([rectPointerReleasedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Rectangle PointerReleased event - Mouse lifted");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    rectPointerReleasedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 150 /*relX*/, 0 /*relY*/, DefaultSpeed /*velocityFactor*/);
            rectDropCompletedEvent->WaitForDefault();

            RestoreForegroundWindowIfAvailable();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(borderDragEnterCount, 1u);
                VERIFY_IS_TRUE(borderDragOverCount > 0);
                VERIFY_ARE_EQUAL(borderDragLeaveCount, 0u);
                VERIFY_ARE_EQUAL(borderDropCount, 1u);
                VERIFY_ARE_EQUAL(rectDragStartingCount, 1u);
                VERIFY_ARE_EQUAL(rectDropCompletedCount, 1u);

            });
        }

        void BasicDragDropTests::DragBetweenPoints(wf::Point point1, wf::Point point2)
        {
            // This is a customization of Input Helper's drag because for drag/drop we need greater control over
            // the cursor movement since we are interacting with a background process (DataExchangeHost) and
            // (as an example) we need to do things like ensure that the the Drag Enter has round tripped before
            // we can release the pointer.  Otherwise, the pointer could be released before DataExchangeHost has
            // captured the pointer input, thus losing us the drop.
 
            // Our objective is to use 25 steps to move the mouse.  But we adjust this for both a minimum and maximum 
            // distance per step.
            const float minStepDelta = 2;
            const float maxStepDelta = 10;
            int stepCount = 25;
            wf::Point delta = { point2.X - point1.X, point2.Y - point1.Y };

            auto stepDelta = max(abs(delta.X), abs(delta.Y)) / stepCount;
            if (stepDelta < minStepDelta)
            {
                stepCount = (int)round(stepDelta / minStepDelta);
            }
            else if (stepDelta > maxStepDelta)
            {
                stepCount = (int)round(stepDelta / maxStepDelta);
            }

            // We want our drag to be slow enough it is gives time for frames to process we will use a minim of a
            // quarter second and pause between generated mouse messages.
            int sleepInterval = 250 / stepCount;

            LOG_OUTPUT(L"Moving mouse from %.2f,%.2f to %.2f,%.2f using %d steps of %.2f,%.2f", point1.X, point1.Y, point2.X, point2.Y, stepCount, delta.X / stepCount, delta.Y / stepCount);

            for (int i = 1; i <= stepCount; i++)
            {
                auto point = point1;
                point.X += ((point2.X - point1.X) * i) / stepCount;
                point.Y += ((point2.Y - point1.Y) * i) / stepCount;
                TestServices::InputHelper->MoveMouse(point);
                ::Sleep(sleepInterval);
            }
        }

        void BasicDragDropTests::TestDragDropBetweenElements(FrameworkElement^ source, FrameworkElement^ target)
        {
            DragSourceHelper sourceHelper(source);
            DropTargetHelper targetHelper(target);
            TestServices::WindowHelper->WaitForIdle();

            auto sourcePoint = GetCenterPoint(source);
            auto targetPoint = GetCenterPoint(target);
            TestServices::InputHelper->MouseButtonDown(sourcePoint, MouseButton::Left);
            
            TestServices::WindowHelper->WaitForIdle();
            Sleep(1000);
            DragBetweenPoints(sourcePoint, targetPoint);

            // Don't do a wait for default here (which may fail out) because we need to ensure we sent 
            // the mouse up in case DataExchangeHost has captured the mouse.
            LOG_OUTPUT(L"Waiting for DragStarting Event");
            auto gotStartedEvent = sourceHelper.DragStartingEvent->WaitForNoThrow(sourceHelper.DragStartingEvent->Timeout);
            auto gotEnteredEvent = false;
            if (gotStartedEvent)
            {
                LOG_OUTPUT(L"Waiting for DragEnter Event");
                gotEnteredEvent = targetHelper.DragEnterEvent->WaitForNoThrow(targetHelper.DragEnterEvent->Timeout);
            }
            TestServices::InputHelper->MouseButtonUp(targetPoint, MouseButton::Left);
            VERIFY_IS_TRUE(gotStartedEvent);
            VERIFY_IS_TRUE(gotEnteredEvent);
            TestServices::WindowHelper->WaitForIdle();
        }


        // (Drag Enters can be missed when DataExchangeHost is loading) causes us to miss drag enters in some
        // tests.  If DataExchangeHost takes too long to load (such as in a cold load or low power machine) then we have
        // already released the pointer before DataExchangeHost tells us that the drag entered us and it is lost.  This
        // code will for the DataExchangeHost to load.
        //
        // Note: This must be run before constructing the actual test scene as it will affect the scene itself.
        void BasicDragDropTests::EnsureDataExchangeHostStarted()
        {
            // If we have done this in the last 30 seconds, don't bother since once DataExchangeHost is running it will
            // continue to run for a period of time.
            static ULONGLONG lastTickCount = 0;
            ULONGLONG prevTickCount = lastTickCount;
            lastTickCount =  GetTickCount64();
            if (lastTickCount - 30000 < prevTickCount) return;

            LOG_OUTPUT(L"Ensuring DataExchangeHost is started");
            DragDropTestScene scene;
            DragSourceHelper source(scene.Rectangle, true /*manualStart*/);
            DropTargetHelper target(scene.Border);
            TestServices::WindowHelper->WaitForIdle();

            for (int i = 0; i < 5; i++)
            {
                LOG_OUTPUT(L"Dragging Rectangle.");
                auto sourcePoint = GetCenterPoint(scene.Rectangle);
                auto targetPoint = GetCenterPoint(scene.Border);
                TestServices::InputHelper->MouseButtonDown(sourcePoint, MouseButton::Left);
                TestServices::WindowHelper->WaitForIdle();
                DragBetweenPoints(sourcePoint, targetPoint);

                LOG_OUTPUT(L"Waiting for DragStarting Event");
                auto hostStarted = source.DragStartingEvent->WaitForNoThrow(std::chrono::milliseconds(2000));
                if (!hostStarted)
                {
                    LOG_OUTPUT(L"Drag Starting event did not fire");
                }
                else
                {
                    LOG_OUTPUT(L"Waiting for DragEnter Event");
                    hostStarted = target.DragEnterEvent->WaitForNoThrow(std::chrono::milliseconds(2000));
                    if (!hostStarted)
                    {
                        LOG_OUTPUT(L"Drag Enter Event did not fire");
                    }
                }
                // Make sure we send the mouse up.
                TestServices::InputHelper->MouseButtonUp(targetPoint, MouseButton::Left);

                if (hostStarted)
                {
                    LOG_OUTPUT(L"Got the Drag Starting and Enter Events - DataExchangeHost should be started");
                    break;
                }
                // Reset everything in case we got any partial completions.
                source.DragStartingEvent->Reset();
                target.Reset();
            }

            target.DropEvent->WaitForDefault();
            LOG_OUTPUT(L"Completed the drop - action complete");
            LOG_OUTPUT(L"DataExchangeHost is running");
        }

        wf::Point BasicDragDropTests::GetCenterPoint(FrameworkElement^ element)
        {
            wf::Point point;
            RunOnUIThread([&]()
                {
                    auto point1 = element->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
                    auto point2 = element->TransformToVisual(nullptr)->TransformPoint(wf::Point((float)element->ActualWidth, (float)element->ActualHeight));

                    point.X = min(point1.X, point2.X) + (abs(point1.X - point2.X) / 2);
                    point.Y = min(point1.Y, point2.Y) + (abs(point1.Y - point2.Y) / 2);
                });
            return point;
        }
        DragDropTestScene::DragDropTestScene()
        {
            // It is unclear whether we need these loaded events anymore or whether they were in place before we had waitforidle.
            // I am leaving the them in, but we could consider removing them.
            auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
            auto borderLoadedRegistration = CreateSafeEventRegistration(Controls::Border, Loaded);
            auto rectLoadedEvent = std::make_shared<Event>();
            auto borderLoadedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                auto canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                Rectangle = ref new Shapes::Rectangle();
                Rectangle->Width = 60;
                Rectangle->Height = 60;
                Rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                canvas->Children->Append(Rectangle);
                rectLoadedRegistration.Attach(Rectangle, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                }));

                auto border = ref new xaml_controls::Border();
                this->Border = border;  // See if we can fix this after we get all the code refactored.  To do so we need to remove controls from the using namespaces.
                border->Width = 200;
                border->Height = 60;
                border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Navy);
                border->SetValue(Canvas::LeftProperty, 100.0f);
                border->SetValue(Canvas::TopProperty, 0.0f);
                canvas->Children->Append(border);
                borderLoadedRegistration.Attach(border, ref new RoutedEventHandler([borderLoadedEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Border Loaded event");
                    borderLoadedEvent->Set();
                }));
                TestServices::WindowHelper->WindowContent = mainGrid;

            });
            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            borderLoadedEvent->WaitForDefault();
        }

        DropTargetHelper::DropTargetHelper(UIElement^ target)
        {
            m_target = target;
            RunOnUIThread([&]()
            {
                m_originalAllowDrop = target->AllowDrop;
                target->AllowDrop = true;
                LOG_OUTPUT(L"Registering for events");
                DragEnterRegistration.Attach(target, ref new DragEventHandler([this](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"DragEnter event");
                    VERIFY_IS_FALSE(DragEnterEvent->HasFired());
                    args->AcceptedOperation = DataPackageOperation::Copy;
                    DragEnterEvent->Set();
                }));
                DragLeaveRegistration.Attach(target, ref new DragEventHandler([this](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"DragLeave event");
                    VERIFY_IS_FALSE(DragLeaveEvent->HasFired());
                    VERIFY_IS_FALSE(DropEvent->HasFired());
                    DragLeaveEvent->Set();
                }));
                DragOverRegistration.Attach(target, ref new DragEventHandler([this](Platform::Object^ sender, DragEventArgs^ args)
                    {
                        VERIFY_IS_TRUE(DragEnterEvent->HasFired());
                        VERIFY_IS_FALSE(DragLeaveEvent->HasFired());
                        VERIFY_IS_FALSE(DropEvent->HasFired());
                        args->AcceptedOperation = DataPackageOperation::Copy;
                    }));
                DropRegistration.Attach(target, ref new DragEventHandler([this](Platform::Object^ sender, DragEventArgs^ args)
                {
                    LOG_OUTPUT(L"Border Drop event");
                    VERIFY_IS_FALSE(DropEvent->HasFired());
                    VERIFY_IS_FALSE(DragLeaveEvent->HasFired());
                    args->AcceptedOperation = DataPackageOperation::Copy;
                    DropEvent->Set();
                }));
            });
        }
        DropTargetHelper::~DropTargetHelper()
        {
            RunOnUIThread([&]()
            {
                m_target->AllowDrop = m_originalAllowDrop;
            });
        }
        void DropTargetHelper::Reset()
        {
            DropEvent->Reset();
            DragLeaveEvent->Reset();
            DragEnterEvent->Reset();
        }

        DragSourceHelper::DragSourceHelper(UIElement^ source, bool manualStart)
        {
            m_source = source;
            RunOnUIThread([&]()
            {
                m_originalCanDrag = source->CanDrag;
                DragStartingRegistration.Attach(source, ref new wf::TypedEventHandler<UIElement^, DragStartingEventArgs^>([&](UIElement^ sender, DragStartingEventArgs^ args)
                    {
                        LOG_OUTPUT(L"DragStarting event");
                        VERIFY_IS_FALSE(DragStartingEvent->HasFired());
                        DragStartingEvent->Set();
                    }));

                if (!manualStart)
                {
                    source->CanDrag = true;
                }
                else
                {
                    // If we want a manual start (rather than waiting for Xaml to automatically start on mouse move), then handle pointer pressed and manually start it.
                    PointerPressedRegistration.Attach(source, ref new PointerEventHandler([&](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                    {
                        CoreDragOperation^ drag = ref new CoreDragOperation();
                        drag->SetPointerId(args->Pointer->PointerId);
                        drag->Data->SetText(ref new Platform::String(L"This is a test"));
                        drag->Data->RequestedOperation = DataPackageOperation::Copy | DataPackageOperation::Move | DataPackageOperation::Link;
                        //concurrency::create_task(drag->StartAsync()).then(
                        //    [&](DataPackageOperation)
                        //{
                        //});

                        {
                            void* result = nullptr;
                            const auto hmod = ::GetModuleHandleA("microsoft.inputstatemanager.dll");
                            if (hmod == nullptr)
                            {
                                return;
                            }

                            using LiftedDragStartAsync = HRESULT(*)(_In_ CoreDragOperation^, //IUnknown*,
                                _In_opt_ IInspectable*,
                                _In_ Point,
                                _Outptr_ void**);
                            const auto StartAsyncDrag = reinterpret_cast<LiftedDragStartAsync>(::GetProcAddress(hmod, "LiftedDragStartAsync"));
                            if (StartAsyncDrag)
                            {
                                StartAsyncDrag(drag, nullptr, {}, &result);

                                // We don't get a drag starting event when starting via the api, so simulate it
                                DragStartingEvent->Set();
                            }
                        }
                    }));
                }
            });
        }
        DragSourceHelper::~DragSourceHelper()
        {
            RunOnUIThread([&]()
                {
                    m_source->CanDrag = m_originalCanDrag;
                });

        }

    } } }
} } } }

