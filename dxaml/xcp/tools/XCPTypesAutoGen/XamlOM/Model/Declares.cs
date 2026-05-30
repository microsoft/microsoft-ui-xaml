// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace XamlOM
{
    public interface IDeclaresProvider
    {
        IEnumerable<Tuple<string, Type[]>> GetDeclares();
    }

    internal static class AssembliesExtensions
    {
        public static IEnumerable<T> GetInstancesOf<T>(this IEnumerable<Assembly> assemblies)
            where T : class
        {
            foreach (var asm in assemblies)
            {
                var typeName = typeof(T).FullName.Replace("OM.I", "OM.");
                var type = asm.GetType(typeName);
                if (type != null)
                {
                    var instance = Activator.CreateInstance(type) as T;
                    if (instance != null)
                    {
                        yield return instance;
                    }
                }
            }
        }
    }

    /// <summary>
    /// Used to describe what types to put in a declares { } section per IDL group.
    /// </summary>
    internal class Declares
    {
        public static IEnumerable<Tuple<string, Type[]>> GetDeclares(IEnumerable<Assembly> assemblies)
        {
            return assemblies
                .GetInstancesOf<IDeclaresProvider>()
                .SelectMany(provider => provider.GetDeclares());
        }
    }
}
