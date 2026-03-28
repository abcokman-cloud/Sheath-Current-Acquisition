#include "dsp_algo.h"
#include <math.h>
#include <stdlib.h>

/* --- Runtime Configuration --- */
static uint8_t  g_ch_num = 0;
static uint16_t g_points_per_period = 0;
static uint16_t g_settle_periods = 0;
static uint32_t g_sample_freq_hz = 0;
static uint8_t  (*g_ch_enable_cb)(uint8_t) = NULL;

/* --- Workspace Pointers --- */
static uint32_t *g_phase_sum_sq = NULL;       // Mapped as [ch_num][points]
static uint32_t *g_channel_offset_raw = NULL; // Mapped as [ch_num]
static float    *g_rms_history = NULL;        // Mapped as [ch_num]
static uint8_t  *g_is_first_calc = NULL;      // Mapped as [ch_num]

static uint64_t g_vec_sum_sq[2] = {0};
static uint64_t g_vec_sum[2] = {0};

/* --- Diagnostic Structure --- */
typedef struct {
    /* --- Accumulators --- */
    int32_t  PhaseSum;          // Phase offset sum
    uint64_t PhaseSumSq;        // Phase offset square sum
    
    int32_t  SymDistortSum;     // Symmetric distortion sum
    uint64_t SymDistortSumSq;   // Symmetric distortion square sum
    
    int16_t  FirstPhaseOffset;  // First phase offset
    int16_t  LastPhaseOffset;   // Last phase offset
    uint16_t CycleCount;        // Cycle counter

    /* --- Final Output --- */
    float    PhaseAvg;          // Average phase
    float    PhaseVariance;     // Phase variance
    
    float    SymDistortAvg;     // Average symmetric distortion
    float    SymDistortVariance;// Symmetric distortion variance
    float    GridFrequency;     // Real grid frequency
} DiagnosticStats;

static DiagnosticStats *ChDiag = NULL; // Mapped as [ch_num]

/**
 * @brief Initialize memory pool
 */
uint8_t DSP_Algo_Init(uint8_t ch_num, uint16_t points_per_period, uint16_t settle_periods, 
                      uint32_t sample_freq_hz, uint8_t *work_buf, uint32_t buf_size, 
                      uint8_t (*ch_enable_cb)(uint8_t)) {
    if (work_buf == NULL || ch_enable_cb == NULL || ch_num == 0 || points_per_period == 0) return 0;
    
    uint32_t req_size = DSP_CALC_WORK_BUF_SIZE(ch_num, points_per_period);
    if (buf_size < req_size) return 0; // Insufficient memory
    
    g_ch_num = ch_num;
    g_points_per_period = points_per_period;
    g_settle_periods = settle_periods;
    g_sample_freq_hz = sample_freq_hz;
    g_ch_enable_cb = ch_enable_cb;
    
    /* --- Memory Partitioning --- */
    uint32_t offset = 0;
    
    g_phase_sum_sq = (uint32_t*)(work_buf + offset);
    offset += ch_num * points_per_period * sizeof(uint32_t);
    
    g_channel_offset_raw = (uint32_t*)(work_buf + offset);
    offset += ch_num * sizeof(uint32_t);
    
    ChDiag = (DiagnosticStats*)(work_buf + offset);
    offset += ch_num * sizeof(DiagnosticStats);
    
    g_rms_history = (float*)(work_buf + offset);
    offset += ch_num * sizeof(float);
    
    g_is_first_calc = (uint8_t*)(work_buf + offset);
    
    // Init first-calc flags
    for(uint8_t i = 0; i < ch_num; i++) {
        g_is_first_calc[i] = 1;
    }
    return 1;
}

/**
 * [Function]: Basic RMS calc
 */
MonitorResult Calc_CirculatingCurrent(uint16_t *data, uint16_t len, float k) {
    MonitorResult res = {0.0f, 0.0f, 0};
    uint32_t sum_raw = 0;
    float sum_sq = 0.0f;

    for(uint16_t i = 0; i < len; i++) {
        sum_raw += data[i];
    }
    // Phase align shift
    res.phase_align_shift = (float)sum_raw / (float)len;

    for(uint16_t i = 0; i < len; i++) {
        float diff = (float)data[i] - res.phase_align_shift;
        sum_sq += diff * diff;
    }
    res.rms = sqrtf(sum_sq / (float)len) * k;
    
    if(res.rms < 0.1f) res.rms = 0.0f; 

    return res;
}

/**
 * [Function]: Basic accumulation
 */
