/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ili9881c_boe.h"
#include "gpio_if.h"
#include "hdf_bl.h"
#include "hdf_disp.h"
#include "osal.h"

/* panel on command payload */
static uint8_t g_payLoad0[] = { 0xFF, 0x98, 0x81, 0x03 };
static uint8_t g_payLoad1[] = { 0x01, 0x00 };
static uint8_t g_payLoad2[] = { 0x02, 0x00 };
static uint8_t g_payLoad3[] = { 0x03, 0x73 };
static uint8_t g_payLoad4[] = { 0x04, 0x00 };
static uint8_t g_payLoad5[] = { 0x05, 0x00 };
static uint8_t g_payLoad6[] = { 0x06, 0x08 };
static uint8_t g_payLoad7[] = { 0x07, 0x00 };
static uint8_t g_payLoad8[] = { 0x08, 0x00 };
static uint8_t g_payLoad9[] = { 0x09, 0x00 };
static uint8_t g_payLoad10[] = { 0x0A, 0x01 };
static uint8_t g_payLoad11[] = { 0x0B, 0x01 };
static uint8_t g_payLoad12[] = { 0x0C, 0x00 };
static uint8_t g_payLoad13[] = { 0x0D, 0x01 };
static uint8_t g_payLoad14[] = { 0x0E, 0x01 };
static uint8_t g_payLoad15[] = { 0x0F, 0x00 };
static uint8_t g_payLoad16[] = { 0x10, 0x00 };
static uint8_t g_payLoad17[] = { 0x11, 0x00 };
static uint8_t g_payLoad18[] = { 0x12, 0x00 };
static uint8_t g_payLoad19[] = { 0x13, 0x1F };
static uint8_t g_payLoad20[] = { 0x14, 0x1F };
static uint8_t g_payLoad21[] = { 0x15, 0x00 };
static uint8_t g_payLoad22[] = { 0x16, 0x00 };
static uint8_t g_payLoad23[] = { 0x17, 0x00 };
static uint8_t g_payLoad24[] = { 0x18, 0x00 };
static uint8_t g_payLoad25[] = { 0x19, 0x00 };
static uint8_t g_payLoad26[] = { 0x1A, 0x00 };
static uint8_t g_payLoad27[] = { 0x1B, 0x00 };
static uint8_t g_payLoad28[] = { 0x1C, 0x00 };
static uint8_t g_payLoad29[] = { 0x1D, 0x00 };
static uint8_t g_payLoad30[] = { 0x1E, 0x40 };
static uint8_t g_payLoad31[] = { 0x1F, 0xC0 };
static uint8_t g_payLoad32[] = { 0x20, 0x06 };
static uint8_t g_payLoad33[] = { 0x21, 0x01 };
static uint8_t g_payLoad34[] = { 0x22, 0x06 };
static uint8_t g_payLoad35[] = { 0x23, 0x01 };
static uint8_t g_payLoad36[] = { 0x24, 0x88 };
static uint8_t g_payLoad37[] = { 0x25, 0x88 };
static uint8_t g_payLoad38[] = { 0x26, 0x00 };
static uint8_t g_payLoad39[] = { 0x27, 0x00 };
static uint8_t g_payLoad40[] = { 0x28, 0x3B };
static uint8_t g_payLoad41[] = { 0x29, 0x03 };
static uint8_t g_payLoad42[] = { 0x2A, 0x00 };
static uint8_t g_payLoad43[] = { 0x2B, 0x00 };
static uint8_t g_payLoad44[] = { 0x2C, 0x00 };
static uint8_t g_payLoad45[] = { 0x2D, 0x00 };
static uint8_t g_payLoad46[] = { 0x2E, 0x00 };
static uint8_t g_payLoad47[] = { 0x2F, 0x00 };
static uint8_t g_payLoad48[] = { 0x30, 0x00 };
static uint8_t g_payLoad49[] = { 0x31, 0x00 };
static uint8_t g_payLoad50[] = { 0x32, 0x00 };
static uint8_t g_payLoad51[] = { 0x33, 0x00 };
static uint8_t g_payLoad52[] = { 0x34, 0x00 };
static uint8_t g_payLoad53[] = { 0x35, 0x00 };
static uint8_t g_payLoad54[] = { 0x36, 0x00 };
static uint8_t g_payLoad55[] = { 0x37, 0x00 };
static uint8_t g_payLoad56[] = { 0x38, 0x00 };
static uint8_t g_payLoad57[] = { 0x39, 0x00 };
static uint8_t g_payLoad58[] = { 0x3A, 0x00 };
static uint8_t g_payLoad59[] = { 0x3B, 0x00 };
static uint8_t g_payLoad60[] = { 0x3C, 0x00 };
static uint8_t g_payLoad61[] = { 0x3D, 0x00 };
static uint8_t g_payLoad62[] = { 0x3E, 0x00 };
static uint8_t g_payLoad63[] = { 0x3F, 0x00 };
static uint8_t g_payLoad64[] = { 0x40, 0x00 };
static uint8_t g_payLoad65[] = { 0x41, 0x00 };
static uint8_t g_payLoad66[] = { 0x42, 0x00 };
static uint8_t g_payLoad67[] = { 0x43, 0x00 };
static uint8_t g_payLoad68[] = { 0x44, 0x00 };
static uint8_t g_payLoad69[] = { 0x50, 0x01 };
static uint8_t g_payLoad70[] = { 0x51, 0x23 };
static uint8_t g_payLoad71[] = { 0x52, 0x45 };
static uint8_t g_payLoad72[] = { 0x53, 0x67 };
static uint8_t g_payLoad73[] = { 0x54, 0x89 };
static uint8_t g_payLoad74[] = { 0x55, 0xAB };
static uint8_t g_payLoad75[] = { 0x56, 0x01 };
static uint8_t g_payLoad76[] = { 0x57, 0x23 };
static uint8_t g_payLoad77[] = { 0x58, 0x45 };
static uint8_t g_payLoad78[] = { 0x59, 0x67 };
static uint8_t g_payLoad79[] = { 0x5A, 0x89 };
static uint8_t g_payLoad80[] = { 0x5B, 0xAB };
static uint8_t g_payLoad81[] = { 0x5C, 0xCD };
static uint8_t g_payLoad82[] = { 0x5D, 0xEF };
static uint8_t g_payLoad83[] = { 0x5E, 0x00 };
static uint8_t g_payLoad84[] = { 0x5F, 0x01 };
static uint8_t g_payLoad85[] = { 0x60, 0x01 };
static uint8_t g_payLoad86[] = { 0x61, 0x06 };
static uint8_t g_payLoad87[] = { 0x62, 0x06 };
static uint8_t g_payLoad88[] = { 0x63, 0x07 };
static uint8_t g_payLoad89[] = { 0x64, 0x07 };
static uint8_t g_payLoad90[] = { 0x65, 0x00 };
static uint8_t g_payLoad91[] = { 0x66, 0x00 };
static uint8_t g_payLoad92[] = { 0x67, 0x02 };
static uint8_t g_payLoad93[] = { 0x68, 0x02 };
static uint8_t g_payLoad94[] = { 0x69, 0x05 };
static uint8_t g_payLoad95[] = { 0x6A, 0x05 };
static uint8_t g_payLoad96[] = { 0x6B, 0x02 };
static uint8_t g_payLoad97[] = { 0x6C, 0x0D };
static uint8_t g_payLoad98[] = { 0x6D, 0x0D };
static uint8_t g_payLoad99[] = { 0x6E, 0x0C };
static uint8_t g_payLoad100[] = { 0x6F, 0x0C };
static uint8_t g_payLoad101[] = { 0x70, 0x0F };
static uint8_t g_payLoad102[] = { 0x71, 0x0F };
static uint8_t g_payLoad103[] = { 0x72, 0x0E };
static uint8_t g_payLoad104[] = { 0x73, 0x0E };
static uint8_t g_payLoad105[] = { 0x74, 0x02 };
static uint8_t g_payLoad106[] = { 0x75, 0x01 };
static uint8_t g_payLoad107[] = { 0x76, 0x01 };
static uint8_t g_payLoad108[] = { 0x77, 0x06 };
static uint8_t g_payLoad109[] = { 0x78, 0x06 };
static uint8_t g_payLoad110[] = { 0x79, 0x07 };
static uint8_t g_payLoad111[] = { 0x7A, 0x07 };
static uint8_t g_payLoad112[] = { 0x7B, 0x00 };
static uint8_t g_payLoad113[] = { 0x7C, 0x00 };
static uint8_t g_payLoad114[] = { 0x7D, 0x02 };
static uint8_t g_payLoad115[] = { 0x7E, 0x02 };
static uint8_t g_payLoad116[] = { 0x7F, 0x05 };
static uint8_t g_payLoad117[] = { 0x80, 0x05 };
static uint8_t g_payLoad118[] = { 0x81, 0x02 };
static uint8_t g_payLoad119[] = { 0x82, 0x0D };
static uint8_t g_payLoad120[] = { 0x83, 0x0D };
static uint8_t g_payLoad121[] = { 0x84, 0x0C };
static uint8_t g_payLoad122[] = { 0x85, 0x0C };
static uint8_t g_payLoad123[] = { 0x86, 0x0F };
static uint8_t g_payLoad124[] = { 0x87, 0x0F };
static uint8_t g_payLoad125[] = { 0x88, 0x0E };
static uint8_t g_payLoad126[] = { 0x89, 0x0E };
static uint8_t g_payLoad127[] = { 0x8A, 0x02 };
static uint8_t g_payLoad128[] = { 0xFF, 0x98, 0x81, 0x04 };
static uint8_t g_payLoad129[] = { 0x6C, 0x15 };
static uint8_t g_payLoad130[] = { 0x6E, 0x2A };
static uint8_t g_payLoad131[] = { 0x6F, 0x33 };
static uint8_t g_payLoad132[] = { 0x8D, 0x1B };
static uint8_t g_payLoad133[] = { 0x87, 0xBA };
static uint8_t g_payLoad134[] = { 0x3A, 0x24 };
static uint8_t g_payLoad135[] = { 0x26, 0x76 };
static uint8_t g_payLoad136[] = { 0xB2, 0xD1 };
static uint8_t g_payLoad137[] = { 0xFF, 0x98, 0x81, 0x01 };
static uint8_t g_payLoad138[] = { 0x22, 0x0A };
static uint8_t g_payLoad139[] = { 0x31, 0x00 };
static uint8_t g_payLoad140[] = { 0x43, 0x66 };
static uint8_t g_payLoad141[] = { 0x53, 0x40 };
static uint8_t g_payLoad142[] = { 0x50, 0x87 };
static uint8_t g_payLoad143[] = { 0x51, 0x82 };
static uint8_t g_payLoad144[] = { 0x60, 0x15 };
static uint8_t g_payLoad145[] = { 0x61, 0x01 };
static uint8_t g_payLoad146[] = { 0x62, 0x0C };
static uint8_t g_payLoad147[] = { 0x63, 0x00 };
static uint8_t g_payLoad148[] = { 0xA0, 0x00 };
static uint8_t g_payLoad149[] = { 0xA1, 0x13 };
static uint8_t g_payLoad150[] = { 0xA2, 0x23 };
static uint8_t g_payLoad151[] = { 0xA3, 0x14 };
static uint8_t g_payLoad152[] = { 0xA4, 0x16 };
static uint8_t g_payLoad153[] = { 0xA5, 0x29 };
static uint8_t g_payLoad154[] = { 0xA6, 0x1E };
static uint8_t g_payLoad155[] = { 0xA7, 0x1D };
static uint8_t g_payLoad156[] = { 0xA8, 0x86 };
static uint8_t g_payLoad157[] = { 0xA9, 0x1E };
static uint8_t g_payLoad158[] = { 0xAA, 0x29 };
static uint8_t g_payLoad159[] = { 0xAB, 0x74 };
static uint8_t g_payLoad160[] = { 0xAC, 0x19 };
static uint8_t g_payLoad161[] = { 0xAD, 0x17 };
static uint8_t g_payLoad162[] = { 0xAE, 0x4B };
static uint8_t g_payLoad163[] = { 0xAF, 0x20 };
static uint8_t g_payLoad164[] = { 0xB0, 0x26 };
static uint8_t g_payLoad165[] = { 0xB1, 0x4C };
static uint8_t g_payLoad166[] = { 0xB2, 0x5D };
static uint8_t g_payLoad167[] = { 0xB3, 0x3F };
static uint8_t g_payLoad168[] = { 0xC0, 0x00 };
static uint8_t g_payLoad169[] = { 0xC1, 0x13 };
static uint8_t g_payLoad170[] = { 0xC2, 0x23 };
static uint8_t g_payLoad171[] = { 0xC3, 0x14 };
static uint8_t g_payLoad172[] = { 0xC4, 0x16 };
static uint8_t g_payLoad173[] = { 0xC5, 0x29 };
static uint8_t g_payLoad174[] = { 0xC6, 0x1E };
static uint8_t g_payLoad175[] = { 0xC7, 0x1D };
static uint8_t g_payLoad176[] = { 0xC8, 0x86 };
static uint8_t g_payLoad177[] = { 0xC9, 0x1E };
static uint8_t g_payLoad178[] = { 0xCA, 0x29 };
static uint8_t g_payLoad179[] = { 0xCB, 0x74 };
static uint8_t g_payLoad180[] = { 0xCC, 0x19 };
static uint8_t g_payLoad181[] = { 0xCD, 0x17 };
static uint8_t g_payLoad182[] = { 0xCE, 0x4B };
static uint8_t g_payLoad183[] = { 0xCF, 0x20 };
static uint8_t g_payLoad184[] = { 0xD0, 0x26 };
static uint8_t g_payLoad185[] = { 0xD1, 0x4C };
static uint8_t g_payLoad186[] = { 0xD2, 0x5D };
static uint8_t g_payLoad187[] = { 0xD3, 0x3F };
static uint8_t g_payLoad188[] = { 0xFF, 0x98, 0x81, 0x00 };
static uint8_t g_payLoad189[] = { 0x11, 0x00 };
static uint8_t g_payLoad190[] = { 0x29, 0x00 };
static uint8_t g_payLoad191[] = { 0x35, 0x00 };

