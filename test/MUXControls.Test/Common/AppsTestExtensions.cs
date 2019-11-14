// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using System;
using System.Collections.Generic;
using System.Text;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    public static class AppsTestExtensions
    {
        public static void SelectItemByName(this ComboBox comboBox, string name)
        {
            Log.Comment("ComboBox.SelectItemByName: Number of items: {0}", comboBox.AllItems.Count);
            foreach (var item in comboBox.AllItems)
            {
                Log.Comment("ComboBox.SelectItemByName: item = {0}", item.Name);
                if (item.Name == name)
                {
                    Log.Comment("ComboBox.SelectItemByName: Selecting item {0}", item);
                    item.Select();
                    Wait.ForIdle();
                    return;
                }
            }

            throw new Exception(String.Format("ComboBox.SelectItemByName: Could not find {0}", name));
        }

        public static void SelectItemById(this ComboBox comboBox, string id)
        {
            Log.Comment("ComboBox.SelectItemById: Number of items: {0}", comboBox.AllItems.Count);
            foreach (var item in comboBox.AllItems)
            {
                Log.Comment("ComboBox.SelectItemById: item = {0}", item.AutomationId);
                if (item.AutomationId == id)
                {
                    Log.Comment("ComboBox.SelectItemById: Selecting item {0}", item);
                    item.Select();
                    Wait.ForIdle();
                    return;
                }
            }

            throw new Exception(String.Format("ComboBox.SelectItemById: Could not find {0}", id));
        }

        public static void InvokeAndWait(this Button button, TimeSpan? timeout = null)
        {
            if (button == null)
            {
                Log.Error("Attempted to invoke a null button! Dumping context...");
                DumpHelper.DumpFullContext();
                throw new ArgumentNullException("button");
            }

            using (var waiter = button.GetInvokedWaiter())
            {
                button.Invoke();
                if(timeout == null)
                {
                    waiter.Wait();
                }
                else
                {
                    waiter.Wait(timeout.Value);
                }
                
            }

            Wait.ForIdle();
        }

        public static void InvokeAndWait(this MenuItem menuItem, TimeSpan? timeout = null)
        {
            if (menuItem == null)
            {
                Log.Error("Attempted to invoke a null MenuItem! Dumping context...");
                DumpHelper.DumpFullContext();
                throw new ArgumentNullException("menuItem");
            }

            using (var waiter = menuItem.GetInvokedWaiter())
            {
                menuItem.Invoke();
                if (timeout == null)
                {
                    waiter.Wait();
                }
                else
                {
                    waiter.Wait(timeout.Value);
                }
            }

            Wait.ForIdle();
        }

        public static void ToggleAndWait(this ToggleButton toggleButton)
        {
            if (toggleButton == null)
            {
                Log.Error("Attempted to toggle a null toggle button! Dumping context...");
                DumpHelper.DumpFullContext();
                throw new ArgumentNullException("toggleButton");
            }
            
            using (var waiter = toggleButton.GetToggledWaiter())
            {
                toggleButton.Toggle();
                waiter.Wait();
            }

            Wait.ForIdle();
        }

        public static void SetValueAndWait(this Edit edit, string s)
        {
            if (edit == null)
            {
                Log.Error("Attempted to set value on a null edit control! Dumping context...");
                DumpHelper.DumpFullContext();
                throw new ArgumentNullException("edit");
            }

            using (var waiter = new ValueChangedEventWaiter(edit, s))
            {
                edit.SetValue(s);
                waiter.Wait();
            }

            Wait.ForIdle();
        }

        public static void InvokeAndWait(this SplitButton splitButton)
        {
            if (splitButton == null)
            {
                Log.Error("Attempted to invoke a null split button! Dumping context...");
                DumpHelper.DumpFullContext();
                throw new ArgumentNullException("splitButton");
            }

            using (var waiter = splitButton.GetInvokedWaiter())
            {
                splitButton.Invoke();
                waiter.Wait();
            }

            Wait.ForIdle();
        }

        public static void ExpandAndWait(this SplitButton splitButton)
        {
            if (splitButton == null)
            {
                Log.Error("Attempted to expand a null split button! Dumping context...");
                DumpHelper.DumpFullContext();
                throw new ArgumentNullException("splitButton");
            }

            using (var waiter = splitButton.GetExpandedWaiter())
            {
                splitButton.Expand();
                waiter.Wait();
            }

            Wait.ForIdle();
        }

        public static void CollapseAndWait(this SplitButton splitButton)
        {
            if (splitButton == null)
            {
                Log.Error("Attempted to collapse a null split button! Dumping context...");
                DumpHelper.DumpFullContext();
                throw new ArgumentNullException("splitButton");
            }

            using (var waiter = splitButton.GetCollapsedWaiter())
            {
                splitButton.Collapse();
                waiter.Wait();
            }

            Wait.ForIdle();
        }

        public static string GetIdentifier(this UIObject obj)
        {
            return string.IsNullOrWhiteSpace(obj.AutomationId) ? obj.Name : obj.AutomationId;
        }

        public static string GetDescription(this UIObject obj)
        {
            List<string> patternsSupported = new List<string>();
            StringBuilder extraInformationBuilder = new StringBuilder();

            DockImplementation dockImplementation = new DockImplementation(obj);
            ExpandCollapseImplementation expandCollapseImplementation = new ExpandCollapseImplementation(obj);
            InvokeImplementation invokeImplementation = new InvokeImplementation(obj);
            RangeValueImplementation rangeValueImplementation = new RangeValueImplementation(obj);
            ScrollImplementation scrollImplementation = new ScrollImplementation(obj);
            TextImplementation textImplementation = new TextImplementation(obj);
            ToggleImplementation toggleImplementation = new ToggleImplementation(obj);
            ValueImplementation valueImplementation = new ValueImplementation(obj);
            WindowImplementation windowImplementation = new WindowImplementation(obj);

            if (dockImplementation.IsAvailable)
            {
                patternsSupported.Add("Dock");
                extraInformationBuilder.AppendFormat(", Dock.DockPosition={0}", dockImplementation.DockPosition);
            }

            if (expandCollapseImplementation.IsAvailable)
            {
                patternsSupported.Add("ExpandCollapse");
                extraInformationBuilder.AppendFormat(", ExpandCollapse.ExpandCollapseState={0}", expandCollapseImplementation.ExpandCollapseState);
            }

            if (invokeImplementation.IsAvailable)
            {
                patternsSupported.Add("Invoke");
            }

            if (rangeValueImplementation.IsAvailable)
            {
                patternsSupported.Add("RangeValue");
                extraInformationBuilder.AppendFormat(", RangeValue.Minimum={0}, RangeValue.Maximum={1}, RangeValue.LargeChange={2}, RangeValue.SmallChange={3}, RangeValue.Value={4}, RangeValue.IsReadOnly={5}",
                        rangeValueImplementation.Minimum, rangeValueImplementation.Maximum,
                        rangeValueImplementation.LargeChange, rangeValueImplementation.SmallChange,
                        rangeValueImplementation.Value,
                        rangeValueImplementation.IsReadOnly);
            }

            if (scrollImplementation.IsAvailable)
            {
                patternsSupported.Add("Scroll");
                extraInformationBuilder.AppendFormat(", Scroll.HorizontallyScrollable={0}, Scroll.VerticallyScrollable={1}, Scroll.HorizontalScrollPercent={2}, Scroll.VerticalScrollPercent={3}, Scroll.HorizontalViewSize={4}, Scroll.VerticalViewSize={5}",
                        scrollImplementation.HorizontallyScrollable, scrollImplementation.VerticallyScrollable,
                        scrollImplementation.HorizontalScrollPercent, scrollImplementation.VerticalScrollPercent,
                        scrollImplementation.HorizontalViewSize, scrollImplementation.VerticalViewSize);
            }

            if (textImplementation.IsAvailable)
            {
                patternsSupported.Add("Text");
                string textContent = textImplementation.DocumentRange.GetText(-1);
                extraInformationBuilder.AppendFormat(", Text.Text=\"{0}\", Text.SupportsTextSelection={1}", textContent.Substring(0, Math.Min(100, textContent.Length)), textImplementation.SupportsTextSelection);
            }

            if (toggleImplementation.IsAvailable)
            {
                patternsSupported.Add("Toggle");
                extraInformationBuilder.AppendFormat(", Toggle.ToggleState={0}", toggleImplementation.ToggleState);
            }

            if (valueImplementation.IsAvailable)
            {
                patternsSupported.Add("Value");
                extraInformationBuilder.AppendFormat(", Value.Value=\"{0}\", Value.IsReadOnly={1}", valueImplementation.Value, valueImplementation.IsReadOnly);
            }

            if (windowImplementation.IsAvailable)
            {
                patternsSupported.Add("Window");
                extraInformationBuilder.AppendFormat(", Window.CanMaximize={0}, Window.CanMinimize={1}, Window.IsModal={2}, Window.WindowVisualState={3}, Window.WindowInteractionState={4}, Window.IsTopmost={5}",
                    windowImplementation.CanMaximize, windowImplementation.CanMinimize, windowImplementation.IsModal,
                    windowImplementation.WindowVisualState, windowImplementation.WindowInteractionState, windowImplementation.IsTopmost);
            }

            string patternsSupportedString = string.Empty;

            if (patternsSupported.Count > 0)
            {
                patternsSupportedString = string.Format(", PatternsSupported={0}", string.Join(";", patternsSupported));
            }

            return string.Format("{0} [Aid={1}, Class={2}, Type={3}{4}{5}]", obj.Name, obj.AutomationId, obj.ClassName, obj.ControlType, patternsSupportedString, extraInformationBuilder.ToString());
        }

        public static string GetText(this UIObject textControl, bool shouldTrim = true)
        {
            string textControlContents = null;
            TextImplementation textImplementation = new TextImplementation(textControl);
            ValueImplementation valueImplementation = new ValueImplementation(textControl);

            if (textImplementation.IsAvailable)
            {
                textControlContents = textImplementation.DocumentRange.GetText(-1);
            }
            else if (valueImplementation.IsAvailable)
            {
                textControlContents = valueImplementation.Value;
            }
            else
            {
                Verify.Fail(string.Format("Expected the text control to implement either IValue or IText. {0} is {1}", string.IsNullOrEmpty(textControl.AutomationId) ? textControl.Name : textControl.AutomationId, textControl.GetDescription()));
            }
            
            // On phone, copying or pasting text causes a space to be added at the end.
            // Unless we explicitly care about that space, we'll remove it from the return value.
            if (shouldTrim)
            {
                textControlContents = textControlContents.Trim();
            }

            return textControlContents;
        }

        public static string GetTextSelection(this UIObject textControl, bool shouldTrim = true)
        {
            string textControlContents = null;
            TextImplementation textImplementation = new TextImplementation(textControl);

            if (textImplementation.IsAvailable)
            {
                textControlContents = textImplementation.SupportsTextSelection ? textImplementation.GetSelection().GetText(-1) : string.Empty;
            }
            else
            {
                Verify.Fail(string.Format("Expected the text control to implement IText. {0} is {1}", string.IsNullOrEmpty(textControl.AutomationId) ? textControl.Name : textControl.AutomationId, textControl.GetDescription()));
            }

            // On phone, copying or pasting text causes a space to be added at the end.
            // Unless we explicitly care about that space, we'll remove it from the return value.
            if (shouldTrim)
            {
                textControlContents = textControlContents.Trim();
            }

            return textControlContents;
        }
    }
}
