#include "parser_html.h"

std::string html_parser::complete_url(const std::string& in_url, const std::string& url_base)
{
    if (in_url.empty())
    {
        return  url_base;
    }

    std::string res_url = in_url;
    std::regex_replace(res_url, std::regex("/$"), "");

    auto pos = res_url.find("https://");
    if (pos == 0) return res_url;

    pos = res_url.find("http://");
    if (pos == 0) return res_url;

    pos = res_url.find("www.");
    if (pos == 0) return res_url;

    pos = res_url.find("/");
    if (pos == 0)
    {
        res_url.erase(0, 1);
    }


    res_url = url_base + "/" + res_url;

    return res_url;
}

std::set<std::string> html_parser::get_urls_from_html(const std::string& html_body_str, const std::string& base_str, bool this_host_only, std::string host_url)
{
    std::set<std::string> urls_set;
    std::string s2 = html_body_str;

    host_url = get_base_host(host_url);

    std::smatch res1;
    std::regex r1("<a (.?[^>]*?)href=\"(.*?)\"");
    while (regex_search(s2, res1, r1))
    {
        std::string find_str = res1.str();
        std::string url_str = std::regex_replace(find_str, std::regex("<a (.?[^>]*?)href=\""), "");
        url_str = std::regex_replace(url_str, std::regex("\""), ""); 
        url_str = std::regex_replace(url_str, std::regex("/$"), "");   
        std::string new_base_path = get_base_path(url_str); 


        std::string suf_str = url_str.erase(0, new_base_path.size());
        std::string final_url = complete_url(new_base_path, base_str) + suf_str;

        if (check_this_host_only(host_url, final_url, this_host_only))
        {
            urls_set.insert(final_url);
        }

        s2 = res1.suffix(); 
    };
    return urls_set;
}

std::string html_parser::clear_tags(const std::string& html_body_str)
{

    std::string str = html_body_str;

    str = std::regex_replace(str, std::regex("[\n\t]"), " ");
    str = std::regex_replace(str, std::regex("< {1,}"), "<");
    str = std::regex_replace(str, std::regex(" {1,}>"), ">");
    str = std::regex_replace(str, std::regex(">"), "> ");
    str = std::regex_replace(str, std::regex("< /"), "</");
    str = std::regex_replace(str, std::regex("</ "), "</");

    std::string regex;

    std::string finish_str;
    std::smatch res;

    regex = ("^.+?(<body)");
    str = std::regex_replace(str, std::regex(regex), "");

    regex = "<title>(.?)[^<]*";
    if (regex_search(str, res, std::regex(regex)))
    {
        finish_str = res.str();
        finish_str = std::regex_replace(finish_str, std::regex("<title>"), "");
    }

    regex = "<(.?)[^>][^<]*>";
    while (regex_search(str, res, std::regex(regex)))
    {
        str = std::regex_replace(str, std::regex(regex), "");
    };

    regex = ("(u003c)");
    str = std::regex_replace(str, std::regex(regex), " ");
    regex = ("(u003e)");
    str = std::regex_replace(str, std::regex(regex), " ");

    finish_str += " ";
    finish_str += str;

    finish_str = std::regex_replace(finish_str, std::regex("[\.,:;@!~=%&#\^\|\$/\?\<>\(\)\{\}\"'\*\+_\-]"), " ");
    finish_str = std::regex_replace(finish_str, std::regex(" {2,}"), " ");

    int pos;
    while (!((pos = finish_str.find("\\")) == std::string::npos))
    {
        finish_str.erase(pos, 1);
    }

    while (!((pos = finish_str.find("]")) == std::string::npos))
    {
        finish_str.erase(pos, 1);
    }


    std::transform(finish_str.begin(), finish_str.end(), finish_str.begin(),
        [](unsigned char c) { return std::tolower(c); });


    return finish_str;
}

std::string html_parser::get_base_host(const std::string& url_str)
{
    std::string res_str = url_str;

    std::string http_prefix = "https://";

    if (res_str.find(http_prefix) == std::string::npos)
    {
        http_prefix = "http://";
        if (res_str.find(http_prefix) == std::string::npos)
        {
            http_prefix = "";
        }
    }

    res_str.erase(0, http_prefix.size());

    auto pos = res_str.find("/");
    if (pos != std::string::npos)
    {
        int num = res_str.size() - pos;
        res_str.erase(pos, num);
    }

    return http_prefix + res_str;
}

std::string html_parser::get_base_path(const std::string& in_str)
{
    std::string res_str = in_str;
    res_str = std::regex_replace(res_str, std::regex("/$"), "");

    std::string http_prefix = "https://";
    if (res_str.find(http_prefix) == std::string::npos)
    {
        http_prefix = "http://";
        if (res_str.find(http_prefix) == std::string::npos)
        {
            http_prefix = "";
        }
    }

    res_str.erase(0, http_prefix.size());

    std::smatch res;
    std::regex r("/(.[^/]*)$");
    if (regex_search(res_str, res, r))
    {
        std::string find_str = res.str();
        auto n_pos = res_str.find(find_str);
        if (find_str.find(".") != std::string::npos)
        {
            res_str.erase(n_pos, find_str.size());
        }
    };

    res_str = std::regex_replace(res_str, std::regex("/{2,}"), "/");

    return http_prefix + res_str;
}

std::map<std::string, unsigned  int> html_parser::collect_words(const std::string& text_str)
{
    std::string search_str = text_str;

    std::smatch res;
    std::regex r("(.[^ ]*)");

    std::map<std::string, unsigned  int> words_map;

    while (regex_search(search_str, res, r))
    {
        std::string find_str = res.str();
        find_str = std::regex_replace(find_str, std::regex(" "), "");

        int len = find_str.size();
        if ((len >= min_word_len) && (len <= max_word_len))
        {
            auto word_pair = words_map.find(find_str);
            if (word_pair != words_map.end())
            {
                words_map[find_str] = words_map[find_str] + 1;                
            }
            else
            {
                words_map[find_str] = 1;
            }
        }

        search_str = res.suffix();
    };

    return words_map;
}

bool html_parser::check_this_host_only(const std::string& host_url, const std::string& url_str, bool this_host_only) 
{
    if (!(this_host_only)) return true; 

    if (url_str.find(host_url) != 0)
    {
        return false;
    }
    else
        return true;
}