// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using System;
using System.Collections.Generic;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name="Image", Icon="Image.png")]
    public sealed partial class ImagePage : TestPage
    {
        private object asyncEventReportingLock = new object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private int imageIndex = 0;

        public ImagePage()
        {
            this.InitializeComponent();

            image.ImageFailed += Image_ImageFailed;
            image.ImageOpened += Image_ImageOpened;

            Loaded += ImagePage_Loaded;
        }

        private void ImagePage_Loaded(object sender, RoutedEventArgs e)
        {
            UpdateImage();

            BtnGetImageMinWidth_Click(null, null);
            BtnGetImageMaxWidth_Click(null, null);
            BtnGetImageWidth_Click(null, null);
            BtnGetImageActualWidth_Click(null, null);
            BtnGetImageDesiredWidth_Click(null, null);
            BtnGetImageMinHeight_Click(null, null);
            BtnGetImageMaxHeight_Click(null, null);
            BtnGetImageHeight_Click(null, null);
            BtnGetImageActualHeight_Click(null, null);
            BtnGetImageDesiredHeight_Click(null, null);
            BtnGetImageRenderWidth_Click(null, null);
            BtnGetImageRenderHeight_Click(null, null);
            BtnGetImageHorizontalAlignment_Click(null, null);
            BtnGetImageVerticalAlignment_Click(null, null);
            BtnGetImageMargin_Click(null, null);
            BtnGetImageParentWidth_Click(null, null);
            BtnGetImageParentHeight_Click(null, null);
        }

        private void Image_ImageOpened(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("Image_ImageOpened");
        }

        private void Image_ImageFailed(object sender, ExceptionRoutedEventArgs e)
        {
            AppendAsyncEventMessage($"Image_ImageFailed ErrorMessage={e.ErrorMessage}");
        }

        private void ChkImageProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (svImageProperties != null)
                svImageProperties.Visibility = Visibility.Visible;
        }

        private void ChkImageProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svImageProperties != null)
                svImageProperties.Visibility = Visibility.Collapsed;
        }

        private void ChkImageParentProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (svImageParentProperties != null)
                svImageParentProperties.Visibility = Visibility.Visible;
        }

        private void ChkImageParentProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svImageParentProperties != null)
                svImageParentProperties.Visibility = Visibility.Collapsed;
        }

        private void ChkLogs_Checked(object sender, RoutedEventArgs e)
        {
            if (grdLogs != null)
                grdLogs.Visibility = Visibility.Visible;
        }

        private void ChkLogs_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdLogs != null)
                grdLogs.Visibility = Visibility.Collapsed;
        }

        private void BtnGetBitmapImageUriSource_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtBitmapImageUriSource != null)
                {
                    txtBitmapImageUriSource.Text = imageIndex.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetBitmapImageUriSource_Click(object sender, RoutedEventArgs e)
        {
            UpdateImage();
        }

        private void BtnGetBitmapImageDecodePixelWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtBitmapImageDecodePixelWidth != null && image != null)
                {
                    txtBitmapImageDecodePixelWidth.Text = (image.Source as BitmapImage).DecodePixelWidth.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetBitmapImageDecodePixelWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateImage();
        }

        private void BtnGetBitmapImageDecodePixelHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtBitmapImageDecodePixelHeight != null && image != null)
                {
                    txtBitmapImageDecodePixelHeight.Text = (image.Source as BitmapImage).DecodePixelHeight.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetBitmapImageDecodePixelHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateImage();
        }

        private void BtnGetImageStretch_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (cmbImageStretch != null && image != null)
                {
                    cmbImageStretch.SelectedIndex = (int) image.Stretch;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageStretch_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (cmbImageStretch != null && image != null)
                {
                    image.Stretch = (Stretch) cmbImageStretch.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageMinWidth != null && image != null)
                {
                    txtImageMinWidth.Text = image.MinWidth.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageMaxWidth != null && image != null)
                {
                    txtImageMaxWidth.Text = image.MaxWidth.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageWidth != null && image != null)
                {
                    txtImageWidth.Text = image.Width.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageActualWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageActualWidth != null && image != null)
                {
                    txtImageActualWidth.Text = image.ActualWidth.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageDesiredWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageDesiredWidth != null && image != null)
                {
                    txtImageDesiredWidth.Text = image.DesiredSize.Width.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageRenderWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageRenderWidth != null && image != null)
                {
                    txtImageRenderWidth.Text = image.RenderSize.Width.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageMinWidth != null && !string.IsNullOrWhiteSpace(txtImageMinWidth.Text) && image != null)
                {
                    image.MinWidth = double.Parse(txtImageMinWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageMaxWidth != null && !string.IsNullOrWhiteSpace(txtImageMaxWidth.Text) && image != null)
                {
                    image.MaxWidth = double.Parse(txtImageMaxWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageWidth != null && !string.IsNullOrWhiteSpace(txtImageWidth.Text) && image != null)
                {
                    image.Width = double.Parse(txtImageWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageMinHeight != null && image != null)
                {
                    txtImageMinHeight.Text = image.MinHeight.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageMaxHeight != null && image != null)
                {
                    txtImageMaxHeight.Text = image.MaxHeight.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageHeight != null && image != null)
                {
                    txtImageHeight.Text = image.Height.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageActualHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageActualHeight != null && image != null)
                {
                    txtImageActualHeight.Text = image.ActualHeight.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageDesiredHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageDesiredHeight != null && image != null)
                {
                    txtImageDesiredHeight.Text = image.DesiredSize.Height.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageRenderHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageRenderHeight != null && image != null)
                {
                    txtImageRenderHeight.Text = image.RenderSize.Height.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageMinHeight != null && !string.IsNullOrWhiteSpace(txtImageMinHeight.Text) && image != null)
                {
                    image.MinHeight = double.Parse(txtImageMinHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageMaxHeight != null && !string.IsNullOrWhiteSpace(txtImageMaxHeight.Text) && image != null)
                {
                    image.MaxHeight = double.Parse(txtImageMaxHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtImageHeight != null && !string.IsNullOrWhiteSpace(txtImageHeight.Text) && image != null)
                {
                    image.Height = double.Parse(txtImageHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageHorizontalAlignment_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (image != null && cmbImageHorizontalAlignment != null)
                {
                    cmbImageHorizontalAlignment.SelectedIndex = (int) image.HorizontalAlignment;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageVerticalAlignment_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (image != null && cmbImageVerticalAlignment != null)
                {
                    cmbImageVerticalAlignment.SelectedIndex = (int)image.VerticalAlignment;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageHorizontalAlignment_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (image != null && cmbImageHorizontalAlignment != null)
                {
                    image.HorizontalAlignment = (HorizontalAlignment) cmbImageHorizontalAlignment.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageVerticalAlignment_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (image != null && cmbImageVerticalAlignment != null)
                {
                    image.VerticalAlignment = (VerticalAlignment) cmbImageVerticalAlignment.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (image != null && txtImageMargin != null)
                {
                    txtImageMargin.Text = image.Margin.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (image != null && txtImageMargin != null)
                {
                    image.Margin = GetThicknessFromString(txtImageMargin.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageParentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (imagePanel != null && txtImageParentWidth != null)
                {
                    txtImageParentWidth.Text = imagePanel.Width.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetImageParentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (imagePanel != null && txtImageParentHeight != null)
                {
                    txtImageParentHeight.Text = imagePanel.Height.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageParentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (imagePanel != null && txtImageParentWidth != null)
                {
                    imagePanel.Width = double.Parse(txtImageParentWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetImageParentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (imagePanel != null && txtImageParentHeight != null)
                {
                    imagePanel.Height = double.Parse(txtImageParentHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void UpdateImage()
        {
            try
            {
                if (txtBitmapImageUriSource != null &&
                    txtBitmapImageDecodePixelWidth != null &&
                    txtBitmapImageDecodePixelHeight != null)
                {
                    int imageIndexTemp = int.Parse(txtBitmapImageUriSource.Text);

                    if (imageIndexTemp >= 1 && imageIndexTemp <= 126)
                    {
                        imageIndex = imageIndexTemp;

                        Uri uri = new Uri(string.Format("ms-appx:///Images/vette{0}.jpg", imageIndex));

                        BitmapImage bitmapImage = new BitmapImage() {
                            DecodePixelWidth = int.Parse(txtBitmapImageDecodePixelWidth.Text),
                            DecodePixelHeight = int.Parse(txtBitmapImageDecodePixelHeight.Text),
                            UriSource = uri
                        };

                        image.Source = bitmapImage;
                    }
                    else
                    {
                        AppendAsyncEventMessage("Image number out of bounds. Pick a number between 1 and 126.");
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void AppendAsyncEventMessage(string asyncEventMessage)
        {
            lock (asyncEventReportingLock)
            {
                while (asyncEventMessage.Length > 0)
                {
                    string msgHead = asyncEventMessage;

                    if (asyncEventMessage.Length > 110)
                    {
                        int commaIndex = asyncEventMessage.IndexOf(',', 110);
                        if (commaIndex != -1)
                        {
                            msgHead = asyncEventMessage.Substring(0, commaIndex);
                            asyncEventMessage = asyncEventMessage.Substring(commaIndex + 1);
                        }
                        else
                        {
                            asyncEventMessage = string.Empty;
                        }
                    }
                    else
                    {
                        asyncEventMessage = string.Empty;
                    }

                    lstAsyncEventMessage.Add(msgHead);
                }
                var ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Normal, AppendAsyncEventMessage);

            }
        }

        private void AppendAsyncEventMessage()
        {
            lock (asyncEventReportingLock)
            {
                foreach (string asyncEventMessage in lstAsyncEventMessage)
                {
                    lstLogs.Items.Add(asyncEventMessage);
                }
                lstAsyncEventMessage.Clear();
            }
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private Thickness GetThicknessFromString(string thickness)
        {
            string[] lengths = thickness.Split(',');
            if (lengths.Length < 4)
                return new Thickness(
                    Convert.ToDouble(lengths[0]));
            else
                return new Thickness(
                    Convert.ToDouble(lengths[0]), Convert.ToDouble(lengths[1]), Convert.ToDouble(lengths[2]), Convert.ToDouble(lengths[3]));
        }
    }
}
