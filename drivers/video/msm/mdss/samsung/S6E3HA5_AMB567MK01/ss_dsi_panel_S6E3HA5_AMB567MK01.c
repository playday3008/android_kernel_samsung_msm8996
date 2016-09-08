/*
 * =================================================================
 *
 *
 *	Description:  samsung display panel file
 *
 *	Author: jb09.kim
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2012, Samsung Electronics. All rights reserved.

*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
*/
#include "ss_dsi_panel_S6E3HA5_AMB567MK01.h"
#include "ss_dsi_mdnie_S6E3HA5_AMB567MK01.h"
#include "../../../mdss/mdss_dsi.h"

/* AOD Mode status on AOD Service */
enum {
	AOD_MODE_ALPM_2NIT_ON = MAX_LPM_MODE + 1,
	AOD_MODE_HLPM_2NIT_ON,
	AOD_MODE_ALPM_60NIT_ON,
	AOD_MODE_HLPM_60NIT_ON,
};

enum {
	ALPM_CTRL_2NIT,
	ALPM_CTRL_60NIT,
	HLPM_CTRL_2NIT,
	HLPM_CTRL_60NIT,
	MAX_LPM_CTRL,
};

/* Register to control brightness level */
#define ALPM_REG	0x53
/* Register to cnotrol ALPM/HLPM mode */
#define ALPM_CTRL_REG	0xBB

#define LUMINANCE_MAX 74

static int mdss_panel_on_pre(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	LCD_INFO("+: ndx=%d \n", ctrl->ndx);
	mdss_panel_attach_set(ctrl, true);
	LCD_INFO("-: ndx=%d \n", ctrl->ndx);

	return true;
}

static int mdss_panel_on_post(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	return true;
}

static int mdss_panel_revision(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	if (vdd->manufacture_id_dsi[ctrl->ndx] == PBA_ID)
		mdss_panel_attach_set(ctrl, false);
	else
		mdss_panel_attach_set(ctrl, true);

	vdd->aid_subdivision_enable = true;

	if (mdss_panel_rev_get(ctrl) == 0x80)
		vdd->panel_revision = 'A';
	else {
		LCD_ERR("panel_rev not found\n");
		vdd->panel_revision = 'A';
	}
	vdd->panel_revision -= 'A';

	LCD_DEBUG("panel_revision = %c %d \n",
					vdd->panel_revision + 'A', vdd->panel_revision);

	return true;
}

static int mdss_manufacture_date_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	unsigned char date[4];
	int year, month, day;
	int hour, min;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read mtp (C8h 41,42th) for manufacture date */
	if (vdd->dtsi_data[ctrl->ndx].manufacture_date_rx_cmds[vdd->panel_revision].cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl,
			&vdd->dtsi_data[ctrl->ndx].manufacture_date_rx_cmds[vdd->panel_revision],
			date, PANEL_LEVE1_KEY);

		year = date[0] & 0xf0;
		year >>= 4;
		year += 2011; // 0 = 2011 year
		month = date[0] & 0x0f;
		day = date[1] & 0x1f;
		hour = date[2]& 0x0f;
		min = date[3] & 0x1f;

		vdd->manufacture_date_dsi[ctrl->ndx] = year * 10000 + month * 100 + day;
		vdd->manufacture_time_dsi[ctrl->ndx] = hour * 100 + min;

		LCD_ERR("manufacture_date DSI%d = (%d%04d) - year(%d) month(%d) day(%d) hour(%d) min(%d)\n",
			ctrl->ndx, vdd->manufacture_date_dsi[ctrl->ndx], vdd->manufacture_time_dsi[ctrl->ndx],
			year, month, day, hour, min);

	} else {
		LCD_ERR("DSI%d no manufacture_date_rx_cmds cmds(%d)",  ctrl->ndx,vdd->panel_revision);
		return false;
	}

	return true;
}

static int mdss_ddi_id_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char ddi_id[5];
	int loop;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read mtp (D6h 1~5th) for ddi id */
	if (vdd->dtsi_data[ctrl->ndx].ddi_id_rx_cmds[vdd->panel_revision].cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].ddi_id_rx_cmds[vdd->panel_revision]),
			ddi_id, PANEL_LEVE1_KEY);

		for(loop = 0; loop < 5; loop++)
			vdd->ddi_id_dsi[ctrl->ndx][loop] = ddi_id[loop];

		LCD_INFO("DSI%d : %02x %02x %02x %02x %02x\n", ctrl->ndx,
			vdd->ddi_id_dsi[ctrl->ndx][0], vdd->ddi_id_dsi[ctrl->ndx][1],
			vdd->ddi_id_dsi[ctrl->ndx][2], vdd->ddi_id_dsi[ctrl->ndx][3],
			vdd->ddi_id_dsi[ctrl->ndx][4]);
	} else {
		LCD_ERR("DSI%d no ddi_id_rx_cmds cmds", ctrl->ndx);
		return false;
	}

	return true;
}

static int get_hbm_candela_value(int level)
{
	if (level == 13)
		return 443;
	else if (level == 6)
		return 465;
	else if (level == 7)
		return 488;
	else if (level == 8)
		return 510;
	else if (level == 9)
		return 533;
	else if (level == 10)
		return 555;
	else if (level == 11)
		return 578;
	else if (level == 12)
		return 600;
	else
		return 600;
}

static struct dsi_panel_cmds *mdss_hbm_gamma(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi[ctrl->ndx]->generate_hbm_gamma)) {
		LCD_ERR("generate_hbm_gamma is NULL error");
		return NULL;
	} else {
		if (mdss_panel_id1_get(ctrl) & 0x20) { /* GRACE */
			vdd->smart_dimming_dsi[ctrl->ndx]->generate_hbm_gamma(
				vdd->smart_dimming_dsi[ctrl->ndx],
				vdd->auto_brightness,
				&vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[1]);

			*level_key = PANEL_LEVE1_KEY;

			return &vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision];
		} else {
			if (vdd->auto_brightness > 6)
				return NULL;
			return &vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision];
		}
	}
}

static struct dsi_panel_cmds *mdss_hbm_etc(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	char elvss_3th_val, elvss_24th_val, elvss_25th_val;
	char elvss_443_offset, elvss_465_offset, elvss_488_offset, elvss_510_offset, elvss_533_offset;
	char elvss_555_offset, elvss_578_offset, elvss_600_offset;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	elvss_3th_val = elvss_24th_val = elvss_25th_val = 0;

	/* OTP value - B5 25th */
	elvss_24th_val = vdd->display_status_dsi[ctrl->ndx].elvss_value1;
	elvss_25th_val = vdd->display_status_dsi[ctrl->ndx].elvss_value2;

	/* ELVSS 0xB5 3th*/
	elvss_443_offset = 0x13;
	elvss_465_offset = 0x11;
	elvss_488_offset = 0x10;
	elvss_510_offset = 0x0F;
	elvss_533_offset = 0x0E;
	elvss_555_offset = 0x0C;
	elvss_578_offset = 0x0B;
	elvss_600_offset = 0x0A;

	/* ELVSS 0xB5 3th*/
	if (vdd->auto_brightness == HBM_MODE) /* 465CD */
		elvss_3th_val = elvss_465_offset;
	else if (vdd->auto_brightness == HBM_MODE + 1) /* 488CD */
		elvss_3th_val = elvss_488_offset;
	else if (vdd->auto_brightness == HBM_MODE + 2) /* 510CD */
		elvss_3th_val = elvss_510_offset;
	else if (vdd->auto_brightness == HBM_MODE + 3) /* 533CD */
		elvss_3th_val = elvss_533_offset;
	else if (vdd->auto_brightness == HBM_MODE + 4) /* 555CD */
		elvss_3th_val= elvss_555_offset;
	else if (vdd->auto_brightness == HBM_MODE + 5) /* 578CD */
		elvss_3th_val = elvss_578_offset;
	else if (vdd->auto_brightness == HBM_MODE + 6)/* 600CD */
		elvss_3th_val = elvss_600_offset;
	else if (vdd->auto_brightness == HBM_MODE + 7) /* 443CD */
		elvss_3th_val = elvss_443_offset;

	/* 0xB5 2nd temperature */
	vdd->dtsi_data[ctrl->ndx].hbm_etc_tx_cmds[vdd->panel_revision].cmds[1].payload[1] =
			vdd->temperature > 0 ? vdd->temperature : 0x80|(-1*vdd->temperature);

	/* ELVSS 0xB5 3th, elvss_24th_val */
	vdd->dtsi_data[ctrl->ndx].hbm_etc_tx_cmds[vdd->panel_revision].cmds[1].payload[3] = elvss_3th_val;
	vdd->dtsi_data[ctrl->ndx].hbm_etc_tx_cmds[vdd->panel_revision].cmds[3].payload[1] = elvss_25th_val;

	*level_key = PANEL_LEVE1_KEY;

	LCD_INFO("0xB5 3th: 0x%x 0xB5 elvss_25th_val(elvss val) 0x%x\n", elvss_3th_val, elvss_25th_val);

	return &vdd->dtsi_data[ctrl->ndx].hbm_etc_tx_cmds[vdd->panel_revision];
}

