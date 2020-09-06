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

SettingLayout::SettingLayout(
	Host *host,
	Settings *settings,
	IO *io,
	std::function<void(chiaki::ui::CustomDialog::Ref)> show_custom_dialog_cb):
		host(host),
		settings(settings),
		io(io),
		show_custom_dialog_cb(show_custom_dialog_cb) {
	// main setting layout
	// Use host == nullptr to display global settings
	// Use the same object to display General settings
	// and Host advanced settings
	if(this->host == nullptr){
		this->title = pu::ui::elm::TextBlock::New(30, 30, "Global Settings");
	} else {
		std::string host_name_string = settings->GetHostName(host);
		this->title = pu::ui::elm::TextBlock::New(30, 30, host_name_string + " Settings");
	}
	this->title->SetFont("DefaultFont@30");
	this->Add(this->title);


	// default color scheme
	this->menu_color = pu::ui::Color(224,224,224,255);
	this->menu_focus_color = pu::ui::Color(192,192,192,255);
	// main menu holder
	int item_count = 5;
	if(this->host != nullptr){
		item_count = 10;
	}
	this->setting_menu = pu::ui::elm::Menu::New(0,100,1280,
		this->menu_color, 60, item_count);
	this->setting_menu->SetOnFocusColor(this->menu_focus_color);

	// use fake string to create items
	// text values are maintained by UpdateSettings() function
	this->psn_online_id_item = pu::ui::elm::MenuItem::New("PSN Online ID");
	this->psn_account_id_item = pu::ui::elm::MenuItem::New("PSN Account ID (v7.0 and greater)");

	this->video_resolution_item = pu::ui::elm::MenuItem::New("Resolution");
	this->video_fps_item = pu::ui::elm::MenuItem::New("FPS");
#ifdef CHIAKI_ENABLE_SWITCH_OVERCLOCK
	this->cpu_overclock_item = pu::ui::elm::MenuItem::New("Overclock");
#endif
	this->host_ipaddr_item = pu::ui::elm::MenuItem::New("PS4 IP Address");
	this->host_name_item = pu::ui::elm::MenuItem::New("PS4 Hostname");
	this->host_rp_regist_key_item = pu::ui::elm::MenuItem::New("PS4 RP Register Key");
	this->host_rp_key_type_item = pu::ui::elm::MenuItem::New("PS4 RP Key Type");
	this->host_rp_key_item = pu::ui::elm::MenuItem::New("PS4 RP Key");

	// build configuration item's list
	if(this->host != nullptr){
		this->setting_menu->AddItem(this->host_name_item);
		this->setting_menu->AddItem(this->host_ipaddr_item);
	}

	this->setting_menu->AddItem(this->psn_online_id_item);
	this->setting_menu->AddItem(this->psn_account_id_item);
	this->setting_menu->AddItem(this->video_resolution_item);
	this->setting_menu->AddItem(this->video_fps_item);
#ifdef CHIAKI_ENABLE_SWITCH_OVERCLOCK
	this->setting_menu->AddItem(this->cpu_overclock_item);
#endif
	if(this->host != nullptr){
		this->setting_menu->AddItem(this->host_rp_regist_key_item);
		this->setting_menu->AddItem(this->host_rp_key_type_item);
		this->setting_menu->AddItem(this->host_rp_key_item);
	}
	this->Add(this->setting_menu);

	// nested custom menu
	int x = 300, y = 300, width = 300, item_size = 60;

	this->video_resolution_menu = pu::ui::elm::Menu::New(x,y,width,
		this->menu_color, item_size, 3);
	this->video_resolution_menu->SetOnFocusColor(this->menu_focus_color);
	this->video_res_720p = pu::ui::elm::MenuItem::New("720p");
	this->video_res_540p = pu::ui::elm::MenuItem::New("540p");
	this->video_res_360p = pu::ui::elm::MenuItem::New("360p");
	this->video_res_720p->AddOnClick(std::bind(&SettingLayout::SetVideoResolutionCallback, this, CHIAKI_VIDEO_RESOLUTION_PRESET_720p));
    this->video_res_540p->AddOnClick(std::bind(&SettingLayout::SetVideoResolutionCallback, this, CHIAKI_VIDEO_RESOLUTION_PRESET_540p));
    this->video_res_360p->AddOnClick(std::bind(&SettingLayout::SetVideoResolutionCallback, this, CHIAKI_VIDEO_RESOLUTION_PRESET_360p));
	this->video_resolution_menu->AddItem(this->video_res_720p);
	this->video_resolution_menu->AddItem(this->video_res_540p);
	this->video_resolution_menu->AddItem(this->video_res_360p);
	this->video_resolution_dialog = chiaki::ui::CustomDialog::New("Video",
		"Resolution", this->video_resolution_menu);

	this->video_fps_menu = pu::ui::elm::Menu::New(x,y,width,
		this->menu_color, item_size, 2);
	this->video_fps_menu->SetOnFocusColor(this->menu_focus_color);
	this->video_fps_60 = pu::ui::elm::MenuItem::New("60 FPS");
	this->video_fps_30 = pu::ui::elm::MenuItem::New("30 FPS");
	this->video_fps_60->AddOnClick(std::bind(&SettingLayout::SetVideoFPSCallback, this, CHIAKI_VIDEO_FPS_PRESET_60));
	this->video_fps_30->AddOnClick(std::bind(&SettingLayout::SetVideoFPSCallback, this, CHIAKI_VIDEO_FPS_PRESET_30));
	this->video_fps_menu->AddItem(this->video_fps_60);
	this->video_fps_menu->AddItem(this->video_fps_30);
	this->video_fps_dialog = chiaki::ui::CustomDialog::New("Video",
		"FPS", this->video_fps_menu);
#ifdef CHIAKI_ENABLE_SWITCH_OVERCLOCK
	this->cpu_overclock_menu = pu::ui::elm::Menu::New(x,y,width,
		this->menu_color, item_size, 5);
	this->cpu_overclock_menu->SetOnFocusColor(this->menu_focus_color);
	this->cpu_oc_1785 = pu::ui::elm::MenuItem::New("1785 MHz (max)");
	this->cpu_oc_1580 = pu::ui::elm::MenuItem::New("1580 MHz");
	this->cpu_oc_1326 = pu::ui::elm::MenuItem::New("1326 MHz");
	this->cpu_oc_1220 = pu::ui::elm::MenuItem::New("1220 MHz");
	this->cpu_oc_1020 = pu::ui::elm::MenuItem::New("1020 MHz (default)");
	this->cpu_oc_1785->AddOnClick(std::bind(&SettingLayout::SetCPUOverclockCallback, this, OC_1785));
	this->cpu_oc_1580->AddOnClick(std::bind(&SettingLayout::SetCPUOverclockCallback, this, OC_1580));
	this->cpu_oc_1326->AddOnClick(std::bind(&SettingLayout::SetCPUOverclockCallback, this, OC_1326));
	this->cpu_oc_1220->AddOnClick(std::bind(&SettingLayout::SetCPUOverclockCallback, this, OC_1220));
	this->cpu_oc_1020->AddOnClick(std::bind(&SettingLayout::SetCPUOverclockCallback, this, OC_1020));
	this->cpu_overclock_menu->AddItem(this->cpu_oc_1785);
	this->cpu_overclock_menu->AddItem(this->cpu_oc_1580);
	this->cpu_overclock_menu->AddItem(this->cpu_oc_1326);
	this->cpu_overclock_menu->AddItem(this->cpu_oc_1220);
	this->cpu_overclock_menu->AddItem(this->cpu_oc_1020);
	this->cpu_overclock_dialog = chiaki::ui::CustomDialog::New("CPU",
		"OverClock", this->cpu_overclock_menu);
#endif

	// build call back/action system

	this->psn_account_id_item->AddOnClick(std::bind(&SettingLayout::SetPSNAccountIDCallback, this));
	this->psn_online_id_item->AddOnClick(std::bind(&SettingLayout::SetPSNOnlineIDCallback, this));

	this->video_resolution_item->AddOnClick(std::bind(show_custom_dialog_cb, this->video_resolution_dialog));
	this->video_fps_item->AddOnClick(std::bind(show_custom_dialog_cb, this->video_fps_dialog));
#ifdef CHIAKI_ENABLE_SWITCH_OVERCLOCK
	this->cpu_overclock_item->AddOnClick(std::bind(show_custom_dialog_cb, this->cpu_overclock_dialog));
#endif
	// this->host_ipaddr_item->AddOnClick(std::function< void()> Callback, u64 Key=KEY_A);

	// Update string info
	this->UpdateSettings();
}

