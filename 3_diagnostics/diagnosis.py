import dash
from dash import dcc, html, Input, Output, State, dash_table
import dash_bootstrap_components as dbc  # 引入高级样式库
import plotly.graph_objs as go
import pandas as pd
import numpy as np
from scipy.signal import butter, filtfilt
import io
import base64

# --- 核心算法 (保持不变) ---
def butter_lowpass_filter(data, cutoff, fs, order=5):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='low', analog=False)
    y = filtfilt(b, a, data, padlen=min(len(data)-1, 150))
    return y

def process_sheath_data(raw_data, fs=50000, freq=50):
    raw_data = np.asarray(raw_data)
    points_per_cycle = int(fs / freq)
    total_cycles = len(raw_data) // points_per_cycle
    if total_cycles < 2: raise ValueError("数据过短")
    clean_data = raw_data[:total_cycles * points_per_cycle]
    filtered_data = butter_lowpass_filter(clean_data, cutoff=1000, fs=fs)
    valid_data = filtered_data[points_per_cycle:] 
    reshaped_data = valid_data.reshape(total_cycles - 1, points_per_cycle)
    golden_cycle = np.mean(reshaped_data, axis=0)
    dc_offset = np.mean(golden_cycle)
    zero_aligned = golden_cycle - dc_offset
    rms = np.sqrt(np.mean(zero_aligned**2))
    
    t = np.arange(points_per_cycle) / fs
    harmonics = []
    for h in [1, 2, 3, 5, 7]:
        c = np.sum(zero_aligned * np.exp(-1j * 2 * np.pi * h * freq * t)) / points_per_cycle
        harmonics.append({"特征": f"{h}次谐波", "幅值(A)": round(np.abs(c)*2, 4), "相位(°)": round(np.angle(c, deg=True), 2)})
    return {"golden_cycle": zero_aligned, "rms": round(rms, 4), "dc": round(dc_offset, 4), "harmonics": harmonics}

# --- Dash 应用 (使用 Bootstrap 主题) ---
app = dash.Dash(__name__, external_stylesheets=[dbc.themes.FLATLY])

app.layout = dbc.Container([
    # 顶部标题栏
    dbc.Row([
        dbc.Col(html.H2("⚡ 护套环流深处理计算与AI诊断平台", className="text-center my-4 text-primary"))
    ]),

    dbc.Row([
        # 左侧控制面板
        dbc.Col([
            dbc.Card([
                dbc.CardHeader("输入参数配置", className="bg-primary text-white"),
                dbc.CardBody([
                    html.Label("采样频率 (Hz):", className="fw-bold"),
                    dbc.Input(id='input-fs', type='number', value=12800, className="mb-3"),
                    dcc.Upload(
                        id='upload-data',
                        children=html.Div(['拖拽或 ', html.A('选择CSV文件')], className="p-3"),
                        className="text-center border border-dashed rounded",
                        style={'cursor': 'pointer'}
                    ),
                    html.Hr(),
                    html.Div(id='metrics-output')
                ])
            ], className="shadow")
        ], width=4),

        # 右侧图表区
        dbc.Col([
            dbc.Card([
                dbc.CardHeader("黄金周波合成", className="bg-info text-white"),
                dbc.CardBody([
                    dcc.Graph(id='main-plot', config={'displayModeBar': False})
                ])
            ], className="shadow mb-4"),
            
            dbc.Card([
                dbc.CardHeader("各次谐波分析详情", className="bg-dark text-white"),
                dbc.CardBody([
                    dash_table.DataTable(
                        id='h-table',
                        style_header={'backgroundColor': '#f8f9fa', 'fontWeight': 'bold'},
                        style_cell={'textAlign': 'center'}
                    )
                ])
            ], className="shadow")
        ], width=8)
    ]),
    
    dbc.Alert(id='error-msg', color="danger", is_open=False, className="mt-3")
], fluid=True)

@app.callback(
    [Output('main-plot', 'figure'), Output('metrics-output', 'children'), 
     Output('h-table', 'data'), Output('h-table', 'columns'),
     Output('error-msg', 'children'), Output('error-msg', 'is_open')],
    [Input('upload-data', 'contents')],
    [State('upload-data', 'filename'), State('input-fs', 'value')]
)
def update(contents, filename, fs):
    if not contents: return go.Figure(), "", [], [], "", False
    try:
        content_string = contents.split(',')[1]
        decoded = base64.b64decode(content_string)
        df = pd.read_csv(io.StringIO(decoded.decode('utf-8')))
        res = process_sheath_data(df.iloc[:, 0].values, fs=fs)

        fig = go.Figure(data=[go.Scatter(y=res['golden_cycle'], line=dict(color='#e74c3c', width=3))])
        fig.update_layout(margin=dict(l=20, r=20, t=20, b=20), height=300)

        metrics = [
            html.Div([html.Span("有效值 (RMS): ", className="fw-bold"), html.Span(f"{res['rms']} A")], className="mb-2"),
            html.Div([html.Span("直流偏移: ", className="fw-bold"), html.Span(f"{res['dc']} A")], className="mb-2"),
        ]
        
        cols = [{"name": i, "id": i} for i in ["特征", "幅值(A)", "相位(°)"]]
        return fig, metrics, res['harmonics'], cols, "", False
    except Exception as e:
        return go.Figure(), "", [], [], f"错误: {str(e)}", True

if __name__ == '__main__':
    app.run(debug=True, port=8051)