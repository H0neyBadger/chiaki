
##################
# nanopb
##################

find_package(PythonInterp 3 REQUIRED) # Make sure nanopb doesn't find Python 2.7 because Python 2 should just die.

add_subdirectory(nanopb EXCLUDE_FROM_ALL)
set(NANOPB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/nanopb")
set(NANOPB_SOURCE_DIR "${NANOPB_SOURCE_DIR}" PARENT_SCOPE)
set(NANOPB_GENERATOR_PY "${NANOPB_SOURCE_DIR}/generator/nanopb_generator.py" PARENT_SCOPE)

##################
# gf-complete
##################

set(GF_COMPLETE_SOURCE
		gf-complete/src/gf.c
		gf-complete/src/gf_wgen.c
		gf-complete/src/gf_w4.c
		gf-complete/src/gf_w8.c
		gf-complete/src/gf_w16.c
		gf-complete/src/gf_w32.c
		gf-complete/src/gf_w64.c
		gf-complete/src/gf_w128.c
		gf-complete/src/gf_rand.c
		gf-complete/src/gf_general.c
		gf-complete/src/gf_cpu.c)

# TODO: support NEON

add_library(gf_complete STATIC ${GF_COMPLETE_SOURCE})
target_include_directories(gf_complete PUBLIC gf-complete/include)

##################
# jerasure
##################

set(JERASURE_SOURCE
		jerasure/src/galois.c
		jerasure/src/jerasure.c
		jerasure/src/reed_sol.c
		jerasure/src/cauchy.c
		jerasure/src/liberation.c)

add_library(jerasure STATIC ${JERASURE_SOURCE})
target_include_directories(jerasure PUBLIC jerasure/include)
target_link_libraries(jerasure gf_complete)

if(CHIAKI_ENABLE_SWITCH AND NOT CHIAKI_SWITCH_ENABLE_LINUX)
	set(PLUTONIUM_SOURCE
			Plutonium/Plutonium/Source/pu/audio/audio_Music.cpp
			Plutonium/Plutonium/Source/pu/audio/audio_Sfx.cpp
			Plutonium/Plutonium/Source/pu/pu_String.cpp
			Plutonium/Plutonium/Source/pu/sdl2/ttf_SDL_ttf.c
			Plutonium/Plutonium/Source/pu/ttf/ttf_Font.cpp
			Plutonium/Plutonium/Source/pu/ui/render/render_SDL2.cpp
			Plutonium/Plutonium/Source/pu/ui/render/render_Renderer.cpp
			Plutonium/Plutonium/Source/pu/ui/ui_Types.cpp
			Plutonium/Plutonium/Source/pu/ui/extras/extras_Toast.cpp
			Plutonium/Plutonium/Source/pu/ui/elm/elm_Rectangle.cpp
			Plutonium/Plutonium/Source/pu/ui/elm/elm_Element.cpp
			Plutonium/Plutonium/Source/pu/ui/elm/elm_Toggle.cpp
			Plutonium/Plutonium/Source/pu/ui/elm/elm_ProgressBar.cpp
			Plutonium/Plutonium/Source/pu/ui/elm/elm_Button.cpp
			Plutonium/Plutonium/Source/pu/ui/elm/elm_Menu.cpp
			Plutonium/Plutonium/Source/pu/ui/elm/elm_Image.cpp
			Plutonium/Plutonium/Source/pu/ui/elm/elm_TextBlock.cpp
			Plutonium/Plutonium/Source/pu/ui/ui_Layout.cpp
			Plutonium/Plutonium/Source/pu/ui/ui_Overlay.cpp
			Plutonium/Plutonium/Source/pu/ui/ui_Dialog.cpp
			Plutonium/Plutonium/Source/pu/ui/ui_Container.cpp
			Plutonium/Plutonium/Source/pu/ui/ui_Application.cpp)
	add_library(plutonium STATIC ${PLUTONIUM_SOURCE})
	# workaroud to include ft2build.h
	target_include_directories(plutonium PRIVATE ${PORTLIBS_PREFIX}/include/freetype2)
	target_include_directories(plutonium PUBLIC Plutonium/Plutonium/Include)

	find_library(FREETYPE freetype)
	find_library(BZ2 bz2)
	find_library(Z z)
	# find_package(sdl2 CONFIG REQUIRED)
	find_library(SDL2_MIXER SDL2_mixer)
	find_library(OPUSFILE opusfile)
	find_library(OPUS opus)
	find_library(MODPLUG modplug)
	find_library(MPG123 mpg123)
	find_library(VORBISIDEC vorbisidec)
	find_library(OGG ogg)
	find_library(SDL2_TTF SDL2_ttf)
	find_library(SDL2_GFX SDL2_gfx)
	find_library(SDL2_IMAGE SDL2_image)
	find_library(SDL2 SDL2)
	find_library(EGL EGL)
	find_library(GLESV2 GLESv2)
	find_library(GLAPI glapi)
	find_library(DRM_NOUVEAU drm_nouveau)
	find_library(WEBP webp)
	find_library(PNG png)
	find_library(JPEG jpeg)
	target_link_libraries(plutonium
		${FREETYPE}
		${BZ}
		${Z}
		${SDL2_MIXER}
		${OPUSFILE}
		${OPUS}
		${MODPLUG}
		${MPG123}
		${VORBISIDEC}
		${OGG}
		${SDL2_TTF}
		${SDL2_GFX}
		${SDL2_IMAGE}
		${SDL2}
		${EGL}
		${GLESV2}
		${GLAPI}
		${DRM_NOUVEAU}
		${WEBP}
		${PNG}
		${JPEG})
endif()

