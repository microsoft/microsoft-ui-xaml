// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <collection.h>

using namespace Platform;
using namespace Platform::Collections;

ref class MocoBasicFlatDataModel sealed: xaml_data::ICustomPropertyProvider, xaml_data::INotifyPropertyChanged
{
public:
    MocoBasicFlatDataModel(String^ name)
    {
        Name = name;
        MinHeight = 50;
    }

    virtual xaml_data::ICustomProperty^ GetCustomProperty(String^ name)
    {
        return nullptr;
    }

    virtual xaml_data::ICustomProperty^ GetIndexedProperty(String^ name, ::Windows::UI::Xaml::Interop::TypeName typeName)
    {
        throw ref new NotImplementedException();
    }

    virtual String^ GetStringRepresentation()
    {
        return Name;
    }

    virtual property ::Windows::UI::Xaml::Interop::TypeName Type
    {
        virtual ::Windows::UI::Xaml::Interop::TypeName get()
        {
            ::Windows::UI::Xaml::Interop::TypeName tn;
            tn.Name = L"MocoBasicFlatDataModel";
            tn.Kind = ::Windows::UI::Xaml::Interop::TypeKind::Custom;
            return tn;
        }
    }

    void OnPropertyChanged(String^ propertyName)
    {
        PropertyChanged(this, ref new xaml_data::PropertyChangedEventArgs(propertyName));
    }

    virtual event xaml_data::PropertyChangedEventHandler^ PropertyChanged;

    property String^ Name
    {
        String^ get()
        {
            return _name;
        }
        void set(String^ value)
        {
            if (_name != value)
            {
                _name = value;
                OnPropertyChanged(L"Name");
            }
        }
    }

    property int MinHeight
    {
        int get()
        {
            return _minHeight;
        }
        void set(int value)
        {
            if (_minHeight != value)
            {
                _minHeight = value;
            }
        }
    }

    property bool IsExpanded
    {
        bool get()
        {
            return _isExpanded;
        }
        void set(bool value)
        {
            if (_isExpanded != value)
            {
                _isExpanded = value;
            }
        }
    }

private:
    String^ _name;
    int _minHeight;
    bool _isExpanded;
};

ref class MocoBasicGroupedDataModel sealed : public wfc::IVector<Object^>, xaml_data::ICustomPropertyProvider, xaml_data::INotifyPropertyChanged
{
public:
    MocoBasicGroupedDataModel(String^ key)
    {
        Header = key;
        _height = 50;
        _width = 50;
    }

    virtual xaml_data::ICustomProperty^ GetCustomProperty(String^ name)
    {
        return nullptr;
    }

    virtual xaml_data::ICustomProperty^ GetIndexedProperty(String^ name, ::Windows::UI::Xaml::Interop::TypeName typeName)
    {
        throw ref new NotImplementedException();
    }

    virtual String^ GetStringRepresentation()
    {
        return Header;
    }

    virtual property ::Windows::UI::Xaml::Interop::TypeName Type
    {
        virtual ::Windows::UI::Xaml::Interop::TypeName get()
        {
            ::Windows::UI::Xaml::Interop::TypeName tn;
            tn.Name = L"MocoBasicGroupedDataModel";
            tn.Kind = ::Windows::UI::Xaml::Interop::TypeKind::Custom;
            return tn;
        }
    }

    void OnPropertyChanged(String^ propertyName)
    {
        PropertyChanged(this, ref new xaml_data::PropertyChangedEventArgs(propertyName));
    }

    virtual event xaml_data::PropertyChangedEventHandler^ PropertyChanged;

    property String^ Header
    {
        String^ get()
        {
            return _header;
        }
        void set(String^ value)
        {
            if (_header != value)
            {
                _header = value;
                OnPropertyChanged(L"Header");
            }
        }
    }
    property int Height
    {
        int get()
        {
            return _height;
        }

        void set(int value)
        {
            if (_height != value)
            {
                _height = value;
            }
        }
    }
    property int Width
    {
        int get()
        {
            return _width;
        }
        void set(int value)
        {
            if (_width != value)
            {
                _width = value;
            }
        }
    }

    // IVector overrides
    virtual ::Windows::Foundation::Collections::IIterator<Object^>^ First()
    {
        return _items.First();
    }

    virtual property unsigned int Size
    {
        unsigned int get ()
        {
            return _items.Size;
        }
    }

    virtual Object^ GetAt(unsigned int location)
    {
        return _items.GetAt(location);
    }

    virtual ::Windows::Foundation::Collections::IVectorView<Object^>^ GetView()
    {
        return _items.GetView();
    }

    virtual bool IndexOf(Object^ value, unsigned int* index)
    {
        return _items.IndexOf(value, index);
    }

    virtual void SetAt(unsigned int index, Object^ value)
    {
        _items.SetAt(index, value);
    }

    virtual void InsertAt(unsigned int index, Object^ value)
    {
        _items.InsertAt(index, value);
    }

    virtual void RemoveAt(unsigned int index)
    {
        _items.RemoveAt(index);
    }

    virtual void Append(Object^ value)
    {
        _items.Append(value);
    }

    virtual void RemoveAtEnd()
    {
        _items.RemoveAtEnd();
    }

    virtual void Clear()
    {
        _items.Clear();
    }

    virtual unsigned int GetMany(unsigned int startIndex, WriteOnlyArray<Object^>^ items)
    {
        return _items.GetMany(startIndex, items);
    }

    virtual void ReplaceAll(const Array<Object^>^ items)
    {
        _items.ReplaceAll(items);
    }

private:
    Vector<Object^> _items;
    String^ _header;
    int _height;
    int _width;
};


ref class MocoBasicDataSource sealed
{
public:
     static ::Windows::Foundation::Collections::IVector<Object^>^ GetGroupedDataCollection(int count, int itemsCount)
     {
         auto dataCollection = ref new Collections::Vector<Object^>();
         PopulateWithGroupedData(dataCollection, count, itemsCount);
         return dataCollection;
     }

 private:

     static void PopulateWithGroupedData(::Windows::Foundation::Collections::IVector<Object^>^ dataCollection, int count, int itemsCount)
     {
         String^ groupHeaderNames = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

         for (int i = 0; i < count; i++)
         {
             std::wstring ws(groupHeaderNames->Data());
             auto substring = ws.substr(i % groupHeaderNames->Length(), 1);
             MocoBasicGroupedDataModel^ group = ref new MocoBasicGroupedDataModel(ref new String(substring.data()));
             for(int j = 0; j < itemsCount; j++)
             {
                 group->Append(ref new MocoBasicFlatDataModel(group->Header + " - Item" + j));
             }
             dataCollection->Append(group);
         }
     }
 };
