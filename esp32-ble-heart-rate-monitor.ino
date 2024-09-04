#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLECharacteristic.h>

#define HEART_RATE_SERVICE_UUID "0000180D-0000-1000-8000-00805F9B34FB"
#define HEART_RATE_CHARACTERISTIC_UUID "00002A37-0000-1000-8000-00805F9B34FB"

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Client connected");
  };

  void onDisconnect(NimBLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Client disconnected");
    NimBLEDevice::startAdvertising();
  }
};


#define ANALOG_PIN 36
#define LO_PLUS 2
#define LO_MINUS 15

const int SAMPLE_RATE = 250;                 // Sample rate in Hz
const int FILTER_SIZE = 5;                   // Size for moving average filter
const int WINDOW_SIZE = 150;                 // 600ms at 250Hz
const float HIGH_PASS_ALPHA = 0.99;          // Alpha for high-pass filter
const int BASELINE_CORRECTION_WINDOW = 100;  // Window size for baseline correction

// Signal buffers for filtering and processing
int samples[WINDOW_SIZE];
int filtered[WINDOW_SIZE];
int smoothed[WINDOW_SIZE];           // For the moving average filter
int baselineCorrected[WINDOW_SIZE];  // After baseline correction
int windowIndex = 0;
float maxSignal = 0;  // For adaptive scaling

// BPM Detection Variables
unsigned long lastPeakTime = 0;
unsigned long currentPeakTime = 0;
int bpm = 0;
const int BPM_SAMPLE_SIZE = 10;  // Buffer size for smoothing
int bpmArray[BPM_SAMPLE_SIZE];
int bpmIndex = 0;

// High-pass filter state
float prevInput = 0;
float prevFilteredOutput = 0;

// Baseline correction variables
float baselineSum = 0;
int baselineWindow[BASELINE_CORRECTION_WINDOW];
int baselineIndex = 0;


// Function to send the BPM over BLE
void sendBPMOverBLE(int bpm) {
  uint8_t hrmData[2];
  hrmData[0] = 0x06;        // Flags (Heart Rate Value Format set to uint8)
  hrmData[1] = bpm & 0xFF;  // BPM value (low byte)

  // Notify connected device with the BPM value
  heartRateChar->setValue(hrmData, 2);
  heartRateChar->notify();
}