void Calc_Accumulate_Block(uint8_t ch, uint16_t *data, uint16_t len) {
    if(ch >= g_ch_num || g_ch_enable_cb(ch) == 0) return;

    for(uint16_t i = 0; i < len; i++) {
        uint16_t idx = i % g_points_per_period;
        g_phase_sum_sq[ch * g_points_per_period + idx] += (uint32_t)data[i] * data[i]; 
        g_channel_offset_raw[ch] += data[i]; 
    }
}

/**
 * [Function]: Accumulation with Phase Alignment
 */
uint8_t Calc_Accumulate_WithAlign(uint8_t ch, uint16_t *data, int16_t *shift_out) {
    if(ch >= g_ch_num || g_ch_enable_cb(ch) == 0) return 0;

    uint16_t max_val = 0;
    uint16_t peak_idx = 0;
    uint16_t min_val = 65535;
    uint16_t valley_idx = 0;
    
    for(uint16_t i = 0; i < g_points_per_period; i++) {
        if(data[i] > max_val) {
            max_val = data[i];
            peak_idx = i;
        }
        if(data[i] < min_val) {
            min_val = data[i];
            valley_idx = i;
        }
    }
    // Alignment logic: Target 90 degrees
    int16_t phase_offset = (int16_t)peak_idx - (g_points_per_period / 4);
    *shift_out = phase_offset;
    
    for(uint16_t i = 0; i < g_points_per_period; i++) {
        int16_t real_idx = (i + *shift_out + g_points_per_period) % g_points_per_period;
        uint32_t val = data[real_idx];
        g_phase_sum_sq[ch * g_points_per_period + i] += val * val; 
        g_channel_offset_raw[ch] += val; 
    }

    /* --- Diagnostic stream accumulation --- */
    DiagnosticStats* diag = &ChDiag[ch];
    
    if (diag->CycleCount == 0) {
        diag->FirstPhaseOffset = phase_offset;
    }
    diag->LastPhaseOffset = phase_offset;
    
    int16_t dist = abs((int16_t)peak_idx - (int16_t)valley_idx);
    int16_t sym_err = dist - (g_points_per_period / 2); // Theoretical distance
    
    diag->PhaseSum += phase_offset;
    diag->PhaseSumSq += (phase_offset * phase_offset);
    diag->SymDistortSum += sym_err;
    diag->SymDistortSumSq += (sym_err * sym_err);
    diag->CycleCount++;
    
    /* --- Periodic Settlement --- */
    if (diag->CycleCount >= g_settle_periods) {
        diag->PhaseAvg = (float)diag->PhaseSum / g_settle_periods;
        diag->PhaseVariance = ((float)diag->PhaseSumSq / g_settle_periods) - (diag->PhaseAvg * diag->PhaseAvg);
        if (diag->PhaseVariance < 0.0f) diag->PhaseVariance = 0.0f; // Prevent FP errors
        
        diag->SymDistortAvg = (float)diag->SymDistortSum / g_settle_periods;
        diag->SymDistortVariance = ((float)diag->SymDistortSumSq / g_settle_periods) - (diag->SymDistortAvg * diag->SymDistortAvg);
        if (diag->SymDistortVariance < 0.0f) diag->SymDistortVariance = 0.0f;
        
        // 3. Calc grid frequency
        int16_t delta_offset = diag->LastPhaseOffset - diag->FirstPhaseOffset;
        int32_t total_points_expected = g_settle_periods * g_points_per_period;
        if ((total_points_expected - delta_offset) > 0) {
            diag->GridFrequency = (float)(g_sample_freq_hz * g_settle_periods) / (float)(total_points_expected - delta_offset);
        } else {
            diag->GridFrequency = 50.0f; // Extreme anomaly protection
        }

        diag->PhaseSum = 0; diag->PhaseSumSq = 0;
        diag->SymDistortSum = 0; diag->SymDistortSumSq = 0;
        diag->CycleCount = 0;
        
        return 1; // Trigger settlement
    }
    return 0; // Continue accumulation
}

/* Goertzel feature extraction */
static float Calc_Goertzel_Amplitude(float* data, uint16_t num_points, uint8_t harmonic_order) {
    float omega = (2.0f * 3.1415926535f * harmonic_order) / num_points;
    float cosine = cosf(omega);
    float sine = sinf(omega);
    float coeff = 2.0f * cosine;
    float q1 = 0.0f, q2 = 0.0f, q0 = 0.0f;
    for(uint16_t i = 0; i < num_points; i++) {
        q0 = coeff * q1 - q2 + data[i];
        q2 = q1;
        q1 = q0;
    }
    float real = (q1 - q2 * cosine) / (num_points / 2.0f);
    float imag = (q2 * sine) / (num_points / 2.0f);
    return sqrtf(real * real + imag * imag);
}


