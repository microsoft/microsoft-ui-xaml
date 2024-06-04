// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Media.Media3D
{

    [NativeName("CTransform3D")]
    [Guids(ClassGuid = "4dfc066b-3bd3-4c61-9af0-e889963a26b3")]
    public abstract class Transform3D
     : Microsoft.UI.Xaml.DependencyObject
    {
        protected Transform3D() { }
    }

    [CodeGen(partial: true)]
    [BuiltinStruct("CMatrix4x4")]
    [Guids(ClassGuid = "99f793d3-5388-4142-befb-b32b158b417a")]
    public struct Matrix3D
    {

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._11")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M11
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._12")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M12
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._13")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M13
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._14")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M14
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._21")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M21
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._22")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M22
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._23")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M23
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._24")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M24
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._31")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M31
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._32")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M32
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._33")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M33
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._34")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M34
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._41")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double OffsetX
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._42")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double OffsetY
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._43")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double OffsetZ
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._44")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double M44
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyFlags(IsHelper = true)]
        [ReadOnly]
        public Windows.Foundation.Boolean HasInverse
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.Media.Media3D.Matrix3D Identity
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyFlags(IsHelper = true)]
        [ReadOnly]
        public Windows.Foundation.Boolean IsIdentity
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        [Mutator]
        public void Invert()
        {
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Media.Media3D.Matrix3D Multiply(Microsoft.UI.Xaml.Media.Media3D.Matrix3D matrix1, Microsoft.UI.Xaml.Media.Media3D.Matrix3D matrix2)
        {
            return default(Microsoft.UI.Xaml.Media.Media3D.Matrix3D);
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Media.Media3D.Matrix3D FromElements(Windows.Foundation.Double m11, Windows.Foundation.Double m12, Windows.Foundation.Double m13, Windows.Foundation.Double m14, Windows.Foundation.Double m21, Windows.Foundation.Double m22, Windows.Foundation.Double m23, Windows.Foundation.Double m24, Windows.Foundation.Double m31, Windows.Foundation.Double m32, Windows.Foundation.Double m33, Windows.Foundation.Double m34, Windows.Foundation.Double offsetX, Windows.Foundation.Double offsetY, Windows.Foundation.Double offsetZ, Windows.Foundation.Double m44)
        {
            return default(Microsoft.UI.Xaml.Media.Media3D.Matrix3D);
        }
    }

}

