// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Numerics;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Xaml.Navigation;

using ScrollSnapPointBase = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointBase;
using ScrollSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPoint;
using RepeatedScrollSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.RepeatedScrollSnapPoint;
using ScrollSnapPointsAlignment = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointsAlignment;
using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ScrollingScrollOptions = Microsoft.UI.Xaml.Controls.ScrollingScrollOptions;

using ScrollPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollPresenterTestHooks;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterScrollSnapPointsPage : TestPage
    {
        private const int snapPointColumns = 29;
        private const int snapPointColumnWidth = 10;
        private const int snapPointColumnHeight = 10000;
        private int colorIndex = 0;
        private List<string> fullLogs = new List<string>();

        public ScrollPresenterScrollSnapPointsPage()
        {
            this.InitializeComponent();
            Loaded += ScrollPresenterScrollSnapPointsPage_Loaded;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            if (chkLogScrollPresenterMessages.IsChecked == true)
            {
                MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
            }

            base.OnNavigatedFrom(e);
        }

        private void ScrollPresenterScrollSnapPointsPage_Loaded(object sender, RoutedEventArgs e)
        {
            SolidColorBrush brush = new SolidColorBrush(Colors.Red);
            for (int j = 0; j < snapPointColumns; j++)
            {
                Grid innerGrid = new Grid();
                innerGrid.Width = snapPointColumnWidth;
                innerGrid.Height = snapPointColumnHeight;
                this.stackPanel.Children.Add(innerGrid);
            }
            this.markupScrollPresenter.ViewChanged += MarkupScrollPresenter_ViewChanged;
            this.markupScrollPresenter.StateChanged += MarkupScrollPresenter_StateChanged;
        }

        private void MarkupScrollPresenter_ViewChanged(ScrollPresenter sender, object args)
        {
            this.txtScrollPresenterOffset.Text = this.markupScrollPresenter.VerticalOffset.ToString();
            this.txtScrollPresenterZoomFactor.Text = this.markupScrollPresenter.ZoomFactor.ToString();
        }

        private void MarkupScrollPresenter_StateChanged(ScrollPresenter sender, object args)
        {
            this.txtScrollPresenterState.Text = this.markupScrollPresenter.State.ToString();
        }

        private void BtnMIAddSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtMISnapPointValue.Text);
                ScrollSnapPoint newSnapPoint = new ScrollSnapPoint(value, (ScrollSnapPointsAlignment)cmbMISnapPointAlignment.SelectedIndex);
                Color fillColor = GetNewColor();
                ScrollPresenterTestHooks.SetSnapPointVisualizationColor(newSnapPoint, fillColor);
                markupScrollPresenter.VerticalSnapPoints.Add(newSnapPoint);

                FillSnapPoint(new List<double> { value }, value, value, fillColor);
                FixConsolidatedView();
            }
            catch (Exception ex)
            {
                this.fullLogs.Add(ex.ToString());
                txtExceptionReport.Text = ex.ToString();
            }
        }

#if ApplicableRangeType
        private void BtnOIAddSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtOISnapPointValue.Text);
                double range = Convert.ToDouble(txtOIApplicableRange.Text);
                ScrollSnapPoint newSnapPoint = new ScrollSnapPoint(value, range, ScrollSnapPointsAlignment.Near);
                Color fillColor = GetNewColor();
                ScrollPresenterTestHooks.SetSnapPointVisualizationColor(newSnapPoint, fillColor);
                markupScrollPresenter.VerticalSnapPoints.Add(newSnapPoint);

                FillSnapPoint(new List<double> { value }, (value - range), (value + range), fillColor);
                FixConsolidatedView();
            }
            catch (Exception ex)
            {
                this.fullLogs.Add(ex.ToString());
                txtExceptionReport.Text = ex.ToString();
            }
        }
