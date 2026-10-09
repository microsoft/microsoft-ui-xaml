// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
using Axe.Windows.Automation;
using System.Diagnostics;
using System.Text;
using Axe.Windows.Desktop.UIAutomation;
using System;

namespace MUXTestInfra.Shared.Infra
{
    public class AxeTestHelper
    {
        private static IScanner scanner = null;
        public static IScanner AxeScanner
        {
            get
            {
                if (scanner == null)
                {
                    LoadScanner();
                }
                return scanner;
            }
        }

        private static void LoadScanner()
        {
            var processes = Process.GetProcessesByName("MUXControlsTestApp");
            Verify.IsTrue(processes.Length > 0);

            string directory = Environment.GetEnvironmentVariable("TEMP") + @"\"; // For instance C:\Users\TDPUser\AppData\Local\Temp\
            var config = Config.Builder.ForProcessId(processes[0].Id).WithOutputFileFormat(OutputFileFormat.A11yTest).WithOutputDirectory(directory).Build();
            scanner = ScannerFactory.CreateScanner(config);
        }

        public static void TestForAxeIssues()
        {
            var result = AxeScanner.Scan();

            foreach(var error in result.Errors)
            {
                Log.Error($"{error.ToString()} - {error.Element.ToString()} - {error.Rule.ToString()} - {error.Rule.HowToFix}");
                Log.Error($"    Violating element: {DescribeElement(error.Element)}");
                Log.Error($"    Ancestry (nearest first): {DescribeAncestry(error.Element)}");
                Log.Error($"    All properties: {DescribeAllProperties(error.Element)}");
            }

            Log.Comment($"Axe A11yTest output file: {result.OutputFile.A11yTest}");

            Verify.AreEqual(0, result.ErrorCount, "Found " + result.ErrorCount + " Axe errors.");
        }

        // A rule ID alone cannot be acted on: it says what is wrong but never which element is wrong.
        // Control type, Name, AutomationId and the ancestry chain are what turn a failure log into a
        // pointer at a specific piece of markup or a specific automation peer.
        private static string DescribeElement(ElementInfo element)
        {
            if (element == null)
            {
                return "<null element>";
            }

            return string.Format(
                "ControlType='{0}' Name='{1}' AutomationId='{2}' ClassName='{3}' LocalizedControlType='{4}' IsKeyboardFocusable='{5}' BoundingRectangle='{6}'",
                GetProperty(element, "ControlType"),
                GetProperty(element, "Name"),
                GetProperty(element, "AutomationId"),
                GetProperty(element, "ClassName"),
                GetProperty(element, "LocalizedControlType"),
                GetProperty(element, "IsKeyboardFocusable"),
                GetProperty(element, "BoundingRectangle"));
        }

        private static string DescribeAncestry(ElementInfo element)
        {
            var builder = new StringBuilder();

            // Bounded: a corrupt provider tree could otherwise hand back a cycle, and a diagnostic
            // must never be the thing that hangs the run it is trying to explain.
            var ancestor = element?.Parent;
            for (int depth = 0; ancestor != null && depth < 20; depth++, ancestor = ancestor.Parent)
            {
                if (builder.Length > 0)
                {
                    builder.Append(" <- ");
                }

                builder.AppendFormat(
                    "{0}[Name='{1}' AutomationId='{2}' ClassName='{3}']",
                    GetProperty(ancestor, "ControlType"),
                    GetProperty(ancestor, "Name"),
                    GetProperty(ancestor, "AutomationId"),
                    GetProperty(ancestor, "ClassName"));
            }

            return builder.Length > 0 ? builder.ToString() : "<no parent reported>";
        }

        private static string DescribeAllProperties(ElementInfo element)
        {
            if (element?.Properties == null)
            {
                return "<none>";
            }

            var builder = new StringBuilder();
            foreach (var property in element.Properties)
            {
                if (builder.Length > 0)
                {
                    builder.Append("; ");
                }

                builder.Append(property.Key).Append('=').Append('\'').Append(property.Value).Append('\'');
            }

            if (element.Patterns != null)
            {
                builder.Append(" | Patterns: ").Append(string.Join(",", element.Patterns));
            }

            return builder.Length > 0 ? builder.ToString() : "<none>";
        }

        private static string GetProperty(ElementInfo element, string name)
        {
            // Axe drops properties whose text value is empty, so a missing key IS the finding for a
            // rule like NameNotNull. Say so rather than printing an ambiguous blank.
            if (element?.Properties != null && element.Properties.TryGetValue(name, out var value) &&
                !string.IsNullOrEmpty(value))
            {
                return value;
            }

            return "<null or empty>";
        }
    }
}
