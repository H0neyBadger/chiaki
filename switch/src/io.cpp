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

#ifdef __SWITCH__
#include <switch.h>
#else
#include <iostream>
#endif
#include <mutex>
#include <condition_variable>

#include "io.h"

// source:
// https://github.com/thestr4ng3r/chiaki/blob/master/gui/src/avopenglwidget.cpp
//
// examples :
// https://www.roxlu.com/2014/039/decoding-h264-and-yuv420p-playback
// https://gist.github.com/roxlu/9329339

static std::mutex mtx;
static std::condition_variable produce,consume;

// use OpenGl to decode YUV
// the aim is to spare CPU load on nintendo switch

static const char* shader_vert_glsl = R"glsl(
#version 150 core
in vec2 pos_attr;
out vec2 uv_var;
void main()
{
	uv_var = pos_attr;
	gl_Position = vec4(pos_attr * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);
}
)glsl";

static const char *yuv420p_shader_frag_glsl = R"glsl(
#version 150 core
uniform sampler2D plane1; // Y
uniform sampler2D plane2; // U
uniform sampler2D plane3; // V
in vec2 uv_var;
out vec4 out_color;
void main()
{
	vec3 yuv = vec3(
		(texture(plane1, uv_var).r - (16.0 / 255.0)) / ((235.0 - 16.0) / 255.0),
		(texture(plane2, uv_var).r - (16.0 / 255.0)) / ((240.0 - 16.0) / 255.0) - 0.5,
		(texture(plane3, uv_var).r - (16.0 / 255.0)) / ((240.0 - 16.0) / 255.0) - 0.5);
	vec3 rgb = mat3(
		1.0,		1.0,		1.0,
		0.0,		-0.21482,	2.12798,
		1.28033,	-0.38059,	0.0) * yuv;
	out_color = vec4(rgb, 1.0);
}
)glsl";

static const float vert_pos[] = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	1.0f, 1.0f
};

IO::IO(ChiakiLog * log):
#ifndef CHIAKI_SWITCH_ENABLE_OPENGL
	codec(nullptr),
	texture(nullptr),
	pict(nullptr),
	sws_context(nullptr),
#endif
	resize(true),
	log(log),
	codec_context(nullptr),
	sdl_window(nullptr),
	renderer(nullptr)
{
	//TODO
}

IO::~IO(){
	IO::FreeVideo();
}

// #define DEBUG_OPENGL 1

#ifdef CHIAKI_SWITCH_ENABLE_OPENGL
void IO::SetMesaConfig(){
  //TRACE("%s", "Mesaconfig");
  //setenv("MESA_GL_VERSION_OVERRIDE", "3.3", 1);
  //setenv("MESA_GLSL_VERSION_OVERRIDE", "330", 1);
  // Uncomment below to disable error checking and save CPU time (useful for production):
  //setenv("MESA_NO_ERROR", "1", 1);
#ifdef DEBUG_OPENGL
  // Uncomment below to enable Mesa logging:
  setenv("EGL_LOG_LEVEL", "debug", 1);
  setenv("MESA_VERBOSE", "all", 1);
  setenv("NOUVEAU_MESA_DEBUG", "1", 1);

  // Uncomment below to enable shader debugging in Nouveau:
  //setenv("NV50_PROG_OPTIMIZE", "0", 1);
  setenv("NV50_PROG_DEBUG", "1", 1);
  //setenv("NV50_PROG_CHIPSET", "0x120", 1);
#endif
}
#endif

#if defined(DEBUG_OPENGL) && defined(CHIAKI_SWITCH_ENABLE_OPENGL)
#define D(x){ (x); CheckGLError(__func__, __FILE__, __LINE__); }
#define S(x){ (x); CheckSDLError(__func__, __FILE__, __LINE__); }

void IO::CheckGLError(const char* func, const char* file, int line) {
	GLenum err;
	while( (err = glGetError()) != GL_NO_ERROR ){
		CHIAKI_LOGE(this->log, "glGetError: %x function: %s from %s line %d", err, func, file, line);
		//GL_INVALID_VALUE, 0x0501
		// Given when a value parameter is not a legal value for that function. T
		// his is only given for local problems;
		// if the spec allows the value in certain circumstances,
		// where other parameters or state dictate those circumstances,
		// then GL_INVALID_OPERATION is the result instead.
	}
}

