// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Text;
using System.Linq;
using System.Threading.Tasks;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;

using Microsoft.UI.Xaml.Input;
using Windows.Foundation;

using Microsoft.UI.Xaml.Documents;

using Microsoft.UI.Xaml.Controls;

namespace Microsoft.UI.Xaml.Tests.Foundation.Graphics.AccessKeys
{
    class AccessKeyHelper : IDisposable
    {
        public static AccessKeyHelper CreateWithKeyTips(XamlRoot xamlRoot)
        {
            return new AccessKeyHelper(xamlRoot, true);
        }

        public static AccessKeyHelper CreateNoKeyTips(XamlRoot xamlRoot)
        {
            return new AccessKeyHelper(xamlRoot, false);
        }

        public AccessKeyHelper(XamlRoot xamlRoot, bool enableKeyTips)
        {
            TestServices.WindowHelper.SetLastInputMethod(global::Private.Infrastructure.LastInputDeviceType.Mouse, xamlRoot);
            TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

            UIExecutor.Execute(() =>
            {
                _wereKeyTipsEnabled = AccessKeyManager.AreKeyTipsEnabled;
                AccessKeyManager.AreKeyTipsEnabled = enableKeyTips;
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        public void Dispose()
        {
            UIExecutor.Execute(() =>
            {
                if (AccessKeyManager.IsDisplayModeEnabled)
                {
                    AccessKeyManager.ExitDisplayMode();
                }
                AccessKeyManager.AreKeyTipsEnabled = _wereKeyTipsEnabled;
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        public static void TryMovingFocusToXamlForInit()
        {
            UIExecutor.Execute(() =>
            {
                var element = FocusManager.FindFirstFocusableElement(TestServices.WindowHelper.WindowContent) as Microsoft.UI.Xaml.UIElement;
                if (element != null)
                {
                    element.Focus(FocusState.Programmatic);
                }
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        bool _wereKeyTipsEnabled;
    }
}
