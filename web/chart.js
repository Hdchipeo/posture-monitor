/**
 * @file chart.js
 * @brief High-performance, zero-dependency HTML5 Canvas rolling real-time chart.
 * Designed for ESP32 embedded web server (Apple Health aesthetic, Retina crisp).
 */

class PostureRealtimeChart {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    if (!this.canvas) return;

    this.ctx = this.canvas.getContext('2d');
    this.maxSamples = 60; // 30 seconds of history at 2 Hz or downsampled 10 Hz
    this.dataPoints = [];
    this.currentThreshold = 15.0;
    this.lang = 'en';

    // Set initial baseline
    for (let i = 0; i < this.maxSamples; i++) {
      this.dataPoints.push({
        val: 2.0,
        thresh: 15.0
      });
    }

    this.handleResize = this.resize.bind(this);
    window.addEventListener('resize', this.handleResize);
    this.resize();
  }

  setLanguage(lang) {
    this.lang = lang || 'en';
    this.render();
  }

  resize() {
    if (!this.canvas) return;
    const rect = this.canvas.parentElement.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;

    this.width = rect.width;
    this.height = rect.height || 180;

    this.canvas.width = this.width * dpr;
    this.canvas.height = this.height * dpr;

    if (typeof this.ctx.resetTransform === 'function') {
      this.ctx.resetTransform();
    } else {
      this.ctx.setTransform(1, 0, 0, 1, 0, 0);
    }
    this.ctx.scale(dpr, dpr);

    this.render();
  }

  pushSample(deviation, threshold) {
    if (typeof deviation !== 'number' || isNaN(deviation)) return;

    this.currentThreshold = typeof threshold === 'number' ? threshold : this.currentThreshold;

    this.dataPoints.push({
      val: Math.max(0, deviation),
      thresh: this.currentThreshold
    });

    if (this.dataPoints.length > this.maxSamples) {
      this.dataPoints.shift();
    }

    this.render();
  }

  render() {
    if (!this.ctx || !this.width) return;

    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;
    const padTop = 18;
    const padBottom = 26;
    const padLeft = 8;
    const padRight = 8;
    const graphH = h - padTop - padBottom;
    const graphW = w - padLeft - padRight;
    const isVi = this.lang === 'vi';

    // Y Axis Range: 0 to max(30, currentThreshold + 10)
    const maxVal = Math.max(28, this.currentThreshold + 8);
    const getY = (val) => padTop + graphH - (Math.min(val, maxVal) / maxVal) * graphH;

    // Clear frame
    ctx.clearRect(0, 0, w, h);

    // 1. Draw subtle horizontal grid lines (0, 10, 20 deg)
    ctx.lineWidth = 1;
    ctx.strokeStyle = '#F0F0F2';
    ctx.fillStyle = '#A1A1A6';
    ctx.font = '10px -apple-system, BlinkMacSystemFont, "SF Pro Text", sans-serif';
    ctx.textAlign = 'left';

    const gridSteps = [0, 10, 20];
    gridSteps.forEach(step => {
      const y = getY(step);
      ctx.beginPath();
      ctx.moveTo(padLeft, y);
      ctx.lineTo(w - padRight, y);
      ctx.stroke();

      if (step > 0) {
        ctx.fillText(`${step}°`, padLeft + 4, y - 4);
      }
    });

    // 2. Draw Threshold Dotted Line
    const threshY = getY(this.currentThreshold);
    ctx.save();
    ctx.strokeStyle = 'rgba(255, 59, 48, 0.6)';
    ctx.lineWidth = 1.5;
    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.moveTo(padLeft, threshY);
    ctx.lineTo(w - padRight, threshY);
    ctx.stroke();
    ctx.fillStyle = '#FF3B30';
    ctx.textAlign = 'right';
    ctx.fillText(`${this.currentThreshold.toFixed(0)}° ${isVi ? 'ngưỡng' : 'limit'}`, w - padRight - 4, threshY - 4);
    ctx.restore();

    // 3. Draw Deviation Area Fill & Curve
    if (this.dataPoints.length > 1) {
      const stepX = graphW / (this.maxSamples - 1);

      // Area gradient
      const gradient = ctx.createLinearGradient(0, padTop, 0, h - padBottom);
      gradient.addColorStop(0, 'rgba(29, 29, 31, 0.12)');
      gradient.addColorStop(1, 'rgba(29, 29, 31, 0.00)');

      // Path for curve
      ctx.beginPath();
      for (let i = 0; i < this.dataPoints.length; i++) {
        const x = padLeft + i * stepX;
        const y = getY(this.dataPoints[i].val);

        if (i === 0) {
          ctx.moveTo(x, y);
        } else {
          // Bezier smoothing between points
          const prevX = padLeft + (i - 1) * stepX;
          const prevY = getY(this.dataPoints[i - 1].val);
          const cpX = (prevX + x) / 2;
          ctx.bezierCurveTo(cpX, prevY, cpX, y, x, y);
        }
      }

      // Fill area under line
      ctx.save();
      const lastX = padLeft + (this.dataPoints.length - 1) * stepX;
      ctx.lineTo(lastX, h - padBottom);
      ctx.lineTo(padLeft, h - padBottom);
      ctx.closePath();
      ctx.fillStyle = gradient;
      ctx.fill();
      ctx.restore();

      // Stroke Line
      ctx.save();
      ctx.beginPath();
      for (let i = 0; i < this.dataPoints.length; i++) {
        const x = padLeft + i * stepX;
        const y = getY(this.dataPoints[i].val);

        if (i === 0) {
          ctx.moveTo(x, y);
        } else {
          const prevX = padLeft + (i - 1) * stepX;
          const prevY = getY(this.dataPoints[i - 1].val);
          const cpX = (prevX + x) / 2;
          ctx.bezierCurveTo(cpX, prevY, cpX, y, x, y);
        }
      }
      ctx.lineWidth = 2.5;
      ctx.strokeStyle = '#1D1D1F';
      ctx.lineCap = 'round';
      ctx.lineJoin = 'round';
      ctx.stroke();

      // Draw active head dot at the latest point
      const curX = padLeft + (this.dataPoints.length - 1) * stepX;
      const curVal = this.dataPoints[this.dataPoints.length - 1].val;
      const curY = getY(curVal);

      ctx.beginPath();
      ctx.arc(curX, curY, 4, 0, Math.PI * 2);
      ctx.fillStyle = curVal > this.currentThreshold ? '#FF3B30' : '#1D1D1F';
      ctx.fill();
      ctx.lineWidth = 2;
      ctx.strokeStyle = '#FFFFFF';
      ctx.stroke();
      ctx.restore();
    }

    // 4. Time Axis markers at bottom
    ctx.fillStyle = '#86868B';
    ctx.font = '10px -apple-system, BlinkMacSystemFont, "SF Pro Text", sans-serif';
    ctx.textAlign = 'left';
    ctx.fillText(isVi ? '30s trước' : '30s ago', padLeft, h - 6);
    ctx.textAlign = 'center';
    ctx.fillText('15s', w / 2, h - 6);
    ctx.textAlign = 'right';
    ctx.fillText(isVi ? 'Hiện tại' : 'Now', w - padRight, h - 6);
  }
}

window.PostureRealtimeChart = PostureRealtimeChart;
