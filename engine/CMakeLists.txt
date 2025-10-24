set(SFML_COMPONENTS Graphics Window System Audio)

# First attempt to find system installation
find_package(SFML 3 COMPONENTS ${SFML_COMPONENTS})


if(SFML_FOUND)
    message(STATUS "Found system installation of SFML in ${SFML_DIR}")
else()
    message(STATUS "System installation of SFML not found, looking for install in extern 
                    by appending ${CMAKE_SOURCE_DIR}/extern/SFML-3.0.0 to CMAKE_PREFIX_PATH...")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/extern/SFML-3.0.0")
    find_package(SFML 3 REQUIRED COMPONENTS ${SFML_COMPONENTS})

    set(CMAKE_INSTALL_RPATH "$ORIGIN/../../extern/SFML-3.0.0/lib")

    set(LOCAL_INSTALL TRUE)
endif()


add_library(ECS ECSEngine.h EntityManager.h EntityManager.cpp Entity.cpp Entity.h ComponentStorage.h MathUtil.h SpriteManager.h SpriteManager.cpp CameraComponent.h InputComponent.h LocationComponent.h CollisionComponent.h SpriteComponent.h SoundManager.cpp SoundManager.h WindowManager.cpp WindowManager.h SpawnComponent.h ScoreComponent.h )
add_executable(ecsp1 main.cpp)


target_include_directories(ECS INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ecsp1 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(ECS PUBLIC /opt/homebrew/include/)
target_compile_features(ECS PRIVATE cxx_std_20)
target_compile_features(ecsp1 PRIVATE cxx_std_20)

if(LOCAL_INSTALL)
    target_link_options(ECS PRIVATE -Wl,--disable-new-dtags)
    target_link_options(ecsp1 PRIVATE -Wl,--disable-new-dtags)
endif()

target_link_libraries(ECS PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)
target_link_libraries(ecsp1 PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)

target_link_libraries(ecsp1 PRIVATE ECS)


# We recommend adding -Werror once you've fixed all unused variables.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic-errors -Wall -Wextra -Wpedantic")

# On Windows, copy SFML DLLs to build directory for convenience
if(WIN32)
    add_custom_command(TARGET ecsp1 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:ecsp1>
            $<TARGET_FILE_DIR:ecsp1>
        COMMAND_EXPAND_LISTS
    )
endif()

#include(CTest)
#enable_testing()

