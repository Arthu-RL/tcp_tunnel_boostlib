#include <boost/asio.hpp>
#include <thread>

#include <plog/Log.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

#define ADDR_BUF_SIZE 128

static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;

using boost::asio::ip::tcp;

/*
* @brief This class provides tcp tunnel that can be used if port fowarding
*
* Usage
* @example TcpTunnel tunnel(io_context, server_endpoint, client_endpoint);
*
* Compiling and Running
* @example g++ -g -Wall -Wextra -o tcp_tunnel tcp_tunnel.cpp -lboost_system
* @example ./tcp_tunnel "192.168.42.56" "0.0.0.0" "25565"
*/

class TcpTunnel {
  public:
    TcpTunnel(boost::asio::io_context &io_context,
              const tcp::endpoint &server_endpoint,
              const tcp::endpoint &client_endpoint) : 
      _io_context(io_context), 
      _server_endpoint(server_endpoint),
      _acceptor(io_context, client_endpoint) 
    {
      start_accept();
    }


  private:
    void start_accept() {
      std::shared_ptr<tcp::socket> tcp_socket = std::make_shared<tcp::socket>(_io_context);

      _acceptor.async_accept(*tcp_socket, [this, tcp_socket](const boost::system::error_code &err) {
        if (!err) {
          PLOG_VERBOSE << "Connection accepted, creating a tcp session...";
          std::make_shared<TcpTunnel::TcpSession>(this->_io_context, tcp_socket, this->_server_endpoint)->start_session();
        } else {
          PLOG_ERROR << "Accept error: " << err.message();
        }
        
        start_accept();
      });
    }


    boost::asio::io_context &_io_context;
    tcp::endpoint _server_endpoint;
    tcp::acceptor _acceptor;


    class TcpSession : public std::enable_shared_from_this<TcpSession> {
      public:
        TcpSession(boost::asio::io_context &io_context,
                  std::shared_ptr<tcp::socket> client_socket,
                  const tcp::endpoint &server_endpoint) : 
                _io_context(io_context),
                _client_socket(client_socket),
                _server_endpoint(server_endpoint),
                _server_socket(io_context) { }


        void start_session() {
          PLOG_INFO << "Attempting to connect session to server at " << _server_endpoint;
          _server_socket.async_connect(_server_endpoint, [this](const boost::system::error_code &err) {
            if (!err) {
                PLOG_VERBOSE << "Connection with server created, start reading from server/client operations...";
                start_read_from_client();
                start_read_from_server();
            } else {
                PLOG_ERROR << "Connect error: " << err.message();
            }
          });
        }

        
      private:
        void start_read_from_client() {
          std::shared_ptr<TcpTunnel::TcpSession> self = shared_from_this();

          _client_socket->async_read_some(boost::asio::buffer(_client_buffer), 
            [this, self](const boost::system::error_code &err, std::size_t bytes_transferred) {
              if (!err) {
                PLOG_VERBOSE << "Reading from client was success, bytes: " << std::string(this->_client_buffer.data(), bytes_transferred);
                boost::asio::async_write(
                    this->_server_socket, 
                    boost::asio::buffer(this->_client_buffer, bytes_transferred),
                    [this, self](const boost::system::error_code &err, std::size_t bytes_transferred) {
                      if (!err) {
                        PLOG_VERBOSE << "Write to server success, bytes: " << std::string(this->_client_buffer.data(), bytes_transferred);
                        start_read_from_client();
                      } else {
                        PLOG_ERROR << "Write to server error: " << err.message();
                      }
                    });
              } else {
                PLOG_ERROR << "Read from client error: " << err.message();
              }
            });        
        }


        void start_read_from_server() {
          std::shared_ptr<TcpTunnel::TcpSession> self = shared_from_this();
          
          _server_socket.async_read_some(boost::asio::buffer(_server_buffer),
            [this, self](const boost::system::error_code &err, std::size_t bytes_transferred) {
              if (!err) {
                PLOG_VERBOSE << "Reading from server was success, bytes: " << std::string(this->_server_buffer.data(), bytes_transferred);
                boost::asio::async_write(
                  *this->_client_socket,
                  boost::asio::buffer(this->_server_buffer, bytes_transferred),
                  [this, self](const boost::system::error_code &err, std::size_t bytes_transferred) {
                    if (!err) {
                      PLOG_VERBOSE << "Write to client success, bytes: " << std::string(this->_server_buffer.data(), bytes_transferred);
                      start_read_from_server();
                    } else {
                      PLOG_ERROR << "Write to client error: " << err.message();
                    }
                  });
              } else {
                PLOG_ERROR << "Read from server error: " << err.message();
              }
            });
        }


        boost::asio::io_context &_io_context;
        std::shared_ptr<tcp::socket> _client_socket;
        tcp::endpoint _server_endpoint;
        tcp::socket _server_socket;
        std::array<char, 8192> _client_buffer;
        std::array<char, 8192> _server_buffer;
    };
};


int main(int argc, char **argv) {
  plog::init(plog::debug, &consoleAppender);

  try
  {
    // Default values
    char server_hostname[ADDR_BUF_SIZE] = "127.0.0.1";  // Remote server address
    char client_addr[ADDR_BUF_SIZE] = "0.0.0.0";  // Local endpoint for accepting connections
    uint32_t port = 25565; // Minecraft port
    
    if (argc == 4) {
      memset(server_hostname, 0, ADDR_BUF_SIZE);
      memset(client_addr, 0, ADDR_BUF_SIZE);

      strcpy(server_hostname, argv[1]);
      strcpy(client_addr, argv[2]);
      port = static_cast<uint32_t>(std::stoi(argv[3]));
    }

    boost::asio::io_context io_context;

    tcp::endpoint server_endpoint(boost::asio::ip::make_address(server_hostname), port);
    tcp::endpoint client_endpoint(boost::asio::ip::make_address(client_addr), port);

    PLOG_INFO << "Server listening on " << server_hostname << ":" << port;

    TcpTunnel tunnel(io_context, server_endpoint, client_endpoint);

    std::thread t([&io_context]() {
      io_context.run();
    });
    t.join();
  }
  catch(const std::exception& e)
  {
    PLOG_ERROR << "Exception: " << e.what();
  }
  catch(...)
  {
    PLOG_ERROR << "Unhandled exception.";
  }

  return 0;
}

