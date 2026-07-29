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

const { AttributeArea } = require('./attr/AttributeArea');
const { ModifyNode } = require('./hcs/ModifyNode');
const { DataType, NodeType } = require('./hcs/NodeTools');
const { NodeTools } = require('./hcs/NodeTools');
const INT64_MAX = 9223372036854775807n;
const INT64_MIN = -9223372036854775808n;
const CASE_DELETE = 5;
const CASE_QUOTE = 4;
const CASE_BOOL = 3;
const CASE_ARRAY = 2;
const CASE_CHARACTER_STR = 1;
const CASE_INTEGER = 0;

class AttrEditor {
  constructor() {
    AttributeArea.gi().registRecvCallback(this.onChange);
    this.files_ = [];
    this.callback_ = null;
  }
  setFiles(files) {
    this.files_ = files;
  }
  freshEditor(fn, node) {
    this.filePoint_ = fn;
    this.node_ = node;
    if (fn in this.files_) {
      this.root_ = this.files_[fn];
    }

    AttributeArea.gi().clear();
    if (this.node_ !== null && this.node_ !== undefined) {
      AttributeArea.gi().addTitle(this.node_.name_);
      switch (this.node_.type_) {
        case DataType.NODE:
          switch (this.node_.nodeType_) {
            case NodeType.DATA:
              this.freshDataNodeNotInheritEditor(this.node_);
              break;
            case NodeType.COPY:
            case NodeType.REFERENCE:
            case NodeType.INHERIT:
              this.freshcopyNodeEditor(this.node_);
              break;
            case NodeType.DELETE:
              this.freshdeleteNodeEditor(this.node_);
              break;
            case NodeType.TEMPLETE:
              this.freshTempletNodeEditor(this.node_);
              break;
            default:
              break;
          }
          break;
        case DataType.ATTR:
          this.freshDefineAttributeEditor(this.node_);
          break;
      }
    } else {
      AttributeArea.gi().addTitle('Property editing area');
    }

    AttributeArea.gi().flush();
  }

  freshDataNodeNotInheritEditor(node) {
    AttributeArea.gi().addDotLine();
    AttributeArea.gi().addTopInput(
      'name',
      'Node Name',
      node.name_,
      this.root_ == this.node_
    );
    AttributeArea.gi().addSelect(
      'node_type',
      'Node Type',
      AttrEditor.NODE_TYPE_STR,
      AttrEditor.NODE_TYPE_STR[node.nodeType_],
      this.root_ == this.node_
    );
  }
  changeDataNodeNotInherit(searchId, type, value) {
    if (searchId === 'node_type') {
      AttrEditor.gi().changeNodeType(value);
    } else if (searchId === 'name' && this.node_.name_ !== value) {
      ModifyNode.modifyName(this.files_, this.root_, this.node_, value);
    } else if (searchId === 'add_child_node') {
      this.node_.isOpen_ = true;
      let newNode = ModifyNode.addChildNode(this.root_, this.node_);
      this.callCallback('change_current_select', newNode);
    } else if (searchId === 'add_child_attr') {
      this.node_.isOpen_ = true;
      let newNode = ModifyNode.addChildAttr(this.root_, this.node_);
      this.callCallback('change_current_select', newNode);
    } else if (searchId === 'delete') {
      ModifyNode.deleteNode(this.node_);
      this.freshEditor();
    }
  }
  freshcopyNodeEditor(node) {
    AttributeArea.gi().addDotLine();
    AttributeArea.gi().addTopInput('name', 'Node Name', node.name_);
    AttributeArea.gi().addSelect(
      'node_type',
      'Node Type',
      AttrEditor.NODE_TYPE_STR,
      AttrEditor.NODE_TYPE_STR[node.nodeType_],
      this.root_ == this.node_
    );
    let btnName = node.ref_;
    if (btnName === 'unknow') {
      switch (node.nodeType_) {
        case NodeType.COPY:
          btnName = '取消复制';
          break;
        case NodeType.INHERIT:
          btnName = '取消继承';
          break;
        case NodeType.REFERENCE:
          btnName = '取消引用';
          break;
      }
    }
    AttributeArea.gi().addLabelButton(
      'change_target',
      btnName,
      'Original Node'
    );
  }

  changecopyNode(searchId, type, value) {
    if (searchId === 'name' && this.node_.name_ !== value) {
      ModifyNode.modifyName(this.files_, this.root_, this.node_, value);
    } else if (searchId === 'change_target') {
      if (this.node_.ref_ === 'unknow') {
        ModifyNode.modifyNodeType(this.files_, this.root_, this.node_, 0);
        this.freshEditor(this.filePoint_, this.node_);
        this.callCallback('cancel_change_target', this.node_);
      } else {
        this.callCallback('change_target', this.node_);
      }
    } else if (searchId === 'node_type') {
      AttrEditor.gi().changeNodeType(value);
    } else if (searchId === 'add_child_node') {
      ModifyNode.addChildNode(this.root_, this.node_);
    } else if (searchId === 'add_child_attr') {
      ModifyNode.addChildAttr(this.root_, this.node_);
    } else if (searchId === 'delete') {
      ModifyNode.deleteNode(this.node_);
      this.freshEditor();
    }
  }

