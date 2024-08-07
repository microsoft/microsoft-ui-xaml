﻿// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 16.0.0.0
//  
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
// ------------------------------------------------------------------------------
namespace XamlGen.Templates.Code.Framework.Headers
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
    public partial class Struct : CppCodeGenerator<ClassDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Copyright>()));
            this.Write("\r\n\r\n#pragma once\r\n\r\nnamespace ");
            this.Write(this.ToStringHelper.ToStringWithCulture(OMContext.DefaultImplementationNamespace));
            this.Write("\r\n{\r\n    class ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GeneratedFactoryName));
            this.Write(":\r\n");
 foreach (var version in Model.Versions.OrderBy(v => v.Version).Select(v => v.GetProjection())) {
       if (version.IdlClassInfo.HasFactoryMethods) { 
            this.Write("        public ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(version.IdlClassInfo.FullFactoryInterfaceName)));
            this.Write(",\r\n");
     }
       if (version.IdlClassInfo.HasStaticMembers) { 
            this.Write("        public ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(version.IdlClassInfo.FullStaticMembersInterfaceName)));
            this.Write(",\r\n");
     }
   } 
            this.Write("        public ");
            this.Write(this.ToStringHelper.ToStringWithCulture(GetBaseFactoryFullName(Model)));
            this.Write("\r\n    {\r\n");
 if (Model.IdlClassInfo.HasAnyFactoryInterfaces) { 
            this.Write("        BEGIN_INTERFACE_MAP(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GeneratedFactoryName));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(GetBaseFactoryFullName(Model)));
            this.Write(")\r\n");
 foreach (var version in Model.Versions.OrderBy(v => v.Version).Select(v => v.GetProjection())) {
       if (version.IdlClassInfo.HasFactoryMethods) { 
            this.Write("            INTERFACE_ENTRY(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GeneratedFactoryName));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(version.IdlClassInfo.FullFactoryInterfaceName)));
            this.Write(")\r\n");
     }
       if (version.IdlClassInfo.HasStaticMembers) { 
            this.Write("            INTERFACE_ENTRY(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GeneratedFactoryName));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(version.IdlClassInfo.FullStaticMembersInterfaceName)));
            this.Write(")\r\n");
     }
   } 
            this.Write("        END_INTERFACE_MAP(");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GeneratedFactoryName));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(GetBaseFactoryFullName(Model)));
            this.Write(")\r\n");
 } 
            this.Write("\r\n    public:\r\n        // Extension methods.\r\n");
 foreach (var method in Model.InstanceMethods) { 
            this.Write("        ");
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Method>(method)));
            this.Write("\r\n");
 } 
            this.Write("\r\n        // Static properties.\r\n");
 foreach (var property in Model.StaticProperties) { 
            this.Write("        ");
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Property>(property)));
            this.Write("\r\n");
 } 
            this.Write("\r\n        // Static methods.\r\n");
 foreach (var method in Model.StaticMethods) { 
            this.Write("        ");
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Method>(method)));
            this.Write("\r\n");
 } 
            this.Write("\r\n    protected:\r\n");
 if (Model.IdlClassInfo.HasAnyFactoryInterfaces) { 
            this.Write("        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) ove" +
                    "rride;\r\n");
 } 
            this.Write("    };\r\n}\r\n\r\n");
 if (Model.GeneratePartialClass) { 
            this.Write("#include \"");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.CppFrameworkHeaderFileName));
            this.Write("\"\r\n");
 } 
            return this.GenerationEnvironment.ToString();
        }
    }
}
