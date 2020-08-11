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

#ifndef CHIAKI_GUI_H
#define CHIAKI_GUI_H


// Include Plutonium's main header
#include <pu/Plutonium>

#include <map>
#include "host.h"
#include "settings.h"
#include "discoverymanager.h"
#include "io.h"
#include "ui/clickableimage.h"
#include "ui/customdialog.h"

class SettingLayout : public pu::ui::Layout::Layout {
	protected:
		// global setting object
		Settings * settings;
		// display custom dialog from main app
		std::function<void(chiaki::ui::CustomDialog::Ref)> show_custom_dialog_cb;
		// default color schemes
		pu::ui::Color menu_color;
		pu::ui::Color menu_focus_color;

		pu::ui::elm::Menu::Ref setting_menu;
		pu::ui::elm::MenuItem::Ref account_item;
		pu::ui::elm::MenuItem::Ref resolution_item;
		pu::ui::elm::MenuItem::Ref fps_item;
		pu::ui::elm::MenuItem::Ref overclock_item;
		pu::ui::elm::MenuItem::Ref ip_item;

		pu::ui::elm::Menu::Ref resolution_menu;
		chiaki::ui::CustomDialog::Ref resolution_dialog;
		pu::ui::elm::Menu::Ref fps_menu;
		chiaki::ui::CustomDialog::Ref fps_dialog;
		pu::ui::elm::Menu::Ref overclock_menu;
		chiaki::ui::CustomDialog::Ref overclock_dialog;

		// to store account ids
		pu::String psn_online_id;
		pu::String psn_account_id;
		// resolution choice items
		pu::ui::elm::MenuItem::Ref res_720p;
		pu::ui::elm::MenuItem::Ref res_540p;
		pu::ui::elm::MenuItem::Ref res_360p;
		// FPS choice items
		pu::ui::elm::MenuItem::Ref fps_60;
		pu::ui::elm::MenuItem::Ref fps_30;
		// overclock choices
		pu::ui::elm::MenuItem::Ref oc_1785;
		pu::ui::elm::MenuItem::Ref oc_1580;
		pu::ui::elm::MenuItem::Ref oc_1326;
		pu::ui::elm::MenuItem::Ref oc_1220;
		pu::ui::elm::MenuItem::Ref oc_1020;
		// to store Ip addr
		pu::String ip_address;

	public:
		SettingLayout(Settings *settings,
			std::function<void(chiaki::ui::CustomDialog::Ref)> show_custom_dialog_cb);
		PU_SMART_CTOR(SettingLayout)
};

class AddLayout : public pu::ui::Layout::Layout {
	public:
		AddLayout();
		PU_SMART_CTOR(AddLayout)
		chiaki::ui::ClickableImage::Ref button;
};

class MainLayout : public pu::ui::Layout {
	private:
		pu::ui::elm::Menu::Ref console_menu;
		pu::ui::elm::TextBlock::Ref no_host_found;
		std::map<std::string, Host> * hosts;
		std::function<void(Host *)> DiscoverySendFn;
		std::function<void(Host *)> SetHostFn;
		std::function<void(Host *)> WakeupHostFn;
		std::function<void(Host *)> ConfigureHostFn;
		// to maintain a host status/update in menu thread
		std::map<std::string, pu::ui::elm::MenuItem::Ref> host_menuitems;
		void UpdateValues();
		bool UpdateOrCreateHostMenuItem(Host * host);
		int thread_counter = 0;
	public:
		MainLayout(std::map<std::string, Host> * hosts,
			std::function<void(Host *)> DiscoverySendFn,
			std::function<void(Host *)> SetHostFn,
			std::function<void(Host *)> WakeupHostFn,
			std::function<void(Host *)> ConfigureHostFn);
		// Have ::Ref alias and ::New() static constructor
		PU_SMART_CTOR(MainLayout)
		chiaki::ui::ClickableImage::Ref discover_button;
		chiaki::ui::ClickableImage::Ref add_button;
		chiaki::ui::ClickableImage::Ref setting_button;
};

class MainApplication : public pu::ui::Application {
	private:
		MainLayout::Ref main_layout;
		SettingLayout::Ref setting_layout;
		AddLayout::Ref add_layout;
		std::map<std::string, Host> * hosts;
		Settings * settings;
		DiscoveryManager * discoverymanager;
		Host * host;
		IO * io;
		void LoadSettingLayout();
		void LoadAddLayout();
		void ReturnToMainMenu();
		void AddInput(u64 down, u64 up, u64 held);
		void SettingInput(u64 down, u64 up, u64 held);
	public:
		//using Application::Application;
		//MainApplication();
		MainApplication(
			pu::ui::render::Renderer::Ref Renderer,
			std::map<std::string, Host> * hosts,
			Settings * settings,
			DiscoveryManager * discoverymanager,
			IO * io):
				pu::ui::Application(Renderer),
				hosts(hosts),
				settings(settings),
				discoverymanager(discoverymanager),
				host(nullptr),
				io(io) {};

		PU_SMART_CTOR(MainApplication)
		Host * GetHost();
		void ShowCustomDialogCallback(chiaki::ui::CustomDialog::Ref custom_dialog);
		void SetHostCallback(Host * host);
		void WakeupHostCallback(Host * host);
		void ConfigureHostCallback(Host * host);
		void DiscoverySendCallback(Host * host);
		void OnLoad() override;
};

#endif // CHIAKI_GUI_H

