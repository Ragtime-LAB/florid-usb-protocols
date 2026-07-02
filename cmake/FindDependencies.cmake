if(NOT TARGET rpl::rpl)
    find_package(rpl QUIET)
endif()

if(NOT TARGET rpl::rpl)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rpl ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/rpl)
endif()

if(NOT TARGET unordered_dense::unordered_dense)
    include(FetchContent)
    FetchContent_Declare(unordered_dense
            GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
            GIT_TAG v4.8.1
            GIT_SHALLOW true
    )
    FetchContent_MakeAvailable(unordered_dense)
endif()
