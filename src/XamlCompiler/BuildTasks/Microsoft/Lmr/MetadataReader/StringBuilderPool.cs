// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.IO;
using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// This is a very simple pool with a fixed size (MaxListSize). Additional allocations are allowed to GC.
    /// Exposes two static methods, Get and Release which are threadsafe and protected via a synclock.
    /// </summary>
    /// <remarks>
    /// Used to decrease GC pressure and avoid lots of small allocation for each interop call. 
    /// </remarks>
    static class StringBuilderPool
    {
        private const int DefaultCapacity = 128;

        // Max number of SB's kept in the list. It need not be large to get memory savings benefits.
        private const int MaxListSize = 5;  

        // Prevents us from keeping extremely large SB's around.
        private const int MaxCapacity = 4096;

        private static StringBuilder[] s_pool = new StringBuilder[MaxListSize];
        private static object s_synclock = new object();

        /// <summary>
        /// Returns a stringbuilder from the pool or creates a new one with the default capacity.
        /// </summary>
        public static StringBuilder Get()
        {
            return Get(DefaultCapacity);
        }

        /// <summary>
        /// Returns a stringbuilder from the pool or creates a new one, ensuring the capacity is at least "capacity".
        /// </summary>
        public static StringBuilder Get(int capacity)
        {
            StringBuilder stringResult = null;

            lock (s_synclock)
            {
                for (int i = 0; i < s_pool.Length; i++)
                {
                    if (s_pool[i] != null)
                    {
                        stringResult = s_pool[i];
                        s_pool[i] = null;
                        break;
                    }
                }
            }

            // Couldn't find available SB in the pool, just create a new one. 
            if (stringResult == null)
            {
                stringResult = new StringBuilder(capacity);
            }

            // Important: Length needs to be set before Capacity is changed. If done the other way around,
            // setting Length could change Capacity, which would affect all interop calls that use
            // this buffer (since Capacity is usually passed as one of the arguments).
            stringResult.Length = 0;

            stringResult.EnsureCapacity(capacity);

            Debug.Assert(stringResult.Capacity >= capacity,
                string.Format(System.Globalization.CultureInfo.InvariantCulture, 
                "stringResult.Length = 0; gave us smaller StringBuilder than what we requested. We wanted {0} chars and we got {1} chars",
                capacity, stringResult.Capacity));

            return stringResult;
        }

        /// <summary>
        /// Releases the stringbuilder back into the pool.
        /// The parameter is passed by reference and set to null so that no further access to it is done.
        /// </summary>
        public static void Release(ref StringBuilder builder)
        {
            if (builder != null && builder.Capacity < MaxCapacity)
            {
                lock (s_synclock)
                {
                    for (int i = 0; i < s_pool.Length; i++)
                    {
                        if (s_pool[i] == null)
                        {
                            s_pool[i] = builder;
                            break;
                        }
                    }
                }
            }

            builder = null;  // Ensure no one uses this after this method call.
        }
    }
}
