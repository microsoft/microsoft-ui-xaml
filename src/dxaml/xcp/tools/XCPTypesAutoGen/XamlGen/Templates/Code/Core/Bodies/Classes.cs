﻿// ------------------------------------------------------------------------------
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
    public partial class Classes : CppCodeGenerator<OMContextView>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Copyright>()));
            this.Write("\r\n\r\n#include \"precomp.h\"\r\n#include \"StableXbfIndexes.g.h\"\r\n#include <RuntimeProfi" +
                    "ler.h>\r\n#include \"GeneratedClasses.g.h\"\r\n\r\n");
 foreach (var type in Model.GetAllClasses().Where(c => !c.IsAEventArgs && c.GenerateInCore)) { 
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Class>(type)));
            this.Write("\r\n\r\n");
} 
            return this.GenerationEnvironment.ToString();
        }
    }
}
