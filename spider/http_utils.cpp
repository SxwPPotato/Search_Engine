#include "http_utils.h"
#include "http_client_async_ssl.cpp"
#include "http_client_async.cpp"

#include <regex>

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ip = boost::asio::ip;
namespace ssl = boost::asio::ssl;

using tcp = boost::asio::ip::tcp;

Spider::Spider(parameters spider_data)
{
	db_connection_string = spider_data.db_connection_string;

	pool_queue.empty_sleep_for_time = spider_data.empty_thread_sleep_time;
	max_depth = spider_data.search_depth;
	this_host_only = spider_data.this_host_only;
	max_threads_num = spider_data.max_threads_num;
	start_url = spider_data.start_url;
	my_html_parser.min_word_len = spider_data.min_word_length;
	my_html_parser.max_word_len = spider_data.max_word_length;
}

Spider::~Spider()
{
	for (auto& el : th_vector)
	{
		auto thr_id = el.get_id();
		el.join();
		std::cout << "work thread id = " << thr_id << " finished\n";
	}

	delete database;

	std::cout << "\nIndexing finished. Total " << pool_queue.list_of_urls.size() << " pages were processed:\n";
	print_urls_list();
}


void Spider::work(const int& thread_index)
{
	std::unique_lock lk(threads_start_mutex);
	start_threads.wait(lk, [this] {return start_work; });
	std::cout << "start working id: " << std::this_thread::get_id() << " thread_index = " << thread_index << " \n";
	lk.unlock();

	while (!task_generator_finished(thread_index))
	{
		if (process_next_task(thread_index))
		{
			std::unique_lock lk_state(threads_get_state); 									
			std::cout << get_queue_state();
			lk_state.unlock();
			std::cout << get_threads_state();
		}
	}
}

void Spider::start_spider()
{
	database = new Database(db_connection_string);

	if (!database->start_db())
	{
		std::cout << "Database not started. Further work is impossible. \n";

		database->print_last_er(); 
		return;
	}

	start_work_threads(); 
}

bool Spider::process_next_task(const int& thread_index)
{
	std::set<std::string> new_urls_set;
	std::map<std::string, unsigned int> new_words_map;

	bool result = false;

	url_item task;
	if (pool_queue.sq_pop(task, thread_index)) 
	{
		th_vector[thread_index].in_work = true; 
		th_vector[thread_index].thread_task = task;

		if (work_function(task, new_urls_set, new_words_map))
		{
			add_url_words_to_database(task.url, new_words_map); 

			for (auto& el : new_urls_set)
			{
				pool_queue.sq_push(url_item{ el, task.url_depth + 1 }, thread_index);
			}

			std::string out_str = "url " + task.url + " " + std::to_string(task.url_depth) + " thread " + std::to_string(thread_index) + " finished\n";
			std::cout << out_str;
			std::cout << get_queue_state();
		}
		result = true;
		total_pages_processed++;
	}

	th_vector[thread_index].in_work = false;

	return result;
}

void Spider::submit(const url_item new_url_item, const int work_thread_num) 
{
	pool_queue.sq_push(new_url_item, work_thread_num);
}


void Spider::start_work_threads()
{
	const auto cores = std::thread::hardware_concurrency();
	std::cout << "Total threads number = " << cores << "\n";
	int cores_num = ((cores - 1) < max_threads_num ? (cores - 1) : max_threads_num); 
	std::cout << "Work threads number = " << cores_num << "\n";

	for (int i = 0; i < cores_num; ++i)
	{
		th_vector.push_back(url_processing_thread{ std::thread(&Spider::work, this, i) });
		std::cout << "work thread id = " << th_vector[i].get_id() << " created\n";
	}

	submit(url_item{ start_url, 1 }, -1);


	start_work = true;
	start_threads.notify_all();
}


std::atomic<bool> Spider::task_generator_finished(const int thread_num)
{
	if (pool_queue.not_empty())
		return false;

	for (auto& el : th_vector)
	{
		if (el.in_work)
			return false;
	}

	return true;
}

