#pragma once
#include <iostream>

class Exception_error : public std::exception
{
public:
	const char* what() const override
	{
		return "������ �������\n";
	}
};

class Exception_no_file : public std::exception
{
public:
	const char* what() const override
	{
		return "������ �� ����� �� ���������\n";
	}
};

class Exception_no_section : public std::exception
{
public:
	const char* what() const override
	{
		return  "C����� �� ����������\n";
	}
};

class Exception_incorrect_data : public std::exception
{
public:
	const char* what() const override
	{
		return "������������ ������\n";
	}
};

class Exception_no_variable : public std::exception
{
public:
	const char* what() const override
	{
		return "��� ����������\n";
	}
};

class Exception_incorrect_request : public std::exception
{
public:
	const char* what() const override
	{
		return "����������� ������\n";
	}
};

class Exception_type_error : public std::exception
{
public:
	const char* what() const override
	{
		return "�������� ��� ���������� � �������\n";
	}
};