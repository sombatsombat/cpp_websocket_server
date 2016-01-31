#include "server_ws.hpp"
#include "client_ws.hpp"

using namespace std;

typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
typedef SimpleWeb::SocketClient<SimpleWeb::WS> WsClient;

int main() {
    //WebSocket (WS)-server at port 8080 using 4 threads
    WsServer server(8080, 4);
    
    //Example 1: echo WebSocket endpoint
    //  Added debug messages for example use of the callbacks
    //  Test with the following JavaScript:
    //    var ws=new WebSocket("ws://localhost:8080/echo");
    //    ws.onmessage=function(evt){console.log(evt.data);};
    //    ws.send("test");
    auto& echo=server.endpoint["^/IR_decode/?$"];
    
    echo.onmessage=[&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
        //WsServer::Message::string() is a convenience function for:
        /*
        cout << "I am here1" << endl;
        stringstream data_ss;
        cout << "I am here2" << endl;
        data_ss << message->rdbuf();
        cout << data_ss << endl;
        cout << "I am here3" << endl;
        cout << data_ss.str() << endl;
        */
        size_t size;
        size = message->size();        
        cout << "Stream size = " << size << endl;
        
        unsigned char adata[size];
        memcpy ( adata, (message->string()).c_str(), size);
        
        for(int i=0;i<size;i++){
            printf("%02X ", adata[i]);
        }
        cout << endl;
        /*
        return;
        */
        //auto message_str = data_ss.str();
        auto message_str=message->string();
        
        cout << "Server: Message received: \"" << message_str << "\" from " << (size_t)connection.get() << endl;
                
        cout << "Server: Sending message \"" << message_str <<  "\" to " << (size_t)connection.get() << endl;
        
        auto send_stream=make_shared<WsServer::SendStream>();
        *send_stream << message_str;
        //server.send is an asynchronous function
        server.send(connection, send_stream, [](const boost::system::error_code& ec){
            if(ec) {
                cout << "Server: Error sending message. " <<
                //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
                        "Error: " << ec << ", error message: " << ec.message() << endl;
            }
        });
    };
    
    echo.onopen=[](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << (size_t)connection.get() << endl;
    };
    
    //See RFC 6455 7.4.1. for status codes
    echo.onclose=[](shared_ptr<WsServer::Connection> connection, int status, const string& reason) {
        cout << "Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
    };
    
    //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    echo.onerror=[](shared_ptr<WsServer::Connection> connection, const boost::system::error_code& ec) {
        cout << "Server: Error in connection " << (size_t)connection.get() << ". " << 
                "Error: " << ec << ", error message: " << ec.message() << endl;
    };
    
    //Example 2: Echo thrice
    //  Send a received message three times back to the client
    //  Test with the following JavaScript:
    //    var ws=new WebSocket("ws://localhost:8080/echo_thrice");
    //    ws.onmessage=function(evt){console.log(evt.data);};
    //    ws.send("test");
    auto& echo_thrice=server.endpoint["^/echo_thrice/?$"];
    echo_thrice.onmessage=[&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
        auto message_str=message->string();
        
        auto send_stream1=make_shared<WsServer::SendStream>();
        *send_stream1 << message_str;
        //server.send is an asynchronous function
        server.send(connection, send_stream1, [&server, connection, message_str](const boost::system::error_code& ec) {
            if(!ec) {
                auto send_stream3=make_shared<WsServer::SendStream>();
                *send_stream3 << message_str;
                server.send(connection, send_stream3); //Sent after send_stream1 is sent, and most likely after send_stream2
            }
        });
        //Do not reuse send_stream1 here as it most likely is not sent yet
        auto send_stream2=make_shared<WsServer::SendStream>();
        *send_stream2 << message_str;
        server.send(connection, send_stream2); //Most likely queued, and sent after send_stream1
    };

    //Example 3: Echo to all WebSocket endpoints
    //  Sending received messages to all connected clients
    //  Test with the following JavaScript on more than one browser windows:
    //    var ws=new WebSocket("ws://localhost:8080/echo_all");
    //    ws.onmessage=function(evt){console.log(evt.data);};
    //    ws.send("test");
    auto& echo_all=server.endpoint["^/echo_all/?$"];
    echo_all.onmessage=[&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
        auto message_str=message->string();
        
        //echo_all.get_connections() can also be used to solely receive connections on this endpoint
        for(auto a_connection: server.get_connections()) {
            auto send_stream=make_shared<WsServer::SendStream>();
            *send_stream << message_str;
            
            //server.send is an asynchronous function
            server.send(a_connection, send_stream);
        }
    };
    
    thread server_thread([&server](){
        //Start WS-server
        server.start();
    });
    server_thread.join();
    
    return 0;
}
