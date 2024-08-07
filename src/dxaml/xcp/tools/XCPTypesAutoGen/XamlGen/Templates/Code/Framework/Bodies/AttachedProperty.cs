﻿// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 16.0.0.0
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
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "16.0.0.0")]
    public partial class AttachedProperty : CppCodeGenerator<AttachedPropertyDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write("_Check_return_ HRESULT ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.DeclaringClass.GeneratedFactoryFullName)));
            this.Write("::");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GetterName));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.TargetType.AnnotatedAbiParameterName)));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.AnnotatedAbiReturnParameterName)));
            this.Write(")\r\n{\r\n");
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<AttachedPropertyGetterBody>(Model)));
            this.Write("\r\n}\r\n\r\n");
 if (!Model.IsReadOnly) { 
            this.Write("_Check_return_ HRESULT ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.DeclaringClass.GeneratedFactoryFullName)));
            this.Write("::");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.SetterName));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.TargetType.AnnotatedAbiParameterName)));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.AnnotatedAbiParameterName)));
            this.Write(")\r\n{\r\n");
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<AttachedPropertySetterBody>(Model)));
            this.Write("\r\n}\r\n\r\n");
 } 
            this.Write("\r\n");
 if (!Model.IdlDPInfo.IsExcluded) {
       if (Model.IsHandlePublic) { 
            this.Write("IFACEMETHODIMP ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.DeclaringClass.GeneratedFactoryFullName)));
            this.Write("::");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.HandleGetterName));
            this.Write("(_Out_ ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyProperty")));
            this.Write("** ppValue)\r\n{\r\n    RRETURN(MetadataAPI::GetIDependencyProperty(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.IndexName));
            this.Write(", ppValue));\r\n}\r\n\r\n");
     } 
            this.Write("\r\nIFACEMETHODIMP ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.DeclaringClass.GeneratedFactoryFullName)));
            this.Write("::");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.IdlAPInfo.GetterName));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.TargetType.AnnotatedAbiParameterName)));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.AnnotatedAbiReturnParameterName)));
            this.Write(")\r\n{\r\n    RRETURN(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GetterName));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.TargetType.AbiParameterName));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.PropertyType.AbiReturnParameterName));
            this.Write("));\r\n}\r\n\r\n");
     if (!Model.IsReadOnly) { 
            this.Write("IFACEMETHODIMP ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.DeclaringClass.GeneratedFactoryFullName)));
            this.Write("::");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.IdlAPInfo.SetterName));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.TargetType.AnnotatedAbiParameterName)));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.AnnotatedAbiParameterName)));
            this.Write(")\r\n{\r\n    RRETURN(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.SetterName));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.TargetType.AbiParameterName));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.PropertyType.AbiParameterName));
            this.Write("));\r\n}\r\n\r\n");
     }
   } 
            return this.GenerationEnvironment.ToString();
        }
    }
}
