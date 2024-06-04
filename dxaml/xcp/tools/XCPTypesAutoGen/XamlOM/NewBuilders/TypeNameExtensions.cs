// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.CSharp;

namespace XamlOM.NewBuilders
{
    public static class TypeNameExtensions
    {
        public static string GetFriendlyName(this Type type)
        {
            var normalizedType = Helper.NormalizeType(type);
            string friendlyName = Helper.NormalizeTypeName(normalizedType);
            if (type.IsGenericType)
            {
                friendlyName = FixGenericFriendlyName(friendlyName, normalizedType.GetGenericArguments(), false);
            }
            else
            {
                if (friendlyName.Contains("."))
                {
                    friendlyName = friendlyName.Substring(friendlyName.LastIndexOf(".") + 1);
                }
            }
            return friendlyName;
        }

        public static string GetFriendlyFullName(this Type type)
        {
            var normalizedType = Helper.NormalizeType(type);
            string friendlyName = Helper.NormalizeTypeFullName(normalizedType);
            if (type.IsGenericType && friendlyName != null)
            {
                friendlyName = FixGenericFriendlyName(friendlyName, normalizedType.GetGenericArguments(), true);
            }
            return friendlyName;
        }

        private static string FixGenericFriendlyName(string genericName, Type[] typeParameters, bool useFull)
        {
            string friendlyName = genericName;
            int iBacktick = friendlyName.IndexOf('`');
            if (iBacktick > 0)
            {
                friendlyName = friendlyName.Remove(iBacktick);
            }
            friendlyName += "<";
            for (int i = 0; i < typeParameters.Length; ++i)
            {
                var normalizedType = Helper.NormalizeType(typeParameters[i]);
                if (normalizedType.IsGenericType)
                {
                    // Make a new generic type with the normalized type
                    normalizedType = Helper.NormalizeType(normalizedType.GetGenericTypeDefinition()).MakeGenericType(normalizedType.GetGenericArguments());
                }
                string typeParamName = Helper.NormalizeTypeFullName(normalizedType);
                if (normalizedType.IsGenericType)
                {
                    typeParamName = FixGenericFriendlyName(typeParamName, normalizedType.GetGenericArguments(), useFull);
                }

                if (ShouldShortenTypeName(typeParamName, useFull))
                {
                    typeParamName = typeParamName.Substring(typeParamName.LastIndexOf(".") + 1);
                }
                friendlyName += (i == 0 ? typeParamName : "," + typeParamName);
            }
            // MIDL doesn't like ">>" so put a space to make it "> >"
            if (friendlyName.Last() == '>')
            {
                friendlyName += " ";
            }
            friendlyName += ">";

            return friendlyName;
        }

        // MIDL compiler doesn't like primitive winrt types to use their full names. When creating a generic,
        // we always want to make sure we produce Windows.Foundation.IVector<String> and not 
        // Windows.Foundation.IVector<Windows.Foundation.String>. So if the type name is any of these, we
        // should shorten them. There are a few special piggies like Byte, Char16, and Float, where those
        // aren't the real names of the types, but that's what our code gen model says they are.
        private static HashSet<string> s_primitiveWinRtTypes = new HashSet<string>()
        {
            { typeof(Windows.Foundation.Object).FullName },
            { typeof(Windows.Foundation.Boolean).FullName },
            { typeof(Windows.Foundation.Byte).FullName.Replace("Byte", "UInt8")  },
            { typeof(Windows.Foundation.Char16).FullName.Replace("Char16", "Char")  },
            { typeof(Windows.Foundation.String).FullName },
            { typeof(Windows.Foundation.Int32).FullName },
            { typeof(Windows.Foundation.Int64).FullName },
            { typeof(Windows.Foundation.Float).FullName.Replace("Float", "Single") },
            { typeof(Windows.Foundation.Double).FullName },
            { typeof(Windows.Foundation.UInt32).FullName },
            { typeof(Windows.Foundation.UInt16).FullName },
            { typeof(Windows.Foundation.UInt64).FullName },
        };
        static bool ShouldShortenTypeName(string typeName, bool useFull)
        {
            if (s_primitiveWinRtTypes.Contains(typeName))
            {
                return true;
            }
            return typeName.Contains(".") && !useFull;
        }


    }
}
