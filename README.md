<!-- BoostChain: C++ LLM Agent Framework — like LangChain but for C++ -->
<p align="center">
  <img src="https://img.shields.io/badge/C++-17-blue.svg" alt="C++17">
  <img src="https://img.shields.io/badge/CMake-3.15+-green.svg" alt="CMake">
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License">
  <img src="https://img.shields.io/badge/LLM-Agent_Framework-orange.svg" alt="Agent Framework">
</p>

<h1 align="center">BoostChain</h1>

<p align="center">
  <strong>C++ Agent Framework for LLM Applications</strong>
  <br>Build AI agents, chains, and tools with OpenAI, Claude, Gemini, DeepSeek, and more — in C++.
</p>

<p align="center">
  <a href="#quick-start">Quick Start</a> ·
  <a href="#llm-provider-setup">Providers</a> ·
  <a href="#code-examples">Examples</a> ·
  <a href="CONTRIBUTING.md">Contributing</a>
</p>

---

BoostChain provides a composable, interface-driven architecture for orchestrating LLM interactions in C++. It supports **chain-based workflows**, **autonomous agents with tool calling**, **memory management**, and **universal multi-provider support** — all compiled into a single static library with zero runtime dependencies.

Think of it as **LangChain for C++**: same concepts (chains, agents, tools, memory), but designed for performance-critical and embedded environments where Python isn't an option.

## Features

- **Universal LLM Support** — OpenAI, Claude, Gemini, DeepSeek, Kimi, GLM, Qwen, and any OpenAI-compatible API through a unified `ILLMProvider` interface
- **Chain Engine** — Composable node-based execution pipeline for building multi-step LLM workflows
- **Agent Loop** — Autonomous agent that reasons, selects tools, executes them, and iterates until a task is complete
- **Tool Calling** — Let the LLM call external functions; ship custom tools by implementing `ITool`
- **Memory System** — Short-term (sliding window) and long-term (vector retrieval) memory with serialization
- **Dual Error Handling** — Rust-style `Result<T>` for exception-free paths alongside a rich exception hierarchy
- **Thread Pool** — Built-in `Executor` for concurrent task submission with `std::future`
- **Prompt Templates** — `{{variable}}` substitution with safety checks
- **State Persistence** — Save and restore agent conversation history to/from disk
- **Zero Runtime Dependencies** — Only links `pthreads`; header-only deps (nlohmann/json, cpp-httplib) are bundled

---

## Quick Start

### 1. Clone and Build

```bash
git clone https://github.com/free-revalution/BoostChain.git
cd BoostChain
mkdir build && cd build
cmake .. -DBOOSTCHAIN_BUILD_TESTS=OFF
cmake --build .
```

### 2. Install Dependencies

The bundled `deps/` directory requires two header-only libraries. Download them:

```bash
cd deps
curl -L -o json.hpp https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp
curl -L -o httplib.h https://github.com/yhirose/cpp-httplib/releases/download/v0.18.3/httplib.h
```

### 3. Use It

```cpp
#include <boostchain/boostchain.hpp>
#include <boostchain/chain.hpp>
#include <boostchain/llm/openai_provider.hpp>
#include <iostream>

int main() {
    auto llm = std::make_shared<boostchain::OpenAIProvider>(
        std::getenv("OPENAI_API_KEY")
    );

    boostchain::Chain chain;
    chain.add_prompt("You are a helpful assistant. Answer concisely.")
         .add_llm(llm);

    auto result = chain.run({{"user_input", "What is BoostChain?"}});
    std::cout << result.output << std::endl;
    return 0;
}
```

Compile:
```bash
g++ -std=c++17 -I include -I deps main.cpp -L build -lboostchain -o my_app
```

---

## Architecture

BoostChain uses a **four-layer architecture** where each layer depends only on the layer below it:

