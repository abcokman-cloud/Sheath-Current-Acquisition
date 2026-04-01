import numpy as np
import pandas as pd

def generate_custom_harmonics(filename="diagnostic_data_60s.csv", duration=60, freq=50, ppc=256):
    """
    生成特定谐波占比的测试数据
    - 基波 (1st): 10.0 A (100%)
    - 2次 (2nd): 0.3 A  (3%)
    - 3次 (3rd): 0.9 A  (9%)
    - 5次 (5th): 0.6 A  (6%)
    - 7次 (7th): 0.3 A  (3%)
    """
    fs = freq * ppc  # 12800 Hz
    t = np.linspace(0, duration, int(fs * duration), endpoint=False)

    # 设定基波参考幅值为 10.0
    base_amp = 10.0
    
    # 构造各次分量 (添加了随机相位让波形更真实)
    wave_1 = base_amp * np.sin(2 * np.pi * freq * t)
    wave_2 = (base_amp * 0.03) * np.sin(2 * np.pi * (2 * freq) * t + 0.2)
    wave_3 = (base_amp * 0.09) * np.sin(2 * np.pi * (3 * freq) * t + 0.5)
    wave_4 = (base_amp * 0.00) * np.sin(2 * np.pi * (4 * freq) * t) # 0%
    wave_5 = (base_amp * 0.06) * np.sin(2 * np.pi * (5 * freq) * t + 0.8)
    wave_7 = (base_amp * 0.03) * np.sin(2 * np.pi * (7 * freq) * t + 1.2)
    
    # 直流偏移和噪声
    dc_offset = 0.1
    noise = np.random.normal(0, 0.05, len(t))

    # 合成信号
    signal = wave_1 + wave_2 + wave_3 + wave_4 + wave_5 + wave_7 + dc_offset + noise

    # 保存
    df = pd.DataFrame({"current_a": signal.astype(np.float32)}) # 使用 float32 减小文件体积
    df.to_csv(filename, index=False)
    
    print(f"✅ 成功生成 60 秒诊断数据: {filename}")
    print(f"📈 预期指标: 基波 10A, 3次 0.9A(9%), 5次 0.6A(6%)")
    print(f"📂 总行数: {len(signal)}")

if __name__ == "__main__":
    generate_custom_harmonics()