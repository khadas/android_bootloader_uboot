/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _OSD_HW_H_
#define _OSD_HW_H_

#include <asm/arch/register.h>
#include "osd.h"

#define REG_OFFSET (0x20)
#define OSD_RELATIVE_BITS 0x33370

#ifdef AML_S5_DISPLAY
#define SLICE_NUM 4
#endif

extern int osd_get_chip_type(void);
void osd_init_hw(u32 index);
extern void osd_set_color_key_hw(u32 index, u32 bpp, u32 colorkey);
extern void osd_srckey_enable_hw(u32  index, u8 enable);
extern void osd_set_gbl_alpha_hw(u32 index, u32 gbl_alpha);
extern u32 osd_get_gbl_alpha_hw(u32  index);
extern int read_rdma_table(void);
extern void osd_set_color_mode(u32 index,
			       const struct color_bit_define_s *color);
extern void osd_update_disp_axis_hw(
	u32 index,
	u32 display_h_start,
	u32 display_h_end,
	u32 display_v_start,
	u32 display_v_end,
	u32 xoffset,
	u32 yoffset,
	u32 mode_change);
extern void osd_setup_hw(u32 index,
			 u32 xoffset,
			 u32 yoffset,
			 u32 xres,
			 u32 yres,
			 u32 xres_virtual ,
			 u32 yres_virtual,
			 u32 disp_start_x,
			 u32 disp_start_y,
			 u32 disp_end_x,
			 u32 disp_end_y,
			 u32 fbmem,
			 const struct color_bit_define_s *color);
extern void osd_set_order_hw(u32 index, u32 order);
extern void osd_get_order_hw(u32 index, u32 *order);
extern void osd_set_free_scale_enable_hw(u32 index, u32 enable);
extern void osd_get_free_scale_enable_hw(u32 index, u32 *free_scale_enable);
extern void osd_set_free_scale_mode_hw(u32 index, u32 freescale_mode);
extern void osd_get_free_scale_mode_hw(u32 index, u32 *freescale_mode);
extern void osd_set_4k2k_fb_mode_hw(u32 fb_for_4k2k);
extern void osd_get_free_scale_mode_hw(u32 index, u32 *freescale_mode);
extern void osd_set_free_scale_width_hw(u32 index, u32 width);
extern void osd_get_free_scale_width_hw(u32 index, u32 *free_scale_width);
extern void osd_set_free_scale_height_hw(u32 index, u32 height);
extern void osd_get_free_scale_height_hw(u32 index, u32 *free_scale_height);
extern void osd_get_free_scale_axis_hw(u32 index, s32 *x0, s32 *y0, s32 *x1,
				       s32 *y1);
extern void osd_set_free_scale_axis_hw(u32 index, s32 x0, s32 y0, s32 x1,
				       s32 y1);
extern void osd_get_scale_axis_hw(u32 index, s32 *x0, s32 *y0, s32 *x1,
				  s32 *y1);
extern void osd_get_window_axis_hw(u32 index, s32 *x0, s32 *y0, s32 *x1,
				   s32 *y1);
extern void osd_set_window_axis_hw(u32 index, s32 x0, s32 y0, s32 x1, s32 y1);
extern void osd_set_scale_axis_hw(u32 index, s32 x0, s32 y0, s32 x1, s32 y1);
extern void osd_get_block_windows_hw(u32 index, u32 *windows);
extern void osd_set_block_windows_hw(u32 index, u32 *windows);
extern void osd_get_block_mode_hw(u32 index, u32 *mode);
extern void osd_set_block_mode_hw(u32 index, u32 mode);
extern void osd_enable_3d_mode_hw(u32 index, u32 enable);
extern void osd_set_2x_scale_hw(u32 index, u16 h_scale_enable,
				u16 v_scale_enable);
extern void osd_get_flush_rate_hw(u32 *break_rate);
extern void osd_set_reverse_hw(u32 index, u32 reverse);
extern void osd_get_reverse_hw(u32 index, u32 *reverse);
extern void osd_set_rotate_on_hw(u32 index, u32 on_off);
extern void osd_get_rotate_on_hw(u32 index, u32 *on_off);
extern void osd_set_antiflicker_hw(u32 index, u32 vmode, u32 yres);
extern void osd_get_antiflicker_hw(u32 index, u32 *on_off);
extern void osd_set_update_state_hw(u32 index, u32 up_free);
extern void osd_get_update_state_hw(u32 index, u32 *up_free);
extern void osd_get_angle_hw(u32 index, u32 *angle);
extern void osd_set_angle_hw(u32 index, u32 angle, u32  virtual_osd1_yres,
			     u32 virtual_osd2_yres);
extern void osd_get_clone_hw(u32 index, u32 *clone);
extern void osd_set_clone_hw(u32 index, u32 clone);
extern void osd_set_update_pan_hw(u32 index);
extern void osd_set_rotate_angle_hw(u32 index, u32 angle);
extern void osd_get_rotate_angle_hw(u32 index, u32 *angle);
extern void osd_get_prot_canvas_hw(u32 index, s32 *x_start, s32 *y_start,
				   s32 *x_end, s32 *y_end);
extern void osd_set_prot_canvas_hw(u32 index, s32 x_start, s32 y_start,
				   s32 x_end, s32 y_end);
extern void osd_setpal_hw(u32 index, unsigned regno, unsigned red,
			  unsigned green, unsigned blue, unsigned transp);
extern void osd_enable_hw(u32 index, u32 enable);
extern void osd_pan_display_hw(u32 index, unsigned int xoffset,
			       unsigned int yoffset);
extern void osd_get_hw_para(struct hw_para_s **para);
extern void osd_update_blend(struct pandata_s *disp_data);
extern void osd_hist_enable(u32 osd_index);
extern int osd_get_hist_stat(u32 *hist_result);
#ifdef AML_C3_DISPLAY
int test_for_c3(u32 osd_index, u32 fb_data);
void osd_update_blend_c3(void);
#endif
#ifdef AML_S5_DISPLAY
void vpp_post_blend_set(u32 vpp_index, struct vpp_post_blend_s *vpp_blend);
void vpp1_post_blend_set(struct vpp1_post_blend_s *vpp_blend);
void vpp_post_slice_set(u32 vpp_index, struct vpp0_post_s *vpp_post);
void vpp_vd1_hwin_set(u32 vpp_index, struct vpp0_post_s *vpp_post);
void vpp_post_proc_set(u32 vpp_index, struct vpp0_post_s *vpp_post);
void vpp1_post_proc_set(struct vpp1_post_s *vpp_post);
void vpp_post_padding_set(u32 vpp_index, struct vpp0_post_s *vpp_post);
#endif
void osd_init_hw_viux(u32 index);
u32 osd_get_state(u32 index);

#ifdef VEHICLE_CONFIG
void osd_set_config_finish(void);
void transfer_info_to_rtos(void);
bool is_osd2_configed(void);
void osd2_config_with_dimm(int *axis);
void osd2_setup_hw(u32 index,
		  u32 xoffset,
		  u32 yoffset,
		  u32 xres,
		  u32 yres,
		  u32 xres_virtual,
		  u32 yres_virtual,
		  u32 disp_start_x,
		  u32 disp_start_y,
		  u32 disp_end_x,
		  u32 disp_end_y,
		  u32 fbmem,
		  const struct color_bit_define_s *color);
#endif
void osd_set_dimm(u32 index, u32 dim_color, u32 en);
void stop_osd_log(void);
void start_osd_log(void);

#endif
