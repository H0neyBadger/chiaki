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

#include "ui/customdialog.h"

namespace chiaki::ui
{
	// https://github.com/XorTroll/Plutonium/blob/master/Plutonium/Source/pu/ui/ui_Dialog.cpp
	CustomDialog::CustomDialog(pu::String Title, pu::String Content, pu::ui::elm::Element::Ref Element)
	{
		this->tfont_name = "DefaultFont@30";
		this->cfont_name = "DefaultFont@20";
		this->stitle = Title;
		this->scnt = Content;
		this->title = pu::ui::render::RenderText(this->tfont_name, Title, { 10, 10, 10, 255 });
		this->cnt = pu::ui::render::RenderText(this->cfont_name, Content, { 20, 20, 20, 255 });
		this->osel = 0;
		//this->prevosel = 0;
		//this->selfact = 255;
		//this->pselfact = 0;
		this->cancel = false;
		this->element = Element;
	}

	CustomDialog::~CustomDialog()
	{
		if(this->title != nullptr)
		{
			pu::ui::render::DeleteTexture(this->title);
			this->title = nullptr;
		}
		if(this->cnt != nullptr)
		{
			pu::ui::render::DeleteTexture(this->cnt);
			this->cnt = nullptr;
		}
	}

	i32 CustomDialog::Show(pu::ui::render::Renderer::Ref &Drawer, void *App)
	{
		// start form element object
		// and stack up content and title on top
		//
		// element's height, width, x, y;
		// use 20 px as margin
		i32 margin = 20;
		i32 ew = this->element->GetWidth();
		i32 eh = this->element->GetHeight();
		i32 ex = this->element->GetProcessedX();
		i32 ey = this->element->GetProcessedY();
		// Rounded rectangles
		i32 rh = eh, rw = ew, rx = 0, ry = 0;
		// title's height, width, x, y;
		i32 th = 0, tw = 0, tx = ex, ty = ey;
		// content's height, width, x, y;
		i32 ch = 0, cw = 0, cx = ex, cy = ey;

		if(!this->scnt.IsEmpty())
		{
			ch = pu::ui::render::GetTextHeight(this->cfont_name, this->scnt);
			cw = pu::ui::render::GetTextWidth(this->cfont_name, this->scnt);
			// stack on top of element Y
			cy = ey - margin - ch;
			rh += ch + margin;
			if(cw > rw)
			{
				// increase rectangle width
				rw = cw;
			}
		}

		if(!this->stitle.IsEmpty())
		{
			th = pu::ui::render::GetTextHeight(this->tfont_name, this->stitle);
			tw = pu::ui::render::GetTextWidth(this->tfont_name, this->stitle);
			// stack on top of content Y
			ty = cy - margin - th;
			rh += th + margin;
			if(tw > rw)
			{
				// increase rectangle width
				rw = tw;
			}
		}

		// add top / bottom margin
		rw += 2 * margin;
		rh += 2 * margin;

		// set XY position based on title XY
		rx = tx - margin;
		ry = ty - margin;
		i32 initfact = 0;
		// white opaque color
		pu::ui::Color clr = { 255, 255, 255, 255 };
		while(true)
		{
			bool ok = reinterpret_cast<pu::ui::Application*>(App)->CallForRenderWithRenderOver([&](pu::ui::render::Renderer::Ref &Drawer) -> bool
			{
				// read user inputs
				u64 k = hidKeysDown(CONTROLLER_P1_AUTO);
				u64 h = hidKeysHeld(CONTROLLER_P1_AUTO);
				u64 u = hidKeysUp(CONTROLLER_P1_AUTO);
				u64 th = hidKeysDown(CONTROLLER_HANDHELD);
				pu::ui::Touch tch = pu::ui::Touch::Empty;
				if(th & KEY_TOUCH)
				{
					touchPosition nxtch;
					hidTouchRead(&nxtch, 0);
					tch.X = nxtch.px;
					tch.Y = nxtch.py;
				}
				else if(k & KEY_A)
				{
					this->cancel = false;
					return false;
				}
				else if(k & KEY_B)
				{
					this->cancel = true;
					return false;
				}
				// grey backgroud
				Drawer->RenderRectangleFill({ 0, 0, 0, 125 }, 0, 0, 1280, 720);
				// then create a white rouded rectangle (with radius = 30)
				Drawer->RenderRoundedRectangleFill(clr, rx, ry, rw, rh, 30);
				// push title, and content
				pu::ui::render::SetAlphaValue(this->title, 255);
				pu::ui::render::SetAlphaValue(this->cnt, 255);
				Drawer->RenderTexture(this->title, tx, ty);
				Drawer->RenderTexture(this->cnt, cx, cy);

				// forward input to element
				this->element->SetVisible(true);
				this->element->OnRender(Drawer, this->element->GetProcessedX(), this->element->GetProcessedY());
				this->element->OnInput(k, u, h, tch);

				return true;
			});
			if(!ok)
			{
				((pu::ui::Application*)App)->CallForRenderWithRenderOver([&](pu::ui::render::Renderer::Ref &Drawer) -> bool { return false; });
				if(this->element != nullptr)
				{
					this->element->SetVisible(false);
				}
				break;
			}
		}
		return 0;
	}

	bool CustomDialog::UserCancelled()
	{
		return this->cancel;
	}

}
