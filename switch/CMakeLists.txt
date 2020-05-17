
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(FFMPEG REQUIRED COMPONENTS avcodec avutil swscale)
find_package(SDL2 REQUIRED)

if(CHIAKI_SWITCH_ENABLE_LINUX AND CHIAKI_SWITCH_ENABLE_OPENGL)
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
target_link_libraries(chiaki SDL2 EGL FFMPEG::avcodec FFMPEG::avutil)
if(CHIAKI_SWITCH_ENABLE_OPENGL)
	add_definitions(-DCHIAKI_SWITCH_ENABLE_OPENGL)
	if(CHIAKI_SWITCH_ENABLE_LINUX)
		target_link_libraries(chiaki dl)
	else()
		#  -lSDL2 -lavcodec -lavutil -lswscale -lglad -lEGL -lglapi -ldrm_nouveau -lnx
		# use libnx last
		target_link_libraries(chiaki glad)
	endif()
else()
	target_link_libraries(chiaki FFMPEG::swscale)
endif()

# OS links
if(CHIAKI_SWITCH_ENABLE_LINUX)
	target_link_libraries(chiaki dl)
else()
	# use libnx last
	target_link_libraries(chiaki glapi drm_nouveau nx m)
endif()

install(TARGETS chiaki
	RUNTIME DESTINATION bin
	BUNDLE DESTINATION bin)

if(CHIAKI_ENABLE_SWITCH AND NOT CHIAKI_SWITCH_ENABLE_LINUX)
	add_nro_target(chiaki "chiaki" "Chiaki team" "${CHIAKI_VERSION}" "${CMAKE_SOURCE_DIR}/switch/res/icon.jpg" "${CMAKE_SOURCE_DIR}")
endif()
