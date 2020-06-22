// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation;
using Windows.UI.Composition;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;
using Common;

using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using ScrollingScrollMode = Microsoft.UI.Xaml.Controls.ScrollingScrollMode;

namespace MUXControlsTestApp
{
    public sealed partial class SimpleRectanglePage : TestPage
    {
        public SimpleRectanglePage()
        {
            this.InitializeComponent();
            this.AnimatedValuesSpy = null;
            this.HorizontalOffsetAnimation1 = null;
            this.HorizontalOffsetAnimation2 = null;
            this.HasDirectManipulation = false;
            this.UIThreadTicksForValuesSpy = 0;
            this.IsRenderingHooked = false;

            this.sv.ViewChanged += ScrollViewer_ViewChanged;
            this.sv.DirectManipulationStarted += ScrollViewer_DirectManipulationStarted;
            this.sv.DirectManipulationCompleted += ScrollViewer_DirectManipulationCompleted;

            this.VerticalOffsetAnimation1 = null;
            this.VerticalOffsetAnimation2 = null;
            SetupScrollPresenter();

            SetupAnimatedValuesSpy();
            TickForValuesSpy();
        }

        private CompositionPropertySet AnimatedValuesSpy
        {
            get;
            set;
        }

        private ExpressionAnimation HorizontalOffsetAnimation1
        {
            get;
            set;
        }

        private ExpressionAnimation HorizontalOffsetAnimation2
        {
            get;
            set;
        }

        private ExpressionAnimation VerticalOffsetAnimation1
        {
            get;
            set;
        }

        private ExpressionAnimation VerticalOffsetAnimation2
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
            string visualHorizontalTargetedPropertyName =
                PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2) ? "visual.Translation.X" : "visual.TransformMatrix._41";

            if (this.parallaxView1 != null && this.parallaxView1.Child is UIElement)
            {
                UIElement ppChild = this.parallaxView1.Child as UIElement;
                Visual visualPPChild = ElementCompositionPreview.GetElementVisual(ppChild);
                Compositor compositor = visualPPChild.Compositor;

                if (this.AnimatedValuesSpy == null)
                {
                    this.AnimatedValuesSpy = compositor.CreatePropertySet();
                }

                this.AnimatedValuesSpy.InsertScalar("HorizontalOffset1", 0.0f);

                this.HorizontalOffsetAnimation1 = compositor.CreateExpressionAnimation(visualHorizontalTargetedPropertyName);
                this.HorizontalOffsetAnimation1.SetReferenceParameter("visual", visualPPChild);
            }

            if (this.parallaxView2 != null && this.parallaxView2.Child is UIElement)
            {
                UIElement ppChild = this.parallaxView2.Child as UIElement;
                Visual visualPPChild = ElementCompositionPreview.GetElementVisual(ppChild);
                Compositor compositor = visualPPChild.Compositor;

                if (this.AnimatedValuesSpy == null)
                {
                    this.AnimatedValuesSpy = compositor.CreatePropertySet();
                }

                this.AnimatedValuesSpy.InsertScalar("HorizontalOffset2", 0.0f);

                this.HorizontalOffsetAnimation2 = compositor.CreateExpressionAnimation(visualHorizontalTargetedPropertyName);
                this.HorizontalOffsetAnimation2.SetReferenceParameter("visual", visualPPChild);
            }

