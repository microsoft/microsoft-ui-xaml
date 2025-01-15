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
    public partial class Factory : PhoneCppCodeGenerator<ClassDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write("// Dependency Property references.\r\n");
 foreach (var p in Model.DependencyProperties.Where(p => !p.IsAbstract)) { 
            this.Write("wrl::ComPtr<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyProperty")));
            this.Write("> ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FactoryTypeName));
            this.Write("::s_");
            this.Write(this.ToStringHelper.ToStringWithCulture(p.Name));
            this.Write("Property;\r\n");
 }
   foreach (var a in Model.DeclaredAttachedProperties.Where(e => e.GenerateStub)) { 
            this.Write("wrl::ComPtr<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyProperty")));
            this.Write("> ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FactoryTypeName));
            this.Write("::s_");
            this.Write(this.ToStringHelper.ToStringWithCulture(a.Name));
            this.Write("Property;\r\n");
 } 
            this.Write("\r\n// Initializers.\r\n_Check_return_ HRESULT ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FactoryTypeName));
            this.Write("::RuntimeClassInitialize()\r\n{\r\n    HRESULT hr = S_OK;\r\n\r\n    IFC(EnsureProperties" +
                    "());\r\n\r\nCleanup:\r\n    RRETURN(hr);\r\n}\r\n\r\n_Check_return_ HRESULT ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FactoryTypeName));
            this.Write("::EnsureProperties()\r\n{\r\n");
 foreach (var p in Model.DependencyProperties.Where(p => !p.IsAbstract)) { 
            this.Write("    IFC_RETURN(Initialize");
            this.Write(this.ToStringHelper.ToStringWithCulture(p.Name));
            this.Write("Property());\r\n");
 }
   foreach (var a in Model.DeclaredAttachedProperties.Where(e => e.GenerateStub)) { 
            this.Write("    IFC_RETURN(Initialize");
            this.Write(this.ToStringHelper.ToStringWithCulture(a.Name));
            this.Write("Property());\r\n");
 } 
            this.Write("\r\n    return S_OK;\r\n}\r\n\r\nvoid ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FactoryTypeName));
            this.Write("::ClearProperties()\r\n{\r\n");
 foreach (var p in Model.DependencyProperties) { 
            this.Write("        s_");
            this.Write(this.ToStringHelper.ToStringWithCulture(p.Name));
            this.Write("Property.Reset();\r\n");
 }
   foreach (var p in Model.DeclaredAttachedProperties) { 
            this.Write("        s_");
            this.Write(this.ToStringHelper.ToStringWithCulture(p.Name));
            this.Write("Property.Reset();\r\n");
 } 
            this.Write("}\r\n\r\n");
 foreach (var ctor in Model.IdlClassInfo.CustomConstructors) { 
            this.Write("IFACEMETHODIMP ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FactoryTypeName));
            this.Write("::");
            this.Write(this.ToStringHelper.ToStringWithCulture(ctor.IdlConstructorInfo.FactoryMethodName));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(GetConstructorParameterListAsString(ctor)));
            this.Write(")\r\n{\r\n");
     if (Model.IdlClassInfo.IsComposable && ctor.IsParameterless) { 
            this.Write("    HRESULT hr = S_OK;\r\n\r\n    IFC((pctl::AggregableComObject<\r\n            ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.FullName)));
            this.Write(",\r\n            ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.IdlClassInfo.AbiInterfaceFullName)));
            this.Write(">::CreateInstance(\r\n            pOuter,\r\n            ppInner,\r\n            ppInst" +
                    "ance)));\r\n\r\nCleanup:\r\n    RRETURN(hr);\r\n}\r\n");
     }
       else { 
            this.Write("    HRESULT hr = S_OK;\r\n    IFCPTR(ppInstance);\r\n    ARG_VALIDRETURNPOINTER(ppIns" +
                    "tance);\r\n");
              foreach (var p in ctor.Parameters.Where(p => p.RequiresNullCheck)) { 
            this.Write("    ARG_NOTNULL(");
            this.Write(this.ToStringHelper.ToStringWithCulture(p.ParameterType.AbiParameterName));
            this.Write(", \"");
            this.Write(this.ToStringHelper.ToStringWithCulture(p.ParameterType.AbiParameterName));
            this.Write("\");\r\n");
              } 
            this.Write("    IFC(");
            this.Write(this.ToStringHelper.ToStringWithCulture(ctor.FactoryMethodImplName));
            this.Write("(");
            this.Write(this.ToStringHelper.ToStringWithCulture(GetArgumentListAsString(ctor)));
            this.Write("));\r\nCleanup:\r\n    RRETURN(hr);\r\n}\r\n\r\n");
     }
   }
   if (Model.NeedsActivateInstance) {

            this.Write("IFACEMETHODIMP\r\n");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.FactoryTypeName));
            this.Write("::ActivateInstance(\r\n    _Outptr_ IInspectable** ppInspectable)\r\n{\r\n    HRESULT h" +
                    "r = S_OK;\r\n    wrl::ComPtr<");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppType(Model.IdlClassInfo.AbiInterfaceFullName)));
            this.Write("> ");
            this.Write(this.ToStringHelper.ToStringWithCulture(LowerCaseFirstLetter(Model.Name)));
            this.Write(";\r\n\r\n    IFCPTR(ppInspectable);\r\n    IFC(wrl::MakeAndInitialize<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write(">(&");
            this.Write(this.ToStringHelper.ToStringWithCulture(LowerCaseFirstLetter(Model.Name)));
            this.Write("));\r\n\r\n    *ppInspectable = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(LowerCaseFirstLetter(Model.Name)));
            this.Write(".Detach();\r\n\r\nCleanup:\r\n    RRETURN(hr);\r\n}\r\n\r\n");
 } 
            this.Write("// Static properties.\r\n\r\n");
 var dependencyProperties = Model.DependencyProperties.Where(p => !p.IsAbstract);
   if (dependencyProperties.Any()) { 
            this.Write("// Dependency properties initializing functions\r\n");
      foreach (var p in dependencyProperties) { 
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<InitializeProperty>(p)));
            this.Write("\r\n\r\n");
      }
   }
 if (Model.HasPropertyChangeCallback && (Model.DependencyProperties.Any() || Model.DeclaredAttachedProperties.Any())) { 
            this.Write("// Property changed event handler\r\nHRESULT ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GeneratedFactoryName));
            this.Write("::OnPropertyChanged(\r\n    _In_ ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyObject")));
            this.Write("* pSender,\r\n    _In_ ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyProperty")));
            this.Write("ChangedEventArgs* pArgs)\r\n{\r\n    HRESULT hr = S_OK;\r\n    wrl::ComPtr<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyObject")));
            this.Write("> spSenderAsDO(pSender);\r\n    wrl::ComPtr<I");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("> sp");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write(";\r\n\r\n    IFC(spSenderAsDO.As(&sp");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("));\r\n    IFC(static_cast<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write("*>(sp");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.Name));
            this.Write(".Get())->OnPropertyChanged(pArgs));\r\n\r\nCleanup:\r\n    RRETURN(hr);\r\n}\r\n\r\n");
 } 
            this.Write("// Dependency properties.\r\n");
 foreach (var p in Model.IdlClassInfo.DependencyProperties.Where(p => !p.IsAbstract)) { 
            this.Write("IFACEMETHODIMP ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Model.GeneratedFactoryName));
            this.Write("::get_");
            this.Write(this.ToStringHelper.ToStringWithCulture(p.Name));
            this.Write("Property(_Outptr_ ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Helper.PrefixAbiIfNeeded($"{Helper.GetRootNamespaceCpp()}::IDependencyProperty")));
            this.Write("** ppValue)\r\n{\r\n    HRESULT hr = S_OK;\r\n    \r\n    ARG_VALIDRETURNPOINTER(ppValue)" +
                    ";\r\n    IFC(s_");
            this.Write(this.ToStringHelper.ToStringWithCulture(p.Name));
            this.Write("Property.CopyTo(ppValue));\r\n\r\nCleanup:\r\n    RRETURN(hr);\r\n}\r\n\r\n");
 } 
            this.Write("\r\n// Attached properties.\r\n");
 foreach (var a in Model.DeclaredAttachedProperties) { 
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<AttachedPropertyImpl>(a)));
            this.Write("\r\n\r\n");
 } 
            this.Write("\r\n// Static methods.\r\n");
 foreach (var m in Model.StaticMethods.Where(m => m.GenerateStub)) { 
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Method>(m)));
            this.Write("\r\n\r\n");
 } 
            return this.GenerationEnvironment.ToString();
        }
    }
}