static int mdss_elvss_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char elvss_b5[2];
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read mtp (B5h 24th,25th) for elvss*/
	mdss_samsung_panel_data_read(ctrl,
		&(vdd->dtsi_data[ctrl->ndx].elvss_rx_cmds[vdd->panel_revision]),
		elvss_b5, PANEL_LEVE1_KEY);
	vdd->display_status_dsi[ctrl->ndx].elvss_value1 = elvss_b5[0]; /*0xB5 24th OTP value*/
	vdd->display_status_dsi[ctrl->ndx].elvss_value2 = elvss_b5[1]; /*0xB5 25th */

	return true;
}

static int mdss_hbm_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char hbm_buffer1[33];
	int loop;

	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read mtp (B3h 3~35th) for hbm gamma */
	mdss_samsung_panel_data_read(ctrl,
		&(vdd->dtsi_data[ctrl->ndx].hbm_rx_cmds[vdd->panel_revision]),
		hbm_buffer1, PANEL_LEVE1_KEY);

	for (loop = 0; loop < 33; loop++) {
		if (loop == 0) {
			/* V255 RGB MSB */
			vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[1] = (hbm_buffer1[loop] & 0x04) >> 2;
			vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[3] = (hbm_buffer1[loop] & 0x02) >> 1;
			vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[5] = hbm_buffer1[loop] & 0x01;
		} else if (loop == 1) {
			/* V255 R LSB */
			vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[2] = hbm_buffer1[loop];
		} else if (loop == 2) {
			/* V255 G LSB */
			vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[4] = hbm_buffer1[loop];
		} else if (loop == 3) {
			/* V255 B LSB */
			vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[6] = hbm_buffer1[loop];
		} else {
			/* +3 means V255 RGB MSB */
			vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[loop + 3] = hbm_buffer1[loop];
		}
	}

	return true;
}

#define COORDINATE_DATA_SIZE 6
#define MDNIE_SCR_WR_ADDR 	0x32

#define F1(x,y) ((y)-((43*(x))/40)+45)
#define F2(x,y) ((y)-((310*(x))/297)-3)
#define F3(x,y) ((y)+((367*(x))/84)-16305)
#define F4(x,y) ((y)+((333*(x))/107)-12396)

static char coordinate_data_1[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xfb, 0x00, 0xfb, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfc, 0x00, 0xff, 0x00}, /* Tune_2 */
	{0xfb, 0x00, 0xfa, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfe, 0x00, 0xfc, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xfb, 0x00, 0xfc, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xfd, 0x00, 0xff, 0x00, 0xfa, 0x00}, /* Tune_7 */
	{0xfc, 0x00, 0xff, 0x00, 0xfc, 0x00}, /* Tune_8 */
	{0xfb, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_9 */
};

static char coordinate_data_2[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xf6, 0x00, 0xee, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xf7, 0x00, 0xf1, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xf8, 0x00, 0xf4, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xf9, 0x00, 0xee, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfa, 0x00, 0xf1, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfc, 0x00, 0xed, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xfd, 0x00, 0xf1, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xfe, 0x00, 0xf5, 0x00}, /* Tune_9 */
};

static char (*coordinate_data[MAX_MODE])[COORDINATE_DATA_SIZE] = {
	coordinate_data_2,
	coordinate_data_2,
	coordinate_data_2,
	coordinate_data_1,
	coordinate_data_1,
	coordinate_data_1
};

static int mdnie_coordinate_index(int x, int y)
{
	int tune_number = 0;

	if (F1(x,y) > 0) {
		if (F3(x,y) > 0) {
			tune_number = 3;
		} else {
			if (F4(x,y) < 0)
				tune_number = 1;
			else
				tune_number = 2;
		}
	} else {
		if (F2(x,y) < 0) {
			if (F3(x,y) > 0) {
				tune_number = 9;
			} else {
				if (F4(x,y) < 0)
					tune_number = 7;
				else
					tune_number = 8;
			}
		} else {
			if (F3(x,y) > 0)
				tune_number = 6;
			else {
				if (F4(x,y) < 0)
					tune_number = 4;
				else
					tune_number = 5;
			}
		}
	}

	return tune_number;
}

static int mdss_mdnie_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char x_y_location[4];
	int mdnie_tune_index = 0;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read mtp (D6h 1~5th) for ddi id */
	if (vdd->dtsi_data[ctrl->ndx].mdnie_read_rx_cmds[vdd->panel_revision].cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].mdnie_read_rx_cmds[vdd->panel_revision]),
			x_y_location, PANEL_LEVE1_KEY);

		vdd->mdnie_x[ctrl->ndx] = x_y_location[0] << 8 | x_y_location[1];	/* X */
		vdd->mdnie_y[ctrl->ndx] = x_y_location[2] << 8 | x_y_location[3];	/* Y */

		mdnie_tune_index = mdnie_coordinate_index(vdd->mdnie_x[ctrl->ndx], vdd->mdnie_y[ctrl->ndx]);
		coordinate_tunning_multi(ctrl->ndx, coordinate_data, mdnie_tune_index,
			MDNIE_SCR_WR_ADDR, COORDINATE_DATA_SIZE);

		LCD_INFO("DSI%d : X-%d Y-%d \n", ctrl->ndx,
			vdd->mdnie_x[ctrl->ndx], vdd->mdnie_y[ctrl->ndx]);
	} else {
		LCD_ERR("DSI%d no mdnie_read_rx_cmds cmds", ctrl->ndx);
		return false;
	}

	return true;
}

static int mdss_samart_dimming_init(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	vdd->smart_dimming_dsi[ctrl->ndx] = vdd->panel_func.samsung_smart_get_conf();

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi[ctrl->ndx])) {
		LCD_ERR("DSI%d smart_dimming_dsi is null", ctrl->ndx);
		return false;
	} else {
		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].smart_dimming_mtp_rx_cmds[vdd->panel_revision]),
			vdd->smart_dimming_dsi[ctrl->ndx]->mtp_buffer, PANEL_LEVE1_KEY);

		/* Initialize smart dimming related things here */
		/* lux_tab setting for 350cd */
		vdd->smart_dimming_dsi[ctrl->ndx]->lux_tab = vdd->dtsi_data[ctrl->ndx].candela_map_table[vdd->panel_revision].lux_tab;
		vdd->smart_dimming_dsi[ctrl->ndx]->lux_tabsize = vdd->dtsi_data[ctrl->ndx].candela_map_table[vdd->panel_revision].lux_tab_size;
		vdd->smart_dimming_dsi[ctrl->ndx]->man_id = vdd->manufacture_id_dsi[ctrl->ndx];

		/* copy hbm gamma payload for hbm interpolation calc */
		vdd->smart_dimming_dsi[ctrl->ndx]->hbm_payload = &vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[1];

		/* Just a safety check to ensure smart dimming data is initialised well */
		vdd->smart_dimming_dsi[ctrl->ndx]->init(vdd->smart_dimming_dsi[ctrl->ndx]);

		vdd->temperature = 20; // default temperature

		vdd->smart_dimming_loaded_dsi[ctrl->ndx] = true;
	}

	LCD_INFO("DSI%d : --\n", ctrl->ndx);

	return true;
}

static int mdss_cell_id_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char cell_id_buffer[MAX_CELL_ID] = {0,};
	int loop;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read Panel Unique Cell ID (C8h 41~51th) */
	if (vdd->dtsi_data[ctrl->ndx].cell_id_rx_cmds[vdd->panel_revision].cmd_cnt) {
		memset(cell_id_buffer, 0x00, MAX_CELL_ID);

		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].cell_id_rx_cmds[vdd->panel_revision]),
			cell_id_buffer, PANEL_LEVE1_KEY);

		for(loop = 0; loop < MAX_CELL_ID; loop++)
			vdd->cell_id_dsi[ctrl->ndx][loop] = cell_id_buffer[loop];

		LCD_INFO("DSI%d: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			ctrl->ndx, vdd->cell_id_dsi[ctrl->ndx][0],
			vdd->cell_id_dsi[ctrl->ndx][1],	vdd->cell_id_dsi[ctrl->ndx][2],
			vdd->cell_id_dsi[ctrl->ndx][3],	vdd->cell_id_dsi[ctrl->ndx][4],
			vdd->cell_id_dsi[ctrl->ndx][5],	vdd->cell_id_dsi[ctrl->ndx][6],
			vdd->cell_id_dsi[ctrl->ndx][7],	vdd->cell_id_dsi[ctrl->ndx][8],
			vdd->cell_id_dsi[ctrl->ndx][9],	vdd->cell_id_dsi[ctrl->ndx][10]);

	} else {
		LCD_ERR("DSI%d no cell_id_rx_cmds cmd\n", ctrl->ndx);
		return false;
	}

	return true;
}

