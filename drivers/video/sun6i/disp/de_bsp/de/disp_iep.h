#ifndef __DISP_IEP_H_
#define __DISP_IEP_H_

#include "disp_display_i.h"
#include "disp_display.h"


#define DRC_USED 						0x04000000
#define DRC_USED_MASK 					(~(DRC_USED))
#define CMU_SCREEN_EN                   0x10000000
#define CMU_SCREEN_EN_MASK              (~(CMU_SCREEN_EN))
#define CMU_LAYER_EN                    0x20000000
#define CMU_LAYER_EN_MASK               (~(CMU_LAYER_EN))

typedef enum
{
    IEP_DRC_MODE_UI,
    IEP_DRC_MODE_VIDEO,
}__iep_drc_mode_t;

typedef struct{
    __bool b_interlace_out;
    __bool b_trd_out;
    __scal_3d_outmode_t trd_out_mode;
    __disp_rectsz_t in_size;
    __disp_rectsz_t out_size;
}__disp_frame_info_t;


//DEU:

extern __s32 IEP_Deu_Enable(__u32 sel, __u32 enable);
extern __s32 IEP_Deu_Get_Enable(__u32 sel, __u32 enable);
extern __s32 IEP_Deu_Set_Luma_Sharpness_Level(__u32 sel, __u32 level, __disp_frame_info_t frame_info);
extern __s32 IEP_Deu_Set_Chroma_Sharpness_Level(__u32 sel, __u32 level);
extern __s32 IEP_Deu_Set_White_Level_Extension(__u32 sel, __u32 level);
extern __s32 IEP_Deu_Set_Black_Level_Extension(__u32 sel, __u32 level);
extern __s32 IEP_Deu_Get_Luma_Sharpness_Level(__u32 sel);
extern __s32 DE_Deu_Get_Chroma_Sharpness_Level(__u32 sel);
extern __s32 DE_Deu_Get_White_Level_Extension(__u32 sel);
extern __s32 DE_Deu_Get_Black_Level_Extension(__u32 sel);
extern __s32 IEP_Deu_Set_Ready(__u32 sel);
extern __s32 IEP_Deu_Set_Reg_base(__u32 sel);
extern __s32 IEP_Deu_Set_Winodw(__u32 sel, __disp_rect_t window);//full screen for default
extern __s32 IEP_Deu_Output_Select(__u32 sel, __u32 be_ch);
extern __s32 IEP_Deu_Init(__u32 sel);
extern __s32 IEP_Deu_Exit(__u32 sel);
extern __s32 IEP_Deu_Operation_In_Vblanking(__u32 sel);
extern __s32 IEP_Deu_Early_Suspend(__u32 sel);//close clk
extern __s32 IEP_Deu_suspend(__u32 sel);//save register
extern __s32 IEP_Deu_Resume (__u32 sel);//restore register
extern __s32 IEP_Deu_Late_Resume(__u32 sel);//open clk



//DRC:
#if 0
extern __s32 IEP_Drc_Init(__u32 sel);
extern __s32 IEP_Drc_Exit(__u32 sel);
extern __s32 IEP_Drc_Enable(__u32 sel, __u32 en);
extern __s32 IEP_Drc_Set_Mode(__u32 sel, __u32 mode);
extern __s32 IEP_Drc_Set_Imgsize(__u32 sel, __u32 width, __u32 height);
extern __s32 IEP_Drc_Operation_In_Vblanking(__u32 sel, __u32 mode);
extern __s32 IEP_Drc_Set_Reg_Base(__u32 sel, __u32 base);
extern __s32 IEP_Drc_Set_Winodw(__u32 sel, __disp_rect_t *window);//full screen for default
extern __s32 IEP_Drc_Get_Winodw(__u32 sel, __disp_rect_t *window);//full screen for default
extern __s32 IEP_Drc_Get_Bright_Diming(__u32 sel);
extern __s32 IEP_Drc_Early_Suspend(__u32 sel);//close clk
extern __s32 IEP_Drc_suspend(__u32 sel);//save register
extern __s32 IEP_Drc_Resume (__u32 sel);//restore register
extern __s32 IEP_Drc_Late_Resume(__u32 sel);//open clk
#endif

__s32 Disp_drc_start_video_mode(__u32 sel);
__s32 Disp_drc_start_ui_mode(__u32 sel);


#endif
