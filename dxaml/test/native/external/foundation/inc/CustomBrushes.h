// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

ref class XcbPurpleBrush sealed : public Microsoft::UI::Xaml::Media::XamlCompositionBrushBase
{
public:
    XcbPurpleBrush()
    {
        m_compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
    }

    bool IsConnectedCalled()
    {
        return m_isConnectedCalled;
    }

    bool IsDisconnectedCalled()
    {
        return m_isDisconnectedCalled;
    }

    void ResetCallFlags()
    {
        m_isConnectedCalled = false;
        m_isDisconnectedCalled = false;
    }

protected:
    void OnConnected() override
    {
        if (m_brush == nullptr)
        {
            m_brush = m_compositor->CreateColorBrush(Microsoft::UI::Colors::Purple);
        }
        this->CompositionBrush = m_brush;
        m_isConnectedCalled = true;
    }

    void OnDisconnected() override
    {
        this->CompositionBrush = nullptr;
        delete m_brush;    // calls IClosable::Close... (C++/Cx is weird about this)
        m_brush = nullptr;
        m_isDisconnectedCalled = true;
    }

private:
    Microsoft::UI::Composition::Compositor^ m_compositor = nullptr;
    Microsoft::UI::Composition::CompositionBrush^ m_brush = nullptr;
    bool m_isConnectedCalled = false;
    bool m_isDisconnectedCalled = false;
};

ref class XcbNullableBrush sealed : public Microsoft::UI::Xaml::Media::XamlCompositionBrushBase
{
public:
    XcbNullableBrush()
    {
        m_compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
    }

    // Used to validate ability to null brush at any time (not just when brush leaves live tree)
    void NullBrush()
    {
        OnDisconnected();
    }

protected:
    void OnConnected() override
    {
        if (m_brush == nullptr)
        {
            m_brush = m_compositor->CreateColorBrush(Microsoft::UI::Colors::Purple);
        }
        this->CompositionBrush = m_brush;
    }

    void OnDisconnected() override
    {
        this->CompositionBrush = nullptr;
        delete m_brush;
        m_brush = nullptr;
    }

private:
    Microsoft::UI::Composition::Compositor^ m_compositor = nullptr;
    Microsoft::UI::Composition::CompositionBrush^ m_brush = nullptr;
};

ref class SeahawksColorsBrush sealed : public Microsoft::UI::Xaml::Media::XamlCompositionBrushBase
{
public:
    SeahawksColorsBrush()
    {
        m_compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
    }

    property bool UseAlternateColor
    {
        bool get() { return m_useAlternateColor; }
        void set(bool value)
        {
            if (value != m_useAlternateColor)
            {
                m_useAlternateColor = value;

                // Reset the brush
                OnDisconnected();
                OnConnected();
            }
       }
    }

protected:
    void OnConnected() override
    {
        if (m_brush == nullptr)
        {
            m_brush = m_compositor->CreateColorBrush(m_useAlternateColor ? Microsoft::UI::Colors::Green : Microsoft::UI::Colors::Blue);
        }
        this->CompositionBrush = m_brush;
    }

    void OnDisconnected() override
    {
        this->CompositionBrush = nullptr;
        delete m_brush;
        m_brush = nullptr;
    }

private:
    bool m_useAlternateColor = false;
    Microsoft::UI::Composition::Compositor^ m_compositor = nullptr;
    Microsoft::UI::Composition::CompositionBrush^ m_brush = nullptr;
};

ref class XcbImageBrush sealed : public Microsoft::UI::Xaml::Media::XamlCompositionBrushBase
{
public:
    XcbImageBrush()
    {
        m_compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
    }

protected:
    void OnConnected() override
    {
        if (m_brush == nullptr)
        {
            if (m_imageSurface == nullptr)
            {
                auto uri = ref new ::Windows::Foundation::Uri(L"ms-appx:///resources/native/external/foundation/graphics/image/barcelona.jpg");
                m_imageSurface = Microsoft::UI::Xaml::Media::LoadedImageSurface::StartLoadFromUri(uri);
            }
            m_brush = m_compositor->CreateSurfaceBrush(m_imageSurface);
        }
        this->CompositionBrush = m_brush;
    }

    void OnDisconnected() override
    {
        this->CompositionBrush = nullptr;
        delete m_brush;
        m_brush = nullptr;
    }

private:
    Microsoft::UI::Xaml::Media::LoadedImageSurface^ m_imageSurface = nullptr;
    Microsoft::UI::Composition::Compositor^ m_compositor = nullptr;
    Microsoft::UI::Composition::CompositionBrush^ m_brush = nullptr;
};

} } } } } }
