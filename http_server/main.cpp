#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include "parser_ini.h"
#include "server.cpp"
#include "Database_serv.h"
#include <regex>

const std::string ini_file_name = "\\base\\http_server\\Settings.ini";

int main(int argc, char* argv[])
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	parser_ini parser;
	parser.fill_parser(ini_file_name);

	std::string address_str;
	std::string port_str;
	std::string db_connection_string;
	int search_results;

	try
	{
		address_str = parser.get_values<std::string>("Server.host");
		port_str = parser.get_values<std::string>("Server.port");

		db_connection_string = "host=" + parser.get_values<std::string>("Database.host") + " ";	
		db_connection_string += "port=" + parser.get_values<std::string>("Database.port") + " ";	
		db_connection_string += "dbname=" + parser.get_values<std::string>("Database.dbname") + " ";	
		db_connection_string += "user=" + parser.get_values<std::string>("Database.user") + " ";	
		db_connection_string += "password=" + parser.get_values<std::string>("Database.password") + " ";

		search_results = parser.get_values<int>("Search_settings.search_results");

	}
	catch (const Exception_no_file& ex)
	{
		std::cout << ex.what() << "\n";
		std::cout << "Поисковый сервер не готов к работе" << "\n";
		return 0;
	}
	catch (const Exception_no_section& ex)
	{
		std::cout << ex.what() << "\n";
		std::cout << "Поисковый сервер не готов к работе" << "\n";
		return 0;
	}
	catch (const Exception_no_variable& ex)
	{
		std::cout << ex.what() << "\n";
		std::cout << "Поисковый сервер не готов к работе" << "\n";
		return 0;
	}

	Database database(db_connection_string);
	if (!database.start_db())
	{
		std::cout << "Database not started. Further work is impossible. \n";

		database.print_last_error(); 
		return 0;
	}

	auto const address = net::ip::make_address(address_str);
	auto const port = static_cast<unsigned short>(std::stoi(port_str));

	auto const doc_root = std::make_shared<std::string>(".");
	auto const threads = 1;

	net::io_context ioc{ threads };

	std::make_shared<listener>(
		ioc,
		tcp::endpoint{ address, port },
		doc_root,
		search_results,
		&database)->run();
	
	std::vector<std::thread> v;
	v.reserve(threads - 1);
	for (auto i = threads - 1; i >  0; --i) 
		v.emplace_back([&ioc]
			{
				ioc.run();
			});

	std::cout << "Open browser and connect to http://localhost:12345 to see the web server operating" << std::endl;

	ioc.run();
	
	return EXIT_SUCCESS;

}