
add_executable(matching_engine main.cpp ../pulsar_proxy/pulsar_proxy_c.cpp ../pulsar_proxy/pulsar_proxy_c.h manager.cpp manager.h ../include/utils.h ../engine/engine.cpp ../engine/engine.h ../TriggerOrderManager/TriggerOrderManager.cpp ../TriggerOrderManager/TriggerOrderManager.h)

if(APPLE)
    target_link_libraries(matching_engine pthread pulsar ssl crypto dl z curl)
elseif(UNIX)
    target_link_libraries(matching_engine pthread pulsarwithdeps ssl crypto dl z curl)
endif()


install(TARGETS matching_engine DESTINATION ${CMAKE_INSTALL_PREFIX})