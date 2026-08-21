(function () {
  'use strict';
  if (window.__slmInjected) return;
  window.__slmInjected = true;

  // ---------- 工具 ----------
  function el(tag, cls, html) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (html !== undefined) e.innerHTML = html;
    return e;
  }
  function post(cmd, extra) {
    var msg = { cmd: cmd };
    if (extra) for (var k in extra) msg[k] = extra[k];
    var json = JSON.stringify(msg);
    // 命令进队列，C# 侧每 0.15s 轮询读取；同时尝试 vuplex.postMessage 双保险
    var q = window.__slmQueue || (window.__slmQueue = []);
    q.push(json);
    if (window.vuplex && window.vuplex.postMessage) window.vuplex.postMessage(json);
  }
  var $ = function (id) { return document.getElementById(id); };

  // ---------- 样式 ----------
  var css = [
    '#slm-host{position:fixed;left:0;top:0;width:100%;height:100%;z-index:2147483647;pointer-events:none;font-family:"Microsoft YaHei","PingFang SC",sans-serif;}',
    '#slm-host *{box-sizing:border-box;margin:0;padding:0;}',
    '#slm-panel{position:fixed;left:50%;top:50%;transform:translate(-50%,-50%);width:760px;max-width:94vw;max-height:86vh;display:flex;flex-direction:column;background:rgba(24,26,32,0.96);border:1px solid #3a3f4b;border-radius:10px;box-shadow:0 8px 40px rgba(0,0,0,0.6);pointer-events:auto;color:#d8dce4;overflow:hidden;user-select:none;}',
    '#slm-host.slm-hidden #slm-panel{display:none;}',
    '#slm-bar{display:flex;align-items:center;justify-content:space-between;padding:10px 14px;background:#1a1d24;border-bottom:1px solid #2c313c;cursor:move;flex-shrink:0;}',
    '#slm-title{font-size:15px;font-weight:600;color:#e8ebf2;letter-spacing:1px;}',
    '#slm-title b{color:#5b9dff;}',
    '#slm-close{width:26px;height:26px;line-height:24px;text-align:center;border-radius:6px;color:#8b93a5;font-size:16px;cursor:pointer;}',
    '#slm-close:hover{background:#333a47;color:#ff6b6b;}',
    '#slm-tabs{display:flex;gap:4px;padding:8px 12px 0;background:#171a20;border-bottom:1px solid #2c313c;flex-shrink:0;}',
    '.slm-tab{padding:8px 18px;border-radius:8px 8px 0 0;font-size:13px;color:#8b93a5;cursor:pointer;border:1px solid transparent;border-bottom:none;}',
    '.slm-tab:hover{color:#c9cfda;}',
    '.slm-tab.slm-on{background:#22262f;color:#5b9dff;border-color:#2c313c;font-weight:600;}',
    '#slm-body{flex:1;overflow-y:auto;padding:14px 16px 16px;min-height:320px;max-height:60vh;}',
    '.slm-sec{margin-bottom:14px;}',
    '.slm-sec-t{font-size:13px;color:#9aa3b2;margin-bottom:8px;border-left:3px solid #5b9dff;padding-left:8px;}',
    '.slm-row{display:flex;align-items:center;gap:8px;margin-bottom:8px;flex-wrap:wrap;}',
    '.slm-label{width:86px;font-size:13px;color:#b6bdca;flex-shrink:0;}',
    '.slm-input{flex:1;min-width:90px;height:30px;padding:0 10px;background:#111318;border:1px solid #333a47;border-radius:6px;color:#e8ebf2;font-size:13px;outline:none;}',
    '.slm-input:focus{border-color:#5b9dff;}',
    '.slm-btn{padding:7px 14px;background:#262b35;border:1px solid #3a4150;border-radius:6px;color:#d8dce4;font-size:13px;cursor:pointer;white-space:nowrap;}',
    '.slm-btn:hover{background:#2f3642;border-color:#4a5366;}',
    '.slm-btn.slm-pri{background:#2b4d80;border-color:#3a6cb8;color:#cfe4ff;}',
    '.slm-btn.slm-pri:hover{background:#35609f;}',
    '.slm-btn.slm-warn{background:#6b3a2e;border-color:#a05a44;color:#ffd9cc;}',
    '.slm-btn.slm-warn:hover{background:#7d4536;}',
    '.slm-lock{display:flex;align-items:center;gap:6px;font-size:12px;color:#8b93a5;cursor:pointer;white-space:nowrap;}',
    '.slm-lock input{display:none;}',
    '.slm-lock .slm-cb{width:16px;height:16px;border:1px solid #4a5263;border-radius:4px;display:inline-flex;align-items:center;justify-content:center;font-size:11px;color:#fff;}',
    '.slm-lock input:checked+.slm-cb{background:#2b4d80;border-color:#5b9dff;}',
    '.slm-lock input:checked+.slm-cb::after{content:"\\2713";}',
    '.slm-val{font-size:13px;color:#5b9dff;min-width:64px;text-align:right;font-variant-numeric:tabular-nums;}',
    '.slm-slider{flex:1;min-width:120px;accent-color:#5b9dff;height:4px;}',
    '.slm-msg{position:fixed;left:50%;top:8%;transform:translateX(-50%);padding:8px 18px;background:rgba(20,22,28,0.95);border:1px solid #3a4150;border-radius:8px;color:#d8dce4;font-size:13px;pointer-events:none;opacity:0;transition:opacity .25s;z-index:2147483647;}',
    '.slm-msg.slm-ok{border-color:#3a9d5c;color:#7fe0a4;}',
    '.slm-msg.slm-err{border-color:#c0504a;color:#ff9d94;}',
    '.slm-hint{font-size:12px;color:#6a7280;line-height:1.7;}'
  ].join('');
  var styleEl = el('style');
  styleEl.textContent = css;
  document.documentElement.appendChild(styleEl);

  // ---------- 面板 DOM ----------
  var host = el('div', 'slm-hidden');
  host.id = 'slm-host';
  host.innerHTML =
    '<div id="slm-panel">' +
    '  <div id="slm-bar"><div id="slm-title">生存日志 <b>修改器</b></div><div id="slm-close" title="关闭(热键呼出)">✕</div></div>' +
    '  <div id="slm-tabs">' +
    '    <div class="slm-tab slm-on" data-tab="player">玩家</div>' +
    '    <div class="slm-tab" data-tab="shop">商店</div>' +
    '    <div class="slm-tab" data-tab="time">倒计时</div>' +
    '    <div class="slm-tab" data-tab="misc">设置</div>' +
    '  </div>' +
    '  <div id="slm-body">' +
    // 玩家
    '    <div class="slm-page" data-page="player">' +
    '      <div class="slm-sec"><div class="slm-sec-t">金币</div>' +
    '        <div class="slm-row"><span class="slm-label">当前</span><span class="slm-val" id="v-gold">-</span>' +
    '          <input class="slm-input" id="i-gold" type="number" placeholder="输入金额"><button class="slm-btn slm-pri" id="b-gold">设置</button>' +
    '          <label class="slm-lock"><input type="checkbox" id="lk-gold"><span class="slm-cb"></span>锁定</label></div>' +
    '      </div>' +
    '      <div class="slm-sec"><div class="slm-sec-t">五维属性</div>' +
    '        <div class="slm-row"><span class="slm-label">饱腹</span><span class="slm-val" id="v-a1">-</span><input class="slm-slider" id="s-a1" type="range" min="0" max="100" value="0">' +
    '          <input class="slm-input slm-num" id="i-a1" type="number" min="0" max="1000"><button class="slm-btn slm-pri" id="b-a1">设置</button>' +
    '          <label class="slm-lock"><input type="checkbox" id="lk-a1"><span class="slm-cb"></span>锁定</label></div>' +
    '        <div class="slm-row"><span class="slm-label">心态</span><span class="slm-val" id="v-a2">-</span><input class="slm-slider" id="s-a2" type="range" min="0" max="100" value="0">' +
    '          <input class="slm-input slm-num" id="i-a2" type="number" min="0" max="1000"><button class="slm-btn slm-pri" id="b-a2">设置</button>' +
    '          <label class="slm-lock"><input type="checkbox" id="lk-a2"><span class="slm-cb"></span>锁定</label></div>' +
    '        <div class="slm-row"><span class="slm-label">精力</span><span class="slm-val" id="v-a3">-</span><input class="slm-slider" id="s-a3" type="range" min="0" max="100" value="0">' +
    '          <input class="slm-input slm-num" id="i-a3" type="number" min="0" max="1000"><button class="slm-btn slm-pri" id="b-a3">设置</button>' +
    '          <label class="slm-lock"><input type="checkbox" id="lk-a3"><span class="slm-cb"></span>锁定</label></div>' +
    '        <div class="slm-row"><span class="slm-label">健康</span><span class="slm-val" id="v-a4">-</span><input class="slm-slider" id="s-a4" type="range" min="0" max="100" value="0">' +
    '          <input class="slm-input slm-num" id="i-a4" type="number" min="0" max="1000"><button class="slm-btn slm-pri" id="b-a4">设置</button>' +
    '          <label class="slm-lock"><input type="checkbox" id="lk-a4"><span class="slm-cb"></span>锁定</label></div>' +
    '        <div class="slm-row"><span class="slm-label">生命</span><span class="slm-val" id="v-a5">-</span><input class="slm-slider" id="s-a5" type="range" min="0" max="100" value="0">' +
    '          <input class="slm-input slm-num" id="i-a5" type="number" min="0" max="1000"><button class="slm-btn slm-pri" id="b-a5">设置</button>' +
    '          <label class="slm-lock"><input type="checkbox" id="lk-a5"><span class="slm-cb"></span>锁定</label></div>' +
    '      </div>' +
    '    </div>' +
    // 商店
    '    <div class="slm-page" data-page="shop" style="display:none">' +
    '      <div class="slm-sec"><div class="slm-sec-t">商店资金</div>' +
    '        <div class="slm-row"><span class="slm-label">商店金钱</span><span class="slm-val" id="v-shopGold">-</span>' +
    '          <input class="slm-input" id="i-shopGold" type="number" placeholder="输入金额"><button class="slm-btn slm-pri" id="b-shopGold">设置</button>' +
    '          <label class="slm-lock"><input type="checkbox" id="lk-shopGold"><span class="slm-cb"></span>锁定</label></div>' +
    '        <div class="slm-row"><span class="slm-label">待安装包裹</span><span class="slm-val" id="v-pkg">-</span>' +
    '          <input class="slm-input" id="i-pkg" type="number" placeholder="输入数量"><button class="slm-btn slm-pri" id="b-pkg">设置</button>' +
    '          <label class="slm-lock"><input type="checkbox" id="lk-pkg"><span class="slm-cb"></span>锁定</label></div>' +
    '      </div>' +
    '      <div class="slm-sec"><div class="slm-sec-t">库存</div>' +
    '        <div class="slm-row"><button class="slm-btn slm-warn" id="b-restore">恢复商店库存</button>' +
    '          <span class="slm-hint">灾变后商品会售罄(库存熔毁)，一键恢复全部库存</span></div>' +
    '      </div>' +
    '    </div>' +
    // 倒计时
    '    <div class="slm-page" data-page="time" style="display:none">' +
    '      <div class="slm-sec"><div class="slm-sec-t">灾变倒计时</div>' +
    '        <div class="slm-row"><span class="slm-label">剩余秒</span><span class="slm-val" id="v-sec">-</span>' +
    '          <input class="slm-input" id="i-sec" type="number" placeholder="输入秒"><button class="slm-btn slm-pri" id="b-sec">设置</button>' +
    '          <label class="slm-lock"><input type="checkbox" id="lk-sec"><span class="slm-cb"></span>锁定</label></div>' +
    '        <div class="slm-row"><label class="slm-lock"><input type="checkbox" id="lk-frozen"><span class="slm-cb"></span>冻结时间(灾变不降临)</label></div>' +
    '      </div>' +
    '      <div class="slm-sec"><div class="slm-sec-t">灾变阶段</div>' +
    '        <div class="slm-row"><span class="slm-label">当前</span><span class="slm-val" id="v-ws">-</span>' +
    '          <button class="slm-btn" id="b-ws1">灾变前</button><button class="slm-btn" id="b-ws2">灾变后</button></div>' +
    '      </div>' +
    '    </div>' +
    // 设置
    '    <div class="slm-page" data-page="misc" style="display:none">' +
    '      <div class="slm-sec"><div class="slm-sec-t">说明</div>' +
    '        <div class="slm-hint">· 呼出/关闭面板：Insert / F6 / F7 / F8（可在 BepInEx 配置中修改）<br>' +
    '        · 锁定 = 每 0.25 秒强制写回该值，防游戏逻辑修改<br>' +
    '        · 属性数值范围 0-1000，内部按 x1000 存储<br>' +
    '        · 冻结时间 = IsClockFrozen，灾变倒计时停止</div>' +
    '      </div>' +
    '    </div>' +
    '  </div>' +
    '</div>';
  document.documentElement.appendChild(host);

  var msgBox = el('div', 'slm-msg');
  document.documentElement.appendChild(msgBox);
  var msgTimer = null;
  function toast(text, ok) {
    msgBox.textContent = text;
    msgBox.className = 'slm-msg ' + (ok ? 'slm-ok' : 'slm-err');
    msgBox.style.opacity = '1';
    if (msgTimer) clearTimeout(msgTimer);
    msgTimer = setTimeout(function () { msgBox.style.opacity = '0'; }, 1800);
  }

  // ---------- 事件绑定 ----------
  var ATTRS = [1, 2, 3, 4, 5];

  $('slm-close').addEventListener('click', function () { post('toggle'); });

  // tabs
  var tabs = document.querySelectorAll('#slm-tabs .slm-tab');
  for (var i = 0; i < tabs.length; i++) {
    (function (tab) {
      tab.addEventListener('click', function () {
        for (var j = 0; j < tabs.length; j++) tabs[j].classList.remove('slm-on');
        tab.classList.add('slm-on');
        var pages = document.querySelectorAll('#slm-body .slm-page');
        for (var k = 0; k < pages.length; k++) pages[k].style.display = 'none';
        var pg = document.querySelector('#slm-body .slm-page[data-page="' + tab.getAttribute('data-tab') + '"]');
        if (pg) pg.style.display = '';
      });
    })(tabs[i]);
  }

  // 拖动（Pointer Events + capture，基于视觉 rect 计算偏移，兼容 transform 初始定位）
  (function () {
    var bar = $('slm-bar'), panel = $('slm-panel'), dragging = false, ox = 0, oy = 0;
    bar.addEventListener('pointerdown', function (e) {
      dragging = true;
      var rect = panel.getBoundingClientRect();
      panel.style.transform = 'none';
      panel.style.left = rect.left + 'px';
      panel.style.top = rect.top + 'px';
      ox = e.clientX - rect.left;
      oy = e.clientY - rect.top;
      try { bar.setPointerCapture(e.pointerId); } catch (err) {}
      e.preventDefault();
    });
    bar.addEventListener('pointermove', function (e) {
      if (!dragging) return;
      panel.style.left = (e.clientX - ox) + 'px';
      panel.style.top = (e.clientY - oy) + 'px';
      e.preventDefault();
    });
    function endDrag() { dragging = false; }
    bar.addEventListener('pointerup', endDrag);
    bar.addEventListener('pointercancel', endDrag);
  })();

  // 金币
  $('b-gold').addEventListener('click', function () {
    var v = parseInt($('i-gold').value, 10);
    if (isNaN(v)) { toast('请输入有效金额', false); return; }
    post('setGold', { v: v });
  });
  $('lk-gold').addEventListener('change', function () {
    var on = this.checked;
    var v = on ? (parseInt($('i-gold').value, 10) || 0) : 0;
    post('lockGold', { on: on, v: v });
  });

  // 属性
  ATTRS.forEach(function (a) {
    $('s-a' + a).addEventListener('input', function () { $('i-a' + a).value = this.value; });
    $('i-a' + a).addEventListener('input', function () {
      var v = parseInt(this.value, 10);
      if (!isNaN(v)) $('s-a' + a).value = Math.max(0, Math.min(100, v));
    });
    $('b-a' + a).addEventListener('click', function () {
      var v = parseInt($('i-a' + a).value, 10);
      if (isNaN(v)) { toast('请输入有效数值', false); return; }
      post('setAttr', { a: a, v: v });
    });
    $('lk-a' + a).addEventListener('change', function () {
      var on = this.checked;
      var v = on ? (parseInt($('i-a' + a).value, 10) || 0) : 0;
      post('lockAttr', { a: a, on: on, v: v });
    });
  });

  // 商店
  $('b-shopGold').addEventListener('click', function () {
    var v = parseInt($('i-shopGold').value, 10);
    if (isNaN(v)) { toast('请输入有效金额', false); return; }
    post('setShopGold', { v: v });
  });
  $('lk-shopGold').addEventListener('change', function () {
    var on = this.checked;
    post('lockShopGold', { on: on, v: on ? (parseInt($('i-shopGold').value, 10) || 0) : 0 });
  });
  $('b-pkg').addEventListener('click', function () {
    var v = parseInt($('i-pkg').value, 10);
    if (isNaN(v)) { toast('请输入有效数量', false); return; }
    post('setPkg', { v: v });
  });
  $('lk-pkg').addEventListener('change', function () {
    var on = this.checked;
    post('lockPkg', { on: on, v: on ? (parseInt($('i-pkg').value, 10) || 0) : 0 });
  });
  $('b-restore').addEventListener('click', function () { post('restoreStock'); });

  // 倒计时
  $('b-sec').addEventListener('click', function () {
    var v = parseInt($('i-sec').value, 10);
    if (isNaN(v)) { toast('请输入有效秒数', false); return; }
    post('setSeconds', { v: v });
  });
  $('lk-sec').addEventListener('change', function () {
    var on = this.checked;
    post('lockSeconds', { on: on, v: on ? (parseInt($('i-sec').value, 10) || 0) : 0 });
  });
  $('lk-frozen').addEventListener('change', function () { post('setFrozen', { on: this.checked }); });
  $('b-ws1').addEventListener('click', function () { post('setWorldState', { v: 1 }); });
  $('b-ws2').addEventListener('click', function () { post('setWorldState', { v: 2 }); });

  // ---------- C# → JS 推送 ----------
  window.__slPush = function (msg) {
    if (!msg || typeof msg !== 'object') return;
    switch (msg.type) {
      case 'state':
        if (msg.gold >= 0) $('v-gold').textContent = msg.gold;
        if (msg.a1 >= 0) { $('v-a1').textContent = msg.a1; $('s-a1').value = Math.min(100, msg.a1); $('i-a1').value = msg.a1; }
        if (msg.a2 >= 0) { $('v-a2').textContent = msg.a2; $('s-a2').value = Math.min(100, msg.a2); $('i-a2').value = msg.a2; }
        if (msg.a3 >= 0) { $('v-a3').textContent = msg.a3; $('s-a3').value = Math.min(100, msg.a3); $('i-a3').value = msg.a3; }
        if (msg.a4 >= 0) { $('v-a4').textContent = msg.a4; $('s-a4').value = Math.min(100, msg.a4); $('i-a4').value = msg.a4; }
        if (msg.a5 >= 0) { $('v-a5').textContent = msg.a5; $('s-a5').value = Math.min(100, msg.a5); $('i-a5').value = msg.a5; }
        if (msg.shopGold >= 0) $('v-shopGold').textContent = msg.shopGold;
        if (msg.pkg >= 0) $('v-pkg').textContent = msg.pkg;
        if (msg.sec >= 0) $('v-sec').textContent = msg.sec;
        $('v-ws').textContent = msg.ws === 2 ? '灾变后' : (msg.ws === 1 ? '灾变前' : '-');
        $('lk-frozen').checked = !!msg.frozen;
        break;
      case 'panel':
        host.classList.toggle('slm-hidden', !msg.open);
        break;
      case 'result':
        toast(msg.msg || (msg.ok ? 'OK' : '失败'), !!msg.ok);
        if (msg.gold >= 0) $('v-gold').textContent = msg.gold;
        break;
    }
  };

  // ---------- 就绪 ----------
  post('ready');
})();
