/**
  ******************************************************************************
  * @file    ct_ratio.c
  * @brief   Ratio and offset logic, data distribution.
  * @note    Converts raw data to actual physical values and updates Modbus registers.
  ******************************************************************************
  */

#include "ct_ratio.h"
#include "modbus_slave.h" 
#include "sys_config.h"

/* --- Macro: 16-bit clamp to prevent overflow/underflow --- */
#define CLAMP_U16(val)  ((val) > 65535.0f ? 65535 : ((val) < 0.0f ? 0 : (uint16_t)(val)))

/* Global physical waveform buffer */
float g_eth_waveform_buffer[CH_NUM][PHASE_POINTS_COUNT] = {0};

/**
 * @brief Final calculation and Modbus distribution.
 * @param ch Current channel (0-7)
 * @param w_res DSP waveform result
 * @param last_shift Phase shift for grid frequency calc
 */
void Final_Deep_Processing(uint8_t ch, WaveformResult* w_res, int16_t last_shift) {
    if(ch >= CH_NUM) return;

    // 1. Get CT ratio
    float ratio = SYS_Get_CT_Ratio(ch);
    
    // 2. Get CT offset
    float offset = SYS_Get_CT_Offset(ch);
    
    // 3. Calculate real RMS value (y = kx + b)
    float real_rms = (w_res->total_rms * ratio) + offset;
    if (real_rms < 0.0f) real_rms = 0.0f; // Prevent negative values
    
    // 4. Scale and store real current in Modbus registers
    uint32_t val_u32 = (uint32_t)(real_rms * 100.0f);
    uint16_t base_idx = MB_REG_REAL_CUR_START + (ch * 2);
    g_modbus_regs[base_idx]     = (uint16_t)(val_u32 & 0xFFFF);
    g_modbus_regs[base_idx + 1] = (uint16_t)(val_u32 >> 16);
    
    // 5. Store time-domain diagnostic features
    g_modbus_regs[MB_REG_PHASE_SHIFT_START + ch] = (uint16_t)last_shift;
    g_modbus_regs[MB_REG_PEAK_VALLEY_DIST + ch]  = (uint16_t)w_res->harmonic_status;
    
    // Store variance features
    g_modbus_regs[MB_REG_PHASE_VAR_START + ch]   = CLAMP_U16(w_res->phase_variance * 10.0f);
    g_modbus_regs[MB_REG_SYM_VAR_START + ch]     = CLAMP_U16(w_res->sym_distort_variance * 10.0f);
    
    // 6. Store high-level features
    g_modbus_regs[MB_REG_CREST_FACTOR_START + ch] = CLAMP_U16(w_res->crest_factor * 100.0f);
    g_modbus_regs[MB_REG_GRID_FREQ_START + ch]    = CLAMP_U16(w_res->grid_frequency * 100.0f);
    
    // 8. Store true DC value using IEEE 754 float format
    SYS_Push_IEEE_Float_To_Modbus(MB_REG_TRUE_DC_START, ch, w_res->true_dc_value * ratio);
    
    // 9. Store harmonic data (THD, IHD)
    g_modbus_regs[MB_REG_THD_START + ch]  = CLAMP_U16(w_res->thd * 100.0f);
    g_modbus_regs[MB_REG_IHD2_START + ch] = CLAMP_U16(w_res->ihd_2 * 100.0f);
    g_modbus_regs[MB_REG_IHD3_START + ch] = CLAMP_U16(w_res->ihd_3 * 100.0f);
    g_modbus_regs[MB_REG_IHD5_START + ch] = CLAMP_U16(w_res->ihd_5 * 100.0f);
    g_modbus_regs[MB_REG_IHD7_START + ch] = CLAMP_U16(w_res->ihd_7 * 100.0f);

    // 10. Store absolute time-domain and fundamental parameters
    // Scale amplitude to real current
    g_modbus_regs[MB_REG_PEAK_VAL_START + ch]     = CLAMP_U16(w_res->peak_val * ratio * 10.0f);
    g_modbus_regs[MB_REG_VALLEY_VAL_START + ch]   = CLAMP_U16(w_res->valley_val * ratio * 10.0f);
    // Store phase scaled by 10
    g_modbus_regs[MB_REG_PEAK_PHASE_START + ch]   = CLAMP_U16(w_res->peak_phase * 10.0f);
    g_modbus_regs[MB_REG_VALLEY_PHASE_START + ch] = CLAMP_U16(w_res->valley_phase * 10.0f);
    g_modbus_regs[MB_REG_ZERO_CROSS_START + ch]   = CLAMP_U16(w_res->zero_crossing_phase * 10.0f);
    // Store fundamental RMS as IEEE float
    SYS_Push_IEEE_Float_To_Modbus(MB_REG_FUNDAMENTAL_START, ch, w_res->fundamental_rms * ratio);

    // 11. Update global waveform array for W5500 transmission
    for (uint16_t i = 0; i < PHASE_POINTS_COUNT; i++) {
        float point_real_cur = w_res->point_rms[i] * ratio;
        g_waveform_data[ch][i] = (uint16_t)(point_real_cur * 10.0f);
    }
}
