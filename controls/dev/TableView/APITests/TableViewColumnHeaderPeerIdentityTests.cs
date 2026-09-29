// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    public partial class TableViewAutomationTests
    {
        [TestMethod]
        public void UnrealizedHeaderPeersNeverShareTheTableAsOwner()
        {
            TableView table = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateTable();
                foreach (var column in table.Columns)
                {
                    bool rejected = false;
                    try { _ = new TableViewColumnHeaderAutomationPeer(table, column); }
                    catch (ArgumentException error) { rejected = error.HResult == unchecked((int)0x80070057); }
                    Verify.IsTrue(rejected,
                        "Before the header band realizes, a header peer must be rejected rather than " +
                        "fall back to the TableView, which would give every column the same owner.");
                }

                Content = table;
                table.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(table.IsLoaded);

                var owners = new List<UIElement>();
                foreach (var column in table.Columns)
                {
                    var peer = new TableViewColumnHeaderAutomationPeer(table, column);
                    Verify.IsFalse(peer.Owner == table,
                        "A realized header peer must own its header cell, never the TableView.");
                    owners.Add(peer.Owner);
                }

                Verify.AreEqual(table.Columns.Count, owners.Count);
                Verify.AreEqual(owners.Count, owners.Distinct().Count(),
                    "Header peers must have distinct owners; UIA derives element identity from the owner, " +
                    "so a shared owner makes the columns indistinguishable to assistive technology.");
            });
        }
    }
}
