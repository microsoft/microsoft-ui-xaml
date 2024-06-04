// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Forward declarations for external headers.  Please use in header files instead of declaring manually.

#include <abi/xaml_abi.h>

XAML_ABI_NAMESPACE_BEGIN
namespace Windows {
namespace Media {
    enum AudioBufferAccessMode : int;
    interface IAudioBuffer;
    interface IAudioFrame;
    interface IAudioFrameFactory;
    interface ISystemMediaTransportControls;
namespace Audio {
    class AudioFrameCompletedEventArgs;
    class AudioFrameInputNode;
    class CreateAudioDeviceOutputNodeResult;
    class CreateAudioGraphResult;
    interface IAudioFrameCompletedEventArgs;
    interface IAudioFrameInputNode;
    interface IAudioGraph;
    interface IAudioGraph2;
    interface IAudioGraphSettings;
    interface IAudioGraphSettings2;
    interface IAudioGraphSettingsFactory;
    interface IAudioGraphStatics;
    interface IAudioInputNode;
    interface IAudioInputNode2;
    interface IAudioNode;
    interface IAudioNodeEmitter;
    interface ICreateAudioGraphResult;
} // Audio
namespace Capture {
    interface IMediaCapture;
} // Capture
namespace Casting {
    class CastingConnection;
    class CastingDevicePicker;
    class CastingDeviceSelectedEventArgs;
    class CastingSource;
    enum CastingConnectionErrorStatus : int;
    enum CastingConnectionState : int;
    enum CastingPlaybackTypes : unsigned int;
    interface ICastingConnection;
    interface ICastingDevice;
    interface ICastingDevicePicker;
    interface ICastingDevicePickerFilter;
    interface ICastingDeviceSelectedEventArgs;
    interface ICastingSource;
namespace Internal {
    interface ICastingSourceProperties;
} // Internal
} // Casting
namespace Core {
    class AudioTrack;
    class MediaCueEventArgs;
    class MediaSource;
    class TimedMetadataTrack;
    class TimedTextLine;
    class TimedTextSource;
    class TimedTextSubformat;
    class VideoTrack;
    enum TimedMetadataKind : int;
    enum TimedTextDisplayAlignment : int;
    enum TimedTextFlowDirection : int;
    enum TimedTextFontStyle : int;
    enum TimedTextLineAlignment : int;
    enum TimedTextUnit : int;
    enum TimedTextWeight : int;
    enum TimedTextWrapping : int;
    interface IAudioTrack;
    interface IImageCue;
    interface IMediaCue;
    interface IMediaCueEventArgs;
    interface IMediaSource;
    interface IMediaSource2;
    interface IMediaSourceStatics;
    interface IMediaTrack;
    interface ISingleSelectMediaTrackList;
    interface ITimedMetadataTrack;
    interface ITimedTextCue;
    interface ITimedTextLine;
    interface ITimedTextRegion;
    interface ITimedTextStyle;
    interface ITimedTextStyle2;
    interface ITimedTextSubformat;
    struct TimedTextDouble;
    struct TimedTextPadding;
    struct TimedTextPoint;
    struct TimedTextSize;
} // Core
namespace ClosedCaptioning {
    enum ClosedCaptionColor : int;
    enum ClosedCaptionOpacity : int;
    enum ClosedCaptionSize : int;
    enum ClosedCaptionStyle : int;
    interface IClosedCaptionPropertiesStatics;
} // ClosedCaptioning
namespace MediaProperties {
    interface IAudioEncodingProperties;
    interface IAudioEncodingPropertiesStatics;
    interface IMediaEncodingProperties;
} // MediaProperties
namespace Playback {
    class CurrentMediaPlaybackItemChangedEventArgs;
    class MediaBreak;
    class MediaBreakEndedEventArgs;
    class MediaBreakManager;
    class MediaBreakSkippedEventArgs;
    class MediaBreakStartedEventArgs;
    class MediaPlaybackCommandManagerCommandBehavior;
    class MediaPlaybackItem;
    class MediaPlaybackItemFailedEventArgs;
    class MediaPlaybackList;
    class MediaPlaybackSession;
    class MediaPlaybackTimedMetadataTrackList;
    class MediaPlayer;
    class MediaPlayerFailedEventArgs;
    class TimedMetadataPresentationModeChangedEventArgs;
    enum FailedMediaStreamKind : int;
    enum MediaPlaybackItemErrorCode : int;
    enum MediaPlaybackState : int;
    enum MediaPlayerError : int;
    enum MediaPlayerState : int;
    enum TimedMetadataTrackPresentationMode : int;
    interface ICurrentMediaPlaybackItemChangedEventArgs;
    interface IMediaBreak;
    interface IMediaBreakEndedEventArgs;
    interface IMediaBreakManager;
    interface IMediaBreakSchedule;
    interface IMediaBreakSkippedEventArgs;
    interface IMediaBreakStartedEventArgs;
    interface IMediaPlaybackCommandManager;
    interface IMediaPlaybackCommandManagerCommandBehavior;
    interface IMediaPlaybackItem;
    interface IMediaPlaybackItem2;
    interface IMediaPlaybackItemError;
    interface IMediaPlaybackItemFactory;
    interface IMediaPlaybackItemFailedEventArgs;
    interface IMediaPlaybackList;
    interface IMediaPlaybackSession;
    interface IMediaPlaybackSource;
    interface IMediaPlaybackTimedMetadataTrackList;
    interface IMediaPlayer;
    interface IMediaPlayer2;
    interface IMediaPlayer3;
    interface IMediaPlayer4;
    interface IMediaPlayerFailedEventArgs;
    interface IMediaPlayerSource2;
    interface ITimedMetadataPresentationModeChangedEventArgs;
} // Playback
namespace Protection {
    interface IMediaProtectionManager;
} // Protection
namespace Render {
    enum AudioRenderCategory : int;
} // Render
} // Media
} // Windows
XAML_ABI_NAMESPACE_END
