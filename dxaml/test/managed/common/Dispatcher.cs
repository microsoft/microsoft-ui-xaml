// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

using Windows.ApplicationModel.Core;
using Windows.UI.Core;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public interface IDispatcher
    {
        bool CheckAccess();
        void Invoke(Action a);
        void BeginInvoke(Action a);
    }

    /// <summary>
    /// Represents a Dispatcher that's based on CoreDispatcher.
    /// ActionQueue should be used except for the very basic stuff.
    /// </summary>
    public class Dispatcher : IDispatcher
    {
        private CoreDispatcher _coreDispatcher = null;

        public global::Windows.UI.Core.CoreDispatcher CoreDispatcher { get { return _coreDispatcher; } }

        public Dispatcher(CoreDispatcher coreDispatcher)
        {
            _coreDispatcher = coreDispatcher;
        }

        public bool CheckAccess()
        {
            if (_coreDispatcher != null)
            {
                return _coreDispatcher.HasThreadAccess;
            }
            else
            {
                return false;
            }
        }

        public void Invoke(Action action)
        {
            var resetEvent = new ManualResetEvent(false);
            Exception thrownException = null;

            BeginInvoke(() =>
            {
                try
                {
                    action();
                }
                catch (Exception ex)
                {
                    thrownException = ex;
                }
                finally
                {
                    resetEvent.Set();
                }
            });

            resetEvent.WaitOne();

            if (thrownException != null)
            {
                throw new Exception(thrownException.Message, thrownException);
            }
        }

        public void BeginInvoke(Action a)
        {
            var _ = _coreDispatcher.RunAsync(global::Windows.UI.Core.CoreDispatcherPriority.Low, () => a());
        }
    }
}
