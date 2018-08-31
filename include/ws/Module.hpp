#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
#include <functional>
#include <vector>
#include <utility>


namespace ws {
    template<typename... Ts>
    void print(Ts&&... args) {
        (std::cerr << ... << std::forward<Ts&&>(args) );
    }



    template<typename... Ts>
    void pipe(Ts&&... args) {
        (std::cout << ... << std::forward<Ts&&>(args));
    }



    template<typename... Ts>
    void println(Ts&&... args) {
        (std::cerr << ... << std::forward<Ts&&>(args)) << std::endl;
    }



    template<typename... Ts>
    void pipeln(Ts&&... args) {
        (std::cout << ... << std::forward<Ts&&>(args)) << std::endl;
    }



    class Receiver {
        private:
            uintmax_t       buffer_size;
            std::string     buffer;


        public:
            Receiver(uintmax_t buffer_size_):
                buffer_size(buffer_size_),
                buffer(buffer_size_, '\0')
            {

            }


            // Callback = (const std::string& buffer, int packet_id, bool is_end);
            void read(
                std::function<void(const std::string&, int, bool)> callback
            ) {
                int packet_id = 0;

                while (std::cin.read(&buffer[0], buffer_size)) {
                    callback(buffer, packet_id, false);
                    ++packet_id;
                }

                buffer.erase(buffer.begin() + std::cin.gcount(), buffer.end());
                callback(buffer, packet_id, true);
            }
    };


    namespace {
        enum {
            LOGGER_NORMAL,
            LOGGER_NOTICE,
            LOGGER_WARNING
        };
    }


    class Logger {
        private:
            std::vector<std::string> symbols = {
                "[-] ",  // Normal
                "[*] ",  // Notice
                "[!] "   // Warning
            };


        public:
            Logger() {}


            void print(const std::string& msg) {
                std::cerr << msg << '\n';
            }


            void log(const std::string& msg) {
                std::cerr << symbols[LOGGER_NORMAL] << msg << '\n';
            }


            void notice(const std::string& msg) {
                std::cerr << symbols[LOGGER_NOTICE] << msg << '\n';
            }


            void warn(const std::string& msg) {
                std::cerr << symbols[LOGGER_WARNING] << msg << '\n';
            }
    };
}
