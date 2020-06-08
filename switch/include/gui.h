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
#include "io.h"

// https://github.com/XorTroll/Goldleaf/blob/0.9-dev/Goldleaf/Source/ui/ui_ClickableImage.cpp
class ClickableImage : public pu::ui::elm::Image
{
	public:
		// Have ::Ref alias and ::New() static constructor
		PU_SMART_CTOR(ClickableImage)

		ClickableImage(s32 X, s32 Y, pu::String Image);
		void SetOnClick(std::function<void()> Callback);
		void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch Pos);
	protected:
		//pu::sdl2::Texture ntex;
		std::function<void()> cb;
		std::chrono::steady_clock::time_point touchtp;
		bool touched;
};

class SettingLayout : public pu::ui::Layout::Layout {
	public:
		SettingLayout();
		PU_SMART_CTOR(SettingLayout)
		ClickableImage::Ref button;
};

class AddLayout : public pu::ui::Layout::Layout {
	public:
		AddLayout();
		PU_SMART_CTOR(AddLayout)
		ClickableImage::Ref button;
};

class MainLayout : public pu::ui::Layout {
	private:
		pu::ui::elm::Menu::Ref console_menu;
		pu::ui::elm::TextBlock::Ref no_host_found;
		std::map<std::string, Host> * hosts;
		std::function<void(Host *)> SetHostFn;
		std::function<void(Host *)> WakeupHostFn;
		std::function<void(Host *)> ConfigureHostFn;
		// to maintain a host status/update in menu thread
		std::map<std::string, pu::ui::elm::MenuItem::Ref> host_menuitems;
		void UpdateValues();
		bool UpdateOrCreateHostMenuItem(Host * host);
	public:
		MainLayout(std::map<std::string, Host> * hosts,
			std::function<void(Host *)> SetHostFn,
			std::function<void(Host *)> WakeupHostFn,
			std::function<void(Host *)> ConfigureHostFn);
		// Have ::Ref alias and ::New() static constructor
		PU_SMART_CTOR(MainLayout)
		ClickableImage::Ref discover_button;
		ClickableImage::Ref add_button;
		ClickableImage::Ref setting_button;
};

class MainApplication : public pu::ui::Application {
	private:
		MainLayout::Ref main_layout;
		SettingLayout::Ref setting_layout;
		AddLayout::Ref add_layout;
		std::map<std::string, Host> * hosts;
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
		MainApplication(pu::ui::render::Renderer::Ref Renderer, std::map<std::string, Host> * hosts, IO * io):
			pu::ui::Application(Renderer), hosts(hosts), host(nullptr), io(io) {};
		PU_SMART_CTOR(MainApplication)
		Host * GetHost();
		void SetHostCallback(Host * host);
		void WakeupHostCallback(Host * host);
		void ConfigureHostCallback(Host * host);
		void OnLoad() override;
};

#endif // CHIAKI_GUI_H

