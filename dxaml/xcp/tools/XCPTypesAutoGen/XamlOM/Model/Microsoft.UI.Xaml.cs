// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml
{
    [DXamlIdlGroup("coretypes2")]
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = true)]
    public class TemplateVisualStateAttribute : Attribute
    {
        public string Name { get; set; }
        public string GroupName { get; set; }
    }

    [DXamlIdlGroup("coretypes2")]
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = true)]
    public class StyleTypedPropertyAttribute : Attribute
    {
        public string Property { get; set; }
        public Type StyleTargetType { get; set; }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "78f71c87-1e0f-4cd1-89b2-3ae30a064a26")]
    public static class PointHelper
    {
        public static Windows.Foundation.Point FromCoordinates([CoreType(typeof(Windows.Foundation.Double))] Windows.Foundation.Float x, [CoreType(typeof(Windows.Foundation.Double))] Windows.Foundation.Float y)
        {
            return default(Windows.Foundation.Point);
        }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "8868a334-c3a2-4482-a91c-7ba6625668fb")]
    public static class SizeHelper
    {
        public static Windows.Foundation.Size FromDimensions([CoreType(typeof(Windows.Foundation.Double))] Windows.Foundation.Float width, [CoreType(typeof(Windows.Foundation.Double))] Windows.Foundation.Float height)
        {
            return default(Windows.Foundation.Size);
        }

        [ReadOnly]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public static Windows.Foundation.Size Empty
        {
            get;
            private set;
        }

        [ReturnTypeParameterName("value")]
        public static Windows.Foundation.Boolean GetIsEmpty(Windows.Foundation.Size target)
        {
            return default(Windows.Foundation.Boolean);
        }

        public static Windows.Foundation.Boolean Equals(Windows.Foundation.Size target, Windows.Foundation.Size value)
        {
            return default(Windows.Foundation.Boolean);
        }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IRectHelperStaticsPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Rect DipsRectToPhysicalRect(Windows.Foundation.Rect dipsRect);
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [DXamlIdlGroup("coretypes2")]
    [Implements(typeof(Microsoft.UI.Xaml.IRectHelperStaticsPrivate), IsStaticInterface = true)]
    [Guids(ClassGuid = "ed32af53-3136-4e49-b096-b1eb1ab0989c")]
    public static class RectHelper
    {
        [ReadOnly]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public static Windows.Foundation.Rect Empty
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.Rect FromCoordinatesAndDimensions([CoreType(typeof(Windows.Foundation.Double))] Windows.Foundation.Float x, [CoreType(typeof(Windows.Foundation.Double))] Windows.Foundation.Float y, [CoreType(typeof(Windows.Foundation.Double))] Windows.Foundation.Float width, [CoreType(typeof(Windows.Foundation.Double))] Windows.Foundation.Float height)
        {
            return default(Windows.Foundation.Rect);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.Rect FromPoints(Windows.Foundation.Point point1, Windows.Foundation.Point point2)
        {
            return default(Windows.Foundation.Rect);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.Rect FromLocationAndSize(Windows.Foundation.Point location, Windows.Foundation.Size size)
        {
            return default(Windows.Foundation.Rect);
        }

        [ReturnTypeParameterName("value")]
        public static Windows.Foundation.Boolean GetIsEmpty(Windows.Foundation.Rect target)
        {
            return default(Windows.Foundation.Boolean);
        }

        [ReturnTypeParameterName("value")]
        public static Windows.Foundation.Float GetBottom(Windows.Foundation.Rect target)
        {
            return default(Windows.Foundation.Float);
        }

        [ReturnTypeParameterName("value")]
        public static Windows.Foundation.Float GetLeft(Windows.Foundation.Rect target)
        {
            return default(Windows.Foundation.Float);
        }

        [ReturnTypeParameterName("value")]
        public static Windows.Foundation.Float GetRight(Windows.Foundation.Rect target)
        {
            return default(Windows.Foundation.Float);
        }

        [ReturnTypeParameterName("value")]
        public static Windows.Foundation.Float GetTop(Windows.Foundation.Rect target)
        {
            return default(Windows.Foundation.Float);
        }

        public static Windows.Foundation.Boolean Contains(Windows.Foundation.Rect target, Windows.Foundation.Point point)
        {
            return default(Windows.Foundation.Boolean);
        }

        public static Windows.Foundation.Boolean Equals(Windows.Foundation.Rect target, Windows.Foundation.Rect value)
        {
            return default(Windows.Foundation.Boolean);
        }

        public static Windows.Foundation.Rect Intersect(Windows.Foundation.Rect target, Windows.Foundation.Rect rect)
        {
            return default(Windows.Foundation.Rect);
        }

        [DXamlName("UnionWithPoint")]
        [DXamlOverloadName("Union")]
        [DefaultOverload]
        public static Windows.Foundation.Rect Union(Windows.Foundation.Rect target, Windows.Foundation.Point point)
        {
            return default(Windows.Foundation.Rect);
        }

        [DXamlName("UnionWithRect")]
        [DXamlOverloadName("Union")]
        public static Windows.Foundation.Rect Union(Windows.Foundation.Rect target, Windows.Foundation.Rect rect)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [NativeName("CEventArgs")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false, IsTypeConverter = true, IsHiddenFromIdl = true)]
    [IdlType(typeof(object))]
    [OldCodeGenBaseType(typeof(DependencyObject))]
    [HandWritten]
    [Guids(ClassGuid = "2ff30017-c6eb-448e-9b80-c9042d33ea5b")]
    public class EventArgs
    {
        public EventArgs() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "646d77ad-48ff-4399-ae68-895933e8c6ff")]
    internal class VectorChangedEventArgs
    {
    }

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void EventHandler(Windows.Foundation.Object sender, Windows.Foundation.Object e);

    [DXamlIdlGroup("coretypes2")]
    public delegate void BindingFailedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.BindingFailedEventArgs e);

    [CodeGen(partial: true)]
    [ClassFlags(HasTypeConverter = true)]
    [NativeName("CPropertyPath")]
    [Guids(ClassGuid = "071dda43-6ef0-4621-936c-4a541e1c681f")]
    public sealed class PropertyPath
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        [OffsetFieldName("m_strPath")]
        public Windows.Foundation.String Path
        {
            get;
            private set;
        }

        internal PropertyPath() { }

        public PropertyPath([Optional] Windows.Foundation.String path) { }
    }

    [ClassFlags(HasTypeConverter = true, HasBaseTypeInDXamlInterface = false)]
    [NativeName("CTargetPropertyPath")]
    [Guids(ClassGuid = "1622e8d5-4563-478b-b7b0-0461ef918b96")]
    public sealed class TargetPropertyPath
        : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CTargetPropertyPath", "Path")]
        public Microsoft.UI.Xaml.PropertyPath Path
        {
            get;
            set;
        }

        [CoreType(typeof(Microsoft.UI.Xaml.DependencyObject))]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CTargetPropertyPath", "Target")]
        public Windows.Foundation.Object Target
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [DependencyPropertyModifier(Modifier.Private)]
        internal Microsoft.UI.Xaml.Internal.DependencyPropertyProxy CachedStyleSetterProperty
        {
            get;
            set;
        }

        public TargetPropertyPath() { }

        public TargetPropertyPath(Microsoft.UI.Xaml.Internal.DependencyPropertyProxy targetProperty) { }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface ISourceInfoPrivate
    {
        [DependencyPropertyModifier(Modifier.Private)]
        Windows.Foundation.Int32 Line
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        Windows.Foundation.Int32 Column
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        Windows.Foundation.String ParseUri
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        Windows.Foundation.String XbfHash
        {
            get;
            set;
        }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IMemoryInfoPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.UInt64 GetCountOfDescendantUIElements();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.UInt64 GetEstimatedSizeOfDescendantImages();
    }

    [CodeGen(CodeGenLevel.Idl)]
    [NativeName("CDependencyObject")]
    [ClassFlags(IsTypeConverter = true)]
    [Implements(typeof(Microsoft.UI.Xaml.ISourceInfoPrivate))]
    [Implements(typeof(Microsoft.UI.Xaml.IMemoryInfoPrivate))]
    [HandWritten]
    public abstract class DependencyObject
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyFlags(IsReadOnlyExceptForParser = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strName")]
        internal Windows.Foundation.String Name
        {
            get;
            set;
        }

        internal event Microsoft.UI.Xaml.EventHandler Collect;

        [EventFlags(UseEventManager = true)]
        internal event Microsoft.UI.Xaml.EventHandler RaiseAsyncCallback;

        protected DependencyObject() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Object GetValue(DependencyProperty dp)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetValue(DependencyProperty dp, Windows.Foundation.Object value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void ClearValue(DependencyProperty dp)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Object ReadLocalValue(DependencyProperty dp)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Object GetAnimationBaseValue(DependencyProperty dp)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [OrderHint(1)]
        [ReadOnly]
        [TypeTable(IsExcludedFromCore = true, IsExcludedFromNewTypeTable = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.UI.Core.CoreDispatcher Dispatcher
        {
            get
            {
                return default(Windows.UI.Core.CoreDispatcher);
            }
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [OrderHint(1)]
        [ReadOnly]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Dispatching.DispatcherQueue DispatcherQueue
        {
            get
            {
                return default(Microsoft.UI.Dispatching.DispatcherQueue);
            }
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Int64 RegisterPropertyChangedCallback(DependencyProperty dp, DependencyPropertyChangedCallback callback)
        {
            return default(Windows.Foundation.Int64);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void UnregisterPropertyChangedCallback(DependencyProperty dp, Windows.Foundation.Int64 token)
        {
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object DeferredStorage
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object RealizingProxy
        {
            get;
            set;
        }
    }

    [BuiltinStruct("CThickness")]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "f6913a37-ff5e-4478-95d0-27ee3b382eb1")]
    public struct Thickness
    {
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_thickness.left")]
        public Windows.Foundation.Double Left
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_thickness.top")]
        public Windows.Foundation.Double Top
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_thickness.right")]
        public Windows.Foundation.Double Right
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_thickness.bottom")]
        public Windows.Foundation.Double Bottom
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Thickness FromLengths(Windows.Foundation.Double left, Windows.Foundation.Double top, Windows.Foundation.Double right, Windows.Foundation.Double bottom)
        {
            return default(Microsoft.UI.Xaml.Thickness);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Thickness FromUniformLength(Windows.Foundation.Double uniformLength)
        {
            return default(Microsoft.UI.Xaml.Thickness);
        }
    }

    [BuiltinStruct("CCornerRadius")]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "00911a21-81a2-468f-8bad-4658560997b9")]
    public struct CornerRadius
    {
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_cornerRadius.topLeft")]
        public Windows.Foundation.Double TopLeft
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_cornerRadius.topRight")]
        public Windows.Foundation.Double TopRight
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_cornerRadius.bottomRight")]
        public Windows.Foundation.Double BottomRight
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_cornerRadius.bottomLeft")]
        public Windows.Foundation.Double BottomLeft
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.CornerRadius FromRadii(Windows.Foundation.Double topLeft, Windows.Foundation.Double topRight, Windows.Foundation.Double bottomRight, Windows.Foundation.Double bottomLeft)
        {
            return default(Microsoft.UI.Xaml.CornerRadius);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.CornerRadius FromUniformRadius(Windows.Foundation.Double uniformRadius)
        {
            return default(Microsoft.UI.Xaml.CornerRadius);
        }
    }

    [CodeGen(partial: true)]
    [BuiltinStruct("CDuration")]
    [CoreBaseType(typeof(Windows.Foundation.TimeSpan))]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "f41528e9-9168-4b67-a345-d0599d570d35")]
    public struct Duration
    {
        [TypeTable(IsExcludedFromCore = true)]

        [NativeStorageType(OM.ValueType.valueDouble)]
        public Windows.Foundation.TimeSpan TimeSpan
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true, IsExcludedFromNewTypeTable = true)]
        public Microsoft.UI.Xaml.DurationType Type
        {
            get;
            set;
        }

        [ReadOnly]
        public static Microsoft.UI.Xaml.Duration Automatic
        {
            get;
            private set;
        }

        [ReadOnly]
        public static Microsoft.UI.Xaml.Duration Forever
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyFlags(IsHelper = true)]
        [ReadOnly]
        public Windows.Foundation.Boolean HasTimeSpan
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Duration Add(Microsoft.UI.Xaml.Duration duration)
        {
            return default(Microsoft.UI.Xaml.Duration);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Windows.Foundation.Int32 Compare(Microsoft.UI.Xaml.Duration duration1, Microsoft.UI.Xaml.Duration duration2)
        {
            return default(Windows.Foundation.Int32);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Boolean Equals(Microsoft.UI.Xaml.Duration value)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Duration Subtract(Microsoft.UI.Xaml.Duration duration)
        {
            return default(Microsoft.UI.Xaml.Duration);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Duration FromTimeSpan(Windows.Foundation.TimeSpan timeSpan)
        {
            return default(Microsoft.UI.Xaml.Duration);
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CUIElement")]
    [Platform("Feature_Xaml2018", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "c47ebf2e-f068-42c2-996a-be695fa71839")]
    [Velocity(Feature = "Feature_Xaml2018")]
    [Implements(typeof(Microsoft.UI.Composition.IAnimationObject), Version = 1)]
    [Implements(typeof(Microsoft.UI.Composition.IVisualElement), Version = 1)]
    [Implements(typeof(Microsoft.UI.Composition.IVisualElement2), Version = 1)]
    public abstract partial class UIElement
     : Microsoft.UI.Xaml.DependencyObject
    {
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CUIElement", "GetChildrenInternal")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetSubgraphDirty")]
        internal Microsoft.UI.Xaml.Controls.UIElementCollection ChildrenInternal
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal object DirectManipulationContainer
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal object CanvasOffset
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal object OpacityExpression
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Size DesiredSize
        {
            get;
            private set;
        }

        [PropertyFlags(IsValueInherited = true)]
        [NativeMethod("CUIElement", "AllowDrop")]
        [NativeStorageType(OM.ValueType.valueBool)]
        public Windows.Foundation.Boolean AllowDrop
        {
            get;
            set;
        }

        [PropertyFlags(IsIndependentlyAnimatable = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_eOpacityPrivate")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetOpacityDirty")]
        public Windows.Foundation.Double Opacity
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [CoreType(typeof(Microsoft.UI.Xaml.Media.Geometry))]
        [PropertyFlags(IsIndependentlyAnimatable = true, HadFieldInBlue = true)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetClipDirty")]
        public Microsoft.UI.Xaml.Media.RectangleGeometry Clip
        {
            get;
            set;
        }

        [Strictness(Strictness.NonStrictOnly)]
        [PropertyFlags(IsIndependentlyAnimatable = true, HadFieldInBlue = true)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetTransformDirty")]
        public Microsoft.UI.Xaml.Media.Transform RenderTransform
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Microsoft.UI.Xaml.Media.Transform HandOffVisualTransform
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Microsoft.UI.Xaml.Media.Media3D.Matrix3D HandOffVisualTransformMatrix3D
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Microsoft.UI.Xaml.Media.RectangleGeometry HandOffVisualClip
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object OffsetXAnimation
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object OffsetYAnimation
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Object OpacityAnimation
        {
            get;
            set;
        }

        [Strictness(Strictness.NonStrictOnly)]
        [PropertyFlags(IsIndependentlyAnimatable = true, HadFieldInBlue = true)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetProjectionDirty")]
        public Microsoft.UI.Xaml.Media.Projection Projection
        {
            get;
            set;
        }

        [Strictness(Strictness.NonStrictOnly)]
        [PropertyFlags(IsIndependentlyAnimatable = true)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetTransform3DDirty")]
        public Microsoft.UI.Xaml.Media.Media3D.Transform3D Transform3D
        {
            get;
            set;
        }

        [Strictness(Strictness.NonStrictOnly)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetTransformDirty")]
        public Windows.Foundation.Point RenderTransformOrigin
        {
            get;
            set;
        }

        [NativeMethod("CUIElement", "HitTestVisible")]
        [NativeStorageType(OM.ValueType.valueBool)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Windows.Foundation.Boolean IsHitTestVisible
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeMethod("CUIElement", "VisibilityState")]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetVisibilityDirty")]
        public Microsoft.UI.Xaml.Visibility Visibility
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CUIElement", "GetRenderSize")]
        [NativeStorageType(OM.ValueType.valueSize)]
        [ReadOnly]
        public Windows.Foundation.Size RenderSize
        {
            get;
            private set;
        }

        [PropertyFlags(AffectsArrange = true, AffectsMeasure = true)]
        [NativeMethod("CUIElement", "UseLayoutRounding")]
        [NativeStorageType(OM.ValueType.valueBool)]
        public Windows.Foundation.Boolean UseLayoutRounding
        {
            get;
            set;
        }

        [PropertyFlags(HadFieldInBlue = true, IsValueCreatedOnDemand = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection Transitions
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true, AffectsMeasure = true, HadFieldInBlue = true)]
        public Microsoft.UI.Xaml.Media.CacheMode CacheMode
        {
            get;
            set;
        }

        [NativeMethod("CUIElement", "IsTapEnabled")]
        [NativeStorageType(OM.ValueType.valueBool)]
        public Windows.Foundation.Boolean IsTapEnabled
        {
            get;
            set;
        }

        [NativeMethod("CUIElement", "IsDoubleTapEnabled")]
        [NativeStorageType(OM.ValueType.valueBool)]
        public Windows.Foundation.Boolean IsDoubleTapEnabled
        {
            get;
            set;
        }

        [NativeMethod("CUIElement", "CanDrag")]
        [NativeStorageType(OM.ValueType.valueBool)]
        public Windows.Foundation.Boolean CanDrag
        {
            get;
            set;
        }

        [PropertyFlags(HadFieldInBlue = true)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetTransitionTargetDirty")]
        internal Microsoft.UI.Xaml.Media.Animation.TransitionTarget TransitionTarget
        {
            get;
            set;
        }

        [NativeMethod("CUIElement", "IsRightTapEnabled")]
        [NativeStorageType(OM.ValueType.valueBool)]
        public Windows.Foundation.Boolean IsRightTapEnabled
        {
            get;
            set;
        }

        [NativeMethod("CUIElement", "IsHoldEnabled")]
        [NativeStorageType(OM.ValueType.valueBool)]
        public Windows.Foundation.Boolean IsHoldingEnabled
        {
            get;
            set;
        }

        [NativeMethod("CUIElement", "ManipulationMode")]
        public Microsoft.UI.Xaml.Input.ManipulationModes ManipulationMode
        {
            get;
            set;
        }

        [CollectionType(CollectionKind.Indexable)]
        [PropertyFlags(HadFieldInBlue = true)]
        public Microsoft.UI.Xaml.Input.PointerCollection PointerCaptures
        {
            get;
            private set;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase ContextFlyout
        {
            get;
            set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent KeyDownEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent KeyUpEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent PointerEnteredEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent PointerPressedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent PointerMovedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent PointerReleasedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent PointerExitedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent PointerCaptureLostEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent PointerCanceledEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent PointerWheelChangedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent TappedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent DoubleTappedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent HoldingEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent RightTappedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        internal static Microsoft.UI.Xaml.RoutedEvent RightTappedUnhandledEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent ManipulationStartingEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent ManipulationInertiaStartingEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent ManipulationStartedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent ManipulationDeltaEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent ManipulationCompletedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent DragEnterEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent DragLeaveEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent DragOverEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent DropEvent
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetCompositeModeDirty")]
        public Microsoft.UI.Xaml.Media.ElementCompositeMode CompositeMode
        {
            get;
            set;
        }

        [NativeMethod("CUIElement", "GlobalScaleFactor")]
        internal Windows.Foundation.Float GlobalScaleFactor
        {
            get;
            set;
        }

        [CoreType(typeof(Microsoft.UI.Xaml.Media.XamlLightCollection))]
        [PropertyFlags(IsValueCreatedOnDemand = true, IsIndependentlyAnimatable = false)]
        [CollectionType(CollectionKind.Vector)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetLightCollectionDirty")]
        public Windows.Foundation.Collections.IVector<Microsoft.UI.Xaml.Media.XamlLight> Lights
        {
            get;
        }

        [Strictness(Strictness.Agnostic)]
        [VelocityFeature("Feature_Xaml2018")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Collections.IVector<Microsoft.UI.Xaml.InteractionBase> Interactions
        {
            get;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        protected Microsoft.UI.Input.InputCursor ProtectedCursor
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean CanBeScrollAnchor
        {
            get;
            set;
        }

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.KeyEventHandler KeyUp;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.KeyEventHandler KeyDown;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler GotFocus;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler LostFocus;

        [EventFlags(IsImplVirtual = true)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [TypeTable(IsExcludedFromCore = true)]
        public event Microsoft.UI.Xaml.DragStartingEventHandler DragStarting;

        [EventFlags(IsImplVirtual = true)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [TypeTable(IsExcludedFromCore = true)]
        public event Microsoft.UI.Xaml.DropCompletedEventHandler DropCompleted;

        [EventFlags(IsControlEvent = true)]
        public event Windows.Foundation.TypedEventHandler<UIElement, Microsoft.UI.Xaml.Input.CharacterReceivedRoutedEventArgs> CharacterReceived;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.DragEventHandler DragEnter;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.DragEventHandler DragLeave;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.DragEventHandler DragOver;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.DragEventHandler Drop;

        [EventFlags(IsHidden = true)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler TextComposition;

        [EventFlags(IsHidden = true)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler InputMethodNotify;

        [EventFlags(IsHidden = true)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler InputLangChange;

        [EventFlags(IsHidden = true)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler TabProcessing;

        [EventFlags(IsHidden = true)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler ManipulationInertiaProcessing;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.PointerEventHandler PointerPressed;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.PointerEventHandler PointerMoved;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.PointerEventHandler PointerReleased;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.PointerEventHandler PointerEntered;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.PointerEventHandler PointerExited;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.PointerEventHandler PointerCaptureLost;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.PointerEventHandler PointerCanceled;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.PointerEventHandler PointerWheelChanged;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.TappedEventHandler Tapped;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.DoubleTappedEventHandler DoubleTapped;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.HoldingEventHandler Holding;

        [EventFlags(IsControlEvent = true, IsImplVirtual = true)]
        public event Windows.Foundation.TypedEventHandler<UIElement, Microsoft.UI.Xaml.Input.ContextRequestedEventArgs> ContextRequested;

        [EventFlags(IsControlEvent = true, IsImplVirtual = true)]
        public event Windows.Foundation.TypedEventHandler<UIElement, RoutedEventArgs> ContextCanceled;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.RightTappedEventHandler RightTapped;

        [EventFlags(IsControlEvent = true, IsHidden = true)]
        internal event Microsoft.UI.Xaml.Input.RightTappedUnhandledEventHandler RightTappedUnhandled;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.ManipulationStartingEventHandler ManipulationStarting;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.ManipulationInertiaStartingEventHandler ManipulationInertiaStarting;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.ManipulationStartedEventHandler ManipulationStarted;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.ManipulationDeltaEventHandler ManipulationDelta;

        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.Input.ManipulationCompletedEventHandler ManipulationCompleted;

        internal UIElement() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void Measure(Windows.Foundation.Size availableSize)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void Arrange(Windows.Foundation.Rect finalRect)
        {
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeClassName("CUIElement")]
        public Windows.Foundation.Boolean CaptureMouse()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeClassName("CUIElement")]
        public void ReleaseMouseCapture()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean CapturePointer(Microsoft.UI.Xaml.Input.Pointer value)
        {
            return default(Windows.Foundation.Boolean);
        }

        [PInvoke]
        public void ReleasePointerCapture(Microsoft.UI.Xaml.Input.Pointer value)
        {
        }

        [PInvoke]
        public void ReleasePointerCaptures()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void AddHandler(Microsoft.UI.Xaml.RoutedEvent routedEvent, [CoreType(typeof(Microsoft.UI.Xaml.DependencyObject))] Windows.Foundation.Object handler, Windows.Foundation.Boolean handledEventsToo)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void RemoveHandler(Microsoft.UI.Xaml.RoutedEvent routedEvent, [CoreType(typeof(Microsoft.UI.Xaml.DependencyObject))] Windows.Foundation.Object handler)
        {
        }

        public Microsoft.UI.Xaml.Media.GeneralTransform TransformToVisual([Optional] Microsoft.UI.Xaml.UIElement visual)
        {
            return default(Microsoft.UI.Xaml.Media.GeneralTransform);
        }

        [CodeGen(CodeGenLevel.IdlAndStub)]
        public void InvalidateMeasure()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndStub)]
        public void InvalidateArrange()
        {
        }

        [PInvoke]
        public void UpdateLayout()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual Microsoft.UI.Xaml.Automation.Peers.AutomationPeer OnCreateAutomationPeer()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnDisconnectVisualChildren()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual Windows.Foundation.Collections.IIterable<Windows.Foundation.Collections.IIterable<Windows.Foundation.Point>> FindSubElementsForTouchTargeting(Windows.Foundation.Point point, Windows.Foundation.Rect boundingRect)
        {
            return default(Windows.Foundation.Collections.IIterable<Windows.Foundation.Collections.IIterable<Windows.Foundation.Point>>);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual Windows.Foundation.Collections.IIterable<Microsoft.UI.Xaml.DependencyObject> GetChildrenInTabFocusOrder()
        {
            return default(Windows.Foundation.Collections.IIterable<Microsoft.UI.Xaml.DependencyObject>);
        }

        [PInvoke]
        internal Windows.Foundation.Float LayoutRound(Windows.Foundation.Float value)
        {
            return default(Windows.Foundation.Float);
        }

        [PInvoke]
        internal void ResetGlobalScaleFactor()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean CancelDirectManipulations()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.IAsyncOperation<Windows.ApplicationModel.DataTransfer.DataPackageOperation> StartDragAsync(
            Microsoft.UI.Input.PointerPoint pointerPoint)
        {
            return default(Windows.Foundation.IAsyncOperation<Windows.ApplicationModel.DataTransfer.DataPackageOperation>);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.Boolean TryStartDirectManipulation(Microsoft.UI.Xaml.Input.Pointer value)
        {
            return default(Windows.Foundation.Boolean);
        }
    }

    [Guids(ClassGuid = "15e646dd-1e0c-4484-9804-71f998a4a712")]
    [CodeGen(partial: true)]
    public sealed class BringIntoViewOptions
    {
        public Windows.Foundation.Boolean AnimationDesired { get; set; }

        public Windows.Foundation.Rect? TargetRect { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public double HorizontalAlignmentRatio { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public double VerticalAlignmentRatio { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public double HorizontalOffset { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public double VerticalOffset { get; set; }
    }

    [NativeName("CBringIntoViewRequestedEventArgs")]
    [Guids(ClassGuid = "22c0ecc7-88c0-4c31-808d-7744d604b06a")]
    public sealed class BringIntoViewRequestedEventArgs : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [DelegateToCore]
        public Microsoft.UI.Xaml.UIElement TargetElement { get; set; }

        [DelegateToCore]
        public Windows.Foundation.Boolean AnimationDesired { get; set; }

        [DelegateToCore]
        public Windows.Foundation.Rect TargetRect { get; set; }

        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Double HorizontalAlignmentRatio
        {
            get;
            private set;
        }

        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Double VerticalAlignmentRatio
        {
            get;
            private set;
        }

        [DelegateToCore]
        public Windows.Foundation.Double HorizontalOffset
        {
            get;
            set;
        }

        [DelegateToCore]
        public Windows.Foundation.Double VerticalOffset
        {
            get;
            set;
        }

        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        internal BringIntoViewRequestedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CFrameworkElement")]
    [Guids(ClassGuid = "ca0bea2c-b556-4b05-8650-b9d1af48b953")]
    public abstract partial class FrameworkElement
     : Microsoft.UI.Xaml.UIElement
    {
        [CodeGen(CodeGenLevel.Idl)]
        [DependencyPropertyModifier(Modifier.Private)]
        internal int AutomationPeerFactoryIndex
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true, HadFieldInBlue = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.TriggerCollection Triggers
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [PropertyFlags(IsValueCreatedOnDemand = true, HadFieldInBlue = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Xaml.ResourceDictionary Resources
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Windows.Foundation.Object Tag
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_strLanguageString")]
        public Windows.Foundation.String Language
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_isTextScaleFactorEnabled")]
        internal Windows.Foundation.Boolean IsTextScaleFactorEnabledInternal
        {
            get;
            set;
        }

        [NativeMethod("CFrameworkElement", "ActualWidth")]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [ReadOnly]
        public Windows.Foundation.Double ActualWidth
        {
            get;
            private set;
        }

        [NativeMethod("CFrameworkElement", "ActualHeight")]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [ReadOnly]
        public Windows.Foundation.Double ActualHeight
        {
            get;
            private set;
        }

        [PropertyFlags(AffectsMeasure = true, NeedsInvoke = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_eWidth")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double Width
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, NeedsInvoke = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_eHeight")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double Height
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_pLayoutProperties")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_eMinWidth")]
        public Windows.Foundation.Double MinWidth
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_pLayoutProperties")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_eMaxWidth")]
        public Windows.Foundation.Double MaxWidth
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_pLayoutProperties")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_eMinHeight")]
        public Windows.Foundation.Double MinHeight
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_pLayoutProperties")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_eMaxHeight")]
        public Windows.Foundation.Double MaxHeight
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true, IsInStorageGroup = true)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_pLayoutProperties")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_horizontalAlignment")]
        public Microsoft.UI.Xaml.HorizontalAlignment HorizontalAlignment
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true, IsInStorageGroup = true)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_pLayoutProperties")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_verticalAlignment")]
        public Microsoft.UI.Xaml.VerticalAlignment VerticalAlignment
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true)]
        [NativeStorageType(OM.ValueType.valueThickness)]
        [OffsetFieldName("m_pLayoutProperties")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_margin")]
        public Microsoft.UI.Xaml.Thickness Margin
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [UnderlyingDependencyProperty(typeof(DependencyObject), "Name")]
        public new Windows.Foundation.String Name
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]

        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Uri BaseUri
        {
            get;
            private set;
        }

        [PropertyFlags(IsValueInherited = true, IsExcludedFromVisualTree = true)]
        public Windows.Foundation.Object DataContext
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Windows.Foundation.Boolean AllowFocusOnInteraction
        {
            get;
            set;
        }

        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(OM.ValueType.valueThickness)]
        public Microsoft.UI.Xaml.Thickness FocusVisualMargin
        {
            get;
            set;
        }

        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(OM.ValueType.valueThickness)]
        public Microsoft.UI.Xaml.Thickness FocusVisualSecondaryThickness
        {
            get;
            set;
        }

        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(OM.ValueType.valueThickness)]
        public Microsoft.UI.Xaml.Thickness FocusVisualPrimaryThickness
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush FocusVisualSecondaryBrush
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush FocusVisualPrimaryBrush
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Windows.Foundation.Boolean AllowFocusWhenDisabled
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueObject)]
        [PropertyFlags(HadFieldInBlue = true)]
        public Microsoft.UI.Xaml.Style Style
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueAny)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CFrameworkElement", "Parent")]
        [ReadOnly]
        public Microsoft.UI.Xaml.DependencyObject Parent
        {
            get;
            private set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFlowDirection")]
        public Microsoft.UI.Xaml.FlowDirection FlowDirection
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueEnum)]
        public Microsoft.UI.Xaml.ElementTheme RequestedTheme
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Rect EffectiveViewport
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Rect MaxViewport
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Double BringIntoViewDistanceX
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Double BringIntoViewDistanceY
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [ReadOnly]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsLoaded
        {
            get;
        }

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler Loaded;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler Unloaded;

        [TypeTable(IsExcludedFromCore = true)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.DataContextChangedEventHandler DataContextChanged;

        [TypeTable(IsExcludedFromCore = true)]
        public event Microsoft.UI.Xaml.SizeChangedEventHandler SizeChanged;

        [TypeTable(IsExcludedFromCore = true)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler LayoutUpdated;

        protected FrameworkElement() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual Windows.Foundation.Size MeasureOverride(Windows.Foundation.Size availableSize)
        {
            return default(Windows.Foundation.Size);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual Windows.Foundation.Size ArrangeOverride(Windows.Foundation.Size finalSize)
        {
            return default(Windows.Foundation.Size);
        }

        [PInvoke]
        internal Windows.Foundation.Boolean InvokeApplyTemplate()
        {
            return default(Windows.Foundation.Boolean);
        }

        [PInvoke]
        internal Windows.Foundation.Boolean InvokeFocus(Microsoft.UI.Xaml.FocusState value)
        {
            return default(Windows.Foundation.Boolean);
        }

        [NativeClassName("CFrameworkElement")]
        [PInvoke]
        protected virtual void OnApplyTemplate()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeClassName("CFrameworkElement")]
        public Windows.Foundation.Object FindName(Windows.Foundation.String name)
        {
            return default(Windows.Foundation.Object);
        }

        // DEPRECATED: remove once XamlCompiler no longer emits code depending on it
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeClassName("CFrameworkElement")]
        static public void DeferTree(Microsoft.UI.Xaml.DependencyObject element)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeClassName("CFrameworkElement")]
        public void SetBinding(Microsoft.UI.Xaml.DependencyProperty dp, Microsoft.UI.Xaml.Data.BindingBase binding)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual Windows.Foundation.Boolean GoToElementStateCore(Windows.Foundation.String stateName, Windows.Foundation.Boolean useTransitions)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Data.BindingExpression GetBindingExpression(Microsoft.UI.Xaml.DependencyProperty dp)
        {
            return default(Microsoft.UI.Xaml.Data.BindingExpression);
        }

        [EventFlags(UseEventManager = true)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.EventHandler Loading;
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [ClassFlags(IsVisibleInXAML = false, RequiresCoreServices = false)]
    [NativeName("CEffectiveViewportChangedEventArgs")]
    [Guids(ClassGuid = "b6e8e616-e705-4e35-9801-e9817b247fd1")]
    public sealed class EffectiveViewportChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [DelegateToCore]
        public Windows.Foundation.Rect EffectiveViewport
        {
            get;
        }

        [DelegateToCore]
        public Windows.Foundation.Rect MaxViewport
        {
            get;
        }

        [DelegateToCore]
        public Windows.Foundation.Double BringIntoViewDistanceX
        {
            get;
        }

        [DelegateToCore]
        public Windows.Foundation.Double BringIntoViewDistanceY
        {
            get;
        }

        internal EffectiveViewportChangedEventArgs() { }
    }

    [AllowsMultipleAssociations]
    [CodeGen(partial: true)]
    [NativeName("CStaggerFunctionBase")]
    [DXamlComposability(Modifier.Internal)]
    [Guids(ClassGuid = "30470b6b-646c-408b-9c73-230fabb14272")]
    internal abstract class StaggerFunctionBase
     : Microsoft.UI.Xaml.DependencyObject
    {
        internal StaggerFunctionBase() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "43f8ee08-3932-4ee4-b391-398eebd5eb89")]
    internal sealed class PVLStaggerFunction
     : Microsoft.UI.Xaml.StaggerFunctionBase
    {
        [FieldBacked]
        public Windows.Foundation.Double Delay
        {
            get;
            set;
        }

        [FieldBacked]
        public Windows.Foundation.Double Maximum
        {
            get;
            set;
        }

        [FieldBacked]
        public Windows.Foundation.Double DelayReduce
        {
            get;
            set;
        }

        [FieldBacked]
        public Windows.Foundation.Boolean Reverse
        {
            get;
            set;
        }

        internal PVLStaggerFunction() { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTriggerCollection")]
    [Guids(ClassGuid = "920efa8e-1030-44ec-a287-424f713c100b")]
    public sealed class TriggerCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<TriggerBase>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Xaml.TriggerBase ContentProperty
        {
            get;
            set;
        }

        internal TriggerCollection() { }
    }

    [NativeName("CTriggerBase")]
    [Guids(ClassGuid = "7da86fc4-57a3-49ed-8f27-e39315c6cbbd")]
    public abstract class TriggerBase
     : Microsoft.UI.Xaml.DependencyObject
    {
        internal TriggerBase() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CEventTrigger")]
    [ContentProperty("Actions")]
    [Guids(ClassGuid = "5d36bb9d-a19a-4a41-8b7c-2486095bbb66")]
    public sealed class EventTrigger
     : Microsoft.UI.Xaml.TriggerBase
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [CoreType(typeof(string))]
        [DependencyPropertyModifier(Modifier.Private)]
        [OffsetFieldName("m_strRouted")]
        public Microsoft.UI.Xaml.RoutedEvent RoutedEvent
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pTriggerActions")]
        public Microsoft.UI.Xaml.TriggerActionCollection Actions
        {
            get;
            set;
        }

        public EventTrigger() { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CTriggerActionCollection")]
    [Guids(ClassGuid = "3144770e-fd38-48a1-8f6b-a2dca36c7009")]
    public sealed class TriggerActionCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<TriggerAction>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Xaml.TriggerAction ContentProperty
        {
            get;
            set;
        }

        public TriggerActionCollection() { }
    }

    [NativeName("CTriggerAction")]
    [Guids(ClassGuid = "52085c66-b7e8-4028-a453-ff4051fd13db")]
    public abstract class TriggerAction
     : Microsoft.UI.Xaml.DependencyObject
    {
        internal TriggerAction() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromNewTypeTable = true)]
    [Guids(ClassGuid = "90e72981-7783-4e2a-861b-55660ef58d33")]
    public sealed class DependencyPropertyChangedEventArgs
    {
        internal DependencyPropertyChangedEventArgs()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.DependencyProperty Property
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object OldValue
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object NewValue
        {
            get;
            internal set;
        }
    }

    [FrameworkTypePattern]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true)]
    [Guids(ClassGuid = "bbbaab17-9f2e-4f8f-843a-58cdad3243b3")]
    public sealed class DataContextChangedEventArgs
    {
        internal DataContextChangedEventArgs()
        {
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object NewValue
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "e447a787-543a-435d-9b6b-c2ea37275011")]
    public sealed class DragOperationDeferral
    {
        internal DragOperationDeferral() { }

        public void Complete()
        { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CDragEventArgs")]
    [Guids(ClassGuid = "9c6b4e13-b6b8-45f9-8a1b-250371c0aeae")]
    public sealed class DragEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strFilePaths")]
        private Windows.Foundation.String FilePaths
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bAllowDataAccess")]
        [DelegateToCore]
        private Windows.Foundation.Boolean AllowDataAccess
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [CoreType(typeof(Microsoft.UI.Xaml.DependencyObject))]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CDragEventArgs", "Data")]
        public Windows.ApplicationModel.DataTransfer.DataPackage Data
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.ApplicationModel.DataTransfer.DataPackageView DataView
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.DragUIOverride DragUIOverride
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.ApplicationModel.DataTransfer.DragDrop.DragDropModifiers Modifiers
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_acceptedOperation")]
        [DelegateToCore]
        public Windows.ApplicationModel.DataTransfer.DataPackageOperation AcceptedOperation
        {
            get;
            set;
        }

        public DragOperationDeferral GetDeferral()
        {
            return null;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_allowedOperations")]
        [DelegateToCore]
        public Windows.ApplicationModel.DataTransfer.DataPackageOperation AllowedOperations
        {
            get;
            internal set;
        }

        internal DragEventArgs() { }

        [PInvoke]
        public Windows.Foundation.Point GetPosition([Optional] Microsoft.UI.Xaml.UIElement relativeTo)
        {
            return default(Windows.Foundation.Point);
        }
    }

    internal interface IDragOperationDeferralTarget
    {
        void DeferralAdded();
        void DeferralCompleted();

        // Accepted operation might be setup asynchronously
        void SetAcceptedOperation(Windows.Foundation.Object source, Windows.ApplicationModel.DataTransfer.DataPackageOperation acceptedOperation);
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.IDragOperationDeferralTarget))]
    [Guids(ClassGuid = "200f072c-dafb-4cd3-a06b-8093ca146825")]
    public sealed class DragStartingEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.ApplicationModel.DataTransfer.DataPackage Data
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.DragUI DragUI
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public DragOperationDeferral GetDeferral()
        {
            return null;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Point GetPosition([Optional] Microsoft.UI.Xaml.UIElement relativeTo)
        {
            return default(Windows.Foundation.Point);
        }

        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.ApplicationModel.DataTransfer.DataPackageOperation AllowedOperations
        {
            get;
            set;
        }

        internal DragStartingEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "f994a62f-b072-4e48-a337-ea367bc1a208")]
    public sealed class DragUI
    {
        internal DragUI() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("SetContentFromBitmapImage")]
        [DXamlOverloadName("SetContentFromBitmapImage")]
        public void SetContentFromBitmapImage(Microsoft.UI.Xaml.Media.Imaging.BitmapImage bitmapImage) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("SetContentFromBitmapImageWithAnchorPoint")]
        [DXamlOverloadName("SetContentFromBitmapImage")]
        public void SetContentFromBitmapImage(Microsoft.UI.Xaml.Media.Imaging.BitmapImage bitmapImage, Windows.Foundation.Point anchorPoint) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("SetContentFromSoftwareBitmap")]
        [DXamlOverloadName("SetContentFromSoftwareBitmap")]
        public void SetContentFromSoftwareBitmap(Windows.Graphics.Imaging.SoftwareBitmap softwareBitmap) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("SetContentFromSoftwareBitmapWithAnchorPoint")]
        [DXamlOverloadName("SetContentFromSoftwareBitmap")]
        public void SetContentFromSoftwareBitmap(Windows.Graphics.Imaging.SoftwareBitmap softwareBitmap, Windows.Foundation.Point anchorPoint) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetContentFromDataPackage() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "{a1fcdb3c-2a9e-488a-a3e6-3ac7db0e1cfa}")]
    public sealed class DragUIOverride
    {
        internal DragUIOverride() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Clear() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("SetContentFromBitmapImage")]
        [DXamlOverloadName("SetContentFromBitmapImage")]
        public void SetContentFromBitmapImage(Microsoft.UI.Xaml.Media.Imaging.BitmapImage bitmapImage) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("SetContentFromBitmapImageWithAnchorPoint")]
        [DXamlOverloadName("SetContentFromBitmapImage")]
        public void SetContentFromBitmapImage(Microsoft.UI.Xaml.Media.Imaging.BitmapImage bitmapImage, Windows.Foundation.Point anchorPoint) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("SetContentFromSoftwareBitmap")]
        [DXamlOverloadName("SetContentFromSoftwareBitmap")]
        public void SetContentFromSoftwareBitmap(Windows.Graphics.Imaging.SoftwareBitmap softwareBitmap) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("SetContentFromSoftwareBitmapWithAnchorPoint")]
        [DXamlOverloadName("SetContentFromSoftwareBitmap")]
        public void SetContentFromSoftwareBitmap(Windows.Graphics.Imaging.SoftwareBitmap softwareBitmap, Windows.Foundation.Point anchorPoint) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public string Caption { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public bool IsContentVisible { get; set; }


        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public bool IsCaptionVisible { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public bool IsGlyphVisible { get; set; }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "7c1a59cb-0f5d-4604-9017-2cc8227b0fd5")]
    public sealed class DropCompletedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.ApplicationModel.DataTransfer.DataPackageOperation DropResult
        {
            get;
            internal set;
        }

        internal DropCompletedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CResourceDictionary")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<DependencyObject>))]
    [Implements(typeof(Windows.Foundation.Collections.IMap<Windows.Foundation.Object, Windows.Foundation.Object>))]
    [ClassFlags(IsIDictionary = true)]
    [Guids(ClassGuid = "22f9304f-eff6-4368-a49c-f193daf642b1")]
    public class ResourceDictionary
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Xaml.DependencyObject ContentProperty
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strSource")]
        public Windows.Foundation.Uri Source
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pMergedDictionaries")]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Internal.ResourceDictionaryCollection MergedDictionaries
        {
            get;
            set;
        }

        [CoreType(typeof(Microsoft.UI.Xaml.ResourceDictionary))]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pThemeDictionaries")]
        [ReadOnly]
        public Windows.Foundation.Collections.IMap<Windows.Foundation.Object, Windows.Foundation.Object> ThemeDictionaries
        {
            get;
            private set;
        }

        public ResourceDictionary() { }
    }

    [NativeName("CColorPaletteResources")]
    [Guids(ClassGuid = "ec22abc3-034e-4a37-a82d-eda167e9f28b")]
    [ClassFlags(IsIDictionary = true)]
    public class ColorPaletteResources
        : Microsoft.UI.Xaml.ResourceDictionary
    {
        // Base colors
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_AltHigh>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? AltHigh { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_AltLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? AltLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_AltMedium>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? AltMedium { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_AltMediumHigh>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? AltMediumHigh { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_AltMediumLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? AltMediumLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_BaseHigh>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? BaseHigh { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_BaseLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? BaseLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_BaseMedium>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? BaseMedium { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_BaseMediumHigh>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? BaseMediumHigh { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_BaseMediumLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? BaseMediumLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeAltLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeAltLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeBlackHigh>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeBlackHigh { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeBlackLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeBlackLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeBlackMediumLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeBlackMediumLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeBlackMedium>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeBlackMedium { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeDisabledHigh>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeDisabledHigh { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeDisabledLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeDisabledLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeHigh>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeHigh { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeMedium>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeMedium { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeMediumLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeMediumLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeWhite>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeWhite { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ChromeGray>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ChromeGray { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ListLow>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ListLow { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ListMedium>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ListMedium { get; set; }
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_ErrorText>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? ErrorText { get; set; }

        // Accent colors (a.k.a. Immersive colors)
        [NativeMethod("CColorPaletteResources", "Color<KnownPropertyIndex::ColorPaletteResources_Accent>")] [DependencyPropertyModifier(Modifier.Private)] public Windows.UI.Color? Accent { get; set; }

        public ColorPaletteResources() { }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [TypeFlags(IsWebHostHidden = false, GenerateMuseAttribute = true)]
    [Platform(typeof(PrivateApiContract), 1)]
    [Guids(ClassGuid = "16a66ea8-2913-449d-a183-5a5c6eea69ce")]
    public static class ColorDisplayNameHelper
    {
        [AllowCrossThreadAccess]
        public static string ToDisplayName(Windows.UI.Color color)
        {
            return default(string);
        }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "06dcc762-9c8a-4f91-8317-afaa9c1d7939")]
    public sealed class RoutedEvent
     : Windows.Foundation.Object
    {
        internal RoutedEvent() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CRoutedEventArgs")]
    [Guids(ClassGuid = "717320a8-ef3c-4907-86a1-aec7ae2490a3")]
    public class RoutedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true)]
        public Windows.Foundation.Object OriginalSource
        {
            get;
            internal set;
        }

        public RoutedEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CExceptionRoutedEventArgs")]
    [Guids(ClassGuid = "ab8dc20b-9719-4179-80a4-f58b38525a49")]
    public class ExceptionRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strErrorMessage")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.String ErrorMessage
        {
            get;
            private set;
        }

        internal ExceptionRoutedEventArgs() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CDeployment")]
    [Guids(ClassGuid = "b06d7542-2c18-4c8b-b3b6-9f7e89c91c05")]
    public sealed class Deployment
     : Microsoft.UI.Xaml.DependencyObject
    {
        public Deployment() { }
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "e84d11b4-a65f-4410-ba8a-be3f73a05ea1")]
    public sealed class BindingFailedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.String Message
        {
            get;
            internal set;
        }

        internal BindingFailedEventArgs() { }
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 4)]
    [FrameworkTypePattern]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "3e451929-9869-4c09-b43a-c0c294ee37df")]
    public sealed class XamlResourceReferenceFailedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.String Message
        {
            get;
            internal set;
        }

        internal XamlResourceReferenceFailedEventArgs() { }
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 6)]
    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public enum LayoutCycleTracingLevel
    {
        None = 0,
        Low = 1,
        High = 2,
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 6)]
    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public enum LayoutCycleDebugBreakLevel
    {
        None = 0,
        Low = 1,
        High = 2,
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [Guids(ClassGuid = "818dd7e4-0203-478d-a556-6b89215287e0")]
    [DXamlIdlGroup("coretypes2")]
    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 4)]
    [Platform(3, typeof(Microsoft.UI.Xaml.WinUIContract), 6)]
    public sealed class DebugSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean EnableFrameRateCounter
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsBindingTracingEnabled
        {
            get;
            set;
        }

        [Version(2)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsXamlResourceReferenceTracingEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsTextPerformanceVisualizationEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean FailFastOnErrors
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [VelocityFeature("Feature_ExperimentalApi")]
        public LayoutCycleTracingLevel LayoutCycleTracing
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [Version(3)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public LayoutCycleTracingLevel LayoutCycleTracingLevel
        {
            get;
            set;
        }

        [Version(3)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public LayoutCycleDebugBreakLevel LayoutCycleDebugBreakLevel
        {
            get;
            set;
        }

        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [VelocityFeature("Feature_ExperimentalApi")]
        public LayoutCycleDebugBreakLevel LayoutCycleDebugBreaks
        {
            get;
            set;
        }

        [AllowCrossThreadAccess]
        public event Microsoft.UI.Xaml.BindingFailedEventHandler BindingFailed;

        [Version(2)]
        [AllowCrossThreadAccess]
        public event Windows.Foundation.TypedEventHandler<DebugSettings, XamlResourceReferenceFailedEventArgs> XamlResourceReferenceFailed;

        internal DebugSettings() { }
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "A15AC525-CB6C-37F1-927D-B32BCF43EAAA")]
    public sealed class LaunchActivatedEventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        public Windows.Foundation.String Arguments
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        public Windows.ApplicationModel.Activation.LaunchActivatedEventArgs UWPLaunchActivatedEventArgs
        {
            get;
            internal set;
        }

        internal LaunchActivatedEventArgs() { }
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "dd062bdd-ccc8-43c5-88f1-5bb8255060dd")]
    public sealed class WindowEventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        public bool Handled { get; set; }

        internal WindowEventArgs() { }
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "b9796474-209f-4799-b9b1-f3a1ae5e8f57")]
    public sealed class WindowActivatedEventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        public bool Handled
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        public WindowActivationState WindowActivationState
        {
            get;
            internal set;
        }

        internal WindowActivatedEventArgs() { }
    }

    [FrameworkTypePattern]
    [Guids(ClassGuid = "159060E1-BAE1-45BD-B686-67CAC50DCB27")]
    [DXamlIdlGroup("coretypes2")]
    public sealed class WindowVisibilityChangedEventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        public bool Handled
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        public bool Visible
        {
            get;
            internal set;
        }

        internal WindowVisibilityChangedEventArgs() { }
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "a0822dce-f5c8-42b6-9ca4-fe7f3a18d675")]
    [VelocityFeature("Feature_UwpSupportApi")]
    public sealed class WindowCreatedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Window Window
        {
            get;
            internal set;
        }

        internal WindowCreatedEventArgs() { }
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "3245a2ca-e6da-482f-a972-ae0bdf7fec4a")]
    public sealed class WindowSizeChangedEventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        public bool Handled
        {
            get;
            set;
        }

        public Windows.Foundation.Size Size
        {
            get;
            internal set;
        }

        internal WindowSizeChangedEventArgs() { }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public enum ShutdownModel
    {
        Version1 = 1,
        Version2 = 2
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IFrameworkApplicationStaticsPrivate
    {
        [AllowCrossThreadAccess]
        void EnableFailFastOnStowedException();
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IFrameworkApplicationPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        void StartOnCurrentThread([Optional] Microsoft.UI.Xaml.ApplicationInitializationCallback callback);

        //  https://task.ms/19222320: The following factory methods should be consolidated into a single
        //  one once InputSite creation and transfer to the CompositionIsland is implemented.
        //  Specifically,
        //  - The contentBridge inspectables should be removed.
        //  - A hint indicating the particular hosting configuration appears necessary.
        //    For example, the XAML FocusController is only appropriate for nested focus
        //    scenarios (viz., DesktopWindowXamlSource and possibly TabShell).
        Microsoft.UI.Xaml.Hosting.XamlIslandRoot CreateIslandRoot();
        Microsoft.UI.Xaml.Hosting.XamlIslandRoot CreateIslandRootWithContentBridge(object owner, [Optional] object contentBridge);

        void RemoveIsland(Microsoft.UI.Xaml.Hosting.XamlIslandRoot island);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetSynchronizationWindow(UInt64 commitResizeWindow);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        [AllowCrossThreadAccess]
        Windows.Foundation.Collections.IVectorView<Microsoft.UI.Xaml.Window> Windows
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        ShutdownModel ShutdownModel {get; set;}
    }

    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [EnumFlags(HasTypeConverter = true)]
    [VelocityFeature("Feature_ExperimentalApi")]
    [Contract(typeof(WinUIContract), Microsoft.UI.Xaml.WinUIContract.LatestVersion)]
    public enum DispatcherShutdownMode
    {
        OnLastWindowClose = 0,
        OnExplicitShutdown = 1
    }

    [CodeGen(partial: true)]
    [DXamlName("Application")]
    [CustomNames(RealTypeName = "Microsoft.UI.Xaml.Application")]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true)]
    [ClassFlags(IsFreeThreaded = true)]
    [ThreadingModel(ThreadingModel.Both)]
    [Platform("Feature_UwpSupportApi", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Platform("Feature_ExperimentalApi", typeof(Microsoft.UI.Xaml.WinUIContract), Microsoft.UI.Xaml.WinUIContract.LatestVersion)]
    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 5)]
    [Implements(typeof(Microsoft.UI.Xaml.IFrameworkApplicationStaticsPrivate), IsStaticInterface = true)]
    [Implements(typeof(Microsoft.UI.Xaml.IFrameworkApplicationPrivate))]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "9e00ad29-1d7e-48dd-a33a-e22f902499ca")]
    public class FrameworkApplication
     : Windows.Foundation.Object
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        [AllowCrossThreadAccess]
        public static Microsoft.UI.Xaml.FrameworkApplication Current
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.ResourceDictionary Resources
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.DebugSettings DebugSettings
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.ApplicationTheme RequestedTheme
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.FocusVisualKind FocusVisualKind
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [VelocityFeature("Feature_UwpSupportApi")]
        public Microsoft.UI.Xaml.ApplicationRequiresPointerMode RequiresPointerMode
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.ApplicationHighContrastAdjustment HighContrastAdjustment
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromDXaml = true)]
        public event Microsoft.UI.Xaml.UnhandledExceptionEventHandler UnhandledException;

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [VelocityFeature("Feature_UwpSupportApi")]
        public event Microsoft.UI.Xaml.SuspendingEventHandler Suspending;

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [VelocityFeature("Feature_UwpSupportApi")]
        public event Microsoft.UI.Xaml.EventHandler Resuming;

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [VelocityFeature("Feature_UwpSupportApi")]
        public event Microsoft.UI.Xaml.LeavingBackgroundEventHandler LeavingBackground;

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [VelocityFeature("Feature_UwpSupportApi")]
        public event Microsoft.UI.Xaml.EnteredBackgroundEventHandler EnteredBackground;

        [CodeGen(CodeGenLevel.Idl)]
        [EventFlags(IsImplVirtual = true)]
        [Version(2)]
        public event Windows.Foundation.TypedEventHandler<Windows.Foundation.Object, ResourceManagerRequestedEventArgs> ResourceManagerRequested;

        public FrameworkApplication() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        public static void Start([Optional] Microsoft.UI.Xaml.ApplicationInitializationCallback callback)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Exit()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("LoadComponent")]
        [DXamlOverloadName("LoadComponent")]
        public static void LoadComponent(Windows.Foundation.Object component, Windows.Foundation.Uri resourceLocator)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("LoadComponentWithResourceLocation")]
        [DXamlOverloadName("LoadComponent")]
        public static void LoadComponent(Windows.Foundation.Object component, Windows.Foundation.Uri resourceLocator, Microsoft.UI.Xaml.Controls.Primitives.ComponentResourceLocation componentResourceLocation)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [OrderHint(1)]
        [VelocityFeature("Feature_UwpSupportApi")]
        protected virtual void OnWindowCreated(Microsoft.UI.Xaml.WindowCreatedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [VelocityFeature("Feature_UwpSupportApi")]
        protected virtual void OnActivated(Windows.ApplicationModel.Activation.IActivatedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [VelocityFeature("Feature_UwpSupportApi")]
        protected virtual void OnFileActivated(Windows.ApplicationModel.Activation.FileActivatedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [VelocityFeature("Feature_UwpSupportApi")]
        protected virtual void OnSearchActivated(Windows.ApplicationModel.Activation.SearchActivatedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [VelocityFeature("Feature_UwpSupportApi")]
        protected virtual void OnShareTargetActivated(Windows.ApplicationModel.Activation.ShareTargetActivatedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [VelocityFeature("Feature_UwpSupportApi")]
        protected virtual void OnFileOpenPickerActivated(Windows.ApplicationModel.Activation.FileOpenPickerActivatedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [VelocityFeature("Feature_UwpSupportApi")]
        protected virtual void OnFileSavePickerActivated(Windows.ApplicationModel.Activation.FileSavePickerActivatedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [VelocityFeature("Feature_UwpSupportApi")]
        protected virtual void OnCachedFileUpdaterActivated(Windows.ApplicationModel.Activation.CachedFileUpdaterActivatedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [VelocityFeature("Feature_UwpSupportApi")]
        protected virtual void OnBackgroundActivated(Windows.ApplicationModel.Activation.BackgroundActivatedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [VelocityFeature("Feature_ExperimentalApi")]
        public DispatcherShutdownMode DispatcherShutdownMode {get; set;}
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 5)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "5F5C6A8B-68DF-422D-96E3-483CD87705AC")]
    public sealed class ResourceManagerRequestedEventArgs
        : Microsoft.UI.Xaml.EventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public MicrosoftWindows.ApplicationModel.Resources.IResourceManager CustomResourceManager
        {
            get;
            set;
        }

        internal ResourceManagerRequestedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [ThreadingModel(ThreadingModel.Both)]
    [Implements(typeof(Windows.ApplicationModel.Core.IFrameworkViewSource))]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "19dae980-9da7-4843-9388-237f0c965989")]
    public sealed class FrameworkViewSource
     : Windows.Foundation.Object
    {
        public FrameworkViewSource() { }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [ThreadingModel(ThreadingModel.Both)]
    [Implements(typeof(Windows.ApplicationModel.Core.IFrameworkView))]
    [Guids(ClassGuid = "41521520-282b-438b-9bc9-e2ace21ce36e")]
    public sealed class FrameworkView
     : Windows.Foundation.Object
    {
        public FrameworkView() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly, Partial = true)]
    [NativeName("CApplication")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [HandWritten]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "571e0e0d-c1b0-432a-8bfb-17bd24c7e8d3")]
    public class Application
     : Microsoft.UI.Xaml.DependencyObject
    {
        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pRootVisual")]
        public Microsoft.UI.Xaml.UIElement RootVisual
        {
            get;
            set;
        }

        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pResources")]
        public Microsoft.UI.Xaml.ResourceDictionary Resources
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bApplicationStartupCompleted")]
        [ReadOnly]
        public Windows.Foundation.Boolean ApplicationStarted
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueEnum)]
        public Microsoft.UI.Xaml.ApplicationTheme RequestedTheme
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueEnum)]
        public Microsoft.UI.Xaml.ApplicationRequiresPointerMode RequiresPointerMode
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueEnum)]
        public Microsoft.UI.Xaml.ApplicationHighContrastAdjustment HighContrastAdjustment
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.StartupEventHandler Startup;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.EventHandler Exit;

        [CodeGen(CodeGenLevel.CoreOnly)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.StartupEventHandler Starting;

        [CodeGen(CodeGenLevel.CoreOnly)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.EventHandler Started;

        [CodeGen(CodeGenLevel.CoreOnly)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.EventHandler Exiting;

        [CodeGen(CodeGenLevel.CoreOnly)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.EventHandler Exited;

        public Application() { }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeClassName("CApplication")]
        public void RequestStartupEvent()
        {
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeClassName("CApplication")]
        public void RequestStartedEvent()
        {
        }

        [CodeGen(CodeGenLevel.Idl)]
        [NativeClassName("CApplication")]
        public void Run()
        {
        }

        [CodeGen(CodeGenLevel.Idl)]
        [NativeClassName("CApplication")]
        public void RegisterForActivation(Windows.Foundation.String activatableClassId)
        {
        }

        [CodeGen(CodeGenLevel.Idl)]
        [NativeClassName("CApplication")]
        public static void LoadComponent(Windows.Foundation.Object component, Windows.Foundation.String resourceLocator)
        {
        }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CStartupEventArgs")]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "5baa13a3-19c0-4399-8b2b-4b043c014f04")]
    public sealed class StartupEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        internal StartupEventArgs() { }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IAtlasRequestCallback
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        bool AtlasRequest(uint width, uint height, Windows.Graphics.DirectX.DirectXPixelFormat pixelFormat);
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IWindowPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Show();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Hide();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void MoveWindow(int x, int y, int width, int height);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean TransparentBackground
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetAtlasSizeHint(uint width, uint height);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ReleaseGraphicsDeviceOnSuspend(bool enable);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetAtlasRequestCallback([Optional] IAtlasRequestCallback callback);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Rect GetWindowContentBoundsForElement(Microsoft.UI.Xaml.DependencyObject element);
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [Implements(typeof(Microsoft.UI.Xaml.IWindowPrivate))]
    [Implements(typeof(Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop), Version = 1)]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1, ForcePrimaryInterfaceGeneration = true)]
    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 4)]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "b0d8d8be-9fae-4cdc-a457-523fb68b3953")]
    [ContentProperty("Content")]
    [NativeName("CWindow")]
    public class Window
     : Microsoft.UI.Xaml.DependencyObject
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
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
        public Windows.Foundation.Boolean Visible
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.UIElement Content
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
        [Deprecated("The CoreWindow property is deprecated and always returns null")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.UI.Core.CoreWindow CoreWindow
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Composition.Compositor Compositor
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [Deprecated("The Dispatcher property is deprecated and always returns null")]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public new Windows.UI.Core.CoreDispatcher Dispatcher
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public new Microsoft.UI.Dispatching.DispatcherQueue DispatcherQueue
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        public string Title
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        [RequiresMultipleAssociationCheck]
        [Version(2)]
        public Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        [AllowCrossThreadAccess]
        public static Microsoft.UI.Xaml.Window Current
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [Version(2)]
        [ReadOnly]
        public Microsoft.UI.Windowing.AppWindow AppWindow
        {
            get;
        }



        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromDXaml = true)]
        public event Windows.Foundation.TypedEventHandler<Object, Microsoft.UI.Xaml.WindowActivatedEventArgs> Activated;

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromDXaml = true)]
        public event Windows.Foundation.TypedEventHandler<Object, WindowEventArgs> Closed;

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromDXaml = true)]
        public event Windows.Foundation.TypedEventHandler<Object, Microsoft.UI.Xaml.WindowSizeChangedEventArgs> SizeChanged;

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromDXaml = true)]
        public event Windows.Foundation.TypedEventHandler<Object, Microsoft.UI.Xaml.WindowVisibilityChangedEventArgs> VisibilityChanged;

        public Window() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Activate()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Close()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public bool ExtendsContentIntoTitleBar
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]

        public void SetTitleBar([Optional] Microsoft.UI.Xaml.UIElement titleBar)
        {
        }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Platform(typeof(PrivateApiContract), 1)]
    [DXamlIdlGroup("Controls2")]
    [DXamlName("WindowChrome")]
    [NativeName("CWindowChrome")]
    [Guids(ClassGuid = "b2e03930-7cb2-4feb-a4b9-a38498b9d75e")]
    [TemplatePart("LayoutRoot", typeof(UIElement))]
    [TemplatePart("TitleBarMinMaxCloseContainer", typeof(UIElement))]
    [TemplatePart("MinMaxCloseContainer", typeof(UIElement))]
    [TemplatePart("MinimizeButton", typeof(Microsoft.UI.Xaml.Controls.Primitives.ButtonBase))]
    [TemplatePart("MaximizeButton", typeof(Microsoft.UI.Xaml.Controls.Primitives.ButtonBase))]
    [TemplatePart("CloseButton", typeof(Microsoft.UI.Xaml.Controls.Primitives.ButtonBase))] // will be created as a tracker pointer variable m_tpCloseButtonPart
    public sealed class WindowChrome
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
        private WindowChrome()
        {
        }

        public WindowChrome(Microsoft.UI.Xaml.Window parent)
        {
        }

        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_titleBarVisibility")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Visibility CaptionVisibility
        {
            get;
            private set; // Only changed by the core Xaml
        }
    }

    [AllowsMultipleAssociations]
    [NativeName("CFrameworkTemplate")]
    [ContentProperty("Template", forceIncludeInIdl: true)]
    [Guids(ClassGuid = "6d5cf429-6dee-4cd6-8ca3-8c4dd694ff21")]
    public abstract class FrameworkTemplate
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pTemplateContent")]
        internal Microsoft.UI.Xaml.Internal.TemplateContent Template
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Microsoft.UI.Xaml.DependencyObject EventRoot
        {
            get;
            set;
        }

        protected FrameworkTemplate() { }
    }

    [BuiltinStruct("CGridLength")]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "94aed029-a0d1-4d10-b2ce-81264430938f")]
    public struct GridLength
    {
        [PropertyFlags(IsReadOnlyExceptForParser = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_gridLength.value")]
        public Windows.Foundation.Double Value
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_gridLength.type")]
        public Microsoft.UI.Xaml.GridUnitType GridUnitType
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.GridLength Auto
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyFlags(IsHelper = true)]
        [ReadOnly]
        public Windows.Foundation.Boolean IsAbsolute
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyFlags(IsHelper = true)]
        [ReadOnly]
        public Windows.Foundation.Boolean IsAuto
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyFlags(IsHelper = true)]
        [ReadOnly]
        public Windows.Foundation.Boolean IsStar
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Boolean Equals(Microsoft.UI.Xaml.GridLength value)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.GridLength FromPixels(Windows.Foundation.Double pixels)
        {
            return default(Microsoft.UI.Xaml.GridLength);
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.GridLength FromValueAndType(Windows.Foundation.Double value, Microsoft.UI.Xaml.GridUnitType type)
        {
            return default(Microsoft.UI.Xaml.GridLength);
        }
    }

    [DXamlIdlGroup("Controls")]
    public interface IDataTemplateExtension
     : Microsoft.UI.Xaml.Data.INotifyPropertyChanged
    {
        void ResetTemplate();

        bool ProcessBinding(Windows.Foundation.UInt32 phase);

        int ProcessBindings(Microsoft.UI.Xaml.Controls.ContainerContentChangingEventArgs arg);
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "0b29bb5f-eadf-4f48-a1ac-9d73de7b4774")]
    public class ElementFactoryGetArgs : Windows.Foundation.Object
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        public object Data { get; set; }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.UIElement Parent { get; set; }

        public ElementFactoryGetArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "47dd5be5-8a0d-4f61-9219-efc20b6fcaf0")]
    public class ElementFactoryRecycleArgs : Windows.Foundation.Object
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.UIElement Element { get; set; }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.UIElement Parent { get; set; }

        public ElementFactoryRecycleArgs() { }
    }

    [TypeTable(ForceInclude =true)]
    public interface IElementFactory
    {
        Microsoft.UI.Xaml.UIElement GetElement(ElementFactoryGetArgs args);
        void RecycleElement(ElementFactoryRecycleArgs args);
    }

    [NativeName("CDataTemplate")]
    [Guids(ClassGuid = "2ba5f834-0618-4292-bb15-ea4f88f4ecd2")]
    [TypeTable(IsExcludedFromVisualTree = true)]
    [Implements(typeof(Microsoft.UI.Xaml.IElementFactory), 1)]
    [DXamlIdlGroup("Controls")]
    [CodeGen(partial: true)]
    public class DataTemplate
     : Microsoft.UI.Xaml.FrameworkTemplate
    {
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [NativeStorageType(OM.ValueType.valueObject)]
        public static Microsoft.UI.Xaml.IDataTemplateExtension AttachedExtensionInstance
        {
            get;
            set;
        }

        public DataTemplate() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.DependencyObject LoadContent()
        {
            return default(Microsoft.UI.Xaml.DependencyObject);
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CSizeChangedEventArgs")]
    [Guids(ClassGuid = "1b1829a5-a84c-44d0-abee-9a0ec040d8e1")]
    public sealed class SizeChangedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueSize)]
        [OffsetFieldName("m_previousSize")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Size PreviousSize
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueSize)]
        [OffsetFieldName("m_newSize")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Size NewSize
        {
            get;
            private set;
        }

        internal SizeChangedEventArgs() { }
    }

    [AllowsMultipleAssociations]
    [NativeName("CStyle")]
    [ContentProperty("Setters")]
    [TypeTable(IsExcludedFromVisualTree = true)]
    [Guids(ClassGuid = "2f4d5ca5-12c5-4473-a718-31c0fed45a79")]
    public sealed class Style
     : Microsoft.UI.Xaml.DependencyObject
    {

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bIsSealed")]
        [ReadOnly]
        public Windows.Foundation.Boolean IsSealed
        {
            get;
            private set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pSetters")]
        public Microsoft.UI.Xaml.SetterBaseCollection Setters
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        [NativeStorageType(OM.ValueType.valueTypeHandle)]
        [OffsetFieldName("m_targetTypeIndex")]
        public Windows.UI.Xaml.Interop.TypeName TargetType
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pBasedOn")]
        public Microsoft.UI.Xaml.Style BasedOn
        {
            get;
            set;
        }

        public Style() { }

        public Style(Windows.UI.Xaml.Interop.TypeName targetType) { }

        [PInvoke]
        public void Seal()
        {
        }
    }

    [AllowsMultipleAssociations]
    [NativeName("CSetterBase")]
    [Guids(ClassGuid = "52552fd7-930f-4ba1-84fe-a659470b7d94")]
    public abstract class SetterBase
     : Microsoft.UI.Xaml.DependencyObject
    {

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bIsSealed")]
        [ReadOnly]
        public Windows.Foundation.Boolean IsSealed
        {
            get;
            private set;
        }

        internal SetterBase() { }
    }

    [NativeName("CSetter")]
    [Guids(ClassGuid = "7c425f15-7da5-4f20-b7cc-39527248803b")]
    public sealed class Setter
     : Microsoft.UI.Xaml.SetterBase
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pDependencyPropertyProxy")]
        public Microsoft.UI.Xaml.Internal.DependencyPropertyProxy Property
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [OffsetFieldName("m_vValue")]
        [PreserveThemeResourceExtension]
        public Windows.Foundation.Object Value
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_target")]
        public Microsoft.UI.Xaml.TargetPropertyPath Target
        {
            get;
            set;
        }

        public Setter() { }

        // NOTE: no constructor should take a parameter with the name "value". It breaks API guidelines and "value" is the name
        // of the return parameter that ModernIDL produces for us. Our code gen tool special cases this constructor, and provides
        // an overrriden return name. We only have to do this because this API has already shipped. No new APIs should try to
        // use "value".
        public Setter(Microsoft.UI.Xaml.Internal.DependencyPropertyProxy targetProperty, Windows.Foundation.Object value) { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CSetterBaseCollection")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "9edbefcb-0185-4422-a049-e3dec4edbaa4")]
    public class SetterBaseCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<SetterBase>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Xaml.SetterBase ContentProperty
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bIsSealed")]
        [ReadOnly]
        public Windows.Foundation.Boolean IsSealed
        {
            get;
            private set;
        }

        public SetterBaseCollection() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CRootVisual")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "11284afe-6b4c-4ddc-acdd-3afeb1c4550a")]
    public sealed class RootVisual
     : Microsoft.UI.Xaml.Controls.Panel
    {
        public RootVisual() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CFullWindowMediaRoot")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "57ae47fb-9c04-4c89-b631-ce3721d14619")]
    internal sealed class FullWindowMediaRoot
     : Microsoft.UI.Xaml.Controls.Panel
    {
        public FullWindowMediaRoot() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CPopupRoot")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "477028ef-82a2-46b8-a277-10df10fc39d3")]
    internal sealed class PopupRoot
     : Microsoft.UI.Xaml.Controls.Canvas
    {
        public PopupRoot() { }
    }

    [NativeName("CTransitionRoot")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "0db72294-3ad7-49e2-8e09-3c305e06fbd3")]
    internal sealed class TransitionRoot
     : Microsoft.UI.Xaml.Controls.Canvas
    {
        public TransitionRoot() { }
    }

    [NativeName("CRenderTargetBitmapRoot")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "8788eb04-29e1-4ea2-97a6-a2342de92fdc")]
    internal sealed class RenderTargetBitmapRoot
     : Microsoft.UI.Xaml.Controls.Panel
    {
        public RenderTargetBitmapRoot() { }
    }

    [NativeName("CConnectedAnimationRoot")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "22467b81-5594-4435-ad75-dabf1fe3d66a")]
    internal sealed class ConnectedAnimationRoot
     : Microsoft.UI.Xaml.FrameworkElement
    {
        public ConnectedAnimationRoot() { }
    }

    [NativeName("CXamlIslandRootCollection")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "de05fcb7-22f9-4459-b736-4fc40a9b88df")]
    internal sealed class XamlIslandRootCollection
     : Microsoft.UI.Xaml.Controls.Panel
    {
        public XamlIslandRootCollection() { }
    }

    [NativeName("CVisualState")]
    [ContentProperty("Storyboard")]
    [Guids(ClassGuid = "c3a4e1cb-ce7b-41ea-9ffd-2fb355cbe8e2")]
    public sealed class VisualState
     : Microsoft.UI.Xaml.DependencyObject
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public new Windows.Foundation.String Name
        {
            get;
            private set;
        }

        [RequiresMultipleAssociationCheck]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CVisualState", "Storyboard")]
        public Microsoft.UI.Xaml.Media.Animation.Storyboard Storyboard
        {
            get;
            set;
        }

        [Comment("This is a deferred property. Its xaml is not parsed immediately, the content that would be in the storyboard is kept as a node stream and hydrating when it is accessed. TODO: Can it have a NULL name?")]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pDeferredStoryboard")]
        internal Microsoft.UI.Xaml.Internal.TemplateContent __DeferredStoryboard
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_setters")]
        public Microsoft.UI.Xaml.SetterBaseCollection Setters
        {
            get;
            set;
        }

        [Comment("This is a deferred property. Its xaml is not parsed immediately, the content that would be in the setters is kept as a node stream and hydrating when it is accessed. TODO: Can it have a NULL name?")]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pDeferredSetters")]
        internal Microsoft.UI.Xaml.Internal.TemplateContent __DeferredSetters
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [CollectionType(CollectionKind.Vector)]
        [OffsetFieldName("m_pStateTriggerCollection")]
        public Microsoft.UI.Xaml.Internal.StateTriggerCollection StateTriggers
        {
            get;
            set;
        }

        public VisualState() { }
    }

    [Guids(ClassGuid = "c331ef4e-e041-471f-8377-212f97989e8a")]
    [CodeGen(partial: true)]
    [NativeName("CStateTriggerBase")]
    [AllowsMultipleAssociations]
    public abstract class StateTriggerBase
     : Microsoft.UI.Xaml.DependencyObject
    {
        public StateTriggerBase() { }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_triggerState")]
        internal Windows.Foundation.Boolean TriggerState
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected void SetActive(Windows.Foundation.Boolean IsActive)
        {
        }
    }


    [Guids(ClassGuid = "71c12614-5a1c-4bdd-b44d-02fbf330e787")]
    [NativeName("CAdaptiveTrigger")]
    [AllowsMultipleAssociations]
    public class AdaptiveTrigger
     : Microsoft.UI.Xaml.StateTriggerBase
    {
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_minWindowWidth")]
        public Windows.Foundation.Double MinWindowWidth
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_minWindowHeight")]
        public Windows.Foundation.Double MinWindowHeight
        {
            get;
            set;
        }

        public AdaptiveTrigger() { }
    }

    [AllowsMultipleAssociations]
    [Guids(ClassGuid = "cbc3a15e-2ec8-4334-80cc-8b488805ff4a")]
    [NativeName("CStateTrigger")]
    public sealed class StateTrigger
     : Microsoft.UI.Xaml.StateTriggerBase
    {
        public StateTrigger() { }

        [NativeMethod("CStateTrigger", "IsActive")]
        [NativeStorageType(OM.ValueType.valueBool)]
        public Windows.Foundation.Boolean IsActive
        {
            get;
            set;
        }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CStaticResourceExtension")]
    [ClassFlags(IsMarkupExtension = true)]
    [Guids(ClassGuid = "c5cdd55c-bc8d-4e36-9746-9b42a2bda69c")]
    public sealed class StaticResource
     : Microsoft.UI.Xaml.MarkupExtensionBase
    {
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strResourceKey")]
        public Windows.Foundation.String ResourceKey
        {
            get;
            set;
        }

        public StaticResource() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CustomResourceExtension")]
    [ClassFlags(IsMarkupExtension = true)]
    [Guids(ClassGuid = "90d80480-5926-4261-bd0d-97913d306c2f")]
    public sealed class CustomResource
     : Microsoft.UI.Xaml.MarkupExtensionBase
    {
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strResourceKey")]
        public Windows.Foundation.String ResourceKey
        {
            get;
            set;
        }

        public CustomResource() { }
    }

    [AllowsMultipleAssociations]
    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CThemeResourceExtension")]
    [ClassFlags(IsMarkupExtension = true)]
    [Guids(ClassGuid = "669bd442-5963-4f37-bdbe-abd6fef5a47d")]
    public sealed class ThemeResource
     : Microsoft.UI.Xaml.MarkupExtensionBase
    {
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strResourceKey")]
        public Windows.Foundation.String ResourceKey
        {
            get;
            set;
        }

        public ThemeResource() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CBasedOnSetterCollection")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "cf854d3f-475c-41c9-b510-a2f6644a8822")]
    public class BasedOnSetterCollection
     : Microsoft.UI.Xaml.SetterBaseCollection
    {
        public BasedOnSetterCollection() { }
    }

    [CodeGen(partial: true)]
    [Comment("This isn't a type converter, but it's private and shouldn't be parsed from XAML")]
    [NativeName("CVisualStateGroup")]
    [ContentProperty("States")]
    [ClassFlags(IsPublicInSL4 = true, IsVisibleInXAML = false)]
    [Guids(ClassGuid = "f4fa1e98-85c9-4ecd-a13b-2a7b1e236df5")]
    public sealed class VisualStateGroup
     : Microsoft.UI.Xaml.DependencyObject
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public new Windows.Foundation.String Name
        {
            get;
            private set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pTransitions")]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Internal.VisualTransitionCollection Transitions
        {
            get;
            set;
        }

        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pVisualStates")]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Internal.VisualStateCollection States
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.VisualState CurrentState
        {
            get;
            private set;
        }

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.VisualStateChangedEventHandler CurrentStateChanged;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.VisualStateChangedEventHandler CurrentStateChanging;

        public VisualStateGroup() { }
    }

    [NativeName("CVisualTransition")]
    [ContentProperty("Storyboard")]
    [ClassFlags(IsPublicInSL4 = true, IsVisibleInXAML = false)]
    [Guids(ClassGuid = "8d9aa1b2-bfdf-4c34-b179-60eee810942f")]
    public class VisualTransition
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [OffsetFieldName("m_duration")]
        [NativeStorageType(OM.ValueType.valueVO)]
        [PropertyFlags(DoNotEnterOrLeaveValue = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Duration GeneratedDuration
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pEasingFunction")]
        public Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase GeneratedEasingFunction
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strTo")]
        public Windows.Foundation.String To
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strFrom")]
        public Windows.Foundation.String From
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pStoryboard")]
        public Microsoft.UI.Xaml.Media.Animation.Storyboard Storyboard
        {
            get;
            set;
        }

        public VisualTransition() { }
    }

    [NativeName("CVisualStateChangedEventArgs")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "c6885eb2-9995-40e7-b323-8c9422f8a52c")]
    public sealed class VisualStateChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pOldState")]
        [DelegateToCore]
        public Microsoft.UI.Xaml.VisualState OldState
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pNewState")]
        [DelegateToCore]
        public Microsoft.UI.Xaml.VisualState NewState
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pControl")]
        [DelegateToCore]
        public Microsoft.UI.Xaml.Controls.Control Control
        {
            get;
            set;
        }

        public VisualStateChangedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CDependencyObjectCollection")]
    [ClassFlags(IsObservable = true)]
    [OldCodeGenBaseType(typeof(Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Windows.Foundation.Object>))]
    [Guids(ClassGuid = "db2b3183-1b6a-4be7-a31b-6ca9a395f272")]
    public class DependencyObjectCollection
     : Microsoft.UI.Xaml.Collections.ObservablePresentationFrameworkCollection<DependencyObject>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Xaml.DependencyObject ContentProperty
        {
            get;
            set;
        }

        public DependencyObjectCollection() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CPrintRoot")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "a683727d-a3eb-4367-b736-d33a8f0a4c3d")]
    internal sealed class PrintRoot
     : Microsoft.UI.Xaml.Controls.Canvas
    {
        public PrintRoot() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [NativeName("CVisualStateManager")]
    [Guids(ClassGuid = "85a77c1b-a554-4d4c-abc1-6d603c68a50f")]
    public class VisualStateManager
     : Microsoft.UI.Xaml.DependencyObject
    {
        [Attached(TargetParameterName = "obj", TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [PropertyFlags(IsValueCreatedOnDemand = true, HadFieldInBlue = true)]
        [DependencyPropertyModifier(Modifier.Internal)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [CollectionType(CollectionKind.Vector)]
        public static Microsoft.UI.Xaml.Internal.VisualStateGroupCollection AttachedVisualStateGroups
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [Attached(TargetParameterName = "obj", TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public static Microsoft.UI.Xaml.VisualStateManager AttachedCustomVisualStateManager
        {
            get;
            set;
        }

        public VisualStateManager() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.Boolean GoToState([Optional] Microsoft.UI.Xaml.Controls.Control control, [Optional] Windows.Foundation.String stateName, Windows.Foundation.Boolean useTransitions)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual Windows.Foundation.Boolean GoToStateCore([Optional] Microsoft.UI.Xaml.Controls.Control control, Microsoft.UI.Xaml.FrameworkElement templateRoot, [Optional] Windows.Foundation.String stateName, [Optional] Microsoft.UI.Xaml.VisualStateGroup group, [Optional] Microsoft.UI.Xaml.VisualState state, Windows.Foundation.Boolean useTransitions)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        protected void RaiseCurrentStateChanging(Microsoft.UI.Xaml.VisualStateGroup stateGroup, Microsoft.UI.Xaml.VisualState oldState, Microsoft.UI.Xaml.VisualState newState, Microsoft.UI.Xaml.Controls.Control control)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        protected void RaiseCurrentStateChanged(Microsoft.UI.Xaml.VisualStateGroup stateGroup, Microsoft.UI.Xaml.VisualState oldState, Microsoft.UI.Xaml.VisualState newState, Microsoft.UI.Xaml.Controls.Control control)
        {
        }
    }

    [DXamlIdlGroup("coretypes2")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Comment("A number of our attached properties have native declarations on UIElement but managed declarations on other managed types. This presents a problem for our property lookup because its 'native DP on a managed type' logic checks to see whether the declaring type is native.  We need to add entries in the core lookup so the types declaring the attached DPs appear to be native types.")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "64cfc695-0144-45fd-9248-7ff3338ec849")]
    internal static class TextOptions
    {
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_textOptions.m_textHintingMode")]
        public static Microsoft.UI.Xaml.Media.TextHintingMode AttachedTextHintingMode
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_textOptions.m_textFormattingMode")]
        public static Microsoft.UI.Xaml.Media.TextFormattingMode AttachedTextFormattingMode
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_textOptions.m_textRenderingMode")]
        public static Microsoft.UI.Xaml.Media.TextRenderingMode AttachedTextRenderingMode
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("coretypes2")]
    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public interface IElementSoundPlayerStaticsPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void RequestInteractionSoundForElement(Microsoft.UI.Xaml.ElementSoundKind sound, Microsoft.UI.Xaml.DependencyObject element);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.ElementSoundMode GetEffectiveSoundMode(Microsoft.UI.Xaml.DependencyObject element);
    }

    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.IElementSoundPlayerStaticsPrivate), IsStaticInterface = true)]
    [Guids(ClassGuid = "2296ace8-f2ee-41c1-8a1f-9abaefe57f18")]
    public sealed class ElementSoundPlayer
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public static Windows.Foundation.Double Volume
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public static Microsoft.UI.Xaml.ElementSoundPlayerState State
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void Play(Microsoft.UI.Xaml.ElementSoundKind sound)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public static Microsoft.UI.Xaml.ElementSpatialAudioMode SpatialAudioMode
        {
            get;
            set;
        }

        internal ElementSoundPlayer() { }
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [Guids(ClassGuid = "5bc39d6c-9e2c-4a4a-b517-3995caf1aca3")]
    internal sealed class ElementSoundPlayerService
      : Windows.Foundation.Object
    {
        internal ElementSoundPlayerService() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CLengthConverter")]
    [Guids(ClassGuid = "ae22fc5b-ffd5-4857-9359-8e003483b738")]
    public sealed class LengthConverter
     : Microsoft.UI.Xaml.DependencyObject
    {
        public LengthConverter() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly, Partial = true)]
    [NativeName("CMarkupExtensionBase")]
    [ClassFlags(IsMarkupExtension = true, IsHiddenFromIdl = true)]
    [Guids(ClassGuid = "236130de-0403-4271-9282-bc55f50d85dc")]
    public class MarkupExtensionBase
     : Microsoft.UI.Xaml.DependencyObject
    {
        internal MarkupExtensionBase() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CNullExtension")]
    [ClassFlags(IsMarkupExtension = true)]
    [Guids(ClassGuid = "4fa931be-143a-4812-8cac-d14bae5bab1c")]
    public sealed class NullExtension
     : Microsoft.UI.Xaml.MarkupExtensionBase
    {
        public NullExtension() { }
    }

    [CodeGen(CodeGenLevel.Idl)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [ClassFlags(IsFreeThreaded = true)]
    [ThreadingModel(ThreadingModel.Both)]
    [HandWritten]
    [Guids(ClassGuid = "1019f29d-07e1-49ac-ae0a-c2794425a9a6")]
    public sealed class DependencyProperty
     : Windows.Foundation.Object
    {
        internal DependencyProperty()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public PropertyMetadata GetMetadata(Windows.UI.Xaml.Interop.TypeName forType)
        {
            return default(PropertyMetadata);
        }

        public static Windows.Foundation.Object UnsetValue
        {
            get
            {
                return default(Windows.Foundation.Object);
            }
        }

        public static DependencyProperty Register(
            string name,
            Windows.UI.Xaml.Interop.TypeName propertyType,
            Windows.UI.Xaml.Interop.TypeName ownerType,
            PropertyMetadata typeMetadata)
        {
            return default(DependencyProperty);
        }

        public static DependencyProperty RegisterAttached(
            string name,
            Windows.UI.Xaml.Interop.TypeName propertyType,
            Windows.UI.Xaml.Interop.TypeName ownerType,
            PropertyMetadata defaultMetadata)
        {
            return default(DependencyProperty);
        }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "00d8fc9c-275a-47fb-8470-550b0b767e60")]
    public class DataTemplateKey
     : Windows.Foundation.Object
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object DataType
        {
            get;
            set;
        }

        public DataTemplateKey() { }

        [FactoryMethodName("CreateInstanceWithType")]
        public DataTemplateKey(Windows.Foundation.Object dataType) { }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [ClassFlags(IsFreeThreaded = true)]
    [ThreadingModel(ThreadingModel.Both)]
    [Guids(ClassGuid = "c967f75b-2b94-4121-b3f1-5486b763364a")]
    public class PropertyMetadata
     : Windows.Foundation.Object
    {
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [PropertyFlags(UseComPtr = true)]
        public Windows.Foundation.Object DefaultValue
        {
            get;
            internal set;
        }

        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [PropertyFlags(UseComPtr = true)]
        public Microsoft.UI.Xaml.CreateDefaultValueCallback CreateDefaultValueCallback
        {
            get;
            internal set;
        }

        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [PropertyFlags(UseComPtr = true)]
        internal Microsoft.UI.Xaml.PropertyChangedCallback PropertyChangedCallback
        {
            get;
            set;
        }

        [FactoryMethodName("CreateInstanceWithDefaultValue")]
        public PropertyMetadata([Optional] Windows.Foundation.Object defaultValue) { }

        [FactoryMethodName("CreateInstanceWithDefaultValueAndCallback")]
        public PropertyMetadata([Optional] Windows.Foundation.Object defaultValue, [Optional] Microsoft.UI.Xaml.PropertyChangedCallback propertyChangedCallback) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [DXamlOverloadName("Create")]
        [DefaultOverload]
        public static Microsoft.UI.Xaml.PropertyMetadata CreateWithDefaultValue([Optional] Windows.Foundation.Object defaultValue)
        {
            return default(Microsoft.UI.Xaml.PropertyMetadata);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [DXamlOverloadName("Create")]
        [DefaultOverload]
        public static Microsoft.UI.Xaml.PropertyMetadata CreateWithDefaultValueAndCallback([Optional] Windows.Foundation.Object defaultValue, [Optional] Microsoft.UI.Xaml.PropertyChangedCallback propertyChangedCallback)
        {
            return default(Microsoft.UI.Xaml.PropertyMetadata);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [DXamlOverloadName("Create")]
        public static Microsoft.UI.Xaml.PropertyMetadata CreateWithFactory(Microsoft.UI.Xaml.CreateDefaultValueCallback createDefaultValueCallback)
        {
            return default(Microsoft.UI.Xaml.PropertyMetadata);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [DXamlOverloadName("Create")]
        public static Microsoft.UI.Xaml.PropertyMetadata CreateWithFactoryAndCallback(Microsoft.UI.Xaml.CreateDefaultValueCallback createDefaultValueCallback, [Optional] Microsoft.UI.Xaml.PropertyChangedCallback propertyChangedCallback)
        {
            return default(Microsoft.UI.Xaml.PropertyMetadata);
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CMediaFailedRoutedEventArgs")]
    [Guids(ClassGuid = "61ff5170-b605-4cd3-a40e-bcd95fddc491")]
    public sealed class MediaFailedRoutedEventArgs
     : Microsoft.UI.Xaml.ExceptionRoutedEventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strMediaErrorTrace")]
        [ReadOnly]
        public Windows.Foundation.String ErrorTrace
        {
            get;
            private set;
        }

        internal MediaFailedRoutedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [ClassFlags(IsCreateableAfterV2 = false, IsVisibleInXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "7b532db7-0df8-4a1d-b47e-457bbafdb173")]
    internal sealed class LayoutTransitionStaggerItem
     : Microsoft.UI.Xaml.DependencyObject
    {
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.UIElement Element
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.Idl)]
        public Windows.Foundation.Rect Bounds
        {
            get;
            internal set;
        }

        public Windows.Foundation.Int32 Index
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.Idl)]
        public Windows.Foundation.TimeSpan StaggerTime
        {
            get;
            set;
        }

        internal LayoutTransitionStaggerItem() { }
    }

    [TypeTable(IsExcludedFromCore = true)]
    [ThreadingModel(ThreadingModel.Both)]
    [Guids(ClassGuid = "09a587f5-1500-4d9f-8a68-772c490dc433")]
    public sealed class UnhandledExceptionEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.HRESULT Exception
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.String Message
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        internal UnhandledExceptionEventArgs() { }
    }

    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "50f58b0f-03dd-496f-ad6d-27d88079d3b5")]
    public sealed class ApplicationInitializationCallbackParams
     : Windows.Foundation.Object
    {
        internal ApplicationInitializationCallbackParams() { }
    }

    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public delegate void ApplicationInitializationCallback(ApplicationInitializationCallbackParams p);

    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [EnumFlags(HasTypeConverter = true)]
    public enum WindowActivationState
    {
        CodeActivated = 0,
        Deactivated = 1,
        PointerActivated = 2,
    }

    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [EnumFlags(HasTypeConverter = true)]
    public enum DurationType
    {
        Automatic = 0,
        TimeSpan = 1,
        Forever = 2,
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [NativeName("TransitionTriggerPublic")]
    [EnumFlags(IsNativeTypeDef = true)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public enum TransitionTrigger
    {
        [NativeValueName("LoadTriggerPublic")]
        Load = 0,
        [NativeValueName("LayoutTriggerPublic")]
        Layout = 1,
        [NativeValueName("UnloadTriggerPublic")]
        Unload = 2,
        [NativeValueName("ReparentTriggerPublic")]
        Reparent = 3,
        [NativeValueName("NoTrigger")]
        [CodeGen(CodeGenLevel.CoreOnly)]
        NoTrigger = 4
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [NativeName("TransitionParent")]
    [EnumFlags(IsNativeTypeDef = true)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public enum TransitionParent
    {
        [NativeValueName("ParentToRoot")]
        ParentToRoot = 0,
        [NativeValueName("ParentToCommonParent")]
        ParentToCommonParent = 1,
        [NativeValueName("ParentToGrandParent")]
        ParentToGrandParent = 2
    }

    [Comment("WrapWholeWords is not exposed is Silverlight.")]
    [NativeName("TextWrapping")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("Determines whether and how to wrap text across multiple lines.")]
    [DXamlIdlGroup("coretypes2")]
    public enum TextWrapping
    {
        [NativeComment("No line wrapping is performed. In the case when lines are longer than the available block width, the overflow will be treated in accordance with the 'overflow' property specified in the element.")]
        [NativeValueName("NoWrap")]
        NoWrap = 1,
        [NativeComment("Line-breaking occurs if the line overflow the available block width, even if the standard line breaking algorithm cannot determine any opportunity. For example, this deals with the situation of very long words constrained in a fixed-width container with no scrolling allowed.")]
        [NativeValueName("Wrap")]
        Wrap = 2,
        [NativeComment("Forced line breaking is disabled.")]
        [NativeValueName("WrapWholeWords")]
        WrapWholeWords = 3,
    }

    [NativeName("TextTrimming")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("This property determines how text is trimmed when it overflows the edge of its containing box.")]
    [DXamlIdlGroup("coretypes2")]
    public enum TextTrimming
    {
        [NativeComment("Default no trimming.")]
        [NativeValueName("TextTrimmingNone")]
        None = 0,
        [NativeComment("Text is trimmed at character boundary. Ellipsis is drawn in place of invisible part.")]
        [NativeValueName("TextTrimmingCharacterEllipsis")]
        CharacterEllipsis = 1,
        [NativeComment("Text is trimmed at word boundary. Ellipsis is drawn in place of invisible part.")]
        [NativeValueName("TextTrimmingWordEllipsis")]
        WordEllipsis = 2,
        [NativeComment("Text is trimmed by visually clipping the excess glyph(s)")]
        [NativeValueName("TextTrimmingClip")]
        Clip = 3,
    }

    [NativeName("TextReadingOrder")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("This property determines whether the text reading order should be inferred from the pragraph content.")]
    [DXamlIdlGroup("coretypes2")]
    public enum TextReadingOrder
    {
        [Deprecated("Consider using UseFlowDirection, which is an improved version of Default. For more info, see MSDN.")]
        [NativeComment("Default reading order.")]
        [NativeValueName("TextReadingOrderDefault")]
        Default = 0,
        [NativeComment("Use flow direction.")]
        [NativeValueName("TextReadingOrderUseFlowDirection")]
        UseFlowDirection = 0,
        [NativeComment("Detect reading order from content")]
        [NativeValueName("TextReadingOrderDetectFromContent")]
        DetectFromContent = 1,
    }

    [NativeName("OpticalMarginAlignment")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("This property determines if text should be aligned visually or on its glyph edge.")]
    [DXamlIdlGroup("coretypes2")]
    public enum OpticalMarginAlignment
    {
        [NativeComment("Text is aligned to its glyph edge")]
        [NativeValueName("OpticalMarginAlignmentNone")]
        None = 0,
        [NativeComment("Text's side bearings are trimmed and layout boxes are aligned visually.")]
        [NativeValueName("OpticalMarginAlignmentTrimSideBearings")]
        TrimSideBearings = 1,
    }

    [NativeName("Visibility")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("Determines the visibility of the visual. Note that these are stored as a bit, which should be changed when adding enumerations.")]
    public enum Visibility
    {
        [NativeComment("The visual is visible.")]
        [NativeValueName("Visible")]
        Visible = 0,
        [NativeComment("The visual is collapsed.")]
        [NativeValueName("Collapsed")]
        Collapsed = 1,
    }

    [NativeName("ElementSoundKind")]
    [NativeComment("Xaml's platform sounds")]
    public enum ElementSoundKind
    {
        [NativeComment("Plays when focus shifts.")]
        [NativeValueName("Focus")]
        Focus = 0,
        [NativeComment("Plays when an element is invoked.")]
        [NativeValueName("Invoke")]
        Invoke = 1,
        [NativeComment("Plays when an overlaying UI is shown.")]
        [NativeValueName("Show")]
        Show = 2,
        [NativeComment("Plays when an overlaying UI is hidden.")]
        [NativeValueName("Hide")]
        Hide = 3,
        [NativeComment("Plays on navigating to a previous view.")]
        [NativeValueName("MovePrevious")]
        MovePrevious = 4,
        [NativeComment("Plays on navigating to the next view.")]
        [NativeValueName("MoveNext")]
        MoveNext = 5,
        [NativeComment("Plays on back navigation.")]
        [NativeValueName("GoBack")]
        GoBack = 6,
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("ElementSoundPlayerState")]
    [NativeComment("The state of the ElementSoundPlayer.")]
    public enum ElementSoundPlayerState
    {
        [NativeComment("The ElementSoundPlayer plays sounds it deems the context appropriate.")]
        [NativeValueName("Auto")]
        Auto = 0,
        [NativeComment("The ElementSoundPlayer is off.")]
        [NativeValueName("Off")]
        Off = 1,
        [NativeComment("The ElementSoundPlayer is on.")]
        [NativeValueName("On")]
        On = 2,
    }

    [NativeName("ElementSoundMode")]
    [NativeComment("The element sound mode for a control.")]
    public enum ElementSoundMode
    {
        [NativeComment("Respects the ElementSoundPlayerState.")]
        [NativeValueName("Default")]
        Default = 0,
        [NativeComment("Control specific sound effects are off, but focus sounds will play.")]
        [NativeValueName("FocusOnly")]
        FocusOnly = 1,
        [NativeComment("No sounds will play on interaction with this control.")]
        [NativeValueName("Off")]
        Off = 2,
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("ElementSpatialAudioMode")]
    [NativeComment("The state of the ElementSoundPlayer.")]
    public enum ElementSpatialAudioMode
    {
        [NativeComment("Use platform default for ElementSoundPlayer spatial audio setting.")]
        [NativeValueName("Auto")]
        Auto = 0,
        [NativeComment("Override the ElementSoundPlayer platform default: force spatial audio off.")]
        [NativeValueName("Off")]
        Off = 1,
        [NativeComment("Override the ElementSoundPlayer platform default: force spatial audio on.")]
        [NativeValueName("On")]
        On = 2,
    }

    [NativeName("LineStackingStrategy")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("Determines whether Lines used a fixed height, or the max height of their content.")]
    [DXamlIdlGroup("coretypes2")]
    public enum LineStackingStrategy
    {
        [NativeComment("MaxHeight specifies that the line height is the smallest value that contains all the inline elements on that line when those elements are properly aligned.")]
        [NativeValueName("MaxHeight")]
        MaxHeight = 0,
        [NativeComment("BlockLineHeight specifies that the stack height is determined by the block element LineHeight property value.")]
        [NativeValueName("BlockLineHeight")]
        BlockLineHeight = 1,
        [NativeComment("Leading specified as the distance between baselines. Does not affect the distance before the first baseline or after the last.")]
        [NativeValueName("BaselineToBaseline")]
        BaselineToBaseline = 2,
    }

    [NativeName("TextLineBounds")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("Determines the line layout metrics.")]
    [DXamlIdlGroup("coretypes2")]
    public enum TextLineBounds
    {
        [NativeComment("Default")]
        [NativeValueName("Full")]
        Full = 0,
        [NativeComment("Sets ascent to cap height ")]
        [NativeValueName("TrimToCapHeight")]
        TrimToCapHeight = 1,
        [NativeComment("Sets descent to baseline")]
        [NativeValueName("TrimToBaseline")]
        TrimToBaseline = 2,
        [NativeComment("Sets ascent to cap height and Descent to baseline")]
        [NativeValueName("Tight")]
        Tight = 3,
    }

    [NativeName("TextAlignment")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("Determines the text alignment.")]
    [DXamlIdlGroup("coretypes2")]
    public enum TextAlignment
    {
        [NativeValueName("TextAlignmentCenter")]
        Center = 0,
        [NativeValueName("TextAlignmentLeft")]
        Left = 1,
        [NativeValueName("TextAlignmentStart")]
        Start = 1,
        [NativeValueName("TextAlignmentRight")]
        Right = 2,
        [NativeValueName("TextAlignmentEnd")]
        End = 2,
        [NativeValueName("TextAlignmentJustify")]
        Justify = 3,
        [NativeValueName("TextAlignmentDetectFromContent")]
        DetectFromContent = 4,
    }

    [DXamlModifier(Modifier.Public)]
    [NativeName("FontCapitals")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [DXamlIdlGroup("coretypes2")]
    public enum FontCapitals
    {
        [NativeValueName("CapitalsNormal")]
        Normal = 0,
        [NativeValueName("CapitalsAllSmallCaps")]
        AllSmallCaps = 1,
        [NativeValueName("CapitalsSmallCaps")]
        SmallCaps = 2,
        [NativeValueName("CapitalsAllPetiteCaps")]
        AllPetiteCaps = 3,
        [NativeValueName("CapitalsPetiteCaps")]
        PetiteCaps = 4,
        [NativeValueName("CapitalsUnicase")]
        Unicase = 5,
        [NativeValueName("CapitalsTitling")]
        Titling = 6,
    }

    [DXamlModifier(Modifier.Public)]
    [NativeName("FontFraction")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [DXamlIdlGroup("coretypes2")]
    public enum FontFraction
    {
        [NativeValueName("FractionNormal")]
        Normal = 0,
        [NativeValueName("FractionStacked")]
        Stacked = 1,
        [NativeValueName("FractionSlashed")]
        Slashed = 2,
    }

    [DXamlModifier(Modifier.Public)]
    [NativeName("FontNumeralStyle")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [DXamlIdlGroup("coretypes2")]
    public enum FontNumeralStyle
    {
        [NativeValueName("NumeralStyleNormal")]
        Normal = 0,
        [NativeValueName("NumeralStyleLining")]
        Lining = 1,
        [NativeValueName("NumeralStyleOldStyle")]
        OldStyle = 2,
    }

    [DXamlModifier(Modifier.Public)]
    [NativeName("FontNumeralAlignment")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [DXamlIdlGroup("coretypes2")]
    public enum FontNumeralAlignment
    {
        [NativeValueName("NumeralAlignmentNormal")]
        Normal = 0,
        [NativeValueName("NumeralAlignmentProportional")]
        Proportional = 1,
        [NativeValueName("NumeralAlignmentTabular")]
        Tabular = 2,
    }

    [DXamlModifier(Modifier.Public)]
    [NativeName("FontVariants")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [DXamlIdlGroup("coretypes2")]
    public enum FontVariants
    {
        [NativeValueName("VariantsNormal")]
        Normal = 0,
        [NativeValueName("VariantsSuperscript")]
        Superscript = 1,
        [NativeValueName("VariantsSubscript")]
        Subscript = 2,
        [NativeValueName("VariantsOrdinal")]
        Ordinal = 3,
        [NativeValueName("VariantsInferior")]
        Inferior = 4,
        [NativeValueName("VariantsRuby")]
        Ruby = 5,
    }

    [DXamlModifier(Modifier.Public)]
    [NativeName("FontEastAsianLanguage")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [DXamlIdlGroup("coretypes2")]
    public enum FontEastAsianLanguage
    {
        [NativeValueName("EastAsianLanguageNormal")]
        Normal = 0,
        [NativeValueName("EastAsianLanguageHojoKanji")]
        HojoKanji = 1,
        [NativeValueName("EastAsianLanguageJis04")]
        Jis04 = 2,
        [NativeValueName("EastAsianLanguageJis78")]
        Jis78 = 3,
        [NativeValueName("EastAsianLanguageJis83")]
        Jis83 = 4,
        [NativeValueName("EastAsianLanguageJis90")]
        Jis90 = 5,
        [NativeValueName("EastAsianLanguageNlcKanji")]
        NlcKanji = 6,
        [NativeValueName("EastAsianLanguageSimplified")]
        Simplified = 7,
        [NativeValueName("EastAsianLanguageTraditional")]
        Traditional = 8,
        [NativeValueName("EastAsianLanguageTraditionalNames")]
        TraditionalNames = 9,
    }

    [DXamlModifier(Modifier.Public)]
    [NativeName("FontEastAsianWidths")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [DXamlIdlGroup("coretypes2")]
    public enum FontEastAsianWidths
    {
        [NativeValueName("EastAsianWidthsNormal")]
        Normal = 0,
        [NativeValueName("EastAsianWidthsFull")]
        Full = 1,
        [NativeValueName("EastAsianWidthsHalf")]
        Half = 2,
        [NativeValueName("EastAsianWidthsProportional")]
        Proportional = 3,
        [NativeValueName("EastAsianWidthsQuarter")]
        Quarter = 4,
        [NativeValueName("EastAsianWidthsThird")]
        Third = 5,
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("ErrorType")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [EnumFlags(IsNativeTypeDef = true, IsTypeConverter = true)]
    public enum ErrorType
    {
        [NativeValueName("NoError")]
        NoError = 0,
        [NativeValueName("UnknownError")]
        UnknownError = 1,
        [NativeValueName("InitializeError")]
        InitializeError = 2,
        [NativeValueName("ParserError")]
        ParserError = 3,
        [NativeValueName("ObjectModelError")]
        ObjectModelError = 4,
        [NativeValueName("RuntimeError")]
        RuntimeError = 5,
        [NativeValueName("DownloadError")]
        DownloadError = 6,
        [NativeValueName("MediaError")]
        MediaError = 7,
        [NativeValueName("ImageError")]
        ImageError = 8,
        [NativeValueName("ManagedError")]
        ManagedRuntimeError = 9,
    }

    [NativeName("HorizontalAlignment")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum HorizontalAlignment
    {
        [NativeValueName("HorizontalAlignmentLeft")]
        Left = 0,
        [NativeValueName("HorizontalAlignmentCenter")]
        Center = 1,
        [NativeValueName("HorizontalAlignmentRight")]
        Right = 2,
        [NativeValueName("HorizontalAlignmentStretch")]
        Stretch = 3,
    }

    [NativeName("VerticalAlignment")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum VerticalAlignment
    {
        [NativeValueName("VerticalAlignmentTop")]
        Top = 0,
        [NativeValueName("VerticalAlignmentCenter")]
        Center = 1,
        [NativeValueName("VerticalAlignmentBottom")]
        Bottom = 2,
        [NativeValueName("VerticalAlignmentStretch")]
        Stretch = 3,
    }

    [NativeName("ApplicationTheme")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [DXamlIdlGroup("coretypes2")]
    public enum ApplicationTheme
    {
        [NativeValueName("ApplicationThemeLight")]
        Light = 0,
        [NativeValueName("ApplicationThemeDark")]
        Dark = 1,
    }

    [Comment("Determines how an application handles the RequiresPointer property on controls.")]
    [FrameworkTypePattern]
    [DXamlIdlGroup("coretypes2")]
    [NativeName("ApplicationRequiresPointerMode")]
    public enum ApplicationRequiresPointerMode
    {
        Auto = 0,
        WhenRequested = 1,
    }

    [NativeName("ElementTheme")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum ElementTheme
    {
        [NativeValueName("ElementThemeDefault")]
        Default = 0,
        [NativeValueName("ElementThemeLight")]
        Light = 1,
        [NativeValueName("ElementThemeDark")]
        Dark = 2,
    }

    [NativeName("GridUnitType")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [DXamlIdlGroup("coretypes2")]
    public enum GridUnitType
    {
        [NativeComment("The value indicates that content should be calculated without constraints.")]
        [NativeValueName("Auto")]
        Auto = 0,
        [NativeComment("The value is expressed as a pixel.")]
        [NativeValueName("Pixel")]
        Pixel = 1,
        [NativeComment("The value is expressed as a weighted proportion of available space.")]
        [NativeValueName("Star")]
        Star = 2,
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("APAutomationProperties")]
    [NativeCategory(EnumCategory.AutomationEnum)]
    [EnumFlags(IsNativeTypeDef = true, IsTypeConverter = true, NativeUsesNumericValues = false)]
    [NativeValueNamespace("UIAXcp")]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    [DXamlIdlGroup("coretypes2")]
    internal enum AutomationPropertiesEnum
    {
        [NativeValueName("APAcceleratorKeyProperty")]
        AcceleratorKeyProperty,
        [NativeValueName("APAccessKeyProperty")]
        AccessKeyProperty,
        [NativeValueName("APAutomationControlTypeProperty")]
        ControlTypeProperty,
        [NativeValueName("APAutomationIdProperty")]
        AutomationIdProperty,
        [NativeValueName("APBoundingRectangleProperty")]
        BoundingRectangleProperty,
        [NativeValueName("APClassNameProperty")]
        ClassNameProperty,
        [NativeValueName("APClickablePointProperty")]
        ClickablePointProperty,
        [NativeValueName("APHelpTextProperty")]
        HelpTextProperty,
        [NativeValueName("APItemStatusProperty")]
        ItemStatusProperty,
        [NativeValueName("APItemTypeProperty")]
        ItemTypeProperty,
        [NativeValueName("APLabeledByProperty")]
        LabeledByProperty,
        [NativeValueName("APLocalizedControlTypeProperty")]
        LocalizedControlTypeProperty,
        [NativeValueName("APNameProperty")]
        NameProperty,
        [NativeValueName("APOrientationProperty")]
        OrientationProperty,
        [NativeValueName("APHasKeyboardFocusProperty")]
        HasKeyboardFocusProperty,
        [NativeValueName("APIsContentElementProperty")]
        IsContentElementProperty,
        [NativeValueName("APIsControlElementProperty")]
        IsControlElementProperty,
        [NativeValueName("APIsEnabledProperty")]
        IsEnabledProperty,
        [NativeValueName("APIsKeyboardFocusableProperty")]
        IsKeyboardFocusableProperty,
        [NativeValueName("APIsOffscreenProperty")]
        IsOffscreenProperty,
        [NativeValueName("APIsPasswordProperty")]
        IsPasswordProperty,
        [NativeValueName("APIsRequiredForFormProperty")]
        IsRequiredForFormProperty,
        [NativeValueName("APStructureChangeType_ChildrenBulkRemovedProperty")]
        StructureChangeType_ChildrenBulkRemovedProperty,
        [NativeValueName("APStructureChangeType_ChildrenBulkAddedProperty")]
        StructureChangeType_ChildrenBulkAddedProperty,
        [NativeValueName("APStructureChangeType_ChildrenInvalidatedProperty")]
        StructureChangeType_ChildrenInvalidatedProperty,
        [NativeValueName("APStructureChangeType_ChildAddedProperty")]
        StructureChangeType_ChildAddedProperty,
        [NativeValueName("APStructureChangeType_ChildRemovedProperty")]
        StructureChangeType_ChildRemovedProperty,
        [NativeValueName("APDockPositionProperty")]
        DockPositionProperty,
        [NativeValueName("APExpandCollapseStateProperty")]
        ExpandCollapseStateProperty,
        [NativeValueName("APColumnProperty")]
        ColumnProperty,
        [NativeValueName("APColumnSpanProperty")]
        ColumnSpanProperty,
        [NativeValueName("APContainingGridProperty")]
        ContainingGridProperty,
        [NativeValueName("APRowProperty")]
        RowProperty,
        [NativeValueName("APRowSpanProperty")]
        RowSpanProperty,
        [NativeValueName("APColumnCountProperty")]
        ColumnCountProperty,
        [NativeValueName("APRowCountProperty")]
        RowCountProperty,
        [NativeValueName("APCurrentViewProperty")]
        CurrentViewProperty,
        [NativeValueName("APSupportedViewsProperty")]
        SupportedViewsProperty,
        [NativeValueName("APRangeValueIsReadOnlyProperty")]
        RangeValueIsReadOnlyProperty,
        [NativeValueName("APLargeChangeProperty")]
        LargeChangeProperty,
        [NativeValueName("APMaximumProperty")]
        MaximumProperty,
        [NativeValueName("APMinimumProperty")]
        MinimumProperty,
        [NativeValueName("APSmallChangeProperty")]
        SmallChangeProperty,
        [NativeValueName("APRangeValueValueProperty")]
        RangeValueValueProperty,
        [NativeValueName("APHorizontallyScrollableProperty")]
        HorizontallyScrollableProperty,
        [NativeValueName("APHorizontalScrollPercentProperty")]
        HorizontalScrollPercentProperty,
        [NativeValueName("APHorizontalViewSizeProperty")]
        HorizontalViewSizeProperty,
        [NativeValueName("APVerticallyScrollableProperty")]
        VerticallyScrollableProperty,
        [NativeValueName("APVerticalScrollPercentProperty")]
        VerticalScrollPercentProperty,
        [NativeValueName("APVerticalViewSizeProperty")]
        VerticalViewSizeProperty,
        [NativeValueName("APIsSelectedProperty")]
        IsSelectedProperty,
        [NativeValueName("APSelectionContainerProperty")]
        SelectionContainerProperty,
        [NativeValueName("APCanSelectMultipleProperty")]
        CanSelectMultipleProperty,
        [NativeValueName("APIsSelectionRequiredProperty")]
        IsSelectionRequiredProperty,
        [NativeValueName("APSelectionProperty")]
        SelectionProperty,
        [NativeValueName("APColumnHeaderItemsProperty")]
        ColumnHeaderItemsProperty,
        [NativeValueName("APRowHeaderItemsProperty")]
        RowHeaderItemsProperty,
        [NativeValueName("APColumnHeadersProperty")]
        ColumnHeadersProperty,
        [NativeValueName("APRowHeadersProperty")]
        RowHeadersProperty,
        [NativeValueName("APRowOrColumnMajorProperty")]
        RowOrColumnMajorProperty,
        [NativeValueName("APToggleStateProperty")]
        ToggleStateProperty,
        [NativeValueName("APCanMoveProperty")]
        CanMoveProperty,
        [NativeValueName("APCanResizeProperty")]
        CanResizeProperty,
        [NativeValueName("APCanRotateProperty")]
        CanRotateProperty,
        [NativeValueName("APValueIsReadOnlyProperty")]
        ValueIsReadOnlyProperty,
        [NativeValueName("APValueValueProperty")]
        ValueValueProperty,
        [NativeValueName("APCanMaximizeProperty")]
        CanMaximizeProperty,
        [NativeValueName("APCanMinimizeProperty")]
        CanMinimizeProperty,
        [NativeValueName("APIsModalProperty")]
        IsModalProperty,
        [NativeValueName("APIsTopmostProperty")]
        IsTopmostProperty,
        [NativeValueName("APWindowInteractionStateProperty")]
        WindowInteractionStateProperty,
        [NativeValueName("APWindowVisualStateProperty")]
        WindowVisualStateProperty,
        [NativeValueName("APParentProperty")]
        ParentProperty,
        [NativeValueName("APAnnotationTypeIdProperty")]
        AnnotationTypeIdProperty,
        [NativeValueName("APAnnotationTypeNameProperty")]
        AnnotationTypeNameProperty,
        [NativeValueName("APAuthorProperty")]
        AuthorProperty,
        [NativeValueName("APDateTimeProperty")]
        DateTimeProperty,
        [NativeValueName("APTargetProperty")]
        TargetProperty,
        [NativeValueName("APDropEffectProperty")]
        DropEffectProperty,
        [NativeValueName("APDropEffectsProperty")]
        DropEffectsProperty,
        [NativeValueName("APGrabbedItemsProperty")]
        GrabbedItemsProperty,
        [NativeValueName("APIsGrabbedProperty")]
        IsGrabbedProperty,
        [NativeValueName("APDropTargetEffectProperty")]
        DropTargetEffectProperty,
        [NativeValueName("APDropTargetEffectsProperty")]
        DropTargetEffectsProperty,
        [NativeValueName("APLiveSettingProperty")]
        LiveSettingProperty,
        [NativeValueName("APCanZoomProperty")]
        CanZoomProperty,
        [NativeValueName("APZoomLevelProperty")]
        ZoomLevelProperty,
        [NativeValueName("APMaxZoomProperty")]
        MaxZoomProperty,
        [NativeValueName("APMinZoomProperty")]
        MinZoomProperty,
        [NativeValueName("APFormulaProperty")]
        FormulaProperty,
        [NativeValueName("APExtendedPropertiesProperty")]
        ExtendedPropertiesProperty,
        [NativeValueName("APFillColorProperty")]
        FillColorProperty,
        [NativeValueName("APFillPatternColorProperty")]
        FillPatternColorProperty,
        [NativeValueName("APFillPatternStyleProperty")]
        FillPatternStyleProperty,
        [NativeValueName("APShapeProperty")]
        ShapeProperty,
        [NativeValueName("APStyleIdProperty")]
        StyleIdProperty,
        [NativeValueName("APStyleNameProperty")]
        StyleNameProperty,
        [NativeValueName("APAccessibilityViewProperty")]
        AccessibilityViewProperty,
        [NativeValueName("APControlledPeersProperty")]
        ControlledPeersProperty,
        [NativeValueName("APFlowsFromProperty")]
        FlowsFromProperty,
        [NativeValueName("APFlowsToProperty")]
        FlowsToProperty,
        [NativeValueName("APStructureChangeType_ChildernReorderedProperty")]
        StructureChangeType_ChildernReorderedProperty,
        [NativeValueName("APPositionInSetProperty")]
        PositionInSetProperty,
        [NativeValueName("APSizeOfSetProperty")]
        SizeOfSetProperty,
        [NativeValueName("APLevelProperty")]
        LevelProperty,
        [NativeValueName("APAnnotationsProperty")]
        AnnotationsProperty,
        [NativeValueName("APLandmarkTypeProperty")]
        LandmarkTypeProperty,
        [NativeValueName("APLocalizedLandmarkTypeProperty")]
        LocalizedLandmarkTypeProperty,
        [NativeValueName("APEmptyProperty")]
        EmptyProperty,
        [NativeValueName("APIsPeripheralProperty")]
        IsPeripheralProperty,
        [NativeValueName("APIsDataValidForFormProperty")]
        IsDataValidForFormProperty,
        [NativeValueName("APFullDescriptionProperty")]
        FullDescriptionProperty,
        [NativeValueName("APDescribedByProperty")]
        DescribedByProperty,
        [NativeValueName("APCultureProperty")]
        CultureProperty,
        [NativeValueName("APHeadingLevelProperty")]
        HeadingLevelProperty,
        [NativeValueName("APIsDialogProperty")]
        IsDialogProperty,
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("ePropertyInvalidationReason")]
    [EnumFlags(IsTypeConverter = true)]
    [NativeComment("Different operations due to which a property can change value.")]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    internal enum PropertyInvalidationReason
    {
        [NativeValueName("irSetValue")]
        SetValue = 0,
        [NativeOnly]
        [NativeValueName("irSetValueOnAncestor")]
        SetValueOnAncestor = 1,
        [NativeValueName("irClearValue")]
        ClearValue = 2,
        [NativeOnly]
        [NativeValueName("irEnterTree")]
        EnterTree = 3,
        [NativeOnly]
        [NativeValueName("irLeaveTree")]
        LeaveTree = 4,
        [NativeValueName("irApplyStyle")]
        ApplyStyle = 5,
        [NativeValueName("irUnApplyStyle")]
        UnApplyStyle = 6,
    }

    [NativeName("eFlowDirection")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("FrameworkElement.FlowDirection property")]
    public enum FlowDirection
    {
        [NativeValueName("LeftToRight")]
        LeftToRight = 0,
        [NativeValueName("RightToLeft")]
        RightToLeft = 1,
    }

    [DXamlIdlGroup("coretypes2")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("ATAAutomationTextAttributes")]
    [EnumFlags(IsNativeTypeDef = true, IsTypeConverter = true)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public enum AutomationTextAttributesEnum
    {
        [NativeValueName("ATAAnimationStyleAttribute")]
        AnimationStyleAttribute = 40000,
        [NativeValueName("ATABackgroundColorAttribute")]
        BackgroundColorAttribute = 40001,
        [NativeValueName("ATABulletStyleAttribute")]
        BulletStyleAttribute = 40002,
        [NativeValueName("ATACapStyleAttribute")]
        CapStyleAttribute = 40003,
        [NativeValueName("ATACultureAttribute")]
        CultureAttribute = 40004,
        [NativeValueName("ATAFontNameAttribute")]
        FontNameAttribute = 40005,
        [NativeValueName("ATAFontSizeAttribute")]
        FontSizeAttribute = 40006,
        [NativeValueName("ATAFontWeightAttribute")]
        FontWeightAttribute = 40007,
        [NativeValueName("ATAForegroundColorAttribute")]
        ForegroundColorAttribute = 40008,
        [NativeValueName("ATAHorizontalTextAlignmentAttribute")]
        HorizontalTextAlignmentAttribute = 40009,
        [NativeValueName("ATAIndentationFirstLineAttribute")]
        IndentationFirstLineAttribute = 40010,
        [NativeValueName("ATAIndentationLeadingAttribute")]
        IndentationLeadingAttribute = 40011,
        [NativeValueName("ATAIndentationTrailingAttribute")]
        IndentationTrailingAttribute = 40012,
        [NativeValueName("ATAIsHiddenAttribute")]
        IsHiddenAttribute = 40013,
        [NativeValueName("ATAIsItalicAttribute")]
        IsItalicAttribute = 40014,
        [NativeValueName("ATAIsReadOnlyAttribute")]
        IsReadOnlyAttribute = 40015,
        [NativeValueName("ATAIsSubscriptAttribute")]
        IsSubscriptAttribute = 40016,
        [NativeValueName("ATAIsSuperscriptAttribute")]
        IsSuperscriptAttribute = 40017,
        [NativeValueName("ATAMarginBottomAttribute")]
        MarginBottomAttribute = 40018,
        [NativeValueName("ATAMarginLeadingAttribute")]
        MarginLeadingAttribute = 40019,
        [NativeValueName("ATAMarginTopAttribute")]
        MarginTopAttribute = 40020,
        [NativeValueName("ATAMarginTrailingAttribute")]
        MarginTrailingAttribute = 40021,
        [NativeValueName("ATAOutlineStylesAttribute")]
        OutlineStylesAttribute = 40022,
        [NativeValueName("ATAOverlineColorAttribute")]
        OverlineColorAttribute = 40023,
        [NativeValueName("ATAOverlineStyleAttribute")]
        OverlineStyleAttribute = 40024,
        [NativeValueName("ATAStrikethroughColorAttribute")]
        StrikethroughColorAttribute = 40025,
        [NativeValueName("ATAStrikethroughStyleAttribute")]
        StrikethroughStyleAttribute = 40026,
        [NativeValueName("ATATabsAttribute")]
        TabsAttribute = 40027,
        [NativeValueName("ATATextFlowDirectionsAttribute")]
        TextFlowDirectionsAttribute = 40028,
        [NativeValueName("ATAUnderlineColorAttribute")]
        UnderlineColorAttribute = 40029,
        [NativeValueName("ATAUnderlineStyleAttribute")]
        UnderlineStyleAttribute = 40030,
        [NativeValueName("ATAAnnotationTypesAttribute")]
        AnnotationTypesAttribute = 40031,
        [NativeValueName("ATAAnnotationObjectsAttribute")]
        AnnotationObjectsAttribute = 40032,
        [NativeValueName("ATAStyleNameAttribute")]
        StyleNameAttribute = 40033,
        [NativeValueName("ATAStyleIdAttribute")]
        StyleIdAttribute = 40034,
        [NativeValueName("ATALinkAttribute")]
        LinkAttribute = 40035,
        [NativeValueName("ATAIsActiveAttribute")]
        IsActiveAttribute = 40036,
        [NativeValueName("ATASelectionActiveEndAttribute")]
        SelectionActiveEndAttribute = 40037,
        [NativeValueName("ATACaretPositionAttribute")]
        CaretPositionAttribute = 40038,
        [NativeValueName("ATACaretBidiModeAttribute")]
        CaretBidiModeAttribute = 40039,
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("MarkupExtensionType")]
    internal enum MarkupExtensionType
    {
        [NativeValueName("MarkupExtensionTypeNone")]
        None = 0,
        [NativeValueName("MarkupExtensionTypeExtension")]
        Extension = 1,
        [NativeValueName("MarkupExtensionTypeBinding")]
        Binding = 2,
    }

    [NativeName("FocusState")]
    public enum FocusState
    {
        [NativeValueName("FocusStateUnfocused")]
        Unfocused = 0,
        [NativeValueName("FocusStatePointer")]
        Pointer = 1,
        [NativeValueName("FocusStateKeyboard")]
        Keyboard = 2,
        [NativeValueName("FocusStateProgrammatic")]
        Programmatic = 3,
    }

    public enum FocusVisualKind
    {
        DottedLine,
        HighVisibility,
        Reveal
    }

    [NativeName("ElementHighContrastAdjustment")]
    [EnumFlags(AreValuesFlags = true, HasTypeConverter = true, GenerateConsecutiveEnum = true)]
    public enum ElementHighContrastAdjustment
    {
        [NativeValueName("ElementHighContrastAdjustmentNone")]
        None = 0,
        [NativeValueName("ElementHighContrastAdjustmentApplication")]
        Application = unchecked((int)(0x80000000)),
        [NativeValueName("ElementHighContrastAdjustmentAuto")]
        Auto = -1,
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("ApplicationHighContrastAdjustment")]
    [EnumFlags(AreValuesFlags = true, HasTypeConverter = true, GenerateConsecutiveEnum = true)]
    public enum ApplicationHighContrastAdjustment
    {
        [NativeValueName("ApplicationHighContrastAdjustmentNone")]
        None = 0,
        [NativeValueName("ApplicationHighContrastAdjustmentAuto")]
        Auto = -1,
    }

    public delegate void DragEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.DragEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void DragStartingEventHandler(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.DragStartingEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void DropCompletedEventHandler(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.DropCompletedEventArgs e);

    public delegate void RoutedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.RoutedEventArgs e);

    [CodeGen(CodeGenLevel.LookupOnly)]
    [ReturnTypeParameterName("value")]
    public delegate Windows.Foundation.Object CreateDefaultValueCallback();

    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void PropertyChangedCallback(DependencyObject d, Microsoft.UI.Xaml.DependencyPropertyChangedEventArgs e);

    public delegate void DependencyPropertyChangedCallback(DependencyObject sender, DependencyProperty dp);

    public delegate void DependencyPropertyChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.DependencyPropertyChangedEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void DataContextChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.DataContextChangedEventArgs e);

    public delegate void ExceptionRoutedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.ExceptionRoutedEventArgs e);

    [CodeGen(CodeGenLevel.CoreOnly)]
    public delegate void StartupEventHandler(Windows.Foundation.Object sender, Windows.Foundation.Object e);

    public delegate void SizeChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.SizeChangedEventArgs e);

    public delegate void VisualStateChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.VisualStateChangedEventArgs e);

    public delegate void UnhandledExceptionEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.UnhandledExceptionEventArgs e);

    public delegate void SuspendingEventHandler(Windows.Foundation.Object sender, Windows.ApplicationModel.SuspendingEventArgs e);

    public delegate void EnteredBackgroundEventHandler(Windows.Foundation.Object sender, Windows.ApplicationModel.EnteredBackgroundEventArgs e);

    public delegate void LeavingBackgroundEventHandler(Windows.Foundation.Object sender, Windows.ApplicationModel.LeavingBackgroundEventArgs e);

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [NativeName("CDispatcherTimer")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.Media.Animation.Timeline))]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [Guids(ClassGuid = "3ca35ce3-3c1f-4765-b605-dfc8ce330b19")]
    public class DispatcherTimer
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pInterval")]
        public Windows.Foundation.TimeSpan Interval
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Boolean IsEnabled
        {
            get;
            private set;
        }

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.EventHandler Tick;

        public DispatcherTimer() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Start()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Stop()
        {
        }
    }

    [StrictType]
    [CodeGen(partial: true)]
    [VelocityFeature("Feature_Xaml2018")]
    [Guids(ClassGuid = "0c22973c-b574-4edc-b0c8-ffe0508ba922")]
    public abstract class InteractionBase
    {
        public Windows.Foundation.Collections.IVectorView<Microsoft.UI.Xaml.RoutedEvent> GetSupportedEvents() { return null; }

        protected virtual Windows.Foundation.Collections.IVectorView<Microsoft.UI.Xaml.RoutedEvent> GetSupportedEventsCore() { return null; }

        protected virtual void OnKeyDown(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.KeyRoutedEventArgs args) { }

        protected virtual void OnKeyUp(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.KeyRoutedEventArgs args) { }

        protected virtual void OnPointerEntered(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs args) { }

        protected virtual void OnPointerExited(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs args) { }

        protected virtual void OnPointerMoved(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs args) { }

        protected virtual void OnPointerPressed(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs args) { }

        protected virtual void OnPointerReleased(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs args) { }

        protected virtual void OnPointerCaptureLost(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs args) { }

        protected virtual void OnPointerCanceled(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs args) { }

        protected virtual void OnPointerWheelChanged(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs args) { }

        protected virtual void OnTapped(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.TappedRoutedEventArgs args) { }

        protected virtual void OnDoubleTapped(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.DoubleTappedRoutedEventArgs args) { }

        protected virtual void OnHolding(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.HoldingRoutedEventArgs args) { }

        protected virtual void OnRightTapped(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.Input.RightTappedRoutedEventArgs args) { }

        protected virtual void OnDragEnter(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.DragEventArgs args) { }

        protected virtual void OnDragLeave(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.DragEventArgs args) { }

        protected virtual void OnDragOver(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.DragEventArgs args) { }

        protected virtual void OnDrop(Microsoft.UI.Xaml.UIElement sender, Microsoft.UI.Xaml.DragEventArgs args) { }

        protected InteractionBase() { }
    }

    [NativeName("CUIElementWeakCollection")]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "6cac2469-5f7f-43ec-836d-fe8bb0dd0391")]
    [ClassFlags(HasTypeConverter = true, HasBaseTypeInDXamlInterface = false)]
    public class UIElementWeakCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<UIElement>
    {
        public UIElementWeakCollection() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [ClassFlags(IsVisibleInXAML = false, RequiresCoreServices = false)]
    [Guids(ClassGuid = "c0603609-426b-457d-ab5a-b0e45fa98137")]
    public sealed class XamlRootChangedEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        internal XamlRootChangedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "eaad7a20-751b-4a85-b6c9-50231742b28f")]
    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 5)]
    public sealed class XamlRoot
    {
        private XamlRoot() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.UIElement Content { get; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Size Size { get; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public double RasterizationScale { get; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public bool IsHostVisible { get; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Internal)]
        internal bool IsInputActive { get; }

        public event Windows.Foundation.TypedEventHandler<XamlRoot, XamlRootChangedEventArgs> Changed;

        // For this internal event, we need to use a typed event handler that
        // exists for a public event, due to errors from the idl compiler about
        // the interface not being specialized otherwise. As such, we use the
        // same event handler as the above Changed event.
        [EventFlags(UseEventManager = true)]
        internal event Windows.Foundation.TypedEventHandler<XamlRoot, XamlRootChangedEventArgs> InputActivationChanged;

        [Version(2)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Content.ContentIslandEnvironment ContentIslandEnvironment { get; }
    }

    [TypeTable(ForceInclude = true)]
    [DXamlIdlGroup("coretypes2")]
    public interface IXamlServiceProvider
    {
        Windows.Foundation.Object GetService(Windows.UI.Xaml.Interop.TypeName type);
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), Microsoft.UI.Xaml.WinUIContract.LatestVersion)]
    [VelocityFeature("Feature_ExperimentalApi")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "0eb21081-a1b7-4942-925d-23cf4bafd8e1")]
    [ThreadingModel(ThreadingModel.Both)]
    [Implements(typeof(Windows.Foundation.IClosable))]
    [Implements(typeof(Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop), Version = 1)]
    public class XamlIsland
    {
        public XamlIsland()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromDXaml = true)]
        public UIElement Content
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromDXaml = true)]
        public Microsoft.UI.Content.ContentIsland ContentIsland { get; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [RequiresMultipleAssociationCheck]
        public Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop
        {
            get;
            set;
        }
    }

}
