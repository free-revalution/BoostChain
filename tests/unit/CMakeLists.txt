add_executable(test_error test_error.cpp)
target_link_libraries(test_error boostchain::boostchain)
add_test(NAME test_error COMMAND test_error)

add_executable(test_result test_result.cpp)
target_link_libraries(test_result boostchain::boostchain)
add_test(NAME test_result COMMAND test_result)

add_executable(test_http_client test_http_client.cpp)
target_link_libraries(test_http_client boostchain::boostchain)
add_test(NAME test_http_client COMMAND test_http_client)

add_executable(test_llm_provider_interface test_llm_provider_interface.cpp)
target_link_libraries(test_llm_provider_interface boostchain::boostchain)
add_test(NAME test_llm_provider_interface COMMAND test_llm_provider_interface)
