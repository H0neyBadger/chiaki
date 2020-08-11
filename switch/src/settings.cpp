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
#include <fstream>

#include <chiaki/base64.h>
#include "settings.h"

/*
// default settings
static std::string ipaddr = "255.255.255.255";
static ChiakiVideoResolutionPreset resolution = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
static ChiakiVideoFPSPreset fps = CHIAKI_VIDEO_FPS_PRESET_30;
static int overclock = 1326000000;
*/

Settings::ConfigurationItem Settings::ParseLine(std::string *line, std::string *value){
	Settings::ConfigurationItem ci;
	std::smatch m;
	for (auto it = re_map.begin(); it != re_map.end(); it++ )
	{
		if(regex_search(*line, m, it->second)){
			ci = it->first;
			*value = m[1];
			return ci;
		}
	}
	return UNKNOWN;
}

size_t Settings::GetB64encodeSize(size_t in){
	// calculate base64 buffer size after encode
	return ((4 * in / 3) + 3) & ~3;
}

std::string Settings::GetPSNOnlineId(Host * host){
	if(host == nullptr){
		return this->global_psn_online_id;
	} else {
		return host->psn_online_id;
	}
}

std::string Settings::GetPSNAccountId(Host * host){
	if(host == nullptr){
		return this->global_psn_account_id;
	} else {
		return host->psn_account_id;
	}
}

void Settings::SetPSNOnlineId(Host * host, std::string psn_online_id){
	if(host == nullptr){
		this->global_psn_online_id = psn_online_id;
	} else {
		host->psn_online_id = psn_online_id;
	}
}

void Settings::SetPSNAccountId(Host * host, std::string psn_account_id){
	if(host == nullptr){
		this->global_psn_account_id = psn_account_id;
	} else {
		host->psn_account_id = psn_account_id;
	}
}

std::string Settings::ResolutionPresetToString(ChiakiVideoResolutionPreset resolution){
	switch(resolution){
		case CHIAKI_VIDEO_RESOLUTION_PRESET_360p:
			return "360p";
		case CHIAKI_VIDEO_RESOLUTION_PRESET_540p:
			return "540p";
		case CHIAKI_VIDEO_RESOLUTION_PRESET_720p:
			return "720p";
		case CHIAKI_VIDEO_RESOLUTION_PRESET_1080p:
			return "1080p";
	}
	return "UNKNOWN";
}

std::string Settings::FPSPresetToString(ChiakiVideoFPSPreset fps){
	switch(fps){
		case CHIAKI_VIDEO_FPS_PRESET_30:
			return "30";
		case CHIAKI_VIDEO_FPS_PRESET_60:
			return "60";
	}
	return "UNKNOWN";
}

ChiakiVideoResolutionPreset Settings::StringToResolutionPreset(std::string value){
	if (value.compare("1080p") == 0) {
	        return CHIAKI_VIDEO_RESOLUTION_PRESET_1080p;
	} else if (value.compare("720p") == 0) {
	        return CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
	} else if (value.compare("540p") == 0) {
	        return CHIAKI_VIDEO_RESOLUTION_PRESET_540p;
	} else if (value.compare("360p") == 0) {
	        return CHIAKI_VIDEO_RESOLUTION_PRESET_360p;
	}
	// default
	CHIAKI_LOGE(this->log, "Unable to parse String resolution: %s",
		value.c_str());

	return CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
}

ChiakiVideoFPSPreset Settings::StringToFPSPreset(std::string value){
	if (value.compare("60") == 0) {
	        return CHIAKI_VIDEO_FPS_PRESET_60;
	} else if (value.compare("30") == 0) {
	        return CHIAKI_VIDEO_FPS_PRESET_30;
	}
	// default
	CHIAKI_LOGE(this->log, "Unable to parse String fps: %s",
		value.c_str());

	return CHIAKI_VIDEO_FPS_PRESET_30;
}

int Settings::FPSPresetToInt(ChiakiVideoFPSPreset fps){
	switch(fps){
		case CHIAKI_VIDEO_FPS_PRESET_30:
			return 30;
		case CHIAKI_VIDEO_FPS_PRESET_60:
			return 60;
	}
	return 0;
}

