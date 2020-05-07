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

#ifndef CHIAKI_ENABLE_SWITCH_LINUX
#include <switch.h>
#else
// fake libnx function
#include <iostream>
bool appletMainLoop(){return true;}
void consoleInit(void * i){return;}
void consoleUpdate(void * i){return;}
void consoleExit(void * i){return;}
void socketInitializeDefault(){return;};
#endif

#include <SDL2/SDL.h>

// chiaki modules
#include <chiaki/log.h>
// discover and wakeup ps4 host
// from local network
#include "discoverymanager.h"
#include "settings.h"


#define SCREEN_W 1280
#define SCREEN_H 720

#ifndef CHIAKI_ENABLE_SWITCH_LINUX
#define CHIAKI_ENABLE_SWITCH_NXLINK 1
#endif

#define CHIAKI_ENABLE_SWITCH_GUI 1

#ifdef __SWITCH__
// use a custom nintendo switch socket config
// chiaki requiers many threads with udp/tcp sockets
static const SocketInitConfig g_chiakiSocketInitConfig = {
	.bsdsockets_version = 1,

	.tcp_tx_buf_size = 0x8000,
	.tcp_rx_buf_size = 0x10000,
	.tcp_tx_buf_max_size = 0x40000,
	.tcp_rx_buf_max_size = 0x40000,

	.udp_tx_buf_size = 0x40000,
	.udp_rx_buf_size = 0x40000,

	.sb_efficiency = 8,

	.num_bsd_sessions = 16,
	.bsd_service_type = BsdServiceType_User,
};
#endif


#ifdef CHIAKI_ENABLE_SWITCH_NXLINK
static int s_nxlinkSock = -1;

static void initNxLink()
{
	// use chiaki socket config initialization
	if (R_FAILED(socketInitialize(&g_chiakiSocketInitConfig)))
		return;

	s_nxlinkSock = nxlinkStdio();
	if (s_nxlinkSock >= 0)
		printf("initNxLink");
	else
	socketExit();
}

static void deinitNxLink()
{
	if (s_nxlinkSock >= 0)
	{
		close(s_nxlinkSock);
		socketExit();
		s_nxlinkSock = -1;
	}
}

extern "C" void userAppInit()
{
	initNxLink();
}

extern "C" void userAppExit()
{
	deinitNxLink();
}
#endif

void ReadUserKeyboard(ChiakiLog *log, char *buffer, size_t buffer_size){
#ifdef CHIAKI_ENABLE_SWITCH_LINUX
	// use cin to get user input from linux
	std::cin.getline(buffer, buffer_size);
	CHIAKI_LOGI(log, "Got user input: %s\n", buffer);
#else
	// https://kvadevack.se/post/nintendo-switch-virtual-keyboard/
	SwkbdConfig kbd;
	Result rc = swkbdCreate(&kbd, 0);

	if (R_SUCCEEDED(rc)) {
		swkbdConfigMakePresetDefault(&kbd);
        swkbdConfigSetHeaderText(&kbd, "PS4 Remote play PIN code");
        swkbdConfigSetSubText(&kbd, "Please provide ps4 remote play PIN registration code");
		rc = swkbdShow(&kbd, buffer, buffer_size);

		if (R_SUCCEEDED(rc)) {
			CHIAKI_LOGI(log, "Got user input: %s\n", buffer);
		} else {
			CHIAKI_LOGE(log, "swkbdShow() error: %u\n", rc);
		}
		swkbdClose(&kbd);
	} else {
		CHIAKI_LOGE(log, "swkbdCreate() error: %u\n", rc);
	}
#endif
}

int main(int argc, char* argv[]){
#ifndef CHIAKI_ENABLE_SWITCH_GUI
	//Load switch console interface
	consoleInit(NULL);
#endif
	// init chiaki lib
	ChiakiLog log;
#if defined(CHIAKI_ENABLE_SWITCH_NXLINK) || defined(CHIAKI_ENABLE_SWITCH_LINUX) || !defined(CHIAKI_ENABLE_SWITCH_GUI)
	chiaki_log_init(&log, CHIAKI_LOG_ALL ^ CHIAKI_LOG_VERBOSE, chiaki_log_cb_print, NULL);
	//chiaki_log_init(&log, CHIAKI_LOG_ALL, chiaki_log_cb_print, NULL);
#else
	// null log for switch version
	chiaki_log_init(&log, 0, chiaki_log_cb_print, NULL);
#endif

#ifdef __SWITCH__
	//setsysInitialize();
	socketInitialize(&g_chiakiSocketInitConfig);
#endif

	// load chiaki lib
	CHIAKI_LOGI(&log, "Loading chaki lib");

	ChiakiErrorCode err = chiaki_lib_init();
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&log, "Chiaki lib init failed: %s\n", chiaki_error_string(err));
		return 1;
	}

	CHIAKI_LOGI(&log, "Loading socket");
	//setsysInitialize();


