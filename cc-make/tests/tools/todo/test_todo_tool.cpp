#include <catch2/catch_test_macros.hpp>
#include "tools/todo/todo_tool.hpp"
using namespace ccmake;

// Clear todos before each test to avoid interference
struct TodoFixture {
    TodoFixture() { TodoWriteTool::clear_todos(); }
    ~TodoFixture() { TodoWriteTool::clear_todos(); }
};

TEST_CASE("TodoWriteTool definition", "[todo]") {
    TodoWriteTool tool;
    auto& def = tool.definition();
    REQUIRE(def.name == "TodoWrite");
    REQUIRE_FALSE(def.description.empty());
    REQUIRE_FALSE(def.is_read_only);
    REQUIRE_FALSE(def.is_destructive);
    REQUIRE(def.aliases.size() >= 2);
    REQUIRE(def.input_schema.contains("properties"));
    REQUIRE(def.input_schema.contains("required"));
}

TEST_CASE("TodoWriteTool create returns ID", "[todo]") {
    TodoFixture fix;
    TodoWriteTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"action", "create"}, {"subject", "Test task"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("Task created with ID:") != std::string::npos);
    // Verify the ID is in the content
    REQUIRE(result.content.find("todo-") != std::string::npos);
}

TEST_CASE("TodoWriteTool create and list", "[todo]") {
    TodoFixture fix;
    TodoWriteTool tool;
    ToolContext ctx;

    tool.execute({{"action", "create"}, {"subject", "First task"}}, ctx);
    tool.execute({{"action", "create"}, {"subject", "Second task"}}, ctx);

    auto result = tool.execute({{"action", "list"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("First task") != std::string::npos);
    REQUIRE(result.content.find("Second task") != std::string::npos);
}

TEST_CASE("TodoWriteTool update status", "[todo]") {
    TodoFixture fix;
    TodoWriteTool tool;
    ToolContext ctx;

    auto create_result = tool.execute({{"action", "create"}, {"subject", "My task"}}, ctx);
    REQUIRE_FALSE(create_result.is_error);

    // Extract the ID from the create result
    auto id_pos = create_result.content.find("todo-");
    REQUIRE(id_pos != std::string::npos);
    auto id = create_result.content.substr(id_pos);
    // ID is the part after "Task created with ID: "
    auto id_start = create_result.content.find("Task created with ID: ") + std::string("Task created with ID: ").length();
    id = create_result.content.substr(id_start);

    auto update_result = tool.execute({
        {"action", "update"},
        {"task_id", nlohmann::json(id)},
        {"status", "completed"}
    }, ctx);
    REQUIRE_FALSE(update_result.is_error);
    REQUIRE(update_result.content.find("updated") != std::string::npos);

    auto list_result = tool.execute({{"action", "list"}}, ctx);
    REQUIRE(list_result.content.find("completed") != std::string::npos);
}

TEST_CASE("TodoWriteTool delete", "[todo]") {
    TodoFixture fix;
    TodoWriteTool tool;
    ToolContext ctx;

    auto create_result = tool.execute({{"action", "create"}, {"subject", "To be deleted"}}, ctx);
    REQUIRE_FALSE(create_result.is_error);

    auto id_start = create_result.content.find("Task created with ID: ") + std::string("Task created with ID: ").length();
    auto id = create_result.content.substr(id_start);

    auto delete_result = tool.execute({
        {"action", "delete"},
        {"task_id", nlohmann::json(id)}
    }, ctx);
    REQUIRE_FALSE(delete_result.is_error);

    auto list_result = tool.execute({{"action", "list"}}, ctx);
    REQUIRE(list_result.content.find("To be deleted") == std::string::npos);
}

TEST_CASE("TodoWriteTool list empty", "[todo]") {
    TodoFixture fix;
    TodoWriteTool tool;
    ToolContext ctx;

    auto result = tool.execute({{"action", "list"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("No tasks") != std::string::npos);
}

TEST_CASE("TodoWriteTool validate missing action", "[todo]") {
    TodoFixture fix;
    TodoWriteTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("TodoWriteTool validate invalid action", "[todo]") {
    TodoFixture fix;
    TodoWriteTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"action", "invalid"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("TodoWriteTool multiple creates", "[todo]") {
    TodoFixture fix;
    TodoWriteTool tool;
    ToolContext ctx;

    for (int i = 1; i <= 5; i++) {
        auto result = tool.execute({
            {"action", "create"},
            {"subject", "Task " + std::to_string(i)}
        }, ctx);
        REQUIRE_FALSE(result.is_error);
    }

    auto list_result = tool.execute({{"action", "list"}}, ctx);
    REQUIRE_FALSE(list_result.is_error);
    for (int i = 1; i <= 5; i++) {
        REQUIRE(list_result.content.find("Task " + std::to_string(i)) != std::string::npos);
    }
}

TEST_CASE("TodoWriteTool update non-existent task", "[todo]") {
    TodoFixture fix;
    TodoWriteTool tool;
    ToolContext ctx;
    auto result = tool.execute({
        {"action", "update"},
        {"task_id", "nonexistent-id"},
        {"status", "completed"}
    }, ctx);
    REQUIRE(result.is_error);
    REQUIRE(result.content.find("not found") != std::string::npos);
}

TEST_CASE("TodoWriteTool create with description", "[todo]") {
    TodoFixture fix;
    TodoWriteTool tool;
    ToolContext ctx;

    auto create_result = tool.execute({
        {"action", "create"},
        {"subject", "Detailed task"},
        {"description", "This is a detailed description of the task."}
    }, ctx);
    REQUIRE_FALSE(create_result.is_error);

    auto list_result = tool.execute({{"action", "list"}}, ctx);
    REQUIRE(list_result.content.find("Detailed task") != std::string::npos);
    REQUIRE(list_result.content.find("This is a detailed description") != std::string::npos);
}
