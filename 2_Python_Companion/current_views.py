import numpy as np
import plotly.graph_objs as go
from dash import dcc, html

from current_data import (
    CABLES,
    DEFAULT_ALARM_THRESHOLDS,
    MODBUS_DEFAULTS,
    colors,
    get_advanced_mock_data,
    get_channel_status,
    get_channel_toggle_children,
    get_channel_toggle_style,
    nav_style,
    normalize_devices,
    normalize_thresholds,
    parse_channel_enable,
    parse_numeric_text,
    ALARM_PARAMETERS
)


def build_alarm_config_content(thresholds):
    threshold_map = normalize_thresholds(thresholds)
    header_style = {"padding": "10px 12px", "textAlign": "left", "backgroundColor": "#02183e", "borderBottom": f'1px solid {colors["border"]}'}
    cell_style = {"padding": "10px 12px", "borderBottom": f'1px solid {colors["border"]}'}
    input_style = {"width": "100%", "padding": "7px 10px", "borderRadius": "4px", "border": f'1px solid {colors["border"]}', "backgroundColor": "transparent", "color": "white"}
    rows = []
    from current_data import ALARM_PARAMETERS

    for parameter in ALARM_PARAMETERS:
        key = parameter["key"]
        rows.append(
            html.Tr(
                [
                    html.Td(parameter["label"], style=cell_style),
                    html.Td(parameter["unit"], style=cell_style),
                    html.Td(dcc.Input(id={"type": "alarm-lower-input", "index": key}, type="number", value=threshold_map[key]["lower"], style=input_style), style=cell_style),
                    html.Td(dcc.Input(id={"type": "alarm-upper-input", "index": key}, type="number", value=threshold_map[key]["upper"], style=input_style), style=cell_style),
                ]
            )
        )
    return html.Div(
        [
            html.H3("报警阈值设置", style={"marginBottom": "16px"}),
            html.Div(
                html.Table(
                    [
                        html.Thead(html.Tr([html.Th("参数", style=header_style), html.Th("单位", style=header_style), html.Th("下限", style=header_style), html.Th("上限", style=header_style)])),
                        html.Tbody(rows),
                    ],
                    style={"width": "100%", "borderCollapse": "collapse", "backgroundColor": colors["panel_bg"]},
                ),
                style={"backgroundColor": colors["panel_bg"], "borderRadius": "8px", "overflow": "hidden"},
            ),
            html.Div(
                [
                    html.Button("保存报警阈值", id="save-alarm-threshold-btn", style={"marginRight": "10px", "backgroundColor": colors["accent"], "color": "white", "padding": "9px 24px", "border": "none", "borderRadius": "4px", "cursor": "pointer", "fontWeight": "bold"}),
                    html.Div(
                        id="alarm-config-status-message",
                        style={"marginLeft": "12px", "minHeight": "32px", "display": "flex", "alignItems": "center"},
                    ),
                ],
                style={"display": "flex", "alignItems": "center", "marginTop": "14px"},
            ),
        ]
    )


