#include"parser_ini.h"
#include <fstream>

parser_ini::parser_ini()
{
	variables_str_array = new std::vector<std::map<std::string, std::string>>; 
	sections_map = new std::map<std::string, int>; 

	if ((variables_str_array == nullptr) || (sections_map == nullptr))
	{
		parser_invalid = true; 
		return;
	}
}

parser_ini::~parser_ini()
{
	delete variables_str_array;
	delete sections_map;
}

parser_ini::parser_ini(const parser_ini& other) // конструктор копирования
{
	parser_invalid = other.parser_invalid; 
	file_read = other.file_read; 
	invalid_data = other.invalid_data;
	incorrect_str_num = other.incorrect_str_num;  
	incorrect_str = other.incorrect_str; 

	current_section_name = other.current_section_name; 
	current_section_number = other.current_section_number; 

	sections_map = new std::map<std::string, int>;
	*sections_map = *(other.sections_map); 

	variables_str_array = new std::vector<std::map<std::string, std::string>>; 

	for (int i = 0; i < other.variables_str_array->size(); ++i)
	{
		variables_str_array->push_back((*other.variables_str_array)[i]); 
	}
}

parser_ini& parser_ini::operator=(const parser_ini& other) // оператор копирующего присваивания
{
	if (this != &other)
	{
		return *this = parser_ini(other);
	}

	return *this;
}

parser_ini::parser_ini(parser_ini&& other) noexcept	// конструктор перемещения
{
	parser_invalid = other.parser_invalid; 
	file_read = other.file_read;  
	invalid_data = other.invalid_data;
	incorrect_str_num = other.incorrect_str_num;  
	incorrect_str = other.incorrect_str;  

	current_section_name = other.current_section_name; 
	current_section_number = other.current_section_number; 

	sections_map = other.sections_map;
	other.sections_map = nullptr;

	variables_str_array = other.variables_str_array;
	other.variables_str_array = nullptr;
}

parser_ini& parser_ini::operator=(parser_ini&& other) noexcept   // оператор перемещающего присваивания
{
	return *this = parser_ini(other);
}

void parser_ini::fill_parser(const std::string& file_name)
{
	std::ifstream file_ini(file_name);
	if (!file_ini.is_open())
	{
		std::cout << "Error";
		parser_invalid = true;
		return;
	}

	std::string temp_str;
	int count_line = 0; 
	int count_sec = 0;  

	while (getline(file_ini, temp_str)) 
	{
		++count_line;
		std::string temp1 = delete_spaces(temp_str);

		string_type string_type_ = string_type::invalid_;  
		std::string name = ""; 
		std::string value = ""; 

		std::tie(string_type_, name, value) = research_string(temp1);
		std::map<std::string, int>::iterator it_name; 

		switch (string_type_) 
		{
		case string_type::section_:

			current_section_name = name;		
			it_name = sections_map->find(name);

			if (it_name == sections_map->end())
			{
				(*sections_map)[name] = count_sec; 
				current_section_number = count_sec;
				++count_sec;

				std::map<std::string, std::string> new_temp_map{}; 
				variables_str_array->push_back(new_temp_map); 
			}
			else 
			{
				current_section_number = (*sections_map)[name]; 
			}
			break;

		case string_type::variable_:

			if (current_section_number < 0)
			{
				incorrect_str_num = count_line;  
				incorrect_str = temp_str; 
			}
			else
			{
				(*variables_str_array)[current_section_number][name] = value;
			}

			break;

		case string_type::empty_:	  	
			break;

		case string_type::invalid_:  
			incorrect_str_num = count_line;  
			incorrect_str = temp_str; 

			break;
		}

		if (incorrect_str_num != 0) 
		{
			invalid_data = true;
			break;
		}

	}

	file_ini.close();
	file_read = true; 

	if (variables_str_array->size() != sections_map->size())
	{
		parser_invalid = true;
	}

}

