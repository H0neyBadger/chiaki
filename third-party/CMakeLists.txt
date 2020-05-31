
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
	target_include_directories(plutonium PUBLIC Plutonium/Plutonium/Include)
	# /opt/devkitpro/portlibs/switch/bin/sdl2-config --libs
	# -L/opt/devkitpro/portlibs/switch/lib -lSDL2 -march=armv8-a -fPIE -L/opt/devkitpro/libnx/lib -lEGL -lglapi -ldrm_nouveau -lnx
	# /opt/devkitpro/portlibs/switch/bin/freetype-config --libs
	# -L/opt/devkitpro/portlibs/switch/lib -lfreetype -lbz2 -L/opt/devkitpro/portlibs/switch/lib -L/opt/devkitpro/portlibs/switch/lib -lz -lpng16 -lm -lnx -lz
	target_link_libraries(plutonium
			freetype
			SDL2_mixer
			opusfile
			opus
			modplug
			mpg123
			vorbisidec
			ogg
			SDL2_ttf
			SDL2_gfx
			SDL2_image
			SDL2
			EGL
			GLESv2
			glapi
			drm_nouveau
			webp
			png
			jpeg
			nx)
endif()

