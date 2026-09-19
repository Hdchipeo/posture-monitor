/**
 * @file app.js
 * @brief Mobile-first Apple Health Posture Monitor Application Controller.
 * Handles WebSocket 10Hz telemetry, REST API interactions, 2D mannequin kinematics,
 * calibration modal flow, settings management, and standalone demo simulation.
 */

(function () {
  'use strict';

  // =========================================================================
  // INTERNATIONALIZATION (i18n) — VIETNAMESE / ENGLISH DICTIONARY
  // =========================================================================
  const I18N = {
    en: {
      conn_connecting: 'Connecting...',
      conn_connected_ws: 'ESP32 Live',
      conn_connected_http: 'ESP32 (HTTP)',
      conn_offline: 'Offline',
      conn_demo: 'Demo Mode',
      sub_wifi: 'Connected via Wi-Fi',
      sub_polling: 'Connected via REST Polling',
      sub_sim: 'Synthetic IMU Engine Active',
      sub_retry: 'Disconnected — Retrying...',
      demo_mode: 'Demo',
      nav_posture: 'Posture',
      sensor_alert: 'MPU6050 Disconnected! Check I2C wiring: SDA=GPIO 8, SCL=GPIO 9, 3.3V',
      score_label: 'POSTURE SCORE',
      btn_snooze: 'Snooze 10m',
      snoozed_active: 'Snoozed',
      btn_calibrate: 'Tare Calibrate',
      summary_title: "Today's Summary",
      monitored: 'monitored',
      good_posture: 'Good Posture',
      avg_deviation: 'Avg. Deviation',
      target_dev: 'Target < 15.0°',
      alerts: 'Alerts',
      escalations: 'Escalations:',
      longest_streak: 'Longest Streak',
      without_slouching: 'Without slouching',
      live_orientation: 'Live Orientation',
      roll_label: 'Roll (Slouch)',
      pitch_label: 'Pitch (Tilt)',
      yaw_label: 'Yaw (Twist)',
      total_deviation: 'Total Deviation',
      threshold: 'Threshold',
      chart_title: 'Real-Time Deviation',
      chart_subtitle: 'Last 30 seconds of posture activity',
      legend_dev: 'Deviation',
      legend_thresh: 'Threshold',
      eyebrow_analytics: 'Analytics',
      nav_history: 'History',
      hourly_quality: 'Hourly Posture Quality',
      session_tag: '8h Session',
      hourly_subtitle: 'Timeline breakdown throughout the day',
      key_good: '>85% Good',
      key_slouch: 'Slouching',
      key_alert: 'Alert Active',
      breakdown_title: 'Session Breakdown',
      suspected_slouch: 'Suspected Slouch',
      alert_state: 'Alert State',
      event_timeline: 'Event Timeline',
      export_csv: 'Export CSV',
      eyebrow_system: 'Hardware & System',
      nav_device: 'Device',
      connected_wifi: 'Connected via Wi-Fi',
      spec_battery: 'Battery',
      spec_wifi: 'Wi-Fi RSSI',
      spec_uptime: 'Uptime',
      spec_heap: 'Free Heap',
      spec_sensor: 'Sensor',
      spec_rate: 'Stream Rate',
      baseline_title: 'Neutral Posture Baseline',
      group_thresholds: 'POSTURE THRESHOLDS',
      setting_dev_title: 'Deviation Threshold',
      setting_dev_desc: 'Trigger angle deviation limit',
      setting_slouch_title: 'Slouch Grace Period',
      setting_slouch_desc: 'Delay before Level 1 haptic warning',
      setting_esc_title: 'Escalation Delay',
      setting_esc_desc: 'Delay before Level 2 buzzer alarm',
      unit_sec: 'sec',
      group_feedback: 'ALERT FEEDBACK',
      setting_vib_title: 'Vibration Motor',
      setting_vib_desc: 'Tactile haptic pulses on posture error',
      setting_buz_title: 'Audible Buzzer',
      setting_buz_desc: 'Chirps on persistent slouch (L2)',
      btn_save: 'Save to Device',
      btn_reset: 'Reset Factory Defaults',
      calib_title: 'Tare Calibration',
      calib_desc1: 'Sit upright in your natural, comfortable posture with shoulders relaxed and chin level.',
      calib_step1: 'Align your back against your seat.',
      calib_step2: 'Stay still for 3 seconds while sampling.',
      btn_start_calib: 'Start Calibration',
      calib_sampling_title: 'Sampling Baseline...',
      calib_sampling_desc: 'Keep completely still. Recording 100 IMU readings.',
      btn_cancel: 'Cancel',
      calib_complete_title: 'Calibration Complete',
      calib_complete_desc: 'Your neutral zero reference has been persisted to NVS flash memory.',
      calib_roll_base: 'Roll Baseline (Slouch)',
      calib_pitch_base: 'Pitch Baseline (Tilt)',
      calib_samples_avg: 'Samples Averaged',
      calib_samples_count: '100 samples',
      btn_done: 'Done',
      nav_today: 'Today',
      // Dynamic states
      state_good: 'GOOD',
      sub_good: "You're sitting upright",
      state_slouch: 'SLOUCHING',
      sub_slouch: 'Correct your posture',
      state_alert_l1: 'WARNING',
      sub_alert_l1: 'Mild haptic alert active',
      state_alert_l2: 'ALERT',
      sub_alert_l2: 'Posture alarm active — straighten up',
      state_snoozed: 'SNOOZED',
      sub_snoozed: 'Alerts temporarily silenced',
      state_calib: 'CALIBRATING',
      sub_calib: 'Hold still in neutral posture',
      state_sensor_offline: 'SENSOR OFFLINE',
      sub_sensor_offline: 'Check MPU6050 wiring (SDA 8, SCL 9, 3.3V)',
      // Silhouette captions
      caption_aligned: 'Spine angle aligned',
      caption_slouch: 'Forward lean detected',
      caption_alert: 'Excessive curvature — Alert',
      caption_paused: 'Monitoring paused',
      // Toasts
      toast_connected: 'Connected to Posture Monitor',
      toast_demo: 'Demo simulation engine active',
      toast_snoozed: 'Alerts snoozed for 10 minutes',
      toast_snoozed_demo: 'Alerts snoozed for 10 minutes (Demo)',
      toast_snoozed_local: 'Snoozed locally',
      toast_calib_done: 'Baseline calibrated and stored to NVS',
      toast_config_saved: 'Settings successfully saved to NVS',
      toast_config_demo: 'Settings saved to local session (Demo)',
      toast_config_err: 'Error saving settings to device',
      toast_config_reset: 'Reset to factory defaults',
      toast_csv_exported: 'History exported as CSV',
      // Timeline default events
      evt_session_start: 'Session Started',
      evt_session_start_desc: 'Baseline calibrated (Pitch: +1.2°, Roll: -0.4°)',
      evt_slouch_l1: 'Slouch Alert L1',
      evt_slouch_l1_desc: 'Persistent deviation 17.4° for 5.2s',
      evt_corrected: 'Posture Corrected',
      evt_corrected_desc: 'Returned to neutral spine alignment',
      evt_slouch_l2: 'Slouch Alert L2',
      evt_slouch_l2_desc: 'Slouch sustained >20s (Pitch: 22.1°)',
      evt_calib: 'Tare Calibrated',
      evt_calib_desc: 'Zero reference re-centered',
      // Firmware OTA
      group_ota: 'FIRMWARE UPDATE (OTA)',
      ota_title: 'Wireless Firmware Update',
      ota_desc: 'Flash new posture-monitor.bin directly via Wi-Fi',
      ota_select_file: 'Choose .bin file',
      ota_or_drag: 'or drag & drop here',
      btn_start_ota: 'Upload & Flash Firmware',
      ota_flashing_title: 'Flashing Firmware...',
      ota_flashing_desc: 'Writing new binary to passive flash partition. Do not power off or disconnect Wi-Fi.',
      ota_rebooting_label: 'Seconds until device reconnect...',
      toast_ota_select_bin: 'Please select a valid .bin firmware file',
      toast_ota_started: 'OTA firmware upload started...',
      toast_ota_complete: 'Firmware flashed successfully! Rebooting...',
      toast_ota_err: 'OTA Flash failed: ',
      ota_current_ver: 'Active Firmware'
    },
    vi: {
      conn_connecting: 'Đang kết nối...',
      conn_connected_ws: 'ESP32 Trực tiếp',
      conn_connected_http: 'ESP32 (HTTP)',
      conn_offline: 'Ngoại tuyến',
      conn_demo: 'Chế độ Demo',
      sub_wifi: 'Kết nối qua Wi-Fi',
      sub_polling: 'Kết nối qua REST Polling',
      sub_sim: 'Mô phỏng IMU giả lập',
      sub_retry: 'Mất kết nối — Đang thử lại...',
      demo_mode: 'Demo',
      nav_posture: 'Tư thế',
      sensor_alert: 'Mất kết nối MPU6050! Kiểm tra dây I2C: SDA=GPIO 8, SCL=GPIO 9, 3.3V',
      score_label: 'ĐIỂM TƯ THẾ',
      btn_snooze: 'Tạm hoãn 10p',
      snoozed_active: 'Đã tạm hoãn',
      btn_calibrate: 'Hiệu chuẩn 0',
      summary_title: 'Tổng quan hôm nay',
      monitored: 'đã theo dõi',
      good_posture: 'Tư thế tốt',
      avg_deviation: 'Độ lệch TB',
      target_dev: 'Mục tiêu < 15.0°',
      alerts: 'Cảnh báo',
      escalations: 'Báo động L2:',
      longest_streak: 'Chuỗi tốt nhất',
      without_slouching: 'Không gù lưng',
      live_orientation: 'Góc nghiêng thời gian thực',
      roll_label: 'Cúi/Ngửa (Roll)',
      pitch_label: 'Nghiêng (Pitch)',
      yaw_label: 'Xoay (Yaw)',
      total_deviation: 'Tổng độ lệch',
      threshold: 'Ngưỡng giới hạn',
      chart_title: 'Độ lệch thời gian thực',
      chart_subtitle: '30 giây hoạt động gần nhất',
      legend_dev: 'Độ lệch',
      legend_thresh: 'Ngưỡng',
      eyebrow_analytics: 'Phân tích',
      nav_history: 'Lịch sử',
      hourly_quality: 'Chất lượng tư thế theo giờ',
      session_tag: 'Phiên 8 giờ',
      hourly_subtitle: 'Chi tiết phân bổ trong ngày',
      key_good: '>85% Tốt',
      key_slouch: 'Gù lưng',
      key_alert: 'Có cảnh báo',
      breakdown_title: 'Tỷ lệ thời gian',
      suspected_slouch: 'Nghi vấn gù lưng',
      alert_state: 'Trạng thái cảnh báo',
      event_timeline: 'Dòng sự kiện',
      export_csv: 'Xuất CSV',
      eyebrow_system: 'Phần cứng & Hệ thống',
      nav_device: 'Thiết bị',
      connected_wifi: 'Kết nối qua Wi-Fi',
      spec_battery: 'Pin',
      spec_wifi: 'Sóng Wi-Fi',
      spec_uptime: 'Thời gian chạy',
      spec_heap: 'RAM trống',
      spec_sensor: 'Cảm biến',
      spec_rate: 'Tần số gửi',
      baseline_title: 'Gốc tư thế chuẩn (Tare)',
      group_thresholds: 'NGƯỠNG TƯ THẾ',
      setting_dev_title: 'Ngưỡng độ lệch',
      setting_dev_desc: 'Giới hạn góc kích hoạt cảnh báo',
      setting_slouch_title: 'Thời gian chờ gù lưng',
      setting_slouch_desc: 'Thời gian trễ trước khi rung (Mức 1)',
      setting_esc_title: 'Thời gian leo thang',
      setting_esc_desc: 'Thời gian trễ trước khi kêu còi (Mức 2)',
      unit_sec: 'giây',
      group_feedback: 'PHẢN HỒI CẢNH BÁO',
      setting_vib_title: 'Động cơ rung',
      setting_vib_desc: 'Rung xúc giác khi sai tư thế',
      setting_buz_title: 'Còi báo động',
      setting_buz_desc: 'Kêu bíp khi gù lưng kéo dài (Mức 2)',
      btn_save: 'Lưu vào thiết bị',
      btn_reset: 'Khôi phục mặc định',
      calib_title: 'Hiệu chuẩn tư thế chuẩn',
      calib_desc1: 'Ngồi thẳng lưng tự nhiên, thả lỏng vai và giữ mắt nhìn thẳng.',
      calib_step1: 'Tựa lưng thẳng vào ghế ngồi.',
      calib_step2: 'Giữ nguyên tư thế trong 3 giây để lấy mẫu.',
      btn_start_calib: 'Bắt đầu hiệu chuẩn',
      calib_sampling_title: 'Đang đo đạc tư thế chuẩn...',
      calib_sampling_desc: 'Giữ bất động. Đang ghi 100 mẫu cảm biến IMU.',
      btn_cancel: 'Hủy',
      calib_complete_title: 'Hiệu chuẩn thành công',
      calib_complete_desc: 'Tư thế chuẩn đã được lưu vào bộ nhớ flash NVS.',
      calib_roll_base: 'Roll chuẩn (Cúi/Ngửa)',
      calib_pitch_base: 'Pitch chuẩn (Nghiêng)',
      calib_samples_avg: 'Số mẫu trung bình',
      calib_samples_count: '100 mẫu',
      btn_done: 'Hoàn tất',
      nav_today: 'Hôm nay',
      // Dynamic states
      state_good: 'TỐT',
      sub_good: 'Bạn đang ngồi thẳng lưng',
      state_slouch: 'GÙ LƯNG',
      sub_slouch: 'Hãy điều chỉnh lại tư thế',
      state_alert_l1: 'CẢNH BÁO',
      sub_alert_l1: 'Rung cảnh báo mức 1 đang bật',
      state_alert_l2: 'BÁO ĐỘNG',
      sub_alert_l2: 'Còi báo động — hãy ngồi thẳng',
      state_snoozed: 'TẠM HOÃN',
      sub_snoozed: 'Đã tạm dừng thông báo',
      state_calib: 'HIỆU CHUẨN',
      sub_calib: 'Giữ yên ở tư thế chuẩn',
      state_sensor_offline: 'MẤT CẢM BIẾN',
      sub_sensor_offline: 'Kiểm tra dây MPU6050 (SDA 8, SCL 9, 3.3V)',
      // Silhouette captions
      caption_aligned: 'Cột sống thẳng hàng',
      caption_slouch: 'Phát hiện cúi gập người',
      caption_alert: 'Độ cong quá mức — Cảnh báo',
      caption_paused: 'Tạm dừng theo dõi',
      // Toasts
      toast_connected: 'Đã kết nối với Posture Monitor',
      toast_demo: 'Đã kích hoạt chế độ giả lập Demo',
      toast_snoozed: 'Đã tạm hoãn cảnh báo 10 phút',
      toast_snoozed_demo: 'Đã tạm hoãn cảnh báo 10 phút (Demo)',
      toast_snoozed_local: 'Đã tạm hoãn trên trình duyệt',
      toast_calib_done: 'Đã lưu tư thế chuẩn vào NVS',
      toast_config_saved: 'Cấu hình đã được lưu vào NVS',
      toast_config_demo: 'Đã lưu cài đặt tạm thời (Demo)',
      toast_config_err: 'Lỗi khi lưu cài đặt vào thiết bị',
      toast_config_reset: 'Đã khôi phục cài đặt gốc',
      toast_csv_exported: 'Đã xuất lịch sử dưới dạng CSV',
      // Timeline default events
      evt_session_start: 'Bắt đầu phiên theo dõi',
      evt_session_start_desc: 'Đã hiệu chuẩn mốc chuẩn (Pitch: +1.2°, Roll: -0.4°)',
      evt_slouch_l1: 'Cảnh báo gù lưng Mức 1',
      evt_slouch_l1_desc: 'Độ lệch kéo dài 17.4° trong 5.2 giây',
      evt_corrected: 'Đã chỉnh lại tư thế',
      evt_corrected_desc: 'Trở về trạng thái cột sống chuẩn',
      evt_slouch_l2: 'Báo động gù lưng Mức 2',
      evt_slouch_l2_desc: 'Gù lưng liên tục >20 giây (Pitch: 22.1°)',
      evt_calib: 'Hiệu chuẩn Tare',
      evt_calib_desc: 'Đã đặt lại gốc tọa độ mốc 0',
      // Firmware OTA
      group_ota: 'CẬP NHẬT FIRMWARE (OTA)',
      ota_title: 'Cập nhật Firmware không dây',
      ota_desc: 'Nạp file posture-monitor.bin trực tiếp qua Wi-Fi',
      ota_select_file: 'Chọn file .bin',
      ota_or_drag: 'hoặc kéo thả vào đây',
      btn_start_ota: 'Tải lên & Nạp Firmware',
      ota_flashing_title: 'Đang nạp Firmware...',
      ota_flashing_desc: 'Đang ghi firmware mới vào bộ nhớ flash. Tuyệt đối không ngắt nguồn hoặc Wi-Fi.',
      ota_rebooting_label: 'Giây nữa thiết bị sẽ khởi động lại...',
      toast_ota_select_bin: 'Vui lòng chọn file firmware .bin hợp lệ',
      toast_ota_started: 'Bắt đầu nạp firmware OTA...',
      toast_ota_complete: 'Nạp firmware thành công! Đang khởi động lại...',
      toast_ota_err: 'Lỗi nạp OTA: ',
      ota_current_ver: 'Phiên bản đang chạy'
    }
  };

  // Language state persistence ("STATE IS KING")
  let currentLang = 'vi';
  try {
    const saved = localStorage.getItem('posture_lang');
    if (saved === 'en' || saved === 'vi') {
      currentLang = saved;
    } else if (navigator.language && navigator.language.startsWith('en')) {
      currentLang = 'en';
    }
  } catch (e) {
    currentLang = 'vi';
  }

  function i18n(key) {
    if (I18N[currentLang] && I18N[currentLang][key]) {
      return I18N[currentLang][key];
    }
    if (I18N['en'] && I18N['en'][key]) {
      return I18N['en'][key];
    }
    return key;
  }

  // State visual mapping (styling and CSS hooks)
  const STATE_CONFIG = {
    'GOOD': {
      titleKey: 'state_good',
      subKey: 'sub_good',
      cssClass: 'state-good',
      dotColor: '#34C759'
    },
    'SUSPECTED_SLOUCH': {
      titleKey: 'state_slouch',
      subKey: 'sub_slouch',
      cssClass: 'state-slouch',
      dotColor: '#FF9500'
    },
    'ALERT_L1': {
      titleKey: 'state_alert_l1',
      subKey: 'sub_alert_l1',
      cssClass: 'state-alert',
      dotColor: '#FF3B30'
    },
    'ALERT_L2': {
      titleKey: 'state_alert_l2',
      subKey: 'sub_alert_l2',
      cssClass: 'state-alert',
      dotColor: '#FF3B30'
    },
    'SNOOZED': {
      titleKey: 'state_snoozed',
      subKey: 'sub_snoozed',
      cssClass: 'state-snooze',
      dotColor: '#86868B'
    },
    'CALIBRATING': {
      titleKey: 'state_calib',
      subKey: 'sub_calib',
      cssClass: 'state-calib',
      dotColor: '#007AFF'
    }
  };

  // Application Global State
  const app = {
    ws: null,
    wsConnected: false,
    connectionState: 'connecting', // 'connecting', 'connected_ws', 'connected_http', 'offline', 'demo'
    simMode: false,
    simInterval: null,
    chart: null,
    skeleton3d: null,
    activeTab: 'tab-today',
    telemetry: {
      state: 'GOOD',
      pitch: 2.1,
      roll: -0.8,
      yaw: 0.0,
      deviation: 2.2,
      threshold: 15.0,
      battery: 92,
      rssi: -52,
      free_heap: 184320,
      uptime: 9900,
      calibrated: true,
      snoozed: false,
      score: 95
    },
    historyEvents: [
      { time: '09:12:04', type: 'good', titleKey: 'evt_session_start', title: 'Session Started', descKey: 'evt_session_start_desc', desc: 'Baseline calibrated (Pitch: +1.2°, Roll: -0.4°)' },
      { time: '10:41:22', type: 'alert', titleKey: 'evt_slouch_l1', title: 'Slouch Alert L1', descKey: 'evt_slouch_l1_desc', desc: 'Persistent deviation 17.4° for 5.2s' },
      { time: '10:41:35', type: 'good', titleKey: 'evt_corrected', title: 'Posture Corrected', descKey: 'evt_corrected_desc', desc: 'Returned to neutral spine alignment' },
      { time: '13:20:18', type: 'alert', titleKey: 'evt_slouch_l2', title: 'Slouch Alert L2', descKey: 'evt_slouch_l2_desc', desc: 'Slouch sustained >20s (Pitch: 22.1°)' },
      { time: '13:20:45', type: 'good', titleKey: 'evt_corrected', title: 'Posture Corrected', descKey: 'evt_corrected_desc', desc: 'Returned to neutral alignment' },
      { time: '14:05:00', type: 'calib', titleKey: 'evt_calib', title: 'Tare Calibrated', descKey: 'evt_calib_desc', desc: 'Zero reference re-centered' }
    ]
  };

  // =========================================================================
  // =========================================================================
  // DOM ELEMENT REFERENCES
  // =========================================================================
  const dom = {
    // Banner & Connection & Language
    connPill: document.getElementById('connPill'),
    connDot: document.getElementById('connDot'),
    connText: document.getElementById('connText'),
    simToggle: document.getElementById('simToggle'),
    btnLangEn: document.getElementById('btnLangEn'),
    btnLangVi: document.getElementById('btnLangVi'),

    // Navigation & Tabs
    navItems: document.querySelectorAll('.nav-item'),
    tabPanes: document.querySelectorAll('.tab-pane'),

    // Today Tab
    todayDate: document.getElementById('todayDate'),
    heroCard: document.getElementById('heroCard'),
    stateDot: document.getElementById('stateDot'),
    stateTitle: document.getElementById('stateTitle'),
    stateSubtitle: document.getElementById('stateSubtitle'),
    scoreValue: document.getElementById('scoreValue'),
    quickSnoozeBtn: document.getElementById('quickSnoozeBtn'),
    snoozeBtnText: document.getElementById('snoozeBtnText'),
    openCalibModalBtn: document.getElementById('openCalibModalBtn'),

    // Summary Metrics
    sessionTimeTag: document.getElementById('sessionTimeTag'),
    goodPosturePct: document.getElementById('goodPosturePct'),
    goodPostureFill: document.getElementById('goodPostureFill'),
    avgDevVal: document.getElementById('avgDevVal'),
    alertCountVal: document.getElementById('alertCountVal'),
    l2CountVal: document.getElementById('l2CountVal'),
    longestStreakVal: document.getElementById('longestStreakVal'),

    // Live Orientation & 3D Mannequin Skeleton
    postureCanvas3D: document.getElementById('postureCanvas3D'),
    btnReset3D: document.getElementById('btnReset3D'),
    spineCurve: document.getElementById('spineCurve'),
    headCircle: document.getElementById('headCircle'),
    sensorTag: document.getElementById('sensorTag'),
    vectorCaption: document.getElementById('vectorCaption'),
    livePitch: document.getElementById('livePitch'),
    liveRoll: document.getElementById('liveRoll'),
    liveYaw: document.getElementById('liveYaw'),
    liveDeviation: document.getElementById('liveDeviation'),
    liveThreshold: document.getElementById('liveThreshold'),

    // History Tab
    hourlyHeatmap: document.getElementById('hourlyHeatmap'),
    histGoodPct: document.getElementById('histGoodPct'),
    histSlouchPct: document.getElementById('histSlouchPct'),
    histAlertPct: document.getElementById('histAlertPct'),
    timelineList: document.getElementById('timelineList'),
    exportCsvBtn: document.getElementById('exportCsvBtn'),

    // Device Tab
    devBattery: document.getElementById('devBattery'),
    devRssi: document.getElementById('devRssi'),
    devUptime: document.getElementById('devUptime'),
    devHeap: document.getElementById('devHeap'),
    devSensorStatus: document.getElementById('devSensorStatus'),
    deviceSubStatus: document.getElementById('deviceSubStatus'),
    currentBaselineDesc: document.getElementById('currentBaselineDesc'),
    recalibBtn: document.getElementById('recalibBtn'),
    threshSlider: document.getElementById('threshSlider'),
    threshValLabel: document.getElementById('threshValLabel'),
    slouchDelayInput: document.getElementById('slouchDelayInput'),
    escalationDelayInput: document.getElementById('escalationDelayInput'),
    vibrationToggle: document.getElementById('vibrationToggle'),
    buzzerToggle: document.getElementById('buzzerToggle'),
    saveConfigBtn: document.getElementById('saveConfigBtn'),
    resetConfigBtn: document.getElementById('resetConfigBtn'),

    // Sensor Alert Banner
    sensorAlertBanner: document.getElementById('sensorAlertBanner'),
    sensorAlertText: document.getElementById('sensorAlertText'),

    // Calibration Modal
    calibModal: document.getElementById('calibModal'),
    closeCalibModalBtn: document.getElementById('closeCalibModalBtn'),
    calibStage1: document.getElementById('calibStage1'),
    calibStage2: document.getElementById('calibStage2'),
    calibStage3: document.getElementById('calibStage3'),
    startCalibCycleBtn: document.getElementById('startCalibCycleBtn'),
    cancelCalibBtn: document.getElementById('cancelCalibBtn'),
    finishCalibBtn: document.getElementById('finishCalibBtn'),
    calibCountdown: document.getElementById('calibCountdown'),
    calibProgressBar: document.getElementById('calibProgressBar'),
    calibLivePitch: document.getElementById('calibLivePitch'),
    calibLiveRoll: document.getElementById('calibLiveRoll'),
    calibResultPitch: document.getElementById('calibResultPitch'),
    calibResultRoll: document.getElementById('calibResultRoll'),

    // Toast
    toastContainer: document.getElementById('toastContainer'),

    // OTA Firmware Update
    firmwareVerBadge: document.getElementById('firmwareVerBadge'),
    otaCurrentVerDesc: document.getElementById('otaCurrentVerDesc'),
    otaFileInput: document.getElementById('otaFileInput'),
    otaDropZone: document.getElementById('otaDropZone'),
    otaFileDetails: document.getElementById('otaFileDetails'),
    otaFileName: document.getElementById('otaFileName'),
    otaFileSize: document.getElementById('otaFileSize'),
    startOtaBtn: document.getElementById('startOtaBtn'),
    otaCurrentSlot: document.getElementById('otaCurrentSlot'),
    otaModal: document.getElementById('otaModal'),
    otaModalTitle: document.getElementById('otaModalTitle'),
    otaModalDesc: document.getElementById('otaModalDesc'),
    otaProgressBar: document.getElementById('otaProgressBar'),
    otaProgressPct: document.getElementById('otaProgressPct'),
    otaBytesSent: document.getElementById('otaBytesSent'),
    otaCountdownWrap: document.getElementById('otaCountdownWrap'),
    otaCountdownSec: document.getElementById('otaCountdownSec')
  };

  // =========================================================================
  // INITIALIZATION
  // =========================================================================
  function init() {
    if (dom.simToggle) dom.simToggle.checked = false;
    setupLanguage();
    setupNavigation();
    setupChart();
    setup3DSkeleton();
    setupEventListeners();
    renderHourlyHeatmap();
    renderTimeline();

    // Check if running directly on ESP32 or standalone file preview
    if (window.location.protocol === 'file:') {
      enableSimulationMode(true);
    } else {
      initWebSocket();
      startRestPolling();
      fetchConfig();
      fetchHistory();
    }
  }

  // =========================================================================
  // LANGUAGE CONTROLLER & REACTIVE TRANSLATION
  // =========================================================================
  function setupLanguage() {
    setLanguage(currentLang);
  }

  function setLanguage(lang) {
    if (lang !== 'en' && lang !== 'vi') lang = 'en';
    currentLang = lang;
    try {
      localStorage.setItem('posture_lang', lang);
    } catch (e) {}

    document.documentElement.lang = lang;

    // Update button active state
    if (dom.btnLangEn) dom.btnLangEn.classList.toggle('active', lang === 'en');
    if (dom.btnLangVi) dom.btnLangVi.classList.toggle('active', lang === 'vi');

    // Update all static data-i18n DOM elements
    document.querySelectorAll('[data-i18n]').forEach(el => {
      const key = el.getAttribute('data-i18n');
      if (I18N[lang] && I18N[lang][key]) {
        el.textContent = I18N[lang][key];
      }
    });

    // Update dynamic reactive UI
    updateDateHeader();
    updateConnectionDisplay();
    updateHeroCard(app.telemetry);
    updateVectorAndSilhouette(app.telemetry);
    updateDeviceSpecs(app.telemetry);
    renderTimeline();

    if (app.chart && typeof app.chart.setLanguage === 'function') {
      app.chart.setLanguage(lang);
    }
  }

  function updateDateHeader() {
    const now = new Date();
    const options = { weekday: 'long', month: 'long', day: 'numeric' };
    const locale = currentLang === 'vi' ? 'vi-VN' : 'en-US';
    dom.todayDate.textContent = now.toLocaleDateString(locale, options);
  }

  // Reactive Connection State Display
  function setConnectionState(state) {
    app.connectionState = state;
    updateConnectionDisplay();
  }

  function updateConnectionDisplay() {
    const s = app.connectionState;
    if (s === 'connecting') {
      dom.connDot.className = 'status-dot';
      dom.connText.textContent = i18n('conn_connecting');
      dom.deviceSubStatus.textContent = i18n('sub_wifi');
    } else if (s === 'connected_ws') {
      dom.connDot.className = 'status-dot connected';
      dom.connText.textContent = i18n('conn_connected_ws');
      dom.deviceSubStatus.textContent = i18n('sub_wifi');
    } else if (s === 'connected_http') {
      dom.connDot.className = 'status-dot connected';
      dom.connText.textContent = i18n('conn_connected_http');
      dom.deviceSubStatus.textContent = i18n('sub_polling');
    } else if (s === 'demo') {
      dom.connDot.className = 'status-dot simulated';
      dom.connText.textContent = i18n('conn_demo');
      dom.deviceSubStatus.textContent = i18n('sub_sim');
    } else if (s === 'offline') {
      dom.connDot.className = 'status-dot disconnected';
      dom.connText.textContent = i18n('conn_offline');
      dom.deviceSubStatus.textContent = i18n('sub_retry');
    }
  }

  // =========================================================================
  // NAVIGATION & TAB SWITCHING
  // =========================================================================
  function setupNavigation() {
    dom.navItems.forEach(item => {
      item.addEventListener('click', () => {
        const targetTabId = item.getAttribute('data-tab');
        if (targetTabId === app.activeTab) return;

        // Update nav items
        dom.navItems.forEach(n => n.classList.remove('active'));
        item.classList.add('active');

        // Update tab panes
        dom.tabPanes.forEach(pane => {
          if (pane.id === targetTabId) {
            pane.classList.add('active');
          } else {
            pane.classList.remove('active');
          }
        });

        app.activeTab = targetTabId;

        // If switching to Today tab, trigger chart and 3D canvas resize
        if (targetTabId === 'tab-today') {
          if (app.chart) setTimeout(() => app.chart.resize(), 50);
          if (app.skeleton3d) setTimeout(() => app.skeleton3d.resize(), 50);
        } else if (targetTabId === 'tab-history') {
          fetchHistory();
        } else if (targetTabId === 'tab-device') {
          fetchConfig();
        }

        // Scroll to top smoothly
        window.scrollTo({ top: 0, behavior: 'smooth' });
      });
    });
  }

  // =========================================================================
  // CHART INITIALIZATION
  // =========================================================================
  function setupChart() {
    try {
      if (window.PostureRealtimeChart) {
        app.chart = new window.PostureRealtimeChart('realtimeChart');
      }
    } catch (e) {
      console.warn('Realtime chart initialization bypassed:', e);
    }
  }

  // =========================================================================
  // 3D SKELETON INITIALIZATION (Three.js Lite)
  // =========================================================================
  function setup3DSkeleton() {
    try {
      if (window.Skeleton3D && dom.postureCanvas3D) {
        app.skeleton3d = new window.Skeleton3D(dom.postureCanvas3D);
      }
    } catch (e) {
      console.warn('Skeleton3D initialization bypassed:', e);
    }
  }

  // =========================================================================
  // REST POLLING FALLBACK (Robust Dual-Transport)
  // =========================================================================
  let pollTimer = null;

  function startRestPolling() {
    if (pollTimer || app.simMode) return;

    const pollOnce = () => {
      if (app.wsConnected || app.simMode) return;

      fetch('/api/status')
        .then(res => {
          if (!res.ok) throw new Error('HTTP error ' + res.status);
          return res.json();
        })
        .then(data => {
          setConnectionState('connected_http');
          handleTelemetryUpdate(data);
        })
        .catch(() => {
          if (!app.wsConnected) {
            setConnectionState('offline');
          }
        });
    };

    pollOnce();
    pollTimer = setInterval(pollOnce, 1000);
  }

  // =========================================================================
  // WEBSOCKET COMMUNICATION (ESP32-C3)
  // =========================================================================
  function initWebSocket() {
    if (app.simMode) return;

    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;

    setConnectionState('connecting');

    try {
      app.ws = new WebSocket(wsUrl);

      app.ws.onopen = () => {
        app.wsConnected = true;
        setConnectionState('connected_ws');
        showToast(i18n('toast_connected'));
      };

      app.ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          if (msg.type === 'telemetry' || msg.state) {
            handleTelemetryUpdate(msg);
          }
        } catch (e) {
          console.error('Error parsing WS message', e);
        }
      };

      app.ws.onerror = (err) => {
        console.warn('WebSocket error, falling back to REST polling');
        startRestPolling();
      };

      app.ws.onclose = () => {
        app.wsConnected = false;
        setConnectionState('offline');
        startRestPolling();

        // Auto-reconnect after 3 seconds if not in simulation mode
        if (!app.simMode) {
          setTimeout(initWebSocket, 3000);
        }
      };
    } catch (e) {
      console.warn('Failed to construct WebSocket, entering REST polling');
      startRestPolling();
    }
  }

  // =========================================================================
  // TELEMETRY UPDATE DISPATCHER (10 Hz)
  // =========================================================================
  function handleTelemetryUpdate(data) {
    const telem = app.telemetry;
    telem.state = data.state || telem.state;
    telem.pitch = typeof data.pitch === 'number' ? data.pitch : telem.pitch;
    telem.roll = typeof data.roll === 'number' ? data.roll : telem.roll;
    telem.yaw = typeof data.yaw === 'number' ? data.yaw : (telem.yaw || 0.0);
    telem.deviation = typeof data.deviation === 'number' ? data.deviation : Math.sqrt(telem.pitch * telem.pitch + telem.roll * telem.roll);
    telem.threshold = typeof data.threshold === 'number' ? data.threshold : telem.threshold;
    telem.battery = typeof data.battery === 'number' ? data.battery : telem.battery;
    telem.rssi = typeof data.rssi === 'number' ? data.rssi : telem.rssi;
    telem.free_heap = typeof data.free_heap === 'number' ? data.free_heap : telem.free_heap;
    telem.uptime = typeof data.uptime === 'number' ? data.uptime : telem.uptime + 1;
    telem.calibrated = data.calibrated !== undefined ? data.calibrated : telem.calibrated;
    telem.snoozed = data.snoozed !== undefined ? data.snoozed : telem.snoozed;
    telem.sensor_ok = data.sensor_ok !== undefined ? data.sensor_ok : true;

    // Handle sensor connection state
    if (!telem.sensor_ok) {
      if (dom.sensorAlertBanner) dom.sensorAlertBanner.style.display = 'flex';
      if (dom.devSensorStatus) {
        dom.devSensorStatus.textContent = 'MPU6050 (Error)';
        dom.devSensorStatus.style.color = '#FF3B30';
      }
    } else {
      if (dom.sensorAlertBanner) dom.sensorAlertBanner.style.display = 'none';
      if (dom.devSensorStatus) {
        dom.devSensorStatus.textContent = 'MPU6050 (50Hz)';
        dom.devSensorStatus.style.color = '';
      }
    }

    // Calculate dynamic posture score (100 - (dev / threshold)*50 clamp [0, 100])
    const scorePenalty = Math.min(100, (telem.deviation / telem.threshold) * 45);
    telem.score = telem.sensor_ok ? Math.max(10, Math.round(100 - scorePenalty)) : '--';

    // Update UI elements
    updateHeroCard(telem);
    updateVectorAndSilhouette(telem);
    updateAngleReadouts(telem);
    updateDeviceSpecs(telem);

    // Push into real-time canvas chart
    if (app.chart && telem.sensor_ok) {
      app.chart.pushSample(telem.deviation, telem.threshold);
    }
  }

  // Update Hero Card
  function updateHeroCard(telem) {
    if (!telem.sensor_ok) {
      dom.heroCard.className = 'card hero-card state-alert';
      dom.stateTitle.textContent = i18n('state_sensor_offline');
      dom.stateSubtitle.textContent = i18n('sub_sensor_offline');
      dom.scoreValue.textContent = '--';
      return;
    }

    const cfg = STATE_CONFIG[telem.state] || STATE_CONFIG['GOOD'];

    // Update classes
    dom.heroCard.className = `card hero-card ${cfg.cssClass}`;
    dom.stateTitle.textContent = i18n(cfg.titleKey);
    dom.stateSubtitle.textContent = i18n(cfg.subKey);
    dom.scoreValue.textContent = telem.score;

    // Snooze button status
    if (telem.snoozed) {
      dom.snoozeBtnText.textContent = i18n('snoozed_active');
      dom.quickSnoozeBtn.classList.add('btn-primary');
      dom.quickSnoozeBtn.classList.remove('btn-secondary');
    } else {
      dom.snoozeBtnText.textContent = i18n('btn_snooze');
      dom.quickSnoozeBtn.classList.remove('btn-primary');
      dom.quickSnoozeBtn.classList.add('btn-secondary');
    }
  }

  // Update 3D Mannequin Kinematics (Three.js Lite)
  function updateVectorAndSilhouette(telem) {
    const roll = typeof telem.roll === 'number' ? telem.roll : 0.0;
    const pitch = typeof telem.pitch === 'number' ? telem.pitch : 0.0;
    const yaw = typeof telem.yaw === 'number' ? telem.yaw : 0.0;
    const state = telem.state || 'GOOD';

    // 1. Update 3D Skeleton Engine
    if (app.skeleton3d) {
      app.skeleton3d.setAngles(roll, pitch, yaw, state);
    }

    // 2. Fallback SVG support if legacy elements exist
    if (dom.spineCurve) {
      const rollOffset = Math.max(-25, Math.min(25, roll));
      const pitchOffset = Math.max(-20, Math.min(20, pitch));
      const baseX = 80;
      const baseY = 160;
      const headX = baseX + rollOffset * 1.5;
      const headY = 30 + pitchOffset * 0.8;
      const ctrlX = baseX + (rollOffset * 0.8) + (pitchOffset * 0.5);
      const ctrlY = 100 + pitchOffset * 0.3;
      dom.spineCurve.setAttribute('d', `M ${baseX} ${baseY} Q ${ctrlX} ${ctrlY} ${headX} ${headY}`);
      if (dom.headCircle) {
        dom.headCircle.setAttribute('cx', headX);
        dom.headCircle.setAttribute('cy', headY - 14);
      }
      if (dom.sensorTag) {
        dom.sensorTag.setAttribute('x', baseX + (rollOffset * 0.4) - 7);
        dom.sensorTag.setAttribute('y', 70 + pitchOffset * 0.4);
      }
    }

    // 3. Update Status Caption
    if (dom.vectorCaption) {
      if (state === 'GOOD') {
        dom.vectorCaption.textContent = i18n('caption_aligned');
      } else if (state === 'SUSPECTED_SLOUCH') {
        dom.vectorCaption.textContent = i18n('caption_slouch');
      } else if (state === 'ALERT_L1' || state === 'ALERT_L2') {
        dom.vectorCaption.textContent = i18n('caption_alert');
      } else {
        dom.vectorCaption.textContent = i18n('caption_paused');
      }
    }
  }

  // Update Angle Readouts
  function updateAngleReadouts(telem) {
    const formatDeg = (val) => `${val >= 0 ? '+' : ''}${val.toFixed(1)}°`;

    if (dom.liveRoll) dom.liveRoll.textContent = formatDeg(telem.roll);
    if (dom.livePitch) dom.livePitch.textContent = formatDeg(telem.pitch);
    if (dom.liveYaw) dom.liveYaw.textContent = formatDeg(typeof telem.yaw === 'number' ? telem.yaw : 0.0);
    if (dom.liveDeviation) dom.liveDeviation.textContent = `${telem.deviation.toFixed(1)}°`;
    if (dom.liveThreshold) dom.liveThreshold.textContent = `${telem.threshold.toFixed(1)}°`;
  }

  // Update Device Screen Specs
  function updateDeviceSpecs(telem) {
    dom.devBattery.textContent = `${telem.battery}%`;
    dom.devRssi.textContent = `${telem.rssi} dBm`;

    // Format uptime
    const hrs = Math.floor(telem.uptime / 3600);
    const mins = Math.floor((telem.uptime % 3600) / 60);
    dom.devUptime.textContent = `${hrs.toString().padStart(2, '0')}h ${mins.toString().padStart(2, '0')}m`;
    if (dom.sessionTimeTag) {
      dom.sessionTimeTag.textContent = `${hrs.toString().padStart(2, '0')}h ${mins.toString().padStart(2, '0')}m ${i18n('monitored')}`;
    }

    const heapKb = Math.round(telem.free_heap / 1024);
    dom.devHeap.textContent = `${heapKb} KB`;
  }

  // =========================================================================
  // SIMULATION / STANDALONE DEMO MODE
  // =========================================================================
  function enableSimulationMode(enable) {
    app.simMode = enable;
    dom.simToggle.checked = enable;

    if (enable) {
      if (app.ws) {
        app.ws.close();
      }
      setConnectionState('demo');

      let simTick = 0;
      let phase = 'good'; // 'good', 'slouch_grace', 'slouch_alert'
      let phaseCounter = 0;

      if (app.simInterval) clearInterval(app.simInterval);

      app.simInterval = setInterval(() => {
        simTick++;
        phaseCounter++;

        let r = -0.8;
        let p = 2.0;
        let y = 0.0;
        let state = 'GOOD';

        // Periodic scenario cycle: 20s Good -> 6s Slouch Grace -> 10s Alert -> Recover
        // Roll (Cúi/Ngửa), Pitch (Nghiêng), Yaw (Xoay)
        if (phase === 'good') {
          r = -0.8 + Math.cos(simTick * 0.06) * 2.0;
          p = 2.0 + Math.sin(simTick * 0.08) * 2.5;
          y = Math.sin(simTick * 0.04) * 8.0;
          state = 'GOOD';
          if (phaseCounter > 180) { // ~18 seconds
            phase = 'slouch_grace';
            phaseCounter = 0;
          }
        } else if (phase === 'slouch_grace') {
          r = 16.5 + Math.sin(simTick * 0.1) * 2.0; // Exceeds 15 deg threshold
          p = 4.0 + Math.cos(simTick * 0.08) * 1.5;
          y = 5.0 + Math.sin(simTick * 0.07) * 10.0;
          state = 'SUSPECTED_SLOUCH';
          if (phaseCounter > 50) { // ~5 seconds grace period
            phase = 'slouch_alert';
            phaseCounter = 0;
          }
        } else if (phase === 'slouch_alert') {
          r = 19.5 + Math.sin(simTick * 0.15) * 2.0;
          p = 5.2 + Math.cos(simTick * 0.1) * 2.0;
          y = 12.0 + Math.cos(simTick * 0.08) * 12.0;
          state = phaseCounter > 60 ? 'ALERT_L2' : 'ALERT_L1';
          if (phaseCounter > 100) { // ~10 seconds alert, user corrects
            phase = 'good';
            phaseCounter = 0;
            addTimelineEvent('good', 'Posture Corrected', 'User straightened up after haptic cue');
          }
        }

        const dev = Math.sqrt(p * p + r * r);

        handleTelemetryUpdate({
          state: state,
          pitch: parseFloat(p.toFixed(2)),
          roll: parseFloat(r.toFixed(2)),
          yaw: parseFloat(y.toFixed(2)),
          deviation: parseFloat(dev.toFixed(2)),
          threshold: parseFloat(dom.threshSlider.value),
          battery: 92,
          rssi: -54,
          free_heap: 188416,
          uptime: 9940 + Math.floor(simTick / 10),
          calibrated: true,
          snoozed: false
        });
      }, 100); // 10 Hz

      showToast(i18n('toast_demo'));
    } else {
      if (app.simInterval) {
        clearInterval(app.simInterval);
        app.simInterval = null;
      }
      initWebSocket();
    }
  }

  // =========================================================================
  // EVENT LISTENERS & USER ACTIONS
  // =========================================================================
  function setupEventListeners() {
    // 3D Skeleton Camera Reset
    if (dom.btnReset3D) {
      dom.btnReset3D.addEventListener('click', () => {
        if (app.skeleton3d) {
          app.skeleton3d.resetCamera();
        }
      });
    }

    // Language Selector Buttons
    if (dom.btnLangEn) {
      dom.btnLangEn.addEventListener('click', () => setLanguage('en'));
    }
    if (dom.btnLangVi) {
      dom.btnLangVi.addEventListener('click', () => setLanguage('vi'));
    }

    // Demo Mode Toggle
    dom.simToggle.addEventListener('change', (e) => {
      enableSimulationMode(e.target.checked);
    });

    // Quick Snooze Button
    dom.quickSnoozeBtn.addEventListener('click', () => {
      handleSnooze(600);
    });

    // Calibration Modal Triggers
    dom.openCalibModalBtn.addEventListener('click', openCalibModal);
    dom.recalibBtn.addEventListener('click', openCalibModal);
    dom.closeCalibModalBtn.addEventListener('click', closeCalibModal);
    dom.startCalibCycleBtn.addEventListener('click', startCalibrationCycle);
    dom.cancelCalibBtn.addEventListener('click', closeCalibModal);
    dom.finishCalibBtn.addEventListener('click', closeCalibModal);

    // Threshold Slider
    dom.threshSlider.addEventListener('input', (e) => {
      dom.threshValLabel.textContent = `${e.target.value}°`;
      app.telemetry.threshold = parseFloat(e.target.value);
      dom.liveThreshold.textContent = `${e.target.value}.0°`;
    });

    // Settings Buttons
    dom.saveConfigBtn.addEventListener('click', saveConfigToDevice);
    dom.resetConfigBtn.addEventListener('click', resetConfigDefaults);

    // History CSV Export
    dom.exportCsvBtn.addEventListener('click', exportHistoryCsv);

    // OTA Firmware Update
    if (dom.otaFileInput) {
      dom.otaFileInput.addEventListener('change', handleOtaFileSelect);
    }
    if (dom.otaDropZone) {
      dom.otaDropZone.addEventListener('dragover', (e) => {
        e.preventDefault();
        dom.otaDropZone.classList.add('dragover');
      });
      dom.otaDropZone.addEventListener('dragleave', () => {
        dom.otaDropZone.classList.remove('dragover');
      });
      dom.otaDropZone.addEventListener('drop', handleOtaFileDrop);
    }
    if (dom.startOtaBtn) {
      dom.startOtaBtn.addEventListener('click', startOtaUpload);
    }
  }

  // =========================================================================
  // SNOOZE ACTION
  // =========================================================================
  function handleSnooze(seconds) {
    if (app.simMode) {
      app.telemetry.snoozed = true;
      app.telemetry.state = 'SNOOZED';
      updateHeroCard(app.telemetry);
      showToast(i18n('toast_snoozed_demo'));
      return;
    }

    fetch('/api/snooze', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ seconds: seconds })
    })
      .then(res => res.json())
      .then(() => {
        showToast(i18n('toast_snoozed'));
      })
      .catch(err => {
        console.error('Snooze request failed', err);
        showToast(i18n('toast_snoozed_local'));
      });
  }

  // =========================================================================
  // CALIBRATION MODAL CONTROLLER
  // =========================================================================
  let calibTimer = null;

  function openCalibModal() {
    setCalibStage(1);
    dom.calibModal.classList.add('open');
  }

  function closeCalibModal() {
    if (calibTimer) {
      clearInterval(calibTimer);
      calibTimer = null;
    }
    dom.calibModal.classList.remove('open');
  }

  function setCalibStage(stageNum) {
    dom.calibStage1.classList.remove('active');
    dom.calibStage2.classList.remove('active');
    dom.calibStage3.classList.remove('active');

    if (stageNum === 1) dom.calibStage1.classList.add('active');
    if (stageNum === 2) dom.calibStage2.classList.add('active');
    if (stageNum === 3) dom.calibStage3.classList.add('active');
  }

  function startCalibrationCycle() {
    setCalibStage(2);

    if (!app.simMode) {
      fetch('/api/calibrate', { method: 'POST' }).catch(err => console.warn(err));
    }

    let remainingMs = 3000;
    const totalMs = 3000;
    dom.calibProgressBar.style.width = '0%';

    calibTimer = setInterval(() => {
      remainingMs -= 100;
      const progress = Math.min(100, Math.round(((totalMs - remainingMs) / totalMs) * 100));
      dom.calibProgressBar.style.width = `${progress}%`;
      dom.calibCountdown.textContent = `${(Math.max(0, remainingMs) / 1000).toFixed(1)}s`;

      dom.calibLivePitch.textContent = `${app.telemetry.pitch >= 0 ? '+' : ''}${app.telemetry.pitch.toFixed(1)}°`;
      dom.calibLiveRoll.textContent = `${app.telemetry.roll >= 0 ? '+' : ''}${app.telemetry.roll.toFixed(1)}°`;

      if (remainingMs <= 0) {
        clearInterval(calibTimer);
        calibTimer = null;

        // Baseline offsets result
        const basePitch = (app.telemetry.pitch !== undefined ? app.telemetry.pitch : 0).toFixed(2);
        const baseRoll = (app.telemetry.roll !== undefined ? app.telemetry.roll : 0).toFixed(2);

        dom.calibResultPitch.textContent = `${basePitch >= 0 ? '+' : ''}${basePitch}°`;
        dom.calibResultRoll.textContent = `${baseRoll >= 0 ? '+' : ''}${baseRoll}°`;
        dom.currentBaselineDesc.textContent = `Pitch: ${basePitch}° | Roll: ${baseRoll}°`;

        setCalibStage(3);
        fetchConfig();
        fetchHistory();
        showToast(i18n('toast_calib_done'));
      }
    }, 100);
  }

  // =========================================================================
  // REST API CONFIGURATION
  // =========================================================================
  function fetchConfig() {
    fetch('/api/config')
      .then(res => res.json())
      .then(data => {
        if (data.threshold) {
          dom.threshSlider.value = data.threshold;
          dom.threshValLabel.textContent = `${data.threshold}°`;
          app.telemetry.threshold = data.threshold;
        }
        if (data.slouch_delay_s) dom.slouchDelayInput.value = data.slouch_delay_s;
        if (data.escalation_delay_s) dom.escalationDelayInput.value = data.escalation_delay_s;
        if (data.buzzer_enabled !== undefined) dom.buzzerToggle.checked = data.buzzer_enabled;
        if (data.pitch_offset !== undefined && data.roll_offset !== undefined) {
          dom.currentBaselineDesc.textContent = `Roll (Cúi/Ngửa): ${data.roll_offset.toFixed(1)}° | Pitch (Nghiêng): ${data.pitch_offset.toFixed(1)}°`;
        }
        if (data.version) {
          const verStr = data.version.startsWith('v') ? data.version : `v${data.version}`;
          if (dom.firmwareVerBadge) {
            dom.firmwareVerBadge.textContent = verStr;
          }
          if (dom.otaCurrentSlot) {
            dom.otaCurrentSlot.textContent = `${verStr} • ${data.ota_slot || 'ota_0'}`;
          }
          if (dom.otaCurrentVerDesc && data.build_date) {
            dom.otaCurrentVerDesc.textContent = `${i18n('ota_current_ver')}: ${verStr} (${data.build_date}) | Slot: ${data.ota_slot || 'ota_0'}`;
          }
        } else if (data.ota_slot && dom.otaCurrentSlot) {
          dom.otaCurrentSlot.textContent = `Slot: ${data.ota_slot}`;
        }
      })
      .catch(err => console.warn('Could not load config from ESP32:', err));
  }

  function saveConfigToDevice() {
    const payload = {
      threshold: parseFloat(dom.threshSlider.value),
      slouch_delay_s: parseInt(dom.slouchDelayInput.value, 10),
      escalation_delay_s: parseInt(dom.escalationDelayInput.value, 10),
      vibration_enabled: dom.vibrationToggle.checked,
      buzzer_enabled: dom.buzzerToggle.checked
    };

    if (app.simMode) {
      showToast(i18n('toast_config_demo'));
      return;
    }

    fetch('/api/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    })
      .then(res => res.json())
      .then(() => {
        showToast(i18n('toast_config_saved'));
      })
      .catch(err => {
        console.error('Failed to save config', err);
        showToast(i18n('toast_config_err'));
      });
  }

  function resetConfigDefaults() {
    dom.threshSlider.value = 15;
    dom.threshValLabel.textContent = '15°';
    dom.slouchDelayInput.value = 5;
    dom.escalationDelayInput.value = 15;
    dom.vibrationToggle.checked = true;
    dom.buzzerToggle.checked = true;
    saveConfigToDevice();
    showToast(i18n('toast_config_reset'));
  }

  // =========================================================================
  // FIRMWARE OTA UPDATE CONTROLLER
  // =========================================================================
  let selectedOtaFile = null;

  function setSelectedOtaFile(file) {
    if (!file) return;
    if (!file.name.toLowerCase().endsWith('.bin')) {
      showToast(i18n('toast_ota_select_bin'));
      return;
    }
    selectedOtaFile = file;
    if (dom.otaFileName) dom.otaFileName.textContent = file.name;
    if (dom.otaFileSize) {
      const kb = (file.size / 1024).toFixed(1);
      const mb = (file.size / (1024 * 1024)).toFixed(2);
      dom.otaFileSize.textContent = file.size > 1048576 ? `(${mb} MB)` : `(${kb} KB)`;
    }
    if (dom.otaFileDetails) dom.otaFileDetails.style.display = 'block';
    if (dom.startOtaBtn) dom.startOtaBtn.disabled = false;
  }

  function handleOtaFileSelect(e) {
    const file = e.target.files && e.target.files[0];
    if (file) setSelectedOtaFile(file);
  }

  function handleOtaFileDrop(e) {
    e.preventDefault();
    dom.otaDropZone.classList.remove('dragover');
    if (e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files.length > 0) {
      setSelectedOtaFile(e.dataTransfer.files[0]);
    }
  }

  function startOtaUpload() {
    if (!selectedOtaFile) {
      showToast(i18n('toast_ota_select_bin'));
      return;
    }

    if (app.simMode) {
      showToast('Demo Mode: OTA simulation not supported');
      return;
    }

    // Open modal
    dom.otaModal.style.display = 'flex';
    dom.otaCountdownWrap.style.display = 'none';
    dom.otaProgressBar.style.width = '0%';
    dom.otaProgressPct.textContent = '0%';
    dom.otaBytesSent.textContent = `0 KB / ${(selectedOtaFile.size / 1024).toFixed(0)} KB`;
    dom.startOtaBtn.disabled = true;

    showToast(i18n('toast_ota_started'));

    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/ota', true);
    xhr.setRequestHeader('Content-Type', 'application/octet-stream');

    xhr.upload.onprogress = function (e) {
      if (e.lengthComputable) {
        const pct = Math.round((e.loaded / e.total) * 100);
        dom.otaProgressBar.style.width = `${pct}%`;
        dom.otaProgressPct.textContent = `${pct}%`;
        const loadedKB = (e.loaded / 1024).toFixed(0);
        const totalKB = (e.total / 1024).toFixed(0);
        dom.otaBytesSent.textContent = `${loadedKB} KB / ${totalKB} KB`;
      }
    };

    xhr.onload = function () {
      if (xhr.status >= 200 && xhr.status < 300) {
        dom.otaProgressBar.style.width = '100%';
        dom.otaProgressPct.textContent = '100%';
        dom.otaModalTitle.textContent = i18n('toast_ota_complete');
        dom.otaModalDesc.textContent = i18n('ota_rebooting_label');
        dom.otaCountdownWrap.style.display = 'flex';
        showToast(i18n('toast_ota_complete'));

        // 10s countdown to reload
        let count = 10;
        dom.otaCountdownSec.textContent = count;
        const cdInterval = setInterval(() => {
          count--;
          if (dom.otaCountdownSec) dom.otaCountdownSec.textContent = count;
          if (count <= 0) {
            clearInterval(cdInterval);
            window.location.reload();
          }
        }, 1000);
      } else {
        let errMsg = 'HTTP ' + xhr.status;
        try {
          const resp = JSON.parse(xhr.responseText);
          if (resp.message) errMsg = resp.message;
        } catch (err) {}
        showToast(i18n('toast_ota_err') + errMsg);
        dom.otaModal.style.display = 'none';
        dom.startOtaBtn.disabled = false;
      }
    };

    xhr.onerror = function () {
      showToast(i18n('toast_ota_err') + 'Network connection lost');
      dom.otaModal.style.display = 'none';
      dom.startOtaBtn.disabled = false;
    };

    xhr.send(selectedOtaFile);
  }

  // =========================================================================
  // HISTORY & ANALYTICS RENDERING
  // =========================================================================
  function renderHourlyHeatmap() {
    const hours = ['08', '09', '10', '11', '12', '13', '14', '15', '16', '17', '18', '19'];
    // Mock hourly posture status distributions
    const qualities = ['good', 'good', 'warn', 'good', 'good', 'alert', 'good', 'good', 'warn', 'good', 'good', 'good'];

    dom.hourlyHeatmap.innerHTML = '';
    hours.forEach((hr, idx) => {
      const col = document.createElement('div');
      col.className = 'heatmap-col';
      col.innerHTML = `
        <div class="heatmap-bar ${qualities[idx]}"></div>
        <span class="heatmap-hour">${hr}h</span>
      `;
      dom.hourlyHeatmap.appendChild(col);
    });
  }

  function renderTimeline() {
    dom.timelineList.innerHTML = '';
    app.historyEvents.forEach(evt => {
      const title = (evt.titleKey && i18n(evt.titleKey)) ? i18n(evt.titleKey) : evt.title;
      const desc = (evt.descKey && i18n(evt.descKey)) ? i18n(evt.descKey) : evt.desc;
      const item = document.createElement('div');
      item.className = 'timeline-item';
      item.innerHTML = `
        <div class="timeline-dot-track">
          <div class="timeline-bullet ${evt.type}"></div>
        </div>
        <div class="timeline-content">
          <div class="timeline-title-row">
            <span class="timeline-title">${title}</span>
            <span class="timeline-time">${evt.time}</span>
          </div>
          <div class="timeline-desc">${desc}</div>
        </div>
      `;
      dom.timelineList.appendChild(item);
    });
  }

  function fetchHistory() {
    if (app.simMode) return;

    fetch('/api/history')
      .then(res => {
        if (!res.ok) throw new Error('HTTP ' + res.status);
        return res.json();
      })
      .then(data => {
        if (data.events && Array.isArray(data.events) && data.events.length > 0) {
          app.historyEvents = data.events;
          renderTimeline();
        }
        if (data.good_pct !== undefined && dom.histGoodPct) {
          dom.histGoodPct.textContent = `${data.good_pct}%`;
          const goodFill = document.querySelector('.good-fill');
          if (goodFill) goodFill.style.width = `${data.good_pct}%`;
        }
        if (data.slouch_pct !== undefined && dom.histSlouchPct) {
          dom.histSlouchPct.textContent = `${data.slouch_pct}%`;
          const warnFill = document.querySelector('.warn-fill');
          if (warnFill) warnFill.style.width = `${data.slouch_pct}%`;
        }
        if (data.alert_pct !== undefined && dom.histAlertPct) {
          dom.histAlertPct.textContent = `${data.alert_pct}%`;
          const dangerFill = document.querySelector('.danger-fill');
          if (dangerFill) dangerFill.style.width = `${data.alert_pct}%`;
        }
      })
      .catch(err => console.warn('Could not load history from ESP32:', err));
  }

  function addTimelineEvent(type, title, desc) {
    const now = new Date();
    const timeStr = now.toTimeString().split(' ')[0];
    app.historyEvents.unshift({
      time: timeStr,
      type: type,
      title: title,
      desc: desc
    });
    if (app.historyEvents.length > 20) app.historyEvents.pop();
    renderTimeline();
  }

  function exportHistoryCsv() {
    let csv = 'Timestamp,Event,Details\n';
    app.historyEvents.forEach(e => {
      const title = (e.titleKey && i18n(e.titleKey)) ? i18n(e.titleKey) : e.title;
      const desc = (e.descKey && i18n(e.descKey)) ? i18n(e.descKey) : e.desc;
      csv += `"${e.time}","${title}","${desc}"\n`;
    });

    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `posture_history_${new Date().toISOString().slice(0, 10)}.csv`;
    a.click();
    URL.revokeObjectURL(url);
    showToast(i18n('toast_csv_exported'));
  }

  // =========================================================================
  // TOAST FEEDBACK
  // =========================================================================
  function showToast(message) {
    const toast = document.createElement('div');
    toast.className = 'toast';
    toast.textContent = message;
    dom.toastContainer.appendChild(toast);

    setTimeout(() => {
      if (toast.parentElement) {
        toast.parentElement.removeChild(toast);
      }
    }, 2800);
  }

  // Launch on DOM ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

})();
