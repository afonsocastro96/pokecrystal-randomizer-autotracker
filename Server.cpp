#include <set>
#include <iostream>

#include "Server.hpp"
#include "ItemRando.hpp"

typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> con_list;
static con_list m_connections;
static server echo_server;
static RandoState* state;

void shutdown_server(std::string message_to_clients) {
    echo_server.stop_listening();
    con_list::iterator it;
    for (it = m_connections.begin(); it != m_connections.end(); it++) {
        echo_server.pause_reading(*it);
        echo_server.close(*it, websocketpp::close::status::normal, message_to_clients);
    }
    echo_server.stop();
    std::cout << "WebSocket Server stopped. Closing..." << std::endl;
    system("pause");
}

void on_open(server* s, websocketpp::connection_hdl hdl) {
    m_connections.insert(hdl);
}

void on_close(server* s, websocketpp::connection_hdl hdl) {
    m_connections.erase(hdl);
}

// Define a callback to handle incoming messages
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    // check for a special command to instruct the server to stop listening so
    // it can be cleanly exited.
    if (msg->get_payload() == "{\"action\": \"stop\"}") {
        shutdown_server("Connection closed");
        exit(0);
    }

    try {
        std::string payload = msg->get_payload();
        std::transform(payload.begin(), payload.end(), payload.begin(),
            [](unsigned char c) { return std::tolower(c); });
        if (payload == "{\"action\": \"read\"}") {
            std::stringstream ss;
            ss << *state;
            s->send(hdl, ss.str(), msg->get_opcode());
            return;
        }
        else {
            // We can't recognize the message, echo it back to the client
            std::string response = "Unknown message: " + payload + '\n';
            s->send(hdl, response, msg->get_opcode());
        }
    }
    catch (websocketpp::exception const& e) {
        std::cout << e.what() << std::endl;
    }
}

void initialise_server(RandoState* rando_state) {
    state = rando_state;
    std::cout << "Starting Websocket server..." << std::endl;
    echo_server.clear_access_channels(websocketpp::log::alevel::all);
    echo_server.init_asio();
    echo_server.set_message_handler(bind(&on_message, &echo_server, ::_1, ::_2));
    echo_server.set_close_handler(bind(&on_close, &echo_server, ::_1));
    echo_server.set_open_handler(bind(&on_open, &echo_server, ::_1));

    echo_server.listen(DEFAULT_SERVER_PORT);
    echo_server.start_accept();
    std::cout << "Server successfully launched - listening on ws://localhost:" << DEFAULT_SERVER_PORT << std::endl << std::endl;
}

void run_server() {
    echo_server.run();
}