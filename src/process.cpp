#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libsdb/process.hpp>
#include <libsdb/error.hpp>
#include <libsdb/pipe.hpp>

namespace {
    void exit_with_perror(sdb::pipe& channel, std::string const& prefix) {
        auto message = prefix + ": " + std::strerror(errno);
        channel.write(reinterpret_cast<std::byte*>(message.data()), message.size());
        exit(-1);
    }
}

sdb::stop_reason::stop_reason(int wait_status) {
    if(WIFEXITED(wait_status)) {
        reason = process_state::exited;
        info = WEXITSTATUS(wait_status);
    } else if(WIFSIGNALED(wait_status)) {
        reason = process_state::terminated;
        info = WTERMSIG(wait_status);
    } else if(WIFSTOPPED(wait_status)) {
        reason = process_state::stopped;
        info = WSTOPSIG(wait_status);
    }
}

std::unique_ptr<sdb::process> sdb::process::launch(std::filesystem::path path) {
    
    sdb::pipe channel(true);

    pid_t pid = fork();
    if(pid < 0) {
        error::send_errno("fork failed");
    }

    if(pid == 0) {
        channel.close_read();
        if(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
            exit_with_perror(channel, "Tracing failed");
        }

        if(execlp(path.c_str(), path.c_str(), nullptr) < 0) {
            exit_with_perror(channel, "exec failed");
        }
    }

    channel.close_write();
    auto data = channel.read();
    channel.close_read();

    if(data.size() > 0) {
        waitpid(pid, nullptr, 0);
        auto chars = reinterpret_cast<char *>(data.data());
        error::send(std::string(chars, chars + data.size()));
    }

    // want to terminate on end, as we create this new proc
    std::unique_ptr<process> proc(new process(pid, true));
    proc->wait_on_signal();

    return proc;
}

std::unique_ptr<sdb::process> sdb::process::attach(pid_t pid) {
    if(pid == 0) {
        error::send_errno("Invalid PID");
    }

    if(ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
        error::send_errno("Could not attach");
    }

    // do not terminate on child end, as we didnt create it
    std::unique_ptr<process> proc(new process(pid, false));
    proc->wait_on_signal();

    return proc;
}

void sdb::process::resume() {
    if(ptrace(PTRACE_CONT, pid_, nullptr, nullptr) < 0) {
        error::send_errno("Could not reume");
    }

    state_ = process_state::running;
}

sdb::stop_reason sdb::process::wait_on_signal() {
    int wait_status;
    int options = 0;
    if(waitpid(pid_, &wait_status, options) < 0) {
        error::send_errno("waitpid failed");
    }

    stop_reason reason(wait_status);
    state_ = reason.reason;
    return reason;
}

sdb::process::~process() {
    // if we have a valid PID when dtor runs, need to detach. For PTRACE_DETACH
    // to work, inferior must be stopped; send SIGSTOP if its currently running
    if(pid_ != 0) {
        int status;
        if(state_ == process_state::running) {
            kill(pid_, SIGSTOP);
            waitpid(pid_, &status, 0);
        }
        // detach from process and let it continue
        ptrace(PTRACE_DETACH, pid_, nullptr, nullptr);
        kill(pid_, SIGCONT);

        // kill process if necesarry
        if(terminate_on_end) {
            kill(pid_, SIGKILL);
            waitpid(pid_, &status, 0);
        }
    }

}