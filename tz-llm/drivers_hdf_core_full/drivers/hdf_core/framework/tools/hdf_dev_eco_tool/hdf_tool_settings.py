#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2020-2021 Huawei Device Co., Ltd.
# 
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.


import os
import json

from hdf_tool_exception import HdfToolException
from command_line.hdf_command_error_code import CommandErrorCode


def singleton(clazz):
    _instances = {}

    def create_instance():
        if clazz not in _instances:
            _instances[clazz] = clazz()
        return _instances.get(clazz)
    return create_instance


def get_hdf_tool_resources_path():
    cur_dir = os.path.realpath(os.path.dirname(__file__))
    return os.path.join(cur_dir, 'resources')


@singleton
class HdfToolSettings(object):
    def __init__(self):
        self.file_path = os.path.join(get_hdf_tool_resources_path(), 'settings.json')
        self.settings = {}
        if not os.path.exists(self.file_path):
            return
        with open(self.file_path) as file_write:
            try:
                self.settings = json.load(file_write)
            except ValueError as exc:
                raise HdfToolException('file: %s format wrong, %s' %
                                       (self.file_path, str(exc)),
                                       CommandErrorCode.FILE_FORMAT_WRONG)
            finally:
                pass
        self.supported_boards_key = 'supported_boards'
        self.drivers_path_key_framework = 'drivers_path_relative_framework'
        self.drivers_path_key_peripheral = 'drivers_path_relative_peripheral'
        self.drivers_path_key_interface = 'drivers_path_relative_interface'
        self.drivers_adapter_path_key = 'drivers_path_relative_adapter'
        self.user_adapter_path_key = 'user_model_path_relative_adapter'
        self.module_save_path_key = "module_save_path"
        self.dot_configs_key = 'dot_configs'
        self.board_path_key = 'board_parent_path'
        self.dot_config_path_key = 'dot_config_path'
        self.template_path_key = 'template_file_path'
        self.hdi_config_key = "hdi_config"
        self.passwd_group_key = "passwd_group_config"
        self.config_setting_info = "config_setting_file_info"
        self.create_driver_supported_type = "create_driver_board_type"
        self.create_file_config_info = "create_file_config"

    def get_supported_boards(self):
        key = self.supported_boards_key
        if key in self.settings:
            return ','.join(self.settings.get(key).keys())
        return ''

    def get_board_parent_path(self, board_name):
        key = self.supported_boards_key
        board_entry = {}
        if key in self.settings:
            if board_name in self.settings.get(key):
                board_entry = self.settings.get(key).get(board_name)
        key = self.board_path_key
        return board_entry.get(key, '')

    def get_drivers_path_framework(self):
        key = self.drivers_path_key_framework
        return self.settings.get(key, 'hdf')

    def get_drivers_path_peripheral(self):
        key = self.drivers_path_key_peripheral
        return self.settings.get(key, 'hdf')

    def get_drivers_path_interface(self):
        key = self.drivers_path_key_interface
        return self.settings.get(key, 'hdf')

    def get_drivers_path_adapter(self):
        key = self.drivers_adapter_path_key
        return self.settings.get(key, 'hdf')

    def get_template_path(self):
        key = self.template_path_key
        return self.settings.get(key, 'hdf')

    def get_dot_configs(self, board_name):
        key = self.supported_boards_key
        boards = self.settings.get(key, None)
        if not boards:
            return []
        board = boards.get(board_name, None)
        if not board:
            return []
        dot_config_path = board.get(self.dot_config_path_key, '')
        if not dot_config_path:
            return []
        configs = board.get(self.dot_configs_key, [])
        return [os.path.join(dot_config_path, config) for config in configs]

    def get_board_parent_file(self, board_name):
        key = self.supported_boards_key
        if key in self.settings:
            if board_name in self.settings.get(key):
                return self.settings.get(key).get(board_name).get("patch_and_config")
        return ''

    def get_board_list(self):
        key = self.supported_boards_key
        return list(self.settings.get(key).keys())

    def get_create_board_type(self):
        key = self.create_driver_supported_type
        return list(self.settings.get(key))

    def get_user_adapter_path(self):
        key = self.user_adapter_path_key
        return self.settings.get(key, 'hdf')

    def get_hdi_config(self):
        key = self.hdi_config_key
        return self.settings.get(key, 'hdf')

    def get_hdi_file_path(self):
        cur_dir = os.path.realpath(os.path.dirname(__file__))
        return os.path.join(cur_dir, 'resources')

    def get_module_save_path(self):
        key = self.module_save_path_key
        return self.settings.get(key, 'hdf')

    def get_resources_file_path(self):
        return get_hdf_tool_resources_path()

    def get_passwd_group_config(self):
        key = self.passwd_group_key
        return self.settings.get(key, 'hdf')

    def get_config_setting_info(self):
        key = self.config_setting_info
        return self.settings.get(key, 'hdf')

    def get_file_config_info(self):
        key = self.create_file_config_info
        return self.settings.get(key, 'hdf')


