// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Reflection;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal abstract class CodeGenerator<T> : T4Base<T>
    {
        protected virtual string GetPhaseCondition(BindPathStep bindStep)
        {
            StringBuilder condition = new StringBuilder();
            condition.Append("(phase & (NOT_PHASED");
            if (bindStep.IsTrackingSource)
            {
                condition.Append(" | DATA_CHANGED");
            }
            foreach (int phase in bindStep.DistinctPhases)
            {
                condition.AppendFormat(" | (1 << {0})", phase);
            }

            condition.Append(")) != 0");
            return condition.ToString();
        }

        public virtual string GetDirectPhaseCondition(int phase, bool isTracking)
        {
            StringBuilder condition = new StringBuilder();
            condition.AppendFormat("(phase & ((1 << {0}) | NOT_PHASED ", phase);
            if (isTracking)
            {
                condition.Append("| DATA_CHANGED");
            }
            condition.Append(")) != 0");
            return condition.ToString();
        }
    }

    internal abstract class ManagedCodeGenerator<T> : CodeGenerator<T>
    {
        private static string xamlCompilerVersion;

        public static string XamlCompilerVersion
        {
           get
           {
                if (string.IsNullOrEmpty(xamlCompilerVersion))
                {
                    var version = Assembly.GetExecutingAssembly().GetName().Version;
                    xamlCompilerVersion = version.ToString();
                }
                return xamlCompilerVersion;
            }
        }

        public string WeakReferenceTypeName
        {
            get
            {
                return "System.WeakReference";
            }
        }
    }
}
