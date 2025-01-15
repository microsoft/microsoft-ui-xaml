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
    public partial class ReferenceTypeGetValue : PhoneCppCodeGenerator<PropertyDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write("case get_");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.DeclaringType.TypeTableName));
            this.Write("_");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write(":\r\n{\r\n    wrl::ComPtr<");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.DeclaringTypeFullInterfaceNameWithVersion)));
            this.Write("> obj;\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.PhoneImplTypeName)));
            this.Write(" prop;\r\n    IFC(instance->QueryInterface(\r\n        __uuidof(");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.DeclaringTypeFullInterfaceNameWithVersion)));
            this.Write("),\r\n        &obj));\r\n    *value = nullptr;\r\n    IFC(obj->get_");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("(&prop));\r\n    if (prop)\r\n    {\r\n        IFC(prop.CopyTo(value));\r\n    }\r\n    bre" +
                    "ak;\r\n}");
            return this.GenerationEnvironment.ToString();
        }
    }
}