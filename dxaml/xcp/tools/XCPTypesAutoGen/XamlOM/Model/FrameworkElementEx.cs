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
    [NativeName("CFrameworkElementEx")]
    [VelocityFeature("Feature_Xaml2018")]
    [Guids(ClassGuid = "d966489e-8ac3-4851-a506-4ec55d68420d")]
    public abstract class FrameworkElementEx
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
        protected Microsoft.UI.Xaml.Controls.UIElementCollection Children
        {
            get;
        }

        protected FrameworkElementEx()
        {
        }
    }
}
