// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Windows.Foundation;
using System.Threading;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Tests.Foundation.Graphics.AccessKeys
{
    sealed class KeyTipHelper : IDisposable
    {
        public KeyTipHelper(XamlRoot xamlRoot) : this(xamlRoot, 1.0f)
        {
        }

        public KeyTipHelper(XamlRoot xamlRoot, float zoomScale)
        {
            TestServices.WindowHelper.SetLastInputMethod(global::Private.Infrastructure.LastInputDeviceType.Mouse, xamlRoot);
            TestServices.WindowHelper.SetWindowSizeOverrideWithScale(new Size(400, 400), zoomScale);

            UIExecutor.Execute(() =>
            {
                _previousAreKeyTipsEnabled = AccessKeyManager.AreKeyTipsEnabled;
                AccessKeyManager.AreKeyTipsEnabled = true;
            });
        }

        public void Dispose()
        {
            //Revert zoomScale to 1
            TestServices.WindowHelper.SetWindowSizeOverrideWithScale(new Size(400, 400), 1.0f);
            UIExecutor.Execute(() =>
            {
                AccessKeyManager.AreKeyTipsEnabled = _previousAreKeyTipsEnabled;
                if (AccessKeyManager.IsDisplayModeEnabled)
                {
                    AccessKeyManager.ExitDisplayMode();
                }
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        // If this is popup, return KeyTip text.  Else, null.
        public static string GetKeyTipText(Microsoft.UI.Xaml.Controls.Primitives.Popup p)
        {
            var child1 = p.Child;
            if (child1 != null)
            {
                var child2 = GetOnlyChild(child1);
                if (child2 != null)
                {
                    if (child1 is Border && child2 is TextBlock)
                    {
                        return ((TextBlock)child2).Text;
                    }
                }
            }
            return null;
        }

        // Get number of active KeyTips
        public static int GetKeyTipCount()
        {
            int keyTipCount = 0;
            AutoResetEvent keyTipCountSet = new AutoResetEvent(false);
            UIExecutor.Execute(() =>
            {
                var popups = XamlMedia.VisualTreeHelper.GetOpenPopupsForXamlRoot(
                    TestServices.WindowHelper.WindowContent.XamlRoot);
                foreach (var p in popups)
                {
                    if (GetKeyTipText(p) != null)
                    {
                        keyTipCount++;
                    }
                }
                keyTipCountSet.Set();
            });
            keyTipCountSet.WaitOne();
            return keyTipCount;
        }

        public static void VerifyKeyTipCount(int expected)
        {
            Verify.AreEqual(expected, GetKeyTipCount());
        }

        // Wait until the current state of active KeyTips matches the given string
        public static void WaitUntilKeyTips(string keyTipsExpected)
        {
            string normalizedKeyTipsExpected = NormalizeKeyTipString(keyTipsExpected);
            if (!WaitUntil(() => { return GetKeyTipsString() == normalizedKeyTipsExpected; }, TimeSpan.FromSeconds(1)))
            {
                Verify.Fail(string.Format(
                    "Hit timeout waiting for KeyTips to be '{0}'.  KeyTips are '{1}'",
                    normalizedKeyTipsExpected,
                    GetKeyTipsString()));
            }
        }

        // Verify the given KeyTip string matches the current state of active KeyTips.
        public static void VerifyKeyTips(string keyTipsExpected)
        {
            Verify.AreEqual(NormalizeKeyTipString(keyTipsExpected), GetKeyTipsString());
        }

        // Return a sorted string of KeyTips separated by spaces, so we can do string compares
        static string NormalizeKeyTipString(string unsortedString)
        {
            List<string> keyTipList = new List<string>(unsortedString.Split(' '));
            keyTipList.Sort();
            return string.Join(" ", keyTipList);
        }

        // Return a string of the text of active KeyTips, separated by spaces.
        // e.g., "a b c", represents 3 KeyTips, displaying strings "a" "b", and "c".
        public static string GetKeyTipsString()
        {
            List<string> keyTips = new List<string>();

            AutoResetEvent keyTipsCollectedEvent = new AutoResetEvent(false);
            UIExecutor.Execute(() =>
            {
                var popups = XamlMedia.VisualTreeHelper.GetOpenPopupsForXamlRoot(
                    TestServices.WindowHelper.WindowContent.XamlRoot);
                foreach (var p in popups)
                {
                    string keyTipText = GetKeyTipText(p);
                    if (keyTipText != null)
                    {
                        keyTips.Add(keyTipText);
                    }
                }
                keyTipsCollectedEvent.Set();
            });
            keyTipsCollectedEvent.WaitOne();

            keyTips.Sort();
            return string.Join(" ", keyTips);
        }

        public static void WaitUntilKeyTipCountIs(int expectedPopupCount)
        {
            if (!WaitUntil(() => { return GetKeyTipCount() == expectedPopupCount; }, TimeSpan.FromSeconds(1)))
            {
                Verify.Fail(string.Format(
                    "Hit timeout waiting for KeyTip count to be {0}.  Last KeyTip count is {1}",
                    expectedPopupCount,
                    GetKeyTipCount()));
            }
        }

        public static void WaitUntilAnyKeyTips()
        {
            WaitUntilAnyKeyTips(TimeSpan.FromSeconds(1));
        }

        public static void WaitUntilAnyKeyTips(TimeSpan timeout)
        {
            if (!WaitUntil(() => { return GetKeyTipCount() > 0; }, timeout))
            {
                Verify.Fail("Hit timeout while waiting for KeyTips to appear");
            }

        }


        // The the child of this object, if there's only one.  Else null.
        static DependencyObject GetOnlyChild(DependencyObject obj)
        {
            if (VisualTreeHelper.GetChildrenCount(obj) != 1)
            {
                return null;
            }
            return VisualTreeHelper.GetChild(obj, 0);
        }

        public delegate bool WaitingFor();

        static bool WaitUntil(WaitingFor pred, TimeSpan timeout)
        {
            DateTime startTime = DateTime.Now;
            while (!pred() && DateTime.Now.Subtract(startTime) < timeout)
            {
                Sleep(100); // don't hammer the UI thread too hard...
            }

            return DateTime.Now.Subtract(startTime) < timeout;
        }

        public static void Sleep(int milliseconds)
        {
            new AutoResetEvent(false).WaitOne(milliseconds);
        }

        bool _previousAreKeyTipsEnabled;
    }
}
