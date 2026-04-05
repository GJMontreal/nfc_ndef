cmake_minimum_required(VERSION 3.14)
project(nfc_ndef C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---- Library ----------------------------------------------------------------

add_library(nfc_ndef STATIC
    src/st25dv.c
    src/ndef.c
    src/ndef_tlv.c
    src/ndef_text.c
)
target_include_directories(nfc_ndef PUBLIC include)
target_compile_options(nfc_ndef PRIVATE -Wall -Wextra -Wpedantic)

# ---- Tools ------------------------------------------------------------------

add_executable(dump_parse tools/dump_parse.c)
target_link_libraries(dump_parse PRIVATE nfc_ndef)

# ---- Tests ------------------------------------------------------------------

include(CTest)
enable_testing()

include(FetchContent)
FetchContent_Declare(unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.5.2
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(unity)

# Unity's CMakeLists does not export a linkable target in all versions;
# create one from the fetched sources if it wasn't defined.
if(NOT TARGET unity)
    add_library(unity STATIC ${unity_SOURCE_DIR}/src/unity.c)
    target_include_directories(unity PUBLIC ${unity_SOURCE_DIR}/src)
endif()

function(add_nfc_test name)
    add_executable(${name} test/${name}.c)
    target_link_libraries(${name} PRIVATE nfc_ndef unity)
    target_include_directories(${name} PRIVATE test)
    target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_nfc_test(test_ndef)
add_nfc_test(test_ndef_tlv)
add_nfc_test(test_ndef_text)

add_executable(test_st25dv test/test_st25dv.c test/mock_hal.c)
target_link_libraries(test_st25dv PRIVATE nfc_ndef unity)
target_include_directories(test_st25dv PRIVATE test)
target_compile_options(test_st25dv PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME test_st25dv COMMAND test_st25dv)