def build_monitor_content(sel_id, monitor_devices, thresholds):
    threshold_map = normalize_thresholds(thresholds)
    
    def check_alarm(key, val):
        if isinstance(val, str):
            val = parse_numeric_text(val)
        lower = threshold_map[key]["lower"]
        upper = threshold_map[key]["upper"]
        return not (lower <= float(val) <= upper)
    
    device_name = next((item["name"] for item in monitor_devices if item["id"] == sel_id), sel_id)
    t, ya, p_a, v_a, rms_a, harm, thd, peak_p, valley_p, zero_c = get_advanced_mock_data(sel_id)
    wave_fig = go.Figure(data=[go.Scatter(x=t, y=ya, line=dict(color=colors["chart_a"], width=3))])
    wave_fig.add_annotation(x=t[int(np.argmax(ya))], y=p_a, text=f"Peak: {p_a:.2f}A", showarrow=True, arrowhead=2, bgcolor="red", font=dict(color="white"))
    wave_fig.update_layout(template="plotly_dark", paper_bgcolor="rgba(0,0,0,0)", plot_bgcolor="rgba(0,0,0,0)", margin=dict(l=40, r=40, t=30, b=40), height=450, yaxis={"title": "电流 (A)"}, xaxis={"title": "时间 (s)"})
    
    h_labels = ["基波", "2次谐波", "3次谐波", "5次谐波", "7次谐波"]
    plot_indices = [0, 1, 2, 4, 6]
    plot_harm = [harm[i] for i in plot_indices]
    
    marker_colors = []
    default_colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b", "#e377c2"]
    
    # 确保这里的 for 循环和内部逻辑有正确的缩进
    for i in plot_indices:
        h_val = harm[i]
        is_alarm = False
        if i == 0: is_alarm = check_alarm("harmonic_1", h_val)
        elif i == 1: is_alarm = check_alarm("harmonic_2", h_val)
        elif i == 2: is_alarm = check_alarm("harmonic_3", h_val)
        elif i == 4: is_alarm = check_alarm("harmonic_5", h_val) # 对应 plot_indices 中的 4
        elif i == 6: is_alarm = check_alarm("harmonic_7", h_val) # 对应 plot_indices 中的 6
        marker_colors.append("red" if is_alarm else default_colors[plot_indices.index(i)])
        
    bar_fig = go.Figure(data=[go.Bar(x=h_labels, y=plot_harm, text=[f"{v:.1f}%" for v in plot_harm], textposition="outside", marker_color=marker_colors)])
    bar_fig.update_layout(template="plotly_dark", paper_bgcolor="rgba(0,0,0,0)", plot_bgcolor="rgba(0,0,0,0)", margin=dict(l=40, r=40, t=30, b=40), yaxis=dict(range=[0, 120], title="Amplitude (%)"), height=450, xaxis=dict(tickangle=0))
    
    waveform_stats = [
        ("峰值电流", f"{p_a:.2f} A", check_alarm("peak_current", p_a)), 
        ("谷值电流", f"{v_a:.2f} A", check_alarm("valley_current", v_a)), 
        ("峰值相位偏移", peak_p, check_alarm("peak_phase", peak_p)), 
        ("谷值相位偏移", valley_p, check_alarm("valley_phase", valley_p)), 
        ("过零点偏移", zero_c, check_alarm("zero_cross", zero_c)),
    ]
    harmonic_stats = [
        ("1次谐波(基波)", f"{harm[0]:.1f}%", check_alarm("harmonic_1", harm[0])),
        ("2次谐波", f"{harm[1]:.1f}%", check_alarm("harmonic_2", harm[1])),
        ("3次谐波", f"{harm[2]:.1f}%", check_alarm("harmonic_3", harm[2])),
        ("5次谐波", f"{harm[4]:.1f}%", check_alarm("harmonic_5", harm[4])),
        ("7次谐波", f"{harm[6]:.1f}%", check_alarm("harmonic_7", harm[6])),
    ]
    rms_alarm = check_alarm("total_rms", rms_a)
    thd_alarm = check_alarm("thd", thd)
    
    return html.Div(style={"display": "flex", "gap": "20px"}, children=[
        # 左侧面板：实时波形
        html.Div(
            style={"flex": "1", "display": "flex", "flexDirection": "column", "backgroundColor": colors["panel_bg"], "padding": "15px", "borderRadius": "8px"}, 
            children=[
                html.Div([html.Span(f"实时波形 - {device_name}", style={"fontWeight": "bold"}), html.Span(f"Total RMS: {rms_a:.2f} A", className="alarm-flash" if rms_alarm else "", style={"color": colors["chart_a"] if not rms_alarm else "white", "fontSize": "18px", "fontWeight": "bold", "padding": "2px 8px"})], style={"display": "flex", "justifyContent": "space-between"}),
                dcc.Graph(figure=wave_fig, style={"flex": "1"}),
                html.Div([html.Div([html.Small(label), html.Div(value, className="alarm-flash" if is_alarm else "", style={"marginTop": "4px", "padding": "2px 6px"})], style={"textAlign": "center", "display": "flex", "flexDirection": "column", "alignItems": "center"}) for label, value, is_alarm in waveform_stats], style={"display": "flex", "justifyContent": "space-around", "marginTop": "auto", "borderTop": f'1px solid {colors["border"]}', "paddingTop": "15px", "gap": "12px", "flexWrap": "wrap"}),
            ]
        ),
        # 右侧面板：谐波分析
        html.Div(
            style={"flex": "1", "display": "flex", "flexDirection": "column", "backgroundColor": colors["panel_bg"], "padding": "15px", "borderRadius": "8px"}, 
            children=[
                html.Div([html.Span("谐波分析", style={"fontWeight": "bold"}), html.Span(f"THD: {thd:.2f}%", className="alarm-flash" if thd_alarm else "", style={"color": "#fadb14" if not thd_alarm else "white", "fontWeight": "bold", "padding": "2px 8px"})], style={"display": "flex", "justifyContent": "space-between"}),
                dcc.Graph(figure=bar_fig, style={"flex": "1"}),
                html.Div([html.Div([html.Small(label), html.Div(value, className="alarm-flash" if is_alarm else "", style={"marginTop": "4px", "padding": "2px 6px"})], style={"textAlign": "center", "display": "flex", "flexDirection": "column", "alignItems": "center"}) for label, value, is_alarm in harmonic_stats], style={"display": "flex", "justifyContent": "space-around", "marginTop": "auto", "borderTop": f'1px solid {colors["border"]}', "paddingTop": "15px", "gap": "12px", "flexWrap": "wrap"}),
            ]
        ),
    ])