static int mdss_octa_id_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char octa_id_buffer[MAX_OCTA_ID] = {0,};
	int loop;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read Panel Unique OCTA ID (C9h 2nd~21th) */
	if (vdd->dtsi_data[ctrl->ndx].octa_id_rx_cmds[vdd->panel_revision].cmd_cnt) {
		memset(octa_id_buffer, 0x00, MAX_OCTA_ID);

		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].octa_id_rx_cmds[vdd->panel_revision]),
			octa_id_buffer, PANEL_LEVE1_KEY);

		for(loop = 0; loop < MAX_OCTA_ID; loop++)
			vdd->octa_id_dsi[ctrl->ndx][loop] = octa_id_buffer[loop];

		LCD_INFO("DSI%d: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			ctrl->ndx, vdd->octa_id_dsi[ctrl->ndx][0], vdd->octa_id_dsi[ctrl->ndx][1],
			vdd->octa_id_dsi[ctrl->ndx][2],	vdd->octa_id_dsi[ctrl->ndx][3],
			vdd->octa_id_dsi[ctrl->ndx][4],	vdd->octa_id_dsi[ctrl->ndx][5],
			vdd->octa_id_dsi[ctrl->ndx][6],	vdd->octa_id_dsi[ctrl->ndx][7],
			vdd->octa_id_dsi[ctrl->ndx][8],	vdd->octa_id_dsi[ctrl->ndx][9],
			vdd->octa_id_dsi[ctrl->ndx][10],vdd->octa_id_dsi[ctrl->ndx][11],
			vdd->octa_id_dsi[ctrl->ndx][12],vdd->octa_id_dsi[ctrl->ndx][13],
			vdd->octa_id_dsi[ctrl->ndx][14],vdd->octa_id_dsi[ctrl->ndx][15],
			vdd->octa_id_dsi[ctrl->ndx][16],vdd->octa_id_dsi[ctrl->ndx][17],
			vdd->octa_id_dsi[ctrl->ndx][18],vdd->octa_id_dsi[ctrl->ndx][19]);

	} else {
		LCD_ERR("DSI%d no octa_id_rx_cmds cmd\n", ctrl->ndx);
		return false;
	}

	return true;
}


static struct dsi_panel_cmds aid_cmd;
static struct dsi_panel_cmds *mdss_aid(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int cd_index = 0;
	int cmd_idx = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	if (vdd->aid_subdivision_enable) {
		aid_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].aid_subdivision_tx_cmds[vdd->panel_revision].cmds[vdd->bl_level]);
		LCD_ERR("level(%d), aid(%x %x)\n", vdd->bl_level, aid_cmd.cmds->payload[1], aid_cmd.cmds->payload[2]);
	} else {
		cd_index = get_cmd_index(vdd, ctrl->ndx);

		if (!vdd->dtsi_data[ctrl->ndx].aid_map_table[vdd->panel_revision].size ||
			cd_index > vdd->dtsi_data[ctrl->ndx].aid_map_table[vdd->panel_revision].size)
			goto end;

		cmd_idx = vdd->dtsi_data[ctrl->ndx].aid_map_table[vdd->panel_revision].cmd_idx[cd_index];

		aid_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].aid_tx_cmds[vdd->panel_revision].cmds[cmd_idx]);
	}

	aid_cmd.cmd_cnt = 1;
	*level_key = PANEL_LEVE1_KEY;

	return &aid_cmd;

end :
	LCD_ERR("error");
	return NULL;
}

static struct dsi_panel_cmds * mdss_acl_on(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	*level_key = PANEL_LEVE1_KEY;

	if (vdd->gradual_acl_val)
		vdd->dtsi_data[ctrl->ndx].acl_on_tx_cmds[vdd->panel_revision].cmds[0].payload[7] = vdd->gradual_acl_val;

	return &(vdd->dtsi_data[ctrl->ndx].acl_on_tx_cmds[vdd->panel_revision]);
}

static struct dsi_panel_cmds * mdss_acl_off(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	*level_key = PANEL_LEVE1_KEY;

	return &(vdd->dtsi_data[ctrl->ndx].acl_off_tx_cmds[vdd->panel_revision]);
}

#if 0
static struct dsi_panel_cmds acl_percent_cmd;
static struct dsi_panel_cmds * mdss_acl_precent(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int cd_index = 0;
	int cmd_idx = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	cd_index = get_cmd_index(vdd, ctrl->ndx);

	if (!vdd->dtsi_data[ctrl->ndx].acl_map_table[vdd->panel_revision].size ||
		cd_index > vdd->dtsi_data[ctrl->ndx].acl_map_table[vdd->panel_revision].size)
		goto end;

	cmd_idx = vdd->dtsi_data[ctrl->ndx].acl_map_table[vdd->panel_revision].cmd_idx[cd_index];

	acl_percent_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].acl_percent_tx_cmds[vdd->panel_revision].cmds[cmd_idx]);
	acl_percent_cmd.cmd_cnt = 1;

	*level_key = PANEL_LEVE1_KEY;

	return &acl_percent_cmd;

end :
	LCD_ERR("error");
	return NULL;

}
#endif

static struct dsi_panel_cmds elvss_cmd;
static struct dsi_panel_cmds * mdss_elvss(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int cd_index = 0;
	int cmd_idx = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	cd_index = get_cmd_index(vdd, ctrl->ndx);

	if (!vdd->dtsi_data[ctrl->ndx].smart_acl_elvss_map_table[vdd->panel_revision].size ||
		cd_index > vdd->dtsi_data[ctrl->ndx].smart_acl_elvss_map_table[vdd->panel_revision].size)
		goto end;

	cmd_idx = vdd->dtsi_data[ctrl->ndx].smart_acl_elvss_map_table[vdd->panel_revision].cmd_idx[cd_index];

	/* ELVSS Compensation for Low Temperature & Low Birghtness (2~5nit) */
	if (vdd->temperature <= 0 && vdd->candela_level <= 5)
		cmd_idx = 35;

	elvss_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].elvss_tx_cmds[vdd->panel_revision].cmds[cmd_idx]);

	elvss_cmd.cmd_cnt = 1;

	*level_key = PANEL_LEVE1_KEY;

	return &elvss_cmd;

end :
	LCD_ERR("error");
	return NULL;
}

static struct dsi_panel_cmds * mdss_elvss_temperature1(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	char elvss_24th_val;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	/* OTP value - B5 24th */
	elvss_24th_val = vdd->display_status_dsi[ctrl->ndx].elvss_value1;

	LCD_DEBUG("OTP val %x\n", elvss_24th_val);

	/* 0xB5 1th TSET */
	vdd->dtsi_data[ctrl->ndx].elvss_lowtemp_tx_cmds[vdd->panel_revision].cmds[0].payload[1] =
		vdd->temperature > 0 ? vdd->temperature : 0x80|(-1*vdd->temperature);

	/* 0xB5 elvss_24th_val elvss_offset */
	vdd->dtsi_data[ctrl->ndx].elvss_lowtemp_tx_cmds[vdd->panel_revision].cmds[2].payload[1] = elvss_24th_val;

	LCD_DEBUG("acl : %d, interpolation_temp : %d temp : %d, cd : %d, B5 1st :0x%x, B5 elvss_24th_val :0x%x\n",
		vdd->acl_status, vdd->elvss_interpolation_temperature, vdd->temperature, vdd->candela_level,
		vdd->dtsi_data[ctrl->ndx].elvss_lowtemp_tx_cmds[vdd->panel_revision].cmds[0].payload[1],
		vdd->dtsi_data[ctrl->ndx].elvss_lowtemp_tx_cmds[vdd->panel_revision].cmds[2].payload[1]);

	*level_key = PANEL_LEVE1_KEY;

	return &(vdd->dtsi_data[ctrl->ndx].elvss_lowtemp_tx_cmds[vdd->panel_revision]);
}

