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
#endif

#include <cstring>

#include <chiaki/base64.h>

#include <host.h>

static void Regist(ChiakiRegistEvent *event, void *user){
	Host *host = (Host*) user;
	host->RegistCB(event);
}

static void InitAudio(unsigned int channels, unsigned int rate, void *user){
	Host *host = (Host*) user;
	host->InitAudioCB(channels, rate);
}

static bool Video(uint8_t *buf, size_t buf_size, void *user){
	Host *host = (Host*) user;
	return host->VideoCB(buf, buf_size);
}

static void Audio(int16_t *buf, size_t samples_count, void *user){
	Host *host = (Host*) user;
	host->AudioCB(buf, samples_count);
}

Host * Host::GetOrCreate(ChiakiLog *log, std::map<std::string, Host> *hosts, std::string *key){
    // update of create Host instance
    if ( hosts->find(*key) == hosts->end() ) {
        // create host if udefined
        (*hosts)[*key] = Host(log);
    }
    Host *ret = &(hosts->at(*key));
    ret->host_name = *key;
    return ret;
}

void Host::InitVideo(){
	// set libav video context
	// for later stream

	int numBytes;
	uint8_t * buffer = NULL;
	this->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if(!this->codec)
		throw Exception("H264 Codec not available");

	this->codec_context = avcodec_alloc_context3(codec);
	if(!this->codec_context)
		throw Exception("Failed to alloc codec context");

	if(avcodec_open2(codec_context, codec, nullptr) < 0)
	{
		avcodec_free_context(&codec_context);
		throw Exception("Failed to open codec context");
	}
/*
 * CHIAKI_VIDEO_RESOLUTION_PRESET_360p = 1,
 * CHIAKI_VIDEO_RESOLUTION_PRESET_540p = 2,
 * CHIAKI_VIDEO_RESOLUTION_PRESET_720p = 3,
 * CHIAKI_VIDEO_RESOLUTION_PRESET_1080p = 4
 * CHIAKI_VIDEO_FPS_PRESET_30;
 * 640 × 360
 * 960 × 540
 * 1280 x 720
 * 1920 × 1080
 */
	int source_width;
	int source_height;
	switch(this->video_resolution){
		case CHIAKI_VIDEO_RESOLUTION_PRESET_360p:
			source_width = 640;
			source_height = 360;
			break;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_540p:
			source_width = 950;
			source_height = 540;
			break;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_720p:
			source_width = 1280;
			source_height = 720;
			break;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_1080p:
			source_width = 1920;
			source_height = 1080;
			break;
	}
	this->codec_context->width = 1280;
	this->codec_context->height = 720;
	this->codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
	// sws context to convert frame data to YUV420:
	// {"width":1280,"height":720}
	// AV_PIX_FMT_BGR24
	// SWS_BILINEAR | SWS_ACCURATE_RND
	this->sws_context = sws_getContext(
		source_width,
		source_height,
		this->codec_context->pix_fmt,
		this->codec_context->width,
		this->codec_context->height,
		AV_PIX_FMT_YUV420P,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);

	numBytes = av_image_get_buffer_size(
		AV_PIX_FMT_YUV420P,
		this->codec_context->width,
		this->codec_context->height,
		32
	);

	buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

	this->pict = av_frame_alloc();

	av_image_fill_arrays(
		pict->data,
		pict->linesize,
		buffer,
		AV_PIX_FMT_YUV420P,
		this->codec_context->width,
		this->codec_context->height,
		32
	);

	chiaki_connect_video_profile_preset(&(this->video_profile),
		this->video_resolution, this->video_fps);

}

/*
Host::~Host(void){
	sdl2 audio device
}
*/

int Host::Wakeup()
{
	if(strlen(this->rp_regist_key) > 8)
	{
		CHIAKI_LOGE(this->log, "Given registkey is too long");
		return 1;
	} else if (strlen(this->rp_regist_key) <=0){
		CHIAKI_LOGE(this->log, "Given registkey is not defined");
		return 2;
	}

	uint64_t credential = (uint64_t)strtoull(this->rp_regist_key, NULL, 16);
	ChiakiErrorCode ret = chiaki_discovery_wakeup(this->log, NULL, host_addr.c_str(), credential);
	if(ret == CHIAKI_ERR_SUCCESS){
		//FIXME
		//sleep(1);
	}
	return ret;
}