```
+------------------------------------------------------------------+
|                         User Layer                                |
|   Chain / Agent / Memory / Tool / PromptTemplate / Executor       |
+--------------------------------+---------------------------------+
                                 |
+--------------------------------+---------------------------------+
|                      Orchestration Layer                          |
|   Chain Engine  |  Agent Loop  |  Executor (Thread Pool)          |
+--------------------------------+---------------------------------+
                                 |
+--------------------------------+---------------------------------+
|                      Abstraction Layer                           |
|   ILLMProvider  |  IMemory  |  ITool  |  Result<T>  |  Error     |
+--------------------------------+---------------------------------+
                                 |
+--------------------------------+---------------------------------+
|                       Adapter Layer                              |
|   OpenAI  |  Claude  |  Gemini  |  OpenAI Compatible  |  Custom  |
+--------------------------------+---------------------------------+
                                 |
+--------------------------------+---------------------------------+
|                    Infrastructure Layer                          |
|   HttpClient  |  nlohmann/json  |  cpp-httplib  |  pthreads      |
+------------------------------------------------------------------+
```

### Agent Loop

```
User Input --> [System Prompt + Conversation History]
                         |
                   LLM Reasoning
                         |
                +--------+--------+
                |                 |
            Tool Call?          Final Answer
                |                 |
          Execute Tool              Return
                |                 |
          Append Result
                |
           Not Done? --> Continue Loop (max iterations)
```

---

## Comparison with LangChain

| Dimension | BoostChain | LangChain |
|-----------|-----------|-----------|
| **Language** | C++17 | Python / TypeScript |
| **Runtime Dependencies** | None (header-only bundled) | Python + pip packages |
| **Deployment** | Static library, embeddable, edge | Requires Python runtime |
| **LLM Providers** | OpenAI, Claude, Gemini, any OpenAI-compatible | OpenAI, Claude, Gemini, 100+ integrations |
| **Chain Engine** | Node-based pipeline | LCEL (LangChain Expression Language) |
| **Agent Loop** | Built-in with tool calling | ReAct, Plan-and-Execute, custom |
| **Memory** | Short-term + Long-term (vector) | ConversationBuffer, VectorStore |
| **Error Handling** | `Result<T>` + exception hierarchy | Python exceptions |
| **Concurrency** | Built-in thread pool | asyncio / callbacks |
| **Streaming** | Placeholder (planned) | Full SSE streaming |

**When to choose BoostChain:**
- Embedding LLM capabilities into C++ applications (game engines, desktop apps, trading systems)
- Latency-sensitive or memory-constrained environments where Python overhead is unacceptable
- Deploying to embedded/edge devices without a Python runtime
- Building shared libraries or SDKs that expose LLM features

---

## Installation

### Prerequisites

- **CMake** 3.15 or later
- **C++17 compiler**: GCC 8+, Clang 7+, MSVC 2017+
- **pthreads**: Available on Linux/macOS by default; Windows uses native threads

### Build from Source

```bash
git clone https://github.com/free-revalution/BoostChain.git
cd BoostChain
mkdir build && cd build
cmake ..                  # default: builds tests + examples
cmake --build .
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BOOSTCHAIN_BUILD_TESTS` | `ON` | Build unit and integration tests |
| `BOOSTCHAIN_BUILD_EXAMPLES` | `ON` | Build example programs |
| `BOOSTCHAIN_DISABLE_EXCEPTIONS` | `OFF` | Replace exceptions with `std::abort()` |

### Run Tests

```bash
cd build
ctest --output-on-failure
```

Integration tests (OpenAI, Claude, Gemini) require API keys and will be skipped automatically if the corresponding environment variable is not set.

### Integrate into Your Project

```cmake
# Add as a subdirectory
add_subdirectory(BoostChain)

# Link against the library
target_link_libraries(your_target PRIVATE boostchain::boostchain)
```

### Install System-Wide

```bash
cd build
cmake --install . --prefix /usr/local
```

This installs `libboostchain.a` to `/usr/local/lib` and headers to `/usr/local/include/boostchain/`.