static struct DsiCmdDesc g_panelOnCode[] = {
    { 0x39, 0x00, sizeof(g_payLoad0), g_payLoad0 },
    { 0x23, 0x00, sizeof(g_payLoad1), g_payLoad1 },
    { 0x23, 0x00, sizeof(g_payLoad2), g_payLoad2 },
    { 0x23, 0x00, sizeof(g_payLoad3), g_payLoad3 },
    { 0x23, 0x00, sizeof(g_payLoad4), g_payLoad4 },
    { 0x23, 0x00, sizeof(g_payLoad5), g_payLoad5 },
    { 0x23, 0x00, sizeof(g_payLoad6), g_payLoad6 },
    { 0x23, 0x00, sizeof(g_payLoad7), g_payLoad7 },
    { 0x23, 0x00, sizeof(g_payLoad8), g_payLoad8 },
    { 0x23, 0x00, sizeof(g_payLoad9), g_payLoad9 },
    { 0x23, 0x00, sizeof(g_payLoad10), g_payLoad10 },
    { 0x23, 0x00, sizeof(g_payLoad11), g_payLoad11 },
    { 0x23, 0x00, sizeof(g_payLoad12), g_payLoad12 },
    { 0x23, 0x00, sizeof(g_payLoad13), g_payLoad13 },
    { 0x23, 0x00, sizeof(g_payLoad14), g_payLoad14 },
    { 0x23, 0x00, sizeof(g_payLoad15), g_payLoad15 },
    { 0x23, 0x00, sizeof(g_payLoad16), g_payLoad16 },
    { 0x23, 0x00, sizeof(g_payLoad17), g_payLoad17 },
    { 0x23, 0x00, sizeof(g_payLoad18), g_payLoad18 },
    { 0x23, 0x00, sizeof(g_payLoad19), g_payLoad19 },
    { 0x23, 0x00, sizeof(g_payLoad20), g_payLoad20 },
    { 0x23, 0x00, sizeof(g_payLoad21), g_payLoad21 },
    { 0x23, 0x00, sizeof(g_payLoad22), g_payLoad22 },
    { 0x23, 0x00, sizeof(g_payLoad23), g_payLoad23 },
    { 0x23, 0x00, sizeof(g_payLoad24), g_payLoad24 },
    { 0x23, 0x00, sizeof(g_payLoad25), g_payLoad25 },
    { 0x23, 0x00, sizeof(g_payLoad26), g_payLoad26 },
    { 0x23, 0x00, sizeof(g_payLoad27), g_payLoad27 },
    { 0x23, 0x00, sizeof(g_payLoad28), g_payLoad28 },
    { 0x23, 0x00, sizeof(g_payLoad29), g_payLoad29 },
    { 0x23, 0x00, sizeof(g_payLoad30), g_payLoad30 },
    { 0x23, 0x00, sizeof(g_payLoad31), g_payLoad31 },
    { 0x23, 0x00, sizeof(g_payLoad32), g_payLoad32 },
    { 0x23, 0x00, sizeof(g_payLoad33), g_payLoad33 },
    { 0x23, 0x00, sizeof(g_payLoad34), g_payLoad34 },
    { 0x23, 0x00, sizeof(g_payLoad35), g_payLoad35 },
    { 0x23, 0x00, sizeof(g_payLoad36), g_payLoad36 },
    { 0x23, 0x00, sizeof(g_payLoad37), g_payLoad37 },
    { 0x23, 0x00, sizeof(g_payLoad38), g_payLoad38 },
    { 0x23, 0x00, sizeof(g_payLoad39), g_payLoad39 },
    { 0x23, 0x00, sizeof(g_payLoad40), g_payLoad40 },
    { 0x23, 0x00, sizeof(g_payLoad41), g_payLoad41 },
    { 0x23, 0x00, sizeof(g_payLoad42), g_payLoad42 },
    { 0x23, 0x00, sizeof(g_payLoad43), g_payLoad43 },
    { 0x23, 0x00, sizeof(g_payLoad44), g_payLoad44 },
    { 0x23, 0x00, sizeof(g_payLoad45), g_payLoad45 },
    { 0x23, 0x00, sizeof(g_payLoad46), g_payLoad46 },
    { 0x23, 0x00, sizeof(g_payLoad47), g_payLoad47 },
    { 0x23, 0x00, sizeof(g_payLoad48), g_payLoad48 },
    { 0x23, 0x00, sizeof(g_payLoad49), g_payLoad49 },
    { 0x23, 0x00, sizeof(g_payLoad50), g_payLoad50 },
    { 0x23, 0x00, sizeof(g_payLoad51), g_payLoad51 },
    { 0x23, 0x00, sizeof(g_payLoad52), g_payLoad52 },
    { 0x23, 0x00, sizeof(g_payLoad53), g_payLoad53 },
    { 0x23, 0x00, sizeof(g_payLoad54), g_payLoad54 },
    { 0x23, 0x00, sizeof(g_payLoad55), g_payLoad55 },
    { 0x23, 0x00, sizeof(g_payLoad56), g_payLoad56 },
    { 0x23, 0x00, sizeof(g_payLoad57), g_payLoad57 },
    { 0x23, 0x00, sizeof(g_payLoad58), g_payLoad58 },
    { 0x23, 0x00, sizeof(g_payLoad59), g_payLoad59 },
    { 0x23, 0x00, sizeof(g_payLoad60), g_payLoad60 },
    { 0x23, 0x00, sizeof(g_payLoad61), g_payLoad61 },
    { 0x23, 0x00, sizeof(g_payLoad62), g_payLoad62 },
    { 0x23, 0x00, sizeof(g_payLoad63), g_payLoad63 },
    { 0x23, 0x00, sizeof(g_payLoad64), g_payLoad64 },
    { 0x23, 0x00, sizeof(g_payLoad65), g_payLoad65 },
    { 0x23, 0x00, sizeof(g_payLoad66), g_payLoad66 },
    { 0x23, 0x00, sizeof(g_payLoad67), g_payLoad67 },
    { 0x23, 0x00, sizeof(g_payLoad68), g_payLoad68 },
    { 0x23, 0x00, sizeof(g_payLoad69), g_payLoad69 },
    { 0x23, 0x00, sizeof(g_payLoad70), g_payLoad70 },
    { 0x23, 0x00, sizeof(g_payLoad71), g_payLoad71 },
    { 0x23, 0x00, sizeof(g_payLoad72), g_payLoad72 },
    { 0x23, 0x00, sizeof(g_payLoad73), g_payLoad73 },
    { 0x23, 0x00, sizeof(g_payLoad74), g_payLoad74 },
    { 0x23, 0x00, sizeof(g_payLoad75), g_payLoad75 },
    { 0x23, 0x00, sizeof(g_payLoad76), g_payLoad76 },
    { 0x23, 0x00, sizeof(g_payLoad77), g_payLoad77 },
    { 0x23, 0x00, sizeof(g_payLoad78), g_payLoad78 },
    { 0x23, 0x00, sizeof(g_payLoad79), g_payLoad79 },
    { 0x23, 0x00, sizeof(g_payLoad80), g_payLoad80 },
    { 0x23, 0x00, sizeof(g_payLoad81), g_payLoad81 },
    { 0x23, 0x00, sizeof(g_payLoad82), g_payLoad82 },
    { 0x23, 0x00, sizeof(g_payLoad83), g_payLoad83 },
    { 0x23, 0x00, sizeof(g_payLoad84), g_payLoad84 },
    { 0x23, 0x00, sizeof(g_payLoad85), g_payLoad85 },
    { 0x23, 0x00, sizeof(g_payLoad86), g_payLoad86 },
    { 0x23, 0x00, sizeof(g_payLoad87), g_payLoad87 },
    { 0x23, 0x00, sizeof(g_payLoad88), g_payLoad88 },
    { 0x23, 0x00, sizeof(g_payLoad89), g_payLoad89 },
    { 0x23, 0x00, sizeof(g_payLoad90), g_payLoad90 },
    { 0x23, 0x00, sizeof(g_payLoad91), g_payLoad91 },
    { 0x23, 0x00, sizeof(g_payLoad92), g_payLoad92 },
    { 0x23, 0x00, sizeof(g_payLoad93), g_payLoad93 },
    { 0x23, 0x00, sizeof(g_payLoad94), g_payLoad94 },
    { 0x23, 0x00, sizeof(g_payLoad95), g_payLoad95 },
    { 0x23, 0x00, sizeof(g_payLoad96), g_payLoad96 },
    { 0x23, 0x00, sizeof(g_payLoad97), g_payLoad97 },
    { 0x23, 0x00, sizeof(g_payLoad98), g_payLoad98 },
    { 0x23, 0x00, sizeof(g_payLoad99), g_payLoad99 },
    { 0x23, 0x00, sizeof(g_payLoad100), g_payLoad100 },
    { 0x23, 0x00, sizeof(g_payLoad101), g_payLoad101 },
    { 0x23, 0x00, sizeof(g_payLoad102), g_payLoad102 },
    { 0x23, 0x00, sizeof(g_payLoad103), g_payLoad103 },
    { 0x23, 0x00, sizeof(g_payLoad104), g_payLoad104 },
    { 0x23, 0x00, sizeof(g_payLoad105), g_payLoad105 },
    { 0x23, 0x00, sizeof(g_payLoad106), g_payLoad106 },
    { 0x23, 0x00, sizeof(g_payLoad107), g_payLoad107 },
    { 0x23, 0x00, sizeof(g_payLoad108), g_payLoad108 },
    { 0x23, 0x00, sizeof(g_payLoad109), g_payLoad109 },
    { 0x23, 0x00, sizeof(g_payLoad110), g_payLoad110 },
    { 0x23, 0x00, sizeof(g_payLoad111), g_payLoad111 },
    { 0x23, 0x00, sizeof(g_payLoad112), g_payLoad112 },
    { 0x23, 0x00, sizeof(g_payLoad113), g_payLoad113 },
    { 0x23, 0x00, sizeof(g_payLoad114), g_payLoad114 },
    { 0x23, 0x00, sizeof(g_payLoad115), g_payLoad115 },
    { 0x23, 0x00, sizeof(g_payLoad116), g_payLoad116 },
    { 0x23, 0x00, sizeof(g_payLoad117), g_payLoad117 },
    { 0x23, 0x00, sizeof(g_payLoad118), g_payLoad118 },
    { 0x23, 0x00, sizeof(g_payLoad119), g_payLoad119 },
    { 0x23, 0x00, sizeof(g_payLoad120), g_payLoad120 },
    { 0x23, 0x00, sizeof(g_payLoad121), g_payLoad121 },
    { 0x23, 0x00, sizeof(g_payLoad122), g_payLoad122 },
    { 0x23, 0x00, sizeof(g_payLoad123), g_payLoad123 },
    { 0x23, 0x00, sizeof(g_payLoad124), g_payLoad124 },
    { 0x23, 0x00, sizeof(g_payLoad125), g_payLoad125 },
    { 0x23, 0x00, sizeof(g_payLoad126), g_payLoad126 },
    { 0x23, 0x00, sizeof(g_payLoad127), g_payLoad127 },
    { 0x39, 0x00, sizeof(g_payLoad128), g_payLoad128 },
    { 0x23, 0x00, sizeof(g_payLoad129), g_payLoad129 },
    { 0x23, 0x00, sizeof(g_payLoad130), g_payLoad130 },
    { 0x23, 0x00, sizeof(g_payLoad131), g_payLoad131 },
    { 0x23, 0x00, sizeof(g_payLoad132), g_payLoad132 },
    { 0x23, 0x00, sizeof(g_payLoad133), g_payLoad133 },
    { 0x23, 0x00, sizeof(g_payLoad134), g_payLoad134 },
    { 0x23, 0x00, sizeof(g_payLoad135), g_payLoad135 },
    { 0x23, 0x00, sizeof(g_payLoad136), g_payLoad136 },
    { 0x39, 0x00, sizeof(g_payLoad137), g_payLoad137 },
    { 0x23, 0x00, sizeof(g_payLoad138), g_payLoad138 },
    { 0x23, 0x00, sizeof(g_payLoad139), g_payLoad139 },
    { 0x23, 0x00, sizeof(g_payLoad140), g_payLoad140 },
    { 0x23, 0x00, sizeof(g_payLoad141), g_payLoad141 },
    { 0x23, 0x00, sizeof(g_payLoad142), g_payLoad142 },
    { 0x23, 0x00, sizeof(g_payLoad143), g_payLoad143 },
    { 0x23, 0x00, sizeof(g_payLoad144), g_payLoad144 },
    { 0x23, 0x00, sizeof(g_payLoad145), g_payLoad145 },
    { 0x23, 0x00, sizeof(g_payLoad146), g_payLoad146 },
    { 0x23, 0x00, sizeof(g_payLoad147), g_payLoad147 },
    { 0x23, 0x00, sizeof(g_payLoad148), g_payLoad148 },
    { 0x23, 0x00, sizeof(g_payLoad149), g_payLoad149 },
    { 0x23, 0x00, sizeof(g_payLoad150), g_payLoad150 },
    { 0x23, 0x00, sizeof(g_payLoad151), g_payLoad151 },
    { 0x23, 0x00, sizeof(g_payLoad152), g_payLoad152 },
    { 0x23, 0x00, sizeof(g_payLoad153), g_payLoad153 },
    { 0x23, 0x00, sizeof(g_payLoad154), g_payLoad154 },
    { 0x23, 0x00, sizeof(g_payLoad155), g_payLoad155 },
    { 0x23, 0x00, sizeof(g_payLoad156), g_payLoad156 },
    { 0x23, 0x00, sizeof(g_payLoad157), g_payLoad157 },
    { 0x23, 0x00, sizeof(g_payLoad158), g_payLoad158 },
    { 0x23, 0x00, sizeof(g_payLoad159), g_payLoad159 },
    { 0x23, 0x00, sizeof(g_payLoad160), g_payLoad160 },
    { 0x23, 0x00, sizeof(g_payLoad161), g_payLoad161 },
    { 0x23, 0x00, sizeof(g_payLoad162), g_payLoad162 },
    { 0x23, 0x00, sizeof(g_payLoad163), g_payLoad163 },
    { 0x23, 0x00, sizeof(g_payLoad164), g_payLoad164 },
    { 0x23, 0x00, sizeof(g_payLoad165), g_payLoad165 },
    { 0x23, 0x00, sizeof(g_payLoad166), g_payLoad166 },
    { 0x23, 0x00, sizeof(g_payLoad167), g_payLoad167 },
    { 0x23, 0x00, sizeof(g_payLoad168), g_payLoad168 },
    { 0x23, 0x00, sizeof(g_payLoad169), g_payLoad169 },
    { 0x23, 0x00, sizeof(g_payLoad170), g_payLoad170 },
    { 0x23, 0x00, sizeof(g_payLoad171), g_payLoad171 },
    { 0x23, 0x00, sizeof(g_payLoad172), g_payLoad172 },
    { 0x23, 0x00, sizeof(g_payLoad173), g_payLoad173 },
    { 0x23, 0x00, sizeof(g_payLoad174), g_payLoad174 },
    { 0x23, 0x00, sizeof(g_payLoad175), g_payLoad175 },
    { 0x23, 0x00, sizeof(g_payLoad176), g_payLoad176 },
    { 0x23, 0x00, sizeof(g_payLoad177), g_payLoad177 },
    { 0x23, 0x00, sizeof(g_payLoad178), g_payLoad178 },
    { 0x23, 0x00, sizeof(g_payLoad179), g_payLoad179 },
    { 0x23, 0x00, sizeof(g_payLoad180), g_payLoad180 },
    { 0x23, 0x00, sizeof(g_payLoad181), g_payLoad181 },
    { 0x23, 0x00, sizeof(g_payLoad182), g_payLoad182 },
    { 0x23, 0x00, sizeof(g_payLoad183), g_payLoad183 },
    { 0x23, 0x00, sizeof(g_payLoad184), g_payLoad184 },
    { 0x23, 0x00, sizeof(g_payLoad185), g_payLoad185 },
    { 0x23, 0x00, sizeof(g_payLoad186), g_payLoad186 },
    { 0x23, 0x00, sizeof(g_payLoad187), g_payLoad187 },
    { 0x39, 0x00, sizeof(g_payLoad188), g_payLoad188 },
    { 0x23, 0x78, sizeof(g_payLoad189), g_payLoad189 },
    { 0x23, 0x0A, sizeof(g_payLoad190), g_payLoad190 },
    { 0x23, 0x00, sizeof(g_payLoad191), g_payLoad191 },
};

