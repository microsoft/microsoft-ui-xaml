// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Data;

namespace LibManagedDll
{
    public class MyDataClass
    {
        public String MyProperty { get; set; }
    }

    public class MyStringListClass : List<String>
    {
        public int MyListCount { get; set; }
    }

    public class MyDictionaryClass : Dictionary<String, int>
    {
        public int MyDictionaryCount { get; set; }
    }

    public class MySetterClass
    {
        public int MyPrivateSet { get; private set; }
        public int MyPrivateGet { private get; set; }
    }

    public enum MyEnum
    {
        MyEnum1,
        MyEnum2,
        MyEnum7
    }

    [BindableAttribute]
    [Windows.Foundation.Metadata.CreateFromString(MethodName = "test")]
    public class MyEnumTestClass
    {
        public MyEnum MyEnumValue { get; set; }
    }

    public class MyFieldClass
    {
        public String MyProperty { get; set; }

        public String MyStringField;
    }

    public class MyEventClass
    {
        public event EventHandler MyEvent;

        internal void makethecompilerhappy()
        {
            if (MyEvent != null)
            {
                MyEvent(this, null);
            }
        }
    }

    public class MyObservableClass
    {
        public int MyDependencyProperty
        {
            get
            {
                return 1; //  (int)GetValue(MyDependencyPropertyProperty);
            }
            set
            { // SetValue(MyDependencyPropertyProperty, value); 
            }
        }

        // Using a DependencyProperty as the backing store for MyDependencyProperty.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty MyDependencyPropertyProperty =
            DependencyProperty.Register("MyDependencyProperty", typeof(int), typeof(MyObservableClass), new PropertyMetadata(0));
    }


    public class MyContentClass
    {
        public object Content { get; set; }
    }

    public class MyBindableClass
    {
        public object Content { get; set; }
    }

}
