// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;

// No Namespace
public class OutSideAllNSes: Control
{
    public string prop { get; set; }
}

namespace LibManagedDll
{
    //A type defined in UniversalApiContract version 9.0.0.0.  As of this writing the version is 4.0.0.0, so trying to use this type should fail.
    [Windows.Foundation.Metadata.ContractVersion("Windows.Foundation.UniversalApiContract", 0x90000)]
    public class TypeNotInMinVersion
    {
        public string Garbage { get; set; }
    }

    //A type defined in a contract which doesn't exist at all in the min version - we should never be able to use this.
    [Windows.Foundation.Metadata.ContractVersion("Windows.Foundation.NonExistentContract", 0x10000)]
    public class NonExistingContractInMinVersion : InterfaceNotPresentInMinVersion
    {
        public string InvalidMember { get; set; }
    }

    //A type defined in UniversalApiContract version 9.0.0.0.  As of this writing the version is 4.0.0.0, so trying to use the member defined in this interface should fail.
    [Windows.Foundation.Metadata.ContractVersion("Windows.Foundation.UniversalApiContract", 0x90000)]
    public interface InterfaceNotPresentInMinVersion
    {
        string InvalidMember { get; set; }
    }

    //A type defined in UniversalApiContract version 1.0.0.0, but which implements an interface from 9.0.0.0 which adds a new member.
    //We should be able to use this type without error unless we try to access the InvalidMember property.
    [Windows.Foundation.Metadata.ContractVersion("Windows.Foundation.UniversalApiContract", 0x10000)]
    public class MemberNotPresentInMinVersion : InterfaceNotPresentInMinVersion
    {
        public string InvalidMember { get; set; }
    }

    //Experimental class, using should raise compiler warning
    [Windows.Foundation.Metadata.Experimental]
    public class ExperimentalClass
    {
        public string Member { get; set; }
    }

    //TODO: add a test for a non-experimental class using an experimental member? Windows.Foundation.Metadata.Experimental
    //is invalid for members despite its description...

    public class SimpleClass
    {
        public string Prop1 { get; set; }
        public string Prop2 { get; set; }
        public string Prop3 { get; set; }
    }

    class InternalType
    {
        public String Prop1 { get; set; }
    }

    public class ReadOnlyHolder
    {
        public string ReadOnlyStringProp { get { return "Hello, World!"; } }
        public SimpleClass ReadOnlySimpleClassProp { get { return null; } }
    }

    public class WriteOnlyHolder
    {
        string _value;

        public string PrivateSetStringProp { set; private get; }
        public string WOStringProp { set { _value = value; } }
    }

    public class IntList : List<Int32> { }

    public class ButtonDictionary : Dictionary<string, Button> { }

    public class CollectionHolder
    {
        public IntList IntListProp { get; set; }
        public ButtonDictionary ButtonDictionaryProp { get; set; }
    }

    public class PrimitiveHolder
    {
        public short Int16Prop { get; set; }
        public ushort UInt16Prop { get; set; }
        public char Char16Prop { get; set; }
        public int Int32Prop { get; set; }
        public long Int64Prop { get; set; }
        public uint UInt32Prop { get; set; }
        public ulong UInt64Prop { get; set; }
    }

    [ContentPropertyAttribute(Name = "Something")]
    public class GoodCpaClass : Control
    {
        public String Something { get; set; }
        public String SomethingElse { get; set; }
    }

    [ContentPropertyAttribute(Name = "Random")]
    public class BadCpaClass : Control
    {
        public String Something { get; set; }
        public String SomethingElse { get; set; }
    }

    public class MissingCPA : Control
    {
        public String Something { get; set; }
        public String SomethingElse { get; set; }
    }

    public abstract class AbstractBaseClass : Control
    {
        public abstract String AbProp { get; set; }
    }

    public class NoZeroArgumentCtor : Control
    {
        private NoZeroArgumentCtor()
        {
        }

        String _foo;

        public NoZeroArgumentCtor(string foo)
        {
            _foo = foo;
        }
    }

    public class BadProps : Control
    {
        public Enum EnumProp { get; set; }
        public SByte SignedByte { get; set; }
    }

    public class MultiAdd : List<Object>
    {
        public void Add(String str)
        {
            this.Add((object)str);
        }
    }

    public class NonForwardedClass
    {
        public ForwardedType ForwardedTypeMember { get; set; }

        public ForwardedType ForwardedTypeFunction (ForwardedType param, IList<ForwardedType> templatedParam)
        {
            return null;
        }
    }

    public class InheritForwardedType : ForwardedType
    {}

    public class CustomMarkupExtension : MarkupExtension
    {}
}
