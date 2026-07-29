#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2020-2021 Huawei Device Co., Ltd.
# 
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.

class GetToolVersion(object):
    def __init__(self):
        self.hdf_version_major = 0
        self.hdf_version_minor = 2
        self.hdf_version_patch = 0

    def get_version(self):
        return "{}.{}.{}".format(
            self.hdf_version_major, self.hdf_version_minor, self.hdf_version_patch)
