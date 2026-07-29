#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2020-2021 Huawei Device Co., Ltd.
# 
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.


import configparser
import json
import os
import shutil
from string import Template

import hdf_utils
from hdf_tool_exception import HdfToolException
from .hdf_command_error_code import CommandErrorCode
from .hdf_command_handler_base import HdfCommandHandlerBase
from .hdf_defconfig_patch import HdfDefconfigAndPatch
from .hdf_device_info_hcs import HdfDeviceInfoHcsFile
from .hdf_get_handler import HdfGetHandler
from .hdf_vendor_build_file import HdfVendorBuildFile
from .hdf_vendor_kconfig_file import HdfVendorKconfigFile
from .hdf_vendor_makefile import HdfVendorMakeFile
from .hdf_vendor_mk_file import HdfVendorMkFile
from .hdf_module_kconfig_file import HdfModuleKconfigFile
from .hdf_module_mk_file import HdfModuleMkFile
from .hdf_driver_config_file import HdfDriverConfigFile


class HdfDeleteHandler(HdfCommandHandlerBase):
    def __init__(self, args):
        super(HdfDeleteHandler, self).__init__()
        self.cmd = 'delete'
        self.handlers = {
            'vendor': self._delete_vendor_handler,
            'module': self._delete_module_handler,
            'driver': self._delete_driver_handler,
            'module_driver': self._delete_module_driver_handler,
        }
        self.parser.add_argument("--action_type",
                                 help=' '.join(self.handlers.keys()),
                                 required=True)
        self.parser.add_argument("--root_dir", required=True)
        self.parser.add_argument("--vendor_name")
        self.parser.add_argument("--module_name")
        self.parser.add_argument("--driver_name")
        self.parser.add_argument("--board_name")
        self.parser.add_argument("--kernel_name")
        self.parser.add_argument("--device_name")
        self.args_original = args
        self.args = self.parser.parse_args(args)

    def _delete_vendor_handler(self):
        self.check_arg_raise_if_not_exist("vendor_name")
        self.check_arg_raise_if_not_exist("board_name")
        root, vendor, _, _, _ = self.get_args()
        vendor_hdf_dir = hdf_utils.get_vendor_hdf_dir(root, vendor)
        if not os.path.exists(vendor_hdf_dir):
            return
        for module in os.listdir(vendor_hdf_dir):
            mod_dir = os.path.join(vendor_hdf_dir, module)
            if os.path.isdir(mod_dir):
                pass
        shutil.rmtree(vendor_hdf_dir)

    def _delete_module(self, root, model, model_info):
        for key, path_value in model_info.items():
            if key.split("_")[-1] == "name":
                pass
            elif key == "driver_file_path":
                split_str = '/%s/driver' % model
                driver_file = os.path.join(
                    root, path_value.rsplit(split_str, 1)[0], model)
                if os.path.exists(driver_file):
                    shutil.rmtree(driver_file)
            else:
                self._delete_file_func(root, key, model_info, model)
        create_model_data = self._delete_model_info()
        delete_model_info = hdf_utils.get_create_model_info(
            root=root, create_data=json.dumps(create_model_data))
        return delete_model_info

    def _delete_model_info(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.check_arg_raise_if_not_exist("module_name")
        root, _, module, _, _, _, _ = self.get_args()
        create_file_save_path = hdf_utils.module_save_file_info(root)
        data = hdf_utils.read_file(create_file_save_path)
        write_data = json.loads(data)
        write_data.pop(module)
        hdf_utils.write_file(create_file_save_path,
                             json.dumps(write_data, indent=4))
        return write_data

    def _delete_module_handler(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.check_arg_raise_if_not_exist("module_name")
        root, _, module, _, _, _, _ = self.get_args()
        create_file_save_path = hdf_utils.module_save_file_info(root)
        data = hdf_utils.read_file(create_file_save_path)
        file_info = json.loads(data)
        model_info = file_info.get(module, None)
        if model_info is None:
            raise HdfToolException(
                ' delete model "%s" not exist' %
                module, CommandErrorCode.TARGET_NOT_EXIST)
        else:
            return self._delete_module(root, module, model_info)

    def _delete_driver(self, module, driver):
        root, vendor, _, _, board, kernel = self.get_args()
        drv_dir = hdf_utils.get_drv_dir(root, vendor, module, driver)
        if os.path.exists(drv_dir):
            shutil.rmtree(drv_dir)
        k_path = hdf_utils.get_module_kconfig_path(root, vendor, module)
        HdfModuleKconfigFile(root, module, k_path).delete_driver(driver)
        HdfModuleMkFile(root, vendor, module).delete_driver(driver)
        HdfDriverConfigFile(root, board, module, driver, kernel).delete_driver()

    def _delete_driver_handler(self):
        self.check_arg_raise_if_not_exist("vendor_name")
        self.check_arg_raise_if_not_exist("module_name")
        self.check_arg_raise_if_not_exist("driver_name")
        self.check_arg_raise_if_not_exist("board_name")
        _, _, module, driver, _ = self.get_args()
        self._delete_driver(module, driver)

    def _delete_file_func(self, root, key, model_info, model):
        if key.startswith("module_level_config"):
            for key_name, value in model_info[key].items():
                self._delete_config_operation(
                    key_name, value, temp_root=root, temp_model=model)

        elif key == "module_path":
            for file_name, module_value in model_info[key].items():
                self._delete_module_file(root, model, file_name, module_value)

    def _delete_module_file(self, root, model, file_name, module_value):
        if file_name.startswith("adapter"):
            file_path = os.path.join(root, module_value)
            file_info = hdf_utils.read_file_lines(file_path)
            delete_info = "libhdf_%s_hotplug" % model

            for index, line_info in enumerate(file_info):
                if line_info.find(delete_info) > 0:
                    file_info.pop(index)
                    hdf_utils.write_file_lines(file_path, file_info)
                    break
        elif file_name == "group" or file_name == "passwd":
            file_path = os.path.join(root, module_value)
            file_info = hdf_utils.read_file_lines(file_path)
            for index, line in enumerate(file_info):
                if line.find(model) != -1:
                    file_info.pop(index)
            hdf_utils.write_file_lines(file_path, file_info)
        else:
            if module_value.endswith("hcs"):
                hcs_path = os.path.join(root, module_value)
                HdfDeviceInfoHcsFile(
                    root=root, vendor="", module="",
                    board="", driver=" ", path=hcs_path). \
                    delete_driver(module=model)
            else:
                if not module_value:
                    return
                file_path = os.path.join(root, module_value)
                if os.path.exists(file_path):
                    os.remove(file_path)
                model_dir_path = "/".join(file_path.split("/")[:-1])
                if not os.path.exists(model_dir_path):
                    return
                file_list = os.listdir(model_dir_path)
                if not file_list:
                    shutil.rmtree(model_dir_path)

    def _delete_config_operation(self, key_name, value, temp_root, temp_model):
        if key_name == "%s_Makefile" % temp_model:
            HdfVendorMakeFile(
                temp_root, vendor="", kernel="",
                path=os.path.join(temp_root, value)).delete_module(temp_model)
        elif key_name == "%s_Kconfig" % temp_model:
            HdfVendorKconfigFile(
                temp_root, vendor="", kernel="",
                path=os.path.join(temp_root, value)).delete_module(temp_model)
        elif key_name == "%sBuild" % temp_model:
            HdfVendorBuildFile(
                temp_root, vendor="").delete_module(
                file_path=os.path.join(temp_root, value), model=temp_model)
        elif key_name == "%s_hdf_lite" % temp_model:
            HdfVendorMkFile(
                temp_root, vendor="").delete_module(
                file_path=os.path.join(temp_root, value), module=temp_model)
        elif key_name == "%s_dot_configs" % temp_model:
            for dot_path in value:
                if dot_path.split(".")[-1] == "config":
                    template_string = \
                        "LOSCFG_DRIVERS_HDF_${module_upper_case}=y\n"
                else:
                    template_string = \
                        "CONFIG_DRIVERS_HDF_${module_upper_case}=y\n"
                new_demo_config = Template(template_string). \
                    substitute({"module_upper_case": temp_model.upper()})
                defconfig_patch = HdfDefconfigAndPatch(
                    root=temp_root, vendor="", kernel="", board="",
                    data_model="", new_demo_config=new_demo_config)
                defconfig_patch.delete_module(
                    path=os.path.join(temp_root, dot_path))

    def _delete_module_driver_handler(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.check_arg_raise_if_not_exist("kernel_name")
        self.check_arg_raise_if_not_exist("module_name")
        self.check_arg_raise_if_not_exist("driver_name")
        self.check_arg_raise_if_not_exist("board_name")
        self.check_arg_raise_if_not_exist("device_name")
        root, _, module, driver, board, kernel, device = self.get_args()
        get_board = HdfGetHandler(self.args_original)
        section_res = get_board.judgement_device_in_model(device)
        board_module_driver = get_board.delete_module_driver_list()
        res_json_format = json.loads(board_module_driver)
        board_name_list = list(res_json_format.keys())
        temp_board_info = ""
        for board_name in board_name_list:
            board_driver = res_json_format[board_name]
            temp_board_name = board_name.rstrip("_kernel")
            if temp_board_name == board and kernel == "liteos":
                self._delete_liteos_driver_config(
                    root, model_info=board_driver, module_name=module,
                    driver_name=driver)
                temp_board_info = board_name
            elif temp_board_name == board:
                self._delete_linux_driver_config(
                    model_info=board_driver, module_name=module,
                    driver_name=driver)
                temp_board_info = board_name
            if len(board_driver) <= 1:
                del res_json_format[board_name]
        temp_driver_name = "-".join([device, driver])
        if temp_board_info:
            get_board.format_delete_driver_file(
                model_name=module, board_name=temp_board_info,
                driver_name=temp_driver_name)
        get_board.delete_device_operation(device, *section_res)
        res_driver = get_board.judge_create_driver_exist()
        return res_driver

    def _delete_liteos_driver_config(self, root, model_info, module_name, driver_name):
        model_info_key_list = list(model_info.keys())
        for key_name in model_info_key_list:
            file_list = list(model_info[key_name].keys())
            for file_name in file_list:
                file_path = model_info[key_name][file_name]
                if file_name == "Makefile":
                    mk_re_str = r"ifeq.+LOSCFG_DRIVERS_HDF_(?P<name>.+[A-Z0-9])"
                    temp_file = hdf_utils.GnMkFileParse(
                        file_path=file_path, temp_re=mk_re_str)
                    driver_config_index = temp_file.split_driver_start_to_end(
                        flag_str="endif")
                    self.config_delete_operation_liteos(
                        driver_name, driver_config_index, temp_file, file_path)
                elif file_name == "BUILD.gn":
                    mk_re_str = r"if.+LOSCFG_DRIVERS_HDF_(?P<name>.+\w)"
                    temp_file = hdf_utils.GnMkFileParse(
                        file_path=file_path, temp_re=mk_re_str)
                    driver_config_index = temp_file.split_driver_start_to_end(
                        flag_str="}")
                    self.config_delete_operation_liteos(
                        driver_name, driver_config_index, temp_file, file_path)
                if file_name == driver_name:
                    self._delete_driver_file(file_path=file_path)
                    del model_info[key_name][file_name]
                    if not model_info[key_name]:
                        del model_info[key_name]
                else:
                    self.linux_liteos_common(
                        file_name, file_path, module_name, driver_name)

    def _delete_linux_driver_config(
            self, model_info, module_name, driver_name):
        model_info_key_list = list(model_info.keys())
        for key_name in model_info_key_list:
            file_list = list(model_info[key_name].keys())
            for file_name in file_list:
                file_path = model_info[key_name][file_name]
                if file_name == "Makefile":
                    mk_re_str = r"CONFIG_DRIVERS_HDF_(?P<name>.+[A-Z0-9])"
                    flag_re = r"obj"
                    temp_file = hdf_utils.MakefileAndKconfigFileParse(
                        file_path=file_path, flag_str=flag_re, re_name=mk_re_str)
                    driver_config_index = temp_file.split_driver_start_to_end()
                    self.config_delete_operation_linux(
                        driver_name, driver_config_index, file_path)
                elif file_name == driver_name:
                    self._delete_driver_file(file_path=file_path)
                    del model_info[key_name][file_name]
                    if not model_info[key_name]:
                        del model_info[key_name]
                else:
                    self.linux_liteos_common(
                        file_name, file_path, module_name, driver_name)

    def config_delete_operation_liteos(
            self, driver_name, driver_config_index, temp_file, file_path):
        key_list = list(filter(
            lambda x: x.endswith(driver_name.upper()),
            driver_config_index.keys()))
        if key_list:
            key_name = key_list[0]
            res = temp_file.get_driver_config_str(
                driver_index=driver_config_index[key_name])
            temp_info = hdf_utils.read_file(file_path)
            hdf_utils.write_file(file_path, "".join(temp_info.split(res)))

    def config_delete_operation_linux(
            self, driver_name, driver_config_index, file_path):
        key_list = list(filter(
            lambda x: x.endswith(driver_name.upper()),
            driver_config_index.keys()))
        if key_list:
            key_name = key_list[0]
            res = driver_config_index[key_name].rstrip("endif")
            temp_info = hdf_utils.read_file(file_path)
            hdf_utils.write_file(file_path, "".join(temp_info.split(res)))

    def linux_liteos_common(self, file_name, file_path,
                            module_name, driver_name):
        root, _, module, _, _, kernel, device = self.get_args()
        if file_name == "Kconfig":
            config_re_str = r"DRIVERS_HDF_(?P<name>.+[A-Z0-9])"
            flag_re = r"config"
            temp_file = hdf_utils.MakefileAndKconfigFileParse(
                file_path=file_path, flag_str=flag_re, re_name=config_re_str)
            driver_config_index = temp_file.split_driver_start_to_end()
            self.config_delete_operation_linux(
                driver_name, driver_config_index, file_path)
        elif file_name.endswith("hcs"):
            HdfDeviceInfoHcsFile(
                root=root, vendor="", module="",
                board="", driver=" ", path=file_path). \
                delete_driver(module=driver_name, temp_flag="device")
        elif file_name.endswith("dot_configs"):
            if module_name == "sensor":
                temp_res = self.sensor_device_rely(module, kernel, device)
            else:
                temp_res = []
            for dot_path in file_path:
                if dot_path.split(".")[-1] == "config":
                    template_string = \
                        "LOSCFG_DRIVERS_HDF_${module_upper}_${driver_upper}=y\n"
                else:
                    template_string = \
                        "CONFIG_DRIVERS_HDF_${module_upper}_${driver_upper}=y\n"
                new_demo_config = Template(template_string). \
                    substitute({
                        "module_upper": module_name.upper(),
                        "driver_upper": driver_name.upper(),
                    })
                temp_res.append(new_demo_config)
                defconfig_patch = HdfDefconfigAndPatch(
                    root=root, vendor="", kernel="", board="",
                    data_model="", new_demo_config=temp_res)
                defconfig_patch.delete_module(
                    path=os.path.join(root, dot_path))

    def _delete_driver_file(self, file_path):
        for file_path_info in file_path:
            path_temp = file_path_info
            while os.path.exists(path_temp):
                if os.path.isfile(path_temp):
                    os.remove(path_temp)
                elif os.path.isdir(path_temp) and (not os.listdir(path_temp)):
                    os.rmdir(path_temp)
                else:
                    break
                path_temp = os.path.dirname(path_temp)

    def sensor_device_rely(self, module, kernel, device):
        templates_dir_path = hdf_utils.get_templates_lite_dir()
        templates_model_dir = []
        for path, dir_name, _ in os.walk(templates_dir_path):
            if dir_name:
                templates_model_dir.extend(dir_name)
        temp_templates_model_dir = list(
            filter(
                lambda model_dir: module in model_dir,
                templates_model_dir))
        config_file = [
            name for name in os.listdir(
                os.path.join(
                    templates_dir_path,
                    temp_templates_model_dir[0])) if name.endswith(".ini")]
        if config_file:
            config_path = os.path.join(
                templates_dir_path,
                temp_templates_model_dir[0],
                config_file[0])
            config = configparser.ConfigParser()
            config.read(config_path)
            section_list = config.options(section=kernel)
            if device in section_list:
                device_enable_config, _ = hdf_utils.ini_file_read_operation(
                    section_name=kernel,
                    node_name=device, path=config_path)
            else:
                if kernel == "linux":
                    device_enable_config = [
                        "CONFIG_DRIVERS_HDF_SENSOR_ACCEL=y\n"]
                else:
                    device_enable_config = [
                        "LOSCFG_DRIVERS_HDF_SENSOR_ACCEL=y\n"]
        else:
            device_enable_config = [""]
        return device_enable_config
