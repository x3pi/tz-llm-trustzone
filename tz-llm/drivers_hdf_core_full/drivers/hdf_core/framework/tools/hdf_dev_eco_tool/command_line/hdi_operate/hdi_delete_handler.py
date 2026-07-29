#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2023 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.


import json
import os
import re
import shutil
import string

import hdf_tool_settings
import hdf_utils
from command_line.hdf_command_error_code import CommandErrorCode
from command_line.hdf_command_handler_base import HdfCommandHandlerBase
from command_line.hdf_device_info_hcs import HdfDeviceInfoHcsFile
from command_line.hdi_operate.hdi_get_handler import HdiGetHandler
from command_line.operate_group_passwd import OperateGroupPasswd
from hdf_tool_exception import HdfToolException


class HdiDeleteHandler(HdfCommandHandlerBase):
    def __init__(self, args):
        super(HdiDeleteHandler, self).__init__()
        self.cmd = 'deletei'
        self.parser.add_argument("--action_type",
                                 help=' '.join(self.handlers.keys()),
                                 required=True)
        self.parser.add_argument("--root_dir", required=True)
        self.parser.add_argument("--vendor_name")
        self.parser.add_argument("--board_name")
        self.parser.add_argument("--driver_name")
        self.args_original = args
        self.root = None
        self.type = None
        self.vendor = None
        self.board = None
        self.driver = None
        self.args = self.parser.parse_args(args)
        self.hdi_get = self._call_hdi_get()
        self.config_info = None
        self.create_info = None

    def run(self):
        self.root = self.args.root_dir
        self.operate_delete()

    def _call_hdi_get(self):
        get_argument = ["--action_type", "--root_dir"]
        get_args = []
        for argument_name in get_argument:
            index_num = self.args_original.index(argument_name)
            get_args.append(self.args_original[index_num])
            get_args.append(self.args_original[index_num + 1])
        return HdiGetHandler(get_args)

    def _del_comm_handler(self, type_name):
        return self.hdi_get.get_type_create_info(type_name)()

    def operate_delete(self):
        self.type = self.args.action_type
        self.driver = self.args.driver_name
        if self.type == "interface":
            temp_json = self._del_comm_handler("interface")
        elif self.type == "peripheral":
            temp_json = self._del_comm_handler("peripheral")
        elif self.type == "unittest":
            temp_json = self._del_comm_handler("unittest")
        else:
            temp_json = {}
        json_format = json.loads(temp_json)
        res_json = json_format.get(self.driver)
        if res_json is None:
            raise HdfToolException(
                "this %s driver name is not exists" % self.driver,
                CommandErrorCode.TARGET_NOT_EXIST)
        self.config_info = res_json.get("config")
        self.create_info = res_json.get("create_file")
        self.operate_create_file()
        self.operate_config_file()
        self.modify_idl_config()
        del json_format[self.driver]
        print(json.dumps(json_format, indent=4))

    def modify_idl_config(self):
        hdf_setting = hdf_tool_settings.HdfToolSettings()
        config_hdi_dir_path = hdf_setting.get_file_path()
        config_dict = hdf_setting.get_config_setting_info()
        hdi_config_path = os.path.join(
            config_hdi_dir_path,
            config_dict.get("config_pre_dir"),
            config_dict.get("create_hdi_file"))
        hdi_config_info = hdf_utils.read_file(hdi_config_path)
        json_hdi_info = json.loads(hdi_config_info)
        del json_hdi_info[self.type][self.driver]
        hdf_utils.write_file(hdi_config_path, json.dumps(json_hdi_info, indent=4))

    def operate_create_file(self):
        re_dir = r".*%s/" % self.driver
        pre_handler = re.search(re_dir, self.create_info[0])
        for file_path in self.create_info:
            if os.path.exists(file_path):
                os.remove(file_path)
        if pre_handler is None:
            return
        pre_handler = pre_handler.group()
        pre_handler_file_list = os.listdir(pre_handler)
        state_flag = False
        if len(pre_handler_file_list) == 1:
            dir_path = os.path.join(pre_handler, pre_handler_file_list[0])
            while True and os.path.isdir(dir_path):
                temp_list = os.listdir(dir_path)
                if len(temp_list) == 1:
                    dir_path = os.path.join(dir_path, temp_list[0])
                elif len(temp_list) == 0:
                    state_flag = True
                    break
                else:
                    break
        if state_flag:
            shutil.rmtree(pre_handler)

    def operate_config_file(self):
        if self.type == "interface":
            for file_path in self.config_info:
                self._delete_interface_config_file(file_path)
        elif self.type == "peripheral":
            for file_path in self.config_info:
                self._delete_peripheral_config_file(file_path)
        elif self.type == "unittest":
            for file_path in self.config_info:
                self._delete_unittest_config_file(file_path)

    def _delete_peripheral_config_file(self, config_json_path):
        hdi_config = hdf_tool_settings.HdiToolConfig()
        if config_json_path.endswith(".hcs"):
            HdfDeviceInfoHcsFile.hdi_hcs_delete(config_json_path, self.driver)
        elif config_json_path.endswith(".te"):
            if config_json_path.endswith("type.te"):
                _, selinux_temp = hdi_config.get_hdi_selinux_type()
            elif config_json_path.endswith("hdf_service.te"):
                _, selinux_temp = hdi_config.get_hdi_selinux_hdf_service()
            elif config_json_path.endswith("hdf_host.te"):
                _, selinux_temp = hdi_config.get_hdi_selinux_hdf_host()
            self._selinux_config_file_delete(selinux_temp, config_json_path)
        elif config_json_path.endswith("group") \
                or config_json_path.endswith("passwd"):
            hdi_config = hdf_tool_settings.HdfToolSettings()
            group_passwd = OperateGroupPasswd(tool_settings=hdi_config, root_path=self.root)
            group_passwd.delete_group_passwd(name=self.args.driver_name, file_path=config_json_path)
        else:
            _, selinux_temp = hdi_config.get_hdi_selinux_hdf_service_contexts()
            self._selinux_config_file_delete(selinux_temp, config_json_path, align=True)

    def _selinux_config_file_delete(self, selinux_temp, config_path, align=False):
        if isinstance(selinux_temp["info_temp"], str):
            if align:
                temp_new_line = string.Template(
                    selinux_temp["info_temp"]).safe_substitute(
                    {"peripheral_name": self.driver})
                temp_line = temp_new_line.split(" ")
                space_num = selinux_temp["space_len"] - len(temp_line[0])
                new_line = (" " * space_num).join(temp_line)
            else:
                new_line = string.Template(
                    selinux_temp["info_temp"]).safe_substitute(
                    {"peripheral_name": self.driver})
            hdf_service_contexts_lines = hdf_utils.read_file_lines(config_path)
            hdf_service_contexts_lines.remove(new_line)
        elif isinstance(selinux_temp["info_temp"], list):
            hdf_service_contexts_lines = hdf_utils.read_file_lines(config_path)
            temp_re = r"pid=\d+ scontext=u:r:{driver_name}_host:s0".format(driver_name=self.driver)
            for line in hdf_service_contexts_lines:
                temp_res = re.search(temp_re, line)
                if temp_res is not None:
                    pid_num = re.search(r"\d+", temp_res.group()).group()
                    break
            temp_replace_list = []
            for temp_line in selinux_temp["info_temp"]:
                temp_replace_list.append(
                    string.Template(temp_line).safe_substitute({
                        "pid_num": pid_num,
                        "peripheral_name": self.driver
                    }))
            [hdf_service_contexts_lines.remove(temp) for temp in temp_replace_list]
        hdf_utils.write_file_lines(config_path, hdf_service_contexts_lines)

    def _delete_interface_config_file(self, config_json_path):
        file_config = hdf_utils.read_file(config_json_path)
        file_json_type = json.loads(file_config)
        for info in file_json_type['subsystems']:
            if info.get("subsystem") == "hdf":
                key_list_name = []
                for key_name in info.get("components"):
                    key_list_name.append(key_name.get('component'))
                component_name = 'drivers_interface_%s' % self.driver
                if component_name in key_list_name:
                    interface_dict = {
                        'component': component_name,
                        'features': []}
                    info.get("components").remove(interface_dict)
        hdf_utils.write_file(
            config_json_path, json.dumps(file_json_type, indent=4))

    def _delete_unittest_config_file(self, config_json_path):
        file_info_str = hdf_utils.read_file(config_json_path)
        file_info_json = json.loads(file_info_str)
        pre_dict = file_info_json['component']["build"]
        temp_str = pre_dict[list(pre_dict.keys())[0]][0].split(":")[0]
        result_test_str = "/".join([temp_str, "test:{peripheral_name}_unittest".
                                   format(peripheral_name=self.driver)])
        if result_test_str in pre_dict[list(pre_dict.keys())[1]]:
            pre_dict[list(pre_dict.keys())[1]].remove(result_test_str)
        hdf_utils.write_file(
            config_json_path, content=json.dumps(file_info_json, indent=4))
