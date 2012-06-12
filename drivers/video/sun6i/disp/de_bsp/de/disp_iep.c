#include "disp_iep.h"

extern __disp_dev_t         gdisp;

__s32 iep_init(__u32 sel)
{
    __iep_cmu_t *cmu;
    __iep_drc_t *drc;

    //IEP_Deu_Init(sel);
    IEP_Drc_Init(sel);
    //IEP_CMU_Init(sel);


    cmu = &gdisp.screen[sel].cmu;
    drc = &gdisp.screen[sel].drc;

    cmu->status = 0;
	cmu->layer_mode = DISP_ENHANCE_MODE_VIVID;
    memset(&cmu->layer_rect, 0,sizeof(__disp_rect_t));
    cmu->layer_bright = 50;
    cmu->layer_saturation = 50;
    cmu->layer_contrast = 50;
    cmu->layer_hue = 50;


    cmu->screen_mode = DISP_ENHANCE_MODE_VIVID;
    memset(&cmu->screen_rect, 0,sizeof(__disp_rect_t));
    cmu->screen_bright = 50;
    cmu->screen_saturation = 50;
    cmu->screen_contrast = 50;
    cmu->screen_hue = 50;

    drc->enable = 0;
    drc->mode = IEP_DRC_MODE_UI;
    memset(&drc->rect, 0, sizeof(__disp_rect_t));
    drc->rect.x = 0;
    drc->rect.y = 0;
    drc->rect.width = BSP_disp_get_screen_width(sel);
    drc->rect.height = BSP_disp_get_screen_height(sel);

    return DIS_SUCCESS;
}

__s32 iep_exit(__u32 sel)
{
    //IEP_Deu_Exit(sel);
    //IEP_Drc_Exit(sel);
    //IEP_CMU_Exit(sel);
    return DIS_SUCCESS;
}
#define ____SEPARATOR_DRC____
//todo : csc->set_mode   or set_mode->csc?
__s32 BSP_disp_drc_enable(__u32 sel, __u32 en)
{
    if(DISP_OUTPUT_TYPE_LCD == BSP_disp_get_output_type(sel))
    {
        __iep_drc_t *drc;

        drc = &gdisp.screen[sel].drc;

        if(1 == en)
        {
            IEP_Drc_Set_Imgsize(sel, BSP_disp_get_screen_width(sel), BSP_disp_get_screen_height(sel));
            IEP_Drc_Set_Winodw(sel, drc->rect);
            IEP_Drc_Set_Mode(sel,drc->mode);
            if(drc->mode == IEP_DRC_MODE_VIDEO)//video mode
            {
                //todo?  yuv output
                DE_BE_Set_Enhance_ex(sel, 1, 1, 0,50, 50, 50,50);
            }
            IEP_Drc_Enable(sel,TRUE);
            gdisp.screen[sel].drc.enable = 1;
        }else//0, 2
        {
            IEP_Drc_Enable(sel,en);
            if(drc->mode == IEP_DRC_MODE_VIDEO)//video mode
            {
                //todo? change to RGB output
                DE_BE_Set_Enhance_ex(sel, 0,0, 0,50, 50, 50,50);
            }
            gdisp.screen[sel].drc.enable = 0;
        }

        return DIS_SUCCESS;
    }

    return DIS_NOT_SUPPORT;


}

