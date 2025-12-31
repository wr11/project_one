# Add Lua source files
file(GLOB LUA_SOURCES
    "${CMAKE_SOURCE_DIR}/lua/src/*.c"
)

# Exclude unnecessary files
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*lua\\.c$")
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*luac\\.c$")

# Add client source files
set(CLIENT_SOURCES
    client_main.c
    client.c
    client_lua_bindings.c
)

# Create executable
add_executable(gearchat_client
    ${CLIENT_SOURCES}
    ${LUA_SOURCES}
)

# Set output directory to ../client (parent directory)
set_target_properties(gearchat_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../client"
)

# Set include directories
target_include_directories(gearchat_client PRIVATE
    ${CMAKE_SOURCE_DIR}/libuv/include
    ${CMAKE_SOURCE_DIR}/lua/src
)

# Link libraries
target_link_libraries(gearchat_client
    uv
)

# Windows specific settings
if(WIN32)
    target_link_libraries(gearchat_client
        ws2_32
        psapi
        iphlpapi
        userenv
    )
else()
    # Linux/Unix specific libraries
    target_link_libraries(gearchat_client
        pthread
        dl
    )
endif()