---

## LLM Provider Setup

BoostChain supports all major LLM providers through a unified `ILLMProvider` interface:

| Provider | Class | Base URL | Auth Method |
|----------|-------|----------|-------------|
| **OpenAI** | `OpenAIProvider` | `https://api.openai.com/v1` | `Authorization: Bearer` |
| **Claude** | `ClaudeProvider` | `https://api.anthropic.com/v1` | `x-api-key` header |
| **Gemini** | `GeminiProvider` | `https://generativelanguage.googleapis.com/v1beta` | URL query param |
| **DeepSeek** | `OpenAICompatibleProvider` | `https://api.deepseek.com/v1` | `Authorization: Bearer` |
| **Kimi** | `OpenAICompatibleProvider` | `https://api.moonshot.cn/v1` | `Authorization: Bearer` |
| **GLM** | `OpenAICompatibleProvider` | `https://open.bigmodel.cn/api/paas/v4` | `Authorization: Bearer` |
| **Qwen** | `OpenAICompatibleProvider` | `https://dashscope.aliyuncs.com/compatible-mode/v1` | `Authorization: Bearer` |
| **Any OpenAI-compatible** | `OpenAICompatibleProvider` | User-defined | `Authorization: Bearer` |

### OpenAI

```cpp
auto llm = std::make_shared<boostchain::OpenAIProvider>("sk-...");
llm->set_model("gpt-4o");
auto response = llm->chat(request);
```

### Anthropic Claude

```cpp
auto llm = std::make_shared<boostchain::ClaudeProvider>("sk-ant-...");
llm->set_model("claude-sonnet-4-20250514");
auto response = llm->chat(request);
```

### Google Gemini

```cpp
auto llm = std::make_shared<boostchain::GeminiProvider>("AIza...");
llm->set_model("gemini-2.0-flash");
auto response = llm->chat(request);
```

### DeepSeek (OpenAI-Compatible)

```cpp
auto llm = std::make_shared<boostchain::OpenAICompatibleProvider>(
    "https://api.deepseek.com/v1",
    "sk-..."
);
llm->set_model("deepseek-chat");
auto response = llm->chat(request);
```

### Kimi (OpenAI-Compatible)

```cpp
auto llm = std::make_shared<boostchain::OpenAICompatibleProvider>(
    "https://api.moonshot.cn/v1",
    "sk-..."
);
llm->set_model("moonshot-v1-8k");
auto response = llm->chat(request);
```

### GLM / Qwen (OpenAI-Compatible)

```cpp
// GLM (Zhipu AI)
auto glm = std::make_shared<boostchain::OpenAICompatibleProvider>(
    "https://open.bigmodel.cn/api/paas/v4",
    "your-api-key"
);
glm->set_model("glm-4");

// Qwen (Alibaba Cloud)
auto qwen = std::make_shared<boostchain::OpenAICompatibleProvider>(
    "https://dashscope.aliyuncs.com/compatible-mode/v1",
    "sk-..."
);
qwen->set_model("qwen-turbo");
```

---

## API Reference

### Error Handling

```cpp
namespace boostchain {

class Error : public std::runtime_error;          // Base class
class NetworkError : public Error;                  // + int status_code()
class APIError : public Error;                        // + string raw_response()
class ToolError : public Error;                        // + string tool_name()
class SerializationError : public Error;
class ConfigError : public Error;

}
```

### Result\<T\> — Exception-Free Errors

```cpp
namespace boostchain {

template<typename T>
class Result {
public:
    static Result ok(T value);
    static Result error(std::string message);

    bool is_ok() const;
    bool is_error() const;

    const T& unwrap() const;             // throws if error
    const std::string& error_msg() const; // throws if ok

    template<typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<T&>()))>;
};

}
```

Usage:

