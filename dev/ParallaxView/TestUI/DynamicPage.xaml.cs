// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;

using ParallaxSourceOffsetKind = Microsoft.UI.Xaml.Controls.ParallaxSourceOffsetKind;
using ParallaxView = Microsoft.UI.Xaml.Controls.ParallaxView;
using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using ScrollingAnimationMode = Microsoft.UI.Xaml.Controls.ScrollingAnimationMode;
using ScrollingSnapPointsMode = Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode;
using ScrollingScrollOptions = Microsoft.UI.Xaml.Controls.ScrollingScrollOptions;
using ScrollingZoomOptions = Microsoft.UI.Xaml.Controls.ScrollingZoomOptions;
using ScrollingScrollMode = Microsoft.UI.Xaml.Controls.ScrollingScrollMode;
using ScrollingZoomMode = Microsoft.UI.Xaml.Controls.ScrollingZoomMode;

namespace MUXControlsTestApp
{
    public sealed partial class DynamicPage : TestPage
    {
        ScrollPresenter scrollPresenter = null;
        Rectangle rectSC = null;

        public DynamicPage()
        {
            this.InitializeComponent();

            this.ParallaxView = this.parallaxView;
            this.ScrollViewer = null;
            this.ScrollPresenter = null;
            SetupScrollPresenter();
            this.TargetElement = null;
            this.DynamicParallaxView = null;
            this.AnimatedValuesSpy = null;
            this.HorizontalOffsetAnimation = null;
            this.VerticalOffsetAnimation = null;
            this.HasDirectManipulation = false;
            this.UIThreadTicksForValuesSpy = 0;
            this.IsRenderingHooked = false;

            this.chkIsRTL.IsChecked = (this.spInner.FlowDirection == FlowDirection.RightToLeft);

            this.chkAreHorizontalSourceOffsetsRelative.IsChecked = this.parallaxView.HorizontalSourceOffsetKind == ParallaxSourceOffsetKind.Relative;
            this.chkAreVerticalSourceOffsetsRelative.IsChecked = this.parallaxView.VerticalSourceOffsetKind == ParallaxSourceOffsetKind.Relative;
            this.chkIsHorizontalShiftClamped.IsChecked = this.parallaxView.IsHorizontalShiftClamped;
            this.chkIsVerticalShiftClamped.IsChecked = this.parallaxView.IsVerticalShiftClamped;

            btnGetHorizontalShift_Click(null, null);
            btnGetMaxHorizontalShiftRatio_Click(null, null);
            btnGetHorizontalSourceStartOffset_Click(null, null);
            btnGetHorizontalSourceEndOffset_Click(null, null);

            btnGetVerticalShift_Click(null, null);
            btnGetMaxVerticalShiftRatio_Click(null, null);
            btnGetVerticalSourceStartOffset_Click(null, null);
            btnGetVerticalSourceEndOffset_Click(null, null);
        }

        private ParallaxView ParallaxView
        {
            get;
            set;
        }

        private ParallaxView DynamicParallaxView
        {
            get;
            set;
        }

        private ScrollViewer ScrollViewer
        {
            get;
            set;
        }

        private ScrollPresenter ScrollPresenter
        {
            get;
            set;
        }

        private FrameworkElement TargetElement
        {
            get;
            set;
        }

        private CompositionPropertySet AnimatedValuesSpy
        {
            get;
            set;
        }

        private ExpressionAnimation HorizontalOffsetAnimation
        {
            get;
            set;
        }

        private ExpressionAnimation VerticalOffsetAnimation
        {
            get;
            set;
        }

        private bool HasDirectManipulation
        {
            get;
            set;
        }

        private bool IsRenderingHooked
        {
            get;
            set;
        }

        private uint UIThreadTicksForValuesSpy
        {
            get;
            set;
        }

        private void SetupAnimatedValuesSpy()
        {
            StopAnimatedValuesSpy();

            this.AnimatedValuesSpy = null;
            this.HorizontalOffsetAnimation = null;
            this.VerticalOffsetAnimation = null;

            if (this.ParallaxView != null && this.ParallaxView.Child is UIElement &&
                (this.ParallaxView.HorizontalShift != 0 || this.ParallaxView.VerticalShift != 0))
            {
                string visualHorizontalTargetedPropertyName =
                    PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2) ? "visual.Translation.X" : "visual.TransformMatrix._41";
                string visualVerticalTargetedPropertyName =
                    PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2) ? "visual.Translation.Y" : "visual.TransformMatrix._42";

                UIElement ppChild = this.ParallaxView.Child as UIElement;
                Visual visualPPChild = ElementCompositionPreview.GetElementVisual(ppChild);

                Compositor compositor = visualPPChild.Compositor;

                this.AnimatedValuesSpy = compositor.CreatePropertySet();
                this.AnimatedValuesSpy.InsertScalar("HorizontalOffset", 0.0f);
                this.AnimatedValuesSpy.InsertScalar("VerticalOffset", 0.0f);

                this.HorizontalOffsetAnimation = compositor.CreateExpressionAnimation(visualHorizontalTargetedPropertyName);
                this.VerticalOffsetAnimation = compositor.CreateExpressionAnimation(visualVerticalTargetedPropertyName);

                this.HorizontalOffsetAnimation.SetReferenceParameter("visual", visualPPChild);
                this.VerticalOffsetAnimation.SetReferenceParameter("visual", visualPPChild);

                CheckSpyingTicksRequirement();

