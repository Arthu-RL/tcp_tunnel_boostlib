#include <boost/asio.hpp>
#include <thread>
#include <plog/Log.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <array>
#include <cstring>
#include "vars.h"

static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;

using boost::asio::ip::tcp;

/**
 * @brief This class provides a TCP tunnel that can be used for port forwarding.
 *
 * Usage
 * @example TcpTunnel tunnel(io_context, server_endpoint, client_endpoint);
 */
class TcpTunnel {
public:
    TcpTunnel(boost::asio::io_context& io_context,
        const tcp::endpoint& server_endpoint
        /* const tcp::endpoint& client_endpoint */) :
        _io_context(io_context),
        _acceptor(io_context, server_endpoint)
    {
        start_accept();
    }

private:
    void start_accept() {
        std::shared_ptr<tcp::socket> tcp_socket = std::make_shared<tcp::socket>(_io_context);

        _acceptor.async_accept(*tcp_socket, [this, tcp_socket](const boost::system::error_code& err) {
            if (!err) {
                PLOG_VERBOSE << "Connection accepted, creating a tcp session...";
                std::make_shared<TcpSession>(_io_context, tcp_socket)->start();
            }
            else {
                PLOG_ERROR << "Accept error: " << err.message();
            }

            start_accept();
        });
    }

    boost::asio::io_context& _io_context;
    tcp::acceptor _acceptor;

    class TcpSession : public std::enable_shared_from_this<TcpSession> {
    public:
        TcpSession(boost::asio::io_context& io_context,
            std::shared_ptr<tcp::socket> client_socket) :
            _io_context(io_context),
            _client_socket(client_socket),
            _server_socket(io_context)
        { }

        void start() {
            if (!connect_to_server()) return;

            PLOG_INFO << "Connected to server.";

            read_from_client();
            read_from_server();
        }

    private:
        bool connect_to_server() {
            tcp::resolver resolver(_io_context);

            auto endpoints = resolver.resolve(server_hostname, std::to_string(port));

            boost::system::error_code ec;
            boost::asio::connect(_server_socket, endpoints, ec);

            if (ec) {
                PLOG_ERROR << "Failed to connect to server: " << ec.message();
                return false;
            }

            return true;
        }

        void read_from_client() {
            std::shared_ptr<TcpTunnel::TcpSession> self = shared_from_this();
            _client_socket->async_read_some(boost::asio::buffer(_client_buffer), 
                [this, self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                    if (!err) {
                        PLOG_VERBOSE << "Client read was success, bytes: " << std::string(_client_buffer.data(), bytes_transferred);
                        write_to_server(bytes_transferred);
                    } else if (err == boost::asio::error::eof) {
                        PLOG_INFO << "Client disconnected.";
                    } else {
                        PLOG_ERROR << "Client read error: " << err.message();
                    }
                });
        }

        void write_to_server(std::size_t bytes_transferred) {
            std::shared_ptr<TcpTunnel::TcpSession> self = shared_from_this();
            boost::asio::async_write(_server_socket, boost::asio::buffer(_client_buffer, bytes_transferred),
                [this, self](const boost::system::error_code& err, std::size_t) {
                    if (!err) {
                        PLOG_VERBOSE << "Server write success";
                        read_from_client();
                    } else {
                        PLOG_ERROR << "Server write error: " << err.message();
                        close_sockets();
                    }
                });
        }

        void read_from_server() {
            std::shared_ptr<TcpTunnel::TcpSession> self = shared_from_this();
            _server_socket.async_read_some(boost::asio::buffer(_server_buffer), 
                [this, self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                    if (!err) {
                        PLOG_VERBOSE << "Server read was success, bytes: " << std::string(_server_buffer.data(), bytes_transferred);
                        write_to_client(bytes_transferred);
                    } else if (err == boost::asio::error::eof) {
                        PLOG_INFO << "Server disconnected.";
                    } else {
                        PLOG_ERROR << "Server read error: " << err.message();
                    }
                });
        }

        void write_to_client(std::size_t bytes_transferred) {
            std::shared_ptr<TcpTunnel::TcpSession> self = shared_from_this();
            boost::asio::async_write(*_client_socket, boost::asio::buffer(_server_buffer, bytes_transferred),
                [this, self](const boost::system::error_code& err, std::size_t) {
                    if (!err) {
                        PLOG_VERBOSE << "Write on client success";
                        read_from_server();
                    } else {
                        PLOG_ERROR << "Client write error: " << err.message();
                        close_sockets();
                    }
                });
        }

        void close_sockets() {
            if (_client_socket->is_open()) {
                _client_socket->close();
            }
            if (_server_socket.is_open()) {
                _server_socket.close();
            }
            PLOG_INFO << "Sockets closed.";
        }

        boost::asio::io_context& _io_context;
        std::shared_ptr<tcp::socket> _client_socket;
        tcp::socket _server_socket;
        std::array<char, BUFF_SIZE> _client_buffer;
        std::array<char, BUFF_SIZE> _server_buffer;
    };
};

int main(int argc, char** argv) {
    plog::init(plog::debug, &consoleAppender);

    try {
        if (argc == 3) {
            memset(server_hostname, 0, ADDR_BUF_SIZE);
            strncpy(server_hostname, argv[1], ADDR_BUF_SIZE - 1);
            port = static_cast<uint32_t>(std::stoi(argv[2]));
        }

        boost::asio::io_context io_context;

        tcp::endpoint server_endpoint(boost::asio::ip::make_address(server_hostname), port);

        PLOG_INFO << "Server listening on " << server_hostname << ":" << port;

        TcpTunnel tunnel(io_context, server_endpoint);

        std::thread t([&io_context]() {
            io_context.run();
        });
        t.join();
    }
    catch (const std::exception& e) {
        PLOG_ERROR << "Exception: " << e.what();
    }
    catch (...) {
        PLOG_ERROR << "Unhandled exception.";
    }

    return 0;
}
