// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "VectorIterator.h"
#include "VectorChangedEventArgs.h"
#include <algorithm>

// Nearly all Vector need to set DependencyObjectBase flag 
// to make DependencyObject as ComposableBase
// Otherwise we may hit memory leak when interact with .net.
// One exception is AcrylicBrush::CreateAcrylicBrushWorker
// because we know we don't have memory leak.
enum class VectorFlag {
    None = 0, // not Observable, not Bindable and not DependencyObjectBase
    Observable = 1, 
    DependencyObjectBase = 2, 
    Bindable = 4,
    NoTrackerRef = 8
};

template <VectorFlag ...all>
constexpr int MakeVectorParam();

template <VectorFlag first, VectorFlag ...others>
constexpr int  MakeVectorParamImpl()
{
    return static_cast<int>(first) | MakeVectorParam<others...>();
};

template <VectorFlag ...all>
constexpr int MakeVectorParam()
{
    return MakeVectorParamImpl<all...>();
};

template <>
constexpr int MakeVectorParam()
{
    return 0;
};

template <int flag> struct VectorFlagHelper
{
    static constexpr bool isObservable = !!(flag & static_cast<int>(VectorFlag::Observable));
    static constexpr bool isDependencyObjectBase = !!(flag & static_cast<int>(VectorFlag::DependencyObjectBase));
    static constexpr bool isBindable = !!(flag & static_cast<int>(VectorFlag::Bindable));
    static constexpr bool isNoTrackerRef = !!(flag & static_cast<int>(VectorFlag::NoTrackerRef));
};

// TStorageWrapperImpl is used to do the data conversion from T <-> T_Storage
// isNoTrackerRef = true :T <-> T
// isNoTrackerRef = false  : T <-> tracker_ref<T>

template <typename T, bool isNoTrackerRef = false> struct TStorageWrapperImpl;
template <typename T, bool isNoTrackerRef> struct TStorageWrapperImpl
{
    using Holder = typename tracker_ref<T>;

    static Holder wrap(const T& value, const ITrackerHandleManager* trackerHandleManager)
    {
        Holder holder{ trackerHandleManager };
        holder.set(value);
        return holder;
    }

    static T unwrap(const Holder& holder, bool useSafeGet = false)
    {
        return holder.safe_get(useSafeGet);
    }
};
template <typename T> struct TStorageWrapperImpl<T, true>
{
    using Holder = typename T;

    static Holder wrap(const T& value, ITrackerHandleManager* trackerHandleManager)
    {
        return value;
    }

    static T unwrap(const Holder& holder, bool useSafeGet = false)
    {
        return holder;
    }
};

// ObservableTraits defines Observable functions and Observable data types
// isObservable = false, dummy data type and dummy functions
template <class T, bool isObservable, bool isBindable>
struct ObservableTraits
{
    typedef struct Dummy { Dummy(ITrackerHandleManager*) {} } EventSource;
    using EventHandler = typename std::conditional<isBindable,
        winrt::BindableVectorChangedEventHandler,
        winrt::VectorChangedEventHandler<T>
    >::type;
    typedef typename std::conditional<isBindable,
        winrt::IBindableObservableVector,
        winrt::IObservableVector<winrt::IInspectable>>::type SenderType;
    using EventToken = typename winrt::event_token;

    static void RaiseEvent(...) { };
    static winrt::event_token AddEventHandler(...) { return {}; };
    static void RemoveEventHandler(...) { };
};

// IsObservable = true;
template <class T, bool isBindable>
struct ObservableTraits<T, true, isBindable>
{
    using SenderType = typename std::conditional<isBindable,
        winrt::IBindableObservableVector,
        winrt::IObservableVector<T>>::type;

    using EventSource = typename std::conditional<isBindable,
        event_source<winrt::BindableVectorChangedEventHandler>,
        event_source<winrt::VectorChangedEventHandler<T>>
    >::type;