```cpp
// Exception-based
try {
    auto response = llm->chat(request);
} catch (const boostchain::NetworkError& e) {
    std::cerr << "HTTP " << e.status_code() << ": " << e.what() << std::endl;
}

// Result-based
auto result = llm->chat_safe(request);
if (result.is_error()) {
    std::cerr << "Error: " << result.error_msg() << std::endl;
} else {
    auto response = result.unwrap();
}
```

### ILLMProvider — Abstract Interface

All LLM providers implement this interface. It is the central abstraction of BoostChain.

```cpp
namespace boostchain {

// Data structures
struct ToolCall { std::string id, name, arguments; };

struct Message {
    enum Role { system, user, assistant, tool };
    Role role;
    std::string content;
    std::optional<std::string> name;
    std::optional<ToolCall> tool_call;
};

struct ChatRequest {
    std::string model;
    std::vector<Message> messages;
    std::optional<double> temperature;
    std::optional<int> max_tokens;
    std::optional<double> top_p;
    std::optional<double> presence_penalty;
    std::optional<double> frequency_penalty;
    std::optional<std::vector<std::string>> stop;
    std::optional<std::vector<ToolDefinition>> tools;
};

struct Usage { int prompt_tokens, completion_tokens, total_tokens; };

struct ChatResponse {
    std::string id, object, model;
    int created;
    std::vector<Message> messages;
    std::optional<Usage> usage;
    std::optional<std::string> finish_reason;
};

struct EmbeddingRequest {
    std::string model;
    std::vector<std::string> inputs;
    std::optional<std::string> encoding_format;
};

struct EmbeddingResponse {
    std::string object, model;
    std::vector<Embedding> data;
    std::optional<Usage> usage;
};

// Abstract interface
class ILLMProvider {
public:
    virtual ChatResponse chat(const ChatRequest&) = 0;
    virtual Result<ChatResponse> chat_safe(const ChatRequest&) = 0;
    virtual std::future<ChatResponse> chat_async(const ChatRequest&) = 0;
    virtual void chat_stream(const ChatRequest&, StreamCallback) = 0;
    virtual EmbeddingResponse embed(const EmbeddingRequest&) = 0;

    virtual void set_api_key(const std::string&) = 0;
    virtual void set_model(const std::string&) = 0;
    virtual void set_base_url(const std::string&) = 0;
    virtual void set_timeout(int seconds) = 0;

    virtual std::string get_api_key() const = 0;
    virtual std::string get_model() const = 0;
    virtual std::string get_base_url() const = 0;
    virtual int get_timeout() const = 0;
};

}
```

### PromptTemplate

```cpp
namespace boostchain {

using Variables = std::map<std::string, std::string>;

class PromptTemplate {
public:
    explicit PromptTemplate(const std::string& template_str);
    std::string render(const Variables& vars) const;  // throws ConfigError on missing vars
};

}
```

Template syntax: `{{variable_name}}`. Whitespace inside braces is trimmed. All variables must be provided.

```cpp
boostchain::PromptTemplate tpl("Hello, {{name}}! Today is {{day}}.");
std::string rendered = tpl.render({{"name", "Alice"}, {"day", "Monday"}});
// "Hello, Alice! Today is Monday."
```

### Chain

```cpp
namespace boostchain {

struct ChainResult {
    std::string output;
    std::map<std::string, std::string> variables;
    bool success = true;
    std::string error;
};

class Chain {
public:
    Chain();

    Chain& add_prompt(const std::string& template_str);
    Chain& add_llm(std::shared_ptr<ILLMProvider> provider);

    ChainResult run(const std::map<std::string, std::string>& input = {}) const;
    std::string serialize() const;
    static Chain deserialize(const std::string& data);
    size_t node_count() const;
};

}
```

Nodes execute sequentially. Each node receives all accumulated variables from previous nodes. `PromptNode` renders `{{variable}}` templates. `LLMNode` calls the provider with the accumulated context.

