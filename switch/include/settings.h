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

#ifndef CHIAKI_SETTINGS_H
#define CHIAKI_SETTINGS_H

#include <regex>

#include "host.h"
#include <chiaki/log.h>

class Settings {
	private:
		ChiakiLog *log = nullptr;
		const char * filename = "chiaki.conf";
		std::map<std::string, Host> *hosts;

		typedef enum configurationitem{
			UNKNOWN,
			HOST_NAME,
			HOST_IP,
			PSN_ONLINE_ID,
			PSN_ACCOUNT_ID,
			RP_KEY,
			RP_KEY_TYPE,
			RP_REGIST_KEY,
			VIDEO_RESOLUTION,
			VIDEO_FPS,
		} ConfigurationItem;

		// dummy parser implementation
		// the aim is not to have bulletproof parser
		// the goal is to read/write inernal flat configuration file
		const std::map<Settings::ConfigurationItem, std::regex> re_map = {
			{ HOST_NAME, std::regex("^\\[\\s*(.+)\\s*\\]") },
			{ HOST_IP, std::regex("^\\s*host_ip\\s*=\\s*\"?(\\d+\\.\\d+\\.\\d+\\.\\d+)\"?") },
			{ PSN_ONLINE_ID, std::regex("^\\s*psn_online_id\\s*=\\s*\"?(\\w+)\"?") },
			{ PSN_ACCOUNT_ID, std::regex("^\\s*psn_account_id\\s*=\\s*\"?([\\w/=+]+)\"?") },
			{ RP_KEY, std::regex("^\\s*rp_key\\s*=\\s*\"?([\\w/=+]+)\"?") },
			{ RP_KEY_TYPE, std::regex("^\\s*rp_key_type\\s*=\\s*\"?(\\d)\"?") },
			{ RP_REGIST_KEY, std::regex("^\\s*rp_regist_key\\s*=\\s*\"?([\\w/=+]+)\"?") },
			{ VIDEO_RESOLUTION, std::regex("^\\s*video_resolution\\s*=\\s*\"?(1080p|720p|540p|360p)\"?") },
			{ VIDEO_FPS, std::regex("^\\s*video_fps\\s*=\\s*\"?(60|30)\"?") },
		};

		ConfigurationItem ParseLine(std::string *line, std::string *value);
		size_t GetB64encodeSize(size_t);
	public:
		Settings(ChiakiLog * log, std::map<std::string, Host> *hosts): log(log), hosts(hosts){};
		void ParseFile();
		int WriteFile();
};

#endif // CHIAKI_SETTINGS_H
