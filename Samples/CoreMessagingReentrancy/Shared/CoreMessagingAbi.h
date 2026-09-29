#pragma once

enum class MsgCreateFlags : int
{
    Default = 0,
};

enum class MsgPriority : unsigned int
{
    Dormant = 0,
    Low = 1,
    SynchronizeOutput = 2,
    Normal = 3,
};

using MsgActionCallback = HRESULT(CALLBACK*)(void* context);
using MsgGroupCallback = HRESULT(CALLBACK*)(void* context);
using MsgWaitCallback = HRESULT(CALLBACK*)(void* context);

struct __declspec(uuid("4b2edab9-129b-4733-be15-356853d55ace"))
IMessageSession : IUnknown
{
};

struct __declspec(uuid("710a0204-4683-4f67-9c69-432af5ba3683"))
IMessageSessionStable : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetMessageLoopExtensions(void** result) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateGroup(
        MsgGroupCallback dispatchHandler,
        void* dispatchContext,
        MsgGroupCallback cancelHandler,
        void* cancelContext,
        void** result) = 0;

    virtual HRESULT STDMETHODCALLTYPE DeferInvoke(
        MsgActionCallback callback,
        void* callbackContext,
        MsgPriority priority) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterWait(
        HANDLE handle,
        MsgWaitCallback callback,
        void* callbackContext) = 0;

    virtual HRESULT STDMETHODCALLTYPE UnregisterWait(HANDLE handle) = 0;
};
