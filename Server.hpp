#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

class RandoState;

#define UPDATES_PER_SECOND 16
#define DEFAULT_SERVER_PORT 9002

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

extern RandoState* rando_state;

void initialise_server(RandoState* rando_state);
void run_server();
void shutdown_server(std::string message_to_clients);