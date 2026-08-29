// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Xaml;
using System.Linq;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal class CSharp_CodeGenerator<T> : ManagedCodeGenerator<T>
    {
        public override string ToStringWithCulture(ICodeGenOutput codegenOutput)
        {
            return codegenOutput.CSharpName();
        }

        public override string ToStringWithCulture(XamlType type)
        {
            return type.CSharpName();
        }

        public string Globalize(string fullType)
        {
            const string globalPrefix = "global::";
            if (!fullType.StartsWith(globalPrefix))
            {
                return $"{globalPrefix}{fullType}";
            }
            else
            {
                return fullType;
            }
        }

        public string GeneratedCodeAttribute
        {
            get { return $"[global::System.CodeDom.Compiler.GeneratedCodeAttribute(\"{KnownStrings.XAMLBuildTaskAsmName}\",\" {XamlCompilerVersion}\")]"; }
        }

        public string DebuggerNonUserCodeAttribute
        {
            get { return "[global::System.Diagnostics.DebuggerNonUserCodeAttribute()]"; }
        }

        public string OverloadAttribute
        {
            get { return $"[global::Windows.Foundation.Metadata.DefaultOverload]"; }
        }

        public string PrependNamespace (string objectType)
        {
            if(ProjectInfo.UsingCSWinRT)
            {
                return $"System.ComponentModel.{objectType}";
            }
            else
            {
                return $"Microsoft.UI.Xaml.Data.{objectType}";
            }
        }

        public string ObjectCast(XamlType type, string sourceName)
        {
            // We can't just do 'ToString()' on the input type, as that would return the XAML type name, which is
            // the fully qualified type name for the type, and not the actual type expression that is valid to use
            // in C# code. Most of the time, the two are the same. However, they don't match if the type is, for
            // instance, a nested type. That is, the former would say 'Foo+Bar', and not 'Foo.Bar'. This would
            // result in invalid generated code if XAML code is using eg. 'x:DataType' with a nested template type.
            // To address this, we just use 'ToStringWithCulture', which produces the valid C# type expression.
            if (ProjectInfo.UsingCSWinRT)
            {
                return $"global::WinRT.CastExtensions.As<{ToStringWithCulture(type)}>({sourceName})";
            }

            return $"({ToStringWithCulture(type)}){sourceName}";
        }

        public string ObjectCast(string destinationType, string sourceName)
        {
            if (ProjectInfo.UsingCSWinRT)
            {
                // We want to allow callers to pass either globally qualified type names, or normal
                // type names. We never want to produce invalid code because of repeated globals.
                return $"global::WinRT.CastExtensions.As<{Globalize(destinationType)}>({sourceName})";
            }

            return $"({Globalize(destinationType)}){sourceName}";
        }

        public string NotCLSCompliantAttribute
        {
            get
            {
                return ProjectInfo.IsCLSCompliant ? "[global::System.CLSCompliant(false)] " : "";
            }
        }

        public string INPCInterfaceName(BindPathStep step)
        {
            var valueType = step.ValueType;
            if (step is FunctionStep)
            {
                valueType = step.Children.Single(bp => bp.ImplementsINPC).ValueType;
            }
            if (!valueType.ImplementsINotifyPropertyChanged())
            {
                throw new System.InvalidOperationException($"{valueType.Name} doesn't implement INotifyPropertyChanged");
            }

            if (valueType.ImplementsXamlINotifyPropertyChanged())
            {
                return Globalize(KnownTypes.XamlINotifyPropertyChanged);
            }
            else
            {
                return Globalize(KnownTypes.INotifyPropertyChanged);
            }
        }

        public string PropertyChangedEventArgName(BindPathStep step)
        {
            if (!step.ValueType.ImplementsINotifyPropertyChanged())
            {
                throw new System.InvalidOperationException($"{step.ValueType.Name} doesn't implement INotifyPropertyChanged");
            }

            if (step.ValueType.ImplementsXamlINotifyPropertyChanged())
            {
                return Globalize("Microsoft.UI.Xaml.Data.PropertyChangedEventArgs");
            }
            else
            {
                return Globalize("System.ComponentModel.PropertyChangedEventArgs");
            }
        }

        public string INDEIInterfaceName(BindPathStep step)
        {
            var valueType = step.ValueType;
            if (step is FunctionStep)
            {
                valueType = step.Children.Single(bp => bp.ImplementsINDEI).ValueType;
            }
            if (!valueType.ImplementsINotifyDataErrorInfo())
            {
                throw new System.InvalidOperationException($"{valueType.Name} doesn't implement INotifyDataErrorInfo");
            }

            if (valueType.ImplementsXamlINotifyDataErrorInfo())
            {
                return Globalize(KnownTypes.XamlINotifyDataErrorInfo);
            }
            else
            {
                return Globalize(KnownTypes.INotifyDataErrorInfo);
            }
        }

        public string DataErrorsEventArgName(BindPathStep step)
        {
            if (!step.ValueType.ImplementsINotifyDataErrorInfo())
            {
                throw new System.InvalidOperationException($"{step.ValueType.Name} doesn't implement INotifyDataErrorInfo");
            }

            if (step.ValueType.ImplementsXamlINotifyDataErrorInfo())
            {
                return Globalize("Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs");
            }
            else
            {
                return Globalize("System.ComponentModel.DataErrorsChangedEventArgs");
            }
        }

        public string INCCInterfaceName(BindPathStep step)
        {
            var valueType = step.ValueType;
            if (step is FunctionStep)
            {
                valueType = step.Children.Single(bp => bp.ImplementsINPC).ValueType;
            }
            if (!valueType.ImplementsINotifyCollectionChanged())
            {
                throw new System.InvalidOperationException($"{valueType.Name} doesn't implement INotifyCollectionChanged");
            }

            if (valueType.ImplementsXamlINotifyCollectionChanged())
            {
                return Globalize(KnownTypes.XamlINotifyCollectionChanged);
            }
            else
            {
                return Globalize(KnownTypes.INotifyCollectionChanged);
            }
        }

        public string NotifyCollectionChangedEventArgName(BindPathStep step)
        {
            if (!step.ValueType.ImplementsINotifyCollectionChanged())
            {
                throw new System.InvalidOperationException($"{step.ValueType.Name} doesn't implement INotifyCollectionChanged");
            }

            if (step.ValueType.ImplementsXamlINotifyCollectionChanged())
            {
                return Globalize("Microsoft.UI.Xaml.Interop.NotifyCollectionChangedEventArgs");
            }
            else
            {
                return Globalize("System.Collections.Specialized.NotifyCollectionChangedEventArgs");
            }
        }
    }
}
