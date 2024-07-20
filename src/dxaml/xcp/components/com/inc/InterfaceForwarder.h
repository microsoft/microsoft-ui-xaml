// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <inspectable.h>

// the core essence of interface forwarding:
// forwarding all the virtual interface method calls to methods on an owning implementation.
//
// Typical usage pattern:
// struct IFoo : public IUnknown { ... };
// Foo.h
//namespace ctl
//{
//  template<typename impl_type>
//  class interface_forwarder<IFoo, impl_type> : public ctl::IUnknownForwarderBase<IFoo>
//  {
//      impl_type* This() { return This_internal<impl_type>(); }
//      void DoFoo() final { This()->DoFoo(); }
//  };
//}
//
//class CFoo
//    : public BaseClass
//    , public ctl::forwarder_holder<IFoo, CFoo>
//{
//    void DoFoo();
//}
//

namespace ctl
{
    template<typename interface_type, typename impl_type>
    class interface_forwarder;

    // This class holds a single forwarder. Destination implementations derive from this class to
    // receive forwarded calls.
    template<typename Interface, typename Impl>
    struct forwarder_holder
    {
        using interface_type = Interface;
        using impl_type = Impl;
        using forwarder_type = interface_forwarder<interface_type, impl_type>;
        forwarder_type m_forwarder;
    };

    namespace details
    {
        struct interface_forwarded_tag;
        template<typename impl_type, typename interface_type>
        impl_type* impl_cast_helper(interface_type* pInterface, interface_forwarded_tag);
    }

    // This base class encapsulates the core essence of interface forwarding - forwarding
    // all the virtual interface method calls to methods on an owning implementation.
    // The real trick is eschewing a back pointer to save space. Instead, we get to the
    // owner by ensuring the forwarder can only exist as a member of an holder, so that we
    // can simply "thunk" by a fixed amount from 'this' to obtain the holding object.
    // Shout out to Don Box's "Essential COM", which provided inspiration for this trick
    template<typename interface_type>
    class interface_forwarder_base : public interface_type
    {
        // impl_cast_helper calls This_helper(), so give it access
        template<typename impl_type, typename interface_type>
        friend impl_type* details::impl_cast_helper(interface_type* pInterface, details::interface_forwarded_tag);

    protected:
        template<typename impl_type>
        impl_type* This_helper()
        {
            using holder_type = forwarder_holder<interface_type, impl_type>;
            static_assert(std::is_same<impl_type, typename holder_type::impl_type>::value, "impl_type must be the same type as holder_type::impl_type");
            static_assert(std::is_base_of<holder_type, impl_type>::value, "impl_type must derive from holder_type");

            // Obtain a pointer to the forwarder's holder, which is a base class of the implementation, so we
            // can then safely static_cast it down to impl_type
#if !defined(EXP_CLANG)
            static constexpr std::size_t member_offset = offsetof(holder_type, m_forwarder);
#else
            static const std::size_t member_offset = offsetof(holder_type, m_forwarder);
#endif // EXP_CLANG

            // Just to be extra paranoid. Cast this into the exact type of forwarder used by the holder.
            auto pForwarder = static_cast<typename holder_type::forwarder_type*>(this);

            // Jump out to the containing forwarder holder
            auto pHolder = reinterpret_cast<holder_type*>(
                reinterpret_cast<uint8_t*>(pForwarder) - member_offset);
            return static_cast<impl_type*>(pHolder);
        }

        interface_forwarder_base() = default;

        // Disable copying and heap allocation
        interface_forwarder_base(const interface_forwarder_base&) = delete;
        void* operator new(std::size_t) = delete;
        void* operator new[](std::size_t) = delete;
    };

    // It's quite common to have interfaces that derive from IUnknown and/or IInspectable
    // So, define common base forwarding implementations for those methods to cut down and needless boilerplate
    template<typename interface_type, typename impl_type>
    class iunknown_forwarder_base : public interface_forwarder_base<interface_type>
    {
        static_assert(std::is_base_of<IUnknown, interface_type>::value, "interface_type must derive from IUnknown");

    protected:
        iunknown_forwarder_base() = default;

        IFACEMETHODIMP QueryInterface(
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) final
        {
            return iunknown_cast(this->template This_helper<impl_type>())->QueryInterface(riid, ppvObject);
        }

        IFACEMETHODIMP_(ULONG) AddRef(void) final
        {
            return iunknown_cast(this->template This_helper<impl_type>())->AddRef();
        }

        IFACEMETHODIMP_(ULONG) Release(void) final
        {
            return iunknown_cast(this->template This_helper<impl_type>())->Release();
        }
    };

    template<typename interface_type, typename impl_type>
    class iinspectable_forwarder_base : public iunknown_forwarder_base<interface_type, impl_type>
    {
        static_assert(std::is_base_of<IInspectable, interface_type>::value, "interface_type must derive from IUnknown.");

    protected:
        iinspectable_forwarder_base() = default;

