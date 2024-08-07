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
    public partial class Classes : MultiFileXamlCodeGenerator<OMContextView>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
 foreach (var type in Model.GetAllClasses().Where(c => !c.IsAEventArgs && c.GenerateFrameworkCppFile)) {
       if (type.IsValueType) {
           AddGenerator<Struct>(type.CppFrameworkGeneratedBodyFileName, type);
       } else {
           AddGenerator<Class>(type.CppFrameworkGeneratedBodyFileName, type);
       }
   } 
            return this.GenerationEnvironment.ToString();
        }
    }
}
