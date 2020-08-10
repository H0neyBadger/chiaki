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

// chiaki modules
#include <chiaki/log.h>
#include <chiaki/discovery.h>

// discover and wakeup ps4 host
// from local network
#include "discoverymanager.h"
#include "settings.h"
#include "io.h"

#ifdef __SWITCH__
#include <switch.h>
// plutonium gui
#include "gui.h"
#endif

// max                   1785000000
#define SWITCH_OVERCLOCK 1326000000
// #define SWITCH_OVERCLOCK 1220000000
// #define SWITCH_OVERCLOCK 1020000000
#define SCREEN_W 1280
#define SCREEN_H 720

#ifndef CHIAKI_SWITCH_ENABLE_LINUX
#define CHIAKI_ENABLE_SWITCH_NXLINK 1
#endif

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
#endif

#ifdef __SWITCH__
extern "C" void userAppInit()
{
#ifdef CHIAKI_ENABLE_SWITCH_NXLINK
	initNxLink();
#endif
	// to load gui resources
	romfsInit();
	// load socket custom config
	socketInitialize(&g_chiakiSocketInitConfig);
}

extern "C" void userAppExit()
{
#ifdef CHIAKI_ENABLE_SWITCH_NXLINK
	deinitNxLink();
#endif
	//reset OC
	ClkrstSession cpuSession;
	clkrstInitialize();
    clkrstOpenSession(&cpuSession, PcvModuleId_CpuBus, 3);
	// reset default overclock
    clkrstSetClockRate(&cpuSession, 1020000000);
	clkrstCloseSession(&cpuSession);
	clkrstExit();
}
#endif