void parser_ini::check_parser()
{
	if (!file_read)
	{
		throw Exception_no_file();
	}

	if ((parser_invalid) || (sections_map == nullptr) || (variables_str_array == nullptr))
	{
		throw Exception_error();
	}

	if (invalid_data)
	{
		throw Exception_incorrect_data();
	}

}

std::string parser_ini::delete_spaces(const std::string& str)
{
	if (str == "") 	return str;

	std::string temp_str = "";
	int i = 0;
	bool is_beginning = true;

	while ((str[i] != '\n') &&
		(i < str.size()))
	{
		if (((str[i] == ' ') || (str[i] == '\t')) && is_beginning) 
		{
			++i; 
		}
		else
		{
			temp_str += str[i];
			is_beginning = false;
			++i;
		}
	}

	return temp_str;
}

std::tuple<string_type, std::string, std::string> parser_ini::research_string(const std::string& src_str)
{
	string_type temp_string_type = string_type::invalid_; 
	std::string temp_name = "";
	std::string temp_value = ""; 

	if (src_str == "") 
	{
		return std::make_tuple(string_type::empty_, temp_name, temp_value);
	}

	if (src_str[0] == ';') 
	{
		return std::make_tuple(string_type::empty_, temp_name, temp_value);
	}

	if (src_str[0] == '[') 
	{
		int end_pos = src_str.find(']'); 

		if ((end_pos == std::string::npos) || (end_pos == 1)) 
		{
			return std::make_tuple(string_type::invalid_, temp_name, temp_value); 
		}
		else
		{
			for (int i = 1; i < end_pos; ++i)
			{
				if ((src_str[i] == ' ') || (src_str[i] == '\t') || (src_str[i] == '.')) 
				{
					return std::make_tuple(string_type::invalid_, temp_name, temp_value); 
				}
				else
				{
					temp_name += src_str[i];
				}
			}

			for (int i = end_pos + 1; i < src_str.size() - 1; ++i)
			{
				if ((src_str[i] != ' ') && (src_str[i] != '\t') && (src_str[i] != ';'))
				{
					return std::make_tuple(string_type::invalid_, temp_name, temp_value); 
				}

				if (src_str[i] == ';') 
				{
					return std::make_tuple(string_type::section_, temp_name, temp_value); 
				}
			}

			return std::make_tuple(string_type::section_, temp_name, temp_value); 
		}
	}

	int end_pos = src_str.find('='); 

	if (end_pos == std::string::npos) 
	{
		return std::make_tuple(string_type::invalid_, temp_name, temp_value);
	}

	for (int i = 0; (i < end_pos) && (src_str[i] != ' ') && (src_str[i] != '\t') && (src_str[i] != '.'); ++i)
	{
		temp_name += src_str[i];
	}

	if (temp_name == "")
	{
		return std::make_tuple(string_type::invalid_, temp_name, temp_value);
	}

	for (int i = temp_name.size(); i < end_pos; ++i)
	{
		if ((src_str[i] != ' ') && (src_str[i] != '\t'))
		{
			return std::make_tuple(string_type::invalid_, temp_name, temp_value);
		}
	}

	for (int i = end_pos + 1; i < src_str.size(); ++i)
	{
		if (src_str[i] == ';')
		{
			return std::make_tuple(string_type::variable_, temp_name, temp_value); 
		}

		temp_value += src_str[i];
	}

	return std::make_tuple(string_type::variable_, temp_name, temp_value); 
}

std::string parser_ini::get_section_name(const int section_index)
{
	for (auto& item : *sections_map) 
	{
		if (item.second == section_index) 
		{
			return item.first; 
		}
	}

	return "";
}

std::string parser_ini::get_variable_value(const int section_index, const std::string& variable_name)
{
	if (section_index < 0)
	{
		throw Exception_no_section(); 
	}

	if (section_index >= variables_str_array->size())
	{
		throw Exception_error();
	}

	if ((*variables_str_array)[section_index].find(variable_name) == (*variables_str_array)[section_index].end())
	{
		throw Exception_no_variable();
		return "";
	}

	return ((*variables_str_array)[section_index])[variable_name];
}


