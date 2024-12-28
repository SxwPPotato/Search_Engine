#pragma once
#include <iostream>

class Exception_error : public std::exception
{
public:
	const char* what() const override
	{
		return "Ошибка парсера\n";
	}
};

class Exception_no_file : public std::exception
{
public:
	const char* what() const override
	{
		return "Данные из файла не считались\n";
	}
};

class Exception_no_section : public std::exception
{
public:
	const char* what() const override
	{
		return  "Cекции не существует\n";
	}
};

class Exception_incorrect_data : public std::exception
{
public:
	const char* what() const override
	{
		return "Некорректные данные\n";
	}
};

class Exception_no_variable : public std::exception
{
public:
	const char* what() const override
	{
		return "Нет переменной\n";
	}
};

class Exception_incorrect_request : public std::exception
{
public:
	const char* what() const override
	{
		return "Некоректный запрос\n";
	}
};

class Exception_type_error : public std::exception
{
public:
	const char* what() const override
	{
		return "Неверный тип переменной в запросе\n";
	}
};