int main(int argc, char* argv[]){
	// init chiaki lib
	ChiakiLog log;
#if defined(CHIAKI_ENABLE_SWITCH_NXLINK) || defined(CHIAKI_SWITCH_ENABLE_LINUX)
	chiaki_log_init(&log, CHIAKI_LOG_ALL ^ CHIAKI_LOG_VERBOSE, chiaki_log_cb_print, NULL);
	//chiaki_log_init(&log, CHIAKI_LOG_ALL, chiaki_log_cb_print, NULL);
#else
	// null log for switch version
	chiaki_log_init(&log, 0, chiaki_log_cb_print, NULL);
#endif
	// load chiaki lib
	CHIAKI_LOGI(&log, "Loading chaki lib");

	ChiakiErrorCode err = chiaki_lib_init();
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&log, "Chiaki lib init failed: %s\n", chiaki_error_string(err));
		return 1;
	}

	CHIAKI_LOGI(&log, "Loading Window");
	// build sdl OpenGl and AV decoders graphical interface
	IO io = IO(&log); // open Input Output class
	// set video size to 0

	// manage ps4 setting discovery wakeup and registration
	std::map<std::string, Host> hosts;
	// create host objects form config file
	Settings settings = Settings(&log, &hosts);
	CHIAKI_LOGI(&log, "Read chiaki settings file");
	// FIXME use GUI for config
	settings.ParseFile();
	Host * host = nullptr;
	// create sub context to destroy discoverymanager
	{
		DiscoveryManager discoverymanager = DiscoveryManager(&log, &hosts);
		CHIAKI_LOGI(&log, "Call Discover");
		int d = discoverymanager.Send();
#ifdef __SWITCH__
		// load Plutonium GUI
		auto plutonium_renderer_options = pu::ui::render::
			RendererInitOptions(
				SDL_INIT_EVERYTHING,
				pu::ui::render::RendererHardwareFlags
			).WithIMG(
				pu::ui::render::IMGAllFlags
			).WithMixer(
				pu::ui::render::MixerAllFlags
			).WithTTF();

		auto plutonium_renderer = pu::ui::render::
			Renderer::New(plutonium_renderer_options);

		auto plutonium_app = MainApplication::New(plutonium_renderer,
			&hosts, &settings, &discoverymanager, &io);

	    plutonium_app->Prepare();
	    plutonium_app->Show();
		// retrieve ps4 gui host ptr
		host = plutonium_app->GetHost();
		/*
		io.SetSDLWindow(pu::ui::render::GetMainWindow());
		io.SetSDLRenderer(pu::ui::render::GetMainRenderer());
		*/
		printf("bye");
		return 0;
#else
		// wait for discoverymanager
		sleep(1);
		size_t host_count = hosts.size();
		if(host_count != 1){
			// FIXME
			CHIAKI_LOGE(&log, "too many or to too few host to connect");
			return 1;
		}
		// pick the first host of the map
		host = &hosts.begin()->second;
		// int c = discoverymanager.ParseSettings();
		CHIAKI_LOGI(&log, "Open %s host", host->host_addr.c_str());
		int count = 0;
		while(host->state != CHIAKI_DISCOVERY_HOST_STATE_READY && host->rp_key_data && count < 10){
			CHIAKI_LOGI(&log, "Send Wakeup packet count:%d", count);
			host->Wakeup();
			//refresh state (and wait)
			sleep(2);
			discoverymanager.Send(host->host_addr.c_str());
		}

		if(!host->rp_key_data) {
			CHIAKI_LOGI(&log, "Call register");
			char pin_input[9];
			io.ReadUserKeyboard(pin_input, sizeof(pin_input));
			std::string pin = pin_input;
			host->Register(pin);
			sleep(2);
			if(host->rp_key_data && host->registered)
				settings.WriteFile();
		}

		CHIAKI_LOGI(&log, "Connect Session");
		host->ConnectSession(&io);
		sleep(2);

		// run stream session thread
		CHIAKI_LOGI(&log, "Start Session");
		host->StartSession();
		sleep(1);

#endif
		// destroy discoverymanager
	}

	if(!io.InitVideo(0, 0, SCREEN_W, SCREEN_H)){
		CHIAKI_LOGE(&log, "Failed to initiate Video");
		return 1;
	}

	if(host->state == CHIAKI_DISCOVERY_HOST_STATE_UNKNOWN){
		CHIAKI_LOGE(&log, "Failed to discover host (network issue ?)");
		return 1;
	}

	if(host->state == CHIAKI_DISCOVERY_HOST_STATE_STANDBY && !host->rp_key_data){
		CHIAKI_LOGE(&log, "Your PS4 is not ready. Please turn on your ps4 to register this device");
		return 1;
	}

	if(host <= 0){
		CHIAKI_LOGE(&log, "Host %s not found", host->host_addr.c_str());
		return 1;
	}

	CHIAKI_LOGI(&log, "Load sdl joysticks");
	if(!io.InitJoystick()){
		CHIAKI_LOGI(&log, "Faled to initiate Joysticks");
	}

	// store joycon keys
	ChiakiControllerState state = { 0 };

#ifdef __SWITCH__
	CHIAKI_LOGI(&log, "Overclock Nintendo Switch to SWITCH_OVERCLOCK = %d", SWITCH_OVERCLOCK);
	ClkrstSession cpuSession;
	clkrstInitialize();
	clkrstOpenSession(&cpuSession, PcvModuleId_CpuBus, 3);
	clkrstSetClockRate(&cpuSession, SWITCH_OVERCLOCK);
#endif
	int video_width = 0;
	int video_height = 0;
	host->GetVideoResolution(&video_width, &video_height);
	io.ResizeVideo(video_width, video_height);

	CHIAKI_LOGI(&log, "Enter applet main loop");
	while (appletMainLoop() && io.MainLoop(&state))
	{
        host->SendFeedbackState(&state);
	}
#ifdef __SWITCH__
	clkrstCloseSession(&cpuSession); //end OC
	clkrstExit();
#endif
	CHIAKI_LOGI(&log, "Quit applet");
	io.FreeVideo();
	return 0;
}

