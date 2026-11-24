// Smart Parking - firmware ESP32
// Desain lengkap: DESAIN.md

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// ---------- Pin ----------
const uint8_t PIN_SLOT_SENSOR = 27;  // FC-51 OUT, LOW = ada objek
const uint8_t PIN_BEAM        = 26;  // IR break beam receiver, LOW = terhalang
const uint8_t PIN_SERVO       = 13;
const uint8_t PIN_LED_GREEN   = 25;
const uint8_t PIN_LED_RED     = 33;
const uint8_t PIN_BUZZER      = 32;  // buzzer aktif, HIGH = bunyi
// LCD I2C memakai SDA 21 / SCL 22 (default Wire ESP32)

// ---------- Konfigurasi ----------
const uint8_t LCD_ADDRESS = 0x27;  // ganti ke 0x3F jika LCD tidak tampil
const unsigned long SLOT_DEBOUNCE_MS = 1000;
const unsigned long BEAM_DEBOUNCE_MS = 50;
const unsigned long GATE_CLOSE_DELAY_MS = 3000;  // plang tutup setelah beam bersih selama ini
const unsigned long WELCOME_MSG_MS = 2000;       // lama "Silakan Masuk" sebelum "Hati-hati"
const unsigned long FULL_MSG_MS = 2000;          // lama "Parkir Penuh" setelah beam bersih
const int SERVO_CLOSED_DEG = 0;
const int SERVO_OPEN_DEG = 90;
const unsigned long BEEP_ON_MS = 100;
const unsigned long BEEP_OFF_MS = 100;

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
Servo gateServo;

// ---------- Input ter-debounce ----------
struct DebouncedInput {
  bool stable;
  bool rawLast;
  unsigned long rawChangedAt;
  unsigned long debounceMs;
};

DebouncedInput slotInput = {false, false, 0, SLOT_DEBOUNCE_MS};
DebouncedInput beamInput = {false, false, 0, BEAM_DEBOUNCE_MS};

void initInput(DebouncedInput &in, bool raw) {
  in.stable = in.rawLast = raw;
  in.rawChangedAt = millis();
}

// Mengembalikan true jika status stabil berubah.
bool updateInput(DebouncedInput &in, bool raw) {
  unsigned long now = millis();
  if (raw != in.rawLast) {
    in.rawLast = raw;
    in.rawChangedAt = now;
  }
  if (raw != in.stable && now - in.rawChangedAt >= in.debounceMs) {
    in.stable = raw;
    return true;
  }
  return false;
}

// Salinan status slot untuk task jaringan (core lain), lihat network.ino.
volatile bool sharedSlotOccupied = false;
// Mode Manual: plang hanya digerakkan dari dashboard. Ditulis oleh network.ino.
volatile bool manualMode = false;

// Perintah plang dari dashboard. Ditulis oleh network.ino, diambil oleh loop().
enum GateCmd : uint8_t { GATE_CMD_NONE, GATE_CMD_OPEN, GATE_CMD_CLOSE };
volatile GateCmd pendingGateCmd = GATE_CMD_NONE;

bool readSlotRaw() { return digitalRead(PIN_SLOT_SENSOR) == LOW; }
bool readBeamRaw() { return digitalRead(PIN_BEAM) == LOW; }

// ---------- Buzzer (non-blocking) ----------
uint8_t beepsRemaining = 0;
bool buzzerOn = false;
unsigned long buzzerPhaseAt = 0;

void beep(uint8_t count) {
  beepsRemaining = count;
  buzzerOn = false;
  buzzerPhaseAt = millis() - BEEP_OFF_MS;  // bunyi pertama langsung mulai
}

void updateBuzzer() {
  unsigned long now = millis();
  if (buzzerOn && now - buzzerPhaseAt >= BEEP_ON_MS) {
    buzzerOn = false;
    buzzerPhaseAt = now;
    digitalWrite(PIN_BUZZER, LOW);
  } else if (!buzzerOn && beepsRemaining > 0 && now - buzzerPhaseAt >= BEEP_OFF_MS) {
    buzzerOn = true;
    buzzerPhaseAt = now;
    beepsRemaining--;
    digitalWrite(PIN_BUZZER, HIGH);
  }
}

// ---------- Plang ----------
bool gateOpen = false;
unsigned long gateOpenedAt = 0;
unsigned long beamClearedAt = 0;  // kapan beam terakhir menjadi bersih
unsigned long deniedAt = 0;       // kapan terakhir akses ditolak (0 = belum pernah)
bool manualCloseRequested = false; // TUTUP manual menunggu beam bersih

