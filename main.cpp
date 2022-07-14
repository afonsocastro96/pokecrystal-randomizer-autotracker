#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "ItemRando.hpp"

#define UPDATES_PER_SECOND 16
#define SECONDS_BETWEEN_PRINTS_TO_FILE 10
#define DEFAULT_SERVER_PORT 9002

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

// The game state
RandoState* state;


// Define a callback to handle incoming messages
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    // check for a special command to instruct the server to stop listening so
    // it can be cleanly exited.
    if (msg->get_payload() == "{\"action\": \"stop\"}") {
        s->pause_reading(hdl);
        s->stop_listening();
        s->close(hdl, websocketpp::close::status::normal, "Closing server.");
        return;
    }

    try {
        std::string payload = msg->get_payload();
        std::transform(payload.begin(), payload.end(), payload.begin(),
            [](unsigned char c) { return std::tolower(c); });
        if (payload == "{\"action\": \"read\"}") {
            std::stringstream ss;
            ss << *state;
            s->send(hdl, ss.str(), msg->get_opcode());
        }
    }
    catch (websocketpp::exception const& e) {
        std::cout << "Echo failed because: "
            << "(" << e.what() << ")" << std::endl;
    }
}

void update_game_state() {
    std::cout << "WRAM sniffer module successfully launched." << std::endl;
    while (true) {
        state->updateStatus();
        Sleep(1000 / UPDATES_PER_SECOND);
    }

}

int main(int argc, char* argv) { 
    std::wcout << "Welcome to Sceptross' Pokémon Crystal Rando Auto-tracker" << std::endl
        << "You can quit the auto-tracker at any point using (CTRL+C)." << std::endl << 
        "Remember that doing so may have unintended side-effects on the tracker you are using." << std::endl << std::endl;

    std::cout << "Note: Debug mode is ON." << std::endl << std::endl;
  
    
    try {
        state = new RandoState();
        std::thread state_updater_thread(update_game_state);

        // Create a server endpoint
        server echo_server;
        std::cout << "Starting Websocket server..." << std::endl;
        // Set logging settings
        echo_server.clear_access_channels(websocketpp::log::alevel::all);
        // Initialize Asio
        echo_server.init_asio();
        // Register our message handler
        echo_server.set_message_handler(bind(&on_message, &echo_server, ::_1, ::_2));
        // Listen on the defined port
        echo_server.listen(DEFAULT_SERVER_PORT);
        // Start the server accept loop
        echo_server.start_accept();
        std::cout << "Server successfully launched - listening on ws://localhost:" << DEFAULT_SERVER_PORT << std::endl << std::endl;

        // Start the ASIO io_service run loop
        echo_server.run();

        state_updater_thread.join();
    }
    catch (websocketpp::exception const& e) {
        std::cout << e.what() << std::endl;
        system("pause");
    }
    catch (std::runtime_error& e) {
        std::cout << "Runtime Error: " << e.what() << std::endl;
        system("pause");
        return -1;
    }    
}
   