static struct dsi_panel_cmds vint_cmd;
static struct dsi_panel_cmds *mdss_vint(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int cd_index = 0;
	int cmd_idx = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	cd_index = get_cmd_index(vdd, ctrl->ndx);

	if (!vdd->dtsi_data[ctrl->ndx].vint_map_table[vdd->panel_revision].size ||
		cd_index > vdd->dtsi_data[ctrl->ndx].vint_map_table[vdd->panel_revision].size)
		goto end;

	cmd_idx = vdd->dtsi_data[ctrl->ndx].vint_map_table[vdd->panel_revision].cmd_idx[cd_index];

	if (vdd->temperature > vdd->elvss_interpolation_temperature )
		vint_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].vint_tx_cmds[vdd->panel_revision].cmds[cmd_idx]);
	else
		vint_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].vint_tx_cmds[vdd->panel_revision].cmds[0]);
	vint_cmd.cmd_cnt = 1;

	*level_key = PANEL_LEVE1_KEY;

	return &vint_cmd;

end :
	LCD_ERR("error");
	return NULL;
}

static struct dsi_panel_cmds irc_cmd;
static struct dsi_panel_cmds *mdss_irc(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	if (IS_ERR_OR_NULL(vdd->dtsi_data[ctrl->ndx].irc_subdivision_tx_cmds)) {
		LCD_ERR("No irc_subdivision_tx_cmds\n");
		return NULL;
	}

	/* IRC Subdivision works like as AID Subdivision */
	irc_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].irc_subdivision_tx_cmds[vdd->panel_revision].cmds[vdd->bl_level]);

	irc_cmd.cmd_cnt = 1;
	*level_key = PANEL_LEVE1_KEY;

	return &irc_cmd;
}

static char irc_hbm_para_revA[8][20] = {
	{0x6D, 0xB0, 0x48, 0x4C, 0x95, 0xDA, 0x33, 0x69, 0x12, 0x7A, 0xDA, 0x7C, 0x79, 0x6E, 0x27, 0x34, 0x24, 0x1F, 0x25, 0x15},
	{0x6D, 0xB0, 0x48, 0x4C, 0x95, 0xDA, 0x33, 0x69, 0x12, 0x7A, 0xDA, 0x78, 0x75, 0x6A, 0x25, 0x32, 0x22, 0x1E, 0x23, 0x15},
	{0x6D, 0xB0, 0x48, 0x4C, 0x95, 0xDA, 0x33, 0x69, 0x12, 0x7A, 0xDA, 0x73, 0x70, 0x66, 0x24, 0x30, 0x21, 0x1D, 0x22, 0x14},
	{0x6D, 0xB0, 0x48, 0x4C, 0x95, 0xDA, 0x33, 0x69, 0x12, 0x7A, 0xDA, 0x6E, 0x6C, 0x62, 0x23, 0x2E, 0x1F, 0x1C, 0x21, 0x13},
	{0x6D, 0xB0, 0x48, 0x4C, 0x95, 0xDA, 0x33, 0x69, 0x12, 0x7A, 0xDA, 0x6A, 0x67, 0x5E, 0x20, 0x2C, 0x1E, 0x1B, 0x20, 0x12},
	{0x6D, 0xB0, 0x48, 0x4C, 0x95, 0xDA, 0x33, 0x69, 0x12, 0x7A, 0xDA, 0x65, 0x63, 0x59, 0x1F, 0x2A, 0x1E, 0x1A, 0x1E, 0x11},
	{0x6D, 0xB0, 0x48, 0x4C, 0x95, 0xDA, 0x33, 0x69, 0x12, 0x7A, 0xDA, 0x60, 0x5E, 0x55, 0x1E, 0x28, 0x1C, 0x19, 0x1D, 0x11},
	{0x6D, 0xB0, 0x48, 0x4C, 0x95, 0xDA, 0x33, 0x69, 0x12, 0x7A, 0xDA, 0x5C, 0x5A, 0x51, 0x1C, 0x26, 0x1B, 0x17, 0x1B, 0x0F},
};

static struct dsi_panel_cmds *mdss_hbm_irc(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int para_idx = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	if (IS_ERR_OR_NULL(vdd->dtsi_data[ctrl->ndx].irc_tx_cmds)) {
		LCD_ERR("No irc_tx_cmds\n");
		return NULL;
	}

	if (!vdd->samsung_support_irc)
		return NULL;

	*level_key = PANEL_LEVE1_KEY;

	/*auto_brightness is 13 to use 443cd of hbm step on color weakness mode*/
	if (vdd->auto_brightness == HBM_MODE + 7)
		para_idx = 7;
	else
		para_idx = vdd->auto_brightness_level - vdd->auto_brightness;

	/* copy irc default setting */
	memcpy(&vdd->dtsi_data[ctrl->ndx].hbm_irc_tx_cmds[vdd->panel_revision].cmds[0].payload[1],
			irc_hbm_para_revA[para_idx], 20);

	return &(vdd->dtsi_data[ctrl->ndx].hbm_irc_tx_cmds[vdd->panel_revision]);
}

static struct dsi_panel_cmds * mdss_gamma(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	LCD_DEBUG("bl_level : %d candela : %dCD\n", vdd->bl_level, vdd->candela_level);

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi[ctrl->ndx]->generate_gamma)) {
		LCD_ERR("generate_gamma is NULL error");
		return NULL;
	} else {
		vdd->smart_dimming_dsi[ctrl->ndx]->generate_gamma(
			vdd->smart_dimming_dsi[ctrl->ndx],
			vdd->candela_level,
			&vdd->dtsi_data[ctrl->ndx].gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[1]);

		*level_key = PANEL_LEVE1_KEY;

		return &vdd->dtsi_data[ctrl->ndx].gamma_tx_cmds[vdd->panel_revision];
	}
}

static int samsung_panel_off_post(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int rc = 0;


	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	return rc;
}

static struct dsi_panel_cmds * mdss_gamma_hmt(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	LCD_INFO("hmt_bl_level : %d candela : %dCD\n", vdd->hmt_stat.hmt_bl_level, vdd->hmt_stat.candela_level_hmt);

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi_hmt[ctrl->ndx]->generate_gamma)) {
		LCD_ERR("generate_gamma is NULL");
		return NULL;
	} else {
		vdd->smart_dimming_dsi_hmt[ctrl->ndx]->generate_gamma(
			vdd->smart_dimming_dsi_hmt[ctrl->ndx],
			vdd->hmt_stat.candela_level_hmt,
			&vdd->dtsi_data[ctrl->ndx].hmt_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[1]);

		*level_key = PANEL_LEVE1_KEY;

		return &vdd->dtsi_data[ctrl->ndx].hmt_gamma_tx_cmds[vdd->panel_revision];
	}
}

static struct dsi_panel_cmds hmt_aid_cmd;
static struct dsi_panel_cmds *mdss_aid_hmt(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int cmd_idx = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	if (!vdd->dtsi_data[ctrl->ndx].hmt_reverse_aid_map_table[vdd->panel_revision].size ||
		vdd->hmt_stat.cmd_idx_hmt > vdd->dtsi_data[ctrl->ndx].hmt_reverse_aid_map_table[vdd->panel_revision].size)
		goto end;

	cmd_idx = vdd->dtsi_data[ctrl->ndx].hmt_reverse_aid_map_table[vdd->panel_revision].cmd_idx[vdd->hmt_stat.cmd_idx_hmt];

	hmt_aid_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].hmt_aid_tx_cmds[vdd->panel_revision].cmds[cmd_idx]);
	hmt_aid_cmd.cmd_cnt = 1;

	*level_key = PANEL_LEVE1_KEY;

	return &hmt_aid_cmd;

end :
	LCD_ERR("error");
	return NULL;
}

static struct dsi_panel_cmds *mdss_elvss_hmt(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	/* ELVSS(MPS_CON) setting condition is equal to normal birghtness */
	if (vdd->hmt_stat.candela_level_hmt > 40) {
		vdd->dtsi_data[ctrl->ndx].hmt_elvss_tx_cmds[vdd->panel_revision].cmds[1].payload[1] = 0xBC;
	} else {
		vdd->dtsi_data[ctrl->ndx].hmt_elvss_tx_cmds[vdd->panel_revision].cmds[1].payload[1] = 0xAC;
	}

	*level_key = PANEL_LEVE1_KEY;

	return &vdd->dtsi_data[ctrl->ndx].hmt_elvss_tx_cmds[vdd->panel_revision];
}

static struct dsi_panel_cmds *mdss_vint_hmt(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	*level_key = PANEL_LEVE1_KEY;

	return &vdd->dtsi_data[ctrl->ndx].hmt_vint_tx_cmds[vdd->panel_revision];
}