void setGate(bool open) {
  gateOpen = open;
  gateServo.write(open ? SERVO_OPEN_DEG : SERVO_CLOSED_DEG);
  beep(1);
  if (open) gateOpenedAt = millis();
  Serial.println(open ? "Plang: BUKA" : "Plang: TUTUP");
}

// Dipanggil saat beam berubah dari bersih menjadi terhalang.
void onBeamBroken() {
  if (gateOpen) return;    // mobil yang sama masih lewat
  if (manualMode) return;  // mode Manual: beam tidak membuka plang
  if (slotInput.stable) {
    deniedAt = millis();
    beep(3);
    Serial.println("Akses ditolak: parkir penuh");
  } else {
    setGate(true);  // berlaku untuk mobil masuk maupun keluar
  }
}

void handleGateCommand() {
  GateCmd cmd = pendingGateCmd;
  if (cmd == GATE_CMD_NONE) return;
  pendingGateCmd = GATE_CMD_NONE;

  if (!manualMode) {
    Serial.println("Perintah plang diabaikan: mode Auto");
    return;
  }
  if (cmd == GATE_CMD_OPEN) {
    manualCloseRequested = false;
    if (!gateOpen) setGate(true);
  } else {
    manualCloseRequested = gateOpen;
  }
}

void updateGate() {
  if (!manualMode) manualCloseRequested = false;  // kembali ke Auto: logika otomatis yang menutup
  handleGateCommand();

  if (updateInput(beamInput, readBeamRaw())) {
    if (beamInput.stable) {
      onBeamBroken();
    } else {
      beamClearedAt = millis();
    }
  }

  // Tutup otomatis (hanya mode Auto). Pengaman: beam harus bersih cukup lama.
  if (!manualMode && gateOpen && !beamInput.stable &&
      millis() - beamClearedAt >= GATE_CLOSE_DELAY_MS) {
    setGate(false);
  }

  // Tutup manual. Pengaman: ditunda selama beam terhalang.
  if (manualCloseRequested && !beamInput.stable) {
    manualCloseRequested = false;
    setGate(false);
  }
}

// ---------- Tampilan ----------
char lcdLines[2][17] = {"", ""};

void lcdPrintLine(uint8_t row, const char *text) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", text);  // pad spasi agar sisa teks lama terhapus
  if (strcmp(buf, lcdLines[row]) == 0) return;  // tulis hanya saat berubah, agar tidak berkedip
  strcpy(lcdLines[row], buf);
  lcd.setCursor(0, row);
  lcd.print(buf);
}

const char *gateMessage() {
  unsigned long now = millis();
  if (manualMode) return manualCloseRequested ? "Hati-hati" : "Mode: MANUAL";
  if (gateOpen) {
    if (beamInput.stable && now - gateOpenedAt >= WELCOME_MSG_MS) return "Hati-hati";
    return "Silakan Masuk";
  }
  bool denying = deniedAt != 0 &&
                 (beamInput.stable || now - beamClearedAt < FULL_MSG_MS);
  if (denying) return "Parkir Penuh";
  return "";
}

void updateDisplay() {
  bool occupied = slotInput.stable;
  digitalWrite(PIN_LED_GREEN, occupied ? LOW : HIGH);
  digitalWrite(PIN_LED_RED, occupied ? HIGH : LOW);
  lcdPrintLine(0, occupied ? "Slot: PENUH" : "Slot: KOSONG");
  lcdPrintLine(1, gateMessage());
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_SLOT_SENSOR, INPUT);
  pinMode(PIN_BEAM, INPUT_PULLUP);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  lcd.init();
  lcd.backlight();

  gateServo.setPeriodHertz(50);
  gateServo.attach(PIN_SERVO, 500, 2400);
  gateServo.write(SERVO_CLOSED_DEG);

  // Status awal langsung dari sensor, tanpa menunggu debounce.
  initInput(slotInput, readSlotRaw());
  initInput(beamInput, readBeamRaw());
  beamClearedAt = millis();
  sharedSlotOccupied = slotInput.stable;
  Serial.printf("Slot awal: %s\n", slotInput.stable ? "TERISI" : "KOSONG");

  startNetwork();
}

void loop() {
  if (updateInput(slotInput, readSlotRaw())) {
    sharedSlotOccupied = slotInput.stable;
    Serial.printf("Slot: %s\n", slotInput.stable ? "TERISI" : "KOSONG");
  }
  updateGate();
  updateBuzzer();
  updateDisplay();
}