void SettingLayout::SetPSNAccountIDCallback(){
	char psn_account_id[255];
	bool input = io->ReadUserKeyboard(psn_account_id, sizeof(psn_account_id));
	if(input){
		settings->SetPSNAccountID(this->host, psn_account_id);
		this->UpdateSettings();
	}
}

void SettingLayout::SetPSNOnlineIDCallback(){
	char psn_online_id[255];
	bool input = io->ReadUserKeyboard(psn_online_id, sizeof(psn_online_id));
	if(input){
		settings->SetPSNOnlineID(this->host, psn_online_id);
		this->UpdateSettings();
	}
}

void SettingLayout::SetVideoResolutionCallback(ChiakiVideoResolutionPreset value){
	settings->SetVideoResolution(this->host, value);
	this->UpdateSettings();
}

void SettingLayout::SetVideoFPSCallback(ChiakiVideoFPSPreset value){
	settings->SetVideoFPS(this->host, value);
	this->UpdateSettings();
}

#ifdef CHIAKI_ENABLE_SWITCH_OVERCLOCK
void SettingLayout::SetCPUOverclockCallback(int cpu_overclock){
	settings->SetCPUOverclock(this->host, cpu_overclock);
	this->UpdateSettings();
}
#endif

// synchronize settings on disk and gui
void SettingLayout::UpdateSettings(){
	// push changes to local file
	this->settings->WriteFile();
	// return global_settings ids when host == nullptr

	std::string psn_online_id_string = settings->GetPSNOnlineID(host);
	this->psn_online_id_item->SetName("PSN Online ID: "
		+ psn_online_id_string);

	std::string psn_account_id_string = settings->GetPSNAccountID(host);
	this->psn_account_id_item->SetName("PSN Account ID (v7.0 and greater): "
		+ psn_account_id_string);

	std::string video_resolution_string = settings->ResolutionPresetToString(
		settings->GetVideoResolution(host));

	this->video_resolution_item->SetName("Video Resolution: "
		+ video_resolution_string);

	std::string video_fps_string = settings->FPSPresetToString(
		settings->GetVideoFPS(host));

	this->video_fps_item->SetName("Video FPS: "
		+ video_fps_string);

#ifdef CHIAKI_ENABLE_SWITCH_OVERCLOCK
	std::string cpu_overclock_string = std::to_string(settings->GetCPUOverclock(host));
	this->cpu_overclock_item->SetName("CPU Overclock: "
		+ cpu_overclock_string);
#endif

	if(this->host != nullptr){
		std::string host_name_string = settings->GetHostName(host);
		this->host_name_item->SetName("PS4 Hostname: "
			+ host_name_string);

		std::string host_ipaddr_string = settings->GetHostIPAddr(host);
		this->host_ipaddr_item->SetName("PS4 IP Address: "
			+ host_ipaddr_string);

		std::string host_rp_regist_key_string = settings->GetHostRPRegistKey(host);
		this->host_rp_regist_key_item->SetName("RP Register Key: "
			+ host_rp_regist_key_string);

		std::string host_rp_key_string = settings->GetHostRPKey(host);
		this->host_rp_key_item->SetName("RP Key: "
			+ host_rp_key_string);

		std::string host_rp_key_type_string = std::to_string(settings->GetHostRPKeyType(host));
		this->host_rp_key_type_item->SetName("RP Key type: "
			+ host_rp_key_type_string);
	}
	// FIXME: hack to force ReloadItemRenders
	int idx = this->setting_menu->GetSelectedIndex();
	this->setting_menu->SetSelectedIndex(idx);
}


