#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include <memory>
#include <vector>
#include <iostream>
#include <functional>
#include <map>

using boost::asio::io_context;

namespace dndello 
{
    namespace server
    {
        class Panel;
        class Server;
        class Session;

        typedef void(&view_func)(Session&,std::string);
        typedef std::_Binder<struct std::_Unforced, view_func,const std::_Ph<1> &,std::string> binded_view;

        void example_view_func(Session& _session, std::string param);
        void view_one_file(Session& _session, std::string param);

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

                void proceed();

                void send()
                {
                    unsigned int sended_bytes = boost::beast::http::write(socket, response, ec);
                };
                
                void close()
                {
                    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                };

                boost::beast::http::request<boost::beast::http::string_body> request;
                boost::beast::http::response<boost::beast::http::string_body> response;
                std::shared_ptr<Server> server;

            private:

                boost::asio::ip::tcp::socket socket;
                boost::system::error_code ec;
                boost::beast::flat_buffer buffer;
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

                void add_route(std::string path, view_func func, std::string param)
                {
                    binded_view view = std::bind(func, std::placeholders::_1, std::move(param));
                    routes.insert(
                        std::move(
                            std::pair<std::string, binded_view>(
                                path, std::move(view)
                            )
                        )
                    );
                };

                std::map<std::string, binded_view> routes {};
            
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

        void example_view_func(Session& _session, std::string param)
        {
            std::cout << _session.request.target().to_string() << std::endl;
            std::cout << param << std::endl;
            std::string body {""};
            body.append(
                "<!DOCTYPE html>"
                "<html><head><meta charset=\"utf-8\"></head>"
                "<body>"
                "<h1>Это образец функции обработчика входящего запроса</h1>"
                "<h3>Маршрутов на сервере: "
            );
            body.append(std::to_string(_session.server->routes.size()));
            body.append(
                "</h3>"
            );
            body.append("<h4>");
            body.append(param);
            body.append("</h4>");
            body.append(
                "</body></html>"
            );
            _session.response.body() = body;
        };

        void view_one_file(Session& _session, std::string param)
        {

        };

        void Session::proceed()
        {
            auto view = server->routes.find(request.target().to_string());
            if (view != server->routes.end())
                view->second(*this);
            else
                example_view_func(*this, request.target().to_string());
        };
    };
};