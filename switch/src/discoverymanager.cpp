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
#endif

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <discoverymanager.h>

static void Discovery(ChiakiDiscoveryHost *discovered_host, void *user){
	DiscoveryManager *dm = (DiscoveryManager*) user;
	dm->DiscoveryCB(discovered_host);
}

int DiscoveryManager::Discover(struct sockaddr *host_addr, size_t host_addr_len){
	if(!host_addr)
	{
		CHIAKI_LOGE(log, "Null sockaddr");
		return 1;
	}

	ChiakiDiscovery discovery;
	ChiakiErrorCode err = chiaki_discovery_init(&discovery, log, AF_INET);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(log, "Discovery init failed");
		return 1;
	}

	ChiakiDiscoveryThread thread;
	err = chiaki_discovery_thread_start(&thread, &discovery, Discovery, this);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(log, "Discovery thread init failed");
		chiaki_discovery_fini(&discovery);
		return 1;
	}

	((struct sockaddr_in *)host_addr)->sin_port = htons(CHIAKI_DISCOVERY_PORT);

	ChiakiDiscoveryPacket packet;
	memset(&packet, 0, sizeof(packet));
	packet.cmd = CHIAKI_DISCOVERY_CMD_SRCH;

	chiaki_discovery_send(&discovery, &packet, this->host_addr, this->host_addr_len);
	//FIXME
	sleep(1);
	// join discovery thread
	chiaki_discovery_thread_stop(&thread);
	chiaki_discovery_fini(&discovery);
	return 0;
}

int DiscoveryManager::Discover(const char *discover_ip_dest){
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

		this->host_addr_len = ai->ai_addrlen;
		this->host_addr = (struct sockaddr *)malloc(host_addr_len);
		if(!this->host_addr)
			break;
		memcpy(this->host_addr, ai->ai_addr, this->host_addr_len);
	}
	freeaddrinfo(host_addrinfos);

	if(!this->host_addr){
		CHIAKI_LOGE(log, "Failed to get addr for hostname");
		return 1;
	}
	return DiscoveryManager::Discover(this->host_addr, this->host_addr_len);
}

int DiscoveryManager::Discover(){
#ifdef __SWITCH__
	uint32_t current_addr, subnet_mask;
	// init nintendo net interface service
    Result rc = nifmInitialize(NifmServiceType_User);
    if (R_SUCCEEDED(rc)) {
		// read current IP and netmask
		rc = nifmGetCurrentIpConfigInfo(
			&current_addr, &subnet_mask,
			NULL, NULL, NULL);
        nifmExit();
    } else {
		CHIAKI_LOGE(log, "Failed to get nintendo nifmGetCurrentIpConfigInfo");
		return 1;
	}


	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = current_addr | (~subnet_mask);
    addr.sin_port = htons(CHIAKI_DISCOVERY_PORT);

	this->host_addr_len = sizeof(sockaddr_in);
	this->host_addr = (struct sockaddr *)malloc(host_addr_len);
	memcpy(this->host_addr, &addr, this->host_addr_len);

	CHIAKI_LOGI(log, "Read current addr `%p` mask `%p` broadcast `%p`\n", 
		ntohl(current_addr), ntohl(subnet_mask), ntohl(current_addr | (~subnet_mask)));
	return DiscoveryManager::Discover(this->host_addr, this->host_addr_len);
#else
	return DiscoveryManager::Discover("255.255.255.255");
#endif
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

void DiscoveryManager::DiscoveryCB(ChiakiDiscoveryHost *discovered_host){

	// the user ptr is passed as
	// chiaki_discovery_thread_start arg

	std::string key = discovered_host->host_name;
	Host *host = Host::GetOrCreate(this->log, hosts, &key);

	CHIAKI_LOGI(this->log, "--");
	CHIAKI_LOGI(this->log, "Discovered Host:");
	CHIAKI_LOGI(this->log, "State:                             %s", chiaki_discovery_host_state_string(discovered_host->state));
	/* host attr
	uint32_t host_addr;
	int system_version;
	int device_discovery_protocol_version;
	std::string host_name;
	std::string host_type;
	std::string host_id;
	*/
	host->state = discovered_host->state;

	// add host ptr to list
	if(discovered_host->system_version){
		// example: 07020001
		host->system_version = atoi(discovered_host->system_version);
		CHIAKI_LOGI(this->log, "System Version:                    %s", discovered_host->system_version);
	}
	if(discovered_host->device_discovery_protocol_version)
		host->device_discovery_protocol_version = atoi(discovered_host->device_discovery_protocol_version);
		CHIAKI_LOGI(this->log, "Device Discovery Protocol Version: %s", discovered_host->device_discovery_protocol_version);

	if(discovered_host->host_request_port)
		CHIAKI_LOGI(this->log, "Request Port:                      %hu", (unsigned short)discovered_host->host_request_port);

	if(discovered_host->host_addr)
		host->host_addr = discovered_host->host_addr;
		CHIAKI_LOGI(this->log, "Host Addr:                         %s", discovered_host->host_addr);

	if(discovered_host->host_name)
		// FIXME
		host->host_name = discovered_host->host_name;
		CHIAKI_LOGI(this->log, "Host Name:                         %s", discovered_host->host_name);

	if(discovered_host->host_type)
		CHIAKI_LOGI(this->log, "Host Type:                         %s", discovered_host->host_type);

	if(discovered_host->host_id)
		CHIAKI_LOGI(this->log, "Host ID:                           %s", discovered_host->host_id);

	if(discovered_host->running_app_titleid)
		CHIAKI_LOGI(this->log, "Running App Title ID:              %s", discovered_host->running_app_titleid);

	if(discovered_host->running_app_name)
		CHIAKI_LOGI(this->log, "Running App Name:                  %s", discovered_host->running_app_name);

	CHIAKI_LOGI(this->log, "--");
}

