# Button Control Component

## 1. Overview
The `button_ctrl` component wraps the official `espressif/button` component from the ESP Component Registry. It handles hardware contact debouncing and dispatches discrete user actions for posture recalibration and alert snoozing.

## 2. Supported User Gestures
- **Long Press (> 2.0 seconds)**: Dispatches `BUTTON_EVENT_TARE_CALIBRATE` to sample and store a new neutral posture zero-offset into NVS.
- **Double Click**: Dispatches `BUTTON_EVENT_SNOOZE_10MIN` to temporarily mute all alerts while resting or stretching.
- **Single Click**: Dispatches `BUTTON_EVENT_SINGLE_CLICK` to immediately silence an ongoing alarm.

## 3. Configuration Options (Kconfig)
- `CONFIG_POSTURE_BUTTON_GPIO`: Physical pin number for the push button (default: GPIO 9, active-low with internal pull-up).

## 4. API Reference
- `esp_err_t button_ctrl_init(button_event_cb_t callback, void *user_data)`: Initializes the debounced GPIO button driver and registers event callbacks.