static void mdss_make_sdimconf_hmt(struct mdss_dsi_ctrl_pdata *ctrl, struct samsung_display_driver_data *vdd) {
	/* Set the mtp read buffer pointer and read the NVM value*/
	mdss_samsung_panel_data_read(ctrl,
				&(vdd->dtsi_data[ctrl->ndx].smart_dimming_mtp_rx_cmds[vdd->panel_revision]),
				vdd->smart_dimming_dsi_hmt[ctrl->ndx]->mtp_buffer, PANEL_LEVE1_KEY);

	/* Initialize smart dimming related things here */
	/* lux_tab setting for 350cd */
	vdd->smart_dimming_dsi_hmt[ctrl->ndx]->lux_tab = vdd->dtsi_data[ctrl->ndx].hmt_candela_map_table[vdd->panel_revision].lux_tab;
	vdd->smart_dimming_dsi_hmt[ctrl->ndx]->lux_tabsize = vdd->dtsi_data[ctrl->ndx].hmt_candela_map_table[vdd->panel_revision].lux_tab_size;
	vdd->smart_dimming_dsi_hmt[ctrl->ndx]->man_id = vdd->manufacture_id_dsi[ctrl->ndx];

	/* Just a safety check to ensure smart dimming data is initialised well */
	vdd->smart_dimming_dsi_hmt[ctrl->ndx]->init(vdd->smart_dimming_dsi_hmt[ctrl->ndx]);

	LCD_INFO("[HMT] smart dimming done!\n");
}

static int mdss_samart_dimming_init_hmt(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	LCD_INFO("DSI%d : ++\n", ctrl->ndx);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	vdd->smart_dimming_dsi_hmt[ctrl->ndx] = vdd->panel_func.samsung_smart_get_conf_hmt();

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi_hmt[ctrl->ndx])) {
		LCD_ERR("DSI%d error", ctrl->ndx);
		return false;
	} else {
		vdd->hmt_stat.hmt_on = 0;
		vdd->hmt_stat.hmt_bl_level = 0;
		vdd->hmt_stat.hmt_reverse = 0;
		vdd->hmt_stat.hmt_is_first = 1;

		mdss_make_sdimconf_hmt(ctrl, vdd);

		vdd->smart_dimming_hmt_loaded_dsi[ctrl->ndx] = true;
	}

	LCD_INFO("DSI%d : --\n", ctrl->ndx);

	return true;
}

/*
 * This function will update parameters for ALPM_REG/ALPM_CTRL_REG
 * Parameter for ALPM_REG : Control brightness for panel LPM
 * Parameter for ALPM_CTRL_REG : Change panel LPM mode for ALPM/HLPM
 */
static int mdss_update_panel_lpm_cmds(struct mdss_dsi_ctrl_pdata *ctrl, int bl_level, int mode)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct dsi_panel_cmds *alpm_brightness[PANEL_LPM_BRIGHTNESS_MAX] = {NULL, };
	struct dsi_panel_cmds *alpm_ctrl[MAX_LPM_CTRL] = {NULL, };
	struct dsi_panel_cmds *cmd_list[2];
	/*
	 * Init reg_list and cmd list
	 * reg_list[X][0] is reg value
	 * reg_list[X][1] is offset for reg value
	 * cmd_list is the target cmds for searching reg value
	 */
	static int reg_list[2][2] = {
		{ALPM_REG, -EINVAL},
		{ALPM_CTRL_REG, -EINVAL}};

	if (IS_ERR_OR_NULL(ctrl))
		goto end;

	vdd = check_valid_ctrl(ctrl);

	cmd_list[0] = &vdd->dtsi_data[ctrl->ndx].alpm_on_tx_cmds[vdd->panel_revision];
	cmd_list[1] = &vdd->dtsi_data[ctrl->ndx].alpm_on_tx_cmds[vdd->panel_revision];

	/* Init alpm_brightness and alpm_ctrl cmds */
	alpm_brightness[PANEL_LPM_2NIT] =
		&vdd->dtsi_data[ctrl->ndx].lpm_2nit_tx_cmds[vdd->panel_revision];
	alpm_brightness[PANEL_LPM_40NIT] =
		&vdd->dtsi_data[ctrl->ndx].lpm_60nit_tx_cmds[vdd->panel_revision];
	alpm_brightness[PANEL_LPM_60NIT] =
		&vdd->dtsi_data[ctrl->ndx].lpm_60nit_tx_cmds[vdd->panel_revision];

	alpm_ctrl[ALPM_CTRL_2NIT] =
		&vdd->dtsi_data[ctrl->ndx].lpm_ctrl_alpm_2nit_tx_cmds[vdd->panel_revision];
	alpm_ctrl[ALPM_CTRL_60NIT] =
		&vdd->dtsi_data[ctrl->ndx].lpm_ctrl_alpm_60nit_tx_cmds[vdd->panel_revision];
	alpm_ctrl[HLPM_CTRL_2NIT] =
		&vdd->dtsi_data[ctrl->ndx].lpm_ctrl_hlpm_2nit_tx_cmds[vdd->panel_revision];
	alpm_ctrl[HLPM_CTRL_60NIT] =
		&vdd->dtsi_data[ctrl->ndx].lpm_ctrl_hlpm_60nit_tx_cmds[vdd->panel_revision];


	/*
	 * Find offset for alpm_reg and alpm_ctrl_reg
	 * alpm_reg		 : Control register for ALPM/HLPM on/off
	 * alpm_ctrl_reg : Control register for changing ALPM/HLPM mode
	 */
	mdss_init_panel_lpm_reg_offset(ctrl, reg_list, cmd_list,
			sizeof(cmd_list) / sizeof(cmd_list[0]));

	if (reg_list[0][1] != -EINVAL) {
		/* Update parameter for ALPM_REG */
		memcpy(cmd_list[0]->cmds[reg_list[0][1]].payload,
				alpm_brightness[bl_level]->cmds[0].payload,
				sizeof(char) * cmd_list[0]->cmds[reg_list[0][1]].dchdr.dlen);

		LCD_DEBUG("[Panel LPM] change brightness cmd : %x, %x\n",
				cmd_list[0]->cmds[reg_list[0][1]].payload[1],
				alpm_brightness[bl_level]->cmds[0].payload[1]);

	}

	if (reg_list[1][1] != -EINVAL) {
		/* Initialize ALPM/HLPM cmds */
		switch (bl_level) {
			case PANEL_LPM_40NIT:
			case PANEL_LPM_60NIT:
				mode = (mode == ALPM_MODE_ON) ? ALPM_CTRL_60NIT :
					(mode == HLPM_MODE_ON) ? HLPM_CTRL_60NIT : ALPM_CTRL_60NIT;
				break;
			case PANEL_LPM_2NIT:
			default:
				mode = (mode == ALPM_MODE_ON) ? ALPM_CTRL_2NIT :
					(mode == HLPM_MODE_ON) ? HLPM_CTRL_2NIT : ALPM_CTRL_2NIT;
				break;
		}

		/* Update parameter for ALPM_CTRL_REG */
		memcpy(cmd_list[1]->cmds[reg_list[1][1]].payload,
				alpm_ctrl[mode]->cmds[0].payload,
				sizeof(char) * cmd_list[1]->cmds[reg_list[1][1]].dchdr.dlen);
		LCD_DEBUG("[Panel LPM] update alpm ctrl reg(%d)\n", mode);
	}

end:
	return 0;
}