int Settings::ResolutionPresetToInt(ChiakiVideoResolutionPreset resolution){
	switch(resolution){
		case CHIAKI_VIDEO_RESOLUTION_PRESET_360p:
			return 360;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_540p:
			return 540;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_720p:
			return 720;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_1080p:
			return 1080;
	}
	return 0;
}

ChiakiVideoResolutionPreset Settings::GetVideoResolution(Host * host){
	if(host == nullptr){
		return this->global_video_resolution;
	} else {
		return host->video_resolution;
	}
}

ChiakiVideoFPSPreset Settings::GetVideoFPS(Host * host){
	if(host == nullptr){
		return this->global_video_fps;
	} else {
		return host->video_fps;
	}
}

void Settings::SetVideoResolution(Host * host, ChiakiVideoResolutionPreset value){
	if(host == nullptr){
		this->global_video_resolution = value;
	} else {
		host->video_resolution = value;
	}
}

void Settings::SetVideoResolution(Host * host, std::string value){
	ChiakiVideoResolutionPreset p = StringToResolutionPreset(value);
	this->SetVideoResolution(host, p);
}


void Settings::SetVideoFPS(Host * host, ChiakiVideoFPSPreset value){
	if(host == nullptr){
		this->global_video_fps = value;
	} else {
		host->video_fps = value;
	}
}

void Settings::SetVideoFPS(Host * host, std::string value){
	ChiakiVideoFPSPreset p = StringToFPSPreset(value);
	this->SetVideoFPS(host, p);
}

int Settings::GetCPUOverclock(Host * host){
	if(host == nullptr){
		return this->global_cpu_overclock;
	} else {
		return host->cpu_overclock;
	}
}

void Settings::SetCPUOverclock(Host * host, int value){
	int oc = OC_1326;
	if ( value > OC_1580 ) {
		// max OC
		oc = OC_1785;
	} else if ( OC_1580 >= value && value > OC_1326 ) {
		oc = OC_1580;
	} else if ( OC_1326 >= value && value > OC_1220 ) {
		oc = OC_1326;
	} else if ( OC_1220 >= value && value > OC_1020 ) {
		oc = OC_1220;
	} else if ( OC_1020 >= value ) {
		// no overclock
		// default nintendo switch value
		oc = OC_1020;
	}
	if(host == nullptr){
		this->global_cpu_overclock = oc;
	} else {
		host->cpu_overclock = oc;
	}
}

void Settings::SetCPUOverclock(Host * host, std::string value){
	int v = atoi(value.c_str());
	this->SetCPUOverclock(host, v);
}

std::string Settings::GetHostAddr(Host * host){
	if(host != nullptr){
		return host->host_addr;
	} else {
		CHIAKI_LOGE(this->log, "Cannot GetHostAddr from nullptr host");
	}
	return "";
}

int Settings::GetRPKeyType(Host * host){
	if(host != nullptr){
		return host->rp_key_type;
	}
	CHIAKI_LOGE(this->log, "Cannot GetRPKeyType from nullptr host");
	return 0;
}


bool Settings::SetRPKeyType(Host * host, std::string value){
	if(host != nullptr){
		// TODO Check possible rp_type values
		host->rp_key_type = std::atoi(value.c_str());
		return true;
	}
	return false;
}

std::string Settings::GetRPKey(Host * host){
	if(host != nullptr){
		if(host->rp_key_data || host->registered){
			size_t rp_key_b64_sz = this->GetB64encodeSize(0x10);
			char rp_key_b64[rp_key_b64_sz + 1] = {0};
			ChiakiErrorCode err;
			err = chiaki_base64_encode(
				host->rp_key, 0x10,
				rp_key_b64, rp_key_b64_sz);
			if(CHIAKI_ERR_SUCCESS == err){
				return rp_key_b64;
			} else {
				CHIAKI_LOGE(this->log, "Failed to encode rp_key to base64");
			}
		}
	} else {
		CHIAKI_LOGE(this->log, "Cannot GetRPKey from nullptr host");
	}
	return "";
}

