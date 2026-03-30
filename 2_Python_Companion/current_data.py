from datetime import datetime

import numpy as np
from dash import html


CABLES = [
    {"id": "cbl-1", "name": "HV Cable 1", "pos": "Switchgear Phase A"},
    {"id": "cbl-2", "name": "HV Cable 2", "pos": "Switchgear Phase B"},
    {"id": "cbl-3", "name": "HV Cable 3", "pos": "Switchgear Phase C"},
    {"id": "cbl-4", "name": "Tie Cable 4", "pos": "Tunnel 1"},
]

ALARM_MENU = [
    {"id": "alarm-list", "name": "Alarm List", "pos": "All abnormal history logs"},
    {"id": "alarm-config", "name": "Alarm Config", "pos": "Threshold and notification settings"},
]

DEVICE_MENU = [
    {"id": "device-add", "name": "Add Device", "pos": "Enter new cable & device info"},
    {"id": "device-list", "name": "Device List", "pos": "Manage existing device ledger"},
]

ALARM_PARAMETERS = [
    {"key": "total_rms", "label": "Total RMS", "unit": "A"},
    {"key": "peak_current", "label": "Peak Current", "unit": "A"},
    {"key": "valley_current", "label": "Valley Current", "unit": "A"},
    {"key": "peak_phase", "label": "Peak Phase Offset", "unit": "deg"},
    {"key": "valley_phase", "label": "Valley Phase Offset", "unit": "deg"},
    {"key": "zero_cross", "label": "Zero-cross Offset", "unit": "deg"},
    {"key": "thd", "label": "THD", "unit": "%"},
    {"key": "harmonic_1", "label": "1st Harmonic", "unit": "%"},
    {"key": "harmonic_2", "label": "2nd Harmonic", "unit": "%"},
    {"key": "harmonic_3", "label": "3rd Harmonic", "unit": "%"},
    {"key": "harmonic_5", "label": "5th Harmonic", "unit": "%"},
    {"key": "harmonic_7", "label": "7th Harmonic", "unit": "%"},
]

DEFAULT_ALARM_THRESHOLDS = {
    "total_rms": {"lower": 0.0, "upper": 20.0},
    "peak_current": {"lower": 0.0, "upper": 30.0},
    "valley_current": {"lower": -30.0, "upper": 0.0},
    "peak_phase": {"lower": -180.0, "upper": 180.0},
    "valley_phase": {"lower": -180.0, "upper": 180.0},
    "zero_cross": {"lower": -180.0, "upper": 180.0},
    "thd": {"lower": 0.0, "upper": 30.0},
    "harmonic_1": {"lower": 90.0, "upper": 110.0},
    "harmonic_2": {"lower": 0.0, "upper": 10.0},
    "harmonic_3": {"lower": 0.0, "upper": 25.0},
    "harmonic_5": {"lower": 0.0, "upper": 15.0},
    "harmonic_7": {"lower": 0.0, "upper": 10.0},
}

NAV_BASE_STYLE = {"padding": "15px 20px", "cursor": "pointer"}

colors = {
    "background": "#000511",
    "panel_bg": "#00102a",
    "text": "#ffffff",
    "accent": "#007bff",
    "chart_a": "#00ff99",
    "border": "#1a2b4a",
}

MODBUS_DEFAULTS = {
    "modbus_slave_id": 1,
    "baudrate": 115200,
    "tcp_port": 502,
    "ip_address": "192.168.1.100",
    "subnet_mask": "255.255.255.0",
    "gateway": "192.168.1.1",
    "ct_ratio": 1.0,
    "ct_offset": 0.0,
    "channel_enable": "1,1,1,1,0,0,0,0",
}


def nav_style(active: bool) -> dict:
    style = dict(NAV_BASE_STYLE)
    if active:
        style.update(
            {
                "borderBottom": f'2px solid {colors["accent"]}',
                "color": colors["accent"],
                "fontWeight": "bold",
            }
        )
    else:
        style["color"] = "#888"
    return style