void IO::CheckSDLError(const char* func, const char* file, int line) {
	const char * err;
	if(err = SDL_GetError())
		CHIAKI_LOGE(this->log, "SDL_GetError: %s function: %s from %s line %d", err, func, file, line);
}
#else
// do nothing
#define D(x){ (x); }
#define S(x){ (x); }
#endif


bool IO::VideoCB(uint8_t *buf, size_t buf_size){
	// callback function to decode video buffer

	AVPacket packet;
	// av_free_packet(&packet) is deprecated
	// no need to free AVPacket
	av_init_packet(&packet);
	packet.data = buf;
	packet.size = buf_size;
	// FramesAvailable
	AVFrame *frame = av_frame_alloc();
	if(!frame){
		CHIAKI_LOGE(this->log, "UpdateFrame Failed to alloc AVFrame");
		return false;
	}

send_packet:
	// Push
	int r = avcodec_send_packet(this->codec_context, &packet);
	if(r != 0) {
		if(r == AVERROR(EAGAIN)){
			CHIAKI_LOGE(this->log, "AVCodec internal buffer is full removing frames before pushing");
			r = avcodec_receive_frame(this->codec_context, frame);
			// send decoded frame for sdl texture update
			if(r != 0){
				CHIAKI_LOGE(this->log, "Failed to pull frame");
				av_frame_free(&frame);
				return false;
			}
			goto send_packet;
		} else {
			char errbuf[128];
			av_make_error_string(errbuf, sizeof(errbuf), r);
			CHIAKI_LOGE(this->log, "Failed to push frame: %s", errbuf);
			av_frame_free(&frame);
			return false;
		}
	}

	// Pull
	r = avcodec_receive_frame(this->codec_context, frame);
	if(r != 0){
		CHIAKI_LOGE(this->log, "Failed to pull frame");
		av_frame_free(&frame);
		return false;
	}

	std::unique_lock<std::mutex> lck(mtx);
	while(cargo != 0) produce.wait(lck);

	if(frame->width != this->video_width
		|| frame->height != this->video_height ){
		this->video_width = frame->width;
		this->video_height = frame->height;
		this->resize=true;
	}

#ifdef CHIAKI_SWITCH_ENABLE_OPENGL
	// send to OpenGl
	SetOpenGlYUVPixels(frame);
#else
	sws_scale(
		this->sws_context,
		(uint8_t const * const *)frame->data,
		frame->linesize,
		0,
		this->codec_context->height,
		this->pict->data,
		this->pict->linesize
	);
#endif
	cargo = !cargo;
	consume.notify_one();

	av_frame_free(&frame);
	//avcodec_flush_buffers(this->codec_context);
	return true;
}


