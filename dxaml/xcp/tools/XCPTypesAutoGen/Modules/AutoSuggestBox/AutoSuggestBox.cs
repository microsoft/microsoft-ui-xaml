// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;
using Microsoft.UI.Xaml.Markup;
using XamlOM.NewBuilders;

namespace Microsoft.UI.Xaml.Controls
{
    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Platform("Feature_HeaderPlacement", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Platform("Feature_InputValidation", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "d7fdbb3a-ed1a-4f96-91da-6666402a8971")]
    [PropertyChange(PropertyChangeCallbackType.OnPropertyChangeCallback)]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInputValidationControl), Velocity = "Feature_InputValidation")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInputValidationControl2), Velocity = "Feature_InputValidation")]
    [InputProperty("Text")]
    public sealed class AutoSuggestBox
        : Microsoft.UI.Xaml.Controls.ItemsControl
    {
        #region Properties

        // Version 1
        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.Double MaxSuggestionListHeight { get; set; }

        public Windows.Foundation.Boolean IsSuggestionListOpen { get; set; }

        public Windows.Foundation.String TextMemberPath { get; set; }

        public Windows.Foundation.String Text { get; set; }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.Boolean UpdateTextOnSelect { get; set; }

        public Windows.Foundation.String PlaceholderText { get; set; }

        public Windows.Foundation.Object Header { get; set; }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.Boolean AutoMaximizeSuggestionArea { get; set; }

        public Microsoft.UI.Xaml.Style TextBoxStyle { get; set; }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.IconElement QueryIcon { get; set; }

        public Microsoft.UI.Xaml.Controls.LightDismissOverlayMode LightDismissOverlayMode { get; set; }

        public object Description { get; set; }

        // Velocity
        [VelocityFeature("Feature_HeaderPlacement")]
        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Controls.ControlHeaderPlacement HeaderPlacement { get; set; }

        #endregion

        #region Events

        // Version 1
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.AutoSuggestBoxSuggestionChosenEventHandler SuggestionChosen;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.AutoSuggestBoxTextChangedEventHandler TextChanged;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.AutoSuggestBoxQuerySubmittedEventHandler QuerySubmitted;

        #endregion
    }

    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum AutoSuggestionBoxTextChangeReason
    {
        UserInput,
        ProgrammaticChange,
        SuggestionChosen
    }
    
    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "360f0fab-8c97-430b-824e-db279240f68c")]
    public sealed class AutoSuggestBoxTextChangedEventArgs
        : Microsoft.UI.Xaml.DependencyObject
    {
        public Microsoft.UI.Xaml.Controls.AutoSuggestionBoxTextChangeReason Reason
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean CheckCurrent()
        {
            return default(Windows.Foundation.Boolean);
        }
    }

    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "7482a1ba-7977-4d3e-b079-89b7962a1fe7")]
    public sealed class AutoSuggestBoxSuggestionChosenEventArgs
        : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.Object SelectedItem
        {
            get;
            internal set;
        }
    }
    
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "130d2448-ab5e-410a-9e2d-684778cecd44")]
    public sealed class AutoSuggestBoxQuerySubmittedEventArgs
        : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.String QueryText
        {
            get;
            internal set;
        }
        
        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.Object ChosenSuggestion
        {
            get;
            internal set;
        }
    }

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void AutoSuggestBoxSuggestionChosenEventHandler(Microsoft.UI.Xaml.Controls.AutoSuggestBox sender, Microsoft.UI.Xaml.Controls.AutoSuggestBoxSuggestionChosenEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void AutoSuggestBoxTextChangedEventHandler(Microsoft.UI.Xaml.Controls.AutoSuggestBox sender, Microsoft.UI.Xaml.Controls.AutoSuggestBoxTextChangedEventArgs e);
    
    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void AutoSuggestBoxQuerySubmittedEventHandler(Microsoft.UI.Xaml.Controls.AutoSuggestBox sender, Microsoft.UI.Xaml.Controls.AutoSuggestBoxQuerySubmittedEventArgs e);

}
