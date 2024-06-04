// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;
using XamlOM.NewBuilders;

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [DXamlName("MediaPlayerElement")]
    [NativeName("CMediaPlayerElement")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "e43b41b4-e101-4624-8a3f-7925febd288d")]
    [TemplatePart("LayoutRoot", typeof(UIElement))]
    [TemplatePart("MediaPlayerPresenter", typeof(Microsoft.UI.Xaml.Controls.MediaPlayerPresenter))]
    [TemplatePart("TransportControlsPresenter", typeof(Microsoft.UI.Xaml.Controls.ContentPresenter))]
    [TemplatePart("TimedTextSourcePresenter", typeof(UIElement))]
    [TemplatePart("PosterImage", typeof(Microsoft.UI.Xaml.Controls.Image))]
    [ClassFlags(IsISupportInitialize = true)]
    [Implements(typeof(Microsoft.UI.Xaml.ComponentModel.ISupportInitialize))]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 3)]
    public class MediaPlayerElement
     : Microsoft.UI.Xaml.Controls.Control
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Media.Playback.IMediaPlaybackSource Source
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.MediaTransportControls TransportControls
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_areTransportControlsEnabled")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Boolean AreTransportControlsEnabled
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, NeedsInvoke = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pPosterSource")]
        [RenderDirtyFlagClassName("CMediaPlayerElement")]
        [RenderDirtyFlagMethodName("NWSetMediaSourceDirty")]
        public Microsoft.UI.Xaml.Media.ImageSource PosterSource
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_Stretch")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Media.Stretch Stretch
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyFlags(NeedsInvoke = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_fAutoPlay")]
        public Windows.Foundation.Boolean AutoPlay
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, NeedsInvoke = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isFullWindow")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Boolean IsFullWindow
        {
            get;
            set;
        }

        public void SetMediaPlayer([Optional]Windows.Media.Playback.MediaPlayer mediaPlayer) {}

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CMediaPlayerElement")]
        [RenderDirtyFlagMethodName("NWSetMediaSourceDirty")]
        public Windows.Media.Playback.MediaPlayer MediaPlayer
        {
            get;
        }
    }
}
