// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.Collections;

namespace Win8Xaml.CompilerProxies
{
    public class XamlClassCodeInfo
    {
        static ProxyHelper _xcciType;
        static PropertyInfo _classNameProperty;
        static PropertyInfo _baseTypeNameProperty;
        static PropertyInfo _baseTypeProperty;
        static PropertyInfo _baseApparentRelativeFolderProperty;
        static PropertyInfo _baseFileNameProperty;
        static PropertyInfo _bindUniverses;
        static PropertyInfo _perXamlFileInfoProperty;
        static PropertyInfo _fieldDeclarationsProperty;
        static PropertyInfo _classTypeProperty;
        static MethodInfo _AddXamlFileInfoMethod;
        static MethodInfo _toStringMethod;

        object _instance;

        static XamlClassCodeInfo()
        {
            _xcciType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XamlClassCodeInfo");
            _classNameProperty = _xcciType.GetProperty("ClassName");
            _classTypeProperty = _xcciType.GetProperty("ClassType");
            _baseTypeNameProperty = _xcciType.GetProperty("BaseTypeName");
            _baseTypeProperty = _xcciType.GetProperty("BaseType");
            _baseApparentRelativeFolderProperty = _xcciType.GetProperty("BaseApparentRelativeFolder");
            _baseFileNameProperty = _xcciType.GetProperty("BaseFileName");
            _bindUniverses = _xcciType.GetProperty("BindUniverses");
            _perXamlFileInfoProperty = _xcciType.GetProperty("PerXamlFileInfo");
            _fieldDeclarationsProperty = _xcciType.GetProperty("FieldDeclarations");
            _AddXamlFileInfoMethod = _xcciType.GetMethod("AddXamlFileInfo");
            _toStringMethod = _xcciType.GetMethod("ToString");
        }

        // ------------------ instance properties ----------------

        public XamlClassCodeInfo(string classFullName, bool isApplication)
        {
            Object[] args = new Object[] { classFullName, isApplication };
            _instance = _xcciType.CreateInstance(args);
        }

        public XamlClassCodeInfo(object instance)
        {
            _instance = instance;
        }

        public object Instance
        {
            get { return _instance; }
        }

        public ClassName ClassName
        {
            get
            {
                object obj = _classNameProperty.GetValue(_instance, null);
                return (obj == null) ? null : new ClassName(obj);
            }
            set
            {
                _classNameProperty.SetValue(_instance, value.Instance);
            }
        }

        public TypeForCodeGen ClassType
        {
            get
            {
                object obj = _classTypeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new TypeForCodeGen(obj);  // this is a proxied type.
            }
            set { _classTypeProperty.SetValue(_instance, value.Instance); }
        }

        public string BaseTypeName
        {
            get { return (string)_baseTypeNameProperty.GetValue(_instance, null); }
            set { _baseTypeNameProperty.SetValue(_instance, value); }
        }

        public TypeForCodeGen BaseType
        {
            get
            {
                object obj = _baseTypeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new TypeForCodeGen(obj);  // this is a proxied type.
            }
        }

        public string BaseApparentRelativeFolder
        {
            get
            {
                return (string)_baseApparentRelativeFolderProperty.GetValue(_instance, null);
            }
        }

        public string BaseFileName
        {
            get
            {
                return (string)_baseFileNameProperty.GetValue(_instance, null);
            }
        }
        public List<BindUniverse> BindUniverses
        {
            get
            {
                IEnumerable objectList = (IEnumerable) _bindUniverses.GetValue(_instance, null);
                List<BindUniverse> bindUniverses = new List<BindUniverse>();
                foreach (Object obj in objectList)
                {
                    BindUniverse bindUniverse = new BindUniverse(obj);
                    bindUniverses.Add(bindUniverse);
                }
                return bindUniverses;
            }
        }

        public List<XamlFileCodeInfo> PerXamlFileInfo
        {
            get
            {
                IEnumerable objectList = (IEnumerable)_perXamlFileInfoProperty.GetValue(_instance, null);
                List<XamlFileCodeInfo> perXamlFileInfoList = new List<XamlFileCodeInfo>();
                foreach (Object obj in objectList)
                {
                    XamlFileCodeInfo xamlFileCodeInfo = new XamlFileCodeInfo(obj);
                    perXamlFileInfoList.Add(xamlFileCodeInfo);
                }
                return perXamlFileInfoList;

            }
        }
        public List<FieldDefinition> FieldDeclarations
        {
            get
            {
                IEnumerable objectList = (IEnumerable)_fieldDeclarationsProperty.GetValue(_instance, null);
                List<FieldDefinition> fieldDeclarationsList = new List<FieldDefinition>();
                foreach (Object obj in objectList)
                {
                    FieldDefinition fieldDefinition = new FieldDefinition(obj);
                    fieldDeclarationsList.Add(fieldDefinition);
                }
                return fieldDeclarationsList;

            }
        }

        public void AddXamlFileInfo(XamlFileCodeInfo fileCodeInfo)
        {
            _AddXamlFileInfoMethod.Invoke(_instance, new object[] { fileCodeInfo.Instance });
        }

        public override string ToString()
        {
            return (string)_toStringMethod.Invoke(_instance, null);
        }

    }
}
