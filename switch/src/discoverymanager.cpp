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

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <discoverymanager.h>

static void DiscoveryCB(ChiakiDiscoveryHost *host, void *user)
{
	// the user ptr is passed as
	// chiaki_discovery_thread_start arg
	std::map<std::string, Host> *hosts = (std::map<std::string, Host>*) user;
	std::string key = host->host_name;
	Host *n_host = Host::GetOrCreate(hosts, &key);

	ChiakiLog* log = nullptr;
	CHIAKI_LOGI(log, "--");
	CHIAKI_LOGI(log, "Discovered Host:");
	CHIAKI_LOGI(log, "State:                             %s", chiaki_discovery_host_state_string(host->state));
	/* host attr
	uint32_t host_addr;
	int system_version;
	int device_discovery_protocol_version;
	std::string host_name;
	std::string host_type;
	std::string host_id;
	*/
	n_host->state = host->state;
	// add host ptr to list
	if(host->system_version){
		// example: 07020001
		n_host->system_version = atoi(host->system_version);
		CHIAKI_LOGI(log, "System Version:                    %s", host->system_version);
	}
	if(host->device_discovery_protocol_version)
		n_host->device_discovery_protocol_version = atoi(host->device_discovery_protocol_version);
		CHIAKI_LOGI(log, "Device Discovery Protocol Version: %s", host->device_discovery_protocol_version);

	if(host->host_request_port)
		CHIAKI_LOGI(log, "Request Port:                      %hu", (unsigned short)host->host_request_port);

	if(host->host_addr)
		n_host->host_addr = host->host_addr;
		CHIAKI_LOGI(log, "Host Addr:                         %s", host->host_addr);

	if(host->host_name)
		// FIXME
		n_host->host_name = host->host_name;
		CHIAKI_LOGI(log, "Host Name:                         %s", host->host_name);

	if(host->host_type)
		CHIAKI_LOGI(log, "Host Type:                         %s", host->host_type);

	if(host->host_id)
		CHIAKI_LOGI(log, "Host ID:                           %s", host->host_id);

	if(host->running_app_titleid)
		CHIAKI_LOGI(log, "Running App Title ID:              %s", host->running_app_titleid);

	if(host->running_app_name)
		CHIAKI_LOGI(log, "Running App Name:                  %s", host->running_app_name);

	CHIAKI_LOGI(log, "--");
}

int DiscoveryManager::ParseSettings(){
	/*
	std::string ap_ssid;
	std::string ap_bssid;
	std::string ap_key;
	std::string ap_name;
	std::string ps4_nickname;
	// mac address = 48 bits
	uint8_t ps4_mac[6];
	char rp_regist_key[CHIAKI_SESSION_AUTH_SIZE];
	uint32_t rp_key_type;
	uint8_t rp_key[0x10];
	*/
	return 0;
}


int DiscoveryManager::Discover(const char *discover_ip_dest)
{
	// use broadcast address to discover host form local network
	// char *host = "255.255.255.255";
	ChiakiDiscovery discovery;
	ChiakiErrorCode err = chiaki_discovery_init(&discovery, log, AF_INET); // TODO: IPv6
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(log, "Discovery init failed");
		return 1;
	}

	ChiakiDiscoveryThread thread;
	err = chiaki_discovery_thread_start(&thread, &discovery, DiscoveryCB, hosts);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(log, "Discovery thread init failed");
		chiaki_discovery_fini(&discovery);
		return 1;
	}

	struct addrinfo *host_addrinfos;
	int r = getaddrinfo(discover_ip_dest, NULL, NULL, &host_addrinfos);
	if(r != 0)
	{
		CHIAKI_LOGE(log, "getaddrinfo failed");
		return 1;
	}

	struct sockaddr *host_addr = NULL;
	socklen_t host_addr_len = 0;
	for(struct addrinfo *ai=host_addrinfos; ai; ai=ai->ai_next)
	{
		if(ai->ai_protocol != IPPROTO_UDP)
			continue;
		if(ai->ai_family != AF_INET) // TODO: IPv6
			continue;

		host_addr_len = ai->ai_addrlen;
		host_addr = (struct sockaddr *)malloc(host_addr_len);
		if(!host_addr)
			break;
		memcpy(host_addr, ai->ai_addr, host_addr_len);
	}
	freeaddrinfo(host_addrinfos);

	if(!host_addr)
	{
		CHIAKI_LOGE(log, "Failed to get addr for hostname");
		return 1;
	}

	((struct sockaddr_in *)host_addr)->sin_port = htons(CHIAKI_DISCOVERY_PORT); // TODO: IPv6

	ChiakiDiscoveryPacket packet;
	memset(&packet, 0, sizeof(packet));
	packet.cmd = CHIAKI_DISCOVERY_CMD_SRCH;

	chiaki_discovery_send(&discovery, &packet, host_addr, host_addr_len);
	//FIXME
	sleep(1);
	// join discovery thread
	chiaki_discovery_thread_stop(&thread);
	chiaki_discovery_fini(&discovery);
	return 0;
}

Host* DiscoveryManager::GetHost(std::string ps4_nickname){
	auto it = hosts->find(ps4_nickname);
	if (it != hosts->end()){
		return &(hosts->at(ps4_nickname));
	} else {
		// object not found
		return 0;
	}
}
