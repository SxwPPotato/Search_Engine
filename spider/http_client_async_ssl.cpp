#include "root_certificates.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <future>

namespace beast = boost::beast;         
namespace http = beast::http;          
namespace net = boost::asio;           
namespace ssl = boost::asio::ssl;     
using tcp = boost::asio::ip::tcp;     


class session_ssl : public std::enable_shared_from_this<session_ssl>
{
    tcp::resolver resolver_;
    ssl::stream<beast::tcp_stream> stream_;
    beast::flat_buffer buffer_;
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;
    std::promise<std::tuple<int, std::string, std::string>>& promise_;      

private:
    std::string const port = "443";
    int version = 11;

public:
    explicit session_ssl(
        net::any_io_executor ex,
        ssl::context& ctx,
        std::promise<std::tuple<int, std::string, std::string>>& promise) :
        resolver_(ex),
        stream_(ex, ctx),
        promise_(promise) 
    {
    }

    void  run(
        char const* host,
        char const* target

    )
    {
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host))
        {
            beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
            std::cerr << ec.message() << "\n";
            return;
        }

        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &session_ssl::on_resolve,
                shared_from_this()));
    }

    void    on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
    {
        if (ec)
            return fail(ec, "resolve");

        beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        beast::get_lowest_layer(stream_).async_connect(
            results,
            beast::bind_front_handler(
                &session_ssl::on_connect,
                shared_from_this()));
    }

    void   on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
    {
        if (ec)
            return fail(ec, "connect");

        stream_.async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(
                &session_ssl::on_handshake,
                shared_from_this()));
    }

    void   on_handshake(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "handshake");

        beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        http::async_write(stream_, req_,
            beast::bind_front_handler(
                &session_ssl::on_write,
                shared_from_this()));
    }

    void  on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        http::async_read(stream_, buffer_, res_,
            beast::bind_front_handler(
                &session_ssl::on_read,
                shared_from_this()));
    }

    void   on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        send_response(res_);

        beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        stream_.async_shutdown(
            beast::bind_front_handler(
                &session_ssl::on_shutdown,
                shared_from_this()));
    }

    void
        on_shutdown(beast::error_code ec)
    {
 

        if (ec != net::ssl::error::stream_truncated)
            return fail(ec, "shutdown");
    }

private:
    void  fail(beast::error_code ec, char const* what)
    {
        if (!(std::string(what) == "shutdown"))
        {
            std::string response_str = "Failed to " + std::string(what);
            std::tuple<int, std::string, std::string> res_tuple = std::make_tuple(0, response_str, "");
            promise_.set_value(res_tuple);
        }

    }

    void send_response(http::response<http::string_body>& res)
    {

        std::string new_location = "";
        std::string res_body = "";

        int status = res.result_int();


        switch (status)
        {
        case 200:
        {
            auto content_type = res.find("Content-Type");
            std::string content_type_str = (*content_type).value();
            int pos = content_type_str.find("text/html");
            if (pos == std::string::npos)
            {
                res_body = "";
            }
            else
            {
                res_body = res.body().data();
            }
            break;
        }
        case 301:
        {
            auto location_it = res.find("Location");
            new_location = (*location_it).value();
            break;
        }

        }

        std::tuple<int, std::string, std::string> res_tuple = std::make_tuple(status, res_body, new_location);
        promise_.set_value(res_tuple);
    }
};