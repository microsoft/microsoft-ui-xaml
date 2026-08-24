// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------------
//
// Copyright(c) 2012 Microsoft Corporation
//--------------------------------------------------------------------------------------------

using System.Collections.Generic;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{

    public class Roots
    {
        private object _instance;
        private PropertyInfo _propertyPathNames;
        static ProxyHelper _rootsType;
        static ProxyHelper _rootPropertyPathNameType;

        public Roots(object instance)
        {
            this._instance = instance;
            _rootsType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.RootLog.Roots");
            _rootPropertyPathNameType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.RootLog.RootPropertyPathName");
            this._propertyPathNames = _rootsType.GetProperty("PropertyPathNames");
        }

        public List<string> PropertyPathNames
        {
            get
            {
                List<string> ret = new List<string>();
                PropertyInfo name = _rootPropertyPathNameType.GetProperty("Name");
                var v = this._propertyPathNames.GetValue(this._instance, null) as System.Collections.IEnumerable;
                foreach(object o in v)
                {
                    ret.Add(name.GetValue(o) as string);
                }
                return ret;
            }
        }
    }
}