/* panel off command payload */
static uint8_t g_offpayLoad0[] = { 0x11 };
static uint8_t g_offpayLoad1[] = { 0x29 };

static struct DsiCmdDesc g_panelOffCode[] = {
    { 0x13, 0x0A, sizeof(g_offpayLoad0), g_offpayLoad0 },
    { 0x13, 0x78, sizeof(g_offpayLoad1), g_offpayLoad1 },
};

#define SET_GPIO_DELAY_5MS   5
#define ILI9881C_MIPI_DATA_LANES   4

static int Ili9881cBoeSendCmds(struct mipi_dsi_device *dsi,
    const struct DsiCmdDesc *cmds, int size)
{
    int32_t i;

    if (dsi == NULL) {
        return -EINVAL;
    }

    for (i = 0; i < size; i++) {
        mipi_dsi_generic_write(dsi, cmds[i].payload, cmds[i].dataLen);
        if (cmds[i].delay) {
            OsalMSleep(cmds[i].delay);
        }
    }
    return HDF_SUCCESS;
}

static struct Ili9881cBoeDev *ToIli9881cBoeDev(const struct PanelData *panel)
{
    return (struct Ili9881cBoeDev *)panel->object->priv;
}

static SetGpioState(uint16_t gpio, uint16_t dir, uint16_t level, uint32_t delay)
{
    int32_t ret;

    ret = GpioSetDir(gpio, dir);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s GpioSetDir failed, ret:%d", __func__, ret);
        return HDF_FAILURE;
    }
    ret = GpioWrite(gpio, level);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s GpioWrite failed, ret:%d", __func__, ret);
        return HDF_FAILURE;
    }
    OsalMSleep(delay);
    return HDF_SUCCESS;
}

