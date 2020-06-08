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

#include <chiaki/discovery.h>

#include "gui.h"

//https://github.com/XorTroll/Goldleaf/blob/0.9-dev/Goldleaf/Source/ui/ui_ClickableImage.cpp
ClickableImage::ClickableImage(s32 X, s32 Y, pu::String Image) : pu::ui::elm::Image::Image(X, Y, Image) {
	this->cb = [&](){};
	this->touched = false;
}

void ClickableImage::SetOnClick(std::function<void()> Callback)
{
	this->cb = Callback;
}

void ClickableImage::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch Pos)
{
	if(touched)
	{
		auto tpnow = std::chrono::steady_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(tpnow - touchtp).count();
		if(diff >= 200)
		{
			touched = false;
			(this->cb)();
		}
	}
	else if(!Pos.IsEmpty())
	{
		touchPosition tch;
		hidTouchRead(&tch, 0);
		int w = this->GetWidth();
		int h = this->GetHeight();
		if((Pos.X >= this->GetProcessedX()) && (Pos.X < (this->GetProcessedX() + w))
			&& (Pos.Y >= this->GetProcessedY()) && (Pos.Y < (this->GetProcessedY() + h)))
		{
			touchtp = std::chrono::steady_clock::now();
			touched = true;
		}
	}
}

SettingLayout::SettingLayout(): pu::ui::Layout::Layout() {
	/*
	this->menu = pu::ui::elm::Menu::New(0,100,1280,
		pu::ui::Color(0,0,0,0), 200, (620 / 200));

	pu::ui::elm::MenuItem::Ref host_item = pu::ui::elm::MenuItem::New(text);
	host_item->SetColor(pu::ui::Color(0,0,0,0));
	*/
	this->button = ClickableImage::New(300, 300, "romfs:/discover-24px.svg");
	this->button->SetWidth(40);
	this->button->SetHeight(40);
	//this->button->SetOnClick();
	this->Add(this->button);
}

/*
SettingLayout::Update() {
	this->menu = pu::ui::elm::Menu::New(0,100,1280,
		pu::ui::Color(0,0,0,0), 200, (620 / 200));
	std::String resolution_str;
	this->settings->GetResolutionString(&resolution_str);
	this->resolution = pu::ui::elm::MenuItem::New("resolution: " + resolution_str);

	this->button = ClickableImage::New(300, 300, "romfs:/discover-24px.svg");
	this->button->SetWidth(40);
	this->button->SetHeight(40);
	//this->button->SetOnClick();
	this->Add(this->button);
}
*/

AddLayout::AddLayout(): pu::ui::Layout::Layout() {

	this->button = ClickableImage::New(300, 300, "romfs:/discover-24px.svg");
	this->button->SetWidth(40);
	this->button->SetHeight(40);
	//this->button->SetOnClick(test);
	this->Add(this->button);
}

MainLayout::MainLayout(std::map<std::string, Host> * hosts,
	std::function<void(Host *)> SetHostFn,
	std::function<void(Host *)> WakeupHostFn,
	std::function<void(Host *)> ConfigureHostFn):
		pu::ui::Layout::Layout(),
		hosts(hosts),
		SetHostFn(SetHostFn),
		WakeupHostFn(WakeupHostFn),
		ConfigureHostFn(ConfigureHostFn){
	// 1280 * 720
	// upper left
	/*
	this->hosts = hosts;
	this->SetHostFn = SetHostFn;
	this->WakeupHostFn = WakeupHostFn;
	this->ConfigureHostFn = ConfigureHostFn;
	*/

	this->discover_button = ClickableImage::New(30, 30, "romfs:/discover-24px.svg");
	this->discover_button->SetWidth(40);
	this->discover_button->SetHeight(40);
	//this->discover_button->SetOnClick();
	this->Add(this->discover_button);

	// upper right
	this->add_button = ClickableImage::New(1160, 30, "romfs:/add-24px.svg");
	this->add_button->SetWidth(40);
	this->add_button->SetHeight(40);
	this->Add(this->add_button);
	// upper right

	this->setting_button = ClickableImage::New(1220, 30, "romfs:/settings-20px.svg");
	this->setting_button->SetWidth(40);
	this->setting_button->SetHeight(40);
	this->Add(this->setting_button);

	// default backgroud message
	this->no_host_found = pu::ui::elm::TextBlock::New(30, 100, "No PS4 Host discovered on LAN,\n"
		"Please turn on your PS4");
	this->Add(this->no_host_found);

	this->console_menu = pu::ui::elm::Menu::New(0,100,1280,
		pu::ui::Color(100,100,100,100), 200, (620 / 200));

	this->Add(this->console_menu);
	// discovery loop
	this->AddThread(std::bind(&MainLayout::UpdateValues, this));
}