void setup() {
  Serial.begin(115200);
  pinMode(LO_PLUS, INPUT);
  pinMode(LO_MINUS, INPUT);
  pinMode(ANALOG_PIN, INPUT);

  // Initialize baseline window to zero
  for (int i = 0; i < BASELINE_CORRECTION_WINDOW; i++) {
    baselineWindow[i] = 0;
  }

  // Initialize NimBLE
  NimBLEDevice::init("ESP32 BLE HR Monitor");

  // Create the BLE Server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create the BLE Service
  NimBLEService* pService = pServer->createService(HEART_RATE_SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    HEART_RATE_CHARACTERISTIC_UUID,
    NIMBLE_PROPERTY::NOTIFY);

  // Start the service
  pService->start();

  // Start advertising
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(HEART_RATE_SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  NimBLEDevice::startAdvertising();
  Serial.println("Waiting for a client connection to notify...");
}



void loop() {
  static unsigned long lastSample = 0;

  if (millis() - lastSample >= 1000 / SAMPLE_RATE) {
    lastSample = millis();

    if (digitalRead(LO_PLUS) == 1 || digitalRead(LO_MINUS) == 1) {
      Serial.println("Lead Off!");
    } else {
      int reading = analogRead(ANALOG_PIN);  // Read the raw ECG data
      processSignal(reading);                // Process and filter the raw data

      // Detect R-peaks and calculate BPM
      detectBPM();

      if (deviceConnected) {
        sendBPMOverBLE(bpm);
      }

      // Print the corrected signal and BPM for plotting (adaptive scaling)
      printForPlotting();
    }
  }
}

// Process and filter the signal
void processSignal(int newSample) {
  samples[windowIndex] = newSample;

  // Step 1: Apply High-Pass Filtering to remove baseline drift
  filtered[windowIndex] = highPassFilter(samples[windowIndex]);

  // Step 2: Smoothing (Moving Average Filter)
  smoothed[windowIndex] = movingAverageFilter(filtered, windowIndex);

  // Step 3: Baseline Correction
  baselineCorrected[windowIndex] = baselineCorrection(smoothed[windowIndex]);

  // Track max amplitude for adaptive scaling
  if (abs(baselineCorrected[windowIndex]) > maxSignal) {
    maxSignal = abs(baselineCorrected[windowIndex]);
  }

  windowIndex = (windowIndex + 1) % WINDOW_SIZE;
}

// High-Pass Filter to remove baseline wander
int highPassFilter(int input) {
  float filteredValue = HIGH_PASS_ALPHA * (prevFilteredOutput + input - prevInput);
  prevInput = input;
  prevFilteredOutput = filteredValue;
  return filteredValue;
}

// Moving Average Filter for smoothing
int movingAverageFilter(int* data, int index) {
  long sum = 0;
  for (int i = 0; i < FILTER_SIZE; i++) {
    sum += data[(index - i + WINDOW_SIZE) % WINDOW_SIZE];
  }
  return sum / FILTER_SIZE;
}

// Baseline correction: subtract the running average from the signal
int baselineCorrection(int signal) {
  baselineSum -= baselineWindow[baselineIndex];  // Subtract oldest value
  baselineWindow[baselineIndex] = signal;        // Add new value
  baselineSum += signal;                         // Update sum
  baselineIndex = (baselineIndex + 1) % BASELINE_CORRECTION_WINDOW;
  int baselineAverage = baselineSum / BASELINE_CORRECTION_WINDOW;
  return signal - baselineAverage;
}

// Adaptive scaling and print for plotting in CSV format
void printForPlotting() {
  int scaledSignal = map(baselineCorrected[windowIndex], -maxSignal, maxSignal, -500, 500);

  // Print as CSV for Serial Plotter
  Serial.print(samples[windowIndex]);  // Raw signal
  Serial.print("\t");
  Serial.print(filtered[windowIndex]);  // High-pass filtered signal
  Serial.print("\t");
  Serial.print(smoothed[windowIndex]);  // Smoothed signal
  Serial.print("\t");
  Serial.print(scaledSignal);  // Baseline corrected and scaled signal
  Serial.print("\t");
  Serial.println(bpm);  // BPM
}

void addBPMSample(int newBPM) {
  bpmArray[bpmIndex] = newBPM;
  bpmIndex = (bpmIndex + 1) % BPM_SAMPLE_SIZE;
}

int calculateAverageBPM() {
  long sum = 0;
  for (int i = 0; i < BPM_SAMPLE_SIZE; i++) {
    sum += bpmArray[i];
  }
  return sum / BPM_SAMPLE_SIZE;
}

// Detect R-peaks and calculate BPM
void detectBPM() {
  int threshold = 400;                          // Set an appropriate threshold for R-peak detection (adjust as needed)
  int signal = baselineCorrected[windowIndex];  // Use baseline-corrected signal

  // Detect if the signal crosses the threshold
  if (signal > threshold) {
    currentPeakTime = millis();  // Get the time of the current peak

    if (lastPeakTime != 0) {
      unsigned long rrInterval = currentPeakTime - lastPeakTime;  // R-R interval in ms
      if (rrInterval > 300 && rrInterval < 1500) {                // Filter out false positives (min 40 BPM, max 200 BPM)
        bpm = 60000 / rrInterval;                                 // Calculate BPM
        addBPMSample(bpm);                                        // Add to the smoothing buffer
        bpm = calculateAverageBPM();                              // Get the smoothed BPM value
      }
    }

    lastPeakTime = currentPeakTime;  // Update last peak time
  }
}