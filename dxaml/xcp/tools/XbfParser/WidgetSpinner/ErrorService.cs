// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.Xaml.WidgetSpinner
{
    /// <summary>
    /// A singleton class for listening to warnings/errors raised by the WidgetSpinner
    /// library
    /// </summary>
    public class ErrorService
    {
        /// <summary>
        /// Get the singleton instance
        /// </summary>
        public static ErrorService Instance { get; } = new ErrorService();

        /// <summary>
        /// Signals a non-fatal warning
        /// </summary>
        public event EventHandler<EventArgsWithMessage> Warning;

        /// <summary>
        /// Signals a fatal error
        /// </summary>
        public event EventHandler<EventArgsWithMessage> Error;

        private ErrorService()
        {
        }

        internal void RaiseWarning(string message)
        {
            var handler = Warning;
            if (handler != null)
            {
                handler(this, new EventArgsWithMessage(message));
            }
        }

        internal void RaiseError(string message)
        {
            var handler = Error;
            if (handler != null)
            {
                handler(this, new EventArgsWithMessage(message));
            }
        }
    }

    public class EventArgsWithMessage : EventArgs
    {
        public string Message { get; }

        public EventArgsWithMessage(string message)
        {
            Message = message;
        }
    }
}
