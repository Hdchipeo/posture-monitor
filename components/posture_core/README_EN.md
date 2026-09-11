# Posture Core Component

## 1. Overview
The `posture_core` component hosts the decision-making engine and Finite State Machine (FSM) for classifying spine posture and escalating warnings.

## 2. Finite State Machine (FSM)
- `POSTURE_STATE_CALIBRATING`: Collecting zero-offset average angles.
- `POSTURE_STATE_GOOD`: Angular error $|\Delta\theta| \le \theta_{\text{threshold}}$.
- `POSTURE_STATE_SUSPECTED_SLOUCH`: $|\Delta\theta| > \theta_{\text{threshold}}$ during the grace window ($t < T_{\text{slouch}}$). Prevents false positives when reaching for items or stretching.
- `POSTURE_STATE_ALERT_L1`: Grace window expired ($t \ge T_{\text{slouch}}$). Triggers intermittent vibration feedback.
- `POSTURE_STATE_ALERT_L2`: Persistent slouching ($t \ge T_{\text{slouch}} + T_{\text{escalation}}$). Triggers audible buzzer alarm.
- `POSTURE_STATE_SNOOZED`: Temporary muting upon button double-click.

## 3. Mathematical Model
$$\Delta \text{Pitch} = |\text{Pitch}_{\text{current}} - \text{Pitch}_{\text{offset}}|$$

$$\Delta \text{Roll} = |\text{Roll}_{\text{current}} - \text{Roll}_{\text{offset}}|$$

$$\text{Slouch Condition} \iff (\Delta \text{Pitch} > \theta_{\text{thresh}}) \lor (\Delta \text{Roll} > \theta_{\text{thresh}})$$

## 4. API Reference
- `esp_err_t posture_core_init(const posture_calib_data_t *initial_calib)`: Sets initial offsets and thresholds.
- `void posture_core_process_sample(float dt, const posture_angles_t *current_angles, posture_fsm_state_t *out_state)`: Updates state machine based on elapsed time and angles.
- `void posture_core_start_calibration(void)`: Starts zero-reference averaging.
- `esp_err_t posture_core_add_calibration_sample(const posture_angles_t *angles, posture_calib_data_t *out_calib)`: Feeds sample into calibration averager.
- `void posture_core_snooze(uint32_t seconds)`: Pauses alerts for designated interval.
