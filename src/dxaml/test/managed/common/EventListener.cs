// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Tests.Common.EventsListeners
{
    /// <summary>
    /// Provides a strongly typed interface to subscribe to events.
    /// </summary>
    /// <typeparam name="TSender"></typeparam>
    public class EventListener<TSender>
    {
        private Func<TSender, Action, Delegate> _addActionHandlerFunc;
        private Action<TSender, Delegate> _removeActionHandlerFunc;
        private Dictionary<Action, Delegate> _actionDelegateMapping;

        /// <summary>
        /// Initializes a new instance.
        /// </summary>
        public EventListener(
            Func<TSender, Action, Delegate> addActionHandlerFunc,
            Action<TSender, Delegate> removeActionHandlerFunc)
        {
            _addActionHandlerFunc = addActionHandlerFunc;
            _removeActionHandlerFunc = removeActionHandlerFunc;
        }

        /// <summary>
        /// Subscribes to this instance's event and call the given
        /// handler when the event is raised.
        /// </summary>
        public void AddActionHandler(TSender sender, Action handler)
        {
            if(_actionDelegateMapping == null)
            {
                _actionDelegateMapping = new Dictionary<Action,Delegate>();
            }

            Delegate token = _addActionHandlerFunc(sender, handler);
            _actionDelegateMapping[handler] = token;
        }

        /// <summary>
        /// Unsubscribes the given handler from this instance's event.
        /// </summary>
        public void RemoveActionHandler(TSender sender, Action handler)
        {
            Delegate token = null;
            if(_actionDelegateMapping == null || !_actionDelegateMapping.TryGetValue(handler, out token))
            {
                throw new InvalidOperationException();
            }

            _removeActionHandlerFunc(sender, token);
            _actionDelegateMapping.Remove(handler);            
        }
    }
}
