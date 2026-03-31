# Contributing to BoostChain

Thank you for your interest in contributing to BoostChain! This guide covers everything you need to get started.

## Development Setup

### Prerequisites

- **CMake** 3.15 or later
- **C++17 compiler**: GCC 8+, Clang 7+, or MSVC 2017+
- **GNU Make** (or Ninja)

### Building

```bash
git clone https://github.com/free-revalution/BoostChain.git
cd BoostChain
mkdir build && cd build
cmake ..                    # builds library + tests + examples by default
cmake --build .
```

### Dependencies

BoostChain bundles two header-only libraries in `deps/`:

| Library | File | Version |
|---------|------|---------|
| [nlohmann/json](https://github.com/nlohmann/json) | `deps/json.hpp` | v3.12.0 |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | `deps/httplib.h` | v0.18.3 |

If they are missing, download them:

```bash
cd deps
curl -L -o json.hpp https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp
curl -L -o httplib.h https://github.com/yhirose/cpp-httplib/releases/download/v0.18.3/httplib.h
```

### Running Tests

```bash
cd build
ctest --output-on-failure
```

Unit tests require no network access. Integration tests are skipped automatically if the corresponding API key environment variable is not set.

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BOOSTCHAIN_BUILD_TESTS` | `ON` | Build unit and integration tests |
| `BOOSTCHAIN_BUILD_EXAMPLES` | `ON` | Build example programs |
| `BOOSTCHAIN_DISABLE_EXCEPTIONS` | `OFF` | Replace exception throws with `std::abort()` |

---

## Workflow

1. **Fork** the repository
2. **Create a feature branch**: `git checkout -b feature/my-feature`
3. **Implement** your changes (follow coding conventions below)
4. **Add tests** for new functionality
5. **Verify** all tests pass: `cd build && cmake --build . && ctest --output-on-failure`
6. **Commit** with a conventional commit message
7. **Push** to your fork and **open a Pull Request**

---

## Coding Conventions

### Namespace

All public API lives in `namespace boostchain { ... }`. Internal implementation details may use nested namespaces or anonymous namespaces within `.cpp` files.

### Include Guards

Use `#pragma once` in all headers:

```cpp
#pragma once

namespace boostchain {
// ...
}
```

### Naming

| Element | Style | Example |
|---------|-------|---------|
| Classes / Structs | `PascalCase` | `ChatRequest`, `ShortTermMemory` |
| Functions / Methods | `snake_case` | `add_prompt()`, `chat_safe()` |
| Variables | `snake_case` | `api_key_`, `max_tokens` |
| Member variables | `snake_case` with trailing `_` | `model_`, `base_url_` |
| Macros / Constants | `UPPER_SNAKE_CASE` | `BOOSTCHAIN_VERSION` |
| Template parameters | `PascalCase` | `template<typename T>` |

### Error Handling

BoostChain provides a **dual error handling** system:

- **Exceptions** for programming errors and truly unexpected failures:
  ```cpp
  throw boostchain::NetworkError("Connection failed", 503);
  ```

- **`Result<T>`** for recoverable errors where the caller decides how to handle them:
  ```cpp
  auto result = provider->chat_safe(request);
  if (result.is_error()) {
      // handle error
  }
  ```

When adding a new provider or component, implement both `chat()` (throws) and `chat_safe()` (returns `Result<T>`).

### Thread Safety

Shared mutable state must be protected with `std::mutex` and `std::lock_guard`:

```cpp
class MyProvider : public ILLMProvider {
    mutable std::mutex mutex_;
    std::string model_;

    void set_model(const std::string& m) override {
        std::lock_guard<std::mutex> lock(mutex_);
        model_ = m;
    }

    std::string get_model() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return model_;
    }
};
```

### Pimpl Idiom

For complex implementation classes that expose large dependencies in their headers, use the Pimpl idiom:

```cpp
// In header
class HttpClient {
public:
    HttpClient();
    ~HttpClient();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// In .cpp
class HttpClient::Impl {
    // actual implementation details
};
```

### Smart Pointers

- Use `std::shared_ptr` for shared ownership (e.g., `std::shared_ptr<ILLMProvider>`)
- Use `std::unique_ptr` for exclusive ownership (e.g., Pimpl, internal state)
- Never use raw `new`/`delete`

---

## Testing

### Test Framework

BoostChain uses **no external test framework**. Tests are self-contained C++ files using `assert()` with a single `main()` function per file.

### Adding a Unit Test

Create a new file in `tests/unit/`:

```cpp
// tests/unit/test_my_feature.cpp
#include <cassert>
#include <boostchain/my_feature.hpp>

int main() {
    // Arrange
    boostchain::MyFeature feature;

    // Act
    auto result = feature.do_something("input");

    // Assert
    assert(result.success == true);
    assert(result.value == "expected");

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
```

Register it in `tests/unit/CMakeLists.txt`:

```cmake
add_executable(test_my_feature test_my_feature.cpp)
target_link_libraries(test_my_feature PRIVATE boostchain)
add_test(NAME test_my_feature COMMAND test_my_feature)
```

### Adding an Integration Test

Create a new file in `tests/integration/` with an API key guard:

```cpp
// tests/integration/test_my_provider.cpp
#include <cstdlib>
#include <boostchain/llm/my_provider.hpp>

int main() {
    const char* api_key = std::getenv("MY_PROVIDER_API_KEY");
    if (!api_key) {
        std::cout << "Skipping: MY_PROVIDER_API_KEY not set" << std::endl;
        return 0;
    }

    // ... test implementation ...

    return 0;
}
```

---

## Commit Messages

BoostChain uses [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]
```

### Types

| Type | Purpose |
|------|---------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `refactor` | Code change that neither fixes a bug nor adds a feature |
| `test` | Adding or updating tests |
| `chore` | Build process, dependencies, tooling |

### Examples

```
feat: add Claude provider with streaming support
fix: resolve thread safety issue in OpenAI provider config
feat(memory): implement LongTermMemory with cosine similarity
docs: update API reference for Chain
test: add integration test for Gemini provider
refactor: simplify prompt template rendering to single-pass
```

---

## Adding a New LLM Provider

1. Create `include/boostchain/llm/my_provider.hpp` implementing `ILLMProvider`
2. Create `src/llm/my_provider.cpp` with the implementation
3. Add the `.cpp` to `CMakeLists.txt` under `target_sources`
4. Add unit tests in `tests/unit/test_my_provider.cpp`
5. Add integration tests in `tests/integration/test_my_provider.cpp`
6. Document the provider in `README.md`

### OpenAI-Compatible Providers

If the provider uses the OpenAI API format (DeepSeek, Kimi, GLM, Qwen, etc.), no new code is needed. Just use `OpenAICompatibleProvider`:

```cpp
auto provider = std::make_shared<boostchain::OpenAICompatibleProvider>(
    "https://api.example.com/v1",    // base URL
    "your-api-key"                    // API key
);
```

## Adding a New Tool

1. Create a class inheriting from `boostchain::ITool`
2. Implement `get_definition()` and `execute()`
3. Add tests in `tests/unit/test_tool.cpp` (or a new test file)
4. Register the tool with an `Agent`:

```cpp
class MyTool : public boostchain::ITool {
public:
    boostchain::ToolDefinition get_definition() const override {
        return {"my_tool", "Description of what this tool does",
                {{"param_name", {"string", "Param description", true}}}};
    }

    boostchain::ToolResult execute(const std::string& args_json) override {
        auto args = nlohmann::json::parse(args_json);
        // ... implement logic ...
        return {"result content"};
    }
};

// Usage
auto tool = std::make_shared<MyTool>();
boostchain::Agent agent(llm, {tool});
```

---

## Project Structure Overview

```
include/boostchain/     # Public headers (this is the API surface)
src/                    # Implementation (.cpp files)
tests/unit/             # Self-contained unit tests (no network)
tests/integration/      # Provider tests (require API keys)
examples/               # Usage examples
deps/                   # Bundled header-only dependencies
docs/                   # Design documents and plans
```

---

## Questions?

If you have questions, feel free to open a GitHub Issue. Please search existing issues first to avoid duplicates.