std::string Settings::GetRPRegistKey(Host * host){
	if(host != nullptr){
		if(host->rp_key_data || host->registered){
			size_t rp_regist_key_b64_sz = this->GetB64encodeSize(CHIAKI_SESSION_AUTH_SIZE);
			char rp_regist_key_b64[rp_regist_key_b64_sz + 1] = {0};
			ChiakiErrorCode err;
			err = chiaki_base64_encode(
				(uint8_t *) host->rp_regist_key, CHIAKI_SESSION_AUTH_SIZE,
				rp_regist_key_b64, rp_regist_key_b64_sz);
			if(CHIAKI_ERR_SUCCESS == err){
				return rp_regist_key_b64;
			} else {
				CHIAKI_LOGE(this->log, "Failed to encode rp_regist_key to base64");
			}
		}
	} else {
		CHIAKI_LOGE(this->log, "Cannot GetRPRegistKey from nullptr host");
	}
	return "";
}

bool Settings::SetRPKey(Host * host, std::string rp_key_b64){
	if(host != nullptr){
		size_t rp_key_sz = sizeof(host->rp_key);
		ChiakiErrorCode err = chiaki_base64_decode(
			rp_key_b64.c_str(), rp_key_b64.length(),
			host->rp_key, &rp_key_sz);
		if(CHIAKI_ERR_SUCCESS != err){
			CHIAKI_LOGE(this->log, "Failed to parse RP_KEY %s (it must be a base64 encoded)", rp_key_b64.c_str());
		} else {
			return true;
		}
	} else {
		CHIAKI_LOGE(this->log, "Cannot SetRPKey from nullptr host");
	}
	return false;
}

bool Settings::SetRPRegistKey(Host * host, std::string rp_regist_key_b64){
	if(host != nullptr){
		size_t rp_regist_key_sz = sizeof(host->rp_regist_key);
		ChiakiErrorCode err = chiaki_base64_decode(
			rp_regist_key_b64.c_str(), rp_regist_key_b64.length(),
			(uint8_t*) host->rp_regist_key, &rp_regist_key_sz);
		if(CHIAKI_ERR_SUCCESS != err){
			CHIAKI_LOGE(this->log, "Failed to parse RP_REGIST_KEY %s (it must be a base64 encoded)", rp_regist_key_b64.c_str());
		} else {
			return true;
		}
	} else {
		CHIAKI_LOGE(this->log, "Cannot SetRPKey from nullptr host");
	}
	return false;
}

void Settings::ParseFile(){
	CHIAKI_LOGI(this->log, "fstream");
	std::fstream config_file;
	//CHIAKI_LOGI(this->log, "Open config file %s", this->filename);
	config_file.open(this->filename, std::fstream::in);
	std::string line;
	std::string value;
	bool rp_key_b, rp_regist_key_b, rp_key_type_b;
	Host *current_host = nullptr;
	if(config_file.is_open()){
		CHIAKI_LOGV(this->log, "Config file opened");
		Settings::ConfigurationItem ci;
		while(getline(config_file, line)){
			CHIAKI_LOGV(this->log, "Parse config line `%s`", line.c_str());
			// for each line loop over config regex
			ci = this->ParseLine(&line, &value);
			switch(ci){
				// got to next line
				case UNKNOWN: CHIAKI_LOGV(this->log, "UNKNOWN config"); break;
				case HOST_NAME:
					CHIAKI_LOGV(this->log, "HOST_NAME %s", value.c_str());
					// current host is in context
					current_host = Host::GetOrCreate(this->log, this->hosts, &value);
					// all following case will edit the current_host config
					break;
				case HOST_IP:
					CHIAKI_LOGV(this->log, "HOST_IP %s", value.c_str());
					if(current_host != nullptr){
						current_host->host_addr = value;
					}
					// reset bool flags
					rp_key_b=false;
					rp_regist_key_b=false;
					rp_key_type_b=false;
					break;
				case PSN_ONLINE_ID:
					CHIAKI_LOGV(this->log, "PSN_ONLINE_ID %s", value.c_str());
					// current_host == nullptr
					// means we are in global ini section
					// update default setting
					this->SetPSNOnlineId(current_host, value);
					break;
				case PSN_ACCOUNT_ID:
					CHIAKI_LOGV(this->log, "PSN_ACCOUNT_ID %s", value.c_str());
					this->SetPSNAccountId(current_host, value);
					break;
				case RP_KEY:
					CHIAKI_LOGV(this->log, "RP_KEY %s", value.c_str());
					if(current_host != nullptr){
						rp_key_b = this->SetRPKey(current_host, value);
					}
					break;
				case RP_KEY_TYPE:
					CHIAKI_LOGV(this->log, "RP_KEY_TYPE %s", value.c_str());
					if(current_host != nullptr){
						// TODO Check possible rp_type values
						rp_key_type_b = this->SetRPKeyType(current_host, value);
					}
					break;
				case RP_REGIST_KEY:
					CHIAKI_LOGV(this->log, "RP_REGIST_KEY %s", value.c_str());
					if(current_host != nullptr){
						rp_regist_key_b = this->SetRPRegistKey(current_host, value);
					}
					break;
				case VIDEO_RESOLUTION:
					this->SetVideoResolution(current_host, value);
					break;
				case VIDEO_FPS:
					this->SetVideoFPS(current_host, value);
					break;
				case CPU_OVERCLOCK:
					this->SetCPUOverclock(current_host, value);
					break;
			} // ci switch
			if(rp_key_b && rp_regist_key_b && rp_key_type_b){
				// the current host contains rp key data
				current_host->rp_key_data = true;
			}
		} // is_open
		config_file.close();
	}
	return;
}

