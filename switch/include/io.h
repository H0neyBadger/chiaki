/*
 * This file is part of Chiaki.
 *
 * Chiaki is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chiaki is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Chiaki.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CHIAKI_IO_H
#define CHIAKI_IO_H

#include <cstdint>
#include <SDL2/SDL.h>

#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions
#include <glad/glad.h>  // glad library (OpenGL loader)

#include <glm/mat4x4.hpp>
//#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


/*
	https://github.com/devkitPro/switch-glad/blob/master/include/glad/glad.h
	https://glad.dav1d.de/#profile=core&language=c&specification=gl&api=gl%3D4.3&extensions=GL_EXT_texture_compression_s3tc&extensions=GL_EXT_texture_filter_anisotropic

	Language/Generator: C/C++
	Specification: gl
	APIs: gl=4.3
	Profile: core
	Extensions:
		GL_EXT_texture_compression_s3tc,
		GL_EXT_texture_filter_anisotropic
	Loader: False
	Local files: False
	Omit khrplatform: False
	Reproducible: False
*/

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <chiaki/log.h>
#include <chiaki/controller.h>

#include "exception.h"

#define PLANES_COUNT 3

class IO {
	private:
		ChiakiLog * log;
		bool init_sdl;
		int video_width;
		int video_height;
		// opengl reader writer
		int cargo = 0;
		// default nintendo switch res
		int screen_width = 1280;
		int screen_height = 720;
		GLuint vao;
		GLuint vbo;
		GLuint tex[PLANES_COUNT];
		GLuint pbo[PLANES_COUNT];
		GLuint vert;
		GLuint frag;
		GLuint prog;
		AVCodecContext * codec_context;
		AVCodec * codec;
		SDL_Window * sdl_window;
		SDL_GLContext sdl_gl_context;
		SDL_AudioDeviceID sdl_audio_device_id;
		SDL_Event sdl_event;
	private:
		bool InitSDLWindow();
		bool FreeSDLWindow();
		bool InitAVCodec();
		bool InitOpenGl();
		bool InitOpenGlTextures();
		bool InitOpenGlShader();
		GLuint CreateAndCompileShader(GLenum type, const char* source);
		void SetOpenGlYUVPixels(AVFrame * frame);
		void OpenGlDraw(int x, int y, int w, int h);
		bool ReadGameKeys(SDL_Event * event, ChiakiControllerState * state);
		void SetMesaConfig();
		void CheckGLError(const char* func, const char* file, int line);
		void CheckSDLError(const char* func, const char* file, int line);
	public:
		IO(ChiakiLog * log);
		~IO();
		bool VideoCB(uint8_t * buf, size_t buf_size);
		void InitAudioCB(unsigned int channels, unsigned int rate);
		void AudioCB(int16_t * buf, size_t samples_count);
		bool InitVideo(int video_width, int video_height, int screen_width, int screen_height);
		bool FreeVideo();
		bool InitJoystick();
		bool ReadUserKeyboard(char * buffer, size_t buffer_size);
		bool MainLoop(ChiakiControllerState * state);
		bool OpenGlResizeScreen(int width, int height);
};

#endif //CHIAKI_IO_H