```cpp
auto llm = std::make_shared<boostchain::OpenAIProvider>("sk-...");

boostchain::Chain chain;
chain.add_prompt("Translate to {{language}}: {{text}}")
     .add_llm(llm);

auto result = chain.run({
    {"language", "French"},
    {"text", "Hello, world!"}
});
// result.output contains the translated text
```

### Agent

```cpp
namespace boostchain {

struct AgentResult {
    std::string final_answer;
    int steps_taken;
    bool completed;
};

class Agent {
public:
    Agent(std::shared_ptr<ILLMProvider> llm);
    Agent(std::shared_ptr<ILLMProvider> llm,
          std::vector<std::shared_ptr<ITool>> tools);

    AgentResult run(const std::string& user_input);
    void set_system_prompt(const std::string& prompt);
    void reset();
    void set_max_iterations(int max);  // default: 10

    std::string save_state() const;                // JSON serialization
    bool load_state(const std::string& json_data);  // returns false on error
};

}
```

The agent maintains conversation history across multiple `run()` calls. When tools are registered, the agent loop works as follows:

1. Send user input + history + tool definitions to the LLM
2. If the LLM returns a tool call, execute the tool and send the result back
3. Repeat until the LLM returns a final answer or `max_iterations` is reached

```cpp
auto llm = std::make_shared<boostchain::OpenAIProvider>("sk-...");
llm->set_model("gpt-4o");

auto calc = std::make_shared<boostchain::CalculatorTool>();

boostchain::Agent agent(llm, {calc});
agent.set_system_prompt("Use the calculator for math. Show your work.");

auto result = agent.run("What is (123 + 456) * 789?");
std::cout << "Answer: " << result.final_answer << std::endl;
std::cout << "Steps: " << result.steps_taken << std::endl;
```

### Memory System

```cpp
namespace boostchain {

struct MemoryItem {
    std::string content;
    double relevance = 1.0;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;
};

class IMemory {
public:
    virtual void add(const MemoryItem&) = 0;
    virtual void add(const std::string& content, double relevance = 1.0) = 0;
    virtual std::vector<MemoryItem> retrieve(const std::string& query,
                                               size_t max_items = 10) const = 0;
    virtual std::vector<MemoryItem> get_all() const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& data) = 0;
};

class ShortTermMemory : public IMemory {
public:
    explicit ShortTermMemory(size_t max_items = 100);
    // FIFO sliding window. retrieve() returns most-recent-first.
};

class LongTermMemory : public IMemory {
public:
    using EmbeddingFn = std::function<std::vector<double>(const std::string&)>;
    explicit LongTermMemory(size_t max_items = 10000,
                              EmbeddingFn embed_fn = nullptr);
    // With embed_fn: cosine similarity retrieval
    // Without: substring matching + relevance scoring
};

}
```

### Tool System

```cpp
namespace boostchain {

struct ToolResult {
    std::string content;
    bool success = true;
    std::string error;
};

class ITool {
public:
    virtual boostchain::ToolDefinition get_definition() const = 0;
    virtual boostchain::ToolResult execute(const std::string& arguments_json) = 0;
    virtual std::string get_name() const;
};

class CalculatorTool : public ITool {
public:
    CalculatorTool();  // Supports +, -, *, /, parentheses
};

}
```

Create a custom tool by implementing `ITool`:

```cpp
class WeatherTool : public boostchain::ITool {
public:
    boostchain::ToolDefinition get_definition() const override {
        return {"weather", "Get current weather for a city",
                {{"city", {"string", "City name", true}}}};
    }

    boostchain::ToolResult execute(const std::string& json) override {
        auto args = nlohmann::json::parse(json);
        std::string city = args["city"];
        // Call a weather API here...
        return {"Sunny, 22C in " + city};
    }
};
```

### Executor (Thread Pool)

```cpp
namespace boostchain {

class Executor {
public:
    explicit Executor(size_t num_threads = 0);  // 0 = hardware_concurrency

    template<typename F>
    auto submit(F&& func) -> std::future<decltype(func())>;

    size_t thread_count() const;
};

}
```