int parser_ini::get_section_number(const std::string& section_name)
{
	if ((*sections_map).find(section_name) == (*sections_map).end())
	{
		return -1;
	}

	int temp = (*sections_map)[section_name];
	return temp;
}


std::tuple<std::string, std::string> parser_ini::get_section_variable_names(const std::string& src_str)
{
	std::string section_name = "";
	std::string variable_name = "";

	std::string temp_str = delete_spaces(src_str);

	int point_pos = temp_str.find('.');
	if ((point_pos == std::string::npos) ||  (temp_str.length() < 3))
	{
		throw Exception_incorrect_request();
	}

	char symb; 
	int i = 0;

	do
	{
		symb = temp_str[i];
		i++;
		if (symb == ' ') 
		{
			throw Exception_incorrect_request();
		}

		if (symb == '.')
		{
			break;
		}
		else
		{
			section_name += symb;
		}

	} while (i < temp_str.length());

	if (temp_str.length() < i) 
	{
		throw Exception_incorrect_request();
	}

	do
	{
		symb = temp_str[i];
		if ((symb == '.') || (symb == ' ')) 
		{
			throw Exception_incorrect_request();
		}

		variable_name += symb;
		++i;

	} while (i < temp_str.length());


	if ((section_name == "") || (variable_name == ""))
	{
		throw Exception_incorrect_request();
	}

	return std::make_tuple(section_name, variable_name); 
}


bool parser_ini::print_all_sections()
{
	bool print_result = true;

	try
	{
		check_parser();  
	}
	catch (const Exception_incorrect_data& ex)
	{
		print_result = false;
	}

	if (sections_map->size() > 0)
	{
		std::cout << "В файле имеются следующие секции:\n";
	}

	for (auto& item : *sections_map) 
	{
		std::cout << item.first << "\n";
	}

	return print_result;
}

bool parser_ini::print_all_sections_info() 
{
	std::cout << "\nПарсер содержит следующие данные: \n";

	bool print_result = true;

	try
	{
		check_parser();  
	}
	catch (const Exception_incorrect_data& ex)
	{
		print_result = false;	
	}

	for (int i = 0; i < sections_map->size(); ++i)
	{
		std::cout << i << ": " << get_section_name(i) << "\n"; 
	}

	std::cout << "Всего секций = " << variables_str_array->size() << "\n\n";

	for (int i = 0; i < variables_str_array->size(); ++i)
	{
		std::cout << "Размер секции " << i << " (" << get_section_name(i) << ") " << " = " << (*variables_str_array)[i].size() << "\n";

		std::cout << "Список переменных:\n";
		for (auto& item : (*variables_str_array)[i])
		{
			std::cout << item.first << " = " << item.second << "\n";
		}
		std::cout << "\n";
	}

	std::cout << "***************************" << "\n";
	return print_result;
}

bool parser_ini::print_all_variables(const std::string& section_name) 
{
	bool print_result = true;

	try
	{
		check_parser();  
	}
	catch (const Exception_incorrect_data& ex)
	{
		print_result = false;
	}


	int section_index = get_section_number(section_name); 
	if (section_index < 0)
	{
		throw(Exception_no_section());
	}
	else
	{
		if ((*variables_str_array)[section_index].size() > 0)
		{
			std::cout << "В секции " << section_name << " имеются следующие переменные:\n";
			for (auto& item : (*variables_str_array)[section_index]) 
			{
				std::cout << item.first << " = " << item.second << "\n";
			}
		}
	}

	return print_result;
}

void parser_ini::print_incorrect_info()
{
	std::cout << "В файле содержится некорректная информация.\n";
	std::cout << "Номер строки с ошибкой: " << incorrect_str_num << "\n";
	std::cout << "Строка: \"" << incorrect_str << "\"\n";
}

std::string parser_ini::get_section_from_request(const std::string& request_str)
{
	std::string section_name = "";
	std::string variable_name = "";

	std::tie(section_name, variable_name) = get_section_variable_names(request_str); 

	return section_name;
}
