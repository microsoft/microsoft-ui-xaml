// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Runtime.InteropServices;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if !BUILD_WINDOWS
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ScrollerSnapPointBase = Microsoft.UI.Xaml.Controls.Primitives.ScrollerSnapPointBase;
using ScrollerSnapPointAlignment = Microsoft.UI.Xaml.Controls.Primitives.ScrollerSnapPointAlignment;
using ScrollerSnapPointRegular = Microsoft.UI.Xaml.Controls.Primitives.ScrollerSnapPointRegular;
using ScrollerSnapPointIrregular = Microsoft.UI.Xaml.Controls.Primitives.ScrollerSnapPointIrregular;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollerTests
    {
        [TestMethod]
        [TestProperty("Description", "Create a bunch of snap points with invalid arguments.")]
        public void SnapPointsWithInvalidArgsThrow()
        {
            Verify.Throws<COMException>(() => { new ScrollerSnapPointRegular(offset:  10, interval:  0, start:  10, end: 100, alignment: ScrollerSnapPointAlignment.Near); });
            Verify.Throws<COMException>(() => { new ScrollerSnapPointRegular(offset:  10, interval: -1, start:  10, end: 100, alignment: ScrollerSnapPointAlignment.Near); });
            Verify.Throws<COMException>(() => { new ScrollerSnapPointRegular(offset:  10, interval: 10, start:  10, end:   1, alignment: ScrollerSnapPointAlignment.Near); });
            Verify.Throws<COMException>(() => { new ScrollerSnapPointRegular(offset:  10, interval: 10, start:  10, end:  10, alignment: ScrollerSnapPointAlignment.Near); });
            Verify.Throws<COMException>(() => { new ScrollerSnapPointRegular(offset:   1, interval: 10, start:   1, end:  10, applicableRange: -10, alignment: ScrollerSnapPointAlignment.Near); });
            Verify.Throws<COMException>(() => { new ScrollerSnapPointRegular(offset:   1, interval: 10, start:   1, end:  10, applicableRange:   0, alignment: ScrollerSnapPointAlignment.Near); });
            Verify.Throws<COMException>(() => { new ScrollerSnapPointRegular(offset:  50, interval: 10, start: 100, end: 200, applicableRange:   2, alignment: ScrollerSnapPointAlignment.Near); });
            Verify.Throws<COMException>(() => { new ScrollerSnapPointRegular(offset: 250, interval: 10, start: 100, end: 200, applicableRange:   2, alignment: ScrollerSnapPointAlignment.Near); });
            Verify.Throws<COMException>(() => { new ScrollerSnapPointIrregular(snapPointValue: 0, applicableRange:  0, alignment: ScrollerSnapPointAlignment.Near); });
            Verify.Throws<COMException>(() => { new ScrollerSnapPointIrregular(snapPointValue: 0, applicableRange: -1, alignment: ScrollerSnapPointAlignment.Near); });
        }

        [TestMethod]
        [TestProperty("Description", "Verify that overlapping regular snap points throw while adjacent ones do not.")]
        public void OverlappingRegularSnapPointsThrow()
        {
            RunOnUIThread.Execute(() =>
            {
                Scroller scroller = new Scroller();
                ScrollerSnapPointRegular snapPoint1 = new ScrollerSnapPointRegular(offset:  10, interval: 10, start:  10, end: 100, alignment: ScrollerSnapPointAlignment.Near);
                ScrollerSnapPointRegular snapPoint2 = new ScrollerSnapPointRegular(offset:  10, interval: 10, start:  10, end: 100, alignment: ScrollerSnapPointAlignment.Near);
                ScrollerSnapPointRegular snapPoint3 = new ScrollerSnapPointRegular(offset:   0, interval:  2, start:   0, end:  12, alignment: ScrollerSnapPointAlignment.Near);
                ScrollerSnapPointRegular snapPoint4 = new ScrollerSnapPointRegular(offset:   0, interval:  2, start:   0, end:  10, alignment: ScrollerSnapPointAlignment.Near);
                ScrollerSnapPointRegular snapPoint5 = new ScrollerSnapPointRegular(offset: 100, interval:  2, start: 100, end: 200, alignment: ScrollerSnapPointAlignment.Near);

                scroller.VerticalSnapPoints.Add(snapPoint1);
                Verify.Throws<ArgumentException>(() => { scroller.VerticalSnapPoints.Add(snapPoint2); });
                Verify.Throws<ArgumentException>(() => { scroller.VerticalSnapPoints.Add(snapPoint3); });
                scroller.HorizontalSnapPoints.Add(snapPoint4);
                scroller.HorizontalSnapPoints.Add(snapPoint5);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Add and remove snap points and make sure the corresponding collections look correct.")]
        public void CanAddAndRemoveSnapPointsFromAScroller()
        {
            Scroller scroller = null;
            ScrollerSnapPointBase snapPointBase1 = null;
            ScrollerSnapPointBase snapPointBase2 = null;
            ScrollerSnapPointBase snapPointBase3 = null;
            ScrollerSnapPointBase snapPointBase4 = null;
            RunOnUIThread.Execute(() =>
            {
                scroller = new Scroller();
                snapPointBase1 = new ScrollerSnapPointIrregular(snapPointValue: 10, alignment: ScrollerSnapPointAlignment.Near);
                snapPointBase2 = new ScrollerSnapPointIrregular(snapPointValue: 10, applicableRange: 3, alignment: ScrollerSnapPointAlignment.Near);
                snapPointBase3 = new ScrollerSnapPointRegular(offset:  10, interval: 10, start:  10, end: 100, alignment: ScrollerSnapPointAlignment.Near);
                snapPointBase4 = new ScrollerSnapPointRegular(offset: 100, interval: 10, start: 100, end: 200, applicableRange: 3, alignment: ScrollerSnapPointAlignment.Near);
                scroller.HorizontalSnapPoints.Add(snapPointBase1);
                scroller.VerticalSnapPoints.Add(snapPointBase1);
                scroller.ZoomSnapPoints.Add(snapPointBase1);
            });
            
            IdleSynchronizer.Wait();
            
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(1, scroller.HorizontalSnapPoints.Count);
                Verify.AreEqual<int>(1, scroller.VerticalSnapPoints.Count);
                Verify.AreEqual<int>(1, scroller.ZoomSnapPoints.Count);
                scroller.HorizontalSnapPoints.Add(snapPointBase2);
                scroller.HorizontalSnapPoints.Add(snapPointBase3);
                scroller.HorizontalSnapPoints.Add(snapPointBase4);
            });
            
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(4, scroller.HorizontalSnapPoints.Count);
                scroller.HorizontalSnapPoints.Remove(snapPointBase2);
            });           

            IdleSynchronizer.Wait(); 

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(3, scroller.HorizontalSnapPoints.Count);
                scroller.HorizontalSnapPoints.Remove(snapPointBase2);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(3, scroller.HorizontalSnapPoints.Count);
                scroller.HorizontalSnapPoints.Clear();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(0, scroller.HorizontalSnapPoints.Count);
                Verify.AreEqual<int>(1, scroller.VerticalSnapPoints.Count);
                Verify.AreEqual<int>(1, scroller.ZoomSnapPoints.Count);
            });
        }
    }
}
