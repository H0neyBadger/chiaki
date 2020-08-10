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


#include "gui.h"

SettingLayout::SettingLayout(std::function<void(chiaki::ui::CustomDialog::Ref)> show_custom_dialog_cb):
	pu::ui::Layout::Layout(),
	show_custom_dialog_cb(show_custom_dialog_cb){
	// main layout
	this->menu_color = pu::ui::Color(224,224,224,255);
	this->menu_focus_color = pu::ui::Color(192,192,192,255);
	this->setting_menu = pu::ui::elm::Menu::New(0,100,1280,
		this->menu_color, 60, 5);
	this->setting_menu->SetOnFocusColor(this->menu_focus_color);

	this->account_item = pu::ui::elm::MenuItem::New("PSN Account");
	this->resolution_item = pu::ui::elm::MenuItem::New("Resolution");
	this->fps_item = pu::ui::elm::MenuItem::New("FPS");
	this->overclock_item = pu::ui::elm::MenuItem::New("Overclock");
	this->ip_item = pu::ui::elm::MenuItem::New("PS4 IP Address");

	this->setting_menu->AddItem(account_item);
	this->setting_menu->AddItem(resolution_item);
	this->setting_menu->AddItem(fps_item);
	this->setting_menu->AddItem(overclock_item);
	this->setting_menu->AddItem(ip_item);

	this->Add(this->setting_menu);

	// nested custom menu
	int x = 300, y = 300, width = 300, item_size = 60;

	this->resolution_menu = pu::ui::elm::Menu::New(x,y,width,
		this->menu_color, item_size, 3);
	this->resolution_menu->SetOnFocusColor(this->menu_focus_color);
	this->res_720p = pu::ui::elm::MenuItem::New("720p");
	this->res_540p = pu::ui::elm::MenuItem::New("540p");
	this->res_360p = pu::ui::elm::MenuItem::New("360p");
	this->resolution_menu->AddItem(res_720p);
	this->resolution_menu->AddItem(res_540p);
	this->resolution_menu->AddItem(res_360p);
	this->resolution_dialog = chiaki::ui::CustomDialog::New("Video",
		"Resolution", this->resolution_menu);

	this->fps_menu = pu::ui::elm::Menu::New(x,y,width,
		this->menu_color, item_size, 2);
	this->fps_menu->SetOnFocusColor(this->menu_focus_color);
	this->fps_60 = pu::ui::elm::MenuItem::New("60 FPS");
	this->fps_30 = pu::ui::elm::MenuItem::New("30 FPS");
	this->fps_menu->AddItem(fps_60);
	this->fps_menu->AddItem(fps_30);
	this->fps_dialog = chiaki::ui::CustomDialog::New("Video",
		"FPS", this->fps_menu);


	this->overclock_menu = pu::ui::elm::Menu::New(x,y,width,
		this->menu_color, item_size, 5);
	this->overclock_menu->SetOnFocusColor(this->menu_focus_color);
	this->oc_1785 = pu::ui::elm::MenuItem::New("1785 MHz (max)");
	this->oc_1580 = pu::ui::elm::MenuItem::New("1580 MHz");
	this->oc_1326 = pu::ui::elm::MenuItem::New("1326 MHz");
	this->oc_1220 = pu::ui::elm::MenuItem::New("1220 MHz");
	this->oc_1020 = pu::ui::elm::MenuItem::New("1020 MHz (default)");
	this->overclock_menu->AddItem(oc_1785);
	this->overclock_menu->AddItem(oc_1580);
	this->overclock_menu->AddItem(oc_1326);
	this->overclock_menu->AddItem(oc_1220);
	this->overclock_menu->AddItem(oc_1020);
	this->overclock_dialog = chiaki::ui::CustomDialog::New("CPU",
		"OverClock", this->overclock_menu);

	// this->account_item->AddOnClick(std::function< void()> Callback, u64 Key=KEY_A);
	this->resolution_item->AddOnClick(std::bind(show_custom_dialog_cb, this->resolution_dialog));
	this->fps_item->AddOnClick(std::bind(show_custom_dialog_cb, this->fps_dialog));
	this->overclock_item->AddOnClick(std::bind(show_custom_dialog_cb, this->overclock_dialog));
	// this->ip_item->AddOnClick(std::function< void()> Callback, u64 Key=KEY_A);


}



/*
SettingLayout::Update() {
	this->menu = pu::ui::elm::Menu::New(0,100,1280,
		pu::ui::Color(0,0,0,0), 200, (620 / 200));
	std::String resolution_str;
	this->settings->GetResolutionString(&resolution_str);
	this->resolution = pu::ui::elm::MenuItem::New("resolution: " + resolution_str);

	this->button = chiaki::ui::ClickableImage::New(300, 300, "romfs:/discover-24px.svg");
	this->button->SetWidth(40);
	this->button->SetHeight(40);
	//this->button->SetOnClick();
	this->Add(this->button);
}
*/

