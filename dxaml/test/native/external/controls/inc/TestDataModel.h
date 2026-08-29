// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <collection.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Platform;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {
    ref class MocoBasicFlatDataModel sealed: xaml_data::ICustomPropertyProvider, xaml_data::INotifyPropertyChanged
    {
    public:
    MocoBasicFlatDataModel(Platform::String^ name)
        {
            Name = name;
            Subject = L"A dummy subject";
            Message = L"This is a dummy message";
            MinHeight = 50;
        }
    
        virtual xaml_data::ICustomProperty^ GetCustomProperty(Platform::String^ name)
        {
            return nullptr;
        }

        virtual xaml_data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, ::Windows::UI::Xaml::Interop::TypeName typeName)
        {
            throw ref new Platform::NotImplementedException(); 
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

        property Platform::String^ Name 
        {
            Platform::String^ get()
            {
                return _name;
            }
            void set(Platform::String^ value)
            {
                if (_name != value)
                {
                    _name = value;
                    OnPropertyChanged(L"Name");
                }
            }
        }

        property Platform::String^ Subject 
        {
            Platform::String^ get()
            {
                return _subject;
            }
            void set(Platform::String^ value)
            {
                if (_subject != value)
                {
                    _subject = value;
                    OnPropertyChanged(L"Subject");
                }
            }
        }

        property Platform::String^ Message 
        {
            Platform::String^ get()
            {
                return _message;
            }
            void set(Platform::String^ value)
            {
                if (_message != value)
                {
                    _message = value;
                    OnPropertyChanged(L"Message");
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
        Platform::String^ _name;
        Platform::String^ _subject;
        Platform::String^ _message;
        int _minHeight;
        bool _isExpanded;
    };
    
    ref class MocoBasicGroupedDataModel sealed : public wfc::IVector<Object^>, xaml_data::ICustomPropertyProvider, xaml_data::INotifyPropertyChanged
    {
    public:
        MocoBasicGroupedDataModel(Platform::String^ key)
        {
            Header = key;
            _height = 50;
            _width = 50;
        }

        virtual xaml_data::ICustomProperty^ GetCustomProperty(Platform::String^ name)
        {
            return nullptr;
        }

        virtual xaml_data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, ::Windows::UI::Xaml::Interop::TypeName typeName)
        {
            throw ref new Platform::NotImplementedException(); 
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

        property Platform::String^ Header 
        {
            Platform::String^ get()
            {
                return _header;
            }
            void set(Platform::String^ value)
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

        virtual unsigned int GetMany(unsigned int startIndex, Platform::WriteOnlyArray<Object^>^ items)
        {
            return _items.GetMany(startIndex, items);
        }

        virtual void ReplaceAll(const Platform::Array<Object^>^ items)
        {
            _items.ReplaceAll(items);
        }

    private:
        Platform::Collections::Vector<Object^> _items;
        Platform::String^ _header;
        int _height;
        int _width;
    };

    ref class VectorChangedEventArgs sealed : public ::Windows::Foundation::Collections::IVectorChangedEventArgs
    {
    public:
        VectorChangedEventArgs(wfc::CollectionChange change, unsigned int index)
        {
            this->change = change;
            this->index = index;
        }

        virtual property wfc::CollectionChange CollectionChange
        {
            wfc::CollectionChange get()
            {
                return change;
            }
        }

        virtual property unsigned int Index
        {
            unsigned int get()
            {
                return index;
            }
        }

    private:
        wfc::CollectionChange change;
        unsigned int index;
    };

    ref class MocoResettableCollection sealed : public wfc::IObservableVector<Object^>
    {
    public:
        MocoResettableCollection()
        {
        }

        void Reset()
        {
            OnVectorChanged(wfc::CollectionChange::Reset, 0);
        }

        void ResetRemove(unsigned int index)
        {
            if (index >= 0)
            {
                _items.RemoveAt(index);
            }
            
            OnVectorChanged(wfc::CollectionChange::Reset, 0);
        }

        void ResetFlip()
        {
            for (unsigned int i = 0; i < _items.Size / 2; i++)
            {
                unsigned int otherIndex = _items.Size - 1 - i;

                Object^ temp = _items.GetAt(i);
                _items.SetAt(i, _items.GetAt(otherIndex));
                _items.SetAt(otherIndex, temp);
            }

            OnVectorChanged(wfc::CollectionChange::Reset, 0);
        }

        void ResetRemoveFlip(unsigned int index)
        {
            if (index >= 0)
            {
                _items.RemoveAt(index);
            }
            
            ResetFlip();
        }
        
        void OnVectorChanged(wfc::CollectionChange change, unsigned int index)
        {
            VectorChanged(this, ref new VectorChangedEventArgs(change, index));
        }

        // IObservableVector overrides
        virtual event wfc::VectorChangedEventHandler<Object^>^ VectorChanged;
        
        // IVector overrides
        virtual ::Windows::Foundation::Collections::IIterator<Object^>^ First()
        {
            return _items.First();
        }

        virtual property unsigned int Size
        {
            unsigned int get()
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
            OnVectorChanged(wfc::CollectionChange::ItemChanged, index);
        }

        virtual void InsertAt(unsigned int index, Object^ value)
        {
            _items.InsertAt(index, value);
            OnVectorChanged(wfc::CollectionChange::ItemInserted, index);
        }

        virtual void RemoveAt(unsigned int index)
        {
            _items.RemoveAt(index);
            OnVectorChanged(wfc::CollectionChange::ItemRemoved, index);
        }

        virtual void Append(Object^ value)
        {
            _items.Append(value);
            OnVectorChanged(wfc::CollectionChange::ItemInserted, _items.Size - 1);
        }

        virtual void RemoveAtEnd()
        {
            _items.RemoveAtEnd();
            OnVectorChanged(wfc::CollectionChange::ItemRemoved, _items.Size);
        }

        virtual void Clear()
        {
            _items.Clear();
            OnVectorChanged(wfc::CollectionChange::Reset, 0);
        }

        virtual unsigned int GetMany(unsigned int startIndex, Platform::WriteOnlyArray<Object^>^ items)
        {
            return _items.GetMany(startIndex, items);
        }

        virtual void ReplaceAll(const Platform::Array<Object^>^ items)
        {
            _items.ReplaceAll(items);
            OnVectorChanged(wfc::CollectionChange::Reset, 0);
        }

    private:
        Platform::Collections::Vector<Object^> _items;
    };

   ref class MocoBasicDataSource sealed
    {
    public:
        static ::Windows::Foundation::Collections::IVector<Object^>^ GetFlatDataCollection(int count)
        {
            Platform::Collections::Vector<Object^>^ dataCollection = ref new Platform::Collections::Vector<Object^>();
            PopulateWithFlatData(dataCollection, count);
            return dataCollection;
        }

        static ::Windows::Foundation::Collections::IVector<Object^>^ GetGroupedDataCollection(int count, int itemsCount, bool includeEmptyGroups)
        {
            auto dataCollection = ref new Platform::Collections::Vector<Object^>();
            PopulateWithGroupedData(dataCollection, count, itemsCount, includeEmptyGroups);
            return dataCollection;
        }
        
        static MocoResettableCollection^ GetResettableDataCollection(int count)
        {
            MocoResettableCollection^ dataCollection = ref new MocoResettableCollection();
            PopulateWithFlatData(dataCollection, count);
            return dataCollection;
        }

        static MocoResettableCollection^ GetGroupedResettableDataCollection(int count, int itemsCount, bool includeEmptyGroups)
        {
            auto dataCollection = ref new MocoResettableCollection();
            PopulateWithGroupedData(dataCollection, count, itemsCount, includeEmptyGroups);
            return dataCollection;
        }

    private:
        static void PopulateWithFlatData(::Windows::Foundation::Collections::IVector<Object^>^ dataCollection, int count)
        {
            for (unsigned int i = 0; i < static_cast<unsigned int>(count); i++)
            {
                dataCollection->Append(ref new MocoBasicFlatDataModel(L"Item" + i));
            }
        }
        
        static void PopulateWithGroupedData(::Windows::Foundation::Collections::IVector<Object^>^ dataCollection, int count, int itemsCount, bool includeEmptyGroups)
        {
            Platform::String^ groupHeaderNames = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

            for (int i = 0; i < count; i++)
            {
                std::wstring ws(groupHeaderNames->Data());
                auto substring = ws.substr(i % groupHeaderNames->Length(), 1);
                MocoBasicGroupedDataModel^ group = ref new MocoBasicGroupedDataModel(ref new Platform::String(substring.data()));
                if (!(includeEmptyGroups && i % 2 == 1)) // every alternate group is empty if includeEmptyGroups flag is set
                {
                    for(int j = 0; j < itemsCount; j++)
                    {
                        group->Append(ref new MocoBasicFlatDataModel(group->Header + " - Item " + j));
                    }
                }
                dataCollection->Append(group);
            }
        }
    };
}}}}}