#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2022 Shenzhen Kaihong Digital Industry Development Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.

import pytest
from selenium import webdriver
from selenium.webdriver.chrome.webdriver import WebDriver

global_param = {}


@pytest.fixture
def _set_global_data(global_driver):
    driver = None
    global_param[global_driver] = driver


@pytest.fixture
def _get_global_data(global_driver):
    return global_param.get(global_driver)


def pytest_addoption(parser):
    parser.addoption(
        "--browser", action="store", default="chrome", help="browser option: firefox or chrome"
             )


@pytest.fixture(scope='session')
def browser(request):
    driver = _get_global_data('global_driver')
    if driver is None:
        name = request.config.getoption("--browser")
        if name == "firefox":
            driver = webdriver.Firefox()
        elif name == "chrome":
            driver = webdriver.Chrome()
        else:
            driver = webdriver.Firefox()

    def fn():
        driver.quit()

    request.addfinalizer(fn)
    return driver
