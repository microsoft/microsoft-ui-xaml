// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Windows.Foundation;
using Windows.Graphics.Imaging;
using Windows.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;
using Windows.UI;

using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Windows.Media.Playback;
using Private.Infrastructure;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.Foundation.Graphics.Media
{
    public abstract class AbstractMediaTester<TElement> : FrameworkElementTester<TElement>
        where TElement: FrameworkElement
    {
        protected AbstractMediaTester()
        {
            this.AreTransportControlsEnabled = false;
            this.Source = this.ResourcesPath + "blueframe_video.mp4";
            this.PosterSource = this.ResourcesPath + "Barcelona.jpg";
            this.IsFullWindow = false;
            this.AutoPlay = false;
            this.Stretch = XamlMedia.Stretch.None;
            this.Width = 360.0;
            this.Height = 360.0;
            this.SeekPosition = TimeSpan.FromSeconds(6);
        }

        public bool AreTransportControlsEnabled { get; set; }
        public bool IsFullWindow { get; set; }
        public bool AutoPlay { get; set; }
        public XamlMedia.Stretch Stretch { get; set; }
        public string Source { get; set; }
        public string PosterSource { get; set; }
        public TimeSpan SeekPosition { get; set; }

        private string ResourcesPath
        {
            get
            {
                return XamlTestsBase.Context.TestDeploymentDir + @"\resources\native\external\foundation\graphics\media\";
            }
        }

        protected override void ConfigureElement(StringBuilder sb)
        {
            base.ConfigureElement(sb);

            sb.AppendLine($"  AreTransportControlsEnabled='{this.AreTransportControlsEnabled}'");
            sb.AppendLine($"  IsFullWindow='{this.IsFullWindow}'");
            sb.AppendLine($"  AutoPlay='{this.AutoPlay}'");
            sb.AppendLine($"  Stretch='{this.Stretch}'");

            if (!string.IsNullOrEmpty(this.Source))
            {
                sb.AppendLine($"  Source='{this.Source}'");
            }
            if (!string.IsNullOrEmpty(this.PosterSource))
            {
                sb.AppendLine($"  PosterSource='{this.PosterSource}'");
            }
        }

        private readonly static Color videoColor = ColorHelper.FromArgb(0xFF, 0x02, 0xB0, 0xF2);

        protected async Task TestVideoIsRenderedProtected(UIElement uiElement)
        {
            using(var bitmap = await uiElement.CaptureVisualAsync())
            {
                Verify.IsNotNull(bitmap, "bitmap");
                using (var buffer = new LockedBufferBgra8(bitmap, BitmapBufferAccessMode.Read))
                {
                    var centerPoint = new Point(0.5, 0.5);
                    var centerColor = buffer.GetPixelColor(centerPoint);
                    // Testing the specific color is not 100% reliable as the H264 codec could render the
                    // color slightly different.
                    // However, we log the reported color for future diagnostics.
                    Log.Comment($"Expected Video color is {videoColor}");
                    Log.Comment($"Video color reported is {centerColor}");
                    //Verify.AreEqual(centerColor, videoColor, "centerColor == videoColor");
                    if (centerColor != videoColor)
                    {
                        await bitmap.LogAsync(uiElement.GetType().Name);
                    }
                    Verify.AreNotEqual(centerColor, Colors.Black, "centerColor != Black");
                }
            }
        }
    }
}
