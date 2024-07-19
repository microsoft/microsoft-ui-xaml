// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using CodeGen;
    using DirectUI;
    using Properties;
    
    internal class XamlTypeInfoValidator
    {
        private DirectUISchemaContext schema;

        private List<XamlCompileError> errors = new List<XamlCompileError>();
        private List<XamlCompileWarning> warnings = new List<XamlCompileWarning>();

        public IEnumerable<XamlCompileError> Errors { get { return errors; } }
        public IEnumerable<XamlCompileWarning> Warnings { get { return warnings; } }

        public XamlTypeInfoValidator(DirectUISchemaContext duiSchema)
        {
            this.schema = duiSchema;
        }

        public void Validate(IEnumerable<InternalTypeEntry> typeTable)
        {
            ValidateCreateFromStringMethods(typeTable);
        }

        private void ValidateCreateFromStringMethods(IEnumerable<InternalTypeEntry> typeTable)
        {
            foreach (var typeWithCreateFromString in typeTable.Where(t => t.UserTypeInfo != null && t.UserTypeInfo.CreateFromStringMethod.Exists))
            {
                ValidateCreateFromStringMethod(typeWithCreateFromString);
            }
        }


        /// <summary>
        /// Validates that type has a valid CreateFromString method that exists
        /// </summary>
        /// <param name="type"></param>
        private void ValidateCreateFromStringMethod(InternalTypeEntry declaringType)
        {
            XamlCompileError err = schema.EnsureCreateFromStringResolved(declaringType.Name, declaringType.UserTypeInfo.CreateFromStringMethod, null);
            if (err != null)
            {
                errors.Add(err);
            }
        }
    }
}
