// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

namespace Microsoft.UI.Xaml.Tests.Common.EventsListeners
{
    public static class ItemCollectionEvents
    {
        private static EventListener<ItemCollection> _VectorChanged;
        public static EventListener<ItemCollection> VectorChanged
        {
            get
            {
                if(_VectorChanged == null)
                {
                    _VectorChanged = new EventListener<ItemCollection>(
                        (sender, action) =>
                        {
                            VectorChangedEventHandler<Object> handler = delegate { action(); };
                            sender.VectorChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.VectorChanged -= (VectorChangedEventHandler<Object>) handler;
                        }
                    );
                }
                return _VectorChanged;
            }
        }

    }

    public static class ItemContainerGeneratorEvents
    {
        private static EventListener<ItemContainerGenerator> _ItemsChanged;
        public static EventListener<ItemContainerGenerator> ItemsChanged
        {
            get
            {
                if(_ItemsChanged == null)
                {
                    _ItemsChanged = new EventListener<ItemContainerGenerator>(
                        (sender, action) =>
                        {
                            ItemsChangedEventHandler handler = delegate { action(); };
                            sender.ItemsChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ItemsChanged -= (ItemsChangedEventHandler) handler;
                        }
                    );
                }
                return _ItemsChanged;
            }
        }

    }

    public static class FlyoutBaseEvents
    {
        private static EventListener<FlyoutBase> _Closed;
        public static EventListener<FlyoutBase> Closed
        {
            get
            {
                if(_Closed == null)
                {
                    _Closed = new EventListener<FlyoutBase>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.Closed += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Closed -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _Closed;
            }
        }

        private static EventListener<FlyoutBase> _Opened;
        public static EventListener<FlyoutBase> Opened
        {
            get
            {
                if(_Opened == null)
                {
                    _Opened = new EventListener<FlyoutBase>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.Opened += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Opened -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _Opened;
            }
        }

        private static EventListener<FlyoutBase> _Opening;
        public static EventListener<FlyoutBase> Opening
        {
            get
            {
                if(_Opening == null)
                {
                    _Opening = new EventListener<FlyoutBase>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.Opening += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Opening -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _Opening;
            }
        }

    }

    public static class DebugSettingsEvents
    {
        private static EventListener<DebugSettings> _BindingFailed;
        public static EventListener<DebugSettings> BindingFailed
        {
            get
            {
                if(_BindingFailed == null)
                {
                    _BindingFailed = new EventListener<DebugSettings>(
                        (sender, action) =>
                        {
                            BindingFailedEventHandler handler = delegate { action(); };
                            sender.BindingFailed += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.BindingFailed -= (BindingFailedEventHandler) handler;
                        }
                    );
                }
                return _BindingFailed;
            }
        }

    }

    public static class DependencyObjectCollectionEvents
    {
        private static EventListener<DependencyObjectCollection> _VectorChanged;
        public static EventListener<DependencyObjectCollection> VectorChanged
        {
            get
            {
                if(_VectorChanged == null)
                {
                    _VectorChanged = new EventListener<DependencyObjectCollection>(
                        (sender, action) =>
                        {
                            VectorChangedEventHandler<DependencyObject> handler = delegate { action(); };
                            sender.VectorChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.VectorChanged -= (VectorChangedEventHandler<DependencyObject>) handler;
                        }
                    );
                }
                return _VectorChanged;
            }
        }

    }

    public static class DispatcherTimerEvents
    {
        private static EventListener<DispatcherTimer> _Tick;
        public static EventListener<DispatcherTimer> Tick
        {
            get
            {
                if(_Tick == null)
                {
                    _Tick = new EventListener<DispatcherTimer>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.Tick += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Tick -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _Tick;
            }
        }

    }

    public static class ApplicationEvents
    {
        private static EventListener<Application> _Resuming;
        public static EventListener<Application> Resuming
        {
            get
            {
                if(_Resuming == null)
                {
                    _Resuming = new EventListener<Application>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.Resuming += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Resuming -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _Resuming;
            }
        }

        private static EventListener<Application> _Suspending;
        public static EventListener<Application> Suspending
        {
            get
            {
                if(_Suspending == null)
                {
                    _Suspending = new EventListener<Application>(
                        (sender, action) =>
                        {
                            SuspendingEventHandler handler = delegate { action(); };
                            sender.Suspending += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Suspending -= (SuspendingEventHandler) handler;
                        }
                    );
                }
                return _Suspending;
            }
        }

        private static EventListener<Application> _UnhandledException;
        public static EventListener<Application> UnhandledException
        {
            get
            {
                if(_UnhandledException == null)
                {
                    _UnhandledException = new EventListener<Application>(
                        (sender, action) =>
                        {
                            UnhandledExceptionEventHandler handler = delegate { action(); };
                            sender.UnhandledException += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.UnhandledException -= (UnhandledExceptionEventHandler) handler;
                        }
                    );
                }
                return _UnhandledException;
            }
        }

    }

    public static class UIElementEvents
    {
        private static EventListener<UIElement> _DoubleTapped;
        public static EventListener<UIElement> DoubleTapped
        {
            get
            {
                if(_DoubleTapped == null)
                {
                    _DoubleTapped = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            DoubleTappedEventHandler handler = delegate { action(); };
                            sender.DoubleTapped += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DoubleTapped -= (DoubleTappedEventHandler) handler;
                        }
                    );
                }
                return _DoubleTapped;
            }
        }

        private static EventListener<UIElement> _DragEnter;
        public static EventListener<UIElement> DragEnter
        {
            get
            {
                if(_DragEnter == null)
                {
                    _DragEnter = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            DragEventHandler handler = delegate { action(); };
                            sender.DragEnter += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DragEnter -= (DragEventHandler) handler;
                        }
                    );
                }
                return _DragEnter;
            }
        }

        private static EventListener<UIElement> _DragLeave;
        public static EventListener<UIElement> DragLeave
        {
            get
            {
                if(_DragLeave == null)
                {
                    _DragLeave = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            DragEventHandler handler = delegate { action(); };
                            sender.DragLeave += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DragLeave -= (DragEventHandler) handler;
                        }
                    );
                }
                return _DragLeave;
            }
        }

