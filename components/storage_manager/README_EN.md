# Storage Manager Component (NVS with CRC32 Verification)

## 1. Overview
The `storage_manager` component provides fault-tolerant persistence for the Posture Monitor firmware. It stores calibration zero-reference angles (`pitch_offset`, `roll_offset`) and threshold configurations into ESP32-C3 Non-Volatile Storage (NVS).

## 2. Technical Features
- **Deterministic CRC32 Checksum**: Guarantees detection of incomplete writes caused by sudden power cuts or battery exhaustion.
- **Magic Word Validation**: Validates schema version (`0x504F5354` - "POST") to prevent loading uninitialized memory.
- **Automatic Factory Reset Fallback**: Restores deterministic default values if flash corruption or invalid size is detected.

## 3. Data Structure Definition
```c
typedef struct {
    uint32_t magic;               // 0x504F5354 ("POST")
    float    pitch_offset;        // Baseline pitch offset (degrees)
    float    roll_offset;         // Baseline roll offset (degrees)
    float    angle_threshold;     // Tolerance angle (degrees)
    uint32_t slouch_delay_s;      // Grace period before Level 1 alert
    uint32_t escalation_delay_s;  // Delay before Level 2 buzzer alert
    uint32_t crc32;               // IEEE 802.3 CRC32 checksum
} __attribute__((packed)) posture_calib_data_t;
```

## 4. API Interface
- `esp_err_t storage_manager_init(void)`: Initializes the default NVS partition.
- `esp_err_t storage_manager_load_calibration(posture_calib_data_t *out_data)`: Loads and verifies calibration from NVS.
- `esp_err_t storage_manager_save_calibration(const posture_calib_data_t *in_data)`: Calculates CRC32 and commits to NVS.
- `esp_err_t storage_manager_reset_to_default(posture_calib_data_t *out_data)`: Writes default parameters to NVS.

## 5. Memory Footprint & Performance
- **RAM**: No heap allocation; operations are stack-bounded (~64 bytes).
- **Flash**: Consumes 1 NVS blob under namespace `posture_cfg` with key `calib_blob`.