bool MainLayout::UpdateOrCreateHostMenuItem(Host * host){
	bool ret = false;
	std::string text = "hostname: " + host->host_name + "\n"
						+ "IP: " + host->host_addr + "\n"
						+ "state: " + chiaki_discovery_host_state_string(host->state) + "\n"
						+ "discovered: " + (host->discovered ? "true": "false") + "\n"
						+ "registered: " + (host->registered ? "true": "false") + "\n"
						+ "host id: " + (host->host_id);

    if ( this->host_menuitems.find(host->host_name) == this->host_menuitems.end() ) {
        // create host if udefined
        this->host_menuitems[host->host_name] = pu::ui::elm::MenuItem::New(text);
		this->console_menu->AddItem(this->host_menuitems[host->host_name]);
		this->host_menuitems[host->host_name]->SetIcon("romfs:/console.svg");
		this->host_menuitems[host->host_name]->SetColor(pu::ui::Color(0,0,0,0));
		// bind menu item to specific host
		this->host_menuitems[host->host_name]->AddOnClick(std::bind(this->SetHostFn, host));
		this->host_menuitems[host->host_name]->AddOnClick(std::bind(this->WakeupHostFn, host), KEY_Y);
		this->host_menuitems[host->host_name]->AddOnClick(std::bind(this->ConfigureHostFn, host), KEY_X);
		ret = false;
    } else {
		// update with latest text
		this->host_menuitems[host->host_name]->SetName(text);
    	ret = true;
	}
	return ret;
}

void MainLayout::UpdateValues() {
	for(auto it = this->hosts->begin(); it != this->hosts->end(); it++){
		this->UpdateOrCreateHostMenuItem(&it->second);
	}
}


void MainApplication::OnLoad() {
	std::function<void(Host *)> host_cb = std::bind(&MainApplication::SetHostCallback, this, std::placeholders::_1);
	std::function<void(Host *)> host_wakeup_cb = std::bind(&MainApplication::WakeupHostCallback, this, std::placeholders::_1);
	std::function<void(Host *)> host_setting_cb = std::bind(&MainApplication::ConfigureHostCallback, this, std::placeholders::_1);
	this->main_layout = MainLayout::New(this->hosts, host_cb, host_wakeup_cb, host_setting_cb);
	this->main_layout->add_button->SetOnClick(std::bind(&MainApplication::LoadAddLayout, this));
	this->main_layout->setting_button->SetOnClick(std::bind(&MainApplication::LoadSettingLayout, this));
	this->LoadLayout(this->main_layout);

	this->add_layout = AddLayout::New();
	this->add_layout->SetOnInput(std::bind(&MainApplication::AddInput, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	this->setting_layout = SettingLayout::New();
	this->setting_layout->SetOnInput(std::bind(&MainApplication::SettingInput, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void MainApplication::ReturnToMainMenu() {
	this->LoadLayout(this->main_layout);
}

void MainApplication::SettingInput(u64 down, u64 up, u64 held) {
	//this->setting_layout->UpdateState();
	if(down & KEY_B) this->ReturnToMainMenu();
}

void MainApplication::AddInput(u64 down, u64 up, u64 held) {
	//this->setting_layout->UpdateState();
	if(down & KEY_B) this->ReturnToMainMenu();
}

void MainApplication::LoadSettingLayout() {
	this->LoadLayout(this->setting_layout);
}

void MainApplication::LoadAddLayout() {
	this->LoadLayout(this->add_layout);
}

void MainApplication::SetHostCallback(Host * host) {
	this->host = host;
	char pin_input[9];
	int retry = 0;

	if(host->state != CHIAKI_DISCOVERY_HOST_STATE_READY) {
		// host in standby mode
		this->CreateShowDialog("Failed to initiate session", "Please turn on your PS4", { "OK" }, true);
	} else if(!host->rp_key_data) {
		// the host is not registered yet
		this->CreateShowDialog("Initiate session", "Please enter PS4 registration PIN code", { "OK" }, true);
		// spawn keyboard
		while(retry == 0){
			io->ReadUserKeyboard(pin_input, sizeof(pin_input));
			host->Register(pin_input);
			// FIXME
			sleep(1);
			if(!host->rp_key_data || !host->registered){
				// registration success
				retry = this->CreateShowDialog("Session Registration Failed", "Please verify your PS4 settings", { "Retry", "Cancel" }, true);
			} else {
				break;
			}
		}
	}
	if(!host->rp_key_data) {
		host->ConnectSession(this->io);
		this->Close();
	}
}

void MainApplication::WakeupHostCallback(Host * host) {
	host->Wakeup();
	this->CreateShowDialog("Wakeup", "PS4 Wakeup packet sent", { "OK" }, true);
}

void MainApplication::ConfigureHostCallback(Host * host) {
	int ret = this->CreateShowDialog("Settings", "TODO", { "OK" }, true);
}

Host * MainApplication::GetHost() {
	return this->host;
}
