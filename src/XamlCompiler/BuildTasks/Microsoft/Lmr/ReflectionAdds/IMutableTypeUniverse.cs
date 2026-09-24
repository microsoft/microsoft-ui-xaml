// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


namespace System.Reflection.Adds
{
    using System.Reflection;
    using Type = System.Type;

    /// <summary>
    /// Mutable universe extends a ITypeUniverse with operations to actually load assemblies into the universe.
    /// </summary>
    internal interface IMutableTypeUniverse : ITypeUniverse
    {
        /// <summary>
        /// Register the assembly as being loaded into this universe
        /// </summary>
        /// <param name="assembly">An assembly that was created to be in this universe. It should implement IAssembly2
        /// and have its TypeUniverse set to this instance.</param>
        void AddAssembly(Assembly assembly);
    }

}
