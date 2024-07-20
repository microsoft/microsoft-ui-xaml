// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Shapes
{
    [NativeName("CShape")]
    [Guids(ClassGuid = "e8768ede-cdd5-4bc1-b75b-0b05e81da93e")]
    public abstract class Shape
     : Microsoft.UI.Xaml.FrameworkElement
    {

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pFill")]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetFillBrushDirty")]
        public Microsoft.UI.Xaml.Media.Brush Fill
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, HadFieldInBlue = true)]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetStrokeBrushDirty")]
        [NativeMethod("CShape", "Stroke")]
        public Microsoft.UI.Xaml.Media.Brush Stroke
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double StrokeMiterLimit
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double StrokeThickness
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Media.PenLineCap StrokeStartLineCap
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Media.PenLineCap StrokeEndLineCap
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Media.PenLineJoin StrokeLineJoin
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double StrokeDashOffset
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Media.PenLineCap StrokeDashCap
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [PropertyFlags(IsValueCreatedOnDemand = true, HadFieldInBlue = true)]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [NativeMethod("CShape", "StrokeDashArray")]
        public Microsoft.UI.Xaml.Media.DoubleCollection StrokeDashArray
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

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CShape", "GetShapeGeometryTransform")]
        [ReadOnly]
        public Microsoft.UI.Xaml.Media.Transform GeometryTransform
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Composition.CompositionBrush GetAlphaMask()
        {
            return default(Microsoft.UI.Composition.CompositionBrush);
        }

        protected Shape() { }
    }

    [NativeName("CPath")]
    [Guids(ClassGuid = "c60b2ff5-9a62-49e4-9012-3d2752ff3c3e")]
    public class Path
     : Microsoft.UI.Xaml.Shapes.Shape
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pGeometryData")]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetGeometryDirty")]
        public Microsoft.UI.Xaml.Media.Geometry Data
        {
            get;
            set;
        }

        public Path() { }
    }

    [NativeName("CEllipse")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "af868e54-8484-4a49-ace2-03c94ce7c23a")]
    public class Ellipse
     : Microsoft.UI.Xaml.Shapes.Shape
    {
        public Ellipse() { }
    }

    [NativeName("CLine")]
    [Guids(ClassGuid = "d0d8a3d1-7559-4055-b841-4dd9684e2438")]
    public sealed class Line
     : Microsoft.UI.Xaml.Shapes.Shape
    {
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eX1")]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetGeometryDirty")]
        public Windows.Foundation.Double X1
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eY1")]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetGeometryDirty")]
        public Windows.Foundation.Double Y1
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eX2")]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetGeometryDirty")]
        public Windows.Foundation.Double X2
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eY2")]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetGeometryDirty")]
        public Windows.Foundation.Double Y2
        {
            get;
            set;
        }

        public Line() { }
    }

    [NativeName("CPolygon")]
    [Guids(ClassGuid = "2a3756f2-801e-4947-865e-06041fbecf7c")]
    public sealed class Polygon
     : Microsoft.UI.Xaml.Shapes.Shape
    {

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_nFillRule")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Media.FillRule FillRule
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true, IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pPoints")]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetGeometryDirty")]
        public Microsoft.UI.Xaml.Media.PointCollection Points
        {
            get;
            set;
        }

        public Polygon() { }
    }

    [NativeName("CPolyline")]
    [Guids(ClassGuid = "0d7b50f1-4277-44c1-995c-a4ba0d863140")]
    public sealed class Polyline
     : Microsoft.UI.Xaml.Shapes.Shape
    {

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_nFillRule")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Media.FillRule FillRule
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true, IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pPoints")]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetGeometryDirty")]
        public Microsoft.UI.Xaml.Media.PointCollection Points
        {
            get;
            set;
        }

        public Polyline() { }
    }

    [NativeName("CRectangle")]
    [Guids(ClassGuid = "f08ddae9-8acc-4bf5-aa40-0e5431ec3409")]
    public sealed class Rectangle
     : Microsoft.UI.Xaml.Shapes.Shape
    {
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eRadiusX")]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetGeometryDirty")]
        public Windows.Foundation.Double RadiusX
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eRadiusY")]
        [RenderDirtyFlagClassName("CShape")]
        [RenderDirtyFlagMethodName("NWSetGeometryDirty")]
        public Windows.Foundation.Double RadiusY
        {
            get;
            set;
        }

        public Rectangle() { }
    }

}