#ifdef CHIAKI_ENABLE_SWITCH_GUI
	CHIAKI_LOGI(&log, "Loading SDL2 window");
	// build sdl graphical interface

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER)) {
		CHIAKI_LOGE(&log, "SDL initialization failed: %s", SDL_GetError());
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow("Chiaki Stream",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		SCREEN_W,
		SCREEN_H,
		SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

	if (!window) {
		CHIAKI_LOGE(&log, "SDL_CreateWindow failed: %s", SDL_GetError());
		sleep(2);
		SDL_Quit();
		return -1;
	}

	SDL_GL_SetSwapInterval(1);

	//create a renderer (OpenGL ES2)
	//
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if (!renderer) {
		CHIAKI_LOGE(&log, "SDL_CreateRenderer failed: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	// https://github.com/raullalves/player-cpp-ffmpeg-sdl/blob/master/Player.cpp
	SDL_Texture	*texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
	if (!texture) {
		CHIAKI_LOGE(&log, "SDL_CreateTexture failed: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}
#endif

	// manage ps4 setting discovery wakeup and registration
	std::map<std::string, Host> hosts;
	// create host objects form config file
	Settings settings = Settings(&log, &hosts);
	CHIAKI_LOGI(&log, "Read chiaki settings file");
	settings.ParseFile();
	DiscoveryManager discoverymanager = DiscoveryManager(&log, &hosts);
	size_t host_count = hosts.size();

	if(host_count != 1){
		// FIXME
		CHIAKI_LOGE(&log, "too many or to too few host to connect");
		SDL_Quit();
		exit(1);
	}
	// pick the first host of the map
	Host *host = &hosts.begin()->second;
	// int c = discoverymanager.ParseSettings();
	CHIAKI_LOGI(&log, "Call Discover");
	int d = discoverymanager.Discover(host->host_addr.c_str());
	CHIAKI_LOGI(&log, "Discover ran");
	// retrieve first host on the list
	CHIAKI_LOGI(&log, "Open %s host", host->host_addr.c_str());

	if(host <= 0){
		CHIAKI_LOGE(&log, "Host %s not found", host->host_addr.c_str());
#ifndef CHIAKI_ENABLE_SWITCH_GUI
		consoleUpdate(NULL);
		sleep(5);
		consoleExit(NULL);
#endif
		return 1;
	}

	//FIXME
	//if(host->rp_key_data) {
		//CHIAKI_LOGI(&log, "Call Wakeup");
		//int w = host->Wakeup();
	//}

	//simple video var init
	host->InitVideo();

	if(!host->rp_key_data) {
		CHIAKI_LOGI(&log, "Call register");
		char pin_input[9];
		ReadUserKeyboard(&log, pin_input, sizeof(pin_input));
		std::string pin = pin_input;
		host->Register(pin);
		sleep(2);
		if(host->rp_key_data && host->registered)
			settings.WriteFile();
	}

	CHIAKI_LOGI(&log, "Connect Session");
	host->ConnectSession();
	sleep(2);

	// run stream session thread
	CHIAKI_LOGI(&log, "Start Session");
	host->StartSession();
	sleep(1);

#ifdef CHIAKI_ENABLE_SWITCH_GUI
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = SCREEN_W;
	rect.h = SCREEN_H;

	// https://github.com/switchbrew/switch-examples/blob/master/graphics/sdl2/sdl2-simple/source/main.cpp#L57
    // open CONTROLLER_PLAYER_1 and CONTROLLER_PLAYER_2
    // when railed, both joycons are mapped to joystick #0,
    // else joycons are individually mapped to joystick #0, joystick #1, ...
#ifdef __SWITCH__
    for (int i = 0; i < 2; i++) {
        if (SDL_JoystickOpen(i) == NULL) {
            CHIAKI_LOGE(&log, "SDL_JoystickOpen: %s\n", SDL_GetError());
            SDL_Quit();
            exit(1);
        }
    }
#endif

	SDL_Event event;
	AVFrame *pict = host->pict;
	// store joycon keys
	ChiakiControllerState state = { 0 };
#endif
	CHIAKI_LOGI(&log, "Enter applet main loop");
	while (appletMainLoop())
	{
#ifdef CHIAKI_ENABLE_SWITCH_GUI
		//uint32_t start = SDL_GetTicks();
		// update sdl textrure
		SDL_UpdateYUVTexture(
			texture,
			&rect,
			pict->data[0],
			pict->linesize[0],
			pict->data[1],
			pict->linesize[1],
			pict->data[2],
			pict->linesize[2]
		);

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);

		// handle SDL events
		while(SDL_PollEvent(&event)){
			host->ReadGameKeys(&event, &state);

			switch(event.type)
			{
				case SDL_QUIT:
				{
					SDL_Quit();
					goto exit;
				}
				break;
			}
		}
        host->SendFeedbackState(&state);
		/*
		uint32_t end = SDL_GetTicks();
		// ~60 fps
		int delay = (1000 / 60) - (end - start);
		if(delay > 0){
			SDL_Delay((1000 / 60) - (end - start));
		}
		*/
#else
		//refresh console every one sec
		consoleUpdate(NULL);
		sleep(1);
#endif
	}

exit:
#ifdef CHIAKI_ENABLE_SWITCH_GUI
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
#else
	consoleExit(NULL);
#endif
	return 0;
}
