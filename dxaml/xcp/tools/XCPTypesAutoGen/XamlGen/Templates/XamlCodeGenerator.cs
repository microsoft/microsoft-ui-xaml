// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;

namespace XamlGen.Templates
{
    /// <summary>
    /// Base class for all XAML code generators.
    /// </summary>
    public abstract class XamlCodeGenerator<TModel> : T4CodeGenerator
    {
        public TModel Model
        {
            get;
            set;
        }

        public override void SetModel(object model)
        {
            Model = (TModel)model;
        }

        protected string EncodeMacroParameter(string str)
        {
            return str.Replace(",", " COMMA");
        }
       
        protected string PrefixAbi(string expression)
        {
            return OM.Helper.PrefixAbi(expression).Replace(".", "::");
        }

        protected string AsCppType(string expression)
        {
            return OM.Helper.PrefixAbiIfNeeded(expression).Replace(".", "::");
        }

        protected string AsCppBoolean(bool value)
        {
            return value.ToString().ToLower();
        }

        protected string AsStringStorage(string str)
        {
            if (string.IsNullOrEmpty(str))
            {
                return "{ /* empty string */ }";
            }
            return string.Format("XSTRING_PTR_STORAGE(L\"{0}\")", str);
        }

        /// <summary>
        /// Executes a template and appends the result to the GenerationEnvironment.
        /// </summary>
        /// <typeparam name="TTemplate"></typeparam>
        /// <returns></returns>
        protected string IncludeTemplate<TTemplate>() where TTemplate : T4CodeGenerator, new()
        {
            return IncludeTemplate<TTemplate>(null);
        }

        /// <summary>
        /// Executes a template and appends the result to the GenerationEnvironment.
        /// </summary>
        /// <typeparam name="TTemplate"></typeparam>
        /// <param name="model"></param>
        /// <param name="args"></param>
        /// <returns></returns>
        protected string IncludeTemplate<TTemplate>(object model, params object[] args) where TTemplate : T4CodeGenerator, new()
        {
            TTemplate template = new TTemplate();
            if (model != null)
            {
                template.SetModel(model);
            }
            template.Arguments = args;
            string result = template.TransformText().Trim('\r', '\n');
            if (!string.IsNullOrEmpty(result))
            {
                PushIndent(GetCurrentIndent());
                Write(result);
                PopIndent();
            }

            // Return nothing... We only use a return type so we can use the <#= #> syntax in the templates.
            return string.Empty;
        }
    }
}