int Host::Register(std::string pin){
	// use pin and accont_id to negociate secrets for session
	ChiakiRegist regist = {};
	ChiakiRegistInfo regist_info = { 0 };
	// convert psn_account_id into uint8_t[CHIAKI_PSN_ACCOUNT_ID_SIZE]
	// CHIAKI_PSN_ACCOUNT_ID_SIZE == 8
	size_t psn_account_id_size = sizeof(uint8_t[CHIAKI_PSN_ACCOUNT_ID_SIZE]);
	// PS4 firmware > 7.0
	if(system_version >= 7000000){
		// use AccountID for ps4 > 7.0
		chiaki_base64_decode(this->psn_account_id.c_str(), this->psn_account_id.length(),
			regist_info.psn_account_id, &(psn_account_id_size));
		regist_info.psn_online_id = NULL;
	} else if( this->system_version < 7000000 && this->system_version > 0) {
		// use oline ID for ps4 < 7.0
		regist_info.psn_online_id = this->psn_online_id.c_str();
		// regist_info.psn_account_id = {0};
	} else {
		CHIAKI_LOGE(this->log, "Undefined PS4 system version (please run discover first)");
	}
	regist_info.pin = atoi(pin.c_str());
	regist_info.host = this->host_addr.c_str();
	regist_info.broadcast = false;
	CHIAKI_LOGI(this->log, "Registering to host `%s` `%s` with PSN AccountID `%s` pin `%s`",
		this->host_name.c_str(), this->host_addr.c_str(), psn_account_id.c_str(), pin.c_str());
	chiaki_regist_start(&regist, this->log, &regist_info, Regist, this);
	//FIXME poll host->registered
	sleep(1);
	chiaki_regist_stop(&regist);
	chiaki_regist_fini(&regist);
	return 0;
}

int Host::ConnectSession() {
	// Build chiaki ps4 stream session
	chiaki_opus_decoder_init(&(this->opus_decoder), this->log);
	ChiakiAudioSink audio_sink;
	ChiakiConnectInfo chiaki_connect_info;
	chiaki_connect_info.host = this->host_addr.c_str();
	chiaki_connect_info.video_profile = this->video_profile;

	memcpy(chiaki_connect_info.regist_key, this->rp_regist_key, sizeof(chiaki_connect_info.regist_key));

	memcpy(chiaki_connect_info.morning, this->rp_key, sizeof(chiaki_connect_info.morning));

	// set keybord state to 0
	memset(&(this->keyboard_state), 0, sizeof(keyboard_state));

	ChiakiErrorCode err = chiaki_session_init(&(this->session), &chiaki_connect_info, this->log);
	if(err != CHIAKI_ERR_SUCCESS)
		throw Exception(chiaki_error_string(err));
	// audio setting_cb and frame_cb
	chiaki_opus_decoder_set_cb(&this->opus_decoder, InitAudio, Audio, this);
	chiaki_opus_decoder_get_sink(&this->opus_decoder, &audio_sink);
	chiaki_session_set_audio_sink(&(this->session), &audio_sink);
	chiaki_session_set_video_sample_cb(&(this->session), Video, this);
	// TODO
	chiaki_session_set_event_cb(&(this->session), NULL, this);
	return 0;
}

void Host::StartSession()
{
	ChiakiErrorCode err = chiaki_session_start(&this->session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		chiaki_session_fini(&this->session);
		throw Exception("Chiaki Session Start failed");
	}
	sleep(1);
}

bool Host::ReadGameKeys(SDL_Event *event, ChiakiControllerState *state){
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
#ifdef __SWITCH__
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

#endif
		default:
			ret = false;
		}
	return ret;
}

void Host::SendFeedbackState(ChiakiControllerState *state){
	// send controller/joystick key
	chiaki_session_set_controller_state(&this->session, state);
}

