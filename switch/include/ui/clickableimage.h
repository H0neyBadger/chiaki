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

#ifndef CHIAKI_UI_CLICKABLEIMAGE_H
#define CHIAKI_UI_CLICKABLEIMAGE_H

// Include Plutonium's main header
#include <pu/Plutonium>

namespace chiaki::ui
{
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
			std::function<void()> cb;
			std::chrono::steady_clock::time_point touchtp;
			bool touched;
	};
}
#endif // CHIAKI_UI_CLICKABLEIMAGE_H
