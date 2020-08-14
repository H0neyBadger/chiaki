
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#find_package(SDL2 CONFIG REQUIRED)
find_library(SDL2 SDL2)
find_library(SDL2_TTF SDL2_ttf)
find_library(AVCODEC avcodec)
find_library(AVUTIL avutil)
find_library(SWSCALE swscale)
find_library(VPX vpx)
find_library(SWRESAMPLE swresample)

if(CHIAKI_SWITCH_ENABLE_LINUX AND CHIAKI_SWITCH_ENABLE_OPENGL)
	set(RESOURCE_FILES src/glad.c)
else()
	set(RESOURCE_FILES "")
endif()

# find -type f | grep -P '\.(h|cpp)$' | sed 's#\./#\t\t#g'
add_executable(chiaki WIN32
	${RESOURCE_FILES}
	src/ui/customdialog.cpp
	src/ui/clickableimage.cpp
	src/discoverymanager.cpp
	src/settings.cpp
	src/io.cpp
	src/host.cpp
	src/main.cpp
	src/gui.cpp
)

target_include_directories(chiaki PRIVATE include)

# WARNING lib order matter
target_link_libraries(chiaki chiaki-lib)
target_link_libraries(chiaki ${SDL2} ${AVCODEC} ${AVUTIL} ${VPX} ${SWRESAMPLE})
if(CHIAKI_SWITCH_ENABLE_OPENGL)
	add_definitions(-DCHIAKI_SWITCH_ENABLE_OPENGL)
	if(CHIAKI_SWITCH_ENABLE_LINUX)
		find_library(DL dl)
		target_link_libraries(chiaki ${DL})
	else()
		#  -lSDL2 -lavcodec -lavutil -lswscale -lglad -lEGL -lglapi -ldrm_nouveau -lnx
		# use libnx last
		find_library(GLAD glad)
		target_link_libraries(chiaki ${GLAD})
	endif()
else()
	# requires swscale only
	target_link_libraries(chiaki ${SWSCALE})
endif()

# OS links
if(CHIAKI_SWITCH_ENABLE_LINUX)
	target_link_libraries(chiaki dl)
else()
	# libnx is forced by the switch toolchain
	find_library(FREETYPE freetype)
	find_library(BZ2 bz2)
	find_library(GLAPI glapi)
	find_library(DRM_NOUVEAU drm_nouveau)
	target_link_libraries(chiaki plutonium ${SDL2_TTF} ${FREETYPE} ${BZ2} ${GLAPI} ${DRM_NOUVEAU})

endif()

install(TARGETS chiaki
	RUNTIME DESTINATION bin
	BUNDLE DESTINATION bin)

if(CHIAKI_ENABLE_SWITCH AND NOT CHIAKI_SWITCH_ENABLE_LINUX)
	add_nro_target(chiaki "chiaki" "Chiaki team" "${CHIAKI_VERSION}" "${CMAKE_SOURCE_DIR}/switch/res/icon.jpg" "${CMAKE_SOURCE_DIR}/switch/res")
endif()