AddLayout::AddLayout(): pu::ui::Layout::Layout() {
	// TODO
	this->button = chiaki::ui::ClickableImage::New(300, 300, "romfs:/discover-24px.svg");
	this->button->SetWidth(40);
	this->button->SetHeight(40);
	//this->button->SetOnClick(test);
	this->Add(this->button);
}

MainLayout::MainLayout(std::map<std::string, Host> * hosts,
	std::function<void(Host *)> DiscoverySendFn,
	std::function<void(Host *)> SetHostFn,
	std::function<void(Host *)> WakeupHostFn,
	std::function<void(Host *)> ConfigureHostFn):
		pu::ui::Layout::Layout(),
		hosts(hosts),
		DiscoverySendFn(DiscoverySendFn),
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

	this->discover_button = chiaki::ui::ClickableImage::New(30, 30, "romfs:/discover-24px.svg");
	this->discover_button->SetWidth(40);
	this->discover_button->SetHeight(40);
	//this->discover_button->SetOnClick();
	this->Add(this->discover_button);

	// upper right
	this->add_button = chiaki::ui::ClickableImage::New(1160, 30, "romfs:/add-24px.svg");
	this->add_button->SetWidth(40);
	this->add_button->SetHeight(40);
	this->Add(this->add_button);
	// upper right

	this->setting_button = chiaki::ui::ClickableImage::New(1220, 30, "romfs:/settings-20px.svg");
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
	// do not run every time
	this->thread_counter++;
	this->thread_counter%=60;
	if(this->thread_counter == 0){
		// send broadcast discovery
		this->DiscoverySendFn(nullptr);
		for(auto it = this->hosts->begin(); it != this->hosts->end(); it++){
			// send broadcast discovery
			this->DiscoverySendFn(&it->second);
			this->UpdateOrCreateHostMenuItem(&it->second);
		}
	}
}


void MainApplication::OnLoad() {
	std::function<void(Host *)> discoverysend_cb = std::bind(&MainApplication::DiscoverySendCallback, this, std::placeholders::_1);
	std::function<void(Host *)> host_cb = std::bind(&MainApplication::SetHostCallback, this, std::placeholders::_1);
	std::function<void(Host *)> host_wakeup_cb = std::bind(&MainApplication::WakeupHostCallback, this, std::placeholders::_1);
	std::function<void(Host *)> host_setting_cb = std::bind(&MainApplication::ConfigureHostCallback, this, std::placeholders::_1);
	this->main_layout = MainLayout::New(this->hosts, discoverysend_cb, host_cb, host_wakeup_cb, host_setting_cb);
	this->main_layout->add_button->SetOnClick(std::bind(&MainApplication::LoadAddLayout, this));
	this->main_layout->setting_button->SetOnClick(std::bind(&MainApplication::LoadSettingLayout, this));
	this->LoadLayout(this->main_layout);

	this->add_layout = AddLayout::New();
	this->add_layout->SetOnInput(std::bind(&MainApplication::AddInput, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	std::function<void(chiaki::ui::CustomDialog::Ref)> show_custom_dialog_cb =
		std::bind(&MainApplication::ShowCustomDialogCallback, this, std::placeholders::_1);
	this->setting_layout = SettingLayout::New(show_custom_dialog_cb);
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

void MainApplication::ShowCustomDialogCallback(chiaki::ui::CustomDialog::Ref custom_dialog) {
	custom_dialog->Show(this->rend, this);
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
	if(host->rp_key_data) {
		host->ConnectSession(this->io);
		this->Close();
	}
}

void MainApplication::WakeupHostCallback(Host * host) {
	if(!host->rp_key_data) {
		// the host is not registered yet
		this->CreateShowDialog("Wakeup", "Please register your PS4 first", { "OK" }, true);
	} else {
		int r = host->Wakeup();
		if(r == 0){
			this->CreateShowDialog("Wakeup", "PS4 Wakeup packet sent", { "OK" }, true);
		} else {
			this->CreateShowDialog("Wakeup", "PS4 Wakeup packet Failed", { "OK" }, true);
		}
	}
}

void MainApplication::ConfigureHostCallback(Host * host) {
	int ret = this->CreateShowDialog("Settings", "TODO", { "OK" }, true);
}

void MainApplication::DiscoverySendCallback(Host * host) {
	if(host){
		this->discoverymanager->Send(host->host_addr.c_str());
	} else {
		// broadcast send discovery
		this->discoverymanager->Send();
	}
}

Host * MainApplication::GetHost() {
	return this->host;
}
