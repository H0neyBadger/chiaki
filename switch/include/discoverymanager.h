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

#ifndef CHIAKI_DISCOVERYMANAGER_H
#define CHIAKI_DISCOVERYMANAGER_H

#include <map>
#include <string>

#include <chiaki/log.h>
#include <chiaki/discoveryservice.h>

#include "host.h"
#include "settings.h"

static void Discovery(ChiakiDiscoveryHost*, void*);

class DiscoveryManager {
	private:
		Settings *settings;
		ChiakiLog *log;
		ChiakiDiscovery discovery;
		ChiakiDiscoveryThread thread;
		struct sockaddr * host_addr;
		size_t host_addr_len;
	public:

        typedef enum hoststate{
            UNKNOWN,
            READY,
            STANDBY,
            SHUTDOWN,
        } HostState;

		explicit DiscoveryManager(Settings *settings);
		~DiscoveryManager();
		int Send();
		int Send(struct sockaddr *host_addr, size_t host_addr_len);
		int Send(const char *);
		void DiscoveryCB(ChiakiDiscoveryHost*);
};

#endif //CHIAKI_DISCOVERYMANAGER_H
