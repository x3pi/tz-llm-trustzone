/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

const { X2DFast } = require('../graphics/X2DFast');

class XButton {
  constructor(x, y, w, h, name) {
    this.pm2f_ = X2DFast.gi();
    this.move(x, y, w, h);
    this.name_ = name;
    this.touchDown_ = false;
    this.clicked_ = false;
    this.rightClicked_ = false;
    this.disable_ = false;
    this.nameColor_ = 0xffffffff;
    this.backgroundColor_ = 0xff487eb8;
  }

  move(x, y, w, h) {
    this.posX_ = x;
    this.posY_ = y;
    this.posW_ = w;
    this.posH_ = h;
    return this;
  }

  draw() {
    const COLOR_OFF = 0x00202020;
    const SIZE = 14;
    let coloroff = 0;
    if (this.touchDown_) {
      coloroff = COLOR_OFF;
    }
    this.pm2f_.fillRect(
      this.posX_,
      this.posY_,
      this.posW_,
      this.posH_,
      this.backgroundColor_ - coloroff
    );
    if (this.name_ !== undefined && this.name_.length > 0)
    {
      let middle = 2;
      let yOffset = 2;
      let sw = 1;
      let sh = 1;
      let ra = 0;
      let ox = -2;
      let oy = -2;
      this.pm2f_.drawText(this.name_, SIZE, this.posX_ + this.posW_ / middle, this.posY_ +
      this.posH_ / middle + yOffset, sw, sh, ra, ox, oy, this.nameColor_ - coloroff);
    }
  }

  isTouchInButton(x, y) {
    if (x < this.posX_) {
      return false;
    }
    if (y < this.posY_) {
      return false;
    }
    if (x > this.posX_ + this.posW_) {
      return false;
    }
    if (y > this.posY_ + this.posH_) {
      return false;
    }
    return true;
  }
  procTouch(msg, x, y) {
    let isIn = this.isTouchInButton(x, y);
    switch (msg) {
      case 1:
        if (isIn) {
          this.touchDown_ = true;
        }
        break;
      case 2:
        break;
      case 3:
        if (this.touchDown_ && isIn) {
          this.clicked_ = true;
        }
        this.touchDown_ = false;
        break;
      case 4:
        if (isIn) {
          this.rightDown_ = true;
        }
        break;
      case 6:
        if (this.rightDown_ && isIn) {
          this.rightClicked_ = true;
        }
        this.rightDown_ = false;
        break;
    }
    return isIn;
  }

  isClicked() {
    if (this.clicked_) {
      this.clicked_ = false;
      return true;
    }
    return false;
  }

  isRightClicked() {
    if (this.rightClicked_) {
      this.rightClicked_ = false;
      return true;
    }
    return false;
  }
}

module.exports = {
  XButton,
};
