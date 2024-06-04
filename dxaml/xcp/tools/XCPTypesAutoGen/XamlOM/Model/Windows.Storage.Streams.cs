// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.Storage.Streams
{
    [Imported("robytestream.idl")]
    [WindowsTypePattern]
    [Guids(ClassGuid = "77ce0a68-5ca5-4cea-b430-8d0f8c27f97e")]
    public interface IBuffer
    {
    }

    [Imported("robytestream.idl")]
    [WindowsTypePattern]
    public interface IRandomAccessStream
    {
    }

    [Imported("storageitem.idl")]
    [WindowsTypePattern]
    public interface IRandomAccessStreamReference
    {
    }

    [Imported("storageitem.idl")]
    [WindowsTypePattern]
    public interface IInputStream
    {
    }
}