__s32 BSP_disp_drc_get_enable(__u32 sel)
{
    if(DISP_OUTPUT_TYPE_LCD == BSP_disp_get_output_type(sel))
    {
        return gdisp.screen[sel].drc.enable;
    }

    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_drc_set_window(__u32 sel,__disp_rect_t *regn)
{
    if(DISP_OUTPUT_TYPE_LCD == BSP_disp_get_output_type(sel))
    {
        memcpy(&gdisp.screen[sel].drc.rect, regn, sizeof(__disp_rect_t));
        if(BSP_disp_drc_get_enable(sel))
        {
            IEP_Drc_Set_Winodw(sel,*regn);
        }
        return DIS_SUCCESS;
    }

    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_drc_get_window(__u32 sel,__disp_rect_t *regn)
{
    if(DISP_OUTPUT_TYPE_LCD == BSP_disp_get_output_type(sel))
    {
        memcpy(regn, &gdisp.screen[sel].drc.rect, sizeof(__disp_rect_t));
        return DIS_SUCCESS;
    }

    return DIS_NOT_SUPPORT;
}


__s32 BSP_disp_drc_set_mode(__u32 sel,__u32 mode)
{
    if(DISP_OUTPUT_TYPE_LCD == BSP_disp_get_output_type(sel))
    {
        gdisp.screen[sel].drc.mode = mode;
        if(BSP_disp_drc_get_enable(sel))
        {
            IEP_Drc_Set_Mode(sel,mode);
        }
        return DIS_SUCCESS;
    }

    return DIS_NOT_SUPPORT;
}
__s32 BSP_disp_drc_get_mode(__u32 sel)
{
    if(DISP_OUTPUT_TYPE_LCD == BSP_disp_get_output_type(sel))
    {
        return gdisp.screen[sel].drc.mode;
    }

    return DIS_NOT_SUPPORT;
}


__s32 Disp_drc_start_video_mode(__u32 sel)
{
    gdisp.screen[sel].drc.mode = IEP_DRC_MODE_VIDEO;
    if(BSP_disp_drc_get_enable(sel))
    {
        IEP_Drc_Set_Mode(sel,gdisp.screen[sel].drc.mode);
    }
    return DIS_SUCCESS;
}

__s32 Disp_drc_start_ui_mode(__u32 sel)
{
    gdisp.screen[sel].drc.mode = IEP_DRC_MODE_UI;
    if(BSP_disp_drc_get_enable(sel))
    {
        IEP_Drc_Set_Mode(sel,gdisp.screen[sel].drc.mode);
    }

    return DIS_SUCCESS;
}

#define ____SEPARATOR_DEU____
#if 0
__s32 BSP_disp_deu_enable(__u8 sel, __u32 hid,  __u32 enable)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        __disp_scaler_t * scaler;

        scaler = &(gdisp.scaler[layer_man->scaler_index]);

        if(enable && (!gdisp.scaler[layer_man->scaler_index].deu.enable) &&
        ((!scaler->b_trd_out) || ((scaler->b_trd_out) && scaler->out_trd_mode
        != DISP_3D_OUT_MODE_CI_1))
        {
            __disp_frame_info_t frame_info;

            scaler->out_fb.seq= DISP_SEQ_P3210;
            scaler->out_fb.format= DISP_FORMAT_YUV444;
            Scaler_Set_Para(layer_man->scaler_index,scaler);

            frame_info.b_interlace_out = Disp_get_screen_scan_mode(sel);
            frame_info.b_trd_out = layer_man->para.b_trd_out;
            frame_info.trd_out_mode = layer_man->para.out_trd_mode;
            memcpy(&frame_info.in_size, &scaler->in_fb.size, sizeof(__disp_rectsz_t));
            memcpy(&frame_info.out_size, &scaler->out_fb.size, sizeof(__disp_rectsz_t));
            IEP_Deu_Set_Luma_Sharpness_Level(sel, scaler->deu.luma_sharpe_level, frame_info);
            IEP_Deu_Set_Chroma_Sharpness_Level(sel, scaler->deu.chroma_sharpe_level);
            IEP_Deu_Set_Black_Level_Extension(sel, scaler->deu.black_exten_level);
            IEP_Deu_Set_White_Level_Extension(sel, scaler->deu.while_exten_level);
            IEP_Deu_Enable(layer_man->scaler_index, enable);
        }else(!enable && gdisp.scaler[layer_man->scaler_index].deu.enable)
        {
            scaler->out_fb.seq= DISP_SEQ_ARGB;
            scaler->out_fb.format= DISP_FORMAT_RGB888;
            Scaler_Set_Para(layer_man->scaler_index,scaler);
            IEP_Deu_Enable(layer_man->scaler_index, enable);
        }

        gdisp.scaler[layer_man->scaler_index].deu.enable = enable;
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}


__s32 BSP_disp_deu_get_enable(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        return gdisp.scaler[layer_man->scaler_index].deu.enable;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_deu_set_luma_sharpness_level(__u32 sel, __u32 hid,__u32 level)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        __disp_frame_info_t frame_info;
        __disp_scaler_t * scaler;

        scaler = &(gdisp.scaler[layer_man->scaler_index]);

        scaler->deu.luma_sharpe_level = level;
        if(scaler->deu.enable)
        {
            frame_info.b_interlace_out = Disp_get_screen_scan_mode(sel);
            frame_info.b_trd_out = layer_man->para.b_trd_out;
            frame_info.trd_out_mode = layer_man->para.out_trd_mode;
            memcpy(&frame_info.in_size, &scaler->in_fb.size, sizeof(__disp_rectsz_t));
            memcpy(&frame_info.out_size, &scaler->out_fb.size, sizeof(__disp_rectsz_t));
            IEP_Deu_Set_Luma_Sharpness_Level(layer_man->scaler_index, scaler->deu.luma_sharpe_level, frame_info);
        }
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_deu_get_luma_sharpness_level(__u32 sel, __u32 hid,__u32 level)

{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        return gdisp.scaler[layer_man->scaler_index].deu.luma_sharpe_level;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_deu_set_chroma_sharp_level(__u32 sel, __u32 hid, __u32 level)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        gdisp.scaler[layer_man->scaler_index].deu.chroma_sharpe_level = level;
        if(gdisp.scaler[layer_man->scaler_index].deu.enable)
        {
            IEP_Deu_Set_Chroma_Sharpness_Level(layer_man->scaler_index,level);
        }
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_deu_get_chroma_sharp_level(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        return gdisp.scaler[layer_man->scaler_index].deu.chroma_sharpe_level;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_deu_set_white_exten_level(__u32 sel, __u32 hid, __u32 level)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        gdisp.scaler[layer_man->scaler_index].deu.while_exten_level = level;
        if(gdisp.scaler[layer_man->scaler_index].deu.enable)
        {
            IEP_Deu_Set_White_Level_Extension(layer_man->scaler_index,level);
        }

        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_deu_get_white_exten_level(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        return gdisp.scaler[layer_man->scaler_index].deu.while_exten_level;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_deu_set_black_exten_level(__u32 sel, __u32 hid, __u32 level)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        gdisp.scaler[layer_man->scaler_index].deu.black_exten_level = level;
        if(gdisp.scaler[layer_man->scaler_index].deu.enable)
        {
        IEP_Deu_Set_Black_Level_Extension(layer_man->scaler_index,level);
        }
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_deu_get_black_exten_level(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        return gdisp.scaler[layer_man->scaler_index].black_exten_level;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_deu_set_window(__u32 sel, __u32 hid, __disp_rect_t *rect)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        IEP_Deu_Set_Winodw(layer_man->scaler_index,rect);
    }
    return DIS_NOT_SUPPORT;
}


#endif

#define ____SEPARATOR_CMU____

__s32 BSP_disp_cmu_layer_enable(__u32 sel,__u32 hid, __bool en)
{
	__layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
	{
	    if(en && !(gdisp.screen[sel].cmu.status & CMU_SCREEN_EN))
        {
            IEP_CMU_Set_Imgsize(sel,layer_man->para.scn_win.width, layer_man->para.scn_win.height);
            IEP_CMU_Set_Par(sel,gdisp.screen[sel].cmu.layer_hue,gdisp.screen[sel].cmu.layer_saturation,
                gdisp.screen[sel].cmu.layer_bright,gdisp.screen[sel].cmu.layer_mode);
            IEP_CMU_Set_Data_Flow(sel,layer_man->scaler_index+1);//fe0 : 1, fe1 :2
            IEP_CMU_Set_Window(sel,&gdisp.screen[sel].cmu.layer_rect);
            IEP_CMU_Enable(sel, TRUE);
            gdisp.screen[sel].cmu.status |= CMU_LAYER_EN;
        }else
        {
            IEP_CMU_Enable(sel, FALSE);
            gdisp.screen[sel].cmu.status &= CMU_LAYER_EN_MASK;
        }
		return DIS_SUCCESS;
	}

	return DIS_NOT_SUPPORT;

}

__s32 BSP_disp_cmu_layer_get_enable(__u32 sel,__u32 hid)
{
	__layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);
    __inf("BSP_disp_cmu_layer_get_enable, sel=%d, hid=%d, status=0x%08x\n", sel, hid, gdisp.screen[sel].cmu.status);
    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
	{
        return (gdisp.screen[sel].cmu.status & CMU_LAYER_EN);
	}

	return DIS_NOT_SUPPORT;
}

__s32 disp_cmu_layer_clear(__u32 sel)
{
	gdisp.screen[sel].cmu.layer_mode = DISP_ENHANCE_MODE_STANDARD;
    memset(&gdisp.screen[sel].cmu.layer_rect, 0,sizeof(__disp_rect_t));
    gdisp.screen[sel].cmu.layer_bright = 50;
    gdisp.screen[sel].cmu.layer_saturation = 50;
    gdisp.screen[sel].cmu.layer_contrast = 50;
    gdisp.screen[sel].cmu.layer_hue = 50;

	return DIS_SUCCESS;
}

__s32 BSP_disp_cmu_layer_set_window(__u32 sel, __u32 hid, __disp_rect_t *rect)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        memcpy(&gdisp.screen[sel].cmu.layer_rect, rect, sizeof(__disp_rect_t));
        if(gdisp.screen[sel].cmu.status & CMU_LAYER_EN)
        {
            IEP_CMU_Set_Window(sel,&gdisp.screen[sel].cmu.layer_rect);
        }
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_cmu_layer_get_window(__u32 sel, __u32 hid, __disp_rect_t *rect)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        memcpy(rect, &gdisp.screen[sel].cmu.layer_rect, sizeof(__disp_rect_t));
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}



__s32 BSP_disp_cmu_layer_set_bright(__u32 sel, __u32 hid, __u32 bright)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {

        gdisp.screen[sel].cmu.layer_bright = bright;
        if(gdisp.screen[sel].cmu.status & CMU_LAYER_EN)
        {
            IEP_CMU_Set_Par(sel,gdisp.screen[sel].cmu.layer_hue,gdisp.screen[sel].cmu.layer_saturation,
                gdisp.screen[sel].cmu.layer_bright,gdisp.screen[sel].cmu.layer_mode);
        }
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_cmu_layer_get_bright(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        return gdisp.screen[sel].cmu.layer_bright;
    }
    return DIS_NOT_SUPPORT;
}


__s32 BSP_disp_cmu_layer_set_saturation(__u32 sel, __u32 hid, __u32 saturation)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {

        gdisp.screen[sel].cmu.layer_saturation= saturation;
        if(gdisp.screen[sel].cmu.status & CMU_LAYER_EN)
        {
            IEP_CMU_Set_Par(sel,gdisp.screen[sel].cmu.layer_hue,gdisp.screen[sel].cmu.layer_saturation,
                gdisp.screen[sel].cmu.layer_bright,gdisp.screen[sel].cmu.layer_mode);
        }
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_cmu_layer_get_saturation(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        return gdisp.screen[sel].cmu.layer_saturation;
    }
    return DIS_NOT_SUPPORT;
}


__s32 BSP_disp_cmu_layer_set_hue(__u32 sel, __u32 hid, __u32 hue)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {

        gdisp.screen[sel].cmu.layer_hue = hue;
        if(gdisp.screen[sel].cmu.status & CMU_LAYER_EN)
        {
            IEP_CMU_Set_Par(sel,gdisp.screen[sel].cmu.layer_hue,gdisp.screen[sel].cmu.layer_saturation,
                gdisp.screen[sel].cmu.layer_bright,gdisp.screen[sel].cmu.layer_mode);
        }
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_cmu_layer_get_hue(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        return gdisp.screen[sel].cmu.layer_hue;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_cmu_layer_set_contrast(__u32 sel, __u32 hid, __u32 contrast)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {

        gdisp.screen[sel].cmu.layer_contrast = contrast;
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_cmu_layer_get_contrast(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        return gdisp.screen[sel].cmu.layer_contrast;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_cmu_layer_set_mode(__u32 sel, __u32 hid, __u32 mode)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {

        gdisp.screen[sel].cmu.layer_mode = mode;
        if(gdisp.screen[sel].cmu.status & CMU_LAYER_EN)
        {
            IEP_CMU_Set_Par(sel,gdisp.screen[sel].cmu.layer_hue,gdisp.screen[sel].cmu.layer_saturation,
                gdisp.screen[sel].cmu.layer_bright,gdisp.screen[sel].cmu.layer_mode);
        }
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_cmu_layer_get_mode(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        return gdisp.screen[sel].cmu.layer_mode;
    }
    return DIS_NOT_SUPPORT;
}


__s32 BSP_disp_cmu_enable(__u32 sel,__bool en)
{
    if(en)
    {
        if(!(gdisp.screen[sel].cmu.status & CMU_LAYER_EN))
        {
            IEP_CMU_Set_Imgsize(sel,BSP_disp_get_screen_width(sel), BSP_disp_get_screen_height(sel));
            IEP_CMU_Set_Par(sel,gdisp.screen[sel].cmu.screen_hue,gdisp.screen[sel].cmu.screen_saturation,gdisp.screen[sel].cmu.screen_bright,gdisp.screen[sel].cmu.screen_mode);
            IEP_CMU_Set_Data_Flow(sel,0);
            IEP_CMU_Set_Window(sel,&gdisp.screen[sel].cmu.screen_rect);
            IEP_CMU_Enable(sel, TRUE);
            gdisp.screen[sel].cmu.status |= CMU_SCREEN_EN;
		return DIS_SUCCESS;
        }
    }else
    {
        IEP_CMU_Enable(sel, FALSE);
        gdisp.screen[sel].cmu.status &= CMU_SCREEN_EN_MASK;
    }
	return DIS_NOT_SUPPORT;

}

__s32 BSP_disp_cmu_get_enable(__u32 sel)
{
    return gdisp.screen[sel].cmu.status & CMU_SCREEN_EN;
}

__s32 BSP_disp_cmu_set_window(__u32 sel, __disp_rect_t *rect)
{
    memcpy(&gdisp.screen[sel].cmu.screen_rect, rect, sizeof(__disp_rect_t));
    if(gdisp.screen[sel].cmu.status & CMU_SCREEN_EN)
    {
        IEP_CMU_Set_Window(sel,&gdisp.screen[sel].cmu.screen_rect);
    }
    return DIS_SUCCESS;
}

__s32 BSP_disp_cmu_get_window(__u32 sel, __disp_rect_t *rect)
{
    memcpy(rect, &gdisp.screen[sel].cmu.screen_rect, sizeof(__disp_rect_t));
    return DIS_SUCCESS;
}


__s32 BSP_disp_cmu_set_bright(__u32 sel, __u32 bright)
{
    gdisp.screen[sel].cmu.screen_bright = bright;
    if(gdisp.screen[sel].cmu.status & CMU_SCREEN_EN)
    {
        IEP_CMU_Set_Par(sel,gdisp.screen[sel].cmu.screen_hue,gdisp.screen[sel].cmu.screen_saturation,
            gdisp.screen[sel].cmu.screen_bright,gdisp.screen[sel].cmu.screen_mode);
    }
    return DIS_SUCCESS;
}

__s32 BSP_disp_cmu_get_bright(__u32 sel)
{
    return gdisp.screen[sel].cmu.screen_bright;
}


__s32 BSP_disp_cmu_set_saturation(__u32 sel,__u32 saturation)
{
    gdisp.screen[sel].cmu.screen_saturation= saturation;
    if(gdisp.screen[sel].cmu.status & CMU_SCREEN_EN)
    {
        IEP_CMU_Set_Par(sel,gdisp.screen[sel].cmu.screen_hue,gdisp.screen[sel].cmu.screen_saturation,
            gdisp.screen[sel].cmu.screen_bright,gdisp.screen[sel].cmu.screen_mode);
    }
    return DIS_SUCCESS;

}

__s32 BSP_disp_cmu_get_saturation(__u32 sel)
{
    return gdisp.screen[sel].cmu.screen_saturation;
}

__s32 BSP_disp_cmu_set_hue(__u32 sel, __u32 hue)
{
    gdisp.screen[sel].cmu.screen_hue = hue;
    if(gdisp.screen[sel].cmu.status & CMU_SCREEN_EN)
    {
        IEP_CMU_Set_Par(sel,gdisp.screen[sel].cmu.screen_hue,gdisp.screen[sel].cmu.screen_saturation,
            gdisp.screen[sel].cmu.screen_bright,gdisp.screen[sel].cmu.screen_mode);
    }
    return DIS_SUCCESS;

}

__s32 BSP_disp_cmu_get_hue(__u32 sel)
{
    return gdisp.screen[sel].cmu.screen_hue;
}



__s32 BSP_disp_cmu_set_contrast(__u32 sel,__u32 contrast)
{
    return DIS_SUCCESS;
}

__s32 BSP_disp_cmu_get_contrast(__u32 sel)
{
    return gdisp.screen[sel].cmu.screen_contrast;
}

__s32 BSP_disp_cmu_set_mode(__u32 sel,__u32 mode)
{
    gdisp.screen[sel].cmu.screen_mode= mode;
    if(gdisp.screen[sel].cmu.status & CMU_SCREEN_EN)
    {
        IEP_CMU_Set_Par(sel,gdisp.screen[sel].cmu.screen_hue,gdisp.screen[sel].cmu.screen_saturation,
            gdisp.screen[sel].cmu.screen_bright,gdisp.screen[sel].cmu.screen_mode);
    }
    return DIS_SUCCESS;

}

__s32 BSP_disp_cmu_get_mode(__u32 sel)
{
    return gdisp.screen[sel].cmu.screen_mode;
}