def build_alarm_content(sel_id, alarm_events, thresholds):
    if sel_id != "alarm-list":
        return build_alarm_config_content(thresholds)
    table_header = html.Thead(html.Tr([html.Th("发生时间"), html.Th("设备名称"), html.Th("报警项"), html.Th("实时值"), html.Th("阈值范围"), html.Th("说明"), html.Th("操作")], style={"backgroundColor": "#02183e", "textAlign": "left", "padding": "12px"}))
    table_rows = []
    for alarm in alarm_events:
        table_rows.append(html.Tr([
            html.Td(alarm["time"]),
            html.Td(alarm.get("device_name", "-")),
            html.Td(alarm["item"]),
            html.Td(alarm["value"]),
            html.Td(alarm["limit"]),
            html.Td(alarm["info"]),
            html.Td(html.Button("AI分析", id={"type": "row-ai-btn", "index": alarm["id"]}, n_clicks=0, style={"backgroundColor": "#722ed1", "color": "white", "border": "none", "borderRadius": "4px", "cursor": "pointer", "padding": "4px 10px"})),
        ], style={"borderBottom": f'1px solid {colors["border"]}', "padding": "12px"}))
    if not table_rows:
        table_rows.append(html.Tr([html.Td("当前没有超限报警", colSpan=7, style={"padding": "18px 12px", "textAlign": "center", "color": "#8ba3c7"})]))
    return html.Div([html.H3("实时报警列表", style={"marginBottom": "20px"}), html.Table([table_header, html.Tbody(table_rows)], style={"width": "100%", "borderCollapse": "collapse", "color": "white"}), html.Div(id="ai-analysis-container", n_clicks=0, style={"marginTop": "20px", "display": "none"})])