---

## Code Examples

### 30-Second Agent with Tool Calling

```cpp
#include <boostchain/agent.hpp>
#include <boostchain/tool.hpp>
#include <boostchain/llm/openai_compatible.hpp>
#include <iostream>

int main() {
    // Use any LLM provider — DeepSeek, Kimi, GLM, Qwen, OpenAI...
    auto llm = std::make_shared<boostchain::OpenAICompatibleProvider>(
        "https://api.deepseek.com/v1", std::getenv("DEEPSEEK_API_KEY")
    );
    llm->set_model("deepseek-chat");

    // Create an agent with a built-in calculator tool
    boostchain::Agent agent(llm, {std::make_shared<boostchain::CalculatorTool>()});

    // The agent reasons, calls tools, and iterates automatically
    auto result = agent.run("What is (123 + 456) * 789?");
    std::cout << result.final_answer << std::endl;  // "456615"
    std::cout << "Steps: " << result.steps_taken << std::endl;  // 2

    // Multi-turn: context is preserved
    auto r2 = agent.run("Divide that by 100.");
    std::cout << r2.final_answer << std::endl;  // "4566.15"

    // Save and restore state
    std::ofstream("state.json") << agent.save_state();
}
```

### Simple Chain

```cpp
auto llm = std::make_shared<boostchain::OpenAIProvider>("sk-...");

boostchain::Chain chain;
chain.add_prompt("Translate to {{language}}: {{text}}")
     .add_llm(llm);

auto result = chain.run({
    {"language", "French"},
    {"text", "Hello, world!"}
});
// result.output → "Bonjour, le monde !"
```

### Multi-Provider Agent

```cpp
#include <boostchain/agent.hpp>
#include <boostchain/tool.hpp>
#include <boostchain/llm/openai_compatible.hpp>
#include <iostream>

int main() {
    // Use DeepSeek as the LLM backend
    auto llm = std::make_shared<boostchain::OpenAICompatibleProvider>(
        "https://api.deepseek.com/v1",
        std::getenv("DEEPSEEK_API_KEY")
    );
    llm->set_model("deepseek-chat");

    auto calc = std::make_shared<boostchain::CalculatorTool>();

    boostchain::Agent agent(llm, {calc});
    agent.set_system_prompt("You are a helpful assistant.");

    // Multi-turn conversation
    auto r1 = agent.run("What is 2 + 3 * 4?");
    std::cout << "Turn 1: " << r1.final_answer << std::endl;

    auto r2 = agent.run("And 100 - that result?");
    std::cout << "Turn 2: " << r2.final_answer << std::endl;

    // Save and restore state
    std::ofstream("state.json") << agent.save_state();
    std::ifstream ifs("state.json");
    std::string state((std::istreambuf_iterator<char>(ifs),
                         std::istreambuf_iterator<char>()));
    agent.load_state(state);

    return 0;
}
```

### Async Execution

```cpp
#include <boostchain/llm/openai_provider.hpp>
#include <iostream>

int main() {
    auto llm = std::make_shared<boostchain::OpenAIProvider>(
        std::getenv("OPENAI_API_KEY")
    );

    // Async call
    boostchain::ChatRequest req;
    req.model = "gpt-4o";
    req.messages.push_back({boostchain::Message::user, "Explain C++ move semantics briefly."});

    auto future = llm->chat_async(req);
    // ... do other work ...
    auto response = future.get();

    std::cout << response.messages[0].content << std::endl;
    return 0;
}
```

### Short-Term Memory

```cpp
#include <boostchain/memory.hpp>
#include <iostream>

int main() {
    boostchain::ShortTermMemory memory(5);  // Keep last 5 items

    memory.add("The sky is blue");
    memory.add("C++ is fast");
    memory.add("BoostChain is an agent framework");
    memory.add("Machine learning is fascinating");
    memory.add("The sky is cloudy");

    auto results = memory.retrieve("sky");
    for (const auto& item : results) {
        std::cout << item.content << " (relevance: " << item.relevance << ")" << std::endl;
    }
    // Output:
    //   The sky is cloudy (relevance: 1.0)
    //   The sky is blue (relevance: 1.0)

    return 0;
}
```

