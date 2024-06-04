// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;
using System.Collections.Generic;
using System.IO;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using MSBuildInterop;
    using Tracing;
    using Utilities;

    internal class FingerPrinter
    {
        private static bool s_VcMetaIsLoaded = false;

        private String[] _ignorePathsList;
        private String _localAssemblyPath;
        private String[] _nonSystemReferenceAssemblies;

        private readonly bool _useVcMetaManaged = true;

        public FingerPrinter(IAssemblyItem localAssembly, IEnumerable<IAssemblyItem> referenceAssemblies, String[] ignorePaths, string vcInstallDir, string vcInstallPath32, string vcInstallPath64, bool useVCMetaManaged)
        {
            IgnorePathsList = ignorePaths;
            SetLocalAssembly(localAssembly);
            // Reference assemblies are filtered by the Ignore paths.
            SetReferenceAssemblies(referenceAssemblies, IgnorePathsList);
            _useVcMetaManaged = useVCMetaManaged;
            if (!_useVcMetaManaged)
            {
                FingerPrinter.s_VcMetaIsLoaded = NativeMethodsHelper.EnsureVcMetaIsLoaded(vcInstallDir, vcInstallPath32, vcInstallPath64);
            }
        }

        // ---------- Public Properties

        public String LocalAssemblyPath { get { return _localAssemblyPath; } }
        public void SetLocalAssembly(IAssemblyItem localAssembly)
        {
            _localAssemblyPath = null;
            if (localAssembly != null)
            {
                _localAssemblyPath = ToLowerFullFilePath(localAssembly.ItemSpec);
            }
        }

        public String[] ReferenceAssemblyPaths { get { return _nonSystemReferenceAssemblies; } }
        public void SetReferenceAssemblies(IEnumerable<IAssemblyItem> referenceAssemblies, String[] ignoreList)
        {
            List<String> refasmBuilder = new List<string>();
            foreach (var item in referenceAssemblies)
            {
                string assemblyPath = ToLowerFullFilePath(item.ItemSpec);
                if (_localAssemblyPath == assemblyPath)
                {
                    continue;
                }

                // Don't hash the nuget assembly references as these are immutable
                if (item.IsNuGetReference || ignoreList.Any(x => assemblyPath.StartsWith(x)))
                {
                    continue;
                }

                refasmBuilder.Add(assemblyPath);
            }
            _nonSystemReferenceAssemblies = refasmBuilder.ToArray();
        }

        public String[] IgnorePathsList
        {
            get { return _ignorePathsList; }
            private set
            {
                List<String> ignoreBuilder = new List<string>();
                if (value != null)
                {
                    foreach (string path in value)
                    {
                        string lowerPath = path.ToLowerInvariant();
                        ignoreBuilder.Add(lowerPath);
                    }
                }
                _ignorePathsList = ignoreBuilder.ToArray();
            }
        }

        // ---------  Tests to check if anything has changed

        public bool HasAssemblyFileListChanged(HashSet<String> asmFileNames)
        {
            bool different = false;

            if (asmFileNames != null)
            {
                // if we have fewer items (i.e. one removed, or one added)
                if (asmFileNames.Count != ReferenceAssemblyPaths.Length)
                {
                    PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "Number of assemblies changed");
                    different = true;
                }
                else
                {
                    // If one was removed, and a new one added, the newly added isn't in the old list, 
                    // so we're different.
                    foreach (String asmPath in ReferenceAssemblyPaths)
                    {
                        if (asmFileNames.Contains(asmPath) == false)
                        {
                            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "New2: " + asmPath);
                            different = true;
                            break;
                        }
                    }
                }
            }
            // If they were different, then start remembering the new list.
            if (different)
            {
                asmFileNames.Clear();
                foreach (String asmPath in ReferenceAssemblyPaths)
                {
                    asmFileNames.Add(asmPath);
                }
            }
            return different;
        }

        public bool HasLocalAssemblyHashChanged(Dictionary<String, Guid> dictionaryOfGuidHashs)
        {
            if (LocalAssemblyPath == null)
            {
                return false;
            }

            if (!_useVcMetaManaged && !s_VcMetaIsLoaded)
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "vcmeta.dll is not loaded");
                return true;    // If the Hashing code is not loaded then, yes they might have changed.
            }

            bool different = false;
            try
            {
                if (HasAssemblyChanged(LocalAssemblyPath, dictionaryOfGuidHashs))
                {
                    different = true;
                }
            }
            catch (Exception exc)
            {
                // If the vcmeta hash function threw an exception then, yes the files might have changed.
                different = true;
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "HashForWinMD threw an Exception: " + exc);
            }
            return different;
        }

        public bool HaveReferenceAssembliesHashesChanged(Dictionary<String, Guid> dictionaryOfGuidHashs)
        {
            if (ReferenceAssemblyPaths == null || ReferenceAssemblyPaths.Length == 0)
            {
                return false;
            }

            if (!_useVcMetaManaged && !s_VcMetaIsLoaded)
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "vcmeta.dll is not loaded");
                return true;    // If the Hashing code is not loaded then, yes they might have changed.
            }

            bool different = false;

            try
            {
                // Even if the current Hash is different than before - keep processing all assemblies
                // This keeps the list up to date.
                foreach (var asmPath in ReferenceAssemblyPaths)
                {
                    if (HasAssemblyChanged(asmPath, dictionaryOfGuidHashs))
                    {
                        different = true;
                    }
                }
            }
            catch (Exception)
            {
                // If the vcmeta hash function threw an exception then, yes the files might have changed.
                different = true;
            }
            return different;
        }

        // -------------- private --------------------

        private string ToLowerFullFilePath(string filename)
        {
            string lowerFullPath = Path.GetFullPath(filename).ToLowerInvariant();
            return lowerFullPath;
        }

        private bool HasAssemblyChanged(string asmPath, Dictionary<String, Guid> dictionaryOfGuidHashs)
        {
            Guid currentHash;
            int hresult = 0;
            
            if (_useVcMetaManaged)
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "Using managed HashForWinMD");
                currentHash = VCMetaManaged.HashForWinMD(asmPath);
            }
            else
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "Using HashForWinMD from vcmeta.dll");
                hresult = NativeMethods.HashForWinMD(asmPath, out currentHash);
            }

            // If we fail to get a result (hresult!=0), or we can't get a hash (Guid.Empty)
            if ((hresult != 0) || (currentHash == Guid.Empty))
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "API failure");
                return true;
            }

            // Update the lookup with the new hash.
            Guid oldHash;
            if (dictionaryOfGuidHashs.TryGetValue(asmPath, out oldHash))
            {
                if (oldHash == currentHash)
                {
                    return false;  // no change, they are the same.
                }
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "Differ: " + asmPath);
                dictionaryOfGuidHashs[asmPath] = currentHash;
            }
            else
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "New1: " + asmPath);
                // this assembly is new (probably first build)
                dictionaryOfGuidHashs.Add(asmPath, currentHash);
            }
            return true;
        }



    }
}
