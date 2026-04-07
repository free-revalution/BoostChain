#include <catch2/catch_test_macros.hpp>

#include "app/commands.hpp"

#include <sstream>

using namespace ccmake;

// --- SimpleCommand tests ---

TEST_CASE("SimpleCommand name and description") {
    SimpleCommand cmd("test", "A test command", {}, [](const CommandContext&) {
        return true;
    });
    REQUIRE(cmd.name() == "test");
    REQUIRE(cmd.description() == "A test command");
}

TEST_CASE("SimpleCommand with aliases") {
    SimpleCommand cmd("test", "A test command", {"t", "tst"}, [](const CommandContext&) {
        return true;
    });
    auto aliases = cmd.aliases();
    REQUIRE(aliases.size() == 2);
    REQUIRE(aliases[0] == "t");
    REQUIRE(aliases[1] == "tst");
}

TEST_CASE("SimpleCommand handler called") {
    bool called = false;
    std::string received_args;
    SimpleCommand cmd("test", "A test command", {},
                      [&](const CommandContext& ctx) -> bool {
                          called = true;
                          received_args = ctx.args;
                          return true;
                      });

    CommandContext ctx;
    ctx.args = "hello world";
    bool result = cmd.execute(ctx);
    REQUIRE(called);
    REQUIRE(received_args == "hello world");
    REQUIRE(result == true);
}

TEST_CASE("SimpleCommand needs_engine") {
    SimpleCommand cmd_no_engine("test", "test", {}, [](const CommandContext&) {
        return true;
    });
    REQUIRE(cmd_no_engine.needs_engine() == false);

    SimpleCommand cmd_with_engine("test", "test", {}, [](const CommandContext&) {
        return true;
    }, /*needs_engine=*/true);
    REQUIRE(cmd_with_engine.needs_engine() == true);
}

// --- CommandRegistry tests ---

TEST_CASE("CommandRegistry register and find") {
    CommandRegistry registry;
    auto cmd = std::make_shared<SimpleCommand>(
        "greet", "Greet the user", std::vector<std::string>{},
        [](const CommandContext&) { return true; });
    registry.register_command(cmd);

    REQUIRE(registry.find("greet") != nullptr);
    REQUIRE(registry.find("greet")->name() == "greet");
}

TEST_CASE("CommandRegistry find by alias") {
    CommandRegistry registry;
    auto cmd = std::make_shared<SimpleCommand>(
        "greet", "Greet the user", std::vector<std::string>{"hi", "hello"},
        [](const CommandContext&) { return true; });
    registry.register_command(cmd);

    REQUIRE(registry.find("hi") != nullptr);
    REQUIRE(registry.find("hello") != nullptr);
    REQUIRE(registry.find("hi")->name() == "greet");
}

TEST_CASE("CommandRegistry has returns true for registered") {
    CommandRegistry registry;
    auto cmd = std::make_shared<SimpleCommand>(
        "greet", "Greet the user", std::vector<std::string>{},
        [](const CommandContext&) { return true; });
    registry.register_command(cmd);

    REQUIRE(registry.has("greet") == true);
}

TEST_CASE("CommandRegistry has returns false for unknown") {
    CommandRegistry registry;
    REQUIRE(registry.has("unknown") == false);
}

TEST_CASE("CommandRegistry all_names returns unique names") {
    CommandRegistry registry;
    auto cmd1 = std::make_shared<SimpleCommand>(
        "greet", "Greet the user", std::vector<std::string>{"hi"},
        [](const CommandContext&) { return true; });
    auto cmd2 = std::make_shared<SimpleCommand>(
        "farewell", "Say goodbye", std::vector<std::string>{"bye"},
        [](const CommandContext&) { return true; });
    registry.register_command(cmd1);
    registry.register_command(cmd2);

    auto names = registry.all_names();
    REQUIRE(names.size() == 2);
    bool has_greet = false, has_farewell = false;
    for (const auto& n : names) {
        if (n == "greet") has_greet = true;
        if (n == "farewell") has_farewell = true;
    }
    REQUIRE(has_greet);
    REQUIRE(has_farewell);
}