### Long-Term Memory with Embeddings

```cpp
#include <boostchain/memory.hpp>
#include <boostchain/llm/openai_provider.hpp>

int main() {
    auto llm = std::make_shared<boostchain::OpenAIProvider>("sk-...");

    // Create embedding function from the LLM
    boostchain::LongTermMemory::EmbeddingFn embed_fn =
        [llm](const std::string& text) -> std::vector<double> {
            boostchain::EmbeddingRequest req;
            req.model = "text-embedding-3-small";
            req.inputs = {text};
            auto resp = llm->embed(req);
            std::vector<double> vec;
            for (const auto& emb : resp.data) {
                // Parse embedding string to float vector
                // (implementation depends on embedding format)
            }
            return vec;
        };

    boostchain::LongTermMemory memory(10000, embed_fn);
    memory.add("Paris is the capital of France");
    memory.add("The Eiffel Tower is in Paris");
    memory.add("Berlin is the capital of Germany");

    auto results = memory.retrieve("European capitals");
    // Returns items ranked by cosine similarity
}
```

---

## Testing

BoostChain uses a lightweight, self-contained test approach with no external test framework dependency.

### Test Structure

```
tests/
├── unit/                     # 10 test executables (no network required)
│   ├── test_error.cpp           # Error hierarchy
│   ├── test_result.cpp          # Result<T>
│   ├── test_http_client.cpp     # HTTP operations
│   ├── test_llm_provider_interface.cpp  # Data structures
│   ├── test_prompt.cpp          # Template rendering
│   ├── test_chain.cpp           # Chain serialization
│   ├── test_agent.cpp           # Agent loop, tools, persistence
│   ├── test_memory.cpp          # Memory add/retrieve/serialize
│   ├── test_tool.cpp            # CalculatorTool arithmetic
│   └── test_executor.cpp        # Thread pool concurrency
└── integration/              # 4 tests (skip if no API key)
    ├── test_openai_provider.cpp
    ├── test_openai_compatible.cpp
    ├── test_claude_provider.cpp
    └── test_gemini_provider.cpp
```

### Running Tests

```bash
cd build
ctest --output-on-failure
```

Unit tests are fully self-contained. Integration tests require API keys:

| Environment Variable | Test |
|---------------------|------|
| `OPENAI_API_KEY` | OpenAI provider |
| `DEEPSEEK_API_KEY` | OpenAI-compatible provider |
| `ANTHROPIC_API_KEY` | Claude provider |
| `GEMINI_API_KEY` | Gemini provider |

---

## Project Structure

```
BoostChain/
├── CMakeLists.txt                  # Root build configuration
├── README.md                       # This file
├── CONTRIBUTING.md                  # Contributing guide
├── LICENSE                         # MIT License
├── .gitignore
├── deps/                           # Bundled header-only dependencies
│   ├── json.hpp                    # nlohmann/json v3.12.0
│   └── httplib.h                   # cpp-httplib
├── include/boostchain/             # Public API headers
│   ├── boostchain.hpp              # Umbrella header (version + config)
│   ├── config.hpp                  # Platform/compiler detection
│   ├── version.hpp                 # Version macros
│   ├── error.hpp                   # Exception hierarchy
│   ├── result.hpp                  # Result<T> template
│   ├── llm_provider.hpp            # ILLMProvider + data structures
│   ├── chain.hpp                   # Chain engine
│   ├── agent.hpp                   # Agent with tool calling
│   ├── memory.hpp                  # IMemory, ShortTermMemory, LongTermMemory
│   ├── tool.hpp                    # ITool, CalculatorTool
│   ├── prompt.hpp                  # PromptTemplate
│   ├── executor.hpp                # Thread pool
│   ├── http_client.hpp             # HTTP client wrapper
│   └── llm/
│       ├── openai_provider.hpp
│       ├── openai_compatible.hpp
│       ├── claude_provider.hpp
│       └── gemini_provider.hpp
├── src/                            # Implementation files (.cpp)
│   ├── error.cpp
│   ├── http_client.cpp
│   ├── chain.cpp
│   ├── agent.cpp
│   ├── memory.cpp
│   ├── tool.cpp
│   ├── prompt.cpp
│   ├── executor.cpp
│   └── llm/
│       ├── openai_provider.cpp
│       ├── openai_compatible.cpp
│       ├── claude_provider.cpp
│       └── gemini_provider.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   └── integration/
├── examples/
│   └── simple_chain/
│       └── main.cpp
└── docs/
    └── plans/                      # Design and implementation docs
```

