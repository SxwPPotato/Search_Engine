#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "http_utils.h"
#include "parser_html.h"
#include "parser_ini.h"
#include <functional>
#include <regex>
#include "Windows.h"
#include "Exeption.h"

std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;
const std::string ini_file_name = "\\base\\spider\\Settings.ini";

int main()
{

	setlocale(LC_ALL, "rus");
	SetConsoleOutputCP(1251);

	parser_ini parser;
	parser.fill_parser(ini_file_name);

	parameters spider_data;

	try
	{
		spider_data.start_url = parser.get_values<std::string>("URLs.start_url");

		spider_data.db_connection_string = "host=" + parser.get_values<std::string>("Database.host") + " ";	
		spider_data.db_connection_string += "port=" + parser.get_values<std::string>("Database.port") + " ";	
		spider_data.db_connection_string += "dbname=" + parser.get_values<std::string>("Database.dbname") + " ";	
		spider_data.db_connection_string += "user=" + parser.get_values<std::string>("Database.user") + " ";	
		spider_data.db_connection_string += "password=" + parser.get_values<std::string>("Database.password") + " ";

		spider_data.search_depth = parser.get_values<int>("Search_settings.search_depth");
		spider_data.max_word_length = parser.get_values<int>("Search_settings.max_word_length");
		spider_data.min_word_length = parser.get_values<int>("Search_settings.min_word_length");

		spider_data.max_threads_num = parser.get_values<int>("Process_settings.max_threads");
		spider_data.empty_thread_sleep_time = parser.get_values<int>("Process_settings.empty_thread_sleep_time");
		spider_data.this_host_only = parser.get_values<bool>("Process_settings.this_host_only");
	}
	catch (const Exception_no_file& ex)
	{
		std::cout << ex.what() << "\n";
		return 0;
	}
	catch (const Exception_no_section& ex)
	{
		std::cout << ex.what() << "\n";
		return 0;
	}
	catch (const Exception_no_variable& ex)
	{
		std::cout << ex.what() << "\n";
		return 0;
	}

	Spider spider(spider_data);
	spider.start_spider();

	return 0;
}