void IO::InitAudioCB(unsigned int channels, unsigned int rate){
	SDL_AudioSpec want, have, test;
	SDL_memset(&want, 0, sizeof(want));

	//source
	//[I] Audio Header:
	//[I]   channels = 2
	//[I]   bits = 16
	//[I]   rate = 48000
	//[I]   frame size = 480
	//[I]   unknown = 1
	want.freq = rate;
	want.format = AUDIO_S16SYS;
	// 2 == stereo
	want.channels = channels;
	want.samples = 1024;
	want.callback = NULL;

	this->sdl_audio_device_id = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
	if(this->sdl_audio_device_id < 0){
		CHIAKI_LOGE(this->log, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
	} else {
		SDL_PauseAudioDevice(this->sdl_audio_device_id, 0);
	}
}

void IO::AudioCB(int16_t *buf, size_t samples_count){
	//int az = SDL_GetQueuedAudioSize(host->audio_device_id);
	// len the number of bytes (not samples!) to which (data) points
	int success = SDL_QueueAudio(this->sdl_audio_device_id, buf, sizeof(int16_t)*samples_count*2);
	if(success != 0){
		CHIAKI_LOGE(this->log, "SDL_QueueAudio failed: %s\n", SDL_GetError());
	}
}

bool IO::InitVideo(int video_width, int video_height, int screen_width, int screen_height){
	CHIAKI_LOGV(this->log, "load InitVideo");
	this->video_width = video_width;
	this->video_height = video_height;

	this->screen_width = screen_width;
	this->screen_height = screen_height;

	this->init_sdl = InitSDLWindow();
	if(!this->init_sdl){
		throw Exception("Failed to initiate SDL window");
	}

	if(!InitAVCodec()){
		throw Exception("Failed to initiate libav codec");
	}
#ifdef CHIAKI_SWITCH_ENABLE_OPENGL
	SetMesaConfig();
	if(!InitOpenGl()){
		throw Exception("Failed to initiate OpenGl");
	}
	SDL_GL_MakeCurrent(this->sdl_window, NULL);
#else
	if(!InitSDLTextures()){
		throw Exception("Failed to initiate SDLTexture");
	}
#endif
	return true;
}

bool IO::FreeVideo(){
	bool ret = true;

	if(this->init_sdl){
		ret &= FreeSDLWindow();
	}

	SDL_DestroyRenderer(this->renderer);
#ifndef CHIAKI_SWITCH_ENABLE_OPENGL
	//codec(nullptr),
	if(this->pict){
		av_frame_free(&this->pict);
	}
	SDL_DestroyWindow(this->sdl_window);
#endif
	return ret;
}

bool IO::ReadUserKeyboard(char *buffer, size_t buffer_size){
#ifdef CHIAKI_SWITCH_ENABLE_LINUX
	// use cin to get user input from linux
	std::cin.getline(buffer, buffer_size);
	CHIAKI_LOGI(this->log, "Got user input: %s\n", buffer);
#else
	// https://kvadevack.se/post/nintendo-switch-virtual-keyboard/
	SwkbdConfig kbd;
	Result rc = swkbdCreate(&kbd, 0);

	if (R_SUCCEEDED(rc)) {
		swkbdConfigMakePresetDefault(&kbd);
		rc = swkbdShow(&kbd, buffer, buffer_size);

		if (R_SUCCEEDED(rc)) {
			CHIAKI_LOGI(this->log, "Got user input: %s\n", buffer);
		} else {
			CHIAKI_LOGE(this->log, "swkbdShow() error: %u\n", rc);
			return false;
		}
		swkbdClose(&kbd);
	} else {
		CHIAKI_LOGE(this->log, "swkbdCreate() error: %u\n", rc);
		return false;
	}
#endif
	return true;
}



bool IO::ReadGameKeys(SDL_Event *event, ChiakiControllerState *state){
    // return true if an event changed (gamepad input)

	// TODO
	// touchpad touchscreen
	// touchpad button zones
	// share vs PS button
	// Gyro ?
	// rumble ?
	bool ret = true;
	switch(event->type){
		case SDL_JOYAXISMOTION:
			if(event->jaxis.which == 0){
				// left joystick
				if(event->jaxis.axis == 0){
					// Left-right movement
					state->left_x = event->jaxis.value;
				}else if(event->jaxis.axis == 1){
					// Up-Down movement
					state->left_y = event->jaxis.value;
				}else if(event->jaxis.axis == 2){
					// Left-right movement
					state->right_x = event->jaxis.value;
				}else if(event->jaxis.axis == 3){
					// Up-Down movement
					state->right_y = event->jaxis.value;
				} else {
					ret = false;
				}
			} else if (event->jaxis.which == 1) {
				// right joystick
				if(event->jaxis.axis == 0){
					// Left-right movement
					state->right_x = event->jaxis.value;
				}else if(event->jaxis.axis == 1){
					// Up-Down movement
					state->right_y = event->jaxis.value;
				}else{
					ret = false;
				}
			} else {
				ret = false;
			}
			break;
		case SDL_JOYBUTTONDOWN:
			// printf("Joystick %d button %d DOWN\n",
			//	event->jbutton.which, event->jbutton.button);
			switch(event->jbutton.button){
				case 0:  state->buttons |= CHIAKI_CONTROLLER_BUTTON_MOON; break; // KEY_A
				case 1:  state->buttons |= CHIAKI_CONTROLLER_BUTTON_CROSS; break; // KEY_B
				case 2:  state->buttons |= CHIAKI_CONTROLLER_BUTTON_PYRAMID; break; // KEY_X
				case 3:  state->buttons |= CHIAKI_CONTROLLER_BUTTON_BOX; break; // KEY_Y
				case 12: state->buttons |= CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT; break; // KEY_DLEFT
				case 14: state->buttons |= CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT; break; // KEY_DRIGHT
				case 13: state->buttons |= CHIAKI_CONTROLLER_BUTTON_DPAD_UP; break; // KEY_DUP
				case 15: state->buttons |= CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN; break; // KEY_DDOWN
				case 6:  state->buttons |= CHIAKI_CONTROLLER_BUTTON_L1; break; // KEY_L
				case 7:  state->buttons |= CHIAKI_CONTROLLER_BUTTON_R1; break; // KEY_R
				case 8:  state->l2_state = 0xff; break; // KEY_ZL
				case 9:  state->r2_state = 0xff; break; // KEY_ZR
				case 4:  state->buttons |= CHIAKI_CONTROLLER_BUTTON_L3; break; // KEY_LSTICK
				case 5:  state->buttons |= CHIAKI_CONTROLLER_BUTTON_R3; break; // KEY_RSTICK
				case 10: state->buttons |= CHIAKI_CONTROLLER_BUTTON_OPTIONS; break; // KEY_PLUS
				// case 11: state->buttons |= CHIAKI_CONTROLLER_BUTTON_SHARE; break; // KEY_MINUS
				case 11: state->buttons |= CHIAKI_CONTROLLER_BUTTON_PS; break; // KEY_MINUS
				//case KEY_??: state->buttons |= CHIAKI_CONTROLLER_BUTTON_PS; break;
				//FIXME CHIAKI_CONTROLLER_BUTTON_TOUCHPAD
				default:
					ret = false;
			}
			break;
		case SDL_JOYBUTTONUP:
			// printf("Joystick %d button %d UP\n",
			//	event->jbutton.which, event->jbutton.button);
			switch(event->jbutton.button){
				case 0:  state->buttons ^= CHIAKI_CONTROLLER_BUTTON_MOON; break; // KEY_A
				case 1:  state->buttons ^= CHIAKI_CONTROLLER_BUTTON_CROSS; break; // KEY_B
				case 2:  state->buttons ^= CHIAKI_CONTROLLER_BUTTON_PYRAMID; break; // KEY_X
				case 3:  state->buttons ^= CHIAKI_CONTROLLER_BUTTON_BOX; break; // KEY_Y
				case 12: state->buttons ^= CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT; break; // KEY_DLEFT
				case 14: state->buttons ^= CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT; break; // KEY_DRIGHT
				case 13: state->buttons ^= CHIAKI_CONTROLLER_BUTTON_DPAD_UP; break; // KEY_DUP
				case 15: state->buttons ^= CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN; break; // KEY_DDOWN
				case 6:  state->buttons ^= CHIAKI_CONTROLLER_BUTTON_L1; break; // KEY_L
				case 7:  state->buttons ^= CHIAKI_CONTROLLER_BUTTON_R1; break; // KEY_R
				case 8:  state->l2_state = 0x00; break; // KEY_ZL
				case 9:  state->r2_state = 0x00; break; // KEY_ZR
				case 4:  state->buttons ^= CHIAKI_CONTROLLER_BUTTON_L3; break; // KEY_LSTICK
				case 5:  state->buttons ^= CHIAKI_CONTROLLER_BUTTON_R3; break; // KEY_RSTICK
				case 10: state->buttons ^= CHIAKI_CONTROLLER_BUTTON_OPTIONS; break; // KEY_PLUS
				//case 11: state->buttons ^= CHIAKI_CONTROLLER_BUTTON_SHARE; break; // KEY_MINUS
				case 11: state->buttons ^= CHIAKI_CONTROLLER_BUTTON_PS; break; // KEY_MINUS
				//case KEY_??: state->buttons |= CHIAKI_CONTROLLER_BUTTON_PS; break;
				//FIXME CHIAKI_CONTROLLER_BUTTON_TOUCHPAD
				default:
					ret = false;
			}
			break;
		case SDL_FINGERDOWN:
            state->buttons |= CHIAKI_CONTROLLER_BUTTON_TOUCHPAD; // touchscreen
            break;
        case SDL_FINGERUP:
            state->buttons ^= CHIAKI_CONTROLLER_BUTTON_TOUCHPAD; // touchscreen
            break;
		default:
			ret = false;
		}
	return ret;
}

bool IO::InitSDLWindow(){
	// https://github.com/Dav1dde/glad/blob/master/example/c%2B%2B/sdl.cpp
	CHIAKI_LOGI(this->log, "Loading SDL2 window");
	// build sdl graphical interface

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER)) {
		CHIAKI_LOGE(this->log, "SDL initialization failed: %s", SDL_GetError());
		return false;
	}

	if(!this->sdl_window){
		this->sdl_window = SDL_CreateWindow("Chiaki Stream",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			this->screen_width,
			this->screen_height,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	}

	if (!this->sdl_window) {
		CHIAKI_LOGE(this->log, "SDL_CreateWindow failed: %s", SDL_GetError());
		return false;
	}

   	return true;
}

