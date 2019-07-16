#include "pch.h"
#include <UserSwitcherViewModel.h>

using namespace FlickCpp;
using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Interop;

UserSwitcherViewModel::UserSwitcherViewModel()
{
}

UserSwitcherViewModel::~UserSwitcherViewModel()
{
}

IVector<String^>^ UserSwitcherViewModel::Users::get()
{
    if (m_users == nullptr)
    {
        m_users = ref new Vector<String^>();
        m_users->Append("Item 0");
        /*m_users->Append("Item 1");
        m_users->Append("Item 2");
        m_users->Append("Item 3");
        m_users->Append("Item 4");
        m_users->Append("Item 5");*/
    }

    return m_users;
}

void UserSwitcherViewModel::Users::set(IVector<String^>^ value)
{
    m_users = value;
    PropertyChanged(this, ref new PropertyChangedEventArgs(L"Data"));
}

String^ UserSwitcherViewModel::SelectedUser::get()
{
    return m_selectedUser;
}

void UserSwitcherViewModel::SelectedUser::set(String^ value)
{
    m_selectedUser = value;
    PropertyChanged(this, ref new PropertyChangedEventArgs(L"SelectedUser"));
}
