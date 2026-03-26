# BoostChain - C++ Agent Framework Design Document

**Date:** 2026-03-26
**Version:** 0.1.0
**Status:** Design

---

## Overview

BoostChain is a lightweight, high-performance C++ framework for building LLM-powered Agents, inspired by LangChain. It focuses on:

- **Lightweight**: Minimal dependencies, suitable for embedded/edge deployment
- **High Performance**: Zero-runtime overhead, compile-time optimization
- **Universal**: Supports all major LLM providers via unified interface
- **Production Ready**: Complete persistence, error handling, and concurrency

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        User Layer                            │
│  Chain / Agent / Memory / Tool - High-level APIs            │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────┐
│                      Orchestration Layer                     │
│  Chain Engine │ Agent Loop │ Executor                       │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────┐
│                      Abstraction Layer                       │
│  ILLMProvider │ IMemory │ ITool │ ISerializer              │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────┐
│                      Adapter Layer                          │
│  OpenAI │ Claude │ Gemini │ OpenAI Compatible │ Custom     │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────┐
│                      Infrastructure Layer                    │
│  HTTP │ JSON │ Logging │ Serialization │ Thread Pool       │
└─────────────────────────────────────────────────────────────┘
```

### Design Principles

- **Layered Decoupling**: Each layer independently testable and replaceable
- **Interface-Driven**: Core logic depends only on abstract interfaces
- **Zero Runtime Overhead**: Compile-time polymorphism (CRTP) + inlining
- **Static Library Deployment**: No runtime dependencies

---

## Core Components

### 1. Chain - Chaining Engine

```cpp
class Chain {
public:
    Chain& add_prompt(const std::string& template_str);
    Chain& add_llm(std::shared_ptr<ILLMProvider> provider);
    Chain& add_tool(std::shared_ptr<ITool> tool);
    Chain& add_output_parser(std::shared_ptr<IParser> parser);

    ChainResult run(const Variables& input = {});
    std::future<ChainResult> run_async(const Variables& input = {});

    std::string serialize() const;
    static Chain deserialize(const std::string& data);
};
```

**Key Concepts:**
- **Node**: Basic unit, processes input → produces output
- **Variables**: Data container between nodes, supports `{{variable}}` template injection
- **Executor**: Manages concurrent execution, parallel node optimization

### 2. Agent - Intelligent Loop

```cpp
class Agent {
public:
    Agent(std::shared_ptr<ILLMProvider> llm,
          std::vector<std::shared_ptr<ITool>> tools);

    struct StepResult {
        Action action;
        Observation observation;
        bool should_continue;
    };
    StepResult step(const std::string& user_input);

    AgentResult run(const std::string& user_input, int max_steps = 10);

    void set_memory(std::shared_ptr<IMemory> memory);

    std::string save_state() const;
    void load_state(const std::string& state);
};
```

**Agent Loop Logic:**
```
User Input → LLM Reasoning → Tool Selection → Execute Tool → Observe Result
    ↑                                                    ↓
    └────────────────── Not Done? Continue Loop ←────────┘
                          ↓
                    Return Result
```

---

## LLM Provider Abstraction

### Unified Interface

```cpp
struct Message {
    enum Role { system, user, assistant, tool };
    Role role;
    std::string content;
    std::optional<std::string> tool_call_id;
    std::optional<ToolCall> tool_call;
};

struct ChatRequest {
    std::vector<Message> messages;
    std::string model;
    double temperature = 0.7;
    std::vector<ToolDefinition> tools;
    bool stream = false;
};

struct ChatResponse {
    Message message;
    std::optional<ToolCall> tool_call;
    std::map<std::string, std::string> usage;
    std::string raw_response;
};

