// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.Media.Playback
{
    [Imported("windows.media.idl")]
    [WindowsTypePattern]
    public enum FailedMediaStreamKind
    {
        Unknown = 0,
        Audio = 1,
        Video = 2,
    }

    [Imported("windows.media.idl")]
    [DXamlName("MediaPlaybackItem")]
    [WindowsTypePattern]
    public sealed class MediaPlaybackItem
    {
    }

    [Imported("windows.media.idl")]
    [WindowsTypePattern]
    public interface IMediaPlaybackSource
    {
    }

    [Imported("windows.media.idl")]
    [WindowsTypePattern]
    public interface IMediaPlaybackItem
    {
    }

    [Imported("windows.media.playback.idl")]
    [WindowsTypePattern]
    public sealed class MediaPlayer
    {
    }
}