bool IO::FreeSDLWindow(){
#ifdef CHIAKI_SWITCH_ENABLE_OPENGL
	SDL_GL_DeleteContext(this->sdl_gl_context);
#endif
	SDL_DestroyWindow(this->sdl_window);
	SDL_Quit();
	return true;
}

bool IO::InitAVCodec(){
	CHIAKI_LOGV(this->log, "loading AVCodec");
	// set libav video context
	this->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if(!this->codec)
		throw Exception("H264 Codec not available");

	this->codec_context = avcodec_alloc_context3(codec);
	if(!this->codec_context)
		throw Exception("Failed to alloc codec context");

	if(avcodec_open2(this->codec_context, this->codec, nullptr) < 0)
	{
		avcodec_free_context(&this->codec_context);
		throw Exception("Failed to open codec context");
	}
	return true;
}

#ifdef CHIAKI_SWITCH_ENABLE_OPENGL
bool IO::InitOpenGl(){
	CHIAKI_LOGV(this->log, "loading OpenGL");

	// Set SDL OpenGl context
	S(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE));
	S(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
	S(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3));

	//S(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));
	S(SDL_GL_SetSwapInterval(1));
	S(SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1));

	this->sdl_gl_context = SDL_GL_CreateContext(this->sdl_window);
	if(this->sdl_gl_context == NULL){
		CHIAKI_LOGE(this->log, "SSDL_GL_CreateContext failed: %s", SDL_GetError());
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	SDL_GL_MakeCurrent(this->sdl_window, this->sdl_gl_context);

    // Load GL extensions using glad
    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
		CHIAKI_LOGE(this->log, "Failed to initialize the OpenGL context.");
		return false;
    }
	CHIAKI_LOGI(this->log, "OpenGL version loaded: %d.%d", GLVersion.major, GLVersion.minor);

	if(!InitOpenGlShader()){
		return false;
	}

	if(!InitOpenGlTextures()){
		return false;
	}

	return true;
}

