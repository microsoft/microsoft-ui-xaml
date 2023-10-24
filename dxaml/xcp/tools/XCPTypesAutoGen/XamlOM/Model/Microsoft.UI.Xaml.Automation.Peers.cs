// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CAutomationPeerCollection")]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "758b8d2c-b5b0-4e4f-b992-c431d7da0579")]
    public sealed class AutomationPeerCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<AutomationPeer>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Automation.Peers.AutomationPeer ContentProperty
        {
            get;
            set;
        }

        internal AutomationPeerCollection() { }
    }

    //TODO: this should become a tearoff, it'd save a bunch of space
    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public interface IAutomationPeerPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetAutomationFocus();
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CAutomationPeer")]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "812cc853-129b-4cfb-b349-21c116f71ead")]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Peers.IAutomationPeerPrivate))]
    public abstract class AutomationPeer
     : Microsoft.UI.Xaml.DependencyObject
    {
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Automation.Peers.AutomationPeer EventsSource
        {
            get;
            set;
        }

        protected AutomationPeer() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Windows.Foundation.Boolean ListenerExists(Microsoft.UI.Xaml.Automation.Peers.AutomationEvents eventId)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Object GetPattern(Microsoft.UI.Xaml.Automation.Peers.PatternInterface patternInterface)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void RaiseAutomationEvent(Microsoft.UI.Xaml.Automation.Peers.AutomationEvents eventId)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void RaisePropertyChangedEvent(Microsoft.UI.Xaml.Automation.AutomationProperty automationProperty, Windows.Foundation.Object oldValue, Windows.Foundation.Object newValue)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetAcceleratorKey()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetAccessKey()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Microsoft.UI.Xaml.Automation.Peers.AutomationControlType GetAutomationControlType()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationControlType);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetAutomationId()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Rect GetBoundingRectangle()
        {
            return default(Windows.Foundation.Rect);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection GetChildren()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection);
        }

        [TypeTable(IsExcludedFromCore = true)]
        [CollectionType(CollectionKind.Vector)]
        protected virtual Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection GetChildrenCore()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Object Navigate(AutomationNavigationDirection direction)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetClassName()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Point GetClickablePoint()
        {
            return default(Windows.Foundation.Point);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetHelpText()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetItemStatus()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetItemType()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Microsoft.UI.Xaml.Automation.Peers.AutomationPeer GetLabeledBy()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetLocalizedControlType()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetName()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Microsoft.UI.Xaml.Automation.Peers.AutomationOrientation GetOrientation()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationOrientation);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean HasKeyboardFocus()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean IsContentElement()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean IsControlElement()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean IsEnabled()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean IsKeyboardFocusable()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean IsOffscreen()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean IsPassword()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean IsRequiredForForm()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual void SetFocus()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("Consider using Navigate with AutomationNavigationDirection::Parent, which is an improved version of GetParent. For more info, see MSDN.", DeprecationType.Remove)]
        public Microsoft.UI.Xaml.Automation.Peers.AutomationPeer GetParent()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void InvalidatePeer()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("Consider using GetElementFromPoint, which is an improved version of GetPeerFromPoint. For more info, see MSDN.", DeprecationType.Remove)]
        public Microsoft.UI.Xaml.Automation.Peers.AutomationPeer GetPeerFromPoint(Windows.Foundation.Point point)
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual Microsoft.UI.Xaml.Automation.Peers.AutomationPeer GetPeerFromPointCore(Windows.Foundation.Point point)
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Object GetElementFromPoint(Windows.Foundation.Point pointInWindowCoordinates)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Object GetFocusedElement()
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected Microsoft.UI.Xaml.Automation.Peers.AutomationPeer PeerFromProvider(Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple provider)
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple ProviderFromPeer(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer peer)
        {
            return default(Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Microsoft.UI.Xaml.Automation.Peers.AutomationLiveSetting GetLiveSetting()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationLiveSetting);
        }

        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void ShowContextMenuCore()
        {
        }

        [TypeTable(IsExcludedFromCore = true)]
        [CollectionType(CollectionKind.Indexable)]
        protected virtual Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection GetControlledPeersCore()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static RawElementProviderRuntimeId GenerateRawElementProviderRuntimeId()
        {
            return default(RawElementProviderRuntimeId);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void ShowContextMenu()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [CollectionType(CollectionKind.Indexable)]
        public Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection GetControlledPeers()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [CollectionType(CollectionKind.Vector)]
        public virtual Microsoft.UI.Xaml.Automation.Peers.AutomationPeerAnnotationCollection GetAnnotations()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeerAnnotationCollection);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void SetParent(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer peer)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void RaiseTextEditTextChangedEvent(Microsoft.UI.Xaml.Automation.AutomationTextEditChangeType automationTextEditChangeType, Windows.Foundation.Collections.IVectorView<Windows.Foundation.String> changedData)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Int32 GetPositionInSet()
        {
            return default(Windows.Foundation.Int32);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Int32 GetSizeOfSet()
        {
            return default(Windows.Foundation.Int32);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Int32 GetLevel()
        {
            return default(Windows.Foundation.Int32);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void RaiseStructureChangedEvent(Microsoft.UI.Xaml.Automation.Peers.AutomationStructureChangeType structureChangeType, [Optional] Microsoft.UI.Xaml.Automation.Peers.AutomationPeer child)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Microsoft.UI.Xaml.Automation.Peers.AutomationLandmarkType GetLandmarkType()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationLandmarkType);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetLocalizedLandmarkType()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean IsPeripheral()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean IsDataValidForForm()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.String GetFullDescription()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [CollectionType(CollectionKind.Enumerable)]
        protected virtual Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection GetDescribedByCore()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [CollectionType(CollectionKind.Enumerable)]
        protected virtual Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection GetFlowsToCore()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [CollectionType(CollectionKind.Enumerable)]
        protected virtual Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection GetFlowsFromCore()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeerCollection);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Int32 GetCulture()
        {
            return default(Windows.Foundation.Int32);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void RaiseNotificationEvent(Microsoft.UI.Xaml.Automation.Peers.AutomationNotificationKind notificationKind, Microsoft.UI.Xaml.Automation.Peers.AutomationNotificationProcessing notificationProcessing, [Optional] Windows.Foundation.String displayString, Windows.Foundation.String activityId)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Microsoft.UI.Xaml.Automation.Peers.AutomationHeadingLevel GetHeadingLevel()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationHeadingLevel);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public virtual Windows.Foundation.Boolean IsDialog()
        {
            return default(Windows.Foundation.Boolean);
        }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public interface FrameworkElementAutomationPeerPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Collections.IVector<Microsoft.UI.Xaml.Automation.Peers.AutomationPeer> GetAutomationPeersForChildrenOfElement(Microsoft.UI.Xaml.UIElement element);
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CFrameworkElementAutomationPeer")]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeerPrivate))]
    [Guids(ClassGuid = "5386ff01-f630-485d-b78d-7faff4bf303a")]
    public class FrameworkElementAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.AutomationPeer
    {
        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.UIElement Owner
        {
            get;
            private set;
        }

        internal FrameworkElementAutomationPeer() { }

        [FactoryMethodName("CreateInstanceWithOwner")]
        public FrameworkElementAutomationPeer(Microsoft.UI.Xaml.FrameworkElement owner) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Automation.Peers.AutomationPeer FromElement(Microsoft.UI.Xaml.UIElement element)
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Automation.Peers.AutomationPeer CreatePeerForElement(Microsoft.UI.Xaml.UIElement element)
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [InterfaceDetails(ClassGuid = "cc08bb80-0563-46b5-a869-d43fb90fa4da")]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollItemProvider))]
    internal class ScrollItemAdapter
     : Windows.Foundation.Object
    {
        internal ScrollItemAdapter() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [NativeName("CImageAutomationPeer")]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "0397b28f-e884-431b-b41d-2a2a4a9cb18c")]
    public class ImageAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ImageAutomationPeer(Microsoft.UI.Xaml.Controls.Image owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "6280834e-fc3f-45f8-9d23-460d8b11a23c")]
    public class FlyoutPresenterAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public FlyoutPresenterAutomationPeer(Microsoft.UI.Xaml.Controls.FlyoutPresenter owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IWindowProvider))]
    [Guids(ClassGuid = "251c97a9-f58e-4a45-a0b2-6183bb39c853")]
    internal class SplitViewPaneAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public SplitViewPaneAutomationPeer(Microsoft.UI.Xaml.FrameworkElement owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "a345b150-9825-4148-a617-ebfde91524f8")]
    public abstract class ButtonBaseAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        protected ButtonBaseAutomationPeer(Microsoft.UI.Xaml.Controls.Primitives.ButtonBase owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Guids(ClassGuid = "312f7f3c-d928-4cb3-8aad-60a1143346c7")]
    public class ButtonAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ButtonBaseAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ButtonAutomationPeer(Microsoft.UI.Xaml.Controls.Button owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Guids(ClassGuid = "265ea9b0-af30-4fc6-bdf7-01fc9127e793")]
    internal class ItemInvokeAdapter
     : Windows.Foundation.Object
    {
        internal ItemInvokeAdapter() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextAdapter")]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ITextProvider))]
    [Guids(ClassGuid = "5f84e62b-aebc-4977-a1ea-dac6ec4080ff")]
    internal class TextAdapter
     : Microsoft.UI.Xaml.DependencyObject
    {
        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.UIElement Owner
        {
            get;
            private set;
        }

        internal TextAdapter() { }

    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextRangeAdapter")]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider))]
    [Guids(ClassGuid = "ca080273-d417-49fa-ab2b-e43d1896b811")]
    internal class TextRangeAdapter
     : Microsoft.UI.Xaml.DependencyObject
    {
        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Automation.Peers.TextAdapter Owner
        {
            get;
            private set;
        }

        internal TextRangeAdapter() { }

    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Guids(ClassGuid = "c4d1f836-fe97-4225-8611-5f222893263c")]
    public class RepeatButtonAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ButtonBaseAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public RepeatButtonAutomationPeer(Microsoft.UI.Xaml.Controls.Primitives.RepeatButton owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Guids(ClassGuid = "99832b72-caf9-4614-974d-f67b147c2b52")]
    public class HyperlinkButtonAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ButtonBaseAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public HyperlinkButtonAutomationPeer(Microsoft.UI.Xaml.Controls.HyperlinkButton owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [NativeName("CHyperlinkAutomationPeer")]
    [Guids(ClassGuid = "cd7d8887-bf19-4f3d-914e-5beb4c2cc52e")]
    internal class HyperlinkAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.AutomationPeer
    {
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "9116a88c-8611-43a1-bd38-6560baa1b3e0")]
    public class ThumbAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ThumbAutomationPeer(Microsoft.UI.Xaml.Controls.Primitives.Thumb owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IToggleProvider))]
    [Guids(ClassGuid = "f69a2dda-ef1b-42a5-814c-80c1948fe9ac")]
    public class ToggleButtonAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ButtonBaseAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ToggleButtonAutomationPeer(Microsoft.UI.Xaml.Controls.Primitives.ToggleButton owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IToggleProvider))]
    [Guids(ClassGuid = "0bd1ab82-7f16-4343-af86-13fd20c429c7")]
    public class ToggleSwitchAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ToggleSwitchAutomationPeer(Microsoft.UI.Xaml.Controls.ToggleSwitch owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IToggleProvider))]
    [Guids(ClassGuid = "7270c021-1d02-4c41-81b8-f86dc819873b")]
    public class SemanticZoomAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public SemanticZoomAutomationPeer(Microsoft.UI.Xaml.Controls.SemanticZoom owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ISelectionItemProvider))]
    [Guids(ClassGuid = "9bd05c22-83bf-4398-9d50-21afc3d8943b")]
    public class RadioButtonAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ToggleButtonAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public RadioButtonAutomationPeer(Microsoft.UI.Xaml.Controls.RadioButton owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "9558e255-6447-49e1-8b43-b76811db3d2e")]
    public class CheckBoxAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ToggleButtonAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public CheckBoxAutomationPeer(Microsoft.UI.Xaml.Controls.CheckBox owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IRangeValueProvider))]
    [Guids(ClassGuid = "c0f86879-19bd-413d-a36a-58796b3bfe96")]
    public class RangeBaseAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public RangeBaseAutomationPeer(Microsoft.UI.Xaml.Controls.Primitives.RangeBase owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "c28c3ad5-91d8-45e1-b7ef-8f0441e9aa57")]
    public class SliderAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.RangeBaseAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public SliderAutomationPeer(Microsoft.UI.Xaml.Controls.Slider owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "12d88b0f-b386-4e3a-9904-95d7639c9d86")]
    public class ScrollBarAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.RangeBaseAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ScrollBarAutomationPeer(Microsoft.UI.Xaml.Controls.Primitives.ScrollBar owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IItemContainerProvider))]
    [Guids(ClassGuid = "576ca77e-c5b4-4f6e-a0fb-faff9ab60f82")]
    public class ItemsControlAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        internal ItemsControlAutomationPeer() { }

        [FactoryMethodName("CreateInstanceWithOwner")]
        public ItemsControlAutomationPeer(Microsoft.UI.Xaml.Controls.ItemsControl owner)
            : base(owner) { }

        [MethodFlags(IsImplVirtual = true)]
        protected virtual Microsoft.UI.Xaml.Automation.Peers.ItemAutomationPeer OnCreateItemAutomationPeer(Windows.Foundation.Object item)
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.ItemAutomationPeer);
        }

        public Microsoft.UI.Xaml.Automation.Peers.ItemAutomationPeer CreateItemAutomationPeer(Windows.Foundation.Object item)
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.ItemAutomationPeer);
        }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ISelectionProvider))]
    [Guids(ClassGuid = "91e5180d-2140-4ba3-aca2-fdd1528ca86c")]
    public class SelectorAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ItemsControlAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public SelectorAutomationPeer(Microsoft.UI.Xaml.Controls.Primitives.Selector owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "0beb2e66-cad8-46a1-a3ba-ba0cc03d90a8")]
    public class ListBoxAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.SelectorAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ListBoxAutomationPeer(Microsoft.UI.Xaml.Controls.ListBox owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IDropTargetProvider))]
    [Guids(ClassGuid = "5c1870f6-55bf-4dfb-bf08-18545e8bb6e5")]
    public class ListViewBaseAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.SelectorAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ListViewBaseAutomationPeer(Microsoft.UI.Xaml.Controls.ListViewBase owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "b5696491-0054-49ca-9265-b0f1a871f3c8")]
    public class ListViewAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ListViewBaseAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ListViewAutomationPeer(Microsoft.UI.Xaml.Controls.ListView owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "067eab2b-4443-45a8-8b21-5d9f1b889000")]
    public class GridViewAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ListViewBaseAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public GridViewAutomationPeer(Microsoft.UI.Xaml.Controls.GridView owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "7a88fc54-9e47-4916-a01d-dce76d8b4327")]
    public class FlipViewAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.SelectorAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public FlipViewAutomationPeer(Microsoft.UI.Xaml.Controls.FlipView owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollProvider))]
    [Guids(ClassGuid = "7d94c164-bbe5-4a8d-bab9-ef09d775b6d0")]
    public class ScrollViewerAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ScrollViewerAutomationPeer(Microsoft.UI.Xaml.Controls.ScrollViewer owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IVirtualizedItemProvider))]
    [Guids(ClassGuid = "6ab81293-c043-4a99-8320-5dbcc9e0538b")]
    public class ItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.AutomationPeer
    {
        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [ReadOnly]
        public Windows.Foundation.Object Item
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Automation.Peers.ItemsControlAutomationPeer ItemsControlAutomationPeer
        {
            get;
            private set;
        }

        protected internal ItemAutomationPeer() { }

        [FactoryMethodName("CreateInstanceWithParentAndItem")]
        public ItemAutomationPeer(Windows.Foundation.Object item, Microsoft.UI.Xaml.Automation.Peers.ItemsControlAutomationPeer parent) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ISelectionItemProvider))]
    [Guids(ClassGuid = "86c414b2-30a2-4f9b-9128-3929688e406b")]
    public class SelectorItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ItemAutomationPeer
    {
        internal SelectorItemAutomationPeer() { }

        [FactoryMethodName("CreateInstanceWithParentAndItem")]
        public SelectorItemAutomationPeer(Windows.Foundation.Object item, Microsoft.UI.Xaml.Automation.Peers.SelectorAutomationPeer parent)
            : base(item, parent) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollItemProvider))]
    [Guids(ClassGuid = "7112957c-92dd-4c2d-bc9a-c97241d1bebd")]
    public class ListBoxItemDataAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.SelectorItemAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithParentAndItem")]
        public ListBoxItemDataAutomationPeer(Windows.Foundation.Object item, Microsoft.UI.Xaml.Automation.Peers.ListBoxAutomationPeer parent)
            : base(item, parent) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "539c1b86-b3fa-4925-959b-24c082d261a8")]
    public class ListBoxItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ListBoxItemAutomationPeer(Microsoft.UI.Xaml.Controls.ListBoxItem owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(IsHiddenFromIdl = true)]
    [Guids(ClassGuid = "f875aa65-e76c-4e07-b0b5-0063b058838a")]
    public abstract class ListViewBaseItemDataAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.SelectorItemAutomationPeer
    {
        internal ListViewBaseItemDataAutomationPeer() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(IsHiddenFromIdl = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IDragProvider))]
    [Guids(ClassGuid = "33968b7b-b3f5-491a-9f3a-1d0ccd473498")]
    public abstract class ListViewBaseItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        internal ListViewBaseItemAutomationPeer() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollItemProvider))]
    [Guids(ClassGuid = "4c51cee4-4727-496f-a874-b602f06f038d")]
    public class ListViewItemDataAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ListViewBaseItemDataAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithParentAndItem")]
        public ListViewItemDataAutomationPeer(Windows.Foundation.Object item, Microsoft.UI.Xaml.Automation.Peers.ListViewBaseAutomationPeer parent) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "cfa5a4a2-6f23-4f0b-8185-81d8ce109657")]
    public class ListViewItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ListViewBaseItemAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ListViewItemAutomationPeer(Microsoft.UI.Xaml.Controls.ListViewItem owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollItemProvider))]
    [Guids(ClassGuid = "ae334930-a2b9-405b-bcc5-b5125ea9bd6a")]
    public class GridViewItemDataAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ListViewBaseItemDataAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithParentAndItem")]
        public GridViewItemDataAutomationPeer(Windows.Foundation.Object item, Microsoft.UI.Xaml.Automation.Peers.GridViewAutomationPeer parent) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "1c899221-46cc-4b07-a132-58cef41cfdda")]
    public class GridViewItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ListViewBaseItemAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public GridViewItemAutomationPeer(Microsoft.UI.Xaml.Controls.GridViewItem owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollItemProvider))]
    [Guids(ClassGuid = "956bd5ca-6eaf-4070-945c-013e9bf4ab58")]
    public class FlipViewItemDataAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.SelectorItemAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithParentAndItem")]
        public FlipViewItemDataAutomationPeer(Windows.Foundation.Object item, Microsoft.UI.Xaml.Automation.Peers.FlipViewAutomationPeer parent)
            : base(item, parent) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "c44a8983-2e9d-429e-936a-60dbc0e30886")]
    public class FlipViewItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public FlipViewItemAutomationPeer(Microsoft.UI.Xaml.Controls.FlipViewItem owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "1e405b56-86bb-4432-90bc-41975a3234fa")]
    public abstract class ListViewBaseHeaderItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ListViewBaseHeaderItemAutomationPeer(Microsoft.UI.Xaml.Controls.ListViewBaseHeaderItem owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "ba409b44-abb4-4c7c-9d7e-d5a4ffe3686e")]
    public class ListViewHeaderItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ListViewBaseHeaderItemAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ListViewHeaderItemAutomationPeer(Microsoft.UI.Xaml.Controls.ListViewHeaderItem owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "ac3aaeef-98c8-4d9e-96dd-226d32c4effb")]
    public class GridViewHeaderItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ListViewBaseHeaderItemAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public GridViewHeaderItemAutomationPeer(Microsoft.UI.Xaml.Controls.GridViewHeaderItem owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "00d3e6be-7169-49ff-9a85-21c2165501f1")]
    public class GroupItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public GroupItemAutomationPeer(Microsoft.UI.Xaml.Controls.GroupItem owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "0e7dcefc-e407-4044-9413-caa9590e92b7")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 3)]
    public class MediaPlayerElementAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public MediaPlayerElementAutomationPeer(Microsoft.UI.Xaml.Controls.MediaPlayerElement owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "06fb3327-9162-4802-8063-a1612704d879")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 3)]
    public class MediaTransportControlsAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public MediaTransportControlsAutomationPeer(Microsoft.UI.Xaml.Controls.MediaTransportControls owner)
            : base(owner) { }
    }


    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IWindowProvider))]
    [Guids(ClassGuid = "cd91cced-8f16-4d49-89c8-b38a9c0a1b85")]
    internal class PopupAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        internal PopupAutomationPeer() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Guids(ClassGuid = "8f475d5e-29c6-4c6f-b1e4-e36ed78315b2")]
    internal class PopupRootAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        internal PopupRootAutomationPeer() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "18058394-033b-4141-9274-380c3f4a874f")]
    internal class ToolTipAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        internal ToolTipAutomationPeer() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "60b7733c-ff56-4e00-89b2-19320369b918")]
    public class DatePickerAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public DatePickerAutomationPeer(Microsoft.UI.Xaml.Controls.DatePicker owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "4cd84693-f5b3-4828-9ee2-9b245edc99f4")]
    public class TimePickerAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public TimePickerAutomationPeer(Microsoft.UI.Xaml.Controls.TimePicker owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Guids(ClassGuid = "00d6dd23-24db-438d-b922-8354af6ab305")]
    internal sealed class AppBarLightDismissAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {

    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "ba2a8a84-c38a-4b8d-85f5-1f8fec968668")]
    internal sealed class FaceplateContentPresenterAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {

    }


    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Guids(ClassGuid = "41c74c09-09c8-4c78-bbab-cd8f741801be")]
    internal class SplitViewLightDismissAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public SplitViewLightDismissAutomationPeer(Microsoft.UI.Xaml.FrameworkElement owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ITableProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IGridProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IValueProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ISelectionProvider))]
    [Guids(ClassGuid = "e7b4451c-60f9-4c6b-a59b-1040b5d71eb9")]
    internal sealed class CalendarViewAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {

    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IGridItemProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollItemProvider))]
    [Guids(ClassGuid = "a6b27b0c-930c-4d87-907d-a05d8def72b6")]
    internal abstract class CalendarViewBaseItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {

    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ITableItemProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ISelectionItemProvider))]
    [Guids(ClassGuid = "58ac696d-b5c8-42b1-b3cb-202daaa0c326")]
    internal sealed class CalendarViewDayItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.CalendarViewBaseItemAutomationPeer
    {

    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ITableItemProvider))]
    [Guids(ClassGuid = "8748f38a-c361-4921-88b0-4d1b97d5961c")]
    internal sealed class CalendarViewItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.CalendarViewBaseItemAutomationPeer
    {

    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "18f3437f-531d-4fe6-b28e-bdbf10b47126")]
    internal sealed class CalendarScrollViewerAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.ScrollViewerAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public CalendarScrollViewerAutomationPeer(Microsoft.UI.Xaml.Controls.ScrollViewer owner)
            : base(owner)
        { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "3917e780-b9c0-4847-b411-c21d42c4b7db")]
    internal sealed class CalendarViewHeaderAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.AutomationPeer
    {
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "6876369e-b050-4631-a186-e25889f31d80")]
    internal sealed class LandmarkTargetAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {

    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "5926dc72-5f90-4037-82ba-8cbe07608a9e")]
    internal sealed class NamedContainerAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {

    }

    [NativeName("APAutomationControlType")]
    [NativeCategory(EnumCategory.AutomationEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true, NativeUsesNumericValues = false)]
    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 4)]
    [NativeValueNamespace("UIAXcp")]
    public enum AutomationControlType
    {
        [NativeValueName("ACTButton")]
        Button,
        [NativeValueName("ACTCalendar")]
        Calendar,
        [NativeValueName("ACTCheckBox")]
        CheckBox,
        [NativeValueName("ACTComboBox")]
        ComboBox,
        [NativeValueName("ACTEdit")]
        Edit,
        [NativeValueName("ACTHyperlink")]
        Hyperlink,
        [NativeValueName("ACTImage")]
        Image,
        [NativeValueName("ACTListItem")]
        ListItem,
        [NativeValueName("ACTList")]
        List,
        [NativeValueName("ACTMenu")]
        Menu,
        [NativeValueName("ACTMenuBar")]
        MenuBar,
        [NativeValueName("ACTMenuItem")]
        MenuItem,
        [NativeValueName("ACTProgressBar")]
        ProgressBar,
        [NativeValueName("ACTRadioButton")]
        RadioButton,
        [NativeValueName("ACTScrollBar")]
        ScrollBar,
        [NativeValueName("ACTSlider")]
        Slider,
        [NativeValueName("ACTSpinner")]
        Spinner,
        [NativeValueName("ACTStatusBar")]
        StatusBar,
        [NativeValueName("ACTTab")]
        Tab,
        [NativeValueName("ACTTabItem")]
        TabItem,
        [NativeValueName("ACTText")]
        Text,
        [NativeValueName("ACTToolBar")]
        ToolBar,
        [NativeValueName("ACTToolTip")]
        ToolTip,
        [NativeValueName("ACTTree")]
        Tree,
        [NativeValueName("ACTTreeItem")]
        TreeItem,
        [NativeValueName("ACTCustom")]
        Custom,
        [NativeValueName("ACTGroup")]
        Group,
        [NativeValueName("ACTThumb")]
        Thumb,
        [NativeValueName("ACTDataGrid")]
        DataGrid,
        [NativeValueName("ACTDataItem")]
        DataItem,
        [NativeValueName("ACTDocument")]
        Document,
        [NativeValueName("ACTSplitButton")]
        SplitButton,
        [NativeValueName("ACTWindow")]
        Window,
        [NativeValueName("ACTPane")]
        Pane,
        [NativeValueName("ACTHeader")]
        Header,
        [NativeValueName("ACTHeaderItem")]
        HeaderItem,
        [NativeValueName("ACTTable")]
        Table,
        [NativeValueName("ACTTitleBar")]
        TitleBar,
        [NativeValueName("ACTSeparator")]
        Separator,
        [NativeValueName("ACTSemanticZoom")]
        SemanticZoom,
        [NativeValueName("ACTAppBar")]
        AppBar,
        [NativeValueName("ACTFlipView")]
        [Version(2)]
        FlipView,
    }

    [NativeName("APAutomationEvents")]
    [NativeCategory(EnumCategory.AutomationEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true, NativeUsesNumericValues = false)]
    [NativeValueNamespace("UIAXcp")]
    public enum AutomationEvents
    {
        [NativeValueName("AEToolTipOpened")]
        ToolTipOpened,
        [NativeValueName("AEToolTipClosed")]
        ToolTipClosed,
        [NativeValueName("AEMenuOpened")]
        MenuOpened,
        [NativeValueName("AEMenuClosed")]
        MenuClosed,
        [NativeValueName("AEAutomationFocusChanged")]
        AutomationFocusChanged,
        [NativeValueName("AEInvokePatternOnInvoked")]
        InvokePatternOnInvoked,
        [NativeValueName("AESelectionItemPatternOnElementAddedToSelection")]
        SelectionItemPatternOnElementAddedToSelection,
        [NativeValueName("AESelectionItemPatternOnElementRemovedFromSelection")]
        SelectionItemPatternOnElementRemovedFromSelection,
        [NativeValueName("AESelectionItemPatternOnElementSelected")]
        SelectionItemPatternOnElementSelected,
        [NativeValueName("AESelectionPatternOnInvalidated")]
        SelectionPatternOnInvalidated,
        [NativeValueName("AETextPatternOnTextSelectionChanged")]
        TextPatternOnTextSelectionChanged,
        [NativeValueName("AETextPatternOnTextChanged")]
        TextPatternOnTextChanged,
        [NativeValueName("AEAsyncContentLoaded")]
        AsyncContentLoaded,
        [NativeValueName("AEPropertyChanged")]
        PropertyChanged,
        [NativeValueName("AEStructureChanged")]
        StructureChanged,
        [NativeValueName("AEDragStart")]
        DragStart,
        [NativeValueName("AEDragCancel")]
        DragCancel,
        [NativeValueName("AEDragComplete")]
        DragComplete,
        [NativeValueName("AEDragEnter")]
        DragEnter,
        [NativeValueName("AEDragLeave")]
        DragLeave,
        [NativeValueName("AEDropped")]
        Dropped,
        [NativeValueName("AELiveRegionChanged")]
        LiveRegionChanged,
        [NativeValueName("AEInputReachedTarget")]
        InputReachedTarget,
        [NativeValueName("AEInputReachedOtherElement")]
        InputReachedOtherElement,
        [NativeValueName("AEInputDiscarded")]
        InputDiscarded,
        [NativeValueName("AEWindowClosed")]
        WindowClosed,
        [NativeValueName("AEWindowOpened")]
        WindowOpened,
        [NativeValueName("AEConversionTargetChanged")]
        ConversionTargetChanged,
        [NativeValueName("AETextEditTextChanged")]
        TextEditTextChanged,
        [NativeValueName("AELayoutInvalidated")]
        LayoutInvalidated,
    }

    [NativeName("APPatternInterface")]
    [NativeCategory(EnumCategory.AutomationEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true, NativeUsesNumericValues = false)]
    [NativeValueNamespace("UIAXcp")]
    public enum PatternInterface
    {
        [NativeValueName("PIInvoke")]
        Invoke,
        [NativeValueName("PISelection")]
        Selection,
        [NativeValueName("PIValue")]
        Value,
        [NativeValueName("PIRangeValue")]
        RangeValue,
        [NativeValueName("PIScroll")]
        Scroll,
        [NativeValueName("PIScrollItem")]
        ScrollItem,
        [NativeValueName("PIExpandCollapse")]
        ExpandCollapse,
        [NativeValueName("PIGrid")]
        Grid,
        [NativeValueName("PIGridItem")]
        GridItem,
        [NativeValueName("PIMultipleView")]
        MultipleView,
        [NativeValueName("PIWindow")]
        Window,
        [NativeValueName("PISelectionItem")]
        SelectionItem,
        [NativeValueName("PIDock")]
        Dock,
        [NativeValueName("PITable")]
        Table,
        [NativeValueName("PITableItem")]
        TableItem,
        [NativeValueName("PIToggle")]
        Toggle,
        [NativeValueName("PITransform")]
        Transform,
        [NativeValueName("PIText")]
        Text,
        [NativeValueName("PIItemContainer")]
        ItemContainer,
        [NativeValueName("PIVirtualizedItem")]
        VirtualizedItem,
        [NativeValueName("PIText2")]
        Text2,
        [NativeValueName("PITextChild")]
        TextChild,
        [NativeValueName("PITextRange")]
        TextRange,
        [NativeValueName("PIAnnotation")]
        Annotation,
        [NativeValueName("PIDrag")]
        Drag,
        [NativeValueName("PIDropTarget")]
        DropTarget,
        [NativeValueName("PIObjectModel")]
        ObjectModel,
        [NativeValueName("PISpreadsheet")]
        Spreadsheet,
        [NativeValueName("PISpreadsheetItem")]
        SpreadsheetItem,
        [NativeValueName("PIStyles")]
        Styles,
        [NativeValueName("PITransform2")]
        Transform2,
        [NativeValueName("PISynchronizedInput")]
        SynchronizedInput,
        [NativeValueName("PITextEdit")]
        TextEdit,
        [NativeValueName("PICustomNavigation")]
        CustomNavigation,
    }

    [NativeName("OrientationType")]
    [NativeCategory(EnumCategory.AutomationCoreEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true)]
    public enum AutomationOrientation
    {
        [NativeValueName("OrientationType_None")]
        None = 0,
        [NativeValueName("OrientationType_Horizontal")]
        Horizontal = 1,
        [NativeValueName("OrientationType_Vertical")]
        Vertical = 2,
    }

    [NativeName("LiveSetting")]
    [NativeCategory(EnumCategory.AutomationCoreEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true)]
    public enum AutomationLiveSetting
    {
        [NativeValueName("LiveSetting_Off")]
        Off = 0,
        [NativeValueName("LiveSetting_Polite")]
        Polite = 1,
        [NativeValueName("LiveSetting_Assertive")]
        Assertive = 2,
    }

    [NativeName("AccessibilityView")]
    [NativeCategory(EnumCategory.AutomationCoreEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true)]
    public enum AccessibilityView
    {
        [NativeValueName("AccessibilityView_Raw")]
        Raw = 0,
        [NativeValueName("AccessibilityView_Control")]
        Control = 1,
        [NativeValueName("AccessibilityView_Content")]
        Content = 2,
    }

    [NativeName("AutomationStructureChangeType")]
    [NativeCategory(EnumCategory.AutomationCoreEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true)]
    public enum AutomationStructureChangeType
    {
        [NativeValueName("AutomationStructureChangeType_ChildAdded")]
        ChildAdded = 0,
        [NativeValueName("AutomationStructureChangeType_ChildRemoved")]
        ChildRemoved = 1,
        [NativeValueName("AutomationStructureChangeType_ChildrenInvalidated")]
        ChildrenInvalidated = 2,
        [NativeValueName("AutomationStructureChangeType_ChildrenBulkAdded")]
        ChildrenBulkAdded = 3,
        [NativeValueName("AutomationStructureChangeType_ChildrenBulkRemoved")]
        ChildrenBulkRemoved = 4,
        [NativeValueName("AutomationStructureChangeType_ChildrenReordered")]
        ChildrenReordered = 5,
    }

    [NativeName("AutomationNavigationDirection")]
    [NativeCategory(EnumCategory.AutomationCoreEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true)]
    public enum AutomationNavigationDirection
    {
        [NativeValueName("AutomationNavigationDirection_Parent")]
        Parent = 0,
        [NativeValueName("AutomationNavigationDirection_NextSibling")]
        NextSibling = 1,
        [NativeValueName("AutomationNavigationDirection_PreviousSibling")]
        PreviousSibling = 2,
        [NativeValueName("AutomationNavigationDirection_FirstChild")]
        FirstChild = 3,
        [NativeValueName("AutomationNavigationDirection_LastChild")]
        LastChild = 4,
    }

    [NativeName("AutomationLandmarkType")]
    [NativeCategory(EnumCategory.AutomationCoreEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true)]
    public enum AutomationLandmarkType
    {
        [NativeValueName("AutomationLandmarkType_None")]
        None = 0,
        [NativeValueName("AutomationLandmarkType_Custom")]
        Custom = 1,
        [NativeValueName("AutomationLandmarkType_Form")]
        Form = 2,
        [NativeValueName("AutomationLandmarkType_Main")]
        Main = 3,
        [NativeValueName("AutomationLandmarkType_Navigation")]
        Navigation = 4,
        [NativeValueName("AutomationLandmarkType_Search")]
        Search = 5,
    }

    [NativeName("AutomationNotificationKind")]
    [NativeCategory(EnumCategory.AutomationCoreEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true)]
    public enum AutomationNotificationKind
    {
        [NativeValueName("AutomationNotificationKind_ItemAdded")]
        ItemAdded = 0,
        [NativeValueName("AutomationNotificationKind_ItemRemoved")]
        ItemRemoved = 1,
        [NativeValueName("AutomationNotificationKind_ActionCompleted")]
        ActionCompleted = 2,
        [NativeValueName("AutomationNotificationKind_ActionAborted")]
        ActionAborted = 3,
        [NativeValueName("AutomationNotificationKind_Other")]
        Other = 4,
    }

    [NativeName("AutomationNotificationProcessing")]
    [NativeCategory(EnumCategory.AutomationCoreEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true)]
    public enum AutomationNotificationProcessing
    {
        [NativeValueName("AutomationNotificationProcessing_ImportantAll")]
        ImportantAll = 0,
        [NativeValueName("AutomationNotificationProcessing_ImportantMostRecent")]
        ImportantMostRecent = 1,
        [NativeValueName("AutomationNotificationProcessing_All")]
        All = 2,
        [NativeValueName("AutomationNotificationProcessing_MostRecent")]
        MostRecent = 3,
        [NativeValueName("AutomationNotificationProcessing_CurrentThenMostRecent")]
        CurrentThenMostRecent = 4,
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    public struct RawElementProviderRuntimeId
    {
        public Windows.Foundation.UInt32 Part1
        {
            get;
            set;
        }

        public Windows.Foundation.UInt32 Part2
        {
            get;
            set;
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CAutomationPeerAnnotationCollection")]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "d240b3db-b699-4331-845f-4b067aefd5f7")]
    public sealed class AutomationPeerAnnotationCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<AutomationPeerAnnotation>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Automation.Peers.AutomationPeerAnnotation ContentProperty
        {
            get;
            set;
        }

        internal AutomationPeerAnnotationCollection() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "974d1c99-cc45-4681-984e-d6cdc98539e2")]
    public sealed class AutomationPeerAnnotation
     : Microsoft.UI.Xaml.DependencyObject
    {
        public Microsoft.UI.Xaml.Automation.AnnotationType Type
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Automation.Peers.AutomationPeer Peer
        {
            get;
            set;
        }

        public AutomationPeerAnnotation() { }

        public AutomationPeerAnnotation(Microsoft.UI.Xaml.Automation.AnnotationType type) { }

        [FactoryMethodName("CreateWithPeerParameter")]
        public AutomationPeerAnnotation(Microsoft.UI.Xaml.Automation.AnnotationType type, Microsoft.UI.Xaml.Automation.Peers.AutomationPeer peer) { }
    }

    [NativeName("AutomationHeadingLevel")]
    [NativeCategory(EnumCategory.AutomationCoreEnum)]
    [EnumFlags(HasTypeConverter = true, IsTypeConverter = true)]
    public enum AutomationHeadingLevel
    {
        [NativeValueName("AutomationHeadingLevel_None")]
        None = 0,
        [NativeValueName("AutomationHeadingLevel_HeadingLevel1")]
        Level1 = 1,
        [NativeValueName("AutomationHeadingLevel_HeadingLevel2")]
        Level2 = 2,
        [NativeValueName("AutomationHeadingLevel_HeadingLevel3")]
        Level3 = 3,
        [NativeValueName("AutomationHeadingLevel_HeadingLevel4")]
        Level4 = 4,
        [NativeValueName("AutomationHeadingLevel_HeadingLevel5")]
        Level5 = 5,
        [NativeValueName("AutomationHeadingLevel_HeadingLevel6")]
        Level6 = 6,
        [NativeValueName("AutomationHeadingLevel_HeadingLevel7")]
        Level7 = 7,
        [NativeValueName("AutomationHeadingLevel_HeadingLevel8")]
        Level8 = 8,
        [NativeValueName("AutomationHeadingLevel_HeadingLevel9")]
        Level9 = 9,
    }
}