#endif

        private void BtnMRAddSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double offset = Convert.ToDouble(txtMRSnapPointOffset.Text);
                double interval = Convert.ToDouble(txtMRSnapPointInterval.Text);
                double start = Convert.ToDouble(txtMRSnapPointStart.Text);
                double end = Convert.ToDouble(txtMRSnapPointEnd.Text);
                RepeatedScrollSnapPoint newSnapPoint = new RepeatedScrollSnapPoint(offset, interval, start, end, (ScrollSnapPointsAlignment)cmbMRSnapPointAlignment.SelectedIndex);
                Color fillColor = GetNewColor();
                ScrollPresenterTestHooks.SetSnapPointVisualizationColor(newSnapPoint, fillColor);
                markupScrollPresenter.VerticalSnapPoints.Add(newSnapPoint);

                double value = GetFirstRepeatedSnapPoint(offset, interval, start);
                List<double> values = new List<double>();

                while (value <= end)
                {
                    values.Add(value);
                    value += interval;
                }

                FillSnapPoint(values, start, end, fillColor);
                FixConsolidatedView();
            }
            catch (Exception ex)
            {
                this.fullLogs.Add(ex.ToString());
                txtExceptionReport.Text = ex.ToString();
            }
        }

#if ApplicableRangeType
        private void BtnORAddSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double offset = Convert.ToDouble(txtORSnapPointOffset.Text);
                double interval = Convert.ToDouble(txtORSnapPointInterval.Text);
                double range = Convert.ToDouble(txtORApplicableRange.Text);
                double start = Convert.ToDouble(txtORSnapPointStart.Text);
                double end = Convert.ToDouble(txtORSnapPointEnd.Text);

                RepeatedScrollSnapPoint newSnapPoint = new RepeatedScrollSnapPoint(offset, interval, start, end, range, ScrollSnapPointsAlignment.Near);
                Color fillColor = GetNewColor();
                ScrollPresenterTestHooks.SetSnapPointVisualizationColor(newSnapPoint, fillColor);
                markupScrollPresenter.VerticalSnapPoints.Add(newSnapPoint);

                double value = GetFirstRepeatedSnapPoint(offset, interval, start);
                
                int minColumn = FillSnapPoint(new List<double>(), start, end, fillColor);
                while (value <= end)
                {
                    FillSnapPoint(new List<double> { value }, Math.Max(start, value - range), Math.Min(end, value + range), fillColor, minColumn);
                    value += interval;
                }
                FixConsolidatedView();
            }
            catch (Exception ex)
            {
                this.fullLogs.Add(ex.ToString());
                txtExceptionReport.Text = ex.ToString();
            }
        }