bool IO::InitOpenGlTextures() {
	CHIAKI_LOGV(this->log, "loading OpenGL textrures");

	D(glGenTextures(PLANES_COUNT, this->tex));
	D(glGenBuffers(PLANES_COUNT, this->pbo));
	uint8_t uv_default[] = {0x7f, 0x7f};
	for(int i=0; i < PLANES_COUNT; i++)
	{
		D(glBindTexture(GL_TEXTURE_2D, this->tex[i]));
		D(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		D(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		D(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		D(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		D(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, i > 0 ? uv_default : nullptr));
	}

	D(glUseProgram(this->prog));
	// bind only as many planes as we need
	const char *plane_names[] = {"plane1", "plane2", "plane3"};
	for(int i=0; i < PLANES_COUNT; i++)
	{
		D(glUniform1i(glGetUniformLocation(this->prog, plane_names[i]), i));
	}

	D(glGenVertexArrays(1, &this->vao));
	D(glBindVertexArray(this->vao));

	D(glGenBuffers(1, &this->vbo));
	D(glBindBuffer(GL_ARRAY_BUFFER, this->vbo));
	D(glBufferData(GL_ARRAY_BUFFER, sizeof(vert_pos), vert_pos, GL_STATIC_DRAW));

	D(glBindBuffer(GL_ARRAY_BUFFER, this->vbo));
	D(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
	D(glEnableVertexAttribArray(0));

	D(glCullFace(GL_BACK));
	D(glEnable(GL_CULL_FACE));
	D(glClearColor(0.5, 0.5, 0.5, 1.0));
	return true;
}

GLuint IO::CreateAndCompileShader(GLenum type, const char* source){
	GLint success;
	GLchar msg[512];

	GLuint handle = glCreateShader(type);
	if (!handle){
		CHIAKI_LOGE(this->log, "%u: cannot create shader", type);
		return false;
	}

	glShaderSource(handle, 1, &source, nullptr);
	glCompileShader(handle);
	glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

	if (!success) {
		glGetShaderInfoLog(handle, sizeof(msg), nullptr, msg);
		CHIAKI_LOGE(this->log, "%u: %s\n", type, msg);
		glDeleteShader(handle);
	}
	return handle;
}

bool IO::InitOpenGlShader() {
	CHIAKI_LOGV(this->log, "loading OpenGl Shaders");
	this->vert = CreateAndCompileShader(GL_VERTEX_SHADER, shader_vert_glsl);
	this->frag = CreateAndCompileShader(GL_FRAGMENT_SHADER, yuv420p_shader_frag_glsl);
	this->prog = glCreateProgram();

	D(glAttachShader(this->prog, this->vert));
	D(glAttachShader(this->prog, this->frag));
	D(glBindAttribLocation(this->prog, 0, "pos_attr"));
	D(glLinkProgram(this->prog));

    GLint success;
    glGetProgramiv(this->prog, GL_LINK_STATUS, &success);
    if (!success)
    {
        char buf[512];
        glGetProgramInfoLog(this->prog, sizeof(buf), nullptr, buf);
		CHIAKI_LOGE(this->log, "OpenGL link error: %s", buf);
		return false;
    }

    glDeleteShader(this->vert);
    glDeleteShader(this->frag);

	return true;
}

inline void IO::SetOpenGlYUVPixels(AVFrame * frame){
	int planes[][3] = {
		// { width_divide, height_divider, data_per_pixel }
		{ 1, 1, 1 }, // Y
		{ 2, 2, 1 }, // U
		{ 2, 2, 1 }  // V
	};

	D(SDL_GL_MakeCurrent(this->sdl_window, this->sdl_gl_context));
	for(int i = 0; i < PLANES_COUNT; i++){
		int width = frame->width / planes[i][0];
		int height = frame->height / planes[i][1];
		int size = width * height * planes[i][2];
		uint8_t * buf;

		D(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, this->pbo[i]));
		D(glBufferData(GL_PIXEL_UNPACK_BUFFER, size, nullptr, GL_STREAM_DRAW));

		D(buf = reinterpret_cast<uint8_t *>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)));
		if(!buf)
		{
			GLint data;
			D(glGetBufferParameteriv(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE, &data));
			CHIAKI_LOGE(this->log, "AVOpenGLFrame failed to map PBO");
			CHIAKI_LOGE(this->log, "Info buf == %p. size %d frame %d * %d, divs %d, %d, pbo %d GL_BUFFER_SIZE %x",
				buf, size, frame->width, frame->height, planes[i][0], planes[i][1], pbo[i], data);
			continue;
		}

		if(frame->linesize[i] == width){
			// Y
			memcpy(buf, frame->data[i], size);
		} else {
			// UV
			for(int l=0; l<height; l++)
				memcpy(buf + width * l * planes[i][2],
					frame->data[i] + frame->linesize[i] * l,
					width * planes[i][2]);
		}
		D(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
		D(glBindTexture(GL_TEXTURE_2D, tex[i]));
		D(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr));
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}
	glFinish();
	SDL_GL_MakeCurrent(this->sdl_window, NULL);

	if(!this->renderer){
		this->renderer = SDL_CreateRenderer(this->sdl_window, -1, SDL_RENDERER_ACCELERATED);
		if (!renderer) {
			CHIAKI_LOGE(this->log, "SDL_CreateRenderer failed: %s", SDL_GetError());
			return false;
		}
	}
}

inline void IO::OpenGlDraw() {
	glClear(GL_COLOR_BUFFER_BIT);

	for(int i=0; i< PLANES_COUNT; i++) {
		D(glActiveTexture(GL_TEXTURE0 + i));
		D(glBindTexture(GL_TEXTURE_2D, this->tex[i]));
	}

	D(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
	D(glFinish());
}
#else

bool IO::InitSDLTextures() {
	// https://github.com/raullalves/player-cpp-ffmpeg-sdl/blob/master/Player.cpp
	printf("renderer %p\n", this->renderer);
	if(!this->renderer){
		this->renderer = SDL_CreateRenderer(this->sdl_window, -1, SDL_RENDERER_ACCELERATED);
		if (!this->renderer) {
			CHIAKI_LOGE(this->log, "SDL_CreateRenderer failed: %s", SDL_GetError());
			return false;
		}
	}

	this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_YV12,
		SDL_TEXTUREACCESS_STREAMING, this->screen_width, this->screen_height);

	if(!this->texture){
		CHIAKI_LOGE(log, "SDL_CreateTexture failed: %s", SDL_GetError());
		SDL_Quit();
		return false;
	}
	return true;
}



inline void IO::SDLDraw() {

	SDL_UpdateYUVTexture(
		this->texture,
		&this->rect,
		this->pict->data[0],
		this->pict->linesize[0],
		this->pict->data[1],
		this->pict->linesize[1],
		this->pict->data[2],
		this->pict->linesize[2]
	);

	SDL_RenderClear(this->renderer);
	SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);
	SDL_RenderPresent(this->renderer);
	SDL_UpdateWindowSurface(this->sdl_window);

}
#endif

