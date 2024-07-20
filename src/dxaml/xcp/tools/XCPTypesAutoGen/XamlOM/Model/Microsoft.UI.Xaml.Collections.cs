// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;
using XamlOM.NewBuilders;

namespace Microsoft.UI.Xaml.Collections
{
    [NativeName("CCollection")]
    [Modifier(Modifier.Internal)]
    [ClassFlags(IsHiddenFromIdl = true)]
    [HideFromOldCodeGen]
    [HandWritten]
    [Guids(ClassGuid = "793a3945-9f9c-42fc-93b1-5f63bdfb6094")]
    public abstract class PresentationFrameworkCollection : DependencyObject
    {
        [NativeMethod("CCollection", "GetItemCount")]
        [ReadOnly]
        public Windows.Foundation.Double Count
        {
            get;
            private set;
        }

        internal PresentationFrameworkCollection()
        {
        }
    }

    [NativeName("CCollection")]
    [TypeFlags(IsDXamlSystemType = true)]
    [ClassFlags(IsTypeConverter = true, IsHiddenFromIdl = true)]
    [OldCodeGenBaseType(typeof(DependencyObject))]
    public class PresentationFrameworkCollection<T>
     : PresentationFrameworkCollection
    {
        [NativeMethod("CCollection", "GetItemCount")]
        [NativeStorageType(ValueType.valueFloat)]
        [ReadOnly]
        [HideFromNewCodeGen]
        public new Windows.Foundation.Double Count
        {
            get;
            private set;
        }

        public PresentationFrameworkCollection() { }
    }

    [ClassFlags(IsHiddenFromIdl = true)]
    [HideFromOldCodeGen]
    public class ObservablePresentationFrameworkCollection<T> : PresentationFrameworkCollection<T>
    {
    }
}

