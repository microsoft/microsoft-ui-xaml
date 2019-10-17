// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Numerics;
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

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using ScrollSnapPointsAlignment = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointsAlignment;
using ScrollSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPoint;
using RepeatedScrollSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.RepeatedScrollSnapPoint;
using ZoomSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPoint;
using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollingPresenterTests
    {
        [TestMethod]
        [TestProperty("Description", "Create a bunch of snap points with invalid arguments.")]
        public void SnapPointsWithInvalidArgsThrow()
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.Throws<ArgumentException>(() => { new RepeatedScrollSnapPoint(offset:  10, interval:  0, start:  10, end: 100, alignment: ScrollSnapPointsAlignment.Near); });
                Verify.Throws<ArgumentException>(() => { new RepeatedScrollSnapPoint(offset:  10, interval: -1, start:  10, end: 100, alignment: ScrollSnapPointsAlignment.Near); });
                Verify.Throws<ArgumentException>(() => { new RepeatedScrollSnapPoint(offset:  10, interval: 10, start:  10, end:   1, alignment: ScrollSnapPointsAlignment.Near); });
                Verify.Throws<ArgumentException>(() => { new RepeatedScrollSnapPoint(offset:  10, interval: 10, start:  10, end:  10, alignment: ScrollSnapPointsAlignment.Near); });
#if ApplicableRangeType
                Verify.Throws<ArgumentException>(() => { new RepeatedScrollSnapPoint(offset:   1, interval: 10, start:   1, end:  10, applicableRange: -10, alignment: ScrollSnapPointsAlignment.Near); });
                Verify.Throws<ArgumentException>(() => { new RepeatedScrollSnapPoint(offset:   1, interval: 10, start:   1, end:  10, applicableRange:   0, alignment: ScrollSnapPointsAlignment.Near); });
                Verify.Throws<ArgumentException>(() => { new RepeatedScrollSnapPoint(offset:  50, interval: 10, start: 100, end: 200, applicableRange:   2, alignment: ScrollSnapPointsAlignment.Near); });
                Verify.Throws<ArgumentException>(() => { new RepeatedScrollSnapPoint(offset: 250, interval: 10, start: 100, end: 200, applicableRange:   2, alignment: ScrollSnapPointsAlignment.Near); });
                Verify.Throws<ArgumentException>(() => { new ScrollSnapPoint(snapPointValue: 0, applicableRange:  0, alignment: ScrollSnapPointsAlignment.Near); });
                Verify.Throws<ArgumentException>(() => { new ScrollSnapPoint(snapPointValue: 0, applicableRange: -1, alignment: ScrollSnapPointsAlignment.Near); });
#endif
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verify that overlapping repeated snap points throw while adjacent ones do not.")]
        public void OverlappingRepeatedSnapPointsThrow()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollingPresenter scrollingPresenter = new ScrollingPresenter();
                RepeatedScrollSnapPoint snapPoint1 = new RepeatedScrollSnapPoint(offset:  10, interval: 10, start:  10, end: 100, alignment: ScrollSnapPointsAlignment.Near);
                RepeatedScrollSnapPoint snapPoint2 = new RepeatedScrollSnapPoint(offset:  10, interval: 10, start:  10, end: 100, alignment: ScrollSnapPointsAlignment.Near);
                RepeatedScrollSnapPoint snapPoint3 = new RepeatedScrollSnapPoint(offset:   0, interval:  2, start:   0, end:  12, alignment: ScrollSnapPointsAlignment.Near);
                RepeatedScrollSnapPoint snapPoint4 = new RepeatedScrollSnapPoint(offset:   0, interval:  2, start:   0, end:  10, alignment: ScrollSnapPointsAlignment.Near);
                RepeatedScrollSnapPoint snapPoint5 = new RepeatedScrollSnapPoint(offset: 100, interval:  2, start: 100, end: 200, alignment: ScrollSnapPointsAlignment.Near);

                scrollingPresenter.VerticalSnapPoints.Add(snapPoint1);
                Verify.Throws<ArgumentException>(() => { scrollingPresenter.VerticalSnapPoints.Add(snapPoint2); });
                Verify.Throws<ArgumentException>(() => { scrollingPresenter.VerticalSnapPoints.Add(snapPoint3); });
                scrollingPresenter.HorizontalSnapPoints.Add(snapPoint4);
                scrollingPresenter.HorizontalSnapPoints.Add(snapPoint5);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Add and remove snap points and make sure the corresponding collections look correct.")]
        public void CanAddAndRemoveSnapPointsFromAScrollingPresenter()
        {
            CanAddAndRemoveSnapPointsFromAScrollingPresenter(ScrollSnapPointsAlignment.Near);
            CanAddAndRemoveSnapPointsFromAScrollingPresenter(ScrollSnapPointsAlignment.Center);
            CanAddAndRemoveSnapPointsFromAScrollingPresenter(ScrollSnapPointsAlignment.Far);
        }

        private void CanAddAndRemoveSnapPointsFromAScrollingPresenter(ScrollSnapPointsAlignment alignment)
        { 
            ScrollingPresenter scrollingPresenter = null;
            ScrollSnapPoint snapPoint2 = null;
            RepeatedScrollSnapPoint snapPoint3 = null;
            RepeatedScrollSnapPoint snapPoint4 = null;

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenter = new ScrollingPresenter();
                ScrollSnapPoint snapPoint1 = new ScrollSnapPoint(snapPointValue: 10, alignment: alignment);
                snapPoint2 = new ScrollSnapPoint(snapPointValue: 10, alignment: alignment);
                snapPoint3 = new RepeatedScrollSnapPoint(offset:  10, interval: 10, start:  10, end: 100, alignment: alignment);
                snapPoint4 = new RepeatedScrollSnapPoint(offset: 100, interval: 10, start: 100, end: 200, alignment: alignment);
                ZoomSnapPoint snapPoint5 = new ZoomSnapPoint(snapPointValue: 10);
                scrollingPresenter.HorizontalSnapPoints.Add(snapPoint1);
                scrollingPresenter.VerticalSnapPoints.Add(snapPoint1);
                scrollingPresenter.ZoomSnapPoints.Add(snapPoint5);
            });
            
            IdleSynchronizer.Wait();
            
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(1, scrollingPresenter.HorizontalSnapPoints.Count);
                Verify.AreEqual<int>(1, scrollingPresenter.VerticalSnapPoints.Count);
                Verify.AreEqual<int>(1, scrollingPresenter.ZoomSnapPoints.Count);
                scrollingPresenter.HorizontalSnapPoints.Add(snapPoint2);
                scrollingPresenter.HorizontalSnapPoints.Add(snapPoint3);
                scrollingPresenter.HorizontalSnapPoints.Add(snapPoint4);
            });
            
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(4, scrollingPresenter.HorizontalSnapPoints.Count);
                scrollingPresenter.HorizontalSnapPoints.Remove(snapPoint2);
            });           

            IdleSynchronizer.Wait(); 

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(3, scrollingPresenter.HorizontalSnapPoints.Count);
                scrollingPresenter.HorizontalSnapPoints.Remove(snapPoint2);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(3, scrollingPresenter.HorizontalSnapPoints.Count);
                scrollingPresenter.HorizontalSnapPoints.Clear();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(0, scrollingPresenter.HorizontalSnapPoints.Count);
                Verify.AreEqual<int>(1, scrollingPresenter.VerticalSnapPoints.Count);
                Verify.AreEqual<int>(1, scrollingPresenter.ZoomSnapPoints.Count);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Add scroll snap points with various alignments.")]
        public void CanAddScrollSnapPointsWithMixedAlignments()
        {
            ScrollingPresenter scrollingPresenter = null;

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenter = new ScrollingPresenter();
                ScrollSnapPoint nearSnapPoint = new ScrollSnapPoint(snapPointValue: 10, alignment: ScrollSnapPointsAlignment.Near);
                ScrollSnapPoint centerSnapPoint = new ScrollSnapPoint(snapPointValue: 20, alignment: ScrollSnapPointsAlignment.Center);
                ScrollSnapPoint farSnapPoint = new ScrollSnapPoint(snapPointValue: 30, alignment: ScrollSnapPointsAlignment.Far);
                RepeatedScrollSnapPoint nearRepeatedScrollSnapPoint = new RepeatedScrollSnapPoint(offset: 50, interval: 10, start: 50, end: 100, alignment: ScrollSnapPointsAlignment.Near);
                RepeatedScrollSnapPoint centerRepeatedScrollSnapPoint = new RepeatedScrollSnapPoint(offset: 180, interval: 10, start: 175, end: 225, alignment: ScrollSnapPointsAlignment.Center);
                RepeatedScrollSnapPoint farRepeatedScrollSnapPoint = new RepeatedScrollSnapPoint(offset: 280, interval: 5, start: 280, end: 300, alignment: ScrollSnapPointsAlignment.Far);
                scrollingPresenter.HorizontalSnapPoints.Add(nearSnapPoint);
                scrollingPresenter.HorizontalSnapPoints.Add(centerSnapPoint);
                scrollingPresenter.HorizontalSnapPoints.Add(farSnapPoint);
                scrollingPresenter.VerticalSnapPoints.Add(nearSnapPoint);
                scrollingPresenter.VerticalSnapPoints.Add(centerSnapPoint);
                scrollingPresenter.VerticalSnapPoints.Add(farSnapPoint);
                scrollingPresenter.HorizontalSnapPoints.Add(nearRepeatedScrollSnapPoint);
                scrollingPresenter.HorizontalSnapPoints.Add(centerRepeatedScrollSnapPoint);
                scrollingPresenter.HorizontalSnapPoints.Add(farRepeatedScrollSnapPoint);
                scrollingPresenter.VerticalSnapPoints.Add(nearRepeatedScrollSnapPoint);
                scrollingPresenter.VerticalSnapPoints.Add(centerRepeatedScrollSnapPoint);
                scrollingPresenter.VerticalSnapPoints.Add(farRepeatedScrollSnapPoint);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(6, scrollingPresenter.HorizontalSnapPoints.Count);
                Verify.AreEqual<int>(6, scrollingPresenter.VerticalSnapPoints.Count);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Add the same snap points to multiple collections and ensure they use collection-specific data.")]
        public void CanShareSnapPointsInMultipleCollections()
        {
            ScrollingPresenter scrollingPresenter1 = null;
            ScrollingPresenter scrollingPresenter2 = null;
            ScrollingPresenter scrollingPresenter3 = null;

            ScrollSnapPoint scrollSnapPoint1 = null;
            ScrollSnapPoint scrollSnapPoint2 = null;
            ScrollSnapPoint scrollSnapPoint3 = null;

            RepeatedScrollSnapPoint repeatedScrollSnapPoint1 = null;
            RepeatedScrollSnapPoint repeatedScrollSnapPoint2 = null;
            RepeatedScrollSnapPoint repeatedScrollSnapPoint3 = null;

            ZoomSnapPoint zoomSnapPoint1 = null;
            ZoomSnapPoint zoomSnapPoint2 = null;
            ZoomSnapPoint zoomSnapPoint3 = null;

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenter1 = new ScrollingPresenter();
                scrollingPresenter2 = new ScrollingPresenter();
                scrollingPresenter3 = new ScrollingPresenter();

                scrollSnapPoint1 = new ScrollSnapPoint(snapPointValue: 10, alignment: ScrollSnapPointsAlignment.Near);
                scrollSnapPoint2 = new ScrollSnapPoint(snapPointValue: 20, alignment: ScrollSnapPointsAlignment.Near);
                scrollSnapPoint3 = new ScrollSnapPoint(snapPointValue: 30, alignment: ScrollSnapPointsAlignment.Near);

                repeatedScrollSnapPoint1 = new RepeatedScrollSnapPoint(offset:  10, interval: 10, start:  10, end: 100, alignment: ScrollSnapPointsAlignment.Near);
                repeatedScrollSnapPoint2 = new RepeatedScrollSnapPoint(offset: 200, interval: 10, start: 110, end: 200, alignment: ScrollSnapPointsAlignment.Near);
                repeatedScrollSnapPoint3 = new RepeatedScrollSnapPoint(offset: 300, interval: 10, start: 210, end: 300, alignment: ScrollSnapPointsAlignment.Near);

                zoomSnapPoint1 = new ZoomSnapPoint(snapPointValue: 1);
                zoomSnapPoint2 = new ZoomSnapPoint(snapPointValue: 2);
                zoomSnapPoint3 = new ZoomSnapPoint(snapPointValue: 3);

                scrollingPresenter1.HorizontalSnapPoints.Add(scrollSnapPoint1);
                scrollingPresenter1.HorizontalSnapPoints.Add(scrollSnapPoint2);
                scrollingPresenter1.VerticalSnapPoints.Add(scrollSnapPoint1);
                scrollingPresenter1.VerticalSnapPoints.Add(scrollSnapPoint3);

                scrollingPresenter2.HorizontalSnapPoints.Add(repeatedScrollSnapPoint1);
                scrollingPresenter2.HorizontalSnapPoints.Add(repeatedScrollSnapPoint2);
                scrollingPresenter2.VerticalSnapPoints.Add(repeatedScrollSnapPoint1);
                scrollingPresenter2.VerticalSnapPoints.Add(repeatedScrollSnapPoint3);

                scrollingPresenter1.ZoomSnapPoints.Add(zoomSnapPoint1);
                scrollingPresenter1.ZoomSnapPoints.Add(zoomSnapPoint2);
                scrollingPresenter2.ZoomSnapPoints.Add(zoomSnapPoint1);
                scrollingPresenter2.ZoomSnapPoints.Add(zoomSnapPoint3);

                scrollingPresenter3.HorizontalSnapPoints.Add(scrollSnapPoint1);
                scrollingPresenter3.HorizontalSnapPoints.Add(scrollSnapPoint1);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Vector2 horizontalScrollSnapPoint11ApplicableZone = ScrollingPresenterTestHooks.GetHorizontalSnapPointActualApplicableZone(scrollingPresenter1, scrollSnapPoint1);
                Vector2 verticalScrollSnapPoint11ApplicableZone = ScrollingPresenterTestHooks.GetVerticalSnapPointActualApplicableZone(scrollingPresenter1, scrollSnapPoint1);
                Log.Comment("horizontalScrollSnapPoint11ApplicableZone=" + horizontalScrollSnapPoint11ApplicableZone.ToString());
                Log.Comment("verticalScrollSnapPoint11ApplicableZone=" + verticalScrollSnapPoint11ApplicableZone.ToString());

                Vector2 horizontalScrollSnapPoint21ApplicableZone = ScrollingPresenterTestHooks.GetHorizontalSnapPointActualApplicableZone(scrollingPresenter2, repeatedScrollSnapPoint1);
                Vector2 verticalScrollSnapPoint21ApplicableZone = ScrollingPresenterTestHooks.GetVerticalSnapPointActualApplicableZone(scrollingPresenter2, repeatedScrollSnapPoint1);
                Log.Comment("horizontalScrollSnapPoint21ApplicableZone=" + horizontalScrollSnapPoint21ApplicableZone.ToString());
                Log.Comment("verticalScrollSnapPoint21ApplicableZone=" + verticalScrollSnapPoint21ApplicableZone.ToString());

                Vector2 zoomSnapPoint11ApplicableZone = ScrollingPresenterTestHooks.GetZoomSnapPointActualApplicableZone(scrollingPresenter1, zoomSnapPoint1);
                Vector2 zoomSnapPoint21ApplicableZone = ScrollingPresenterTestHooks.GetZoomSnapPointActualApplicableZone(scrollingPresenter2, zoomSnapPoint1);
                Log.Comment("zoomSnapPoint11ApplicableZone=" + zoomSnapPoint11ApplicableZone.ToString());
                Log.Comment("zoomSnapPoint21ApplicableZone=" + zoomSnapPoint21ApplicableZone.ToString());

                int combinationCount11 = ScrollingPresenterTestHooks.GetHorizontalSnapPointCombinationCount(scrollingPresenter1, scrollSnapPoint1);
                int combinationCount31 = ScrollingPresenterTestHooks.GetHorizontalSnapPointCombinationCount(scrollingPresenter3, scrollSnapPoint1);
                Log.Comment("combinationCount11=" + combinationCount11);
                Log.Comment("combinationCount31=" + combinationCount31);

                Log.Comment("Expecting different applicable zones for ScrollSnapPoint in horizontal and vertical collections");
                Verify.AreEqual<float>(15.0f, horizontalScrollSnapPoint11ApplicableZone.Y);
                Verify.AreEqual<float>(20.0f, verticalScrollSnapPoint11ApplicableZone.Y);

                Log.Comment("Expecting identical applicable zones for RepeatedScrollSnapPoint in horizontal and vertical collections");
                Verify.AreEqual<float>(10.0f, horizontalScrollSnapPoint21ApplicableZone.X);
                Verify.AreEqual<float>(10.0f, verticalScrollSnapPoint21ApplicableZone.X);
                Verify.AreEqual<float>(100.0f, horizontalScrollSnapPoint21ApplicableZone.Y);
                Verify.AreEqual<float>(100.0f, verticalScrollSnapPoint21ApplicableZone.Y);

                Log.Comment("Expecting different applicable zones for ZoomSnapPoint in two zoom collections");
                Verify.AreEqual<float>(1.5f, zoomSnapPoint11ApplicableZone.Y);
                Verify.AreEqual<float>(2.0f, zoomSnapPoint21ApplicableZone.Y);

                Log.Comment("Expecting different combination counts for ScrollSnapPoint in two horizontal collections");
                Verify.AreEqual<int>(0, combinationCount11);
                Verify.AreEqual<int>(1, combinationCount31);
            });
        }
    }
}
