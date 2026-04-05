#include <catch2/catch_test_macros.hpp>
#include "mcp/config.hpp"
#include "mcp/json_rpc.hpp"

#include <fstream>
#include <filesystem>

using namespace ccmake;

namespace {

// Helper to create a temp directory with a YAML file
struct TempDir {
    std::filesystem::path path;

    TempDir() {
        path = std::filesystem::temp_directory_path() / "ccmake-test-mcp-XXXXXX";
        std::filesystem::create_directories(path);
    }

    ~TempDir() {
        std::filesystem::remove_all(path);
    }

    void write_file(const std::string& name, const std::string& content) {
        std::ofstream ofs(path / name);
        ofs << content;
    }
};

}  // anonymous namespace

// ============================================================
// McpConfigLoader tests
// ============================================================

TEST_CASE("load_from_file parses valid YAML config") {
    TempDir dir;
    dir.write_file("mcp.yaml", R"(
mcpServers:
  test-server:
    command: npx
    args: ["-y", "@anthropic/mcp-server-test"]
    env:
      API_KEY: test-key
)");

    auto result = McpConfigLoader::load_from_file(dir.path / "mcp.yaml");
    REQUIRE(result.has_value());

    auto& config = result.value();
    REQUIRE(config.servers.count("test-server") == 1);

    auto& server = config.servers.at("test-server");
    REQUIRE(server.command == "npx");
    REQUIRE(server.args.size() == 2);
    REQUIRE(server.args[0] == "-y");
    REQUIRE(server.args[1] == "@anthropic/mcp-server-test");
    REQUIRE(server.env.size() == 1);
    REQUIRE(server.env[0].first == "API_KEY");
    REQUIRE(server.env[0].second == "test-key");
    REQUIRE_FALSE(server.disabled);
}

TEST_CASE("load_from_file parses SSE config") {
    TempDir dir;
    dir.write_file("mcp.yaml", R"(
mcpServers:
  remote-server:
    url: "http://localhost:3001/sse"
)");

    auto result = McpConfigLoader::load_from_file(dir.path / "mcp.yaml");
    REQUIRE(result.has_value());

    auto& server = result.value().servers.at("remote-server");
    REQUIRE(server.url == "http://localhost:3001/sse");
    REQUIRE(server.command.empty());
}

TEST_CASE("load_from_file handles disabled server") {
    TempDir dir;
    dir.write_file("mcp.yaml", R"(
mcpServers:
  disabled-server:
    command: echo
    disabled: true
)");

    auto result = McpConfigLoader::load_from_file(dir.path / "mcp.yaml");
    REQUIRE(result.has_value());

    auto& server = result.value().servers.at("disabled-server");
    REQUIRE(server.disabled);
}

TEST_CASE("load_from_file returns empty config for nonexistent file") {
    auto result = McpConfigLoader::load_from_file("/nonexistent/path/mcp.yaml");
    REQUIRE(result.has_value());
    REQUIRE(result.value().servers.empty());
}

TEST_CASE("load_from_file handles invalid YAML gracefully") {
    TempDir dir;
    dir.write_file("bad.yaml", R"(
mcpServers:
  - this is not valid
    for a map structure
)");

    auto result = McpConfigLoader::load_from_file(dir.path / "bad.yaml");
    // Should not crash, may return error or empty config
    // YAML parser is lenient, so we accept either outcome
}

TEST_CASE("load_project returns empty config when no mcp file exists") {
    TempDir dir;
    auto result = McpConfigLoader::load_project(dir.path);
    REQUIRE(result.has_value());
    REQUIRE(result.value().servers.empty());
}

TEST_CASE("merge project overrides user servers") {
    McpConfig project;
    McpConfig user;

    McpServerConfig user_server;
    user_server.name = "shared";
    user_server.command = "old-command";
    user.servers["shared"] = user_server;

    McpServerConfig project_server;
    project_server.name = "shared";
    project_server.command = "new-command";
    project.servers["shared"] = project_server;

    McpServerConfig project_only;
    project_only.name = "project-only";
    project_only.command = "proj-cmd";
    project.servers["project-only"] = project_only;

    auto merged = McpConfigLoader::merge(project, user);

    REQUIRE(merged.servers.count("shared") == 1);
    REQUIRE(merged.servers.at("shared").command == "new-command");
    REQUIRE(merged.servers.count("project-only") == 1);
}

// ============================================================
// McpManager tests
// ============================================================

TEST_CASE("McpManager constructor") {
    McpManager manager(std::filesystem::temp_directory_path());
    REQUIRE(manager.connected_servers().empty());
}

TEST_CASE("McpManager connect_all with empty config") {
    McpManager manager(std::filesystem::temp_directory_path());
    auto result = manager.connect_all();
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0);
}

TEST_CASE("McpManager disconnect_all is safe when empty") {
    McpManager manager(std::filesystem::temp_directory_path());
    manager.disconnect_all();
    REQUIRE(manager.connected_servers().empty());
}

TEST_CASE("McpManager get_client returns nullptr for unknown server") {
    McpManager manager(std::filesystem::temp_directory_path());
    REQUIRE(manager.get_client("nonexistent") == nullptr);
}
