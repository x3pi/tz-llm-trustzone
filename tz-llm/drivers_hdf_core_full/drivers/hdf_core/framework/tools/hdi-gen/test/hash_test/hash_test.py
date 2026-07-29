#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2023 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.

import os
import subprocess
import time


def get_time_stamp():
    return int(round(time.time() * 1000))


def print_success(info):
    print("\033[32m{}\033[0m".format(info))


def print_failure(info):
    print("\033[31m{}\033[0m".format(info))


def compare_file(first_file, second_file):
    with open(first_file, 'r') as first_hash_file:
        with open(second_file, 'r') as second_hash_file:
            first_hash_info = first_hash_file.read()
            second_hash_info = second_hash_file.read()
            return first_hash_info == second_hash_info


def exec_command(command):
    return subprocess.getstatusoutput(command)


def file_exists(file_path):
    return os.path.isfile(file_path)


class Test:
    def __init__(self, name):
        self.name = name

    def run(self):
        # please add test code here
        return False

    def test(self):
        print_success("[ RUN       ] {}".format(self.name))
        start_time = get_time_stamp()
        result = self.run()
        end_time = get_time_stamp()

        if result:
            print_success("[        OK ] {} ({}ms)".format(self.name, end_time - start_time))
        else:
            print_failure("[    FAILED ] {} ({}ms)".format(self.name, end_time - start_time))
        return result


# get hash key and print standard ouput
class TestHashGood1(Test):
    def run(self):
        result_hash_file_path = "./good/hash.txt"
        command = "../../hdi-gen -D ./good/ -r ohos.hdi:./good/ --hash"
        status, exec_result = exec_command(command)
        if status != 0:
            print(exec_result)
            return False
        temp_hash_info = exec_result

        with open(result_hash_file_path, 'r') as result_hash_file:
            result_hash_info = result_hash_file.read().rstrip()
            return temp_hash_info == result_hash_info


# get hash key and print file
class TestHashGood2(Test):
    def run(self):
        result_hash_file_path = "./good/hash.txt"
        temp_hash_file_path = "./good/temp.txt"
        command = "../../hdi-gen -D ./good/ -r ohos.hdi:./good/ --hash -o {}".format(temp_hash_file_path)
        status, result = exec_command(command)
        if status != 0:
            print(result)
            return False

        result = False
        if compare_file(temp_hash_file_path, result_hash_file_path):
            result = True
        exec_command("rm -f ./good/temp.txt")
        return result


# nothing idl files
class TestBadHash01(Test):
    def run(self):
        command = "../../hdi-gen -D ./bad_01/ -r ohos.hdi:./bad_01/ --hash"
        status, _ = exec_command(command)
        return status != 0


# empty idl file
class TestBadHash02(Test):
    def run(self):
        command = "../../hdi-gen -D ./bad_02/ -r ohos.hdi:./bad_02/ --hash"
        status, _ = exec_command(command)
        return status != 0


# the idl file has no package name
class TestBadHash03(Test):
    def run(self):
        command = "../../hdi-gen -D ./bad_03/ -r ohos.hdi:./bad_03/ --hash"
        status, _ = exec_command(command)
        return status != 0


# the idl file has error package name
class TestBadHash04(Test):
    def run(self):
        command = "../../hdi-gen -D ./bad_04/ -r ohos.hdi:./bad_04/ --hash"
        status, _ = exec_command(command)
        return status != 0


class Tests:
    test_cases = [
        TestHashGood1("TestHashPrintStd"),
        TestHashGood2("TestHashPrintFile"),
        TestBadHash01("TestBadHashWithNoFile"),
        TestBadHash02("TestBadHashWithEmptyFile"),
        TestBadHash03("TestBadHashWithNoPackage"),
        TestBadHash04("TestBadHashWithErrorPackage"),
    ]

    @staticmethod
    def set_up_test_case():
        hdi_gen_file = "../../hdi-gen"
        ret = file_exists(hdi_gen_file)
        if not ret:
            print_failure("[===========] no such file:{}".format(hdi_gen_file))
        return ret

    @staticmethod
    def set_down_test_case():
        pass

    @staticmethod
    def test():
        test_case_num = len(Tests.test_cases)
        success_case_num = 0
        print_success("[===========] start {} test".format(test_case_num))
        for test_case in Tests.test_cases:
            if test_case.test():
                success_case_num += 1
        print_success("[    PASSED ] {} test".format(success_case_num))
        failure_case_num = test_case_num - success_case_num
        if failure_case_num > 0:
            print_failure("[    FAILED ] {} test".format(failure_case_num))


if __name__ == "__main__":
    if not Tests.set_up_test_case():
        exit(-1)
    Tests.test()
    Tests.set_down_test_case()
