#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include <memory>
#include <vector>
#include <iostream>
#include <functional>
#include <map>
#include <fstream>

using boost::asio::io_context;
using boost::asio::ip::tcp;
using std::shared_ptr;
using std::string;
using std::cout;
using std::endl;
using std::move;

namespace dndello 
{
    namespace server
    {
        class Panel;
        class Server;
        class Session;

        typedef void(&view_func)(Session&,string);
        typedef std::_Binder<struct std::_Unforced, view_func,const std::_Ph<1> &,string> binded_view;

        void example_view_func(Session& _session, string param);
        void static_file_view(dndello::server::Session& session, std::string param);

        class Session: public std::enable_shared_from_this<Session>
        {   
            public:
                Session(tcp::socket _socket, shared_ptr<Server> _server):
                    socket(move(_socket)),
                    server(move(_server))
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

                tcp::socket socket;
                boost::system::error_code ec;
                boost::beast::flat_buffer buffer;
        };

        class Server: public std::enable_shared_from_this<Server>
        {
            public:

                Server(io_context& _context, shared_ptr<Panel> _panel, string _host, unsigned short _port):
                    context(_context),
                    panel(move(_panel)),
                    acceptor(context, {boost::asio::ip::address::from_string(_host), _port})
                {
                    
                };

                void sync_run()
                {
                    tcp::socket in{context};                    
                    while (true)
                    {
                        in = acceptor.accept();
                        Session s{move(in), move(shared_from_this())};
                        s.receive();
                        s.proceed();
                        s.send();
                        s.close();
                    };
                };

                void add_route(string path, view_func func, string param)
                {
                    binded_view view = std::bind(func, std::placeholders::_1, move(param));
                    routes.insert(
                        move(
                            std::pair<string, binded_view>(
                                path, move(view)
                            )
                        )
                    );
                };

                std::map<string, binded_view> routes {};
            
            private:

                io_context& context;
                std::shared_ptr<Panel> panel;
                tcp::acceptor acceptor;
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

                shared_ptr<Server> make_server(string host, unsigned short port)
                {
                    shared_ptr<Server> new_server = std::make_shared<Server>(context, shared_from_this(), host, port);
                    servers.emplace_back(new_server->shared_from_this());
                    return new_server->shared_from_this();
                };
            
            private:

                io_context context;
                std::vector<shared_ptr<Server>> servers {};
        };

        void example_view_func(Session& _session, string param)
        {
            cout << _session.request.target().to_string() << endl;
            cout << param << endl;
            string body {""};
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

        void Session::proceed()
        {
            auto view = server->routes.find(request.target().to_string());
            if (view != server->routes.end())
                view->second(*this);
            else
                example_view_func(*this, request.target().to_string());
        };

        void static_file_view(dndello::server::Session& session, std::string param)
        {
            std::fstream file;
            unsigned int realsize;
            unsigned int max_size = 10240;

            file.open(param, std::ios::in|std::ios::binary);
            if (!file.is_open())
            {
                session.response.body() = std::string("Not found");
                session.response.result(404);
                return;
            };

            file.seekg(0, std::ios::end);
            realsize = file.tellg();
            std::cout << realsize << " " << max_size << std::endl;
            if (realsize > max_size)
            {
                session.response.body() = std::string("Large file");
                session.response.result(501);
                file.close();
                return;
            };
            file.seekg(0);
            
            char buffer;

            while (file.tellg() != realsize)
            {
                file.read(&buffer, 1);
                session.response.body().append(1, buffer);
            };
            return;
        };
    };
};