﻿// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 17.0.0.0
//  
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
// ------------------------------------------------------------------------------
namespace XamlGen.Templates.Code.Core.Headers
{
    using System.Linq;
    using System.Text;
    using System.Collections.Generic;
    using OM;
    using System;
    
    /// <summary>
    /// Class to produce the template output
    /// </summary>
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "17.0.0.0")]
    public partial class SimplePropertiesAdapter : CppCodeGenerator<OMContextView>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Copyright>()));
            this.Write("\r\n\r\n#pragma once\r\n\r\nnamespace SimpleProperty\r\n{\r\n    // Specializations for host " +
                    "types.\r\n");
 foreach (var type in Model.GetAllTypeTableClassesWithSimpleProperties().Where((c) => c.SimpleProperties.Any((p) => p.SimpleStorage.Value == SimplePropertyStorage.Sparse || p.SimpleStorage.Value == SimplePropertyStorage.SparseGroup))) { 
            this.Write("\r\n    template <>\r\n    struct sparse_host<");
            this.Write(this.ToStringHelper.ToStringWithCulture(type.CoreName));
            this.Write(">\r\n    {\r\n        using declaring_type = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(type.CoreName));
            this.Write(";\r\n        static constexpr sparse_host_query_t<declaring_type> query = &declarin" +
                    "g_type::IsSimpleSparseSet;\r\n        static constexpr sparse_host_setter_t<declar" +
                    "ing_type> setter = &declaring_type::SetSimpleSparseFlag;\r\n    };\r\n");
 } 
            this.Write("\r\n    namespace details\r\n    {\r\n        // Sparse table picker specializations.\r\n" +
                    "");
 foreach (var property in Model.GetAllTypeTableSimpleProperties().Where((p) => p.SimpleStorage.Value == SimplePropertyStorage.Sparse || p.SimpleStorage.Value == SimplePropertyStorage.SparseGroup)) { 
            this.Write("\r\n        template <>\r\n        static auto& PickSparseTable<");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexName));
            this.Write(">(sparsetables& tables)\r\n        {\r\n");
   if (property.SimpleStorage.Value == SimplePropertyStorage.Sparse) { 
            this.Write("            return tables.m_");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexNameWithoutPrefix));
            this.Write(";\r\n");
   } else { 
            this.Write("            return tables.m_");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.SimpleGroupStorageClass));
            this.Write(";\r\n");
   } 
            this.Write("        }\r\n");
 } 
            this.Write("\r\n        // Change handlers picker specializations.\r\n");
 foreach (var property in Model.GetAllTypeTableSimpleProperties()) { 
            this.Write("\r\n        template <>\r\n        static auto& PickHandlersTable<");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexName));
            this.Write(">(changehandlerstables& handlers)\r\n        {\r\n            return handlers.m_");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexNameWithoutPrefix));
            this.Write(";\r\n        }\r\n");
 } 
            this.Write("\r\n        // Positions in s_runtimePropertyInfos array need to be in the same rel" +
                    "ative order as KnownPropertyIndices of properties.\r\n");
 uint index = 0; 
 foreach (var property in Model.GetAllTypeTableSimpleProperties()) { 
            this.Write("        static_assert(");
            this.Write(this.ToStringHelper.ToStringWithCulture(index++));
            this.Write(" == static_cast<size_t>(");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexName));
            this.Write(") - KnownDependencyPropertyCount, \"Index mismatch for ");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexNameWithoutPrefix));
            this.Write("\");\r\n");
 } 
            this.Write("\r\n        const RuntimePropertyInfo s_runtimePropertyInfos[] =\r\n        {\r\n");
 foreach (var property in Model.GetAllTypeTableSimpleProperties()) { 
            this.Write("            GENERATE_RUNTIMEINFO(");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexName));
            this.Write("),\r\n");
 } 
            this.Write("        };\r\n    }\r\n");
 foreach (var type in Model.GetAllTypeTableClassesWithSimpleProperties()) { 
            this.Write("\r\n    template <>\r\n    void Property::NotifyDestroyed<");
            this.Write(this.ToStringHelper.ToStringWithCulture(type.CoreName));
            this.Write(">(objid_t obj)\r\n    {\r\n");
 foreach (var property in Model.GetAllTypeTableSimpleProperties().Where((p) => p.DeclaringClass == type)) { 
            this.Write("        id<");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexName));
            this.Write(">::DestroyInstance(obj);\r\n");
 } 
            this.Write("    }\r\n");
 } 
            this.Write("\r\n");
 foreach (var type in Model.GetAllTypeTableClassesWithSimpleProperties()) { 
            this.Write("    template void Property::NotifyDestroyed<");
            this.Write(this.ToStringHelper.ToStringWithCulture(type.CoreName));
            this.Write(">(objid_t);\r\n");
 } 
            this.Write("\r\n    void Property::DestroyAllProperties()\r\n    {\r\n");
 foreach (var property in Model.GetAllTypeTableSimpleProperties()) { 
            this.Write("        id<");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexName));
            this.Write(">::DestroyProperty();\r\n");
 } 
            this.Write("    }\r\n\r\n");
 foreach (var property in Model.GetAllTypeTableSimpleProperties()) { 
            this.Write("    template class Property::id<");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexName));
            this.Write(">;\r\n");
 } 
            this.Write("\r\n#if DBG\r\n    bool Property::AreAllCleanedUp(const_objid_t obj)\r\n    {\r\n        " +
                    "bool result = true;\r\n\r\n");
 foreach (var property in Model.GetAllTypeTableSimpleProperties()) { 
            this.Write("        result &= id<");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.IndexName));
            this.Write(">::IsCleanedUp(obj);\r\n");
 } 
            this.Write("\r\n        return result;\r\n    }\r\n#endif\r\n}\r\n");
            return this.GenerationEnvironment.ToString();
        }
    }
}
