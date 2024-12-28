#pragma once
#include <iostream>
#include <fstream>
#include <regex>
#include <set>

#include "Database_serv.h"


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio; 
using tcp = boost::asio::ip::tcp;      

beast::string_view mime_type(beast::string_view path);
std::string path_cat(beast::string_view base, beast::string_view path);
void fail(beast::error_code ec, char const* what); 


std::string open_start_file_search_result(const std::string& file_path);
bool split_str_content(const std::string& source_str, std::string& start_str, std::string& end_str);
std::string clear_request_string(const std::string& source_str);
std::set<std::string> get_words_request_set(const std::string& source_str); 
bool urls_vector_cmp(std::pair<std::string, int> pair_a, std::pair<std::string, int> pair_b); 
std::string get_post_request_result_string(const std::string& request_string, Database* data_base, int search_results);  
std::string prepare_body_string(const std::string& path, const std::string& request_string, const std::string& search_result_string);

template <class Body, class Allocator>
http::message_generator handle_request(beast::string_view doc_root,http::request<Body, http::basic_fields<Allocator>>&& req, int search_results, Database* database)
{
    auto const bad_request =
        [&req](beast::string_view why)
        {
            http::response<http::string_body> res{ http::status::bad_request, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "<p>" + std::string(why) + "</p>";
            res.prepare_payload();
            return res;
        };

    auto const not_found =
        [&req](beast::string_view target)
        {
            http::response<http::string_body> res{ http::status::not_found, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "<p>The resource '" + std::string(target) + "' was not found.</p><p>Try from <a href=\"Search_page.html\">the start page</a></p>";
            res.prepare_payload();
            return res;
        };

    auto const server_error =
        [&req](beast::string_view what)
        {
            http::response<http::string_body> res{ http::status::internal_server_error, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "<p>An error has occurred: '" + std::string(what) + "'</p>";
            res.prepare_payload();
            return res;
        };

    if (req.method() != http::verb::get && req.method() != http::verb::head && req.method() != http::verb::post) {
        return bad_request("Unknown HTTP method");
    }

    if (req.target().empty() || req.target()[0] != '/' || req.target().find("..") != beast::string_view::npos) {
        return bad_request("Invalid request target");
    }

    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/') {
        path = "\\base\\http_server\\Search_page.html";
    }

    beast::error_code ec;
    http::file_body::value_type body;

    body.open(path.c_str(), beast::file_mode::scan, ec);

    if (ec == beast::errc::no_such_file_or_directory) {
        return not_found(req.target());
    }

    if (ec) {
        return server_error(ec.message());
    }

    auto const size = body.size();

    if (req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{ http::status::ok, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    else if (req.method() == http::verb::get)
    {
        http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version()) };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }


    std::string request_string = clear_request_string(req.body());
    std::cout << "request_string = " << request_string << "\n\n";
    std::string search_result_string = get_post_request_result_string(request_string, database, search_results); 

    http::response<http::string_body> res{ http::status::ok, req.version() };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.keep_alive(req.keep_alive());
    res.body() = prepare_body_string(path, request_string, search_result_string);
    res.content_length(res.body().size());
    res.prepare_payload();
    std::cout << "get req search_results = " << search_results << "\n";
    return res;
}