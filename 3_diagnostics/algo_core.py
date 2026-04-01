import numpy as np
from scipy.signal import butter, filtfilt

def butter_lowpass_filter(data, cutoff, fs, order=5):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='low', analog=False)
    # 增加 padlen 减少小样本数据的边缘效应
    y = filtfilt(b, a, data, padlen=min(len(data)-1, 150))
    return y

def process_sheath_data(raw_data, fs=50000, freq=50):
    """
    针对 Dash 优化的核心算法
    """
    # 强制转换为 numpy 数组，确保后续计算不出错
    raw_data = np.asarray(raw_data)
    
    points_per_cycle = int(fs / freq)
    total_cycles = len(raw_data) // points_per_cycle
    
    # 修改：返回格式统一，方便 Dash 错误处理
    if total_cycles < 2:
        raise ValueError("数据长度过短：至少需要 2 个完整周波的数据。")

    # 1. 预处理
    clean_data = raw_data[:total_cycles * points_per_cycle]
    
    # 2. 滤波与预热 (保留原有逻辑，舍弃首个周期)
    filtered_data = butter_lowpass_filter(clean_data, cutoff=1000, fs=fs)
    valid_data = filtered_data[points_per_cycle:] 
    valid_cycles = total_cycles - 1

    # 3. 同步平均
    reshaped_data = valid_data.reshape(valid_cycles, points_per_cycle)
    golden_cycle = np.mean(reshaped_data, axis=0)

    # 4. 特征提取
    dc_offset = np.mean(golden_cycle)
    zero_aligned_cycle = golden_cycle - dc_offset
    
    # 有效值计算
    rms = np.sqrt(np.mean(zero_aligned_cycle**2))
    
    # 5. 谐波分析 (向量化优化)
    t = np.arange(points_per_cycle) / fs
    harmonics = {}
    test_harmonics = [1, 2, 3, 5, 7]
    
    for h in test_harmonics:
        # 使用基础傅里叶变换公式提取指定频率分量
        # 频率 f = h * freq
        omega = 2 * np.pi * h * freq
        c = np.sum(zero_aligned_cycle * np.exp(-1j * omega * t)) / points_per_cycle
        
        harmonics[f"{h}次幅值"] = np.abs(c) * 2
        harmonics[f"{h}次相位"] = np.angle(c, deg=True)

    return {
        "golden_cycle": zero_aligned_cycle,
        "rms": round(rms, 4),
        "dc_offset": round(dc_offset, 6),
        "harmonics": harmonics,
        "status": "success"
    }