AddLayout::AddLayout(): pu::ui::Layout::Layout() {
	// TODO
	this->button = chiaki::ui::ClickableImage::New(300, 300, "romfs:/discover-24px.svg");
	this->button->SetWidth(40);
	this->button->SetHeight(40);
	//this->button->SetOnClick(test);
	this->Add(this->button);
}

MainLayout::MainLayout(
	std::map<std::string, Host> * hosts,
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
	// TODO
	this->discover_button = chiaki::ui::ClickableImage::New(30, 30, "romfs:/discover-24px.svg");
	this->discover_button->SetWidth(40);
	this->discover_button->SetHeight(40);
	//this->discover_button->SetOnClick();
	this->Add(this->discover_button);

	// upper right
	// TODO
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

	// Host color scheme
	pu::ui::Color menu_color = pu::ui::Color(224,224,224,255);
	pu::ui::Color menu_focus_color = pu::ui::Color(192,192,192,255);
	this->console_menu = pu::ui::elm::Menu::New(0,100,1280,
		menu_color, 200, (620 / 200));
	this->console_menu->SetOnFocusColor(menu_focus_color);

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
	// FIXME: hack to force ReloadItemRenders
	int idx = this->console_menu->GetSelectedIndex();
	this->console_menu->SetSelectedIndex(idx);
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
	// use Host == nullptr to get global settings only
	// share setting to handle user UI
	// share IO to prompt interactiv keybord
	// show_custom_dialog_cb callback that show global settings layout in front
	this->setting_layout = SettingLayout::New(nullptr, this->settings, this->io, show_custom_dialog_cb);
	this->setting_layout->SetOnInput(std::bind(&MainApplication::SettingInput, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	// FIXME: we cannot use the EventConnectedCallback because sdl2 is already in used by plutonium
	// durring the connection process all audio/video .. objects are initialized.
	// so we have to release all plutonium object before entering into chiaki session
	// this->io->SetEventConnectedCallback(std::bind(&MainApplication::HostConnectedCallback, this));
	//
	this->io->SetEventLoginPinRequestCallback(std::bind(&MainApplication::HostPinCodeCallback, this, std::placeholders::_1));
	// this->io->SetEventQuit();
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

	if(host->state != CHIAKI_DISCOVERY_HOST_STATE_READY) {
		// host in standby mode
		this->CreateShowDialog("Failed to initiate session", "Please turn on your PS4", { "OK" }, true);
		return;
	}

	// TODO test if account exist ?
	if(false) {
		// account not configured
		this->CreateShowDialog("Failed to initiate session", "No PSN account provided", { "OK" }, true);
		return;
	}

	if(!host->rp_key_data){
		// add HostConnected function to regist_event_type_finished_success
		this->host->SetRegistEventTypeFinishedSuccess(std::bind(&MainApplication::HostConnectSession, this));
		this->HostPinCodeCallback(false);
	} else {
		// the host is already registered
		// start session directly
		this->HostConnectSession();
	}
}

bool MainApplication::HostPinCodeCallback(bool pin_incorrect){
	bool pin_provided = false;
	if(pin_incorrect) {
		bool retry = this->CreateShowDialog("Session Registration Failed", "Please verify your PS4 settings", { "Retry", "Cancel" }, true);
		if(retry != 0){
			return false;
		}
	} else {
		// the host is not registered yet
		this->CreateShowDialog("Initiate session", "Please enter PS4 registration PIN code", { "OK" }, true);
	}
	char pin_input[9] = {0};
	pin_provided = io->ReadUserKeyboard(pin_input, sizeof(pin_input));
	if(pin_provided){
		this->host->Register(pin_input);
	} else {
		// user canceled the pin registration process
		// this->CreateShowDialog("Initiate session", "No code provided", { "OK" }, true);
	}
	return true;
}

bool MainApplication::HostConnectSession(){
	if(host->registered){
		// save RP keys
		this->settings->WriteFile();
	}
	// connect host sesssion
	host->ConnectSession(this->io);
	host->StartSession();
	// FIXME: it does not wait for complete connection
	// quite completely the gui app to release SDL2 objects
	this->Close();
	// we cannot display errors on screen if the connection failed
	return true;
}

bool MainApplication::HostConnectedCallback(){
	// quite completely the gui app
	this->Close();
	// from here the GUI is unloaded
	// now SDL2 is managed by the IO object
	return true;
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
	std::function<void(chiaki::ui::CustomDialog::Ref)> show_custom_dialog_cb =
		std::bind(&MainApplication::ShowCustomDialogCallback, this, std::placeholders::_1);
	// display host's settings
	SettingLayout::Ref host_setting_layout = SettingLayout::New(host, this->settings, this->io, show_custom_dialog_cb);
	host_setting_layout->SetOnInput(std::bind(&MainApplication::SettingInput, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	this->LoadLayout(host_setting_layout);
	//int ret = this->CreateShowDialog("Settings", "TODO", { "OK" }, true);
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
