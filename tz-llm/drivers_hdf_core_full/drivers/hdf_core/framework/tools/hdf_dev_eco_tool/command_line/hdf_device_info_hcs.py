#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2020-2021 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.


import os
import re
from string import Template

import hdf_utils
from hdf_tool_exception import HdfToolException
from hdf_tool_settings import HdfToolSettings
from .hdf_command_error_code import CommandErrorCode


class HdfDeviceInfoHcsFile(object):
    def __init__(self, root, vendor, module, board, driver, path):
        self.model_space_num = " " * 8
        self.driver_space_num = " " * 12
        if not path:
            self.module = module
            self.vendor = vendor
            self.board = board
            self.root = root
            self.driver = driver
            self.lines = None
            board_hcs_temp = HdfToolSettings().get_board_parent_path(self.board)
            board_hcs_path = board_hcs_temp.format(vendor=vendor, board=board)
            self.hcspath = os.path.join(self.root, board_hcs_path, "device_info.hcs")
            self.data = {
                "driver_name": self.driver,
                "model_name": self.module,
                "module_upper_case": self.module.upper(),
                "driver_upper_case": self.driver.upper(),
                "module_name": "_".join([self.module, self.driver]).upper()
            }
        else:
            self.hcspath = path
            self.root = root
        self.file_path = hdf_utils.get_template_file_path(root)
        if not os.path.exists(self.file_path):
            raise HdfToolException(
                'template file: %s not exist' %
                self.file_path, CommandErrorCode.TARGET_NOT_EXIST)
        if not os.path.exists(self.hcspath):
            raise HdfToolException(
                'hcs file: %s not exist' %
                self.hcspath, CommandErrorCode.TARGET_NOT_EXIST)

    def _save(self):
        if self.lines:
            config_info = HdfToolSettings().get_file_config_info()
            write_fd = os.open(self.hcspath, config_info["flags"], config_info["modes"])
            with os.fdopen(write_fd, "w+", encoding="utf-8") as lwrite:
                for line in self.lines:
                    lwrite.write(line)

    def delete_driver(self, module, temp_flag="host"):
        hcs_config = hdf_utils.read_file_lines(self.hcspath)
        index_info = {}
        count = 0
        for index, line in enumerate(hcs_config):
            if line.find("%s :: %s" % (module, temp_flag)) == -1:
                continue
            index_info["start_index"] = index
            for child_index in range(
                    index_info.get("start_index"), len(hcs_config)):
                if hcs_config[child_index].strip().find("{") != -1:
                    count += 1
                elif hcs_config[child_index].strip() == "}":
                    count -= 1
                if count == 0:
                    index_info["end_index"] = child_index
                    break
            break
        if index_info:
            self.lines = hcs_config[0:index_info.get("start_index")] \
                         + hcs_config[index_info.get("end_index") + 1:]
            self._save()
            return True

    def add_model_hcs_file_config(self):
        template_file_path = os.path.join(
            self.file_path, 'device_info_hcs.template')
        return self.add_model_hcs_config_common(template_file_path)

    def add_model_hcs_file_config_user(self):
        template_file_path = os.path.join(
            self.file_path, 'User_device_info_hcs.template')
        return self.add_model_hcs_config_common(template_file_path)

    def add_model_hcs_config_common(self, template_file_path):
        temp_lines = list(map(
            lambda x: self.model_space_num + x,
            hdf_utils.read_file_lines(template_file_path)))

        old_lines = list(filter(
            lambda x: x != "\n",
            hdf_utils.read_file_lines(self.hcspath)))

        for index, _ in enumerate(temp_lines):
            temp_lines[index] = Template(
                temp_lines[index]).substitute(self.data)
        if temp_lines[0] not in old_lines:
            new_data = old_lines[:-2] + temp_lines + old_lines[-2:]
            self.lines = new_data
            self._save()
        return self.hcspath

    def add_hcs_config_to_exists_model(self, device):
        template_path = os.path.join(
            self.file_path, 'exists_model_hcs_info.template')
        lines = list(map(
            lambda x: self.driver_space_num + x,
            hdf_utils.read_file_lines(template_path)))

        old_lines = list(filter(
            lambda x: x != "\n",
            hdf_utils.read_file_lines(self.hcspath)))

        if self.judge_module_branch_exists(date_lines=old_lines):
            return self.hcspath
        end_index, start_index = self._get_model_index(old_lines)
        model_hcs_lines = old_lines[start_index:end_index]
        hcs_judge = self.judge_driver_hcs_exists(date_lines=model_hcs_lines)
        if hcs_judge:
            return self.hcspath
        temp_replace_dict = {
            "device_upper_case": device.upper()
        }
        self.data.update(temp_replace_dict)
        for index, _ in enumerate(lines):
            lines[index] = Template(lines[index]).substitute(self.data)
        self.lines = old_lines[:end_index] + lines + old_lines[end_index:]
        self._save()
        return self.hcspath

    def _get_model_index(self, old_lines):
        model_start_index = 0
        model_end_index = 0
        start_state = False
        count = 0
        for index, old_line in enumerate(old_lines):
            if old_line.strip().startswith(self.module) and start_state == False:
                model_start_index = index
                start_state = True
            if start_state and old_line.find("{") != -1:
                count += 1
            elif start_state and old_line.find("}") != -1:
                count -= 1
                if count != 0:
                    continue
                start_state = False
                model_end_index = index
        return model_end_index, model_start_index

    def judge_driver_hcs_exists(self, date_lines):
        for _, line in enumerate(date_lines):
            if line.startswith("#"):
                continue
            elif line.find(self.driver) != -1:
                return True
        return False

    def judge_module_branch_exists(self, date_lines):
        module_branch_start = "%s :: host" % self.module
        for _, line in enumerate(date_lines):
            if line.startswith("#"):
                continue
            elif line.strip().find(module_branch_start) != -1:
                return False
        return True

    @staticmethod
    def hdi_hcs_delete(file_path, driver_name):
        hcs_config = hdf_utils.read_file_lines(file_path)
        index_info = {}
        count = 0
        for index, line in enumerate(hcs_config):
            if line.find("%s_dal :: %s" % (driver_name, "host")) == -1:
                continue
            index_info["start_index"] = index
            for child_index in range(
                    index_info.get("start_index"), len(hcs_config)):
                if hcs_config[child_index].strip().find("{") != -1:
                    count += 1
                elif hcs_config[child_index].strip() == "}":
                    count -= 1
                if count == 0:
                    index_info["end_index"] = child_index
                    break
            break
        if index_info:
            temp_lines = hcs_config[0:index_info.get("start_index")] \
                         + hcs_config[index_info.get("end_index") + 1:]
            hdf_utils.write_file_lines(file_path, temp_lines)
