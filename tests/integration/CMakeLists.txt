# Integration tests

# OpenAI provider integration test
add_executable(test_openai_provider test_openai_provider.cpp)
target_link_libraries(test_openai_provider boostchain)

add_test(NAME openai_provider_integration COMMAND test_openai_provider)
set_tests_properties(openai_provider_integration PROPERTIES
    ENVIRONMENT "OPENAI_API_KEY=$ENV{OPENAI_API_KEY}"
    SKIP_RETURN_CODE 0  # Skip if API key not set
)
