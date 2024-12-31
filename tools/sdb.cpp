#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <libsdb/process.hpp>
#include <libsdb/error.hpp>

namespace {
    std::vector<std::string> split(std::string_view str, char delimiter) {
        std::vector<std::string> out{};
        std::stringstream ss {std::string{str}};
        std::string item;

        while(std::getline(ss, item, delimiter)) {
            out.push_back(item);
        }

        return out;
    }

    bool is_prefix(std::string_view str, std::string_view of) {
        if(str.size() > of.size()) {
            return false;
        }

        // check if "str" is a prefix of "of"
        return std::equal(str.begin(), str.end(), of.begin());
    }

    void resume(pid_t pid) {
        // continue executing the process
        if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) < 0) {
            std::cerr << "Couldnt continue\n";
            std::exit(-1);
        }
    }

    void wait_on_signal(pid_t pid) {
        int wait_status;
        int options = 0;
        if(waitpid(pid, &wait_status, options) < 0) {
            std::perror("waitpid failed");
            std::exit(-1);
        }
    }

    std::unique_ptr<sdb::process> attach(int argc, const char** argv) {
        pid_t pid = 0;
        if(argc == 3 && argv[1] == std::string_view("-p")) 
        { // passing the PID
            pid = std::atoi(argv[2]);
            return sdb::process::attach(pid);
        } else { // passing the program name
            const char *program_path = argv[1];
            return sdb::process::launch(program_path);            
        }
    }

    void print_stop_reason(const sdb::process& process, sdb::stop_reason reason) {
        std::cout << "Process " << process.pid() << ' ';

        switch(reason.reason) {
            case sdb::process_state::exited: 
                std::cout << "Exited with status " << static_cast<int>(reason.info);
                break;
            case sdb::process_state::terminated: 
                std::cout << "Terminated with signal " << sigabbrev_np(reason.info);
                break;
            case sdb::process_state::stopped: 
                std::cout << "Stopped with signal " << sigabbrev_np(reason.info);
                break;
        }

        std::cout << std::endl;
    }

    void handle_command(std::unique_ptr<sdb::process>& process, std::string_view line) {
        std::vector<std::string> args = split(line, ' ');
        std::string command = args[0];

        if(is_prefix(command, "continue")) {
            process->resume();
            auto reason = process->wait_on_signal();
            print_stop_reason(*process, reason);
        } else {
            std::cerr << "Unknown command\n";
        }
    }

    void main_loop(std::unique_ptr<sdb::process>& process) {
        
        char *line = nullptr;
        while((line = readline("sdb> ")) != nullptr) {
            std::string line_str;

            if(line == std::string_view("")) {
                free(line);
                if(history_length > 0) {
                    line_str = history_list()[history_length - 1]->line;
                }
            } else {
                line_str = line;
                add_history(line);
                free(line);
            }

            if(!line_str.empty()) {
                try {
                    handle_command(process, line_str);
                } catch(const sdb::error& err) {
                    std::cout << err.what() << std::endl;
                }
            }
        }
    }

};


int main(int argc, const char **argv) {
    if(argc == 1) {
        std::cerr << "No arguments given\n";
        return -1;
    }

    try {
        std::unique_ptr<sdb::process> process = attach(argc, argv);
        main_loop(process);
    } catch(const sdb::error& err) {
        std::cout << err.what() << std::endl;
    }

    
}