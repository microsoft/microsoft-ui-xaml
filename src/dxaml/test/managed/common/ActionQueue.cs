// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public interface IActionQueue
    {
        IDispatcher Dispatcher { get; }

        IActionQueue EnqueueAction(Action action);
        IActionQueue ScheduleOnUIThread(Action action);
        IActionQueue ScheduleOnUIThreadAsync(Action action);
        
        IActionQueue Commit();
        void CommitAsync();
    }

    /// <summary>
    /// Represents a queue of actions that can be executed synchronously or asynchronously on
    /// or off the UI thread.
    /// </summary>
    public class ActionQueue : IActionQueue
    {
        // Used to execute code on the UI thread.
        private readonly IDispatcher _dispatcher;

        // List of input actions to be executed when the ActionQueue is committed.
        private readonly Queue<Action> _actionsQueue = new Queue<Action>();

        // Specifies that the queue is being committed.
        private bool _isQueueBusy;

        /// <summary>
        /// Gets the dispatcher used by this queue.
        /// </summary>
        public IDispatcher Dispatcher
        {
            get { return _dispatcher; }
        }

        /// <summary>
        /// Initializes a new instance of ActionQueue with the
        /// given dispatcher.
        /// </summary>
        public ActionQueue(IDispatcher dispatcher)
        {
            _dispatcher = dispatcher;
        }

        /// <summary>
        /// Enqueues an action that's going to be executed
        /// when Commit or CommitAsync are called in a FIFO manner.
        /// </summary>
        public IActionQueue EnqueueAction(Action action)
        {
            EnsureReady();
            _actionsQueue.Enqueue(action);
            return this;
        }

        /// <summary>
        /// Enqueues an action that's going to be synchronously executed on the UI thread.
        /// </summary>
        public IActionQueue ScheduleOnUIThread(Action action)
        {
            EnqueueAction(() => _dispatcher.Invoke(action));
            return this;
        }

        /// <summary>
        /// Enqueues an action that's going to be asynchronously executed on the UI thread.
        /// </summary>
        public IActionQueue ScheduleOnUIThreadAsync(Action action)
        {
            EnqueueAction(() => _dispatcher.BeginInvoke(action));
            return this;
        }

        /// <summary>
        /// Executes all the actions in the queue synchronously.
        /// </summary>
        public IActionQueue Commit()
        {
            EnsureReady();
            RunAllTests();            

            return this;
        }

        /// <summary>
        /// Executes all the actions in the queue asynchronously.
        /// </summary>
        public async void CommitAsync()
        {
            EnsureReady();

            await Task.Run(() =>
            {
                Commit();
            });
        }

        /// <summary>
        /// Test that the queued actions are not being executed.
        /// </summary>
        private void EnsureReady()
        {
            if(_isQueueBusy)
            {
                throw new InvalidOperationException("Queued actions are being executed.");
            }
        }

        /// <summary>
        /// Executes all the queued actions.
        /// </summary>
        private void RunAllTests()
        {
            try
            {
                _isQueueBusy = true;

                while (_actionsQueue.Count > 0)
                {
                    var action = _actionsQueue.Dequeue();
                    action();
                }
            }
            finally
            {
                _isQueueBusy = false;
                _actionsQueue.Clear();
            }
        }
    }
}
