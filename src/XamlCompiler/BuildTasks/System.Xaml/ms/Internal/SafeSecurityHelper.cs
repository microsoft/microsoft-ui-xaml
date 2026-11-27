// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

// Purpose:  Helper functions that require elevation but are safe to use.
// History:
//   10/15/04:    Microsoft        Created

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Security;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using TypeConverterHelper = System.Xaml.TypeConverterHelper;

// The SafeSecurityHelper class differs between assemblies and could not actually be
//  shared, so it is duplicated across namespaces to prevent name collision.
namespace System.Xaml
{
    internal static partial class SafeSecurityHelper
    {




        // Cache of Assembly -> AssemblyName, because calling new AssemblyName() is expensive.
        // If the assembly is static, the key is the assembly; if it's dynamic, the key is a WeakRefKey
        // pointing to the assembly, so we don't root collectible assemblies.
        //
        // This cache is bound (gated) by the number of assemblies in the appdomain.
        // We use a callback on GC to purge out collected assemblies, so we don't grow indefinitely.
        //
        static Dictionary<object, AssemblyName> _assemblies; // get key via GetKeyForAssembly
        static object syncObject = new object();
        static bool _isGCCallbackPending;

        // PERF: Cache delegate for CleanupCollectedAssemblies to avoid allocating it each time.
        static readonly WaitCallback _cleanupCollectedAssemblies = CleanupCollectedAssemblies;

        /// <summary>
        ///     This function iterates through the assemblies loaded in the current
        ///     AppDomain and finds one that has the same assembly name passed in.
        /// </summary>
        /// <SecurityNote>
        ///     The method is being kept in this class to ease auditing and
        ///     should have a security review if changed.
        ///
        ///     WARNING! Don't use this method for retrievals of assemblies that
        ///              should rely on a strong match with the given assembly name
        ///              since this method allows assemblies with the same short name
        ///              to be returned even when other name parts are different.
        ///
        ///        E.g.  AssemblyShortName
        ///     matches  AssemblyShortName, Version=2.0.0.0, Culture=en-us, PublicKeyToken=b03f5f7f11d50a3a
        /// </SecurityNote>
        internal static Assembly GetLoadedAssembly(AssemblyName assemblyName)
        {
            Assembly[] assemblies = AppDomain.CurrentDomain.GetAssemblies();

            Version reqVersion = assemblyName.Version;
            CultureInfo reqCulture = assemblyName.CultureInfo;
            byte[] reqKeyToken = assemblyName.GetPublicKeyToken();

            for (int i = assemblies.Length - 1; i >= 0; i--)
            {
                AssemblyName curAsmName = GetAssemblyName(assemblies[i]);
                Version curVersion = curAsmName.Version;
                CultureInfo curCulture = curAsmName.CultureInfo;
                byte[] curKeyToken = curAsmName.GetPublicKeyToken();

                if ( (String.Compare(curAsmName.Name, assemblyName.Name, true, TypeConverterHelper.InvariantEnglishUS) == 0) &&
                     (reqVersion == null || reqVersion.Equals(curVersion)) &&
                     (reqCulture == null || reqCulture.Equals(curCulture)) &&
                     (reqKeyToken == null || IsSameKeyToken(reqKeyToken, curKeyToken) ) )
                {
                    return assemblies[i];
                }
            }
            return null;
        }

        static AssemblyName GetAssemblyName(Assembly assembly)
        {
            object key = assembly.IsDynamic ? (object)new WeakRefKey(assembly) : assembly;
            lock (syncObject)
            {
                AssemblyName result;
                if (_assemblies == null)
                {
                    _assemblies = new Dictionary<object, AssemblyName>();
                }
                else
	            {
                    if (_assemblies.TryGetValue(key, out result))
                    {
                        return result;
                    }
	            }
                //
                // We use AssemblyName ctor here because GetName demands FileIOPermission
                // and does load more than just the required information.
                // Essentially we use AssemblyName just to help parsing the name, version, culture
                // and public key token from the assembly's name.
                //
                result = new AssemblyName(assembly.FullName);
                _assemblies.Add(key, result);
                if (assembly.IsDynamic && !_isGCCallbackPending)
                {
                    // Make sure we clean up the cache if/when this dynamic assembly is GCed
                    GCNotificationToken.RegisterCallback(_cleanupCollectedAssemblies, null);
                    _isGCCallbackPending  = true;
                }
                return result;
            }
        }