class ILLMProvider {
public:
    virtual ChatResponse chat(const ChatRequest& req) = 0;
    virtual std::future<ChatResponse> chat_async(const ChatRequest& req) = 0;
    virtual void chat_stream(const ChatRequest& req,
                            StreamCallback callback) = 0;
    virtual std::vector<float> embed(const std::string& text);
};
```

### Adapter Strategy

| Provider | Strategy |
|----------|----------|
| OpenAI | Native adapter |
| Claude | Native adapter |
| Gemini | Native adapter |
| DeepSeek, Kimi, GLM, Qwen | OpenAI-compatible adapter (one-line support) |
| Custom | User-defined adapter |

```cpp
// OpenAI Compatible - supports most providers
class OpenAICompatibleProvider : public ILLMProvider {
public:
    OpenAICompatibleProvider(std::string base_url, std::string api_key)
        : impl_(base_url, std::move(api_key)) {}

    ChatResponse chat(const ChatRequest& req) override {
        return impl_.chat(req);
    }
private:
    OpenAIProvider impl_;
};
```

---

## Memory System

### IMemory Interface

```cpp
struct MemoryItem {
    std::string content;
    double relevance = 1.0;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;
};

class IMemory {
public:
    virtual void add(const std::string& content,
                    const std::map<std::string, std::string>& metadata = {}) = 0;
    virtual std::vector<MemoryItem> retrieve(const std::string& query,
                                            size_t top_k = 5) = 0;
    virtual std::vector<MemoryItem> get_all() const = 0;
    virtual void clear() = 0;
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& data) = 0;
};
```

### Implementations

- **ShortTermMemory**: Conversation history in RAM, sliding window with auto-summary
- **LongTermMemory**: Vector retrieval with embedding model support

---

## Tool System

### ITool Interface

```cpp
struct ToolDefinition {
    std::string name;
    std::string description;
    std::map<std::string, ToolParameter> parameters;
};

struct ToolCall {
    std::string name;
    std::string arguments;  // JSON string
    std::string id;
};

struct ToolResult {
    std::string content;
    bool is_error = false;
    std::string error_message;
};

class ITool {
public:
    virtual ToolDefinition get_definition() const = 0;
    virtual ToolResult execute(const std::string& arguments_json) = 0;
};
```

### Built-in Tools

- **HttpTool**: HTTP requests
- **CodeExecutionTool**: Sandboxed code execution
- **FileSystemTool**: File operations
- **CalculatorTool**: Mathematical calculations

---

## Error Handling

### Error Types

```cpp
class Error : public std::runtime_error;
class NetworkError : public Error;
class APIError : public Error;
class ToolError : public Error;
class SerializationError : public Error;
class ConfigError : public Error;
```

### Result<T> - No-Exception Path

```cpp
template<typename T>
class Result {
public:
    static Result ok(T value);
    static Result error(std::string message);

    bool is_ok() const;
    T& unwrap();
    const std::string& error_msg() const;

    template<typename F, typename U>
    Result<U> map(F&& func);
};
```

### Dual API

```cpp
class ILLMProvider {
public:
    // Exception version (default)
    virtual ChatResponse chat(const ChatRequest& req) = 0;

    // No-exception version
    virtual Result<ChatResponse> chat_safe(const ChatRequest& req) noexcept;
};
```

---

## Concurrency Model

### Executor

```cpp
class Executor {
public:
    explicit Executor(size_t num_threads);

    template<typename F>
    auto submit(F&& func) -> std::future<decltype(func())>;

    template<typename F, typename... Args>
    auto submit_batch(F&& func, Args&&... args)
        -> std::vector<std::future<decltype(func(args))>>;
};
```

### Concurrent Strategies

- **Async API**: `std::future<T>` return type
- **Stream API**: Callback for real-time response
- **Parallel Chain**: Automatic dependency analysis, parallel node execution

---

## Persistence

### Agent Snapshot

```cpp
struct AgentSnapshot {
    std::string version = "1.0";
    std::vector<Message> conversation_history;
    std::string short_term_memory_data;
    std::string long_term_memory_data;
    int current_step = 0;
    std::chrono::system_clock::time_point last_save;
    std::map<std::string, std::string> metadata;

