#ifndef SDB_PROCESS_HPP
#define SDB_PROCESS_HPP

#include <filesystem>
#include <memory>
#include <sys/types.h>

namespace sdb {

    enum class process_state {
        stopped,
        running,
        exited,
        terminated
    };

    // hold reason for stop, value of exit, signal that caused stop
    struct stop_reason {
        stop_reason(int wait_status);
        process_state reason;
        std::uint8_t info;
    };
    

    class process {
    public:
        static std::unique_ptr<process> launch(std::filesystem::path path);
        static std::unique_ptr<process> attach(pid_t pid);

        void resume();
        sdb::stop_reason wait_on_signal();

        pid_t pid() const { return pid_; }

        /** users shouldnt be able to directly create or copy process objs */
        process() = delete;
        process& operator=(const process&) = delete;

        process_state state_ = process_state::stopped;

        ~process();

    private:
        pid_t pid_ = 0;
        bool terminate_on_end = true;

        process(pid_t pid, bool terminate_on_end) : pid_(pid), terminate_on_end(terminate_on_end) {}

    };
}

#endif