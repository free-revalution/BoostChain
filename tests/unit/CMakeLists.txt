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

add_executable(test_prompt test_prompt.cpp)
target_link_libraries(test_prompt boostchain::boostchain)
add_test(NAME test_prompt COMMAND test_prompt)

add_executable(test_chain test_chain.cpp)
target_link_libraries(test_chain boostchain::boostchain)
add_test(NAME test_chain COMMAND test_chain)

add_executable(test_agent test_agent.cpp)
target_link_libraries(test_agent boostchain::boostchain)
add_test(NAME test_agent COMMAND test_agent)

add_executable(test_memory test_memory.cpp)
target_link_libraries(test_memory boostchain::boostchain)
add_test(NAME test_memory COMMAND test_memory)

add_executable(test_tool test_tool.cpp)
target_link_libraries(test_tool boostchain::boostchain)
add_test(NAME test_tool COMMAND test_tool)

add_executable(test_executor test_executor.cpp)
target_link_libraries(test_executor boostchain::boostchain)
add_test(NAME test_executor COMMAND test_executor)