static void mdss_get_panel_lpm_mode(struct mdss_dsi_ctrl_pdata *ctrl, u8 *mode)
{
	struct samsung_display_driver_data *vdd = NULL;
	int panel_lpm_mode = 0, lpm_bl_level = 0;
	static u8 prev_lpm_mode = MODE_OFF;

	if (IS_ERR_OR_NULL(ctrl))
		return;

	/*
	 * if the mode value is lower than MAX_LPM_MODE
	 * this function was not called by mdss_samsung_alpm_store()
	 * so the mode will not be changed
	 */
	if (*mode < MAX_LPM_MODE)
		return;

	vdd = check_valid_ctrl(ctrl);

	/* default Hz is 30Hz */
	vdd->panel_lpm.hz = PANEL_LPM_30HZ;

	/* Check mode and bl_level */
	switch (*mode) {
		case AOD_MODE_ALPM_2NIT_ON:
			panel_lpm_mode = ALPM_MODE_ON;
			lpm_bl_level = PANEL_LPM_2NIT;
			break;
		case AOD_MODE_HLPM_2NIT_ON:
			panel_lpm_mode = HLPM_MODE_ON;
			lpm_bl_level = PANEL_LPM_2NIT;
			break;
		case AOD_MODE_ALPM_60NIT_ON:
			panel_lpm_mode = ALPM_MODE_ON;
			lpm_bl_level = PANEL_LPM_60NIT;
			break;
		case AOD_MODE_HLPM_60NIT_ON:
			panel_lpm_mode = HLPM_MODE_ON;
			lpm_bl_level = PANEL_LPM_60NIT;
			break;
		default:
			panel_lpm_mode = MODE_OFF;
			break;
	}

	*mode = panel_lpm_mode;

	/*
	 * If the bl_level or mode was changed, the panel LPM
	 * command will be updated but if the bl_level or mode
	 * was not changed, do nothing.
	 */
	if ((vdd->panel_lpm.lpm_bl_level != lpm_bl_level) ||
		(prev_lpm_mode != panel_lpm_mode)) {
		vdd->panel_lpm.param_changed = true;
		prev_lpm_mode = panel_lpm_mode;
	} else
		return;

	/* Save mode and bl_level */
	vdd->panel_lpm.lpm_bl_level = lpm_bl_level;

	mdss_update_panel_lpm_cmds(ctrl, lpm_bl_level, panel_lpm_mode);
}

static int ddi_hw_cursor(struct mdss_dsi_ctrl_pdata *ctrl, int *input)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	char *payload;

	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("dsi_ctrl is NULL\n");
		return 0;
	}

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return 0;
	}

	if (IS_ERR_OR_NULL(input)) {
		LCD_ERR("input is NULL\n");
		return 0;
	}

	if (IS_ERR_OR_NULL(vdd->dtsi_data[ctrl->ndx].hw_cursor_tx_cmds[vdd->panel_revision].cmds[0].payload)) {
		LCD_ERR("hw_cursor_tx_cmds is NULL\n");
		return 0;
	}


	LCD_INFO("On/Off:(%d), Por/Land:(%d), On_Select:(%d), X:(%d), Y:(%d), Width:(%d), Length:(%d), Color:(0x%x), Period:(%x), TR_TIME(%x)\n",
		input[0], input[1], input[2], input[3], input[4], input[5],
		input[6], input[7], input[8], input[9]);

	payload = vdd->dtsi_data[ctrl->ndx].hw_cursor_tx_cmds[vdd->panel_revision].cmds[0].payload;

	/* Cursor On&Off (0:Off, 1:On) */
	payload[1] = input[0] & 0x1;

	/* 3rd bit : CURSOR_ON_SEL, 2nd bit : Port/Land, 1st bit : BLINK_ON(Default On)*/
	payload[2] = (input[2] & 0x1) << 2 | (input[1] & 0x1) << 1 | 0x1;

	/* Start X address */
	payload[3] = (input[3] & 0x700) >> 8;
	payload[4] = input[3] & 0xFF;

	/* Start Y address */
	payload[5] = (input[4] & 0x700) >> 8;
	payload[6] = input[4] & 0xFF;

	/* Width */
	payload[7] = input[5] & 0xF;

	/* Length */
	payload[8] = (input[6] & 0x100) >> 8;
	payload[9] = input[6] & 0xFF;

	/* Color */
	payload[10] = (input[7] & 0xFF0000) >> 16;
	payload[11] = (input[7] & 0xFF00) >> 8;
	payload[12] = input[7] & 0xFF;

	/* Period */
	payload[13] = input[8] & 0xFF;

	/* TR_TIME */
	payload[14] = input[9] & 0xFF;

	mdss_samsung_send_cmd(ctrl, PANEL_LEVE1_KEY_ENABLE);
	mdss_samsung_send_cmd(ctrl, PANEL_HW_CURSOR);
	mdss_samsung_send_cmd(ctrl, PANEL_LEVE1_KEY_DISABLE);

	return 1;
}

static void mdss_send_colorweakness_ccb_cmd(struct samsung_display_driver_data *vdd, int mode)
{
	LCD_INFO("mode (%d) color_weakness_value (%x) \n", mode, vdd->color_weakness_value);

	if (mode) {
		vdd->dtsi_data[DISPLAY_1].ccb_on_tx_cmds[vdd->panel_revision].cmds[1].payload[1] = vdd->color_weakness_value;
		mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], PANEL_COLOR_WEAKNESS_ENABLE);
	} else
		mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], PANEL_COLOR_WEAKNESS_DISABLE);
}

static void mdss_panel_multires(struct samsung_display_driver_data *vdd)
{
	if(vdd->multires_stat.prev_mode != vdd->multires_stat.curr_mode)
	{
		LCD_INFO("vdd->multires_stat.prev_mode = %d, vdd-multires_stat.curr_mode = %d\n",
			vdd->multires_stat.prev_mode, vdd->multires_stat.curr_mode);

		if(vdd->multires_stat.curr_mode == MULTIRES_FHD)
			mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], PANEL_MULTIRES_FHD);
		else if(vdd->multires_stat.curr_mode == MULTIRES_HD)
			mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], PANEL_MULTIRES_HD);
		else if(vdd->multires_stat.curr_mode == MULTIRES_WQHD)
		{
			if(vdd->multires_stat.prev_mode == MULTIRES_FHD)
				mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], PANEL_MULTIRES_FHD_TO_WQHD);
			else
				mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], PANEL_MULTIRES_HD_TO_WQHD);
		}
		vdd->multires_stat.prev_mode = vdd->multires_stat.curr_mode;
	}
}

#if 0
static void mdss_panel_cover_control(struct mdss_dsi_ctrl_pdata *ctrl, struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("dsi_ctrl is NULL\n");
		return;
	}

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is NULL\n");
		return;
	}

	if (vdd->cover_control) {
		mdss_samsung_send_cmd(ctrl, PANEL_COVER_CONTROL_ENABLE);
		LCD_INFO("Send Cover Contorl Enable CMD\n");
	} else {
		mdss_samsung_send_cmd(ctrl, PANEL_COVER_CONTROL_DISABLE);
		LCD_INFO("Send Cover Contorl Disable CMD\n");
	}
}
#endif

