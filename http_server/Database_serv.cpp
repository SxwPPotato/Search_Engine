#include <algorithm>
#include <tuple>
#include <map>

#include "Database_serv.h"


Database::Database(const std::string params_str) noexcept : connection_str{ params_str } {}

// конструктор перемещения
Database::Database(Database&& other) noexcept : connection_str{ other.connection_str }
{
	connection_ptr = other.connection_ptr; 
	last_er = other.last_er;  

	other.connection_ptr = nullptr;
}

Database& Database::operator=(Database&& other) noexcept   // оператор перемещающего присваивания
{
	connection_ptr = other.connection_ptr; 
	last_er = other.last_er;  

	other.connection_ptr = nullptr;
	return *this;
}

Database::~Database()
{
	delete connection_ptr;
}

bool Database::connect_db() 
{
	try 
	{
		connection_ptr = new pqxx::connection(connection_str);
		return true;
	}
	catch (const std::exception& ex)
	{
		last_er = ex.what();
		return false;
	}
}

std::string Database::get_last_error_desc()
{
	return last_er;
}

void Database::print_last_error() 
{
	std::cout << "Ошибка: " << last_er << "\n";
}

bool Database::create_tables()
{
	if ((connection_ptr == nullptr) || (!(connection_ptr->is_open())))
	{
		last_er = "Ошибка создания таблицы. Ошибка подключения к базе данных";
		return false;
	}

	try
	{
		pqxx::work tx{ *connection_ptr };

		//таблица Документы
		tx.exec(
			"CREATE table IF NOT EXISTS documents ( "
			"id serial primary KEY, "
			"url varchar(250) NOT NULL  UNIQUE, "
			"constraint url_unique unique(url)); ");

		//таблица Слова
		tx.exec(
			"CREATE table IF NOT EXISTS words ( "
			"id serial primary KEY, "
			"word varchar(32) NOT NULL  UNIQUE, "
			"constraint word_unique unique(word)); ");

		//Промежуточная таблица
		tx.exec(
			"CREATE table IF NOT EXISTS urls_words ( "
			"word_id integer references words(id), "
			"url_id integer references documents(id), "
			"quantity integer NOT NULL,"
			"constraint pk primary key(word_id, url_id)); ");

		tx.commit();
		return true;
	}
	catch (...)
	{
		last_er = "Ошибка создания таблиц";
		return false;
	}
}

bool Database::start_db()
{
	bool result = false;

	if (connect_db()) 
	{
		result = create_tables();  			 
	}

	return result;
}

std::map<std::string, int>  Database::get_urls_list_by_words(const std::set<std::string>& words_set)
{
	std::map<std::string, int> result_map;
	if (words_set.empty()) { return result_map; }

	if (connection_ptr == nullptr)
	{
		last_er = "Ошибка подключения к базе данных";
		return result_map;
	}

	last_er = "";

	try
	{
		pqxx::work tx{ *connection_ptr };

		std::string where_str = prepare_words_where_or(words_set, tx); 
		std::string request_str = "select distinct url from "
			"(select * from urls_words inner join documents on  urls_words.url_id = documents.id) as Table_A "
			"inner join words on Table_A.word_id = words.id where " + where_str;

		auto query_res = tx.exec(request_str);

		for (auto row : query_res)
		{
			result_map[row["url"].as<std::string>()] = -1;
		}

	}
	catch (const std::exception& ex)
	{
		last_er = ex.what();
		return result_map;
	}

	return result_map;
}

int Database::count_url_words(const std::set<std::string>& words_set, std::string url)
{
	if (words_set.empty()) { return 0; }

	if (connection_ptr == nullptr)
	{
		last_er = "Ошибка подключения к базе данных";
		return 0;
	}

	last_er = "";

	try
	{
		pqxx::work tx{ *connection_ptr };

		std::string where_str = prepare_words_where_or(words_set, tx); 

		std::string request_str = "select count(*) from (select* from urls_words "
			"inner join documents on  urls_words.url_id = documents.id) as Table_A "
			"inner join words on Table_A.word_id = words.id "
			"where (" + where_str + ") and url = '" + url + "'";

		auto query_res = tx.exec(request_str);

		auto row = query_res.begin();
		int res_int = row["count"].as<int>();
		return res_int; 
	}
	catch (const std::exception& ex)
	{
		last_er = ex.what();
		return 0;
	}
}

std::string Database::prepare_words_where_or(const std::set<std::string>& words_set, pqxx::work& tx) 
{
	std::string where_str; 
	for (const std::string& word : words_set)
	{
		where_str += "word = '" + tx.esc(word) + "' or ";
	}
	where_str.erase((where_str.size() - 4), 4);

	return where_str;
}

std::multimap<std::string, int> Database::get_words_urls_table(const std::set<std::string>& words_set) 
{
	std::multimap<std::string, int> result_map;

	if (words_set.empty()) { return result_map; }

	if (connection_ptr == nullptr)
	{
		last_er = "Ошибка подключения к базе данных";
		return result_map;
	}

	last_er = "";

	try
	{
		pqxx::work tx{ *connection_ptr };

		std::string where_str = prepare_words_where_or(words_set, tx); 
		std::string request_str = "select quantity, url from (select * from urls_words "
			"inner  join documents on  urls_words.url_id = documents.id) as Table_A "
			"inner join words on Table_A.word_id = words.id where " + where_str;

		auto query_res = tx.exec(request_str);

		std::pair<std::string, int> map_pair;

		for (auto row : query_res)
		{
			map_pair.first = row["url"].as<std::string>();
			map_pair.second = row["quantity"].as<int>();

			result_map.insert(map_pair);
		}

	}
	catch (const std::exception& ex)
	{
		last_er = ex.what();
		std::cout << "Ошибка = " << ex.what() << "\n";
		return result_map;
	}

	return result_map;
}