// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Threading;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Settings;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace MUXControlsTestApp.Utilities
{
    public class ApiTestBase
    {
        private Border _host;

        public const int DefaultWaitTimeInMS = 5000;

        // TAEF will set a value on this, where we can read out the Data:XamlOptionalChanges
        public TestContext TestContext { get; set; }

        // Set this content instead of using App.CurrentWindow.Content
        // because the latter requires you to tick the UI thread
        // before a layout pass can happen while you can directly call
        // UpdateLayout after the former, which is faster and less
        // sensitive to timing issues.
        public UIElement Content
        {
            get { return _host.Child; }
            set { _host.Child = value; }
        }

        [TestInitialize]
        public void Setup()
        {
            IdleSynchronizer.Wait();
            var hostLoaded = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                UpdateXamlOptionalChanges();

                // XamlOptionalChanges for the test might have selected different styles. Load them if needed. Note that the
                // styles are in XamlControlsResources, which selected a Source depending on the XamlOptionalChanges at the
                // time and have already been loaded. We don't reevaluate the Source for the current XamlControlsResources,
                // because updating a live dictionary calls InvalidateImplicitStyles, which walks the tree and triggers
                // resource updates, which can cause resource not found errors while we're updating the dictionary. Instead
                // we just insert a new instance of XamlControlsResources and throw the old one away. As an optimization,
                // we'll check the Source of the new XCR aginst the existing XCR and only replace if they're different.
                var mergedDicts = Application.Current.Resources.MergedDictionaries;
                for (int i = 0; i < mergedDicts.Count; i++)
                {
                    if (mergedDicts[i] is XamlControlsResources)
                    {
                        var newXamlControlsResources = new XamlControlsResources();
                        if (newXamlControlsResources.Source != mergedDicts[i].Source)
                        {
                            mergedDicts.RemoveAt(i);
                            mergedDicts.Insert(i, newXamlControlsResources);
                        }
                        break;
                    }
                }

                _host = new Border();
                _host.Loaded += delegate { hostLoaded.Set(); };
                MUXControlsTestApp.App.TestContentRoot = _host;
            });
            Verify.IsTrue(hostLoaded.WaitOne(DefaultWaitTimeInMS), "Waiting for loaded event");
        }

        [TestCleanup]
        public void Cleanup()
        {
            TestUtilities.ClearVisualTreeRoot();
        }

        public static void EnableAllXamlOptionalChanges()
        {
            Log.Comment($"> XamlOptionalChanges test override: Enabling all XamlOptionalChanges by default");
            Verify.IsTrue(XamlOptionalChanges.EnableChange(XamlChangeId.IconNoGridOptimization));
            Verify.IsTrue(XamlOptionalChanges.EnableChange(XamlChangeId.DelayApplyStyleOptimization));
            Verify.IsTrue(XamlOptionalChanges.EnableChange(XamlChangeId.DefaultStyleOptimizations));
        }

        public void UpdateXamlOptionalChanges()
        {
            // Check if any XamlOptionalChanges need to be enabled/disabled based on a TAEF parameter. Tests
            // declare TEST_METHOD_PROPERTY(L"Data:XamlOptionalChanges", L"{<XamlChangeId name>:false}") or
            // L"{<XamlChangeId name>:true}" in the header, and TAEF makes the value available via TestData
            // during TestSetup. Multiple values can be specified by separating with pipe (|), such as:
            //     L"{IconNoGridOptimization:false|DelayApplyStyleOptimization:false}"
            // (Do not use commas or semicolons, since TAEF interprets those as parameter-set separators,
            // per: https://learn.microsoft.com/windows-hardware/drivers/taef/light-weight-data-driven-testing )
            // Test-specified values override the defaults and the "test-default" states set below.
            var changeOverrides = GetXamlOptionalChangesTestOverrides(TestContext);

            if (changeOverrides.Count > 0)
            {
                RunOnUIThread.Execute(() =>
                {
                    DxamlCoreTestHooks.GetForCurrentThread().ResetOptionalChanges();
                    EnableAllXamlOptionalChanges();

                    // Apply per-test overrides from XamlOptionalChanges test data.
                    foreach (var changeId in changeOverrides.Keys)
                    {
                        var enabled = changeOverrides[changeId];
                        if (enabled)
                        {
                            Log.Comment($"> XamlOptionalChanges test override: Enabling {changeId}");
                            Verify.IsTrue(XamlOptionalChanges.EnableChange(changeId));
                        }
                        else
                        {
                            Log.Comment($"> XamlOptionalChanges test override: Disabling {changeId}");
                            Verify.IsTrue(XamlOptionalChanges.DisableChange(changeId));
                        }
                    }
                });
            }
        }

        public Dictionary<XamlChangeId, bool> GetXamlOptionalChangesTestOverrides(TestContext testContext)
        {
            var xamlOptionalChanges = new Dictionary<XamlChangeId, bool>();
            var xamlOptInString = string.Empty;

            if (testContext.DataRow.Table.Columns.Contains("XamlOptionalChanges"))
            {
                xamlOptInString = testContext.DataRow["XamlOptionalChanges"].ToString();
                if (!string.IsNullOrEmpty(xamlOptInString))
                {
                    // Parse format: "{Name:true|Name2:false}"
                    var input = xamlOptInString;
                    var pos = 0;

                    // Strip leading/trailing braces
                    if (!string.IsNullOrEmpty(input) && input[0] == '{') pos = 1;
                    if (!string.IsNullOrEmpty(input) && input[input.Length - 1] == '}') input = input.Substring(0, input.Length - 1);

                    while (pos < input.Length)
                    {
                        var colonPos = input.IndexOf(':', pos);
                        if (colonPos == -1) break;
                        var name = input.Substring(pos, colonPos - pos);

                        var sepPos = input.IndexOf('|', colonPos + 1);
                        if (sepPos == -1) sepPos = input.Length;
                        var value = input.Substring(colonPos + 1, sepPos - (colonPos + 1));

                        var enabled = string.Equals(value, "true", StringComparison.OrdinalIgnoreCase);

                        XamlChangeId changeId = XamlChangeId._Reserved;
                        if (string.Equals(name, "IconNoGridOptimization", StringComparison.OrdinalIgnoreCase))
                        {
                            changeId = XamlChangeId.IconNoGridOptimization;
                        }
                        else if (string.Equals(name, "DelayApplyStyleOptimization", StringComparison.OrdinalIgnoreCase))
                        {
                            changeId = XamlChangeId.DelayApplyStyleOptimization;
                        }
                        else if (string.Equals(name, "DefaultStyleOptimizations", StringComparison.OrdinalIgnoreCase))
                        {
                            changeId = XamlChangeId.DefaultStyleOptimizations;
                        }

                        Verify.AreNotEqual(changeId, XamlChangeId._Reserved, "Unknown XamlChangeId: " + name);

                        xamlOptionalChanges.Add(changeId, enabled);
                        pos = sepPos + 1;
                    }

                    foreach (var key in xamlOptionalChanges.Keys)
                    {
                        Log.Comment($"> XamlOptionalChanges test override: XamlChangeId={key} -> {xamlOptionalChanges[key]}");
                    }
                }
            }

            return xamlOptionalChanges;
        }

    }
}