TEST_CASE("CommandRegistry size returns count") {
    CommandRegistry registry;
    REQUIRE(registry.size() == 0);

    auto cmd1 = std::make_shared<SimpleCommand>(
        "a", "A", std::vector<std::string>{"a1"},
        [](const CommandContext&) { return true; });
    auto cmd2 = std::make_shared<SimpleCommand>(
        "b", "B", std::vector<std::string>{},
        [](const CommandContext&) { return true; });
    registry.register_command(cmd1);
    registry.register_command(cmd2);

    REQUIRE(registry.size() == 2);
}

TEST_CASE("CommandRegistry remove command") {
    CommandRegistry registry;
    auto cmd = std::make_shared<SimpleCommand>(
        "greet", "Greet", std::vector<std::string>{},
        [](const CommandContext&) { return true; });
    registry.register_command(cmd);
    REQUIRE(registry.has("greet"));

    registry.remove("greet");
    REQUIRE_FALSE(registry.has("greet"));
}

TEST_CASE("CommandRegistry remove removes aliases too") {
    CommandRegistry registry;
    auto cmd = std::make_shared<SimpleCommand>(
        "greet", "Greet", std::vector<std::string>{"hi", "hello"},
        [](const CommandContext&) { return true; });
    registry.register_command(cmd);

    registry.remove("greet");
    REQUIRE_FALSE(registry.has("greet"));
    REQUIRE_FALSE(registry.has("hi"));
    REQUIRE_FALSE(registry.has("hello"));
}

TEST_CASE("CommandRegistry execute with slash prefix") {
    CommandRegistry registry;
    bool called = false;
    auto cmd = std::make_shared<SimpleCommand>(
        "greet", "Greet", std::vector<std::string>{},
        [&](const CommandContext&) -> bool {
            called = true;
            return true;
        });
    registry.register_command(cmd);

    bool result = registry.execute("/greet", {});
    REQUIRE(called);
    REQUIRE(result == true);
}

TEST_CASE("CommandRegistry execute without slash prefix") {
    CommandRegistry registry;
    bool called = false;
    auto cmd = std::make_shared<SimpleCommand>(
        "greet", "Greet", std::vector<std::string>{},
        [&](const CommandContext&) -> bool {
            called = true;
            return true;
        });
    registry.register_command(cmd);

    bool result = registry.execute("greet", {});
    REQUIRE(called);
    REQUIRE(result == true);
}

TEST_CASE("CommandRegistry execute passes args") {
    CommandRegistry registry;
    std::string received_args;
    auto cmd = std::make_shared<SimpleCommand>(
        "echo", "Echo", std::vector<std::string>{},
        [&](const CommandContext& ctx) -> bool {
            received_args = ctx.args;
            return true;
        });
    registry.register_command(cmd);

    registry.execute("/echo hello world", {});
    REQUIRE(received_args == "hello world");
}

TEST_CASE("CommandRegistry execute unknown returns false") {
    CommandRegistry registry;
    bool result = registry.execute("/unknown", {});
    REQUIRE(result == false);
}

TEST_CASE("CommandRegistry execute empty string returns false") {
    CommandRegistry registry;
    bool result = registry.execute("", {});
    REQUIRE(result == false);
}

TEST_CASE("CommandRegistry execute strips slash prefix") {
    CommandRegistry registry;
    auto cmd = std::make_shared<SimpleCommand>(
        "test", "Test", std::vector<std::string>{},
        [](const CommandContext&) { return true; });
    registry.register_command(cmd);

    REQUIRE(registry.execute("/test", {}) == true);
    REQUIRE(registry.execute("test", {}) == true);
}

TEST_CASE("CommandRegistry double register same name keeps latest") {
    CommandRegistry registry;
    bool first_called = false;
    bool second_called = false;

    auto cmd1 = std::make_shared<SimpleCommand>(
        "cmd", "First", std::vector<std::string>{},
        [&](const CommandContext&) -> bool {
            first_called = true;
            return true;
        });
    auto cmd2 = std::make_shared<SimpleCommand>(
        "cmd", "Second", std::vector<std::string>{},
        [&](const CommandContext&) -> bool {
            second_called = true;
            return true;
        });

    registry.register_command(cmd1);
    registry.register_command(cmd2);
    registry.execute("/cmd", {});

    REQUIRE_FALSE(first_called);
    REQUIRE(second_called);
    REQUIRE(registry.find("cmd")->description() == "Second");
}

