// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml
{
    [StrictType]
    [NativeName("CPanelEx")]
    [VelocityFeature("Feature_Xaml2018")]
    [Guids(ClassGuid = "f8330cc6-6a96-44b2-8f59-04f38d07a3c9")]
    [ContentProperty("Children")]
    public class PanelEx
     : Microsoft.UI.Xaml.UIElement
    {
        [NativeMethod("CLayoutElement", "ActualWidth")]
        [NativeStorageType(OM.ValueType.valueFloat)]
        public Windows.Foundation.Double ActualWidth
        {
            get;
        }

        [NativeMethod("CLayoutElement", "ActualHeight")]
        [NativeStorageType(OM.ValueType.valueFloat)]
        public Windows.Foundation.Double ActualHeight
        {
            get;
        }

        [PropertyFlags(AffectsMeasure = true, NeedsInvoke = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_width")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double Width
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, NeedsInvoke = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_height")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double Height
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_minWidth")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double MinWidth
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_maxWidth")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double MaxWidth
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_minHeight")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double MinHeight
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_maxHeight")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double MaxHeight
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_horizontalAlignment")]
        public Microsoft.UI.Xaml.HorizontalAlignment HorizontalAlignment
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_verticalAlignment")]
        public Microsoft.UI.Xaml.VerticalAlignment VerticalAlignment
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(OM.ValueType.valueThickness)]
        [OffsetFieldName("m_margin")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness Margin
        {
            get;
            set;
        }

        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CUIElement", "GetChildren")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetSubgraphDirty")]
        public Microsoft.UI.Xaml.Controls.UIElementCollection Children
        {
            get;
        }

        public PanelEx()
        {
        }
    }
}
