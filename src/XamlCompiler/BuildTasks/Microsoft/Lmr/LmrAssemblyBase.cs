// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Common base class of the LMR implementations of <see cref="Assembly"/> (and of the compiler's wrapper of them).
    /// </summary>
    /// <remarks>
    /// Same pattern as <see cref="LmrTypeBase"/>: the members of <see cref="Assembly"/> annotated for trimming or single-file
    /// are overridden once, here, with the same annotations, and forward to unannotated "*Core" members. LMR assemblies are
    /// read from metadata files, so their types can't be trimmed and they always have a file: code working with LMR assemblies
    /// calls the "*Core" members (directly or through <see cref="LmrReflection"/>).
    /// </remarks>
    internal abstract class LmrAssemblyBase : Assembly
    {
        // The same messages as the ones on the members of Assembly
        internal const string TypesMightBeRemovedMessage = "Types might be removed";
        internal const string GetTypeRequiresUnreferencedCodeMessage = "Types might be removed by trimming. If the type name is a string literal, consider using Type.GetType instead.";
        internal const string AssemblyReferencesMightBeRemovedMessage = "Assembly references might be removed";
        internal const string RequiresAssemblyFilesMessage = "This member throws an exception for assemblies embedded in a single-file app";

        [RequiresUnreferencedCode(TypesMightBeRemovedMessage)]
        public sealed override Type[] GetTypes()
        {
            return GetTypesCore();
        }

        internal abstract Type[] GetTypesCore();

        [RequiresUnreferencedCode(TypesMightBeRemovedMessage)]
        public sealed override Type[] GetExportedTypes()
        {
            return GetExportedTypesCore();
        }

        /// <summary>
        /// Same as the implementation of <see cref="Assembly.GetExportedTypes"/>, for the assemblies that don't override it.
        /// </summary>
        internal virtual Type[] GetExportedTypesCore()
        {
            throw new NotImplementedException();
        }

        [RequiresUnreferencedCode(GetTypeRequiresUnreferencedCodeMessage)]
        public sealed override Type GetType(string name)
        {
            return GetTypeCore(name);
        }

        /// <summary>
        /// Same as the implementation of <see cref="Assembly.GetType(string)"/>, for the assemblies that don't override it.
        /// </summary>
        internal virtual Type GetTypeCore(string name)
        {
            return GetTypeCore(name, false, false);
        }

        [RequiresUnreferencedCode(GetTypeRequiresUnreferencedCodeMessage)]
        public sealed override Type GetType(string name, bool throwOnError)
        {
            return GetTypeCore(name, throwOnError);
        }

        /// <summary>
        /// Same as the implementation of <see cref="Assembly.GetType(string, bool)"/>, for the assemblies that don't override it.
        /// </summary>
        internal virtual Type GetTypeCore(string name, bool throwOnError)
        {
            return GetTypeCore(name, throwOnError, false);
        }

        [RequiresUnreferencedCode(GetTypeRequiresUnreferencedCodeMessage)]
        public sealed override Type GetType(string name, bool throwOnError, bool ignoreCase)
        {
            return GetTypeCore(name, throwOnError, ignoreCase);
        }

        /// <summary>
        /// Same as the implementation of <see cref="Assembly.GetType(string, bool, bool)"/>, for the assemblies that don't override it.
        /// </summary>
        internal virtual Type GetTypeCore(string name, bool throwOnError, bool ignoreCase)
        {
            throw new NotImplementedException();
        }

        [RequiresUnreferencedCode(AssemblyReferencesMightBeRemovedMessage)]
        public sealed override AssemblyName[] GetReferencedAssemblies()
        {
            return GetReferencedAssembliesCore();
        }

        /// <summary>
        /// Same as the implementation of <see cref="Assembly.GetReferencedAssemblies"/>, for the assemblies that don't override it.
        /// </summary>
        internal virtual AssemblyName[] GetReferencedAssembliesCore()
        {
            throw new NotImplementedException();
        }

        // Location has no annotation, but reading it on an Assembly makes the single-file analyzer warn (it is empty for the
        // assemblies of a single-file app, IL3000). The location of an LMR assembly is the metadata file it was read from.
        public sealed override string Location
        {
            get { return LocationCore; }
        }

        internal abstract string LocationCore
        {
            get;
        }

        [RequiresAssemblyFiles(RequiresAssemblyFilesMessage)]
        public sealed override string CodeBase
        {
            get { return CodeBaseCore; }
        }

        /// <summary>
        /// Same as the implementation of <see cref="Assembly.CodeBase"/>, for the assemblies that don't override it.
        /// </summary>
        internal virtual string CodeBaseCore
        {
            get { throw new NotImplementedException(); }
        }

        [RequiresAssemblyFiles(RequiresAssemblyFilesMessage)]
        public sealed override FileStream GetFile(string name)
        {
            return GetFileCore(name);
        }

        /// <summary>
        /// Same as the implementation of <see cref="Assembly.GetFile"/>, for the assemblies that don't override it.
        /// </summary>
        internal virtual FileStream GetFileCore(string name)
        {
            throw new NotImplementedException();
        }

        [RequiresAssemblyFiles(RequiresAssemblyFilesMessage)]
        public sealed override FileStream[] GetFiles()
        {
            return GetFilesCore();
        }

        /// <summary>
        /// Same as the implementation of <see cref="Assembly.GetFiles()"/>, for the assemblies that don't override it.
        /// </summary>
        internal virtual FileStream[] GetFilesCore()
        {
            return GetFilesCore(false);
        }

        [RequiresAssemblyFiles(RequiresAssemblyFilesMessage)]
        public sealed override FileStream[] GetFiles(bool getResourceModules)
        {
            return GetFilesCore(getResourceModules);
        }

        /// <summary>
        /// Same as the implementation of <see cref="Assembly.GetFiles(bool)"/>, for the assemblies that don't override it.
        /// </summary>
        internal virtual FileStream[] GetFilesCore(bool getResourceModules)
        {
            throw new NotImplementedException();
        }
    }
}
