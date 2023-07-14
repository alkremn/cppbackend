#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>
#include <sstream>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using EmptyResponse = http::response<http::empty_body>;

struct ContentType
{
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
};

StringResponse MakeStringResponse(http::status status, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML)
{
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.keep_alive(keep_alive);
    return response;
}

EmptyResponse MakeEmptyResponse(http::status status, unsigned http_version,
                                bool keep_alive,
                                std::string_view content_type = ContentType::TEXT_HTML)
{
    EmptyResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleStringRequest(http::status status, const StringRequest &req)
{
    return MakeStringResponse(status, req.version(), req.keep_alive());
}

EmptyResponse HandleEmptyRequest(http::status status, const StringRequest &req)
{
    return MakeEmptyResponse(status, req.version(), req.keep_alive());
}

std::optional<StringRequest> ReadRequest(tcp::socket &socket, beast::flat_buffer &buffer)
{
    beast::error_code ec;
    StringRequest req;

    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream)
    {
        return std::nullopt;
    }
    if (ec)
    {
        throw std::runtime_error("Failed to read request: "s.append(ec.message()));
    }

    return req;
}

void HandleConnection(tcp::socket &socket)
{
    try
    {
        beast::flat_buffer buffer;

        while (auto request = ReadRequest(socket, buffer))
        {
            if (request->method() == http::verb::get)
            {
                StringResponse response = HandleStringRequest(http::status::ok, *request);
                std::stringstream ss;
                ss << "<strong>Hello "sv << request->target().substr(1) << "</strong>"sv;
                response.body() = ss.str();
                response.content_length(ss.str().size());
                http::write(socket, response);

                if (response.need_eof())
                {
                    break;
                }
            }
            else if (request->method() == http::verb::head)
            {
                EmptyResponse response = HandleEmptyRequest(http::status::ok, *request);
                response.content_length(0);
                http::write(socket, response);

                if (response.need_eof())
                {
                    break;
                }
            }
            else
            {
                StringResponse response = HandleStringRequest(http::status::method_not_allowed, *request);
                std::stringstream ss;
                ss << "Invalid method"sv;
                response.body() = ss.str();
                response.content_length(ss.str().size());
                http::write(socket, response);

                if (response.need_eof())
                {
                    break;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

int main()
{
    // Выведите строчку "Server has started...", когда сервер будет готов принимать подключения
    constexpr unsigned short port = 8080;

    net::io_context io_context;
    tcp::acceptor acceptor(io_context, {net::ip::make_address("0.0.0.0"sv), port});
    std::cout << "Server has started..." << std::endl;

    while (true)
    {
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        HandleConnection(socket);
    }
}
