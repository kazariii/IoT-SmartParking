// Node-RED function node: modeToButtonState
// Input : msg.payload dari topik smartparking/<idkelompok>/mode ("AUTO" / "MANUAL").
// Output: { enabled } untuk tombol Buka/Tutup. Tombol aktif hanya di mode Manual.
// Pesan baru tanpa payload, supaya tombol hanya berubah status aktifnya.

const mode = String(msg.payload).trim().toUpperCase();

if (mode !== "AUTO" && mode !== "MANUAL") {
    node.warn("Payload mode tidak dikenal: " + msg.payload);
    return null;
}

return { enabled: mode === "MANUAL" };
