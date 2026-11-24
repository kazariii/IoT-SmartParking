// Menyusun flows.json dari kode di folder functions/.
// Edit function di functions/*.js, lalu jalankan: node node-red/build-flows.js
// Hasilnya di-import ke Node-RED (Menu -> Import).

const fs = require("fs");
const path = require("path");

const TOPIC_PREFIX = "smartparking/kelompok1/"; // samakan dengan firmware/network.ino
const BROKER_HOST = "xxxxxxxx.ala.asia-southeast1.emqxsl.com";

const fn = (name) =>
    fs.readFileSync(path.join(__dirname, "functions", name + ".js"), "utf8");

const functionNode = (id, name, x, y, wires) => ({
    id, type: "function", z: "sp_tab", name, func: fn(name), outputs: 1, timeout: 0,
    noerr: 0, initialize: "", finalize: "", libs: [], x, y, wires,
});

const mqttIn = (id, name, topic, x, y, wires) => ({
    id, type: "mqtt in", z: "sp_tab", name, topic: TOPIC_PREFIX + topic, qos: "1",
    datatype: "utf8", broker: "sp_broker", nl: false, rap: true, rh: 0, inputs: 0, x, y, wires,
});

const mqttOut = (id, name, topic, x, y) => ({
    id, type: "mqtt out", z: "sp_tab", name, topic: TOPIC_PREFIX + topic, qos: "1",
    retain: "false", respTopic: "", contentType: "", userProps: "", correl: "", expiry: "",
    broker: "sp_broker", x, y, wires: [],
});

const uiButton = (id, label, payload, order, x, y) => ({
    id, type: "ui-button", z: "sp_tab", group: "sp_group", name: label, label, order,
    width: 0, height: 0, emulateClick: false, tooltip: "", color: "", bgcolor: "",
    className: "", icon: "", iconPosition: "left", payload, payloadType: "str",
    topic: "topic", topicType: "msg", buttonColor: "", textColor: "", iconColor: "",
    enableClick: true, enablePointerdown: false, pointerdownPayload: "",
    pointerdownPayloadType: "str", enablePointerup: false, pointerupPayload: "",
    pointerupPayloadType: "str", x, y, wires: [["sp_fn_gate_cmd"]],
});

const flow = [
    { id: "sp_tab", type: "tab", label: "Smart Parking", disabled: false, info: "" },

    // ---------- Koneksi broker ----------
    {
        id: "sp_tls", type: "tls-config", name: "EMQX TLS", cert: "", key: "", ca: "",
        certname: "", keyname: "", caname: "", servername: "", verifyservercert: true, alpnprotocol: "",
    },
    {
        id: "sp_broker", type: "mqtt-broker", name: "EMQX Serverless", broker: BROKER_HOST,
        port: "8883", tls: "sp_tls", clientid: "", autoConnect: true, usetls: true,
        protocolVersion: 4, keepalive: 60, cleansession: true, autoUnsubscribe: true,
        birthTopic: "", birthQos: "0", birthRetain: "false", birthPayload: "", birthMsg: {},
        closeTopic: "", closeQos: "0", closeRetain: "false", closePayload: "", closeMsg: {},
        willTopic: "", willQos: "0", willRetain: "false", willPayload: "", willMsg: {},
        userProps: "", sessionExpiry: "",
    },

    // ---------- Dashboard 2.0 ----------
    {
        id: "sp_base", type: "ui-base", name: "Smart Parking", path: "/dashboard", appIcon: "",
        includeClientData: true, acceptsClientConfig: ["ui-notification", "ui-control"],
        showPathInSidebar: false, headerContent: "page", navigationStyle: "default",
        titleBarStyle: "default", showReconnectNotification: true, notificationDisplayTime: 1,
        showDisconnectNotification: true, allowInstall: false,
    },
    {
        id: "sp_theme", type: "ui-theme", name: "Default",
        colors: { surface: "#fcfcfd", primary: "#0d74ce", bgPage: "#f9f9fb", groupBg: "#ffffff", groupOutline: "#d9d9e0" },
        sizes: { density: "default", pagePadding: "12px", groupGap: "12px", groupBorderRadius: "4px", widgetGap: "12px" },
    },
    {
        id: "sp_page", type: "ui-page", name: "Parkir", ui: "sp_base", path: "/parkir", icon: "car",
        layout: "grid", theme: "sp_theme",
        breakpoints: [
            { name: "Default", px: 0, cols: 3 }, { name: "Tablet", px: 576, cols: 6 },
            { name: "Small Desktop", px: 768, cols: 9 }, { name: "Desktop", px: 1024, cols: 12 },
        ],
        order: 1, className: "", visible: "true", disabled: "false",
    },
    {
        id: "sp_group", type: "ui-group", name: "Status", page: "sp_page", width: 6, height: 1,
        order: 1, showTitle: true, className: "", visible: "true", disabled: "false", groupType: "default",
    },

    // ---------- Status slot ----------
    mqttIn("sp_mqtt_slot", "slot", "slot", 110, 80, [["sp_fn_slot"]]),
    functionNode("sp_fn_slot", "formatSlot", 300, 80, [["sp_ui_slot"]]),
    {
        id: "sp_ui_slot", type: "ui-text", z: "sp_tab", group: "sp_group", order: 1, width: 0,
        height: 0, name: "Status Slot", label: "Status Slot", format: "{{msg.payload}}",
        layout: "row-spread", style: true, font: "Helvetica", fontSize: 20, color: "#717171",
        wrapText: false, className: "", value: "payload", valueType: "msg", x: 500, y: 80, wires: [],
    },

    // ---------- Mode Auto/Manual ----------
    mqttIn("sp_mqtt_mode", "mode", "mode", 110, 160, [["sp_fn_mode_switch", "sp_fn_btn_state"]]),
    functionNode("sp_fn_mode_switch", "modeToSwitch", 300, 160, [["sp_ui_mode"]]),
    {
        // decouple: klik hanya mengirim perintah; posisi switch mengikuti laporan ESP32.
        id: "sp_ui_mode", type: "ui-switch", z: "sp_tab", name: "Mode Manual", label: "Mode Manual",
        group: "sp_group", order: 2, width: 0, height: 0, passthru: false, decouple: true,
        topic: "topic", topicType: "msg", style: "", className: "", layout: "row-spread",
        clickableArea: "switch", onvalue: "true", onvalueType: "bool", onicon: "", oncolor: "",
        offvalue: "false", offvalueType: "bool", officon: "", offcolor: "",
        x: 500, y: 160, wires: [["sp_fn_mode_set"]],
    },
    functionNode("sp_fn_mode_set", "switchToModeSet", 700, 160, [["sp_mqtt_mode_set"]]),
    mqttOut("sp_mqtt_mode_set", "mode/set", "mode/set", 900, 160),

    // ---------- Tombol Buka/Tutup ----------
    functionNode("sp_fn_btn_state", "modeToButtonState", 300, 240, [["sp_ui_btn_open", "sp_ui_btn_close"]]),
    uiButton("sp_ui_btn_open", "Buka", "BUKA", 3, 500, 220),
    uiButton("sp_ui_btn_close", "Tutup", "TUTUP", 4, 500, 260),
    functionNode("sp_fn_gate_cmd", "buttonToGateCmd", 700, 240, [["sp_mqtt_gate_cmd"]]),
    mqttOut("sp_mqtt_gate_cmd", "gate/cmd", "gate/cmd", 900, 240),
];

fs.writeFileSync(path.join(__dirname, "flows.json"), JSON.stringify(flow, null, 2) + "\n");
console.log("flows.json: " + flow.length + " node");
