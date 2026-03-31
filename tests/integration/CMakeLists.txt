# Integration tests

# OpenAI provider integration test
add_executable(test_openai_provider test_openai_provider.cpp)
target_link_libraries(test_openai_provider boostchain)

add_test(NAME openai_provider_integration COMMAND test_openai_provider)
set_tests_properties(openai_provider_integration PROPERTIES
    ENVIRONMENT "OPENAI_API_KEY=$ENV{OPENAI_API_KEY}"
    SKIP_RETURN_CODE 0  # Skip if API key not set
)

# OpenAI-Compatible provider integration test
add_executable(test_openai_compatible test_openai_compatible.cpp)
target_link_libraries(test_openai_compatible boostchain)

add_test(NAME openai_compatible_integration COMMAND test_openai_compatible)
set_tests_properties(openai_compatible_integration PROPERTIES
    ENVIRONMENT "DEEPSEEK_API_KEY=$ENV{DEEPSEEK_API_KEY}"
    SKIP_RETURN_CODE 0  # Skip if API key not set
)

# Claude provider integration test
add_executable(test_claude_provider test_claude_provider.cpp)
target_link_libraries(test_claude_provider boostchain)

add_test(NAME claude_provider_integration COMMAND test_claude_provider)
set_tests_properties(claude_provider_integration PROPERTIES
    ENVIRONMENT "ANTHROPIC_API_KEY=$ENV{ANTHROPIC_API_KEY}"
    SKIP_RETURN_CODE 0  # Skip if API key not set
)

# Gemini provider integration test
add_executable(test_gemini_provider test_gemini_provider.cpp)
target_link_libraries(test_gemini_provider boostchain)

add_test(NAME gemini_provider_integration COMMAND test_gemini_provider)
set_tests_properties(gemini_provider_integration PROPERTIES
    ENVIRONMENT "GEMINI_API_KEY=$ENV{GEMINI_API_KEY}"
    SKIP_RETURN_CODE 0  # Skip if API key not set
)