bool Spider::work_function(const url_item& new_url_item, std::set<std::string>& new_urls_set, std::map<std::string, unsigned int>& new_words_map)
{

	std::string url_address = new_url_item.url;

	host_type url_host_type = get_host_type(url_address);
	if (url_host_type == host_type::host_unknown) return false;

	std::string host;
	std::string target;
	get_host_and_target(url_address, host, target);

	std::string out_str = "host = " + host + "\n";
	std::cout << out_str;
	out_str = "target = " + target + "\n";
	std::cout << out_str;


	net::io_context ioc; 
	std::promise<std::tuple<int, std::string, std::string>> promise;
	std::future<std::tuple<int, std::string, std::string>> future = promise.get_future();

	if (url_host_type == host_type::host_http) // http
	{
		std::make_shared<session>(ioc, promise)->run(host.c_str(), target.c_str());
	}
	else if (url_host_type == host_type::host_https) // https
	{
		ssl::context ctx{ ssl::context::tlsv12_client };

		load_root_certificates(ctx);

		ctx.set_verify_mode(ssl::verify_peer);

		std::make_shared<session_ssl>(net::make_strand(ioc), ctx, promise)->run(host.c_str(), target.c_str());
	}
	else
	{
		return false;
	}

	ioc.run();
	std::tuple<int, std::string, std::string> result = future.get();

	request_result url_request_result = static_cast<request_result>(std::get<0>(result));
	std::string url_body = std::get<1>(result);
	std::string redirection_url = std::get<2>(result);

	switch (url_request_result)
	{
	case request_result::req_res_ok:
	{
		if (new_url_item.url_depth <= (max_depth - 1))
		{
			std::string base_host = my_html_parser.get_base_host(new_url_item.url);
			new_urls_set.clear();
			new_urls_set = my_html_parser.get_urls_from_html(url_body, base_host, this_host_only, new_url_item.url);

			if (new_urls_set.size() == 0)
			{
				std::string out_str = "no links got from page " + new_url_item.url + "\n";
				std::cout << out_str;
			}
		}

		std::string text_str = my_html_parser.clear_tags(url_body);
		new_words_map.clear();
		new_words_map = my_html_parser.collect_words(text_str);
		break;
	}
	case request_result::req_res_redirect:
	{
		new_urls_set.insert(redirection_url);
		break;
	}
	default: 	return false;
	}

	return true;

}

void Spider::get_host_and_target(std::string& new_url, std::string& host, std::string& target)
{
	int pos;
	while (!(pos = new_url.find("/")))
	{
		new_url.erase(0, 1);
	}

	pos = new_url.find("/");

	if (pos == std::string::npos)
	{
		host = new_url;
		target = "/";
	}
	else
	{
		target = new_url.substr(pos);
		host = new_url.erase(pos, target.length());
	}
}

Spider::host_type Spider::get_host_type(std::string& new_url)
{
	size_t pos = new_url.find("http://");
	if (pos == 0)
	{
		new_url.erase(0, std::string("http://").size());
		return host_type::host_http;
	}

	pos = new_url.find("https://");
	if (pos == 0)
	{
		new_url.erase(0, std::string("https://").size());
		return host_type::host_https;
	}

	return host_type::host_unknown;
}

void Spider::add_url_words_to_database(const std::string& url_str, const std::map<std::string, unsigned int>& words_map)
{
	std::unique_lock lk_db(data_base_mutex); 

	if (words_map.size() == 0) return;

	database->add_new_url(url_str);

	int url_id = database->get_url_id(url_str);
	if (url_id < 0)
	{
		return;
	}

	for (auto& el : words_map)
	{
		database->add_new_word(el.first);
		int word_id = database->get_word_id(el.first);
		if (word_id < 0)
		{
			continue;
		}

		if (database->get_word_url_exist(url_id, word_id))
		{
			database->update_word_url_pair(url_id, word_id, el.second); 
		}
		else
		{
			database->add_new_word_url_pair(url_id, word_id, el.second); 
		}
	}
}


std::string Spider::get_queue_state()
{
	std::string res_str = "\n________Indexing state:\n";
	res_str += "urls in queue to be processed: " + std::to_string(pool_queue.get_queue_size()) + "\n";
	res_str += "urls already have been processed: " + std::to_string(total_pages_processed) + "\n";
	res_str += "total urls in list: " + std::to_string(pool_queue.list_of_urls.size()) + "\n";
	res_str += "\n\n";

	return res_str;
}

std::string Spider::get_threads_state()
{
	std::string res_str = "\n________Threads state:\n";

	for (int i = 0; i < th_vector.size(); ++i)
	{
		res_str += "Thread num " + std::to_string(i);
		if (th_vector[i].in_work)
		{
			res_str += " - is in work: " + th_vector[i].thread_task.url + " depth = " + std::to_string(th_vector[i].thread_task.url_depth) + " \n";
		}
		else
		{
			res_str += " - is idle\n";
		}
	}

	res_str += "\n";
	return res_str;
}

void Spider::print_urls_list()
{
	for (auto& el : pool_queue.list_of_urls)
	{
		std::cout << el << "\n";
	}
}