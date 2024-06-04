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
        
        public string ObjectCast(string destinationType, string sourceName)
        {
            if (ProjectInfo.UsingCSWinRT)
            {
                return $"global::WinRT.CastExtensions.As<{Globalize(destinationType)}>({sourceName})";
            }
            else
            {
                return $"({destinationType}){sourceName}";
            }
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