static int32_t Ili9881cBoePrepare(const struct Ili9881cBoeDev *ili9881cBoeDev)
{
    int32_t i;
    int32_t ret;
    int32_t items;
    struct GpioTiming *timing = NULL;

    HDF_LOGI("%s()", __func__);
    ret = regulator_enable(ili9881cBoeDev->supply);
    if (ret < 0) {
        HDF_LOGE("regulator_enable failed");
    }
    ret = SetGpioState(ili9881cBoeDev->avddGpio, GPIO_DIR_OUT, GPIO_VAL_HIGH, SET_GPIO_DELAY_5MS); /* delay 5ms */
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s SetGpioState failed, gpio:%d", __func__, ili9881cBoeDev->avddGpio);
        return HDF_FAILURE;
    }
    ret = SetGpioState(ili9881cBoeDev->aveeGpio, GPIO_DIR_OUT, GPIO_VAL_HIGH, SET_GPIO_DELAY_5MS);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s SetGpioState failed, gpio:%d", __func__, ili9881cBoeDev->aveeGpio);
        return HDF_FAILURE;
    }
    ret = SetGpioState(ili9881cBoeDev->vghlGpio, GPIO_DIR_OUT, GPIO_VAL_HIGH, SET_GPIO_DELAY_5MS);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s SetGpioState failed, gpio:%d", __func__, ili9881cBoeDev->vghlGpio);
        return HDF_FAILURE;
    }
    ret = SetGpioState(ili9881cBoeDev->tsrstGpio, GPIO_DIR_OUT, GPIO_VAL_HIGH, SET_GPIO_DELAY_5MS);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s SetGpioState failed, gpio:%d", __func__, ili9881cBoeDev->tsrstGpio);
        return HDF_FAILURE;
    }
    ret = GpioSetDir(ili9881cBoeDev->resetGpio, GPIO_DIR_OUT);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioSetDir failed, ret:%d", ret);
        return HDF_FAILURE;
    }
    items = ili9881cBoeDev->rstOnSeq.items;
    timing = ili9881cBoeDev->rstOnSeq.timing;
    for (i = 0; i < items; i++) {
        GpioWrite(ili9881cBoeDev->resetGpio, timing[i].level);
        OsalMSleep(timing[i].delay);
    }
    return HDF_SUCCESS;
}

