// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal class VB_CodeGenerator<T> : ManagedCodeGenerator<T>
    {
        public override string ToStringWithCulture(ICodeGenOutput codegenOutput)
        {
            return codegenOutput.VBName();
        }

        public override string ToStringWithCulture(XamlType type)
        {
            return type.VBName();
        }

        public override string ToStringWithCulture(bool value)
        {
            return value ? "True" : "False";
        }

        public string Globalize(string fullType)
        {
            return $"Global.{fullType}";
        }

        public string PropertyChangedEventArgName(BindPathStep step)
        {
            if (!step.ValueType.ImplementsINotifyPropertyChanged())
            {
                throw new System.InvalidOperationException($"{step.ValueType.Name} doesn't implement INotifyPropertyChanged");
            }

            if (step.ValueType.ImplementsXamlINotifyPropertyChanged())
            {
                return Globalize($"{KnownNamespaces.XamlData}.PropertyChangedEventArgs");
            }
            else
            {
                return Globalize("System.ComponentModel.PropertyChangedEventArgs");
            }
        }

        public string INPCInterfaceName(BindPathStep step)
        {
            if (!step.ValueType.ImplementsINotifyPropertyChanged())
            {
                throw new System.InvalidOperationException($"{step.ValueType.Name} doesn't implement INotifyPropertyChanged");
            }

            if (step.ValueType.ImplementsXamlINotifyPropertyChanged())
            {
                return Globalize(KnownTypes.XamlINotifyPropertyChanged);
            }
            else
            {
                return Globalize(KnownTypes.INotifyPropertyChanged);
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
                return Globalize($"{KnownNamespaces.XamlData}.DataErrorsChangedEventArgs");
            }
            else
            {
                return Globalize("System.ComponentModel.DataErrorsChangedEventArgs");
            }
        }

        public string INDEIInterfaceName(BindPathStep step)
        {
            if (!step.ValueType.ImplementsINotifyDataErrorInfo())
            {
                throw new System.InvalidOperationException($"{step.ValueType.Name} doesn't implement INotifyDataErrorInfo");
            }

            if (step.ValueType.ImplementsXamlINotifyDataErrorInfo())
            {
                return Globalize(KnownTypes.XamlINotifyDataErrorInfo);
            }
            else
            {
                return Globalize(KnownTypes.INotifyDataErrorInfo);
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

        public string INCCInterfaceName(BindPathStep step)
        {
            if (!step.ValueType.ImplementsINotifyCollectionChanged())
            {
                throw new System.InvalidOperationException($"{step.ValueType.Name} doesn't implement INotifyCollectionChanged");
            }

            if (step.ValueType.ImplementsXamlINotifyCollectionChanged())
            {
                return Globalize(KnownTypes.XamlINotifyCollectionChanged);
            }
            else
            {
                return Globalize(KnownTypes.INotifyCollectionChanged);
            }
        }

        public string GeneratedCodeAttribute
        {
            get { return $"<Global.System.CodeDom.Compiler.GeneratedCodeAttribute(\"{KnownStrings.XAMLBuildTaskAsmName}\", \" {XamlCompilerVersion}\")>  _"; }
        }

        public string DebuggerNonUserCodeAttribute
        {
            get { return "<Global.System.Diagnostics.DebuggerNonUserCodeAttribute()>  _"; }
        }

        public string NotCLSCompliantAttribute
        {
            get
            {
                return ProjectInfo.IsCLSCompliant ? "<Global.System.CLSCompliant(False)> " : "";
            }
        }

        protected override string GetPhaseCondition(BindPathStep bindStep)
        {
            StringBuilder condition = new StringBuilder();
            condition.Append("(phase And (NOT_PHASED");
            if (bindStep.IsTrackingSource)
            {
                condition.Append(" Or DATA_CHANGED");
            }
            foreach(int phase in bindStep.DistinctPhases)
            { 
                condition.AppendFormat(" Or (1 << {0})", phase);
            } 
            condition.Append(")) <> 0");
            return condition.ToString();
        }

        public override string GetDirectPhaseCondition(int phase, bool isTracking)
        {
            StringBuilder condition = new StringBuilder();
            condition.AppendFormat("(phase And ((1 << {0}) Or NOT_PHASED ", phase);
            if (isTracking)
            {
                condition.Append("Or DATA_CHANGED");
            }
            condition.Append(")) <> 0");
            return condition.ToString();
        }
    }
}
