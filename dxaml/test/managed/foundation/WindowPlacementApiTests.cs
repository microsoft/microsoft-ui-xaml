// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Tests.Common;
using Windows.Graphics;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.Foundation
{
    [TestClass]
    public class WindowPlacementApiTests : XamlTestsBase
    {
        private static RectInt32 Normal => new RectInt32(-20, -10, 40, 30);
        private static RectInt32 WorkArea => new RectInt32(-100, -50, 200, 100);

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            XamlTestsBase.SetupBase(context);
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            CommonClassCleanup();
        }

        [TestMethod]
        public void ConstructorAndNullableDefaults()
        {
            var placement = new WindowPlacement(Normal, WorkArea, 96);
            Verify.AreEqual(Normal, placement.NormalRect);
            Verify.AreEqual(WorkArea, placement.WorkArea);
            Verify.AreEqual(96, placement.Dpi);
            Verify.AreEqual(WindowPlacementState.Normal, placement.State);
            Verify.IsNull(placement.SnapRect);
            Verify.IsNull(placement.VirtualDesktopId);
            Verify.AreEqual(string.Empty, placement.DisplayDeviceName);

            var options = new WindowShowOptions();
            Verify.IsNull(options.Placement);
            Verify.AreEqual(WindowShowReason.Default, options.Reason);
            Verify.AreEqual(WindowCascadeBehavior.Automatic, options.CascadeBehavior);
            Verify.IsFalse(options.DoNotActivate);
            Verify.IsFalse(options.SkipInitialPlacement);
        }

        [TestMethod]
        public void ConstructorRejectsInvalidGeometryAndDpi()
        {
            AssertInvalid(() => new WindowPlacement(Normal, WorkArea, 95));
            AssertInvalid(() => new WindowPlacement(new RectInt32(0, 0, 0, 10), WorkArea, 96));
            AssertInvalid(() => new WindowPlacement(Normal, new RectInt32(0, 0, -1, 10), 96));
            AssertInvalid(() => new WindowPlacement(new RectInt32(int.MaxValue, 0, 1, 10), WorkArea, 96));
            AssertInvalid(() => new WindowPlacement(new RectInt32(100, 0, 10, 10), WorkArea, 96));
            AssertInvalid(() => new WindowPlacement(Normal, new RectInt32(0, int.MaxValue, 10, 1), 96));
            var valid = new WindowPlacement(Normal, WorkArea, int.MaxValue);
            Verify.AreEqual(int.MaxValue, valid.Dpi);
        }

        [TestMethod]
        public void EditingIsPermissiveAndOptionsRetainReference()
        {
            var placement = new WindowPlacement(Normal, WorkArea, 96);
            var options = new WindowShowOptions { Placement = placement };
            placement.Dpi = -1;
            placement.State = (WindowPlacementState)999;
            placement.NormalRect = new RectInt32(0, 0, -1, 0);
            placement.DisplayDeviceName = "embedded\0nul";
            options.Reason = (WindowShowReason)999;
            options.CascadeBehavior = (WindowCascadeBehavior)(-1);
            options.SkipInitialPlacement = true;
            Verify.AreEqual(-1, options.Placement.Dpi);
            Verify.AreEqual((WindowPlacementState)999, options.Placement.State);
            Verify.AreEqual("embedded\0nul", options.Placement.DisplayDeviceName);
            Verify.AreEqual((WindowShowReason)999, options.Reason);
            Verify.AreEqual((WindowCascadeBehavior)(-1), options.CascadeBehavior);
            Verify.IsTrue(options.SkipInitialPlacement);
            options.Placement = null;
            Verify.IsNull(options.Placement);
        }

        [TestMethod]
        public void OptionalValuesAndIndependentEdits()
        {
            var first = new WindowPlacement(Normal, WorkArea, 96);
            var second = new WindowPlacement(first.NormalRect, first.WorkArea, first.Dpi);
            var id = Guid.NewGuid();
            first.SnapRect = new RectInt32(0, 0, 10, 20);
            first.VirtualDesktopId = id;
            first.DisplayDeviceName = null;
            Verify.AreEqual(new RectInt32(0, 0, 10, 20), first.SnapRect.Value);
            Verify.AreEqual(id, first.VirtualDesktopId.Value);
            Verify.AreEqual(string.Empty, first.DisplayDeviceName);
            Verify.IsNull(second.SnapRect);
            Verify.IsNull(second.VirtualDesktopId);
            first.VirtualDesktopId = Guid.Empty;
            Verify.AreEqual(Guid.Empty, first.VirtualDesktopId.Value);
            first.SnapRect = null;
            first.VirtualDesktopId = null;
            Verify.IsNull(first.SnapRect);
            Verify.IsNull(first.VirtualDesktopId);
        }

        [TestMethod]
        public void DetachedLoadRejectsEmptyIdsAndReturnsNullForMissingValues()
        {
            AssertInvalid(() => WindowPlacement.LoadForPersistPlacementId(null));
            AssertInvalid(() => WindowPlacement.LoadForPersistPlacementId(string.Empty));
            Verify.IsNull(WindowPlacement.LoadForPersistPlacementId("WindowPlacementApiTests.Missing"));
        }

        [TestMethod]
        public void ValuesCanBeCreatedAndEditedAcrossThreads()
        {
            var placement = new WindowPlacement(Normal, WorkArea, 96);
            var options = new WindowShowOptions { Placement = placement };
            Task.Run(() =>
            {
                Verify.AreEqual(Normal, options.Placement.NormalRect);
                placement.Dpi = 144;
                options.DoNotActivate = true;
                var other = new WindowPlacement(Normal, WorkArea, 192);
                options.Placement = other;
            }).GetAwaiter().GetResult();
            Verify.AreEqual(144, placement.Dpi);
            Verify.AreEqual(192, options.Placement.Dpi);
            Verify.IsTrue(options.DoNotActivate);
        }

        private static void AssertInvalid(Action action)
        {
            try
            {
                action();
                Verify.Fail("Expected ArgumentException for invalid constructor input.");
            }
            catch (ArgumentException error)
            {
                Verify.AreEqual(unchecked((int)0x80070057), error.HResult);
            }
        }
    }
}
