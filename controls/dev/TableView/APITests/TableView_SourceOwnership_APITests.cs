// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewSortingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewSizingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class TableViewResizeCancelTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a canceled resize applies gripper availability changes deferred while the drag was in flight.")]
        public void VerifyCanceledResizeRefreshesGripperAvailability()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateResizableTable(
                    ("Name", new GridLength(150.0, GridUnitType.Pixel)),
                    ("Role", new GridLength(150.0, GridUnitType.Pixel)));
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var gripper = RequireGripper(tableView, 0);
                var siblingGripper = RequireGripper(tableView, 1);
                Verify.AreEqual(Visibility.Visible, siblingGripper.Visibility, "Precondition: the sibling column is resizable.");

                gripper.BeginDrag();
                // Pins the sibling while availability refreshes are held for the drag.
                tableView.Columns[1].MinWidth = 150.0;
                tableView.Columns[1].MaxWidth = 150.0;
                gripper.EndDrag(true /* canceled */);

                Verify.AreEqual(Visibility.Collapsed, siblingGripper.Visibility,
                    "A canceled drag must still apply the availability change it held back.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies unloading the table mid-drag cancels the resize and restores Width, and that a new drag works after the table is re-added.")]
        public void VerifyUnloadMidDragRevertsWidthAndAllowsANewDrag()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateResizableTable(
                    ("Name", new GridLength(150.0, GridUnitType.Pixel)),
                    ("Role", new GridLength(150.0, GridUnitType.Pixel)));
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var gripper = RequireGripper(tableView, 0);
                gripper.BeginDrag();
                gripper.TryDrag(40.0);
                Verify.AreEqual(190.0, tableView.Columns[0].Width.Value, "Precondition: the drag must have written a width.");

                Content = null;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new GridLength(150.0, GridUnitType.Pixel), tableView.Columns[0].Width,
                    "Unloading mid-drag must cancel the resize and restore the authored Width.");
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var gripper = RequireGripper(tableView, 0);
                Verify.IsFalse(gripper.IsDragging, "No drag may survive the unload.");

                gripper.BeginDrag();
                gripper.TryDrag(20.0);
                gripper.EndDrag(false);

                Verify.AreEqual(new GridLength(170.0, GridUnitType.Pixel), tableView.Columns[0].Width,
                    "A new drag after re-adding the table must resize from the restored width.");
            });
        }
    }
}
