#include "pch.h"
#include "FolderNode.h"
#include "FolderNode.g.cpp"

using namespace winrt;
using namespace Windows::Foundation::Collections;

namespace
{
    bool HasSubdirectories(std::filesystem::path const& path) noexcept
    {
        std::error_code error;
        std::filesystem::directory_iterator current{ path, error };
        std::filesystem::directory_iterator const end;

        while (!error && current != end)
        {
            std::error_code typeError;
            if (current->is_directory(typeError) && !typeError)
            {
                return true;
            }

            current.increment(error);
        }

        return false;
    }

    bool IsDriveRoot(std::filesystem::path const& path) noexcept
    {
        auto const rootName = path.root_name().native();
        return path.has_root_directory() &&
            path.relative_path().empty() &&
            rootName.size() == 2 &&
            rootName[1] == L':';
    }

    hstring DisplayName(std::filesystem::path const& path, hstring const& fullPath)
    {
        auto name = path.filename();
        if (name.empty() && path != path.root_path())
        {
            name = path.parent_path().filename();
        }

        return name.empty() ? fullPath : hstring{ name.c_str() };
    }
}

namespace winrt::FolderTreeAppCpp::implementation
{
    FolderNode::FolderNode(hstring const& path) :
        m_fullPath(path),
        m_children(single_threaded_observable_vector<FolderTreeAppCpp::FolderNode>())
    {
        if (path.empty())
        {
            throw hresult_invalid_argument(L"A folder path is required.");
        }

        std::filesystem::path const filesystemPath{ path.c_str() };
        m_name = DisplayName(filesystemPath, path);
        m_hasUnrealizedChildren = HasSubdirectories(filesystemPath);
    }

    hstring FolderNode::Name() const
    {
        return m_name;
    }

    hstring FolderNode::FullPath() const
    {
        return m_fullPath;
    }

    IObservableVector<FolderTreeAppCpp::FolderNode> FolderNode::Children() const
    {
        return m_children;
    }

    bool FolderNode::HasUnrealizedChildren() const
    {
        return m_hasUnrealizedChildren;
    }

    hstring FolderNode::Glyph() const
    {
        return IsDriveRoot(std::filesystem::path{ m_fullPath.c_str() }) ? L"\uEDA2" : L"\uE8B7";
    }

    void FolderNode::LoadChildren()
    {
        if (!m_hasUnrealizedChildren)
        {
            return;
        }

        m_hasUnrealizedChildren = false;
        m_children.Clear();

        std::error_code error;
        std::filesystem::directory_iterator current{
            std::filesystem::path{ m_fullPath.c_str() },
            error
        };
        std::filesystem::directory_iterator const end;

        while (!error && current != end)
        {
            std::error_code typeError;
            if (current->is_directory(typeError) && !typeError)
            {
                m_children.Append(FolderTreeAppCpp::FolderNode{ current->path().c_str() });
            }

            current.increment(error);
        }
    }
}