        private static EventListener<UIElement> _DragOver;
        public static EventListener<UIElement> DragOver
        {
            get
            {
                if(_DragOver == null)
                {
                    _DragOver = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            DragEventHandler handler = delegate { action(); };
                            sender.DragOver += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DragOver -= (DragEventHandler) handler;
                        }
                    );
                }
                return _DragOver;
            }
        }

        private static EventListener<UIElement> _Drop;
        public static EventListener<UIElement> Drop
        {
            get
            {
                if(_Drop == null)
                {
                    _Drop = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            DragEventHandler handler = delegate { action(); };
                            sender.Drop += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Drop -= (DragEventHandler) handler;
                        }
                    );
                }
                return _Drop;
            }
        }

        private static EventListener<UIElement> _GotFocus;
        public static EventListener<UIElement> GotFocus
        {
            get
            {
                if(_GotFocus == null)
                {
                    _GotFocus = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.GotFocus += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.GotFocus -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _GotFocus;
            }
        }

        private static EventListener<UIElement> _Holding;
        public static EventListener<UIElement> Holding
        {
            get
            {
                if(_Holding == null)
                {
                    _Holding = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            HoldingEventHandler handler = delegate { action(); };
                            sender.Holding += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Holding -= (HoldingEventHandler) handler;
                        }
                    );
                }
                return _Holding;
            }
        }

        private static EventListener<UIElement> _KeyDown;
        public static EventListener<UIElement> KeyDown
        {
            get
            {
                if(_KeyDown == null)
                {
                    _KeyDown = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            KeyEventHandler handler = delegate { action(); };
                            sender.KeyDown += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.KeyDown -= (KeyEventHandler) handler;
                        }
                    );
                }
                return _KeyDown;
            }
        }

        private static EventListener<UIElement> _KeyUp;
        public static EventListener<UIElement> KeyUp
        {
            get
            {
                if(_KeyUp == null)
                {
                    _KeyUp = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            KeyEventHandler handler = delegate { action(); };
                            sender.KeyUp += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.KeyUp -= (KeyEventHandler) handler;
                        }
                    );
                }
                return _KeyUp;
            }
        }

        private static EventListener<UIElement> _LostFocus;
        public static EventListener<UIElement> LostFocus
        {
            get
            {
                if(_LostFocus == null)
                {
                    _LostFocus = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.LostFocus += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.LostFocus -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _LostFocus;
            }
        }

        private static EventListener<UIElement> _ManipulationCompleted;
        public static EventListener<UIElement> ManipulationCompleted
        {
            get
            {
                if(_ManipulationCompleted == null)
                {
                    _ManipulationCompleted = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            ManipulationCompletedEventHandler handler = delegate { action(); };
                            sender.ManipulationCompleted += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ManipulationCompleted -= (ManipulationCompletedEventHandler) handler;
                        }
                    );
                }
                return _ManipulationCompleted;
            }
        }

        private static EventListener<UIElement> _ManipulationDelta;
        public static EventListener<UIElement> ManipulationDelta
        {
            get
            {
                if(_ManipulationDelta == null)
                {
                    _ManipulationDelta = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            ManipulationDeltaEventHandler handler = delegate { action(); };
                            sender.ManipulationDelta += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ManipulationDelta -= (ManipulationDeltaEventHandler) handler;
                        }
                    );
                }
                return _ManipulationDelta;
            }
        }

        private static EventListener<UIElement> _ManipulationInertiaStarting;
        public static EventListener<UIElement> ManipulationInertiaStarting
        {
            get
            {
                if(_ManipulationInertiaStarting == null)
                {
                    _ManipulationInertiaStarting = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            ManipulationInertiaStartingEventHandler handler = delegate { action(); };
                            sender.ManipulationInertiaStarting += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ManipulationInertiaStarting -= (ManipulationInertiaStartingEventHandler) handler;
                        }
                    );
                }
                return _ManipulationInertiaStarting;
            }
        }

        private static EventListener<UIElement> _ManipulationStarted;
        public static EventListener<UIElement> ManipulationStarted
        {
            get
            {
                if(_ManipulationStarted == null)
                {
                    _ManipulationStarted = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            ManipulationStartedEventHandler handler = delegate { action(); };
                            sender.ManipulationStarted += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ManipulationStarted -= (ManipulationStartedEventHandler) handler;
                        }
                    );
                }
                return _ManipulationStarted;
            }
        }

        private static EventListener<UIElement> _ManipulationStarting;
        public static EventListener<UIElement> ManipulationStarting
        {
            get
            {
                if(_ManipulationStarting == null)
                {
                    _ManipulationStarting = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            ManipulationStartingEventHandler handler = delegate { action(); };
                            sender.ManipulationStarting += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ManipulationStarting -= (ManipulationStartingEventHandler) handler;
                        }
                    );
                }
                return _ManipulationStarting;
            }
        }

        private static EventListener<UIElement> _PointerCanceled;
        public static EventListener<UIElement> PointerCanceled
        {
            get
            {
                if(_PointerCanceled == null)
                {
                    _PointerCanceled = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            PointerEventHandler handler = delegate { action(); };
                            sender.PointerCanceled += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.PointerCanceled -= (PointerEventHandler) handler;
                        }
                    );
                }
                return _PointerCanceled;
            }
        }

        private static EventListener<UIElement> _PointerCaptureLost;
        public static EventListener<UIElement> PointerCaptureLost
        {
            get
            {
                if(_PointerCaptureLost == null)
                {
                    _PointerCaptureLost = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            PointerEventHandler handler = delegate { action(); };
                            sender.PointerCaptureLost += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.PointerCaptureLost -= (PointerEventHandler) handler;
                        }
                    );
                }
                return _PointerCaptureLost;
            }
        }

        private static EventListener<UIElement> _PointerEntered;
        public static EventListener<UIElement> PointerEntered
        {
            get
            {
                if(_PointerEntered == null)
                {
                    _PointerEntered = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            PointerEventHandler handler = delegate { action(); };
                            sender.PointerEntered += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.PointerEntered -= (PointerEventHandler) handler;
                        }
                    );
                }
                return _PointerEntered;
            }
        }

        private static EventListener<UIElement> _PointerExited;
        public static EventListener<UIElement> PointerExited
        {
            get
            {
                if(_PointerExited == null)
                {
                    _PointerExited = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            PointerEventHandler handler = delegate { action(); };
                            sender.PointerExited += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.PointerExited -= (PointerEventHandler) handler;
                        }
                    );
                }
                return _PointerExited;
            }
        }

        private static EventListener<UIElement> _PointerMoved;
        public static EventListener<UIElement> PointerMoved
        {
            get
            {
                if(_PointerMoved == null)
                {
                    _PointerMoved = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            PointerEventHandler handler = delegate { action(); };
                            sender.PointerMoved += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.PointerMoved -= (PointerEventHandler) handler;
                        }
                    );
                }
                return _PointerMoved;
            }
        }

        private static EventListener<UIElement> _PointerPressed;
        public static EventListener<UIElement> PointerPressed
        {
            get
            {
                if(_PointerPressed == null)
                {
                    _PointerPressed = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            PointerEventHandler handler = delegate { action(); };
                            sender.PointerPressed += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.PointerPressed -= (PointerEventHandler) handler;
                        }
                    );
                }
                return _PointerPressed;
            }
        }

        private static EventListener<UIElement> _PointerReleased;
        public static EventListener<UIElement> PointerReleased
        {
            get
            {
                if(_PointerReleased == null)
                {
                    _PointerReleased = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            PointerEventHandler handler = delegate { action(); };
                            sender.PointerReleased += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.PointerReleased -= (PointerEventHandler) handler;
                        }
                    );
                }
                return _PointerReleased;
            }
        }

        private static EventListener<UIElement> _PointerWheelChanged;
        public static EventListener<UIElement> PointerWheelChanged
        {
            get
            {
                if(_PointerWheelChanged == null)
                {
                    _PointerWheelChanged = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            PointerEventHandler handler = delegate { action(); };
                            sender.PointerWheelChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.PointerWheelChanged -= (PointerEventHandler) handler;
                        }
                    );
                }
                return _PointerWheelChanged;
            }
        }

        private static EventListener<UIElement> _RightTapped;
        public static EventListener<UIElement> RightTapped
        {
            get
            {
                if(_RightTapped == null)
                {
                    _RightTapped = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            RightTappedEventHandler handler = delegate { action(); };
                            sender.RightTapped += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.RightTapped -= (RightTappedEventHandler) handler;
                        }
                    );
                }
                return _RightTapped;
            }
        }

        private static EventListener<UIElement> _Tapped;
        public static EventListener<UIElement> Tapped
        {
            get
            {
                if(_Tapped == null)
                {
                    _Tapped = new EventListener<UIElement>(
                        (sender, action) =>
                        {
                            TappedEventHandler handler = delegate { action(); };
                            sender.Tapped += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Tapped -= (TappedEventHandler) handler;
                        }
                    );
                }
                return _Tapped;
            }
        }

    }

    public static class FrameworkElementEvents
    {
        private static EventListener<FrameworkElement> _LayoutUpdated;
        public static EventListener<FrameworkElement> LayoutUpdated
        {
            get
            {
                if(_LayoutUpdated == null)
                {
                    _LayoutUpdated = new EventListener<FrameworkElement>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.LayoutUpdated += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.LayoutUpdated -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _LayoutUpdated;
            }
        }

        private static EventListener<FrameworkElement> _Loaded;
        public static EventListener<FrameworkElement> Loaded
        {
            get
            {
                if(_Loaded == null)
                {
                    _Loaded = new EventListener<FrameworkElement>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.Loaded += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Loaded -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _Loaded;
            }
        }

        private static EventListener<FrameworkElement> _SizeChanged;
        public static EventListener<FrameworkElement> SizeChanged
        {
            get
            {
                if(_SizeChanged == null)
                {
                    _SizeChanged = new EventListener<FrameworkElement>(
                        (sender, action) =>
                        {
                            SizeChangedEventHandler handler = delegate { action(); };
                            sender.SizeChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.SizeChanged -= (SizeChangedEventHandler) handler;
                        }
                    );
                }
                return _SizeChanged;
            }
        }

        private static EventListener<FrameworkElement> _Unloaded;
        public static EventListener<FrameworkElement> Unloaded
        {
            get
            {
                if(_Unloaded == null)
                {
                    _Unloaded = new EventListener<FrameworkElement>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.Unloaded += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Unloaded -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _Unloaded;
            }
        }

        private static EventListener<FrameworkElement> _DataContextChanged;
        public static EventListener<FrameworkElement> DataContextChanged
        {
            get
            {
                if(_DataContextChanged == null)
                {
                    _DataContextChanged = new EventListener<FrameworkElement>(
                        (sender, action) =>
                        {
                            TypedEventHandler<FrameworkElement, DataContextChangedEventArgs> handler = delegate { action(); };
                            sender.DataContextChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DataContextChanged -= (TypedEventHandler<FrameworkElement, DataContextChangedEventArgs>) handler;
                        }
                    );
                }
                return _DataContextChanged;
            }
        }

    }

    public static class ImageEvents
    {
        private static EventListener<Image> _ImageFailed;
        public static EventListener<Image> ImageFailed
        {
            get
            {
                if(_ImageFailed == null)
                {
                    _ImageFailed = new EventListener<Image>(
                        (sender, action) =>
                        {
                            ExceptionRoutedEventHandler handler = delegate { action(); };
                            sender.ImageFailed += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ImageFailed -= (ExceptionRoutedEventHandler) handler;
                        }
                    );
                }
                return _ImageFailed;
            }
        }

        private static EventListener<Image> _ImageOpened;
        public static EventListener<Image> ImageOpened
        {
            get
            {
                if(_ImageOpened == null)
                {
                    _ImageOpened = new EventListener<Image>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.ImageOpened += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ImageOpened -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _ImageOpened;
            }
        }

    }

    public static class ItemsPresenterEvents
    {
        private static EventListener<ItemsPresenter> _HorizontalSnapPointsChanged;
        public static EventListener<ItemsPresenter> HorizontalSnapPointsChanged
        {
            get
            {
                if(_HorizontalSnapPointsChanged == null)
                {
                    _HorizontalSnapPointsChanged = new EventListener<ItemsPresenter>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.HorizontalSnapPointsChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.HorizontalSnapPointsChanged -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _HorizontalSnapPointsChanged;
            }
        }

        private static EventListener<ItemsPresenter> _VerticalSnapPointsChanged;
        public static EventListener<ItemsPresenter> VerticalSnapPointsChanged
        {
            get
            {
                if(_VerticalSnapPointsChanged == null)
                {
                    _VerticalSnapPointsChanged = new EventListener<ItemsPresenter>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.VerticalSnapPointsChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.VerticalSnapPointsChanged -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _VerticalSnapPointsChanged;
            }
        }

    }

    public static class RichTextBlockEvents
    {
        private static EventListener<RichTextBlock> _ContextMenuOpening;
        public static EventListener<RichTextBlock> ContextMenuOpening
        {
            get
            {
                if(_ContextMenuOpening == null)
                {
                    _ContextMenuOpening = new EventListener<RichTextBlock>(
                        (sender, action) =>
                        {
                            ContextMenuOpeningEventHandler handler = delegate { action(); };
                            sender.ContextMenuOpening += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ContextMenuOpening -= (ContextMenuOpeningEventHandler) handler;
                        }
                    );
                }
                return _ContextMenuOpening;
            }
        }

        private static EventListener<RichTextBlock> _SelectionChanged;
        public static EventListener<RichTextBlock> SelectionChanged
        {
            get
            {
                if(_SelectionChanged == null)
                {
                    _SelectionChanged = new EventListener<RichTextBlock>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.SelectionChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.SelectionChanged -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _SelectionChanged;
            }
        }

    }

    public static class StackPanelEvents
    {
        private static EventListener<StackPanel> _HorizontalSnapPointsChanged;
        public static EventListener<StackPanel> HorizontalSnapPointsChanged
        {
            get
            {
                if(_HorizontalSnapPointsChanged == null)
                {
                    _HorizontalSnapPointsChanged = new EventListener<StackPanel>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.HorizontalSnapPointsChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.HorizontalSnapPointsChanged -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _HorizontalSnapPointsChanged;
            }
        }

        private static EventListener<StackPanel> _VerticalSnapPointsChanged;
        public static EventListener<StackPanel> VerticalSnapPointsChanged
        {
            get
            {
                if(_VerticalSnapPointsChanged == null)
                {
                    _VerticalSnapPointsChanged = new EventListener<StackPanel>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.VerticalSnapPointsChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.VerticalSnapPointsChanged -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _VerticalSnapPointsChanged;
            }
        }

    }

    public static class TextBlockEvents
    {
        private static EventListener<TextBlock> _ContextMenuOpening;
        public static EventListener<TextBlock> ContextMenuOpening
        {
            get
            {
                if(_ContextMenuOpening == null)
                {
                    _ContextMenuOpening = new EventListener<TextBlock>(
                        (sender, action) =>
                        {
                            ContextMenuOpeningEventHandler handler = delegate { action(); };
                            sender.ContextMenuOpening += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ContextMenuOpening -= (ContextMenuOpeningEventHandler) handler;
                        }
                    );
                }
                return _ContextMenuOpening;
            }
        }

        private static EventListener<TextBlock> _SelectionChanged;
        public static EventListener<TextBlock> SelectionChanged
        {
            get
            {
                if(_SelectionChanged == null)
                {
                    _SelectionChanged = new EventListener<TextBlock>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.SelectionChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.SelectionChanged -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _SelectionChanged;
            }
        }

    }

    public static class CarouselPanelEvents
    {
        private static EventListener<CarouselPanel> _HorizontalSnapPointsChanged;
        public static EventListener<CarouselPanel> HorizontalSnapPointsChanged
        {
            get
            {
                if(_HorizontalSnapPointsChanged == null)
                {
                    _HorizontalSnapPointsChanged = new EventListener<CarouselPanel>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.HorizontalSnapPointsChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.HorizontalSnapPointsChanged -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _HorizontalSnapPointsChanged;
            }
        }

        private static EventListener<CarouselPanel> _VerticalSnapPointsChanged;
        public static EventListener<CarouselPanel> VerticalSnapPointsChanged
        {
            get
            {
                if(_VerticalSnapPointsChanged == null)
                {
                    _VerticalSnapPointsChanged = new EventListener<CarouselPanel>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.VerticalSnapPointsChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.VerticalSnapPointsChanged -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _VerticalSnapPointsChanged;
            }
        }

    }

    public static class OrientedVirtualizingPanelEvents
    {
        private static EventListener<OrientedVirtualizingPanel> _HorizontalSnapPointsChanged;
        public static EventListener<OrientedVirtualizingPanel> HorizontalSnapPointsChanged
        {
            get
            {
                if(_HorizontalSnapPointsChanged == null)
                {
                    _HorizontalSnapPointsChanged = new EventListener<OrientedVirtualizingPanel>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.HorizontalSnapPointsChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.HorizontalSnapPointsChanged -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _HorizontalSnapPointsChanged;
            }
        }

        private static EventListener<OrientedVirtualizingPanel> _VerticalSnapPointsChanged;
        public static EventListener<OrientedVirtualizingPanel> VerticalSnapPointsChanged
        {
            get
            {
                if(_VerticalSnapPointsChanged == null)
                {
                    _VerticalSnapPointsChanged = new EventListener<OrientedVirtualizingPanel>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.VerticalSnapPointsChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.VerticalSnapPointsChanged -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _VerticalSnapPointsChanged;
            }
        }

    }

    public static class VirtualizingStackPanelEvents
    {
        private static EventListener<VirtualizingStackPanel> _CleanUpVirtualizedItemEvent;
        public static EventListener<VirtualizingStackPanel> CleanUpVirtualizedItemEvent
        {
            get
            {
                if(_CleanUpVirtualizedItemEvent == null)
                {
                    _CleanUpVirtualizedItemEvent = new EventListener<VirtualizingStackPanel>(
                        (sender, action) =>
                        {
                            CleanUpVirtualizedItemEventHandler handler = delegate { action(); };
                            sender.CleanUpVirtualizedItemEvent += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.CleanUpVirtualizedItemEvent -= (CleanUpVirtualizedItemEventHandler) handler;
                        }
                    );
                }
                return _CleanUpVirtualizedItemEvent;
            }
        }

    }

    public static class PopupEvents
    {
        private static EventListener<Popup> _Closed;
        public static EventListener<Popup> Closed
        {
            get
            {
                if(_Closed == null)
                {
                    _Closed = new EventListener<Popup>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.Closed += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Closed -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _Closed;
            }
        }

        private static EventListener<Popup> _Opened;
        public static EventListener<Popup> Opened
        {
            get
            {
                if(_Opened == null)
                {
                    _Opened = new EventListener<Popup>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.Opened += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Opened -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _Opened;
            }
        }

    }

    public static class SwapChainPanelEvents
    {
        private static EventListener<SwapChainPanel> _CompositionScaleChanged;
        public static EventListener<SwapChainPanel> CompositionScaleChanged
        {
            get
            {
                if(_CompositionScaleChanged == null)
                {
                    _CompositionScaleChanged = new EventListener<SwapChainPanel>(
                        (sender, action) =>
                        {
                            TypedEventHandler<SwapChainPanel, Object> handler = delegate { action(); };
                            sender.CompositionScaleChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.CompositionScaleChanged -= (TypedEventHandler<SwapChainPanel, Object>) handler;
                        }
                    );
                }
                return _CompositionScaleChanged;
            }
        }

    }

    public static class VisualStateGroupEvents
    {
        private static EventListener<VisualStateGroup> _CurrentStateChanged;
        public static EventListener<VisualStateGroup> CurrentStateChanged
        {
            get
            {
                if(_CurrentStateChanged == null)
                {
                    _CurrentStateChanged = new EventListener<VisualStateGroup>(
                        (sender, action) =>
                        {
                            VisualStateChangedEventHandler handler = delegate { action(); };
                            sender.CurrentStateChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.CurrentStateChanged -= (VisualStateChangedEventHandler) handler;
                        }
                    );
                }
                return _CurrentStateChanged;
            }
        }

        private static EventListener<VisualStateGroup> _CurrentStateChanging;
        public static EventListener<VisualStateGroup> CurrentStateChanging
        {
            get
            {
                if(_CurrentStateChanging == null)
                {
                    _CurrentStateChanging = new EventListener<VisualStateGroup>(
                        (sender, action) =>
                        {
                            VisualStateChangedEventHandler handler = delegate { action(); };
                            sender.CurrentStateChanging += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.CurrentStateChanging -= (VisualStateChangedEventHandler) handler;
                        }
                    );
                }
                return _CurrentStateChanging;
            }
        }

    }

    public static class WindowEvents
    {
        private static EventListener<Window> _Activated;
        public static EventListener<Window> Activated
        {
            get
            {
                if(_Activated == null)
                {
                    _Activated = new EventListener<Window>(
                        (sender, action) =>
                        {
                            global::Windows.Foundation.TypedEventHandler<object, WindowActivatedEventArgs> handler = delegate { action(); };
                            sender.Activated += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Activated -= (global::Windows.Foundation.TypedEventHandler<object, WindowActivatedEventArgs>) handler;
                        }
                    );
                }
                return _Activated;
            }
        }

        private static EventListener<Window> _Closed;
        public static EventListener<Window> Closed
        {
            get
            {
                if(_Closed == null)
                {
                    _Closed = new EventListener<Window>(
                        (sender, action) =>
                        {
                            global::Windows.Foundation.TypedEventHandler<object, WindowEventArgs> handler = delegate { action(); };
                            sender.Closed += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Closed -= (global::Windows.Foundation.TypedEventHandler<object, WindowEventArgs>) handler;
                        }
                    );
                }
                return _Closed;
            }
        }

        private static EventListener<Window> _SizeChanged;
        public static EventListener<Window> SizeChanged
        {
            get
            {
                if(_SizeChanged == null)
                {
                    _SizeChanged = new EventListener<Window>(
                        (sender, action) =>
                        {
                            global::Windows.Foundation.TypedEventHandler<object, WindowSizeChangedEventArgs> handler = delegate { action(); };
                            sender.SizeChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.SizeChanged -= (global::Windows.Foundation.TypedEventHandler<object, WindowSizeChangedEventArgs>) handler;
                        }
                    );
                }
                return _SizeChanged;
            }
        }

        private static EventListener<Window> _VisibilityChanged;
        public static EventListener<Window> VisibilityChanged
        {
            get
            {
                if(_VisibilityChanged == null)
                {
                    _VisibilityChanged = new EventListener<Window>(
                        (sender, action) =>
                        {
                            global::Windows.Foundation.TypedEventHandler<object, WindowVisibilityChangedEventArgs> handler = delegate { action(); };
                            sender.VisibilityChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.VisibilityChanged -= (global::Windows.Foundation.TypedEventHandler<object, WindowVisibilityChangedEventArgs>) handler;
                        }
                    );
                }
                return _VisibilityChanged;
            }
        }

    }

    public static class ControlEvents
    {
        private static EventListener<Control> _IsEnabledChanged;
        public static EventListener<Control> IsEnabledChanged
        {
            get
            {
                if(_IsEnabledChanged == null)
                {
                    _IsEnabledChanged = new EventListener<Control>(
                        (sender, action) =>
                        {
                            DependencyPropertyChangedEventHandler handler = delegate { action(); };
                            sender.IsEnabledChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.IsEnabledChanged -= (DependencyPropertyChangedEventHandler) handler;
                        }
                    );
                }
                return _IsEnabledChanged;
            }
        }

    }

    public static class DatePickerEvents
    {
        private static EventListener<DatePicker> _DateChanged;
        public static EventListener<DatePicker> DateChanged
        {
            get
            {
                if(_DateChanged == null)
                {
                    _DateChanged = new EventListener<DatePicker>(
                        (sender, action) =>
                        {
                            EventHandler<DatePickerValueChangedEventArgs> handler = delegate { action(); };
                            sender.DateChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DateChanged -= (EventHandler<DatePickerValueChangedEventArgs>) handler;
                        }
                    );
                }
                return _DateChanged;
            }
        }

    }

    public static class SemanticZoomEvents
    {
        private static EventListener<SemanticZoom> _ViewChangeCompleted;
        public static EventListener<SemanticZoom> ViewChangeCompleted
        {
            get
            {
                if(_ViewChangeCompleted == null)
                {
                    _ViewChangeCompleted = new EventListener<SemanticZoom>(
                        (sender, action) =>
                        {
                            SemanticZoomViewChangedEventHandler handler = delegate { action(); };
                            sender.ViewChangeCompleted += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ViewChangeCompleted -= (SemanticZoomViewChangedEventHandler) handler;
                        }
                    );
                }
                return _ViewChangeCompleted;
            }
        }

        private static EventListener<SemanticZoom> _ViewChangeStarted;
        public static EventListener<SemanticZoom> ViewChangeStarted
        {
            get
            {
                if(_ViewChangeStarted == null)
                {
                    _ViewChangeStarted = new EventListener<SemanticZoom>(
                        (sender, action) =>
                        {
                            SemanticZoomViewChangedEventHandler handler = delegate { action(); };
                            sender.ViewChangeStarted += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ViewChangeStarted -= (SemanticZoomViewChangedEventHandler) handler;
                        }
                    );
                }
                return _ViewChangeStarted;
            }
        }

    }

    public static class PasswordBoxEvents
    {
        private static EventListener<PasswordBox> _ContextMenuOpening;
        public static EventListener<PasswordBox> ContextMenuOpening
        {
            get
            {
                if(_ContextMenuOpening == null)
                {
                    _ContextMenuOpening = new EventListener<PasswordBox>(
                        (sender, action) =>
                        {
                            ContextMenuOpeningEventHandler handler = delegate { action(); };
                            sender.ContextMenuOpening += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ContextMenuOpening -= (ContextMenuOpeningEventHandler) handler;
                        }
                    );
                }
                return _ContextMenuOpening;
            }
        }

        private static EventListener<PasswordBox> _PasswordChanged;
        public static EventListener<PasswordBox> PasswordChanged
        {
            get
            {
                if(_PasswordChanged == null)
                {
                    _PasswordChanged = new EventListener<PasswordBox>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.PasswordChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.PasswordChanged -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _PasswordChanged;
            }
        }

        private static EventListener<PasswordBox> _Paste;
        public static EventListener<PasswordBox> Paste
        {
            get
            {
                if(_Paste == null)
                {
                    _Paste = new EventListener<PasswordBox>(
                        (sender, action) =>
                        {
                            TextControlPasteEventHandler handler = delegate { action(); };
                            sender.Paste += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Paste -= (TextControlPasteEventHandler) handler;
                        }
                    );
                }
                return _Paste;
            }
        }

    }

    public static class RichEditBoxEvents
    {
        private static EventListener<RichEditBox> _ContextMenuOpening;
        public static EventListener<RichEditBox> ContextMenuOpening
        {
            get
            {
                if(_ContextMenuOpening == null)
                {
                    _ContextMenuOpening = new EventListener<RichEditBox>(
                        (sender, action) =>
                        {
                            ContextMenuOpeningEventHandler handler = delegate { action(); };
                            sender.ContextMenuOpening += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ContextMenuOpening -= (ContextMenuOpeningEventHandler) handler;
                        }
                    );
                }
                return _ContextMenuOpening;
            }
        }

        private static EventListener<RichEditBox> _SelectionChanged;
        public static EventListener<RichEditBox> SelectionChanged
        {
            get
            {
                if(_SelectionChanged == null)
                {
                    _SelectionChanged = new EventListener<RichEditBox>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.SelectionChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.SelectionChanged -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _SelectionChanged;
            }
        }

        private static EventListener<RichEditBox> _TextChanged;
        public static EventListener<RichEditBox> TextChanged
        {
            get
            {
                if(_TextChanged == null)
                {
                    _TextChanged = new EventListener<RichEditBox>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.TextChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.TextChanged -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _TextChanged;
            }
        }

        private static EventListener<RichEditBox> _Paste;
        public static EventListener<RichEditBox> Paste
        {
            get
            {
                if(_Paste == null)
                {
                    _Paste = new EventListener<RichEditBox>(
                        (sender, action) =>
                        {
                            TextControlPasteEventHandler handler = delegate { action(); };
                            sender.Paste += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Paste -= (TextControlPasteEventHandler) handler;
                        }
                    );
                }
                return _Paste;
            }
        }

    }

    public static class ScrollViewerEvents
    {
        private static EventListener<ScrollViewer> _ViewChanged;
        public static EventListener<ScrollViewer> ViewChanged
        {
            get
            {
                if(_ViewChanged == null)
                {
                    _ViewChanged = new EventListener<ScrollViewer>(
                        (sender, action) =>
                        {
                            EventHandler<ScrollViewerViewChangedEventArgs> handler = delegate { action(); };
                            sender.ViewChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ViewChanged -= (EventHandler<ScrollViewerViewChangedEventArgs>) handler;
                        }
                    );
                }
                return _ViewChanged;
            }
        }

        private static EventListener<ScrollViewer> _ViewChanging;
        public static EventListener<ScrollViewer> ViewChanging
        {
            get
            {
                if(_ViewChanging == null)
                {
                    _ViewChanging = new EventListener<ScrollViewer>(
                        (sender, action) =>
                        {
                            EventHandler<ScrollViewerViewChangingEventArgs> handler = delegate { action(); };
                            sender.ViewChanging += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ViewChanging -= (EventHandler<ScrollViewerViewChangingEventArgs>) handler;
                        }
                    );
                }
                return _ViewChanging;
            }
        }

    }

    public static class TextBoxEvents
    {
        private static EventListener<TextBox> _ContextMenuOpening;
        public static EventListener<TextBox> ContextMenuOpening
        {
            get
            {
                if(_ContextMenuOpening == null)
                {
                    _ContextMenuOpening = new EventListener<TextBox>(
                        (sender, action) =>
                        {
                            ContextMenuOpeningEventHandler handler = delegate { action(); };
                            sender.ContextMenuOpening += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ContextMenuOpening -= (ContextMenuOpeningEventHandler) handler;
                        }
                    );
                }
                return _ContextMenuOpening;
            }
        }

        private static EventListener<TextBox> _SelectionChanged;
        public static EventListener<TextBox> SelectionChanged
        {
            get
            {
                if(_SelectionChanged == null)
                {
                    _SelectionChanged = new EventListener<TextBox>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.SelectionChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.SelectionChanged -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _SelectionChanged;
            }
        }

        private static EventListener<TextBox> _TextChanged;
        public static EventListener<TextBox> TextChanged
        {
            get
            {
                if(_TextChanged == null)
                {
                    _TextChanged = new EventListener<TextBox>(
                        (sender, action) =>
                        {
                            TextChangedEventHandler handler = delegate { action(); };
                            sender.TextChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.TextChanged -= (TextChangedEventHandler) handler;
                        }
                    );
                }
                return _TextChanged;
            }
        }

        private static EventListener<TextBox> _Paste;
        public static EventListener<TextBox> Paste
        {
            get
            {
                if(_Paste == null)
                {
                    _Paste = new EventListener<TextBox>(
                        (sender, action) =>
                        {
                            TextControlPasteEventHandler handler = delegate { action(); };
                            sender.Paste += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Paste -= (TextControlPasteEventHandler) handler;
                        }
                    );
                }
                return _Paste;
            }
        }

    }

    public static class ToggleSwitchEvents
    {
        private static EventListener<ToggleSwitch> _Toggled;
        public static EventListener<ToggleSwitch> Toggled
        {
            get
            {
                if(_Toggled == null)
                {
                    _Toggled = new EventListener<ToggleSwitch>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.Toggled += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Toggled -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _Toggled;
            }
        }

    }

    public static class ToolTipEvents
    {
        private static EventListener<ToolTip> _Closed;
        public static EventListener<ToolTip> Closed
        {
            get
            {
                if(_Closed == null)
                {
                    _Closed = new EventListener<ToolTip>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.Closed += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Closed -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _Closed;
            }
        }

        private static EventListener<ToolTip> _Opened;
        public static EventListener<ToolTip> Opened
        {
            get
            {
                if(_Opened == null)
                {
                    _Opened = new EventListener<ToolTip>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.Opened += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Opened -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _Opened;
            }
        }

    }

    public static class ButtonBaseEvents
    {
        private static EventListener<ButtonBase> _Click;
        public static EventListener<ButtonBase> Click
        {
            get
            {
                if(_Click == null)
                {
                    _Click = new EventListener<ButtonBase>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.Click += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Click -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _Click;
            }
        }

    }

    public static class RangeBaseEvents
    {
        private static EventListener<RangeBase> _ValueChanged;
        public static EventListener<RangeBase> ValueChanged
        {
            get
            {
                if(_ValueChanged == null)
                {
                    _ValueChanged = new EventListener<RangeBase>(
                        (sender, action) =>
                        {
                            RangeBaseValueChangedEventHandler handler = delegate { action(); };
                            sender.ValueChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ValueChanged -= (RangeBaseValueChangedEventHandler) handler;
                        }
                    );
                }
                return _ValueChanged;
            }
        }

    }

    public static class ScrollBarEvents
    {
        private static EventListener<ScrollBar> _Scroll;
        public static EventListener<ScrollBar> Scroll
        {
            get
            {
                if(_Scroll == null)
                {
                    _Scroll = new EventListener<ScrollBar>(
                        (sender, action) =>
                        {
                            ScrollEventHandler handler = delegate { action(); };
                            sender.Scroll += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Scroll -= (ScrollEventHandler) handler;
                        }
                    );
                }
                return _Scroll;
            }
        }

    }

    public static class SelectorEvents
    {
        private static EventListener<Selector> _SelectionChanged;
        public static EventListener<Selector> SelectionChanged
        {
            get
            {
                if(_SelectionChanged == null)
                {
                    _SelectionChanged = new EventListener<Selector>(
                        (sender, action) =>
                        {
                            SelectionChangedEventHandler handler = delegate { action(); };
                            sender.SelectionChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.SelectionChanged -= (SelectionChangedEventHandler) handler;
                        }
                    );
                }
                return _SelectionChanged;
            }
        }

    }

    public static class ComboBoxEvents
    {
        private static EventListener<ComboBox> _DropDownClosed;
        public static EventListener<ComboBox> DropDownClosed
        {
            get
            {
                if(_DropDownClosed == null)
                {
                    _DropDownClosed = new EventListener<ComboBox>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.DropDownClosed += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DropDownClosed -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _DropDownClosed;
            }
        }

        private static EventListener<ComboBox> _DropDownOpened;
        public static EventListener<ComboBox> DropDownOpened
        {
            get
            {
                if(_DropDownOpened == null)
                {
                    _DropDownOpened = new EventListener<ComboBox>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.DropDownOpened += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DropDownOpened -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _DropDownOpened;
            }
        }

    }

    public static class ListViewBaseEvents
    {
        private static EventListener<ListViewBase> _DragItemsStarting;
        public static EventListener<ListViewBase> DragItemsStarting
        {
            get
            {
                if(_DragItemsStarting == null)
                {
                    _DragItemsStarting = new EventListener<ListViewBase>(
                        (sender, action) =>
                        {
                            DragItemsStartingEventHandler handler = delegate { action(); };
                            sender.DragItemsStarting += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DragItemsStarting -= (DragItemsStartingEventHandler) handler;
                        }
                    );
                }
                return _DragItemsStarting;
            }
        }

        private static EventListener<ListViewBase> _ItemClick;
        public static EventListener<ListViewBase> ItemClick
        {
            get
            {
                if(_ItemClick == null)
                {
                    _ItemClick = new EventListener<ListViewBase>(
                        (sender, action) =>
                        {
                            ItemClickEventHandler handler = delegate { action(); };
                            sender.ItemClick += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ItemClick -= (ItemClickEventHandler) handler;
                        }
                    );
                }
                return _ItemClick;
            }
        }

        private static EventListener<ListViewBase> _ContainerContentChanging;
        public static EventListener<ListViewBase> ContainerContentChanging
        {
            get
            {
                if(_ContainerContentChanging == null)
                {
                    _ContainerContentChanging = new EventListener<ListViewBase>(
                        (sender, action) =>
                        {
                            TypedEventHandler<ListViewBase, ContainerContentChangingEventArgs> handler = delegate { action(); };
                            sender.ContainerContentChanging += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.ContainerContentChanging -= (TypedEventHandler<ListViewBase, ContainerContentChangingEventArgs>) handler;
                        }
                    );
                }
                return _ContainerContentChanging;
            }
        }

    }

    public static class ThumbEvents
    {
        private static EventListener<Thumb> _DragCompleted;
        public static EventListener<Thumb> DragCompleted
        {
            get
            {
                if(_DragCompleted == null)
                {
                    _DragCompleted = new EventListener<Thumb>(
                        (sender, action) =>
                        {
                            DragCompletedEventHandler handler = delegate { action(); };
                            sender.DragCompleted += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DragCompleted -= (DragCompletedEventHandler) handler;
                        }
                    );
                }
                return _DragCompleted;
            }
        }

        private static EventListener<Thumb> _DragDelta;
        public static EventListener<Thumb> DragDelta
        {
            get
            {
                if(_DragDelta == null)
                {
                    _DragDelta = new EventListener<Thumb>(
                        (sender, action) =>
                        {
                            DragDeltaEventHandler handler = delegate { action(); };
                            sender.DragDelta += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DragDelta -= (DragDeltaEventHandler) handler;
                        }
                    );
                }
                return _DragDelta;
            }
        }

        private static EventListener<Thumb> _DragStarted;
        public static EventListener<Thumb> DragStarted
        {
            get
            {
                if(_DragStarted == null)
                {
                    _DragStarted = new EventListener<Thumb>(
                        (sender, action) =>
                        {
                            DragStartedEventHandler handler = delegate { action(); };
                            sender.DragStarted += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.DragStarted -= (DragStartedEventHandler) handler;
                        }
                    );
                }
                return _DragStarted;
            }
        }

    }

    public static class ToggleButtonEvents
    {
        private static EventListener<ToggleButton> _Checked;
        public static EventListener<ToggleButton> Checked
        {
            get
            {
                if(_Checked == null)
                {
                    _Checked = new EventListener<ToggleButton>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.Checked += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Checked -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _Checked;
            }
        }

        private static EventListener<ToggleButton> _Indeterminate;
        public static EventListener<ToggleButton> Indeterminate
        {
            get
            {
                if(_Indeterminate == null)
                {
                    _Indeterminate = new EventListener<ToggleButton>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.Indeterminate += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Indeterminate -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _Indeterminate;
            }
        }

        private static EventListener<ToggleButton> _Unchecked;
        public static EventListener<ToggleButton> Unchecked
        {
            get
            {
                if(_Unchecked == null)
                {
                    _Unchecked = new EventListener<ToggleButton>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.Unchecked += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Unchecked -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _Unchecked;
            }
        }

    }

    public static class AppBarEvents
    {
        private static EventListener<AppBar> _Closed;
        public static EventListener<AppBar> Closed
        {
            get
            {
                if(_Closed == null)
                {
                    _Closed = new EventListener<AppBar>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.Closed += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Closed -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _Closed;
            }
        }

        private static EventListener<AppBar> _Opened;
        public static EventListener<AppBar> Opened
        {
            get
            {
                if(_Opened == null)
                {
                    _Opened = new EventListener<AppBar>(
                        (sender, action) =>
                        {
                            EventHandler<Object> handler = delegate { action(); };
                            sender.Opened += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Opened -= (EventHandler<Object>) handler;
                        }
                    );
                }
                return _Opened;
            }
        }

    }

    public static class FrameEvents
    {
        private static EventListener<Frame> _Navigated;
        public static EventListener<Frame> Navigated
        {
            get
            {
                if(_Navigated == null)
                {
                    _Navigated = new EventListener<Frame>(
                        (sender, action) =>
                        {
                            NavigatedEventHandler handler = delegate { action(); };
                            sender.Navigated += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Navigated -= (NavigatedEventHandler) handler;
                        }
                    );
                }
                return _Navigated;
            }
        }

        private static EventListener<Frame> _Navigating;
        public static EventListener<Frame> Navigating
        {
            get
            {
                if(_Navigating == null)
                {
                    _Navigating = new EventListener<Frame>(
                        (sender, action) =>
                        {
                            NavigatingCancelEventHandler handler = delegate { action(); };
                            sender.Navigating += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Navigating -= (NavigatingCancelEventHandler) handler;
                        }
                    );
                }
                return _Navigating;
            }
        }

        private static EventListener<Frame> _NavigationFailed;
        public static EventListener<Frame> NavigationFailed
        {
            get
            {
                if(_NavigationFailed == null)
                {
                    _NavigationFailed = new EventListener<Frame>(
                        (sender, action) =>
                        {
                            NavigationFailedEventHandler handler = delegate { action(); };
                            sender.NavigationFailed += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.NavigationFailed -= (NavigationFailedEventHandler) handler;
                        }
                    );
                }
                return _NavigationFailed;
            }
        }

        private static EventListener<Frame> _NavigationStopped;
        public static EventListener<Frame> NavigationStopped
        {
            get
            {
                if(_NavigationStopped == null)
                {
                    _NavigationStopped = new EventListener<Frame>(
                        (sender, action) =>
                        {
                            NavigationStoppedEventHandler handler = delegate { action(); };
                            sender.NavigationStopped += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.NavigationStopped -= (NavigationStoppedEventHandler) handler;
                        }
                    );
                }
                return _NavigationStopped;
            }
        }

    }

    public static class HubEvents
    {
        private static EventListener<Hub> _SectionHeaderClick;
        public static EventListener<Hub> SectionHeaderClick
        {
            get
            {
                if(_SectionHeaderClick == null)
                {
                    _SectionHeaderClick = new EventListener<Hub>(
                        (sender, action) =>
                        {
                            HubSectionHeaderClickEventHandler handler = delegate { action(); };
                            sender.SectionHeaderClick += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.SectionHeaderClick -= (HubSectionHeaderClickEventHandler) handler;
                        }
                    );
                }
                return _SectionHeaderClick;
            }
        }

        private static EventListener<Hub> _SectionsInViewChanged;
        public static EventListener<Hub> SectionsInViewChanged
        {
            get
            {
                if(_SectionsInViewChanged == null)
                {
                    _SectionsInViewChanged = new EventListener<Hub>(
                        (sender, action) =>
                        {
                            SectionsInViewChangedEventHandler handler = delegate { action(); };
                            sender.SectionsInViewChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.SectionsInViewChanged -= (SectionsInViewChangedEventHandler) handler;
                        }
                    );
                }
                return _SectionsInViewChanged;
            }
        }

    }

    public static class MenuFlyoutItemEvents
    {
        private static EventListener<MenuFlyoutItem> _Click;
        public static EventListener<MenuFlyoutItem> Click
        {
            get
            {
                if(_Click == null)
                {
                    _Click = new EventListener<MenuFlyoutItem>(
                        (sender, action) =>
                        {
                            RoutedEventHandler handler = delegate { action(); };
                            sender.Click += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.Click -= (RoutedEventHandler) handler;
                        }
                    );
                }
                return _Click;
            }
        }

    }

    public static class TimePickerEvents
    {
        private static EventListener<TimePicker> _TimeChanged;
        public static EventListener<TimePicker> TimeChanged
        {
            get
            {
                if(_TimeChanged == null)
                {
                    _TimeChanged = new EventListener<TimePicker>(
                        (sender, action) =>
                        {
                            EventHandler<TimePickerValueChangedEventArgs> handler = delegate { action(); };
                            sender.TimeChanged += handler;
                            return handler;
                        },
                        (sender, handler) =>
                        {
                            sender.TimeChanged -= (EventHandler<TimePickerValueChangedEventArgs>) handler;
                        }
                    );
                }
                return _TimeChanged;
            }
        }

    }


}
