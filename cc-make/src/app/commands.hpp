#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ccmake {

// Context passed to command handlers
struct CommandContext {
    // Leave as opaque for now - will be populated in Phase 12.4
    std::string args;     // Arguments after command name
    void* engine = nullptr;   // QueryEngine* (opaque for header)
    void* repl = nullptr;     // Repl* (opaque for header)
};

// Interface for a command
class Command {
public:
    virtual ~Command() = default;
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual std::vector<std::string> aliases() const { return {}; }
    virtual bool execute(const CommandContext& ctx) = 0;
    virtual bool needs_engine() const { return false; }
};

// Helper to create simple commands from lambdas
class SimpleCommand : public Command {
public:
    SimpleCommand(std::string name, std::string description,
                  std::vector<std::string> aliases,
                  std::function<bool(const CommandContext&)> handler,
                  bool needs_engine = false);
    std::string name() const override;
    std::string description() const override;
    std::vector<std::string> aliases() const override;
    bool execute(const CommandContext& ctx) override;
    bool needs_engine() const override;

private:
    std::string name_;
    std::string description_;
    std::vector<std::string> aliases_;
    std::function<bool(const CommandContext&)> handler_;
    bool needs_engine_;
};

// Registry for commands
class CommandRegistry {
public:
    void register_command(std::shared_ptr<Command> cmd);

    // Find command by name or alias
    Command* find(const std::string& name) const;
    bool has(const std::string& name) const;

    // List all registered command names
    std::vector<std::string> all_names() const;

    // Execute a command string like "/compact some args"
    // Returns true if command was found and executed
    bool execute(const std::string& input, const CommandContext& ctx);

    // Remove a command
    void remove(const std::string& name);

    size_t size() const;

private:
    std::unordered_map<std::string, std::shared_ptr<Command>> commands_;
};

// Create and return all built-in commands
std::vector<std::shared_ptr<Command>> create_default_commands();

}  // namespace ccmake