        IFACEMETHODIMP GetIids(
            /* [out] */ __RPC__out ULONG *iidCount,
            /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids) final
        {
            return iinspectable_cast(this->template This_helper<impl_type>())->GetIids(iidCount, iids);
        }

        IFACEMETHODIMP GetRuntimeClassName(
            /* [out] */ __RPC__deref_out_opt HSTRING *className) final
        {
            return iinspectable_cast(this->template This_helper<impl_type>())->GetRuntimeClassName(className);
        }

        IFACEMETHODIMP GetTrustLevel(
            /* [out] */ __RPC__out TrustLevel *trustLevel) final
        {
            return iinspectable_cast(this->template This_helper<impl_type>())->GetTrustLevel(trustLevel);
        }
    };

    // impl_cast and interface_cast are helpers that allow hopping back and forth between forwarders
    // and implementers. Originally, when we directly inherited from public COM interfaces
    // we could easily convert back and forth between the interface and our internal concrete class.
    // When we sever that type relationship, we need to perform a more type-aware conversion.
    // impl_cast and interface_cast utilize the forwarder's type machinery to generalize these conversions
    namespace details
    {
        struct interface_forwarded_tag {};
        struct interface_inherited_tag {};
        template<typename impl_type, typename interface_type>
        struct interface_relationship
        {
            // If impl_type doesn't inherit from interface_type, assume interface forwarding
            using type = typename std::conditional<
                std::is_base_of<interface_type, impl_type>::value, interface_inherited_tag, interface_forwarded_tag
            >::type;
        };

        template<typename interface_type, typename impl_type>
        interface_type* interface_cast_helper(impl_type* pImpl, interface_inherited_tag)
        {
            // impl_type derives from interface_type, so a simple, implicit, derived-to-base conversion will suffice
            static_assert(std::is_base_of<interface_type, impl_type>::value, "impl_type must derive from interface_type to use interface_inherited_tag");
            return pImpl;
        }
        template<typename interface_type, typename impl_type>
        interface_type* interface_cast_helper(ctl::forwarder_holder<interface_type, impl_type>* pHolder, interface_forwarded_tag)
        {
            // Obtain a pointer to a given interface, given a pointer to the owning implementation class
            // Any destination implementation of an interface forwarder should inherit from the
            // corresponding forwarder_holder. Utilize implicit derived-to-base conversion from the impl
            // to that holder to automagically obtain the correct forwarder.
            interface_type* pInterface = nullptr;
            if (pHolder) {
                using holder_type = forwarder_holder<interface_type, impl_type>;
                using forwarder_type = typename holder_type::forwarder_type;

                // Guarantee that converting to the holder type is a safe derived-to-base implicit conversion
                static_assert(std::is_base_of<holder_type, impl_type>::value, "impl_type must derive from holder_type");
                static_assert(std::is_base_of<interface_type, forwarder_type>::value, "forwarder_type must derive from interface_type");

                pInterface = &(pHolder->m_forwarder);
            }
            return pInterface;
        }

        template<typename impl_type, typename interface_type>
        impl_type* impl_cast_helper(interface_type* pInterface, interface_inherited_tag)
        {
            // impl_type derives from interface_type, so a simple, static downcast will suffice
            static_assert(std::is_base_of<interface_type, impl_type>::value, "impl_type must derive from interface_type to use interface_inherited_tag");
            return static_cast<impl_type*>(pInterface);
        }
        template<typename impl_type, typename interface_type>
        impl_type* impl_cast_helper(interface_type* pInterface, interface_forwarded_tag)
        {
            // For converting forwarded interface-to-impl, downcast the interface into a forwarder, and then call its This helper
            impl_type* pImpl = nullptr;
            if (pInterface) {
                using holder_type = forwarder_holder<interface_type, impl_type>;
                static_assert(std::is_base_of<holder_type, impl_type>::value, "impl_type must derive from holder_type. Ensure you are casting to the matching implementation for this interface.");

                using forwarder_type = typename forwarder_holder<interface_type, impl_type>::forwarder_type;
                static_assert(std::is_base_of<interface_type, forwarder_type>::value, "forwarder_type must derive from interface_type");

                pImpl = static_cast<forwarder_type*>(pInterface)->template This_helper<impl_type>();
            }
            return pImpl;
        }
    }

    template<typename impl_type, typename interface_type>
    impl_type* impl_cast(interface_type* pInterface)
    {
        using tag = typename details::interface_relationship<impl_type, interface_type>::type;
        return details::impl_cast_helper<impl_type>(pInterface, tag());
    }

    template<typename interface_type, typename impl_type>
    interface_type* interface_cast(ctl::forwarder_holder<interface_type, impl_type>* pHolder)
    {
        using tag = typename details::interface_relationship<impl_type, interface_type>::type;
        return details::interface_cast_helper<interface_type>(pHolder, tag());
    }

    // Base template, This should get a partial specialization to really be worth anything.
    // See the example at the top of the file
    template<typename interface_type, typename impl_type>
    class interface_forwarder : public interface_forwarder_base<interface_type>
    {};

}
