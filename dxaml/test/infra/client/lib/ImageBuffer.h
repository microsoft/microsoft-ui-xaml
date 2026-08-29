// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Only supports BGRA
class ImageBuffer final
{
public:

    void Clear();
    void SetSize(uint32_t width, uint32_t height);
    void Init();
    void EnsureInit();
    bool IsInitialized() { return !!m_pixels; }

    uint32_t* GetPixelBuffer();
    void SetPixel(int x, int y, uint8_t a, uint8_t r, uint8_t g, uint8_t b);
    void SetPixel(int x, int y, uint32_t pixelValue);

    void SaveToFile(const wchar_t* fileName);

private:

    std::unique_ptr<uint32_t[]> m_pixels;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};