#pragma once
#include <iostream>
#include <pqxx/pqxx> 

class Database
{
private:
	const std::string connection; 
	pqxx::connection* connection_ptr = NULL;
	std::string last_er; 
	bool connect_db(); 
	bool create_tables(); 
	bool create_templates();

	bool add_new_str(const std::string& str, std::string tmpl);
	int get_str_id(const std::string& str, std::string tmpl);
	bool new_word_url_pair(int url_id, int word_id, int num, std::string tmpl);

public:
	bool test_insert();
	bool start_db(); //����� ���� ������
	void print_last_er();
	explicit Database(const std::string params_str)  noexcept;

	std::string get_last_error();
	

	Database& operator=(const Database& other) = delete;  
	Database& operator=(Database&& other) noexcept;
	Database(const Database& other) = delete; 
	Database(Database&& other) noexcept;
	~Database();


	bool add_new_url(const std::string& url_str); //�������� ����� url
	bool add_new_word(const std::string& word_str); //�������� ����� �����
	bool add_new_word_url_pair(int url_id, int word_id, int num); //�������� ����� ���������� ����
	bool update_word_url_pair(int url_id, int word_id, int num); //�������� ���������� ����

	int get_url_id(const std::string& url_str); //�������� id url
	int get_word_id(const std::string& word_str); //�������� id �����
	bool get_word_url_exist(int url_id, int word_id); //���������� �� ������ � ������ id �������� � �����
};