// --- Default commands tests ---

TEST_CASE("create_default_commands returns expected count") {
    auto cmds = create_default_commands();
    REQUIRE(cmds.size() >= 20);
}

TEST_CASE("create_default_commands has help") {
    auto cmds = create_default_commands();
    bool found = false;
    for (const auto& cmd : cmds) {
        if (cmd->name() == "help") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("create_default_commands has quit") {
    auto cmds = create_default_commands();
    bool found = false;
    for (const auto& cmd : cmds) {
        if (cmd->name() == "quit") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("create_default_commands has compact") {
    auto cmds = create_default_commands();
    bool found = false;
    for (const auto& cmd : cmds) {
        if (cmd->name() == "compact") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("Default /help command executes successfully") {
    auto cmds = create_default_commands();
    CommandRegistry registry;
    for (const auto& cmd : cmds) {
        registry.register_command(cmd);
    }

    bool result = registry.execute("/help", {});
    REQUIRE(result == true);
}

TEST_CASE("Default /quit command returns false") {
    auto cmds = create_default_commands();
    CommandRegistry registry;
    for (const auto& cmd : cmds) {
        registry.register_command(cmd);
    }

    bool result = registry.execute("/quit", {});
    REQUIRE(result == false);
}

TEST_CASE("create_default_commands has tools") {
    auto cmds = create_default_commands();
    bool found = false;
    for (const auto& cmd : cmds) {
        if (cmd->name() == "tools") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("create_default_commands has diff") {
    auto cmds = create_default_commands();
    bool found = false;
    for (const auto& cmd : cmds) {
        if (cmd->name() == "diff") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("create_default_commands has status") {
    auto cmds = create_default_commands();
    bool found = false;
    for (const auto& cmd : cmds) {
        if (cmd->name() == "status") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("create_default_commands has theme") {
    auto cmds = create_default_commands();
    bool found = false;
    for (const auto& cmd : cmds) {
        if (cmd->name() == "theme") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("create_default_commands has permissions") {
    auto cmds = create_default_commands();
    bool found = false;
    for (const auto& cmd : cmds) {
        if (cmd->name() == "permissions") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("create_default_commands has memory") {
    auto cmds = create_default_commands();
    bool found = false;
    for (const auto& cmd : cmds) {
        if (cmd->name() == "memory") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("create_default_commands has bug") {
    auto cmds = create_default_commands();
    bool found = false;
    for (const auto& cmd : cmds) {
        if (cmd->name() == "bug") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("Default /diff command executes without engine") {
    auto cmds = create_default_commands();
    CommandRegistry registry;
    for (const auto& cmd : cmds) {
        registry.register_command(cmd);
    }
    bool result = registry.execute("/diff", {});
    REQUIRE(result == true);
}

TEST_CASE("Default /status command executes without engine") {
    auto cmds = create_default_commands();
    CommandRegistry registry;
    for (const auto& cmd : cmds) {
        registry.register_command(cmd);
    }
    bool result = registry.execute("/status", {});
    REQUIRE(result == true);
}

TEST_CASE("Default /memory command executes") {
    auto cmds = create_default_commands();
    CommandRegistry registry;
    for (const auto& cmd : cmds) {
        registry.register_command(cmd);
    }
    bool result = registry.execute("/memory", {});
    REQUIRE(result == true);
}

TEST_CASE("Default /bug command executes") {
    auto cmds = create_default_commands();
    CommandRegistry registry;
    for (const auto& cmd : cmds) {
        registry.register_command(cmd);
    }
    bool result = registry.execute("/bug", {});
    REQUIRE(result == true);
}

TEST_CASE("All commands have non-empty descriptions") {
    auto cmds = create_default_commands();
    for (const auto& cmd : cmds) {
        REQUIRE_FALSE(cmd->description().empty());
        REQUIRE_FALSE(cmd->name().empty());
    }
}
