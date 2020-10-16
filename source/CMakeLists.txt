file(GLOB_RECURSE SRC_APP "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(camscii)
target_sources(camscii
    PRIVATE
        ${SRC_APP}
    )
target_include_directories(camscii
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )

set_target_properties(camscii
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(camscii
    PRIVATE
        project_options
        project_warnings
        pthread
        stdc++fs
        kibble # My utility lib
        v4l2
)

cotire(camscii)