void Host::RegistCB(ChiakiRegistEvent *event){
	// Chiaki callback fuction
	// fuction called by lib chiaki regist
	// durring client pin code registration
	//
	// read data from lib and record secrets into Host object

	this->registered = false;
	switch(event->type)
	{
		case CHIAKI_REGIST_EVENT_TYPE_FINISHED_CANCELED:
			//FIXME
			break;
		case CHIAKI_REGIST_EVENT_TYPE_FINISHED_FAILED:
			//FIXME
			CHIAKI_LOGI(this->log, "Register failed %s", this->host_name.c_str());
			break;
		case CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS:
		{
			ChiakiRegisteredHost *r_host = event->registered_host;
			// copy values form ChiakiRegisteredHost object
			this->ap_ssid = r_host->ap_ssid;
			this->ap_key = r_host->ap_key;
			this->ap_name = r_host->ap_name;
			memcpy( &(this->ps4_mac), &(r_host->ps4_mac), sizeof(this->ps4_mac) );
			this->ps4_nickname = r_host->ps4_nickname;
			memcpy( &(this->rp_regist_key),  &(r_host->rp_regist_key), sizeof(this->rp_regist_key) );
			this->rp_key_type = r_host->rp_key_type;
			memcpy( &(this->rp_key), &(r_host->rp_key), sizeof(this->rp_key) );
			// mark host as registered
			this->registered = true;
			this->rp_key_data = true;
			CHIAKI_LOGI(this->log, "Register Success %s", this->host_name.c_str());
			break;
		}
	}
}

bool Host::VideoCB(uint8_t *buf, size_t buf_size){
	// callback function to decode video buffer
	// access chiaki session from Host object
	AVPacket packet;
	av_init_packet(&packet);
	packet.data = buf;
	packet.size = buf_size;
	int r;

send_packet:
	// TODO AVCodec internal buffer is full removing frames before pushing
	r = avcodec_send_packet(this->codec_context, &packet);
	if(r != 0) {
		if(r == AVERROR(EAGAIN)){
			CHIAKI_LOGE(this->log, "AVCodec internal buffer is full removing frames before pushing");
			AVFrame *frame = av_frame_alloc();

			if(!frame){
				CHIAKI_LOGE(this->log, "Failed to alloc AVFrame");
				return false;
			}

			r = avcodec_receive_frame(this->codec_context, frame);
			// send decoded frame for sdl texture update
			av_frame_free(&frame);
			if(r != 0){
				CHIAKI_LOGE(this->log, "Failed to pull frame");
				return false;
			}
			goto send_packet;
		} else {
			char errbuf[128];
			av_make_error_string(errbuf, sizeof(errbuf), r);
			CHIAKI_LOGE(this->log, "Failed to push frame: %s", errbuf);
			return false;
		}
	}

	// FramesAvailable
	AVFrame *frame = av_frame_alloc();
	AVFrame *next_frame = av_frame_alloc();
	AVFrame *tmp_swp = frame;

	if(!frame){
		CHIAKI_LOGE(this->log, "UpdateFrame Failed to alloc AVFrame");
		return -1;
	}

	int ret;
	/*
	ret = avcodec_receive_frame(this->codec_context, frame);

	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	{
		CHIAKI_LOGE(this->log, "Error while decoding: EAGAIN or AVERROR_EOF");
		return ret;
	}
	else if (ret < 0)
	{
		CHIAKI_LOGE(this->log, "Error while decoding.");
		return -1;
	}
	*/
	// decode frame
	do {
		tmp_swp = frame;
		frame = next_frame;
		next_frame = tmp_swp;
		ret = avcodec_receive_frame(this->codec_context, next_frame);
	} while(ret == 0);

	// adjust frame to pict
	sws_scale(
		this->sws_context,
		(uint8_t const * const *)frame->data,
		frame->linesize,
		0,
		this->codec_context->height,
		pict->data,
		pict->linesize
	);

	av_frame_free(&frame);
	av_frame_free(&next_frame);
	return true;
}

void Host::InitAudioCB(unsigned int channels, unsigned int rate){
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

	this->audio_device_id = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
	if(this->audio_device_id < 0){
		CHIAKI_LOGE(this->log, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
	} else {
		SDL_PauseAudioDevice(this->audio_device_id, 0);
	}
}

void Host::AudioCB(int16_t *buf, size_t samples_count){
	//int az = SDL_GetQueuedAudioSize(host->audio_device_id);
	// len the number of bytes (not samples!) to which (data) points
	int success = SDL_QueueAudio(this->audio_device_id, buf, sizeof(int16_t)*samples_count*2);
	if(success != 0){
		CHIAKI_LOGE(this->log, "SDL_QueueAudio failed: %s\n", SDL_GetError());
	}
}
