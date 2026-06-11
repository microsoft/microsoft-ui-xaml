// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Tests.Common;

namespace Private.Infrastructure
{
    public static class UtilitiesExtensions
    {
        public static IDisposable CreateRenderingScopeGuard(
            this Utilities @this,
            DCompRendering rendering,
            bool resizeWindow = true,
            bool injectMockDComp = true,
            bool resetDevice = false,
            bool resetWindowContent = true)
        {
            return @this.CreateRenderingScopeGuard(rendering, resizeWindow, injectMockDComp, resetDevice, resetWindowContent);
        }

        private class DpiChangedEventTester: EventTester<
            Windows.Graphics.Display.DisplayInformation,
            object>
        {
            private DpiChangedEventTester(Windows.Graphics.Display.DisplayInformation displayInfo)
                : base(displayInfo, "DpiChanged")
            {
            }

            public static DpiChangedEventTester Create()
            {
                var displayInfo = default(Windows.Graphics.Display.DisplayInformation);
                UIExecutor.Execute(() =>
                {
                    displayInfo = Windows.Graphics.Display.DisplayInformation.GetForCurrentView();
                });
                return new DpiChangedEventTester(displayInfo);
            }
        }

        private static async Task<IEnumerable<IDisposable>> SetDpiForAll(
            this Utilities @this,
            DisplayDPIRange dpiOverride)
        {
            var allRestoreDpi = new List<IDisposable>();
            var qsFilfer = Windows.Devices.Display.DisplayMonitor.GetDeviceSelector();
            using(var dpiChangedEvent = DpiChangedEventTester.Create())
            {
                foreach(var device in await Windows.Devices.Enumeration.DeviceInformation.FindAllAsync(qsFilfer))
                {
                    var monitor = await Windows.Devices.Display.DisplayMonitor.FromInterfaceIdAsync(device.Id);
                    var restoreDpi = @this.SetDpi(monitor.DisplayAdapterId, 0 /*monitor.DisplayAdapterTargetId*/, dpiOverride);
                    if (restoreDpi!=null)
                    {
                        //DPI actually changed, save restorer
                        allRestoreDpi.Add(restoreDpi);
                    }
                }
                if (allRestoreDpi.Any())
                {
                    await dpiChangedEvent.VerifyEventRaised();
                }
            }
            return allRestoreDpi;
        }

        private class DisposeAll: IDisposable
        {
            private readonly IEnumerable<IDisposable> disposers;

            public DisposeAll(IEnumerable<IDisposable> disposers)
            {
                this.disposers = disposers;
            }

            void IDisposable.Dispose()
            {
                using(var dpiChangedEvent = DpiChangedEventTester.Create())
                {
                    foreach(var disposer in this.disposers)
                    {
                        disposer.Dispose();
                    }
                    if (this.disposers.Any())
                    {
                        dpiChangedEvent.Wait();
                    }
                }
            }
        }

        public static async Task<IDisposable> ChangeDpi(
            this Utilities @this,
            DisplayDPIRange dpiOverride = DisplayDPIRange.AboveDefaultFirst)
        {
            var allRestoreDpi = await @this.SetDpiForAll(dpiOverride);
            return new DisposeAll(allRestoreDpi);
        }
    }
}

