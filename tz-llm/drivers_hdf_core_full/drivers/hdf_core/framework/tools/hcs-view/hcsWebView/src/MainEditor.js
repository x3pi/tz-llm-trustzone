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

const { XMessage } = require('./message/XMessage');
const { Lexer } = require('./hcs/lexer');
const { Generator } = require('./hcs/Generator');
const { Scr } = require('./engine/XDefine');
const { XButton } = require('./engine/control/XButton');
const { AttrEditor } = require('./AttrEditor');
const { NapiLog } = require('./hcs/NapiLog');
const { XSelect } = require('./engine/control/XSelect');
const { NodeTools, DataType, NodeType } = require('./hcs/NodeTools');
const { ModifyNode } = require('./hcs/ModifyNode');
const { CanvasInput } = require('./hcs/CanvasInput');
const { RightMenu } = require('./engine/RightMenu');
const { XTexture } = require('./engine/graphics/XTexture');
const { XTools } = require('./engine/XTools');

const { ObjectType } = require('./hcs/ast');
const DISPLAY_TEXT_MAX = 30;
const ELLIPSIS_LEN = 3;
const EQUAL_SIGN_LEN = 3;
const MAX_RANDOM = 255;
const CONSTANT_MIDDLE = 2;
const CONSTANT_QUARTER = 4;
const DRAW_HEIGHT = 4;
const COLOR_MAX = 10;
const GRAY_LEVEL = 192;
const INIT_VALUE = -1;
const KEY_VALUE_MAX = 30;
const ONE_KEY_VALUE_MAX = 29;
const TWO_KEY_VALUE_MAX = 28;
const THREE_KEY_VALUE_MAX = 27;
const BGPIC_WIDTH = 156;
const BGPIC_HEIGHT = 112;
const DRAWTEXT_OFFSETX = -3;
const DRAWTEXT_OFFSETY = -3;

const MouseType = {
  DOWN: 1, // 按下
  MOVE: 2, // 移动
  UP: 3, // 抬起
};

function rgba(colorArr) {
  return 0xff000000 | (colorArr[0] << 16) | (colorArr[1] << 8) | colorArr[2];
}

function getDarker(colorArr) {
  colorArr.forEach((item, index) => {
    if (item - 0 > COLOR_MAX) {
      item = item - COLOR_MAX;
    } 
  });
  return rgba(colorArr);
}

function isDarkColor(colorArr) {
  let grayLevel =
    colorArr[0] * 0.299 + colorArr[1] * 0.587 + colorArr[2] * 0.114;
  return grayLevel < GRAY_LEVEL;
}

function getVsCodeTheme() {
  MainEditor.CANVAS_BG = 0xff272727;
  MainEditor.CANVAS_LINE = 0xff000000;
  MainEditor.NODE_TEXT_COLOR = 0xffffffff;
  let canvasBg = document.getElementById('canvas_bg');
  var bgStyle = document.defaultView.getComputedStyle(canvasBg, null);
  var bgColor = bgStyle.getPropertyValue('background-color').match(/\d{1,3}/g);
  if (bgColor) {
    MainEditor.CANVAS_BG = rgba(bgColor);
    MainEditor.CANVAS_LINE = getDarker(bgColor);
    MainEditor.IS_DARK_BG = isDarkColor(bgColor);
    RightMenu.isDarkBackground_ = MainEditor.IS_DARK_BG;
  }

  var txtColor = bgStyle.getPropertyValue('color').match(/\d{1,3}/g);
  if (txtColor) {
    MainEditor.NODE_TEXT_COLOR = rgba(txtColor);
  }
}

class MainEditor {
  constructor() {
    this.files_ = {};
    this.nodeCount_ = {};
    this.filePoint_ = null;
    this.rootPoint_ = null;
    this.nodePoint_ = null;
    this.offX_ = 100;
    this.offY_ = 100;
    this.searchKey = null;
    this.touchQueue_ = [];
    this.keyQueue_ = [];
    this.dropAll_ = {
      locked: false,
      oldx: INIT_VALUE,
      oldy: INIT_VALUE,
    };
    getVsCodeTheme();
    this.nodeBtns = [];
    this.nodeMoreBtns = [];
    this.nodeBtnPoint_ = 0;
    this.nodeMoreBtnPoint_ = 0;
    XMessage.gi().registRecvCallback(this.onReceive);
    XMessage.gi().send('inited', '');
    AttrEditor.gi().freshEditor();

    this.sltInclude = new XSelect(['a', 'b', 'c'], 'b');
    this.sltInclude.registCallback(this.onSelectInclude);
    NapiLog.registError(this.onError);
    this.errorMsg_ = [];
    this.cutImgDict_ = {};
    this.initMainEditorWithInitValue();
    this.selectNode_ = {
      type: null,
      pnode: null,
    };
    this.btnCancelSelect_ = new XButton();

    AttrEditor.gi().registCallback(this.onAttributeChange);

    this.mousePos_ = {
      x: 0,
      y: 0,
    };

    this.whiteImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle.png'
    );
    this.whiteCut_ = XTexture.gi().makeCut(
      this.whiteImg_,
      0,
      0,
      132,
      32,
      132,
      32
    );
    this.cutImgDict_.whiteCut = 'rectangle.png';

    this.cicleImg_ = XTexture.gi().loadTextureFromImage('../images/circle.png');
    this.circleCut_ = XTexture.gi().makeCut(
      this.cicleImg_,
      0,
      0,
      20,
      20,
      20,
      20
    );
    this.cutImgDict_.circleCut = 'circle.png';

    this.cicleOpenImg_ = XTexture.gi().loadTextureFromImage(
      '../images/circle_open.png'
    );
    this.circleOpenCut_ = XTexture.gi().makeCut(
      this.cicleOpenImg_,
      0,
      0,
      20,
      20,
      20,
      20
    );
    this.cutImgDict_.circleOpenCut = 'circle_open.png';

