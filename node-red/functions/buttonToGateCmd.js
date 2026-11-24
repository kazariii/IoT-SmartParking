// Node-RED function node: buttonToGateCmd
// Input : msg.payload dari tombol dashboard ("BUKA" / "TUTUP").
// Output: perintah ke topik smartparking/<idkelompok>/gate/cmd.
// ESP32 menjalankan perintah hanya di mode Manual, dan menunda TUTUP
// selama beam terhalang.

const cmd = String(msg.payload).trim().toUpperCase();

if (cmd !== "BUKA" && cmd !== "TUTUP") {
    node.warn("Perintah plang tidak dikenal: " + msg.payload);
    return null;
}

msg.payload = cmd;
return msg;
