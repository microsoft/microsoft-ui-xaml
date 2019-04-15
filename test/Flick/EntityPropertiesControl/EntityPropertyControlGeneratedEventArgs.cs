// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;

namespace MUXControlsTestApp.Utilities
{
    public class EntityPropertyControlGeneratedEventArgs : EventArgs
    {
        internal EntityPropertyControlGeneratedEventArgs(int level, string propertyName, Type propertyType, FrameworkElement propertyControl)
        {
            this.Level = level;
            this.PropertyName = propertyName;
            this.PropertyType = propertyType;
            this.PropertyControl = propertyControl;
        }

        public int Level
        {
            get;
            private set;
        }

        public string PropertyName
        {
            get;
            private set;
        }

        public Type PropertyType
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
