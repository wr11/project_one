# Stress test client

# Add source files
set(STRESS_SOURCES
    stress_client.c
)

# Create executable
add_executable(stress_client
    ${STRESS_SOURCES}
)

# Set include directories
# Note: CMAKE_SOURCE_DIR points to ge/ directory when benchmark is added as subdirectory
target_include_directories(stress_client PRIVATE
    ${CMAKE_SOURCE_DIR}/libuv/include
)

# Link libraries
target_link_libraries(stress_client
    uv
)

# Windows specific settings
if(WIN32)
    target_link_libraries(stress_client
        ws2_32
        psapi
        iphlpapi
        userenv
    )
else()
    # Linux/Unix specific libraries
    target_link_libraries(stress_client
        pthread
        dl
    )
endif()

# Set output directory
set_target_properties(stress_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