    using EventHandler = typename std::conditional<isBindable,
        winrt::BindableVectorChangedEventHandler,
        winrt::VectorChangedEventHandler<T>
    >::type;
    
    using EventToken = typename winrt::event_token;

    static void RaiseEvent(EventSource* e, SenderType sender, winrt::CollectionChange collectionChange, uint32_t index)
    {
        auto args = winrt::make<VectorChangedEventArgs>(collectionChange, index);
        (*e)(sender, args);
    }
    static winrt::event_token AddEventHandler(EventSource* e, EventHandler const& handler)
    {
        return e->add(handler);
    };
    static void RemoveEventHandler(EventSource* e, winrt::event_token const& token)
    {
        e->remove(token);
    };
};

// This are callback functions and the owner of inner vector should implement this interface.
// The Inner Vector doesn't hold ITrackerHandleManager, also doesn't have enough information to send out the event
// All the information is deduced from the owner
template <typename EventSource, typename T>
struct IVectorOwner
{
    virtual ITrackerHandleManager* GetExternalTrackerHandleManager() = 0;
    virtual winrt::IInspectable GetVectorEventSender() = 0;
    virtual EventSource* GetVectorEventSource() = 0;
    virtual std::function<bool(T const& value, uint32_t& index)> GetCustomIndexOfFunction() { return nullptr; };
};

// Vector Inner Implementation without Observable function
template <typename VectorOptions, typename Wrapper>
struct VectorInnerImpl
{
private:
    using T_type = typename VectorOptions::T_type;

    typename T_type GetAt(uint32_t const index, bool useSafeGet)
    {
        if (index < static_cast<uint32_t>(m_vector.size()))
        {
            return unwrap(m_vector[index], useSafeGet);
        }
        else
        {
            throw winrt::hresult_out_of_bounds();
        }
    }

public:
    VectorInnerImpl(ITrackerHandleManager* trackerHandleManager)
    {
        m_trackerHandleManager = trackerHandleManager;
    }
    uint32_t Size()
    {
        return static_cast<uint32_t>(m_vector.size());
    }

    typename T_type GetAt(uint32_t const index)
    {
        return GetAt(index, false);
    }

    typename T_type SafeGetAt(uint32_t const index)
    {
        return GetAt(index, true);
    }

    void SetAt(uint32_t const index, typename T_type const& value)
    {
        if (index < static_cast<uint32_t>(m_vector.size()))
        {
            m_vector[index] = wrap(value);
            RaiseChildrenChanged(winrt::CollectionChange::ItemChanged, index);
        }
        else
        {
            throw winrt::hresult_out_of_bounds();
        }
    }

    void Append(typename T_type const& value)
    {
        m_vector.push_back(wrap(value));
        RaiseChildrenChanged(winrt::CollectionChange::ItemInserted, static_cast<uint32_t>(m_vector.size()) - 1);
    }

    bool IndexOf(typename T_type const& value, uint32_t& index)
    {
        index = 0;

        auto it = std::find(m_vector.begin(), m_vector.end(), wrap(value));

        if (it != m_vector.end())
        {
            index = (uint32_t)(it - m_vector.begin());
            return true;
        }
        return false;
    }

    uint32_t GetMany(uint32_t const startIndex, winrt::array_view<T_type> values)
    {
        if (startIndex >= m_vector.size())
        {
            return 0;
        }

        uint32_t actual = static_cast<uint32_t>(m_vector.size() - startIndex);

        if (actual > values.size())
        {
            actual = values.size();
        }

        for (uint32_t i = 0; i < actual; i++)
        {
            values[i] = GetAt(startIndex + i);
        }
        return actual;
    }

    void InsertAt(uint32_t const index, typename T_type const& value)
    {
        if (index <= static_cast<uint32_t>(m_vector.size()))
        {
            m_vector.insert(m_vector.begin() + index, wrap(value));
            RaiseChildrenChanged(winrt::CollectionChange::ItemInserted, index);
        }
        else
        {
            throw winrt::hresult_out_of_bounds();
        }
        
    }