int Settings::WriteFile(){
	std::fstream config_file;
	CHIAKI_LOGI(this->log, "Open config file %s", this->filename);
	// flush file (trunc)
	// the config file is completely overwritten
	config_file.open(this->filename, std::fstream::out | std::ofstream::trunc);
	std::string line;
	std::string value;
	if(this->hosts == nullptr){
		return -1;
	}

	if(config_file.is_open()){
		// save global settings
		std::string value;
		if(this->global_video_resolution){
			config_file << "video_resolution = \""
				<< this->ResolutionPresetToString(this->GetVideoResolution(nullptr))
				<< "\"\n";
		}

		if(this->global_video_fps){
			config_file << "video_fps = "
				<< this->FPSPresetToString(this->GetVideoFPS(nullptr))
				<< "\n";
		}

		if(this->global_cpu_overclock > 0){
			config_file << "cpu_overclock = " << this->global_cpu_overclock << "\n";
		}

		if(this->global_psn_online_id.length()){
			config_file << "psn_online_id = \"" << this->global_psn_online_id << "\"\n";
		}

		if(this->global_psn_account_id.length()){
			config_file << "psn_account_id = \"" << this->global_psn_account_id << "\"\n";
		}

		// write host config in file
		// loop over all configured
		for( auto it = this->hosts->begin(); it != this->hosts->end(); it++ ){
			// first is std::string
			// second is Host
			config_file << "[" << it->first << "]\n"
				<< "host_ip = \"" << it->second.host_addr << "\"\n";

			if(it->second.video_resolution){
				config_file << "video_resolution = \""
					<< this->ResolutionPresetToString(this->GetVideoResolution(&it->second))
					<< "\"\n";
			}

			if(it->second.video_fps){
				config_file << "video_fps = "
					<< this->FPSPresetToString(this->GetVideoFPS(&it->second))
					<< "\n";
			}

			if(it->second.cpu_overclock > 0){
				config_file << "cpu_overclock = " << it->second.cpu_overclock << "\n";
			}

			if(it->second.psn_online_id.length()){
				config_file << "psn_online_id = \"" << it->second.psn_online_id << "\"\n";
			}

			if(it->second.psn_account_id.length()){
				config_file << "psn_account_id = \"" << it->second.psn_account_id << "\"\n";
			}

			if(it->second.rp_key_data || it->second.registered){
				char rp_key_type[33] = { 0 };
                snprintf(rp_key_type, sizeof(rp_key_type), "%d", it->second.rp_key_type);
				// save registered rp key for auto login
				config_file << "rp_key = \"" << this->GetRPKey(&it->second) << "\"\n"
					<< "rp_regist_key = \"" << this->GetRPRegistKey(&it->second) << "\"\n"
					<< "rp_key_type = " << rp_key_type << "\n";
			} //
			config_file << "\n";
		} // for host
	} // is_open
	config_file.close();
	return 0;
}

