/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "player/gui/widgets/icon_label.h"

namespace fastoplayer {
namespace gui {

class Button : public IconLabel {
 public:
  typedef IconLabel base_class;

  Button();
  Button(const SDL_Color& back_ground_color);
  virtual ~Button();

  bool IsPressed() const;

  virtual void Draw(SDL_Renderer* render) override;

 protected:
  virtual void OnFocusChanged(bool focus) override;
  virtual void OnMouseClicked(Uint8 button, const SDL_Point& position) override;
  virtual void OnMouseReleased(Uint8 button, const SDL_Point& position) override;

 private:
  bool pressed_;
};

}  // namespace gui
}  // namespace fastoplayer
