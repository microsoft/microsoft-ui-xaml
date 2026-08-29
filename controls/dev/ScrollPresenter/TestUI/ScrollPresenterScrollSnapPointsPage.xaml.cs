// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Numerics;
using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Private.Controls;

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
            SolidColorBrush brush = new SolidColorBrush(Microsoft.UI.Colors.Red);
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
            this.txtScrollPresenterOffset.Text = this.markupScrollPresenter.VerticalOffset.ToString("F2");
            this.txtScrollPresenterZoomFactor.Text = this.markupScrollPresenter.ZoomFactor.ToString("F2");
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
                markupScrollPresenter.AddScrollVelocity(new Vector2(0.0f, (float)((changeAmount * 3) + 30)), null);
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
                markupScrollPresenter.ScrollBy(0.0, 10.0, new ScrollingScrollOptions(ScrollingAnimationMode.Auto, ScrollingSnapPointsMode.Default));
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
                markupScrollPresenter.ScrollBy(0.0, 10.0, new ScrollingScrollOptions(ScrollingAnimationMode.Auto, ScrollingSnapPointsMode.Ignore));
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
                for (int i = minColumn; i < snapPointColumns; i++)
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
                            pointRectangle.Fill = new SolidColorBrush(Microsoft.UI.Colors.Black);
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
                    return Microsoft.UI.Colors.YellowGreen;
                case 1:
                    return Microsoft.UI.Colors.AliceBlue;
                case 2:
                    return Microsoft.UI.Colors.AntiqueWhite;
                case 3:
                    return Microsoft.UI.Colors.Aqua;
                case 4:
                    return Microsoft.UI.Colors.Aquamarine;
                case 5:
                    return Microsoft.UI.Colors.Azure;
                case 6:
                    return Microsoft.UI.Colors.Beige;
                case 7:
                    return Microsoft.UI.Colors.Bisque;
                case 8:
                    return Microsoft.UI.Colors.BlanchedAlmond;
                case 9:
                    return Microsoft.UI.Colors.Blue;
                case 10:
                    return Microsoft.UI.Colors.BlueViolet;
                case 11:
                    return Microsoft.UI.Colors.Brown;
                case 12:
                    return Microsoft.UI.Colors.BurlyWood;
                case 13:
                    return Microsoft.UI.Colors.CadetBlue;
                case 14:
                    return Microsoft.UI.Colors.Chartreuse;
                case 15:
                    return Microsoft.UI.Colors.Chocolate;
                case 16:
                    return Microsoft.UI.Colors.Coral;
                case 17:
                    return Microsoft.UI.Colors.CornflowerBlue;
                case 18:
                    return Microsoft.UI.Colors.Cornsilk;
                case 19:
                    return Microsoft.UI.Colors.Crimson;
                case 20:
                    return Microsoft.UI.Colors.Cyan;
                case 21:
                    return Microsoft.UI.Colors.DarkBlue;
                case 22:
                    return Microsoft.UI.Colors.DarkCyan;
                case 23:
                    return Microsoft.UI.Colors.DarkGoldenrod;
                case 24:
                    return Microsoft.UI.Colors.DarkGray;
                case 25:
                    return Microsoft.UI.Colors.DarkGreen;
                case 26:
                    return Microsoft.UI.Colors.DarkKhaki;
                case 27:
                    return Microsoft.UI.Colors.DarkMagenta;
                case 28:
                    return Microsoft.UI.Colors.DarkOliveGreen;
                case 29:
                    return Microsoft.UI.Colors.DarkOrange;
                case 30:
                    return Microsoft.UI.Colors.DarkOrchid;
                case 31:
                    return Microsoft.UI.Colors.DarkRed;
                case 32:
                    return Microsoft.UI.Colors.DarkSalmon;
                case 33:
                    return Microsoft.UI.Colors.DarkSeaGreen;
                case 34:
                    return Microsoft.UI.Colors.DarkSlateBlue;
                case 35:
                    return Microsoft.UI.Colors.DarkSlateGray;
                case 36:
                    return Microsoft.UI.Colors.DarkTurquoise;
                case 37:
                    return Microsoft.UI.Colors.DarkViolet;
                case 38:
                    return Microsoft.UI.Colors.DeepPink;
                case 39:
                    return Microsoft.UI.Colors.DeepSkyBlue;
                case 40:
                    return Microsoft.UI.Colors.DimGray;
                case 41:
                    return Microsoft.UI.Colors.DodgerBlue;
                case 42:
                    return Microsoft.UI.Colors.Firebrick;
                case 43:
                    return Microsoft.UI.Colors.FloralWhite;
                case 44:
                    return Microsoft.UI.Colors.ForestGreen;
                case 45:
                    return Microsoft.UI.Colors.Fuchsia;
                case 46:
                    return Microsoft.UI.Colors.Gainsboro;
                case 47:
                    return Microsoft.UI.Colors.GhostWhite;
                case 48:
                    return Microsoft.UI.Colors.Gold;
                case 49:
                    return Microsoft.UI.Colors.Goldenrod;
                case 50:
                    return Microsoft.UI.Colors.Gray;
                case 51:
                    return Microsoft.UI.Colors.Green;
                case 52:
                    return Microsoft.UI.Colors.GreenYellow;
                case 53:
                    return Microsoft.UI.Colors.Honeydew;
                case 54:
                    return Microsoft.UI.Colors.HotPink;
                case 55:
                    return Microsoft.UI.Colors.IndianRed;
                case 56:
                    return Microsoft.UI.Colors.Indigo;
                case 57:
                    return Microsoft.UI.Colors.Ivory;
                case 58:
                    return Microsoft.UI.Colors.Khaki;
                case 59:
                    return Microsoft.UI.Colors.Lavender;
                case 60:
                    return Microsoft.UI.Colors.LavenderBlush;
                case 61:
                    return Microsoft.UI.Colors.LawnGreen;
                case 62:
                    return Microsoft.UI.Colors.LemonChiffon;
                case 63:
                    return Microsoft.UI.Colors.LightBlue;
                case 64:
                    return Microsoft.UI.Colors.LightCoral;
                case 65:
                    return Microsoft.UI.Colors.LightCyan;
                case 66:
                    return Microsoft.UI.Colors.LightGoldenrodYellow;
                case 67:
                    return Microsoft.UI.Colors.LightGray;
                case 68:
                    return Microsoft.UI.Colors.LightGreen;
                case 69:
                    return Microsoft.UI.Colors.LightPink;
                case 70:
                    return Microsoft.UI.Colors.LightSalmon;
                case 71:
                    return Microsoft.UI.Colors.LightSeaGreen;
                case 72:
                    return Microsoft.UI.Colors.LightSkyBlue;
                case 73:
                    return Microsoft.UI.Colors.LightSlateGray;
                case 74:
                    return Microsoft.UI.Colors.LightSteelBlue;
                case 75:
                    return Microsoft.UI.Colors.LightYellow;
                case 76:
                    return Microsoft.UI.Colors.Lime;
                case 77:
                    return Microsoft.UI.Colors.LimeGreen;
                case 78:
                    return Microsoft.UI.Colors.Linen;
                case 79:
                    return Microsoft.UI.Colors.Magenta;
                case 80:
                    return Microsoft.UI.Colors.Maroon;
                case 81:
                    return Microsoft.UI.Colors.MediumAquamarine;
                case 82:
                    return Microsoft.UI.Colors.MediumBlue;
                case 83:
                    return Microsoft.UI.Colors.MediumOrchid;
                case 84:
                    return Microsoft.UI.Colors.MediumPurple;
                case 85:
                    return Microsoft.UI.Colors.MediumSeaGreen;
                case 86:
                    return Microsoft.UI.Colors.MediumSlateBlue;
                case 87:
                    return Microsoft.UI.Colors.MediumSpringGreen;
                case 88:
                    return Microsoft.UI.Colors.MediumTurquoise;
                case 89:
                    return Microsoft.UI.Colors.MediumVioletRed;
                case 90:
                    return Microsoft.UI.Colors.MidnightBlue;
                case 91:
                    return Microsoft.UI.Colors.MintCream;
                case 92:
                    return Microsoft.UI.Colors.MistyRose;
                case 93:
                    return Microsoft.UI.Colors.Moccasin;
                case 94:
                    return Microsoft.UI.Colors.NavajoWhite;
                case 95:
                    return Microsoft.UI.Colors.Navy;
                case 96:
                    return Microsoft.UI.Colors.OldLace;
                case 97:
                    return Microsoft.UI.Colors.Olive;
                case 98:
                    return Microsoft.UI.Colors.OliveDrab;
                case 99:
                    return Microsoft.UI.Colors.Orange;
                case 100:
                    return Microsoft.UI.Colors.OrangeRed;
                case 101:
                    return Microsoft.UI.Colors.Orchid;
                case 102:
                    return Microsoft.UI.Colors.PaleGoldenrod;
                case 103:
                    return Microsoft.UI.Colors.PaleGreen;
                case 104:
                    return Microsoft.UI.Colors.PaleTurquoise;
                case 105:
                    return Microsoft.UI.Colors.PaleVioletRed;
                case 106:
                    return Microsoft.UI.Colors.PapayaWhip;
                case 107:
                    return Microsoft.UI.Colors.PeachPuff;
                case 108:
                    return Microsoft.UI.Colors.Peru;
                case 109:
                    return Microsoft.UI.Colors.Pink;
                case 110:
                    return Microsoft.UI.Colors.Plum;
                case 111:
                    return Microsoft.UI.Colors.PowderBlue;
                case 112:
                    return Microsoft.UI.Colors.Purple;
                case 113:
                    return Microsoft.UI.Colors.Red;
                case 114:
                    return Microsoft.UI.Colors.RosyBrown;
                case 115:
                    return Microsoft.UI.Colors.RoyalBlue;
                case 116:
                    return Microsoft.UI.Colors.SaddleBrown;
                case 117:
                    return Microsoft.UI.Colors.Salmon;
                case 118:
                    return Microsoft.UI.Colors.SandyBrown;
                case 119:
                    return Microsoft.UI.Colors.SeaGreen;
                case 120:
                    return Microsoft.UI.Colors.SeaShell;
                case 121:
                    return Microsoft.UI.Colors.Sienna;
                case 122:
                    return Microsoft.UI.Colors.Silver;
                case 123:
                    return Microsoft.UI.Colors.SkyBlue;
                case 124:
                    return Microsoft.UI.Colors.SlateBlue;
                case 125:
                    return Microsoft.UI.Colors.SlateGray;
                case 126:
                    return Microsoft.UI.Colors.Snow;
                case 127:
                    return Microsoft.UI.Colors.SpringGreen;
                case 128:
                    return Microsoft.UI.Colors.Tan;
                case 129:
                    return Microsoft.UI.Colors.Teal;
                case 130:
                    return Microsoft.UI.Colors.Thistle;
                case 131:
                    return Microsoft.UI.Colors.Tomato;
                case 132:
                    return Microsoft.UI.Colors.Turquoise;
                case 133:
                    return Microsoft.UI.Colors.Violet;
                case 134:
                    return Microsoft.UI.Colors.Wheat;
                case 135:
                    return Microsoft.UI.Colors.WhiteSmoke;
                case 136:
                    return Microsoft.UI.Colors.Yellow;
                case 137:
                    return Microsoft.UI.Colors.YellowGreen;
                default:
                    return Microsoft.UI.Colors.Black;
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

