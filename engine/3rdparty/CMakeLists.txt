set(third_party_folder "ThirdParty")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(NOT TARGET imgui)
    include(imgui.cmake)
    set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder}/imgui)
endif()

if(NOT TARGET soloud)
    include(soloud.cmake)
endif()

if(NOT TARGET ImGuiFileDialog)
    add_subdirectory(ImGuiFileDialog)
    target_include_directories(ImGuiFileDialog PUBLIC imgui)
    target_link_libraries(ImGuiFileDialog PUBLIC imgui)
endif()
