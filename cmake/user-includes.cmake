if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/user_includes.txt")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/user_includes.txt" user_includes)

    string(REPLACE "\r" "" user_includes "${user_includes}")
    string(REPLACE "\n" ";" user_includes_list "${user_includes}")

    foreach(dir IN LISTS user_includes_list)
        if(dir STREQUAL "" OR dir MATCHES "^#")
            continue()
        endif()

        get_filename_component(full_path "${CMAKE_CURRENT_SOURCE_DIR}/${dir}" ABSOLUTE)

        message(STATUS "Adding include directory: ${full_path}")

        include_directories(${full_path})
    endforeach()
else()
    message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}/user_includes.txt does not exist, skipping include directories processing.")
endif()
