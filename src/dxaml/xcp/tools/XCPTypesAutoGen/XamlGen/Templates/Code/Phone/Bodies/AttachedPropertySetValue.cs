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
    public partial class AttachedPropertySetValue : CppCodeGenerator<AttachedPropertyDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write("case put_");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringType.TypeTableName));
            this.Write("_Attached");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write(":\r\n{\r\n    wrl::ComPtr<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyObject")));
            this.Write("> dependencyObj;\r\n    wrl::ComPtr<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyProperty")));
            this.Write("> dependencyProp;\r\n    wrl::ComPtr<");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.DeclaringType.DeclaringNamespace.Name)));
            this.Write("::I");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringType.TypeTableName));
            this.Write("Statics> statics;\r\n    IFC(instance->QueryInterface(\r\n        __uuidof(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyObject")));
            this.Write("),\r\n        &dependencyObj));\r\n    IFC(");
            this.Write(this.ToStringHelper.ToStringWithCulture(PrefixAbi("Windows::Foundation::GetActivationFactory")));
            this.Write("(\r\n        Microsoft::WRL::Wrappers::HStringReference(L\"");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringType.TypeTableFullName));
            this.Write("\").Get(),\r\n        statics.GetAddressOf()));\r\n    IFC(statics->get_");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("Property(\r\n        &dependencyProp));\r\n    IFC(dependencyObj->SetValue(\r\n        " +
                    "dependencyProp.Get(),\r\n        value));\r\n    break;\r\n  }");
            return this.GenerationEnvironment.ToString();
        }
    }
}
