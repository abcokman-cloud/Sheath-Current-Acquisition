import json
from datetime import datetime

import dash
from dash import Input, Output, State, html

from current_data import (
    ALARM_MENU,
    ALARM_PARAMETERS,
    CABLES,
    DEFAULT_ALARM_THRESHOLDS,
    DEVICE_MENU,
    MODBUS_DEFAULTS,
    build_left_items,
    format_channel_enable,
    generate_alarm_events,
    get_channel_toggle_children,
    get_channel_toggle_style,
    nav_style,
    normalize_devices,
)
from current_views import build_alarm_content, build_device_content, build_monitor_content


def register_callbacks(app):
    @app.callback(
        [
            Output("page-store", "data"),
            Output("selected-id-store", "data"),
            Output("nav-monitor", "style"),
            Output("nav-alarm", "style"),
            Output("nav-device", "style"),
        ],
        [Input("nav-monitor", "n_clicks"), Input("nav-alarm", "n_clicks"), Input("nav-device", "n_clicks")],
        prevent_initial_call=True,
    )
    def switch_page(n1, n2, n3):
        del n1, n2, n3
        bid = dash.callback_context.triggered[0]["prop_id"].split(".")[0]
        if bid == "nav-alarm":
            return "alarm", "alarm-list", nav_style(False), nav_style(True), nav_style(False)
        if bid == "nav-device":
            return "device", "device-add", nav_style(False), nav_style(False), nav_style(True)
        return "monitor", "cbl-1", nav_style(True), nav_style(False), nav_style(False)

    @app.callback(Output("selected-id-store", "data", allow_duplicate=True), Input({"type": "cable-item", "index": dash.ALL}, "n_clicks"), prevent_initial_call=True)
    def handle_left_click(n_clicks):
        ctx = dash.callback_context
        if not n_clicks or not any(n_clicks) or not ctx.triggered:
            return dash.no_update
        return json.loads(ctx.triggered[0]["prop_id"].split(".")[0])["index"]

    @app.callback(
        [Output("main-content-container", "children"), Output("left-list-container", "children"), Output("left-list-title", "children")],
        [
            Input("page-store", "data"),
            Input("selected-id-store", "data"),
            Input("alarm-threshold-store", "data"),
            Input("alarm-events-store", "data"),
            Input("device-form-store", "data"),
            Input("editing-device-store", "data"),
            Input("devices-store", "data"),
            Input("update-timer", "n_intervals"),
        ],
    )
    def render_ui(page, sel_id, alarm_thresholds, alarm_events, form_data, editing_device_id, devices_data, n_intervals):
        ctx = dash.callback_context
        trigger_id = ctx.triggered[0]["prop_id"].split(".")[0] if ctx.triggered else None
        if page == "monitor" and trigger_id in {"alarm-threshold-store", "alarm-events-store", "device-form-store", "editing-device-store"}:
            return dash.no_update, dash.no_update, dash.no_update
        if page == "device" and trigger_id in {"alarm-threshold-store", "alarm-events-store"}:
            return dash.no_update, dash.no_update, dash.no_update
        if page == "alarm" and trigger_id in {"device-form-store", "editing-device-store"}:
            return dash.no_update, dash.no_update, dash.no_update
        if page == "alarm" and sel_id != "alarm-list" and trigger_id in {"alarm-threshold-store", "alarm-events-store"}:
            return dash.no_update, dash.no_update, dash.no_update
        if trigger_id == "update-timer" and page != "monitor":
            return dash.no_update, dash.no_update, dash.no_update

        monitor_devices = normalize_devices(devices_data or CABLES)
        if page == "monitor":
            cur_list = monitor_devices
            list_title = "Cable Line List"
            content = build_monitor_content(sel_id, monitor_devices, alarm_thresholds)
        elif page == "alarm":
            cur_list = ALARM_MENU
            list_title = "Alarm Menus"
            content = build_alarm_content(sel_id, alarm_events or [], alarm_thresholds or {})
        else:
            cur_list = DEVICE_MENU
            list_title = "Device Menus"
            content = build_device_content(sel_id, monitor_devices, form_data, editing_device_id)
        return content, build_left_items(cur_list, sel_id), list_title

    @app.callback(Output("live-clock", "children"), Input("update-timer", "n_intervals"))
    def update_live_clock(n):
        del n
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    @app.callback(
        [Output("devices-store", "data"), Output("selected-id-store", "data", allow_duplicate=True), Output("device-form-store", "data"), Output("editing-device-store", "data")],
        Input("save-device-btn", "n_clicks"),
        [
            State("devices-store", "data"),
            State("editing-device-store", "data"),
            State("device-cable-name", "value"),
            State("device-cable-model", "value"),
            State("device-cable-length", "value"),
            State("device-laying-method", "value"),
            State("device-tunnel-name", "value"),
            State("device-phase-select", "value"),
            State("device-location", "value"),
            State("conn-type-dropdown", "value"),
            State("device-modbus-id", "value"),
            State("device-baudrate", "value"),
            State("device-tcp-port", "value"),
            State("device-ip-address", "value"),
            State("device-subnet-mask", "value"),
            State("device-gateway", "value"),
            State("device-485-address", "value"),
            State("device-ct-ratio", "value"),
            State("device-ct-offset", "value"),
            State("device-channel-count", "value"),
            State("device-channel-enable", "data"),
        ],
        prevent_initial_call=True,
    )
    def save_device(n_clicks, devices_data, editing_device_id, cable_name, cable_model, cable_length, laying_method, tunnel_name, phase, device_location, conn_type, modbus_slave_id, baudrate, tcp_port, ip_address, subnet_mask, gateway, addr_485, ct_ratio, ct_offset, channel_count, channel_enable):
        if not n_clicks:
            return dash.no_update, dash.no_update, dash.no_update, dash.no_update
        current_devices = normalize_devices(devices_data or CABLES)
        display_name = cable_name or f"New Device {len(current_devices) + 1}"
        position_parts = [part for part in [tunnel_name, phase, device_location] if part]
        channel_index = int(channel_count or 1)
        normalized_flags = [1 if index == channel_index - 1 else 0 for index in range(8)]
        if (channel_enable or "enabled") != "enabled":
            normalized_flags = [0] * 8
        new_device = {
            "id": editing_device_id or f"cbl-user-{len(current_devices) + 1}",
            "name": display_name,
            "pos": " / ".join(position_parts) if position_parts else "No Location Info",
            "model": cable_model or "-",
            "length": cable_length or "-",
            "laying_method": laying_method or "-",
            "tunnel_name": tunnel_name or "-",
            "phase": phase or "-",
            "location": device_location or "-",
            "connection_type": conn_type or "IP",
            "modbus_slave_id": int(modbus_slave_id if modbus_slave_id is not None else MODBUS_DEFAULTS["modbus_slave_id"]),
            "baudrate": int(baudrate if baudrate is not None else MODBUS_DEFAULTS["baudrate"]),
            "tcp_port": int(tcp_port if tcp_port is not None else MODBUS_DEFAULTS["tcp_port"]),
            "ip_address": ip_address or MODBUS_DEFAULTS["ip_address"],
            "subnet_mask": subnet_mask or MODBUS_DEFAULTS["subnet_mask"],
            "gateway": gateway or MODBUS_DEFAULTS["gateway"],
            "addr_485": addr_485 or "-",
            "ct_ratio": float(ct_ratio if ct_ratio is not None else MODBUS_DEFAULTS["ct_ratio"]),
            "ct_offset": float(ct_offset if ct_offset is not None else MODBUS_DEFAULTS["ct_offset"]),
            "channel_enable": format_channel_enable(normalized_flags),
            "online_status": "Online" if (ip_address or addr_485) else "Offline",
        }
        if editing_device_id:
            current_devices = [new_device if device["id"] == editing_device_id else device for device in current_devices]
        else:
            current_devices.append(new_device)
        return current_devices, "device-list", {}, None

    @app.callback([Output("devices-store", "data", allow_duplicate=True), Output("selected-id-store", "data", allow_duplicate=True)], Input({"type": "delete-device-btn", "index": dash.ALL}, "n_clicks"), State("devices-store", "data"), prevent_initial_call=True)
    def delete_device(n_clicks, devices_data):
        ctx = dash.callback_context
        if not n_clicks or not any(n_clicks) or not ctx.triggered:
            return dash.no_update, dash.no_update
        target_id = json.loads(ctx.triggered[0]["prop_id"].split(".")[0])["index"]
        filtered_devices = [device for device in normalize_devices(devices_data or CABLES) if device["id"] != target_id]
        return filtered_devices, "device-list"

    @app.callback(
        [Output("page-store", "data", allow_duplicate=True), Output("selected-id-store", "data", allow_duplicate=True), Output("device-form-store", "data", allow_duplicate=True), Output("editing-device-store", "data", allow_duplicate=True)],
        Input({"type": "manage-device-btn", "index": dash.ALL}, "n_clicks"),
        State("devices-store", "data"),
        prevent_initial_call=True,
    )
    def manage_device(n_clicks, devices_data):
        ctx = dash.callback_context
        if not n_clicks or not any(n_clicks) or not ctx.triggered:
            return dash.no_update, dash.no_update, dash.no_update, dash.no_update
        target_id = json.loads(ctx.triggered[0]["prop_id"].split(".")[0])["index"]
        current_devices = normalize_devices(devices_data or CABLES)
        target_device = next((device for device in current_devices if device["id"] == target_id), None)
        if target_device is None:
            return dash.no_update, dash.no_update, dash.no_update, dash.no_update
        form_data = {
            "name": "" if target_device.get("name") == "Unnamed Device" else target_device.get("name", ""),
            "model": "" if target_device.get("model") == "-" else target_device.get("model", ""),
            "length": "" if target_device.get("length") == "-" else target_device.get("length", ""),
            "laying_method": "" if target_device.get("laying_method") == "-" else target_device.get("laying_method", ""),
            "tunnel_name": "" if target_device.get("tunnel_name") == "-" else target_device.get("tunnel_name", ""),
            "phase": "" if target_device.get("phase") == "-" else target_device.get("phase", ""),
            "location": "" if target_device.get("location") == "-" else target_device.get("location", ""),
            "connection_type": target_device.get("connection_type", "IP"),
            "modbus_slave_id": target_device.get("modbus_slave_id", MODBUS_DEFAULTS["modbus_slave_id"]),
            "baudrate": target_device.get("baudrate", MODBUS_DEFAULTS["baudrate"]),
            "tcp_port": target_device.get("tcp_port", MODBUS_DEFAULTS["tcp_port"]),
            "ip_address": target_device.get("ip_address", MODBUS_DEFAULTS["ip_address"]),
            "subnet_mask": target_device.get("subnet_mask", MODBUS_DEFAULTS["subnet_mask"]),
            "gateway": target_device.get("gateway", MODBUS_DEFAULTS["gateway"]),
            "addr_485": "" if target_device.get("addr_485") == "-" else target_device.get("addr_485", ""),
            "ct_ratio": target_device.get("ct_ratio", MODBUS_DEFAULTS["ct_ratio"]),
            "ct_offset": target_device.get("ct_offset", MODBUS_DEFAULTS["ct_offset"]),
            "channel_enable": target_device.get("channel_enable", MODBUS_DEFAULTS["channel_enable"]),
        }
        return "device", "device-add", form_data, target_id

    @app.callback(
        [
            Output("alarm-threshold-store", "data"),
            Output("alarm-config-status-message", "children"),
        ],
        Input("save-alarm-threshold-btn", "n_clicks"),
        [
            State({"type": "alarm-lower-input", "index": dash.ALL}, "value"),
            State({"type": "alarm-upper-input", "index": dash.ALL}, "value"),
        ],
        prevent_initial_call=True,
    )
    def save_alarm_thresholds(n_clicks, lower_values, upper_values):
        if not n_clicks:
            return dash.no_update, dash.no_update
        thresholds = {}
        for idx, parameter in enumerate(ALARM_PARAMETERS):
            default = DEFAULT_ALARM_THRESHOLDS[parameter["key"]]
            lower = lower_values[idx] if idx < len(lower_values) and lower_values[idx] is not None else default["lower"]
            upper = upper_values[idx] if idx < len(upper_values) and upper_values[idx] is not None else default["upper"]
            thresholds[parameter["key"]] = {"lower": float(lower), "upper": float(upper)}
            
        # Using CSS animations instead of Interval timer
        success_msg = html.Div(
            "Saved Successfully!",
            id=f"toast-msg-{n_clicks}", # Assign new ID on every click to force replay animation
            style={
                "color": "#52c41a",
                "fontWeight": "bold",
                "padding": "5px 12px",
                "borderRadius": "999px",
                "border": "1px solid rgba(82, 196, 26, 0.4)",
                "backgroundColor": "rgba(37, 187, 154, 0.18)",
                "minWidth": "90px",
                "textAlign": "center",
                "display": "inline-flex",
                "alignItems": "center",
                "animation": "fadeOutMessage 3s forwards", # Auto fade out after 3s
                "pointerEvents": "none",
            }
        )
        return thresholds, success_msg

    @app.callback(Output("alarm-events-store", "data"), [Input("update-timer", "n_intervals"), Input("devices-store", "data"), Input("alarm-threshold-store", "data")])
    def refresh_alarm_events(n, devices_data, thresholds):
        del n
        return generate_alarm_events(devices_data or CABLES, thresholds or DEFAULT_ALARM_THRESHOLDS)

    @app.callback(
        [Output("ai-analysis-container", "children"), Output("ai-analysis-container", "style")],
        [Input({"type": "row-ai-btn", "index": dash.ALL}, "n_clicks"), Input("ai-analysis-container", "n_clicks")],
        [State("ai-analysis-container", "style"), State("alarm-events-store", "data")],
        prevent_initial_call=True,
    )
    def handle_ai_click(n_clicks_list, n_container_clicks, current_style, alarm_events):
        del n_clicks_list, n_container_clicks, current_style
        ctx = dash.callback_context
        if not ctx.triggered:
            return dash.no_update
        trigger_id = ctx.triggered[0]["prop_id"]
        if "ai-analysis-container" in trigger_id:
            return "", {"display": "none"}
        if "row-ai-btn" in trigger_id:
            callback_id = json.loads(trigger_id.split(".")[0])
            alarm_data = next((item for item in (alarm_events or []) if item["id"] == callback_id["index"]), None)
            if alarm_data is None:
                return dash.no_update
            analysis_text = f"AI Analysis for '{alarm_data['item']}': The current data has exceeded the threshold. It is recommended to prioritize checking the grounding loop, sensor wiring, and recent field condition changes."
            content = html.Div(style={"padding": "20px", "backgroundColor": "#120338", "border": "1px solid #722ed1", "borderRadius": "8px"}, children=[html.H4(f"AI Analysis Report - {alarm_data['item']}", style={"color": "#b37feb", "marginTop": "0"}), html.P(analysis_text, style={"lineHeight": "1.6"}), html.Button("Click here to close the analysis report", style={"marginTop": "10px", "padding": "5px 15px", "backgroundColor": "#434343", "color": "white", "border": "none", "borderRadius": "4px", "cursor": "pointer"})])
            return content, {"display": "block", "marginTop": "20px"}
        return dash.no_update

    @app.callback([Output("ip-input-container", "style"), Output("485-input-container", "style")], [Input("conn-type-dropdown", "value")])
    def toggle_conn_inputs(conn_type):
        del conn_type
        stable_style = {"display": "block", "minWidth": "0"}
        return stable_style, stable_style

    @app.callback([Output("device-channel-enable", "data"), Output("device-channel-enable-toggle", "children"), Output("device-channel-enable-toggle", "style")], Input("device-channel-enable-toggle", "n_clicks"), State("device-channel-enable", "data"), prevent_initial_call=True)
    def toggle_channel_enable(n_clicks, current_status):
        del n_clicks
        next_status = "disabled" if current_status == "enabled" else "enabled"
        return next_status, get_channel_toggle_children(next_status), get_channel_toggle_style(next_status)
