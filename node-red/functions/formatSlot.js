// Node-RED function node: formatSlot
// Input : msg.payload dari topik smartparking/<idkelompok>/slot ("KOSONG" / "TERISI")
// Output: teks + warna untuk widget ui-text (Dashboard 2.0)

const status = String(msg.payload).trim().toUpperCase();

if (status === "KOSONG") {
    msg.payload = "KOSONG";
    msg.ui_update = { color: "#1a7f37" };
} else if (status === "TERISI") {
    msg.payload = "TERISI";
    msg.ui_update = { color: "#cf222e" };
} else {
    node.warn("Payload slot tidak dikenal: " + msg.payload);
    return null;
}

return msg;
