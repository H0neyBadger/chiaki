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


#include "ui/clickableimage.h"

namespace chiaki::ui
{
	//https://github.com/XorTroll/Goldleaf/blob/0.9-dev/Goldleaf/Source/ui/ui_ClickableImage.cpp
	ClickableImage::ClickableImage(s32 X, s32 Y, pu::String Image) : pu::ui::elm::Image::Image(X, Y, Image) 
	{
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
}