static int32_t Ili9881cBoeUnprepare(const struct Ili9881cBoeDev *ili9881cBoeDev)
{
    int32_t i;
    int32_t ret;
    int32_t items;
    struct GpioTiming *timing = NULL;

    HDF_LOGI("%s()", __func__);
    ret = SetGpioState(ili9881cBoeDev->avddGpio, GPIO_DIR_OUT, GPIO_VAL_LOW, SET_GPIO_DELAY_5MS); /* delay 5ms */
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s SetGpioState failed, gpio:%d", __func__, ili9881cBoeDev->avddGpio);
        return HDF_FAILURE;
    }
    ret = SetGpioState(ili9881cBoeDev->aveeGpio, GPIO_DIR_OUT, GPIO_VAL_LOW, SET_GPIO_DELAY_5MS);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s SetGpioState failed, gpio:%d", __func__, ili9881cBoeDev->aveeGpio);
        return HDF_FAILURE;
    }
    ret = SetGpioState(ili9881cBoeDev->vghlGpio, GPIO_DIR_OUT, GPIO_VAL_LOW, SET_GPIO_DELAY_5MS);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s SetGpioState failed, gpio:%d", __func__, ili9881cBoeDev->vghlGpio);
        return HDF_FAILURE;
    }
    ret = SetGpioState(ili9881cBoeDev->tsrstGpio, GPIO_DIR_OUT, GPIO_VAL_LOW, SET_GPIO_DELAY_5MS);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s SetGpioState failed, gpio:%d", __func__, ili9881cBoeDev->tsrstGpio);
        return HDF_FAILURE;
    }
    ret = GpioSetDir(ili9881cBoeDev->resetGpio, GPIO_DIR_OUT);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioSetDir failed, ret:%d", ret);
        return HDF_FAILURE;
    }
    items = ili9881cBoeDev->rstOffSeq.items;
    timing = ili9881cBoeDev->rstOffSeq.timing;
    for (i = 0; i < items; i++) {
        GpioWrite(ili9881cBoeDev->resetGpio, timing[i].level);
        OsalMSleep(timing[i].delay);
    }
    regulator_disable(ili9881cBoeDev->supply);
    return HDF_SUCCESS;
}

