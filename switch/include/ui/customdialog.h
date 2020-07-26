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

#ifndef CHIAKI_UI_CUSTOMDIALOG_H
#define CHIAKI_UI_CUSTOMDIALOG_H

// Include Plutonium's main header
#include <pu/Plutonium>

namespace chiaki::ui
{
	// https://github.com/XorTroll/Plutonium/blob/master/Plutonium/Source/pu/ui/ui_Dialog.cpp
	class CustomDialog
	{
		protected:
			pu::String tfont_name;
			pu::String cfont_name;
			pu::String stitle;
			pu::String scnt;
			pu::sdl2::Texture title;
			pu::sdl2::Texture cnt;
			i32 osel;
			bool cancel;
			pu::ui::elm::Element::Ref element;
		public:
			// Have ::Ref alias and ::New() static constructor
			PU_SMART_CTOR(CustomDialog)
			CustomDialog(pu::String Title, pu::String Content, pu::ui::elm::Element::Ref Element);
			~CustomDialog();
			i32 Show(pu::ui::render::Renderer::Ref &Drawer, void *App);
			bool UserCancelled();
	};
}

#endif // CHIAKI_UI_CLICKABLEIMAGE_H