/**
 * [Function]: Final settlement
 */
void Calc_GetFullWaveform(uint8_t ch, float k, int16_t last_shift, uint16_t actual_cycles, WaveformResult* out_res) {
    // Initialize structure members
    out_res->total_rms = 0.0f;
    for(uint16_t i=0; i<g_points_per_period; i++) out_res->point_rms[i] = 0.0f;
    out_res->harmonic_status = 0;
    out_res->phase_variance = 0.0f;
    out_res->sym_distort_variance = 0.0f;
    out_res->crest_factor = 1.414f;
    out_res->true_dc_value = 0.0f;
    out_res->grid_frequency = 50.0f;
    
    out_res->peak_val = 0.0f;
    out_res->peak_phase = 0.0f;
    out_res->valley_val = 0.0f;
    out_res->valley_phase = 0.0f;
    out_res->zero_crossing_phase = 0.0f;
    out_res->fundamental_rms = 0.0f;
    
    out_res->thd = 0.0f;
    out_res->ihd_2 = 0.0f;
    out_res->ihd_3 = 0.0f;
    out_res->ihd_5 = 0.0f;
    out_res->ihd_7 = 0.0f;

    if(ch >= g_ch_num || out_res->point_rms == NULL) return; // Security check

    if (g_ch_enable_cb(ch) == 0) {
        for(uint16_t i = 0; i < g_points_per_period; i++) g_phase_sum_sq[ch * g_points_per_period + i] = 0;
        g_channel_offset_raw[ch] = 0;
        return;
    }
    
    float square_sum_of_points = 0.0f;

    // --- Aux variables for peak/valley ---
    float min_p_val = 1000000.0f;
    float max_p_val = -1000000.0f; // Min value for negative comp
    uint16_t valley_idx = 0;       // Prevent overflow
    uint16_t peak_idx = 0;
    float mean_val = 0.0f;         // AC zero-line reference

    // Discard if data < 1 cycle
    // Avoid div by zero
    if (actual_cycles < 1) {
        return;
    }

    for(uint16_t i = 0; i < g_points_per_period; i++) {
        // Adapt variable cycles
        float p_rms_sq = (float)g_phase_sum_sq[ch * g_points_per_period + i] / (float)actual_cycles;
        
        // Waveform generation
        out_res->point_rms[i] = sqrtf(p_rms_sq) * k;
        
        // Detect peak/valley
        if(out_res->point_rms[i] < min_p_val) {
            min_p_val = out_res->point_rms[i];
            valley_idx = i;
        }
        if(out_res->point_rms[i] > max_p_val) {
            max_p_val = out_res->point_rms[i];
            peak_idx = i;
        }
        
        mean_val += out_res->point_rms[i]; // Accumulate for mean

        square_sum_of_points += (out_res->point_rms[i] * out_res->point_rms[i]);
        g_phase_sum_sq[ch * g_points_per_period + i] = 0; 
    }
    
    mean_val /= g_points_per_period; // Calculate true AC zero-line
    
    // Time-domain features extraction
    out_res->peak_val = max_p_val;
    out_res->valley_val = min_p_val;
    // Map to 0-360 degrees
    out_res->peak_phase = ((float)peak_idx / g_points_per_period) * 360.0f;
    out_res->valley_phase = ((float)valley_idx / g_points_per_period) * 360.0f;

    // Find positive zero-crossing
    out_res->zero_crossing_phase = 0.0f;
    for(uint16_t i = 0; i < g_points_per_period; i++) {
        uint16_t next_idx = (i + 1) % g_points_per_period;
        if(out_res->point_rms[i] <= mean_val && out_res->point_rms[next_idx] > mean_val) {
            // Sub-sample linear interpolation
            float y1 = out_res->point_rms[i] - mean_val;
            float y2 = out_res->point_rms[next_idx] - mean_val;
            float exact_idx = (float)i + (0.0f - y1) / (y2 - y1);
            
            out_res->zero_crossing_phase = (exact_idx / g_points_per_period) * 360.0f;
            break; // Break on first zero-crossing
        }
    }

    // Calculate peak-valley distance
    // Distance relative to peak
    int16_t dist = (int16_t)valley_idx - (g_points_per_period / 4);
    if(dist < 0) dist += g_points_per_period; 
    out_res->harmonic_status = (uint8_t)dist; // Ideal value is a quarter period

    // Total RMS
    out_res->total_rms = sqrtf(square_sum_of_points / g_points_per_period);
    
    // Export variance features
    out_res->phase_variance = ChDiag[ch].PhaseVariance;
    out_res->sym_distort_variance = ChDiag[ch].SymDistortVariance;
    out_res->grid_frequency = ChDiag[ch].GridFrequency;

    // True DC
    float mean_raw = (float)g_channel_offset_raw[ch] / (actual_cycles * g_points_per_period);
    out_res->true_dc_value = (mean_raw - 2048.0f) * k; // Assume 12-bit ADC offset at 2048

    // Crest Factor
    if (out_res->total_rms > 0.1f) {
        out_res->crest_factor = max_p_val / out_res->total_rms;
    } else {
        out_res->crest_factor = 1.414f; // Default to sqrt(2) if signal too weak
    }

    if(out_res->total_rms < 0.1f) out_res->total_rms = 0.0f; 

    // Frequency-domain diagnostic
    if (out_res->total_rms > 0.1f) { // Analyze only if signal is strong enough
        float rms_1 = Calc_Goertzel_Amplitude(out_res->point_rms, g_points_per_period, 1);
        float rms_2 = Calc_Goertzel_Amplitude(out_res->point_rms, g_points_per_period, 2);
        float rms_3 = Calc_Goertzel_Amplitude(out_res->point_rms, g_points_per_period, 3);
        float rms_5 = Calc_Goertzel_Amplitude(out_res->point_rms, g_points_per_period, 5);
        float rms_7 = Calc_Goertzel_Amplitude(out_res->point_rms, g_points_per_period, 7);

        if (rms_1 > 0.001f) {
            out_res->fundamental_rms = rms_1; // Fundamental RMS
            out_res->ihd_2 = (rms_2 / rms_1) * 100.0f;
            out_res->ihd_3 = (rms_3 / rms_1) * 100.0f;
            out_res->ihd_5 = (rms_5 / rms_1) * 100.0f;
            out_res->ihd_7 = (rms_7 / rms_1) * 100.0f;
            out_res->thd = (sqrtf(rms_2*rms_2 + rms_3*rms_3 + rms_5*rms_5 + rms_7*rms_7) / rms_1) * 100.0f;
        }
    }

    // Double-layer filtering
    // Uses synchronous averaging + EMA
    if (g_is_first_calc[ch]) {
        g_rms_history[ch] = out_res->total_rms; 
        g_is_first_calc[ch] = 0;
    } else {
        // EMA damping
        g_rms_history[ch] = (out_res->total_rms * 0.2f) + (g_rms_history[ch] * 0.8f);
        out_res->total_rms = g_rms_history[ch]; // Output smoothed result
    }

    g_channel_offset_raw[ch] = 0; 
}