    this.rectangleFocusImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle_focus.png'
    );
    this.rectangleFocusCut_ = XTexture.gi().makeCut(
      this.rectangleFocusImg_,
      0,
      0,
      132,
      32,
      132,
      32
    );
    this.cutImgDict_['rectangleFocusCut'] = 'rectangle_focus.png';

    this.nodeIconImg_ = XTexture.gi().loadTextureFromImage(
      '../images/node_icon.png'
    );
    this.nodeIconCut_ = XTexture.gi().makeCut(
      this.nodeIconImg_,
      0,
      0,
      8,
      8,
      8,
      8
    );
    this.cutImgDict_['nodeIconCut'] = 'node_icon.png';

    this.attrIconImg_ = XTexture.gi().loadTextureFromImage(
      '../images/attribute_icon.png'
    );
    this.attrIconCut_ = XTexture.gi().makeCut(
      this.attrIconImg_,
      0,
      0,
      8,
      8,
      8,
      8
    );
    this.cutImgDict_['attrIconCut'] = 'attribute_icon.png';

    this.rootIconImg_ = XTexture.gi().loadTextureFromImage(
      '../images/root_btn.png'
    );
    this.rootIconCut_ = XTexture.gi().makeCut(
      this.rootIconImg_,
      0,
      0,
      132,
      32,
      132,
      32
    );
    this.cutImgDict_['rootIconCut'] = 'root_btn.png';

    this.rootIconFocusImg_ = XTexture.gi().loadTextureFromImage(
      '../images/root_btn_focus.png'
    );
    this.rootIconFocusCut_ = XTexture.gi().makeCut(
      this.rootIconFocusImg_,
      0,
      0,
      132,
      32,
      132,
      32
    );
    this.cutImgDict_['rootIconFocusCut'] = 'root_btn_focus.png';

    this.leftRectCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle.png'
    );
    this.leftRectCicleCut_ = XTexture.gi().makeCut(
      this.leftRectCicleImg_,
      0,
      0,
      8,
      32,
      132,
      32
    );
    this.centerRectImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle.png'
    );
    this.centerRectCut_ = XTexture.gi().makeCut(
      this.centerRectImg_,
      8,
      0,
      116,
      32,
      132,
      32
    );
    this.rightRectCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle.png'
    );
    this.rightRectCicleCut_ = XTexture.gi().makeCut(
      this.rightRectCicleImg_,
      124,
      0,
      8,
      32,
      132,
      32
    );

    this.leftRectFocusCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle_focus.png'
    );
    this.leftRectFocusCicleCut_ = XTexture.gi().makeCut(
      this.leftRectFocusCicleImg_,
      0,
      0,
      8,
      32,
      132,
      32
    );
    this.centerFocusImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle_focus.png'
    );
    this.centerFocusCut_ = XTexture.gi().makeCut(
      this.centerFocusImg_,
      8,
      0,
      116,
      32,
      132,
      32
    );
    this.rightRectFocusCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle_focus.png'
    );
    this.rightRectFocusCicleCut_ = XTexture.gi().makeCut(
      this.rightRectFocusCicleImg_,
      124,
      0,
      8,
      32,
      132,
      32
    );

    this.reloadBgPic();

    RightMenu.popItemFocusImg_ = XTexture.gi().loadTextureFromImage(
      '../images/pop_item_focus.png'
    );
    RightMenu.popItemFocusCut_ = XTexture.gi().makeCut(
      RightMenu.popItemFocusImg_,
      0,
      0,
      148,
      32,
      148,
      32
    );
    this.cutImgDict_['popItemFocusCut'] = 'pop_item_focus.png';

    this.searchBgImg_ = XTexture.gi().loadTextureFromImage(
      '../images/search_bg.png'
    );
    this.searchBgCut_ = XTexture.gi().makeCut(
      this.searchBgImg_,
      0,
      0,
      494,
      56,
      494,
      56
    );
    this.cutImgDict_['searchBgCut'] = 'search_bg.png';

    this.upImg_ = XTexture.gi().loadTextureFromImage(
      '../images/chevron-up.png'
    );
    this.upCut_ = XTexture.gi().makeCut(this.upImg_, 0, 0, 16, 16, 16, 16);
    this.cutImgDict_['upCut'] = 'chevron-up.png';

    this.downImg_ = XTexture.gi().loadTextureFromImage(
      '../images/chevron-down.png'
    );
    this.downCut_ = XTexture.gi().makeCut(this.downImg_, 0, 0, 16, 16, 16, 16);
    this.cutImgDict_['downCut'] = 'chevron-down.png';

    this.closeImg_ = XTexture.gi().loadTextureFromImage('../images/close.png');
    this.closeCut_ = XTexture.gi().makeCut(
      this.closeImg_,
      0,
      0,
      16,
      16,
      16,
      16
    );
    this.cutImgDict_['closeCut'] = 'close.png';

    this.searchImg_ = XTexture.gi().loadTextureFromImage(
      '../images/search.png'
    );
    this.searchCut_ = XTexture.gi().makeCut(
      this.searchImg_,
      0,
      0,
      16,
      16,
      16,
      16
    );
    this.cutImgDict_['searchCut'] = 'search.png';

    this.searchRectFocusCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/search_nood_rect.png'
    );
    this.leftSearchFocusCicleCut_ = XTexture.gi().makeCut(
      this.searchRectFocusCicleImg_,
      0,
      0,
      8,
      32,
      132,
      32
    );
    this.centerSearchCut_ = XTexture.gi().makeCut(
      this.searchRectFocusCicleImg_,
      8,
      0,
      116,
      32,
      132,
      32
    );
    this.rightSearchFocusCicleCut_ = XTexture.gi().makeCut(
      this.searchRectFocusCicleImg_,
      124,
      0,
      8,
      32,
      132,
      32
    );
    this.cutImgDict_['searchNoodRectImg'] = 'search_nood_rect.png';

    this.searchAttrCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/search_attr_rect.png'
    );
    this.leftSearchAttrCicleCut_ = XTexture.gi().makeCut(
      this.searchAttrCicleImg_,
      0,
      0,
      8,
      32,
      132,
      32
    );
    this.centerSearchAttrCut_ = XTexture.gi().makeCut(
      this.searchAttrCicleImg_,
      8,
      0,
      116,
      32,
      132,
      32
    );
    this.rightSearchAttrCicleCut_ = XTexture.gi().makeCut(
      this.searchAttrCicleImg_,
      124,
      0,
      8,
      32,
      132,
      32
    );
    this.cutImgDict_['searchAttrRectImg'] = 'search_attr_rect.png';

    XMessage.gi().send('cutImgDict', {
      data: this.cutImgDict_,
    });
    this.modifyPos_ = null;
    this.isFirstDraw = true;
    this.lenHierarchy = 0;

    this.searchInput = null;
    this.historyZ = [];
    this.historyBase = {};
    this.historyPushed = false;
  }

  initMainEditorWithInitValue() {
    this.whiteImg_ = INIT_VALUE;
    this.whiteCut_ = INIT_VALUE;
    this.cicleImg_ = INIT_VALUE;
    this.circleCut_ = INIT_VALUE;
    this.cicleOpenImg_ = INIT_VALUE;
    this.circleOpenCut_ = INIT_VALUE;
    this.rectangleFocusImg_ = INIT_VALUE;
    this.rectangleFocusCut_ = INIT_VALUE;
    this.nodeIconImg_ = INIT_VALUE;
    this.nodeIconCut_ = INIT_VALUE;
    this.attrIconImg_ = INIT_VALUE;
    this.attrIconCut_ = INIT_VALUE;
    this.rootIconImg_ = INIT_VALUE;
    this.rootIconCut_ = INIT_VALUE;
    this.rootIconFocusImg_ = INIT_VALUE;
    this.rootIconFocusCut_ = INIT_VALUE;
    RightMenu.backgroundImg_ = INIT_VALUE;
    RightMenu.backgroundCut_ = INIT_VALUE;
    RightMenu.popItemFocusImg_ = INIT_VALUE;
    RightMenu.popItemFocusCut_ = INIT_VALUE;
    this.leftRectCicleCut_ = INIT_VALUE;
    this.centerRectCut_ = INIT_VALUE;
    this.rightRectCicleCut_ = INIT_VALUE;
    this.leftRectFocusCicleCut_ = INIT_VALUE;
    this.centerFocusCut_ = INIT_VALUE;
    this.rightRectFocusCicleCut_ = INIT_VALUE;
    this.delay_ = 0;
    this.searchBgImg_ = INIT_VALUE;
    this.searchBgCut_ = INIT_VALUE;
    this.upImg_ = INIT_VALUE;
    this.upCut_ = INIT_VALUE;
    this.downImg_ = INIT_VALUE;
    this.downCut_ = INIT_VALUE;
    this.closeImg_ = INIT_VALUE;
    this.closeCut_ = INIT_VALUE;
    this.searchImg_ = INIT_VALUE;
    this.searchCut_ = INIT_VALUE;
    this.isSearchResult_ = false;
    this.searchRectFocusCicleImg_ = INIT_VALUE;
    this.leftSearchFocusCicleCut_ = INIT_VALUE;
    this.centerSearchCut_ = INIT_VALUE;
    this.rightSearchFocusCicleCut_ = INIT_VALUE;

    this.searchAttrCicleImg_ = INIT_VALUE;
    this.leftSearchAttrCicleCut_ = INIT_VALUE;
    this.centerSearchAttrCut_ = INIT_VALUE;
    this.rightSearchAttrCicleCut_ = INIT_VALUE;
  }

  reloadMenuBgPic() {
    let bgPic = this.reloadBgPic();
    XMessage.gi().send('reloadMenuBg', {
      data: bgPic,
    });
  }

  reloadBgPic() {
    let bgPic = RightMenu.isDarkBackground_ ? 'pop_background.png' : 'pop_background_light.png';
    let bgPicPath = '../images/' + bgPic;
    RightMenu.backgroundImg_ = XTexture.gi().loadTextureFromImage(bgPicPath);
    RightMenu.backgroundCut_ = XTexture.gi().makeCut(
      RightMenu.backgroundImg_,
      0,
      0,
      BGPIC_WIDTH,
      BGPIC_HEIGHT,
      BGPIC_WIDTH,
      BGPIC_HEIGHT
    );
    this.cutImgDict_.backgroundCut = bgPic;
    return bgPic;
  }

  calcPostionY(data, y) {
    data.posY = y;
    let ty = y;
    switch (data.type_) {
      case DataType.INT8:
      case DataType.INT16:
      case DataType.INT32:
      case DataType.INT64:
        y += MainEditor.LINE_HEIGHT;
        break;
      case DataType.STRING:
        y += MainEditor.LINE_HEIGHT;
        break;
      case DataType.NODE:
        if (!data.isOpen_) {
          y += MainEditor.LINE_HEIGHT;
        } else {
          data.value_.forEach((item, index) => {
            y = this.calcPostionY(item, y);
          }); 
        }
        break;
      case DataType.ATTR:
        y = this.calcPostionY(data.value_, y);
        break;
      case DataType.ARRAY:
        y += MainEditor.LINE_HEIGHT;
        break;
      case DataType.REFERENCE:
        y += MainEditor.LINE_HEIGHT;
        break;
      case DataType.DELETE:
        y += MainEditor.LINE_HEIGHT;
        break;
      case DataType.BOOL:
        y += MainEditor.LINE_HEIGHT;
        break;
      default:
        NapiLog.logError('unknow' + data.type_);
        break;
    }
    if (y > ty) {
      data.posY = (ty + y - MainEditor.LINE_HEIGHT) / CONSTANT_MIDDLE;
    }
    return y > ty + MainEditor.LINE_HEIGHT ? y : ty + MainEditor.LINE_HEIGHT;
  }

  getNodeText(data) {
    switch (data.nodeType_) {
      case NodeType.DATA:
        return data.name_;
      case NodeType.DELETE:
        return data.name_ + ' : delete';
      case NodeType.TEMPLETE:
        return 'templete ' + data.name_;
      case NodeType.INHERIT:
        if (data.ref_ === 'unknow') {
          return data.name_;
        }
        return data.name_ + ' :: ' + data.ref_;
      case NodeType.COPY:
        if (data.ref_ === 'unknow') {
          return data.name_;
        }
        return data.name_ + ' : ' + data.ref_;
      case NodeType.REFERENCE:
        if (data.ref_ === 'unknow') {
          return data.name_;
        }
        return data.name_ + ' : &' + data.ref_;
      default:
        return 'unknow node type';
    }
  }

  drawNode(pm2f, s, size, x, y, type, data) {
    const SPACE = 2;
    const MAXLEN_DISPLAY_WITH_SPACE = 25;
    const MAXLEN_DISPLAY_NO_SPACE = 26;
    const DISPLAY_TEXT_MAX_NOPOINT = 27;
    let w = pm2f.getTextWidth(type === DataType.ATTR ? s + ' = ' : s, size);
    if (data.parent_ === undefined) {
      return w;
    }

    if (type === DataType.ATTR) {
      let lenDisplay = DISPLAY_TEXT_MAX - EQUAL_SIGN_LEN;
      let orangeColor = 0xffa9a9a9;
      if (s.length < MAXLEN_DISPLAY_WITH_SPACE) {
        pm2f.drawText(s, size, x - (data.parent_ !== undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE,
        y + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        pm2f.drawText(' = ', size, x - (data.parent_ !== undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE + pm2f.getTextWidth(s, size),
        y + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 0, 0, orangeColor);
      } else if (s.length === MAXLEN_DISPLAY_WITH_SPACE) {
        pm2f.drawText(s, size, x - (data.parent_ !== undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE,
        y + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        pm2f.drawText(' =', size, x - (data.parent_ !== undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE + pm2f.getTextWidth(s, size),
        y + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 0, 0, orangeColor);
      } else if (s.length === MAXLEN_DISPLAY_NO_SPACE) {
        pm2f.drawText(s, size, x - (data.parent_ !== undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE,
        y + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        pm2f.drawText('=', size, x - (data.parent_ !== undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE + pm2f.getTextWidth(s, size),
        y + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 0, 0, orangeColor);
      } else if (s.length > MAXLEN_DISPLAY_NO_SPACE) {
        s = s.substring(0, lenDisplay) + '...';
        pm2f.drawText(s, size, x - (data.parent_ !== undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE,
        y + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
      }
    } else {
      let logoSizeTimes = 2; // logosize显示大小是logoSize长度的2倍
      pm2f.drawText( s.length > DISPLAY_TEXT_MAX ? s.substring(0, DISPLAY_TEXT_MAX_NOPOINT) + '...' : s, size, x -
        (MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_) + MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * logoSizeTimes,
      y + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    }
    return w;
  }

  drawBrokenLine(pm2f, data, offy, i) {
    let dis =
      data.parent_ !== undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0;
    let baseX_ =
      data.posX +
      MainEditor.NODE_RECT_WIDTH -
      (MainEditor.NODE_RECT_WIDTH - data.nodeWidth_) +
      MainEditor.NODE_TEXT_OFFX +
      MainEditor.NODE_MORE_CHILD -
      dis;
    pm2f.drawLine(
      baseX_,
      offy + data.posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE,
      baseX_ + MainEditor.LINE_WIDTH,
      offy + data.posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE,
      MainEditor.NODE_LINE_COLOR,
      0.5
    );

    pm2f.drawLine(
      baseX_ + MainEditor.LINE_WIDTH,
      offy + data.posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE,
      baseX_ + MainEditor.LINE_WIDTH,
      offy + data.value_[i].posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE,
      MainEditor.NODE_LINE_COLOR,
      0.5
    );

    pm2f.drawLine(
      baseX_ + MainEditor.LINE_WIDTH,
      offy + data.value_[i].posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE,
      baseX_ + MainEditor.LINE_WIDTH * 2,
      offy + data.value_[i].posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE,
      MainEditor.NODE_LINE_COLOR,
      0.5
    );
  }

  arrayNodeProc(w, pm2f, data, offx, offy) {
    let ss = '[' + data.value_.length + ']' + NodeTools.arrayToString(data);
    let keyAndValue = data.parent_.name_ + ' = ';

    if (keyAndValue.length >= KEY_VALUE_MAX) {
      return;
    } else if (keyAndValue.length === ONE_KEY_VALUE_MAX) {
      w = pm2f.drawText('.', MainEditor.NODE_TEXT_SIZE, offx, offy + data.posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - 
      MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    } else if (keyAndValue.length === TWO_KEY_VALUE_MAX) {
      w = pm2f.drawText('..', MainEditor.NODE_TEXT_SIZE, offx, offy + data.posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - 
      MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    } else if (keyAndValue.length === THREE_KEY_VALUE_MAX) {
      w = pm2f.drawText('...', MainEditor.NODE_TEXT_SIZE, offx, offy + data.posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - 
      MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    } else if (keyAndValue.length < THREE_KEY_VALUE_MAX) {
      let displayValueLen = DISPLAY_TEXT_MAX - keyAndValue.length;
      if (ss.length > displayValueLen) {
        ss = ss.substring(0, displayValueLen - 3) + '...';
      }
      w = pm2f.drawText(ss, MainEditor.NODE_TEXT_SIZE, offx, offy + data.posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - 
      MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    }
  }

  configNodeProc(w, pm2f, data, offx, offy, path) {
    let dis = data.parent_ !== undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0;
    this.setNodeButton(pm2f, offx, offy + data.posY, w, MainEditor.NODE_TEXT_SIZE, path, data);
    if (data.value_.length > 0) {
      this.setNodeMoreButton(pm2f, offx - dis, offy + data.posY, MainEditor.NODE_MORE_CHILD, MainEditor.NODE_MORE_CHILD, data);
    }
    let drawNodeX_ = offx + MainEditor.NODE_RECT_WIDTH + MainEditor.NODE_SIZE_BG_OFFX + MainEditor.NODE_MORE_CHILD + MainEditor.LINE_WIDTH * 2 - dis;
    if (data.type_ === DataType.NODE) {
      data.value_.forEach((item, index) => {
        if (item.parent_.type_ === DataType.NODE && item.parent_.isOpen_) {
          this.drawObj(pm2f, item, drawNodeX_, offy, path + '.');
          this.drawBrokenLine(pm2f, data, offy, index);
        } else if (item.parent_.type_ === DataType.ATTR) {
          this.drawObj(pm2f, item, drawNodeX_, offy, path + '.');
          pm2f.drawLine(data.posX + w, offy + data.posY + 10,
            item.posX, offy + item.posY + 10, MainEditor.NODE_TEXT_COLOR, 1);
        } else {
          NapiLog.logInfo('Node collapse does not need to draw child node');
        }
      }); 
    } else {
      data.value_.forEach((item, index) => {
        this.drawObj(pm2f, item, drawNodeX_, offy, path + '.');
        pm2f.drawLine(data.posX + w, offy + data.posY + 10,
          item.posX, offy + item.posY + 10, MainEditor.NODE_TEXT_COLOR, 1);
      }); 
    }
  }

  /**
   * 绘制Attr对象
   * @param {} pm2f X2DFast
   * @param {} data 节点数据对象
   * @param {} offx x偏移值
   * @param {} offy y偏移值
   * @param {} path 节点路径
   */
  drawAttrObj(pm2f, data, offx, offy, path) {
    let w = this.drawNode(pm2f, data.name_, MainEditor.NODE_TEXT_SIZE, offx, offy + data.posY, data.type_, data);
    this.setNodeButton(pm2f, offx, offy + data.posY, w, MainEditor.NODE_TEXT_SIZE, path, data);
    this.drawObj(pm2f, data.value_, offx + w, offy, path);
    pm2f.drawCut(this.attrIconCut_, offx + MainEditor.LOGO_LEFT_PADDING - (MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_),
      offy + data.posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - MainEditor.LOGO_SIZE / CONSTANT_MIDDLE);
  }

  /**
   * 绘制Node对象
   * @param {} pm2f X2DFast
   * @param {} data 节点数据对象
   * @param {} offx x偏移值
   * @param {} offy y偏移值
   * @param {} path 节点路径
   */  
  drawNodeObj(pm2f, data, offx, offy, path) {
    let w = this.drawNode(pm2f, this.getNodeText(data), MainEditor.NODE_TEXT_SIZE, offx, offy + data.posY, data.type_, data);
    this.configNodeProc(w, pm2f, data, offx, offy, path);
    if (data.parent_ !== undefined) {
      pm2f.drawCut(this.nodeIconCut_, offx + MainEditor.LOGO_LEFT_PADDING - (MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_),
        offy + data.posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - MainEditor.LOGO_SIZE / CONSTANT_MIDDLE);
    }
  }

  /**
   * 绘制引用对象
   * @param {} pm2f X2DFast
   * @param {} data 节点数据对象
   * @param {} drawTextX_ x偏移值
   * @param {} drawTextY_ y偏移值
   */
  drawReferenceObj(pm2f, data, drawTextX_, drawTextY_) {
    let content = data.parent_.name_ + ' = ';
    if (content.length > DISPLAY_TEXT_MAX) {
      content = '';
    } else if ((content + data.value_).length > DISPLAY_TEXT_MAX) {
      content = data.value_.substring((data.parent_.name_ + ' = ').length, THREE_KEY_VALUE_MAX) + '...';
    } else {
      content = data.value_;
    }
    pm2f.drawText('&' + content, MainEditor.NODE_TEXT_SIZE, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_,
      1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
  }

  /**
   * 绘制引用对象
   * @param {} pm2f X2DFast
   * @param {} data 节点数据对象
   * @param {} drawTextX_ x偏移值
   * @param {} drawTextY_ y偏移值
   */  
  drawStringObj(pm2f, data, drawTextX_, drawTextY_) {
    let value = data.value_;
    let keyAndValue = data.parent_.name_ + ' = ' + data.value_;
    if (keyAndValue.length > DISPLAY_TEXT_MAX) {
      value = keyAndValue.substring((data.parent_.name_ + ' = ').length, THREE_KEY_VALUE_MAX) + '...';
    }
    let w = pm2f.drawText('"' + value + '"', MainEditor.NODE_TEXT_SIZE, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_,
      1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
  }

  drawObj(pm2f, data, offx, offy, path) {
    let w;
    path += data.name_;
    data.posX = offx;
    if (this.maxX < offx) {
      this.maxX = offx;
    }
    let parentTextWidth = pm2f.getTextWidth(' = ', MainEditor.NODE_TEXT_SIZE);
    let drawTextX_ = offx + MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE + parentTextWidth;
    let drawTextY_ = offy + data.posY + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - 
    MainEditor.NODE_TEXT_SIZE / CONSTANT_MIDDLE;
    switch (data.type_) {
      case DataType.INT8:
      case DataType.INT16:
      case DataType.INT32:
      case DataType.INT64:
        w = pm2f.drawText(NodeTools.jinZhi10ToX(data.value_, data.jinzhi_), MainEditor.NODE_TEXT_SIZE,
          drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_),
          drawTextY_, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        break;
      case DataType.STRING:
        this.drawStringObj(pm2f, data, drawTextX_, drawTextY_);
        break;
      case DataType.NODE:
        this.drawNodeObj(pm2f, data, offx, offy, path);
        break;
      case DataType.ATTR:
        this.drawAttrObj(pm2f, data, offx, offy, path);
        break;
      case 8:
        this.arrayNodeProc(w, pm2f, data, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), offy);
        break;
      case DataType.REFERENCE:
        this.drawReferenceObj(pm2f, data, drawTextX_, drawTextY_);
        break;
      case DataType.DELETE:
        w = pm2f.drawText('delete', MainEditor.NODE_TEXT_SIZE, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_,
          1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        break;
      case DataType.BOOL:
        this.drawBoolObj(pm2f, data, drawTextX_, drawTextY_);
        break;
      default:
        NapiLog.logError('unknow' + data.type_);
        break;
    }
    this.drawErrorRect(pm2f, data, offx, offy);
  }

  /**
   * 绘制Bool对象
   * @param {} pm2f X2DFast
   * @param {} data 节点数据对象
   * @param {} drawTextX_ x偏移值
   * @param {} drawTextY_ y偏移值
   */  
  drawBoolObj(pm2f, data, drawTextX_, drawTextY_) {
    let w;
    if (data.value_) {
      w = pm2f.drawText('true', MainEditor.NODE_TEXT_SIZE, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_,
        1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    } else {
      w = pm2f.drawText('false', MainEditor.NODE_TEXT_SIZE, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_,
        1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    }
  }

  /**
   * 绘制错误显示框
   * @param {} pm2f X2DFast
   * @param {} data 节点数据对象
   * @param {} offx x偏移值
   * @param {} offy y偏移值
   */  
  drawErrorRect(pm2f, data, offx, offy) {
    if (data.errMsg_ !== null && data.errMsg_ !== undefined) {
      let displayFreq = 2; // 显示频率
      let frameNo = 10;
      if (parseInt(this.delay_ / frameNo) % displayFreq === 0) {
        pm2f.drawRect(offx - (MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_), offy + data.posY,
          data.nodeWidth_, MainEditor.NODE_RECT_HEIGHT, 0xffff0000, 1);
      }
      pm2f.drawText(data.errMsg_, MainEditor.NODE_TEXT_SIZE, offx - (MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_),
        offy + data.posY + 5, 1, 1, 0, -1, -3, 0xffff0000);
    }
  }

  /**
   * 获取绘制框宽度
   * @param {} pm2f X2DFast
   * @param {} node 节点数据对象
   * @return {} rectWidth 绘制框宽度
   */
  getRectWidth(pm2f, node) {
    let displayValue;
    let rectWidth = 0;
    if (node.value_.type_ === ObjectType.PARSEROP_ARRAY) {
      let arrayValue = NodeTools.arrayToString(node.value_);
      displayValue = '[' + node.value_.value_.length + ']' + arrayValue;
    } else if (
      node.value_.type_ === ObjectType.PARSEROP_UINT8 ||
      node.value_.type_ === ObjectType.PARSEROP_UINT16 ||
      node.value_.type_ === ObjectType.PARSEROP_UINT32 ||
      node.value_.type_ === ObjectType.PARSEROP_UINT64
    ) {
      displayValue = NodeTools.jinZhi10ToX(
        node.value_.value_,
        node.value_.jinzhi_
      );
    } else if (node.value_.type_ === ObjectType.PARSEROP_DELETE) {
      displayValue = 'delete';
    } else if (node.value_.type_ === ObjectType.PARSEROP_BOOL) {
      if (node.value_) {
        displayValue = 'true';
      } else {
        displayValue = 'false';
      }
    } else {
      displayValue = node.value_.value_;
    }
    
    let keyAndValue;
    let lenDisplay = THREE_KEY_VALUE_MAX;
    let moreLenDisplayOne = 1;
    let moreLenDisplayTwo = 2;
    if (node.name_.length <= lenDisplay) {
      keyAndValue = node.name_ + ' = ' + displayValue;
    } else if (node.name_.length === lenDisplay + moreLenDisplayOne) {
      keyAndValue = node.name_ + ' =';
    } else if (node.name_.length === lenDisplay + moreLenDisplayTwo) {
      keyAndValue = node.name_ + '=';
    } else if (node.name_.length >= DISPLAY_TEXT_MAX) {
      keyAndValue = node.name_;
    }

    if (keyAndValue.length >= DISPLAY_TEXT_MAX) {
      keyAndValue = keyAndValue.substring(0, THREE_KEY_VALUE_MAX) + '...';
    }
    rectWidth = pm2f.getTextWidth(keyAndValue, MainEditor.NODE_TEXT_SIZE);
    return rectWidth;
  }

  /**
   * 绘制节点名称
   * @param {} pm2f X2DFast
   * @param {} node 节点数据对象
   * @param {} x x坐标值
   * @param {} y y坐标值
   * @param {} w 宽度
   * @param {} h 高度
   */  
  drawNodeNameText(pm2f, node, x, y, w, h) {
    pm2f.drawText(
      node.name_,
      MainEditor.NODE_TEXT_SIZE,
      x + MainEditor.NODE_RECT_WIDTH / CONSTANT_MIDDLE - w / CONSTANT_MIDDLE,
      y + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - h / CONSTANT_MIDDLE,
      1,
      1,
      0,
      1,
      1,
      MainEditor.NODE_TEXT_COLOR
    );
  }

  setNodeButton(pm2f, x, y, w, h, path, node) {
    let rectWidth = MainEditor.NODE_RECT_WIDTH;
    if (node.parent_ === undefined) {
      if (this.nodePoint_ === node) {
        pm2f.drawCut(this.rootIconFocusCut_, x, y);
      } else {
        pm2f.drawCut(this.rootIconCut_, x, y);
      }
      node.nodeWidth_ = MainEditor.NODE_RECT_WIDTH;
      this.drawNodeNameText(pm2f, node, x, y, w, h);
    } else {
      if (node.type_ === DataType.ATTR) {
        rectWidth = this.getRectWidth(pm2f, node);
      } else {
        rectWidth = pm2f.getTextWidth(
          this.getNodeText(node).length > DISPLAY_TEXT_MAX ? this.getNodeText(node).substring(0, THREE_KEY_VALUE_MAX) + '...' : this.getNodeText(node),
          MainEditor.NODE_TEXT_SIZE
        );
      }
      this.drawNodeRectButton(pm2f, x, y, rectWidth, node);
    }

    if (this.nodeBtnPoint_ >= this.nodeBtns.length) {
      this.nodeBtns.push(new XButton());
    }
    let pbtn = this.nodeBtns[this.nodeBtnPoint_];
    let singleOffset = 6;
    let numberOffset = 2;
    let widthOffset = singleOffset * numberOffset;
    let logoSizeOffset = 8;
    pbtn.move(
      x - (node.parent_ === undefined ? 0 : MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_),
      y, node.parent_ === undefined ? MainEditor.NODE_RECT_WIDTH : rectWidth + widthOffset + MainEditor.LOGO_SIZE + logoSizeOffset, MainEditor.NODE_RECT_HEIGHT
    );
    pbtn.name_ = path;
    pbtn.node_ = node;
    this.nodeBtnPoint_ += 1;
  }

  drawNodeRectButton(pm2f, x, y, rectWidth, node) {
    let singleOffset = 6;
    let numberOffset = 2;
    let widthOffset = singleOffset * numberOffset;
    let logoSizeOffset = 8;
    let width = rectWidth + widthOffset + MainEditor.LOGO_SIZE + logoSizeOffset;
    if (node.type_ === DataType.ATTR) {
      switch (node.value_.type_) {
        case ObjectType.PARSEROP_UINT8:
        case ObjectType.PARSEROP_UINT16:
        case ObjectType.PARSEROP_UINT32:
        case ObjectType.PARSEROP_UINT64:
        case ObjectType.PARSEROP_ARRAY:
          width = width;
          break;
        default:
          width = width + 14;
          break;
      }
    }
    if (this.nodePoint_ === node) {
      if (this.isSearchResult_) {
        pm2f.drawCut(this.leftSearchAttrCicleCut_, x - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
        pm2f.drawCut(this.centerSearchAttrCut_, x + 8 - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y, width / 116);
        pm2f.drawCut(this.rightSearchAttrCicleCut_, x + 8 + width - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
      } else {
        pm2f.drawCut(this.leftRectFocusCicleCut_, x - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
        pm2f.drawCut(this.centerFocusCut_, x + 8 - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y, width / 116);
        pm2f.drawCut(this.rightRectFocusCicleCut_, x + 8 + width - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
      }
    } else {
      if (this.searchKey !== null && node.name_.indexOf(this.searchKey) > -1 && this.isSearchResult_) {
        pm2f.drawCut( this.leftSearchFocusCicleCut_, x - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
        pm2f.drawCut(this.centerSearchCut_, x + 8 - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y, width / 116);
        pm2f.drawCut( this.rightSearchFocusCicleCut_, x + 8 + width - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
      } else {
        pm2f.drawCut( this.leftRectCicleCut_, x - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
        pm2f.drawCut( this.centerRectCut_, x + 8 - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y, width / 116);
        pm2f.drawCut(this.rightRectCicleCut_, x + 8 + width - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
      }
    }
    node.nodeWidth_ = 8 * 2 + width;
    let tx = x + 8 + width - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_) + 8;
    if (this.maxX < tx) {
      this.maxX = tx;
    }
  }

  /**
   * 绘制节点数
   * @param {} pm2f X2DFast
   * @param {*} x 起始x坐标
   * @param {*} y 起始y坐标
   * @param {*} h 节点高度
   * @param {*} node 节点
   */
  setNodeMoreButton(pm2f, x, y, w, h, node) {
    if (this.nodeMoreBtnPoint_ >= this.nodeMoreBtns.length) {
      this.nodeMoreBtns.push(new XButton());
    }
    let pbtn = this.nodeMoreBtns[this.nodeMoreBtnPoint_];
    if (node.parent_ === undefined) {
      pbtn.move(x + MainEditor.NODE_RECT_WIDTH + MainEditor.NODE_SIZE_BG_OFFX,
        y + (MainEditor.NODE_RECT_HEIGHT - MainEditor.NODE_MORE_CHILD) / CONSTANT_MIDDLE, w, h);
    } else {
      if (node.isOpen_) {
        pbtn.move(x + node.nodeWidth_ + MainEditor.NODE_SIZE_BG_OFFX,
          y + (MainEditor.NODE_RECT_HEIGHT - MainEditor.NODE_MORE_CHILD) / CONSTANT_MIDDLE, w, h);
      } else {
        pbtn.move(x + node.nodeWidth_ + MainEditor.NODE_SIZE_BG_OFFX,
          y + (MainEditor.NODE_RECT_HEIGHT - MainEditor.NODE_MORE_CHILD) / CONSTANT_MIDDLE, w, h);
      }
    }
    if (node.isOpen_) {
      pm2f.drawCut(this.circleOpenCut_, x + node.nodeWidth_ + MainEditor.NODE_SIZE_BG_OFFX,
        y + (MainEditor.NODE_RECT_HEIGHT - MainEditor.NODE_MORE_CHILD) / CONSTANT_MIDDLE);
    } else {
      let c = 0xffffffff;
      let oy = 1;
      let ox = 1;
      let ra = 0;
      let sh = 1;
      let sw = 1;
      let yOffset = 4;
      let textSize = 10;
      pm2f.drawCut(this.circleCut_, x + node.nodeWidth_ + MainEditor.NODE_SIZE_BG_OFFX,
        y + (MainEditor.NODE_RECT_HEIGHT - MainEditor.NODE_MORE_CHILD) / CONSTANT_MIDDLE);
      let textWidth = pm2f.getTextWidth(node.value_.length, 10);
      pm2f.drawText(node.value_.length, textSize, x + node.nodeWidth_ + MainEditor.NODE_SIZE_BG_OFFX +
        MainEditor.NODE_MORE_CHILD / CONSTANT_MIDDLE - textWidth / CONSTANT_MIDDLE,
        y + MainEditor.NODE_RECT_HEIGHT / CONSTANT_MIDDLE - yOffset, sw, sh, ra, ox, oy, c);
    }
    pbtn.node_ = node;
    this.nodeMoreBtnPoint_ += 1;
  }

  /**
   * 移动绘制
   * @param {} xOffset x偏移值
   */
  moveAndDraw(xOffset) {
    let x = 16;
    let y = 20;
    let w = window.innerWidth - xOffset - DRAW_HEIGHT - x;
    let h = 20;
    this.sltInclude.move(x, y, w, h).draw();
  }

  /**
   * 绘制Rect
   * @param {} pm2f X2DFast
   * @param {*} xx 起始x坐标
   * @param {*} yy 起始y坐标
   * @param {*} xOffset x偏移值
   */
  fillWholeRect(pm2f, xx, yy, xOffset) {
    pm2f.fillRect(xx, yy, window.innerWidth, DRAW_HEIGHT, MainEditor.CANVAS_LINE);
    pm2f.fillRect(
      window.innerWidth - xOffset - DRAW_HEIGHT,
      0,
      DRAW_HEIGHT,
      window.innerHeight,
      MainEditor.CANVAS_LINE
    );
    yy = 4;
    let wSpace = 4;
    let h = 48;
    pm2f.fillRect(xx, yy, window.innerWidth - xOffset - wSpace, h, MainEditor.CANVAS_BG);
    yy = 52;
    pm2f.fillRect(
      xx,
      yy,
      window.innerWidth - xOffset - DRAW_HEIGHT,
      DRAW_HEIGHT,
      MainEditor.CANVAS_LINE
    );
  }

  /**
   * 修改节点位置
   */
  subFuncModify() {
    this.offX_ -= this.modifyPos_.node.posX - this.modifyPos_.x;
    this.offY_ -= this.modifyPos_.node.posY - this.modifyPos_.y;
    this.modifyPos_ = null;
  }

  /**
   * 绘制Rect
   * @param {} pm2f X2DFast
   * @param {*} x 起始x坐标
   * @param {*} y 起始y坐标
   * @param {*} data 绘制的数据对象
   */
  subFuncFirstDraw(pm2f, x, y, data) {
    this.offX_ = 0;
    this.offY_ = -data.posY + Scr.logich / CONSTANT_MIDDLE;
    this.maxX = 0;
    this.drawObj(pm2f, data, this.offX_, this.offY_, '');
    pm2f.fillRect(x, y, Scr.logicw, Scr.logich, MainEditor.CANVAS_BG);
    this.offX_ = (Scr.logicw - this.maxX) / CONSTANT_MIDDLE;
    this.isFirstDraw = false;
  }

  /**
   * 绘制接口
   * @param {} pm2f X2DFast
   * @param {*} xx 起始x坐标
   * @param {*} yy 起始y坐标
   */  
  subFuncDraw(pm2f, xx, yy) {
    let data = this.files_[this.filePoint_];
    this.calcPostionY(data, 0);
    if (this.modifyPos_) {
      this.subFuncModify();
    } else if (this.isFirstDraw) {
      this.subFuncFirstDraw(pm2f, xx, yy, data);
    }
    this.nodeBtnPoint_ = 0;
    this.nodeMoreBtnPoint_ = 0;
    this.drawObj(pm2f, data, this.offX_, this.offY_, '');
  }

  /**
   * 绘制选择文本
   * @param {} pm2f X2DFast
   */   
  drawSelectText(pm2f) {
    let drawTextName = '点击选择目标';
    this.callDrawText(pm2f, drawTextName);
    this.btnCancelSelect_.name_ = '取消选择';
  }

  callDrawText(pm2f, drawTextName) {
    pm2f.drawText(drawTextName, MainEditor.NODE_TEXT_SIZE, this.mousePos_.x,
      this.mousePos_.y, 1, 1, 0, DRAWTEXT_OFFSETX, DRAWTEXT_OFFSETY, MainEditor.NODE_TEXT_COLOR);
  }

  /**
   * 绘制复制文本
   * @param {} pm2f X2DFast
   */   
  drawCopyText(pm2f) {
    let drawTextName = '已复制' + this.selectNode_.pnode.name_;
    this.callDrawText(pm2f, drawTextName);
    this.btnCancelSelect_.name_ = '取消复制';
  }

  /**
   * 绘制剪切文本
   * @param {} pm2f X2DFast
   * @param {*} xx 起始x坐标
   * @param {*} yy 起始y坐标
   */   
  drawCutText(pm2f) {
    pm2f.drawText(
      '已剪切' + this.selectNode_.pnode.name_,
      18,
      this.mousePos_.x,
      this.mousePos_.y,
      1,
      1,
      0,
      -3,
      -3,
      MainEditor.NODE_TEXT_COLOR
    );
    this.btnCancelSelect_.name_ = '取消剪切';
  }

  /**
   * 绘制错误信息文本
   * @param {} pm2f X2DFast
   * @param {*} xx 起始x坐标
   * @param {*} yy 起始y坐标
   */    
  drawErrorText(pm2f, a, y) {
    a = a << 24;
    pm2f.fillRect(0, y, Scr.logicw, 20, 0xff0000 | a);
    pm2f.drawText(
      this.errorMsg_[i][1],
      MainEditor.NODE_TEXT_SIZE,
      Scr.logicw / CONSTANT_MIDDLE,
      y,
      1,
      1,
      0,
      -2,
      -1,
      MainEditor.NODE_TEXT_COLOR
    );
  }

  /**
   * 绘制一组剪切文本
   * @param {} pm2f X2DFast
   * @param {*} xx 起始x坐标
   * @param {*} yy 起始y坐标
   */  
  drawCuts(pm2f, x, y) {
    let xOffset = 28;
    let yDoubleOffset = 56;
    let ySpace = 8;
    pm2f.drawCut(this.searchBgCut_, x, y);
    pm2f.drawCut(this.searchCut_, x + xOffset, y + yDoubleOffset / CONSTANT_MIDDLE - ySpace);
    x = x + 16 + 290 + 16;

    let searchResultTxt =
    this.searchInput.result.length === 0 ? 'No result' : this.searchInput.point + 1 + '/' + this.searchInput.result.length;
    let color = 0xffffffff;
    let offsetY = -2;
    let offsetX = -1;
    let ra = 0;
    let sh = 1;
    let sw = 1;
    let yOffsetBase = 56;
    let yOffset = 3;
    let xStep = 16;
    let yRightSpace = 8;
    let start = 74;

    x += pm2f.drawText(
      searchResultTxt,
      MainEditor.NODE_TEXT_SIZE,
      x,
      y + yOffsetBase / CONSTANT_MIDDLE + yOffset,
      sw,
      sh,
      ra,
      offsetX,
      offsetY,
      color
    );
    x += start - pm2f.getTextWidth(searchResultTxt, MainEditor.NODE_TEXT_SIZE);
    pm2f.drawCut(this.upCut_, x, y + yOffsetBase / CONSTANT_MIDDLE - yRightSpace);
    this.searchInput.btnUp.move(x, y + yOffsetBase / CONSTANT_MIDDLE - yRightSpace, xStep, xStep);
    x += xStep + xStep;
    pm2f.drawCut(this.downCut_, x, y + yOffsetBase / CONSTANT_MIDDLE - yRightSpace);
    this.searchInput.btnDown.move(x, y + yOffsetBase / CONSTANT_MIDDLE - yRightSpace, xStep, xStep);
    x += xStep + xStep;
    pm2f.drawCut(this.closeCut_, x, y + yOffsetBase / CONSTANT_MIDDLE - yRightSpace);
    this.searchInput.btnClose.move(x, y + yOffsetBase / CONSTANT_MIDDLE - yRightSpace, xStep, xStep);
  }

  /**
   * MainEditor绘制
   * @param {} pm2f X2DFast
   */  
  draw(pm2f) {    
    let xx = 0;
    let yy = 0;
    getVsCodeTheme();
    pm2f.fillRect(xx, yy, Scr.logicw, Scr.logich, MainEditor.CANVAS_BG);
    if (this.filePoint_ !== null && this.filePoint_ in this.files_) {      
      this.subFuncDraw(pm2f, xx, yy);
    }
    let xOffset = 420;
    this.fillWholeRect(pm2f, xx, yy, xOffset);    
    this.sltInclude.setColor(MainEditor.CANVAS_BG, MainEditor.NODE_TEXT_COLOR);
    this.moveAndDraw(xOffset);
    
    if (this.selectNode_.type !== null) {
      if (this.selectNode_.type === 'change_target') {
        this.drawSelectText(pm2f);
      } else if (this.selectNode_.type === 'copy_node') {
        this.drawCopyText(pm2f);
      } else if (this.selectNode_.type === 'cut_node') {
        this.drawCutText(pm2f);
      }
      this.btnCancelSelect_.move(Scr.logicw - 250, Scr.logich - 30, 100, 20);
    }

    if (this.errorMsg_.length > 0) {
      let ts = new Date().getTime();
      while (this.errorMsg_.length > 0 && this.errorMsg_[0][0] < ts) {
        this.errorMsg_.shift();
      }

      this.errorMsg_.forEach((item, index) => {  
        let sizeNumber = 20;
        let y = Scr.logich / CONSTANT_MIDDLE - this.errorMsg_.length * sizeNumber + i * sizeNumber;
        let a = parseInt((item[0] - ts) / CONSTANT_MIDDLE);
        if (a > MAX_RANDOM) {
          a = MAX_RANDOM;
        }
        NapiLog.logError(a);
        this.drawErrorText(pm2f, a, y); 
      }); 
    }

    this.delay_ += 1;
    RightMenu.Draw();
    if (this.searchInput) {
      let x = this.searchInput.pos[0];
      let y = this.searchInput.pos[1];
      this.drawCuts(pm2f, x, y);
    }
    this.procAll();
  }
  
  buttonClickedProc(nodeBtns) {
    if (
      this.selectNode_.type === null || this.selectNode_.type === 'copy_node' ||
      this.selectNode_.type === 'cut_node') {
      this.nodePoint_ = nodeBtns.node_;
      AttrEditor.gi().freshEditor(this.filePoint_, this.nodePoint_);
      return true;
    }
    if (this.selectNode_.type === 'change_target') {
      let pn = nodeBtns.node_;
      if (pn.type_ === DataType.NODE) {
        if (this.selectNode_.pnode.type_ === DataType.NODE) {
          if (
            NodeTools.getPathByNode(this.selectNode_.pnode.parent_) ===
            NodeTools.getPathByNode(pn.parent_)
          ) {
            this.selectNode_.pnode.ref_ = pn.name_;
          } else {
            this.selectNode_.pnode.ref_ = NodeTools.getPathByNode(pn);
          }
        } else if (this.selectNode_.pnode.type_ === DataType.REFERENCE) {
          if (
            NodeTools.getPathByNode(this.selectNode_.pnode.parent_.parent_) ===
            NodeTools.getPathByNode(pn.parent_)
          ) {
            this.selectNode_.pnode.value_ = pn.name_;
          } else {
            this.selectNode_.pnode.value_ = NodeTools.getPathByNode(pn);
          }
        }

        this.selectNode_.type = null;
        AttrEditor.gi().freshEditor(this.filePoint_, this.nodePoint_);
        this.onAttributeChange('writefile');
      } else {
        XMessage.gi().send('WrongNode', '');
      }
    }
    return true;
  }

  dropAllLocked(msg, x, y) {
    if (msg === MouseType.MOVE) {
      this.offX_ += x - this.dropAll_.oldx;
      this.offY_ += y - this.dropAll_.oldy;
      this.dropAll_.oldx = x;
      this.dropAll_.oldy = y;
    }
    if (msg === MouseType.UP) {
      this.dropAll_.locked = false;
    }
  }

  procTouch(msg, x, y) {
    const ADD = 6;
    const DELETE = 7;
    if (this.searchInput) {
      if (XTools.InRect(x, y, ...this.searchInput.pos)) {
        if (this.searchInput.btnUp.procTouch(msg, x, y)) {
          if (this.searchInput.btnUp.isClicked()) {
            this.searchInput.point -= 1;
            if (this.searchInput.point < 0) {
              this.searchInput.point = this.searchInput.result.length - 1;
            }
            this.locateNode(this.searchInput.result[this.searchInput.point]);
          }
        }
        if (this.searchInput.btnDown.procTouch(msg, x, y)) {
          if (this.searchInput.btnDown.isClicked()) {
            this.searchInput.point += 1;
            if (this.searchInput.point >= this.searchInput.result.length) {
              this.searchInput.point = 0;
            }
            this.locateNode(this.searchInput.result[this.searchInput.point]);
          }
        }
        return true;
      } else {
        if (msg === MouseType.DOWN) {
          this.searchInput = null;
          this.isSearchResult_ = false;
        }
        return true;
      }
    }

    if (RightMenu.Touch(msg, x, y)) {
      return true;
    }
    this.mousePos_.x = x;
    this.mousePos_.y = y;
    if (this.dropAll_.locked) {
      this.dropAllLocked(msg, x, y);
      return true;
    }

    if (this.sltInclude.procTouch(msg, x, y)) {
      return true;
    }

    if (this.selectNode_.type !== null) {
      if (this.btnCancelSelect_.procTouch(msg, x, y)) {
        if (this.btnCancelSelect_.isClicked()) {
          this.selectNode_.type = null;
        }
        return true;
      }
    }

    for (let i = 0; i < this.nodeBtns.length; i++) {
      if (this.nodeBtns[i].procTouch(msg, x, y)) {
        let nodeBtns = this.nodeBtns[i];
        if (nodeBtns.isClicked()) {
          this.buttonClickedProc(nodeBtns);
        } else if (nodeBtns.isRightClicked()) {
          this.onAttributeChange('change_current_select', nodeBtns.node_);
          switch (nodeBtns.node_.type_) {
            case ADD:
              RightMenu.Reset(
                [
                  RightMenu.Button(null, 'Add Child Node', null, () => {
                    this.procAddNodeAction();
                    this.onAttributeChange('writefile');
                  }),
                  RightMenu.Button(null, 'Add Sub Property', null, () => {
                    this.procAddAttAction();
                    this.onAttributeChange('writefile');
                  }),
                  RightMenu.Button(null, 'Delete', null, () => {
                    this.procDeleteAction();
                    this.onAttributeChange('writefile');
                  }),
                ],
                nodeBtns.posX_,
                nodeBtns.posY_ + MainEditor.NODE_RECT_HEIGHT
              );
              break;
            case DELETE:
              RightMenu.Reset(
                [
                  RightMenu.Button(null, 'Delete', null, () => {
                    this.procDeleteAction();
                    this.onAttributeChange('writefile');
                  }),
                ],
                nodeBtns.posX_,
                nodeBtns.posY_ + MainEditor.NODE_RECT_HEIGHT
              );
              break;
          }
        }

        return true;
      }   
  }

    this.nodeMoreBtns.forEach((item, index) => {
      if (item.procTouch(msg, x, y)) {
        let nodeMoreBtn = item;
        if (nodeMoreBtn.isClicked()) {
          this.buttonClickedProc(nodeMoreBtn);
          item.node_.isOpen_ = !item.node_.isOpen_;
          this.modifyPos_ = {
            node: item.node_,
            x: item.node_.posX,
            y: item.node_.posY,
          };
        }
        return true;
      }      
    });

    if (msg === MouseType.DOWN && !this.dropAll_.locked) {
      this.dropAll_.locked = true;
      this.dropAll_.oldx = x;
      this.dropAll_.oldy = y;
      return true;
    }
    return false;
  }

  procAddNodeAction() {
    let pattr = AttrEditor.gi();
    pattr.changeDataNodeNotInherit('add_child_node', 'button', '');
  }

  procAddAttAction() {
    let pattr = AttrEditor.gi();
    pattr.changeDataNodeNotInherit('add_child_attr', 'button', '');
  }

  procDeleteAction() {
    let pattr = AttrEditor.gi();
    pattr.changeDataNodeNotInherit('delete', 'button', '');
  }
  searchNodeByName(data, name, out) {
    this.searchKey = name;
    if (data.name_.indexOf(name) >= 0) {
      out.push(data);
    }
    switch (data.type_) {
      case DataType.NODE:
        data.value_.forEach((item, index) => {  
          this.searchNodeByName(item, name, out);  
        });
        break;
      case DataType.ATTR:
        this.searchNodeByName(data.value_, name, out);
        break;
    }
  }
  expandAllParents(curNdoe) {
    if (curNdoe.parent_) {
      curNdoe.parent_.isOpen_ = true;
      this.expandAllParents(curNdoe.parent_);
    }
  }
  locateNode(node) {
    if (!node) {
      return;
    }
    this.expandAllParents(node);
    this.isSearchResult_ = true;
    this.offX_ -= node.posX - Scr.logicw / CONSTANT_MIDDLE;
    this.offY_ -= this.offY_ + node.posY - Scr.logich / CONSTANT_MIDDLE;
    this.nodePoint_ = node;
    AttrEditor.gi().freshEditor();
  }

  /**
   * Ctrl+Z 快捷键处理
   */   
  procCtrlZ() {
    let h;
    if (this.historyZ.length <= 0) {
      h = this.historyBase[this.filePoint_];
    } else {
      if (this.historyZ.length > 1 && this.historyPushed) {
        this.historyZ.pop();
        this.historyPushed = false;
      }
      h = this.historyZ.pop();
    }

    this.filePoint_ = h.fn;
    this.rootPoint_ = h.fn;
    Lexer.FILE_AND_DATA[this.filePoint_] = h.data;
    this.parse(this.filePoint_);
    if (h.sel) {
      this.nodePoint_ = NodeTools.getNodeByPath(
        this.files_[this.filePoint_],
        h.sel
      );
    } else {
      this.nodePoint_ = null;
    }
    AttrEditor.gi().freshEditor(this.filePoint_, this.nodePoint_);
  }

  /**
   * Ctrl+F 快捷键处理
   */  
  procCtrlF() {
    let xOffset = 300;
    let posWidth = 450;
    let posHeight = 40;
    this.searchInput = {
      pos: [(Scr.logicw - xOffset) / CONSTANT_MIDDLE, Scr.logich / CONSTANT_QUARTER, posWidth, posHeight],
      result: [],
      point: 0,
      btnUp: new XButton(0, 0, 0, 0, '上一个'),
      btnDown: new XButton(0, 0, 0, 0, '下一个'),
      btnClose: new XButton(0, 0, 0, 0, '关闭'),
    };
    let x = this.searchInput.pos[0];
    let y = this.searchInput.pos[1];
    let w = this.searchInput.pos[2];
    let h = this.searchInput.pos[3];
    let width = 258;
    let height = 32;
    let hOffset = 20;
    CanvasInput.Reset(x, y + (h - hOffset) / CONSTANT_MIDDLE, width, height, '', null, (v) => {
      this.searchInput.result = [];
      if (v.length > 0) {
        this.searchNodeByName(
          this.files_[this.filePoint_],
          v,
          this.searchInput.result
        );
        if (this.searchInput.result.length > 0) {
          this.locateNode(this.searchInput.result[0]);
          this.searchInput.point = 0;
        }
      }
    });
    CanvasInput.SetSafeArea(...this.searchInput.pos);
  }

  /**
   * Ctrl+V 快捷键处理
   */
  procCtrlV() {
    if (this.selectNode_.type !== null && this.nodePoint_ !== null) {
      let parent = this.nodePoint_;
      if (this.nodePoint_.type_ !== DataType.NODE) {
        parent = this.nodePoint_.parent_;
      }
      parent.value_.push(NodeTools.copyNode(this.selectNode_.pnode, parent));
      if (this.selectNode_.type === 'cut_node') {
        ModifyNode.deleteNode(this.selectNode_.pnode);
        this.selectNode_.type = null;
      }
      this.checkAllError();
    }
  }

  procKey(k) {
    if (k === 'ctrl+z') {
      if (this.selectNode_.type !== null) {
        this.selectNode_.type = null;
      }
      console.log('!!! popHistory ', this.historyZ.length);
      this.procCtrlZ();  
    } else if (k === 'ctrl+f') {
      this.procCtrlF();
    } else if (k === 'ctrl+c') {
      if (this.nodePoint_ !== null) {
        this.selectNode_ = {
          type: 'copy_node',
          pnode: this.nodePoint_,
        };
      }
    } else if (k === 'ctrl+x') {
      if (this.nodePoint_ !== null) {
        this.selectNode_ = {
          type: 'cut_node',
          pnode: this.nodePoint_,
        };
      }
    } else if (k === 'ctrl+v') {
      this.procCtrlV();
    } else if (k === 'Delete') {
      if (this.nodePoint_ !== null) {
        ModifyNode.deleteNode(this.nodePoint_);
        AttrEditor.gi().freshEditor();
      }
    }
  }

  procAll() {
    while (this.touchQueue_.length > 0) {
      let touch = this.touchQueue_.shift();
      let msgIndex = 0;
      let xIndex = 1;
      let yIndex = 2;
      this.procTouch(touch[msgIndex], touch[xIndex], touch[yIndex]);
    }

    while (this.keyQueue_.length > 0) {
      let k = this.keyQueue_.shift();
      this.procKey(k);
    }
  }
  onSelectInclude(sel) {
    MainEditor.gi().filePoint_ = sel;
    MainEditor.gi().rootPoint_ = sel;
    AttrEditor.gi().freshEditor();
  }

  nodeCount(data) {
    let ret = 1;
    switch (data.type_) {
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
        break;
      case 6:
        data.value_.forEach((item, index) => {  
          ret += this.nodeCount(item);  
        });
        break;
      case 7:
        ret += this.nodeCount(data.value_);
        break;
      case 8:
      case 9:
      case 10:
      case 11:
        break;
      default:
        NapiLog.logError('unknow' + data.type_);
        break;
    }
    return ret;
  }
  isNodeCountChanged(fn, bset = true) {
    if (!(fn in this.nodeCount_)) {
      this.nodeCount_[fn] = -1;
    }
    let newcount = this.nodeCount(this.files_[fn]);
    if (this.nodeCount_[fn] !== newcount) {
      if (bset) {
        this.nodeCount_[fn] = newcount;
      }
      return true;
    }
    return false;
  }
  saveHistory(fn, data2, pth, pos = null) {
    console.log('!!! save History ', this.historyZ.length, pos);
    if (fn in this.historyBase) {
      this.historyZ.push({
        fn: fn,
        data: data2,
        sel: pth,
      });
      this.historyPushed = true;
    } else {
      this.historyBase[fn] = {
        fn: fn,
        data: data2,
        sel: pth,
      };
    }
  }
  onAttributeChange(type, value) {
    let pme = MainEditor.gi();
    let lenChangeTarget = 13;
    if (type === 'writefile') {
      let data1 = Generator.gi().makeHcs(pme.filePoint_, pme.files_[pme.filePoint_]);
      let data2 = [];
      let dataCharArr = Array.from(data1);
      dataCharArr.forEach((item, index) => {
        data2.push(data1.charCodeAt(index));  
      });
      if (pme.isNodeCountChanged(pme.filePoint_)) {
        Lexer.FILE_AND_DATA[pme.filePoint_] = data2;
        pme.parse(pme.filePoint_);
        let t = NodeTools.getPathByNode(pme.nodePoint_, false);
        if (t) {
          pme.nodePoint_ = NodeTools.getNodeByPath(pme.files_[pme.filePoint_], t);
        } else {
          pme.nodePoint_ = null;
        }
        if (pme.selectNode_.pnode) {
          let t = NodeTools.getPathByNode(pme.selectNode_.pnode, false);
          if (t) {
            pme.selectNode_.pnode = NodeTools.getNodeByPath(pme.files_[pme.filePoint_], t);
          } else {
            pme.selectNode_.pnode = null;
          }
        }
        AttrEditor.gi().freshEditor(pme.filePoint_, pme.nodePoint_);
      }
      pme.checkAllError();
      XMessage.gi().send('writefile', {
        fn: pme.filePoint_,
        data: data1,
      });
      pme.saveHistory(pme.filePoint_, data2, NodeTools.getPathByNode(pme.nodePoint_), 1);
    } else if (type.substring(0, lenChangeTarget) === 'change_target') {
      pme.selectNode_.type = type;
      pme.selectNode_.pnode = value;
    } else if (type.startsWith('cancel_change_target')) {
      pme.selectNode_.type = null;
    } else if (type === 'change_current_select') {
      pme.nodePoint_ = value;
      AttrEditor.gi().freshEditor(pme.filePoint_, pme.nodePoint_);
    }
  }
  onError(msg) {}
  onTouch(msg, x, y) {
    this.touchQueue_.push([msg, x, y]);
  }
  onKey(k) {
    this.keyQueue_.push(k);
  }
  onReceive(type, data) {
    console.log('onReceive type=' + type);
    NapiLog.logError(type);
    let me = MainEditor.gi();
    if (type === 'parse') {
      me.parse(data);
    } else if (type === 'filedata') {
      me.saveHistory(data.fn, data.data, null, 2);
      Lexer.FILE_AND_DATA[data.fn] = data.data;
      me.parse(data.fn);
    } else if (type === 'freshfiledata') {
      me.saveHistory(data.fn, data.data, null, 3);
      Lexer.FILE_AND_DATA[data.fn] = data.data;
    } else if (type === 'whiteCutImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initCutData(wurl);
    } else if (type === 'circleImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initCicleImgData(wurl);
    } else if (type === 'cicleOpenImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initcicleOpenImgData(wurl);
    } else if (type === 'rectangleFocusImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initRectangleFocusImgData(wurl);
    } else if (type === 'nodeIconImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initNodeIconImgData(wurl);
    } else if (type === 'attrIconImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initAttrIconImgData(wurl);
    } else if (type === 'rootIconImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initRootIconImgData(wurl);
    } else if (type === 'rootIconFocusImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initRootIconFocusImgData(wurl);
    } else if (type === 'backgroundImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initBackgroundImgData(wurl);
    } else if (type === 'popItemFocusImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initPopItemFocusImgData(wurl);
    } else if (type === 'colorThemeChanged') {
      me.reloadMenuBgPic();
    } else if (type === 'searchBgImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initSearchBgImgData(wurl);
    } else if (type === 'upCutImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initUpImgData(wurl);
    } else if (type === 'downCut') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initDownImgData(wurl);
    } else if (type === 'closeCutImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initCloseImgData(wurl);
    } else if (type === 'searchCutImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initSearchImgData(wurl);
    } else if (type === 'searchNoodRectImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initSearchNoodRectImgData(wurl);
    } else if (type === 'searchAttrRectImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initSearchAttrRectImgData(wurl);
    } else {
      NapiLog.logError('onReceive is not need');
    }
  }

  initSearchAttrRectImgData(wurl) {
    const HEIGHT = 32;
    const NEW_WIDTH = 132;
    const OLD_WIDTH = 8;
    const CS_X_VAL = 8;
    const RS_X_VAL = 124;
    const CS_OLD_WIDTH = 116;
    this.searchAttrCicleImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.leftSearchAttrCicleCut_ = XTexture.gi().makeCut(this.searchAttrCicleImg_, 0, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.centerSearchAttrCut_ = XTexture.gi().makeCut(this.searchAttrCicleImg_, CS_X_VAL, 0, CS_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.rightSearchAttrCicleCut_ = XTexture.gi().makeCut(this.searchAttrCicleImg_, RS_X_VAL, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
  }

  initSearchNoodRectImgData(wurl) {
    const HEIGHT = 32;
    const NEW_WIDTH = 132;
    const OLD_WIDTH = 8;
    const CS_X_VAL = 8;
    const RS_X_VAL = 124;
    const CS_OLD_WIDTH = 116;
    this.searchRectFocusCicleImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.leftSearchFocusCicleCut_ = XTexture.gi().makeCut(this.searchRectFocusCicleImg_, 0, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.centerSearchCut_ = XTexture.gi().makeCut(this.searchRectFocusCicleImg_, CS_X_VAL, 0, CS_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.rightSearchFocusCicleCut_ = XTexture.gi().makeCut(this.searchRectFocusCicleImg_, RS_X_VAL, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
  }

  initSearchImgData(wurl) {
    const WIDTH = 16;
    const HEIGHT = 16;
    this.searchImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.searchCut_ = XTexture.gi().makeCut(this.searchImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initCloseImgData(wurl) {
    const WIDTH = 16;
    const HEIGHT = 16;
    this.closeImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.closeCut_ = XTexture.gi().makeCut(this.closeImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initDownImgData(wurl) {
    const WIDTH = 16;
    const HEIGHT = 16;
    this.downImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.downCut_ = XTexture.gi().makeCut(this.downImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initUpImgData(wurl) {
    const WIDTH = 16;
    const HEIGHT = 16;
    this.upImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.upCut_ = XTexture.gi().makeCut(this.upImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initSearchBgImgData(wurl) {
    const WIDTH = 494;
    const HEIGHT = 56;
    this.searchBgImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.searchBgCut_ = XTexture.gi().makeCut(this.searchBgImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initPopItemFocusImgData(wurl) {
    const WIDTH = 148;
    const HEIGHT = 32;
    RightMenu.popItemFocusImg_ = XTexture.gi().loadTextureFromImage(wurl);
    RightMenu.popItemFocusCut_ = XTexture.gi().makeCut(RightMenu.popItemFocusImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initBackgroundImgData(wurl) {
    const WIDTH = 156;
    const HEIGHT = 112;
    RightMenu.backgroundImg_ = XTexture.gi().loadTextureFromImage(wurl);
    RightMenu.backgroundCut_ = XTexture.gi().makeCut(RightMenu.backgroundImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initRootIconFocusImgData(wurl) {
    const WIDTH = 132;
    const HEIGHT = 32;
    this.rootIconFocusImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.rootIconFocusCut_ = XTexture.gi().makeCut(this.rootIconFocusImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initRootIconImgData(wurl) {
    const WIDTH = 132;
    const HEIGHT = 32;
    this.rootIconImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.rootIconCut_ = XTexture.gi().makeCut(this.rootIconImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initAttrIconImgData(wurl) {
    const WIDTH = 8;
    const HEIGHT = 8;
    this.attrIconImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.attrIconCut_ = XTexture.gi().makeCut(this.attrIconImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initNodeIconImgData(wurl) {
    const WIDTH = 8;
    const HEIGHT = 8;
    this.nodeIconImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.nodeIconCut_ = XTexture.gi().makeCut(this.nodeIconImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initcicleOpenImgData(wurl) {
    const WIDTH = 20;
    const HEIGHT = 20;
    this.cicleOpenImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.circleOpenCut_ = XTexture.gi().makeCut(this.cicleOpenImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initRectangleFocusImgData(wurl) {
    const HEIGHT = 32;
    const NEW_WIDTH = 132;
    const OLD_WIDTH = 8;
    const RF_OLD_WIDTH = 132;
    const CF_X_VAL = 8;
    const CF_OLD_WIDTH = 116;
    const RR_X_VAL = 124;
    this.rectangleFocusImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.rectangleFocusCut_ = XTexture.gi().makeCut(this.rectangleFocusImg_, 0, 0, RF_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.leftRectFocusCicleCut_ = XTexture.gi().makeCut(this.rectangleFocusImg_, 0, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.centerFocusCut_ = XTexture.gi().makeCut(this.rectangleFocusImg_, CF_X_VAL, 0, CF_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.rightRectFocusCicleCut_ = XTexture.gi().makeCut(this.rectangleFocusImg_, RR_X_VAL, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
  }

  initCicleImgData(wurl) {
    const WIDTH = 20;
    const HEIGHT = 20;
    this.cicleImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.circleCut_ = XTexture.gi().makeCut(this.cicleImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initCutData(wurl) {
    const HEIGHT = 32;
    const NEW_WIDTH = 132;
    const OLD_WIDTH = 8;
    const CR_X_VAL = 8;
    const RR_X_VAL = 124;
    const WC_OLD_WIDTH = 132;
    const CR_OLD_WIDTH = 116;
    this.whiteImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.whiteCut_ = XTexture.gi().makeCut(this.whiteImg_, 0, 0, WC_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.leftRectCicleCut_ = XTexture.gi().makeCut(this.whiteImg_, 0, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.centerRectCut_ = XTexture.gi().makeCut(this.whiteImg_, CR_X_VAL, 0, CR_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.rightRectCicleCut_ = XTexture.gi().makeCut(this.whiteImg_, RR_X_VAL, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
  }

  syncOpenStatus(newNode, oldParentNode) {
    let oldNode = null;
    oldParentNode.value_.forEach((item, index) => {  
      if (newNode.name_ === item.name_) {
        oldNode = item;
      } 
    });
    if (oldNode === null) {
      return;
    }
    newNode.isOpen_ = oldNode.isOpen_;

    if (newNode.type_ === DataType.NODE) {
      newNode.value_.forEach((j, index) => {
        this.syncOpenStatus(j, oldNode);
      });
    }
  }

  syncRootStatus(newRoot, oldRoot) {
    newRoot.isOpen_ = oldRoot.isOpen_;
    newRoot.value_.forEach((item, index) => {
      this.syncOpenStatus(item, oldRoot);
    });
  }

  parse(fn) {
    if (this.rootPoint_ === null) {
      this.rootPoint_ = fn;
    }
    let t = Generator.gi().hcsToAst(fn);
    if (!t) {
      return;
    }

    let fs = [];
    Object.keys(t).forEach((index) => {
      let newRoot = Generator.gi().astToObj(t[index].ast.astRoot_);

      if (this.files_[index]) {
        this.syncRootStatus(newRoot, this.files_[index]);
      }

      this.files_[index] = newRoot;
      fs.push(index);  
    });

    this.filePoint_ = this.rootPoint_;
    this.sltInclude.resetList(fs, this.filePoint_);
    AttrEditor.gi().setFiles(this.files_);

    this.checkAllError();
  }

  checkAllError() {
    NapiLog.clearError();
    let n1 = Generator.gi().mergeObj(this.files_);
    if (n1) {
      n1 = Generator.gi().expandObj(n1);
      if (NapiLog.getResult()[0]) {
        return true;
      }
    }
    return false;
  }
}
MainEditor.LINE_HEIGHT = 50;
MainEditor.NODE_RECT_HEIGHT = 32;
MainEditor.NODE_RECT_WIDTH = 132;
MainEditor.NODE_TEXT_COLOR = 0xffffffff; // white
MainEditor.NODE_TEXT_SIZE = 14;
MainEditor.BTN_CONTENT_OFFY = 4;
MainEditor.NODE_TEXT_OFFX = 5;
MainEditor.NODE_LINE_COLOR = 0xff979797;
MainEditor.NODE_SIZE_BG_OFFX = 4;
MainEditor.NODE_MORE_CHILD = 20;
MainEditor.LINE_WIDTH = 30;
MainEditor.LOGO_LEFT_PADDING = 14;
MainEditor.LOGO_SIZE = 8;

MainEditor.pInstance_ = null;
MainEditor.gi = function () {
  if (MainEditor.pInstance_ === null) {
    MainEditor.pInstance_ = new MainEditor();
  }
  return MainEditor.pInstance_;
};

module.exports = {
  MainEditor,
};
