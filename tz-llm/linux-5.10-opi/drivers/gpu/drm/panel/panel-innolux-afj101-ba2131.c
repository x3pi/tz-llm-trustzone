// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019-2020 Kali Prasad <kprasadvnsi@protonmail.com>
 */

#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>

#define _INIT_CMD(...) { \
	.len = sizeof((char[]){__VA_ARGS__}), \
	.data = (char[]){__VA_ARGS__} }

struct panel_init_cmd {
	u8 dtype;
	u8 wait;
	u8 dlen;
	const char *data;
};

static const char * const regulator_names[] = {
	"dvdd",
	"avdd",
	"cvdd"
};

struct afj101_ba2131 {
	struct drm_panel	panel;
	struct mipi_dsi_device	*dsi;

	struct regulator_bulk_data supplies[ARRAY_SIZE(regulator_names)];
	struct gpio_desc	*reset;
	struct gpio_desc	*enable;
};

static inline struct afj101_ba2131 *panel_to_sl101_pn27d1665(struct drm_panel *panel)
{
	return container_of(panel, struct afj101_ba2131, panel);
}

/*
 * Display manufacturer failed to provide init sequencing according to
 * https://chromium-review.googlesource.com/c/chromiumos/third_party/coreboot/+/892065/
 * so the init sequence stems from a register dump of a working panel.
 */
static const struct panel_init_cmd afj101_ba2131_init_cmds[] = {
	{ .dtype = 0x39, .wait =  0x00, .dlen =  0x04, .data = (char[]){0xB9,0xFF,0x83,0x99}},
	{ .dtype = 0x15, .wait =  0x00, .dlen =  0x02, .data = (char[]){0xBA,0x43}},
	{ .dtype = 0x15, .wait =  0x00, .dlen =  0x02, .data = (char[]){0xD2,0x44}},
	{ .dtype = 0x39, .wait =  0x00, .dlen =  0x0D, .data = (char[]){0xB1,0x00,0x7C,0x34,0x34,0x44,0x09,0x22,0x22,0x71,0xF1,0xB2,0x4A}},
	{ .dtype = 0x39, .wait =  0x00, .dlen =  0x0B, .data = (char[]){0xB2,0x00,0x80,0x00,0x7F,0x05,0x07,0x23,0x4D,0x21,0x01}},
	{ .dtype = 0x39, .wait =  0x00, .dlen =  0x29, .data = (char[]){0xB4,0x00,0xFF,0x02,0x40,0x02,0x40,0x00,0x00,0x06,0x00,0x01,0x02,0x00,0x0F,0x01,0x02,0x05,0x20,0x00,0x04,0x44,0x02,0x40,0x02,0x40,0x00,0x00,0x06,0x00,0x01,0x02,0x00,0x0F,0x01,0x02,0x05,0x00,0x00,0x04,0x44}},
	{ .dtype = 0x39, .wait =  0x05, .dlen =  0x20, .data = (char[]){0xD3,0x00,0x01,0x00,0x00,0x00,0x06,0x00,0x00,0x10,0x04,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x05,0x05,0x07,0x00,0x00,0x00,0x05,0x08}}, 
	{ .dtype = 0x39, .wait =  0x05, .dlen =  0x21, .data = (char[]){0xD5,0x18,0x18,0x19,0x19,0x18,0x18,0x21,0x20,0x01,0x00,0x07,0x06,0x05,0x04,0x03,0x02,0x18,0x18,0x18,0x18,0x18,0x18,0x30,0x30,0x31,0x31,0x32,0x32,0x18,0x18,0x18,0x18}}, 
	{ .dtype = 0x39, .wait =  0x05, .dlen =  0x21, .data = (char[]){0xD6,0x18,0x18,0x19,0x19,0x40,0x40,0x20,0x21,0x06,0x07,0x00,0x01,0x02,0x03,0x04,0x05,0x40,0x40,0x40,0x40,0x40,0x40,0x30,0x30,0x31,0x31,0x32,0x32,0x40,0x40,0x40,0x40}},
	{ .dtype = 0x39, .wait =  0x00, .dlen =  0x31, .data = (char[]){0xD8,0xA2,0xAA,0x02,0xA0,0xA2,0xA8,0x02,0xA0,0xB0,0x00,0x00,0x00,0xB0,0x00,0x00,0x00,0xB0,0x00,0x00,0x00,0xB0,0x00,0x00,0x00,0xE2,0xAA,0x03,0xF0,0xE2,0xAA,0x03,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE2,0xAA,0x03,0xF0,0xE2,0xAA,0x03,0xF0}},
	{ .dtype = 0x39, .wait =  0x00, .dlen =  0x03, .data = (char[]){0xB6,0x29,0x29}},
	{ .dtype = 0x39, .wait =  0x00, .dlen =  0x2B, .data = (char[]){0xE0,0x01,0x06,0x06,0x2A,0x2F,0x3E,0x0F,0x3A,0x05,0x09,0x0F,0x13,0x15,0x14,0x15,0x12,0x18,0x07,0x16,0x07,0x14,0x01,0x06,0x06,0x2A,0x2F,0x3E,0x0F,0x3A,0x05,0x09,0x0F,0x13,0x15,0x14,0x15,0x12,0x18,0x07,0x16,0x07,0x14}},
	{ .dtype = 0x05, .wait =  0x00, .dlen =  0x01, .data = (char[]){0x21}}, 
	{ .dtype = 0x15, .wait =  0x00, .dlen =  0x02, .data = (char[]){0x36,0x02}}, 
	{ .dtype = 0x05, .wait =  0xff, .dlen =  0x01, .data = (char[]){0x11}},
	{ .dtype = 0x05, .wait =  0xff, .dlen =  0x01, .data = (char[]){0x29}},
};