static void dsi_update_mdnie_data(void)
{
	/* Update mdnie command */
	mdnie_data.DSI0_COLOR_BLIND_MDNIE_1 = DSI0_COLOR_BLIND_MDNIE_1;
	mdnie_data.DSI0_RGB_SENSOR_MDNIE_1 = DSI0_RGB_SENSOR_MDNIE_1;
	mdnie_data.DSI0_RGB_SENSOR_MDNIE_2 = DSI0_RGB_SENSOR_MDNIE_2;
	mdnie_data.DSI0_RGB_SENSOR_MDNIE_3 = DSI0_RGB_SENSOR_MDNIE_3;
	mdnie_data.DSI0_TRANS_DIMMING_MDNIE = DSI0_RGB_SENSOR_MDNIE_3;
	mdnie_data.DSI0_UI_DYNAMIC_MDNIE_2 = DSI0_UI_DYNAMIC_MDNIE_2;
	mdnie_data.DSI0_UI_STANDARD_MDNIE_2 = DSI0_UI_STANDARD_MDNIE_2;
	mdnie_data.DSI0_UI_AUTO_MDNIE_2 = DSI0_UI_AUTO_MDNIE_2;
	mdnie_data.DSI0_VIDEO_DYNAMIC_MDNIE_2 = DSI0_VIDEO_DYNAMIC_MDNIE_2;
	mdnie_data.DSI0_VIDEO_STANDARD_MDNIE_2 = DSI0_VIDEO_STANDARD_MDNIE_2;
	mdnie_data.DSI0_VIDEO_AUTO_MDNIE_2 = DSI0_VIDEO_AUTO_MDNIE_2;
	mdnie_data.DSI0_CAMERA_MDNIE_2 = DSI0_CAMERA_MDNIE_2;
	mdnie_data.DSI0_CAMERA_AUTO_MDNIE_2 = DSI0_CAMERA_AUTO_MDNIE_2;
	mdnie_data.DSI0_GALLERY_DYNAMIC_MDNIE_2 = DSI0_GALLERY_DYNAMIC_MDNIE_2;
	mdnie_data.DSI0_GALLERY_STANDARD_MDNIE_2 = DSI0_GALLERY_STANDARD_MDNIE_2;
	mdnie_data.DSI0_GALLERY_AUTO_MDNIE_2 = DSI0_GALLERY_AUTO_MDNIE_2;
	mdnie_data.DSI0_VT_DYNAMIC_MDNIE_2 = DSI0_VT_DYNAMIC_MDNIE_2;
	mdnie_data.DSI0_VT_STANDARD_MDNIE_2 = DSI0_VT_STANDARD_MDNIE_2;
	mdnie_data.DSI0_VT_AUTO_MDNIE_2 = DSI0_VT_AUTO_MDNIE_2;
	mdnie_data.DSI0_BROWSER_DYNAMIC_MDNIE_2 = DSI0_BROWSER_DYNAMIC_MDNIE_2;
	mdnie_data.DSI0_BROWSER_STANDARD_MDNIE_2 = DSI0_BROWSER_STANDARD_MDNIE_2;
	mdnie_data.DSI0_BROWSER_AUTO_MDNIE_2 = DSI0_BROWSER_AUTO_MDNIE_2;
	mdnie_data.DSI0_EBOOK_DYNAMIC_MDNIE_2 = DSI0_EBOOK_DYNAMIC_MDNIE_2;
	mdnie_data.DSI0_EBOOK_STANDARD_MDNIE_2 = DSI0_EBOOK_STANDARD_MDNIE_2;
	mdnie_data.DSI0_EBOOK_AUTO_MDNIE_2 = DSI0_EBOOK_AUTO_MDNIE_2;
	mdnie_data.DSI0_TDMB_DYNAMIC_MDNIE_2 = DSI0_TDMB_DYNAMIC_MDNIE_2;
	mdnie_data.DSI0_TDMB_STANDARD_MDNIE_2 = DSI0_TDMB_STANDARD_MDNIE_2;
	mdnie_data.DSI0_TDMB_AUTO_MDNIE_2 = DSI0_TDMB_AUTO_MDNIE_2;

	mdnie_data.DSI0_BYPASS_MDNIE = DSI0_BYPASS_MDNIE;
	mdnie_data.DSI0_NEGATIVE_MDNIE = DSI0_NEGATIVE_MDNIE;
	mdnie_data.DSI0_COLOR_BLIND_MDNIE = DSI0_COLOR_BLIND_MDNIE;
	mdnie_data.DSI0_HBM_CE_MDNIE = DSI0_HBM_CE_MDNIE;
	mdnie_data.DSI0_RGB_SENSOR_MDNIE = DSI0_RGB_SENSOR_MDNIE;
	mdnie_data.DSI0_UI_DYNAMIC_MDNIE = DSI0_UI_DYNAMIC_MDNIE;
	mdnie_data.DSI0_UI_STANDARD_MDNIE = DSI0_UI_STANDARD_MDNIE;
	mdnie_data.DSI0_UI_NATURAL_MDNIE = DSI0_UI_NATURAL_MDNIE;
	mdnie_data.DSI0_UI_MOVIE_MDNIE = DSI0_UI_MOVIE_MDNIE;
	mdnie_data.DSI0_UI_AUTO_MDNIE = DSI0_UI_AUTO_MDNIE;
	mdnie_data.DSI0_VIDEO_OUTDOOR_MDNIE = NULL;
	mdnie_data.DSI0_VIDEO_DYNAMIC_MDNIE = DSI0_VIDEO_DYNAMIC_MDNIE;
	mdnie_data.DSI0_VIDEO_STANDARD_MDNIE = DSI0_VIDEO_STANDARD_MDNIE;
	mdnie_data.DSI0_VIDEO_NATURAL_MDNIE = DSI0_VIDEO_NATURAL_MDNIE;
	mdnie_data.DSI0_VIDEO_MOVIE_MDNIE = DSI0_VIDEO_MOVIE_MDNIE;
	mdnie_data.DSI0_VIDEO_AUTO_MDNIE = DSI0_VIDEO_AUTO_MDNIE;
	mdnie_data.DSI0_VIDEO_WARM_OUTDOOR_MDNIE = DSI0_VIDEO_WARM_OUTDOOR_MDNIE;
	mdnie_data.DSI0_VIDEO_WARM_MDNIE = DSI0_VIDEO_WARM_MDNIE;
	mdnie_data.DSI0_VIDEO_COLD_OUTDOOR_MDNIE = DSI0_VIDEO_COLD_OUTDOOR_MDNIE;
	mdnie_data.DSI0_VIDEO_COLD_MDNIE = DSI0_VIDEO_COLD_MDNIE;
	mdnie_data.DSI0_CAMERA_OUTDOOR_MDNIE = DSI0_CAMERA_OUTDOOR_MDNIE;
	mdnie_data.DSI0_CAMERA_MDNIE = DSI0_CAMERA_MDNIE;
	mdnie_data.DSI0_CAMERA_AUTO_MDNIE = DSI0_CAMERA_AUTO_MDNIE;
	mdnie_data.DSI0_GALLERY_DYNAMIC_MDNIE = DSI0_GALLERY_DYNAMIC_MDNIE;
	mdnie_data.DSI0_GALLERY_STANDARD_MDNIE = DSI0_GALLERY_STANDARD_MDNIE;
	mdnie_data.DSI0_GALLERY_NATURAL_MDNIE = DSI0_GALLERY_NATURAL_MDNIE;
	mdnie_data.DSI0_GALLERY_MOVIE_MDNIE = DSI0_GALLERY_MOVIE_MDNIE;
	mdnie_data.DSI0_GALLERY_AUTO_MDNIE = DSI0_GALLERY_AUTO_MDNIE;
	mdnie_data.DSI0_VT_DYNAMIC_MDNIE = DSI0_VT_DYNAMIC_MDNIE;
	mdnie_data.DSI0_VT_STANDARD_MDNIE = DSI0_VT_STANDARD_MDNIE;
	mdnie_data.DSI0_VT_NATURAL_MDNIE = DSI0_VT_NATURAL_MDNIE;
	mdnie_data.DSI0_VT_MOVIE_MDNIE = DSI0_VT_MOVIE_MDNIE;
	mdnie_data.DSI0_VT_AUTO_MDNIE = DSI0_VT_AUTO_MDNIE;
	mdnie_data.DSI0_BROWSER_DYNAMIC_MDNIE = DSI0_BROWSER_DYNAMIC_MDNIE;
	mdnie_data.DSI0_BROWSER_STANDARD_MDNIE = DSI0_BROWSER_STANDARD_MDNIE;
	mdnie_data.DSI0_BROWSER_NATURAL_MDNIE = DSI0_BROWSER_NATURAL_MDNIE;
	mdnie_data.DSI0_BROWSER_MOVIE_MDNIE = DSI0_BROWSER_MOVIE_MDNIE;
	mdnie_data.DSI0_BROWSER_AUTO_MDNIE = DSI0_BROWSER_AUTO_MDNIE;
	mdnie_data.DSI0_EBOOK_DYNAMIC_MDNIE = DSI0_EBOOK_DYNAMIC_MDNIE;
	mdnie_data.DSI0_EBOOK_STANDARD_MDNIE = DSI0_EBOOK_STANDARD_MDNIE;
	mdnie_data.DSI0_EBOOK_NATURAL_MDNIE = DSI0_EBOOK_NATURAL_MDNIE;
	mdnie_data.DSI0_EBOOK_MOVIE_MDNIE = DSI0_EBOOK_MOVIE_MDNIE;
	mdnie_data.DSI0_EBOOK_AUTO_MDNIE = DSI0_EBOOK_AUTO_MDNIE;
	mdnie_data.DSI0_EMAIL_AUTO_MDNIE = DSI0_EMAIL_AUTO_MDNIE;
	mdnie_data.DSI0_GAME_LOW_MDNIE = DSI0_GAME_LOW_MDNIE;
	mdnie_data.DSI0_GAME_MID_MDNIE = DSI0_GAME_MID_MDNIE;
	mdnie_data.DSI0_GAME_HIGH_MDNIE = DSI0_GAME_HIGH_MDNIE;
	mdnie_data.DSI0_TDMB_DYNAMIC_MDNIE = DSI0_TDMB_DYNAMIC_MDNIE;
	mdnie_data.DSI0_TDMB_STANDARD_MDNIE = DSI0_TDMB_STANDARD_MDNIE;
	mdnie_data.DSI0_TDMB_NATURAL_MDNIE = DSI0_TDMB_NATURAL_MDNIE;
	mdnie_data.DSI0_TDMB_MOVIE_MDNIE = DSI0_TDMB_MOVIE_MDNIE;
	mdnie_data.DSI0_TDMB_AUTO_MDNIE = DSI0_TDMB_AUTO_MDNIE;
	mdnie_data.DSI0_GRAYSCALE_MDNIE = DSI0_GRAYSCALE_MDNIE;
	mdnie_data.DSI0_GRAYSCALE_NEGATIVE_MDNIE= DSI0_GRAYSCALE_NEGATIVE_MDNIE;
	mdnie_data.DSI0_CURTAIN = DSI0_SCREEN_CURTAIN_MDNIE;
	mdnie_data.DSI0_NIGHT_MODE_MDNIE = DSI0_NIGHT_MODE_MDNIE;
	mdnie_data.DSI0_NIGHT_MODE_MDNIE_1 = DSI0_NIGHT_MODE_MDNIE_1;
	mdnie_data.DSI0_COLOR_BLIND_MDNIE_SCR = DSI0_COLOR_BLIND_MDNIE_1;
	mdnie_data.DSI0_RGB_SENSOR_MDNIE_SCR = DSI0_RGB_SENSOR_MDNIE_1;

	mdnie_data.mdnie_tune_value_dsi0 = mdnie_tune_value_dsi0;
	mdnie_data.mdnie_tune_value_dsi1 = mdnie_tune_value_dsi0;
	mdnie_data.hmt_color_temperature_tune_value_dsi0 = hmt_color_temperature_tune_value_dsi0;
	mdnie_data.hmt_color_temperature_tune_value_dsi1 = hmt_color_temperature_tune_value_dsi0;

	mdnie_data.hdr_tune_value_dsi0 = hdr_tune_value_dsi0;
	mdnie_data.hdr_tune_value_dsi1 = hdr_tune_value_dsi0;

	/* Update MDNIE data related with size, offset or index */
	mdnie_data.dsi0_bypass_mdnie_size = ARRAY_SIZE(DSI0_BYPASS_MDNIE);
	mdnie_data.mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
	mdnie_data.mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
	mdnie_data.mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
	mdnie_data.mdnie_step_index[MDNIE_STEP3] = MDNIE_STEP3_INDEX;
	mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = ADDRESS_SCR_WHITE_RED;
	mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = ADDRESS_SCR_WHITE_GREEN;
	mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = ADDRESS_SCR_WHITE_BLUE;
	mdnie_data.dsi0_rgb_sensor_mdnie_1_size = DSI0_RGB_SENSOR_MDNIE_1_SIZE;
	mdnie_data.dsi0_rgb_sensor_mdnie_2_size = DSI0_RGB_SENSOR_MDNIE_2_SIZE;
	mdnie_data.dsi0_rgb_sensor_mdnie_3_size = DSI0_RGB_SENSOR_MDNIE_3_SIZE;

	mdnie_data.dsi0_trans_dimming_data_index = MDNIE_TRANS_DIMMING_DATA_INDEX;

	mdnie_data.dsi0_adjust_ldu_table = adjust_ldu_data;
	mdnie_data.dsi1_adjust_ldu_table = adjust_ldu_data;
	mdnie_data.dsi0_max_adjust_ldu = 6;
	mdnie_data.dsi1_max_adjust_ldu = 6;
	mdnie_data.dsi0_night_mode_table = night_mode_data;
	mdnie_data.dsi1_night_mode_table = night_mode_data;
	mdnie_data.dsi0_max_night_mode_index = 11;
	mdnie_data.dsi1_max_night_mode_index = 11;
	mdnie_data.dsi0_scr_step_index = MDNIE_STEP1_INDEX;
	mdnie_data.dsi1_scr_step_index = MDNIE_STEP1_INDEX;
}

