if(NOT ENABLE_SDL)
  message(STATUS "SDL frontend is disabled.")
  return()
endif()

find_package(SDL2 CONFIG REQUIRED)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/thirdparty/imgui)
set(SDL_SOURCES
  sdlcelestialbrowser.cpp
  sdlcontextmenu.cpp
  sdlinfoviewer.cpp
  sdlmain.cpp
  sdlpreferences.cpp
  sdlui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl.cpp
  ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp)
add_executable(celestia-sdl ${SDL_SOURCES})
target_compile_definitions(celestia-sdl PRIVATE IMGUI_USER_CONFIG="celestia_imconfig.h")
add_dependencies(celestia-sdl celestia)
target_include_directories(celestia-sdl PRIVATE ${IMGUI_DIR})
target_link_libraries(celestia-sdl PRIVATE Freetype::Freetype celestia)
if(TARGET SDL2::SDL2main AND TARGET SDL2::SDL2)
  if(MINGW)
    target_compile_definitions(celestia-sdl PRIVATE SDL_MAIN_HANDLED)
  endif()
  target_link_libraries(celestia-sdl PRIVATE SDL2::SDL2main SDL2::SDL2)
else()
  target_include_directories(celestia-sdl PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(celestia-sdl PRIVATE ${SDL2_LIBRARIES})
endif()
install(TARGETS celestia-sdl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
