// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct IViewObjectPresentNotifySite;

class WindowsPresentTarget
    final : public CXcpObjectBase<IObject>
{
private:
    WindowsPresentTarget(
        XUINT32 width,
        XUINT32 height,
        _In_opt_ XHANDLE hTargetWindow,
        _In_opt_ IViewObjectPresentNotifySite *pIPresentNotifySite
        );

    ~WindowsPresentTarget() override;

    static _Check_return_ HRESULT CreateWorker(
        XUINT32 width,
        XUINT32 height,
        _In_opt_ XHANDLE hTargetWindow,
        _In_opt_ IViewObjectPresentNotifySite *pIPresentNotifySite,
        _Outptr_ WindowsPresentTarget **ppPresentTarget
        );


public:
#if DBG
    FORWARD_ADDREF_RELEASE(CXcpObjectBase<IObject>);
#endif /* DBG */

    static _Check_return_ HRESULT CreateWindowedPresentTarget(
        XUINT32 width,
        XUINT32 height,
        _In_ XHANDLE hTargetWindow,
        _Outptr_ WindowsPresentTarget **ppPresentTarget
        );

    static _Check_return_ HRESULT CreateCompositedWindowlessPresentTarget(
        XUINT32 width,
        XUINT32 height,
        _In_ IViewObjectPresentNotifySite *pIPresentNotifySite,
        _Outptr_ WindowsPresentTarget **ppPresentTarget
        );

    XUINT32 GetWidth() const
    {
        return m_width;
    }

    XUINT32 GetHeight() const
    {
        return m_height;
    }

    _Check_return_ HRESULT SetWidth(XUINT32 width)
    {
        m_width = width;
        return S_OK;
    }

    _Check_return_ HRESULT SetHeight(XUINT32 height)
    {
        m_height = height;
        return S_OK;
    }

    _Ret_maybenull_ XHANDLE GetTargetWindowHandle() const
    {
        return m_hTargetWindow;
    }

    //--------------------------------------------------------------------------
    //
    //  WindowsPresentTarget interface
    //
    //--------------------------------------------------------------------------

    _Ret_maybenull_ IViewObjectPresentNotifySite *GetPresentNotifySite() const
    {
        return m_pIPresentNotifySite;
    }

private:
    XUINT32 m_width;
    XUINT32 m_height;

    _Maybenull_ HWND m_hTargetWindow;
    _Maybenull_ IViewObjectPresentNotifySite *m_pIPresentNotifySite;
};
