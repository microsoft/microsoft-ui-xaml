// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal class BindingInfoDefinition : Definition
    {
        public BindingInfoDefinition(XamlProjectInfo projectInfo, XamlSchemaCodeInfo schemaInfo)
            : base(projectInfo, schemaInfo)
        {
        }

        internal Dictionary<string, XamlType> ObservableVectorTypes { get; set; }
        internal Dictionary<string, XamlType> ObservableMapTypes { get; set; }
        internal Dictionary<string, XamlMember> BindingSetters { get; set; }
        internal bool EventBindingUsed { get; set; }
    }
}
