// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DataVirtualizationItemsRangeInfoHelper.h"
#include "DataVirtualizationSelectionInfoHelper.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    // DataSource class
    ref class DataVirtualizationDataSource sealed
        : public ::Windows::Foundation::Collections::IVector<Platform::Object^>
        , public Microsoft::UI::Xaml::Data::IItemsRangeInfo
        , public Microsoft::UI::Xaml::Data::ISelectionInfo
    {
    private:
        Platform::Collections::Vector<Platform::Object^>^ m_Items;
        Platform::Collections::Vector<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ m_Ranges;

    public:
        DataVirtualizationDataSource(unsigned int numberOfItems)
        {
            int count = 0;
            m_Items = ref new Platform::Collections::Vector<Platform::Object^>(numberOfItems);
            for (auto i : m_Items)
            {
                i = count++;
            }

            m_Ranges = ref new Platform::Collections::Vector<Microsoft::UI::Xaml::Data::ItemIndexRange^>();
        }

        virtual ~DataVirtualizationDataSource()
        {

        }

        virtual void RangesChanged(Microsoft::UI::Xaml::Data::ItemIndexRange^ visibleRange, ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ trackedItems)
        {
            VERIFY_IS_GREATER_THAN_OR_EQUAL(visibleRange->FirstIndex, 0);

            ItemsRangeInfoStatics::RangesChangedInvoked = true;
        }

        virtual void SelectRange(Microsoft::UI::Xaml::Data::ItemIndexRange^ itemIndexRange)
        {
            VERIFY_IS_GREATER_THAN_OR_EQUAL(itemIndexRange->FirstIndex, 0);

            // for testing purposes, we're clearing all selected ranges and just selecting the newly passed range
            m_Ranges->Clear();

            if (itemIndexRange->FirstIndex != -1)
            {
                m_Ranges->Append(itemIndexRange);
            }

            SelectionInfoStatics::SelectRangeInvoked = true;
        }

        virtual void DeselectRange(Microsoft::UI::Xaml::Data::ItemIndexRange^ itemIndexRange)
        {
            VERIFY_IS_GREATER_THAN_OR_EQUAL(itemIndexRange->FirstIndex, 0);

            m_Ranges->Clear();

            SelectionInfoStatics::DeselectRangeInvoked = true;
        }

        virtual bool IsSelected(int index)
        {
            SelectionInfoStatics::IsSelectedInvoked = true;

            // checking to see if index is inside the selected ranges
            for (auto range : m_Ranges)
            {
                if (index >= range->FirstIndex && index <= range->LastIndex)
                {
                    return true;
                }
            }

            return false;
        }

        virtual ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ GetSelectedRanges()
        {
            VERIFY_IS_GREATER_THAN_OR_EQUAL(m_Ranges->Size, static_cast<unsigned int>(0));

            SelectionInfoStatics::GetSelectedRangesInvoked = true;

            return m_Ranges->GetView();
        }

        virtual ::Windows::Foundation::Collections::IIterator<Platform::Object^>^ First()
        {
            return m_Items->First();
        }

        property unsigned int Size
        {
            virtual unsigned int get()
            {
                return m_Items->Size;
            }
        }

        virtual Platform::Object^ GetAt(unsigned int index)
        {
            return m_Items->GetAt(index);
        }

        virtual ::Windows::Foundation::Collections::IVectorView<Platform::Object^>^ GetView()
        {
            return m_Items->GetView();
        }

        virtual bool IndexOf(Platform::Object^ value, unsigned int* index)
        {
            return m_Items->IndexOf(value, index);
        }

        virtual void SetAt(unsigned int index, Platform::Object^ value)
        {
            m_Items->SetAt(index, value);
        }

        virtual void InsertAt(unsigned int index, Platform::Object^ value)
        {
            m_Items->InsertAt(index, value);
        }

        virtual void RemoveAt(unsigned int index)
        {
            m_Items->RemoveAt(index);
        }

        virtual void Append(Platform::Object^ value)
        {
            m_Items->Append(value);
        }

        virtual void RemoveAtEnd()
        {
            m_Items->RemoveAtEnd();
        }

        virtual void Clear()
        {
            m_Items->Clear();
        }

        virtual unsigned int GetMany(unsigned int startIndex, Platform::WriteOnlyArray<Platform::Object^>^ items)
        {
            return m_Items->GetMany(startIndex, items);
        }

        virtual void ReplaceAll(const Platform::Array<Platform::Object^>^ items)
        {
            m_Items->ReplaceAll(items);
        }
    };

} } } } }