  freshdeleteNodeEditor(node) {
    AttributeArea.gi().addTopInput('name', 'Node Name', node.name_);
    AttributeArea.gi().addSelect(
      'node_type',
      'Node Type',
      AttrEditor.NODE_TYPE_STR,
      AttrEditor.NODE_TYPE_STR[node.nodeType_],
      this.root_ == this.node_
    );
  }

  changeNodeType(value) {
    ModifyNode.modifyNodeType(
      this.files_,
      this.root_,
      this.node_,
      AttrEditor.NODE_TYPE_STR.indexOf(value)
    );
    this.freshEditor(this.filePoint_, this.node_);
    if (
      this.node_.nodeType_ === NodeType.COPY ||
      this.node_.nodeType_ === NodeType.INHERIT ||
      this.node_.nodeType_ === NodeType.REFERENCE
    ) {
      this.callCallback('change_target', this.node_);
    }
  }
  changedeleteNode(searchId, type, value) {
    if (searchId === 'name' && this.node_.name_ !== value) {
      ModifyNode.modifyName(this.files_, this.root_, this.node_, value);
    } else if (searchId === 'delete') {
      ModifyNode.deleteNode(this.node_);
      this.freshEditor();
    } else if (searchId === 'node_type') {
      AttrEditor.gi().changeNodeType(value);
    }
  }
  freshTempletNodeEditor(node) {
    AttributeArea.gi().addDotLine();
    AttributeArea.gi().addTopInput('name', 'Node Name', node.name_);
    AttributeArea.gi().addSelect(
      'node_type',
      'Node Type',
      AttrEditor.NODE_TYPE_STR,
      AttrEditor.NODE_TYPE_STR[node.nodeType_],
      this.root_ == this.node_
    );
  }
  changeTempletNode(searchId, type, value) {
    if (searchId === 'name') {
      ModifyNode.modifyName(this.files_, this.root_, this.node_, value);
    } else if (searchId === 'add_child_node') {
      ModifyNode.addChildNode(this.root_, this.node_);
    } else if (searchId === 'add_child_attr') {
      ModifyNode.addChildAttr(this.root_, this.node_);
    } else if (searchId === 'delete') {
      ModifyNode.deleteNode(this.node_);
      this.freshEditor();
    } else if (searchId === 'node_type') {
      AttrEditor.gi().changeNodeType(value);
    }
  }

  freshDefineAttributeEditor(node) {
    let v = node.value_;
    AttributeArea.gi().addTopInput('name', 'Name', node.name_);

    if (
      v.type_ === DataType.INT8 ||
      v.type_ === DataType.INT16 ||
      v.type_ === DataType.INT32 ||
      v.type_ === DataType.INT64) {
      AttributeArea.gi().addSelect('value_type', 'Type', AttrEditor.ATTR_TYPE_STR, AttrEditor.ATTR_TYPE_STR[CASE_INTEGER]);
      AttributeArea.gi().addValidatorInput(
        'value', 'Attribute Value', NodeTools.jinZhi10ToX(v.value_, v.jinzhi_), false, '请输入整数');
    } else if (v.type_ === DataType.STRING) {
      AttributeArea.gi().addSelect('value_type', 'Type', AttrEditor.ATTR_TYPE_STR, AttrEditor.ATTR_TYPE_STR[CASE_CHARACTER_STR]);
      AttributeArea.gi().addInput('value', 'Attribute Value', v.value_);
    } else if (v.type_ === DataType.ARRAY) {
      AttributeArea.gi().addSelect('value_type', 'Type', AttrEditor.ATTR_TYPE_STR, AttrEditor.ATTR_TYPE_STR[CASE_ARRAY]);
      AttributeArea.gi().addTextArea('value', 'Attribute Value', NodeTools.arrayToString(v, 20));
    } else if (v.type_ === DataType.BOOL) {
      AttributeArea.gi().addSelect('value_type', 'Type', AttrEditor.ATTR_TYPE_STR, AttrEditor.ATTR_TYPE_STR[CASE_BOOL]);
      AttributeArea.gi().addSelect('value', 'Attribute Value', [true, false], v.value_ == 1);
    } else if (v.type_ === DataType.REFERENCE) {
      AttributeArea.gi().addSelect('value_type', 'Type', AttrEditor.ATTR_TYPE_STR, AttrEditor.ATTR_TYPE_STR[CASE_QUOTE]);
      AttributeArea.gi().addLabelButton('change_target', v.value_, 'Original Node');
    } else if (v.type_ === DataType.DELETE) {
      AttributeArea.gi().addSelect('value_type', 'Type', AttrEditor.ATTR_TYPE_STR, AttrEditor.ATTR_TYPE_STR[CASE_DELETE]);
    }
  }

