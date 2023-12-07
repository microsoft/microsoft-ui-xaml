// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Controls;
using XamlOM;

namespace Microsoft.UI.Xaml.Internal
{
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CInternalTransform")]
    [Guids(ClassGuid = "2e93250c-816c-4575-9940-611cacd96e0c")]
    internal sealed class InternalTransform
     : Microsoft.UI.Xaml.Media.GeneralTransform
    {
        public InternalTransform() { }

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

    [NativeName("CTextPointerWrapper")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "b087bd24-afb0-4aa1-a675-d3465e2114db")]
    internal sealed class TextPointerWrapper
     : Microsoft.UI.Xaml.DependencyObject
    {
        public TextPointerWrapper() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CMediaBase")]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "67b43417-f358-4c2f-85ef-3d59f64d3a93")]
    internal abstract class MediaBase
     : Microsoft.UI.Xaml.FrameworkElement
    {
        protected MediaBase() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CSwapChainElement")]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "5273a9b5-548b-48c6-af4a-c5c10c2ad81e")]
    internal abstract class SwapChainElement
     : Microsoft.UI.Xaml.UIElement
    {
        protected SwapChainElement() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CMediaSwapChainElement")]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "46079c92-298b-4bcd-89e8-156b87b893ae")]
    internal abstract class MediaSwapChainElement
     : Microsoft.UI.Xaml.UIElement
    {
        protected MediaSwapChainElement() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Unnamed]
    [NativeName("HWCompNode")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "f2e8c53e-eca5-46f4-a375-ff79f362e64e")]
    internal abstract class HWCompNode
     : Microsoft.UI.Xaml.DependencyObject
    {
        protected HWCompNode() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Unnamed]
    [NativeName("HWCompLeafNode")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "1967dde2-93a4-4298-a55c-ef6e562e8391")]
    internal class HWCompLeafNode
     : Microsoft.UI.Xaml.Internal.HWCompNode
    {
        protected HWCompLeafNode() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Unnamed]
    [NativeName("HWCompRenderDataNode")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "6829cdc6-dc28-4252-ad13-e6b2c0f96283")]
    internal class HWCompRenderDataNode
     : Microsoft.UI.Xaml.Internal.HWCompLeafNode
    {
        protected HWCompRenderDataNode() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Unnamed]
    [NativeName("HWCompMediaNode")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "ab1b9c8f-9fad-42ec-999e-4288ba88c1ec")]
    internal class HWCompMediaNode
     : Microsoft.UI.Xaml.Internal.HWCompLeafNode
    {
        protected HWCompMediaNode() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Unnamed]
    [NativeName("HWCompSwapChainNode")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "6797a72a-5617-4275-aadf-6c1ee8f48ce1")]
    internal class HWCompSwapChainNode
     : Microsoft.UI.Xaml.Internal.HWCompLeafNode
    {
        protected HWCompSwapChainNode() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Unnamed]
    [NativeName("HWCompTreeNode")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "e916aff9-4c5c-462d-a2da-c75ac32db838")]
    internal class HWCompTreeNode
     : Microsoft.UI.Xaml.Internal.HWCompNode
    {
        protected HWCompTreeNode() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Unnamed]
    [NativeName("HWRedirectedCompTreeNodeWinRT")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "c1db02ab-ee90-429b-9a6f-973055a4ca34")]
    internal class HWRedirectedCompTreeNodeWinRT
     : Microsoft.UI.Xaml.Internal.HWCompTreeNode
    {
        protected HWRedirectedCompTreeNodeWinRT() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Unnamed]
    [NativeName("HWWindowedPopupCompTreeNodeWinRT")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "03454d2d-0735-4e45-a37b-698d8a7d660e")]
    internal class HWWindowedPopupCompTreeNodeWinRT
     : Microsoft.UI.Xaml.Internal.HWRedirectedCompTreeNodeWinRT
    {
        protected HWWindowedPopupCompTreeNodeWinRT() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CErrorEventArgs")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [Guids(ClassGuid = "9e5c1d9c-2656-4ea6-bbf7-7ebf4b8a7421")]
    internal class ErrorEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strErrorMessage")]
        public Windows.Foundation.String ErrorMessage
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_iErrorCode")]
        public Windows.Foundation.Int32 ErrorCode
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_eType")]
        public Microsoft.UI.Xaml.ErrorType ErrorType
        {
            get;
            set;
        }

        public ErrorEventArgs() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextBoxBase")]
    [Guids(ClassGuid = "852ce07a-5e85-4e0c-b771-c4a4645a7b52")]
    internal abstract class TextBoxBase
     : Microsoft.UI.Xaml.Controls.Control
    {
        protected TextBoxBase() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextSelectionGripper")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "84ac1b5d-e4b8-4be5-aead-6e30a2de8a66")]
    internal class TextSelectionGripper
     : Microsoft.UI.Xaml.Controls.Canvas
    {
        protected TextSelectionGripper() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextBoxView")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollInfo))]
    [Guids(ClassGuid = "053f2498-8d9c-4a3f-b52b-d130903fba13")]
    internal sealed class TextBoxView
     : Microsoft.UI.Xaml.FrameworkElement
    {
        public TextBoxView() { }
    }

    [HandWritten(InCore = true)]
    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CCaretBrowsingCaret")]
    [ClassFlags(IsVisibleInXAML = false)]
    internal sealed class CaretBrowsingCaret
     : Microsoft.UI.Xaml.Controls.Panel
    {
        public CaretBrowsingCaret() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CLayoutTransitionElement")]
    [ClassFlags(IsVisibleInXAML = false)]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "01c959ad-77f6-48b3-b64b-87ab500c13af")]
    internal sealed class LayoutTransitionElement
     : Microsoft.UI.Xaml.UIElement
    {
        public LayoutTransitionElement() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CIsEnabledChangedEventArgs")]
    [Guids(ClassGuid = "52d07be9-4d8d-4165-9009-132f5133227b")]
    internal sealed class IsEnabledChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_fOldValue")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Boolean OldValue
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_fNewValue")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Boolean NewValue
        {
            get;
            private set;
        }

        public IsEnabledChangedEventArgs() { }
    }

    [CodeGen(CodeGenLevel.Stub)]
    [TypeFlags(IsCreateableFromXAML = false, IsExcludedFromDXamlInterface = true)]
    [NativeName("CDisplayMemberTemplate")]
    [Guids(ClassGuid = "fed3a8d2-cb3a-42f8-bc78-05cd36719eec")]
    internal sealed class DisplayMemberTemplate
     : Microsoft.UI.Xaml.DataTemplate
    {
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strMemberPath")]
        public Windows.Foundation.String DisplayMemberPath
        {
            get;
            set;
        }

        public DisplayMemberTemplate() { }
    }

    [AllowsMultipleAssociations]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CManagedObjectReference")]
    [Guids(ClassGuid = "dc678962-0653-4c92-92cd-95943d593900")]
    internal sealed class ExternalObjectReference
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_markupExtensionType")]
        internal Microsoft.UI.Xaml.MarkupExtensionType MarkupExtensionType
        {
            get;
            private set;
        }

        [CoreType(typeof(DependencyObject), NewCodeGenPropertyType = typeof(Windows.Foundation.Object))]
        [OffsetFieldName("m_nativeValue")]
        public Windows.Foundation.Object NativeValue
        {
            get;
            set;
        }

        public ExternalObjectReference() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CAutomationPeerEventArgs")]
    [Guids(ClassGuid = "c6fb72fb-d0ae-447a-89d0-cb5a157d721c")]
    internal abstract class AutomationPeerEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        public AutomationPeerEventArgs() { }
    }

    [AllowsMultipleAssociations]
    [CodeGen(CodeGenLevel.Stub, Partial = true)]
    [TypeFlags(IsCreateableFromXAML = false, IsExcludedFromDXamlInterface = true)]
    [NativeName("CDependencyPropertyProxy")]
    [ClassFlags(HasTypeConverter = true, IsHiddenFromIdl = true)]
    [IdlType(typeof(DependencyProperty))]
    [Guids(ClassGuid = "ecb57fd7-7fb8-4b60-acea-5d28d1399d76")]
    public sealed class DependencyPropertyProxy
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_nPropertyIndex")]
        public Windows.Foundation.Int32 PropertyId
        {
            get;
            set;
        }

        public DependencyPropertyProxy() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CResourceDictionaryCollection")]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "f7825620-fda9-41f2-8622-2dc2a0563452")]
    public sealed class ResourceDictionaryCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<ResourceDictionary>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.ResourceDictionary ContentProperty
        {
            get;
            set;
        }

        public ResourceDictionaryCollection() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CVisualStateGroupCollection")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "6ccb6be8-8b1d-4212-a80d-c438bf7332d6")]
    public sealed class VisualStateGroupCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<VisualStateGroup>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.VisualStateGroup ContentProperty
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pActiveTransitions")]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Internal.VisualTransitionCollection ActiveTransitions
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pActiveStoryboards")]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Internal.StoryboardCollection ActiveStoryboards
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pDeferredStateTriggers")]
        [CollectionType(CollectionKind.Vector)]
        internal Microsoft.UI.Xaml.Internal.StateTriggerCollection DeferredStateTriggers
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pDeferredSetters")]
        [CollectionType(CollectionKind.Vector)]
        internal Microsoft.UI.Xaml.SetterBaseCollection DeferredSetters
        {
            get;
            set;
        }
        public VisualStateGroupCollection() { }
    }

    [Guids(ClassGuid = "a14b144f-848c-4e71-a8e8-56d64b7890e8")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CStateTriggerCollection")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Modifier(Modifier.Internal)]
    public sealed class StateTriggerCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Microsoft.UI.Xaml.StateTriggerBase>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.StateTriggerBase ContentProperty
        {
            get;
            set;
        }

        public StateTriggerCollection() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CVisualTransitionCollection")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "c1f47036-b831-436c-9935-a1d325c1eca8")]
    public sealed class VisualTransitionCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<VisualTransition>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.VisualTransition ContentProperty
        {
            get;
            set;
        }

        public VisualTransitionCollection() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CStoryboardCollection")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "69098eb0-dc2c-4d6e-9742-1ac49b6fd9b2")]
    public sealed class StoryboardCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Microsoft.UI.Xaml.Media.Animation.Storyboard>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Media.Animation.Storyboard ContentProperty
        {
            get;
            set;
        }

        public StoryboardCollection() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CVisualStateCollection")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "f1753425-b756-483a-a39b-6cd4b301554b")]
    public sealed class VisualStateCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<VisualState>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.VisualState ContentProperty
        {
            get;
            set;
        }

        public VisualStateCollection() { }
    }

    [AllowsMultipleAssociations]
    [NativeName("CTemplateContent")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "a6e509db-665a-4eab-b140-7d6108d1e053")]
    internal sealed class TemplateContent
     : Microsoft.UI.Xaml.DependencyObject
    {
        public TemplateContent() { }
    }

    [AllowsMultipleAssociations]
    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false, IsExcludedFromDXamlInterface = true)]
    [NativeName("CDependencyObjectWrapper")]
    [Guids(ClassGuid = "947f3d39-ebc5-405f-b405-6b1a92c1e73f")]
    internal sealed class DependencyObjectWrapper
     : Microsoft.UI.Xaml.DependencyObject
    {
        public DependencyObjectWrapper() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [EnumFlags(AreValuesFlags = true)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    internal enum TypeFlags
    {
        [NativeValueName("tfNone")]
        None = 0,
        [NativeValueName("tfHasContentProperty")]
        HasContentProperty = 1,
        [NativeValueName("tfHasTypeConverter")]
        HasTypeConverter = 2,
        [NativeValueName("tfIsIList")]
        IsIList = 4,
        [NativeValueName("tfIsISupportInitialize")]
        IsISupportInitialize = 8,
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("ManagedEvent")]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [NativeComment("Events that have no counterpart in a DO or in the core")]
    internal enum ManagedEvent
    {
        [NativeValueName("ManagedEventSizeChanged")]
        ManagedEventSizeChanged = 1,
        [NativeValueName("ManagedEventLayoutUpdated")]
        ManagedEventLayoutUpdated = 2,
        [NativeValueName("ManagedEventRendering")]
        ManagedEventRendering = 3,
        [NativeValueName("ManagedEventRendered")]
        ManagedEventRendered = 4,
        [NativeValueName("ManagedEventTouch")]
        ManagedEventTouch = 5,
        [NativeValueName("ManagedEventInheritanceContextChanged")]
        ManagedEventInheritanceContextChanged = 6,
        [NativeValueName("ManagedEventSurfaceContentsLost")]
        ManagedEventSurfaceContentsLost = 7,
        [NativeValueName("ManagedEventFocusedElementRemoved")]
        ManagedEventFocusedElementRemoved = 8,
        [NativeValueName("ManagedEventGotFocus")]
        ManagedEventGotFocus = 9,
        [NativeValueName("ManagedEventLostFocus")]
        ManagedEventLostFocus = 10,
        [NativeValueName("ManagedEventGettingFocus")]
        ManagedEventGettingFocus = 11,
        [NativeValueName("ManagedEventLosingFocus")]
        ManagedEventLosingFocus = 12,
    }

    [TypeTable(IsExcludedFromNewTypeTable = true)]
    internal enum InputPaneState
    {
        [NativeValueName("InputPaneHidden")]
        InputPaneHidden = 0,
        [NativeValueName("InputPaneShowing")]
        InputPaneShowing = 1,
    }

    [TypeTable(IsExcludedFromNewTypeTable = true)]
    internal enum CollectionChangeType
    {
        [NativeValueName("CollectionChangeReset")]
        CollectionChangeReset = 0,
        [NativeValueName("CollectionChangeItemInserted")]
        CollectionChangeItemInserted = 1,
        [NativeValueName("CollectionChangeItemRemoved")]
        CollectionChangeItemRemoved = 2,
        [NativeValueName("CollectionChangeItemChanged")]
        CollectionChangeItemChanged = 3,
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    internal delegate void ErrorEventHandler(Windows.Foundation.Object sender, Windows.Foundation.Object e);

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "baa2f6ef-22bf-4ddd-af25-60dd90aaa8a1")]
    internal class RootScrollViewer
     : Microsoft.UI.Xaml.Controls.ScrollViewer
    {
    }

    [NativeName("CPointerKeyFrameCollection")]
    [Guids(ClassGuid = "c6e57ab3-3fd5-4317-be5b-4cd102e04e12")]
    internal sealed class PointerKeyFrameCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<PointerKeyFrame>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Internal.PointerKeyFrame ContentProperty
        {
            get;
            set;
        }

        public PointerKeyFrameCollection() { }
    }

    [NativeName("PointerDirection")]
    internal enum PointerDirection
    {
        [NativeValueName("PointerDirectionXAxis")]
        PointerDirection_XAxis = 0,
        [NativeValueName("PointerDirectionYAxis")]
        PointerDirection_YAxis = 1,
        [NativeValueName("PointerDirectionBothAxes")]
        PointerDirection_BothAxes = 2,
    };

    [NativeName("CPointerAnimationUsingKeyFrames")]
    [ContentProperty("KeyFrames")]
    [Guids(ClassGuid = "71c52fe6-9ed5-4814-9bbf-c16ac3a9ddbf")]
    internal sealed class PointerAnimationUsingKeyFrames
     : Microsoft.UI.Xaml.Media.Animation.Timeline
    {
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pKeyFrames")]
        [ReadOnly]
        public Microsoft.UI.Xaml.Internal.PointerKeyFrameCollection KeyFrames
        {
            get;
            private set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pointerSource")]
        public Microsoft.UI.Xaml.Internal.PointerDirection PointerSource
        {
            get;
            set;
        }

        [NativeClassName("CPointerAnimationUsingKeyFrames")]
        [PInvoke]
        public void SetRelativeToObject(Microsoft.UI.Xaml.DependencyObject relativeToObject)
        {
        }

        [NativeClassName("CPointerAnimationUsingKeyFrames")]
        [PInvoke]
        public void SetRelativeToObjectName(Windows.Foundation.String relativeToObjectName)
        {
        }

        public PointerAnimationUsingKeyFrames() { }
    }

    [NativeName("CPointerKeyFrame")]
    [Guids(ClassGuid = "917596dc-9f87-44dc-a347-b7d6023fc6a0")]
    internal sealed class PointerKeyFrame
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_pointerValue")]
        public Windows.Foundation.Double PointerValue
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_value")]
        public Windows.Foundation.Double TargetValue
        {
            get;
            set;
        }

        public PointerKeyFrame() { }
    }

    [NativeName("CParametricCurveSegment")]
    [Guids(ClassGuid = "69b4af49-70c0-4770-9c03-fbcdd3d8d631")]
    internal sealed class ParametricCurveSegment
     : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_beginOffset")]
        public Windows.Foundation.Double BeginOffset
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_constantCoefficient")]
        public Windows.Foundation.Double ConstantCoefficient
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_linearCoefficient")]
        public Windows.Foundation.Double LinearCoefficient
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_quadraticCoefficient")]
        public Windows.Foundation.Double QuadraticCoefficient
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_cubicCoefficient")]
        public Windows.Foundation.Double CubicCoefficient
        {
            get;
            set;
        }

        public ParametricCurveSegment() { }
    }

    [NativeName("CParametricCurveSegmentCollection")]
    [Guids(ClassGuid = "3c55e858-b4b5-48c7-b629-4611b1d2cd6a")]
    internal sealed class ParametricCurveSegmentCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<ParametricCurveSegment>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Internal.ParametricCurveSegment ContentProperty
        {
            get;
            set;
        }

        public ParametricCurveSegmentCollection() { }
    }

    [NativeName("CParametricCurve")]
    [Guids(ClassGuid = "4c7a0ea8-283d-4faa-be43-59c527b3c7e0")]
    internal sealed class ParametricCurve
     : Microsoft.UI.Xaml.DependencyObject
    {
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCurveSegments")]
        [ReadOnly]
        public Microsoft.UI.Xaml.Internal.ParametricCurveSegmentCollection CurveSegments
        {
            get;
            private set;
        }

        [NativeClassName("CParametricCurve")]
        [PInvoke]
        public void SetPrimaryContentProperty(Windows.Foundation.String primaryContentProperty)
        {
        }

        [NativeClassName("CParametricCurve")]
        [PInvoke]
        public void SetSecondaryContentProperty(DirectManipulationProperty secondaryContentProperty, Windows.Foundation.String associatedDependencyProperty)
        {
        }

        public ParametricCurve() { }
    }

    [NativeName("CParametricCurveCollection")]
    [Guids(ClassGuid = "d46316ab-a992-489e-bd43-379aaf626dbb")]
    internal sealed class ParametricCurveCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<ParametricCurve>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Internal.ParametricCurve ContentProperty
        {
            get;
            set;
        }

        public ParametricCurveCollection() { }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [NativeName("CSecondaryContentRelationship")]
    [Guids(ClassGuid = "017b1a2a-1b24-42f2-9474-2051514b13da")]
    public sealed class SecondaryContentRelationship
     : Microsoft.UI.Xaml.DependencyObject
    {
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCurves")]
        [ReadOnly]
        internal Microsoft.UI.Xaml.Internal.ParametricCurveCollection Curves
        {
            get;
            private set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isDescendant")]
        internal Windows.Foundation.Boolean IsDescendant
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_shouldTargetClip")]
        internal Windows.Foundation.Boolean ShouldTargetClip
        {
            get;
            set;
        }

        [NativeClassName("CSecondaryContentRelationship")]
        [PInvoke]
        internal void SetPrimaryContent(Microsoft.UI.Xaml.UIElement primaryContent)
        {
        }

        [NativeClassName("CSecondaryContentRelationship")]
        [PInvoke]
        internal void SetSecondaryContent(Microsoft.UI.Xaml.UIElement secondaryContent, Microsoft.UI.Xaml.DependencyObject dependencyPropertyHolder)
        {
        }

        [NativeClassName("CSecondaryContentRelationship")]
        [PInvoke]
        public void SetAuxiliaryDependencyPropertyHolder(Microsoft.UI.Xaml.DependencyObject auxiliaryDependencyPropertyHolder)
        {
        }

        [NativeClassName("CSecondaryContentRelationship")]
        [PInvoke]
        public void PrepareForCurveUpdate()
        {
        }

        [NativeClassName("CSecondaryContentRelationship")]
        [PInvoke]
        public void Apply()
        {
        }

        [NativeClassName("CSecondaryContentRelationship")]
        [PInvoke]
        public void Remove()
        {
        }

        [NativeClassName("CSecondaryContentRelationship")]
        [PInvoke]
        public void UpdateDependencyProperties()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Internal.SecondaryContentRelationship CreateStickyHeaderRelationship(Microsoft.UI.Xaml.UIElement scrollViewer, Microsoft.UI.Xaml.UIElement panelObject, Microsoft.UI.Xaml.DependencyObject panelTransform, Microsoft.UI.Xaml.DependencyObject headerTransform, double groupTopY, double groupBottomY, double headerHeight)
        {
            return default(Microsoft.UI.Xaml.Internal.SecondaryContentRelationship);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Internal.SecondaryContentRelationship CreateClipTransformRelationship(Microsoft.UI.Xaml.UIElement scrollViewer, Microsoft.UI.Xaml.UIElement clipOwner, Microsoft.UI.Xaml.DependencyObject clipTransform, double listExtentHeight, double listViewportHeight)
        {
            return default(Microsoft.UI.Xaml.Internal.SecondaryContentRelationship);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Internal.SecondaryContentRelationship CreateParallaxRelationship(Microsoft.UI.Xaml.UIElement scrollViewer, Microsoft.UI.Xaml.UIElement headerObject, Microsoft.UI.Xaml.DependencyObject headerTransform, double[] primaryOffsets, double[] secondaryOffsets)
        {
            return default(Microsoft.UI.Xaml.Internal.SecondaryContentRelationship);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Internal.SecondaryContentRelationship CreateStaticElementRelationship(Microsoft.UI.Xaml.UIElement scrollViewer, Microsoft.UI.Xaml.UIElement staticElement, Microsoft.UI.Xaml.DependencyObject elementTransform, Windows.Foundation.Boolean isHorizontallyStatic, Windows.Foundation.Boolean isInverted)
        {
            return default(Microsoft.UI.Xaml.Internal.SecondaryContentRelationship);
        }

        internal SecondaryContentRelationship() { }
    }

    [NativeName("XcpDirectManipulationProperty")]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    internal enum DirectManipulationProperty
    {
        [NativeValueName("XcpDirectManipulationPropertyNone")]
        None = 0,
        [NativeValueName("XcpDirectManipulationPropertyTranslationX")]
        TranslationX = 1,
        [NativeValueName("XcpDirectManipulationPropertyTranslationY")]
        TranslationY = 2,
        [NativeValueName("XcpDirectManipulationPropertyZoom")]
        Zoom = 3,
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "fd1cab1a-0f0d-4d69-864e-e5e0a98037cb")]
    public sealed class LayoutTransitionElementUtilities
        : Windows.Foundation.Object
    {
        internal LayoutTransitionElementUtilities() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.UIElement CreateLayoutTransitionElement(Microsoft.UI.Xaml.UIElement source, [Optional] Microsoft.UI.Xaml.UIElement parent)
        {
            return default(Microsoft.UI.Xaml.UIElement);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static void DestroyLayoutTransitionElement(Microsoft.UI.Xaml.UIElement source, [Optional] Microsoft.UI.Xaml.UIElement parent, Microsoft.UI.Xaml.UIElement layoutTransitionElement)
        {
        }
    }

    [NativeName("CDeferredElement")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "dfb84602-dd75-4ca3-bbe1-1823d4b244b4")]
    internal sealed class DeferredElement
     : Microsoft.UI.Xaml.DependencyObject
    {
        public DeferredElement() { }
    }

    [Guids(ClassGuid = "44194ce2-3683-4bcc-93a0-9f6c4d1c4152")]
    [FrameworkTypePattern]
    [ClassFlags(IsVisibleInXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    internal sealed class MediaPlaybackItemConverter
     : Microsoft.UI.Xaml.DependencyObject
    {
    }

    [NativeName("CNullKeyedResource")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "acb7908c-1d2a-4967-a8bf-14907ce57ff6")]
    internal sealed class NullKeyedResource
        : Microsoft.UI.Xaml.DependencyObject
    {
        public NullKeyedResource() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [ClassFlags(IsVisibleInXAML = false)]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "ca463f39-a1ae-4cc6-ae39-4d92798cc47e")]
    [HandWritten(InCore = true, InFramework = false)]
    public sealed class ValidationErrorsCollection
        : Microsoft.UI.Xaml.Collections.ObservablePresentationFrameworkCollection<InputValidationError>
    {
    }
}