    void RemoveAt(uint32_t const index)
    {
        if (index < static_cast<uint32_t>(m_vector.size()))
        {
            m_vector.erase(m_vector.begin() + index);
            RaiseChildrenChanged(winrt::CollectionChange::ItemRemoved, index);
        }
        else
        {
            throw winrt::hresult_out_of_bounds();
        }
    }

    void RemoveAtEnd()
    {
        if (!m_vector.empty())
        {
            m_vector.pop_back();
            RaiseChildrenChanged(winrt::CollectionChange::ItemRemoved, static_cast<uint32_t>(m_vector.size()));
        }
    }

    void Clear()
    {
        m_vector.clear();
        RaiseChildrenChanged(winrt::CollectionChange::Reset, 0u);
    }

    void ReplaceAll(winrt::array_view<T_type const> values)
    {
        Clear();
        for (auto value : values)
        {
            Append(value);
        }
    }

    virtual void RaiseChildrenChanged(winrt::CollectionChange collectionChange, unsigned int index) {};

    void reserve(unsigned int n) { m_vector.reserve(n); }
protected:
    using T_Storage = typename Wrapper::Holder;

    inline ITrackerHandleManager* GetTrackerHandlerManager() { return m_trackerHandleManager; }
    inline T_Storage wrap(typename T_type value)
    {
        return Wrapper::wrap(value, GetTrackerHandlerManager());
    }
    inline typename T_type unwrap(T_Storage const& hold, bool useSafeGet = false)
    {
        return Wrapper::unwrap(hold, useSafeGet);
    }

    std::vector<T_Storage> m_vector;
    ITrackerHandleManager* m_trackerHandleManager{ nullptr };
};

// Vector Inner implementation with Observable function
template <typename VectorOptions, typename Wrapper = TStorageWrapperImpl<typename VectorOptions::T_type, VectorOptions::NoTrackRef>>
struct ObservableVectorInnerImpl:
    VectorInnerImpl<VectorOptions, Wrapper>
{
private:
    using T_type = typename VectorOptions::T_type;
    using Traits = typename VectorOptions::Traits;
    using SenderType = typename VectorOptions::SenderType;
    using EventSource = typename VectorOptions::EventSource;
    using EventHandler = typename VectorOptions::EventHandler;
    using EventToken = typename VectorOptions::EventToken;

public:
    ObservableVectorInnerImpl(IVectorOwner<EventSource, T_type>* pIVectorExternal) :
        VectorInnerImpl<VectorOptions, Wrapper>(pIVectorExternal->GetExternalTrackerHandleManager()),
        m_pIVectorExternal(pIVectorExternal)
    {
    }

    void RaiseChildrenChanged(winrt::CollectionChange collectionChange, unsigned int index) override
    {
        if (auto sender = m_pIVectorExternal->GetVectorEventSender().try_as< SenderType>()) {
            Traits::RaiseEvent(m_pIVectorExternal->GetVectorEventSource(), sender, collectionChange, index);
        }
    }
    
    winrt::event_token AddEventHandler(EventHandler const& handler)
    {
        return Traits::AddEventHandler(m_pIVectorExternal->GetVectorEventSource(), handler);
    };

    void RemoveEventHandler(winrt::event_token const& token)
    {
       Traits::RemoveEventHandler(m_pIVectorExternal->GetVectorEventSource(), token);
    };

private:
    IVectorOwner<EventSource, T_type>* m_pIVectorExternal{ nullptr };
};


// VectorInterfaceHelper hold all the variable data types which depends on bindable flag
// isBindable = false
template <typename T, bool isBindable> struct VectorInterfaceHelper
{
    using VectorType = winrt::IVector<T>;
    using IteratorType = winrt::IIterator<T>;
    using IterableType = winrt::IIterable<T>;
    using VectorViewType = winrt::IVectorView<T>;
    using ObservableVectorType = winrt::IObservableVector<T>;
};

