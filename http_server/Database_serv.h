#pragma once
#include <iostream>
#include <pqxx/pqxx> 
#include <set>
#include <map>


class Database
{
private:
	const std::string connection_str; 
	pqxx::connection* connection_ptr = NULL;
	std::string last_er; 
	bool connect_db(); 
	bool create_tables(); 


	std::set<std::string> list_of_urls;

public:

	explicit Database(const std::string params_str)  noexcept;

	bool start_db(); 
	std::string get_last_error_desc(); 
	void print_last_error(); 

	Database(const Database& other) = delete; //конструктор копирования
	Database(Database&& other) noexcept;	// конструктор перемещения
	Database& operator=(const Database& other) = delete;  //оператор присваивания 
	Database& operator=(Database&& other) noexcept;       // оператор перемещающего присваивания
	~Database();

	std::map<std::string, int>  get_urls_list_by_words(const std::set<std::string>& words_set);
	int count_url_words(const std::set<std::string>& words_set, std::string url); 
	std::multimap<std::string, int> get_words_urls_table(const std::set<std::string>& words_set); 
	std::string prepare_words_where_or(const std::set<std::string>& words_set, pqxx::work& tx);


};