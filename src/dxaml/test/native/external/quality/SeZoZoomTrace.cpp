// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SeZoZoomTrace.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <TraceConsumerSession.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Platform;
using namespace Concurrency;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Quality {
                    bool SeZoZoomTrace::ClassSetup()
                    {
                        CommonTestSetupHelper::CommonTestClassSetup();

                        return true;
                    }

                    bool SeZoZoomTrace::TestCleanup()
                    {
                        TestServices::WindowHelper->VerifyTestCleanup();

                        return true;
                    }

                    void SeZoZoomTrace::VerifyZoomingTraceWheel()
                    {
                        TestCleanupWrapper cleanup;
                        Grid^ grid = nullptr;

                        SemanticZoom^ seZo = nullptr;
                        GridView^ innerView = nullptr;
                        GridView^ outerView = nullptr;

                        GridViewItem^ innerItem = nullptr;
                        GridViewItem^ outerItem = nullptr;

                        ScrollViewer^ seZoScrollViewer = nullptr;

                        auto viewChangeCompletedEvent = std::make_shared<Event>();
                        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

                        auto viewChangedEvent = std::make_shared<Event>();
                        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

                        SetUpPage(grid, seZo, innerView, outerView, innerItem, outerItem);

                        RunOnUIThread([&]()
                        {
                            seZoScrollViewer = TreeHelper::GetVisualChildByType<ScrollViewer>(seZo);
                            seZoScrollViewer->ZoomMode = ZoomMode::Enabled;

                            viewChangeCompletedRegistration.Attach(seZo, ref new xaml_controls::SemanticZoomViewChangedEventHandler([viewChangeCompletedEvent](Platform::Object^ sender, SemanticZoomViewChangedEventArgs^ e)
                            {
                                LOG_OUTPUT(L"View Change Completed");
                                viewChangeCompletedEvent->Set();
                            }));

                            viewChangedRegistration.Attach(seZoScrollViewer, ref new ::Windows::Foundation::EventHandler<ScrollViewerViewChangedEventArgs^>([viewChangedEvent](Platform::Object^ sender, ScrollViewerViewChangedEventArgs^ e)
                            {
                                if (!e->IsIntermediate)
                                {
                                    LOG_OUTPUT(L"Scroll Viewer View Change Completed");

                                    viewChangedEvent->Set();
                                }
                            }));
                        });

                        {
                            TraceConsumerSession traceSession;

                            LOG_OUTPUT(L"Zooming out with scroll wheel");
                            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl");
                            TestServices::WindowHelper->WaitForIdle();
                            TestServices::InputHelper->ScrollMouseWheel(seZo, -10);
                            TestServices::WindowHelper->WaitForIdle();
                            TestServices::KeyboardHelper->PressKeySequence(L"$u$_ctrl");

                            viewChangeCompletedEvent->WaitFor(std::chrono::seconds(10));
                            viewChangedEvent->WaitFor(std::chrono::seconds(10));
                            TestServices::WindowHelper->WaitForIdle();
                        }

                        TraceConsumer::VerifyEventTraced("SeZoZoom", 1);

                        viewChangeCompletedEvent->Reset();
                        viewChangedEvent->Reset();

                        {
                            TraceConsumerSession traceSession;

                            LOG_OUTPUT(L"Zooming in with scroll wheel");
                            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl");
                            TestServices::WindowHelper->WaitForIdle();
                            TestServices::InputHelper->ScrollMouseWheel(seZo, 10);
                            TestServices::WindowHelper->WaitForIdle();
                            TestServices::KeyboardHelper->PressKeySequence(L"$u$_ctrl");

                            viewChangeCompletedEvent->WaitFor(std::chrono::seconds(10));
                            viewChangedEvent->WaitFor(std::chrono::seconds(10));
                            TestServices::WindowHelper->WaitForIdle();
                        }

                        TraceConsumer::VerifyEventTraced("SeZoZoom", 1);
                    }

                    void SeZoZoomTrace::VerifyZoomingTracePinch()
                    {
                        TestCleanupWrapper cleanup;
                        Grid^ grid = nullptr;

                        SemanticZoom^ seZo = nullptr;
                        GridView^ innerView = nullptr;
                        GridView^ outerView = nullptr;

                        GridViewItem^ innerItem = nullptr;
                        GridViewItem^ outerItem = nullptr;

                        ScrollViewer^ seZoScrollViewer = nullptr;

                        auto viewChangeCompletedEvent = std::make_shared<Event>();
                        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

                        auto viewChangedEvent = std::make_shared<Event>();
                        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

                        SetUpPage(grid, seZo, innerView, outerView, innerItem, outerItem);

                        RunOnUIThread([&]()
                        {
                            seZoScrollViewer = TreeHelper::GetVisualChildByType<ScrollViewer>(seZo);

                            viewChangeCompletedRegistration.Attach(seZo, ref new xaml_controls::SemanticZoomViewChangedEventHandler([viewChangeCompletedEvent](Platform::Object^ sender, SemanticZoomViewChangedEventArgs^ e)
                            {
                                LOG_OUTPUT(L"View Change Completed");
                                viewChangeCompletedEvent->Set();
                            }));

                            viewChangedRegistration.Attach(seZoScrollViewer, ref new ::Windows::Foundation::EventHandler<ScrollViewerViewChangedEventArgs^>([viewChangedEvent](Platform::Object^ sender, ScrollViewerViewChangedEventArgs^ e)
                            {
                                if (!e->IsIntermediate)
                                {
                                    LOG_OUTPUT(L"Scroll Viewer View Change Completed");

                                    viewChangedEvent->Set();
                                }
                            }));
                        });

                        {
                            TraceConsumerSession traceSession;

                            LOG_OUTPUT(L"Zooming out with pinch");

                            //zoom by pinch
                            TestServices::InputHelper->ZoomOutFromEdges(seZo, 500, Orientation::Horizontal, 1);

                            viewChangeCompletedEvent->WaitFor(std::chrono::seconds(10));
                            viewChangedEvent->WaitFor(std::chrono::seconds(10));
                            TestServices::WindowHelper->WaitForIdle();
                        }

                        TraceConsumer::VerifyEventTraced("SeZoZoom", 1);

                        viewChangeCompletedEvent->Reset();
                        viewChangedEvent->Reset();

                        {
                            TraceConsumerSession traceSession;

                            LOG_OUTPUT(L"Zooming in with pinch");

                            //zoom by pinch
                            TestServices::InputHelper->ZoomInToEdges(seZo, 500, Orientation::Horizontal, 1);

                            viewChangeCompletedEvent->WaitFor(std::chrono::seconds(10));
                            viewChangedEvent->WaitFor(std::chrono::seconds(10));
                            TestServices::WindowHelper->WaitForIdle();
                        }

                        TraceConsumer::VerifyEventTraced("SeZoZoom", 1);
                    }

                    void SeZoZoomTrace::VerifyZoomingTraceTap()
                    {
                        TestCleanupWrapper cleanup;
                        Grid^ grid = nullptr;

                        SemanticZoom^ seZo = nullptr;
                        GridView^ innerView = nullptr;
                        GridView^ outerView = nullptr;

                        GridViewItem^ innerItem = nullptr;
                        GridViewItem^ outerItem = nullptr;

                        ScrollViewer^ seZoScrollViewer = nullptr;

                        auto viewChangeCompletedEvent = std::make_shared<Event>();
                        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

                        auto viewChangedEvent = std::make_shared<Event>();
                        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

                        SetUpPage(grid, seZo, innerView, outerView, innerItem, outerItem);

                        RunOnUIThread([&]()
                        {
                            seZoScrollViewer = TreeHelper::GetVisualChildByType<ScrollViewer>(seZo);

                            viewChangeCompletedRegistration.Attach(seZo, ref new xaml_controls::SemanticZoomViewChangedEventHandler([viewChangeCompletedEvent](Platform::Object^ sender, SemanticZoomViewChangedEventArgs^ e)
                            {
                                LOG_OUTPUT(L"View Change Completed");
                                viewChangeCompletedEvent->Set();
                            }));

                            viewChangedRegistration.Attach(seZoScrollViewer, ref new ::Windows::Foundation::EventHandler<ScrollViewerViewChangedEventArgs^>([viewChangedEvent](Platform::Object^ sender, ScrollViewerViewChangedEventArgs^ e)
                            {
                                if (!e->IsIntermediate)
                                {
                                    LOG_OUTPUT(L"Scroll Viewer View Change Completed");

                                    viewChangedEvent->Set();
                                }
                            }));
                        });

                        {
                            TraceConsumerSession traceSession;

                            RunOnUIThread([&]()
                            {
                                //same codepath as tap
                                seZo->ToggleActiveView();
                            });

                            viewChangeCompletedEvent->WaitFor(std::chrono::seconds(10));
                            viewChangedEvent->WaitFor(std::chrono::seconds(10));
                            TestServices::WindowHelper->WaitForIdle();
                        }

                        TraceConsumer::VerifyEventTraced("SeZoZoom", 1);

                        viewChangeCompletedEvent->Reset();
                        viewChangedEvent->Reset();

                        {
                            TraceConsumerSession traceSession;

                            LOG_OUTPUT(L"Zooming in with tap");

                            //zoom by pinch
                            TestServices::InputHelper->Tap(outerItem);

                            viewChangeCompletedEvent->WaitFor(std::chrono::seconds(10));
                            viewChangedEvent->WaitFor(std::chrono::seconds(10));
                            TestServices::WindowHelper->WaitForIdle();
                        }

                        TraceConsumer::VerifyEventTraced("SeZoZoom", 1);
                    }

                    void SeZoZoomTrace::SetUpPage(Grid^ &grid, SemanticZoom^ &seZo, GridView^ &innerView, GridView^ &outerView, GridViewItem^ &innerItem, GridViewItem^ &outerItem)
                    {
                        RunOnUIThread([&]()
                        {
                            LOG_OUTPUT(L"Initializing elements");

                            grid = ref new Grid();
                            Window::Current->Content = grid;

                            seZo = ref new SemanticZoom();
                            grid->Children->Append(seZo);

                            //the gridview is defined via Xaml so the scrollchaining can be turned off easily
                            Platform::String^ gridViewXaml =
                                L"<GridView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                                L"          ScrollViewer.IsHorizontalScrollChainingEnabled='False' ScrollViewer.IsVerticalScrollChainingEnabled='False'>"
                                L"</GridView>";

                            innerView = dynamic_cast<xaml_controls::GridView^> (xaml_markup::XamlReader::Load(gridViewXaml));
                            seZo->ZoomedInView = innerView;

                            outerView = dynamic_cast<xaml_controls::GridView^> (xaml_markup::XamlReader::Load(gridViewXaml));
                            seZo->ZoomedOutView = outerView;

                            innerItem = ref new GridViewItem();
                            innerItem->Content = "Inner";
                            innerItem->Width = 300;
                            innerItem->Height = 300;
                            innerView->Items->Append(innerItem);

                            outerItem = ref new GridViewItem();
                            outerItem->Content = "Outer";
                            outerItem->Width = 300;
                            outerItem->Height = 300;
                            outerView->Items->Append(outerItem);
                        });

                        TestServices::WindowHelper->WaitForIdle();
                    }
                }
            }
        }
    }
}