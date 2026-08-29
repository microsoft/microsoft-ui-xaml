// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using Common;
using MUXControlsTestApp.Utilities;

using Windows.Foundation.Metadata;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Input;
using Windows.UI;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public partial class FocusTests : ApiTestBase
    {
        [ClassInitialize]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context) { }

        [TestMethod]
        public void FocusManagerWithXamlRootScope()
        {
            StackPanel root = null;
            Button button1 = null;
            Button button2 = null;

            IdleSynchronizer.TryWait();
            RunOnUIThread.Execute(() =>
            {
                root = new StackPanel();
                button1 = new Button { Content = "Button1", Name="Button1" };
                button2 = new Button { Content = "Button2", Name="Button2" };
                root.Children.Add(button1);
                root.Children.Add(button2);
                MUXControlsTestApp.App.TestContentRoot = root;
            });
            IdleSynchronizer.TryWait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Set focus to button1");
                button1.Focus(FocusState.Keyboard);
            });
            IdleSynchronizer.TryWait();


            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(button1.FocusState, FocusState.Keyboard);

                Log.Comment("Call FindNextElement with a SearchRoot set to XamlRoot.Content, it should not throw");
                FindNextElementOptions options = new FindNextElementOptions {
                    SearchRoot = MUXControlsTestApp.App.TestContentRoot.XamlRoot.Content
                    };
                var next = (FrameworkElement)FocusManager.FindNextElement(FocusNavigationDirection.Next, options);
                Verify.AreEqual(next.Name, button2.Name);
            });
            IdleSynchronizer.TryWait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Call TryMoveFocus with a SearchRoot set to XamlRoot.Content");
                FindNextElementOptions options = new FindNextElementOptions {
                    SearchRoot = MUXControlsTestApp.App.TestContentRoot.XamlRoot.Content
                    };
                FocusManager.TryMoveFocus(FocusNavigationDirection.Next, options);
                // Focus should now be on button2. The FocusState will depend on the last user input device type,
                // so it could be either keyboard or pointer. (TryMoveFocus sets focus as FocusStat.Programmatic,
                // and CoerceFocusState will change that based on lastInputDeviceType.)
                Verify.IsTrue(button2.FocusState == FocusState.Keyboard || button2.FocusState == FocusState.Pointer);
            });
            IdleSynchronizer.TryWait();
        }

    }
}
