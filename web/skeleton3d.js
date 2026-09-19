/**
 * @file skeleton3d.js
 * @brief Ultra-lightweight, zero-dependency 3D Isometric Mini Skeleton Engine (Three.js Lite).
 * Runs 100% offline directly on ESP32 SoftAP (<12 KB).
 * Renders biomechanical spine kinematics with Roll (Slouch), Pitch (Lateral Tilt), and Yaw (Axial Twist).
 * Supports touch/mouse orbit controls, Hi-DPI retina rendering, and dynamic state LED glow.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

(function (root, factory) {
  if (typeof define === 'function' && define.amd) {
    define([], factory);
  } else if (typeof module === 'object' && module.exports) {
    module.exports = factory();
  } else {
    root.Skeleton3D = factory();
  }
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  // ---------------------------------------------------------------------------
  // 3D Math Utilities (Vector3 & Matrix4 Projection)
  // ---------------------------------------------------------------------------
  const DEG2RAD = Math.PI / 180;

  function vec3(x, y, z) {
    return { x: x || 0, y: y || 0, z: z || 0 };
  }

  function lerp(a, b, t) {
    return a + (b - a) * t;
  }

  // ---------------------------------------------------------------------------
  // Skeleton3D Class Definition
  // ---------------------------------------------------------------------------
  class Skeleton3D {
    constructor(canvas, options) {
      this.canvas = typeof canvas === 'string' ? document.getElementById(canvas) : canvas;
      if (!this.canvas) throw new Error('Skeleton3D: Canvas element not found');

      this.ctx = this.canvas.getContext('2d');
      this.options = Object.assign({
        fov: 40,
        cameraDistance: 290,
        defaultAzimuth: 0.65,    // ~37 degrees isometric angle
        defaultElevation: 0.32,  // ~18 degrees isometric elevation
        backgroundColor: 'transparent'
      }, options);

      // Camera Orbit Controls
      this.azimuth = this.options.defaultAzimuth;
      this.elevation = this.options.defaultElevation;
      this.targetAzimuth = this.azimuth;
      this.targetElevation = this.elevation;
      this.distance = this.options.cameraDistance;

      // Kinematic angles (Current interpolated and Target from telemetry)
      this.current = { roll: 0, pitch: 0, yaw: 0 };
      this.target = { roll: 0, pitch: 0, yaw: 0 };
      this.state = 'GOOD';
      this.calibrated = true;

      // Interaction state
      this.isDragging = false;
      this.lastMouseX = 0;
      this.lastMouseY = 0;
      this.touchDistance = 0;

      // Rendering state
      this.width = 0;
      this.height = 0;
      this.dpr = window.devicePixelRatio || 1;
      this.animId = null;
      this.pulseTime = 0;

      this.init();
    }

    init() {
      this.resize();
      window.addEventListener('resize', () => this.resize());
      this.bindEvents();
      this.startLoop();
    }

    resize() {
      const rect = this.canvas.getBoundingClientRect();
      this.width = rect.width || 240;
      this.height = rect.height || 240;
      this.dpr = Math.min(window.devicePixelRatio || 1, 2);

      this.canvas.width = this.width * this.dpr;
      this.canvas.height = this.height * this.dpr;
    }

    bindEvents() {
      const c = this.canvas;

      // Mouse drag
      c.addEventListener('mousedown', (e) => {
        this.isDragging = true;
        this.lastMouseX = e.clientX;
        this.lastMouseY = e.clientY;
      });

      window.addEventListener('mousemove', (e) => {
        if (!this.isDragging) return;
        const dx = e.clientX - this.lastMouseX;
        const dy = e.clientY - this.lastMouseY;
        this.lastMouseX = e.clientX;
        this.lastMouseY = e.clientY;

        this.targetAzimuth -= dx * 0.01;
        this.targetElevation = Math.max(-0.4, Math.min(0.9, this.targetElevation - dy * 0.01));
      });

      window.addEventListener('mouseup', () => {
        this.isDragging = false;
      });

      // Touch drag
      c.addEventListener('touchstart', (e) => {
        if (e.touches.length === 1) {
          this.isDragging = true;
          this.lastMouseX = e.touches[0].clientX;
          this.lastMouseY = e.touches[0].clientY;
        } else if (e.touches.length === 2) {
          this.touchDistance = Math.hypot(
            e.touches[0].clientX - e.touches[1].clientX,
            e.touches[0].clientY - e.touches[1].clientY
          );
        }
      }, { passive: true });

      c.addEventListener('touchmove', (e) => {
        if (e.touches.length === 1 && this.isDragging) {
          const dx = e.touches[0].clientX - this.lastMouseX;
          const dy = e.touches[0].clientY - this.lastMouseY;
          this.lastMouseX = e.touches[0].clientX;
          this.lastMouseY = e.touches[0].clientY;

          this.targetAzimuth -= dx * 0.01;
          this.targetElevation = Math.max(-0.4, Math.min(0.9, this.targetElevation - dy * 0.01));
        } else if (e.touches.length === 2) {
          const dist = Math.hypot(
            e.touches[0].clientX - e.touches[1].clientX,
            e.touches[0].clientY - e.touches[1].clientY
          );
          const diff = this.touchDistance - dist;
          this.distance = Math.max(180, Math.min(420, this.distance + diff * 0.5));
          this.touchDistance = dist;
        }
      }, { passive: true });

      c.addEventListener('touchend', () => {
        this.isDragging = false;
      }, { passive: true });

      // Wheel Zoom
      c.addEventListener('wheel', (e) => {
        e.preventDefault();
        this.distance = Math.max(180, Math.min(420, this.distance + e.deltaY * 0.4));
      }, { passive: false });
    }

    resetCamera() {
      this.targetAzimuth = this.options.defaultAzimuth;
      this.targetElevation = this.options.defaultElevation;
      this.distance = this.options.cameraDistance;
    }

    /**
     * @brief Update target kinematic Euler angles from telemetry
     * @param {number} roll   Cúi/Ngửa (Sagittal slouch forward/backward) in degrees
     * @param {number} pitch  Nghiêng (Coronal lateral tilt left/right) in degrees
     * @param {number} yaw    Xoay (Axial twist) in degrees
     * @param {string} state  Posture state ('GOOD', 'SUSPECTED_SLOUCH', 'ALERT_L1', 'ALERT_L2', etc.)
     */
    setAngles(roll, pitch, yaw, state) {
      this.target.roll = typeof roll === 'number' ? roll : 0;
      this.target.pitch = typeof pitch === 'number' ? pitch : 0;
      this.target.yaw = typeof yaw === 'number' ? yaw : 0;
      if (state) this.state = state;
    }

    startLoop() {
      const render = () => {
        this.update();
        this.draw();
        this.animId = requestAnimationFrame(render);
      };
      this.animId = requestAnimationFrame(render);
    }

    stopLoop() {
      if (this.animId) {
        cancelAnimationFrame(this.animId);
        this.animId = null;
      }
    }

    update() {
      // Smooth interpolation for angles
      this.current.roll = lerp(this.current.roll, this.target.roll, 0.16);
      this.current.pitch = lerp(this.current.pitch, this.target.pitch, 0.16);
      this.current.yaw = lerp(this.current.yaw, this.target.yaw, 0.16);

      // Smooth interpolation for camera orbit
      this.azimuth = lerp(this.azimuth, this.targetAzimuth, 0.12);
      this.elevation = lerp(this.elevation, this.targetElevation, 0.12);

      this.pulseTime += 0.05;
    }

    // -------------------------------------------------------------------------
    // 3D Point Projection onto 2D Canvas
    // -------------------------------------------------------------------------
    project(p) {
      // Camera orbit rotation
      const cosA = Math.cos(this.azimuth);
      const sinA = Math.sin(this.azimuth);
      const cosE = Math.cos(this.elevation);
      const sinE = Math.sin(this.elevation);

      // 1. Azimuth rotation around Y
      const x1 = p.x * cosA - p.z * sinA;
      const y1 = p.y;
      const z1 = p.x * sinA + p.z * cosA;

      // 2. Elevation rotation around X
      const x2 = x1;
      const y2 = y1 * cosE - z1 * sinE;
      const z2 = y1 * sinE + z1 * cosE + this.distance;

      // Perspective projection
      const fov = 340;
      const scale = (fov / Math.max(10, z2)) * this.dpr;
      const cx = (this.width / 2) * this.dpr;
      const cy = (this.height / 2 + 15) * this.dpr;

      return {
        x: cx + x2 * scale,
        y: cy - y2 * scale, // Canvas Y is inverted
        depth: z2,
        scale: scale
      };
    }

    // -------------------------------------------------------------------------
    // Main Render Pipeline
    // -------------------------------------------------------------------------
    draw() {
      const ctx = this.ctx;
      ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

      // 1. Draw Isometric Ground Pedestal
      this.drawGroundGrid(ctx);

      // 2. Draw Ghost Neutral Baseline (Translucent reference)
      this.drawSpine(ctx, { roll: 0, pitch: 0, yaw: 0 }, true);

      // 3. Draw Active Biomechanical Skeleton
      this.drawSpine(ctx, this.current, false);
    }

    // -------------------------------------------------------------------------
    // Ground Grid & Isometric Pedestal
    // -------------------------------------------------------------------------
    drawGroundGrid(ctx) {
      const rings = [24, 48, 70];
      const segments = 24;

      ctx.save();
      // Concentric rings on Y = -5
      rings.forEach((r, idx) => {
        ctx.beginPath();
        for (let i = 0; i <= segments; i++) {
          const theta = (i / segments) * Math.PI * 2;
          const pt = this.project(vec3(Math.cos(theta) * r, -5, Math.sin(theta) * r));
          if (i === 0) ctx.moveTo(pt.x, pt.y);
          else ctx.lineTo(pt.x, pt.y);
        }
        ctx.strokeStyle = idx === 2 ? 'rgba(0, 0, 0, 0.12)' : 'rgba(0, 0, 0, 0.05)';
        ctx.lineWidth = 1 * this.dpr;
        ctx.stroke();
      });

      // Subtle axes (Red = Lateral X, Blue = Sagittal Z)
      const origin = this.project(vec3(0, -5, 0));
      const axisX = this.project(vec3(26, -5, 0));
      const axisZ = this.project(vec3(0, -5, 26));

      ctx.beginPath();
      ctx.moveTo(origin.x, origin.y);
      ctx.lineTo(axisX.x, axisX.y);
      ctx.strokeStyle = 'rgba(255, 59, 48, 0.3)';
      ctx.lineWidth = 1.5 * this.dpr;
      ctx.stroke();

      ctx.beginPath();
      ctx.moveTo(origin.x, origin.y);
      ctx.lineTo(axisZ.x, axisZ.y);
      ctx.strokeStyle = 'rgba(0, 122, 255, 0.3)';
      ctx.lineWidth = 1.5 * this.dpr;
      ctx.stroke();

      ctx.restore();
    }

    // -------------------------------------------------------------------------
    // Biomechanical Spine & Skeleton Kinematics
    // -------------------------------------------------------------------------
    drawSpine(ctx, angles, isGhost) {
      // Kinematic mapping:
      // Roll (Cúi/Ngửa): Forward lean bends along Z axis (sagittal flexion)
      // Pitch (Nghiêng): Lateral lean bends along X axis (coronal tilt)
      // Yaw (Xoay): Torso twist rotates around Y axis (axial rotation)
      const rollRad = angles.roll * DEG2RAD;
      const pitchRad = angles.pitch * DEG2RAD;
      const yawRad = angles.yaw * DEG2RAD;

      const numVertebrae = 10;
      const totalHeight = 90;
      const segLength = totalHeight / numVertebrae;

      const vertebrae = [];
      let currentPos = vec3(0, 0, 0);

      // Cumulative Forward Kinematics along the spine
      for (let i = 0; i <= numVertebrae; i++) {
        const t = i / numVertebrae; // 0.0 at sacrum, 1.0 at upper neck
        // Non-linear curvature: thoracic spine flexes more during slouching
        const flexFactor = Math.pow(t, 1.3);

        // Curvature offsets
        const dx = Math.sin(pitchRad * flexFactor) * (segLength * i);
        const dz = -Math.sin(rollRad * flexFactor) * (segLength * i); // forward is -Z in our coords
        const dy = Math.cos(rollRad * flexFactor) * Math.cos(pitchRad * flexFactor) * (segLength * i);

        // Apply progressive yaw twist up the spinal column
        const currentYaw = yawRad * t;
        const rotatedX = dx * Math.cos(currentYaw) - dz * Math.sin(currentYaw);
        const rotatedZ = dx * Math.sin(currentYaw) + dz * Math.cos(currentYaw);

        vertebrae.push({
          pos: vec3(rotatedX, dy, rotatedZ),
          t: t,
          yaw: currentYaw
        });
      }

      const sacrum = vertebrae[0];
      const midThoracic = vertebrae[6]; // T4 area where sensor is mounted
      const upperThoracic = vertebrae[8]; // T1-T2 shoulder level
      const headTop = vertebrae[vertebrae.length - 1];

      // Colors based on state
      let stateColor = '#34C759'; // GOOD
      if (this.state === 'SUSPECTED_SLOUCH') stateColor = '#FF9500';
      else if (this.state === 'ALERT_L1' || this.state === 'ALERT_L2') stateColor = '#FF3B30';
      else if (this.state === 'CALIBRATING') stateColor = '#007AFF';
      else if (this.state === 'SNOOZED') stateColor = '#86868B';

      const spineColor = isGhost ? 'rgba(142, 142, 147, 0.25)' : (this.state === 'GOOD' ? '#1D1D1F' : stateColor);
      const boneColor = isGhost ? 'rgba(142, 142, 147, 0.2)' : 'rgba(29, 29, 31, 0.85)';

      ctx.save();

      // A. Draw Vertebrae Spine Column
      ctx.beginPath();
      vertebrae.forEach((v, idx) => {
        const p = this.project(v.pos);
        if (idx === 0) ctx.moveTo(p.x, p.y);
        else ctx.lineTo(p.x, p.y);
      });
      ctx.strokeStyle = spineColor;
      ctx.lineWidth = (isGhost ? 3 : 5) * this.dpr;
      ctx.lineCap = 'round';
      ctx.lineJoin = 'round';
      if (isGhost) ctx.setLineDash([3 * this.dpr, 4 * this.dpr]);
      ctx.stroke();
      ctx.setLineDash([]);

      // Draw vertebral discs
      if (!isGhost) {
        vertebrae.forEach((v, idx) => {
          if (idx % 2 === 0) {
            const p = this.project(v.pos);
            ctx.beginPath();
            ctx.arc(p.x, p.y, (idx === 0 ? 3.5 : 2.5) * p.scale * 0.08, 0, Math.PI * 2);
            ctx.fillStyle = '#1D1D1F';
            ctx.fill();
          }
        });
      }

      // B. Draw Pelvis Base Bar
      const pelvisWidth = 24;
      const pLeft = this.project(vec3(-pelvisWidth, sacrum.pos.y, sacrum.pos.z));
      const pRight = this.project(vec3(pelvisWidth, sacrum.pos.y, sacrum.pos.z));
      ctx.beginPath();
      ctx.moveTo(pLeft.x, pLeft.y);
      ctx.lineTo(pRight.x, pRight.y);
      ctx.strokeStyle = boneColor;
      ctx.lineWidth = (isGhost ? 2 : 3.5) * this.dpr;
      ctx.stroke();

      // C. Draw Ribcage Hoops (Thoracic oval wireframe)
      const rib1 = vertebrae[4];
      const rib2 = vertebrae[7];
      [rib1, rib2].forEach((ribV, rIdx) => {
        const rSize = rIdx === 0 ? 18 : 22;
        ctx.beginPath();
        const segs = 16;
        for (let i = 0; i <= segs; i++) {
          const a = (i / segs) * Math.PI * 2;
          const rx = Math.cos(a) * rSize;
          const rz = Math.sin(a) * (rSize * 0.65);
          // Rotate with thoracic yaw
          const rotX = rx * Math.cos(ribV.yaw) - rz * Math.sin(ribV.yaw);
          const rotZ = rx * Math.sin(ribV.yaw) + rz * Math.cos(ribV.yaw);
          const pt = this.project(vec3(ribV.pos.x + rotX, ribV.pos.y, ribV.pos.z + rotZ));
          if (i === 0) ctx.moveTo(pt.x, pt.y);
          else ctx.lineTo(pt.x, pt.y);
        }
        ctx.strokeStyle = isGhost ? 'rgba(142, 142, 147, 0.15)' : 'rgba(29, 29, 31, 0.2)';
        ctx.lineWidth = 1.2 * this.dpr;
        ctx.stroke();
      });

      // D. Draw Clavicle / Shoulders
      const shoulderWidth = 32;
      const shYaw = upperThoracic.yaw;
      const shLeft3D = vec3(
        upperThoracic.pos.x + (-shoulderWidth * Math.cos(shYaw)),
        upperThoracic.pos.y + 1,
        upperThoracic.pos.z + (-shoulderWidth * Math.sin(shYaw))
      );
      const shRight3D = vec3(
        upperThoracic.pos.x + (shoulderWidth * Math.cos(shYaw)),
        upperThoracic.pos.y + 1,
        upperThoracic.pos.z + (shoulderWidth * Math.sin(shYaw))
      );

      const shL = this.project(shLeft3D);
      const shR = this.project(shRight3D);

      ctx.beginPath();
      ctx.moveTo(shL.x, shL.y);
      ctx.lineTo(shR.x, shR.y);
      ctx.strokeStyle = boneColor;
      ctx.lineWidth = (isGhost ? 2 : 4) * this.dpr;
      ctx.stroke();

      // Shoulder ball joints
      if (!isGhost) {
        [shL, shR].forEach(sh => {
          ctx.beginPath();
          ctx.arc(sh.x, sh.y, 3 * this.dpr, 0, Math.PI * 2);
          ctx.fillStyle = '#1D1D1F';
          ctx.fill();
        });
      }

      // E. Draw Head & Directional Visor
      const headCenter3D = vec3(headTop.pos.x, headTop.pos.y + 15, headTop.pos.z);
      const headCenter = this.project(headCenter3D);
      const headRadius = (isGhost ? 10 : 12) * headCenter.scale * 0.08;

      ctx.beginPath();
      ctx.arc(headCenter.x, headCenter.y, headRadius, 0, Math.PI * 2);
      ctx.fillStyle = isGhost ? 'rgba(142, 142, 147, 0.15)' : (this.state === 'GOOD' ? '#1D1D1F' : stateColor);
      ctx.fill();

      // Directional Visor on front of head (shows where head is facing)
      if (!isGhost) {
        const visorAngle = headTop.yaw;
        const visorDist = headRadius * 0.85;
        const vx = headCenter.x + Math.sin(visorAngle - this.azimuth) * visorDist;
        const vy = headCenter.y;
        ctx.beginPath();
        ctx.arc(vx, vy, 2.5 * this.dpr, 0, Math.PI * 2);
        ctx.fillStyle = '#FFFFFF';
        ctx.fill();
      }

      // F. Draw Posture Sensor Puck on Thoracic Spine (T2-T4)
      if (!isGhost) {
        // Sensor mounted slightly behind the spine (+Z in local back normal)
        const sensorOffsetZ = 4.5;
        const sYaw = midThoracic.yaw;
        const sensorPos3D = vec3(
          midThoracic.pos.x + Math.sin(sYaw) * sensorOffsetZ,
          midThoracic.pos.y + 2,
          midThoracic.pos.z + Math.cos(sYaw) * sensorOffsetZ
        );
        const sProj = this.project(sensorPos3D);

        // Outer sensor body (Square puck with rounded corners)
        const puckW = 14 * this.dpr;
        const puckH = 10 * this.dpr;
        ctx.save();
        ctx.translate(sProj.x, sProj.y);

        // Puck shadow/casing
        ctx.fillStyle = '#2C2C2E';
        ctx.beginPath();
        if (ctx.roundRect) ctx.roundRect(-puckW / 2, -puckH / 2, puckW, puckH, 3 * this.dpr);
        else ctx.rect(-puckW / 2, -puckH / 2, puckW, puckH);
        ctx.fill();

        // Pulsing LED glow
        const pulse = (this.state === 'ALERT_L1' || this.state === 'ALERT_L2') ? (0.6 + Math.sin(this.pulseTime * 4) * 0.4) : 1.0;
        ctx.shadowColor = stateColor;
        ctx.shadowBlur = 8 * pulse * this.dpr;

        // LED dot on puck
        ctx.beginPath();
        ctx.arc(0, 0, 2.5 * this.dpr, 0, Math.PI * 2);
        ctx.fillStyle = stateColor;
        ctx.fill();

        ctx.restore();
      }

      ctx.restore();
    }
  }

  return Skeleton3D;
});
