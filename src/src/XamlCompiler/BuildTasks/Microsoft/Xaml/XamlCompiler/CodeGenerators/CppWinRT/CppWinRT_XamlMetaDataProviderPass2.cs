﻿// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 17.0.0.0
//  
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
// ------------------------------------------------------------------------------
namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    using System;
    
    /// <summary>
    /// Class to produce the template output
    /// </summary>
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "17.0.0.0")]
    internal partial class CppWinRT_XamlMetaDataProviderPass2 : CppWinRT_CodeGenerator<TypeInfoDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
  if(!String.IsNullOrEmpty(ProjectInfo.PrecompiledHeaderFile)) { 
            this.Write("#include \"");
            this.Write(this.ToStringHelper.ToStringWithCulture(ProjectInfo.PrecompiledHeaderFile));
            this.Write("\"\r\n");
  }
            return this.GenerationEnvironment.ToString();
        }
    }
}