#pragma once

#include "FolderNode.g.h"

namespace winrt::FolderTreeAppCpp::implementation
{
    struct FolderNode : FolderNodeT<FolderNode>
    {
        FolderNode(winrt::hstring const& path);

        winrt::hstring Name() const;
        winrt::hstring FullPath() const;
        Windows::Foundation::Collections::IObservableVector<FolderTreeAppCpp::FolderNode> Children() const;
        bool HasUnrealizedChildren() const;
        winrt::hstring Glyph() const;
        void LoadChildren();

    private:
        winrt::hstring m_name;
        winrt::hstring m_fullPath;
        Windows::Foundation::Collections::IObservableVector<FolderTreeAppCpp::FolderNode> m_children{ nullptr };
        bool m_hasUnrealizedChildren{ false };
    };
}

namespace winrt::FolderTreeAppCpp::factory_implementation
{
    struct FolderNode : FolderNodeT<FolderNode, implementation::FolderNode>
    {
    };
}