// isBindable = true
template <typename T> struct VectorInterfaceHelper<T, true>
{
    using VectorType = winrt::IBindableVector;
    using IteratorType = winrt::IBindableIterator;
    using VectorViewType = winrt::IBindableVectorView;
    using IterableType = winrt::IBindableIterable;
    using ObservableVectorType = winrt::IBindableObservableVector;
};

// Parameter used to support SetComposableBasePointersImplT for Vector
// Internally derive from DependencyObject to get ReferenceTracker behavior.
// But No Xaml application have problem to create DependencyObject.
// Create a dummy object for it.
template <bool isDependencyObjectBase>
struct ComposableBasePointersImplTType
{
    using WinRTBase = typename winrt::DependencyObject;
    using WinRTBaseFactoryInterface = typename winrt::IDependencyObjectFactory;
};

template <> 
struct ComposableBasePointersImplTType<false>
{
    using WinRTBase = typename winrt::IInspectable;
    using WinRTBaseFactoryInterface = typename winrt::IInspectable;
};

// VectorOptions hold all dynamic information which is used for Vector implementation and Observable implementation.
template <typename T, bool isObservable, bool isBindable, bool isDependencyObjectBase, bool isNoTrackerRef = false>
struct VectorOptionsBase: VectorInterfaceHelper<T, isBindable>, ComposableBasePointersImplTType<isDependencyObjectBase>
{
    static constexpr bool Bindable = isBindable;
    static constexpr bool Observable = isObservable;
    static constexpr bool DependencyObjectBase = isDependencyObjectBase;
    static constexpr bool NoTrackRef = isNoTrackerRef;

    //using type = typename VectorOptions<T, isObservable, isBindable, isDependencyObjectBase>;
    using T_type = typename T;
    using Traits = typename ObservableTraits<T, isObservable, isBindable>;
    using SenderType = typename Traits::SenderType;
    using EventSource = typename Traits::EventSource;
    using EventHandler = typename Traits::EventHandler;
    using EventToken = typename Traits::EventToken;
    using IVectorOwner = typename IVectorOwner<EventSource, T>;
};

template <typename T, bool isObservable, bool isBindable, bool isDependencyObjectBase, bool isNoTrackerRef>
struct VectorOptions: VectorOptionsBase<T, isObservable, isBindable, isDependencyObjectBase, isNoTrackerRef>
{ 
};

template <typename T, bool isObservable, bool isDependencyObjectBase, bool isNoTrackerRef>
struct VectorOptions<T, isObservable, true, isDependencyObjectBase, isNoTrackerRef>:
    VectorOptionsBase<winrt::IInspectable, isObservable, true, isDependencyObjectBase, isNoTrackerRef>
{
};

template <typename T, int flag, typename Helper = VectorFlagHelper<flag>>
struct VectorOptionsFromFlag :
    VectorOptions<T, Helper::isObservable, Helper::isBindable, Helper::isDependencyObjectBase, Helper::isNoTrackerRef>
{
};

// Implement IObservable or IBindableObservable Interface
#define Implement_IObservable(Options) \
    public: \
        winrt::event_token VectorChanged(typename Options##::EventHandler const& value) \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            return inner->AddEventHandler(value); \
        } \
        void VectorChanged(typename Options##::EventToken const& token) \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            inner->RemoveEventHandler(token); \
        } \
    private:

