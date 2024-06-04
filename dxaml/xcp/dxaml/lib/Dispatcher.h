// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    Defines IDispatcher, a simple dispatcher interface for use within
//    XAML framework code.

#pragma once

namespace DirectUI
{

class DXamlCore;
class ICallback;

//-----------------------------------------------------------------------------
//
// IDispatcher is XAML's internal dispatcher interface, used within the dxaml
// layer of our framework.
//
// IDispatcher is used instead of CoreDispatcher because CoreDispatcher isn't
// available in all the environments we support. For example, when being hosted
// by the shell, there is no CoreDispatcher available.
//
// To get an instance of IDispatcher, use:
//   DXamlCore::GetXamlDispatcher()
//   DependencyObject::GetXamlDispatcher()
//     
//-----------------------------------------------------------------------------
class IDispatcher : public IUnknown
{
public:
    //-----------------------------------------------------------------------------
    //
    // RunAsync schedules a work item for asynchronous invocation on the thread
    // associated with this dispatcher.
    //
    // To make an ICallback, use one of the MakeCallback() overloads. Search for
    // existing callers of RunAsync() to see usage examples.
    //
    // The fAllowReentrancy parameter controls whether we take a reentrancy guard
    // when invoking the callback. Normally we should not take the guard - this
    // matches the behavior we'd get with CoreDispatcher.
    //     
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT RunAsync(
        _In_ ctl::ComPtr<ICallback> spCallback, 
        bool fAllowReentrancy = true) = 0;

    //-----------------------------------------------------------------------------
    //
    // Returns TRUE if the calling thread is the same thread used by this dispatcher
    // to invoke work items.
    //     
    //-----------------------------------------------------------------------------
    virtual bool OnDispatcherThread() = 0;
};



//-----------------------------------------------------------------------------
//
// Implementation of DirectUI::IDispatcher. Don't use this directly outside
// of DXamlCore.cpp - program to the interface (IDispatcher) instead.
//     
//-----------------------------------------------------------------------------
class DispatcherImpl : public ctl::ComBase, public IDispatcher
{
public:
    _Check_return_ HRESULT RunAsync(_In_ ctl::ComPtr<ICallback> spCallback, bool fAllowReentrancy = true) override;
    bool OnDispatcherThread() override;

    DispatcherImpl() : m_pCore(NULL) { }

private:
    DXamlCore* m_pCore;
    DWORD      m_dwDispatcherThreadId{};

    _Check_return_ HRESULT Connect(_In_ DXamlCore* pCore);
    void Disconnect();

    friend class DXamlCore;
};

}
