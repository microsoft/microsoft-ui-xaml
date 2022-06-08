using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Devices.Input;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;

// The Templated Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234235

namespace SwitcherPrototype
{

    internal class InteractionInfo
    {
        internal uint CapturedPointerId
        {
            get;
            set;
        }

        internal bool IsPointerOver
        {
            get;
            set;
        }
    }

    public class ItemsViewContainer : ContentControl
    {
        public ItemsViewContainer()
        {
            this.IsTapEnabled = true;
            this.DefaultStyleKey = typeof(ItemsViewContainer);

            AddHandler(TappedEvent, new TappedEventHandler(ItemsViewContainer_PointerTapped), true /*handledEventsToo*/);
        }

        public IElementFactory EditTemplate
        {
            get { return GetValue(EditTemplateProperty) as IElementFactory; }
            set { SetValue(EditTemplateProperty, value); }
        }

        public static readonly DependencyProperty EditTemplateProperty =
            DependencyProperty.Register(
                nameof(EditTemplate),
                typeof(IElementFactory),
                typeof(ItemsViewContainer),
                new PropertyMetadata(null));

        protected virtual bool SupportsCurrency
        {
            get
            {
                return true;
            }
        }

        
        internal bool IsPointerOver
        {
            get
            {
                return this.InteractionInfo != null && this.InteractionInfo.IsPointerOver;
            }

            private set
            {
                if (value && this.InteractionInfo == null)
                {
                    this.InteractionInfo = new InteractionInfo();
                }

                if (this.InteractionInfo != null)
                {
                    this.InteractionInfo.IsPointerOver = value;
                }

                UpdateVisualState(true /*animate*/);
            }
        }

        public bool? IsSelected
        {
            get { return (bool?)GetValue(IsSelectedProperty); }
            set { SetValue(IsSelectedProperty, value); }
        }

        public static readonly DependencyProperty IsSelectedProperty =
            DependencyProperty.Register("IsSelected", typeof(bool?), typeof(ItemsViewContainer), new PropertyMetadata(null, new PropertyChangedCallback(OnIsSelectedChanged)));

        private static void OnIsSelectedChanged(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            (sender as ItemsViewContainer).UpdateVisualState(true);
        }

        protected override void OnApplyTemplate()
        {
            base.OnApplyTemplate();
            UpdateVisualState(true);
        }

        private InteractionInfo InteractionInfo
        {
            get;
            set;
        }

        internal virtual void UpdateVisualState(bool animate)
        {
            // CommonStates
            if (this.IsPointerOver && this.IsSelected.GetValueOrDefault(false))
            {
                VisualStateManager.GoToState(this, "PointerOverSelected", animate);
            }
            else if (this.IsPointerOver)
            {
                VisualStateManager.GoToState(this, "PointerOver", animate);
            }
            else if (this.IsSelected.GetValueOrDefault(false))
            {
                VisualStateManager.GoToState(this, "Selected", animate);
            }
            else
            {
                VisualStateManager.GoToState(this, "Normal", animate);
            }

            
        }

        private void CancelPointer(PointerRoutedEventArgs e)
        {
            if (this.InteractionInfo != null && this.InteractionInfo.CapturedPointerId == e.Pointer.PointerId)
            {
                this.InteractionInfo.CapturedPointerId = 0u;
            }

            this.IsPointerOver = false;
        }

        private void UpdateIsPointerOver(bool isPointerOver)
        {
            if (this.InteractionInfo != null && this.InteractionInfo.CapturedPointerId != 0u)
            {
                return;
            }

            this.IsPointerOver = isPointerOver;
        }

        protected override void OnPointerCanceled(PointerRoutedEventArgs e)
        {
            CancelPointer(e);
            base.OnPointerCanceled(e);
        }

        protected override void OnPointerCaptureLost(PointerRoutedEventArgs e)
        {
            CancelPointer(e);
            base.OnPointerCaptureLost(e);
        }

        protected override void OnPointerPressed(PointerRoutedEventArgs e)
        {
            if (e.Pointer.PointerDeviceType == PointerDeviceType.Touch &&
                (this.InteractionInfo == null || this.InteractionInfo.CapturedPointerId == 0u) &&
                this.CapturePointer(e.Pointer))
            {
                if (this.InteractionInfo == null)
                {
                    this.InteractionInfo = new InteractionInfo();
                }

                this.InteractionInfo.CapturedPointerId = e.Pointer.PointerId;
            }
            base.OnPointerPressed(e);
        }

        protected override void OnPointerReleased(PointerRoutedEventArgs e)
        {
            if (this.InteractionInfo != null && this.InteractionInfo.CapturedPointerId == e.Pointer.PointerId)
            {
                ReleasePointerCapture(e.Pointer);
            }
            base.OnPointerReleased(e);
        }

        protected override void OnPointerEntered(PointerRoutedEventArgs e)
        {
            UpdateIsPointerOver(true);
            base.OnPointerEntered(e);
        }

        protected override void OnPointerExited(PointerRoutedEventArgs e)
        {
            UpdateIsPointerOver(false);
            base.OnPointerExited(e);
        }

        protected override void OnPointerMoved(PointerRoutedEventArgs e)
        {
            UpdateIsPointerOver(true);
            base.OnPointerMoved(e);
        }

        private void ItemsViewContainer_PointerTapped(object sender, TappedRoutedEventArgs e)
        {
            IsSelected = true;
        }
    }
}
