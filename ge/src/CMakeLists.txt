# Add Lua source files
file(GLOB LUA_SOURCES
    "${CMAKE_SOURCE_DIR}/lua/src/*.c"
)

# Exclude unnecessary files
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*lua\\.c$")
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*luac\\.c$")

# Add server engine source files
set(ENGINE_SOURCES
    main.c
    server.c
    lua_bindings.c
    bson_handler.c
)

# Create executable
# ge is the short name for GearEngine (齿轮引擎)
add_executable(ge
    ${ENGINE_SOURCES}
    ${LUA_SOURCES}
)

# Set output directory to ../server (parent directory)
set_target_properties(ge PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../server"
)

# Set include directories (ensure all headers can be found)
target_include_directories(ge PRIVATE
    ${CMAKE_SOURCE_DIR}/libuv/include
    ${CMAKE_SOURCE_DIR}/libbson/src/bson
    ${CMAKE_SOURCE_DIR}/lua/src
)

# Link libraries
target_link_libraries(ge
    uv
)

# Link libbson (prefer static library)
if(TARGET bson_static)
    target_link_libraries(ge bson_static)
    target_compile_definitions(ge PRIVATE BSON_STATIC)
elseif(TARGET bson_shared)
    target_link_libraries(ge bson_shared)
endif()

# Windows specific settings
if(WIN32)
    target_link_libraries(ge
        ws2_32
        psapi
        iphlpapi
        userenv
    )
endif()
