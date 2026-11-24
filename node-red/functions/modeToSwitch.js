// Node-RED function node: modeToSwitch
// Input : msg.payload dari topik smartparking/<idkelompok>/mode ("AUTO" / "MANUAL"),
//         yaitu mode asli yang dilaporkan ESP32.
// Output: true (MANUAL) / false (AUTO) untuk widget ui-switch.
// Switch diset "decouple", jadi pesan ini hanya mengubah tampilan dan tidak
// dikirim balik ke ESP32 (tidak ada loop echo).

const mode = String(msg.payload).trim().toUpperCase();

if (mode !== "AUTO" && mode !== "MANUAL") {
    node.warn("Payload mode tidak dikenal: " + msg.payload);
    return null;
}

msg.payload = mode === "MANUAL";
return msg;