static void  mdss_panel_init(struct samsung_display_driver_data *vdd)
{
	LCD_ERR("%s", vdd->panel_name);

	vdd->support_mdnie_lite = true;

	vdd->support_mdnie_trans_dimming = true;

	vdd->mdnie_tune_size1 = sizeof(DSI0_BYPASS_MDNIE_1);
	vdd->mdnie_tune_size2 = sizeof(DSI0_BYPASS_MDNIE_2);
	vdd->mdnie_tune_size3 = sizeof(DSI0_BYPASS_MDNIE_3);

	/* ON/OFF */
	vdd->panel_func.samsung_panel_on_pre = mdss_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = mdss_panel_on_post;
	vdd->panel_func.samsung_panel_off_post = samsung_panel_off_post;

	/* DDI RX */
	vdd->panel_func.samsung_panel_revision = mdss_panel_revision;
	vdd->panel_func.samsung_manufacture_date_read = mdss_manufacture_date_read;
	vdd->panel_func.samsung_ddi_id_read = mdss_ddi_id_read;

	vdd->panel_func.samsung_hbm_read = NULL;
	vdd->panel_func.samsung_mdnie_read = NULL;
	vdd->panel_func.samsung_smart_dimming_init = NULL;
	vdd->panel_func.samsung_smart_get_conf = NULL;

	vdd->panel_func.samsung_elvss_read = mdss_elvss_read;
	vdd->panel_func.samsung_hbm_read = mdss_hbm_read;
	vdd->panel_func.samsung_mdnie_read = mdss_mdnie_read;
	vdd->panel_func.samsung_smart_dimming_init = mdss_samart_dimming_init;
	vdd->panel_func.samsung_smart_get_conf = smart_get_conf_S6E3HA5_AMB567MK01;

	vdd->panel_func.samsung_cell_id_read = mdss_cell_id_read;
	vdd->panel_func.samsung_octa_id_read = mdss_octa_id_read;

	/* Brightness */
	vdd->panel_func.samsung_brightness_hbm_off = NULL;
	vdd->panel_func.samsung_brightness_aid = mdss_aid;
	vdd->panel_func.samsung_brightness_acl_on = mdss_acl_on;
	vdd->panel_func.samsung_brightness_acl_percent = NULL;
	vdd->panel_func.samsung_brightness_acl_off = mdss_acl_off;
	vdd->panel_func.samsung_brightness_elvss = mdss_elvss;
	vdd->panel_func.samsung_brightness_elvss_temperature1 = mdss_elvss_temperature1;
	vdd->panel_func.samsung_brightness_elvss_temperature2 = NULL;
	vdd->panel_func.samsung_brightness_vint = mdss_vint;
	vdd->panel_func.samsung_brightness_irc = mdss_irc;
	vdd->panel_func.samsung_brightness_gamma = mdss_gamma;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = mdss_hbm_gamma;
	vdd->panel_func.samsung_hbm_etc = mdss_hbm_etc;
	vdd->panel_func.samsung_hbm_irc = mdss_hbm_irc;
	vdd->panel_func.get_hbm_candela_value = get_hbm_candela_value;

	/* Event */
	vdd->panel_func.samsung_change_ldi_fps = NULL;

	/* HMT */
	vdd->panel_func.samsung_brightness_gamma_hmt = mdss_gamma_hmt;
	vdd->panel_func.samsung_brightness_aid_hmt = mdss_aid_hmt;
	vdd->panel_func.samsung_brightness_elvss_hmt = mdss_elvss_hmt;
	vdd->panel_func.samsung_brightness_vint_hmt = mdss_vint_hmt;
	vdd->panel_func.samsung_smart_dimming_hmt_init = mdss_samart_dimming_init_hmt;
	vdd->panel_func.samsung_smart_get_conf_hmt = smart_get_conf_S6E3HA5_AMB567MK01_hmt;

	dsi_update_mdnie_data();

	/* Panel LPM */
	vdd->panel_func.samsung_get_panel_lpm_mode = mdss_get_panel_lpm_mode;

	/* send recovery pck before sending image date (for ESD recovery) */
	vdd->send_esd_recovery = false;

	vdd->auto_brightness_level = 12;

	/* Enable panic on first pingpong timeout */
	vdd->debug_data->panic_on_pptimeout = true;

	/* Set IRC init value */
	vdd->irc_info.irc_enable_status = true;
	vdd->irc_info.irc_mode = IRC_MODERATO_MODE;

	/* COLOR WEAKNESS */
	vdd->panel_func.color_weakness_ccb_on_off =  mdss_send_colorweakness_ccb_cmd;

	/* Support DDI HW CURSOR */
	vdd->panel_func.ddi_hw_cursor = ddi_hw_cursor;

	/* MULTI_RESOLUTION */
	vdd->panel_func.samsung_multires = mdss_panel_multires;

	/* COVER Open/Close */
	vdd->panel_func.samsung_cover_control = NULL;

	/* ACL default ON */
	vdd->acl_status = 1;
}

static int __init samsung_panel_init(void)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	char panel_string[] = "ss_dsi_panel_S6E3HA5_AMB567MK01_WQHD";

	vdd->panel_name = mdss_mdp_panel + 8;

	LCD_INFO("%s \n", vdd->panel_name);

	if (!strncmp(vdd->panel_name, panel_string, strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = mdss_panel_init;

	return 0;
}

early_initcall(samsung_panel_init);
