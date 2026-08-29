// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Reflection;
    using Lmr;
    using Utilities;

    internal class DirectUISystem
    {
        private List<DirectUIAssembly> systemAssemblies;
        private List<XamlTypeUniverse> xamlTypeUniverses;
        private List<DirectUIAssembly> platformAssemblies;

        private Lazy<Type> _Void;
        private Lazy<Type> _Object;
        private Lazy<Type> _String;
        private Lazy<Type> _Double;
        private Lazy<Type> _Int32;
        private Lazy<Type> _Boolean;
        private Lazy<Type> _Nullable;
        private Lazy<Type> _IReference;
        private Lazy<Type> _Type;
        private Lazy<Type> _frameworkTemplate;
        private Lazy<Type> _dataTemplate;
        private Lazy<Type> _controlTemplate;
        private Lazy<Type> _dependencyObject;
        private Lazy<Type> _dependencyProperty;
        private Lazy<Type> _frameworkElement;
        private Lazy<Type> _style;
        private Lazy<Type> _IComponentConnector;
        private Lazy<Type> _setter;
        private Lazy<Type> _binding;
        private Lazy<Type> _propertyPath;
        private Lazy<Type> _relativeSource;
        private Lazy<Type> _contentPropertyAttribute;
        private Lazy<Type> _inlineCollection;
        private Lazy<Type> _inline;
        private Lazy<Type> _lineBreak;
        private Lazy<Type> _uiElement;
        private Lazy<Type> _resourceDictionary;
        private Lazy<Type> _Deprecated;
        private Lazy<Type> _Delegate;
        private Lazy<Type> _flyoutBase;
        private Lazy<Type> _markupExtension;
        private Lazy<Type> _textBox;
        private Lazy<Type> _validationCommand;
        private Lazy<Type> _window;

        public DirectUISystem(IList<Assembly> referenceAssemblies)
        {
            this.LoadCoreDirectUIAssemblies(referenceAssemblies);

            this._Void = new Lazy<Type>(() => this.DirectUISystemGetType("System.Void"));
            this._Object = new Lazy<Type>(() => this.DirectUISystemGetType("System.Object"));
            this._String = new Lazy<Type>(() => this.DirectUISystemGetType("System.String"));
            this._Double = new Lazy<Type>(() => this.DirectUISystemGetType("System.Double"));
            this._Int32 = new Lazy<Type>(() => this.DirectUISystemGetType("System.Int32"));
            this._Boolean = new Lazy<Type>(() => this.DirectUISystemGetType("System.Boolean"));
            this._Nullable = new Lazy<Type>(() => this.DirectUISystemGetType("System.Nullable`1"));
            this._IReference = new Lazy<Type>(() => this.DirectUISystemGetType("Windows.Foundation.IReference`1", false)); // IReference is special, and may not be present for managed assemblies
            this._Type = new Lazy<Type>(() => this.DirectUISystemGetType("System.Type"));
            this._frameworkTemplate = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.FrameworkTemplate));
            this._dataTemplate = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.DataTemplate));
            this._controlTemplate = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.ControlTemplate));
            this._dependencyObject = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.DependencyObject));
            this._dependencyProperty = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.DependencyProperty));
            this._frameworkElement = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.FrameworkElement));
            this._style = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.Style));
            this._IComponentConnector = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.IComponentConnector));
            this._setter = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.Setter));
            this._binding = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.Binding));
            this._propertyPath = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.PropertyPath));
            this._relativeSource = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.RelativeSource));
            this._contentPropertyAttribute = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.ContentPropertyAttribute));
            this._inlineCollection = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.InlineCollection));
            this._inline = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.Inline));
            this._lineBreak = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.LineBreak));
            this._uiElement = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.UIElement));
            this._flyoutBase = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.FlyoutBase));
            this._resourceDictionary = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.ResourceDictionary));
            this._markupExtension = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.MarkupExtension));
            this._textBox = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.TextBox));
            this._Deprecated = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.DeprecatedAttribute));
            this._Delegate = new Lazy<Type>(() => this.DirectUISystemGetType("System.Delegate"));
            this._validationCommand = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.InputValidationCommand));
            this._window = new Lazy<Type>(() => this.DirectUISystemGetType(KnownTypes.Window));
        }

        internal Type Void { get { return this._Void.Value; } }
        internal Type Object { get { return this._Object.Value; } }
        internal Type String { get { return this._String.Value; } }
        internal Type Double { get { return this._Double.Value; } }
        internal Type Int32 { get { return this._Int32.Value; } }
        internal Type Boolean { get { return this._Boolean.Value; } }
        internal Type Nullable { get { return this._Nullable.Value; } }
        internal Type IReference { get { return this._IReference.Value; } }
        internal Type Type { get { return this._Type.Value; } }
        internal Type FrameworkTemplate { get { return this._frameworkTemplate.Value; } }
        internal Type DataTemplate { get { return this._dataTemplate.Value; } }
        internal Type ControlTemplate { get { return this._controlTemplate.Value; } }
        internal Type DependencyObject { get { return this._dependencyObject.Value; } }
        internal Type DependencyProperty { get { return this._dependencyProperty.Value; } }
        internal Type FrameworkElement { get { return this._frameworkElement.Value; } }
        internal Type Style { get { return this._style.Value; } }
        internal Type IComponentConnector { get { return this._IComponentConnector.Value; } }
        internal Type Setter { get { return this._setter.Value; } }
        internal Type Binding { get { return this._binding.Value; } }
        internal Type PropertyPath { get { return this._propertyPath.Value; } }
        internal Type RelativeSource { get { return this._relativeSource.Value; } }
        internal Type ContentPropertyAttribute { get { return this._contentPropertyAttribute.Value; } }
        internal Type InlineCollection { get { return this._inlineCollection.Value; } }
        internal Type Inline { get { return this._inline.Value; } }
        internal Type LineBreak { get { return this._lineBreak.Value; } }
        internal Type UIElement { get { return this._uiElement.Value; } }
        internal Type ResourceDictionary { get { return this._resourceDictionary.Value; } }
        internal Type Deprecated { get { return this._Deprecated.Value; } }
        internal Type Delegate { get { return this._Delegate.Value; } }
        internal Type FlyoutBase {  get { return this._flyoutBase.Value; } }
        internal Type MarkupExtension { get { return this._markupExtension.Value; } }
        internal Type TextBox { get { return this._textBox.Value; } }
        internal Type ValidationCommand { get { return this._validationCommand.Value; } }
        internal Type Window { get { return this._window.Value; } }
        internal List<DirectUIAssembly> DirectUIBaseAssemblies
        {
            get
            {
                if (this.systemAssemblies == null)
                {
                    this.systemAssemblies = new List<DirectUIAssembly>();
                }
                return this.systemAssemblies;
            }
        }

        internal List<DirectUIAssembly> PlatformAssemblies
        {
            get
            {
                if (this.platformAssemblies == null)
                {
                    this.platformAssemblies = new List<DirectUIAssembly>();
                }
                return this.platformAssemblies;
            }
        }

        internal List<XamlTypeUniverse> XamlTypeUniverses
        {
            get
            {
                if (this.xamlTypeUniverses == null)
                {
                    this.xamlTypeUniverses = new List<XamlTypeUniverse>();
                    // findstr the Type Universe.
                    foreach (DirectUIAssembly duiAsm in this.PlatformAssemblies)
                    {
                        Assembly wrappedAssembly = duiAsm.WrappedAssembly;
                        System.Reflection.Adds.IAssembly2 asm2Interface = wrappedAssembly as System.Reflection.Adds.IAssembly2;
                        this.xamlTypeUniverses.Add(asm2Interface.TypeUniverse as XamlTypeUniverse);
                    }
                }
                return this.xamlTypeUniverses;
            }
        }

        internal Type DirectUISystemGetType(string typeName, bool mustExist = true)
        {
            Type type = null;
            foreach (DirectUIAssembly asm in this.DirectUIBaseAssemblies)
            {
                type = asm.GetType(typeName);
                if (type != null && type.IsPublic)
                {
                    break;
                }
            }

            if (type == null)
            {
                // If a system type is always expected to exist, assert here if it doesn't.
                // Some system types may not always be available, so we shouldn't assert if they aren't.
                // E.g. Windows.Foundation.IReference is only available in the metadata
                // for native projects, but in managed projects the Xaml compiler's logic may query for both the correct System.Nullable
                // and incorrect Windows.Foundation.IReference.
                Debug.Assert(!mustExist, $"Missing system type '{typeName}'");
            }

            return type;
        }

        private void LoadCoreDirectUIAssemblies(IList<Assembly> referenceAssemblies)
        {
            foreach (DirectUIAssembly asm in referenceAssemblies)
            {
                if (FileHelpers.IsPlatformAssembly(asm) || FileHelpers.IsWinUIAssembly(asm))
                {
                    PlatformAssemblies.Add(asm);
                    DirectUIBaseAssemblies.Add(asm);
                }
            }

            bool msCorLibFound = false;

            foreach (DirectUIAssembly asm in referenceAssemblies)
            {
                if (KS.EqIgnoreCase(KnownStrings.MsCorLib, asm.GetName().Name))
                {
                    this.DirectUIBaseAssemblies.Add(asm);
                    msCorLibFound = true;
                
                    break;
                }
            }

            // if mscorlib was not found in the reference assemblies then add the one running in the compler
            // This is needed for processing code types Int32, String, etc... when parsing C++ based XAML
            if (!msCorLibFound)
            {
                DirectUIAssembly mscorlib = DirectUIAssembly.Wrap(typeof(string).Assembly);
                this.DirectUIBaseAssemblies.Add(mscorlib);
            }
        }

        private DirectUIAssembly GetSystemAssembly(string assemblyName)
        {
            foreach (DirectUIAssembly asm in DirectUIBaseAssemblies)
            {
                if (KS.EqIgnoreCase(assemblyName, asm.BaseName))
                    return asm;
            }
            return null;
        }
    }
}