static int32_t Ili9881cBoeOn(struct PanelData *panel)
{
    int32_t ret;
    struct Ili9881cBoeDev *ili9881cBoeDev = NULL;

    HDF_LOGI("%s()", __func__);
    ili9881cBoeDev = ToIli9881cBoeDev(panel);
    ret = Ili9881cBoePrepare(ili9881cBoeDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s Ili9881cBoePrepare failed", __func__);
        return HDF_FAILURE;
    }
    ret = Ili9881cBoeSendCmds(ili9881cBoeDev->dsiDev, g_panelOnCode,
                              sizeof(g_panelOnCode) / sizeof(g_panelOnCode[0]));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s Ili9881cBoeSendCmds failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t Ili9881cBoeOff(struct PanelData *panel)
{
    int32_t ret;
    struct Ili9881cBoeDev *ili9881cBoeDev = NULL;

    ili9881cBoeDev = ToIli9881cBoeDev(panel);
    HDF_LOGI(" %s line = %d", __func__, __LINE__);
    ret = Ili9881cBoeSendCmds(ili9881cBoeDev->dsiDev, g_panelOffCode,
                              sizeof(g_panelOffCode) / sizeof(g_panelOffCode[0]));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s Ili9881cBoeSendCmds failed", __func__);
        return HDF_FAILURE;
    }
    ret = Ili9881cBoeUnprepare(ili9881cBoeDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s Ili9881cBoeUnprepare failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t Ili9881cBoeInit(struct PanelData *panel)
{
    return 0;
}

#define BLK_PWM_INDEX             2
#define PWM_MAX_PERIOD            40000
/* backlight setting */
#define MIN_LEVEL                 0
#define MAX_LEVEL                 255
#define DEFAULT_LEVEL             127

static struct PanelInfo g_panelInfo = {
    .width = 800,           /* width */
    .height = 1280,         /* height */
    .hbp = 80,              /* horizontal back porch */
    .hfp = 80,              /* horizontal front porch */
    .hsw = 20,              /* horizontal sync width */
    .vbp = 12,              /* vertical back porch */
    .vfp = 20,              /* vertical front porch */
    .vsw = 4,               /* vertical sync width */
    .clockFreq = 76800000,  /* clock */
    .pWidth = 150,          /* physical width */
    .pHeight = 240,         /* physical height */
    .blk = { BLK_PWM, MIN_LEVEL, MAX_LEVEL, DEFAULT_LEVEL },
};

static struct GpioTiming g_rstOnSeq[] = {
    {1, 5},   /* high, delay 5ms */
    {0, 20},  /* low, delay 20ms */
    {1, 30},  /* high, delay 30ms */
};

static struct GpioTiming g_rstOffSeq = {0, 10}; /* low, delay 10ms */

static void Ili9881cBoeResInit(struct Ili9881cBoeDev *ili9881cBoeDev)
{
    ili9881cBoeDev->avddGpio = AVDD_GPIO;
    ili9881cBoeDev->aveeGpio = AVEE_GPIO;
    ili9881cBoeDev->vghlGpio = VGHL_GPIO;
    ili9881cBoeDev->tsrstGpio = TSRST_GPIO;
    ili9881cBoeDev->resetGpio = RESET_GPIO;
    ili9881cBoeDev->rstOnSeq.items = sizeof(g_rstOnSeq) / sizeof(struct GpioTiming);
    ili9881cBoeDev->rstOnSeq.timing = g_rstOnSeq;
    ili9881cBoeDev->rstOffSeq.items = 1; /* number of reset off sequence */
    ili9881cBoeDev->rstOffSeq.timing = &g_rstOffSeq;
    ili9881cBoeDev->dsiDev->lanes = ILI9881C_MIPI_DATA_LANES; /* number of active data lanes */
    ili9881cBoeDev->dsiDev->format = MIPI_DSI_FMT_RGB888; /* pixel format for video mode */
    ili9881cBoeDev->dsiDev->mode_flags = (MIPI_DSI_MODE_VIDEO \
                                        | MIPI_DSI_MODE_VIDEO_BURST); /* DSI operation mode related flags */
    ili9881cBoeDev->panel.info = &g_panelInfo;
    ili9881cBoeDev->panel.init = Ili9881cBoeInit;
    ili9881cBoeDev->panel.on = Ili9881cBoeOn;
    ili9881cBoeDev->panel.off = Ili9881cBoeOff;
    ili9881cBoeDev->panel.priv = ili9881cBoeDev->dsiDev;
}

int32_t Ili9881cBoeEntryInit(struct HdfDeviceObject *object)
{
    struct device_node *panelNode = NULL;
    struct Ili9881cBoeDev *ili9881cBoeDev = NULL;

    ili9881cBoeDev = (struct Ili9881cBoeDev *)OsalMemCalloc(sizeof(struct Ili9881cBoeDev));
    if (ili9881cBoeDev == NULL) {
        HDF_LOGE("%s ili9881cBoeDev malloc fail", __func__);
        return HDF_FAILURE;
    }
    panelNode = of_find_compatible_node(NULL, NULL, "sprd,generic-mipi-panel");
    if (panelNode == NULL) {
        HDF_LOGE("%s of_find_compatible_node fail", __func__);
        goto FAIL;
    }
    ili9881cBoeDev->dsiDev = of_find_mipi_dsi_device_by_node(panelNode);
    if (ili9881cBoeDev->dsiDev == NULL) {
        HDF_LOGE("%s of_find_mipi_dsi_device_by_node fail", __func__);
        goto FAIL;
    }
    ili9881cBoeDev->supply = devm_regulator_get(&ili9881cBoeDev->dsiDev->dev, "power");
    if (ili9881cBoeDev->supply == NULL) {
        HDF_LOGE("Get regulator fail");
        goto FAIL;
    }
    Ili9881cBoeResInit(ili9881cBoeDev);
    ili9881cBoeDev->panel.blDev = GetBacklightDev("hdf_pwm");
    if (ili9881cBoeDev->panel.blDev == NULL) {
        HDF_LOGE("%s GetBacklightDev fail", __func__);
        goto FAIL;
    }
    ili9881cBoeDev->panel.object = object;
    object->priv = ili9881cBoeDev;
    if (RegisterPanel(&ili9881cBoeDev->panel) == HDF_SUCCESS) {
        HDF_LOGI("%s success", __func__);
        return HDF_SUCCESS;
    }

FAIL:
    OsalMemFree(ili9881cBoeDev);
    return HDF_FAILURE;
}

struct HdfDriverEntry g_ili9881cBoeDevEntry = {
    .moduleVersion = 1,
    .moduleName = "LCD_ILI9881CBOE",
    .Init = Ili9881cBoeEntryInit,
};

HDF_INIT(g_ili9881cBoeDevEntry);