#endif

        private void BtnRemoveFirst_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                markupScrollPresenter.VerticalSnapPoints.RemoveAt(0);
                // TODO: Consider refreshing visualization of snap points
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnRemoveAll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                markupScrollPresenter.VerticalSnapPoints.Clear();
                // TODO: Consider clearing visualization of snap points
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnScrollPresenterOffsetChange_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double changeAmount = Convert.ToDouble(txtScrollPresenterOffsetChange.Text);
                markupScrollPresenter.ScrollFrom(new Vector2(0.0f, (float)((changeAmount * 3) + 30)), null);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnOffsetPlus10With_Click(object sender, RoutedEventArgs e)
        {
            try
            { 
                markupScrollPresenter.ScrollBy(0.0, 10.0, new ScrollingScrollOptions(AnimationMode.Auto, SnapPointsMode.Default));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnOffsetPlus10Without_Click(object sender, RoutedEventArgs e)
        {
            try
            {                
                markupScrollPresenter.ScrollBy(0.0, 10.0, new ScrollingScrollOptions(AnimationMode.Auto, SnapPointsMode.Ignore));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private int FillSnapPoint(List<double> values, double min, double max, int minColumn = 0)
        {
            return FillSnapPoint(values, min, max, GetNewColor(), minColumn);
        }

        private int FillSnapPoint(List<double> values, double min, double max, Color fillColor, int minColumn = 0)
        {
            bool argsAreValid = true;
            foreach (int value in values)
            {
                if (!(min <= value && value <= max))
                {
                    argsAreValid = false;
                    break;
                }
            }
            if (argsAreValid)
            {
                for (int i=minColumn; i < snapPointColumns; i++)
                {
                    Grid snapPointColumn = ((Grid)this.stackPanel.Children[i]);
                    bool isOccupied = false;
                    foreach (Rectangle child in snapPointColumn.Children)
                    {
                        double previousSnapPointMin = child.Margin.Top;
                        double previousSnapPointMax = child.Height + previousSnapPointMin;
                        if ((min >= previousSnapPointMin && min <= previousSnapPointMax) ||
                           (min < previousSnapPointMin && max >= previousSnapPointMin))
                        {
                            isOccupied = true;
                            break;
                        }
                    }
                    if (!isOccupied)
                    {
                        Rectangle rangeRectangle = new Rectangle();
                        rangeRectangle.Width = snapPointColumnWidth;
                        rangeRectangle.Height = max - min;
                        rangeRectangle.Margin = new Thickness(0, min, 0, 0);
                        rangeRectangle.Fill = new SolidColorBrush(fillColor);
                        rangeRectangle.VerticalAlignment = VerticalAlignment.Top;
                        snapPointColumn.Children.Add(rangeRectangle);

                        foreach (int value in values)
                        {
                            Rectangle pointRectangle = new Rectangle();
                            pointRectangle.Width = snapPointColumnWidth;
                            pointRectangle.Height = 1;
                            pointRectangle.Margin = new Thickness(0, value, 0, 0);
                            pointRectangle.Fill = new SolidColorBrush(Colors.Black);
                            pointRectangle.VerticalAlignment = VerticalAlignment.Top;
                            snapPointColumn.Children.Add(pointRectangle);
                        }
                        return i;
                    }
                }
                this.fullLogs.Add("Ran out of room to display additional snap points.");
            }
            else
            {
                throw (new ArgumentException(string.Format("The following inequality must be true: min <= All Values <= max. {0} <= {1} <= {2}", min, values.ToString(), max)));
            }
            return -1;
        }

        private void FixConsolidatedView()
        {
            consolidatedView.Children.Clear();
            foreach (ScrollSnapPointBase snapPoint in ScrollPresenterTestHooks.GetConsolidatedVerticalScrollSnapPoints(markupScrollPresenter))
            {
                Vector2 zone = ScrollPresenterTestHooks.GetVerticalSnapPointActualApplicableZone(markupScrollPresenter, snapPoint);
                zone.X = Math.Max(0, zone.X);
                zone.Y = Math.Min(snapPointColumnHeight, zone.Y);
                Rectangle rangeRectangle = new Rectangle();
                rangeRectangle.Width = snapPointColumnWidth - 1;
                rangeRectangle.Height = zone.Y - zone.X;
                rangeRectangle.Margin = new Thickness(0, zone.X, 0, 0);
                rangeRectangle.Fill = new SolidColorBrush(ScrollPresenterTestHooks.GetSnapPointVisualizationColor(snapPoint));
                rangeRectangle.VerticalAlignment = VerticalAlignment.Top;
                consolidatedView.Children.Add(rangeRectangle);
            }
        }

        private double GetFirstRepeatedSnapPoint(double offset, double interval, double start)
        {
            Debug.Assert(offset >= start);
            Debug.Assert(interval > 0);

            return offset - (int)((offset - start) / interval) * interval;
        }

        private Color GetNewColor()
        {
            colorIndex++;
            switch (colorIndex)
            {
                case 0:
                    return Colors.YellowGreen;
                case 1:
                    return Colors.AliceBlue;
                case 2:
                    return Colors.AntiqueWhite;
                case 3:
                    return Colors.Aqua;
                case 4:
                    return Colors.Aquamarine;
                case 5:
                    return Colors.Azure;
                case 6:
                    return Colors.Beige;
                case 7:
                    return Colors.Bisque;
                case 8:
                    return Colors.BlanchedAlmond;
                case 9:
                    return Colors.Blue;
                case 10:
                    return Colors.BlueViolet;
                case 11:
                    return Colors.Brown;
                case 12:
                    return Colors.BurlyWood;
                case 13:
                    return Colors.CadetBlue;
                case 14:
                    return Colors.Chartreuse;
                case 15:
                    return Colors.Chocolate;
                case 16:
                    return Colors.Coral;
                case 17:
                    return Colors.CornflowerBlue;
                case 18:
                    return Colors.Cornsilk;
                case 19:
                    return Colors.Crimson;
                case 20:
                    return Colors.Cyan;
                case 21:
                    return Colors.DarkBlue;
                case 22:
                    return Colors.DarkCyan;
                case 23:
                    return Colors.DarkGoldenrod;
                case 24:
                    return Colors.DarkGray;
                case 25:
                    return Colors.DarkGreen;
                case 26:
                    return Colors.DarkKhaki;
                case 27:
                    return Colors.DarkMagenta;
                case 28:
                    return Colors.DarkOliveGreen;
                case 29:
                    return Colors.DarkOrange;
                case 30:
                    return Colors.DarkOrchid;
                case 31:
                    return Colors.DarkRed;
                case 32:
                    return Colors.DarkSalmon;
                case 33:
                    return Colors.DarkSeaGreen;
                case 34:
                    return Colors.DarkSlateBlue;
                case 35:
                    return Colors.DarkSlateGray;
                case 36:
                    return Colors.DarkTurquoise;
                case 37:
                    return Colors.DarkViolet;
                case 38:
                    return Colors.DeepPink;
                case 39:
                    return Colors.DeepSkyBlue;
                case 40:
                    return Colors.DimGray;
                case 41:
                    return Colors.DodgerBlue;
                case 42:
                    return Colors.Firebrick;
                case 43:
                    return Colors.FloralWhite;
                case 44:
                    return Colors.ForestGreen;
                case 45:
                    return Colors.Fuchsia;
                case 46:
                    return Colors.Gainsboro;
                case 47:
                    return Colors.GhostWhite;
                case 48:
                    return Colors.Gold;
                case 49:
                    return Colors.Goldenrod;
                case 50:
                    return Colors.Gray;
                case 51:
                    return Colors.Green;
                case 52:
                    return Colors.GreenYellow;
                case 53:
                    return Colors.Honeydew;
                case 54:
                    return Colors.HotPink;
                case 55:
                    return Colors.IndianRed;
                case 56:
                    return Colors.Indigo;
                case 57:
                    return Colors.Ivory;
                case 58:
                    return Colors.Khaki;
                case 59:
                    return Colors.Lavender;
                case 60:
                    return Colors.LavenderBlush;
                case 61:
                    return Colors.LawnGreen;
                case 62:
                    return Colors.LemonChiffon;
                case 63:
                    return Colors.LightBlue;
                case 64:
                    return Colors.LightCoral;
                case 65:
                    return Colors.LightCyan;
                case 66:
                    return Colors.LightGoldenrodYellow;
                case 67:
                    return Colors.LightGray;
                case 68:
                    return Colors.LightGreen;
                case 69:
                    return Colors.LightPink;
                case 70:
                    return Colors.LightSalmon;
                case 71:
                    return Colors.LightSeaGreen;
                case 72:
                    return Colors.LightSkyBlue;
                case 73:
                    return Colors.LightSlateGray;
                case 74:
                    return Colors.LightSteelBlue;
                case 75:
                    return Colors.LightYellow;
                case 76:
                    return Colors.Lime;
                case 77:
                    return Colors.LimeGreen;
                case 78:
                    return Colors.Linen;
                case 79:
                    return Colors.Magenta;
                case 80:
                    return Colors.Maroon;
                case 81:
                    return Colors.MediumAquamarine;
                case 82:
                    return Colors.MediumBlue;
                case 83:
                    return Colors.MediumOrchid;
                case 84:
                    return Colors.MediumPurple;
                case 85:
                    return Colors.MediumSeaGreen;
                case 86:
                    return Colors.MediumSlateBlue;
                case 87:
                    return Colors.MediumSpringGreen;
                case 88:
                    return Colors.MediumTurquoise;
                case 89:
                    return Colors.MediumVioletRed;
                case 90:
                    return Colors.MidnightBlue;
                case 91:
                    return Colors.MintCream;
                case 92:
                    return Colors.MistyRose;
                case 93:
                    return Colors.Moccasin;
                case 94:
                    return Colors.NavajoWhite;
                case 95:
                    return Colors.Navy;
                case 96:
                    return Colors.OldLace;
                case 97:
                    return Colors.Olive;
                case 98:
                    return Colors.OliveDrab;
                case 99:
                    return Colors.Orange;
                case 100:
                    return Colors.OrangeRed;
                case 101:
                    return Colors.Orchid;
                case 102:
                    return Colors.PaleGoldenrod;
                case 103:
                    return Colors.PaleGreen;
                case 104:
                    return Colors.PaleTurquoise;
                case 105:
                    return Colors.PaleVioletRed;
                case 106:
                    return Colors.PapayaWhip;
                case 107:
                    return Colors.PeachPuff;
                case 108:
                    return Colors.Peru;
                case 109:
                    return Colors.Pink;
                case 110:
                    return Colors.Plum;
                case 111:
                    return Colors.PowderBlue;
                case 112:
                    return Colors.Purple;
                case 113:
                    return Colors.Red;
                case 114:
                    return Colors.RosyBrown;
                case 115:
                    return Colors.RoyalBlue;
                case 116:
                    return Colors.SaddleBrown;
                case 117:
                    return Colors.Salmon;
                case 118:
                    return Colors.SandyBrown;
                case 119:
                    return Colors.SeaGreen;
                case 120:
                    return Colors.SeaShell;
                case 121:
                    return Colors.Sienna;
                case 122:
                    return Colors.Silver;
                case 123:
                    return Colors.SkyBlue;
                case 124:
                    return Colors.SlateBlue;
                case 125:
                    return Colors.SlateGray;
                case 126:
                    return Colors.Snow;
                case 127:
                    return Colors.SpringGreen;
                case 128:
                    return Colors.Tan;
                case 129:
                    return Colors.Teal;
                case 130:
                    return Colors.Thistle;
                case 131:
                    return Colors.Tomato;
                case 132:
                    return Colors.Turquoise;
                case 133:
                    return Colors.Violet;
                case 134:
                    return Colors.Wheat;
                case 135:
                    return Colors.WhiteSmoke;
                case 136:
                    return Colors.Yellow;
                case 137:
                    return Colors.YellowGreen;
                default:
                    return Colors.Black;
            }
        }

        private void ChkLogScrollPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            //Turn on info and verbose logging for the ScrollPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            //Turn off info and verbose logging for the ScrollPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string senderName = string.Empty;
            FrameworkElement fe = sender as FrameworkElement;

            if (fe != null)
            {
                senderName = "s:" + fe.Name + ", ";
            }

            if (args.IsVerboseLevel)
            {
                this.fullLogs.Add("Verbose: " + senderName + "m:" + msg);
            }
            else
            {
                this.fullLogs.Add("Info: " + senderName + "m:" + msg);
            }
        }

        private void btnGetFullLog_Click(object sender, RoutedEventArgs e)
        {
            foreach (string log in this.fullLogs)
            {
                this.cmbFullLog.Items.Add(log);
            }
        }

        private void btnClearFullLog_Click(object sender, RoutedEventArgs e)
        {
            this.fullLogs.Clear();
            this.cmbFullLog.Items.Clear();
        }
    }
}

