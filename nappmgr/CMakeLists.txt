set(TARGET "nappmgr")
set(NAMESPACE "WPEFramework")
add_executable(${TARGET}
   nappmgr.cpp
   helpers/serviceconnector.cpp
   helpers/serviceconnector.h
)
find_package(PkgConfig)
find_package(${NAMESPACE}WebSocket REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package( ${NAMESPACE}SecurityUtil REQUIRED)

target_link_libraries(${TARGET}
    ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
    ${NAMESPACE}SecurityUtil
)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )
target_include_directories(${TARGET} PRIVATE
        ${WPEFRAMEWORKWEBSOCKET_INCLUDE_DIRS})

install(TARGETS ${TARGET} DESTINATION bin)
