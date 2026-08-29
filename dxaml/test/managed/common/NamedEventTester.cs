// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Reflection;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Input;
using WEX.Logging;
using WEX.Logging.Interop;
using WEX.TestExecution;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public class NamedEventTester : IDisposable
    {
        private readonly String eventName;
        private readonly EventWaitHandle eventWaitHandle;

        public NamedEventTester(string eventName)
        {
            this.eventName = eventName;
            this.eventWaitHandle = new EventWaitHandle(false, EventResetMode.ManualReset, eventName);
        }

        public TimeSpan Timeout
        {
            get
            {
                if (Debugger.IsAttached)
                {
                    return TimeSpan.FromMilliseconds(-1); // Wait indefinitely if debugger is attached.
                }
                return TimeSpan.FromSeconds(1);
            }
        }

        public void Wait()
        {
            this.Wait(this.Timeout);
        }

        public void Wait(TimeSpan timeout)
        {
            if (!this.eventWaitHandle.WaitOne(timeout))
            {
                Verify.Fail($"Event '{this.eventName}' was not raised before timeout.");
            }
        }

        public async Task<bool> WaitAsync()
        {
            return await this.WaitAsync(this.Timeout);
        }

        public Task<bool> WaitAsync(TimeSpan timeout)
        {
            var tcs = new TaskCompletionSource<bool>();
            Task.Factory.StartNew(() =>
            {
                tcs.SetResult(this.WaitNoThrow(timeout));
            });
            return tcs.Task;
        }

        public async Task VerifyEventRaised()
        {
            Verify.IsTrue(await this.WaitAsync(), $"Event '{this.eventName}' was raised before {this.Timeout}.");
        }

        public async Task VerifyEventNotRaised()
        {
            Verify.IsFalse(await this.WaitAsync(), $"Event '{this.eventName}' was NOT raised before {this.Timeout}.");
        }

        public bool WaitNoThrow()
        {
            return this.WaitNoThrow(this.Timeout);
        }

        public bool IsFired { get; private set; }

        public bool WaitNoThrow(TimeSpan timeout)
        {
            this.IsFired = this.eventWaitHandle.WaitOne(timeout);
            return this.IsFired;
        }

        private bool disposedValue = false; // To detect redundant calls

        private void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    this.eventWaitHandle.Dispose();
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                disposedValue = true;
            }
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            // GC.SuppressFinalize(this);
        }
    }
}
