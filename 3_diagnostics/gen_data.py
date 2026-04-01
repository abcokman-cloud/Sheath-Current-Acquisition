import numpy as np
import pandas as pd

def generate_mock_sheath_data(filename="test_data.csv", duration_sec=60, freq=50, points_per_cycle=256):
    """
    生成模拟的护套环流数据
    - duration_sec: 持续时间（秒）
    - freq: 基波频率 (50Hz)
    - points_per_cycle: 每个周期的采样点数 (256)
    """
    # 计算总采样率 fs
    fs = freq * points_per_cycle  # 12800 Hz
    t = np.linspace(0, duration_sec, int(fs * duration_sec), endpoint=False)

    # 构造信号组件
    base_wave = 10.0 * np.sin(2 * np.pi * freq * t)          # 10A 基波
    h3_wave = 0.8 * np.sin(2 * np.pi * (3 * freq) * t + 0.5) # 3次谐波
    h5_wave = 0.4 * np.sin(2 * np.pi * (5 * freq) * t + 1.2) # 5次谐波
    dc_offset = 0.5                                          # 0.5A 直流偏移
    noise = np.random.normal(0, 0.1, len(t))                 # 随机噪声

    # 合成总信号
    signal = base_wave + h3_wave + h5_wave + dc_offset + noise

    # 保存为 CSV
    df = pd.DataFrame({"current_a": signal})
    df.to_csv(filename, index=False)
    print(f"成功生成模拟数据: {filename}")
    print(f"采样率: {fs} Hz, 总点数: {len(signal)}")

if __name__ == "__main__":
    generate_mock_sheath_data()