bool IO::InitJoystick(){
	// https://github.com/switchbrew/switch-examples/blob/master/graphics/sdl2/sdl2-simple/source/main.cpp#L57
    // open CONTROLLER_PLAYER_1 and CONTROLLER_PLAYER_2
    // when railed, both joycons are mapped to joystick #0,
    // else joycons are individually mapped to joystick #0, joystick #1, ...
#ifdef __SWITCH__
    for (int i = 0; i < 2; i++) {
        if (SDL_JoystickOpen(i) == NULL) {
            CHIAKI_LOGE(this->log, "SDL_JoystickOpen: %s\n", SDL_GetError());
			return false;
        }
    }
#endif
	//FIXME
	return true;
}

bool IO::ResizeVideo(int width, int height) {
	if(width <= 0 || height <=0){
		CHIAKI_LOGE(this->log, "Invalid Video size %dx%d", width, height);
		return false;
	}
	this->video_width = width;
	this->video_height = height;

#ifdef CHIAKI_SWITCH_ENABLE_OPENGL
	float vp_height, vp_width;
	float aspect = (float)this->video_width / (float)this->video_height;
	if(aspect < (float)this->screen_width / (float)this->screen_height){
		vp_height = this->screen_height;
		vp_width = (GLsizei)(vp_height * aspect);
	} else {
		vp_width = this->screen_width;
		vp_height = (GLsizei)(vp_width / aspect);
	}
	D(glViewport((this->screen_width - vp_width) / 2, (this->screen_height - vp_height) / 2, vp_width, vp_height));

#else
	this->rect.x = 0;
	this->rect.y = 0;
	this->rect.w = this->screen_width;
	this->rect.h = this->screen_height;

	this->codec_context->width = this->screen_width;
	this->codec_context->height = this->screen_height;
	this->codec_context->pix_fmt = AV_PIX_FMT_YUV420P;

	this->sws_context = sws_getContext(
		this->video_width,
		this->video_height,
		this->codec_context->pix_fmt,
		this->codec_context->width,
		this->codec_context->height,
		AV_PIX_FMT_YUV420P,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);

	this->pict = av_frame_alloc();
    this->pict->format = AV_PIX_FMT_YUV420P;
    this->pict->width  = this->screen_width;
    this->pict->height = this->screen_height;

	if(av_frame_get_buffer(this->pict, 0) < 0){
		CHIAKI_LOGE(this->log, "failed to get frame buffer");
	}

#endif
	this->resize = false;
	return true;
}
bool IO::MainLoop(ChiakiControllerState * state){
	// handle SDL events
	while(SDL_PollEvent(&this->sdl_event)){
		this->ReadGameKeys(&this->sdl_event, state);
		switch(this->sdl_event.type)
		{
			case SDL_QUIT:
				return false;
		}
	}

	std::unique_lock<std::mutex> lck(mtx);
	while(cargo == 0) consume.wait(lck);

	if(this->resize){
		IO::ResizeVideo(this->video_width, this->video_height);
	}
#ifdef CHIAKI_SWITCH_ENABLE_OPENGL
	D(SDL_GL_MakeCurrent(this->sdl_window, this->sdl_gl_context));
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	OpenGlDraw();
	//SDL_UpdateWindowSurface(this->sdl_window);
	SDL_GL_SwapWindow(this->sdl_window);
	SDL_GL_MakeCurrent(this->sdl_window, NULL);
#else
	SDLDraw();
#endif
	cargo = !cargo;
	produce.notify_one();

	/*
	uint32_t end = SDL_GetTicks();
	// ~60 fps
	int delay = (1000 / 60) - (end - start);
	if(delay > 0){
		SDL_Delay((1000 / 60) - (end - start));
	}
	*/
	return true;
}

