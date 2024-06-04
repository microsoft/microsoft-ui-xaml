// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Media.Animation
{
    [AttributeUsage(AttributeTargets.Property)]
    [IdlAttributeTarget(AttributeTargets.Property)]
    public class IndependentlyAnimatableAttribute : Attribute
    {
    }

    [AttributeUsage(AttributeTargets.Property)]
    [IdlAttributeTarget(AttributeTargets.Property)]
    public class ConditionallyIndependentlyAnimatableAttribute : Attribute
    {
    }

    [CodeGen(partial: true)]
    [BuiltinStruct("CRepeatBehavior")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.Duration))]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "c17e7e07-d381-4d72-bb39-af4ffc113ff8")]
    public struct RepeatBehavior
    {
        [TypeTable(IsExcludedFromCore = true)]

        [NativeStorageType(OM.ValueType.valueFloat)]
        public Windows.Foundation.Double Count
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [ReadOnly]
        public Windows.Foundation.TimeSpan Duration
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromCore = true, IsExcludedFromNewTypeTable = true)]
        public Microsoft.UI.Xaml.Media.Animation.RepeatBehaviorType Type
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.Media.Animation.RepeatBehavior Forever
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyFlags(IsHelper = true)]
        [ReadOnly]
        public Windows.Foundation.Boolean HasCount
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyFlags(IsHelper = true)]
        [ReadOnly]
        public Windows.Foundation.Boolean HasDuration
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Boolean Equals(Microsoft.UI.Xaml.Media.Animation.RepeatBehavior value)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Media.Animation.RepeatBehavior FromCount(Windows.Foundation.Double count)
        {
            return default(Microsoft.UI.Xaml.Media.Animation.RepeatBehavior);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Media.Animation.RepeatBehavior FromDuration(Windows.Foundation.TimeSpan duration)
        {
            return default(Microsoft.UI.Xaml.Media.Animation.RepeatBehavior);
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CTimeline")]
    [Guids(ClassGuid = "9947b284-352a-4dc9-9358-7796d1e96ee1")]
    public abstract class Timeline
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_fAutoReverse")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetForceDCompAnimationSubtreeDirty")]
        public Windows.Foundation.Boolean AutoReverse
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pBeginTime")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetForceDCompAnimationSubtreeDirty")]
        public Windows.Foundation.TimeSpan? BeginTime
        {
            get;
            set;
        }

        [OffsetFieldName("m_duration")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetForceDCompAnimationSubtreeDirty")]
        [NativeStorageType(OM.ValueType.valueVO)]
        [PropertyFlags(DoNotEnterOrLeaveValue = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Duration Duration
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_rSpeedRatio")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetForceDCompAnimationSubtreeDirty")]
        public Windows.Foundation.Double SpeedRatio
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_fillBehavior")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetForceDCompAnimationSubtreeDirty")]
        public Microsoft.UI.Xaml.Media.Animation.FillBehavior FillBehavior
        {
            get;
            set;
        }

        [OffsetFieldName("m_repeatBehavior")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetForceDCompAnimationSubtreeDirty")]
        [NativeStorageType(OM.ValueType.valueVO)]
        [PropertyFlags(DoNotEnterOrLeaveValue = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Media.Animation.RepeatBehavior RepeatBehavior
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public static Windows.Foundation.Boolean AllowDependentAnimations
        {
            get;
            set;
        }

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.EventHandler Completed;

        protected Timeline() { }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        internal virtual void CreateTimelines(Windows.Foundation.Boolean onlyGenerateSteadyState, Microsoft.UI.Xaml.Media.Animation.TimelineCollection timelineCollection)
        {
        }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CParallelTimeline")]
    [Guids(ClassGuid = "670d45f8-2e1e-4d22-b0ef-f016fb0e277a")]
    public sealed class ParallelTimeline
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        internal ParallelTimeline() { }
    }

    [AllowsMultipleAssociations]
    [CodeGen(partial: true)]
    [NativeName("CEasingFunctionBase")]
    [Guids(ClassGuid = "e4ec7ec1-6d51-4e9b-aa09-2c14a0eff318")]
    public abstract class EasingFunctionBase
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_eEasingMode")]
        public Microsoft.UI.Xaml.Media.Animation.EasingMode EasingMode
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.Excluded)]
        internal EasingFunctionBase() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Double Ease(Windows.Foundation.Double normalizedTime)
        {
            return default(Windows.Foundation.Double);
        }
    }

    [NativeName("CCircInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "b21b9177-4ac9-434d-9b8b-368375c1887d")]
    public class CircleEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        public CircleEase() { }
    }

    [NativeName("CBackInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "75780e85-dd59-4b56-ad01-78a18ec799e1")]
    public class BackEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_fAmplitude")]
        public Windows.Foundation.Double Amplitude
        {
            get;
            set;
        }

        public BackEase() { }
    }

    [NativeName("CExponentialInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "8e2eecff-9a0b-466e-9b08-e6802e47c923")]
    public class ExponentialEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_fExponent")]
        public Windows.Foundation.Double Exponent
        {
            get;
            set;
        }

        public ExponentialEase() { }
    }

    [NativeName("CPowerInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "9f361baa-8c67-4017-b39b-42c5bab6b427")]
    public class PowerEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_fPower")]
        public Windows.Foundation.Double Power
        {
            get;
            set;
        }

        public PowerEase() { }
    }

    [NativeName("CQuadraticInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "b602bfe3-7817-4456-98a0-2249c78905d3")]
    public class QuadraticEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        public QuadraticEase() { }
    }

    [NativeName("CCubicInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "ea2c24e1-171b-4bac-a2b1-8e14d973f551")]
    public class CubicEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        public CubicEase() { }
    }

    [NativeName("CQuarticInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "7dea9b41-a81b-4239-9e14-3b6fdf35a9ab")]
    public class QuarticEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        public QuarticEase() { }
    }

    [NativeName("CQuinticInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "3c808bdf-9cf6-4192-a26d-0dd1dc4acb87")]
    public class QuinticEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        public QuinticEase() { }
    }

    [NativeName("CElasticInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "22569172-8236-42aa-aa3a-856d1a773ef8")]
    public class ElasticEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        [NativeStorageType(OM.ValueType.valueSigned)]
        [OffsetFieldName("m_iOscillations")]
        public Windows.Foundation.Int32 Oscillations
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_fSpringiness")]
        public Windows.Foundation.Double Springiness
        {
            get;
            set;
        }

        public ElasticEase() { }
    }

    [NativeName("CBounceInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "71481f67-6266-48fa-875f-53b9a38455da")]
    public class BounceEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        [NativeStorageType(OM.ValueType.valueSigned)]
        [OffsetFieldName("m_iBounces")]
        public Windows.Foundation.Int32 Bounces
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_fBounciness")]
        public Windows.Foundation.Double Bounciness
        {
            get;
            set;
        }

        public BounceEase() { }
    }

    [NativeName("CSineInterpolator")]
    [DXamlComposability(Modifier.Private)]
    [Guids(ClassGuid = "18694f54-a460-44fd-945d-b99e5215abf3")]
    public class SineEase
     : Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase
    {
        public SineEase() { }
    }

    [NativeName("CBeginStoryboard")]
    [ContentProperty("Storyboard")]
    [Guids(ClassGuid = "16a69bed-ff9d-4bdb-92c6-51c5d24a7f5e")]
    public sealed class BeginStoryboard
     : Microsoft.UI.Xaml.TriggerAction
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(NeedsInvoke = true)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pStoryboard")]
        public Microsoft.UI.Xaml.Media.Animation.Storyboard Storyboard
        {
            get;
            set;
        }

        public BeginStoryboard() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CStoryboard")]
    [ContentProperty("Children")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.Media.Animation.ParallelTimeline))]
    [Guids(ClassGuid = "443444f4-43ab-4998-a25d-8c502de96de4")]
    public sealed class Storyboard
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pChild")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationInSubtreeDirty")]
        public Microsoft.UI.Xaml.Media.Animation.TimelineCollection Children
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "path", TargetType = typeof(Microsoft.UI.Xaml.Media.Animation.Timeline))]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [OffsetFieldName("m_strTargetProperty")]
        public static Windows.Foundation.String AttachedTargetProperty
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "name", TargetType = typeof(Microsoft.UI.Xaml.Media.Animation.Timeline))]
        [OffsetFieldName("m_strTargetName")]
        public static Windows.Foundation.String AttachedTargetName
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_fIsEssential")]
        internal Windows.Foundation.Boolean IsEssential
        {
            get;
            set;
        }

        public Storyboard() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeClassName("CStoryboard")]
        public void Seek(Windows.Foundation.TimeSpan offset)
        {
        }

        [PInvoke]
        public void Stop()
        {
        }

        [PInvoke]
        public void Begin()
        {
        }

        [PInvoke]
        public void Pause()
        {
        }

        [PInvoke]
        public void Resume()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeClassName("CStoryboard")]
        public static void SetTarget(Microsoft.UI.Xaml.Media.Animation.Timeline timeline, Microsoft.UI.Xaml.DependencyObject target)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeClassName("CStoryboard")]
        public Microsoft.UI.Xaml.Media.Animation.ClockState GetCurrentState()
        {
            return default(Microsoft.UI.Xaml.Media.Animation.ClockState);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeClassName("CStoryboard")]
        public Windows.Foundation.TimeSpan GetCurrentTime()
        {
            return default(Windows.Foundation.TimeSpan);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeClassName("CStoryboard")]
        public void SeekAlignedToLastTick(Windows.Foundation.TimeSpan offset)
        {
        }

        [PInvoke]
        public void SkipToFill()
        {
        }
    }

    [DXamlIdlGroup("coretypes2")]
    [Platform(typeof(PrivateApiContract), 1)]
    [AnimationPattern]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "653bc276-fbab-436f-9bb6-e5b7db78d2c5")]
    public abstract class ThemeAnimationBase
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public ThemeAnimationBase() { }

        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void CreateTimelinesInternal(Windows.Foundation.Boolean onlyGenerateSteadyState, Microsoft.UI.Xaml.Media.Animation.TimelineCollection timelineCollection)
        {
        }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "e3f586d1-7e6d-4fa5-92f6-df7377e50f8d")]
    public sealed class FadeInThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        [FieldBacked]
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public FadeInThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "203aca5d-feb5-4eeb-aea5-3102bd84797a")]
    public sealed class FadeOutThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        [FieldBacked]
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public FadeOutThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "be2ea91c-d75d-4f45-abcf-62448caf16ef")]
    public sealed class PopInThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public Windows.Foundation.Double FromHorizontalOffset
        {
            get;
            set;
        }

        public Windows.Foundation.Double FromVerticalOffset
        {
            get;
            set;
        }

        public PopInThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "00407cea-f74e-4dc4-876e-26f59a230b33")]
    public sealed class PopOutThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public PopOutThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "267cb268-33f5-4bd7-8356-401e7e6b6f73")]
    public sealed class RepositionThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public Windows.Foundation.Double FromHorizontalOffset
        {
            get;
            set;
        }

        public Windows.Foundation.Double FromVerticalOffset
        {
            get;
            set;
        }

        public RepositionThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "5daf5537-26ca-45c2-a5c7-48cb9d889519")]
    public sealed class PointerUpThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public PointerUpThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "d2adbff7-944d-4f89-9523-3b08acbc1201")]
    public sealed class PointerDownThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public PointerDownThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "2b3d1dda-2942-436b-888f-8d9818b268a0")]
    public sealed class SplitOpenThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String OpenedTargetName
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject OpenedTarget
        {
            get;
            set;
        }

        public Windows.Foundation.String ClosedTargetName
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject ClosedTarget
        {
            get;
            set;
        }

        public Windows.Foundation.String ContentTargetName
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject ContentTarget
        {
            get;
            set;
        }

        public Windows.Foundation.Double OpenedLength
        {
            get;
            set;
        }

        public Windows.Foundation.Double ClosedLength
        {
            get;
            set;
        }

        public Windows.Foundation.Double OffsetFromCenter
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.AnimationDirection ContentTranslationDirection
        {
            get;
            set;
        }

        public Windows.Foundation.Double ContentTranslationOffset
        {
            get;
            set;
        }

        public SplitOpenThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "fbc108e3-d1c2-430f-bba9-98975de0d107")]
    public sealed class SplitCloseThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String OpenedTargetName
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject OpenedTarget
        {
            get;
            set;
        }

        public Windows.Foundation.String ClosedTargetName
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject ClosedTarget
        {
            get;
            set;
        }

        public Windows.Foundation.String ContentTargetName
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject ContentTarget
        {
            get;
            set;
        }

        public Windows.Foundation.Double OpenedLength
        {
            get;
            set;
        }

        public Windows.Foundation.Double ClosedLength
        {
            get;
            set;
        }

        public Windows.Foundation.Double OffsetFromCenter
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.AnimationDirection ContentTranslationDirection
        {
            get;
            set;
        }

        public Windows.Foundation.Double ContentTranslationOffset
        {
            get;
            set;
        }

        public SplitCloseThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "544ec566-ac18-4685-a97a-b0ea784a7689")]
    public sealed class SwipeBackThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public Windows.Foundation.Double FromHorizontalOffset
        {
            get;
            set;
        }

        public Windows.Foundation.Double FromVerticalOffset
        {
            get;
            set;
        }

        public SwipeBackThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "405e9a79-781f-48f9-8328-3452e4812958")]
    public sealed class SwipeHintThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public Windows.Foundation.Double ToHorizontalOffset
        {
            get;
            set;
        }

        public Windows.Foundation.Double ToVerticalOffset
        {
            get;
            set;
        }

        public SwipeHintThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "36ad1f84-339e-4d09-9b97-92684c9bf5ac")]
    public sealed class DragItemThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public DragItemThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "95a363ad-8ebb-4640-bedc-6e98b9722119")]
    public sealed class DropTargetItemThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public DropTargetItemThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "6aa16ae1-08f6-439b-9189-42ad23e48c46")]
    public sealed class DragOverThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String TargetName
        {
            get;
            set;
        }

        public Windows.Foundation.Double ToOffset
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.AnimationDirection Direction
        {
            get;
            set;
        }

        public DragOverThemeAnimation() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CDynamicTimeline")]
    [ClassFlags(IsHiddenFromIdl = true)]
    [Guids(ClassGuid = "83328713-c2ae-4779-99f1-6cb37c20f2be")]
    public class DynamicTimeline
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        [CodeGen(CodeGenLevel.CoreOnly)]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pChild")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationInSubtreeDirty")]
        public Microsoft.UI.Xaml.Media.Animation.TimelineCollection Children
        {
            get;
            set;
        }

        internal DynamicTimeline() { }
    }

    [AllowsMultipleAssociations]
    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CTransitionTarget")]
    [Guids(ClassGuid = "f8eb3653-4480-428b-9885-0b08605e6af4")]
    internal sealed class TransitionTarget
     : Microsoft.UI.Xaml.DependencyObject
    {
        [PropertyFlags(IsIndependentlyAnimatable = true)]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_opacity")]
        [RenderDirtyFlagClassName("CTransitionTarget")]
        [RenderDirtyFlagMethodName("NWSetOpacityDirty")]
        public Windows.Foundation.Double Opacity
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

        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pClipTransform")]
        [RenderDirtyFlagClassName("CTransitionTarget")]
        [RenderDirtyFlagMethodName("NWSetClipDirty")]
        public Microsoft.UI.Xaml.Media.CompositeTransform ClipTransform
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pxf")]
        [RenderDirtyFlagClassName("CTransitionTarget")]
        [RenderDirtyFlagMethodName("NWSetTransformDirty")]
        public Microsoft.UI.Xaml.Media.CompositeTransform CompositeTransform
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valuePoint)]
        [OffsetFieldName("m_ptRenderTransformOrigin")]
        [RenderDirtyFlagClassName("CTransitionTarget")]
        [RenderDirtyFlagMethodName("NWSetTransformDirty")]
        public Windows.Foundation.Point TransformOrigin
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valuePoint)]
        [OffsetFieldName("m_ptClipTransformOrigin")]
        [RenderDirtyFlagClassName("CTransitionTarget")]
        [RenderDirtyFlagMethodName("NWSetClipDirty")]
        public Windows.Foundation.Point ClipTransformOrigin
        {
            get;
            set;
        }

        internal TransitionTarget() { }
    }

    [NativeName("CDoubleAnimation")]
    [Guids(ClassGuid = "6c8ded1a-c669-4c99-8c5f-6c4a0948f11a")]
    public sealed class DoubleAnimation
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_vFrom")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Windows.Foundation.Double? From
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_vTo")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Windows.Foundation.Double? To
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_vBy")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Windows.Foundation.Double? By
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pEasingFunction")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase EasingFunction
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_enableDependentAnimation")]
        public Windows.Foundation.Boolean EnableDependentAnimation
        {
            get;
            set;
        }

        public DoubleAnimation() { }
    }

    [NativeName("CColorAnimation")]
    [Guids(ClassGuid = "e879403b-a747-4cff-8bec-c1e3cbac4ba0")]
    public sealed class ColorAnimation
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        [NativeStorageType(OM.ValueType.valueColor)]
        [OffsetFieldName("m_vFrom")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Windows.UI.Color? From
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueColor)]
        [OffsetFieldName("m_vTo")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Windows.UI.Color? To
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueColor)]
        [OffsetFieldName("m_vBy")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Windows.UI.Color? By
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pEasingFunction")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase EasingFunction
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_enableDependentAnimation")]
        public Windows.Foundation.Boolean EnableDependentAnimation
        {
            get;
            set;
        }

        public ColorAnimation() { }
    }

    [NativeName("CPointAnimation")]
    [Guids(ClassGuid = "5714c188-0d03-458e-9c6c-51d51c26fb5f")]
    public sealed class PointAnimation
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        [NativeStorageType(OM.ValueType.valuePoint)]
        [OffsetFieldName("m_ptFrom")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Windows.Foundation.Point? From
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valuePoint)]
        [OffsetFieldName("m_ptTo")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Windows.Foundation.Point? To
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valuePoint)]
        [OffsetFieldName("m_ptBy")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Windows.Foundation.Point? By
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pEasingFunction")]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase EasingFunction
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_enableDependentAnimation")]
        public Windows.Foundation.Boolean EnableDependentAnimation
        {
            get;
            set;
        }

        public PointAnimation() { }
    }

    [BuiltinStruct("CKeyTime")]
    [CoreBaseType(typeof(Windows.Foundation.TimeSpan))]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "4a6f16fc-18cd-447d-8f17-1d2803a5ca86")]
    public struct KeyTime
    {
        [TypeTable(IsExcludedFromCore = true)]
        [ReadOnly]
        public Windows.Foundation.TimeSpan TimeSpan
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Media.Animation.KeyTime FromTimeSpan(Windows.Foundation.TimeSpan timeSpan)
        {
            return default(Microsoft.UI.Xaml.Media.Animation.KeyTime);
        }
    }

    [NativeName("CKeySpline")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "1403230b-bacb-4fc1-a85a-f624db3a9317")]
    public sealed class KeySpline
     : Microsoft.UI.Xaml.DependencyObject
    {
        [PropertyFlags(NeedsInvoke = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valuePoint)]
        [OffsetFieldName("m_ControlPoint1")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point ControlPoint1
        {
            get;
            set;
        }

        [PropertyFlags(NeedsInvoke = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valuePoint)]
        [OffsetFieldName("m_ControlPoint2")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point ControlPoint2
        {
            get;
            set;
        }

        public KeySpline() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CColorKeyFrameCollection")]
    [Guids(ClassGuid = "e7d1cf1b-9862-4cbe-b9ee-a2c87f035a7c")]
    public sealed class ColorKeyFrameCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<ColorKeyFrame>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.ColorKeyFrame ContentProperty
        {
            get;
            set;
        }

        public ColorKeyFrameCollection() { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CDoubleKeyFrameCollection")]
    [Guids(ClassGuid = "9854aadc-4b24-487b-a77e-fa5061e482f5")]
    public sealed class DoubleKeyFrameCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<DoubleKeyFrame>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.DoubleKeyFrame ContentProperty
        {
            get;
            set;
        }

        public DoubleKeyFrameCollection() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CPointKeyFrameCollection")]
    [Guids(ClassGuid = "7f80847d-8398-4a48-8e5f-2f9131f5140c")]
    public sealed class PointKeyFrameCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<PointKeyFrame>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.PointKeyFrame ContentProperty
        {
            get;
            set;
        }

        public PointKeyFrameCollection() { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CObjectKeyFrameCollection")]
    [Guids(ClassGuid = "d12f34fc-8567-4406-9673-09c81b43fa91")]
    public sealed class ObjectKeyFrameCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<ObjectKeyFrame>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.ObjectKeyFrame ContentProperty
        {
            get;
            set;
        }

        public ObjectKeyFrameCollection() { }
    }

    [NativeName("CDoubleAnimationUsingKeyFrames")]
    [ContentProperty("KeyFrames")]
    [Guids(ClassGuid = "3f9998e0-55ee-494a-bec8-d2634699c071")]
    public sealed class DoubleAnimationUsingKeyFrames
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pKeyFrames")]
        [ReadOnly]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Microsoft.UI.Xaml.Media.Animation.DoubleKeyFrameCollection KeyFrames
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_enableDependentAnimation")]
        public Windows.Foundation.Boolean EnableDependentAnimation
        {
            get;
            set;
        }

        public DoubleAnimationUsingKeyFrames() { }
    }

    [NativeName("CDoubleKeyFrame")]
    [Guids(ClassGuid = "d49e8093-5bb1-4e95-9114-a4aa04b1ff09")]
    public abstract class DoubleKeyFrame
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_rValue")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double Value
        {
            get;
            set;
        }

        [OffsetFieldName("m_keyTime")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [NativeStorageType(OM.ValueType.valueVO)]
        [PropertyFlags(DoNotEnterOrLeaveValue = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Media.Animation.KeyTime KeyTime
        {
            get;
            set;
        }

        protected DoubleKeyFrame() { }
    }

    [NativeName("CDiscreteDoubleKeyFrame")]
    [Guids(ClassGuid = "dbce3338-501a-49e0-8c09-266dd7510d4f")]
    public sealed class DiscreteDoubleKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.DoubleKeyFrame
    {
        public DiscreteDoubleKeyFrame() { }
    }

    [NativeName("CLinearDoubleKeyFrame")]
    [Guids(ClassGuid = "8344399d-36a4-4cee-834e-11f99e20d760")]
    public sealed class LinearDoubleKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.DoubleKeyFrame
    {
        public LinearDoubleKeyFrame() { }
    }

    [NativeName("CSplineDoubleKeyFrame")]
    [Guids(ClassGuid = "1fec1085-790a-4c2d-8585-93e3efb0e928")]
    public sealed class SplineDoubleKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.DoubleKeyFrame
    {
        [RequiresMultipleAssociationCheck]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pKeySpline")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.KeySpline KeySpline
        {
            get;
            set;
        }

        public SplineDoubleKeyFrame() { }
    }

    [NativeName("CEasingDoubleKeyFrame")]
    [Guids(ClassGuid = "d296df57-dd91-4ae7-98fc-58b6eaba7f66")]
    public sealed class EasingDoubleKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.DoubleKeyFrame
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pEasingFunction")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase EasingFunction
        {
            get;
            set;
        }

        public EasingDoubleKeyFrame() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CColorAnimationUsingKeyFrames")]
    [ContentProperty("KeyFrames")]
    [Guids(ClassGuid = "7e674b6c-518a-4b7f-9bab-7f0bbf02b6a9")]
    public sealed class ColorAnimationUsingKeyFrames
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pKeyFrames")]
        [ReadOnly]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Microsoft.UI.Xaml.Media.Animation.ColorKeyFrameCollection KeyFrames
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_enableDependentAnimation")]
        public Windows.Foundation.Boolean EnableDependentAnimation
        {
            get;
            set;
        }

        public ColorAnimationUsingKeyFrames() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CColorKeyFrame")]
    [Guids(ClassGuid = "03168289-b3cd-4585-8118-ec25cf67fd2c")]
    public abstract class ColorKeyFrame
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(OM.ValueType.valueColor)]
        [OffsetFieldName("m_uValue")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.UI.Color Value
        {
            get;
            set;
        }

        [OffsetFieldName("m_keyTime")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [NativeStorageType(OM.ValueType.valueVO)]
        [PropertyFlags(DoNotEnterOrLeaveValue = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Media.Animation.KeyTime KeyTime
        {
            get;
            set;
        }

        protected ColorKeyFrame() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CDiscreteColorKeyFrame")]
    [Guids(ClassGuid = "66e97d62-2398-40fd-b8f1-a540e6bed225")]
    public sealed class DiscreteColorKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.ColorKeyFrame
    {
        public DiscreteColorKeyFrame() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CLinearColorKeyFrame")]
    [Guids(ClassGuid = "5b8075f8-9d50-41bb-8710-7c268a1a4530")]
    public sealed class LinearColorKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.ColorKeyFrame
    {
        public LinearColorKeyFrame() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CSplineColorKeyFrame")]
    [Guids(ClassGuid = "2eb54730-f8a7-46de-8657-06539f744698")]
    public sealed class SplineColorKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.ColorKeyFrame
    {
        [RequiresMultipleAssociationCheck]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pKeySpline")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.KeySpline KeySpline
        {
            get;
            set;
        }

        public SplineColorKeyFrame() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CEasingColorKeyFrame")]
    [Guids(ClassGuid = "ac66af9d-21ce-49ac-98ff-49ae33dacf35")]
    public sealed class EasingColorKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.ColorKeyFrame
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pEasingFunction")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase EasingFunction
        {
            get;
            set;
        }

        public EasingColorKeyFrame() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CPointAnimationUsingKeyFrames")]
    [ContentProperty("KeyFrames")]
    [Guids(ClassGuid = "06bd8185-84f1-47c7-83a1-fd34f625d8d6")]
    public sealed class PointAnimationUsingKeyFrames
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pKeyFrames")]
        [ReadOnly]
        [RenderDirtyFlagClassName("CTimeline")]
        [RenderDirtyFlagMethodName("SetDCompAnimationDirty")]
        public Microsoft.UI.Xaml.Media.Animation.PointKeyFrameCollection KeyFrames
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_enableDependentAnimation")]
        public Windows.Foundation.Boolean EnableDependentAnimation
        {
            get;
            set;
        }

        public PointAnimationUsingKeyFrames() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CPointKeyFrame")]
    [Guids(ClassGuid = "5b6855b4-7554-45fc-a54d-bdb605a756b3")]
    public abstract class PointKeyFrame
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(OM.ValueType.valuePoint)]
        [OffsetFieldName("m_ptValue")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Point Value
        {
            get;
            set;
        }

        [OffsetFieldName("m_keyTime")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [NativeStorageType(OM.ValueType.valueVO)]
        [PropertyFlags(DoNotEnterOrLeaveValue = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Media.Animation.KeyTime KeyTime
        {
            get;
            set;
        }

        protected PointKeyFrame() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CDiscretePointKeyFrame")]
    [Guids(ClassGuid = "b5cf70f5-9308-4ed0-ab85-26d30ec27fcc")]
    public sealed class DiscretePointKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.PointKeyFrame
    {
        public DiscretePointKeyFrame() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CLinearPointKeyFrame")]
    [Guids(ClassGuid = "c7785333-e097-4b40-a0d5-3e1392b53fc7")]
    public sealed class LinearPointKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.PointKeyFrame
    {
        public LinearPointKeyFrame() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CSplinePointKeyFrame")]
    [Guids(ClassGuid = "0eebbcc3-d99d-4562-8fc4-e0bc8ec996e6")]
    public sealed class SplinePointKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.PointKeyFrame
    {
        [RequiresMultipleAssociationCheck]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pKeySpline")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.KeySpline KeySpline
        {
            get;
            set;
        }

        public SplinePointKeyFrame() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [NativeName("CEasingPointKeyFrame")]
    [Guids(ClassGuid = "01503db8-fe95-4a42-b754-6c2d52c38922")]
    public sealed class EasingPointKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.PointKeyFrame
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pEasingFunction")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.EasingFunctionBase EasingFunction
        {
            get;
            set;
        }

        public EasingPointKeyFrame() { }
    }

    [NativeName("CObjectAnimationUsingKeyFrames")]
    [ContentProperty("KeyFrames")]
    [Guids(ClassGuid = "0cac866c-f012-44ff-8d9b-6ad750f31bfe")]
    public sealed class ObjectAnimationUsingKeyFrames
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pKeyFrames")]
        [ReadOnly]
        public Microsoft.UI.Xaml.Media.Animation.ObjectKeyFrameCollection KeyFrames
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_enableDependentAnimation")]
        public Windows.Foundation.Boolean EnableDependentAnimation
        {
            get;
            set;
        }

        public ObjectAnimationUsingKeyFrames() { }
    }

    [NativeName("CObjectKeyFrame")]
    [Guids(ClassGuid = "b44d5ae8-45c6-4b0c-8632-4edcaff0ad09")]
    public abstract class ObjectKeyFrame
     : Microsoft.UI.Xaml.DependencyObject
    {
        [OffsetFieldName("m_vValue")]
        public Windows.Foundation.Object Value
        {
            get;
            set;
        }

        [OffsetFieldName("m_keyTime")]
        [NativeStorageType(OM.ValueType.valueVO)]
        [PropertyFlags(DoNotEnterOrLeaveValue = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Media.Animation.KeyTime KeyTime
        {
            get;
            set;
        }

        protected ObjectKeyFrame() { }
    }

    [NativeName("CDiscreteObjectKeyFrame")]
    [Guids(ClassGuid = "17587f66-538c-45a3-9197-3ac0170354fd")]
    public sealed class DiscreteObjectKeyFrame
     : Microsoft.UI.Xaml.Media.Animation.ObjectKeyFrame
    {
        public DiscreteObjectKeyFrame() { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CTimelineCollection")]
    [Guids(ClassGuid = "ad375b68-2393-44f4-91b6-65138f8e55ab")]
    public sealed class TimelineCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Timeline>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Animation.Timeline ContentProperty
        {
            get;
            set;
        }

        public TimelineCollection() { }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface ITransitionPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        void CreateStoryboard(Microsoft.UI.Xaml.UIElement element, Windows.Foundation.Rect start, Windows.Foundation.Rect destination, Microsoft.UI.Xaml.TransitionTrigger transitionTrigger, Windows.Foundation.Collections.IVector<Microsoft.UI.Xaml.Media.Animation.Storyboard> storyboards, out Microsoft.UI.Xaml.TransitionParent parentForTransition);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        Windows.Foundation.Boolean ParticipatesInTransition(Microsoft.UI.Xaml.UIElement element, Microsoft.UI.Xaml.TransitionTrigger transitonTrigger);
    }

    [AllowsMultipleAssociations]
    [CodeGen(partial: true)]
    [NativeName("CTransition")]
    [Implements(typeof(Microsoft.UI.Xaml.Media.Animation.ITransitionPrivate))]
    [Implements(typeof(Microsoft.UI.Xaml.Media.Animation.ITransitionFactoryPrivate), IsStaticInterface = true)]
    [Guids(ClassGuid = "86c493cc-6096-4510-a9b5-4ba03fd412fd")]
    public class Transition
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pStaggerFunction")]
        internal Microsoft.UI.Xaml.DependencyObject GeneratedStaggerFunction
        {
            get;
            set;
        }

        protected internal Transition()
        {
        }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface ITransitionFactoryPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Transition CreateInstance(object outer, out object inner);
    }

    [Guids(ClassGuid = "52240216-220f-4632-b587-42d46c46bebf")]
    internal interface ITransitionContextProvider
    {
        [CodeGen(CodeGenLevel.Idl)]
        [Comment("Get the current context")]
        Microsoft.UI.Xaml.Media.Animation.ThemeTransitionContext GetCurrentTransitionContext(Windows.Foundation.Int32 LayoutTickId);

        [CodeGen(CodeGenLevel.Idl)]
        [Comment("Gets the location relative to the root visual, from where dropped items will be moved to their destination")]
        Windows.Foundation.Point GetDropOffsetToRoot();

        [CodeGen(CodeGenLevel.Idl)]
        [Comment("Get whether the collection changes are going fast.")]
        Windows.Foundation.Boolean IsCollectionMutatingFast();
    }

    [Guids(ClassGuid = "0fd251ce-22a2-4357-a54a-fe6bd6dd6022")]
    internal interface IChildTransitionContextProvider
    {
        [CodeGen(CodeGenLevel.Idl)]
        [Comment("Get the current context from child")]
        Microsoft.UI.Xaml.Media.Animation.ThemeTransitionContext GetChildTransitionContext(Microsoft.UI.Xaml.UIElement element, Windows.Foundation.Int32 LayoutTickId);

        [CodeGen(CodeGenLevel.Idl)]
        [Comment("Gets the bounds of child transition.")]
        Windows.Foundation.Rect GetChildTransitionBounds(Microsoft.UI.Xaml.UIElement element);

        [CodeGen(CodeGenLevel.Idl)]
        [Comment("Get whether the collection changse are going fast.")]
        Windows.Foundation.Boolean IsCollectionMutatingFast();
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "3514516d-07a6-4695-acd5-4c84108088cd")]
    public sealed class RepositionThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        [FieldBacked]
        public Windows.Foundation.Boolean IsStaggeringEnabled
        {
            get;
            set;
        }

        public RepositionThemeTransition() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "b41a0f01-4151-4226-b436-48182124a30e")]
    internal sealed class InputPaneThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        public InputPaneThemeTransition() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "a6adab90-56ec-4a06-b431-f12e80a03fab")]
    internal sealed class MenuPopupThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        public Windows.Foundation.Double OpenedLength
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.AnimationDirection Direction
        {
            get;
            set;
        }

        public Windows.Foundation.Double ClosedRatio
        {
            get;
            set;
        }

        public MenuPopupThemeTransition() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "b282fe7e-2f31-4fa3-b8f1-1797a0eb3ad1")]
    internal sealed class PickerFlyoutThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        public Windows.Foundation.Double OpenedLength
        {
            get;
            set;
        }

        public Windows.Foundation.Double OffsetFromCenter
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "e76b0019-77cc-4093-b963-318b1e47148a")]
    public sealed class AddDeleteThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        public AddDeleteThemeTransition() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "6d709bae-6c2d-408b-aa53-9053256d49b1")]
    public sealed class EdgeUIThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        [FieldBacked]
        public Microsoft.UI.Xaml.Controls.Primitives.EdgeTransitionLocation Edge
        {
            get;
            set;
        }

        public EdgeUIThemeTransition() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "a4dcb49f-e34d-4a2f-af27-3a78d961f7d0")]
    public sealed class PaneThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        public Microsoft.UI.Xaml.Controls.Primitives.EdgeTransitionLocation Edge
        {
            get;
            set;
        }

        public PaneThemeTransition() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "ae44c375-6d13-4366-9239-10e647d048fb")]
    public sealed class ContentThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        public Windows.Foundation.Double HorizontalOffset
        {
            get;
            set;
        }

        public Windows.Foundation.Double VerticalOffset
        {
            get;
            set;
        }

        public ContentThemeTransition() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "ce2d8cb3-39fa-4c56-b88a-f0aa1f7ba963")]
    public sealed class PopupThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        public Windows.Foundation.Double FromHorizontalOffset
        {
            get;
            set;
        }

        public Windows.Foundation.Double FromVerticalOffset
        {
            get;
            set;
        }

        public PopupThemeTransition() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "05a43c68-3f61-4e3d-b3f7-05da43a21ce8")]
    public sealed class EntranceThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        public Windows.Foundation.Double FromHorizontalOffset
        {
            get;
            set;
        }

        public Windows.Foundation.Double FromVerticalOffset
        {
            get;
            set;
        }

        [FieldBacked]
        public Windows.Foundation.Boolean IsStaggeringEnabled
        {
            get;
            set;
        }

        public EntranceThemeTransition() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "cb164460-812e-4de0-af90-9011b3cb3e38")]
    internal sealed class ContentDialogOpenCloseThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        public ContentDialogOpenCloseThemeTransition() { }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [DXamlModifier(Modifier.Public)]
    [EnumFlags(IsNativeTypeDef = true)]
    [NativeName("NavigationTrigger")]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public enum NavigationTrigger
    {
        [NativeValueName("NavigatingAway")]
        NavigatingAway = 0,
        [NativeValueName("NavigatingTo")]
        NavigatingTo = 1,
        [NativeValueName("BackNavigatingAway")]
        BackNavigatingAway = 2,
        [NativeValueName("BackNavigatingTo")]
        BackNavigatingTo = 3,
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface INavigationTransitionInfoPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void CreateStoryboards(Microsoft.UI.Xaml.UIElement element, Microsoft.UI.Xaml.Media.Animation.NavigationTrigger trigger, Windows.Foundation.Collections.IVector<Microsoft.UI.Xaml.Media.Animation.Storyboard> storyboards);
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface INavigationTransitionInfoOverridesPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void CreateStoryboardsCore(Microsoft.UI.Xaml.UIElement element, Microsoft.UI.Xaml.Media.Animation.NavigationTrigger trigger, Windows.Foundation.Collections.IVector<Microsoft.UI.Xaml.Media.Animation.Storyboard> storyboards);
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Media.Animation.INavigationTransitionInfoPrivate))]
    [Implements(typeof(Microsoft.UI.Xaml.Media.Animation.INavigationTransitionInfoOverridesPrivate))]
    [Guids(ClassGuid = "92d011e2-fca0-495c-a72e-990f65e32bb9")]
    public abstract class NavigationTransitionInfo
     : Microsoft.UI.Xaml.DependencyObject
    {
        protected NavigationTransitionInfo() { }

        protected virtual Windows.Foundation.String GetNavigationStateCore()
        {
            return default(Windows.Foundation.String);
        }

        protected virtual void SetNavigationStateCore(Windows.Foundation.String navigationState)
        {

        }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "e23a139c-a080-4357-aaad-224526007562")]
    public sealed class ReorderThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        public ReorderThemeTransition() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "c506929a-4ad1-4ea8-aa14-77bd5384961c")]
    public sealed class DrillInThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String EntranceTargetName
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject EntranceTarget
        {
            get;
            set;
        }

        public Windows.Foundation.String ExitTargetName
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject ExitTarget
        {
            get;
            set;
        }

        public DrillInThemeAnimation() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [AnimationPattern]
    [Guids(ClassGuid = "e6401717-c507-435b-9281-237fe1f195b2")]
    public sealed class DrillOutThemeAnimation
     : Microsoft.UI.Xaml.Media.Animation.DynamicTimeline
    {
        public Windows.Foundation.String EntranceTargetName
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject EntranceTarget
        {
            get;
            set;
        }

        public Windows.Foundation.String ExitTargetName
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject ExitTarget
        {
            get;
            set;
        }

        public DrillOutThemeAnimation() { }
    }

    [AllowsMultipleAssociations]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CTransitionCollection")]
    [Guids(ClassGuid = "fd7094ca-066e-405c-8230-4580987ca9bd")]
    public sealed class TransitionCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Transition>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Xaml.Media.Animation.Transition ContentProperty
        {
            get;
            set;
        }

        public TransitionCollection() { }
    }

    [TypeTable(IsExcludedFromCore = true, IsExcludedFromNewTypeTable = true)]
    [EnumFlags(HasTypeConverter = true)]
    internal enum ThemeTransitionContext
    {
        None = 0,
        Entrance = 1,
        ContentTransition = 2,
        SingleAddList = 3,
        SingleDeleteList = 4,
        SingleAddGrid = 5,
        SingleDeleteGrid = 6,
        MultipleAddList = 7,
        MultipleDeleteList = 8,
        MultipleAddGrid = 9,
        MultipleDeleteGrid = 10,
        SingleReorderList = 11,
        SingleReorderGrid = 12,
        MultipleReorderList = 13,
        MultipleReorderGrid = 14,
        ReorderedItem = 15,
        MixedOperationsList = 16,
        MixedOperationsGrid = 17,
    }

    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [EnumFlags(HasTypeConverter = true)]
    public enum RepeatBehaviorType
    {
        Count = 0,
        Duration = 1,
        Forever = 2,
    }

    [NativeName("FillBehavior")]
    [EnumFlags(HasTypeConverter = true)]
    public enum FillBehavior
    {
        [NativeValueName("FillHoldEnd")]
        HoldEnd = 0,
        [NativeValueName("FillStop")]
        Stop = 1,
    }

    [NativeName("EasingMode")]
    [EnumFlags(HasTypeConverter = true, NativeUsesNumericValues = false)]
    public enum EasingMode
    {
        EaseOut,
        EaseIn,
        EaseInOut,
    }

    [NativeName("ExposedClockState")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum ClockState
    {
        [NativeValueName("ClockStateActive")]
        Active = 0,
        [NativeValueName("ClockStateFilling")]
        Filling = 1,
        [NativeValueName("ClockStateStopped")]
        Stopped = 2,
        [NativeValueName("ClockStateNotStarted")]
        [CodeGen(CodeGenLevel.CoreOnly)]
        NotStarted = 3,
    }

    [Comment("Identifies a particular component of a connected animation.")]
    [FrameworkTypePattern]
    [DXamlIdlGroup("coretypes2")]
    [NativeName("ConnectedAnimationComponent")]
    public enum ConnectedAnimationComponent
    {
        OffsetX = 0,
        OffsetY = 1,
        CrossFade = 2,
        Scale = 3,
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [NativeName("CConnectedAnimation")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "dfe8acc7-3e69-485f-b327-4c4b00155c75")]
    public sealed class ConnectedAnimation : Microsoft.UI.Xaml.DependencyObject
    {
        internal ConnectedAnimation() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("TryStart")]
        [DXamlOverloadName("TryStart")]
        public Windows.Foundation.Boolean TryStart(UIElement destination)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("TryStartWithCoordinatedElements")]
        [DXamlOverloadName("TryStart")]
        public Windows.Foundation.Boolean TryStartWithCoordinatedElements(UIElement destination, Windows.Foundation.Collections.IIterable<UIElement> coordinatedElements)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Cancel() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetAnimationComponent(ConnectedAnimationComponent component, [Optional] Microsoft.UI.Composition.ICompositionAnimationBase animation) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsScaleAnimationEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public ConnectedAnimationConfiguration Configuration
        {
            get;
            set;
        }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.EventHandler Completed;
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [NativeName("CConnectedAnimationService")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "7396109d-2c53-4e35-9051-2e80350547bb")]
    public sealed class ConnectedAnimationService : Microsoft.UI.Xaml.DependencyObject
    {
        public static ConnectedAnimationService GetForCurrentView()
        {
            return default(ConnectedAnimationService);
        }

        internal ConnectedAnimationService() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public ConnectedAnimation PrepareToAnimate(Windows.Foundation.String key, UIElement source)
        {
            return default(ConnectedAnimation);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public ConnectedAnimation GetAnimation(Windows.Foundation.String key)
        {
            return default(ConnectedAnimation);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(OM.ValueType.valueObject)]
        public Windows.Foundation.TimeSpan DefaultDuration
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Composition.CompositionEasingFunction DefaultEasingFunction
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "add4b631-99f9-4a5d-b0cd-754eef51253e")]
    public class ConnectedAnimationConfiguration : Windows.Foundation.Object
    {
        internal ConnectedAnimationConfiguration() { }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        [ReturnTypeParameterName("effectPropertySet")]
        internal virtual Microsoft.UI.Composition.CompositionPropertySet GetEffectPropertySet( Windows.Foundation.Numerics.Vector3 scaleFactors)
        {
            return default(Microsoft.UI.Composition.CompositionPropertySet);
        }

    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "f6142b10-7957-4f44-bc46-e3a8cd5c0383")]
    public class BasicConnectedAnimationConfiguration : ConnectedAnimationConfiguration
    {
        public BasicConnectedAnimationConfiguration() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "742c8b46-2243-40fe-b76c-5251fb802c77")]
    public class DirectConnectedAnimationConfiguration : ConnectedAnimationConfiguration
    {
        public DirectConnectedAnimationConfiguration() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "26a94083-090c-4999-ae94-2a2aa5e54b9f")]
    public class GravityConnectedAnimationConfiguration : ConnectedAnimationConfiguration
    {
        public GravityConnectedAnimationConfiguration() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeStorageType(OM.ValueType.valueBool)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsShadowEnabled
        {
            get;
            set;
        }
    }
}
