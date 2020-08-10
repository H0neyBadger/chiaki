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
					if(current_host != nullptr){
						current_host->psn_online_id = value;
					} else {
						// current_host == nullptr
						// means we are in global ini section
						// update default setting
						this->global_psn_online_id = value;
					}
					break;
				case PSN_ACCOUNT_ID:
					CHIAKI_LOGV(this->log, "PSN_ACCOUNT_ID %s", value.c_str());
					if(current_host != nullptr){
						current_host->psn_account_id = value;
					} else {
						this->global_psn_account_id = value;
					}
					break;
				case RP_KEY:
					CHIAKI_LOGV(this->log, "RP_KEY %s", value.c_str());
					if(current_host != nullptr){
						size_t rp_key_sz = sizeof(current_host->rp_key);
						ChiakiErrorCode err = chiaki_base64_decode(
							value.c_str(), value.length(),
							current_host->rp_key, &rp_key_sz);
						if(CHIAKI_ERR_SUCCESS != err){
							CHIAKI_LOGE(this->log, "Failed to parse RP_KEY %s (it must be a base64 encoded)",value.c_str());
						} else {
							rp_key_b=true;
						}
					}
					break;
				case RP_KEY_TYPE:
					CHIAKI_LOGV(this->log, "RP_KEY_TYPE %s", value.c_str());
					if(current_host != nullptr){
						current_host->rp_key_type = std::atoi(value.c_str());
						// TODO Check possible rp_type values
						rp_key_type_b=true;
					}
					break;
				case RP_REGIST_KEY:
					CHIAKI_LOGV(this->log, "RP_REGIST_KEY %s", value.c_str());
					if(current_host != nullptr){
						size_t rp_regist_key_sz = sizeof(current_host->rp_regist_key);
						ChiakiErrorCode err = chiaki_base64_decode(
							value.c_str(), value.length(),
							(uint8_t*) current_host->rp_regist_key, &rp_regist_key_sz);
						if(CHIAKI_ERR_SUCCESS != err){
							CHIAKI_LOGE(this->log, "Failed to parse RP_REGIST_KEY %s (it must be a base64 encoded)",value.c_str());
						} else {
							rp_regist_key_b=true;
						}
					}
					break;
				case VIDEO_RESOLUTION:
					{
						ChiakiVideoResolutionPreset cvrp = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
						if (value.compare("1080p") == 0) {
							cvrp = CHIAKI_VIDEO_RESOLUTION_PRESET_1080p;
						} else if (value.compare("720p") == 0) {
							cvrp = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
						} else if (value.compare("540p") == 0) {
							cvrp = CHIAKI_VIDEO_RESOLUTION_PRESET_540p;
						} else if (value.compare("360p") == 0) {
							cvrp = CHIAKI_VIDEO_RESOLUTION_PRESET_360p;
						}

						if(current_host != nullptr){
							current_host->video_resolution = cvrp;
						} else {
							this->global_video_resolution = cvrp;
						}
					}
					break;
				case VIDEO_FPS:
					{
						ChiakiVideoFPSPreset cvfp = CHIAKI_VIDEO_FPS_PRESET_60;
						if (value.compare("60") == 0) {
							cvfp = CHIAKI_VIDEO_FPS_PRESET_60;
						} else if (value.compare("30") == 0) {
							cvfp = CHIAKI_VIDEO_FPS_PRESET_30;
						}

						if(current_host != nullptr){
							current_host->video_fps = cvfp;
						} else {
							this->global_video_fps = cvfp;
						}
					}
					break;
				case CPU_OVERCLOCK:
					{
						int oc = OC_1326;
						int v = atoi(value.c_str());
						if ( v > OC_1580 ) {
							// max OC
							oc = OC_1785;
						} else if ( OC_1580 >= v && v > OC_1326 ) {
							oc = OC_1580;
						} else if ( OC_1326 >= v && v > OC_1220 ) {
							oc = OC_1326;
						} else if ( OC_1220 >= v && v > OC_1020 ) {
							oc = OC_1220;
						} else if ( OC_1020 >= v ) {
							// no overclock
							// default nintendo switch value
							oc = OC_1020;
						}
						if(current_host != nullptr){
							current_host->cpu_overclock = oc;
						} else {
							this->global_cpu_overclock = oc;
						}
					}
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
		if(this->global_video_resolution){
			switch(this->global_video_resolution){
				case CHIAKI_VIDEO_RESOLUTION_PRESET_360p:
					config_file << "video_resolution = \"360p\"\n";
					break;
				case CHIAKI_VIDEO_RESOLUTION_PRESET_540p:
					config_file << "video_resolution = \"540p\"\n";
					break;
				case CHIAKI_VIDEO_RESOLUTION_PRESET_720p:
					config_file << "video_resolution = \"720p\"\n";
					break;
				case CHIAKI_VIDEO_RESOLUTION_PRESET_1080p:
					config_file << "video_resolution = \"1080p\"\n";
					break;
			}
		}

		if(this->global_video_fps){
			switch(this->global_video_fps){
				case CHIAKI_VIDEO_FPS_PRESET_30:
					config_file << "video_fps = 30\n";
					break;
				case CHIAKI_VIDEO_FPS_PRESET_60:
					config_file << "video_resolution = 60\n";
					break;
			}
		}

		if(this->global_cpu_overclock > 0){
			config_file << "cpu_overclock = \"" << this->global_cpu_overclock << "\"\n";
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

			switch(it->second.video_resolution){
				case CHIAKI_VIDEO_RESOLUTION_PRESET_360p:
					config_file << "video_resolution = \"360p\"\n";
					break;
				case CHIAKI_VIDEO_RESOLUTION_PRESET_540p:
					config_file << "video_resolution = \"540p\"\n";
					break;
				case CHIAKI_VIDEO_RESOLUTION_PRESET_720p:
					config_file << "video_resolution = \"720p\"\n";
					break;
				case CHIAKI_VIDEO_RESOLUTION_PRESET_1080p:
					config_file << "video_resolution = \"1080p\"\n";
					break;
			}

			switch(it->second.video_fps){
				case CHIAKI_VIDEO_FPS_PRESET_30:
					config_file << "video_fps = 30\n";
					break;
				case CHIAKI_VIDEO_FPS_PRESET_60:
					config_file << "video_resolution = 60\n";
					break;
			}

			if(it->second.cpu_overclock > 0){
				config_file << "cpu_overclock = \"" << it->second.cpu_overclock << "\"\n";
			}

			if(it->second.psn_online_id.length()){
				config_file << "psn_online_id = \"" << it->second.psn_online_id << "\"\n";
			}
			if(it->second.psn_account_id.length()){
				config_file << "psn_account_id = \"" << it->second.psn_account_id << "\"\n";
			}
			if(it->second.rp_key_data || it->second.registered){
				// save registered rp key for auto login
				size_t rp_key_b64_sz = this->GetB64encodeSize(0x10);
				size_t rp_regist_key_b64_sz = this->GetB64encodeSize(CHIAKI_SESSION_AUTH_SIZE);
				char rp_key_b64[rp_key_b64_sz + 1] = {0};
				char rp_regist_key_b64[rp_regist_key_b64_sz + 1] = {0};
				char rp_key_type[33] = { 0 };
				//itoa(it->second.rp_key_type, rp_key_type, 10);
				snprintf(rp_key_type, sizeof(rp_key_type), "%d", it->second.rp_key_type);
				ChiakiErrorCode err;
				err = chiaki_base64_encode(
					it->second.rp_key, 0x10,
					rp_key_b64, rp_key_b64_sz);

				err = chiaki_base64_encode(
					(uint8_t *) it->second.rp_regist_key, CHIAKI_SESSION_AUTH_SIZE,
					rp_regist_key_b64, rp_regist_key_b64_sz);

				config_file << "rp_key = \"" << rp_key_b64 << "\"\n"
					<< "rp_regist_key = \"" << rp_regist_key_b64 << "\"\n"
					<< "rp_key_type = " << rp_key_type << "\n";
			} //
			config_file << "\n";
		} // for host
	} // is_open
	config_file.close();
	return 0;
}

