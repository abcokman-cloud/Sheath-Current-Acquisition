#ifndef __DSP_ALGO_H
#define __DSP_ALGO_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

// 5. 状态码定义
#define STATUS_OK             0x00
#define STATUS_HARM_ALARM     0x01 
#define STATUS_FREQ_ALARM     0x02 

/* --- 瞬时计算结果结构体 --- */
typedef struct {
    float rms;                // [数据内容]: 单个周期瞬时有效值 (不具备长周期平滑特性)
    float phase_align_shift;  // [数据内容]: 当前采样块相对于基准 90° 相位的偏移点数
    uint8_t status;           // [数据内容]: 状态反馈标志
} MonitorResult;

/* --- 全局统计结果结构体 --- */
typedef struct {
	float total_rms;                     // [业务核心]: 1秒无噪总有效值。采用“二次计算法”得出，自带 EMA 阻尼平滑，用于 Modbus 上报。
	float *point_rms;                    // [解耦核心]: 改为指针，由外部主工程预先分配好内存后传入，不再死绑256点
	uint8_t harmonic_status;             // [诊断字]: 峰谷错位点数差。用于评估波形存在的偶次谐波畸变程度。
	float phase_variance;                // [诊断字]: 相位抖动方差。评估电网频率或单片机晶振的稳定性 (频偏越严重，方差越大)。
	float sym_distort_variance;          // [诊断字]: 谐波畸变方差。评估波形对称性的波动剧烈度 (是否有间歇性强干扰)。
	float crest_factor;                  // [高阶诊断]: 波峰因数 (CF)。评估波形尖峰冲击度。
	float true_dc_value;                 // [高阶诊断]: 真实直流分量 (DC Bias)。评估漏电与电化学腐蚀风险。
	float grid_frequency;                // [高阶诊断]: 真实电网频率。评估电网与发电机转速稳定性。
	
	float peak_val;                      // [时域新增]: 峰值偏移量 (绝对幅值)
	float peak_phase;                    // [时域新增]: 峰值相位 (0~360度)
	float valley_val;                    // [时域新增]: 谷值偏移量 (绝对幅值)
	float valley_phase;                  // [时域新增]: 谷值相位 (0~360度)
	float zero_crossing_phase;           // [时域新增]: 正向过零点相位 (0~360度)

	float fundamental_rms;               // [频域新增]: 1次谐波(基波)有效值
	float thd;                           // [高阶频域]: 总谐波畸变率 (THD)
	float ihd_2;                         // [高阶频域]: 2次谐波畸变率
	float ihd_3;                         // [高阶频域]: 3次谐波畸变率
	float ihd_5;                         // [高阶频域]: 5次谐波畸变率
	float ihd_7;                         // [高阶频域]: 7次谐波畸变率
} WaveformResult;

/* 
 * [辅助宏]: 供外部工程计算需要为 DSP 引擎分配多大的工作内存 
 * 注：(ch_num * 48) 是内部 DiagnosticStats 结构体的预估极限对齐占用
 */
#define DSP_CALC_WORK_BUF_SIZE(ch_num, points) \
    ( ((ch_num) * (points) * sizeof(uint32_t)) + \
      ((ch_num) * sizeof(uint32_t)) + \
      ((ch_num) * 48) + \
      ((ch_num) * sizeof(float)) + \
      ((ch_num) * sizeof(uint8_t)) )

/* --- 核心引擎生命周期管理 --- */

// [核心机制]: 将运行期参数、内存池、通道回调函数一并注入给库
uint8_t DSP_Algo_Init(uint8_t ch_num, uint16_t points_per_period, uint16_t settle_periods, 
                      uint32_t sample_freq_hz, uint8_t *work_buf, uint32_t buf_size, 
                      uint8_t (*ch_enable_cb)(uint8_t));

MonitorResult Calc_CirculatingCurrent(uint16_t *data, uint16_t len, float k);

/* --- 核心数据流压入接口 --- */
uint8_t Calc_Accumulate_WithAlign(uint8_t ch, uint16_t *data, int16_t *offset_out);

/* --- 核心数据结算接口 --- */
// [核心机制]: 由1秒定时器或满50次时触发。执行“二次计算”逻辑 (Sum_Square -> Point_RMS -> Total_RMS)。

void Calc_GetFullWaveform(uint8_t ch, float k, int16_t last_shift, uint16_t actual_cycles, WaveformResult* out_res);
/* --- 三相矢量和专属引擎 --- */
// 1. 实时流式交叉累加 (每20ms调用)
void Calc_Accumulate_VectorSum(uint16_t *interleaved_data);

// 2. 一秒钟大结算 (提取 3 相叠加后的真实有效值)
float Calc_GetVectorSumRMS(uint8_t group, float k, uint16_t actual_cycles);

#endif
