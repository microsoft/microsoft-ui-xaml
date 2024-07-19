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
namespace XamlGen.Templates.Code.Framework.Bodies
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
    public partial class FieldSetter : CppCodeGenerator<PropertyDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write("    HRESULT hr = S_OK;\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<CheckAPICall>(Model)));
            this.Write("\r\n");
 if (Model.PropertyType.IdlInfo.Type.IsStringType) { 
            this.Write("    IFC(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FrameworkFieldName));
            this.Write(".Set(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.PropertyType.AbiParameterName));
            this.Write("));\r\n");
 } else if (Model.PropertyType.Type.IsTypeNameType) { 
            this.Write("    IFC(CValueBoxer::CopyValue(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.PropertyType.AbiParameterName));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FrameworkFieldName));
            this.Write(".ReleaseAndGetAddressOf()));\r\n");
 } else if (Model.PropertyType.IsValueType) { 
            this.Write("    IFC(CValueBoxer::CopyValue(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.PropertyType.AbiParameterName));
            this.Write(", &");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FrameworkFieldName));
            this.Write("));\r\n");
 } else if (Model.XamlPropertyFlags.UseComPtr) { 
            this.Write("    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FrameworkFieldName));
            this.Write(" = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.PropertyType.AbiParameterName));
            this.Write(";\r\n");
 } else { 
            this.Write("    SetPtrValue(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FrameworkFieldName));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.PropertyType.AbiParameterName));
            this.Write(");\r\n");
 }
   if (!Model.AllowCrossThreadAccess || !Model.XamlPropertyFlags.UseComPtr) { 
            this.Write("Cleanup:\r\n");
 } 
            this.Write("    RRETURN(hr);");
            return this.GenerationEnvironment.ToString();
        }
    }
}
