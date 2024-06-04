// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Media.Media3D
{
    [NativeName("CPerspectiveTransform3D")]
    [Guids(ClassGuid = "1ec4edfa-a1e0-433c-81e9-23da5e5962bd")]
    [DXamlIdlGroup("coretypes2")]
    public sealed class PerspectiveTransform3D
     : Microsoft.UI.Xaml.Media.Media3D.Transform3D
    {
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rDepth")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double Depth
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rOffsetX")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double OffsetX
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rOffsetY")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double OffsetY
        {
            get;
            set;
        }

        public PerspectiveTransform3D() { }
    }

    [NativeName("CCompositeTransform3D")]
    [Guids(ClassGuid = "63982bde-b6cd-47f0-9e1f-50cbda62bb89")]
    [DXamlIdlGroup("coretypes2")]
    public sealed class CompositeTransform3D
     : Microsoft.UI.Xaml.Media.Media3D.Transform3D
    {
        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rCenterX")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double CenterX
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object CenterXAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rCenterY")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double CenterY
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object CenterYAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rCenterZ")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double CenterZ
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object CenterZAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rRotationX")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double RotationX
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object RotationXAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rRotationY")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double RotationY
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object RotationYAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rRotationZ")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double RotationZ
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object RotationZAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rScaleX")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double ScaleX
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object ScaleXAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rScaleY")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double ScaleY
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object ScaleYAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rScaleZ")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double ScaleZ
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object ScaleZAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rTranslateX")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double TranslateX
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object TranslateXAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rTranslateY")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double TranslateY
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object TranslateYAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rTranslateZ")]
        [RenderDirtyFlagClassName("CTransform3D")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double TranslateZ
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object TranslateZAnimation
        {
            get;
            set;
        }

        public CompositeTransform3D() { }
    }
}
