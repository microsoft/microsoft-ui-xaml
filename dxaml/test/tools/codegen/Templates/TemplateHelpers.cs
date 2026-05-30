// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Linq;

namespace Codegen.Templates
{
    public static class TemplateHelpers
    {
        /// <summary>
        /// Return the type name as if it were written in C#.
        /// </summary>
        public static string GetTypeName(Type type)
        {
            string result = type.Name;

            if(type.IsGenericType)
            {
                string arguments = string.Join(", ", type.GenericTypeArguments.Select(t => t.Name));
                result = string.Format("{0}<{1}>", result.Substring(0, result.IndexOf('`')), arguments);
            }

            return result;
        }
    }
}