                TickForValuesSpy();
            }
            else
            {
                ResetSpyOutput();
            }
        }

        private void StartAnimatedValuesSpy()
        {
            if (this.AnimatedValuesSpy != null)
            {
                this.AnimatedValuesSpy.StartAnimation("HorizontalOffset", this.HorizontalOffsetAnimation);
                this.AnimatedValuesSpy.StartAnimation("VerticalOffset", this.VerticalOffsetAnimation);
            }
        }

        private void StopAnimatedValuesSpy()
        {
            if (this.AnimatedValuesSpy != null)
            {
                this.AnimatedValuesSpy.StopAnimation("HorizontalOffset");
                this.AnimatedValuesSpy.StopAnimation("VerticalOffset");
            }
        }

        private void SpyAnimatedValues()
        {
            if (this.AnimatedValuesSpy != null)
            {
                StopAnimatedValuesSpy();

                float offset = 0.0f;
                CompositionGetValueStatus status = this.AnimatedValuesSpy.TryGetScalar("HorizontalOffset", out offset);
                if (CompositionGetValueStatus.Succeeded == status)
                {
                    this.tbHorizontalOutput.Text = offset.ToString();
                }
                else
                {
                    this.tbHorizontalOutput.Text = "status=" + status.ToString();
                }

                status = this.AnimatedValuesSpy.TryGetScalar("VerticalOffset", out offset);
                if (CompositionGetValueStatus.Succeeded == status)
                {
                    this.tbVerticalOutput.Text = offset.ToString();
                }
                else
                {
                    this.tbVerticalOutput.Text = "status=" + status.ToString();
                }

                StartAnimatedValuesSpy();

                System.Diagnostics.Debug.WriteLine("Spied values: " + this.tbHorizontalOutput.Text + ", " + this.tbVerticalOutput.Text);
            }
        }

        private void ResetSpyOutput()
        {
            this.tbHorizontalOutput.Text = "0";
            this.tbVerticalOutput.Text = "0";
        }

        private void RefreshContentAlignments()
        {
            if (this.ScrollViewer != null)
            {
                this.cmbHorizontalContentAlignment.SelectedIndex = (int)this.ScrollViewer.HorizontalContentAlignment;
                this.cmbVerticalContentAlignment.SelectedIndex = (int)this.ScrollViewer.VerticalContentAlignment;
            }
        }

        private void cmbHorizontalContentAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.ScrollViewer != null)
            {
                this.ScrollViewer.HorizontalContentAlignment = (HorizontalAlignment)this.cmbHorizontalContentAlignment.SelectedIndex;
            }
        }

        private void cmbVerticalContentAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.ScrollViewer != null)
            {
                this.ScrollViewer.VerticalContentAlignment = (VerticalAlignment)this.cmbVerticalContentAlignment.SelectedIndex;
            }
        }

        private void RefreshSourceContent()
        {
            if (
                (this.ScrollPresenter != null && this.ScrollPresenter.Content != null) ||
                (this.ScrollViewer != null && this.ScrollViewer.Content != null))
            {
                this.cmbSourceContent.SelectedIndex = 1;
            }
            else
            {
                this.cmbSourceContent.SelectedIndex = 0;
            }
        }

        private void cmbSourceContent_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.ScrollViewer != null)
            {
                switch (this.cmbSourceContent.SelectedIndex)
                {
                    case 0:
                        this.ScrollViewer.Content = null;
                        break;
                    case 1:
                        if (this.ScrollViewer == this.scrollViewer)
                            this.ScrollViewer.Content = this.spSV;
                        break;
                }
            }

            if (this.ScrollPresenter != null)
            {
                switch (this.cmbSourceContent.SelectedIndex)
                {
                    case 0:
                        this.ScrollPresenter.Content = null;
                        break;
                    case 1:
                        if (this.ScrollPresenter == this.scrollPresenter)
                            this.ScrollPresenter.Content = this.rectSC;
                        break;
                }
            }
        }

        private void RefreshSVCAlignments()
        {
            FrameworkElement feSourceContent = null;

            if (this.ScrollViewer != null && this.ScrollViewer.Content is FrameworkElement)
            {
                feSourceContent = this.ScrollViewer.Content as FrameworkElement;
            }
            else if (this.ScrollPresenter != null && this.ScrollPresenter.Content is FrameworkElement)
            {
                feSourceContent = this.ScrollPresenter.Content as FrameworkElement;
            }

            if (feSourceContent != null)
            {
                this.cmbSCHorizontalAlignment.SelectedIndex = (int)feSourceContent.HorizontalAlignment;
                this.cmbSCVerticalAlignment.SelectedIndex = (int)feSourceContent.VerticalAlignment;
            }
        }

        private void cmbSCHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            FrameworkElement feSourceContent = null;

            if (this.ScrollViewer != null && this.ScrollViewer.Content is FrameworkElement)
            {
                feSourceContent = this.ScrollViewer.Content as FrameworkElement;
            }
            else if (this.ScrollPresenter != null && this.ScrollPresenter.Content is FrameworkElement)
            {
                feSourceContent = this.ScrollPresenter.Content as FrameworkElement;
            }

            if (feSourceContent != null)
            {
                feSourceContent.HorizontalAlignment = (HorizontalAlignment)this.cmbSCHorizontalAlignment.SelectedIndex;
            }
        }

        private void cmbSCVerticalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            FrameworkElement feSourceContent = null;

            if (this.ScrollViewer != null && this.ScrollViewer.Content is FrameworkElement)
            {
                feSourceContent = this.ScrollViewer.Content as FrameworkElement;
            }
            else if (this.ScrollPresenter != null && this.ScrollPresenter.Content is FrameworkElement)
            {
                feSourceContent = this.ScrollPresenter.Content as FrameworkElement;
            }

            if (feSourceContent != null)
            {
                feSourceContent.VerticalAlignment = (VerticalAlignment)this.cmbSCVerticalAlignment.SelectedIndex;
            }
        }

        private void RefreshSourceContentSizes()
        {
            btnGetSCWidth_Click(null, null);
            btnGetSCHeight_Click(null, null);
        }

        private void btnGetSCWidth_Click(object sender, RoutedEventArgs e)
        {
            FrameworkElement feSourceContent = null;

            if (this.ScrollViewer != null && this.ScrollViewer.Content is FrameworkElement)
            {
                feSourceContent = this.ScrollViewer.Content as FrameworkElement;
            }
            else if (this.ScrollPresenter != null && this.ScrollPresenter.Content is FrameworkElement)
            {
                feSourceContent = this.ScrollPresenter.Content as FrameworkElement;
            }

            if (feSourceContent != null)
            {
                this.txtSCWidth.Text = feSourceContent.Width.ToString();
            }
        }

        private void btnSetSCWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                FrameworkElement feSourceContent = null;

                if (this.ScrollViewer != null && this.ScrollViewer.Content is FrameworkElement)
                {
                    feSourceContent = this.ScrollViewer.Content as FrameworkElement;
                }
                else if (this.ScrollPresenter != null && this.ScrollPresenter.Content is FrameworkElement)
                {
                    feSourceContent = this.ScrollPresenter.Content as FrameworkElement;
                }

                if (feSourceContent != null)
                {
                    feSourceContent.Width = Convert.ToDouble(this.txtSCWidth.Text);
                }
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetSCHeight_Click(object sender, RoutedEventArgs e)
        {
            FrameworkElement feSourceContent = null;

            if (this.ScrollViewer != null && this.ScrollViewer.Content is FrameworkElement)
            {
                feSourceContent = this.ScrollViewer.Content as FrameworkElement;
            }
            else if (this.ScrollPresenter != null && this.ScrollPresenter.Content is FrameworkElement)
            {
                feSourceContent = this.ScrollPresenter.Content as FrameworkElement;
            }

            if (feSourceContent != null)
            {
                this.txtSCHeight.Text = feSourceContent.Height.ToString();
            }
        }

        private void btnSetSCHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                FrameworkElement feSourceContent = null;

                if (this.ScrollViewer != null && this.ScrollViewer.Content is FrameworkElement)
                {
                    feSourceContent = this.ScrollViewer.Content as FrameworkElement;
                }
                else if (this.ScrollPresenter != null && this.ScrollPresenter.Content is FrameworkElement)
                {
                    feSourceContent = this.ScrollPresenter.Content as FrameworkElement;
                }

                if (feSourceContent != null)
                {
                    feSourceContent.Height = Convert.ToDouble(this.txtSCHeight.Text);
                }
            }
            catch (FormatException)
            {
            }
        }

        private void RefreshTEAlignments()
        {
            this.cmbTEHorizontalAlignment.SelectedIndex = (int)this.TargetElement.HorizontalAlignment;
            this.cmbTEVerticalAlignment.SelectedIndex = (int)this.TargetElement.VerticalAlignment;
        }

        private void RefreshTESizes()
        {
            btnGetTEWidth_Click(null, null);
            btnGetTEHeight_Click(null, null);
            btnGetTEMaxWidth_Click(null, null);
            btnGetTEMaxHeight_Click(null, null);
            btnGetTEActualWidth_Click(null, null);
            btnGetTEActualHeight_Click(null, null);
            btnGetTEMargin_Click(null, null);
        }

        private void cmbTEHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.TargetElement != null)
            {
                this.TargetElement.HorizontalAlignment = (HorizontalAlignment)this.cmbTEHorizontalAlignment.SelectedIndex;
            }

            this.refImg.HorizontalAlignment = (HorizontalAlignment)this.cmbTEHorizontalAlignment.SelectedIndex;
        }

        private void cmbTEVerticalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.TargetElement != null)
            {
                this.TargetElement.VerticalAlignment = (VerticalAlignment)this.cmbTEVerticalAlignment.SelectedIndex;
            }

            this.refImg.VerticalAlignment = (VerticalAlignment)this.cmbTEVerticalAlignment.SelectedIndex;
        }

        private void btnGetTEWidth_Click(object sender, RoutedEventArgs e)
        {
            if (this.TargetElement is FrameworkElement)
            {
                this.txtTEWidth.Text = (this.TargetElement as FrameworkElement).Width.ToString();
            }
        }

        private void btnSetTEWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.TargetElement is FrameworkElement)
                {
                    (this.TargetElement as FrameworkElement).Width = Convert.ToDouble(this.txtTEWidth.Text);
                }

                this.refImg.Width = Convert.ToDouble(this.txtTEWidth.Text);
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetTEHeight_Click(object sender, RoutedEventArgs e)
        {
            if (this.TargetElement is FrameworkElement)
            {
                this.txtTEHeight.Text = (this.TargetElement as FrameworkElement).Height.ToString();
            }
        }

        private void btnSetTEHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.TargetElement is FrameworkElement)
                {
                    (this.TargetElement as FrameworkElement).Height = Convert.ToDouble(this.txtTEHeight.Text);
                }

                this.refImg.Height = Convert.ToDouble(this.txtTEHeight.Text);
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetTEMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            if (this.TargetElement is FrameworkElement)
            {
                this.txtTEMaxWidth.Text = (this.TargetElement as FrameworkElement).MaxWidth.ToString();
            }
        }

        private void btnSetTEMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.TargetElement is FrameworkElement)
                {
                    (this.TargetElement as FrameworkElement).MaxWidth = Convert.ToDouble(this.txtTEMaxWidth.Text);
                }

                this.refImg.MaxWidth = Convert.ToDouble(this.txtTEMaxWidth.Text);
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetTEMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            if (this.TargetElement is FrameworkElement)
            {
                this.txtTEMaxHeight.Text = (this.TargetElement as FrameworkElement).MaxHeight.ToString();
            }
        }

        private void btnSetTEMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.TargetElement is FrameworkElement)
                {
                    (this.TargetElement as FrameworkElement).MaxHeight = Convert.ToDouble(this.txtTEMaxHeight.Text);
                }

                this.refImg.MaxHeight = Convert.ToDouble(this.txtTEMaxHeight.Text);
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetTEActualWidth_Click(object sender, RoutedEventArgs e)
        {
            if (this.TargetElement is FrameworkElement)
            {
                this.txtTEActualWidth.Text = (this.TargetElement as FrameworkElement).ActualWidth.ToString();
            }
        }

        private void btnGetTEActualHeight_Click(object sender, RoutedEventArgs e)
        {
            if (this.TargetElement is FrameworkElement)
            {
                this.txtTEActualHeight.Text = (this.TargetElement as FrameworkElement).ActualHeight.ToString();
            }
        }

        private void btnGetTEMargin_Click(object sender, RoutedEventArgs e)
        {
            if (this.TargetElement != null)
            {
                this.txtTEMarginLeft.Text = this.TargetElement.Margin.Left.ToString();
                this.txtTEMarginTop.Text = this.TargetElement.Margin.Top.ToString();
                this.txtTEMarginRight.Text = this.TargetElement.Margin.Right.ToString();
                this.txtTEMarginBottom.Text = this.TargetElement.Margin.Bottom.ToString();
            }
        }

        private void btnSetTEMargin_Click(object sender, RoutedEventArgs e)
        {
            if (this.TargetElement != null)
            {
                try
                {
                    this.TargetElement.Margin = new Thickness(
                        Convert.ToDouble(this.txtTEMarginLeft.Text),
                        Convert.ToDouble(this.txtTEMarginTop.Text),
                        Convert.ToDouble(this.txtTEMarginRight.Text),
                        Convert.ToDouble(this.txtTEMarginBottom.Text));
                }
                catch (FormatException)
                {
                }
            }
        }

        private void RefreshHorizontalScrollMode()
        {
            if (this.ScrollViewer != null)
            {
                this.cmbHorizontalScrollMode.SelectedIndex = (int)this.ScrollViewer.HorizontalScrollMode;
            }
            else if (this.ScrollPresenter != null)
            {
                switch (this.ScrollPresenter.HorizontalScrollMode)
                {
                    case ScrollingScrollMode.Disabled:
                        this.cmbHorizontalScrollMode.SelectedIndex = 0;
                        break;
                    case ScrollingScrollMode.Enabled:
                        this.cmbHorizontalScrollMode.SelectedIndex = 1;
                        break;
#if USE_SCROLLMODE_AUTO
                    case ScrollMode.Auto:
                        this.cmbHorizontalScrollMode.SelectedIndex = 2;
                        break;
#endif
                }
            }
            else
            {
                this.cmbHorizontalScrollMode.SelectedIndex = 0;
            }
        }

        private void cmbHorizontalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.ScrollViewer != null)
            {
                this.ScrollViewer.HorizontalScrollMode = (Windows.UI.Xaml.Controls.ScrollMode)this.cmbHorizontalScrollMode.SelectedIndex;
            }
            else if (this.ScrollPresenter != null)
            {
                switch (this.cmbHorizontalScrollMode.SelectedIndex)
                {
                    case 0:
                        this.ScrollPresenter.HorizontalScrollMode = ScrollingScrollMode.Disabled;
                        break;
                    case 1:
                        this.ScrollPresenter.HorizontalScrollMode = ScrollingScrollMode.Enabled;
                        break;
                    case 2:
#if USE_SCROLLMODE_AUTO
                        this.ScrollPresenter.HorizontalScrollMode = ScrollMode.Auto;
#else
                        this.cmbHorizontalScrollMode.SelectedIndex = this.ScrollPresenter.HorizontalScrollMode == ScrollingScrollMode.Disabled ? 0 : 1;
#endif
                        break;
                }
            }
        }

        private void RefreshVerticalScrollMode()
        {
            if (this.ScrollViewer != null)
            {
                this.cmbVerticalScrollMode.SelectedIndex = (int)this.ScrollViewer.VerticalScrollMode;
            }
            else if (this.ScrollPresenter != null)
            {
                switch (this.ScrollPresenter.VerticalScrollMode)
                {
                    case ScrollingScrollMode.Disabled:
                        this.cmbVerticalScrollMode.SelectedIndex = 0;
                        break;
                    case ScrollingScrollMode.Enabled:
                        this.cmbVerticalScrollMode.SelectedIndex = 1;
                        break;
#if USE_SCROLLMODE_AUTO
                    case ScrollMode.Auto:
                        this.cmbVerticalScrollMode.SelectedIndex = 2;
                        break;
#endif
                }
            }
            else
            {
                this.cmbVerticalScrollMode.SelectedIndex = 0;
            }
        }

        private void cmbVerticalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.ScrollViewer != null)
            {
                this.ScrollViewer.VerticalScrollMode = (Windows.UI.Xaml.Controls.ScrollMode)this.cmbVerticalScrollMode.SelectedIndex;
            }
            else if (this.ScrollPresenter != null)
            {
                switch (this.cmbVerticalScrollMode.SelectedIndex)
                {
                    case 0:
                        this.ScrollPresenter.VerticalScrollMode = ScrollingScrollMode.Disabled;
                        break;
                    case 1:
                        this.ScrollPresenter.VerticalScrollMode = ScrollingScrollMode.Enabled;
                        break;
                    case 2:
#if USE_SCROLLMODE_AUTO
                        this.ScrollPresenter.VerticalScrollMode = ScrollMode.Auto;
#else
                        this.cmbVerticalScrollMode.SelectedIndex = this.ScrollPresenter.VerticalScrollMode == ScrollingScrollMode.Disabled ? 0 : 1;
#endif
                        break;
                }
            }
        }

        private void RefreshZoomMode()
        {
            if (this.ScrollViewer != null)
            {
                this.cmbZoomMode.SelectedIndex = (int)this.ScrollViewer.ZoomMode;
            }
            else if (this.ScrollPresenter != null)
            {
                switch (this.ScrollPresenter.ZoomMode)
                {
                    case ScrollingZoomMode.Disabled:
                        this.cmbZoomMode.SelectedIndex = 0;
                        break;
                    case ScrollingZoomMode.Enabled:
                        this.cmbZoomMode.SelectedIndex = 1;
                        break;
                }
            }
            else
            {
                this.cmbZoomMode.SelectedIndex = 0;
            }
        }

        private void cmbZoomMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.ScrollViewer != null)
            {
                this.ScrollViewer.ZoomMode = (Windows.UI.Xaml.Controls.ZoomMode)this.cmbZoomMode.SelectedIndex;
            }
            else if (this.ScrollPresenter != null)
            {
                switch (this.cmbZoomMode.SelectedIndex)
                {
                    case 0:
                        this.ScrollPresenter.ZoomMode = ScrollingZoomMode.Disabled;
                        break;
                    case 1:
                        this.ScrollPresenter.ZoomMode = ScrollingZoomMode.Enabled;
                        break;
                }
            }
        }

        private void cmbSource_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.ParallaxView != null)
            {
                if (this.ScrollViewer != null)
                {
                    this.ScrollViewer.ViewChanged -= ScrollViewer_ViewChanged;
                    this.ScrollViewer.DirectManipulationStarted -= ScrollViewer_DirectManipulationStarted;
                    this.ScrollViewer.DirectManipulationCompleted -= ScrollViewer_DirectManipulationCompleted;
                }
                else if (this.ScrollPresenter != null)
                {
                    this.ScrollPresenter.ViewChanged -= ScrollPresenter_ViewChanged;
                }
                switch (this.cmbSource.SelectedIndex)
                {
                    case 0:
                        this.ParallaxView.Source = null;
                        this.ScrollViewer = null;
                        this.ScrollPresenter = null;
                        break;
                    case 1:
                        this.ParallaxView.Source = this.scrollViewer;
                        this.ScrollViewer = this.scrollViewer;
                        this.ScrollPresenter = null;
                        if (this.cmbSourceContent.Items.Count > 1)
                            this.cmbSourceContent.Items.RemoveAt(1);
                        this.cmbSourceContent.Items.Add("stackPanel");
                        RefreshSourceContent();
                        RefreshSourceContentSizes();
                        RefreshSVCAlignments();
                        RefreshContentAlignments();
                        RefreshHorizontalScrollMode();
                        RefreshVerticalScrollMode();
                        RefreshZoomMode();
                        btnGetHorizontalOffset_Click(null, null);
                        btnGetVerticalOffset_Click(null, null);
                        btnGetZoomFactor_Click(null, null);
                        break;
                    case 2:
                        this.ParallaxView.Source = this.scrollPresenter;
                        this.ScrollPresenter = this.scrollPresenter;
                        this.ScrollViewer = null;
                        if (this.cmbSourceContent.Items.Count > 1)
                            this.cmbSourceContent.Items.RemoveAt(1);
                        this.cmbSourceContent.Items.Add("rectangle");
                        RefreshSourceContent();
                        RefreshSourceContentSizes();
                        RefreshSVCAlignments();
                        RefreshContentAlignments();
                        RefreshHorizontalScrollMode();
                        RefreshVerticalScrollMode();
                        RefreshZoomMode();
                        btnGetHorizontalOffset_Click(null, null);
                        btnGetVerticalOffset_Click(null, null);
                        btnGetZoomFactor_Click(null, null);
                        break;
                }

                SetupAnimatedValuesSpy();

                if (this.ScrollViewer != null)
                {
                    this.ScrollViewer.ViewChanged += ScrollViewer_ViewChanged;
                    this.ScrollViewer.DirectManipulationStarted += ScrollViewer_DirectManipulationStarted;
                    this.ScrollViewer.DirectManipulationCompleted += ScrollViewer_DirectManipulationCompleted;
                }
                else if (this.ScrollPresenter != null)
                {
                    this.ScrollPresenter.ViewChanged += ScrollPresenter_ViewChanged;
                }
            }
        }

        private void ScrollViewer_DirectManipulationCompleted(object sender, object e)
        {
            TickForValuesSpy();
            this.HasDirectManipulation = false;
        }

        private void ScrollViewer_DirectManipulationStarted(object sender, object e)
        {
            this.HasDirectManipulation = true;
            CheckSpyingTicksRequirement();
        }

        private void ScrollViewer_ViewChanged(object sender, ScrollViewerViewChangedEventArgs e)
        {
            CheckSpyingTicksRequirement();
            SpyAnimatedValues();
        }

        private void ScrollPresenter_ViewChanged(ScrollPresenter sender, object args)
        {
            SpyAnimatedValues();
        }

        private void TickForValuesSpy()
        {
            this.UIThreadTicksForValuesSpy = 6;
            CheckSpyingTicksRequirement();
        }

        private void CheckSpyingTicksRequirement()
        {
            if (this.ScrollViewer != null &&
                (this.HasDirectManipulation || this.UIThreadTicksForValuesSpy > 0) &&
                this.AnimatedValuesSpy != null &&
                (this.ScrollViewer.HorizontalOffset == 0 || this.ScrollViewer.VerticalOffset == 0 ||
                 this.ScrollViewer.HorizontalOffset == this.ScrollViewer.ScrollableWidth || this.ScrollViewer.VerticalOffset == this.ScrollViewer.ScrollableHeight))
            {
                if (!this.IsRenderingHooked)
                {
                    this.IsRenderingHooked = true;
                    Windows.UI.Xaml.Media.CompositionTarget.Rendering += CompositionTarget_Rendering;
                }
            }
            else
            {
                if (this.IsRenderingHooked)
                {
                    Windows.UI.Xaml.Media.CompositionTarget.Rendering -= CompositionTarget_Rendering;
                    this.IsRenderingHooked = false;
                }
            }
        }

        private void CompositionTarget_Rendering(object sender, object e)
        {
            if (this.UIThreadTicksForValuesSpy > 0)
            {
                this.UIThreadTicksForValuesSpy--;
            }
            CheckSpyingTicksRequirement();
            SpyAnimatedValues();
        }

        private void cmbTargetElement_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.ParallaxView != null)
            {
                switch (this.cmbTargetElement.SelectedIndex)
                {
                    case 0:
                        this.ParallaxView.Child = null;
                        this.TargetElement = null;
                        break;
                    case 1:
                    case 2:
                    case 3:
                        FrameworkElement targetFE = null;

                        if (this.cmbTargetElement.SelectedIndex == 1)
                            targetFE = this.border;
                        else if (this.cmbTargetElement.SelectedIndex == 2)
                            targetFE = this.img;
                        else
                            targetFE = this.rect;

                        if (targetFE.Parent != null)
                            (targetFE.Parent as Panel).Children.Remove(targetFE);
                        targetFE.Visibility = Visibility.Visible;
                        this.ParallaxView.Child = targetFE;
                        this.TargetElement = targetFE;
                        RefreshTEAlignments();
                        RefreshTESizes();
                        break;
                }

                SetupAnimatedValuesSpy();
            }
        }

        private void btnGetWidth_Click(object sender, RoutedEventArgs e)
        {
            this.txtWidth.Text = this.ParallaxView.Width.ToString();
        }

        private void btnSetWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.ParallaxView.Width = Convert.ToDouble(this.txtWidth.Text);
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetHorizontalShift_Click(object sender, RoutedEventArgs e)
        {
            this.txtHorizontalShift.Text = this.ParallaxView.HorizontalShift.ToString();
        }

        private void btnSetHorizontalShift_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.ParallaxView.HorizontalShift = Convert.ToDouble(this.txtHorizontalShift.Text);
                SetupAnimatedValuesSpy();
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetMaxHorizontalShiftRatio_Click(object sender, RoutedEventArgs e)
        {
            this.txtMaxHorizontalShiftRatio.Text = this.ParallaxView.MaxHorizontalShiftRatio.ToString();
        }

        private void btnSetMaxHorizontalShiftRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.ParallaxView.MaxHorizontalShiftRatio = Convert.ToDouble(this.txtMaxHorizontalShiftRatio.Text);
                TickForValuesSpy();
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetHorizontalSourceStartOffset_Click(object sender, RoutedEventArgs e)
        {
            this.txtHorizontalSourceStartOffset.Text = this.ParallaxView.HorizontalSourceStartOffset.ToString();
        }

        private void btnSetHorizontalSourceStartOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.ParallaxView.HorizontalSourceStartOffset = Convert.ToDouble(this.txtHorizontalSourceStartOffset.Text);
                TickForValuesSpy();
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetHorizontalSourceEndOffset_Click(object sender, RoutedEventArgs e)
        {
            this.txtHorizontalSourceEndOffset.Text = this.ParallaxView.HorizontalSourceEndOffset.ToString();
        }

        private void btnSetHorizontalSourceEndOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.ParallaxView.HorizontalSourceEndOffset = Convert.ToDouble(this.txtHorizontalSourceEndOffset.Text);
                TickForValuesSpy();
            }
            catch (FormatException)
            {
            }
        }

        private void chkIsRTL_Unchecked(object sender, RoutedEventArgs e)
        {
            this.spInner.FlowDirection = FlowDirection.LeftToRight;
        }

        private void chkIsRTL_Checked(object sender, RoutedEventArgs e)
        {
            this.spInner.FlowDirection = FlowDirection.RightToLeft;
        }

        private void cmbParallaxView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (this.cmbParallaxView.SelectedIndex)
            {
                case 0:
                    this.ParallaxView = this.parallaxView;
                    break;
                case 1:
                    if (this.DynamicParallaxView != null)
                        this.ParallaxView = this.DynamicParallaxView;
                    else
                        this.cmbParallaxView.SelectedIndex = 0;
                    break;
            }

            this.chkAreHorizontalSourceOffsetsRelative.IsChecked = this.ParallaxView.HorizontalSourceOffsetKind == ParallaxSourceOffsetKind.Relative;
            this.chkAreVerticalSourceOffsetsRelative.IsChecked = this.ParallaxView.VerticalSourceOffsetKind == ParallaxSourceOffsetKind.Relative;
            this.chkIsHorizontalShiftClamped.IsChecked = this.ParallaxView.IsHorizontalShiftClamped;
            this.chkIsVerticalShiftClamped.IsChecked = this.ParallaxView.IsVerticalShiftClamped;
        }

        private void chkAreHorizontalSourceOffsetsRelative_Unchecked(object sender, RoutedEventArgs e)
        {
            this.ParallaxView.HorizontalSourceOffsetKind = ParallaxSourceOffsetKind.Absolute;
            TickForValuesSpy();
        }

        private void chkAreHorizontalSourceOffsetsRelative_Checked(object sender, RoutedEventArgs e)
        {
            this.ParallaxView.HorizontalSourceOffsetKind = ParallaxSourceOffsetKind.Relative;
            TickForValuesSpy();
        }

        private void chkIsHorizontalShiftClamped_Unchecked(object sender, RoutedEventArgs e)
        {
            this.ParallaxView.IsHorizontalShiftClamped = false;
            TickForValuesSpy();
        }

        private void chkIsHorizontalShiftClamped_Checked(object sender, RoutedEventArgs e)
        {
            this.ParallaxView.IsHorizontalShiftClamped = true;
            TickForValuesSpy();
        }

        private void btnGetHeight_Click(object sender, RoutedEventArgs e)
        {
            this.txtHeight.Text = this.ParallaxView.Height.ToString();
        }

        private void btnSetHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            { 
                this.ParallaxView.Height = Convert.ToDouble(this.txtHeight.Text);
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetVerticalShift_Click(object sender, RoutedEventArgs e)
        {
            this.txtVerticalShift.Text = this.ParallaxView.VerticalShift.ToString();
        }

        private void btnSetVerticalShift_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.ParallaxView.VerticalShift = Convert.ToDouble(this.txtVerticalShift.Text);
                SetupAnimatedValuesSpy();
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetMaxVerticalShiftRatio_Click(object sender, RoutedEventArgs e)
        {
            this.txtMaxVerticalShiftRatio.Text = this.ParallaxView.MaxVerticalShiftRatio.ToString();
        }

        private void btnSetMaxVerticalShiftRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.ParallaxView.MaxVerticalShiftRatio = Convert.ToDouble(this.txtMaxVerticalShiftRatio.Text);
                TickForValuesSpy();
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetVerticalSourceStartOffset_Click(object sender, RoutedEventArgs e)
        {
            this.txtVerticalSourceStartOffset.Text = this.ParallaxView.VerticalSourceStartOffset.ToString();
            TickForValuesSpy();
        }

        private void btnSetVerticalSourceStartOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.ParallaxView.VerticalSourceStartOffset = Convert.ToDouble(this.txtVerticalSourceStartOffset.Text);
                TickForValuesSpy();
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetVerticalSourceEndOffset_Click(object sender, RoutedEventArgs e)
        {
            this.txtVerticalSourceEndOffset.Text = this.ParallaxView.VerticalSourceEndOffset.ToString();
        }

        private void btnSetVerticalSourceEndOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.ParallaxView.VerticalSourceEndOffset = Convert.ToDouble(this.txtVerticalSourceEndOffset.Text);
                TickForValuesSpy();
            }
            catch (FormatException)
            {
            }
        }

        private void chkAreVerticalSourceOffsetsRelative_Unchecked(object sender, RoutedEventArgs e)
        {
            this.ParallaxView.VerticalSourceOffsetKind = ParallaxSourceOffsetKind.Absolute;
            TickForValuesSpy();
        }

        private void chkAreVerticalSourceOffsetsRelative_Checked(object sender, RoutedEventArgs e)
        {
            this.ParallaxView.VerticalSourceOffsetKind = ParallaxSourceOffsetKind.Relative;
            TickForValuesSpy();
        }

        private void chkIsVerticalShiftClamped_Unchecked(object sender, RoutedEventArgs e)
        {
            this.ParallaxView.IsVerticalShiftClamped = false;
            TickForValuesSpy();
        }

        private void chkIsVerticalShiftClamped_Checked(object sender, RoutedEventArgs e)
        {
            this.ParallaxView.IsVerticalShiftClamped = true;
            TickForValuesSpy();
        }

        private void btnGetHorizontalOffset_Click(object sender, RoutedEventArgs e)
        {
            if (this.ScrollViewer != null)
            {
                this.txtHorizontalOffset.Text = this.ScrollViewer.HorizontalOffset.ToString();
            }
            else if (this.ScrollPresenter != null)
            {
                this.txtHorizontalOffset.Text = this.ScrollPresenter.HorizontalOffset.ToString();
            }
        }

        private void btnSetHorizontalOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.ScrollViewer != null)
                {
                    this.ScrollViewer.ChangeView(Convert.ToSingle(this.txtHorizontalOffset.Text), null, null, true);
                }
                else if (this.ScrollPresenter != null)
                {
                    this.ScrollPresenter.ScrollTo(
                        Convert.ToSingle(this.txtHorizontalOffset.Text),
                        this.ScrollPresenter.VerticalOffset,
                        new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                }
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            if (this.ScrollViewer != null)
            {
                this.txtVerticalOffset.Text = this.ScrollViewer.VerticalOffset.ToString();
            }
            else if (this.ScrollPresenter != null)
            {
                this.txtVerticalOffset.Text = this.ScrollPresenter.VerticalOffset.ToString();
            }
        }

        private void btnSetVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.ScrollViewer != null)
                {
                    this.ScrollViewer.ChangeView(null, Convert.ToSingle(this.txtVerticalOffset.Text), null, true);
                }
                else if (this.ScrollPresenter != null)
                {
                    this.ScrollPresenter.ScrollTo(
                        this.ScrollPresenter.HorizontalOffset,
                        Convert.ToSingle(this.txtVerticalOffset.Text),
                        new ScrollingScrollOptions(
                            ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                }
            }
            catch (FormatException)
            {
            }
        }

        private void btnGetZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            if (this.ScrollViewer != null)
            {
                this.txtZoomFactor.Text = this.ScrollViewer.ZoomFactor.ToString();
            }
            else if (this.ScrollPresenter != null)
            {
                this.txtZoomFactor.Text = this.ScrollPresenter.ZoomFactor.ToString();
            }
        }

        private void btnSetZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.ScrollViewer != null)
                {
                    this.ScrollViewer.ChangeView(null, null, Convert.ToSingle(this.txtZoomFactor.Text), true);
                }
                else if (this.ScrollPresenter != null)
                {
                    this.ScrollPresenter.ZoomTo(
                        Convert.ToSingle(this.txtZoomFactor.Text),
                        System.Numerics.Vector2.Zero,
                        new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                }
            }
            catch (FormatException)
            {
            }
        }

        private void btnUnparentSource_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                FrameworkElement source = this.ScrollViewer == null ?
                    this.ScrollPresenter as FrameworkElement : 
                    this.ScrollViewer as FrameworkElement;

                if (source != null && source.Parent != null)
                {
                    this.spInner.Children.Remove(source);
                }
            }
            catch
            {
            }
        }

        private void btnReparentSource_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                FrameworkElement source = this.ScrollViewer == null ?
                    this.ScrollPresenter as FrameworkElement : 
                    this.ScrollViewer as FrameworkElement;

                if (source != null && source.Parent == null)
                {
                    this.spInner.Children.Add(source);
                }
            }
            catch
            {
            }
        }

        private void btnCreateParallaxView_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.DynamicParallaxView == null)
                {
                    this.DynamicParallaxView = new ParallaxView();
                    this.spInner.Children.Add(this.DynamicParallaxView);
                }
            }
            catch
            {
            }
        }
        private void btnReleaseParallaxView_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.DynamicParallaxView != null)
                {
                    this.spInner.Children.Remove(this.DynamicParallaxView);
                    this.DynamicParallaxView = null;

                    this.ParallaxView = this.parallaxView;
                    this.cmbParallaxView.SelectedIndex = 0;
                    this.chkAreHorizontalSourceOffsetsRelative.IsChecked = this.ParallaxView.HorizontalSourceOffsetKind == ParallaxSourceOffsetKind.Relative;
                    this.chkAreVerticalSourceOffsetsRelative.IsChecked = this.ParallaxView.VerticalSourceOffsetKind == ParallaxSourceOffsetKind.Relative;
                    this.chkIsHorizontalShiftClamped.IsChecked = this.ParallaxView.IsHorizontalShiftClamped;
                    this.chkIsVerticalShiftClamped.IsChecked = this.ParallaxView.IsVerticalShiftClamped;

                    System.GC.Collect();
                    System.GC.WaitForPendingFinalizers();
                }
            }
            catch
            {
            }
        }

        private void btnRefreshAutomaticHorizontalOffsets_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.DynamicParallaxView != null)
                {
                    this.DynamicParallaxView.RefreshAutomaticHorizontalOffsets();
                }
            }
            catch
            {
            }
        }

        private void btnRefreshAutomaticVerticalOffsets_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (this.DynamicParallaxView != null)
                {
                    this.DynamicParallaxView.RefreshAutomaticVerticalOffsets();
                }
            }
            catch
            {
            }
        }

        private void SetupScrollPresenter()
        {
            LinearGradientBrush threeColorsLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop blueGS = new GradientStop() { Color = Colors.Blue, Offset = 0.0 };
            threeColorsLGB.GradientStops.Add(blueGS);

            GradientStop yellowGS = new GradientStop() { Color = Colors.Yellow, Offset = 0.5 };
            threeColorsLGB.GradientStops.Add(yellowGS);

            GradientStop redGS = new GradientStop() { Color = Colors.Red, Offset = 1.0 };
            threeColorsLGB.GradientStops.Add(redGS);

            this.rectSC = new Rectangle();
            this.rectSC.Width = 400;
            this.rectSC.Height = 900;
            this.rectSC.Fill = threeColorsLGB;

            this.scrollPresenter = new ScrollPresenter();
            this.scrollPresenter.Name = "scrollPresenter";
            this.scrollPresenter.Width = 400;
            this.scrollPresenter.Height = 300;
            this.scrollPresenter.Background = new SolidColorBrush(Windows.UI.Colors.Magenta);
            this.scrollPresenter.Margin = new Thickness(6);
            this.scrollPresenter.HorizontalScrollMode = ScrollingScrollMode.Disabled;
            this.scrollPresenter.VerticalScrollMode = ScrollingScrollMode.Enabled;
            this.scrollPresenter.ZoomMode = ScrollingZoomMode.Enabled;
            this.scrollPresenter.Content = this.rectSC;
            this.scrollPresenter.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter.SetValue(AutomationProperties.NameProperty, "scrollPresenter");

            this.spInner.Children.Add(this.scrollPresenter);

            ComboBoxItem cmbItem = new ComboBoxItem();
            cmbItem.Content = "scrollPresenter";
            cmbItem.SetValue(AutomationProperties.NameProperty, "scrollPresenter");
            cmbSource.Items.Add(cmbItem);
        }
    }
}