        // After a GC, clean up the weakrefs to any collected dynamic assemblies
        static void CleanupCollectedAssemblies(object state) // dummy parameter required by WaitCallback definition
        {
            bool foundLiveDynamicAssemblies = false;
            List<object> keysToRemove = null;
            lock (syncObject)
            {
                foreach (object key in _assemblies.Keys)
                {
                    WeakReference weakRef = key as WeakReference;
                    if (weakRef == null)
                    {
                        continue;
                    }
                    if (weakRef.IsAlive)
                    {
                        // There is a weak ref that is still alive, register another GC callback for next time
                        foundLiveDynamicAssemblies = true;
                    }
                    else
                    {
                        // The target has been collected, add it to our list of keys to remove
                        if (keysToRemove == null)
                        {
                            keysToRemove = new List<object>();
                        }
                        keysToRemove.Add(key);
                    }
                }
                if (keysToRemove != null)
                {
                    foreach (object key in keysToRemove)
                    {
                        _assemblies.Remove(key);
                    }
                }
                if (foundLiveDynamicAssemblies)
                {
                    GCNotificationToken.RegisterCallback(_cleanupCollectedAssemblies, null);
                }
                else
                {
                    _isGCCallbackPending = false;
                }
            }
        }


        //
        // Determine if two Public Key Tokens are the same.
        //
        internal
        static bool IsSameKeyToken(byte[] reqKeyToken, byte[] curKeyToken)
        {
           bool isSame = false;

           if (reqKeyToken == null && curKeyToken == null)
           {
               // Both Key Tokens are not set, treat them as same.
               isSame = true;
           }
           else if (reqKeyToken != null && curKeyToken != null)
           {
               // Both KeyTokens are set.
               if (reqKeyToken.Length == curKeyToken.Length)
               {
                   isSame = true;

                   for (int i = 0; i < reqKeyToken.Length; i++)
                   {
                      if (reqKeyToken[i] != curKeyToken[i])
                      {
                         isSame = false;
                         break;
                      }
                   }
               }
           }

           return isSame;
        }



        internal const string IMAGE = "image";
    }

    // for use as the key to a dictionary, when the "real" key is an object
    // that we should not keep alive by a strong reference.
    class WeakRefKey : WeakReference
    {
        public WeakRefKey(object target)
            :base(target)
        {
            Debug.Assert(target != null);
            _hashCode = target.GetHashCode();
        }

        public override int GetHashCode()
        {
            return _hashCode;
        }

        public override bool Equals(object o)
        {
            WeakRefKey weakRef = o as WeakRefKey;
            if (weakRef != null)
            {
                object target1 = Target;
                object target2 = weakRef.Target;

                if (target1 != null && target2 != null)
                {
                    return (target1 == target2);
                }
            }
            return base.Equals(o);
        }

        public static bool operator ==(WeakRefKey left, WeakRefKey right)
        {
            if (object.ReferenceEquals(left, null))
            {
                return object.ReferenceEquals(right, null);
            }
            return left.Equals(right);
        }

        public static bool operator !=(WeakRefKey left, WeakRefKey right)
        {
            return !(left == right);
        }

        int _hashCode;  // cache target's hashcode, lest it get GC'd out from under us
    }

    // This cleanup token will be immediately thrown away and as a result it will
    // (a couple of GCs later) make it into the finalization queue and when finalized
    // will kick off a thread-pool job that you can use to purge a weakref cache.
    class GCNotificationToken
    {
        WaitCallback callback;
        object state;

        GCNotificationToken(WaitCallback callback, object state)
        {
            this.callback = callback;
            this.state = state;
        }

        ~GCNotificationToken()
        {
            // Schedule cleanup
            ThreadPool.QueueUserWorkItem(callback, state);
        }

        internal static void RegisterCallback(WaitCallback callback, object state)
        {
            new GCNotificationToken(callback, state);
        }
    }
}