            string visualVerticalTargetedPropertyName =
                PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2) ? "visual.Translation.Y" : "visual.TransformMatrix._42";

            if (this.parallaxView3 != null && this.parallaxView3.Child is UIElement)
            {
                UIElement ppChild = this.parallaxView3.Child as UIElement;
                Visual visualPPChild = ElementCompositionPreview.GetElementVisual(ppChild);
                Compositor compositor = visualPPChild.Compositor;

                if (this.AnimatedValuesSpy == null)
                {
                    this.AnimatedValuesSpy = compositor.CreatePropertySet();
                }

                this.AnimatedValuesSpy.InsertScalar("VerticalOffset1", 0.0f);

                this.VerticalOffsetAnimation1 = compositor.CreateExpressionAnimation(visualVerticalTargetedPropertyName);
                this.VerticalOffsetAnimation1.SetReferenceParameter("visual", visualPPChild);
            }

            if (this.parallaxView4 != null && this.parallaxView4.Child is UIElement)
            {
                UIElement ppChild = this.parallaxView4.Child as UIElement;
                Visual visualPPChild = ElementCompositionPreview.GetElementVisual(ppChild);
                Compositor compositor = visualPPChild.Compositor;

                if (this.AnimatedValuesSpy == null)
                {
                    this.AnimatedValuesSpy = compositor.CreatePropertySet();
                }

                this.AnimatedValuesSpy.InsertScalar("VerticalOffset2", 0.0f);

                this.VerticalOffsetAnimation2 = compositor.CreateExpressionAnimation(visualVerticalTargetedPropertyName);
                this.VerticalOffsetAnimation2.SetReferenceParameter("visual", visualPPChild);
            }

            CheckSpyingTicksRequirement();
        }

        private void StartAnimatedValuesSpy()
        {
            if (this.AnimatedValuesSpy != null)
            {
                this.AnimatedValuesSpy.StartAnimation("HorizontalOffset1", this.HorizontalOffsetAnimation1);
                this.AnimatedValuesSpy.StartAnimation("HorizontalOffset2", this.HorizontalOffsetAnimation2);
                this.AnimatedValuesSpy.StartAnimation("VerticalOffset1", this.VerticalOffsetAnimation1);
                this.AnimatedValuesSpy.StartAnimation("VerticalOffset2", this.VerticalOffsetAnimation2);
            }
        }

        private void StopAnimatedValuesSpy()
        {
            if (this.AnimatedValuesSpy != null)
            {
                this.AnimatedValuesSpy.StopAnimation("HorizontalOffset1");
                this.AnimatedValuesSpy.StopAnimation("HorizontalOffset2");
                this.AnimatedValuesSpy.StopAnimation("VerticalOffset1");
                this.AnimatedValuesSpy.StopAnimation("VerticalOffset2");
            }
        }

        private void CheckSpyingTicksRequirement()
        {
            if (this.sv != null &&
                this.AnimatedValuesSpy != null &&
                (this.HasDirectManipulation || this.UIThreadTicksForValuesSpy > 0) &&
                (this.sv.HorizontalOffset == 0 || this.sv.HorizontalOffset == this.sv.ScrollableWidth))
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

        private void TickForValuesSpy()
        {
            this.UIThreadTicksForValuesSpy = 8;
            CheckSpyingTicksRequirement();
        }

        private void SpyAnimatedValues()
        {
            if (this.AnimatedValuesSpy != null)
            {
                StopAnimatedValuesSpy();

                float offset;
                CompositionGetValueStatus status;

                status = this.AnimatedValuesSpy.TryGetScalar("HorizontalOffset1", out offset);
                if (CompositionGetValueStatus.Succeeded == status)
                {
                    this.tbHorizontalOutput1.Text = offset.ToString();
                }
                else
                {
                    this.tbHorizontalOutput1.Text = "status=" + status.ToString();
                }
                System.Diagnostics.Debug.WriteLine("Spied value 1: " + this.tbHorizontalOutput1.Text);
                
                status = this.AnimatedValuesSpy.TryGetScalar("HorizontalOffset2", out offset);
                if (CompositionGetValueStatus.Succeeded == status)
                {
                    this.tbHorizontalOutput2.Text = offset.ToString();
                }
                else
                {
                    this.tbHorizontalOutput2.Text = "status=" + status.ToString();
                }
                System.Diagnostics.Debug.WriteLine("Spied value 2: " + this.tbHorizontalOutput2.Text);

                status = this.AnimatedValuesSpy.TryGetScalar("VerticalOffset1", out offset);
                if (CompositionGetValueStatus.Succeeded == status)
                {
                    this.tbVerticalOutput1.Text = offset.ToString();
                }
                else
                {
                    this.tbVerticalOutput1.Text = "status=" + status.ToString();
                }
                System.Diagnostics.Debug.WriteLine("Spied value 3: " + this.tbVerticalOutput1.Text);

                status = this.AnimatedValuesSpy.TryGetScalar("VerticalOffset2", out offset);
                if (CompositionGetValueStatus.Succeeded == status)
                {
                    this.tbVerticalOutput2.Text = offset.ToString();
                }
                else
                {
                    this.tbVerticalOutput2.Text = "status=" + status.ToString();
                }
                System.Diagnostics.Debug.WriteLine("Spied value 4: " + this.tbVerticalOutput2.Text);

                StartAnimatedValuesSpy();
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

        private void SetupScrollPresenter()
        {
            LinearGradientBrush threeColorsLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop blueGS = new GradientStop() { Color = Colors.Blue, Offset = 0.0 };
            threeColorsLGB.GradientStops.Add(blueGS);

            GradientStop yellowGS = new GradientStop() { Color = Colors.Yellow, Offset = 0.5 };
            threeColorsLGB.GradientStops.Add(yellowGS);

            GradientStop redGS = new GradientStop() { Color = Colors.Red, Offset = 1.0 };
            threeColorsLGB.GradientStops.Add(redGS);

            Rectangle rectSC = new Rectangle();
            rectSC.Width = 1000;
            rectSC.Height = 900;
            rectSC.Fill = threeColorsLGB;

            ScrollPresenter s = new ScrollPresenter();
            s.Name = "s";
            s.Width = 500;
            s.Height = 300;
            s.Background = new SolidColorBrush(Windows.UI.Colors.LightBlue);
            s.Margin = new Thickness(4);
            s.HorizontalScrollMode = ScrollingScrollMode.Enabled;
            s.VerticalScrollMode = ScrollingScrollMode.Enabled;
            s.Content = rectSC;
            s.ViewChanged += ScrollPresenter_ViewChanged;

            sp.Children.Insert(0, s);

            parallaxView3.Source = s;
            parallaxView4.Source = s;
        }

        private void ScrollPresenter_ViewChanged(ScrollPresenter sender, object args)
        {
            SpyAnimatedValues();
        }
    }
}