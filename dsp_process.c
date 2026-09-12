#include "dsp_process.h"

// ==================== DSP相关全局变量（从hal_entry.c移过来） ====================
static int16_t g_dsp_processed_buf[256] = {0};
static float32_t g_dsp_float_buf[256] = {0};
static arm_biquad_casd_df1_inst_f32 g_preemph_filter;
static float32_t g_preemph_state[2] = {0};
static const float32_t g_preemph_coeffs[5] = {0.97f, 0.0f, 0.0f, 0.0f, 0.0f};

// ==================== 初始化函数 ====================
void dsp_process_init(void)
{
    arm_biquad_cascade_df1_init_f32(&g_preemph_filter, 1, g_preemph_coeffs, g_preemph_state);
}

// ==================== 处理函数（完全保留你原有的逻辑） ====================
int16_t* dsp_process_audio(uint16_t *raw_adc_buf)
{
    float32_t mean_val, max_val, gain;
    
    // 1. 转换为16位有符号PCM
    for(uint16_t i = 0; i < 256; i++)
    {
        int32_t pcm = (int32_t)raw_adc_buf[i] - 2048;
        g_dsp_processed_buf[i] = (int16_t)(pcm * 16);
    }
    
    // 2. 转换为浮点
    arm_q15_to_float(g_dsp_processed_buf, g_dsp_float_buf, 256);
    
    // 3. 去除直流偏置
    arm_mean_f32(g_dsp_float_buf, 256, &mean_val);
    for(uint16_t i = 0; i < 256; i++)
    {
        g_dsp_float_buf[i] -= mean_val;
    }
    
    // 4. 预加重滤波
    arm_biquad_cascade_df1_f32(&g_preemph_filter, g_dsp_float_buf, g_dsp_float_buf, 256);
    
    // 5. 自动增益控制(AGC)
    max_val = 0.0f;
    for(uint16_t i = 0; i < 256; i++)
    {
        float32_t abs_val = (g_dsp_float_buf[i] >= 0.0f) ? g_dsp_float_buf[i] : -g_dsp_float_buf[i];
        if(abs_val > max_val)
        {
            max_val = abs_val;
        }
    }
    if(max_val > 1.0f)
    {
        gain = 0.7f * 32767.0f / max_val;
    }
    else
    {
        gain = 1.0f;
    }
    arm_scale_f32(g_dsp_float_buf, gain, g_dsp_float_buf, 256);
    
    // 6. 转回16位整数
    arm_float_to_q15(g_dsp_float_buf, g_dsp_processed_buf, 256);
    
    return g_dsp_processed_buf;
}