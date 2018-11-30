// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;

namespace MUXControlsTestApp.Utilities
{
    public class EntityPropertyControlDiscardedEventArgs : EventArgs
    {
        internal EntityPropertyControlDiscardedEventArgs(string propertyName, FrameworkElement propertyControl)
        {
            this.PropertyName = propertyName;
            this.PropertyControl = propertyControl;
        }

        public string PropertyName
        {
            get;
            private set;
        }

        public FrameworkElement PropertyControl
        {
            get;
            private set;
        }
    }
}
