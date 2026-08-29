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

using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Private.Infrastructure;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public abstract class FrameworkElementTester<TElement> : ElementTester<TElement>
        where TElement: FrameworkElement
    {
        protected FrameworkElementTester()
        {
            this.Width = double.NaN;
            this.Height = double.NaN;
        }

        public double Width { get; set; }
        public double Height { get; set; }

        protected override void ConfigureElement(StringBuilder sb)
        {
            if (this.Width != double.NaN)
            {
                sb.AppendLine($"  Width='{this.Width}' ");
            }
            if (this.Height != double.NaN)
            {
                sb.AppendLine($"  Height='{this.Height}' ");
            }
        }
    }
}
