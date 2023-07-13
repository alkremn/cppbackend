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

struct ContentType
{
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
};

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML)
{
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest &&req)
{

    if (req.method() == http::verb::get)
    {
        std::stringstream ss;
        ss << "<strong>Hello "sv << req.target() << "</strong>"sv;
        [&req](http::status status, std::string_view text)
        {
            return MakeStringResponse(status, text, req.version(), req.keep_alive());
        }(http::status::ok, ss.str());
    }
    return {};
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

template <typename RequestHandler>
void HandleConnection(tcp::socket &socket, RequestHandler &&handle_request)
{
    try
    {
        beast::flat_buffer buffer;

        while (auto request = ReadRequest(socket, buffer))
        {
            StringResponse response = handle_request(std::move(request));
            http::write(socket, response);
            if (response.need_eof())
            {
                break;
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
    }

    // HandleConnection()
}