---

## Roadmap

### v0.2 — Near-Term

- [ ] Wire `IMemory` into `Agent` (add `set_memory()` method)
- [ ] Wire `Executor` into `Chain` for parallel node execution
- [ ] Implement real SSE streaming in `chat_stream()`
- [ ] Add more built-in tools (HttpTool, FileSystemTool, CodeExecutionTool)
- [ ] Add `Chain::add_tool()` node type and `Chain::add_output_parser()` node type
- [ ] Use nlohmann/json throughout for serialization (replace hand-rolled parsers)

### v0.3 — Mid-Term

- [ ] RAG (Retrieval-Augmented Generation) pipeline
- [ ] Multi-agent orchestration (agent-to-agent communication)
- [ ] Structured output / JSON mode
- [ ] Token counting and cost estimation
- [ ] Retry logic with exponential backoff

### v0.4 — Long-Term

- [ ] gRPC/protobuf support for remote agent deployment
- [ ] Plugin system for dynamically loaded providers and tools
- [ ] C++20 coroutines for async/await syntax
- [ ] Observability hooks (tracing, metrics)
- [ ] Comprehensive documentation (API reference docs)

---

## Contributing

Contributions are welcome! Here is how to get started:

### Development Setup

```bash
git clone https://github.com/free-revalution/BoostChain.git
cd BoostChain
mkdir build && cd build
cmake ..                    # builds with tests and examples
cmake --build .
ctest --output-on-failure  # verify all tests pass
```

### Workflow

1. **Fork** the repository
2. **Create a feature branch**: `git checkout -b feature/my-feature`
3. **Implement** your changes following the coding conventions below
4. **Add tests** for new functionality (use `assert()`, no external framework)
5. **Verify** all tests pass: `cd build && cmake --build . && ctest`
6. **Commit** with a conventional commit message (see below)
7. **Open a Pull Request**

### Commit Message Convention

The project uses [Conventional Commits](https://www.conventionalcommits.org/):

```
feat: add Claude provider with streaming support
fix: resolve thread safety issue in OpenAI provider
docs: update API reference for Chain
refactor: simplify memory serialization logic
test: add integration test for Gemini provider
```

### Coding Conventions

- **Namespace**: All public API lives in `namespace boostchain { ... }`
- **Headers**: Use `#pragma once` for include guards
- **Error Handling**: Use `Result<T>` for recoverable errors; throw exceptions for programming errors
- **Thread Safety**: Protect shared mutable state with `std::mutex` + `std::lock_guard`
- **Pimpl Idiom**: Use `std::unique_ptr<Impl>` for complex implementation classes
- **Naming**: `snake_case` for functions and variables, `PascalCase` for classes
- **Tests**: Self-contained with `assert()`, one `main()` per test file, no external framework

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

## Acknowledgments

BoostChain's architecture is inspired by [LangChain](https://github.com/langchain-ai/langchain). It adapts LangChain's core concepts — chains, agents, tools, and memory — to the C++ ecosystem with a focus on performance, zero-overhead abstraction, and embeddability.
