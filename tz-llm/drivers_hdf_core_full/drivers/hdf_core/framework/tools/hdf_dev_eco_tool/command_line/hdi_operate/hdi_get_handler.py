#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2023 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.


import json
import os.path

import hdf_tool_settings
import hdf_utils
from command_line.hdf_command_handler_base import HdfCommandHandlerBase


class HdiGetHandler(HdfCommandHandlerBase):
    def __init__(self, args):
        super(HdiGetHandler, self).__init__()
        self.cmd = "geti"
        self.handlers = {
            'interface': self._get_interface_handler,
            'peripheral': self._get_peripheral_handler,
            'unittest': self._get_create_unittest,
        }
        self.parser.add_argument("--action_type",
                                 help=' '.join(self.handlers.keys()),
                                 required=True)
        self.parser.add_argument("--root_dir", required=True)
        self.args = self.parser.parse_args(args)
        self.set_config = hdf_tool_settings.HdfToolSettings()
        self.root = None
        self.type = self.args.action_type
        self.result_temp = None
        self.hdi_info_config = None

    def _get_type_comm(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.root = self.args.root_dir
        hdf_setting = hdf_tool_settings.HdfToolSettings()
        config_hdi_dir_path = hdf_setting.get_file_path()
        config_dict = hdf_setting.get_config_setting_info()
        hdi_file = os.path.join(
            config_hdi_dir_path,
            config_dict.get("config_pre_dir"),
            config_dict.get("create_hdi_file"))
        self.hdi_info_config = hdf_utils.read_file(hdi_file)

    def _get_interface_handler(self):
        self._get_type_comm()
        self.result_temp = json.loads(self.hdi_info_config)[self.type]
        return self.format_create_result()

    def _get_peripheral_handler(self):
        self._get_type_comm()
        self.result_temp = json.loads(self.hdi_info_config)[self.type]
        return self.format_create_result()

    def _get_create_unittest(self):
        self._get_type_comm()
        self.result_temp = json.loads(self.hdi_info_config)[self.type]
        return self.format_create_result()

    def format_create_result(self):
        if self.result_temp is None:
            return ""
        format_result = {}
        for type_key, value in self.result_temp.items():
            temp = {
                "create_file": list(
                    map(
                        lambda val: os.path.join(self.root, val),
                        value["create_file"])),
                "config": list(
                    map(
                        lambda val: os.path.join(self.root, val),
                        value["config"]))
            }
            format_result[type_key] = temp
        return json.dumps(format_result, indent=4)

    def get_type_create_info(self, type_name):
        return self.handlers.get(type_name)
