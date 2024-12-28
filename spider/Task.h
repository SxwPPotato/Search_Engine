#pragma once
#include <iostream>
#include <queue>
#include <mutex>
#include <functional>
#include <string>
#include <set>
#include <condition_variable>

struct url_item
{
	std::string url;
	int url_depth;
};

class tasks_queue
{
private:
	std::queue<url_item>  urls;	
	std::mutex mutex;
	std::condition_variable data_cond;

public:
	unsigned int empty_sleep_for_time = 100;
	std::set<std::string> list_of_urls;

	void sq_push(const url_item& new_url_item, const int work_thread_num);
	bool sq_pop(url_item& task, const int work_thread_num);
	bool not_empty();
	bool is_empty(); 

	int get_queue_size();

};