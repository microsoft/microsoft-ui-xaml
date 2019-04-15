// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Reflection;
using Windows.UI.Xaml;

namespace MUXControlsTestApp.Utilities
{
    public class EntityPropertyControlNeededEventArgs : EventArgs
    {
        internal EntityPropertyControlNeededEventArgs(int level, PropertyInfo propertyInfo, bool skipUIAutoGeneration)
        {
            this.Level = level;
            this.PropertyInfo = propertyInfo;
            this.SkipUIAutoGeneration = skipUIAutoGeneration;
        }

        public int Level
        {
            get;
            private set;
        }

        public bool SkipUIAutoGeneration
        {
            get;
            set;
        }

        public PropertyInfo PropertyInfo
        {
            get;
            private set;
        }

        public FrameworkElement PropertyControl
        {
            get;
            set;
        }
    }
}
