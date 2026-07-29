#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2020-2021 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.


import os
import json
from ast import literal_eval

import hdf_utils
import hdf_tool_version
from hdf_tool_settings import HdfToolSettings
from hdf_tool_exception import HdfToolException
from hdf_tool_version import GetToolVersion
from .hdf_command_handler_base import HdfCommandHandlerBase
from .hdf_linux_scann import HdfLinuxScan
from .hdf_lite_mk_file import HdfLiteMkFile
from .hdf_liteos_scann import HdfLiteScan
from .hdf_vendor_kconfig_file import HdfVendorKconfigFile
from .hdf_module_kconfig_file import HdfModuleKconfigFile
from .hdf_driver_config_file import HdfDriverConfigFile
from .hdf_command_error_code import CommandErrorCode


class HdfGetHandler(HdfCommandHandlerBase):
    def __init__(self, args):
        super(HdfGetHandler, self).__init__()
        self.cmd = 'get'
        self.handlers = {
            'vendor_list': self._get_vendor_list_handler,
            'current_vendor': self._get_current_vendor_handler,
            'vendor_parent_path': self._get_vendor_parent_path_handler,
            'individual_vendor_path': self._get_individual_vendor_path_handler,
            'board_list': self._get_board_list_handler,
            'driver_list': self._get_driver_list_handler,
            'driver_file': self._get_driver_file_handler,
            'drv_config_file': self._get_drv_config_file_handler,
            'hdf_tool_core_version': self._get_version_handler,
            'model_device_list': self._get_model_device_list,
            'model_driver_list': self._get_device_driver_list,
            'model_list': self._get_model_dict,
            'model_scan': self._mode_scan,
            'version': self._get_version,
        }
        self.parser.add_argument("--action_type",
                                 help=' '.join(self.handlers.keys()),
                                 required=True)
        self.parser.add_argument("--root_dir")
        self.parser.add_argument("--vendor_name")
        self.parser.add_argument("--module_name")
        self.parser.add_argument("--driver_name")
        self.parser.add_argument("--kernel_name")
        self.parser.add_argument("--board_name")
        self.parser.add_argument("--device_name")
        self.args = self.parser.parse_args(args)

    def _get_vendor_list_handler(self):
        self.check_arg_raise_if_not_exist("root_dir")
        root = self.args.root_dir
        vendor_root_dir = hdf_utils.get_vendor_root_dir(root)
        vendors = []
        if os.path.exists(vendor_root_dir):
            for vendor in os.listdir(vendor_root_dir):
                hdf = hdf_utils.get_vendor_hdf_dir(root, vendor)
                if os.path.exists(hdf):
                    vendors.append(vendor)
        return ','.join(vendors)

    def _get_current_vendor_handler(self):
        self.check_arg_raise_if_not_exist("root_dir")
        return HdfLiteMkFile(self.args.root_dir).get_current_vendor()

    @staticmethod
    def _get_board_list_handler():
        settings = HdfToolSettings()
        return settings.get_supported_boards()

    def _get_vendor_parent_path_handler(self):
        self.check_arg_raise_if_not_exist("root_dir")
        target = hdf_utils.get_vendor_root_dir(self.args.root_dir)
        return os.path.realpath(target)

    def _get_individual_vendor_path_handler(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.check_arg_raise_if_not_exist("vendor_name")
        root, vendor, _, _, _ = self.get_args()
        target = hdf_utils.get_vendor_dir(root, vendor)
        return os.path.realpath(target)

    @staticmethod
    def _get_version_handler():
        return hdf_tool_version.get_version()

    def _get_driver_list_handler(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.check_arg_raise_if_not_exist("vendor_name")
        root, vendor, _, _, _ = self.get_args()
        hdf_dir = hdf_utils.get_vendor_hdf_dir(root, vendor)
        if not os.path.exists(hdf_dir):
            raise HdfToolException('vendor "%s" not exist' % vendor,
                                   CommandErrorCode.TARGET_NOT_EXIST)
        modules = os.listdir(hdf_dir)
        vendor_k = HdfVendorKconfigFile(root, vendor, kernel=None, path='')
        module_items = vendor_k.get_module_and_config_path()
        drivers = {}
        for item in module_items:
            module, k_path = item
            if module in modules:
                models = \
                    HdfModuleKconfigFile(root, module,
                                         k_path).get_models()
                drivers[module] = models
        return json.dumps(drivers)

    def _get_driver_file_handler(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.check_arg_raise_if_not_exist("vendor_name")
        self.check_arg_raise_if_not_exist("module_name")
        self.check_arg_raise_if_not_exist("driver_name")
        root = os.path.realpath(self.args.root_dir)
        _, vendor, module, driver, _ = self.get_args()
        drv_dir = hdf_utils.get_drv_dir(root, vendor, module, driver)
        if not os.path.exists(drv_dir):
            raise HdfToolException(
                'driver directory: %s not exist' %
                drv_dir, CommandErrorCode.TARGET_NOT_EXIST)
        for root_path, dirs, files in os.walk(drv_dir):
            for file_name in files:
                if file_name.endswith('.c'):
                    return os.path.realpath(os.path.join(root_path, file_name))
        return ''

    def _get_drv_config_file_handler(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.check_arg_raise_if_not_exist("module_name")
        self.check_arg_raise_if_not_exist("driver_name")
        self.check_arg_raise_if_not_exist("board_name")
        root, _, module, driver, board = self.get_args()
        drv_config = HdfDriverConfigFile(root, board, module, driver, True)
        return drv_config.get_drv_config_path()

    def _get_model_dict(self):
        self.check_arg_raise_if_not_exist("root_dir")
        root, _, _, _, _, _, _ = self.get_args()
        create_model_file_save_path = hdf_utils.module_save_file_info(root)
        out_model_list = []
        data = hdf_utils.read_file(create_model_file_save_path)
        json_type = json.loads(data)
        if not json_type:
            return out_model_list
        file_key_list = list(list(json_type.items())[0][-1].keys())
        for k, _ in json_type.items():
            model_file_path = {}
            for key in file_key_list:
                if key.split("_")[-1] == "path":
                    path_dict = json_type[k][key]
                    model_file_path = hdf_utils.model_info(
                        path_dict, root, model_file_path, key)
            out_model_list.append({k: model_file_path})
        return json.dumps(out_model_list, indent=4)

    def _get_version(self):
        version_end = "\nCopyright (c) 2020-2021 Huawei Device Co., Ltd."
        version_head = "hdf_dev_eco_tool version : "
        return version_head + GetToolVersion().get_version() + version_end

    def _mode_scan(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.check_arg_raise_if_not_exist("vendor_name")
        self.check_arg_raise_if_not_exist("board_name")
        root, vendor, _, _, board, _, _ = self.get_args()
        if board.split("_")[-1] != "linux":
            return HdfLiteScan(
                root=root, vendor=vendor, board=board).get_model_scan()
        else:
            return HdfLinuxScan(
                root=root, vendor=vendor, board=board).get_model_scan()

    def _get_model_device_base(self):
        self.check_arg_raise_if_not_exist("module_name")
        model_device_file_path = ""
        resources_path = HdfToolSettings().get_resources_file_path()
        for file_name in os.listdir(resources_path):
            if file_name.endswith("ini"):
                model_device_file_path = os.path.join(resources_path, file_name)
                break
        return model_device_file_path

    def _get_model_device_list(self):
        model_device_file_path = self._get_model_device_base()
        if not model_device_file_path:
            raise HdfToolException(
                "%s: config file not exit" % CommandErrorCode.TARGET_NOT_EXIST)
        device_list, _ = hdf_utils.ini_file_read_operation(
            path=model_device_file_path, section_name=self.args.module_name,
            node_name="file_dir")
        return json.dumps(device_list)

    def delete_device_operation(self, device_name, *section):
        ini_config_handle, temp_value_type, temp_device_list = section
        device_path = ""
        if isinstance(temp_value_type, str):
            device_path = temp_value_type
        if isinstance(temp_value_type, dict):
            if self.args.board_name == "rk3568":
                device_path = temp_value_type.get('rk3568')
            else:
                device_path = temp_value_type.get('hi3516')
        temp_device_path = os.path.join(self.args.root_dir, device_path, device_name)
        if not os.path.exists(temp_device_path):
            temp_device_list.remove(device_name)
            hdf_utils.ini_file_write_operation(
                self.args.module_name, ini_config_handle, temp_device_list)

    def judgement_device_in_model(self, device_name):
        model_device_file_path = self._get_model_device_base()
        if not model_device_file_path:
            raise HdfToolException(
                "%s: config file not exit" % CommandErrorCode.TARGET_NOT_EXIST)
        device_list, ini_config_handle = hdf_utils.ini_file_read_operation(
            path=model_device_file_path, section_name=self.args.module_name,
            node_name="")
        section_info = hdf_utils.list_dict_tool(device_list)
        temp_value_type = literal_eval(section_info.get("driver_path"))
        temp_device_list = literal_eval(section_info.get("file_dir"))
        if device_name not in temp_device_list:
            raise HdfToolException(
                "%s device not in the %s module" % (device_name, self.args.module_name))
        return ini_config_handle, temp_value_type, temp_device_list

    def _get_crate_model_driver_info(self):
        self.check_arg_raise_if_not_exist("module_name")
        resources_path = HdfToolSettings().get_resources_file_path()
        config_setting_dict = HdfToolSettings().get_config_setting_info()
        temp_file_name = config_setting_dict["create_driver_file"]
        model_driver_file_path = os.path.join(resources_path, temp_file_name)
        hdf_utils.judge_file_path_exists(model_driver_file_path)
        model_driver_info = hdf_utils.read_file(model_driver_file_path)
        model_driver_json = json.loads(model_driver_info)
        return model_driver_json, model_driver_file_path

    def _get_model_driver_list(self):
        model_driver_json, _ = self._get_crate_model_driver_info()
        board_list = list(model_driver_json.keys())
        res_format_json = {}
        for board_name in board_list:
            board_info = model_driver_json.get(board_name)
            if board_info and board_info.get(self.args.module_name):
                res_format_json[board_name] = board_info.get(
                    self.args.module_name)
        self.format_model_driver_res(res_format_json)
        return json.dumps(res_format_json, indent=4)

    def _get_device_driver_list(self):
        temp_res = json.loads(self._get_model_driver_list())
        board_list = list(temp_res.keys())
        res_dict = {}
        for board_name in board_list:
            res_dict[board_name] = {
                "module_level_config": temp_res.get(
                    board_name).get("module_level_config"),
                "driver_file_list": {}
            }
            board_info = temp_res.get(board_name).get("driver_file_list")
            for driver_name, value_info in board_info.items():
                device_driver_name_list = driver_name.split("-")
                temp_device_name = device_driver_name_list[0]
                temp_driver_name = device_driver_name_list[-1]
                temp_device = res_dict.get(board_name).get(
                    "driver_file_list").get(temp_device_name)
                if temp_device:
                    temp_device[temp_driver_name] = value_info
                else:
                    res_dict.get(board_name).get("driver_file_list")[temp_device_name] = {
                        temp_driver_name: value_info
                    }
        return json.dumps(res_dict, indent=4)

    def judge_create_driver_exist(self):
        return self._get_device_driver_list()

    def format_model_driver_res(self, temp_driver_info):
        type_list_config = []
        for type_name in temp_driver_info.keys():
            type_list_config.append(temp_driver_info[type_name])
        for config_path in type_list_config:
            for key_name, value in config_path["module_level_config"].items():
                self.type_judge(value, key_name, config_path, parent_key="module_level_config")
            for key_name, value in config_path["driver_file_list"].items():
                self.type_judge(value, key_name, config_path, parent_key="driver_file_list")

    def type_judge(self, value, key_name, config_path, parent_key):
        if isinstance(value, list):
            config_path[parent_key][key_name] = \
                list(filter(lambda elem: os.path.exists(
                    os.path.join(self.args.root_dir, elem)), value))
            config_path[parent_key][key_name] = \
                list(map(lambda element: os.path.join(self.args.root_dir, element),
                         config_path[parent_key][key_name]))
        else:
            temp_path = os.path.join(self.args.root_dir, value)
            if os.path.exists(temp_path):
                config_path[parent_key][key_name] = temp_path
            else:
                config_path[parent_key][key_name] = ""

    def delete_module_driver_list(self):
        return self._get_model_driver_list()

    def format_delete_driver_file(self, model_name, board_name, driver_name):
        driver_json, driver_file_path = self._get_crate_model_driver_info()
        del driver_json[board_name][model_name]["driver_file_list"][driver_name]
        if not driver_json[board_name][model_name]["driver_file_list"]:
            del driver_json[board_name][model_name]
            if not driver_json[board_name]:
                del driver_json[board_name]
        hdf_utils.write_file(file_path=driver_file_path,
                             content=json.dumps(driver_json, indent=4))
