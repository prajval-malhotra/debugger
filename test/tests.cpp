#include <catch2/catch_test_macros.hpp>
#include <libsdb/process.hpp>
#include <libsdb/error.hpp>
#include <sys/types.h>
#include <signal.h>

using namespace sdb;

namespace {
    bool process_exists(pid_t pid) {
        auto ret = kill(pid, 0);
        return ret != -1 && errno != ESRCH;
    }

    char get_process_status(pid_t pid) {
        std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
        std::string data;
    }
}

TEST_CASE("process::launch success", "[process]") {
    auto proc = process::launch("yes");
    REQUIRE(process_exists(proc->pid()));
}

TEST_CASE("process::launch no such program", "[process]") {
    REQUIRE_THROWS_AS(process::launch("not_a_real_program"), error);
}