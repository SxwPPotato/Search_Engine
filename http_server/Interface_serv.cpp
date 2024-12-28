#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <regex>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

#include "Database_serv.h"

namespace beast = boost::beast;    
namespace http = beast::http; 
namespace net = boost::asio;  
using tcp = boost::asio::ip::tcp;      


void fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

beast::string_view mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
        {
            auto const pos = path.rfind(".");
            if (pos == beast::string_view::npos) {
                return beast::string_view{};
            }
            return path.substr(pos);
        }();
    if (iequals(ext, ".htm"))  return "text/html";
    if (iequals(ext, ".html")) return "text/html";
    if (iequals(ext, ".php"))  return "text/html";
    if (iequals(ext, ".css"))  return "text/css";
    if (iequals(ext, ".txt"))  return "text/plain";
    if (iequals(ext, ".js"))   return "application/javascript";
    if (iequals(ext, ".json")) return "application/json";
    if (iequals(ext, ".xml"))  return "application/xml";
    if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if (iequals(ext, ".flv"))  return "video/x-flv";
    if (iequals(ext, ".png"))  return "image/png";
    if (iequals(ext, ".jpe"))  return "image/jpeg";
    if (iequals(ext, ".jpeg")) return "image/jpeg";
    if (iequals(ext, ".jpg"))  return "image/jpeg";
    if (iequals(ext, ".gif"))  return "image/gif";
    if (iequals(ext, ".bmp"))  return "image/bmp";
    if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff")) return "image/tiff";
    if (iequals(ext, ".tif"))  return "image/tiff";
    if (iequals(ext, ".svg"))  return "image/svg+xml";
    if (iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

std::string path_cat(beast::string_view base, beast::string_view path)
{
    if (base.empty()) {
        return std::string(path);
    }
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if (result.back() == path_separator) {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());

    for (auto& c : result) {
        if (c == '/') {
            c = path_separator;
        }
    }
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator) {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
#endif
    return result;
}

std::string open_start_file_search_result(const std::string& file_path)
{
    std::ifstream res_file(file_path);
    if (!res_file.is_open())
    {
        return "file " + file_path + " not found";
    }

    std::string file_content;
    std::string str;

    while (!res_file.eof())
    {
        std::getline(res_file, str);
        file_content += str;
        file_content += "\n";
    }

    res_file.close();
    return file_content;
}

bool split_str_content(const std::string& source_str, std::string& start_str, std::string& end_str) 
{
    std::smatch res;
    if (regex_search(source_str, res, std::regex("<!--search result here-->")))
    {
        start_str = res.prefix();
        end_str = res.suffix();
        return true;
    }
    else return false;
}

std::string clear_request_string(const std::string& source_str)
{
    std::string field_name = "search_request=";

    if (!source_str.find(field_name) == 0) {
        return "";
    }

    std::string res_string = source_str;

    res_string.erase(0, field_name.size());

    res_string = std::regex_replace(res_string, std::regex("%09"), " ");
    res_string = std::regex_replace(res_string, std::regex("([\.,:;!?\\\"'*+=_~#$^&])"), " "); 
    res_string = std::regex_replace(res_string, std::regex(" {2,}"), " ");

    std::transform(res_string.begin(), res_string.end(), res_string.begin(), 
        [](unsigned char c) { return std::tolower(c); });

    return res_string;
}

std::set<std::string> get_words_request_set(const std::string& source_str)
{
    std::set<std::string> result_set;

    std::istringstream in_string{ source_str };

    std::string single_word;
    while (!in_string.eof())
    {
        in_string >> single_word;
        result_set.insert(single_word);
    }
    return result_set;
}

bool urls_vector_cmp(std::pair<std::string, int> pair_a, std::pair<std::string, int> pair_b) 
{
    return   pair_a.second > pair_b.second ? true : false;
}

std::string get_post_request_result_string(const std::string& request_string, Database* database, int search_results)
{
    std::set<std::string> words_set;
    std::map<std::string, int> map_urls_list; 
    std::multimap<std::string, int> words_urls_table; 
    std::vector<std::pair<std::string, int>> final_array; 

    words_set = get_words_request_set(request_string); 
    const int words_num = words_set.size();


    map_urls_list = database->get_urls_list_by_words(words_set); 



    for (auto& url : map_urls_list)
    {
        int url_words_count = database->count_url_words(words_set, url.first);
        if (words_num == url_words_count)
        {
            url.second = 0;
        }
    }

    words_urls_table = database->get_words_urls_table(words_set); 

    for (auto url : words_urls_table)
    {

        if (map_urls_list[url.first] >= 0) 
        {
            map_urls_list[url.first] = map_urls_list[url.first] + url.second;
        }
    }

    for (auto url : map_urls_list)
    {
        if (url.second > 0)
        {
            final_array.push_back(std::pair<std::string, int>(url.first, url.second));
        }
    }

    sort(final_array.begin(), final_array.end(), urls_vector_cmp);

    for (auto url : final_array)
    {
        std::cout << url.first << " = " << url.second << "\n";
    }

    int results_num = (search_results > final_array.size() ? final_array.size() : search_results);

    std::string search_result_string;
    for (int i = 0; i < results_num; ++i)
    {
        search_result_string += "<p>" + std::to_string(i + 1) + ". <a href = \"" + final_array[i].first + "\">" + final_array[i].first + "</a></p>\n";
    }

    if (search_result_string.empty())
    {
        search_result_string = "<p>Sorry, there are no results for your request. Try another one.</p>\n";
    }

    return search_result_string;
}

std::string prepare_body_string(const std::string& path, const std::string& request_string, const std::string& search_result_string)
{
    std::string body_str = open_start_file_search_result(path);

    std::string start_str;
    std::string end_str;

    if (split_str_content(body_str, start_str, end_str))
    {
        body_str = start_str + "<p>Your search: " + request_string + "</p>\n" + search_result_string + end_str;
    }

    return body_str;
}