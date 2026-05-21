// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;
using WF = Windows.Foundation;
using SD = System.Drawing;
using Xaml = Microsoft.UI.Xaml;

namespace Microsoft.Windows.Interop
{
    /// <summary>
    ///     A Windows Forms control that can be used to host XAML content
    /// </summary>
    partial class XamlContentHost
    {
        /// <summary>
        ///     Overrides the base class implementation of GetPreferredSize to provide
        ///     correct layout behavior for the hosted XAML content.
        /// </summary>
        public override SD.Size GetPreferredSize(SD.Size proposedSize)
        {
            traceSource.TraceEvent(TraceEventType.Verbose, 0, "GetPreferredSize: ProposedSize: {0}", proposedSize);

            if (DesignMode)
            {
                return Size;
            }

            if (this.xamlBridge.Content != null)
            {
                double proposedWidth = proposedSize.Width;
                double proposedHeight = proposedSize.Height;

                // DockStyles will result in a constraint of 1 on the Docked axis. GetPreferredSize
                // must convert this into an unconstrained value.
                if (proposedSize.Height == Int32.MaxValue || proposedSize.Height == 1)
                {
                    proposedHeight = double.PositiveInfinity;
                }

                if (proposedSize.Width == Int32.MaxValue || proposedSize.Width == 1)
                {
                    proposedWidth = double.PositiveInfinity;
                }

                this.xamlBridge.Content.Measure(new WF.Size(proposedWidth, proposedHeight));
            }

            SD.Size preferredSize = SD.Size.Empty;
            if (this.xamlBridge.Content != null)
            {
                preferredSize = new SD.Size((int)this.xamlBridge.Content.DesiredSize.Width, (int)this.xamlBridge.Content.DesiredSize.Height); 
            }

            traceSource.TraceEvent(TraceEventType.Verbose, 0, "GetPreferredSize: PreferredSize: {0}", preferredSize);

            return preferredSize;
        }

        /// <summary>
        ///     Gets XAML content's 'DesiredSize' post-Measure. Called by 
        ///     XamlContentHost's XAML LayoutUpdated event handler.
        /// </summary>
        private Size GetRootXamlElementDesiredSize()
        {
            Size desiredSize = new Size();

            desiredSize.Height = (int)this.xamlBridge.Content.DesiredSize.Height;
            desiredSize.Width = (int)this.xamlBridge.Content.DesiredSize.Width;

            traceSource.TraceEvent(TraceEventType.Verbose, 0, "GetRootXamlElementDesiredSize: {0}", desiredSize);

            return desiredSize;
        }

        /// <summary>
        ///     Gets the default size of the control.
        /// </summary>
        protected override global::System.Drawing.Size DefaultSize
        {
            get
            {
                // XamlContentHost's DefaultSize is 0, 0 
                Size defaultSize = Size.Empty;

                traceSource.TraceEvent(TraceEventType.Verbose, 0, "DefaultSize.get: {0}", defaultSize);

                return defaultSize;
            }
        }
         
        /// <summary>
        /// Called when the location of the host Control changes
        /// </summary>
        /// <param name="e">EventArgs</param>
        protected override void OnLocationChanged(EventArgs e)
        {
            traceSource.TraceEvent(global::System.Diagnostics.TraceEventType.Verbose, 0, "OnLocationChanged: {0}", e);

            base.OnLocationChanged(e);
        }

        /// <summary>
        ///     Responds to XAML's 'LayoutUpdated' event, fired when XAML content 
        ///     layout has changed.  If 'DesiredSize' has changed, re-run 
        ///     Windows Forms layout.
        /// </summary>
        private void FrameworkElement_LayoutUpdated(object sender, object e)
        {
            traceSource.TraceEvent(TraceEventType.Verbose, 0, "FrameworkElement_LayoutUpdated");

            if (DesignMode)
            {
                return;
            }

            // XAML content has changed. Re-run Windows.Forms.Control Layout if parent form is 
            // set to AutoSize.
            if (AutoSize)
            {
                Size prefSize = GetRootXamlElementDesiredSize();

                // Protect against stable layout cycles (oscillating layout cycles will still infinitely loop)
                // TODO: Add protection for oscillating layout cycles (sometimes caused by layout rounding)
                if (lastXamlContentPreferredSize.Height != prefSize.Height || lastXamlContentPreferredSize.Width != prefSize.Width)
                {
                    lastXamlContentPreferredSize = prefSize;
                    PerformLayout();
                }
            }

            // Fire updated event
            if (XamlContentUpdated != null)
            {
                this.XamlContentUpdated(this, new XamlContentUpdatedEventArgs((Xaml.FrameworkElement)this.xamlBridge.Content));
            }
        }

        /// <summary>
        ///     Event handler for XamlContentHost SizeChanged. If the size of the host control
        ///     has changed, re-run Windows Forms layout on this Control instance.
        /// </summary>
        private void XamlContentHost_SizeChanged(object sender, EventArgs e)
        {
            traceSource.TraceEvent(TraceEventType.Verbose, 0, "XamlContentHost_SizeChanged");

            if (DesignMode)
            {
                return;
            }

            if (AutoSize)
            {
                if (this.xamlBridge.Content != null)
                {
                    // XamlContenHost Control.Size has changed. XAML must perform an Arrange pass.
                    // The XAML Arrange pass will expand XAML content with 'HorizontalStretch' and 
                    // 'VerticalStretch' properties to the bounds of the XamlContentHost Control. 
                    var rect = new WF.Rect(0, 0, this.Width, this.Height);
                    this.xamlBridge.Content.Measure(new WF.Size(this.Width, this.Height));
                    this.xamlBridge.Content.Arrange(rect);
                    PerformLayout();
                }
            }
        }
    }
}