// Implement IVector or IBindableVector Interface which will not modify the data
#define Implement_IVector_Read_Functions(Options) \
    public: \
        typename Options##::VectorViewType GetView() \
        { \
            throw winrt::hresult_not_implemented(); \
        } \
        uint32_t Size() \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            return inner->Size(); \
        } \
        typename Options##::T_type GetAt(uint32_t index) \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            return inner->GetAt(index); \
        } \
        bool IndexOf(typename Options##::T_type const& value, uint32_t& index) \
        { \
            if (auto indexOfFunction = GetCustomIndexOfFunction()) \
            { \
                return indexOfFunction(value, index); \
            } \
            else \
            { \
                auto inner = this->GetVectorInnerImpl(); \
                return inner->IndexOf(value, index); \
            } \
        } \
        uint32_t GetMany(uint32_t const startIndex, winrt::array_view<typename Options##::T_type> values) \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            return inner->GetMany(startIndex, values); \
        } \
    private:

// Implement IVector or IBindableVector Interface which will modify the data
#define Implement_IVector_Modify_Functions(Options) \
    public: \
        void SetAt(uint32_t const index, typename Options##::T_type const& value) \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            inner->SetAt(index, value); \
        } \
        void Append(typename Options##::T_type const& value) \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            return inner->Append(value); \
        } \
        void InsertAt(uint32_t index, typename Options##::T_type const& value) \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            return inner->InsertAt(index, value); \
        } \
        void RemoveAt(uint32_t index) \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            return inner->RemoveAt(index); \
        } \
        void RemoveAtEnd() \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            return inner->RemoveAtEnd(); \
        } \
        void Clear() \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            return inner->Clear(); \
        } \
        void ReplaceAll(winrt::array_view<typename Options##::T_type const> value) \
        { \
            auto inner = this->GetVectorInnerImpl(); \
            return inner->ReplaceAll(value); \
        } \
        private:

// Implement IIterator or IBindableIterator Interface
#define Implement_IIterator(Options) \
    public: \
        typename Options##::IteratorType First() \
        { \
            return winrt::make<VectorIterator<typename Options##::T_type, Options##::Bindable, Options::DependencyObjectBase>>(*this); \
        } \
    private:

// Implement IVectorOwner, also define the Inner Vector and Event Source 
#define Implement_Vector_External(Options) \
    private: \
        using VectorInnerType = ObservableVectorInnerImpl<##Options##>; \
    protected: \
        typename VectorInnerType *GetVectorInnerImpl() { return &m_vectorInnerImpl; } \
        ITrackerHandleManager* GetExternalTrackerHandleManager() { return this; }; \
        winrt::IInspectable GetVectorEventSender() { return *this; }; \
        typename Options##::EventSource *GetVectorEventSource() { return &m_vectorEventSource; }; \
    private: \
        typename VectorInnerType m_vectorInnerImpl{ this }; \
        typename Options##::EventSource m_vectorEventSource{ this }; \
    private:

// Implement all interfaces for IXXVector/IXXIterator/IXXObservable
#define Implement_Vector(Options) \
    Implement_IVector_Read_Functions(##Options##) \
    Implement_IVector_Modify_Functions(##Options##) \
    Implement_IIterator(##Options##) \
    Implement_IObservable(##Options##) \
    Implement_Vector_External(##Options##) 

// Implement all interfaces for IXXVector/IXXIterator/IXXObservable except those which will modify the vector
// Like TreeViewNode, we need do additional work before Add/Remove/Modify the vector
#define Implement_Vector_Read(Options) \
    Implement_IVector_Read_Functions(##Options##) \
    Implement_IIterator(##Options##) \
    Implement_IObservable(##Options##) \
    Implement_Vector_External(##Options##) 

// Implement all interfaces for IXXVector/IXXIterator/IXXObservable except those which will modify the vector
// Like TreeViewNode, we need do additional work before Add/Remove/Modify the vector
#define Implement_Vector_Read_NoObservable(Options) \
    Implement_IVector_Read_Functions(##Options##) \
    Implement_IIterator(##Options##) \
    Implement_Vector_External(##Options##) 


template <typename T, bool isObservable, bool isBindable, bool isDependencyObjectBase, bool isNoTrackerRef, typename Options = VectorOptions<T, isObservable, isBindable, isDependencyObjectBase, isNoTrackerRef>>
class VectorBase :
    public ReferenceTracker<
    VectorBase<T, isObservable, isBindable, isDependencyObjectBase, isNoTrackerRef, Options>,
    reference_tracker_implements_t<typename Options::VectorType>::type,
    typename Options::IterableType,
    std::conditional_t<isObservable, typename Options::ObservableVectorType, void>>,
    public Options::IVectorOwner
{
    Implement_Vector(Options)

public:
    VectorBase()
    {
    }

    VectorBase(uint32_t capacity) : VectorBase()
    {
        GetVectorInnerImpl()->reserve(capacity);
    }

protected:
    void SetCustomIndexOfFunction(std::function<bool(T const& value, uint32_t& index)> indexOfFunction)
    {
        m_indexOfFunction = indexOfFunction;
    }
    virtual std::function<bool(T const& value, uint32_t& index)> GetCustomIndexOfFunction() { return m_indexOfFunction; };
private:
    std::function<bool(T const& value, uint32_t& index)> m_indexOfFunction{ };
};


template<typename T, 
    int flags = MakeVectorParam<VectorFlag::Observable, VectorFlag::DependencyObjectBase>(), 
    typename Helper = VectorFlagHelper<flags>>
class Vector :
    public VectorBase<T, Helper::isObservable, Helper::isBindable, Helper::isDependencyObjectBase, Helper::isNoTrackerRef>
{
public:
    Vector() {}
    Vector(uint32_t capacity) : VectorBase<T, Helper::isObservable, Helper::isBindable, Helper::isDependencyObjectBase, Helper::isNoTrackerRef>(capacity) {}

    // The same copy of data for NavigationView split into two parts in top navigationview. So two or more vectors are created to provide multiple datasource for controls.
    // InspectingDataSource is converting C# collections to Vector<winrt::IInspectable>. When GetAt(index) for things like string, a new IInspectable is always returned by C# projection. 
    // ListView use indexOf for selection, so a copied/filtered view of C# collection doesn't work for SelectedItem(s) anymore because IInspectable comparsion always return false.
    // As a workaround, the copied/filtered vector requires others help to IndexOf the orignial C# collection. 
    // So the comparison is done by C# vector other than Inspectable directly comparision. Here is an example:
    // Raw data A is: Home-Apps-Music-Sports 
    // data is splitted two vectors: B and C. B includes Homes, and C includes Apps-Music-Sports
    // Music is the selected item. SplitDataSource is the class help to manage the raw data and provides splitted vectors to ListViews
    // ListView call C.indexOf("Music")
    //                  C ask SplitDataSource.IndexOf
    //                      SplitDataSource calls A.IndexOf (C# provided it)
    //                      SpiltDataSource help to convert the indexInRawData to indexInC
    //                  return index in C    
    Vector(std::function<int(typename T const& value)> indexOfFunction) : m_indexOfFunction(indexOfFunction)
    {
        if (m_indexOfFunction)
        {
            this->SetCustomIndexOfFunction(
                [this](T const& value, uint32_t& index) {
                return CustomIndexOf(value, index);
            });
        }
    }

private:
    bool CustomIndexOf(T const& value, uint32_t& index)
    {
        if (m_indexOfFunction)
        {
            auto delegateIndex = m_indexOfFunction(value);
            if (delegateIndex != -1)
            {
                index = static_cast<uint32_t>(delegateIndex);
                return true;
            }
        }
        return false;
    }

private:
    std::function<int(T const& value)> m_indexOfFunction{};
};


// This type implements IObservableVector<T> *and* IObservableVector<IInspectable> so that the collection can be bound into an ItemsControl
// This implementation currently only supports a reference type for T (e.g. int and string are not yet supported for T).
template <typename T>
struct ObservableVector :
    public winrt::implements<ObservableVector<T>,
    Vector<T>,
    winrt::IObservableVector<winrt::IInspectable>,
    winrt::IVector<winrt::IInspectable>,
    winrt::IVectorView<winrt::IInspectable>,
    winrt::IIterable<winrt::IInspectable>>
{
    winrt::event_token VectorChanged(winrt::VectorChangedEventHandler<winrt::IInspectable> const& handler)
    {
        // Unfortunately we can't just forward the delegates to IObservableVector<T> because callers assume that the sender
        // is IObservableVector<IInspectable> so we have to re-raise. Luckily we can reuse the args.
        if (!m_innerChangedRevoker)
        {
            m_innerChangedRevoker = static_cast<winrt::IObservableVector<T>>(*this).VectorChanged(winrt::auto_revoke,
                [this](auto&& sender, auto &&args)
                {
                    m_changed(*this, args);
                });
        }
        return m_changed.add(handler);
    }

    void VectorChanged(winrt::event_token const cookie)
    {
        m_changed.remove(cookie);
    }

    winrt::IIterator<winrt::IInspectable> First()
    {
        return winrt::make<Iterator>(Vector<T>::First());
    }

    winrt::IVectorView<winrt::IInspectable> GetView()
    {
        throw winrt::hresult_not_implemented();
    }

    uint32_t Size()
    {
        auto inner = this->GetVectorInnerImpl();
        return inner->Size();
    }

    winrt::IInspectable GetAt(uint32_t index)
    {
        auto inner = this->GetVectorInnerImpl();
        return inner->GetAt(index);
    }

    bool IndexOf(winrt::IInspectable const& value, uint32_t& index)
    {
        auto inner = this->GetVectorInnerImpl();
        return inner->IndexOf(value.as<T>(), index);
    }

    uint32_t GetMany(uint32_t const startIndex, winrt::array_view<winrt::IInspectable> values)
    {
        throw winrt::hresult_not_implemented();
    }

    void SetAt(uint32_t const index, winrt::IInspectable const& value)
    {
        auto inner = this->GetVectorInnerImpl();
        inner->SetAt(index, value.as<T>());
    }
    void Append(winrt::IInspectable const& value)
    {
        auto inner = this->GetVectorInnerImpl();
        return inner->Append(value.as<T>());
    }
    void InsertAt(uint32_t index, winrt::IInspectable const& value)
    {
        auto inner = this->GetVectorInnerImpl();
        return inner->InsertAt(index, value.as<T>());
    }
    void RemoveAt(uint32_t index)
    {
        auto inner = this->GetVectorInnerImpl();
        return inner->RemoveAt(index);
    }
    void RemoveAtEnd()
    {
        auto inner = this->GetVectorInnerImpl();
        return inner->RemoveAtEnd();
    }
    void Clear()
    {
        auto inner = this->GetVectorInnerImpl();
        return inner->Clear();
    }
    void ReplaceAll(winrt::array_view<winrt::IInspectable const> value)
    {
        throw winrt::hresult_not_implemented();
    }

private:
    struct Iterator : winrt::implements<Iterator, winrt::IIterator<winrt::IInspectable>>
    {
        explicit Iterator(winrt::IIterator<T>&& inner) noexcept :
            m_inner(std::move(inner))
        {
        }

        winrt::IInspectable Current() const
        {
            return m_inner.Current();
        }

        bool HasCurrent() const noexcept
        {
            return m_inner.HasCurrent();
        }

        bool MoveNext() noexcept
        {
            return m_inner.MoveNext();
        }

        uint32_t GetMany(winrt::array_view<winrt::IInspectable> value)
        {
            throw winrt::hresult_not_implemented();
        }

    private:
        winrt::IIterator<T> m_inner;
    };

    winrt::event<winrt::VectorChangedEventHandler<winrt::IInspectable>> m_changed;
    typename winrt::IObservableVector<T>::VectorChanged_revoker m_innerChangedRevoker;
};