static int afj101_ba2131_prepare(struct drm_panel *panel)
{
	struct afj101_ba2131 *ctx = panel_to_sl101_pn27d1665(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	unsigned int i;
	int ret;

	gpiod_set_value(ctx->enable, 1);
	msleep(25);

	gpiod_set_value(ctx->reset, 1);
	msleep(25);

	gpiod_set_value(ctx->reset, 0);
	msleep(200);

	for (i = 0; i < ARRAY_SIZE(afj101_ba2131_init_cmds); i++) {
		const struct panel_init_cmd *cmd = &afj101_ba2131_init_cmds[i];

		switch (cmd->dtype) {
			case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
			case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
			case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
			case MIPI_DSI_GENERIC_LONG_WRITE:
				ret = mipi_dsi_generic_write(dsi, cmd->data,
								cmd->dlen);
				break;
			case MIPI_DSI_DCS_SHORT_WRITE:
			case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
			case MIPI_DSI_DCS_LONG_WRITE:
				ret = mipi_dsi_dcs_write_buffer(dsi, cmd->data,
								cmd->dlen);
				break;
			default:
				return -EINVAL;
		}

		if (ret < 0)
			goto powerdown;

		if (cmd->wait)
				msleep(cmd->wait);
	}

	return 0;

powerdown:
	gpiod_set_value(ctx->reset, 1);
	msleep(50);

	// return regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	return 0;
}

static int afj101_ba2131_enable(struct drm_panel *panel)
{
	struct afj101_ba2131 *ctx = panel_to_sl101_pn27d1665(panel);
	int ret;

	msleep(150);

	ret = mipi_dsi_dcs_set_display_on(ctx->dsi);
	if (ret < 0)
		return ret;

	msleep(50);

	return 0;
}

static int afj101_ba2131_disable(struct drm_panel *panel)
{
	struct afj101_ba2131 *ctx = panel_to_sl101_pn27d1665(panel);

	return mipi_dsi_dcs_set_display_off(ctx->dsi);
}

static int afj101_ba2131_unprepare(struct drm_panel *panel)
{
	struct afj101_ba2131 *ctx = panel_to_sl101_pn27d1665(panel);
	int ret;

	ret = mipi_dsi_dcs_set_display_off(ctx->dsi);
	if (ret < 0)
		dev_err(panel->dev, "failed to set display off: %d\n", ret);

	ret = mipi_dsi_dcs_enter_sleep_mode(ctx->dsi);
	if (ret < 0)
		dev_err(panel->dev, "failed to enter sleep mode: %d\n", ret);

	msleep(200);

	gpiod_set_value(ctx->reset, 1);
	msleep(20);

	gpiod_set_value(ctx->enable, 0);
	msleep(20);

	// return regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	return 0;
}

static const struct drm_display_mode afj101_ba2131_default_mode = {
	.clock = 136000,

	.hdisplay    = 1080,
	.hsync_start = 1080 + 45,
	.hsync_end   = 1080 + 45 + 45,
	.htotal      = 1080 + 45 + 45 + 5,

	.vdisplay    = 1920,
	.vsync_start = 1920 + 9,
	.vsync_end   = 1920 + 9 + 4,
	.vtotal      = 1920 + 9 + 4 + 3,

	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
	.width_mm	= 136,
	.height_mm	= 217,
};

static int afj101_ba2131_get_modes(struct drm_panel *panel,
				  struct drm_connector *connector)
{
	struct afj101_ba2131 *ctx = panel_to_sl101_pn27d1665(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &afj101_ba2131_default_mode);
	if (!mode) {
		dev_err(&ctx->dsi->dev, "failed to add mode %ux%u@%u\n",
			afj101_ba2131_default_mode.hdisplay,
			afj101_ba2131_default_mode.vdisplay,
			drm_mode_vrefresh(&afj101_ba2131_default_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs afj101_ba2131_funcs = {
	.disable = afj101_ba2131_disable,
	.unprepare = afj101_ba2131_unprepare,
	.prepare = afj101_ba2131_prepare,
	.enable = afj101_ba2131_enable,
	.get_modes = afj101_ba2131_get_modes,
};

static int afj101_ba2131_dsi_probe(struct mipi_dsi_device *dsi)
{
	struct afj101_ba2131 *ctx;
	int ret;

	ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);
	ctx->dsi = dsi;

	ctx->enable = devm_gpiod_get(&dsi->dev, "enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->enable)) {
		dev_err(&dsi->dev, "Couldn't get our enable GPIO\n");
		return PTR_ERR(ctx->enable);
	}

	ctx->reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset)) {
		dev_err(&dsi->dev, "Couldn't get our reset GPIO\n");
		return PTR_ERR(ctx->reset);
	}

	drm_panel_init(&ctx->panel, &dsi->dev, &afj101_ba2131_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret){
		return ret;
	}

	drm_panel_add(&ctx->panel);

	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->lanes = 4;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	return 0;
}

static int afj101_ba2131_dsi_remove(struct mipi_dsi_device *dsi)
{
	struct afj101_ba2131 *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id afj101_ba2131_of_match[] = {
	{ .compatible = "innolux,afj101-ba2131", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, afj101_ba2131_of_match);

static struct mipi_dsi_driver afj101_ba2131_driver = {
	.probe = afj101_ba2131_dsi_probe,
	.remove = afj101_ba2131_dsi_remove,
	.driver = {
		.name = "innolux-afj101-ba2131",
		.of_match_table = afj101_ba2131_of_match,
	},
};
module_mipi_dsi_driver(afj101_ba2131_driver);

MODULE_AUTHOR("Kali Prasad <kprasadvnsi@protonmail.com>");
MODULE_DESCRIPTION("Innolux AFJ101 BA2131 MIPI-DSI LCD panel");
MODULE_LICENSE("GPL");
