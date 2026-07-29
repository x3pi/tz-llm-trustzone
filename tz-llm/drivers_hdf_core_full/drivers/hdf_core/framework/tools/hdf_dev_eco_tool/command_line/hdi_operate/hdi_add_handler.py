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
from command_line.operate_group_passwd import OperateGroupPasswd
from hdf_tool_exception import HdfToolException


class HdiAddHandler(HdfCommandHandlerBase):
    def __init__(self, args):
        super(HdiAddHandler, self).__init__()
        self.handlers = {
            'interface': self._add_interface_handler,
            'peripheral': self._add_peripheral_handler,
            'unittest': self.create_unittest,
        }
        self.parser.add_argument("--action_type",
                                 help=' '.join(self.handlers.keys()),
                                 required=True)
        self.parser.add_argument("--root_dir", required=True)
        self.parser.add_argument("--vendor_name")
        self.parser.add_argument("--interface_name")
        self.parser.add_argument("--peripheral_name")
        self.parser.add_argument("--board_name")
        self.parser.add_argument("--version_number", type=float, default=1.0)
        self.args = self.parser.parse_args(args)
        self.set_config = hdf_tool_settings.HdfToolSettings()
        self.result_json = {
            "create_file": [],
            "config": []
        }
        self.space_num = 8

    def _hdi_config_operation(self, opt_type, config_path):
        file_config = hdf_utils.read_file(config_path)
        file_json_type = json.loads(file_config)
        for info in file_json_type['subsystems']:
            if info.get("subsystem") == "hdf":
                key_list_name = []
                for key_name in info.get("components"):
                    key_list_name.append(key_name.get('component'))
                if self.args.interface_name:
                    component_name = 'drivers_%s_%s' % \
                                     (opt_type, self.args.interface_name)
                elif self.args.peripheral_name:
                    component_name = 'drivers_%s_%s' % \
                                     (opt_type, self.args.peripheral_name)
                else:
                    component_name = ""
                if opt_type == "interface" and \
                        component_name not in key_list_name:
                    interface_dict = {
                        'component': component_name,
                        'features': []}
                    info.get("components").append(interface_dict)
                elif opt_type == "peripheral" and \
                        component_name not in key_list_name:
                    peripheral_dict = {
                        'component': component_name,
                        'features': []}
                    info.get("components").append(peripheral_dict)
                else:
                    pass
        hdf_utils.write_file(config_path,
                             json.dumps(file_json_type, indent=4))
        return config_path

    def get_template_file_folder(self, temp_folder_name):
        template_folder_path = self.set_config.get_template_path()
        pre_path = self.set_config.get_drivers_path_framework()
        tempath_path = os.path.join(self.args.root_dir,
                                    pre_path, template_folder_path)
        hdi_template_path = ""
        for pre_root, folders, _ in os.walk(tempath_path):
            for folder_name in folders:
                if folder_name.startswith("hdi"):
                    hdi_template_path = os.path.join(
                        pre_root, folder_name, temp_folder_name)
        return hdi_template_path

    def _check_arg(self, temp_type):
        if temp_type == "interface":
            self.check_arg_raise_if_not_exist('root_dir')
            self.check_arg_raise_if_not_exist('interface_name')
            self.check_arg_raise_if_not_exist('board_name')
            self.check_arg_raise_if_not_exist('version_number')
            interface_name = self.args.interface_name
            root = self.args.root_dir
            version = "v" + str(self.args.version_number).replace(".", "_")
            interface_converter = hdf_utils.WordsConverter(interface_name)
            return root, version, interface_converter, interface_name
        elif temp_type == "peripheral":
            self.check_arg_raise_if_not_exist('root_dir')
            self.check_arg_raise_if_not_exist('vendor_name')
            self.check_arg_raise_if_not_exist('peripheral_name')
            self.check_arg_raise_if_not_exist('board_name')
            self.check_arg_raise_if_not_exist('version_number')
            version = "v" + str(self.args.version_number).replace(".", "_")
            peripheral_name = self.args.peripheral_name
            root = self.args.root_dir
            vendor = self.args.vendor_name
            board = self.args.board_name
            peripheral_converter = hdf_utils.WordsConverter(peripheral_name)
            return root, version, peripheral_converter, peripheral_name, board, vendor
        elif temp_type == "unittest":
            self.check_arg_raise_if_not_exist('root_dir')
            self.check_arg_raise_if_not_exist('peripheral_name')
            self.check_arg_raise_if_not_exist('board_name')
            self.check_arg_raise_if_not_exist('version_number')
            peripheral_name = self.args.peripheral_name
            root = self.args.root_dir
            peripheral_converter = hdf_utils.WordsConverter(peripheral_name)
            replace_data = {
                "peripheral_name": peripheral_converter.lower_case(),
                "peripheral_name_capital_letters": peripheral_converter.upper_camel_case(),
                "peripheral_name_upper": peripheral_converter.upper_case(),
            }
            return peripheral_name, root, peripheral_converter, replace_data

    def _add_interface_handler(self):
        root, version, interface_converter, interface_name = \
            self._check_arg(temp_type="interface")
        hdi_template_path = self.get_template_file_folder(
            temp_folder_name="interface")
        interface_path = self.set_config.get_drivers_path_interface()
        interface_folder_path = os.path.join(
            self.args.root_dir, interface_path, self.args.interface_name)
        if os.path.exists(interface_folder_path):
            raise HdfToolException(
                '"%s" is path exist' % interface_folder_path,
                CommandErrorCode.TARGET_NOT_EXIST)
        os.makedirs(interface_folder_path)
        inter_name_version = os.path.join(interface_folder_path, version)
        os.makedirs(inter_name_version)
        replace_data = {
            "interface_name": interface_converter.lower_case(),
            "interface_name_capital_letters": interface_converter.upper_camel_case(),
            "interface_name_upper": interface_converter.upper_case(),
        }
        for file in os.listdir(hdi_template_path):
            file_src = file
            if file.startswith("I"):
                file = file.replace("interface",
                                    interface_converter.upper_camel_case())
                file = '.'.join(file.split('.')[0].split('_'))
            else:
                file = '.'.join(file.split('.')[0].split('_'))
            src_path = os.path.join(hdi_template_path, file_src)
            if file.endswith("json"):
                dst_path = os.path.join(interface_folder_path, file)
                if not os.path.exists(dst_path):
                    file_info = self.template_replace(src_path, replace_data)
                    self._write_file(dst_path, file_info)
            else:
                dst_path = os.path.join(inter_name_version, file)
                if not os.path.exists(dst_path):
                    file_info = self.template_replace(src_path, replace_data)
                    self._write_file(dst_path, file_info)
            create_file_path = self.format_file_path(dst_path, root)
            self.result_json["create_file"].append(create_file_path)
        hdi_dict = self.set_config.get_hdi_config()
        path_test = hdi_dict.get("interface", "")
        operation_file = os.path.join(root, path_test.strip("/"))
        re_config_path = self._hdi_config_operation(
            opt_type="interface", config_path=operation_file)
        config_file_path = self.format_file_path(re_config_path, root)
        self.result_json["config"].append(config_file_path)
        self.format_result_json(
            hdi_dict, create_type="interface", target_name=interface_name)
        return json.dumps(self.result_json, indent=4)

    def _add_peripheral_handler(self):
        root, version, peripheral_converter, peripheral_name, board, vendor = \
            self._check_arg(temp_type="peripheral")
        replace_data = {
            "peripheral_name": peripheral_converter.lower_case(),
            "peripheral_name_capital_letters": peripheral_converter.upper_camel_case(),
            "peripheral_name_upper": peripheral_converter.upper_case(),
        }
        pre_path = self.set_config.get_drivers_path_peripheral()
        hdi_config_arg = self.set_config.get_hdi_config()
        folder_name = hdi_config_arg["peripheral_folder"]["hdi_path"]
        peripheral_folder = os.path.join(root, pre_path, peripheral_name)
        hid_service_folder = os.path.join(peripheral_folder, folder_name)
        out_path = hdi_config_arg["output_path"].format(
            product=board, interface_name=peripheral_name)
        full_out_path = os.path.join(root, out_path)
        if not os.path.exists(full_out_path):
            raise HdfToolException(msg="interface file does not compile",
                                   error_code=CommandErrorCode.INTERFACE_ERROR)
        if not os.path.exists(peripheral_folder):
            os.makedirs(peripheral_folder)
        if not os.path.exists(hid_service_folder):
            os.makedirs(hid_service_folder)
        # Copy the file to the current location
        if os.path.exists(full_out_path):
            for file_name_template in hdi_config_arg["move_list"]:
                file_name = file_name_template.format(
                    interface_name=peripheral_name)
                target_file_path = os.path.join(
                    root, out_path, version, file_name)
                dst_file_path = os.path.join(
                    root, hid_service_folder, file_name)
                if os.path.exists(target_file_path) and \
                        not os.path.exists(dst_file_path):
                    shutil.copyfile(target_file_path, dst_file_path)
                temp_create = self.format_file_path(dst_file_path, root)
                self.result_json["create_file"].append(temp_create)
        self._option_peripheral_config(
            board, root, replace_data, hid_service_folder, peripheral_folder, vendor)
        path_test = hdi_config_arg.get("peripheral", "")
        operation_file = os.path.join(root, path_test.strip("/"))
        self._hdi_config_operation(opt_type="peripheral", config_path=operation_file)
        self.format_result_json(hdi_config_arg, create_type="peripheral",
                                target_name=peripheral_name)
        return json.dumps(self.result_json, indent=4)

    def _option_peripheral_config(
            self, board, root, replace_data, hid_service_folder,
            peripheral_folder, vendor):
        hdi_template_path = self.get_template_file_folder(
            temp_folder_name="peripheral")
        file_list = os.listdir(hdi_template_path)
        for template_file_name in file_list:
            folder_file_path = os.path.join(hdi_template_path, template_file_name)
            if os.path.isdir(folder_file_path):
                self._hdi_server_config(
                    folder_file_path, board, root, replace_data, vendor)
                continue
            src_path = os.path.join(hdi_template_path, template_file_name)
            if template_file_name.endswith("hdi"):
                target_location = os.path.join(root, hid_service_folder)
                target_file_name = template_file_name.split(
                    ".")[0].replace("_", ".")
                target_file_path = os.path.join(
                    target_location, str(target_file_name))
            else:
                target_file_name = template_file_name.split(
                    ".")[0].replace("_", ".")
                target_file_path = os.path.join(
                    peripheral_folder, target_file_name)
            if not os.path.exists(target_file_path):
                file_info = self.template_replace(src_path, replace_data)
                self._write_file(target_file_path, file_info)
            temp_config = self.format_file_path(target_file_path, root)
            self.result_json["create_file"].append(temp_config)

    def _hdi_server_config(self, config_path, board_name,
                           root_path, replace_data, vendor):
        template_list = os.listdir(config_path)
        temp_hcs_path = os.path.join(config_path, template_list[0])
        for file_name in template_list:
            if "hcs" in file_name:
                hcs_file_name_temp = file_name.split(".")[0].replace("_", ".")
                snake_case = re.sub(
                    r"(?P<key>[A-Z])",
                    r"_\g<key>",
                    hcs_file_name_temp)
                hcs_name = snake_case.lower().strip('_')

        board_type = "%s_user" % board_name
        board_parent_path_temp = hdf_tool_settings.HdfToolSettings(). \
            get_board_parent_path(board_type)
        board_parent_path = board_parent_path_temp.format(vendor=vendor)
        hcs_file_parent = os.path.join(root_path, board_parent_path)
        if os.path.exists(hcs_file_parent):
            hcs_target_file = os.path.join(
                root_path, board_parent_path, hcs_name)
        else:
            raise HdfToolException(
                'hcs config path %s not exist' % hcs_file_parent)
        lines = list(map(
            lambda x: " " * self.space_num + x,
            hdf_utils.read_file_lines(temp_hcs_path)))
        old_lines_temp = hdf_utils.read_file_lines(hcs_target_file)
        old_lines = list(filter(lambda x: x != "\n", old_lines_temp))
        status = False
        for line in old_lines:
            str_line = string.Template(lines[0]).substitute(replace_data)
            if line.split("::")[0].strip() == str_line.split("::")[0].strip():
                status = True
        if not status:
            new_data = old_lines[:-2] + lines + old_lines[-2:]
            for index, _ in enumerate(new_data):
                new_data[index] = string.Template(new_data[index]).\
                    substitute(replace_data)
            hdf_utils.write_file_lines(hcs_target_file, new_data)
        temp_config = self.format_file_path(hcs_target_file, root_path)
        self.result_json["config"].append(temp_config)
        self.config_group_passwd(root_path, replace_data)

    def config_group_passwd(self, root_path, replace_data):
        hdi_config = hdf_tool_settings.HdfToolSettings()
        group_passwd = OperateGroupPasswd(tool_settings=hdi_config, root_path=root_path)
        # group
        peripheral_name = replace_data.get("peripheral_name")
        group_file_path = group_passwd.operate_group(name=peripheral_name)
        temp_config = self.format_file_path(group_file_path, root_path)
        self.result_json["config"].append(temp_config)
        # passwd
        passwd_file_path = group_passwd.operate_passwd(name=peripheral_name)
        temp_config = self.format_file_path(passwd_file_path, root_path)
        self.result_json["config"].append(temp_config)
        self.config_selinux(root_path, replace_data)

    def config_selinux(self, root_path, replace_data):
        hdi_config = hdf_tool_settings.HdiToolConfig()
        # selinux --- type.te
        pre_path, selinux_type_info = hdi_config.get_hdi_selinux_type()
        temp_type_path = self._selinux_file_fill(
            pre_path, selinux_temp=selinux_type_info,
            root_path=root_path, replace_data=replace_data
        )
        temp_config = self.format_file_path(temp_type_path, root_path)
        self.result_json["config"].append(temp_config)

        # selinux --- hdf_service.te
        pre_path, selinux_hdf_service_info = hdi_config.get_hdi_selinux_hdf_service()
        temp_hdf_service_path = self._selinux_file_fill(
            pre_path, selinux_temp=selinux_hdf_service_info,
            root_path=root_path, replace_data=replace_data
        )
        temp_config = self.format_file_path(temp_hdf_service_path, root_path)
        self.result_json["config"].append(temp_config)

        # selinux --- hdf_service_contexts
        pre_path, selinux_hdf_service_contexts_info = \
            hdi_config.get_hdi_selinux_hdf_service_contexts()
        temp_service_contexts_path = self._selinux_file_fill(
            pre_path, selinux_temp=selinux_hdf_service_contexts_info,
            root_path=root_path, replace_data=replace_data, align=True
        )
        temp_config = self.format_file_path(temp_service_contexts_path, root_path)
        self.result_json["config"].append(temp_config)

        # selinux --- hdf_host.te
        pre_path, selinux_hdf_host = hdi_config.get_hdi_selinux_hdf_host()
        temp_host_path = self._selinux_file_fill(
            pre_path, selinux_temp=selinux_hdf_host,
            root_path=root_path, replace_data=replace_data
        )
        temp_config = self.format_file_path(temp_host_path, root_path)
        self.result_json["config"].append(temp_config)

        # selinux --- peripheral config
        pre_path, selinux_peripheral_host = hdi_config.get_selinux_peripheral_hdf_host()
        self._selinux_peripheral_config(
            pre_path, selinux_temp=selinux_peripheral_host,
            root_path=root_path, replace_data=replace_data
        )

    def _selinux_peripheral_config(self, pre_path, selinux_temp,
                                   root_path, replace_data):
        peripheral_config_path = os.path.join(root_path, pre_path, selinux_temp["path"])
        temp_peripheral_path = string.Template(
            peripheral_config_path).safe_substitute(replace_data)
        if not os.path.exists(temp_peripheral_path):
            os.makedirs(temp_peripheral_path)
        config_list = selinux_temp.get('peripheral_file_list')
        for key_name in list(config_list.keys()):
            file_name = config_list.get(key_name)
            temp_file_name = string.Template(file_name).safe_substitute(replace_data)
            file_info = "".join(selinux_temp.get(key_name))
            temp_file_info = string.Template(file_info).safe_substitute(replace_data)
            file_path = os.path.join(temp_peripheral_path, temp_file_name)
            config_info = hdf_tool_settings.HdfToolSettings().get_file_config_info()
            write_fd = os.open(file_path, config_info["flags"], config_info["modes"])
            with os.fdopen(write_fd, "wb") as f_write:
                f_write.write(temp_file_info.encode("utf-8"))
            temp_create = self.format_file_path(file_path, root_path)
            self.result_json["create_file"].append(temp_create)

    def create_unittest(self):
        peripheral_name, root, peripheral_converter, replace_data =\
            self._check_arg(temp_type="unittest")
        pre_path = self.set_config.get_drivers_path_peripheral()
        peripheral_folder = os.path.join(root, pre_path, peripheral_name)
        hdi_config_arg = self.set_config.get_hdi_config()
        folder_name = hdi_config_arg["peripheral_folder"]["unittest_path"]
        unittest_folder = os.path.join(peripheral_folder, folder_name)
        if not os.path.exists(unittest_folder):
            os.makedirs(unittest_folder)
        unittest_template_path = self.get_template_file_folder(
            temp_folder_name="unittest")
        for template_file in os.listdir(unittest_template_path):
            template_file_src = template_file
            template_file = template_file.replace(
                "name", peripheral_converter.lower_case())
            template_file = '.'.join(template_file.split('.')[0].rsplit('_', 1))
            src_path = os.path.join(unittest_template_path, template_file_src)
            if template_file.endswith("gn"):
                unittest_folder_upper_level = re.split(r'\w+$', unittest_folder)[0]
                dst_path = os.path.join(unittest_folder_upper_level, template_file)
                if not os.path.exists(dst_path):
                    file_info = self.template_replace(src_path, replace_data)
                    self._write_file(dst_path, file_info)
            else:
                dst_path = os.path.join(unittest_folder, template_file)
                if not os.path.exists(dst_path):
                    file_info = self.template_replace(src_path, replace_data)
                    self._write_file(dst_path, file_info)
            temp_create = self.format_file_path(dst_path, root)
            self.result_json["create_file"].append(temp_create)
        # add bundle.json test
        for file_name in os.listdir(peripheral_folder):
            if file_name.endswith(".json"):
                config_json_path = os.path.join(peripheral_folder, file_name)
                file_info_str = hdf_utils.read_file(config_json_path)
                file_info_json = json.loads(file_info_str)
                pre_dict = file_info_json['component']["build"]
                temp_str = pre_dict[list(pre_dict.keys())[0]][0].split(":")[0]
                result_test_str = "/".join([temp_str, "test:{peripheral_name}_unittest".
                                           format(peripheral_name=peripheral_name)])
                if result_test_str not in pre_dict[list(pre_dict.keys())[1]]:
                    pre_dict[list(pre_dict.keys())[1]].append(result_test_str)
                hdf_utils.write_file(config_json_path,
                                     content=json.dumps(file_info_json, indent=4))
                temp_create = self.format_file_path(config_json_path, root)
                self.result_json["config"].append(temp_create)
        self.format_result_json(hdi_config_arg, create_type="unittest",
                                target_name=peripheral_name)
        return json.dumps(self.result_json, indent=4)

    def _selinux_file_fill(self, pre_path, selinux_temp,
                           root_path, replace_data, align=False):
        temp_file_name = selinux_temp["file_name"]
        target_config_file_path = os.path.join(
            root_path, pre_path, selinux_temp["path"], temp_file_name)
        if not os.path.exists(target_config_file_path):
            raise HdfToolException(
                    'file: %s not exist' %
                    target_config_file_path, CommandErrorCode.TARGET_NOT_EXIST)
        temp_lines = self.read_lines_binary(path=target_config_file_path)
        if isinstance(selinux_temp["info_temp"], str):
            if align:
                temp_new_line = string.Template(
                    selinux_temp["info_temp"]).safe_substitute(replace_data)
                temp_line = temp_new_line.split(" ")
                space_num = selinux_temp["space_len"] - len(temp_line[0])
                new_line = (" " * space_num).join(temp_line)
            else:
                new_line = string.Template(
                    selinux_temp["info_temp"]).safe_substitute(replace_data)
            temp_replace_list = [new_line.encode('utf-8')]
        elif isinstance(selinux_temp["info_temp"], list):
            hdf_host_pid_list = self.count_hdf_host_pid(temp_lines)
            replace_data.update({
                "pid_num": OperateGroupPasswd.generate_id(max(hdf_host_pid_list))
            })
            temp_replace_list = []
            splice_str = ""
            for temp_line in selinux_temp["info_temp"]:
                splice_str += temp_line
                temp_replace_list.append(
                    string.Template(temp_line).safe_substitute(
                        replace_data).encode('utf-8'))
            temp_replace_list = temp_replace_list[2:]
            new_line = string.Template(splice_str).safe_substitute(replace_data)
        else:
            temp_replace_list = []
            new_line = ""
        if not set(temp_replace_list) < set(temp_lines):
            new_line_temp = new_line.encode('utf-8')
            temp_lines.append(new_line_temp)
            self.writer_lines_binary(target_config_file_path, temp_lines)
        return target_config_file_path

    def template_replace(self, src_path, replace_data):
        file_info = hdf_utils.read_file(src_path)
        str_header = string.Template(file_info)
        res_info = str_header.substitute(replace_data)
        return res_info

    def _write_file(self, dst_path, file_info):
        hdf_utils.write_file(dst_path, content=file_info)

    def count_hdf_host_pid(self, temp_lines):
        hdf_host_pid_list = []
        for line in temp_lines:
            line = line.strip().decode("utf-8")
            pid_re_result = re.search(r"pid=\d+", line)
            if pid_re_result:
                pid_num = int(pid_re_result.group().split("=")[-1])
                if pid_num not in hdf_host_pid_list:
                    hdf_host_pid_list.append(pid_num)
        return hdf_host_pid_list

    def format_file_path(self, file_path, parent_root):
        file_path_temp = file_path.lstrip(parent_root)
        temp_path_list = file_path_temp.split(os.path.sep)
        temp_path = "/".join(temp_path_list)
        return temp_path

    def format_result_json(self, config_dict, create_type, target_name):
        out_config_name = config_dict.get("out_config_name")
        if not out_config_name:
            out_config_name = "create_idl_hdi.config"
        config_parent = self.set_config.get_hdi_file_path()
        out_config_path = os.path.join(config_parent, out_config_name)
        if not os.path.exists(out_config_path):
            raise HdfToolException(
                '"%s" is path exist' %
                out_config_path, CommandErrorCode.TARGET_NOT_EXIST)
        read_file_info = hdf_utils.read_file(out_config_path)
        read_file_json = json.loads(read_file_info)
        temp_dict = {
            target_name: self.result_json
        }
        if create_type == "interface":
            create_interface_info = read_file_json.get("interface")
            if create_interface_info and not create_interface_info.get(target_name, False):
                create_interface_info.update(temp_dict)
            else:
                read_file_json["interface"] = temp_dict
        elif create_type == "peripheral":
            create_interface_info = read_file_json.get("peripheral")
            if create_interface_info and not create_interface_info.get(target_name, False):
                create_interface_info.update(temp_dict)
            else:
                read_file_json["peripheral"] = temp_dict
        elif create_type == "unittest":
            create_interface_info = read_file_json.get("unittest")
            if create_interface_info and not create_interface_info.get(target_name, False):
                create_interface_info.update(temp_dict)
            else:
                read_file_json["unittest"] = temp_dict
        hdf_utils.write_file(out_config_path, json.dumps(read_file_json, indent=4))

    def read_lines_binary(self, path):
        with open(path, "rb") as f_read:
            read_lines = f_read.readlines()
        return read_lines

    def writer_lines_binary(self, file_path, content):
        with open(file_path, 'wb') as file_write:
            file_write.writelines(content)
