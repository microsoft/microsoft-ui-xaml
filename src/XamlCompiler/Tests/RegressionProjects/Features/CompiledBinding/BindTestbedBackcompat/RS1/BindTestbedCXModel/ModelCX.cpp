// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ModelCX.h"

using namespace BindTestbedCXModel;

DependencyProperty^ StarCx::_CoordsProperty =
DependencyProperty::Register("Coords",
    Coordinates::typeid,
    StarCx::typeid,
    ref new PropertyMetadata(nullptr)
);

DependencyProperty^ StarCx::_TestStringProperty =
DependencyProperty::Register("TestString",
    String::typeid,
    StarCx::typeid,
    ref new PropertyMetadata(nullptr)
);

ModelCX::ModelCX()
{
	this->m_vectorOfStrings = ref new Vector<String^>();
}

void ModelCX::InitializeValues()
{
	if (this->m_vectorOfStrings->Size == 0)
	{
		this->m_vectorOfStrings->Append(L"String 1");
		this->m_vectorOfStrings->Append(L"String 2");
		this->m_vectorOfStrings->Append(L"String 3");
	}
	else
	{
		_ASSERT(this->m_vectorOfStrings->Size == 3);
		this->m_vectorOfStrings->SetAt(0, L"String 1");
		this->m_vectorOfStrings->SetAt(1, L"String 2");
		this->m_vectorOfStrings->SetAt(2, L"String 3");
	}
}

void ModelCX::UpdateValues()
{
	_ASSERT(this->m_vectorOfStrings->Size == 3);
	this->m_vectorOfStrings->SetAt(0, L"String 1.1");
	this->m_vectorOfStrings->SetAt(1, L"String 2.1");
	this->m_vectorOfStrings->SetAt(2, L"String 3.1");
}
