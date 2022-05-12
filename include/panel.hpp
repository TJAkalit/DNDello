#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include <memory>
#include <vector>

using boost::asio::io_context;

namespace dndello 
{
    namespace server
    {
        class Panel;
        class Server;

        class Session: public std::enable_shared_from_this<Session>
        {   
            public:
                Session(boost::asio::ip::tcp::socket _socket, std::shared_ptr<Server> _server):
                    socket(std::move(_socket)),
                    server(std::move(_server))
                {

                };

                void receive()
                {
                    unsigned int received_bytes = boost::beast::http::read(socket, buffer, request, ec);
                };

                void proceed()
                {
                    response.body() = std::string("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><body><h1>Дарова, друг!</h1></body></head></html>");
                    response.set("X-Exclusive-header", "123");
                };

                void send()
                {
                    unsigned int sended_bytes = boost::beast::http::write(socket, response, ec);
                };
                
                void close()
                {
                    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                };

            private:

                boost::asio::ip::tcp::socket socket;
                boost::system::error_code ec;
                boost::beast::flat_buffer buffer;
                boost::beast::http::request<boost::beast::http::string_body> request;
                boost::beast::http::response<boost::beast::http::string_body> response;
                std::shared_ptr<Server> server;
        };

        class Server: public std::enable_shared_from_this<Server>
        {
            public:

                Server(io_context& _context, std::shared_ptr<Panel> _panel, std::string _host, unsigned short _port):
                    context(_context),
                    panel(std::move(_panel)),
                    acceptor(context, {boost::asio::ip::address::from_string(_host), _port})
                {

                };

                void sync_run()
                {
                    boost::asio::ip::tcp::socket in{context};                    
                    while (true)
                    {
                        in = acceptor.accept();
                        Session s{std::move(in), std::move(shared_from_this())};
                        s.receive();
                        s.proceed();
                        s.send();
                        s.close();
                    };
                };
            
            private:

                io_context& context;
                std::shared_ptr<Panel> panel;
                boost::asio::ip::tcp::acceptor acceptor;
        };

        class Panel: public std::enable_shared_from_this<Panel>
        {
            public:

                Panel(int threads):
                    context(threads)
                {

                };

                io_context& get_context()
                {
                    return context;
                };

                std::shared_ptr<Server> make_server(std::string host, unsigned short port)
                {
                    std::shared_ptr<Server> new_server = std::make_shared<Server>(context, shared_from_this(), host, port);
                    servers.emplace_back(new_server->shared_from_this());
                    return new_server->shared_from_this();
                };
            
            private:

                io_context context;
                std::vector<std::shared_ptr<Server>> servers {};
        };
    };
};