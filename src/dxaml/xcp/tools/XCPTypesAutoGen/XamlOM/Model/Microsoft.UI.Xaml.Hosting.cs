// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;
using System;

namespace Microsoft.UI.Xaml.Hosting
{
    [Platform(typeof(PrivateApiContract), 1)]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [NativeName("CXamlIslandRoot")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "b3d608be-c816-469b-b645-9679b55717c7")]
    [ThreadingModel(ThreadingModel.Both)]
    public sealed class XamlIslandRoot
        : Microsoft.UI.Xaml.Controls.Panel
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public UIElement Content
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [DelegateToCore]
        public void SetScreenOffsetOverride(Windows.Foundation.Point offsetOnScreen) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [DelegateToCore]
        public bool TrySetFocus()
        {
             return default(bool);
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [DelegateToCore]
        public object FocusController
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        public static Microsoft.UI.Xaml.Hosting.XamlIslandRoot GetIslandFromElement(Microsoft.UI.Xaml.DependencyObject element) { return default(Microsoft.UI.Xaml.Hosting.XamlIslandRoot); }
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1, ForcePrimaryInterfaceGeneration = true)]
    [Guids(ClassGuid = "fbc87d10-1b92-4ee6-8109-0ebc3d640852")]
    [DXamlIdlGroup("Controls2")]
    public static class ElementCompositionPreview
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReturnTypeParameterName("result")]
        public static Microsoft.UI.Composition.Visual GetElementVisual(Microsoft.UI.Xaml.UIElement element)
        {
            return default(Microsoft.UI.Composition.Visual);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReturnTypeParameterName("result")]
        public static Microsoft.UI.Composition.Visual GetElementChildVisual(Microsoft.UI.Xaml.UIElement element)
        {
            return default(Microsoft.UI.Composition.Visual);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetElementChildVisual(Microsoft.UI.Xaml.UIElement element, [Optional] Microsoft.UI.Composition.Visual visual)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReturnTypeParameterName("result")]
        public static Microsoft.UI.Composition.CompositionPropertySet GetScrollViewerManipulationPropertySet(Microsoft.UI.Xaml.Controls.ScrollViewer scrollViewer)
        {
            return default(Microsoft.UI.Composition.CompositionPropertySet);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetImplicitShowAnimation(Microsoft.UI.Xaml.UIElement element, [Optional] Microsoft.UI.Composition.ICompositionAnimationBase animation)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetImplicitHideAnimation(Microsoft.UI.Xaml.UIElement element, [Optional] Microsoft.UI.Composition.ICompositionAnimationBase animation)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void SetIsTranslationEnabled(Microsoft.UI.Xaml.UIElement element, Windows.Foundation.Boolean value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReturnTypeParameterName("result")]
        public static Microsoft.UI.Composition.CompositionPropertySet GetPointerPositionPropertySet(UIElement targetElement)
        {
            return default(Microsoft.UI.Composition.CompositionPropertySet);
        }
    }

    [Contract(typeof(WinUIContract), 5)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [DXamlIdlGroup("coretypes2")]
    [HandWritten]
    public enum XamlSourceFocusNavigationReason
    {
        //  Programmatic focus setting
        Programmatic = 0,

        // Focus restoration for task switching
        Restore = 1,

        // Semantic (non-Cartesian), bidirectional navigation (e.g. Tab/Shift-Tab)
        First = 3,
        Last = 4,

        // Cartesian four-directional navigation (e.g. D-pad, keyboard arrow keys)
        Left = 7,
        Up = 8,
        Right = 9,
        Down = 10,
    };

    [Contract(typeof(WinUIContract), 5)]
    [NativeName("CNavigationFocusRequest")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "9ac12b98-d130-43a2-b20d-13516945bbbd")]
    [FrameworkTypePattern]
    [ThreadingModel(ThreadingModel.Both)]
    public sealed class XamlSourceFocusNavigationRequest
    {
        [FactoryMethodName("CreateInstance")]
        public XamlSourceFocusNavigationRequest(XamlSourceFocusNavigationReason reason)
        {
        }

        [FactoryMethodName("CreateInstanceWithHintRect")]
        public XamlSourceFocusNavigationRequest(XamlSourceFocusNavigationReason reason,
            Windows.Foundation.Rect hintRect)
        {
        }

        [FactoryMethodName("CreateInstanceWithHintRectAndCorrelationId")]
        public XamlSourceFocusNavigationRequest(XamlSourceFocusNavigationReason reason,
            Windows.Foundation.Rect hintRect,
            Windows.Foundation.Guid correlationId)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public XamlSourceFocusNavigationReason Reason
        {
            get
            {
                return default(XamlSourceFocusNavigationReason);
            }
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Rect HintRect
        {
            get
            {
                return default(Windows.Foundation.Rect);
            }
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Guid CorrelationId
        {
            get
            {
                return default(Windows.Foundation.Guid);
            }
        }
    };

    [Contract(typeof(WinUIContract), 5)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("coretypes2")]
    [FrameworkTypePattern]
    [HandWritten]
    public sealed class DesktopWindowXamlSourceGotFocusEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        internal DesktopWindowXamlSourceGotFocusEventArgs() { }

        public XamlSourceFocusNavigationRequest Request
        {
            get
            {
                return default(XamlSourceFocusNavigationRequest);
            }
        }
    }

    [Contract(typeof(WinUIContract), 5)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("coretypes2")]
    [FrameworkTypePattern]
    [HandWritten]
    public sealed class DesktopWindowXamlSourceTakeFocusRequestedEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        internal DesktopWindowXamlSourceTakeFocusRequestedEventArgs() { }

        public XamlSourceFocusNavigationRequest Request
        {
            get
            {
                return default(XamlSourceFocusNavigationRequest);
            }
        }
    }

    [Contract(typeof(WinUIContract), 5)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("coretypes2")]
    [FrameworkTypePattern]
    [HandWritten]
    public sealed class XamlSourceFocusNavigationResult
    {
        public XamlSourceFocusNavigationResult(bool focusMoved) { }

        public bool WasFocusMoved
        {
            get
            {
                return default(bool);
            }
        }
    };

    [Contract(typeof(WinUIContract), 5)]
    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 6)]
    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "e6336955-51ed-4e64-9067-506501e80451")]
    [ThreadingModel(ThreadingModel.Both)]
    [Implements(typeof(Windows.Foundation.IClosable))]
    [Implements(typeof(Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop), Version = 1)]
    public class DesktopWindowXamlSource
    {
        public DesktopWindowXamlSource()
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
        public bool HasFocus
        {
            get
            {
                return default(bool);
            }
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [RequiresMultipleAssociationCheck]
        public Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [Version(2)]
        public bool ShouldConstrainPopupsToWorkArea
        {
            get;
            set;
        }

        public XamlSourceFocusNavigationResult NavigateFocus(XamlSourceFocusNavigationRequest request)
        {
            return default(XamlSourceFocusNavigationResult);
        }

        [EventFlags(IsImplVirtual = false)]
        public event Windows.Foundation.TypedEventHandler<DesktopWindowXamlSource, DesktopWindowXamlSourceTakeFocusRequestedEventArgs> TakeFocusRequested;

        [EventFlags(IsImplVirtual = false)]
        public event Windows.Foundation.TypedEventHandler<DesktopWindowXamlSource, DesktopWindowXamlSourceGotFocusEventArgs> GotFocus;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Initialize(Microsoft.UI.WindowId parentWindowId)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromDXaml = true)]
        public Microsoft.UI.Content.DesktopChildSiteBridge SiteBridge
        {
            get;
        }
    }

    [Contract(typeof(WinUIContract), 6)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "c21339b3-e8f2-42b1-b5fc-335b78e66879")]
    public sealed class XamlShutdownCompletedOnThreadEventArgs
    {
        internal XamlShutdownCompletedOnThreadEventArgs() { }

        public Windows.Foundation.Deferral GetDispatcherQueueDeferral() { return null; }
    }

    [Contract(typeof(WinUIContract), 5, ForcePrimaryInterfaceGeneration = true)]
    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 6)]
    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "3b953d5c-2c9f-4e49-88a7-c030616de1f0")]
    [ThreadingModel(ThreadingModel.Both)]
    [Implements(typeof(Windows.Foundation.IClosable))]
    public sealed class WindowsXamlManager
    {
        internal WindowsXamlManager() {}

        public static WindowsXamlManager InitializeForCurrentThread()
        {
            return default(WindowsXamlManager);
        }

        [Version(2)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Windows.Foundation.TypedEventHandler<WindowsXamlManager, XamlShutdownCompletedOnThreadEventArgs> XamlShutdownCompletedOnThread;

        [Version(2)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [OptionalReturnValue]
	    public static WindowsXamlManager GetForCurrentThread()
        {
            return default(WindowsXamlManager);
        }
    }
}
