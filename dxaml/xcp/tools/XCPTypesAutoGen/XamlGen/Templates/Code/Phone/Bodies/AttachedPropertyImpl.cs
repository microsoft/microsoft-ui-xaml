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
namespace XamlGen.Templates.Code.Phone.Bodies
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
    public partial class AttachedPropertyImpl : CppCodeGenerator<AttachedPropertyDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<InitializeProperty>(Model)));
            this.Write("\r\n\r\n");
 if (Model.Modifier == Modifier.Public) { 
            this.Write("IFACEMETHODIMP ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringClass.GeneratedFactoryName));
            this.Write("::get_");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("Property(_Outptr_ ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyProperty")));
            this.Write("** ppValue)\r\n{\r\n    HRESULT hr = S_OK;\r\n    \r\n    ARG_VALIDRETURNPOINTER(ppValue)" +
                    ";\r\n    \r\n    IFC(s_");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("Property.CopyTo(ppValue));\r\n\r\nCleanup:\r\n    RRETURN(hr);\r\n}\r\n\r\n");
      if (!Model.IsReadOnly) { 
            this.Write("IFACEMETHODIMP ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringClass.GeneratedFactoryName));
            this.Write("::Set");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.TargetType.AnnotatedAbiParameterName)));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.AnnotatedAbiParameterName)));
            this.Write(")\r\n{\r\n    RRETURN(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringClass.GeneratedFactoryName));
            this.Write("::Set");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("Static(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.TargetType.AbiParameterName));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.AbiParameterName)));
            this.Write("));\r\n}\r\n");
      } 
            this.Write("\r\nIFACEMETHODIMP ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringClass.GeneratedFactoryName));
            this.Write("::Get");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.TargetType.AnnotatedAbiParameterName)));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.AnnotatedAbiReturnParameterName)));
            this.Write(")\r\n{\r\n    RRETURN(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringClass.GeneratedFactoryName));
            this.Write("::Get");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("Static(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.TargetType.AbiParameterName));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.AbiReturnParameterName)));
            this.Write("));\r\n}\r\n");
 } 
            this.Write("\r\n");
 if (!Model.IsReadOnly) { 
            this.Write("_Check_return_ HRESULT\r\n");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringClass.GeneratedFactoryName));
            this.Write("::Set");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("Static(");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.TargetType.AnnotatedAbiParameterName)));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.AnnotatedAbiParameterName)));
            this.Write(")\r\n{\r\n");
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<PropertySetterBody>(Model)));
            this.Write("\r\n}\r\n\r\n");
 } 
            this.Write("_Check_return_ HRESULT\r\n");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringClass.GeneratedFactoryName));
            this.Write("::Get");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("Static(");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.TargetType.AnnotatedAbiParameterName)));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.AnnotatedAbiReturnParameterName)));
            this.Write(")\r\n{\r\n");
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<PropertyGetterBody>(Model)));
            this.Write("\r\n}\r\n");
            return this.GenerationEnvironment.ToString();
        }
    }
}