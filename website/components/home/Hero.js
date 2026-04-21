'use client';

import Link from 'next/link';
import { useEffect, useRef } from 'react';
import { FIRMWARE_VERSION, BUILD_ID } from '@/content/meta';

const PHRASES = [
  'unigeek wifi evil-twin --target "Airport_Free"',
  'unigeek ble spam --mode apple',
  'unigeek flash --board t-embed-cc1101',
  'unigeek ir replay --file samsung_tv.ir',
];

function useTypedLoop(targetRef) {
  useEffect(() => {
    const el = targetRef.current;
    if (!el) return;
    let phraseIdx = 0;
    let charIdx = 0;
    let deleting = false;
    let timer;
    let cancelled = false;

    function loop() {
      if (cancelled) return;
      const word = PHRASES[phraseIdx];
      if (!deleting) {
        charIdx++;
        el.textContent = word.substring(0, charIdx);
        if (charIdx === word.length) {
          deleting = true;
          timer = setTimeout(loop, 1800);
          return;
        }
        timer = setTimeout(loop, 40 + Math.random() * 40);
      } else {
        charIdx--;
        el.textContent = word.substring(0, charIdx);
        if (charIdx === 0) {
          deleting = false;
          phraseIdx = (phraseIdx + 1) % PHRASES.length;
          timer = setTimeout(loop, 400);
          return;
        }
        timer = setTimeout(loop, 20);
      }
    }
    loop();
    return () => { cancelled = true; clearTimeout(timer); };
  }, [targetRef]);
}

export default function Hero({ moduleCount, boardCount }) {
  const typedRef = useRef(null);
  useTypedLoop(typedRef);

  return (
    <section className="hero">
      <div className="hero-meta">
        <div><span>00 / System</span><strong>ESP32 · ESP32-S3</strong></div>
        <div><span>01 / Boards</span><strong>{boardCount} supported</strong></div>
        <div><span>02 / Modules</span><strong>{moduleCount} features</strong></div>
        <div><span>03 / License</span><strong>Open source</strong></div>
      </div>

      <div className="hero-grid">
        <div>
          <div className="annotation" style={{ marginBottom: 20 }}>
            {`// unigeek/firmware/v${FIRMWARE_VERSION} — build ${BUILD_ID}`}
          </div>
          <h1 className="hero-title">
            Multi-tool<br />
            firmware<span className="slash">//</span><br />
            <span className="glitch">for ESP32.</span>
          </h1>
          <p className="hero-sub">
            One firmware, {boardCount} boards, {moduleCount} modules. WiFi, BLE, NFC, IR, Sub-GHz, USB HID
            — all running from a single reflashable image. Built for tinkerers, researchers, and red-teamers.
          </p>
          <div className="hero-ctas">
            <Link href="/install" className="btn btn-primary">
              Install firmware
              <span style={{ fontFamily: 'var(--font-mono)' }}>→</span>
            </Link>
            <Link href="/features" className="btn">Browse features</Link>
          </div>
          <div className="hero-list">
            <div>Open-source, hackable</div>
            <div>OTA &amp; web-flash ready</div>
            <div>{moduleCount} modules &amp; growing</div>
            <div>Zero cloud dependencies</div>
          </div>
        </div>

        <div className="panel tick-corners">
          <div className="terminal-chrome">
            <div style={{ display: 'flex', gap: 10, alignItems: 'center' }}>
              <div className="chrome-dots"><span /><span /><span /></div>
              <span>unigeek@esp32 · /dev/ttyUSB0 · 115200</span>
            </div>
            <span>READY</span>
          </div>
          <div className="terminal">
            <div><span className="prompt">$</span> <span className="path">unigeek</span> --init</div>
            <div className="out">  booting unigeek firmware v{FIRMWARE_VERSION} ...</div>
            <div className="out">  detected board: <span className="key">T-Embed CC1101 (esp32-s3)</span></div>
            <div className="out">  mounting LittleFS ............ <span className="ok">ok</span></div>
            <div className="out">  probing radios: wifi / ble ... <span className="ok">ok</span></div>
            <div className="out">  probing radios: cc1101 ....... <span className="ok">ok</span></div>
            <div className="out">  probing radios: nrf24l01 ..... <span className="ok">ok</span></div>
            <div className="out">  loading modules: {moduleCount}/{moduleCount} ....... <span className="ok">ok</span></div>
            <div style={{ marginTop: 10 }}><span className="prompt">$</span> <span className="path">unigeek</span> wifi --list-modules</div>
            <div className="out">  <span className="key">[01]</span> evil-twin      <span className="ok">stable</span></div>
            <div className="out">  <span className="key">[02]</span> eapol-capture  <span className="ok">stable</span></div>
            <div className="out">  <span className="key">[03]</span> karma-attack   <span className="ok">stable</span></div>
            <div className="out">  <span className="key">[04]</span> network-mitm   <span className="ok">stable</span></div>
            <div className="out">  <span className="key">[05]</span> cctv-sniffer   <span className="ok">stable</span></div>
            <div className="out">  <span className="key">[..]</span> +16 more modules</div>
            <div style={{ marginTop: 10 }}>
              <span className="prompt">$</span>{' '}
              <span ref={typedRef} />
              <span className="cursor" />
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
