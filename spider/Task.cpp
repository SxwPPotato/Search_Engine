#include <chrono>
#include "Task.h"

using namespace std::chrono_literals;

void tasks_queue::sq_push(const url_item& new_url_item, const int work_thread_num)
{
	std::lock_guard<std::mutex> lk(mutex);

	int queue_length = list_of_urls.size();
	list_of_urls.insert(new_url_item.url);

	if (list_of_urls.size() > queue_length)
	{
		urls.push(new_url_item);
		std::cout << "new task added: " << new_url_item.url << " depth = " << new_url_item.url_depth << " thread " << work_thread_num << " \n";
	}

	data_cond.notify_all();
}

bool tasks_queue::sq_pop(url_item& task, const int work_thread_num)
{
	if (is_empty())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(empty_sleep_for_time));
		return false;
	}

	std::unique_lock lk(mutex);

	data_cond.wait(lk, [this] {return (!(urls.empty())); });

	if (!(urls.empty()))
	{
		task = urls.front();
		urls.pop();
		std::cout << "thread " << work_thread_num << ":  task pop : url = " << task.url << " depth = " << task.url_depth << std::endl;
	}
	else
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(empty_sleep_for_time));
	}
	lk.unlock();

	return true;
}

bool tasks_queue::not_empty()
{
	std::lock_guard<std::mutex> lk(mutex);
	return !urls.empty();
}

bool tasks_queue::is_empty()
{
	std::lock_guard<std::mutex> lk(mutex);
	return urls.empty();
}

int tasks_queue::get_queue_size()
{
	std::lock_guard<std::mutex> lk(mutex);
	return urls.size();
}