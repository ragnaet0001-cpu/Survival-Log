/* Neverlose-styled Survival Log panel. Runs inside the game's existing WebView. */
(function () {
  if (window.__slcInjected && document.getElementById('slc-neverlose-host')) return;
  window.__slcInjected = true;

  window.__cheatQueue = window.__cheatQueue || [];
  var lastWakeAt = 0;
  function slcPostMessage(message) {
    try { if (window.vuplex && typeof window.vuplex.postMessage === 'function') window.vuplex.postMessage(message); } catch (_) {}
  }
  function send(command) {
    window.__cheatQueue.push(JSON.stringify(command));
    // The native bridge also polls the queue every 250 ms. Coalescing wake
    // messages avoids flooding the game's WebUI protocol with custom events.
    var now = Date.now();
    if (now - lastWakeAt >= 100) {
      lastWakeAt = now;
      slcPostMessage('slc-wake');
    }
  }

  var scrollbarCss = 'html,body,*{scrollbar-width:none!important;-ms-overflow-style:none!important;}html::-webkit-scrollbar,body::-webkit-scrollbar,*::-webkit-scrollbar{display:none!important;width:0!important;height:0!important;}';
  var outerScrollbarFix = document.getElementById('slc-outer-scrollbar-fix');
  if (!outerScrollbarFix) {
    outerScrollbarFix = document.createElement('style');
    outerScrollbarFix.id = 'slc-outer-scrollbar-fix';
    outerScrollbarFix.textContent = scrollbarCss;
    (document.head || document.documentElement).appendChild(outerScrollbarFix);
  }

  // Inventory pages are same-origin iframes created after the root page loads.
  // Parent-page CSS cannot cross that document boundary, so wire every frame
  // and reapply after game-driven iframe reloads.
  function installFrameScrollbarFix(frame) {
    try {
      var doc = frame && frame.contentDocument;
      if (!doc || !doc.documentElement) return;
      var style = doc.getElementById('slc-frame-scrollbar-fix');
      if (!style) {
        style = doc.createElement('style');
        style.id = 'slc-frame-scrollbar-fix';
        style.textContent = scrollbarCss;
        (doc.head || doc.documentElement).appendChild(style);
      }
    } catch (_) {}
  }
  function wireFrameScrollbarFix(frame) {
    if (!frame || frame.__slcScrollbarWired) return;
    frame.__slcScrollbarWired = true;
    frame.addEventListener('load', function () { installFrameScrollbarFix(frame); });
    installFrameScrollbarFix(frame);
  }
  function scanFrameScrollbars() {
    try {
      var frames = document.querySelectorAll('iframe');
      for (var i = 0; i < frames.length; i++) wireFrameScrollbarFix(frames[i]);
    } catch (_) {}
  }
  scanFrameScrollbars();
  try {
    new MutationObserver(scanFrameScrollbars).observe(document.documentElement, { childList: true, subtree: true });
  } catch (_) {}

  var outerScrollLock = null;
  function setOuterScrollLock(on) {
    if (on) {
      if (!outerScrollLock || !outerScrollLock.isConnected) {
        outerScrollLock = document.createElement('style');
        outerScrollLock.id = 'slc-outer-scroll-lock';
        outerScrollLock.textContent = 'html,body{overflow:hidden!important;overscroll-behavior:none!important;}';
        (document.head || document.documentElement).appendChild(outerScrollLock);
      }
    } else if (outerScrollLock && outerScrollLock.isConnected) {
      outerScrollLock.remove();
      outerScrollLock = null;
    }
  }

  var host = document.createElement('div');
  host.id = 'slc-neverlose-host';
  host.setAttribute('style', 'position:fixed;left:50%;top:50%;z-index:2147483647;overflow:visible;transform:translate(-50%,-50%);will-change:transform;');
  (document.body || document.documentElement).appendChild(host);
  var shadow = host.attachShadow({ mode: 'open' });

  shadow.innerHTML = `
  <style>
    * { box-sizing: border-box; }
    :host { all: initial; }
    .nl { width: min(830px, calc(100vw - 32px)); height: min(580px, calc(100vh - 32px)); min-height: 430px;
      display: none; overflow: hidden; color: #fff; background: rgba(8, 10, 17, .78); border: 1px solid rgba(119, 143, 184, .2); border-radius: 14px;
      backdrop-filter: blur(22px) saturate(120%); -webkit-backdrop-filter: blur(22px) saturate(120%);
      font: 13px "Microsoft YaHei", "Segoe UI", sans-serif; box-shadow: 0 24px 80px rgba(0,0,0,.62), inset 0 1px rgba(255,255,255,.035); user-select: none; cursor: move; will-change: transform; backface-visibility: hidden; }
    .sidebar { width: 192px; flex: 0 0 192px; background: rgba(3, 22, 38, .82); border-right: 1px solid rgba(119, 143, 184, .16); backdrop-filter: blur(18px); -webkit-backdrop-filter: blur(18px); display: flex; flex-direction: column; }
    .brand { height: 60px; padding: 19px 17px 0; font: 700 18px Arial, sans-serif; letter-spacing: 0; cursor: move; touch-action: none; }
    .brand b { color: #00a5f3; } .brand span { color: #fff; }
    .tabs { padding: 10px 10px; overflow: auto; flex: 1; }
    .section { margin: 8px 0 16px; padding-left: 10px; color: #a2b0b9; font-size: 12px; }
    .nav { height: 34px; width: 100%; padding: 0 10px; border: 0; border-radius: 4px; background: transparent; color: #a2b0b9;
      text-align: left; cursor: pointer; font: inherit; display: flex; align-items: center; gap: 10px; }
    .nav:hover { background: #021221; color: #fff; } .nav.on { background: #021221; color: #fff; }
    .nav i { width: 16px; color: #00a5f3; font-style: normal; text-align: center; }
    .profile { min-height: 76px; border-top: 1px solid #0c2134; padding: 9px 12px; display: flex; align-items: center; gap: 9px; }
    .avatar { width: 32px; height: 32px; border-radius: 50%; background: #04192a; color: #00a5f3; display: grid; place-items: center; font-size: 15px; }
    .profile > div:last-child { min-width: 0; flex: 1; } .profile strong { display: block; font-size: 12px; font-weight: 600; } .profile small { display: block; color: #a2b0b9; font-size: 9px; line-height: 1.35; white-space: normal; }
    .main { min-width: 0; flex: 1; background: rgba(9, 8, 13, .55); display: flex; flex-direction: column; }
    .top { height: 60px; flex: 0 0 60px; border-bottom: 1px solid #0c2134; display: flex; align-items: center; padding: 0 20px; gap: 12px; cursor: move; touch-action: none; }
    .title { flex: 0 0 auto; font-size: 14px; font-weight: 600; } .status { margin-left: auto; color: #7ca3b7; font-size: 12px; } .status.ready { color: #59c176; }
    .buff-header-actions { display: none; align-items: center; gap: 6px; margin-left: 2px; }
    .buff-header-actions.show { display: flex; }
    .buff-header-actions button.action { height: 26px; padding: 0 9px; font-size: 10px; }
    .icon { width: 30px; height: 30px; border: 0; background: transparent; color: #a2b0b9; cursor: pointer; font-size: 17px; }
    .icon:hover { color: #fff; background: #021221; border-radius: 4px; }
    .body { padding: 22px 22px 18px; overflow: auto; flex: 1; } .pane { display: none; } .pane.on { display: block; animation: enter .16s ease-out; }
    @keyframes enter { from { opacity: 0; transform: translateY(4px); } to { opacity: 1; transform: translateY(0); } }
    .grid { display: grid; grid-template-columns: minmax(220px,1fr) minmax(220px,1fr); gap: 16px; }
    .box { min-width: 0; min-height: 116px; overflow: hidden; background: rgba(7, 13, 24, .58); border: 1px solid rgba(119, 143, 184, .17); border-radius: 12px; padding: 16px; box-shadow: inset 0 1px rgba(255,255,255,.025); backdrop-filter: blur(14px); -webkit-backdrop-filter: blur(14px); }
    .box.wide { grid-column: 1 / -1; } .box h3 { margin: 0 0 15px; font-size: 12px; font-weight: 500; color: #d8e0e5; letter-spacing: 0; }
    .row { min-height: 30px; display: flex; flex-wrap: wrap; align-items: center; gap: 8px; margin: 7px 0; color: #aebbc3; }
    .row label { min-width: 82px; } .row .value { color: #fff; font-weight: 600; }
    .field-grid { display: grid; grid-template-columns: 58px minmax(0,1fr); gap: 8px; align-items: center; margin: 10px 0; color: #aebbc3; }
    .field-grid input { width: 100% !important; max-width: 100%; } .actions { display: flex; flex-wrap: wrap; gap: 8px; margin-top: 10px; }
    input, select { min-width: 0; height: 28px; border: 1px solid #0c2134; border-radius: 3px; outline: none; background: #02050c; color: #fff; padding: 0 8px; font: inherit; user-select: text; caret-color: #00a5f3; cursor: text; }
    input:focus, select:focus { border-color: #00a5f3; } input.invalid { border-color: #c54b4b; background: #1d080a; } input[type=number] { width: 104px; appearance: textfield; -moz-appearance: textfield; } input[type=number]::-webkit-inner-spin-button, input[type=number]::-webkit-outer-spin-button { appearance: none; -webkit-appearance: none; margin: 0; } input[type=search] { width: 100%; }
    button.action, button.preset { height: 28px; border: 1px solid #23282c; border-radius: 3px; color: #dce7ec; background: #101114; padding: 0 10px; font: inherit; cursor: pointer; }
    button:focus { outline: none; } button:focus-visible { outline: 1px solid #00a5f3; outline-offset: -1px; }
    button.action:hover, button.preset:hover { background: #19212a; border-color: #006da6; } button.action.primary { background: #006da6; border-color: #008bd2; color: #fff; }
    button.action.primary:hover { background: #008bd2; } button.action.danger:hover { border-color: #b84848; background: #331313; }
    .presets { display: flex; flex-wrap: wrap; gap: 5px; margin-top: 8px; } button.preset { font-size: 11px; padding: 0 8px; }
    .hint { color: #7d8d97; font-size: 11px; line-height: 1.45; margin-top: 9px; }
    .chips { display: flex; flex-wrap: wrap; gap: 5px; margin: 9px 0; } .chip { border: 1px solid #23282c; border-radius: 6px; background: rgba(20, 22, 31, .72); color: #aebbc3; padding: 4px 9px; font-size: 11px; cursor: pointer; }
    .chip:hover, .chip.on { color: #fff; border-color: #006da6; background: #04192a; }
    .buff-toolbar { grid-column: 1 / -1; display: flex; align-items: center; justify-content: space-between; gap: 14px; padding: 10px 12px; background: rgba(18, 20, 30, .62); border: 1px solid rgba(119, 143, 184, .16); border-radius: 12px; backdrop-filter: blur(16px); -webkit-backdrop-filter: blur(16px); }
    .buff-scope, .buff-subnav { display: flex; gap: 4px; padding: 3px; background: rgba(0, 0, 0, .18); border: 1px solid rgba(119, 143, 184, .12); border-radius: 8px; }
    .buff-tab { min-width: 72px; height: 28px; border: 0; border-radius: 6px; background: transparent; color: #86929f; font: inherit; cursor: pointer; }
    .buff-tab:hover, .buff-tab.on { background: rgba(71, 103, 180, .3); color: #fff; box-shadow: inset 0 1px rgba(255,255,255,.08); }
    .buff-subnav { margin-left: auto; }
    .buff-toolbar .buff-caption { flex: 1; color: #7d8d97; font-size: 11px; }
    .buff-grid .buff-library-box, .buff-grid .buff-current-box, .buff-grid .buff-planning-box { grid-column: span 1; }
    .buff-grid { grid-template-columns: minmax(0, 1fr); gap: 12px; }
    .buff-grid .buff-library-box, .buff-grid .buff-current-box, .buff-grid .buff-planning-box { grid-column: 1 / -1; }
    .buff-grid #buff-config-list, .buff-grid #buff-list, .buff-grid #survival-plan-list { max-height: 300px; overflow-y: auto; scrollbar-width: none; }
    .buff-grid #buff-config-list::-webkit-scrollbar, .buff-grid #buff-list::-webkit-scrollbar, .buff-grid #survival-plan-list::-webkit-scrollbar { display: none; }
    .buff-planning-box .hint { margin-top: 0; }
    .plan-item { display: grid; grid-template-columns: minmax(0,1fr) auto auto; gap: 8px; align-items: center; padding: 11px 0; border-bottom: 1px solid rgba(119, 143, 184, .12); }
    .plan-item:last-child { border-bottom: 0; }
    .plan-item strong { display: block; color: #edf3f7; font-size: 12px; }
    .plan-item small { display: block; margin-top: 4px; color: #84929e; line-height: 1.35; }
    .plan-state { color: #71d19a; font-size: 11px; white-space: nowrap; }
    .items, .bag { height: 210px; overflow: auto; contain: content; border: 1px solid #0c2134; border-radius: 3px; background: #02050c; cursor: default; }
    .body, .tabs, .items, .bag { scrollbar-width: none; -ms-overflow-style: none; }
    .body::-webkit-scrollbar, .tabs::-webkit-scrollbar, .items::-webkit-scrollbar, .bag::-webkit-scrollbar { display: none; width: 0; height: 0; }
    .item, .bagrow { min-height: 32px; display: flex; align-items: center; gap: 8px; padding: 0 9px; border-bottom: 1px solid #16191c; cursor: pointer; color: #cbd5da; }
    .item:last-child, .bagrow:last-child { border-bottom: 0; } .item:hover, .item.sel { background: #04192a; } .item .name { overflow: hidden; white-space: nowrap; text-overflow: ellipsis; flex: 1; }
    .muted { color: #7d8d97; } .id { color: #658092; font-size: 11px; } .bagrow { cursor: default; } .bagrow .name { flex: 1; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .bagrow input { width: 65px; } .bagrow button { height: 23px; padding: 0 6px; font-size: 11px; }
    .attribute { display: grid; grid-template-columns: minmax(58px,1fr) 76px 68px 42px 68px 42px 30px; gap: 5px; align-items: center; min-height: 36px; border-bottom: 1px solid #17191b; }
    .attribute.head { min-height: 24px; color: #667985; font-size: 10px; } .attribute input { width: 100%; } .attribute button.action { padding: 0 5px; }
    .attribute:last-of-type { border-bottom: 0; } .attribute .lock { width: auto; accent-color: #00a5f3; }
    .proficiency { display: grid; grid-template-columns: minmax(90px,1fr) 52px 105px 90px 74px 76px; gap: 7px; align-items: center; min-height: 42px; border-bottom: 1px solid #17191b; }
    .proficiency:last-child { border-bottom: 0; } .proficiency input { width: 100%; } .proficiency .exp { color: #9fb0ba; font-size: 11px; } .proficiency .add-level { min-width: 76px; padding: 0 8px; white-space: nowrap; }
    .switch { display: inline-flex; align-items: center; gap: 8px; cursor: pointer; color: #cbd5da; } .switch input { position: absolute; opacity: 0; }
    .switch span { width: 29px; height: 15px; border-radius: 9px; background: #1d252a; position: relative; transition: .15s; }
    .switch span:after { content: ''; width: 11px; height: 11px; top: 2px; left: 2px; border-radius: 50%; position: absolute; background: #6d7b83; transition: .15s; }
    .switch input:checked + span { background: #004b73; }.switch input:checked + span:after { left: 16px; background: #00a5f3; }
    .toast { position: absolute; right: 22px; bottom: 20px; display: none; max-width: 360px; padding: 10px 12px; color: #dce7ec; background: #101820; border: 1px solid #006da6; border-radius: 3px; box-shadow: 0 8px 26px rgba(0,0,0,.45); }
    .toast.error { border-color: #a64242; } .toast.show { display: block; }
    .settings { position: absolute; right: 14px; top: 53px; z-index: 3; width: 270px; max-width: calc(100% - 28px); padding: 15px; background: rgba(11,11,13,.98); border: 1px solid #1d252a; border-radius: 4px; box-shadow: 0 14px 44px rgba(0,0,0,.6); display: none; }
    .settings.open { display: block; }.settings h4 { margin: 0 0 13px; font-size: 13px; }.settings .logo { margin: 11px 0 17px; text-align: center; font: 700 17px "Microsoft YaHei", sans-serif; }.settings .logo b { color: #00a5f3; }
    .settings .row { margin: 0; }.settings .row label { flex: 1; }.scale-row select { width: 112px; }
    .about-content { max-width: 680px; margin: 6px auto 0; padding: 10px 0 18px; } .support-message { color: #dce7ec; line-height: 1.7; margin: 2px 0 18px; text-align: center; } .support-image { display: block; width: min(360px, 100%); height: auto; aspect-ratio: 1; object-fit: contain; margin: 0 auto; border-radius: 4px; }
    @media (max-width: 680px) { .sidebar { width: 145px; flex-basis: 145px; }.brand { padding-left: 12px; font-size: 14px; }.tabs { padding: 8px; }.nav { padding: 0 7px; }.grid { grid-template-columns: 1fr; }.box.wide { grid-column: auto; }.body { padding: 16px; }.profile strong, .profile small { display:none; } .attribute { grid-template-columns: minmax(48px,1fr) 66px 58px 36px 58px 36px 28px; gap: 4px; }.proficiency { grid-template-columns: minmax(80px,1fr) 48px 90px; }.field-grid { grid-template-columns: 48px minmax(0,1fr); } }
    /* Neverlose-inspired visual system: quiet glass layers, compact controls, strong hierarchy. */
    :host { color-scheme: dark; }
    .nl { width: min(960px, calc(100vw - 44px)); height: min(650px, calc(100vh - 44px)); min-height: 500px; background: rgba(10, 12, 19, .78); border: 1px solid rgba(177, 192, 224, .2); border-radius: 18px; backdrop-filter: blur(28px) saturate(125%); -webkit-backdrop-filter: blur(28px) saturate(125%); box-shadow: 0 28px 90px rgba(0, 0, 0, .65), inset 0 1px rgba(255,255,255,.06); }
    .sidebar { width: 214px; flex-basis: 214px; background: rgba(11, 14, 23, .73); border-right: 1px solid rgba(177, 192, 224, .13); }
    .brand { height: 76px; padding: 24px 20px 0; font-size: 19px; letter-spacing: -.1px; }
    .brand b { color: #6d91ff; } .brand span { color: #f2f5fb; }
    .tabs { padding: 15px 12px; }
    .section { margin: 12px 10px 8px; padding: 0; color: #697487; font-size: 11px; text-transform: uppercase; letter-spacing: .7px; }
    .nav { height: 38px; padding: 0 12px; border-radius: 9px; color: #8c96a7; gap: 12px; transition: background .14s ease, color .14s ease, transform .14s ease; }
    .nav:hover { background: rgba(86, 108, 169, .14); color: #e7ebf3; transform: translateX(1px); }
    .nav.on { background: linear-gradient(90deg, rgba(82, 108, 180, .3), rgba(82, 108, 180, .12)); color: #f4f7ff; box-shadow: inset 2px 0 #6d91ff, inset 0 1px rgba(255,255,255,.045); }
    .nav i { width: 18px; color: #6d91ff; font-size: 14px; }
    .profile { min-height: 82px; padding: 12px 16px; border-top-color: rgba(177, 192, 224, .13); }
    .avatar { width: 35px; height: 35px; background: rgba(84, 113, 196, .18); color: #8eabff; border: 1px solid rgba(127, 157, 255, .2); overflow: hidden; padding: 0; }
    .avatar img { width: 100%; height: 100%; display: block; object-fit: cover; }
    .profile strong { color: #e9edf5; font-size: 12px; } .profile small { color: #687486; font-size: 9px; }
    .main { background: rgba(9, 11, 18, .48); }
    .top { height: 66px; flex-basis: 66px; padding: 0 24px; border-bottom-color: rgba(177, 192, 224, .13); gap: 10px; }
    .title { color: #f1f4fa; font-size: 16px; letter-spacing: .1px; } .status { color: #7f8ba0; font-size: 11px; } .status.ready { color: #6ad39b; }
    .icon { width: 34px; height: 34px; border-radius: 8px; color: #8d97a8; transition: background .14s ease, color .14s ease; }
    .icon:hover { color: #f5f7ff; background: rgba(86, 108, 169, .16); }
    .body { padding: 24px; }
    .grid { gap: 18px; }
    .box { min-height: 126px; padding: 18px; background: rgba(15, 18, 28, .56); border: 1px solid rgba(177, 192, 224, .15); border-radius: 14px; box-shadow: inset 0 1px rgba(255,255,255,.035), 0 9px 28px rgba(0,0,0,.12); backdrop-filter: blur(18px); -webkit-backdrop-filter: blur(18px); }
    .box h3 { margin-bottom: 17px; color: #e7ebf3; font-size: 13px; font-weight: 600; letter-spacing: .1px; }
    .row { min-height: 34px; gap: 9px; margin: 6px 0; color: #a0aabb; }
    .row label { min-width: 88px; color: #929daf; } .row .value { color: #f0f3fa; font-weight: 600; }
    .field-grid { gap: 9px; margin: 11px 0; color: #929daf; }
    .actions { gap: 8px; margin-top: 12px; }
    input, select { height: 34px; border: 1px solid rgba(177, 192, 224, .14); border-radius: 8px; background: rgba(4, 6, 12, .58); color: #edf1f8; padding: 0 10px; transition: border-color .14s ease, background .14s ease, box-shadow .14s ease; }
    input:hover, select:hover { background: rgba(26, 30, 43, .68); } input:focus, select:focus { border-color: rgba(109, 145, 255, .8); box-shadow: 0 0 0 3px rgba(109, 145, 255, .13); }
    input.invalid { border-color: #d05b70; background: rgba(73, 16, 27, .48); }
    button.action, button.preset { height: 34px; border: 1px solid rgba(177, 192, 224, .14); border-radius: 8px; color: #cbd3e0; background: rgba(29, 32, 43, .68); padding: 0 12px; font-size: 12px; transition: background .14s ease, border-color .14s ease, transform .14s ease, box-shadow .14s ease; }
    button.action:hover, button.preset:hover { background: rgba(65, 79, 122, .42); border-color: rgba(109, 145, 255, .6); color: #fff; transform: translateY(-1px); }
    button.action.primary { background: linear-gradient(180deg, #668cff, #4c70dc); border-color: rgba(143, 168, 255, .68); color: #fff; box-shadow: 0 5px 16px rgba(64, 99, 222, .24); }
    button.action.primary:hover { background: linear-gradient(180deg, #7699ff, #587ce8); box-shadow: 0 7px 20px rgba(64, 99, 222, .34); }
    button.action.danger { color: #e5a8b1; border-color: rgba(199, 85, 108, .28); background: rgba(70, 22, 34, .38); }
    button.action.danger:hover { border-color: rgba(226, 102, 126, .72); background: rgba(111, 32, 48, .52); }
    .presets { gap: 6px; margin-top: 9px; } button.preset { font-size: 11px; padding: 0 10px; }
    .hint { color: #747f91; font-size: 11px; line-height: 1.5; margin-top: 9px; }
    .chips { gap: 6px; margin: 10px 0; } .chip { border-color: rgba(177, 192, 224, .13); border-radius: 7px; background: rgba(29, 32, 43, .6); color: #9ca7b8; padding: 5px 10px; }
    .chip:hover, .chip.on { color: #fff; border-color: rgba(109, 145, 255, .7); background: rgba(75, 105, 190, .34); }
    .items, .bag { height: 224px; border-color: rgba(177, 192, 224, .13); border-radius: 10px; background: rgba(3, 5, 11, .48); }
    .item, .bagrow { min-height: 35px; border-bottom-color: rgba(177, 192, 224, .09); color: #cbd3df; }
    .item:hover, .item.sel { background: rgba(75, 105, 190, .2); }
    .attribute, .proficiency { border-bottom-color: rgba(177, 192, 224, .1); }
    .attribute.head { color: #687487; } .proficiency .exp { color: #8e9aad; }
    .switch span { width: 38px; height: 20px; border-radius: 11px; background: rgba(90, 101, 122, .46); box-shadow: inset 0 0 0 1px rgba(255,255,255,.06); }
    .switch span:after { width: 16px; height: 16px; top: 2px; left: 2px; background: #8c97a7; box-shadow: 0 2px 6px rgba(0,0,0,.34); }
    .switch input:checked + span { background: #5d83f4; } .switch input:checked + span:after { left: 20px; background: #fff; }
    .buff-toolbar { padding: 11px 13px; background: rgba(20, 23, 34, .62); border-color: rgba(177, 192, 224, .14); border-radius: 12px; }
    .buff-scope, .buff-subnav { background: rgba(0, 0, 0, .18); border-color: rgba(177, 192, 224, .1); border-radius: 9px; }
    .buff-tab { height: 30px; border-radius: 7px; color: #8590a2; } .buff-tab:hover, .buff-tab.on { background: rgba(93, 131, 244, .32); color: #fff; }
    .plan-item { border-bottom-color: rgba(177, 192, 224, .1); }
    .toast { right: 24px; bottom: 22px; padding: 11px 14px; background: rgba(18, 22, 32, .9); border-color: rgba(109, 145, 255, .7); border-radius: 9px; box-shadow: 0 12px 36px rgba(0,0,0,.52); }
    .toast.error { border-color: rgba(218, 91, 115, .75); }
    .settings { right: 16px; top: 59px; padding: 17px; background: rgba(18, 21, 31, .92); border-color: rgba(177, 192, 224, .18); border-radius: 12px; box-shadow: 0 18px 50px rgba(0,0,0,.6); backdrop-filter: blur(22px); -webkit-backdrop-filter: blur(22px); }
    #buff-config-list, #buff-list, #survival-plan-list { max-height: 310px; overflow-y: auto; scrollbar-width: none; -ms-overflow-style: none; }
    #buff-config-list::-webkit-scrollbar, #buff-list::-webkit-scrollbar, #survival-plan-list::-webkit-scrollbar { display: none; width: 0; height: 0; }
    /* Final layout pass: keep controls aligned while preserving the compact glass hierarchy. */
    .tabs { display: flex; flex-direction: column; gap: 4px; }
    .tabs .section { flex: 0 0 auto; }
    .nav { flex: 0 0 40px; }
    .nav .nav-icon { width: 28px; height: 28px; flex: 0 0 28px; display: grid; place-items: center; border: 1px solid rgba(177, 192, 224, .12); border-radius: 8px; background: rgba(39, 45, 63, .5); color: #9fb5ff; font: 15px/1 "Segoe UI Symbol", "Microsoft YaHei", sans-serif; transition: background .14s ease, border-color .14s ease, color .14s ease, transform .14s ease; }
    .nav:hover .nav-icon { background: rgba(100, 128, 220, .24); border-color: rgba(135, 160, 255, .35); color: #dce5ff; transform: scale(1.04); }
    .nav.on .nav-icon { background: linear-gradient(145deg, #6b8eff, #4e6ed6); border-color: rgba(177, 198, 255, .72); color: #fff; box-shadow: 0 5px 14px rgba(64, 98, 211, .25); }
    .nav-label { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .body { scroll-padding: 24px; }
    .grid { align-items: start; grid-auto-rows: min-content; }
    .box { height: 100%; }
    .box.wide { height: auto; }
    .box h3 { display: flex; align-items: center; min-height: 20px; }
    .row > input[type="search"] { flex: 1 1 220px; width: auto; min-width: 0; }
    .row > .hint { flex: 1 1 160px; min-width: 0; }
    .actions, .presets { align-items: center; }
    button.action, button.preset { font-weight: 500; white-space: nowrap; }
    .items, .bag { box-shadow: inset 0 1px rgba(255,255,255,.025); }
    .item, .bagrow { transition: background .12s ease, color .12s ease; }
    .item.sel { box-shadow: inset 2px 0 #6d91ff; }
    .attribute.head { position: sticky; top: 0; z-index: 1; background: rgba(15, 18, 28, .92); backdrop-filter: blur(12px); }
    .buff-toolbar { position: sticky; top: -24px; z-index: 2; }
    @media (max-width: 760px) {
      .nl { width: calc(100vw - 20px); height: calc(100vh - 20px); min-height: 420px; }
      .sidebar { width: 184px; flex-basis: 184px; }
      .brand { padding-left: 16px; }
      .body { padding: 18px; }
      .grid { grid-template-columns: minmax(0, 1fr); gap: 14px; }
      .box.wide { grid-column: auto; }
    }
    @media (max-width: 560px) {
      .sidebar { width: 158px; flex-basis: 158px; }
      .brand { font-size: 16px; padding-left: 12px; }
      .nav { padding: 0 8px; gap: 8px; }
      .nav .nav-icon { width: 25px; height: 25px; flex-basis: 25px; font-size: 13px; }
      .top { padding: 0 14px; }
      .body { padding: 14px; }
      .box { padding: 14px; }
      .profile { padding-left: 10px; padding-right: 10px; }
    }
    /* Compact sidebar and low-cost compositing keep every navigation entry visible. */
    .brand { height: 56px; padding: 18px 16px 0; }
    .tabs { padding: 8px 10px 6px; gap: 0; }
    .section { margin: 6px 8px 2px; line-height: 14px; }
    .nav { flex-basis: 30px; height: 30px; padding: 0 9px; gap: 9px; }
    .nav .nav-icon { width: 23px; height: 23px; flex-basis: 23px; font-size: 13px; border-radius: 7px; }
    .profile { min-height: 56px; padding: 8px 12px; }
    .avatar { width: 31px; height: 31px; }
    .nl { contain: layout style; isolation: isolate; }
    .sidebar, .box, .buff-toolbar, .settings { backdrop-filter: none; -webkit-backdrop-filter: none; }
    .pane.on { animation: enter .12s cubic-bezier(.2,.75,.25,1); }
    @media (max-width: 560px) {
      .brand { height: 52px; padding-top: 16px; }
      .tabs { padding: 6px 8px 4px; }
      .section { margin-top: 5px; }
      .nav { flex-basis: 29px; height: 29px; padding-left: 7px; }
      .nav .nav-icon { width: 22px; height: 22px; flex-basis: 22px; }
      .profile { min-height: 52px; padding-top: 6px; padding-bottom: 6px; }
      .avatar { width: 29px; height: 29px; }
    }
    /* Keep the navigation readable on desktop while using the sidebar space. */
    .brand { height: 70px; padding: 23px 20px 0; }
    .tabs { padding: 12px 12px 10px; }
    .section { margin: 9px 10px 4px; }
    .nav { flex-basis: 38px; height: 38px; padding: 0 12px; gap: 11px; }
    .nav .nav-icon { width: 28px; height: 28px; flex-basis: 28px; font-size: 15px; border-radius: 8px; }
    .profile { min-height: 72px; padding: 10px 14px; }
    .avatar { width: 34px; height: 34px; }
    #save-memory { position: relative; font-size: 0; }
    #save-memory::before { content: ''; position: absolute; left: 9px; top: 8px; width: 15px; height: 17px; border: 1.7px solid currentColor; border-radius: 3px; box-shadow: inset 0 5px 0 rgba(255,255,255,.08); }
    #save-memory::after { content: ''; position: absolute; left: 12px; top: 17px; width: 9px; height: 6px; border: 1.7px solid currentColor; border-radius: 1px 1px 2px 2px; background: transparent; }
    .settings .visual-row { margin-top: 9px; }
    .settings .visual-row label { min-width: 0; flex: 1; }
    .settings .visual-row select { width: 112px; }
    .nl.solid-bg { background: #0b0e17; backdrop-filter: none; -webkit-backdrop-filter: none; }
    .nl.solid-bg .sidebar { background: #101522; }
    .nl.solid-bg .main { background: #0c0f18; }
    .nl.solid-bg .box { background: #121725; }
    .nl.theme-light { color: #1c2638; background: rgba(245, 247, 251, .38); border-color: rgba(73, 91, 125, .26); }
    .nl.theme-light .sidebar { background: rgba(235, 239, 247, .28); border-right-color: rgba(73, 91, 125, .18); }
    .nl.theme-light .main { background: rgba(249, 250, 253, .16); }
    .nl.theme-light .brand span, .nl.theme-light .title, .nl.theme-light .box h3 { color: #1b2638; }
    .nl.theme-light .brand b { color: #3f6fdb; }
    .nl.theme-light .section { color: #78849a; }
    .nl.theme-light .nav { color: #59667b; }
    .nl.theme-light .nav:hover { background: rgba(92, 122, 205, .13); color: #1e2b42; }
    .nl.theme-light .nav.on { background: linear-gradient(90deg, rgba(96, 130, 224, .25), rgba(96, 130, 224, .1)); color: #1c315f; box-shadow: inset 2px 0 #4c72db, inset 0 1px rgba(255,255,255,.75); }
    .nl.theme-light .nav .nav-icon { background: rgba(219, 227, 245, .85); border-color: rgba(78, 108, 184, .2); color: #4c6fc9; }
    .nl.theme-light .nav.on .nav-icon { background: linear-gradient(145deg, #6f92ed, #4d70d4); color: #fff; }
    .nl.theme-light .top { border-bottom-color: rgba(73, 91, 125, .18); }
    .nl.theme-light .status { color: #67758d; }
    .nl.theme-light .status.ready { color: #218a58; }
    .nl.theme-light .box { background: rgba(255, 255, 255, .30); border-color: rgba(73, 91, 125, .18); box-shadow: inset 0 1px rgba(255,255,255,.8), 0 9px 28px rgba(80, 96, 125, .1); }
    .nl.theme-light .row, .nl.theme-light .row label { color: #66748a; }
    .nl.theme-light .row .value { color: #1f2b40; }
    .nl.theme-light input, .nl.theme-light select { background: rgba(244, 247, 252, .9); border-color: rgba(73, 91, 125, .23); color: #1f2b40; }
    .nl.theme-light button.action, .nl.theme-light button.preset { background: rgba(232, 237, 248, .9); border-color: rgba(73, 91, 125, .2); color: #35445c; }
    .nl.theme-light button.action:hover, .nl.theme-light button.preset:hover { background: rgba(201, 214, 246, .9); color: #1d315c; }
    .nl.theme-light .hint { color: #748198; }
    .nl.theme-light .items, .nl.theme-light .bag { background: rgba(243, 246, 251, .82); border-color: rgba(73, 91, 125, .18); }
    .nl.theme-light .item, .nl.theme-light .bagrow { color: #344158; border-bottom-color: rgba(73, 91, 125, .1); }
    .nl.theme-light .item:hover, .nl.theme-light .item.sel { background: rgba(105, 137, 224, .14); }
    .nl.theme-light .buff-toolbar { background: rgba(239, 243, 250, .26); border-color: rgba(73, 91, 125, .18); }
    .nl.theme-light .profile { border-top-color: rgba(73, 91, 125, .18); }
    .nl.theme-light .profile strong { color: #29364c; }
    .nl.theme-light .profile small { color: #718098; }
    .nl.theme-light .settings { background: rgba(255, 255, 255, .58); border-color: rgba(73, 91, 125, .22); }
    .nl.theme-light .settings h4 { color: #1f2b40; }
    .nl.theme-light.solid-bg { background: #f2f5fa; }
    .nl.theme-light.solid-bg .sidebar { background: #e9eef6; }
    .nl.theme-light.solid-bg .main { background: #f2f5fa; }
    .nl.theme-light.solid-bg .box { background: #ffffff; }
    /* Buff controls stay in normal flow; sticky positioning was covering the list while scrolling. */
    .buff-toolbar { position: relative; top: auto; z-index: auto; flex-wrap: wrap; overflow: visible; }
    .buff-toolbar .buff-caption { min-width: 96px; }
    .buff-subnav { margin-left: auto; }
    .attribute.head { position: static; z-index: auto; background: transparent; backdrop-filter: none; -webkit-backdrop-filter: none; }
    .nl.theme-light .attribute.head { background: transparent; }
    /* Medium navigation size: enough room for the profile while keeping About visible. */
    .brand { height: 64px; padding: 21px 18px 0; }
    .tabs { padding: 8px 12px 8px; gap: 0; }
    .section { margin: 7px 10px 3px; }
    .nav { flex-basis: 34px; height: 34px; padding: 0 11px; gap: 10px; }
    .nav .nav-icon { width: 26px; height: 26px; flex-basis: 26px; font-size: 14px; }
    .profile { min-height: 66px; padding: 9px 14px; }
    .avatar { width: 32px; height: 32px; }
    @media (max-width: 560px) {
      .brand { height: 56px; padding: 18px 14px 0; }
      .tabs { padding: 8px 8px 6px; }
      .section { margin-top: 6px; }
      .nav { flex-basis: 32px; height: 32px; padding: 0 8px; gap: 8px; }
      .nav .nav-icon { width: 24px; height: 24px; flex-basis: 24px; font-size: 13px; }
      .profile { min-height: 58px; padding: 8px 10px; }
      .avatar { width: 30px; height: 30px; }
    }
  </style>
  <div class="nl" id="root">
    <aside class="sidebar">
      <div class="brand" id="drag"><span>生存日志</span><b>修改器</b></div>
      <div class="tabs">
        <div class="section">资源</div>
        <button class="nav on" data-tab="prepare"><i class="nav-icon" aria-hidden="true">◷</i><span class="nav-label">准备阶段</span></button>
        <button class="nav" data-tab="items"><i class="nav-icon" aria-hidden="true">▤</i><span class="nav-label">物品</span></button>
        <button class="nav" data-tab="bag"><i class="nav-icon" aria-hidden="true">▣</i><span class="nav-label">背包</span></button>
        <button class="nav" data-tab="resources"><i class="nav-icon" aria-hidden="true">◆</i><span class="nav-label">杂项</span></button>
        <div class="section">生存</div>
        <button class="nav" data-tab="attributes"><i class="nav-icon" aria-hidden="true">♙</i><span class="nav-label">属性</span></button>
        <button class="nav" data-tab="proficiency"><i class="nav-icon" aria-hidden="true">✦</i><span class="nav-label">熟练度</span></button>
        <button class="nav" data-tab="facilities"><i class="nav-icon" aria-hidden="true">⌂</i><span class="nav-label">设施</span></button>
        <button class="nav" data-tab="buffs"><i class="nav-icon" aria-hidden="true">✚</i><span class="nav-label">Buff</span></button>
        <div class="section">其他</div>
        <button class="nav" data-tab="about"><i class="nav-icon" aria-hidden="true">ⓘ</i><span class="nav-label">关于</span></button>
      </div>
    </aside>
    <main class="main">
      <header class="top" id="top-drag"><div class="title" id="page-title">准备阶段</div><div class="buff-header-actions" id="buff-header-actions"><button class="action" id="remove-negative-buffs">移除负面效果</button><button class="action danger" id="clear-all-buffs">清除全部 Buff</button></div><div class="status" id="state">连接中</div><button class="icon" id="save-memory" title="保存当前设置">&#128190;</button><button class="icon" id="settings" title="设置">&#9881;</button><button class="icon" id="minimize" title="最小化">&#8722;</button></header>
      <section class="body">
        <div class="pane on" data-pane="prepare"><div class="grid">
          <div class="box"><h3>钱包</h3><div class="row"><label>当前余额</label><span class="value" id="gold">--</span></div><div class="hint">载入存档后会自动刷新显示的余额。</div></div>
          <div class="box"><h3>修改货币</h3><div class="row"><label>数量</label><input id="money-value" type="number" min="0" value="100000"></div><div class="row"><button class="action primary" id="add-money">增加</button><button class="action" id="set-money">设为指定值</button></div><div class="presets"><button class="preset" data-money="1000">1千</button><button class="preset" data-money="10000">1万</button><button class="preset" data-money="100000">10万</button><button class="preset" data-money="1000000">100万</button></div></div>
          <div class="box"><h3>世界时钟</h3><div class="row"><label>当前时间</label><span class="value" id="clock">--</span></div><div class="row"><label>阶段</label><span class="value" id="phase">--</span></div><div class="row"><label>倒计时</label><span class="value" id="remaining">--</span></div></div>
          <div class="box"><h3>时间控制</h3><div class="presets"><button class="preset" data-hours="1">+1 小时</button><button class="preset" data-hours="3">+3 小时</button><button class="preset" data-hours="5">+5 小时</button><button class="preset" data-hours="10">+10 小时</button></div><div class="row"><label class="switch"><input id="freeze" type="checkbox"><span></span>冻结游戏时间</label></div><div class="hint">倒计时延长仅在准备阶段生效。</div></div>
        </div></div>
        <div class="pane" data-pane="items"><div class="grid">
          <div class="box wide"><h3>物品浏览</h3><div class="row"><input id="item-search" type="search" placeholder="按名称、编号或拼音搜索"><button class="action" id="paste">粘贴</button></div><div class="chips" id="top-cats"></div><div class="chips" id="cats"></div><div class="items" id="item-list"></div><div class="row"><label>数量</label><input id="item-count" type="number" min="1" max="999" value="10"><button class="preset" data-count="1">1</button><button class="preset" data-count="10">10</button><button class="preset" data-count="100">100</button><button class="preset" data-count="999">999</button><button class="action primary" id="add-item">加入背包</button></div><div class="hint" id="item-hint">请选择一个物品后再加入。</div></div>
        </div></div>
        <div class="pane" data-pane="bag"><div class="grid"><div class="box"><h3>背包扩容</h3><div class="row"><label>当前尺寸</label><span class="value" id="bag-size">--</span></div><div class="field-grid"><label for="bag-columns">列数</label><input id="bag-columns" type="number" min="1" max="20"><label for="bag-rows">行数</label><input id="bag-rows" type="number" min="1" max="20"></div><div class="actions"><button class="action primary" id="set-bag-size">应用尺寸</button><button class="action" id="reset-bag-size">恢复原始</button></div><div class="hint">可设置 1 到 20；添加物品空间不足时会自动扩展，恢复原始前需将扩展区域内物品移走。</div></div><div class="box"><h3>负重</h3><div class="row"><label>当前负重</label><span class="value" id="bag-weight">--</span></div><div class="row"><label>最大负重</label><span class="value" id="bag-max-burden">--</span></div><div class="row"><label for="bag-burden-value">目标值</label><input id="bag-burden-value" type="number" min="0"></div><div class="actions"><button class="action primary" id="set-bag-burden">应用负重</button><button class="action" id="reset-bag-burden">恢复原始</button></div></div><div class="box wide"><h3>背包物品</h3><div class="row"><button class="action" id="refresh-bag">刷新</button><span class="hint">可修改数量、复制或删除背包内的物品。</span></div><div class="bag" id="bag-list"></div></div></div></div>
        <div class="pane" data-pane="resources"><div class="grid"><div class="box"><h3>暴露值</h3><div class="row"><label>当前/上限</label><span class="value" id="exposure">--</span></div><div class="row"><label>目标值</label><input id="exposure-value" type="number" min="0" step="1"><button class="action primary" id="set-exposure">设置</button></div><div class="hint">直接修改探索管理器的当前暴露值，数值会限制在当前上限内。</div></div><div class="box"><h3>生存点</h3><div class="row"><label>当前数量</label><span class="value" id="survival-points">--</span></div><div class="row"><label>目标值</label><input id="survival-points-value" type="number" min="0"><button class="action primary" id="set-survival-points">设置</button></div><div class="hint">使用游戏原生生存点接口写入，切换存档后请刷新。</div></div><div class="box"><h3>图鉴</h3><div class="actions"><button class="action primary" id="unlock-all-codex">解锁全图鉴</button></div><div class="hint">调用游戏原生图鉴管理器，解锁当前存档中的全部图鉴条目。</div></div><div class="box"><h3>成就</h3><div class="actions"><button class="action primary" id="unlock-all-achievements">解锁全成就</button></div><div class="hint">调用游戏原生成就接口并同步平台成就，操作不可撤销。</div></div></div></div>
        <div class="pane legacy-time" data-pane="legacy-time"><div class="grid"></div></div>
        <div class="pane" data-pane="attributes"><div class="grid"><div class="box wide"><h3>角色状态</h3><div id="attributes"></div><div class="hint">右侧开关用于锁定属性，锁定后会自动恢复到输入的目标值。</div></div><div class="box"><h3>邻居好感度</h3><div class="row"><label>名称</label><span class="value" id="companion">--</span></div><div class="row"><label>好感度</label><span class="value" id="affinity">--</span></div><div class="row"><label>设置好感</label><input id="relationship-value" type="number" min="0"><button class="action" id="set-relationship">设置</button></div><div class="row"><button class="action" id="refresh-relationship">刷新</button></div></div><div class="box"><h3>移动速度</h3><div class="row"><label>当前速度</label><span class="value" id="move-speed-current">--</span></div><div class="row"><label>速度倍率</label><input id="move-speed-multiplier" type="number" min="0.5" max="5" step="0.1" value="1"><button class="action primary" id="set-move-speed">应用</button></div><div class="presets"><button class="preset" data-speed="0.5">0.5x</button><button class="preset" data-speed="1">1x</button><button class="preset" data-speed="1.5">1.5x</button><button class="preset" data-speed="2">2x</button><button class="preset" data-speed="3">3x</button><button class="preset" data-speed="5">5x</button></div><div class="actions"><button class="action" id="reset-move-speed">恢复原始</button></div><div class="hint">倍率以进入当前角色时的原始基础速度计算，不会重复叠加。</div></div></div></div>
        <div class="pane" data-pane="proficiency"><div class="grid"><div class="box wide"><h3>技能熟练度</h3><div id="proficiency-list"><div class="hint">进入存档后读取熟练度。</div></div><div class="actions"><button class="action" id="refresh-proficiency">刷新</button></div><div class="hint">增加经验会使用游戏原生升级逻辑；“升一级”会触发对应等级效果。最高等级为 Lv 5，不支持降级。</div></div></div></div>
        <div class="pane" data-pane="facilities"><div class="grid"><div class="box"><h3>住宅门耐久</h3><div class="row"><label>当前状态</label><span class="value" id="door-durability">--</span></div><div class="row"><label>目标耐久</label><input id="door-durability-value" type="number" min="1" max="100000000" value="10000"></div><div class="actions"><button class="action primary" id="set-door-durability">应用到全部门</button></div><div class="hint">同时修改已建造住宅门的当前耐久和耐久上限。</div></div><div class="box"><h3>住宅窗耐久</h3><div class="row"><label>当前状态</label><span class="value" id="window-durability">--</span></div><div class="row"><label>目标耐久</label><input id="window-durability-value" type="number" min="1" max="100000000" value="10000"></div><div class="actions"><button class="action primary" id="set-window-durability">应用到全部窗</button></div><div class="hint">同时修改已建造住宅窗的当前耐久和耐久上限。</div></div><div class="box wide"><h3>食物保质期</h3><div class="row"><label class="switch"><input id="infinite-food" type="checkbox"><span></span>无限食物保质期</label></div><div class="hint">开启后持续冻结可腐败食物的计时；关闭后由游戏重新计算背包、箱子和冰箱的正常腐败速度。</div></div></div></div>
        <div class="pane" data-pane="buffs"><div class="grid"><div class="box"><h3>添加 Buff</h3><div class="row"><label>配置 ID</label><input id="buff-config-id" type="number" min="1"><button class="action primary" id="add-buff">添加</button></div><div class="row"><button class="action" id="refresh-buffs">刷新</button></div><div class="hint">填写游戏 Buff 配置表中的 ID。已有 Buff 会显示在右侧。</div></div><div class="box wide"><h3>当前 Buff</h3><div id="buff-list"><div class="hint">进入存档后读取。</div></div></div></div></div>
        <div class="pane" data-pane="about"><div class="about-content"><div class="support-message">如果您觉得好用的话可以支持一下，您的鼓励是我源源不断更新的动力，谢谢！</div></div></div>
      </section>
      <div class="settings" id="settings-panel"><h4>界面设置</h4><div class="logo"><span>生存日志</span><b>修改器</b></div><div class="row scale-row"><label for="ui-scale">界面缩放</label><select id="ui-scale"><option value="70">70%</option><option value="75">75%</option><option value="80">80%</option><option value="85">85%</option><option value="90">90%</option><option value="95">95%</option><option value="100" selected>100%</option><option value="105">105%</option><option value="110">110%</option><option value="115">115%</option><option value="120">120%</option><option value="125">125%</option></select></div><div class="row fps-row"><label for="ui-fps">修改器帧率</label><select id="ui-fps"><option value="0">跟随游戏帧数</option><option value="60">60 帧</option><option value="90">90 帧</option><option value="120">120 帧</option><option value="144">144 帧</option><option value="240">240 帧</option></select></div><div class="hint fps-hint">此项设置 Chromium 的目标更新率，最终显示帧数仍受游戏帧率和显示器刷新率限制。当前界面已初始化时，重启游戏后生效。</div></div>
      <div class="toast" id="toast"></div>
    </main>
  </div>`;

  var $ = function (id) { return shadow.getElementById(id); };
  var state = { activeTab: 'prepare', connected: null, memory: null, items: [], itemIndexVersion: 0, cats: [], tcats: [], top: -1, cat: -1, selected: -1, bag: [], bagInfo: {}, attributes: {}, attributeSource: null, locks: {}, moveSpeed: {}, facilities: {}, time: {}, relationship: null, proficiency: [], resources: {}, buffs: [], buffConfigs: [], survivalPlans: [], survivalCatalog: [], buffFilter: 'all', buffView: 'current', buffScope: 'player', gold: -1, dragging: false };
  var titles = { prepare: '准备阶段', money: '准备阶段', items: '物品', bag: '背包', resources: '杂项', time: '准备阶段', attributes: '属性', proficiency: '熟练度', facilities: '设施', buffs: 'Buff', about: '关于' };

  function number(value, fallback) { var n = parseInt(value, 10); return isNaN(n) ? fallback : Math.max(0, n); }
  function readInteger(input, minimum, maximum, label) {
    var raw = String(input.value == null ? '' : input.value).trim();
    var value = Number(raw);
    if (!/^\d+$/.test(raw) || !Number.isSafeInteger(value) || value < minimum || value > maximum) {
      input.classList.add('invalid');
      input.focus();
      toast(label + '范围为 ' + minimum + ' 到 ' + maximum, true);
      return null;
    }
    input.classList.remove('invalid');
    input.value = String(value);
    return value;
  }
  function readDecimal(input, minimum, maximum, label) {
    var raw = String(input.value == null ? '' : input.value).trim();
    var value = Number(raw);
    if (!/^(?:\d+(?:\.\d*)?|\.\d+)$/.test(raw) || !Number.isFinite(value) || value < minimum || value > maximum) {
      input.classList.add('invalid');
      input.focus();
      toast(label + '范围为 ' + minimum + ' 到 ' + maximum, true);
      return null;
    }
    value = Math.round(value * 100) / 100;
    input.classList.remove('invalid');
    input.value = String(value);
    return value;
  }
  function syncInput(input, value) {
    if (!input || shadow.activeElement === input || input.dataset.dirty === '1') return;
    input.value = value == null ? '' : String(value);
  }
  function commitInput(input) { if (input) input.dataset.dirty = '0'; }
  function bindEnter(input, button) { input.addEventListener('keydown', function (event) { if (event.key === 'Enter') { event.preventDefault(); button.click(); } }); }
  function prepareNumberInput(input) {
    if (!input || input.dataset.enhanced) return;
    input.dataset.enhanced = '1';
    input.addEventListener('input', function () { input.dataset.dirty = '1'; input.classList.remove('invalid'); });
    input.addEventListener('wheel', function (event) { if (shadow.activeElement === input) event.preventDefault(); }, { passive: false });
  }
  function escapeHtml(value) { return String(value == null ? '' : value).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;'); }
  function pinyin(name) { try { var words = window.pinyinPro.pinyin(name, { toneType: 'none' }) || ''; return { full: words.replace(/\s+/g, '').toLowerCase(), short: words.split(/\s+/).map(function (word) { return word.charAt(0); }).join('').toLowerCase() }; } catch (_) { return { full: '', short: '' }; } }
  var buffPinyinCache = Object.create(null);
  function prepareBuffSearch(items) {
    (items || []).forEach(function (item) {
      var name = String(item.name || '');
      if (!buffPinyinCache[name]) buffPinyinCache[name] = pinyin(name);
      item.py = buffPinyinCache[name];
    });
    return items || [];
  }
  function buffSearchMatch(item, query) {
    query = String(query || '').replace(/\s+/g, '').toLowerCase();
    if (!query) return true;
    var name = String(item.name || '').toLowerCase();
    var py = item.py || { full: '', short: '' };
    return name.indexOf(query) >= 0 || py.full.indexOf(query) >= 0 || py.short.indexOf(query) >= 0;
  }
  function scheduleIdle(fn) {
    if (typeof window.requestIdleCallback === 'function') window.requestIdleCallback(fn, { timeout: 1200 });
    else setTimeout(fn, 0);
  }
  function buildPinyinIndex(items, version, start) {
    if (version !== state.itemIndexVersion) return;
    if (state.dragging) { scheduleIdle(function () { buildPinyinIndex(items, version, start); }); return; }
    var end = Math.min(start + 64, items.length);
    for (var i = start; i < end; i++) items[i].py = pinyin(items[i].name);
    if (end < items.length) scheduleIdle(function () { buildPinyinIndex(items, version, end); });
    else if ($('item-search').value.trim()) renderItems();
  }
  function toast(message, error) { var target = $('toast'); target.textContent = message || (error ? '操作失败' : '操作完成'); target.className = 'toast show' + (error ? ' error' : ''); clearTimeout(toast.timer); toast.timer = setTimeout(function () { target.className = 'toast'; }, 3200); }
  function formatGold(value) { return value < 0 ? '--' : value.toLocaleString(); }

	function setTab(tab) {
    state.activeTab = tab;
    shadow.querySelectorAll('.nav').forEach(function (node) { node.classList.toggle('on', node.dataset.tab === tab); });
    shadow.querySelectorAll('.pane').forEach(function (node) { node.classList.toggle('on', node.dataset.pane === tab); });
		$('page-title').textContent = titles[tab];
		$('buff-header-actions').classList.toggle('show', tab === 'buffs');
    send({ cmd: 'setActiveTab', tab: tab });
    if (tab === 'items' && !state.items.length) send({ cmd: 'getItems' });
    if (tab === 'bag') send({ cmd: 'getBag' });
    if (tab === 'proficiency') send({ cmd: 'getProficiency' });
  }
  shadow.querySelectorAll('.nav').forEach(function (node) { node.addEventListener('click', function () { setTab(node.dataset.tab); }); });

  function renderCategories() {
    function createCategory(container, selected, data, callback) {
      container.innerHTML = '';
      var fragment = document.createDocumentFragment();
      var all = document.createElement('button'); all.className = 'chip' + (selected === -1 ? ' on' : ''); all.textContent = '全部'; all.onclick = function () { callback(-1); }; fragment.appendChild(all);
      data.forEach(function (entry) { var button = document.createElement('button'); button.className = 'chip' + (selected === entry.id ? ' on' : ''); button.textContent = entry.name; button.onclick = function () { callback(entry.id); }; fragment.appendChild(button); });
      container.appendChild(fragment);
    }
    createCategory($('top-cats'), state.top, state.tcats, function (id) { state.top = id; state.cat = -1; renderCategories(); renderItems(); });
    var available = state.cats;
    if (state.top >= 0) {
      var availableIds = Object.create(null);
      state.items.forEach(function (item) { if (item.cid === state.top) availableIds[item.cat] = true; });
      available = state.cats.filter(function (cat) { return !!availableIds[cat.id]; });
    }
    createCategory($('cats'), state.cat, available, function (id) { state.cat = id; renderCategories(); renderItems(); });
  }
  function renderItems() {
    var query = $('item-search').value.trim().toLowerCase();
    var items = state.items.filter(function (item) { var py = item.py || { full: '', short: '' }; return (state.top < 0 || item.cid === state.top) && (state.cat < 0 || item.cat === state.cat) && (!query || item.name.toLowerCase().indexOf(query) >= 0 || String(item.id).indexOf(query) >= 0 || py.full.indexOf(query) >= 0 || py.short.indexOf(query) >= 0); });
    var container = $('item-list'); container.innerHTML = '';
    if (!items.length) { container.innerHTML = '<div class="hint" style="padding:10px">没有匹配的物品。</div>'; return; }
    var fragment = document.createDocumentFragment();
    items.slice(0, 200).forEach(function (item) { var row = document.createElement('div'); row.className = 'item' + (item.id === state.selected ? ' sel' : ''); row.innerHTML = '<span class="name">' + escapeHtml(item.name) + '</span><span class="id">#' + item.id + '</span>'; row.onclick = function () { var old = container.querySelector('.item.sel'); if (old) old.classList.remove('sel'); row.classList.add('sel'); state.selected = item.id; $('item-hint').textContent = '已选择：' + item.name + ' (#' + item.id + ')'; }; fragment.appendChild(row); });
    if (items.length > 200) { var more = document.createElement('div'); more.className = 'hint'; more.style.padding = '8px 10px'; more.textContent = '仅显示前 200 项，请细化搜索条件。'; fragment.appendChild(more); }
    container.appendChild(fragment);
  }
  function renderBag() {
    var container = $('bag-list'); container.innerHTML = '';
    if (!state.bag.length) { container.innerHTML = '<div class="hint" style="padding:10px">背包为空或尚未加载。</div>'; return; }
    var fragment = document.createDocumentFragment();
    state.bag.forEach(function (item) { var row = document.createElement('div'); row.className = 'bagrow'; row.innerHTML = '<span class="name">' + escapeHtml(item.name || '未知物品') + '</span><span class="id">#' + item.id + '</span><input type="number" min="1" max="999999" value="' + item.count + '"><button class="action save">设置</button><button class="action duplicate">复制</button><button class="action danger remove">删除</button>'; var input = row.querySelector('input'); var save = row.querySelector('.save'); prepareNumberInput(input); save.onclick = function () { var count = readInteger(input, 1, 999999, '物品数量'); if (count !== null) send({ cmd: 'setBagCount', instanceId: item.id, count: count }); }; bindEnter(input, save); row.querySelector('.duplicate').onclick = function () { send({ cmd: 'dupBag', instanceId: item.id }); }; row.querySelector('.remove').onclick = function () { send({ cmd: 'removeBag', instanceId: item.id }); }; fragment.appendChild(row); });
    container.appendChild(fragment);
  }
  function renderBagInfo() {
    var info = state.bagInfo || {};
    $('bag-size').textContent = info.ready ? info.columns + ' x ' + info.rows : '--';
    $('bag-weight').textContent = info.ready ? info.weight : '--';
    $('bag-max-burden').textContent = info.ready ? info.maxBurden : '--';
    if (info.ready) {
      syncInput($('bag-columns'), info.columns);
      syncInput($('bag-rows'), info.rows);
      syncInput($('bag-burden-value'), info.maxBurden);
    }
  }
  function attributeInputValue(key) {
    var input = $('attributes').querySelector('[data-attr="' + key + '"] .current-input');
    return input ? number(input.value, -1) : -1;
  }
  function attributeMaxInputValue(key) {
    var input = $('attributes').querySelector('[data-attr="' + key + '"] .max-input');
    return input ? number(input.value, -1) : -1;
  }
  function memoryLockValue(key) {
    var memory = state.memory || null;
    var values = memory && memory.lockValues ? memory.lockValues : null;
    var value = values ? values[key] : null;
    if (!(typeof value === 'number' && value >= 0) && memory) {
      var flatNames = { Health: 'lockHpValue', Stamina: 'lockStaminaValue', Satiety: 'lockSatietyValue', Morale: 'lockMoraleValue' };
      value = memory[flatNames[key]];
    }
    return typeof value === 'number' && value >= 0 ? value : null;
  }
  function memoryMaxValue(key) {
    var memory = state.memory || null;
    var values = memory && memory.maxValues ? memory.maxValues : null;
    var value = values ? values[key] : null;
    if (!(typeof value === 'number' && value > 0) && memory) {
      var flatNames = { Health: 'maxHealthValue', Stamina: 'maxStaminaValue', Satiety: 'maxSatietyValue', Morale: 'maxMoraleValue' };
      value = memory[flatNames[key]];
    }
    return typeof value === 'number' && value > 0 ? value : null;
  }
  function applyMemory(memory) {
    if (!memory) return;
    state.memory = memory;
    state.locks = {
      Health: !!memory.lockHp,
      Stamina: !!memory.lockStamina,
      Satiety: !!memory.lockSatiety,
      Morale: !!memory.lockMorale
    };
    if (typeof memory.infiniteFood === 'boolean') $('infinite-food').checked = memory.infiniteFood;
    if (typeof memory.noExploreExposure === 'boolean' && $('no-exposure')) $('no-exposure').checked = memory.noExploreExposure;
    if (typeof memory.timeFrozen === 'boolean') $('freeze').checked = memory.timeFrozen;
    if (typeof memory.moveSpeedMultiplier === 'number') $('move-speed-multiplier').value = String(memory.moveSpeedMultiplier);
    if (typeof memory.bagMaxBurden === 'number' && memory.bagMaxBurden > 0) { $('bag-burden-value').value = String(memory.bagMaxBurden); $('bag-burden-value').dataset.dirty = '0'; }
    ['Satiety', 'Morale', 'Stamina', 'Health'].forEach(function (key) { var value = memoryMaxValue(key), input = $('attributes').querySelector('[data-attr="' + key + '"] .max-input'); if (input && value != null) { input.value = String(value); input.dataset.initialized = '1'; input.dataset.dirty = '0'; } });
    renderAttributes();
    if (memory.activeTab === 'money' || memory.activeTab === 'time') memory.activeTab = 'prepare';
    if (memory.activeTab && titles[memory.activeTab] && memory.activeTab !== state.activeTab) setTab(memory.activeTab);
  }
  function collectMemory() {
    var lockValues = {};
    var maxValues = {};
    ['Satiety', 'Morale', 'Stamina', 'Health'].forEach(function (key) { lockValues[key] = attributeInputValue(key); });
    ['Satiety', 'Morale', 'Stamina', 'Health'].forEach(function (key) { maxValues[key] = attributeMaxInputValue(key); });
    var multiplier = parseFloat($('move-speed-multiplier').value);
    if (!Number.isFinite(multiplier)) multiplier = 1;
    return {
      activeTab: state.activeTab,
      lockHp: !!state.locks.Health,
      lockStamina: !!state.locks.Stamina,
      lockSatiety: !!state.locks.Satiety,
      lockMorale: !!state.locks.Morale,
      lockValues: lockValues,
      maxValues: maxValues,
      infiniteFood: $('infinite-food').checked,
      noExploreExposure: !!($('no-exposure') && $('no-exposure').checked),
      timeFrozen: $('freeze').checked,
      moveSpeedMultiplier: multiplier,
      bagColumns: number($('bag-columns').value, 0),
      bagRows: number($('bag-rows').value, 0),
      bagMaxBurden: number($('bag-burden-value').value, 0)
    };
  }
  function sendLocks() {
    send({ cmd: 'setLocks', hp: !!state.locks.Health, stamina: !!state.locks.Stamina, satiety: !!state.locks.Satiety, morale: !!state.locks.Morale,
      hpValue: attributeInputValue('Health'), staminaValue: attributeInputValue('Stamina'), satietyValue: attributeInputValue('Satiety'), moraleValue: attributeInputValue('Morale') });
  }
  function applyAttributePayload(data) {
    if (state.attributeSource !== null && data.source !== state.attributeSource) {
      $('attributes').querySelectorAll('input').forEach(function (input) { delete input.dataset.initialized; input.dataset.dirty = '0'; });
    }
    state.attributeSource = data.source;
    state.attributes = data.attrs || {};
    renderAttributes();
  }
  function renderAttributes() {
    var container = $('attributes');
    var pairs = [['Satiety', '饱腹'], ['Morale', '心态'], ['Stamina', '精力'], ['Health', '生命']];
    if (!container.dataset.ready) {
      var fragment = document.createDocumentFragment();
      var head = document.createElement('div'); head.className = 'attribute head'; head.innerHTML = '<span>属性</span><span>当前/上限</span><span>当前值</span><span></span><span>上限值</span><span></span><span>锁定</span>'; fragment.appendChild(head);
      pairs.forEach(function (pair) { var key = pair[0], row = document.createElement('div'); row.className = 'attribute'; row.dataset.attr = key; row.innerHTML = '<span>' + pair[1] + '</span><span class="value">--</span><input class="current-input" type="number" min="0" max="1000000"><button class="action current-set">设</button><input class="max-input" type="number" min="1" max="1000000"><button class="action max-set">设</button><label class="switch"><input type="checkbox"><span></span></label>'; var currentInput = row.querySelector('.current-input'), maxInput = row.querySelector('.max-input'), currentButton = row.querySelector('.current-set'), maxButton = row.querySelector('.max-set'); prepareNumberInput(currentInput); prepareNumberInput(maxInput); currentButton.onclick = function () { var value = readInteger(currentInput, 0, 1000000, pair[1] + '当前值'); if (value !== null) { commitInput(currentInput); send({ cmd: 'setAttr', a: key, v: value }); if (state.locks[key]) sendLocks(); } }; maxButton.onclick = function () { var value = readInteger(maxInput, 1, 1000000, pair[1] + '上限值'); if (value !== null) { commitInput(maxInput); send({ cmd: 'setAttrMax', a: key, v: value }); } }; bindEnter(currentInput, currentButton); bindEnter(maxInput, maxButton); row.querySelector('input[type=checkbox]').onchange = function (event) { state.locks[key] = event.target.checked; sendLocks(); }; fragment.appendChild(row); });
      container.appendChild(fragment); container.dataset.ready = '1';
    }
    pairs.forEach(function (pair) { var key = pair[0], entry = state.attributes[key] || {}, row = container.querySelector('[data-attr="' + key + '"]'), currentInput = row.querySelector('.current-input'), maxInput = row.querySelector('.max-input'), current = typeof entry.cur === 'number' ? entry.cur : null, maximum = typeof entry.max === 'number' && entry.max > 0 ? entry.max : null, rememberedCurrent = memoryLockValue(key), rememberedMaximum = memoryMaxValue(key); row.querySelector('.value').textContent = current == null ? '--' : (maximum == null ? String(current) : current + '/' + maximum); if (!currentInput.dataset.initialized && (current != null || rememberedCurrent != null)) { currentInput.value = String(rememberedCurrent != null ? rememberedCurrent : current); currentInput.dataset.initialized = '1'; } if (!maxInput.dataset.initialized && (maximum != null || rememberedMaximum != null)) { maxInput.value = String(rememberedMaximum != null ? rememberedMaximum : maximum); maxInput.dataset.initialized = '1'; } row.querySelector('input[type=checkbox]').checked = !!state.locks[key]; });
  }
  function renderTime() { var time = state.time; $('clock').textContent = time.day >= 0 ? ('第 ' + time.day + ' 天，' + time.hour + ':00') : '--'; $('remaining').textContent = time.remainHour >= 0 ? time.remainHour + ' 小时' : '--'; $('phase').textContent = ({ CountDownTimer: '准备阶段', CountUpTimer: '生存阶段', FrozenTimer: '已冻结' })[time.timer] || time.timer || '--'; $('freeze').checked = !!time.frozen; }
  function renderRelationship() { var relation = state.relationship || {}, input = $('relationship-value'), button = $('set-relationship'), maximum = typeof relation.maxAffinity === 'number' && relation.maxAffinity > 0 ? relation.maxAffinity : 100; $('companion').textContent = relation.name || '--'; $('affinity').textContent = relation.affinity == null ? '--' : relation.affinity + '/' + maximum + (relation.tierName ? ' (' + relation.tierName + ')' : ''); input.max = String(maximum); button.disabled = !!relation.locked; button.title = relation.locked ? '邻居解锁后才能修改好感度' : ''; if (relation.affinity != null) syncInput(input, relation.affinity); }
  function renderMoveSpeed() { var speed = state.moveSpeed || {}; $('move-speed-current').textContent = speed.ready ? Number(speed.current).toFixed(2).replace(/\.?0+$/, '') : '--'; if (speed.ready) syncInput($('move-speed-multiplier'), speed.multiplier); }
  function renderFacilities() { var info = state.facilities || {}; $('door-durability').textContent = info.doorCount > 0 ? info.doorCurrent + '/' + info.doorMax + ' (' + info.doorCount + '扇)' : '--'; $('window-durability').textContent = info.windowCount > 0 ? info.windowCurrent + '/' + info.windowMax + ' (' + info.windowCount + '扇)' : '--'; $('infinite-food').checked = !!info.infiniteFood; }
  function renderResources() { var info = state.resources || {}; $('exposure').textContent = info.ready ? Number(info.exposure).toFixed(1).replace(/\.0$/, '') + '/' + info.maxExposure : '--'; $('survival-points').textContent = info.ready ? String(info.survivalPoints) : '--'; if ($('no-exposure')) $('no-exposure').checked = !!info.noExposure; if (info.ready) { syncInput($('exposure-value'), info.exposure); syncInput($('survival-points-value'), info.survivalPoints); } }
  function ensureExposureManagerUI() { if ($('no-exposure')) return; var input = $('exposure-value'), row = input && input.parentElement, box = row && row.parentElement; if (!row || !box) return; var title = box.querySelector('h3'); if (title) title.textContent = '防暴露'; var currentLabel = box.querySelector('.row label'); if (currentLabel) currentLabel.textContent = '当前暴露/触发阈值'; row.style.display = 'none'; var lockRow = document.createElement('div'); lockRow.className = 'row'; lockRow.innerHTML = '<label class="switch"><input id="no-exposure" type="checkbox"><span></span>不会暴露（锁定为 0）</label>'; box.insertBefore(lockRow, row); }
  function renderProficiencies() {
    var container = $('proficiency-list'); container.innerHTML = '';
    if (!state.proficiency.length) { container.innerHTML = '<div class="hint">熟练度尚未加载，请进入存档后刷新。</div>'; return; }
    var fragment = document.createDocumentFragment();
    state.proficiency.forEach(function (item) {
      var row = document.createElement('div'); row.className = 'proficiency';
      var maxed = item.level >= item.maxLevel;
      row.innerHTML = '<strong>' + escapeHtml(item.name || '未知系统') + '</strong><span class="value">Lv ' + item.level + '</span><span class="exp">' + (maxed ? '已满级' : item.exp + ' / ' + item.nextLevelExp) + '</span><input type="number" min="1" max="100000000" value="500"' + (maxed ? ' disabled' : '') + '><button class="action add-exp"' + (maxed ? ' disabled' : '') + '>加经验</button><button class="action primary add-level"' + (maxed ? ' disabled' : '') + '>升一级</button>';
      var input = row.querySelector('input'), expButton = row.querySelector('.add-exp'), levelButton = row.querySelector('.add-level');
      prepareNumberInput(input);
      expButton.onclick = function () { var amount = readInteger(input, 1, 100000000, '熟练度经验'); if (amount !== null) send({ cmd: 'addProficiencyExp', typeId: item.typeId, amount: amount }); };
      levelButton.onclick = function () { send({ cmd: 'addProficiencyLevel', typeId: item.typeId, levels: 1 }); };
      bindEnter(input, expButton); fragment.appendChild(row);
    });
    container.appendChild(fragment);
  }

  $('add-money').onclick = function () { var value = readInteger($('money-value'), 0, 2147483647, '金币数量'); if (value !== null) send({ cmd: 'addGold', amount: value }); };
  $('set-money').onclick = function () { var value = readInteger($('money-value'), 0, 2147483647, '金币数量'); if (value !== null) send({ cmd: 'setMoney', amount: value }); };
  shadow.querySelectorAll('[data-money]').forEach(function (button) { button.onclick = function () { $('money-value').value = button.dataset.money; }; });
  $('item-search').oninput = function () { clearTimeout(renderItems.timer); renderItems.timer = setTimeout(renderItems, 80); }; $('paste').onclick = function () { send({ cmd: 'getClipboard' }); };
  shadow.querySelectorAll('[data-count]').forEach(function (button) { button.onclick = function () { $('item-count').value = button.dataset.count; }; });
  $('add-item').onclick = function () { if (state.selected < 0) { toast('请先选择物品。', true); return; } var count = readInteger($('item-count'), 1, 999, '物品数量'); if (count !== null) send({ cmd: 'addItem', id: state.selected, count: count }); };
  $('refresh-bag').onclick = function () { commitInput($('bag-columns')); commitInput($('bag-rows')); commitInput($('bag-burden-value')); send({ cmd: 'getBag' }); send({ cmd: 'getBagInfo' }); };
  $('set-bag-size').onclick = function () { var columns = readInteger($('bag-columns'), 1, 20, '背包列数'); var rows = readInteger($('bag-rows'), 1, 20, '背包行数'); if (columns !== null && rows !== null) send({ cmd: 'setBagSize', columns: columns, rows: rows }); };
  $('reset-bag-size').onclick = function () { commitInput($('bag-columns')); commitInput($('bag-rows')); send({ cmd: 'resetBagSize' }); };
  $('set-bag-burden').onclick = function () { var value = readInteger($('bag-burden-value'), 0, 2147483647, '最大负重'); if (value === null) return; send({ cmd: 'setMaxBurden', value: value }); };
  $('reset-bag-burden').onclick = function () { commitInput($('bag-burden-value')); send({ cmd: 'resetMaxBurden' }); };
  shadow.querySelectorAll('[data-hours]').forEach(function (button) { button.onclick = function () { send({ cmd: 'extendTime', hours: Number(button.dataset.hours) }); }; });
  $('freeze').onchange = function () { send({ cmd: 'setTimeFrozen', on: $('freeze').checked }); };
  $('refresh-relationship').onclick = function () { commitInput($('relationship-value')); send({ cmd: 'getRelationship' }); }; $('set-relationship').onclick = function () { var maximum = state.relationship && state.relationship.maxAffinity > 0 ? state.relationship.maxAffinity : 100, value = readInteger($('relationship-value'), 0, maximum, '好感度'); if (value === null) return; send({ cmd: 'setRelationship', v: value }); };
  $('set-move-speed').onclick = function () { var value = readDecimal($('move-speed-multiplier'), 0.5, 5, '移动速度倍率'); if (value === null) return; commitInput($('move-speed-multiplier')); send({ cmd: 'setMoveSpeed', multiplier: value }); };
  $('reset-move-speed').onclick = function () { commitInput($('move-speed-multiplier')); send({ cmd: 'resetMoveSpeed' }); };
  shadow.querySelectorAll('[data-speed]').forEach(function (button) { button.onclick = function () { $('move-speed-multiplier').value = button.dataset.speed; $('set-move-speed').click(); }; });
  $('set-door-durability').onclick = function () { var value = readInteger($('door-durability-value'), 1, 100000000, '门耐久'); if (value !== null) send({ cmd: 'setDoorDurability', value: value }); };
  $('set-window-durability').onclick = function () { var value = readInteger($('window-durability-value'), 1, 100000000, '窗耐久'); if (value !== null) send({ cmd: 'setWindowDurability', value: value }); };
  $('infinite-food').onchange = function () { send({ cmd: 'setInfiniteFood', on: $('infinite-food').checked }); };
  $('refresh-proficiency').onclick = function () { send({ cmd: 'getProficiency' }); };
  // Keep the Buff workflow compact: category -> search -> add/remove.
  /* id="buff-config-list" is created once when the Buff tab is initialized. */
  var buffConfigBox = null, buffCurrentBox = null, buffPlanningBox = null, buffToolbar = null;
  function updateBuffView() {
    if (!buffConfigBox || !buffCurrentBox || !buffPlanningBox) return;
    var showPlanning = state.buffView === 'planning' || state.buffView === 'planningCatalog';
    var showLibrary = state.buffView === 'library';
    var showCurrent = state.buffScope === 'player' && state.buffView === 'current';
    buffConfigBox.style.display = showLibrary ? '' : 'none';
    buffCurrentBox.style.display = showCurrent ? '' : 'none';
    buffPlanningBox.style.display = showPlanning ? '' : 'none';
    if (!buffToolbar) return;
    buffToolbar.querySelectorAll('[data-buff-scope]').forEach(function (button) { button.classList.toggle('on', button.dataset.buffScope === state.buffScope); });
    var subnav = buffToolbar.querySelector('#buff-subnav');
    if (!subnav) return;
    subnav.innerHTML = '';
    var entries = state.buffScope === 'player'
      ? [['current', '当前效果'], ['planning', '生存规划']]
      : [['library', '当前效果'], ['planningCatalog', '生存规划']];
    subnav.style.display = '';
    entries.forEach(function (entry) {
      var button = document.createElement('button');
      button.className = 'buff-tab' + (state.buffView === entry[0] ? ' on' : '');
      button.textContent = entry[1];
      button.onclick = function () { state.buffView = entry[0]; updateBuffView(); }; 
      subnav.appendChild(button);
    });
    renderSurvivalPlans();
  }
  ensureBuffManagerUI = function () {
    if ($('buff-config-search')) return;
    var buffList = $('buff-list'), currentBox = buffList && buffList.parentElement;
    if (!currentBox) return;
    var grid = currentBox.parentElement;
    grid.classList.add('buff-grid');
    buffCurrentBox = currentBox;
    buffCurrentBox.classList.add('buff-current-box');
    buffCurrentBox.classList.remove('wide');
    var currentSearch = document.createElement('div');
    currentSearch.className = 'row';
    currentSearch.innerHTML = '<input id="buff-current-search" type="search" placeholder="搜索当前 Buff 名称或拼音"><button class="action" id="refresh-current-buffs">刷新</button>';
    currentBox.insertBefore(currentSearch, buffList);
    var configBox = currentBox.parentElement && currentBox.parentElement.querySelector('.box:not(.buff-current-box)');
    if (!configBox) return;
    buffConfigBox = configBox;
    buffConfigBox.classList.add('buff-library-box');
    var configTitle = configBox.querySelector('h3');
    if (configTitle) configTitle.textContent = 'Buff 配置库';
    var configHint = configBox.querySelector('.hint');
    if (configHint) configHint.textContent = '按名称或拼音搜索，点击添加即可应用。';
    var manualInput = $('buff-config-id');
    var manualRow = manualInput && manualInput.parentElement;
    if (manualRow) manualRow.style.display = 'none';
    var legacyRefresh = $('refresh-buffs');
    if (legacyRefresh && legacyRefresh.parentElement) legacyRefresh.parentElement.style.display = 'none';
    var configTools = document.createElement('div');
    configTools.className = 'row';
    configTools.innerHTML = '<input id="buff-config-search" type="search" placeholder="搜索 Buff 名称或拼音"><button class="action" id="refresh-buff-configs">刷新列表</button>';
    configBox.appendChild(configTools);
    var filters = document.createElement('div');
    filters.id = 'buff-config-filters';
    filters.className = 'chips';
    [['all', '全部'], ['good', '正面'], ['bad', '负面']].forEach(function (entry) {
      var button = document.createElement('button');
      button.className = 'chip' + (entry[0] === state.buffFilter ? ' on' : '');
      button.textContent = entry[1];
      button.dataset.filter = entry[0];
      button.onclick = function () {
        state.buffFilter = entry[0];
        filters.querySelectorAll('.chip').forEach(function (item) { item.classList.toggle('on', item.dataset.filter === state.buffFilter); });
        renderBuffConfigs();
      };
      filters.appendChild(button);
    });
    configBox.appendChild(filters);
    var configList = document.createElement('div');
    configList.id = 'buff-config-list';
    configList.style.maxHeight = '310px';
    configList.style.overflowY = 'auto';
    configBox.appendChild(configList);
    buffPlanningBox = document.createElement('div');
    buffPlanningBox.className = 'box buff-planning-box';
    buffPlanningBox.innerHTML = '<h3>生存规划</h3><div class="row"><input id="survival-plan-search" type="search" placeholder="搜索规划名称或拼音"><span class="value" id="survival-plan-count">--</span><button class="action" id="refresh-survival-plans">刷新</button></div><div id="survival-plan-list"><div class="hint">进入存档后读取。</div></div>';
    grid.appendChild(buffPlanningBox);
    buffToolbar = document.createElement('div');
    buffToolbar.className = 'buff-toolbar';
    buffToolbar.innerHTML = '<div class="buff-scope"><button class="buff-tab" data-buff-scope="player">已添加</button><button class="buff-tab" data-buff-scope="world">未添加</button></div><div class="buff-subnav" id="buff-subnav"></div>';
    grid.insertBefore(buffToolbar, configBox);
    buffToolbar.querySelectorAll('[data-buff-scope]').forEach(function (button) { button.onclick = function () {
      var planningSelected = state.buffView === 'planning' || state.buffView === 'planningCatalog';
      state.buffScope = button.dataset.buffScope;
      state.buffView = planningSelected
        ? (state.buffScope === 'player' ? 'planning' : 'planningCatalog')
        : (state.buffScope === 'player' ? 'current' : 'library');
      updateBuffView();
    }; });
    $('refresh-survival-plans').onclick = function () { send({ cmd: 'getSurvivalPlans' }); send({ cmd: 'getSurvivalPlanCatalog' }); };
    updateBuffView();
  };
  renderBuffConfigs = function () {
    var container = $('buff-config-list');
    if (!container) return;
    var query = String(($('buff-config-search') && $('buff-config-search').value) || '').trim().toLowerCase();
    var filter = state.buffFilter || 'all';
    var list = state.buffConfigs.filter(function (item) {
      var categoryMatch = filter === 'all' || (filter === 'good' ? !!item.good : !item.good);
      var textMatch = buffSearchMatch(item, query);
      return categoryMatch && textMatch;
    });
    container.innerHTML = '';
    if (!state.buffConfigs.length) { container.innerHTML = '<div class="hint">Buff 配置表尚未加载，请进入存档后点击刷新列表。</div>'; return; }
    if (!list.length) { container.innerHTML = '<div class="hint">没有匹配的 Buff 配置。</div>'; return; }
    var fragment = document.createDocumentFragment();
    list.forEach(function (item) {
      var row = document.createElement('div'); row.className = 'row';
      row.innerHTML = '<span class="value" style="flex:1">' + escapeHtml(item.name || '未知 Buff') + ' <small>' + (item.good ? '正面' : '负面') + '</small></span><button class="action primary">添加</button>';
      row.querySelector('button').onclick = function () { send({ cmd: 'addBuff', configId: item.configId }); };
      fragment.appendChild(row);
    });
    container.appendChild(fragment);
  };
  renderBuffs = function () {
    var container = $('buff-list');
    if (!container) return;
    container.innerHTML = '';
    var query = String(($('buff-current-search') && $('buff-current-search').value) || '').trim().toLowerCase();
    var list = state.buffs.filter(function (item) { return buffSearchMatch(item, query); });
    if (!list.length) { container.innerHTML = '<div class="hint">当前没有匹配的 Buff。</div>'; return; }
    var fragment = document.createDocumentFragment();
    list.forEach(function (item) {
      var row = document.createElement('div'); row.className = 'row';
      var source = item.source ? ' · ' + item.source : '';
      row.innerHTML = '<span class="value" style="flex:1">' + escapeHtml(item.name || '未知 Buff') + ' <small>' + (item.good ? '正面' : '负面') + ' · ' + item.layers + '层' + source + '</small></span><button class="action danger">移除</button>';
      row.querySelector('button').onclick = function () { send(item.removeByConfig ? { cmd: 'removeBuffByConfig', configId: item.configId } : { cmd: 'removeBuff', instanceId: item.instanceId }); };
      fragment.appendChild(row);
    });
    container.appendChild(fragment);
  };
  function renderSurvivalPlans() {
    var container = $('survival-plan-list');
    var count = $('survival-plan-count');
    if (!container) return;
    var catalogMode = state.buffScope === 'world' && state.buffView === 'planningCatalog';
    var source = catalogMode ? (state.survivalCatalog || []) : (state.survivalPlans || []);
    var query = String(($('survival-plan-search') && $('survival-plan-search').value) || '').trim().toLowerCase();
    var list = source.filter(function (item) { return (!catalogMode || !item.active) && buffSearchMatch(item, query); });
    if (count) count.textContent = list.length + ' 项';
    container.innerHTML = '';
    if (!list.length) {
      container.innerHTML = '<div class="hint">' + (catalogMode ? '没有可添加的生存规划。' : '当前存档没有已载入的生存规划。') + '</div>';
      return;
    }
    var fragment = document.createDocumentFragment();
    list.forEach(function (item) {
      var row = document.createElement('div'); row.className = 'plan-item';
      var action = document.createElement('button');
      action.className = 'action ' + (catalogMode ? 'primary' : 'danger');
      action.textContent = catalogMode ? '添加' : '移除';
      action.onclick = function () { send({ cmd: catalogMode ? 'addSurvivalPlan' : 'removeSurvivalPlan', talentId: item.talentId }); };
      row.innerHTML = '<div><strong>' + escapeHtml(item.name || '未命名生存规划') + ' <small>Lv ' + item.level + '</small></strong><small>' + escapeHtml(item.description || '游戏未提供描述') + '</small></div><span class="plan-state">' + (item.active ? '已生效' : (catalogMode ? '可添加' : '已获得')) + '</span>';
      row.appendChild(action);
      fragment.appendChild(row);
    });
    container.appendChild(fragment);
  }
  ensureExposureManagerUI();
  $('no-exposure').onchange = function () { send({ cmd: 'setNoExploreExposure', on: $('no-exposure').checked }); };
  ensureBuffManagerUI();
  $('buff-config-search').oninput = function () { renderBuffConfigs(); };
  $('buff-current-search').oninput = function () { renderBuffs(); };
  $('survival-plan-search').oninput = function () { renderSurvivalPlans(); };
  $('refresh-buff-configs').onclick = function () { send({ cmd: 'getBuffConfigs' }); };
  $('refresh-current-buffs').onclick = function () { send({ cmd: 'getBuffs' }); };
  $('set-exposure').onclick = function () { var value = readDecimal($('exposure-value'), 0, 100000000, '暴露值'); if (value !== null) send({ cmd: 'setExposure', value: value }); };
  $('set-survival-points').onclick = function () { var value = readInteger($('survival-points-value'), 0, 2147483647, '生存点'); if (value !== null) send({ cmd: 'setSurvivalPoints', value: value }); };
  $('unlock-all-codex').onclick = function () { send({ cmd: 'unlockAllCodex' }); };
  $('unlock-all-achievements').onclick = function () { send({ cmd: 'unlockAllAchievements' }); };
  $('add-buff').onclick = function () { var value = readInteger($('buff-config-id'), 1, 2147483647, 'Buff 配置 ID'); if (value !== null) send({ cmd: 'addBuff', configId: value }); };
  $('refresh-buffs').onclick = function () { send({ cmd: 'getBuffs' }); };
  $('remove-negative-buffs').onclick = function () { send({ cmd: 'removeBuffs' }); };
  $('clear-all-buffs').onclick = function () { send({ cmd: 'clearBuffs' }); };

  shadow.querySelectorAll('input[type=number]').forEach(prepareNumberInput);
  bindEnter($('money-value'), $('set-money'));
  bindEnter($('item-count'), $('add-item'));
  bindEnter($('bag-columns'), $('set-bag-size'));
  bindEnter($('bag-rows'), $('set-bag-size'));
  bindEnter($('bag-burden-value'), $('set-bag-burden'));
  bindEnter($('relationship-value'), $('set-relationship'));
  bindEnter($('move-speed-multiplier'), $('set-move-speed'));
  bindEnter($('door-durability-value'), $('set-door-durability'));
  bindEnter($('window-durability-value'), $('set-window-durability'));
  bindEnter($('exposure-value'), $('set-exposure'));
  bindEnter($('survival-points-value'), $('set-survival-points'));
  bindEnter($('buff-config-id'), $('add-buff'));

  function applyScale(value, persist) {
    var percent = Math.min(125, Math.max(70, number(value, 100)));
    var scale = percent / 100;
    $('ui-scale').value = String(percent);
    $('root').style.width = Math.min(830, (window.innerWidth - 32) / scale) + 'px';
    $('root').style.height = Math.min(580, (window.innerHeight - 32) / scale) + 'px';
    $('root').style.transform = 'scale(' + scale + ')';
    $('root').style.transformOrigin = 'center center';
    if (persist) {
      host.style.left = '50%';
      host.style.top = '50%';
      host.style.transform = 'translate(-50%,-50%)';
      try { window.localStorage.setItem('slc-ui-scale', String(percent)); } catch (_) {}
    }
    try { window.dispatchEvent(new Event('slc-layout-changed')); } catch (_) {}
  }
  function applyFrameRate(value, persist) {
    var requested = number(value, 0);
    var fps = requested === 0 ? 0 : Math.min(240, Math.max(30, requested));
    var selector = $('ui-fps');
    if (!selector) return;
    if (!selector.querySelector('option[value="' + fps + '"]')) fps = 0;
    selector.value = String(fps);
    if (persist) {
      try { window.localStorage.setItem('slc-ui-fps', String(fps)); } catch (_) {}
      send({ cmd: 'setFrameRate', fps: fps });
    }
  }
  function applyVisualSettings(theme, solid, persist) {
    var selectedTheme = theme === 'light' ? 'light' : 'dark';
    var opaque = !!solid;
    $('root').classList.toggle('theme-light', selectedTheme === 'light');
    $('root').classList.toggle('solid-bg', opaque);
    var themeSelect = $('ui-theme');
    var solidToggle = $('ui-solid-bg');
    if (themeSelect) themeSelect.value = selectedTheme;
    if (solidToggle) solidToggle.checked = opaque;
    if (persist) {
      try {
        window.localStorage.setItem('slc-ui-theme', selectedTheme);
        window.localStorage.setItem('slc-ui-solid-bg', opaque ? '1' : '0');
      } catch (_) {}
    }
  }
  function setupVisualSettings() {
    var panel = $('settings-panel');
    if (!panel || $('ui-theme')) return;
    var themeRow = document.createElement('div');
    themeRow.className = 'row visual-row';
    themeRow.innerHTML = '<label for="ui-theme">界面颜色</label><select id="ui-theme"><option value="dark">黑色</option><option value="light">白色</option></select>';
    var solidRow = document.createElement('div');
    solidRow.className = 'row visual-row';
    solidRow.innerHTML = '<label class="switch"><input id="ui-solid-bg" type="checkbox"><span></span>不透明背景</label>';
    panel.appendChild(themeRow);
    panel.appendChild(solidRow);
    var savedTheme = 'dark', savedSolid = false;
    try {
      savedTheme = window.localStorage.getItem('slc-ui-theme') || 'dark';
      savedSolid = window.localStorage.getItem('slc-ui-solid-bg') === '1';
    } catch (_) {}
    applyVisualSettings(savedTheme, savedSolid, false);
    $('ui-theme').onchange = function () { applyVisualSettings($('ui-theme').value, $('ui-solid-bg').checked, true); };
    $('ui-solid-bg').onchange = function () { applyVisualSettings($('ui-theme').value, $('ui-solid-bg').checked, true); };
  }
  function keepPanelOnScreen() {
    requestAnimationFrame(function () {
      var rect = $('root').getBoundingClientRect();
      if (rect.left < 0 || rect.top < 0 || rect.right > window.innerWidth || rect.bottom > window.innerHeight) {
        host.style.left = '50%';
        host.style.top = '50%';
        host.style.transform = 'translate(-50%,-50%)';
      }
    });
  }
  $('save-memory').onclick = function () { state.memory = collectMemory(); send(Object.assign({ cmd: 'saveMemory' }, state.memory)); };
  $('settings').onclick = function () { $('settings-panel').classList.toggle('open'); };
  $('ui-scale').onchange = function () { applyScale($('ui-scale').value, true); };
  $('ui-fps').onchange = function () { applyFrameRate($('ui-fps').value, true); };
  try {
    var savedFps = window.localStorage.getItem('slc-ui-fps');
    if (savedFps) applyFrameRate(savedFps, false);
  } catch (_) {}
  window.addEventListener('resize', function () { applyScale($('ui-scale').value, false); });
  (function restoreScale() { var saved = '100'; try { saved = window.localStorage.getItem('slc-ui-scale') || '100'; } catch (_) {} if (!$('ui-scale').querySelector('option[value="' + saved + '"]')) saved = '100'; applyScale(saved, false); })();
  setupVisualSettings();
  $('minimize').onclick = function () {
    // Keep the panel visible until the native side confirms the close. Hiding
    // first can leave an invisible full-screen WebView consuming game clicks
    // when a scene transition drops the queued command.
    slcPostMessage('slc-close-panel');
  };

  (function enableDrag() {
    var supportsPointer = typeof window.PointerEvent === 'function';
    var downEvent = supportsPointer ? 'pointerdown' : 'mousedown';
    var moveEvent = supportsPointer ? 'pointermove' : 'mousemove';
    var dragging = false, activePointer = null, dragHandle = null;
    var startX = 0, startY = 0, baseLeft = 0, baseTop = 0, nextLeft = 0, nextTop = 0;
    var width = 0, height = 0, viewportWidth = 0, viewportHeight = 0, minLeft = 0, maxLeft = 0, minTop = 0, maxTop = 0, frame = 0, keepAliveTimer = 0;
    var geometryReady = false, geometryFrame = 0, nativeStartFrame = 0, nativeDragStarted = false;
    var cachedLeft = 0, cachedTop = 0, cachedWidth = 0, cachedHeight = 0, cachedOffsetX = 0, cachedOffsetY = 0;

    function readGeometry() {
      var rect = host.getBoundingClientRect();
      var visualRect = $('root').getBoundingClientRect();
      cachedLeft = rect.left;
      cachedTop = rect.top;
      cachedWidth = visualRect.width;
      cachedHeight = visualRect.height;
      cachedOffsetX = visualRect.left - rect.left;
      cachedOffsetY = visualRect.top - rect.top;
      geometryReady = true;
    }
    function scheduleGeometryRefresh() {
      if (geometryFrame) window.cancelAnimationFrame(geometryFrame);
      geometryFrame = window.requestAnimationFrame(function () {
        geometryFrame = 0;
        if (!dragging) readGeometry();
      });
    }
    function notifyNativeDragStart() {
      // Let Chromium paint the first movement before crossing the Vuplex bridge.
      // The bridge call can wake the native message path after an idle period.
      nativeStartFrame = window.requestAnimationFrame(function () {
        nativeStartFrame = window.requestAnimationFrame(function () {
          nativeStartFrame = 0;
          if (!dragging) return;
          nativeDragStarted = true;
          slcPostMessage('slc-drag-start');
          keepAliveTimer = setInterval(function () { slcPostMessage('slc-drag-active'); }, 500);
        });
      });
    }

    function paint() {
      frame = 0;
      host.style.transform = 'translate3d(' + (nextLeft - baseLeft) + 'px,' + (nextTop - baseTop) + 'px,0)';
    }
    function move(event) {
      if (!dragging || (supportsPointer && event.pointerId !== activePointer)) return;
      // Keep the rest of the game page from seeing drag moves: this avoids
      // competing page handlers and scroll/selection side effects mid-drag.
      event.preventDefault();
      event.stopPropagation();
      nextLeft = Math.max(minLeft, Math.min(maxLeft, baseLeft + event.clientX - startX));
      nextTop = Math.max(minTop, Math.min(maxTop, baseTop + event.clientY - startY));
      if (!frame) frame = window.requestAnimationFrame(paint);
    }
    function releasePointer() {
      if (dragHandle && supportsPointer && activePointer != null && typeof dragHandle.releasePointerCapture === 'function') {
        try { dragHandle.releasePointerCapture(activePointer); } catch (_) {}
      }
    }
    function end(event) {
      if (!dragging) return;
      if (supportsPointer && event && event.pointerId !== undefined && event.pointerId !== activePointer) return;
      if (frame) { window.cancelAnimationFrame(frame); frame = 0; }
      if (nativeStartFrame) { window.cancelAnimationFrame(nativeStartFrame); nativeStartFrame = 0; }
      if (keepAliveTimer) { clearInterval(keepAliveTimer); keepAliveTimer = 0; }
      dragging = false;
      state.dragging = false;
      releasePointer();
      host.style.left = nextLeft + 'px';
      host.style.top = nextTop + 'px';
      host.style.transform = 'translate3d(0,0,0)';
      cachedLeft = nextLeft;
      cachedTop = nextTop;
      geometryReady = true;
      activePointer = null;
      dragHandle = null;
      if (nativeDragStarted) slcPostMessage('slc-drag-end');
      nativeDragStarted = false;
      scheduleGeometryRefresh();
    }
    function start(event) {
      if (dragging || (supportsPointer && event.isPrimary === false)) return;
      if (event.button !== undefined && event.button !== 0) return;
      if (event.target.closest('button, input, select, label, .items, .bag, .item, .bagrow, .settings, .chip')) return;
      event.preventDefault();
      event.stopPropagation();
      // Layout is cached outside the input event. Status refreshes can dirty a
      // large panel DOM, so forcing layout here used to stall the first move.
      if (!geometryReady) readGeometry();
      dragging = true;
      state.dragging = true;
      activePointer = supportsPointer ? event.pointerId : null;
      dragHandle = event.currentTarget;
      startX = event.clientX; startY = event.clientY;
      baseLeft = cachedLeft; baseTop = cachedTop;
      nextLeft = baseLeft; nextTop = baseTop;
      width = cachedWidth; height = cachedHeight;
      viewportWidth = window.innerWidth; viewportHeight = window.innerHeight;
      var visualOffsetX = cachedOffsetX, visualOffsetY = cachedOffsetY;
      minLeft = -visualOffsetX; maxLeft = Math.max(minLeft, viewportWidth - width - visualOffsetX);
      minTop = -visualOffsetY; maxTop = Math.max(minTop, viewportHeight - height - visualOffsetY);
      host.style.left = baseLeft + 'px';
      host.style.top = baseTop + 'px';
      host.style.transform = 'translate3d(0,0,0)';
      if (supportsPointer && activePointer != null && typeof dragHandle.setPointerCapture === 'function') {
        try { dragHandle.setPointerCapture(activePointer); } catch (_) {}
      }
      notifyNativeDragStart();
    }
    var downOptions = { passive: false, capture: true };
    $('root').addEventListener(downEvent, start, downOptions);
    window.addEventListener(moveEvent, move, { passive: false, capture: true });
    window.addEventListener(supportsPointer ? 'pointerup' : 'mouseup', end, true);
    window.addEventListener(supportsPointer ? 'pointercancel' : 'mouseleave', end, true);
    if (supportsPointer) $('root').addEventListener('lostpointercapture', end, true);
    window.addEventListener('blur', end);
    window.addEventListener('slc-drag-cancel', end);
    window.addEventListener('resize', scheduleGeometryRefresh);
    window.addEventListener('slc-layout-changed', scheduleGeometryRefresh);
    scheduleGeometryRefresh();
  })();

  window.__slcPush = function (json) {
    var data; try { data = JSON.parse(json); } catch (_) { return; }
    switch (data.type) {
      case 'status':
        var wasConnected = state.connected;
        state.connected = !!data.ok;
        $('state').textContent = data.ok ? '已连接' : '等待进入存档';
        $('state').className = 'status' + (data.ok ? ' ready' : '');
        if (data.ok && wasConnected === false) toast('已检测到存档，修改器已激活');
        break;
      case 'memory': applyMemory(data.state); break;
      case 'gold': state.gold = data.val; $('gold').textContent = formatGold(data.val); break;
      case 'items': state.itemIndexVersion++; state.items = data.items || []; state.items.forEach(function (item) { item.py = null; }); state.cats = data.cats || []; state.tcats = data.tcats || []; renderCategories(); renderItems(); buildPinyinIndex(state.items, state.itemIndexVersion, 0); break;
      case 'bag': state.bag = data.items || []; renderBag(); break;
      case 'bagInfo': state.bagInfo = data; renderBagInfo(); break;
      case 'attr': applyAttributePayload(data); break;
      case 'moveSpeed': state.moveSpeed = data; renderMoveSpeed(); break;
      case 'facilities': state.facilities = data; renderFacilities(); break;
      case 'time': state.time = data; renderTime(); break;
      case 'relationship': state.relationship = data; renderRelationship(); break;
      case 'proficiency': state.proficiency = data.items || []; renderProficiencies(); break;
      case 'resources': state.resources = data; renderResources(); break;
      case 'buffConfigs': state.buffConfigs = prepareBuffSearch(data.items || []); renderBuffConfigs(); break;
      case 'survivalPlans': state.survivalPlans = prepareBuffSearch(data.items || []); renderSurvivalPlans(); break;
      case 'survivalPlanCatalog': state.survivalCatalog = prepareBuffSearch(data.items || []); renderSurvivalPlans(); break;
      case 'buffs': state.buffs = prepareBuffSearch(data.items || []); renderBuffs(); break;
      case 'clipboard': $('item-search').value = data.text || ''; renderItems(); break;
      case 'panel':
        $('root').style.display = data.open ? 'flex' : 'none';
        setOuterScrollLock(!!data.open);
        if (data.open) keepPanelOnScreen();
        else window.dispatchEvent(new Event('slc-drag-cancel'));
        break;
      case 'result': toast(data.msg, !data.ok); if (data.gold !== undefined) { state.gold = data.gold; $('gold').textContent = formatGold(data.gold); } break;
    }
  };

  send({ cmd: 'setActiveTab', tab: 'prepare' }); send({ cmd: 'ready' });
})();
