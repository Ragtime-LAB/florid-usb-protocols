find_package(astrial QUIET)
if (NOT astrial_FOUND AND NOT TARGET astrial)
    include(FetchContent)
    FetchContent_Declare(astrial
            GIT_REPOSITORY https://github.com/starwey604/astrial
            GIT_TAG main
            GIT_SHALLOW true
    )
    FetchContent_MakeAvailable(astrial)
endif ()