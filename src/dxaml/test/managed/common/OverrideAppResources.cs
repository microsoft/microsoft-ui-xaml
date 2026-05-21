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

namespace Microsoft.UI.Xaml.Tests.Common
{
    public class OverrideAppResources : IDisposable
    {
        public OverrideAppResources(string xamlResources)
        {
            UIExecutor.Execute(() =>
            {
                _previousResourceDictionary = Application.Current.Resources;
                Application.Current.Resources = (ResourceDictionary)Microsoft.UI.Xaml.Markup.XamlReader.Load(xamlResources);
            });
        }
        public void Dispose()
        {
            UIExecutor.Execute(() =>
            {
                Application.Current.Resources = _previousResourceDictionary;
            });
        }

        ResourceDictionary _previousResourceDictionary;
    }
}
