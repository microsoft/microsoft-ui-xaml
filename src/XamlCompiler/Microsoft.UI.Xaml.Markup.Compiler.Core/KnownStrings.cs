// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    internal static class KnownNamespaceUris
    {
        public const string Wpf2006 = "http://schemas.microsoft.com/winfx/2006/xaml/presentation";
        public const string DirectUI2010 = "http://schemas.microsoft.com/windows/2010/directui";
    }

    internal static class KnownNamespaces
    {
        public const string DirectUI = "Microsoft.UI.Xaml.Markup.Compiler.DirectUI";
        public const string ProxyTypes = DirectUI + ".ProxyTypes";
        public const string SystemPrefix = "System.";
        public const string WindowsFoundation = "Windows.Foundation";
        public const string WindowsFoundationPrefix = "Windows.Foundation.";
        public const string WindowsFoundationCollections = "Windows.Foundation.Collections";
        public const string WindowsUI = "Windows.UI";
        public const string Text = "Windows.UI.Text";
        public const string Xaml = "Microsoft.UI.Xaml"; // change to Microsoft.UI.Xaml to compile using the MUX namespace
        public const string XamlAutomation = Xaml + ".Automation";
        public const string XamlAutomationPeers = XamlAutomation + ".Peers";
        public const string XamlAutomationProvider = XamlAutomation + ".Provider";
        public const string XamlAutomationText = XamlAutomation + ".Text";
        public const string XamlControls = Xaml + ".Controls";
        public const string XamlControlsPrimitives = XamlControls + ".Primitives";
        public const string WindowsXamlData = "Windows.UI.Xaml.Data"; // BindableAttribute lives here.
        public const string XamlData = Xaml + ".Data";
        public const string XamlDocuments = Xaml + ".Documents";
        public const string XamlInput = Xaml + ".Input";
        public const string WindowsXamlInterop = "Windows.UI.Xaml.Interop"; // Windows.UI.Xaml.Interop.TypeName and Windows.UI.Xaml.Interop.TypeKind are staying in that namespace.
        public const string XamlInterop = Xaml + ".Interop";
        public const string XamlMarkup = Xaml + ".Markup";
        public const string XamlMedia = Xaml + ".Media";
        public const string XamlMediaAnimation = XamlMedia + ".Animation";
        public const string XamlMediaImaging = XamlMedia + ".Imaging";
        public const string XamlMediaMedia3D = XamlMedia + ".Media3D";
        public const string XamlNavigation = Xaml + ".Navigation";
        public const string XamlResources = Xaml + ".Resources";
        public const string XamlShapes = Xaml + ".Shapes";
        public const string XamlThreading = Xaml + ".Threading";
        public const string XamlXamlTypeInfo = Xaml + ".XamlTypeInfo";
    }

    internal static class KnownTypes
    {
        // DataAnnotations
        public const string RequiredAttribute = "DataAnnotations.RequiredAttribute";

        // System
        public const string MulticastDelegate = "System.MulticastDelegate";
        public const string Nullable = "System.Nullable";
        public const string ObsoleteAttribute = "System.ObsoleteAttribute";
        public const string Uri = "System.Uri";

        // System.Collections
        public const string IList = "System.Collections.Generic.IList`1";
        public const string ICollection = "System.Collections.Generic.ICollection`1";
        public const string IDictionary = "System.Collections.Generic.IDictionary`2";

        // System.ComponentModel
        public const string INotifyPropertyChanged = "System.ComponentModel.INotifyPropertyChanged";
        public const string INotifyDataErrorInfo = "System.ComponentModel.INotifyDataErrorInfo";
        public const string INotifyCollectionChanged = "System.Collections.Specialized.INotifyCollectionChanged";

        // Windows.Foundation
        public const string IReference = "Windows.Foundation.IReference";

        // Windows.Foundation.Collections
        public const string IObservableVector = "Windows.Foundation.Collections.IObservableVector`1";
        public const string IObservableMap = "Windows.Foundation.Collections.IObservableMap`2";
        public const string IVector = "Windows.Foundation.Collections.IVector`1";
        public const string IMap = "Windows.Foundation.Collections.IMap`2";

        // Windows.Foundation.Metadata
        public const string ActivatableAttribute = "Windows.Foundation.Metadata.ActivatableAttribute";
        public const string CreateFromStringAttribute = "Windows.Foundation.Metadata.CreateFromStringAttribute";
        public const string ContractVersionAttribute = "Windows.Foundation.Metadata.ContractVersionAttribute";
        public const string DeprecatedAttribute = "Windows.Foundation.Metadata.DeprecatedAttribute";
        public const string ExperimentalAttribute = "Windows.Foundation.Metadata.ExperimentalAttribute";
        public const string ThreadingAttribute = "Windows.Foundation.Metadata.ThreadingAttribute";

        // Xaml
        public const string Application = KnownNamespaces.Xaml + ".Application";
        public const string FrameworkElement = KnownNamespaces.Xaml + ".FrameworkElement";
        public const string FrameworkTemplate = KnownNamespaces.Xaml + ".FrameworkTemplate";
        public const string DataContextChangedEventArgs = KnownNamespaces.Xaml + ".DataContextChangedEventArgs";
        public const string DataTemplate = KnownNamespaces.Xaml + ".DataTemplate";
        public const string DependencyObject = KnownNamespaces.Xaml + ".DependencyObject";
        public const string DependencyProperty = KnownNamespaces.Xaml + ".DependencyProperty";
        public const string IDataTemplateExtension = KnownNamespaces.Xaml + ".IDataTemplateExtension";
        public const string PropertyPath = KnownNamespaces.Xaml + ".PropertyPath";
        public const string ResourceDictionary = KnownNamespaces.Xaml + ".ResourceDictionary";
        public const string Style = KnownNamespaces.Xaml + ".Style";
        public const string Setter = KnownNamespaces.Xaml + ".Setter";
        public const string UIElement = KnownNamespaces.Xaml + ".UIElement";
        public const string Visibility = KnownNamespaces.Xaml + ".Visibility";
        public static string VisibilityColonized = KnownStrings.Colonize(Visibility);
        public const string WinUIContract = KnownNamespaces.Xaml + ".WinUIContract";
        public const string Window = KnownNamespaces.Xaml + ".Window";

        // Xaml.Controls
        public const string ContainerContentChangingEventArgs = KnownNamespaces.XamlControls + ".ContainerContentChangingEventArgs";
        public const string Control = KnownNamespaces.XamlControls + ".Control";
        public const string ControlTemplate = KnownNamespaces.XamlControls + ".ControlTemplate";
        public const string TextBox = KnownNamespaces.XamlControls + ".TextBox";
        public const string InputPropertyAttribute = KnownNamespaces.XamlControls + ".InputPropertyAttribute";
        public const string IInputValidationControl = KnownNamespaces.XamlControls + ".IInputValidationControl";
        public const string InputValidationContext = KnownNamespaces.XamlControls + ".InputValidationContext";
        public const string InputValidationCommand = KnownNamespaces.XamlControls + ".InputValidationCommand";
        public const string InputValidationError = KnownNamespaces.XamlControls + ".InputValidationError";
        public const string XamlControlsXamlMetaDataProvider = KnownNamespaces.XamlXamlTypeInfo + ".XamlControlsXamlMetaDataProvider";

        // Xaml.Controls.Primitives
        public const string ComponentResourceLocation = KnownNamespaces.XamlControlsPrimitives + ".ComponentResourceLocation";
        public const string FlyoutBase = KnownNamespaces.XamlControlsPrimitives + ".FlyoutBase";

        // Xaml.Data
        public const string XamlINotifyDataErrorInfo = KnownNamespaces.XamlData + ".INotifyDataErrorInfo";
        public const string XamlINotifyPropertyChanged = KnownNamespaces.XamlData + ".INotifyPropertyChanged";
        public const string WindowsBindableAttribute = KnownNamespaces.WindowsXamlData + ".BindableAttribute";
        public const string BindableAttribute = KnownNamespaces.XamlData + ".BindableAttribute";
        public const string Binding = KnownNamespaces.XamlData + ".Binding";
        public const string ICustomPropertyProvider = KnownNamespaces.XamlData + ".ICustomPropertyProvider";
        public const string IValueConverter = KnownNamespaces.XamlData + ".IValueConverter";
        public const string RelativeSource = KnownNamespaces.XamlData + ".RelativeSource";

        // Xaml.Documents
        public const string Inline = KnownNamespaces.XamlDocuments + ".Inline";
        public const string InlineCollection = KnownNamespaces.XamlDocuments + ".InlineCollection";
        public const string LineBreak = KnownNamespaces.XamlDocuments + ".LineBreak";

        // Xaml.Interop
        public const string XamlINotifyCollectionChanged = KnownNamespaces.XamlInterop + ".INotifyCollectionChanged";

        // Xaml.Markup
        public const string ContentPropertyAttribute = KnownNamespaces.XamlMarkup + ".ContentPropertyAttribute";
        public const string IComponentConnector = KnownNamespaces.XamlMarkup + ".IComponentConnector";
        public const string IDataTemplateComponent = KnownNamespaces.XamlMarkup + ".IDataTemplateComponent";
        public const string IXamlBindScopeDiagnostics = KnownNamespaces.XamlMarkup + ".IXamlBindScopeDiagnostics";
        public const string IXamlMember = KnownNamespaces.XamlMarkup + ".IXamlMember";
        public const string IXamlMetadataProvider = KnownNamespaces.XamlMarkup + ".IXamlMetadataProvider";
        public const string IXamlType = KnownNamespaces.XamlMarkup + ".IXamlType";
        public const string FullXamlMetadataProviderAttribute = KnownNamespaces.XamlMarkup + ".FullXamlMetadataProviderAttribute";
        public const string MarkupExtension = KnownNamespaces.XamlMarkup + ".MarkupExtension";
        public const string XamlBindingHelper = KnownNamespaces.XamlMarkup + ".XamlBindingHelper";
        public static string XamlBindingHelperColonized = KnownStrings.Colonize(XamlBindingHelper);
        public const string XamlMarkupHelper = KnownNamespaces.XamlMarkup + ".XamlMarkupHelper";
        public const string XamlReader = KnownNamespaces.XamlMarkup + ".XamlReader";
        public const string XmlnsDefinition = KnownNamespaces.XamlMarkup + ".XmlnsDefinition";
    }

    internal static class KnownStrings
    {
        public const string UriClrNamespace = "clr-namespace";
        public const string UriAssembly = "assembly";
        public const string DefaultHeaderExtension = ".h";
        public const string GeneratedHppExtension = ".g.hpp";
        public const string XamlExtension = ".xaml";
        public const string XbfExtension = ".xbf";
        public const string FrameworkTemplate = "FrameworkTemplate";
        public const string ResourceDictionary = "ResourceDictionary";
        public const string Source = "Source";
        public const string Library = "Library";
        public const string WinMdObj = "WinMdObj";
        public const string XAMLBuildTaskAsmName = "Microsoft.UI.Xaml.Markup.Compiler";
        public const string BackupSuffix = ".backup";
        public const string XamlBindingInfo = "XamlBindingInfo";
        public const string XamlTypeInfo = "XamlTypeInfo";
        public const string Get = "Get";
        public const string Set = "Set";
        public const string Debug = "Debug";
        public const string Converter = "Converter";
        public const string ConverterLanguage = "ConverterLanguage";
        public const string ConverterParameter = "ConverterParameter";
        public const string FallbackValue = "FallbackValue";
        public const string LostFocus = "LostFocus";
        public const string Mode = "Mode";
        public const string OneTime = "OneTime";
        public const string OneWay = "OneWay";
        public const string TwoWay = "TwoWay";
        public const string TargetNullValue = "TargetNullValue";
        public const string BindBack = "BindBack";
        public const string Path = "Path";
        public const string UpdateSourceTrigger = "UpdateSourceTrigger";
        public const string XColon = "x:";
        public const string op_Implicit = "op_Implicit";
        public const string op_Explicit = "op_Explicit";
        public const string UpdateParamName = "obj";
        public const string UpdateParamBindingsName = "bindings";
        public const string DataChanged = "DATA_CHANGED";
        public const string NotPhased = "NOT_PHASED";
        public const string DirectCast = "DirectCast";
        public const string CType = "CType";
        public const string Platforms = "Platforms";
        public const string UAP = "UAP";
        public const string UsingPrefix = "using:";
        public const string ClrNamespaceColon = "clr-namespace:";
        public const string SemiColonAssemblyEquals = ";assembly=";
        public const string DeprecatedAttributeDefaultMessage = "Deprecated";
        public const string ObsoleteAttributeDefaultMessage = "Obsolete";
        public const string PlatformAssemblySentinelType = "Windows.Foundation.HResult";
        public const string WinUIAssemblySentinelType = "Microsoft.UI.Xaml.DependencyObject";

        public const string MsCorLib = "mscorlib";

        public static String Colonize(string name)
        {
            return name.Replace(".", "::");
        }
    }

    internal static class KnownMembers
    {
        public const string Property = "Property";
        public const string Invoke = "Invoke";
        public const string Append = "Append";
        public const string Insert = "Insert";
        public const string Add = "Add";
        public const String Template = "Template";
        public const String TargetType = "TargetType";
        public const String Language = "Language";
        public const String Name = "Name";
        public const String Text = "Text";
    }

    /// <summary>
    /// String compare and formating class.
    /// </summary>
    internal static class KS
    {
        /// <summary>
        /// Standard String Compare operation.
        /// </summary>
        public static bool Eq(string a, string b)
        {
            return string.Equals(a, b, StringComparison.Ordinal);
        }

        /// <summary>
        /// Standard String Compare operation.  (ignore case)
        /// </summary>
        public static bool EqIgnoreCase(string a, string b)
        {
            return string.Equals(a, b, StringComparison.OrdinalIgnoreCase);
        }


        public static int IndexOf(string src, string chars)
        {
            return src.IndexOf(chars, StringComparison.Ordinal);
        }

        public static bool StartsWith(string src, string target)
        {
            return src.StartsWith(target, StringComparison.Ordinal);
        }

        public static bool StartsWithIgnoreCase(string src, string target)
        {
            return src.StartsWith(target, StringComparison.OrdinalIgnoreCase);
        }

        // for some reason, this is 8 times faster than "List.Contains(s)"
        public static bool ContainsString(IList<string> list, string s)
        {
            int cnt = list.Count;
            for (int i = 0; i < cnt; i++)
            {
                if (string.Equals(list[i], s, StringComparison.Ordinal))
                {
                    return true;
                }
            }
            return false;
        }
    }
}
