// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 16.0.0.0
//  
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
// ------------------------------------------------------------------------------
namespace XamlGen.Templates.Code.Core.Bodies
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
    public partial class SimplePropertiesHelpersBody : CppCodeGenerator<OMContextView>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Copyright>()));
            this.Write("\r\n\r\n#include \"precomp.h\"\r\n#include <SimplePropertiesHelpers.h>\r\n\r\nnamespace Simpl" +
                    "eProperties\r\n{\r\n\r\nbool IsSimplePropertySetToDefault(KnownPropertyIndex propertyI" +
                    "ndex, SimpleProperty::const_objid_t obj)\r\n{\r\n    switch (propertyIndex)\r\n    {\r\n" +
                    "");
 foreach (var simpleProperty in Model.GetAllTypeTableSimpleProperties().Where((def) => def.Modifier != Modifier.Internal)) { 
            this.Write("        case ");
            this.Write(this.ToStringHelper.ToStringWithCulture(simpleProperty.IndexName));
            this.Write(":\r\n        {\r\n            if (SimpleProperty::Property::id<");
            this.Write(this.ToStringHelper.ToStringWithCulture(simpleProperty.IndexName));
            this.Write(">::IsSet(obj))\r\n            {\r\n                return (SimpleProperty::Property::" +
                    "id<");
            this.Write(this.ToStringHelper.ToStringWithCulture(simpleProperty.IndexName));
            this.Write(">::Get(obj) ==\r\n                        SimpleProperty::Property::Default<");
            this.Write(this.ToStringHelper.ToStringWithCulture(simpleProperty.IndexName));
            this.Write(">());\r\n            }\r\n        }\r\n        break;\r\n\r\n");
 } 
            this.Write("        default:\r\n            XAML_FAIL_FAST();  // investigate unknown simple pr" +
                    "operty\r\n    }\r\n\r\n    return true;\r\n}\r\n\r\n} // namespace SimpleProperties");
            return this.GenerationEnvironment.ToString();
        }
    }
}
