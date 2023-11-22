############################## project name #################################
project(myUtils)
########################### cmake version need ##############################
cmake_minimum_required(VERSION 3.0)
########################## project directories ##############################
set(UTILS_ROOT .)
set(THIRD_PARTY_ROOT ${UTILS_ROOT}/../../third_party)
################################ src files ##################################
set(SOURCES
    myCommonUtil.c
    myModuleCommon.c
    CmdLine/myCmdLine.c
    Health/myModuleHealth.c
    Log/myLogIO.c
    Mem/myMem.c
    Msg/myMsg.c
    TPool/myThreadPool.c
    Timer/myTimer.c
    SafetyManager/mySafetyManager.c
)
############################# include dir ###################################
include_directories(
    ${UTILS_ROOT}/../include
    ${THIRD_PARTY_ROOT}/libevent/include
    ${THIRD_PARTY_ROOT}/GmSSL/include
)
############################ target rules ###################################
# lib options
option(BUILD_SHARED_LIBS "Build shared library" OFF)
if (BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
else ()
    add_library(${PROJECT_NAME} STATIC ${SOURCES})
endif ()
# unittest options
option(BUILD_TESTS "Build unit tests" OFF)
if (BUILD_TESTS)
    # UnitTest Src Files
    set(CMAKE_CXX_COMPILER g++)
    set(TEST_SOURCES
        UnitTest/UnitTestMain.c
        UnitTest/UnitTestTPool.c
        UnitTest/UnitTestMsg.c
        UnitTest/UnitTestMem.c
        UnitTest/UnitTestHealth.c
        UnitTest/UnitTestTimer.c
        UnitTest/UnitTestCommonUtil.c
    )
    include_directories(
        ${THIRD_PARTY_ROOT}/googletest/include
    )
    # add binary file rule
    add_executable(UnitTest ${TEST_SOURCES})
    # set g++
    set_source_files_properties(${TEST_SOURCES} PROPERTIES LANGUAGE CXX)    

    # link libs
    target_link_directories(UnitTest PRIVATE 
        ${THIRD_PARTY_ROOT}/googletest/lib
        ${THIRD_PARTY_ROOT}/libevent/lib
    )
    target_link_libraries(UnitTest PRIVATE 
        ${PROJECT_NAME}
        gtest
        event
        pthread
    )

    # enable test
    enable_testing()
    add_test(NAME UnitTests COMMAND UnitTest)

    # break when unitest fail
    set_tests_properties(UnitTests PROPERTIES WILL_FAIL TRUE)
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure)
    add_dependencies(check UnitTest)
    add_custom_command(TARGET check POST_BUILD COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Running tests..." COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure)
endif ()

