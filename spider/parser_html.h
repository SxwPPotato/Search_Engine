#pragma once
#include <iostream>
#include <regex>
#include <set>
#include <map>

class html_parser
{
private:
	std::string complete_url(const std::string& in_url, const std::string& url_base);
	std::string get_base_path(const std::string& in_str);
	bool check_this_host_only(const std::string& host_url, const std::string& url_str, bool this_host_only); 

public:
	int max_word_len = 32; //������������ ����� ����� ��� ���������� � ���� ������
	int min_word_len = 3;  //����������� ����� ����� ��� ���������� � ���� ������

	std::string get_base_host(const std::string& url_str); 
	std::string clear_tags(const std::string& html_body_str);	

	std::set<std::string> get_urls_from_html(const std::string& html_body_str, const std::string& base_str, bool this_host_only, std::string host_url);
	std::map<std::string, unsigned  int> collect_words(const std::string& text_str);
};