#include <boost/asio.hpp>
#include <thread>
#include <plog/Log.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <array>
#include <cstring>

static constexpr uint32_t ADDR_BUF_SIZE = 128;

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
            _client_socket(client_socket)
        { }

        void start() {
            start_read_from_client();
        }

    private:
        void start_read_from_client() {
            auto self = shared_from_this();

            _client_socket->async_read_some(boost::asio::buffer(_client_buffer), 
                [this, self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                    if (!err) {
                        PLOG_VERBOSE << "Reading from client was success, bytes: " << std::string(_client_buffer.data(), bytes_transferred);
                        start_write_to_client(bytes_transferred);
                    } else if (err == boost::asio::error::eof) {
                        PLOG_INFO << "Client disconnected.";
                    } else {
                        PLOG_ERROR << "Read from client error: " << err.message();
                    }
                });
        }

        void start_write_to_client(std::size_t bytes_transferred) {
            auto self = shared_from_this();

            boost::asio::async_write(*_client_socket, boost::asio::buffer(_client_buffer, bytes_transferred),
                [this, self](const boost::system::error_code& err, std::size_t) {
                    if (!err) {
                        PLOG_VERBOSE << "Write to client success";
                        start_read_from_client();
                    } else {
                        PLOG_ERROR << "Write to client error: " << err.message();
                    }
                });
        }

        boost::asio::io_context& _io_context;
        std::shared_ptr<tcp::socket> _client_socket;
        std::array<char, 8192> _client_buffer;
    };
};

int main(int argc, char** argv) {
    plog::init(plog::debug, &consoleAppender);

    try {
        // Default values
        char server_hostname[ADDR_BUF_SIZE] = "0.0.0.0";
        uint32_t port = 25565;

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