def build_device_list_content(saved_devices):
    if not saved_devices:
        return html.Div([html.H3("设备列表", style={"marginBottom": "16px"}), html.Div("暂无已保存设备，请先在“新增设备”中填写并保存。", style={"padding": "18px", "backgroundColor": colors["panel_bg"], "borderRadius": "8px", "color": "#9bb0d1"})])
    header_style = {"textAlign": "left", "padding": "10px 12px", "backgroundColor": "#02183e", "borderBottom": f'1px solid {colors["border"]}', "whiteSpace": "nowrap"}
    cell_style = {"padding": "10px 12px", "borderBottom": f'1px solid {colors["border"]}', "color": "white", "verticalAlign": "middle", "whiteSpace": "nowrap"}
    action_button_style = {"border": "none", "borderRadius": "4px", "cursor": "pointer", "padding": "6px 10px", "color": "white", "fontSize": "12px"}
    column_defs = [("name", "设备名称"), ("model", "型号"), ("length", "长度"), ("laying_method", "敷设方式"), ("tunnel_name", "隧道名称"), ("phase", "相别"), ("location", "位置"), ("connection_type", "连接方式"), ("modbus_slave_id", "Modbus ID"), ("baudrate", "Baudrate"), ("tcp_port", "TCP Port"), ("ip_address", "IP"), ("subnet_mask", "Mask"), ("gateway", "Gateway"), ("addr_485", "485 Addr"), ("ct_ratio", "CT Ratio"), ("ct_offset", "CT Offset")]
    rows = []
    for device in saved_devices:
        status_text = device.get("online_status", "离线")
        status_style = {"display": "inline-block", "padding": "4px 10px", "borderRadius": "999px", "fontWeight": "bold", "fontSize": "12px", "color": "white", "backgroundColor": "#389e0d" if status_text == "在线" else "#cf1322"}
        row_cells = [html.Td(device.get(field, "-"), style=cell_style) for field, _ in column_defs]
        channel_flags = parse_channel_enable(device.get("channel_enable"))
        is_enabled = any(channel_flags)
        channel_index_display = next((i + 1 for i, flag in enumerate(channel_flags) if flag == 1), 1)
        row_cells.append(html.Td(channel_index_display, style={**cell_style, "textAlign": "center"}))
        channel_status_text = "开启" if is_enabled else "关闭"
        channel_status_color = "#52c41a" if is_enabled else "#f5222d"
        channel_status_badge = html.Span(channel_status_text, style={"backgroundColor": channel_status_color, "color": "white", "padding": "2px 8px", "borderRadius": "4px", "fontSize": "11px", "display": "inline-block"})
        row_cells.append(html.Td(channel_status_badge, style=cell_style))
        row_cells.extend([
            html.Td(html.Span(status_text, style=status_style), style=cell_style),
            html.Td(html.Div([html.Button("管理设备", id={"type": "manage-device-btn", "index": device["id"]}, n_clicks=0, style={**action_button_style, "backgroundColor": colors["accent"]}), html.Button("删除设备", id={"type": "delete-device-btn", "index": device["id"]}, n_clicks=0, style={**action_button_style, "backgroundColor": "#cf1322"})], style={"display": "flex", "gap": "8px"}), style=cell_style),
        ])
        rows.append(html.Tr(row_cells))
    header_cells = [html.Th(label, style=header_style) for _, label in column_defs]
    header_cells.extend([html.Th("通道序号", style=header_style), html.Th("通道状态", style=header_style), html.Th("在线状态", style=header_style), html.Th("操作", style=header_style)])
    return html.Div([html.H3("设备列表", style={"marginBottom": "16px"}), html.Div(html.Table([html.Thead(html.Tr(header_cells)), html.Tbody(rows)], style={"minWidth": "2200px", "borderCollapse": "collapse", "backgroundColor": colors["panel_bg"]}), style={"width": "100%", "maxWidth": "100%", "overflowX": "auto", "overflowY": "hidden", "paddingBottom": "8px", "backgroundColor": colors["panel_bg"], "borderRadius": "8px"})], style={"width": "100%", "maxWidth": "100%", "overflow": "hidden"})


