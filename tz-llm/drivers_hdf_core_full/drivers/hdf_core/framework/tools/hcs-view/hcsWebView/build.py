#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2022 Shenzhen Kaihong Digital Industry Development Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.
import os
import stat
from asyncio import subprocess

if __name__ == "__main__":
    with open(r".\..\hcsVSCode\editor.html", "r", encoding="utf8") as file:
        ss = file.read()
    i1 = ss.index("// update js code begin") + len("// update js code begin") + 1
    i2 = ss.index("// update js code end") - 1
    with open(r".\dist\main.js", "r", encoding="utf8") as file:
        destss = file.read()
    ss = ss[:i1] + destss + ss[i2:]
    flags = os.O_RDWR | os.O_CREAT
    modes = stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IWGRP | stat.S_IROTH
    with os.fdopen(os.open(r".\..\hcsVSCode\editor.html", flags, modes),
                   "w", encoding="utf-8") as file:
        file.write(ss)
    print("replaced")
