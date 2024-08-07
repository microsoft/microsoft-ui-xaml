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
    public partial class AttachedPropertyGetterBodyEnum : CppCodeGenerator<AttachedPropertyDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write("wrl::ComPtr<IInspectable> spValueAsII;\r\nwrl::ComPtr<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyObject")));
            this.Write("> spElementAsDO;\r\nwrl::ComPtr<");
            this.Write(this.ToStringHelper.ToStringWithCulture(PrefixAbi("Windows::Foundation::IReference")));
            this.Write("<");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PropertyType.Type.FullName)));
            this.Write(">> spBoxedValueType;\r\n\r\nARG_NOTNULL(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.TargetType.AbiParameterName));
            this.Write(", \"");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.TargetType.AbiParameterName));
            this.Write("\");\r\nARG_VALIDRETURNPOINTER(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GetterReturnType.AbiReturnParameterName));
            this.Write(");\r\n\r\nIFC(Initialize");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("Property());\r\n\r\nIFC(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.TargetType.AbiParameterName));
            this.Write("->QueryInterface(__uuidof(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyObject")));
            this.Write("), &spElementAsDO));\r\nIFC(spElementAsDO->GetValue(s_");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("Property.Get(), &spValueAsII));\r\nIFCPTR(spValueAsII);\r\n\r\nIFC(spValueAsII.As(&spBo" +
                    "xedValueType));\r\nIFC(spBoxedValueType->get_Value(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GetterReturnType.AbiReturnParameterName));
            this.Write("));\r\n");
            return this.GenerationEnvironment.ToString();
        }
    }
}
