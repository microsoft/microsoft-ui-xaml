// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;
using XamlOM.NewBuilders;

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 3)]
    [DXamlName("MediaPlayerPresenter")]
    [NativeName("CMediaPlayerPresenter")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "c9dc45d1-1576-4ca7-ab3c-b6ae8ae1476e")]
    public class MediaPlayerPresenter
     : Microsoft.UI.Xaml.FrameworkElement
    {
        public MediaPlayerPresenter() { }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Media.Playback.MediaPlayer MediaPlayer
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

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isFullWindow")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Boolean IsFullWindow
        {
            get;
            set;
        }
    }
}

