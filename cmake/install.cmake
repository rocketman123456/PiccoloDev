function(auto_install_target target install_dir)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${install_dir}
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:${target}>
            ${install_dir}/
        COMMENT "Auto-installing ${target} to ${install_dir}"
    )
endfunction()

function(auto_install_binary target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${AUTO_INSTALL_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:${target}>
            ${AUTO_INSTALL_DIR}/bin/
        COMMENT "Auto-installing ${target} to ${AUTO_INSTALL_DIR}/bin"
    )
endfunction()

function(auto_install_library target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${AUTO_INSTALL_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:${target}>
            ${AUTO_INSTALL_DIR}/lib/
        COMMENT "Auto-installing ${target} to ${AUTO_INSTALL_DIR}/lib"
    )
endfunction()
