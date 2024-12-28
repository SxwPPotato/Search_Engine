#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <future>

namespace beast = boost::beast;         
namespace http = beast::http;          
namespace net = boost::asio;           
using tcp = boost::asio::ip::tcp;      


class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_; 
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;
    std::promise<std::tuple<int, std::string, std::string>>& promise_;  

private:
    std::string const port = "80";
    int version = 11;

public:
    explicit session(net::io_context& ioc, std::promise<std::tuple<int, std::string, std::string>>& promise) :
        resolver_(net::make_strand(ioc)),
        stream_(net::make_strand(ioc)),
        promise_(promise)  
    {
    }

    void  run(
        char const* host,
        char const* target

    )
    {
        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
    }

    void   on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
    {
        if (ec)
            return fail(ec, "resolve");

        stream_.expires_after(std::chrono::seconds(30));

        stream_.async_connect(
            results,
            beast::bind_front_handler(
                &session::on_connect,
                shared_from_this()));
    }

    void  on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
    {
        if (ec)
            return fail(ec, "connect");


        stream_.expires_after(std::chrono::seconds(30));

        http::async_write(stream_, req_,
            beast::bind_front_handler(
                &session::on_write,
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
                &session::on_read,
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


        stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (ec && ec != beast::errc::not_connected)
            return fail(ec, "shutdown");

    }

private:
    void fail(beast::error_code ec, char const* what)
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