  getNodeValue(v, value) {
    switch (AttrEditor.ATTR_TYPE_STR.indexOf(value)) {
      case CASE_INTEGER:
        v.type_ = DataType.INT8;
        v.value_ = 0;
        v.jinzhi_ = 10;
        break;
      case CASE_CHARACTER_STR:
        v.type_ = DataType.STRING;
        v.value_ = '';
        break;
      case CASE_ARRAY:
        v.type_ = DataType.ARRAY;
        v.value_ = [];
        break;
      case CASE_BOOL:
        v.type_ = DataType.BOOL;
        v.value_ = 1;
        break;
      case CASE_QUOTE:
        v.type_ = DataType.REFERENCE;
        v.value_ = 'unknow';
        break;
      case CASE_DELETE:
        v.type_ = DataType.DELETE;
        break;
    }
    this.freshEditor(this.filePoint_, this.node_);
  }

  validateNumber(value) {
    let validRes = {};
    validRes.errMsg = '';
    let ret = NodeTools.jinZhiXTo10(value);
    if (ret[0] === undefined) {
      validRes.errMsg = '请输入整数';
    }

    if (ret[0] > INT64_MAX || ret[0] < INT64_MIN) {
      validRes.errMsg = '数字大小不能超出Int64范围';
    }
    validRes.value = ret[0];
    validRes.base = ret[1];
    return validRes;
  }

  changeDefineAttribute(searchId, type, value) {
    let v = this.node_.value_;
    if (searchId === 'name' && value !== this.node_.name_) {
      ModifyNode.modifyName(this.files_, this.root_, this.node_, value);
    }
    if (searchId === 'value_type') {
      this.getNodeValue(v, value);
    }
    if (searchId === 'change_target') {
      this.callCallback('change_target', v);
    }
    if (searchId === 'value') {
      if (
        v.type_ === DataType.INT8 ||
        v.type_ === DataType.INT16 ||
        v.type_ === DataType.INT32 ||
        v.type_ === DataType.INT64
      ) {
        let validRes = this.validateNumber(value);
        let validatorLabel = document.getElementById('valid_' + searchId);
        if (validRes.errMsg === '') {
          validatorLabel.style.display = 'none';
          v.value_ = validRes.value;
          v.jinzhi_ = validRes.base;
        } else {
          validatorLabel.innerText = validRes.errMsg;
          validatorLabel.style.display = '';
        }
      } else if (v.type_ === DataType.STRING) {
        v.value_ = value;
      } else if (v.type_ === DataType.ARRAY) {
        v.value_ = NodeTools.stringToArray(value);
      } else if (v.type_ === DataType.BOOL) {
        v.value_ = value === 'true' ? 1 : 0;
      }
    }
    if (searchId === 'delete') {
      ModifyNode.deleteNode(this.node_);
      this.freshEditor();
    }
  }
  onChange(searchId, type, value) {
    let pattr = AttrEditor.gi();
    if (pattr.node_ !== null) {
      AttributeArea.gi().addTitle(pattr.node_.name_);
      switch (pattr.node_.type_) {
        case DataType.NODE:
          switch (pattr.node_.nodeType_) {
            case NodeType.DATA:
              pattr.changeDataNodeNotInherit(searchId, type, value);
              break;
            case NodeType.COPY:
            case NodeType.REFERENCE:
            case NodeType.INHERIT:
              pattr.changecopyNode(searchId, type, value);
              break;
            case NodeType.DELETE:
              pattr.changedeleteNode(searchId, type, value);
              break;
            case NodeType.TEMPLETE:
              pattr.changeTempletNode(searchId, type, value);
              break;
            default:
              break;
          }
          break;
        case DataType.ATTR:
          pattr.changeDefineAttribute(searchId, type, value);
          break;
      }
      pattr.callCallback('writefile');
    }
  }

  callCallback(type, value) {
    if (this.callback_ !== null) {
      this.callback_(type, value);
    }
  }
  registCallback(func) {
    this.callback_ = func;
  }
}

AttrEditor.NODE_TYPE_STR = [
  '数据类不继承',
  '复制类',
  '引用修改类',
  '删除类',
  '模板类',
  '数据类继承',
];
AttrEditor.ATTR_CLASS = ['定义类属性', '删除类属性', '引用类属性'];

AttrEditor.ATTR_TYPE_STR = ['整数', '字符串', '数组', '布尔', '引用', '删除'];

AttrEditor.pInstance_ = null;
AttrEditor.gi = function () {
  if (AttrEditor.pInstance_ === null) {
    AttrEditor.pInstance_ = new AttrEditor();
  }
  return AttrEditor.pInstance_;
};

module.exports = {
  AttrEditor,
};
