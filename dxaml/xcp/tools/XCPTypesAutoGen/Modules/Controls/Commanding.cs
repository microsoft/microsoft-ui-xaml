// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Input
{
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Implements(typeof(Microsoft.UI.Xaml.Input.ICommand))]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "78459458-687e-45d3-b3aa-6e4706934bb7")]
    public class XamlUICommand : Microsoft.UI.Xaml.DependencyObject
    {
        public XamlUICommand() { }
        
        public Windows.Foundation.String Label { get; set; }
        
        public Microsoft.UI.Xaml.Controls.IconSource IconSource { get; set; }
        
        [CollectionType(CollectionKind.Vector)]
        [PropertyFlags(IsValueCreatedOnDemand = true, IsReadOnlyExceptForParser = true, NeedsInvoke = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Input.KeyboardAcceleratorCollection KeyboardAccelerators { get; set; }
        
        public Windows.Foundation.String AccessKey { get; set; }
        
        public Windows.Foundation.String Description { get; set; }
        
        public Microsoft.UI.Xaml.Input.ICommand Command { get; set; }
        
        public void NotifyCanExecuteChanged() { }
        
        public event Windows.Foundation.TypedEventHandler<XamlUICommand, ExecuteRequestedEventArgs> ExecuteRequested;
        
        public event Windows.Foundation.TypedEventHandler<XamlUICommand, CanExecuteRequestedEventArgs> CanExecuteRequested;
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [FrameworkTypePattern]
    public enum StandardUICommandKind
    {
        None,
        Cut,
        Copy,
        Paste,
        SelectAll,
        Delete,
        Share,
        Save,
        Open,
        Close,
        Pause,
        Play,
        Stop,
        Forward,
        Backward,
        Undo,
        Redo,
    }
    
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "0eeac8b5-7bf9-463f-9d3e-be8475935cc6")]
    public class StandardUICommand : XamlUICommand
    {
        public StandardUICommand() { }

        [FactoryMethodName("CreateInstanceWithKind")]
        public StandardUICommand(StandardUICommandKind kind) { }
        
        [DependencyPropertyModifier(Modifier.Internal)]
        public StandardUICommandKind Kind { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.DependencyProperty KindProperty { get; }
    }
    
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Velocity(Feature = "Feature_CommandingImprovements")]
    [Platform("Feature_CommandingImprovements", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "a12d302e-122a-4bb7-aa2a-5b96fa200ffe")]
    [FrameworkTypePattern]
    public sealed class CanExecuteRequestedEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        internal CanExecuteRequestedEventArgs() { }
        
        public Windows.Foundation.Object Parameter { get; private set; }

        [VelocityFeature("Feature_CommandingImprovements")]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject CommandTarget { get; private set; }
        
        [VelocityFeature("Feature_CommandingImprovements")]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.ItemsControl ListCommandTarget { get; private set; }
        
        public Windows.Foundation.Boolean CanExecute { get; set; }
    }
    
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Velocity(Feature = "Feature_CommandingImprovements")]
    [Platform("Feature_CommandingImprovements", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "5c7fe9ed-651e-49c3-8ef3-1caba3c7a38a")]
    [FrameworkTypePattern]
    public sealed class ExecuteRequestedEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        internal ExecuteRequestedEventArgs() { }
        
        public Windows.Foundation.Object Parameter { get; private set; }

        [VelocityFeature("Feature_CommandingImprovements")]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject CommandTarget { get; private set; }
        
        [VelocityFeature("Feature_CommandingImprovements")]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.ItemsControl ListCommandTarget { get; private set; }
    }
}

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "be8b22e5-d70f-4b16-90c8-2e34e0edafa0")]
    [VelocityFeature("Feature_CommandingImprovements")]
    public class CommandingContainer : Microsoft.UI.Xaml.Controls.ContentPresenter
    {
        public CommandingContainer() { }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject CommandingTarget { get; set; }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static CommandingContainer AttachedCommandingContainer { get; set; }

        public static void NotifyContextChanged(Microsoft.UI.Xaml.DependencyObject commandTarget) { }
        
        public event Windows.Foundation.TypedEventHandler<CommandingContainer, CommandingContextChangedEventArgs> ContextChanged;
    }
    
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "7cd71dcd-18e6-4c6f-b068-96f1e1f9e345")]
    [VelocityFeature("Feature_CommandingImprovements")]
    [FrameworkTypePattern]
    public sealed class CommandingContextChangedEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        internal CommandingContextChangedEventArgs() { }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject CommandTarget { get; private set; }
        
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.ItemsControl ListCommandTarget { get; private set; }
    }
}
