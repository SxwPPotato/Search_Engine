#include "Database.h"


Database::Database(const std::string params_str) noexcept : connection{ params_str } {}

std::string Database::get_last_error()
{
	return last_er;
}

bool Database::test_insert()
{
	if (connection_ptr == nullptr)
	{
		last_er = "Ошибка подключения к базе данных";
		return false;
	}

	try
	{
		if (!(connection_ptr->is_open()))
		{
			return false;
		}

		pqxx::work tx{ *connection_ptr };

		tx.exec(
			"insert into documents (url) values "
			"('http://google.com/'), "
			"('http://google2.com/'), "
			"('http://google3.com/'); ");

		tx.commit();
	}
	catch (const std::exception& ex)
	{
		last_er = ex.what();
		return false;
	}
	catch (...)
	{
		last_er = "Ошибка добавления данных";
		return false;
	}

	return true;
}

// конструктор перемещения
Database::Database(Database&& other) noexcept : connection{ other.connection }
{
	connection_ptr = other.connection_ptr;
	last_er = other.last_er;

	other.connection_ptr = nullptr;
}

// оператор перемещающего присваивания
Database& Database::operator=(Database&& other) noexcept
{
	connection_ptr = other.connection_ptr;
	last_er = other.last_er;

	other.connection_ptr = nullptr;
	return *this;
}


// Деструктор
Database::~Database()
{
	delete connection_ptr;
}

bool Database::add_new_url(const std::string& url_str)
{
	return add_new_str(url_str, "insert_url");
}

bool Database::add_new_word(const std::string& word_str)
{
	return add_new_str(word_str, "insert_word");
}

bool Database::add_new_word_url_pair(int url_id, int word_id, int num)
{
	return new_word_url_pair(url_id, word_id, num, "add_url_word_num");
}

bool Database::update_word_url_pair(int url_id, int word_id, int num)
{
	return new_word_url_pair(url_id, word_id, num, "update_url_word_num");
}


int Database::get_url_id(const std::string& url_str)
{
	return get_str_id(url_str, "search_url_id");
}

int Database::get_word_id(const std::string& word_str)
{
	return get_str_id(word_str, "search_word_id");
}

bool Database::get_word_url_exist(int url_id, int word_id)
{
	if (connection_ptr == nullptr)
	{
		last_er = "Ошибка подключения к базе данных";
		return false;
	}

	last_er = "";
	pqxx::work tx{ *connection_ptr };

	auto query_res = tx.exec_prepared("search_url_word_num", url_id, word_id);

	if (query_res.empty())	
	{
		return false;
	}
	else {
		return true;
	}
}

bool Database::connect_db() 
{
	try 
	{
		connection_ptr = new pqxx::connection(connection);
		return true;
	}
	catch (const std::exception& ex)
	{
		last_er = ex.what();
		return false;
	}
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

		tx.exec(
			"CREATE table IF NOT EXISTS documents ( "
			"id serial primary KEY, "
			"url varchar(250) NOT NULL  UNIQUE, "
			"constraint url_unique unique(url)); ");

		tx.exec(
			"CREATE table IF NOT EXISTS words ( "
			"id serial primary KEY, "
			"word varchar(32) NOT NULL  UNIQUE, "
			"constraint word_unique unique(word)); ");
	
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
		last_er = "Ошибка создания таблиц базы данных";
		return false;
	}
}

bool Database::create_templates()
{
	if ((connection_ptr == nullptr) || (!(connection_ptr->is_open())))
	{
		last_er = "Ошибка создания шаблонов запросов – нет подключения к базе данных.";
		return false;
	}

	try
	{
		connection_ptr->prepare("insert_url", "insert into documents (url) values ($1)");

		connection_ptr->prepare("insert_word", "insert into words (word) values ($1)");

		connection_ptr->prepare("search_url_id", "select id from documents where url = $1");

		connection_ptr->prepare("search_word_id", "select id from words where word = $1");

		connection_ptr->prepare("search_url_word_num", "select quantity from urls_words where url_id = $1 and word_id = $2");

		connection_ptr->prepare("add_url_word_num", "insert into urls_words(url_id, word_id, quantity) values($1, $2, $3)");
	
		connection_ptr->prepare("update_url_word_num", "update urls_words set quantity = $3 where  url_id = $1 and word_id = $2");

		return true;
	}

	catch (...)
	{
		last_er = "Ошибка создания шаблонов запросов";
		return false;
	}
}

bool Database::add_new_str(const std::string& str, std::string tmpl)
{
	if (connection_ptr == nullptr)
	{
		last_er = "Ошибка подключения к базе данных";
		return false;
	}

	last_er = "";
	try
	{
		pqxx::work tx{ *connection_ptr };
		tx.exec_prepared(tmpl, str);
		tx.commit();

		return true;
	}
	catch (const std::exception& ex)
	{
		last_er = ex.what();
		return false;
	}

}

int Database::get_str_id(const std::string& str, std::string tmpl)
{
	if (connection_ptr == nullptr)
	{
		last_er = "Ошибка подключения к базе данных";
		return -1;
	}

	last_er = "";

	try
	{
		pqxx::work tx{ *connection_ptr };

		auto query_res = tx.exec_prepared(tmpl, str);
		if (query_res.empty())
		{
			return -1;
		}

		auto row = query_res.begin();
		int res_int = row["id"].as<int>();
		return res_int; 
	}
	catch (const std::exception& ex)
	{
		last_er = ex.what();
		return -1;
	}
}

bool Database::new_word_url_pair(int url_id, int word_id, int num, std::string tmpl)
{
	if (connection_ptr == nullptr)
	{
		last_er = "Ошибка подключения к базе данных";
		return false;
	}

	last_er = "";

	try
	{
		pqxx::work tx{ *connection_ptr };
		tx.exec_prepared(tmpl, url_id, word_id, num);
		tx.commit();

		return true;
	}
	catch (const std::exception& ex)
	{
		last_er = ex.what();
		return false;
	}
}

bool Database::start_db()
{
	bool result = false;

	if (connect_db()) 
	{
		result = create_tables() and create_templates();
	}

	return result;
}

void Database::print_last_er()
{
	std::cout << "Ошибка: " << last_er << "\n";
}

