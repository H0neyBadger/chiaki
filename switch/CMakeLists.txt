
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(FFMPEG REQUIRED COMPONENTS avcodec avutil swscale)
find_package(SDL2 REQUIRED)

if(CHIAKI_ENABLE_SWITCH_LINUX)
	set(RESOURCE_FILES src/glad.c)
else()
	set(RESOURCE_FILES "")
endif()

# find -type f | grep -P '\.(h|cpp)$' | sed 's#\./#\t\t#g'
add_executable(chiaki WIN32
	${RESOURCE_FILES}
	src/discoverymanager.cpp
	src/settings.cpp
	src/io.cpp
	src/host.cpp
	src/main.cpp
)

target_include_directories(chiaki PRIVATE include)

# WARNING lib order matter
target_link_libraries(chiaki chiaki-lib)
target_link_libraries(chiaki FFMPEG::avcodec FFMPEG::avutil FFMPEG::swscale)
if(CHIAKI_ENABLE_SWITCH AND NOT CHIAKI_ENABLE_SWITCH_LINUX)
	#  -lSDL2 -lavcodec -lavutil -lswscale -lglad -lEGL -lglapi -ldrm_nouveau -lnx
	# use libnx last
	target_link_libraries(chiaki SDL2 glad EGL glapi drm_nouveau nx m)
else()
	target_link_libraries(chiaki SDL2::SDL2 EGL glapi dl)
endif()

install(TARGETS chiaki
	RUNTIME DESTINATION bin
	BUNDLE DESTINATION bin)

if(CHIAKI_ENABLE_SWITCH AND NOT CHIAKI_ENABLE_SWITCH_LINUX)
	add_nro_target(chiaki "chiaki" "Chiaki team" "${CHIAKI_VERSION}" "${CMAKE_SOURCE_DIR}/switch/res/icon.jpg" "${CMAKE_SOURCE_DIR}")
endif()
