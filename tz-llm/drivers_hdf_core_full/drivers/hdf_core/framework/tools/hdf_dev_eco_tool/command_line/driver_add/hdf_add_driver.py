#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2022 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.


import configparser
import os
import re
from string import Template

import hdf_utils
from hdf_tool_exception import HdfToolException
from .linux.kconfig_file_add_config import kconfig_file_operation
from .linux.mk_file_add_config import audio_linux_makefile_operation, linux_makefile_operation

from .liteos.gn_file_add_config import audio_build_file_operation, build_file_operation, input_build_file_operation
from .liteos.mk_file_add_config import audio_makefile_file_operation, makefile_file_operation
from ..hdf_command_error_code import CommandErrorCode
from ..hdf_defconfig_patch import HdfDefconfigAndPatch
from ..hdf_device_info_hcs import HdfDeviceInfoHcsFile


class HdfAddDriver(object):
    def __init__(self, args):
        super(HdfAddDriver, self).__init__()
        self.root, self.vendor, self.module, self.driver, \
        self.board, self.kernel, self.device = args
        self.template_file_path = hdf_utils.get_template_file_path(self.root)
        if not os.path.exists(self.template_file_path):
            raise HdfToolException(
                'template file: %s not exist' %
                self.template_file_path, CommandErrorCode.TARGET_NOT_EXIST)

    def add_linux(self, driver_file_path, driver_head_path):
        config_path_result = self.get_kernel_linux_config_path(
            driver_file_path, driver_head_path)
        config_path, file_path, defconfig_patch = config_path_result
        pat_files = []
        patch_list = defconfig_patch.add_module(
            config_path, files=pat_files, codetype=None)
        config_path = defconfig_patch.get_config_patch()
        def_files = []
        defconfig_list = defconfig_patch.add_module(
            config_path, files=def_files, codetype=None)
        file_path[self.module + "_dot_configs"] = \
            list(set(patch_list + defconfig_list))
        return file_path

    def add_liteos(self, driver_file_path, driver_head_path):
        adapter_hdf = hdf_utils.get_vendor_hdf_dir_adapter(
            self.root, self.kernel)
        hdf_utils.judge_file_path_exists(adapter_hdf)

        adapter_model_path = os.path.join(adapter_hdf, 'model', self.module)
        hdf_utils.judge_file_path_exists(adapter_model_path)

        liteos_file_name = ['BUILD.gn', 'Makefile', 'Kconfig']
        file_path = {}
        for file_name in liteos_file_name:
            file_path = self._liteos_config_file_configure(
                file_path, file_name, adapter_model_path,
                driver_file_path, driver_head_path)
        # Modify hcs file
        device_info = HdfDeviceInfoHcsFile(
            self.root, self.vendor, self.module,
            self.board, self.driver, path="")
        hcs_file_path = device_info.add_hcs_config_to_exists_model(self.device)
        file_path["devices_info.hcs"] = hcs_file_path

        dot_file_list = hdf_utils.get_dot_configs_path(
            self.root, self.vendor, self.board)
        template_string = "LOSCFG_DRIVERS_HDF_${module_upper}_${driver_upper}=y\n"
        new_demo_config = Template(template_string).substitute(
            {"module_upper": self.module.upper(),
             "driver_upper": self.driver.upper()})

        device_enable = self.__get_enable_config()
        for dot_file in dot_file_list:
            file_lines_old = hdf_utils.read_file_lines(dot_file)
            if device_enable:
                file_lines = list(
                    filter(
                        lambda x: hdf_utils.judge_enable_line(
                            enable_line=x,
                            device_base=device_enable.split("=")[0]),
                        file_lines_old))
                if device_enable not in file_lines:
                    file_lines.append(device_enable)
            else:
                file_lines = file_lines_old
            file_lines[-1] = "{}\n".format(file_lines[-1].strip())
            if new_demo_config not in file_lines:
                file_lines.append(new_demo_config)
            hdf_utils.write_file_lines(dot_file, file_lines)
        file_path[self.module + "_dot_configs"] = dot_file_list
        return file_path

    def _liteos_config_file_configure(self, file_path, file_name, adapter_model_path,
                                      driver_file_path, driver_head_path):
        if file_name == "BUILD.gn":
            build_file_path = os.path.join(adapter_model_path, file_name)
            if self.module == "audio":
                args_tuple = (driver_file_path, driver_head_path, self.module,
                              self.driver, self.root, self.device, self.kernel)
                audio_build_file_operation(build_file_path, args_tuple)
            elif self.module == "input":
                input_build_file_operation(
                    build_file_path, driver_file_path[0], driver_head_path[0],
                    self.module, self.driver)
            else:
                build_file_operation(
                    build_file_path, driver_file_path[0], driver_head_path[0],
                    self.module, self.driver)
            file_path['BUILD.gn'] = build_file_path

        elif file_name == "Makefile":
            makefile_path = os.path.join(adapter_model_path, file_name)
            if self.module == "audio":
                args_tuple = (driver_file_path, driver_head_path, self.module,
                              self.driver, self.root, self.device, self.kernel)
                audio_makefile_file_operation(makefile_path, args_tuple)
            else:
                makefile_file_operation(
                    makefile_path, driver_file_path[0], driver_head_path[0],
                    self.module, self.driver, self.root)
            file_path['Makefile'] = makefile_path

        elif file_name == "Kconfig":
            kconfig_path = os.path.join(adapter_model_path, file_name)
            kconfig_file_operation(kconfig_path, self.module,
                                   self.driver, self.template_file_path)
            file_path['Kconfig'] = kconfig_path
        return file_path

    def add_kernel(self, driver_file_path, driver_head_path):
        config_path_result = self.get_kernel_linux_config_path(
            driver_file_path, driver_head_path)
        config_path, file_path, defconfig_patch = config_path_result
        files = []
        patch_list = defconfig_patch.add_module(
            config_path, files=files, codetype=None)
        config_path = defconfig_patch.get_config_patch()
        files1 = []
        defconfig_list = defconfig_patch.add_module(
            config_path, files=files1, codetype=None)
        file_path[self.module + "_dot_configs"] = \
            list(set(patch_list + defconfig_list))
        return file_path

    def get_kernel_linux_config_path(self, driver_file_path, driver_head_path):
        file_path = {}
        file_path.update(self.linux_operation_config(driver_file_path, driver_head_path))

        device_info = HdfDeviceInfoHcsFile(self.root, self.vendor,
                                           self.module, self.board,
                                           self.driver, path="")
        hcs_file_path = device_info.add_hcs_config_to_exists_model(self.device)
        file_path["devices_info.hcs"] = hcs_file_path
        device_enable_config_line = self.__get_enable_config()
        template_string = "CONFIG_DRIVERS_HDF_${module_upper}_${driver_upper}=y\n"
        data_model = {
            "module_upper": self.module.upper(),
            "driver_upper": self.driver.upper()
        }

        new_demo_config = Template(template_string).substitute(data_model)
        if device_enable_config_line:
            new_demo_config_list = [device_enable_config_line, new_demo_config]
        else:
            new_demo_config_list = [new_demo_config]
        defconfig_patch = HdfDefconfigAndPatch(
            self.root, self.vendor, self.kernel, self.board,
            data_model, new_demo_config_list)

        config_path = defconfig_patch.get_config_config()
        return config_path, file_path, defconfig_patch

    def driver_create_info_format(self, config_file_json,
                                  config_item, file_path):
        board_type = config_file_json.get(self.board)
        if board_type is None:
            config_file_json[self.board] = {
                config_item.get("module_name"): {
                    'module_level_config': {},
                    "driver_file_list": {
                        config_item.get("driver_name"):
                            config_item.get("driver_file_path") +
                            config_item.get("head_file_path")
                    }
                }
            }
            config_file_json[self.board][self.module]["module_level_config"]\
                .update(file_path)
        else:
            model_type = board_type.get(config_item.get("module_name"))
            if model_type is None:
                temp = config_file_json.get(self.board)
                temp_module = config_item.get("module_name")
                temp[temp_module] = {
                    'module_level_config': {},
                    "driver_file_list": {
                        config_item.get("driver_name"):
                            config_item.get("driver_file_path") +
                            config_item.get("head_file_path")
                    }
                }
                config_file_json.get(self.board).get(self.module).\
                    get("module_level_config").update(file_path)
            else:
                temp = config_file_json.get(self.board).\
                    get(config_item.get("module_name")).get("driver_file_list")
                temp[config_item.get("driver_name")] = \
                    config_item.get("driver_file_path") + \
                    config_item.get("head_file_path")
        return config_file_json

    def add_driver(self, *args_tuple):
        root, vendor, module, driver, board, kernel, device = args_tuple
        drv_converter = hdf_utils.WordsConverter(driver)
        dev_converter = hdf_utils.WordsConverter(device)
        # create driver file path
        source_file, head_path = self.create_model_file_name(
            root, vendor, module, driver, board, kernel, device)
        data_model = {
            'driver_lower_case': drv_converter.lower_case(),
            'driver_upper_camel_case': drv_converter.upper_camel_case(),
            'driver_lower_camel_case': drv_converter.lower_camel_case(),
            'driver_upper_case': drv_converter.upper_case(),
            'device_lower_case': dev_converter.lower_case(),
            'device_upper_camel_case': dev_converter.upper_camel_case(),
            'device_lower_camel_case': dev_converter.lower_camel_case(),
            'device_upper_case': dev_converter.upper_case()
        }
        templates_list, target_path = self.get_model_template_list(module, board)
        source_file_list, source_statu_exist = self.get_template_source_file(
            source_file, data_model, templates_list, target_path, args_tuple)
        head_path_list, head_statu_exist = self.get_template_head_file(
            head_path, data_model, templates_list, target_path, args_tuple)
        if head_statu_exist and source_statu_exist:
            return True, source_file_list, head_path_list
        child_dir_list, operation_object = hdf_utils.ini_file_read_operation(
            section_name=module, node_name='file_dir')
        if device not in child_dir_list:
            child_dir_list.append(device)
            hdf_utils.ini_file_write_operation(
                module, operation_object, child_dir_list)
        return True, source_file_list, head_path_list

    def get_template_source_file(self, source_file, data_model,
                                 templates_list, target_path, *args_tuple):
        # find model template .c
        source_statu_exist = False
        root, vendor, module, driver, board, kernel, device = args_tuple[0]
        if module == "audio" or module == "display":
            if board.startswith("rk3568"):
                source_file_template_list = \
                    list(filter(
                        lambda file_name: "source" in file_name and
                                          file_name.startswith("rk"),
                        templates_list))
            else:
                source_file_template_list = \
                    list(filter(
                        lambda file_name: "source" in file_name and
                                          file_name.startswith("hi"),
                        templates_list))
        elif module == "sensor":
            source_file_template_list = self._sensor_source_head("source", templates_list)
        else:
            source_file_template_list = \
                list(filter(
                    lambda file_name: "source" in file_name,
                    templates_list))
        source_file_template = \
            list(map(
                lambda template_name: os.path.join(target_path, template_name),
                source_file_template_list))
        path_list = list(os.path.split(source_file))
        temp_path = os.path.sep.join(path_list[:-1])
        if not os.path.exists(temp_path):
            os.makedirs(temp_path)

        source_file_list = []
        for source_file_temp in source_file_template:
            if not os.path.exists(source_file):
                os.makedirs(source_file)
            create_name = re.search(r'[a-z]+_source', source_file_temp).group()
            create_source_name = "%s_%s_%s.c" % (device, driver, create_name.split("_")[0])
            data_model.update({'include_file': "%s_%s_%s.h" % (device, driver, create_name.split("_")[0])})
            source_file_name = os.path.join(source_file, create_source_name)
            if os.path.exists(source_file_name):
                source_statu_exist = True
                source_file_list.append(source_file_name)
            else:
                self._template_fill(source_file_temp, source_file_name, data_model)
                source_file_list.append(source_file_name)
        return source_file_list, source_statu_exist

    def get_template_head_file(
            self, head_path, data_model, templates_list, target_path, *args_tuple):
        # find model template .h
        head_statu_exist = False
        root, vendor, module, driver, board, kernel, device = args_tuple[0]
        if module == "audio":
            if board.startswith("rk3568"):
                head_file_template_list = list(filter(
                    lambda file_name:
                    ("head" in file_name and file_name.startswith("rk")),
                    templates_list))
            else:
                head_file_template_list = list(filter(
                    lambda file_name:
                    ("head" in file_name and file_name.startswith("hi")),
                    templates_list))
        elif module == "sensor":
            head_file_template_list = self._sensor_source_head("head", templates_list)
        else:
            head_file_template_list = list(filter(
                lambda file_name: "head" in file_name, templates_list))
        head_file_template = list(map(
            lambda template_name: os.path.join(target_path, template_name),
            head_file_template_list))
        path_list = list(os.path.split(head_path))
        temp_path = os.path.sep.join(path_list[:-1])
        if not os.path.exists(temp_path):
            os.makedirs(temp_path)
        head_path_list = []
        for head_file_temp in head_file_template:
            if not os.path.exists(head_path):
                os.makedirs(head_path)
            create_name = re.search(r'[a-z]+_head', head_file_temp).group()
            create_head_name = "%s_%s_%s.h" % (device, driver,
                                               create_name.split("_")[0])
            head_file_name = os.path.join(head_path, create_head_name)
            if os.path.exists(head_file_name):
                head_statu_exist = True
                head_path_list.append(head_file_name)
            else:
                self._template_fill(head_file_temp, head_file_name, data_model)
                head_path_list.append(head_file_name)
        return head_path_list, head_statu_exist

    def _sensor_source_head(self, file_type, templates_list):
        kernel_base_device = f"{self.kernel}_base_device"
        base_device_list, _ = hdf_utils.ini_file_read_operation(
            section_name=self.module, node_name=kernel_base_device)
        if file_type == "head":
            if self.device == "als" or self.device == "hall" or self.device == "ppg":
                head_file_template_list = \
                    list(filter(
                        lambda file_name: "head" in file_name and
                                          file_name.startswith(self.device),
                        templates_list))
            elif self.device in base_device_list:
                head_file_template_list = \
                    list(filter(
                        lambda file_name: "head" in file_name and
                                          file_name.startswith("accel"),
                        templates_list))
            else:
                head_file_template_list = \
                    list(filter(
                        lambda file_name: "head" in file_name and
                                          file_name.startswith("com"),
                        templates_list))
            return head_file_template_list
        else:
            if self.device == "als" or self.device == "hall" or self.device == "ppg":
                source_file_template_list = \
                    list(filter(
                        lambda file_name: "source" in file_name and
                                          file_name.startswith(self.device),
                        templates_list))
            elif self.device in base_device_list:
                source_file_template_list = \
                    list(filter(
                        lambda file_name: "source" in file_name and
                                          file_name.startswith("accel"),
                        templates_list))
            else:
                source_file_template_list = \
                    list(filter(
                        lambda file_name: "source" in file_name and
                                          file_name.startswith("com"),
                        templates_list))
            return source_file_template_list

    def _file_gen_lite(self, template, source_file_path, model):
        templates_dir = hdf_utils.get_templates_lite_dir()
        template_path = os.path.join(templates_dir, template)
        self._template_fill(template_path, source_file_path, model)

    def _template_fill(self, template_path, output_path, data_model):
        if not os.path.exists(template_path):
            return
        raw_content = hdf_utils.read_file(template_path)
        contents = Template(raw_content).safe_substitute(data_model)
        hdf_utils.write_file(output_path, contents)

    def create_model_file_name(self, *args_tuple):
        root, vendor, module, driver, board, kernel, device = args_tuple
        drv_src_dir = hdf_utils.get_drv_src_dir(root, module)
        if device.strip():
            if module == "sensor":
                relatively_path, _ = hdf_utils.ini_file_read_operation(
                    section_name=module, node_name='driver_path')
                new_mkdir_path = os.path.join(root, relatively_path, device)
            elif module == "audio":
                relatively_path_dict, _ = hdf_utils.ini_file_read_operation(
                    section_name=module, node_name='driver_path')
                if board.startswith("rk3568"):
                    relatively_path = relatively_path_dict["rk3568"]
                else:
                    relatively_path = relatively_path_dict["hi3516"]
                new_mkdir_path = os.path.join(
                    root, relatively_path, device)
            else:
                new_mkdir_path = os.path.join(drv_src_dir, device)

            if not os.path.exists(new_mkdir_path):
                os.mkdir(new_mkdir_path)
            if module == "sensor":
                result_path_source = new_mkdir_path
                result_path_head = new_mkdir_path
            else:
                result_path_source = os.path.join(new_mkdir_path, 'src')
                result_path_head = os.path.join(new_mkdir_path, 'include')
        else:
            if module == "sensor":
                new_mkdir_path = os.path.join(drv_src_dir, 'chipset', driver)
            else:
                new_mkdir_path = os.path.join(drv_src_dir, driver)
            if not os.path.exists(new_mkdir_path):
                os.mkdir(new_mkdir_path)
            result_path_source = os.path.join(
                new_mkdir_path, '%s_driver.c' % driver)
            result_path_head = os.path.join(
                new_mkdir_path, '%s_driver.h' % driver)
        return result_path_source, result_path_head

    def __get_enable_config(self):
        device_enable_config = None
        templates_dir = hdf_utils.get_templates_lite_dir()
        templates_model_dir = []
        for path, dir_name, _ in os.walk(templates_dir):
            if dir_name:
                templates_model_dir.extend(dir_name)
        templates_model_dir = list(
            filter(
                lambda model_dir: self.module in model_dir,
                templates_model_dir))
        config_file = [
            name for name in os.listdir(
                os.path.join(
                    templates_dir,
                    templates_model_dir[0])) if name.endswith("ini")]
        if config_file:
            config_path = os.path.join(
                templates_dir,
                templates_model_dir[0],
                config_file[0])
            config = configparser.ConfigParser()
            config.read(config_path)
            section_list = config.options(section=self.kernel)
            if self.device in section_list:
                device_enable_config, _ = hdf_utils.ini_file_read_operation(
                    section_name=self.kernel,
                    node_name=self.device, path=config_path)
            else:
                if self.kernel == "linux":
                    device_enable_config = [
                        "CONFIG_DRIVERS_HDF_SENSOR_ACCEL=y\n"]
                else:
                    device_enable_config = [
                        "LOSCFG_DRIVERS_HDF_SENSOR_ACCEL=y\n"]

        if device_enable_config:
            return device_enable_config[0]
        else:
            return ""

    def get_model_template_list(self, module, board):
        templates_dir = hdf_utils.get_templates_lite_dir()
        templates_model_dir = []
        for path, dir_name, _ in os.walk(templates_dir):
            if dir_name:
                templates_model_dir.extend(dir_name)
        templates_model_dir = list(filter(
            lambda model_dir: self.module in model_dir,
            templates_model_dir))
        target_template_path = list(map(
            lambda dir_name: os.path.join(templates_dir, dir_name),
            templates_model_dir))[0]
        templates_file_list = os.listdir(target_template_path)
        if module == "audio" and board.startswith("hispark_taurus"):
            templates_file_list = list(filter(
                lambda x: x.startswith("hi35xx"), templates_file_list))
        return templates_file_list, target_template_path

    def linux_operation_config(self, driver_file_path, driver_head_path):
        adapter_hdf = hdf_utils.get_vendor_hdf_dir_adapter(
            self.root, self.kernel)
        hdf_utils.judge_file_path_exists(adapter_hdf)

        adapter_model_path = os.path.join(adapter_hdf, 'model', self.module)
        hdf_utils.judge_file_path_exists(adapter_model_path)

        liteos_file_name = ['Makefile', 'Kconfig']
        file_path_temp = {}
        for file_name in liteos_file_name:
            if file_name == "Makefile":
                linux_makefile_file_path = os.path.join(adapter_model_path, file_name)
                if self.module == "audio":
                    args_tuple = (driver_file_path, driver_head_path, self.module,
                                  self.driver, self.root, self.device, self.board)
                    linux_makefile_file_path = audio_linux_makefile_operation(
                        linux_makefile_file_path, args_tuple)
                else:
                    linux_makefile_operation(
                        linux_makefile_file_path, driver_file_path[0], driver_head_path[0],
                        self.module, self.driver)
                file_path_temp['Makefile'] = linux_makefile_file_path

            elif file_name == "Kconfig":
                kconfig_path = os.path.join(adapter_model_path, file_name)
                kconfig_file_operation(kconfig_path, self.module,
                                       self.driver, self.template_file_path)
                file_path_temp['Kconfig'] = kconfig_path
        return file_path_temp