def get_advanced_mock_data(cable_id: str):
    seed = sum(ord(ch) for ch in cable_id)
    np.random.seed(seed + int(datetime.now().timestamp()) // 3)
    t = np.linspace(0, 0.04, 256)
    cable_params = {
        "cbl-1": {"amp1": 10.0, "amp3": 2.0, "phase": -0.3, "dc": 0.5, "peak_p": "+45.00 deg", "valley_p": "+42.19 deg", "zero_c": "-171.56 deg", "harm": [100.0, 2.5, 20.0, 4.2, 10.0, 1.5, 3.2], "thd": 23.16},
        "cbl-2": {"amp1": 15.5, "amp3": 1.5, "phase": 0.5, "dc": -0.2, "peak_p": "+25.10 deg", "valley_p": "+21.05 deg", "zero_c": "-150.20 deg", "harm": [100.0, 1.8, 15.3, 2.1, 8.5, 1.1, 4.0], "thd": 18.05},
        "cbl-3": {"amp1": 22.0, "amp3": 4.0, "phase": 1.2, "dc": 1.0, "peak_p": "-15.30 deg", "valley_p": "-12.45 deg", "zero_c": "+110.50 deg", "harm": [100.0, 4.5, 28.0, 5.6, 12.0, 2.5, 6.1], "thd": 31.84},
        "cbl-4": {"amp1": 5.2, "amp3": 0.3, "phase": -1.5, "dc": 0.1, "peak_p": "+85.20 deg", "valley_p": "+80.15 deg", "zero_c": "-45.10 deg", "harm": [100.0, 0.8, 8.5, 1.2, 4.3, 0.5, 1.8], "thd": 9.80},
    }
    params = cable_params.get(cable_id, cable_params["cbl-1"])
    ya = (
        params["amp1"] * np.sin(2 * np.pi * 50 * t + params["phase"])
        + params["amp3"] * np.sin(2 * np.pi * 150 * t)
        + params["dc"]
    )
    return (
        t,
        ya,
        float(np.max(ya)),
        float(np.min(ya)),
        float(np.sqrt(np.mean(ya**2))),
        params["harm"],
        params["thd"],
        params["peak_p"],
        params["valley_p"],
        params["zero_c"],
    )


def build_left_items(cur_list, sel_id):
    return [
        html.Div(
            [
                html.Div(
                    item["name"],
                    style={
                        "fontWeight": "bold",
                        "whiteSpace": "normal",
                        "wordBreak": "break-word",
                        "writingMode": "horizontal-tb",
                    },
                ),
                html.Div(
                    item["pos"],
                    style={
                        "fontSize": "11px",
                        "color": "#888",
                        "whiteSpace": "normal",
                        "wordBreak": "break-word",
                        "writingMode": "horizontal-tb",
                        "lineHeight": "1.5",
                    },
                ),
            ],
            id={"type": "cable-item", "index": item["id"]},
            n_clicks=0,
            style={
                "padding": "15px",
                "cursor": "pointer",
                "borderBottom": f'1px solid {colors["border"]}',
                "width": "100%",
                "boxSizing": "border-box",
                "overflow": "hidden",
                "backgroundColor": colors["accent"] if item["id"] == sel_id else "transparent",
                "borderLeft": "4px solid #fff" if item["id"] == sel_id else "none",
            },
        )
        for item in cur_list
    ]


def normalize_devices(devices):
    normalized = []
    for device in devices:
        normalized.append(
            {
                "id": device.get("id", ""),
                "name": device.get("name", "Unnamed Device"),
                "pos": device.get("pos", "No Location Info"),
                "model": device.get("model", "-"),
                "length": device.get("length", "-"),
                "laying_method": device.get("laying_method", "-"),
                "tunnel_name": device.get("tunnel_name", "-"),
                "phase": device.get("phase", "-"),
                "location": device.get("location", device.get("pos", "-")),
                "connection_type": device.get("connection_type", "IP"),
                "ip_address": device.get("ip_address", MODBUS_DEFAULTS["ip_address"]),
                "addr_485": device.get("addr_485", "-"),
                "modbus_slave_id": device.get("modbus_slave_id", MODBUS_DEFAULTS["modbus_slave_id"]),
                "baudrate": device.get("baudrate", MODBUS_DEFAULTS["baudrate"]),
                "tcp_port": device.get("tcp_port", MODBUS_DEFAULTS["tcp_port"]),
                "subnet_mask": device.get("subnet_mask", MODBUS_DEFAULTS["subnet_mask"]),
                "gateway": device.get("gateway", MODBUS_DEFAULTS["gateway"]),
                "ct_ratio": device.get("ct_ratio", MODBUS_DEFAULTS["ct_ratio"]),
                "ct_offset": device.get("ct_offset", MODBUS_DEFAULTS["ct_offset"]),
                "channel_enable": device.get("channel_enable", MODBUS_DEFAULTS["channel_enable"]),
                "online_status": device.get("online_status", "Online" if device.get("ip_address") or device.get("addr_485", "-") != "-" else "Offline"),
            }
        )
    return normalized


def parse_numeric_text(value):
    if isinstance(value, (int, float)):
        return float(value)
    if not value:
        return 0.0
    cleaned = str(value).replace("deg", "").replace("%", "").replace("A", "").strip()
    try:
        return float(cleaned)
    except ValueError:
        return 0.0


def parse_channel_enable(value):
    raw = str(value or MODBUS_DEFAULTS["channel_enable"]).split(",")
    flags = [1 if part.strip() == "1" else 0 for part in raw[:8]]
    flags.extend([0] * (8 - len(flags)))
    return flags[:8]


def format_channel_enable(flags):
    normalized = [1 if int(flag) else 0 for flag in flags[:8]]
    normalized.extend([0] * (8 - len(normalized)))
    return ",".join(str(flag) for flag in normalized[:8])


def get_channel_status(channel_enable):
    return "enabled" if any(parse_channel_enable(channel_enable)) else "disabled"


def get_channel_toggle_style(status):
    enabled = status == "enabled"
    return {
        "width": "120px",
        "height": "38px",
        "border": "none",
        "borderRadius": "999px",
        "cursor": "pointer",
        "padding": "4px",
        "display": "flex",
        "alignItems": "center",
        "justifyContent": "space-between" if enabled else "flex-start",
        "backgroundColor": "#16a34a" if enabled else "#dc2626",
        "color": "white",
        "fontWeight": "bold",
        "boxSizing": "border-box",
    }


def get_channel_toggle_children(status):
    enabled = status == "enabled"
    label = "Enable" if enabled else "Disable"
    knob = html.Span(
        style={
            "width": "30px",
            "height": "30px",
            "borderRadius": "50%",
            "backgroundColor": "white",
            "display": "inline-block",
            "flexShrink": "0",
        }
    )
    text = html.Span(label, style={"padding": "0 10px", "fontSize": "14px", "lineHeight": "30px"})
    return [text, knob] if enabled else [knob, text]


def normalize_thresholds(thresholds):
    normalized = {}
    for parameter in ALARM_PARAMETERS:
        key = parameter["key"]
        current = (thresholds or {}).get(key, DEFAULT_ALARM_THRESHOLDS[key])
        normalized[key] = {
            "lower": float(current.get("lower", DEFAULT_ALARM_THRESHOLDS[key]["lower"])),
            "upper": float(current.get("upper", DEFAULT_ALARM_THRESHOLDS[key]["upper"])),
        }
    return normalized


def get_monitor_metrics(cable_id):
    _, ya, peak_current, valley_current, total_rms, harm, thd, peak_phase, valley_phase, zero_cross = get_advanced_mock_data(cable_id)
    del ya
    return {
        "total_rms": float(total_rms),
        "peak_current": float(peak_current),
        "valley_current": float(valley_current),
        "peak_phase": parse_numeric_text(peak_phase),
        "valley_phase": parse_numeric_text(valley_phase),
        "zero_cross": parse_numeric_text(zero_cross),
        "thd": float(thd),
        "harmonic_1": float(harm[0]),
        "harmonic_2": float(harm[1]),
        "harmonic_3": float(harm[2]),
        "harmonic_5": float(harm[4]),
        "harmonic_7": float(harm[6]),
    }


def generate_alarm_events(devices, thresholds):
    threshold_map = normalize_thresholds(thresholds)
    events = []
    event_id = 0
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    for device in normalize_devices(devices):
        metrics = get_monitor_metrics(device["id"])
        for parameter in ALARM_PARAMETERS:
            key = parameter["key"]
            current_value = metrics[key]
            lower = threshold_map[key]["lower"]
            upper = threshold_map[key]["upper"]
            if lower <= current_value <= upper:
                continue
            events.append(
                {
                    "id": event_id,
                    "time": now,
                    "device_name": device["name"],
                    "item": parameter["label"],
                    "value": f'{current_value:.2f} {parameter["unit"]}'.strip(),
                    "limit": f"{lower:.2f} ~ {upper:.2f} {parameter['unit']}".strip(),
                            "info": f'{device["name"]}\'s {parameter["label"]} is {current_value:.2f}, exceeding the threshold',
                    "info": f'{device["name"]}\'s {parameter["label"]} is {current_value:.2f}, exceeding the threshold',
                }
            )
            event_id += 1
    return events
