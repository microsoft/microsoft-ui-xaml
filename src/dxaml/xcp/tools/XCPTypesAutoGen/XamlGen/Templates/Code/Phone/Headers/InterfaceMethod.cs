// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 15.0.0.0
//  
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
// ------------------------------------------------------------------------------
namespace XamlGen.Templates.Code.Phone.Headers
{
    using System.Linq;
    using System.Text;
    using System.Collections.Generic;
    using OM;
    using System;
    
    /// <summary>
    /// Class to produce the template output
    /// </summary>
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "15.0.0.0")]
    public partial class InterfaceMethod : CppCodeGenerator<MethodDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write("IFACEMETHOD(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.IsVirtual ? Model.IdlMethodInfo.VirtualName : Model.IdlMethodInfo.Name));
            this.Write(")(");
            this.Write(this.ToStringHelper.ToStringWithCulture(GetParameterListAsString(Model.Parameters, Model.ReturnType)));
            this.Write(");\r\nvirtual _Check_return_ HRESULT ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.ImplName));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(GetParameterListAsString(Model.Parameters, Model.ReturnType)));
            this.Write(");\r\n");
            return this.GenerationEnvironment.ToString();
        }
    }
}
