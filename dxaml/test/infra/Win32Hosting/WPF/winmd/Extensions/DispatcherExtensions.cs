// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.Generic;

namespace System.Windows.Threading
{
    internal static class DispatcherExtensions
    {
        private static Dictionary<Dispatcher, int> disabledDispatchers = new Dictionary<Dispatcher, int>();

        private struct PumpDisabler : IDisposable
        {
            DispatcherProcessingDisabled disabled;
            Dispatcher dispatcher;

            public PumpDisabler(Dispatcher dispatcher)
            {
                this.dispatcher = dispatcher;

                this.disabled = this.dispatcher.DisableProcessing();

                if (dispatcher.IsDisabled())
                {
                    disabledDispatchers[this.dispatcher]++;
                }
                else
                {
                    disabledDispatchers.Add(dispatcher, 1);
                }
            }

            public void Dispose()
            {
                this.disabled.Dispose();

                var refCount = --disabledDispatchers[this.dispatcher];
                if (refCount == 0)
                {
                    disabledDispatchers.Remove(this.dispatcher);
                }
            }
        }

        public static IDisposable DisablePumping(this Dispatcher dispatcher)
        {
            return new PumpDisabler(dispatcher);
        }

        public static bool IsDisabled(this Dispatcher dispatcher)
        {
            return disabledDispatchers.ContainsKey(dispatcher);
        }

        /// <summary>
        /// Equivalent to DoEvents in WinForms
        /// </summary>
        /// <seealso cref="http://msdn.microsoft.com/en-us/library/system.windows.threading.dispatcher.pushframe.aspx"/>
        public static void DoEvents(this Dispatcher dispatcher)
        {
            if (dispatcher.IsDisabled())
            {
                return;
            }

            var frame = new DispatcherFrame();
            dispatcher.BeginInvoke(DispatcherPriority.ApplicationIdle, new Action<DispatcherFrame>((p) =>
            {
                p.Continue = false;
            }), frame);
            
            // this blocks the execution till the frame gets executed, the frame will execute only at an idle time due to its priority
            Dispatcher.PushFrame(frame);   
        }
    }
}

