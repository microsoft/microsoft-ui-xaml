// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.Foundation.Collections
{
    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public interface IPropertySet
    {
    }

    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public interface IKeyValuePair<TKey, TValue>
    {

    }

    [ClassFlags(IsIDictionary = true)]
    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public interface IMap<TKey, TValue>
    {
    }

    [ClassFlags(IsICollection = true)]
    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public interface IVector<T>
    {
    }

    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public interface IIterable<T>
    {
    }

    [ClassFlags(IsICollection = true)]
    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public interface IObservableVector<T>
    {
    }

    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public interface IVectorChangedEventArgs
    {
    }

    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public interface IVectorView<T>
    {
    }

    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public enum CollectionChange
    {
        Reset = 0,
        ItemInserted = 1,
        ItemRemoved = 2,
        ItemChanged = 3,
    }
    
}

