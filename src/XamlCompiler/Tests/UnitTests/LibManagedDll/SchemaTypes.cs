// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace LibManagedDll
{
    public class NullableTypeHolder: Control
    {
        public int? NullableInt { get; set; }
    }

    public class ArrayHolder : Control
    {
        public String[] StrArray { get; set; }
    }

    public class DictionaryHolder : Control
    {
        DictionaryWith2Adds _dict = new DictionaryWith2Adds();
        public DictionaryWith2Adds DictionaryWith2Adds
        {
            get { return _dict; }
        }
    }

    public class DictionaryWith2Adds : Dictionary<string, object>
    {
        // I added this extra "Add" method to comfuse the XAML compiler's Schema.
        // In the future we may make it smart enought to deal with this.
        public void Add(string key, string value)
        {
            // the user might want to add extra code when adding a string
            // to what is really a dictionary of "Objects".
            value += "(" + value.Length.ToString() + ")";
            this.Add(key, (Object)value);
        }
    }

    public class UiPrimitives
    {
        public Thickness Thickness { get; set; }
        public Point Point { get; set; }
        public Rect Rect { get; set; }
        public Size Size { get; set; }
        public Orientation Orientation { get; set; }
    }

    public class AttachablePropertyProviderWithMultipleSets: Control
    {
        public static string GetFoo(DependencyObject obj) { return (string)obj.GetValue(FooProperty); }
        public static void SetFoo(DependencyObject obj, string value) { obj.SetValue(FooProperty, value); }
        public static void SetFoo(DependencyObject obj, int value) { }

        public static int GetSomeOtherThing(Object o) { return 42; }
        public static int GetSomeOtherThing(DependencyObject dobj) { return 42; }

        // Lone Setters are dangerous due to a bug in System.Xaml (type of prop is "void")
        public static void SetLoneSetters(DependencyObject obj, string value) { }

        private static readonly DependencyProperty _FooProperty =
        DependencyProperty.RegisterAttached("Foo", typeof(string), typeof(AttachablePropertyProviderWithMultipleSets), new PropertyMetadata("Foo"));
        public static DependencyProperty FooProperty { get { return _FooProperty; } }
    }

    public class HasIndexer
    {
        public string this[int num]
        {
            get { return "hello"; }
            set {  }
        }
    }

    public class HasInstanceItem
    {
        public string Item { get; set; }
    }

    public class HasAttachableItem
    {
        public static string GetItem(DependencyObject obj)
        {
            return "Hello";
        }

        public static void SetItem(DependencyObject obj, string value)
        {
            ;
        }
    }

    public class HasIndexerRenamed
    {
        [System.Runtime.CompilerServices.IndexerName("NotItem")]
        public string this[int num]
        {
            get { return "hello"; }
            set { }
        }
    }

}
