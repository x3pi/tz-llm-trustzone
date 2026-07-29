#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2022 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.


import re
import os
from string import Template

import hdf_utils


def find_build_file_end_index(date_lines, model_name, pre_str='FRAMEWORKS'):
    state = False
    end_index = 0
    frameworks_model_name = "%s_%s_ROOT" % (pre_str, model_name.upper())
    frameworks_model_value = ''
    for index, line in enumerate(date_lines):
        if line.startswith("#"):
            continue
        elif line.find("hdf_driver") != -1:
            state = True
            continue
        elif line.startswith("}") and state:
            end_index = index
            state = False
        elif line.strip().startswith(frameworks_model_name):
            frameworks_model_value = line.split("=")[-1].strip()
        else:
            continue
    result_tuple = (end_index, frameworks_model_name, frameworks_model_value)
    return result_tuple


def audio_build_file_operation(path, args_tuple):
    source_path, head_path, module, driver, root, devices, kernel = args_tuple
    build_gn_path = path
    date_lines = hdf_utils.read_file_lines(build_gn_path)
    result_tuple = find_build_file_end_index(date_lines, model_name=module)
    judge_result = judge_driver_config_exists(date_lines, driver_name=driver)
    if judge_result:
        return
    end_index, frameworks_name, frameworks_value = result_tuple

    first_line = "\n  if (defined(LOSCFG_DRIVERS_HDF_${model_name_upper}_${driver_name_upper})) {\n"
    third_line = '    include_dirs += [ "$${file_parent_path}/${head_path}" ]\n'
    four_line = "  }\n"
    if len(source_path) > 1:
        sources_line = []
        multi_resource = "    sources += [ \n"
        multi_end = "    ]\n"
        temp_line = r'      "$${file_parent_path}/${source_path}",'
        for source in source_path:
            temp_handle = Template(temp_line.replace("\"$", "temp_flag"))
            parent_temp_dict = analyze_parent_path(
                date_lines, source, "", devices, root)
            sources_line.append(temp_handle.substitute(
                parent_temp_dict).replace("temp_flag", "\"$") + "\n")
        sources_str = ''.join(sources_line)
        build_resource = ''.join([multi_resource, sources_str, multi_end])
    else:
        build_resource = '    sources += [ "$${file_parent_path}/${source_path}" ]\n'
        for source in source_path:
            temp_handle = Template(build_resource.replace("\"$", "temp_flag"))
            temp_dict = analyze_parent_path(
                date_lines, source, "", devices, root)
            build_resource = temp_handle.substitute(
                temp_dict).replace("temp_flag", "\"$")
    build_add_template = ''.join([first_line, build_resource, third_line, four_line])
    build_replace_dict = analyze_parent_path(
        date_lines, "", head_path[0], devices, root, kernel_type=kernel)
    temp_handle = Template(build_add_template.replace("\"$", "temp_flag"))
    model_dict = {
        'model_name_upper': module.upper(),
        'driver_name_upper': driver.upper(),
    }
    build_replace_dict.update(model_dict)
    new_line = temp_handle.substitute(build_replace_dict).replace("temp_flag", "\"$")
    date_lines = date_lines[:end_index] + [new_line] + date_lines[end_index:]
    hdf_utils.write_file_lines(build_gn_path, date_lines)


def judge_driver_config_exists(date_lines, driver_name):
    for _, line in enumerate(date_lines):
        if line.startswith("#"):
            continue
        elif line.find(driver_name) != -1:
            return True
    return False


def analyze_parent_path(date, source_path, head_path,
                        devices, root, kernel_type="linux"):
    str_re = "^[A-Z _ 0-9]+="
    macro_definition_dict = {}
    for index, i in enumerate(date):
        re_result = re.search(str_re, i)
        if not re_result:
            continue
        str_split_list = i.strip().split(" = ")
        if len(str_split_list) == 1 and str_split_list[0].endswith("="):
            temp_name = str_split_list[0].split(" ")[0].strip("=")
            macro_definition_dict[temp_name] = date[index + 1].strip()
        else:
            macro_definition_dict[str_split_list[0]] = str_split_list[-1]
    date_replace = {
        "file_parent_path": "",
        "source_path": "",
        "head_path": "",
    }
    if source_path:
        parent_path_temp = source_path.split(devices)[0].strip(
            root).replace("\\", "/").strip("/")
        parent_path = '/'.join(parent_path_temp.split('/')[:3])
    else:
        parent_path_temp = head_path.split(devices)[0].strip(
            root).replace("\\", "/").strip("/")
        parent_path = '/'.join(parent_path_temp.split('/')[:3])
    if kernel_type == "linux":
        for k_name, values in macro_definition_dict.items():
            if values.find(parent_path) == -1:
                continue
            if values.startswith("drivers/hdf/framework") and head_path.strip():
                date_replace['file_parent_path'] = k_name
            elif source_path.strip() and not values.startswith("drivers/hdf/framework"):
                date_replace['file_parent_path'] = k_name
    else:
        for k_name, values in macro_definition_dict.items():
            if values.find(parent_path) != -1:
                date_replace['file_parent_path'] = k_name
    relatively_path_dict, _ = hdf_utils.ini_file_read_operation(
        section_name="audio", node_name='driver_path')
    audio_board_name = parent_path_temp.split('/')[3]
    if audio_board_name.startswith("rk3568"):
        relatively_path = relatively_path_dict["rk3568"]
    else:
        relatively_path = relatively_path_dict["hi3516"]
    res_date_replace = source_hand_file_path(
        relatively_path, source_path, head_path, date_replace)
    return res_date_replace


