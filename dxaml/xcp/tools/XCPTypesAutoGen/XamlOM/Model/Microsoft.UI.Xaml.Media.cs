// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Windows.Foundation;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Media
{
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [DXamlIdlGroup("Controls")]
    [HandWritten]
    [Guids(ClassGuid = "293c5120-987e-4ab1-8d21-78a8e4e8535e")]
    public static class VisualTreeHelper
    {
        [DXamlOverloadName("FindElementsInHostCoordinates")]
        [DefaultOverload]
        [ReturnTypeParameterName("elements")]
        public static Windows.Foundation.Collections.IIterable<UIElement> FindElementsInHostCoordinatesPoint(
            Windows.Foundation.Point intersectingPoint,
            UIElement subtree)
        {
            return null;
        }

        [DXamlOverloadName("FindElementsInHostCoordinates")]
        [ReturnTypeParameterName("elements")]
        public static Windows.Foundation.Collections.IIterable<UIElement> FindElementsInHostCoordinatesRect(
            Windows.Foundation.Rect intersectingRect,
            UIElement subtree)
        {
            return null;
        }

        [DXamlOverloadName("FindElementsInHostCoordinates")]
        [DefaultOverload]
        [ReturnTypeParameterName("elements")]
        public static Windows.Foundation.Collections.IIterable<UIElement> FindAllElementsInHostCoordinatesPoint(
            Windows.Foundation.Point intersectingPoint,
            UIElement subtree,
            bool includeAllElements)
        {
            return null;
        }

        [DXamlOverloadName("FindElementsInHostCoordinates")]
        [ReturnTypeParameterName("elements")]
        public static Windows.Foundation.Collections.IIterable<UIElement> FindAllElementsInHostCoordinatesRect(
            Windows.Foundation.Rect intersectingRect,
            UIElement subtree,
            bool includeAllElements)
        {
            return null;
        }

        [ReturnTypeParameterName("child")]
        public static DependencyObject GetChild(DependencyObject reference, int childIndex)
        {
            return null;
        }

        [ReturnTypeParameterName("count")]
        public static int GetChildrenCount(DependencyObject reference)
        {
            return default(int);
        }

        [ReturnTypeParameterName("parent")]
        public static DependencyObject GetParent(DependencyObject reference)
        {
            return null;
        }

        public static void DisconnectChildrenRecursive(UIElement element)
        {
        }

        [ReturnTypeParameterName("popups")]
        public static Windows.Foundation.Collections.IVectorView<Microsoft.UI.Xaml.Controls.Primitives.Popup> GetOpenPopups(Window window)
        {
            return null;
        }

        [ReturnTypeParameterName("popups")]
        public static Windows.Foundation.Collections.IVectorView<Microsoft.UI.Xaml.Controls.Primitives.Popup> GetOpenPopupsForXamlRoot(Microsoft.UI.Xaml.XamlRoot xamlRoot)
        {
            return null;
        }
    }

    [StubDelegate]
    public delegate void RenderedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Media.RenderedEventArgs args);

    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [HandWritten]
    [Guids(ClassGuid = "d203542c-fc98-419a-b563-0dfcf029301c")]
    public static class CompositionTarget
    {
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public static event Microsoft.UI.Xaml.EventHandler Rendering;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public static event Microsoft.UI.Xaml.Media.RenderedEventHandler Rendered;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public static event Microsoft.UI.Xaml.EventHandler SurfaceContentsLost;

        [ReturnTypeParameterName("result")]
        public static Microsoft.UI.Composition.Compositor GetCompositorForCurrentThread()
        {
            return default(Microsoft.UI.Composition.Compositor);
        }
    }

    [NativeName("CDoubleCollection")]
    [ClassFlags(HasTypeConverter = true, HasBaseTypeInDXamlInterface = false)]
    [Guids(ClassGuid = "5e54181a-5b45-4480-b620-dac941726ed4")]
    public sealed class DoubleCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Windows.Foundation.Double>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double ContentProperty
        {
            get;
            set;
        }

        public DoubleCollection() { }
    }

    [NativeName("CFloatCollection")]
    [ClassFlags(HasTypeConverter = true, ForceIncludeInManifest = true)]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "a8e53ad3-a71d-433d-a5ab-643a8fbe6c68")]
    public sealed class FloatCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Windows.Foundation.Float>
    {
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        public Windows.Foundation.Float ContentProperty
        {
            get;
            set;
        }

        public FloatCollection() { }
    }

    [NativeName("CPointCollection")]
    [ClassFlags(HasTypeConverter = true, HasBaseTypeInDXamlInterface = false)]
    [Guids(ClassGuid = "78c97fd6-09b0-4b74-a6bc-52f58d7b909a")]
    public sealed class PointCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Windows.Foundation.Point>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point ContentProperty
        {
            get;
            set;
        }

        public PointCollection() { }
    }

    [AllowsMultipleAssociations]
    [NativeName("CGeneralTransform")]
    [Guids(ClassGuid = "e1d641fe-43f4-4fa8-bbdf-8d0230bcd65d")]
    public abstract class GeneralTransform
     : Microsoft.UI.Xaml.DependencyObject
    {
        [ReadOnly]
        public abstract Microsoft.UI.Xaml.Media.GeneralTransform Inverse
        {
            get;
        }

        protected GeneralTransform() { }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeClassName("CGeneralTransform")]
        public Windows.Foundation.Point TransformXY(Windows.Foundation.Double x, Windows.Foundation.Double y)
        {
            return default(Windows.Foundation.Point);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeClassName("CGeneralTransform")]
        public Windows.Foundation.Point TransformPoint(Windows.Foundation.Point point)
        {
            return default(Windows.Foundation.Point);
        }

        [TypeTable(IsExcludedFromCore = true)]
        public abstract Windows.Foundation.Boolean TryTransform(Windows.Foundation.Point inPoint, out Windows.Foundation.Point outPoint);

        [TypeTable(IsExcludedFromCore = true)]
        public abstract Windows.Foundation.Rect TransformBounds(Windows.Foundation.Rect rect);
    }

    [CodeGen(partial: true)]
    [NativeName("CTransform")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "aa27af08-40e8-49de-b181-d718746f0af7")]
    public abstract class Transform
     : Microsoft.UI.Xaml.Media.GeneralTransform
    {
        internal Transform() { }
    }

    [NativeName("CPathSegment")]
    [Guids(ClassGuid = "62874795-cf7d-4703-905a-3cc2aaada0df")]
    public abstract class PathSegment
     : Microsoft.UI.Xaml.DependencyObject
    {
        internal PathSegment() { }
    }

    [CodeGen(partial: true)]
    [AllowsMultipleAssociations]
    [NativeName("CBrush")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "b42803d2-98fd-4ade-adc0-427c144a79e7")]
    [Platform("Feature_XamlMotionSystemHoldbacks", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Velocity(Feature = "Feature_XamlMotionSystemHoldbacks")]
    [Implements(typeof(Microsoft.UI.Composition.IAnimationObject), Version = 1)]
    public abstract class Brush
     : Microsoft.UI.Xaml.DependencyObject
    {

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eOpacity")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double Opacity
        {
            get;
            set;
        }

        [Strictness(Strictness.NonStrictOnly)]
        [PropertyFlags(HadFieldInBlue = true)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Transform Transform
        {
            get;
            set;
        }

        [Strictness(Strictness.NonStrictOnly)]
        [PropertyFlags(HadFieldInBlue = true)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Transform RelativeTransform
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void StartAnimation(Microsoft.UI.Composition.ICompositionAnimationBase animation)
        {
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void StopAnimation(Microsoft.UI.Composition.ICompositionAnimationBase animation)
        {
        }

        protected Brush() { }

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void PopulatePropertyInfoOverride(Windows.Foundation.String propertyName, Microsoft.UI.Composition.AnimationPropertyInfo animationPropertyInfo)
        {
        }

    }

    [NativeName("CTileBrush")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "a06927f4-b09e-4fd7-af59-8d77b80b2a84")]
    public abstract class TileBrush
     : Microsoft.UI.Xaml.Media.Brush
    {

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_AlignmentX")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.AlignmentX AlignmentX
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_AlignmentY")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.AlignmentY AlignmentY
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_Stretch")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Stretch Stretch
        {
            get;
            set;
        }

        protected TileBrush() { }
    }


    [Platform(typeof(PrivateApiContract), 1)]
    public interface IXamlCompositionBrushBaseOverridesPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        void OnElementConnected(Microsoft.UI.Xaml.DependencyObject element);
    }

    // ContentRoot temporarily an IInspectable, since MIDL prohibits internal interfaces under Velocity
    [Platform(typeof(PrivateApiContract), 1)]
    public interface IXamlCompositionBrushBasePrivates
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetBrushForXamlRoot(Windows.Foundation.Object xamlRoot, Microsoft.UI.Composition.CompositionBrush brush);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Composition.CompositionBrush GetBrushForXamlRoot(Windows.Foundation.Object xamlRoot);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ClearBrushForXamlRoot(Windows.Foundation.Object xamlRoot, Microsoft.UI.Composition.CompositionBrush brush);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ClearCompositionBrushMap();
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("coretypes2")]
    [NativeName("CXamlCompositionBrush")]
    [Implements(typeof(IXamlCompositionBrushBasePrivates))]
    [Guids(ClassGuid = "4c29bfbe-4cff-4b3e-b75f-8217d05f0478")]
    public abstract class XamlCompositionBrushBase
     : Microsoft.UI.Xaml.Media.Brush
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnConnected()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnDisconnected()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromCore = true)]
        protected Microsoft.UI.Composition.CompositionBrush CompositionBrush
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueColor)]
        [OffsetFieldName("m_fallbackColor")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.UI.Color FallbackColor
        {
            get;
            set;
        }

        protected XamlCompositionBrushBase() { }
    }

    [NativeName("CBrushCollection")]
    [ClassFlags(HasTypeConverter = true, HasBaseTypeInDXamlInterface = false)]
    [Guids(ClassGuid = "7aa6f6b1-8357-4496-907e-1e44e9f5b419")]
    public sealed class BrushCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Brush>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Brush ContentProperty
        {
            get;
            set;
        }

        public BrushCollection() { }
    }

    [NativeName("CXamlLight")]
    [Guids(ClassGuid = "dc77f7b0-d742-474a-9104-6612c0a1a951")]
    public class XamlLight
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(ValueType.valueObject)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected Microsoft.UI.Composition.CompositionLight CompositionLight
        {
            get;
            set;
        }

        public XamlLight() { }

        protected virtual Windows.Foundation.String GetId()
        {
            return default(Windows.Foundation.String);
        }

        protected virtual void OnConnected(Microsoft.UI.Xaml.UIElement newElement)
        {
        }

        protected virtual void OnDisconnected(Microsoft.UI.Xaml.UIElement oldElement)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndStub)]
        public static void AddTargetElement(Windows.Foundation.String lightId, Microsoft.UI.Xaml.UIElement element)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndStub)]
        public static void RemoveTargetElement(Windows.Foundation.String lightId, Microsoft.UI.Xaml.UIElement element)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndStub)]
        public static void AddTargetBrush(Windows.Foundation.String lightId, Microsoft.UI.Xaml.Media.Brush brush)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndStub)]
        public static void RemoveTargetBrush(Windows.Foundation.String lightId, Microsoft.UI.Xaml.Media.Brush brush)
        {
        }
    }

    [NativeName("CXamlLightCollection")]
    [ClassFlags(HasTypeConverter = true, HasBaseTypeInDXamlInterface = false)]
    [Guids(ClassGuid = "deb37c56-5ccd-40a5-baa6-5c44f179fdb1")]
    internal sealed class XamlLightCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<XamlLight>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public XamlLight ContentProperty
        {
            get;
            set;
        }

        public XamlLightCollection() { }
    }


    [NativeName("CGeometry")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "72f9e035-f735-489c-8b95-b4cbfb5c2473")]
    public abstract class Geometry
     : Microsoft.UI.Xaml.DependencyObject
    {
        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTransform")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Transform Transform
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CGeometry", "GetBounds")]
        [ReadOnly]
        public Windows.Foundation.Rect Bounds
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.Media.Geometry Empty
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Windows.Foundation.Double StandardFlatteningTolerance
        {
            get;
            private set;
        }

        internal Geometry() { }
    }

    [AllowsMultipleAssociations]
    [NativeName("CImageSource")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "ebd5cce4-4881-4837-9555-a0ee51333e7d")]
    public abstract class ImageSource
     : Microsoft.UI.Xaml.DependencyObject
    {
        internal ImageSource() { }
    }

    [NativeName("CRotateTransform")]
    [Guids(ClassGuid = "ea45e69d-7527-4314-aea3-1bba433c65a9")]
    public sealed class RotateTransform
     : Microsoft.UI.Xaml.Media.Transform
    {
        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_ptCenter.x")]
        [RenderDirtyFlagClassName("CTransform")]
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
        [OffsetFieldName("m_ptCenter.y")]
        [RenderDirtyFlagClassName("CTransform")]
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
        [OffsetFieldName("m_eAngle")]
        [RenderDirtyFlagClassName("CTransform")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double Angle
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object AngleAnimation
        {
            get;
            set;
        }

        public RotateTransform() { }

        public override Microsoft.UI.Xaml.Media.GeneralTransform Inverse
        {
            get { return default(Microsoft.UI.Xaml.Media.GeneralTransform); }

        }

        public override Windows.Foundation.Boolean TryTransform(Windows.Foundation.Point inPoint, out Windows.Foundation.Point outPoint)
        {
            outPoint = default(Windows.Foundation.Point);
            return default(Windows.Foundation.Boolean);
        }

        public override Windows.Foundation.Rect TransformBounds(Windows.Foundation.Rect rect)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [NativeName("CScaleTransform")]
    [Guids(ClassGuid = "9392595f-8bdb-43df-b216-64f488c6549c")]
    public sealed class ScaleTransform
     : Microsoft.UI.Xaml.Media.Transform
    {
        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_ptCenter.x")]
        [RenderDirtyFlagClassName("CTransform")]
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
        [OffsetFieldName("m_ptCenter.y")]
        [RenderDirtyFlagClassName("CTransform")]
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
        [OffsetFieldName("m_eScaleX")]
        [RenderDirtyFlagClassName("CTransform")]
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
        [OffsetFieldName("m_eScaleY")]
        [RenderDirtyFlagClassName("CTransform")]
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

        public ScaleTransform() { }

        public override Microsoft.UI.Xaml.Media.GeneralTransform Inverse
        {
            get { return default(Microsoft.UI.Xaml.Media.GeneralTransform); }

        }

        public override Windows.Foundation.Boolean TryTransform(Windows.Foundation.Point inPoint, out Windows.Foundation.Point outPoint)
        {
            outPoint = default(Windows.Foundation.Point);
            return default(Windows.Foundation.Boolean);
        }

        public override Windows.Foundation.Rect TransformBounds(Windows.Foundation.Rect rect)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [NativeName("CSkewTransform")]
    [Guids(ClassGuid = "1e6ddde7-b10f-4066-855b-e5e0838ce0c6")]
    public sealed class SkewTransform
     : Microsoft.UI.Xaml.Media.Transform
    {
        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_ptCenter.x")]
        [RenderDirtyFlagClassName("CTransform")]
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
        [OffsetFieldName("m_ptCenter.y")]
        [RenderDirtyFlagClassName("CTransform")]
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
        [OffsetFieldName("m_eAngleX")]
        [RenderDirtyFlagClassName("CTransform")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double AngleX
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object AngleXAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eAngleY")]
        [RenderDirtyFlagClassName("CTransform")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double AngleY
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object AngleYAnimation
        {
            get;
            set;
        }

        public SkewTransform() { }

        public override Microsoft.UI.Xaml.Media.GeneralTransform Inverse
        {
            get { return default(Microsoft.UI.Xaml.Media.GeneralTransform); }

        }

        public override Windows.Foundation.Boolean TryTransform(Windows.Foundation.Point inPoint, out Windows.Foundation.Point outPoint)
        {
            outPoint = default(Windows.Foundation.Point);
            return default(Windows.Foundation.Boolean);
        }

        public override Windows.Foundation.Rect TransformBounds(Windows.Foundation.Rect rect)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [NativeName("CTranslateTransform")]
    [Guids(ClassGuid = "42e7c42a-302e-4d8f-a155-f50c9f588e46")]
    public sealed class TranslateTransform
     : Microsoft.UI.Xaml.Media.Transform
    {
        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eX")]
        [RenderDirtyFlagClassName("CTransform")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double X
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object XAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eY")]
        [RenderDirtyFlagClassName("CTransform")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double Y
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object YAnimation
        {
            get;
            set;
        }

        public TranslateTransform() { }

        public override Microsoft.UI.Xaml.Media.GeneralTransform Inverse
        {
            get { return default(Microsoft.UI.Xaml.Media.GeneralTransform); }

        }

        public override Windows.Foundation.Boolean TryTransform(Windows.Foundation.Point inPoint, out Windows.Foundation.Point outPoint)
        {
            outPoint = default(Windows.Foundation.Point);
            return default(Windows.Foundation.Boolean);
        }

        public override Windows.Foundation.Rect TransformBounds(Windows.Foundation.Rect rect)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CTransformCollection")]
    [Guids(ClassGuid = "f532992c-289a-4704-8194-07d48095de3e")]
    public sealed class TransformCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Transform>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Transform ContentProperty
        {
            get;
            set;
        }

        public TransformCollection() { }
    }

    [NativeName("CTransformGroup")]
    [ContentProperty("Children")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "98c3db0a-f375-42e6-abd8-d2c2d963187c")]
    public sealed class TransformGroup
     : Microsoft.UI.Xaml.Media.Transform
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pChild")]
        [RenderDirtyFlagClassName("CTransformGroup")]
        [RenderDirtyFlagMethodName("NWSetTransformsDirty")]
        public Microsoft.UI.Xaml.Media.TransformCollection Children
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CTransformGroup", "GetTransformValue")]
        [ReadOnly]
        public Microsoft.UI.Xaml.Media.Matrix Value
        {
            get;
            private set;
        }

        public TransformGroup() { }

        public override Microsoft.UI.Xaml.Media.GeneralTransform Inverse
        {
            get { return default(Microsoft.UI.Xaml.Media.GeneralTransform); }

        }

        public override Windows.Foundation.Boolean TryTransform(Windows.Foundation.Point inPoint, out Windows.Foundation.Point outPoint)
        {
            outPoint = default(Windows.Foundation.Point);
            return default(Windows.Foundation.Boolean);
        }

        public override Windows.Foundation.Rect TransformBounds(Windows.Foundation.Rect rect)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [NativeName("CMatrixTransform")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "fe6b2be9-8f0b-4b0a-addc-034be25a14bd")]
    public sealed class MatrixTransform
     : Microsoft.UI.Xaml.Media.Transform
    {

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pMatrix")]
        [RenderDirtyFlagClassName("CTransform")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Matrix Matrix
        {
            get;
            set;
        }

        public MatrixTransform() { }

        public override Microsoft.UI.Xaml.Media.GeneralTransform Inverse
        {
            get { return default(Microsoft.UI.Xaml.Media.GeneralTransform); }

        }

        public override Windows.Foundation.Boolean TryTransform(Windows.Foundation.Point inPoint, out Windows.Foundation.Point outPoint)
        {
            outPoint = default(Windows.Foundation.Point);
            return default(Windows.Foundation.Boolean);
        }

        public override Windows.Foundation.Rect TransformBounds(Windows.Foundation.Rect rect)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [NativeName("CCompositeTransform")]
    [Guids(ClassGuid = "1c588959-eaf0-4409-9b95-9defaad6e7b3")]
    public sealed class CompositeTransform
     : Microsoft.UI.Xaml.Media.Transform
    {
        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_ptCenter.x")]
        [RenderDirtyFlagClassName("CCompositeTransform")]
        [RenderDirtyFlagMethodName("NWSetDirty")]
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
        [OffsetFieldName("m_ptCenter.y")]
        [RenderDirtyFlagClassName("CCompositeTransform")]
        [RenderDirtyFlagMethodName("NWSetDirty")]
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
        [OffsetFieldName("m_eScaleX")]
        [RenderDirtyFlagClassName("CCompositeTransform")]
        [RenderDirtyFlagMethodName("NWSetDirty")]
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
        [OffsetFieldName("m_eScaleY")]
        [RenderDirtyFlagClassName("CCompositeTransform")]
        [RenderDirtyFlagMethodName("NWSetDirty")]
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
        [OffsetFieldName("m_eSkewX")]
        [RenderDirtyFlagClassName("CCompositeTransform")]
        [RenderDirtyFlagMethodName("NWSetDirty")]
        public Windows.Foundation.Double SkewX
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object SkewXAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eSkewY")]
        [RenderDirtyFlagClassName("CCompositeTransform")]
        [RenderDirtyFlagMethodName("NWSetDirty")]
        public Windows.Foundation.Double SkewY
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object SkewYAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eRotation")]
        [RenderDirtyFlagClassName("CCompositeTransform")]
        [RenderDirtyFlagMethodName("NWSetDirty")]
        public Windows.Foundation.Double Rotation
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object RotateAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eTranslateX")]
        [RenderDirtyFlagClassName("CCompositeTransform")]
        [RenderDirtyFlagMethodName("NWSetDirty")]
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
        [OffsetFieldName("m_eTranslateY")]
        [RenderDirtyFlagClassName("CCompositeTransform")]
        [RenderDirtyFlagMethodName("NWSetDirty")]
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

        public CompositeTransform() { }

        public override Microsoft.UI.Xaml.Media.GeneralTransform Inverse
        {
            get { return default(Microsoft.UI.Xaml.Media.GeneralTransform); }

        }

        public override Windows.Foundation.Boolean TryTransform(Windows.Foundation.Point inPoint, out Windows.Foundation.Point outPoint)
        {
            outPoint = default(Windows.Foundation.Point);
            return default(Windows.Foundation.Boolean);
        }

        public override Windows.Foundation.Rect TransformBounds(Windows.Foundation.Rect rect)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [CodeGen(partial: true)]
    [BuiltinStruct("CMatrix")]
    [Guids(ClassGuid = "38a8dad2-1539-4f28-a371-79abfdd1a505")]
    public struct Matrix
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
        [OffsetFieldName("m_matrix._31")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double OffsetX
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_matrix._32")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double OffsetY
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.Media.Matrix Identity
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
        public Windows.Foundation.Point Transform(Windows.Foundation.Point point)
        {
            return default(Windows.Foundation.Point);
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Media.Matrix FromElements(Windows.Foundation.Double m11, Windows.Foundation.Double m12, Windows.Foundation.Double m21, Windows.Foundation.Double m22, Windows.Foundation.Double offsetX, Windows.Foundation.Double offsetY)
        {
            return default(Microsoft.UI.Xaml.Media.Matrix);
        }
    }

    [NativeName("CLineSegment")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "5a127562-bd9f-4c09-9469-5dd3f2f52c6f")]
    public sealed class LineSegment
     : Microsoft.UI.Xaml.Media.PathSegment
    {
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_pt")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point Point
        {
            get;
            set;
        }

        public LineSegment() { }
    }

    [NativeName("CBezierSegment")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "3180d054-7a58-4753-909d-1cecb4bbbf5c")]
    public sealed class BezierSegment
     : Microsoft.UI.Xaml.Media.PathSegment
    {
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptOne")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point Point1
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptTwo")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point Point2
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptThree")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point Point3
        {
            get;
            set;
        }

        public BezierSegment() { }
    }

    [NativeName("CQuadraticSegment")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "a0a8c6e2-9ad7-403a-8294-391e5176649a")]
    public sealed class QuadraticBezierSegment
     : Microsoft.UI.Xaml.Media.PathSegment
    {
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptOne")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point Point1
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptTwo")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point Point2
        {
            get;
            set;
        }

        public QuadraticBezierSegment() { }
    }

    [NativeName("CArcSegment")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "79347acf-f714-4f22-98af-272f6c15e5d8")]
    public sealed class ArcSegment
     : Microsoft.UI.Xaml.Media.PathSegment
    {
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptBase")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point Point
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueSize)]
        [OffsetFieldName("m_size")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Size Size
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eAngle")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double RotationAngle
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bLarge")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Boolean IsLargeArc
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_bClockwise")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.SweepDirection SweepDirection
        {
            get;
            set;
        }

        public ArcSegment() { }
    }

    [NativeName("CPolyLineSegment")]
    [ContentProperty("Points")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "efee1d03-f84f-4066-af03-bfdb415d8af3")]
    public sealed class PolyLineSegment
     : Microsoft.UI.Xaml.Media.PathSegment
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true, IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pPoints")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.PointCollection Points
        {
            get;
            set;
        }

        public PolyLineSegment() { }
    }

    [NativeName("CPolyBezierSegment")]
    [ContentProperty("Points")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "4fd6b5ef-a14d-461c-a849-73dd26b30a4f")]
    public sealed class PolyBezierSegment
     : Microsoft.UI.Xaml.Media.PathSegment
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true, IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pPoints")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.PointCollection Points
        {
            get;
            set;
        }

        public PolyBezierSegment() { }
    }

    [NativeName("CPolyQuadraticSegment")]
    [ContentProperty("Points")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "334f2a46-ef71-4563-8e90-9d0afddb28b3")]
    public sealed class PolyQuadraticBezierSegment
     : Microsoft.UI.Xaml.Media.PathSegment
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true, IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pPoints")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.PointCollection Points
        {
            get;
            set;
        }

        public PolyQuadraticBezierSegment() { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CPathSegmentCollection")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "452e14fe-10a1-40a4-b471-ea0ba769677d")]
    public sealed class PathSegmentCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<PathSegment>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.PathSegment ContentProperty
        {
            get;
            set;
        }

        public PathSegmentCollection() { }
    }

    [NativeName("CPathFigure")]
    [ContentProperty("Segments")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "2c77c8d3-1140-4a8f-95e8-0f668f5c6be8")]
    public sealed class PathFigure
     : Microsoft.UI.Xaml.DependencyObject
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSegments")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.PathSegmentCollection Segments
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptStart")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point StartPoint
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bClosed")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Boolean IsClosed
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bFilled")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Boolean IsFilled
        {
            get;
            set;
        }

        public PathFigure() { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CPathFigureCollection")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "c9608ac9-2ac7-4b23-b50e-0be6fc06f32f")]
    public sealed class PathFigureCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<PathFigure>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.PathFigure ContentProperty
        {
            get;
            set;
        }

        public PathFigureCollection() { }
    }

    [NativeName("CPathGeometry")]
    [ContentProperty("Figures")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "7fc92dfe-3d28-48ff-9bb5-5740e50f914e")]
    public sealed class PathGeometry
     : Microsoft.UI.Xaml.Media.Geometry
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_fillMode")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.FillRule FillRule
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true, IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pFigures")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.PathFigureCollection Figures
        {
            get;
            set;
        }

        public PathGeometry() { }
    }

    [NativeName("CEllipseGeometry")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "f79c4329-b583-4f5c-8b32-62bfa0162e61")]
    public sealed class EllipseGeometry
     : Microsoft.UI.Xaml.Media.Geometry
    {

        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptCenter")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point Center
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eRadiusX")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double RadiusX
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eRadiusY")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double RadiusY
        {
            get;
            set;
        }

        public EllipseGeometry() { }
    }

    [NativeName("CRectangleGeometry")]
    [Guids(ClassGuid = "f663b681-0135-4ed0-b5b7-7875e29009a0")]
    public sealed class RectangleGeometry
     : Microsoft.UI.Xaml.Media.Geometry
    {

        [NativeStorageType(ValueType.valueRect)]
        [OffsetFieldName("m_rc")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Rect Rect
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eRadiusX")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double RadiusX
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eRadiusY")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double RadiusY
        {
            get;
            set;
        }

        public RectangleGeometry() { }
    }

    [NativeName("CLineGeometry")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "57f2b55e-69da-439e-a01f-d0a130acce66")]
    public sealed class LineGeometry
     : Microsoft.UI.Xaml.Media.Geometry
    {

        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptStart")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point StartPoint
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptEnd")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point EndPoint
        {
            get;
            set;
        }

        public LineGeometry() { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CGeometryCollection")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "78d18525-34f4-48ac-8967-a093e791d48f")]
    public sealed class GeometryCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Geometry>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Geometry ContentProperty
        {
            get;
            set;
        }

        public GeometryCollection() { }
    }

    [NativeName("CGeometryGroup")]
    [ContentProperty("Children")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "437cd0b2-bd87-48b7-b0c8-89c55eafd57e")]
    public sealed class GeometryGroup
     : Microsoft.UI.Xaml.Media.Geometry
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_fillMode")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.FillRule FillRule
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pChild")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.GeometryCollection Children
        {
            get;
            set;
        }

        public GeometryGroup() { }
    }

    [NativeName("CSolidColorBrush")]
    [ContentProperty("Color")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "4ad6b000-2d58-4d31-baab-ae1b6970b948")]
    public sealed class SolidColorBrush
     : Microsoft.UI.Xaml.Media.Brush
    {
        [PropertyFlags(IsIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueColor)]
        [OffsetFieldName("m_rgb")]
        [RenderDirtyFlagClassName("CSolidColorBrush")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.UI.Color Color
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object ColorAAnimation
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object ColorRAnimation
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object ColorGAnimation
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object ColorBAnimation
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object ColorAnimation
        {
            get;
            set;
        }

        public SolidColorBrush() { }

        [FactoryMethodName("CreateInstanceWithColor")]
        public SolidColorBrush(Windows.UI.Color color) { }
    }

    [NativeName("CGradientStop")]
    [ContentProperty("Color")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "92412d2f-8d04-4516-9155-b46c92072962")]
    public sealed class GradientStop
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(ValueType.valueColor)]
        [OffsetFieldName("m_rgb")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.UI.Color Color
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_stop.rPosition")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double Offset
        {
            get;
            set;
        }

        public GradientStop() { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CGradientStopCollection")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "9c0dbdd3-45c2-4dd0-9346-ef6ce2e1a327")]
    public sealed class GradientStopCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<GradientStop>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.GradientStop ContentProperty
        {
            get;
            set;
        }

        public GradientStopCollection() { }
    }

    [NativeName("CGradientBrush")]
    [ContentProperty("GradientStops")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "63608038-2b62-4e58-9caf-a8a20f2fab2b")]
    public abstract class GradientBrush
     : Microsoft.UI.Xaml.Media.Brush
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_nSpread")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.GradientSpreadMethod SpreadMethod
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_nMapping")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.BrushMappingMode MappingMode
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_nInterpolate")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.ColorInterpolationMode ColorInterpolationMode
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pStops")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.GradientStopCollection GradientStops
        {
            get;
            set;
        }

        protected GradientBrush() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CLinearGradientBrush")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "194d6a64-ae13-41d4-89d0-64f0af6fb705")]
    [Platform("Feature_XamlMotionSystemHoldbacks", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Velocity(Feature = "Feature_XamlMotionSystemHoldbacks")]
    public sealed class LinearGradientBrush
     : Microsoft.UI.Xaml.Media.GradientBrush
    {
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptStart")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point StartPoint
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptEnd")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point EndPoint
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        public Windows.Foundation.Numerics.Vector2 Translation
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("Translation")]
        public event Microsoft.UI.Xaml.EventHandler TranslationChanged;

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        internal Windows.Foundation.Numerics.Vector2 AnimatedTranslation
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedTranslation")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedTranslationChanged;

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        public Double Rotation
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("Rotation")]
        public event Microsoft.UI.Xaml.EventHandler RotationChanged;

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        internal Double AnimatedRotation
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedRotation")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedRotationChanged;

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "{ 1.0f, 1.0f }")]
        public Windows.Foundation.Numerics.Vector2 Scale
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("Scale")]
        public event Microsoft.UI.Xaml.EventHandler ScaleChanged;

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "{ 1.0f, 1.0f }")]
        internal Windows.Foundation.Numerics.Vector2 AnimatedScale
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedScale")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedScaleChanged;

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        public Windows.Foundation.Numerics.Matrix3x2 TransformMatrix
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("TransformMatrix")]
        public event Microsoft.UI.Xaml.EventHandler TransformMatrixChanged;

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        internal Windows.Foundation.Numerics.Matrix3x2 AnimatedTransformMatrix
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedTransformMatrix")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedTransformMatrixChanged;

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        public Windows.Foundation.Numerics.Vector2 CenterPoint
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("CenterPoint")]
        public event Microsoft.UI.Xaml.EventHandler CenterPointChanged;

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        internal Windows.Foundation.Numerics.Vector2 AnimatedCenterPoint
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedCenterPoint")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedCenterPointChanged;

        public LinearGradientBrush() { }

        [FactoryMethodName("CreateInstanceWithGradientStopCollectionAndAngle")]
        public LinearGradientBrush(Microsoft.UI.Xaml.Media.GradientStopCollection gradientStopCollection, Windows.Foundation.Double angle) { }
    }

    [NativeName("CImageBrush")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "12a869de-963a-49d6-bd1d-30a4e488e8f7")]
    public sealed class ImageBrush
     : Microsoft.UI.Xaml.Media.TileBrush
    {
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pImageSource")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.ImageSource ImageSource
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.ExceptionRoutedEventHandler ImageFailed;

        [NativeStorageType(ValueType.valueObject)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler ImageOpened;

        public ImageBrush() { }
    }

    [AllowsMultipleAssociations]
    [NativeName("CFontFamily")]
    [ClassFlags(HasTypeConverter = true, HasBaseTypeInDXamlInterface = false)]
    [Guids(ClassGuid = "bf8fa44a-c793-4d32-bd97-f1953dbdef1b")]
    public class FontFamily
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DelegateToCore]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.String Source
        {
            get;
            internal set;
        }

        [ReadOnly]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public static Microsoft.UI.Xaml.Media.FontFamily XamlAutoFontFamily
        {
            get;
            private set;
        }
        internal FontFamily() { }

        [FactoryMethodName("CreateInstanceWithName")]
        public FontFamily(Windows.Foundation.String familyName) { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CRenderingEventArgs")]
    [Guids(ClassGuid = "a90b0321-1169-4315-b548-9b8179b3870a")]
    public sealed class RenderingEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pRenderTime")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.TimeSpan RenderingTime
        {
            get;
            private set;
        }

        internal RenderingEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CRenderedEventArgs")]
    [Guids(ClassGuid = "ffd92a5f-a257-4bc3-a44e-bff36dfecc37")]
    public sealed class RenderedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pFrameDuration")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.TimeSpan FrameDuration
        {
            get;
            private set;
        }

        internal RenderedEventArgs() { }
    }

    [AllowsMultipleAssociations]
    [NativeName("CProjection")]
    [Guids(ClassGuid = "f91c8299-4647-4c0c-920a-a45640c37516")]
    public abstract class Projection
     : Microsoft.UI.Xaml.DependencyObject
    {
        protected Projection() { }
    }

    [NativeName("CPlaneProjection")]
    [Guids(ClassGuid = "c4d3813f-0488-42e9-a3c2-5850af688808")]
    public sealed class PlaneProjection
     : Microsoft.UI.Xaml.Media.Projection
    {
        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rLocalOffsetX")]
        [RenderDirtyFlagClassName("CProjection")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double LocalOffsetX
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object LocalOffsetXAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rLocalOffsetY")]
        [RenderDirtyFlagClassName("CProjection")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double LocalOffsetY
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object LocalOffsetYAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rLocalOffsetZ")]
        [RenderDirtyFlagClassName("CProjection")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double LocalOffsetZ
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object LocalOffsetZAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rRotationX")]
        [RenderDirtyFlagClassName("CProjection")]
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
        [RenderDirtyFlagClassName("CProjection")]
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
        [RenderDirtyFlagClassName("CProjection")]
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
        [OffsetFieldName("m_rCenterOfRotationX")]
        [RenderDirtyFlagClassName("CProjection")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double CenterOfRotationX
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object CenterOfRotationXAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rCenterOfRotationY")]
        [RenderDirtyFlagClassName("CProjection")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double CenterOfRotationY
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object CenterOfRotationYAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rCenterOfRotationZ")]
        [RenderDirtyFlagClassName("CProjection")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double CenterOfRotationZ
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object CenterOfRotationZAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rGlobalOffsetX")]
        [RenderDirtyFlagClassName("CProjection")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double GlobalOffsetX
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object GlobalOffsetXAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rGlobalOffsetY")]
        [RenderDirtyFlagClassName("CProjection")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double GlobalOffsetY
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object GlobalOffsetYAnimation
        {
            get;
            set;
        }

        [PropertyFlags(IsConditionallyIndependentlyAnimatable = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_rGlobalOffsetZ")]
        [RenderDirtyFlagClassName("CProjection")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double GlobalOffsetZ
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object GlobalOffsetZAnimation
        {
            get;
            set;
        }

        [NativeMethod("CPlaneProjection", "GetProjectionMatrix")]
        [ReadOnly]
        public Microsoft.UI.Xaml.Media.Media3D.Matrix3D ProjectionMatrix
        {
            get;
            private set;
        }

        public PlaneProjection() { }
    }

    [NativeName("CMatrix3DProjection")]
    [ContentProperty("ProjectionMatrix")]
    [Guids(ClassGuid = "e311620f-674c-4135-9f15-d233a3e12381")]
    public sealed class Matrix3DProjection
     : Microsoft.UI.Xaml.Media.Projection
    {

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pMatrix")]
        [RenderDirtyFlagClassName("CProjection")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Media3D.Matrix3D ProjectionMatrix
        {
            get;
            set;
        }

        public Matrix3DProjection() { }
    }

    [AllowsMultipleAssociations]
    [NativeName("CCacheMode")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "c4f7684c-b814-45db-963f-434158bbc964")]
    public abstract class CacheMode
     : Microsoft.UI.Xaml.DependencyObject
    {
        protected CacheMode() { }
    }

    [NativeName("CBitmapCache")]
    [Guids(ClassGuid = "bf491168-389a-46be-a955-cb2c1519b2d2")]
    public sealed class BitmapCache
     : Microsoft.UI.Xaml.Media.CacheMode
    {
        public BitmapCache() { }
    }

    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum FillRule
    {
        [NativeValueName("XcpFillModeAlternate")]
        EvenOdd = 0,
        [NativeValueName("XcpFillModeWinding")]
        Nonzero = 1,
    }

    [NativeName("SweepDirection")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true, NativeUsesNumericValues = false)]
    public enum SweepDirection
    {
        Counterclockwise,
        Clockwise,
    }

    [NativeName("GradientSpreadMethod")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true, NativeUsesNumericValues = false)]
    public enum GradientSpreadMethod
    {
        Pad,
        Reflect,
        Repeat,
    }

    [NativeName("ColorInterpolationMode")]
    [EnumFlags(HasTypeConverter = true, NativeUsesNumericValues = false)]
    public enum ColorInterpolationMode
    {
        ScRgbLinearInterpolation,
        SRgbLinearInterpolation,
    }

    [NativeName("XcpBrushMappingMode")]
    [EnumFlags(HasTypeConverter = true)]
    [NativeComment("Type of rectangular mapping")]
    public enum BrushMappingMode
    {
        [NativeValueName("XcpBrushMappingModeAbsolute")]
        Absolute = 0,
        [NativeValueName("XcpBrushMappingModeRelative")]
        RelativeToBoundingBox = 1,
    }

    [NativeName("XcpBrushAlignmentX")]
    [EnumFlags(HasTypeConverter = true)]
    [NativeComment("Type of brush alignments in the X coordinate")]
    [DXamlIdlGroup("coretypes2")]
    public enum AlignmentX
    {
        [NativeValueName("XcpBrushAlignmentXLeft")]
        Left = 0,
        [NativeValueName("XcpBrushAlignmentXCenter")]
        Center = 1,
        [NativeValueName("XcpBrushAlignmentXRight")]
        Right = 2,
    }

    [NativeName("XcpBrushAlignmentY")]
    [EnumFlags(HasTypeConverter = true)]
    [NativeComment("Type of brush alignments in the Y coordinate")]
    [DXamlIdlGroup("coretypes2")]
    public enum AlignmentY
    {
        [NativeValueName("XcpBrushAlignmentYTop")]
        Top = 0,
        [NativeValueName("XcpBrushAlignmentYCenter")]
        Center = 1,
        [NativeValueName("XcpBrushAlignmentYBottom")]
        Bottom = 2,
    }

    [NativeName("XcpBrushStretch")]
    [EnumFlags(HasTypeConverter = true)]
    [NativeComment("Type of stretching mechanisms for tile brushes")]
    public enum Stretch
    {
        [NativeComment("Preserve the original size.")]
        [NativeValueName("XcpBrushStretchNone")]
        None = 0,
        [NativeComment("Resizes the content to fill the destination dimensions. Aspect ratio is not preserved.")]
        [NativeValueName("XcpBrushStretchFill")]
        Fill = 1,
        [NativeComment("Resizes content to fit in the destination dimensions, preserving the native aspect ratio.")]
        [NativeValueName("XcpBrushStretchUniform")]
        Uniform = 2,
        [NativeComment("Resizes the content to fill the destination dimensions, preserving the native aspect ratio. If the aspect ratio of the destination rectangle is different from the source, the source content will be clipped to fit in the destination dimensions.")]
        [NativeValueName("XcpBrushStretchUniformToFill")]
        UniformToFill = 3,
    }

    [NativeName("StyleSimulations")]
    [EnumFlags(HasTypeConverter = true)]
    [NativeComment("Contains flags for bold and italic simulation.")]
    public enum StyleSimulations
    {
        [NativeValueName("StyleSimulationNone")]
        None = 0,
        [NativeValueName("StyleSimulationBold")]
        BoldSimulation = 1,
        [NativeValueName("StyleSimulationItalic")]
        ItalicSimulation = 2,
        [NativeValueName("StyleSimulationBoldItalic")]
        BoldItalicSimulation = 3,
    }

    [NativeName("PenLineCap")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum PenLineCap
    {
        [NativeValueName("XcpPenCapFlat")]
        Flat = 0,
        [NativeValueName("XcpPenCapSquare")]
        Square = 1,
        [NativeValueName("XcpPenCapRound")]
        Round = 2,
        [NativeValueName("XcpPenCapTriangle")]
        Triangle = 3,
    }

    [NativeName("PenLineJoin")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum PenLineJoin
    {
        [NativeValueName("XcpLineJoinMiter")]
        Miter = 0,
        [NativeValueName("XcpLineJoinBevel")]
        Bevel = 1,
        [NativeValueName("XcpLineJoinRound")]
        Round = 2,
    }

    [NativeName("TextHintingMode")]
    [EnumFlags(HasTypeConverter = true, NativeUsesNumericValues = false)]
    [NativeComment("TextOptions.TextHintingMode property of UIElements. Note that these are stored as a bit, which should be changed when adding enumerations.")]
    internal enum TextHintingMode
    {
        [NativeValueName("TextHintingMode_Fixed")]
        Fixed,
        [NativeValueName("TextHintingMode_Animated")]
        Animated,
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("TextFormattingMode")]
    [EnumFlags(HasTypeConverter = true, NativeUsesNumericValues = false)]
    [NativeComment("TextOptions.TextFormattingMode property of UIElements")]
    public enum TextFormattingMode
    {
        [NativeValueName("TextFormattingMode_Ideal")]
        Ideal,
        [NativeValueName("TextFormattingMode_Display")]
        Display,
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("TextRenderingMode")]
    [EnumFlags(HasTypeConverter = true, NativeUsesNumericValues = false)]
    [NativeComment("TextOptions.TextRenderingMode property of UIElements")]
    public enum TextRenderingMode
    {
        [NativeValueName("TextRenderingMode_Auto")]
        Auto,
        [NativeValueName("TextRenderingMode_Aliased")]
        Aliased,
        [NativeValueName("TextRenderingMode_Grayscale")]
        Grayscale,
        [NativeValueName("TextRenderingMode_ClearType")]
        ClearType,
    }

    [NativeName("ElementCompositeMode")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("Determines how a UIElement is composited on screen")]
    public enum ElementCompositeMode
    {
        [NativeValueName("ElementCompositeMode_Inherit")]
        Inherit = 0,
        [NativeValueName("ElementCompositeMode_SourceOver")]
        SourceOver = 1,
        [NativeValueName("ElementCompositeMode_MinBlend")]
        MinBlend = 2,
        [CoreEnumValue]
        [NativeValueName("ElementCompositeMode_DestinationInvert")]
        DestInvert = 3,
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 3)]
    public enum FastPlayFallbackBehaviour
    {
        Skip = 0,
        Hide = 1,
        Disable = 2,
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "86b3a048-0b08-43e6-877f-c58a29ba7d30")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 3)]
    public sealed class MediaTransportControlsThumbnailRequestedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetThumbnailImage(Windows.Storage.Streams.IInputStream source)
        {
        }

        public Windows.Foundation.Deferral GetDeferral()
        {
            return null;
        }

        internal MediaTransportControlsThumbnailRequestedEventArgs() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CLoadedImageSurface")]
    [Guids(ClassGuid = "8bea419f-558b-4bb7-8471-9ec1e89a267d")]
    [Implements(typeof(Windows.Foundation.IClosable))]
    [Implements(typeof(Microsoft.UI.Composition.ICompositionSurface))]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    public sealed class LoadedImageSurface
        : Microsoft.UI.Xaml.DependencyObject
    {
        internal LoadedImageSurface() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlOverloadName("StartLoadFromUri")]
        static public LoadedImageSurface StartLoadFromUriWithSize(
            Windows.Foundation.Uri uri,
            Windows.Foundation.Size desiredMaxSize)
        {
            return default(LoadedImageSurface);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlOverloadName("StartLoadFromUri")]
        static public LoadedImageSurface StartLoadFromUri(
            Windows.Foundation.Uri uri)
        {
            return default(LoadedImageSurface);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlOverloadName("StartLoadFromStream")]
        static public LoadedImageSurface StartLoadFromStreamWithSize(
            Windows.Storage.Streams.IRandomAccessStream stream,
            Windows.Foundation.Size desiredMaxSize)
        {
            return default(LoadedImageSurface);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlOverloadName("StartLoadFromStream")]
        static public LoadedImageSurface StartLoadFromStream(
            Windows.Storage.Streams.IRandomAccessStream stream)
        {
            return default(LoadedImageSurface);
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Size DecodedPhysicalSize { get; private set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Size DecodedSize { get; private set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Size NaturalSize { get; private set; }

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<LoadedImageSurface, LoadedImageSourceLoadCompletedEventArgs> LoadCompleted;
    }

    [DXamlIdlGroup("coretypes2")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CLoadedImageSourceLoadCompletedEventArgs")]
    [Guids(ClassGuid = "aa99f55f-dacc-4b3e-83ff-320212048875")]
    public sealed class LoadedImageSourceLoadCompletedEventArgs
        : Microsoft.UI.Xaml.EventArgs
    {
        internal LoadedImageSourceLoadCompletedEventArgs() { }

        [DelegateToCore]
        public LoadedImageSourceLoadStatus Status { get; private set; }
    }

    [DXamlIdlGroup("coretypes2")]
    public enum LoadedImageSourceLoadStatus
    {
        Success = 0,
        NetworkError = 1,
        InvalidFormat = 2,
        Other = 3,
    }

    [AllowsMultipleAssociations]
    [NativeName("CShadow")]
    [Guids(ClassGuid = "c7245d33-11c6-417b-9900-6701e8a61ffb")]
    public abstract class Shadow
        : Microsoft.UI.Xaml.DependencyObject
    {
        internal Shadow() { }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IThemeShadowStaticsPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        bool IsDropShadowMode
        {
            get;
        }
    }

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [NativeName("CThemeShadow")]
    [Guids(ClassGuid = "d9277c9a-1394-4082-9b56-f5a32064c14f")]
    [Implements(typeof(Microsoft.UI.Xaml.Media.IThemeShadowStaticsPrivate), IsStaticInterface = true)]
    public class ThemeShadow
         : Microsoft.UI.Xaml.Media.Shadow
    {
        public ThemeShadow() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromCore = true)]
        internal Microsoft.UI.Composition.CompositionBrush Mask
        {
            get;
            set;
        }

        [PropertyFlags(IsValueCreatedOnDemand = true, IsExcludedFromVisualTree = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.UIElementWeakCollection Receivers
        {
            get;
        }
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 4)]
    [CodeGen(partial: true)]
    [AllowsMultipleAssociations]
    [NativeName("CSystemBackdrop")]
    [Guids(ClassGuid = "4af42535-3f5d-4673-972d-86a2ef34cbef")]
    public abstract class SystemBackdrop
        : Microsoft.UI.Xaml.DependencyObject
    {
        protected SystemBackdrop() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnTargetConnected(Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop connectedTarget, Microsoft.UI.Xaml.XamlRoot xamlRoot)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnTargetDisconnected(Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop disconnectedTarget)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnDefaultSystemBackdropConfigurationChanged(Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop target, Microsoft.UI.Xaml.XamlRoot xamlRoot)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Composition.SystemBackdrops.SystemBackdropConfiguration GetDefaultSystemBackdropConfiguration(Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop target, Microsoft.UI.Xaml.XamlRoot xamlRoot)
        {
            return default(Microsoft.UI.Composition.SystemBackdrops.SystemBackdropConfiguration);
        }
    }
}
