#include <websocketpp/config/asio_no_tls.hpp>
#include <windows.h>
#include <iostream>

#include "Server.hpp"
#include "ItemRando.hpp"

// The game state
void update_game_state(RandoState* rando_state) {
    std::cout << "WRAM sniffer module successfully launched." << std::endl;
    while (true) {
        rando_state->updateStatus();
        Sleep(1000 / UPDATES_PER_SECOND);
    }
}

#ifdef _WIN32
BOOL WINAPI ctrlCHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        shutdown_server("User terminated the auto-tracker.");
        exit(0);
    }
    return TRUE;
}
#endif // _WIN32

int main(int argc, char* argv) { 
    std::wcout << "Welcome to Sceptross' Pokémon Crystal Rando Auto-tracker" << std::endl
        << "You can quit the auto-tracker at any point using (CTRL+C)." << std::endl << 
        "Remember that doing so may have unintended side-effects on the tracker you are using." << std::endl << std::endl;

    #ifdef DEBUG
    std::cout << "Note: Debug mode is ON." << std::endl << std::endl;
    #endif //DEBUG
  
    try {
        RandoState* rando_state = new RandoState();

        #ifdef _WIN32
            // Set CTRL+C handler to exit gracefully
            SetConsoleCtrlHandler(ctrlCHandler, TRUE);
        #endif // _WIN32

        std::thread state_updater_thread(update_game_state, rando_state);
        initialise_server(rando_state);

        // Start the ASIO io_service run loop
        run_server();
        state_updater_thread.join();
        return 0;
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
    catch (std::invalid_argument& e) {
        std::cout << "Invalid Argument: " << e.what() << std::endl;
        system("pause");
    }
}
   