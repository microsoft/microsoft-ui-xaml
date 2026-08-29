// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace ctl
{
    typedef IActivationFactory* (*PFNCreator)();

    struct activation_factory_entry
    {
        xstring_ptr_storage m_strClassIDStorage;
        PFNCreator m_pfnCreator;
    };

    extern const activation_factory_entry __creationMap[];
    extern const XUINT32 __creationMapCount;
    extern IActivationFactory *__factories[];

    class module
    {
    public:
        IActivationFactory* GetActivationFactory(HSTRING hClassID);
        void ClearFactoryCache();
        void ResetFactories();
    };

    #define COMMA ,
    #define ACTIVATION_FACTORY_MAP_BEGIN()    extern const ctl::activation_factory_entry ctl::__creationMap[] = {
    #define ACTIVATION_FACTORY_MAP_END()      }; \
                                              extern const XUINT32 ctl::__creationMapCount = _countof(ctl::__creationMap); \
                                              extern IActivationFactory *ctl::__factories[_countof(ctl::__creationMap)] = { };
    #define ACTIVATION_FACTORY_ENTRY(s, f)    {s, f},

    extern module __module;
}
