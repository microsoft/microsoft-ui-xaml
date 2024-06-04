// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.IO;

namespace XamlGen.Templates
{
    /// <summary>
    /// Base class for multi-file XAML code generators.
    /// </summary>
    public abstract class MultiFileXamlCodeGenerator<TModel> : XamlCodeGenerator<TModel>
    {
        private List<T4CodeGenerator> _generators = new List<T4CodeGenerator>();

        /// <summary>
        /// Adds a generator.
        /// </summary>
        /// <typeparam name="TTemplate"></typeparam>
        /// <param name="fileName"></param>
        /// <param name="model"></param>
        /// <param name="args"></param>
        protected void AddGenerator<TTemplate>(string fileName, object model, params object[] args) where TTemplate : T4CodeGenerator, new()
        {
            TTemplate template = new TTemplate();
            if (model != null)
            {
                template.SetModel(model);
            }
            template.Arguments = args;
            template.OutputPath = Path.Combine(OutputPath, fileName);
            _generators.Add(template);
        }

        public IEnumerable<T4CodeGenerator> GetGenerators()
        {
            TransformText();
            return _generators;
        }

        public override void Write(string textToAppend)
        {
            // Don't write anything. Implementations of MultiFileXamlCodeGenerator are only supposed to 
            // add to _generators.
        }
    }
}