def build_device_content(sel_id, saved_devices, form_data, editing_device_id):
    if sel_id == "device-list":
        return build_device_list_content(saved_devices)
    if sel_id != "device-add":
        return html.H3("设备列表（台账功能建设中）", style={"padding": "20px"})
    form_data = form_data or {}
    is_editing = bool(editing_device_id)
    input_style = {"width": "100%", "padding": "8px 10px", "marginBottom": "10px", "borderRadius": "4px", "border": f'1px solid {colors["border"]}', "backgroundColor": "transparent", "color": "white", "height": "36px", "boxSizing": "border-box"}
    label_style = {"display": "block", "marginBottom": "4px", "color": "#ccc", "fontSize": "13px"}
    card_style = {"backgroundColor": colors["panel_bg"], "padding": "14px 16px", "borderRadius": "8px", "marginBottom": "14px"}
    section_title_style = {"borderBottom": f'1px solid {colors["border"]}', "paddingBottom": "8px", "marginTop": "0", "marginBottom": "12px", "fontSize": "20px"}
    dropdown_style = {"color": "black", "marginBottom": "10px"}
    field_block_style = {"minWidth": "0"}
    two_col_style = {"display": "grid", "gridTemplateColumns": "repeat(2, minmax(220px, 1fr))", "gap": "12px 16px", "alignItems": "start"}
    three_col_style = {"display": "grid", "gridTemplateColumns": "repeat(3, minmax(200px, 1fr))", "gap": "12px 16px", "alignItems": "start"}
    channel_flags = parse_channel_enable(form_data.get("channel_enable", MODBUS_DEFAULTS["channel_enable"]))
    channel_index = next((i + 1 for i, flag in enumerate(channel_flags) if flag == 1), 1)
    channel_options = [{"label": f"通道 {count}", "value": count} for count in range(1, 9)]
    channel_status_str = get_channel_status(form_data.get("channel_enable", MODBUS_DEFAULTS["channel_enable"]))

    return html.Div(
        [
            html.H3("新增设备", style={"marginBottom": "14px"}),
            html.Div("正在编辑设备" if is_editing else "填写设备信息并保存后，会同步到监测列表。", style={"color": "#8ba3c7", "marginBottom": "12px", "fontSize": "13px"}),
            html.Div(
                [
                    html.Div([html.H4("电缆基本信息", style=section_title_style), html.Div([
                        html.Div([html.Label("电缆名称", style=label_style), dcc.Input(id="device-cable-name", placeholder='例如 "1号高压电缆"', value=form_data.get("name"), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("电缆型号", style=label_style), dcc.Input(id="device-cable-model", placeholder='例如 "YJV22-10kV-3x150"', value=form_data.get("model"), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("电缆长度", style=label_style), dcc.Input(id="device-cable-length", placeholder='例如 "500m"', value=form_data.get("length"), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("敷设方式", style=label_style), dcc.Dropdown(id="device-laying-method", options=["隧道敷设", "直埋", "桥架"], placeholder="选择敷设方式", value=form_data.get("laying_method"), style=dropdown_style, persistence=True, persistence_type="session")], style=field_block_style),
                    ], style=two_col_style)], style=card_style),
                    html.Div([html.H4("位置信息", style=section_title_style), html.Div([
                        html.Div([html.Label("隧道名称", style=label_style), dcc.Input(id="device-tunnel-name", placeholder='例如 "1号隧道"', value=form_data.get("tunnel_name"), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("电缆相别", style=label_style), dcc.Dropdown(id="device-phase-select", options=["A相", "B相", "C相"], placeholder="选择相别", value=form_data.get("phase"), style=dropdown_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("设备位置", style=label_style), dcc.Input(id="device-location", placeholder='例如 "配电室A相 1号接头"', value=form_data.get("location"), style=input_style, persistence=True, persistence_type="session")], style={"gridColumn": "1 / -1", "minWidth": "0"}),
                    ], style=two_col_style)], style=card_style),
                    html.Div([html.H4("设备/连接信息", style=section_title_style), html.Div([
                        html.Div([html.Label("连接方式", style=label_style), dcc.Dropdown(id="conn-type-dropdown", options=[{"label": "IP", "value": "IP"}, {"label": "485", "value": "485"}], value=form_data.get("connection_type", "IP"), style=dropdown_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("Modbus Slave ID", style=label_style), dcc.Input(id="device-modbus-id", type="number", value=form_data.get("modbus_slave_id", MODBUS_DEFAULTS["modbus_slave_id"]), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("Baudrate", style=label_style), dcc.Input(id="device-baudrate", type="number", value=form_data.get("baudrate", MODBUS_DEFAULTS["baudrate"]), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("TCP Port", style=label_style), dcc.Input(id="device-tcp-port", type="number", value=form_data.get("tcp_port", MODBUS_DEFAULTS["tcp_port"]), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div(id="ip-input-container", children=[html.Label("IP地址", style=label_style), dcc.Input(id="device-ip-address", placeholder='例如 "192.168.1.100"', value=form_data.get("ip_address", MODBUS_DEFAULTS["ip_address"]), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("子网掩码", style=label_style), dcc.Input(id="device-subnet-mask", placeholder='例如 "255.255.255.0"', value=form_data.get("subnet_mask", MODBUS_DEFAULTS["subnet_mask"]), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("网关", style=label_style), dcc.Input(id="device-gateway", placeholder='例如 "192.168.1.1"', value=form_data.get("gateway", MODBUS_DEFAULTS["gateway"]), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div(id="485-input-container", style=field_block_style, children=[html.Label("485地址号", style=label_style), dcc.Input(id="device-485-address", placeholder='例如 "01"', value=form_data.get("addr_485"), style=input_style, persistence=True, persistence_type="session")]),
                        html.Div([html.Label("CT Ratio", style=label_style), dcc.Input(id="device-ct-ratio", type="number", value=form_data.get("ct_ratio", MODBUS_DEFAULTS["ct_ratio"]), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("CT Offset", style=label_style), dcc.Input(id="device-ct-offset", type="number", value=form_data.get("ct_offset", MODBUS_DEFAULTS["ct_offset"]), style=input_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("通道序号", style=label_style), dcc.Dropdown(id="device-channel-count", options=channel_options, value=channel_index, clearable=False, style=dropdown_style, persistence=True, persistence_type="session")], style=field_block_style),
                        html.Div([html.Label("通道状态", style=label_style), dcc.Store(id="device-channel-enable", data=channel_status_str, storage_type="memory"), html.Button(get_channel_toggle_children(channel_status_str), id="device-channel-enable-toggle", n_clicks=0, style=get_channel_toggle_style(channel_status_str))], style=field_block_style),
                    ], style=three_col_style)], style={"gridColumn": "1 / -1", **card_style}),
                ],
                style={"display": "grid", "gridTemplateColumns": "repeat(2, minmax(320px, 1fr))", "gap": "14px", "maxWidth": "1120px"},
            ),
            html.Button("更新设备信息" if is_editing else "保存设备信息", id="save-device-btn", style={"backgroundColor": colors["accent"], "color": "white", "padding": "9px 24px", "border": "none", "borderRadius": "4px", "cursor": "pointer", "fontSize": "15px", "fontWeight": "bold", "marginTop": "2px"}),
        ],
        style={"maxWidth": "1160px"},
    )

def create_layout():
    return html.Div(
        style={"backgroundColor": colors["background"], "color": colors["text"], "minHeight": "100vh", "display": "flex", "flexDirection": "column"},
        children=[
            html.Div([html.H2("护套环流监测系统", style={"margin": "0", "color": colors["accent"]}), html.Div(id="live-clock", style={"color": "#888"})], style={"display": "flex", "justifyContent": "space-between", "alignItems": "center", "padding": "20px", "borderBottom": f'1px solid {colors["border"]}'}),
            html.Div(style={"display": "flex", "backgroundColor": colors["panel_bg"], "borderBottom": f'1px solid {colors["border"]}', "padding": "0 20px"}, children=[html.Div("设备监测", id="nav-monitor", n_clicks=0, style=nav_style(True)), html.Div("报警管理", id="nav-alarm", n_clicks=0, style=nav_style(False)), html.Div("设备管理", id="nav-device", n_clicks=0, style=nav_style(False))]),
            html.Div(style={"display": "flex", "flex": "1", "minWidth": "0", "overflow": "hidden"}, children=[html.Div(style={"width": "260px", "minWidth": "260px", "flex": "0 0 260px", "overflowY": "auto", "overflowX": "hidden", "borderRight": f'1px solid {colors["border"]}', "backgroundColor": colors["panel_bg"]}, children=[html.Div(id="left-list-title", style={"padding": "15px", "fontSize": "12px", "color": "#888", "fontWeight": "bold"}), html.Div(id="left-list-container")]), html.Div(id="main-content-container", style={"flex": "1 1 auto", "minWidth": "0", "maxWidth": "100%", "padding": "20px", "overflowX": "hidden"})]),
            dcc.Store(id="page-store", data="monitor", storage_type="session"),
            dcc.Store(id="selected-id-store", data="cbl-1", storage_type="session"),
            dcc.Store(id="alarm-threshold-store", data=DEFAULT_ALARM_THRESHOLDS, storage_type="session"),
            dcc.Store(id="alarm-events-store", data=[], storage_type="session"),
            dcc.Store(id="device-form-store", data={}, storage_type="session"),
            dcc.Store(id="editing-device-store", data=None, storage_type="session"),
            dcc.Store(id="devices-store", data=normalize_devices(CABLES), storage_type="session"),
            dcc.Interval(id="update-timer", interval=3000, n_intervals=0),
        ],
    )