    std::string to_json() const;
    static AgentSnapshot from_json(const std::string& json);
};
```

---

## Technical Specifications

### C++ Standard
- **Primary**: C++17
- **Compiler Support**: GCC 8+, Clang 7+, MSVC 2017+
- **Current Target**: Apple Clang 17.0.0 (confirmed)

### Dependencies

| Dependency | Type | Purpose |
|------------|------|---------|
| nlohmann/json | Header-only | JSON parsing |
| httplib | Header-only | HTTP client |
| spdlog | Optional | Logging |

### Build System
- **CMake 3.15+**
- **Static Library**: `libboostchain.a`
- **Options**:
  - `BOOSTCHAIN_BUILD_TESTS`
  - `BOOSTCHAIN_BUILD_EXAMPLES`
  - `BOOSTCHAIN_DISABLE_EXCEPTIONS`
  - `BOOSTCHAIN_USE_EXTERNAL_DEPS`

---

## Project Structure

```
BoostChain/
├── include/boostchain/          # Public headers
├── src/                         # Implementation
│   ├── memory/
│   ├── tools/
│   └── llm/
├── tests/
│   ├── unit/
│   ├── integration/
│   └── mock/
├── examples/
├── deps/                        # Bundled header-only libs
├── docs/
├── CMakeLists.txt
└── README.md
```

---

## Usage Examples

### Simple Chain

```cpp
#include <boostchain/boostchain.hpp>
#include <boostchain/llm/openai_provider.hpp>

auto llm = std::make_shared<OpenAIProvider>("api-key");

Chain chain;
chain.add_prompt("You are a helpful assistant.")
     .add_llm(llm);

auto result = chain.run({{"user_input", "Hello"}});
```

### Basic Agent

```cpp
std::vector<std::shared_ptr<ITool>> tools;
tools.push_back(std::make_shared<HttpTool>());
tools.push_back(std::make_shared<CalculatorTool>());

Agent agent(llm, tools);
auto result = agent.run("What's the weather in Tokyo?");
```

### OpenAI-Compatible Provider

```cpp
// DeepSeek - one line
auto deepseek = std::make_shared<OpenAICompatibleProvider>(
    "https://api.deepseek.com/v1", "api-key"
);

// Kimi
auto kimi = std::make_shared<OpenAICompatibleProvider>(
    "https://api.moonshot.cn/v1", "api-key"
);
```

### State Persistence

```cpp
Agent agent(llm, tools);
agent.run("Start task...");

// Save state
std::ofstream("agent_state.json") << agent.save_state();

// Restore later
Agent new_agent(llm, tools);
std::string state;
std::ifstream("agent_state.json") >> state;
new_agent.load_state(state);
```

---

## Implementation Phases

### Phase 1: Core Infrastructure
- [ ] Project setup (CMake, directory structure)
- [ ] Error handling system
- [ ] Result<T> implementation
- [ ] HTTP client wrapper
- [ ] JSON utilities

### Phase 2: LLM Provider Layer
- [ ] ILLMProvider interface
- [ ] OpenAI adapter
- [ ] OpenAI-compatible adapter
- [ ] Claude adapter
- [ ] Basic tests

### Phase 3: Chain Engine
- [ ] Node system
- [ ] Chain implementation
- [ ] Prompt template
- [ ] Variable substitution

### Phase 4: Agent Loop
- [ ] Agent implementation
- [ ] Tool system
- [ ] Agent-tool integration
- [ ] Built-in tools

### Phase 5: Memory System
- [ ] IMemory interface
- [ ] ShortTermMemory
- [ ] LongTermMemory (with embeddings)

### Phase 6: Persistence
- [ ] Serialization utilities
- [ ] Agent snapshot
- [ ] Memory persistence

### Phase 7: Concurrency
- [ ] Executor (thread pool)
- [ ] Async API
- [ ] Stream API
- [ ] Parallel chain optimization

### Phase 8: Testing & Examples
- [ ] Unit tests
- [ ] Integration tests
- [ ] Example applications
- [ ] Documentation

---

## License

TBD

---

## Contributors

TBD