def source_hand_file_path(relatively_path, source_path, head_path, date_replace):
    if source_path:
        file_source_path_full = source_path.replace("\\", "/")
        date_replace['source_path'] = file_source_path_full.split(
            relatively_path)[-1].strip('/')
    if head_path:
        file_head_path_full = head_path.replace("\\", "/")
        date_replace['head_path'] = '/'.join(file_head_path_full.split(
            relatively_path)[-1].split('/')[:-1]).strip('/')
    return date_replace


def build_file_operation(path, driver_file_path, head_path, module, driver):
    build_gn_path = path
    date_lines = hdf_utils.read_file_lines(build_gn_path)
    source_file_path = driver_file_path.replace('\\', '/')
    judge_result = judge_driver_config_exists(date_lines, driver_name=driver)
    if judge_result:
        return
    if driver_file_path.find("FRAMEWORK".lower()) != -1:
        result_tuple = find_build_file_end_index(date_lines, model_name=module)
        end_index, frameworks_name, frameworks_value = result_tuple
        new_line = build_file_operation_comm(
            frameworks_value, source_file_path, module, driver, head_path)
    else:
        result_tuple = find_build_file_end_index(
            date_lines, model_name=module, pre_str="PERIPHERAL")
        end_index, frameworks_name, frameworks_value = result_tuple
        build_add_template = template_str_splice(type_name=" ")
        include_model_info = frameworks_value.split("/")[-1].strip("\"")
        build_gn_path_config = source_file_path.split(include_model_info + "/", 1)
        temp_handle = Template(
            build_add_template.replace("$PERIPHERAL", "PERIPHERAL"))

        temp_replace = {
            'model_name_upper': module.upper(),
            'driver_name_upper': driver.upper(),
            'source_file_path': build_gn_path_config[-1],
            'head_file_path': '/'.join(list(
                filter(lambda x: x,
                       head_path.split(include_model_info, 1)[-1].
                       strip(os.path.sep).split(os.path.sep)))[:-1])
        }
        new_line = temp_handle.substitute(
            temp_replace).replace("PERIPHERAL", "$PERIPHERAL")
    date_lines = date_lines[:end_index] + [new_line] + date_lines[end_index:]
    hdf_utils.write_file_lines(build_gn_path, date_lines)


def template_str_splice(type_name):
    if type_name == "framework":
        first_line = "\n  if (defined(LOSCFG_DRIVERS_HDF" \
                     "_${model_name_upper}_${driver_name_upper})) {\n"
        include_line = '    include_dirs += [ "$FRAMEWORKS' \
                       '_${model_name_upper}_ROOT/${head_file_path}" ]\n'
        second_line = '    sources += [ "$FRAMEWORKS' \
                      '_${model_name_upper}_ROOT/${source_file_path}" ]\n'
        third_line = "  }\n"
        build_add_template = first_line + include_line + second_line + third_line
    else:
        first_line = "\n  if (defined(LOSCFG_DRIVERS_" \
                     "HDF_${model_name_upper}_${driver_name_upper})) {\n"
        include_line = '    include_dirs += [ "$PERIPHERAL' \
                       '_${model_name_upper}_ROOT/${head_file_path}" ]\n'
        second_line = '    sources += [ "$PERIPHERAL' \
                      '_${model_name_upper}_ROOT/${source_file_path}" ]\n'
        third_line = "  }\n"
        build_add_template = first_line + include_line + second_line + third_line
    return build_add_template


def input_build_file_operation(path, driver_file_path, head_path, module, driver):
    build_gn_path = path
    date_lines = hdf_utils.read_file_lines(build_gn_path)
    source_file_path = driver_file_path.replace('\\', '/')
    judge_result = judge_driver_config_exists(date_lines, driver_name=driver)
    if judge_result:
        return
    result_tuple = find_build_file_end_index(date_lines, model_name=module)
    end_index, frameworks_name, frameworks_value = result_tuple
    re_str_include = "include_dirs ="
    new_line = build_file_operation_comm(
        frameworks_value, source_file_path, module, driver, head_path)
    if re.search(re_str_include, "".join(date_lines[:end_index])) is None:
        include_str = "  include_dirs = []\n"
        date_lines = date_lines[:end_index] + [include_str] + [new_line] + date_lines[end_index:]
    else:
        date_lines = date_lines[:end_index] + [new_line] + date_lines[end_index:]
    hdf_utils.write_file_lines(build_gn_path, date_lines)


def build_file_operation_comm(
        frameworks_value, source_file_path, module, driver, head_path):
    build_add_template = template_str_splice(type_name="framework")
    include_model_info = frameworks_value.split("model")[-1].strip('"') + "/"
    build_gn_path_config = source_file_path.split(include_model_info)
    temp_handle = Template(
        build_add_template.replace("$FRAMEWORKS", "FRAMEWORKS"))
    temp_replace = {
        'model_name_upper': module.upper(),
        'driver_name_upper': driver.upper(),
        'source_file_path': build_gn_path_config[-1],
        'head_file_path': '/'.join(
            list(filter(lambda x: x, head_path.split("model")[-1].strip(
                os.path.sep).split(os.path.sep)[2:-1])))
    }
    new_line = temp_handle.substitute(
        temp_replace).replace("FRAMEWORKS", "$FRAMEWORKS")
    return new_line
