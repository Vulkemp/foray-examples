function (foray_example)

    # name project after the folder its located in
    get_filename_component(proj_name ${CMAKE_CURRENT_LIST_DIR} NAME)

    project(${proj_name})

    MESSAGE("--- << CMAKE of ${PROJECT_NAME} >> --- ")
    MESSAGE(STATUS "CURRENT SOURCE DIR \"${CMAKE_CURRENT_SOURCE_DIR}\"")

    # Enable strict mode for own code
    SET(CMAKE_CXX_FLAGS ${STRICT_FLAGS})


    # collect sources
    file(GLOB_RECURSE src "*.cpp")
    
    # Make sure there are source files, add_executable would otherwise fail
    if (NOT src)
        message(WARNING "Project \"${PROJECT_NAME}\" does not contain any source files")
        return()
    endif ()


    # Declare executable
    add_executable(${PROJECT_NAME} ${src})

    
    # Assign Compile Flags
    if (ENABLE_GBUFFER_BENCH)
        target_compile_definitions(${PROJECT_NAME} ENABLE_GBUFFER_BENCH)
    endif()


    # Link foray lib
    target_link_libraries(
    	${PROJECT_NAME}
    	PUBLIC foray
    )

    # Windows requires SDL2 libs linked specifically
    if (WIN32)
    	target_link_libraries(
    		${PROJECT_NAME}
    		PUBLIC ${SDL2_LIBRARIES}
    	)
    endif()


    # Configure include directories
    target_include_directories(
    	${PROJECT_NAME}
    	PUBLIC "../foray/src"
    	PUBLIC "../foray/third_party"
    	PUBLIC ${Vulkan_INCLUDE_DIRS}
    )
endfunction()