@singleton
class HdiToolConfig(object):
    def __init__(self):
        hdf_tool = HdfToolSettings()
        hdi_config_path = hdf_tool.get_hdi_config()["config_path"]
        cur_dir = os.path.realpath(os.path.dirname(__file__))
        self.hdi_file_path = os.path.join(cur_dir, hdi_config_path)
        if not os.path.exists(self.hdi_file_path):
            raise HdfToolException('file: %s file not exist!' % self.hdi_file_path,
                                   CommandErrorCode.TARGET_NOT_EXIST)
        with open(self.hdi_file_path, "r") as hdi_file_read:
            try:
                self.hdi_settings = json.load(hdi_file_read)
            except ValueError as exc:
                raise HdfToolException('file: %s format wrong, %s' %
                                       (self.file_path, str(exc)),
                                       CommandErrorCode.FILE_FORMAT_WRONG)
        self.passwd_key = 'passwd'
        self.group_key = 'group'
        self.selinux_key = 'selinux'
        self.selinux_type_key = 'type.te'
        self.selinux_hdf_service_key = 'hdf_service.te'
        self.selinux_hdf_service_contexts_key = 'hdf_service_contexts'
        self.selinux_hdf_host_key = 'hdf_host.te'
        self.selinux_peripheral_key = "peripheral_config"

    def get_hdi_passwd(self):
        key = self.passwd_key
        return self.hdi_settings.get(key, 'hdi')

    def get_hdi_group(self):
        key = self.group_key
        return self.hdi_settings.get(key, 'hdi')

    def _get_hdi_selinux(self):
        key = self.selinux_key
        return self.hdi_settings.get(key, 'hdi')

    def get_hdi_selinux_type(self):
        key = self.selinux_type_key
        selinux_config_info = self._get_hdi_selinux()
        parent_path = selinux_config_info.get("pre_path")
        return parent_path, selinux_config_info.get(key, 'hdi')

    def get_hdi_selinux_hdf_service(self):
        key = self.selinux_hdf_service_key
        selinux_config_info = self._get_hdi_selinux()
        parent_path = selinux_config_info.get("pre_path")
        return parent_path, selinux_config_info.get(key, 'hdi')

    def get_hdi_selinux_hdf_service_contexts(self):
        key = self.selinux_hdf_service_contexts_key
        selinux_config_info = self._get_hdi_selinux()
        parent_path = selinux_config_info.get("pre_path")
        return parent_path, selinux_config_info.get(key, 'hdi')

    def get_hdi_selinux_hdf_host(self):
        key = self.selinux_hdf_host_key
        selinux_config_info = self._get_hdi_selinux()
        parent_path = selinux_config_info.get("pre_path")
        return parent_path, selinux_config_info.get(key, 'hdi')

    def get_selinux_peripheral_hdf_host(self):
        key = self.selinux_peripheral_key
        selinux_config_info = self._get_hdi_selinux()
        parent_path = selinux_config_info.get("pre_path")
        return parent_path, selinux_config_info.get(key, 'hdi')
