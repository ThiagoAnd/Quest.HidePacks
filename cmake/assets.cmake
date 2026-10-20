set(ASSETS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets)

set(ASSET_BINARIES_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/binaryAssets)

set(PREPENDED_ASSETS_DIR ${CMAKE_CURRENT_BINARY_DIR}/prependedAssets)
set(ASSET_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/assets.hpp")

set(EXTERN_DECLARATIONS "")

if(EXISTS ${ASSETS_DIRECTORY})
    file(MAKE_DIRECTORY ${ASSET_BINARIES_DIRECTORY})
    file(MAKE_DIRECTORY ${PREPENDED_ASSETS_DIR})

    file(GLOB_RECURSE ASSETS RELATIVE ${ASSETS_DIRECTORY} "${ASSETS_DIRECTORY}/*")
    set(BINARY_ASSET_FILES "")

    set(ASSET_DECLARATIONS "")
    set(INDENT_STRING "    ")

    foreach(REL_FILE IN LISTS ASSETS)
        set(SRC_FILE "${ASSETS_DIRECTORY}/${REL_FILE}")

        if(IS_DIRECTORY ${SRC_FILE})
            continue()
        endif()

        file(SIZE "${SRC_FILE}" FILE_SIZE)

        message("-- Including asset: ${REL_FILE}")

        get_filename_component(REL_DIR ${REL_FILE} DIRECTORY)
        if(NOT REL_DIR STREQUAL "")
            file(MAKE_DIRECTORY "${PREPENDED_ASSETS_DIR}/${REL_DIR}")
        endif()

        add_custom_command(
            OUTPUT "${PREPENDED_ASSETS_DIR}/${REL_FILE}"
            COMMAND ${CMAKE_COMMAND} -E echo_append "                                " > "${PREPENDED_ASSETS_DIR}/${REL_FILE}"
            COMMAND ${CMAKE_COMMAND} -E cat "${SRC_FILE}" >> "${PREPENDED_ASSETS_DIR}/${REL_FILE}"
            COMMAND ${CMAKE_COMMAND} -E echo_append " " >> "${PREPENDED_ASSETS_DIR}/${REL_FILE}"
            DEPENDS "${SRC_FILE}"
        )

        if(NOT REL_DIR STREQUAL "")
            file(MAKE_DIRECTORY "${ASSET_BINARIES_DIRECTORY}/${REL_DIR}")
        endif()

        set(OUTPUT_OBJ "${ASSET_BINARIES_DIRECTORY}/${REL_FILE}.o")

        add_custom_command(
            OUTPUT "${OUTPUT_OBJ}"
            COMMAND ${CMAKE_OBJCOPY} "${REL_FILE}" "${OUTPUT_OBJ}" --input-target binary --output-target elf64-aarch64 --set-section-flags binary=strings
            DEPENDS "${PREPENDED_ASSETS_DIR}/${REL_FILE}"
            WORKING_DIRECTORY "${PREPENDED_ASSETS_DIR}"
        )
        list(APPEND BINARY_ASSET_FILES "${OUTPUT_OBJ}")

        string(REGEX REPLACE "[^a-zA-Z0-9]" "_" FULL_SYMBOL ${REL_FILE})

        get_filename_component(FILE_NAME ${REL_FILE} NAME)
        string(REGEX REPLACE "[^a-zA-Z0-9]" "_" LOCAL_NAME ${FILE_NAME})

        set(EXTERN_DECLARATIONS "${EXTERN_DECLARATIONS}extern \"C\" uint8_t _binary_${FULL_SYMBOL}_start[];\n")
        set(EXTERN_DECLARATIONS "${EXTERN_DECLARATIONS}extern \"C\" uint8_t _binary_${FULL_SYMBOL}_end[];\n")
        set(EXTERN_DECLARATIONS "${EXTERN_DECLARATIONS}extern \"C\" uint8_t _binary_${FULL_SYMBOL}_size[];\n")

        set(INDENT "")
        set(NAMESPACE_OPEN "")
        set(NAMESPACE_CLOSE "")

        if(NOT REL_DIR STREQUAL "")
            string(REPLACE "/" ";" DIR_LIST ${REL_DIR})

            foreach(DIR IN LISTS DIR_LIST)
                string(REGEX REPLACE "[^a-zA-Z0-9]" "_" NS ${DIR})
                set(NAMESPACE_OPEN "${NAMESPACE_OPEN}\n${INDENT}namespace ${NS} {")
                set(NAMESPACE_CLOSE "${INDENT}}  // namespace ${NS}\n${NAMESPACE_CLOSE}")
                set(INDENT "${INDENT}${INDENT_STRING}")
            endforeach()
        endif()

        set(ASSET_DECLARATION "${NAMESPACE_OPEN}
${INDENT}/**
${INDENT} * Binary asset representing the file \"${REL_FILE}\".
${INDENT} *
${INDENT} * This asset was embedded as a binary resource.
${INDENT} */
${INDENT}const IncludedAsset ${LOCAL_NAME} {
${INDENT}    ::IncludedAssets::__AssetExterns__::_binary_${FULL_SYMBOL}_start,
${INDENT}    ::IncludedAssets::__AssetExterns__::_binary_${FULL_SYMBOL}_end
${INDENT}};
${NAMESPACE_CLOSE}")

        set(ASSET_DECLARATIONS "${ASSET_DECLARATIONS}${ASSET_DECLARATION}")
    endforeach()

    string(REPLACE "\n" "\n${INDENT_STRING}" ASSET_DECLARATIONS "${ASSET_DECLARATIONS}")

    string(STRIP "${ASSET_DECLARATIONS}" ASSET_DECLARATIONS)

    string(REPLACE "\n" "\n${INDENT_STRING}${INDENT_STRING}" EXTERN_DECLARATIONS "${EXTERN_DECLARATIONS}")

    string(STRIP "${EXTERN_DECLARATIONS}" EXTERN_DECLARATIONS)

    set(ASSET_HEADER_DATA "#pragma once

struct IncludedAsset {
    IncludedAsset(uint8_t* start, uint8_t* end) : array(reinterpret_cast<Array<uint8_t>*>(start)) {
        array->klass = nullptr;
        array->monitor = nullptr;
        array->bounds = nullptr;
        array->max_length = end - start - 32;
        *(end - 1) = '\\0';
    }

    operator ArrayW<uint8_t>() const {
        init();
        return array;
    }

    operator std::string_view() const {
        return {reinterpret_cast<char*>(array->_values), array->get_Length()};
    }

    operator std::span<uint8_t>() const {
        return {array->_values, array->get_Length()};
    }

    void init() const {
        if (!array->klass) {
            array->klass = classof(Array<uint8_t>*);
        }
    }

   private:
    Array<uint8_t>* array;
};

/**
 * Namespace containing all embedded assets.
 */
namespace IncludedAssets {
    /**
    * @brief Contains raw asset symbols generated by llvm-objcopy.
    * This namespace is private and should not be used directly.
    */
    namespace __AssetExterns__ {
        ${EXTERN_DECLARATIONS}
    }  // namespace __AssetExterns__

    ${ASSET_DECLARATIONS}
}  // namespace IncludedAssets
")

    list(LENGTH BINARY_ASSET_FILES COUNT)
    if(${COUNT} GREATER 0)
        if(EXISTS ${ASSET_HEADER_PATH})
            file(READ ${ASSET_HEADER_PATH} CURRENT_ASSET_HEADER)
        else()
            set(CURRENT_ASSET_HEADER "")
        endif()

        if(NOT "${CURRENT_ASSET_HEADER}" STREQUAL "${ASSET_HEADER_DATA}")
            message("-- Writing '${ASSET_HEADER_PATH}'")
            file(WRITE ${ASSET_HEADER_PATH} "${ASSET_HEADER_DATA}")
        else()
            message("-- '${ASSET_HEADER_PATH}' is up to date.")
        endif()

        add_library(asset_files OBJECT ${BINARY_ASSET_FILES})
        set_target_properties(asset_files PROPERTIES LINKER_LANGUAGE CXX)
        target_link_libraries(${COMPILE_ID} PRIVATE asset_files ${BINARY_ASSET_FILES})
    else()
        if(EXISTS ${ASSET_HEADER_PATH})
            message("-- Removing '${ASSET_HEADER_PATH}' as no assets have been found in '${ASSETS_DIRECTORY}'")
            file(REMOVE ${ASSET_HEADER_PATH})
        endif()
    endif()
else()
    if(EXISTS ${ASSET_HEADER_PATH})
        message("-- Removing '${ASSET_HEADER_PATH}' as the directory '${ASSETS_DIRECTORY}' does not exist")
        file(REMOVE ${ASSET_HEADER_PATH})
    endif()
endif()
