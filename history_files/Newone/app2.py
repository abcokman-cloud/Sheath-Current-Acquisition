from flask import Flask, jsonify, render_template, request
import socket
import struct
import numpy as np
from scipy.fft import fft

try:
    import serial
except ImportError:
    serial = None

app = Flask(__name__)

# ================= 固定配置 =================
STM32_PORT = 502            # 默认 Modbus 端口
CHANNELS_TO_SHOW = 4        # 前端展示 4 个通道
POINTS_PER_CH = 256
WAVEFORM_OFFSET = 275       # 报文偏移量

def analyze_channel(data_array):
    """对单通道的 256 个点进行分析"""
    dc_offset = np.mean(data_array)
    ac_data = data_array - dc_offset
    rms_val = np.sqrt(np.mean(np.square(data_array)))
    
    yf = fft(data_array)
    amps = 2.0 / POINTS_PER_CH * np.abs(yf[0:POINTS_PER_CH//2])
    fund_amp = amps[1] if amps[1] > 0 else 1e-6
    harmonic_sq_sum = np.sum(np.square(amps[2:]))
    thd = (np.sqrt(harmonic_sq_sum) / fund_amp) * 100
    
    return {
        "rms": round(rms_val, 2),
        "dc": round(dc_offset, 2),
        "thd": round(thd, 2)
    }

@app.route('/')
def index():
    # 页面打开时不做任何硬件连接，纯展示前端页面
    return render_template('index.html')

@app.route('/api/get_data')
def get_data():
    # 后台根据前端发来的设备 IP 动态去连接，实现多设备解耦
    target_ip = request.args.get('ip', '')
    
    if not target_ip:
        return jsonify({"status": "error", "message": "未提供设备 IP 地址"})

    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.settimeout(3.0)
    try:
        client.connect((target_ip, STM32_PORT))
        client.send(b'\xAA\x00\x00\x00\x00\x00\x00\x00')
        
        raw_data = b""
        EXPECTED_SIZE = 4371 
        while len(raw_data) < EXPECTED_SIZE:
            packet = client.recv(EXPECTED_SIZE - len(raw_data))
            if not packet:
                break
            raw_data += packet
            
        if len(raw_data) >= EXPECTED_SIZE and raw_data[0] == 0xAA:
            waveform_bytes_len = 8 * POINTS_PER_CH * 2
            waveform_data_raw = raw_data[WAVEFORM_OFFSET : WAVEFORM_OFFSET + waveform_bytes_len]
            
            unpack_fmt = f"<{8 * POINTS_PER_CH}H"
            unpacked_data = struct.unpack(unpack_fmt, waveform_data_raw)
            channels_data = np.array(unpacked_data).reshape((8, POINTS_PER_CH))
            
            response = {"status": "success", "waveforms": [], "metrics": []}
            
            for i in range(CHANNELS_TO_SHOW):
                ch_data = channels_data[i].tolist()
                metrics = analyze_channel(channels_data[i])
                response["waveforms"].append(ch_data)
                response["metrics"].append(metrics)
                
            return jsonify(response)
        else:
            return jsonify({"status": "error", "message": f"报文不完整，接收到 {len(raw_data)} 字节"})
            
    except Exception as e:
        return jsonify({"status": "error", "message": f"通信异常: {str(e)}"})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)