// Node-RED function node: switchToModeSet
// Input : msg.payload dari ui-switch saat diklik (true = MANUAL, false = AUTO).
// Output: perintah ke topik smartparking/<idkelompok>/mode/set.
// Posisi switch baru berubah setelah ESP32 membalas lewat topik mode.

msg.payload = msg.payload === true ? "MANUAL" : "AUTO";
return msg;