/* Vector Sum Engine */
// Accumulator for 2 groups

void Calc_Accumulate_VectorSum(uint16_t *interleaved_data) {
    for(uint16_t i = 0; i < g_points_per_period; i++) {
        // --- Group 1: CH1, CH2, CH3 (Software Index 0, 1, 2) ---
        uint32_t sum1 = 0; // Group 1
        if(g_ch_enable_cb(0)) sum1 += interleaved_data[i * g_ch_num + 0];
        if(g_ch_enable_cb(1)) sum1 += interleaved_data[i * g_ch_num + 1];
        if(g_ch_enable_cb(2)) sum1 += interleaved_data[i * g_ch_num + 2];
        
        g_vec_sum[0] += sum1;
        g_vec_sum_sq[0] += (uint64_t)sum1 * sum1;

        // --- Group 2: CH5, CH6, CH7 (Software Index 4, 5, 6) ---
        uint32_t sum2 = 0; // Group 2
        if(g_ch_enable_cb(4)) sum2 += interleaved_data[i * g_ch_num + 4];
        if(g_ch_enable_cb(5)) sum2 += interleaved_data[i * g_ch_num + 5];
        if(g_ch_enable_cb(6)) sum2 += interleaved_data[i * g_ch_num + 6];
        
        g_vec_sum[1] += sum2;
        g_vec_sum_sq[1] += (uint64_t)sum2 * sum2;
    }
}

float Calc_GetVectorSumRMS(uint8_t group, float k, uint16_t actual_cycles) {
    if (group > 1 || actual_cycles < 1) return 0.0f;
    
    uint32_t total_points = actual_cycles * g_points_per_period;
    
    // Use double to prevent overflow
    double mean = (double)g_vec_sum[group] / total_points;
    double mean_sq = (double)g_vec_sum_sq[group] / total_points;
    double variance = mean_sq - (mean * mean);
    
    // Clear accumulators
    g_vec_sum[group] = 0;
    g_vec_sum_sq[group] = 0;
    
    if (variance < 0.0) variance = 0.0;
    return (float)sqrt(variance) * k;
}
