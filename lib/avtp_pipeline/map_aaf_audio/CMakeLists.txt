SET (SRC_FILES ${SRC_FILES}
	${AVB_SRC_DIR}/map_aaf_audio/openavb_map_aaf_audio.c
	PARENT_SCOPE
)

# Add Open1722 enhanced mapping if enabled
if(OPENAVNU_USE_OPEN1722)
	SET (SRC_FILES ${SRC_FILES}
		${AVB_SRC_DIR}/map_aaf_audio/openavb_map_aaf_audio_open1722.c
		PARENT_SCOPE
	)
	
	# Add test for Open1722 integration
	if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
		# Only add test if we're building this specific module
		add_executable(test-aaf-open1722-integration
			test_aaf_open1722_integration.c
			openavb_map_aaf_audio_open1722.c
		)
		
		target_link_libraries(test-aaf-open1722-integration 
			openavnu-avtp-open1722
		)
		
		target_include_directories(test-aaf-open1722-integration PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}/..
			${CMAKE_CURRENT_SOURCE_DIR}/../include
			${CMAKE_CURRENT_SOURCE_DIR}/../util
			${CMAKE_CURRENT_SOURCE_DIR}/../avtp_open1722
		)
	endif()
	
	message(STATUS "AAF mapping: Enhanced Open1722 support enabled")
else()
	message(STATUS "AAF mapping: Using legacy implementation")
endif()

