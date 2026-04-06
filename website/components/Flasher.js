"use client";
import { useState } from "react";
import FlashLog from "./FlashLog";

// esptool-js imports are dynamic to avoid SSR issues
const USB_FILTERS = [
  { usbVendorId: 0x10c4, usbProductId: 0xea60 }, // CP2102
  { usbVendorId: 0x0403, usbProductId: 0x6010 }, // FT2232H
  { usbVendorId: 0x303a, usbProductId: 0x1001 }, // Espressif JTAG
  { usbVendorId: 0x303a, usbProductId: 0x0002 }, // Espressif CDC
  { usbVendorId: 0x1a86, usbProductId: 0x55d4 }, // CH9102F
  { usbVendorId: 0x1a86, usbProductId: 0x7523 }, // CH340T
  { usbVendorId: 0x0403, usbProductId: 0x6001 }, // FT232R
];

function arrayBufferToBinaryString(buffer) {
  const bytes = new Uint8Array(buffer);
  let binary = "";
  const chunkSize = 0x8000;
  for (let i = 0; i < bytes.length; i += chunkSize) {
    binary += String.fromCharCode(...bytes.subarray(i, i + chunkSize));
  }
  return binary;
}

const STATUS = { IDLE: "idle", CONNECTING: "connecting", CONNECTED: "connected", FLASHING: "flashing", DONE: "done", ERROR: "error" };

const Flasher = ({ board }) => {
  const [log, setLog] = useState("");
  const [status, setStatus] = useState(STATUS.IDLE);
  const [progress, setProgress] = useState(0);
  const [espTransport, setEspTransport] = useState(null);
  const [deviceLoader, setDeviceLoader] = useState(null);

  const writeLine = (line) => setLog((prev) => prev + line + "\n");

  const terminal = {
    clean: () => setLog(""),
    writeLine,
    write: (data) => setLog((prev) => prev + data),
  };

  const doConnect = async () => {
    try {
      setStatus(STATUS.CONNECTING);
      writeLine("Requesting serial port...");

      const { Transport } = await import("esptool-js");
      const { ESPLoader } = await import("esptool-js");

      const port = await navigator.serial.requestPort({ filters: USB_FILTERS });
      const transport = new Transport(port, true);
      setEspTransport(transport);

      writeLine("Connecting to ESP32...");
      const loader = new ESPLoader({ transport, baudrate: 921600, terminal });
      await loader.main();
      await loader.flashId();

      const chipName = loader.chip?.CHIP_NAME || "unknown";
      writeLine(`Connected: ${chipName}`);

      // Chip verification
      const isS3 = chipName.toLowerCase().includes("s3");
      const boardNeedsS3 = board.chip === "ESP32-S3";
      if (isS3 !== boardNeedsS3) {
        writeLine(`ERROR: Wrong chip. Expected ${board.chip}, got ${chipName}.`);
        writeLine("Disconnect to prevent bricking.");
        setStatus(STATUS.ERROR);
        return;
      }

      setDeviceLoader(loader);
      setStatus(STATUS.CONNECTED);
      writeLine(`Ready to flash: ${board.name}`);
    } catch (err) {
      writeLine(`Error: ${err.message}`);
      setStatus(STATUS.ERROR);
    }
  };

  const doInstall = async () => {
    if (!deviceLoader) return;
    try {
      setStatus(STATUS.FLASHING);
      setProgress(0);
      writeLine(`Fetching firmware for ${board.name}...`);

      const resp = await fetch(board.bin);
      if (!resp.ok) throw new Error(`HTTP ${resp.status} fetching ${board.bin}`);
      const buffer = await resp.arrayBuffer();
      const data = arrayBufferToBinaryString(buffer);

      writeLine(`Writing ${(buffer.byteLength / 1024).toFixed(0)} KB...`);
      await deviceLoader.writeFlash({
        fileArray: [{ address: 0, data }],
        flashSize: "keep",
        eraseAll: false,
        compress: true,
        reportProgress: (fileIndex, written, total) => {
          const pct = Math.round((written / total) * 100);
          setProgress(pct);
          if (pct % 10 === 0) writeLine(`  ${pct}%`);
        },
      });

      writeLine("Done! Reset the device.");
      setStatus(STATUS.DONE);
    } catch (err) {
      writeLine(`Error: ${err.message}`);
      setStatus(STATUS.ERROR);
    }
  };

  const doDisconnect = async () => {
    if (espTransport) {
      try {
        await espTransport.disconnect();
      } catch (_) {}
    }
    setEspTransport(null);
    setDeviceLoader(null);
    setStatus(STATUS.IDLE);
    writeLine("Disconnected.");
  };

  const webSerialSupported = typeof navigator !== "undefined" && "serial" in navigator;

  return (
    <div className="install-step">
      <div className="step-label">Step 2 — Flash firmware</div>
      <div style={{ marginBottom: "8px", fontSize: "0.9em", color: "#aaa" }}>
        Board: <strong style={{ color: "#fff" }}>{board.name}</strong> ({board.chip})
      </div>

      {!webSerialSupported && (
        <div style={{ color: "#f5a623", fontSize: "0.85em", marginBottom: "12px" }}>
          Web Serial is not supported in this browser. Use Chrome or Edge on desktop.
        </div>
      )}

      {status === STATUS.FLASHING && (
        <div style={{ marginBottom: "12px", fontSize: "0.85em", color: "#aaa" }}>
          Installing... {progress}%
        </div>
      )}

      <div className="flash-actions">
        {status === STATUS.IDLE || status === STATUS.ERROR ? (
          <>
            <button className="btn fill" onClick={doConnect} disabled={!webSerialSupported}>
              Connect &amp; Install
            </button>
            <a className="btn" href={board.bin} download>
              Download .bin
            </a>
          </>
        ) : status === STATUS.CONNECTED ? (
          <>
            <button className="btn fill" onClick={doInstall}>
              Flash Firmware
            </button>
            <button className="btn" onClick={doDisconnect}>
              Disconnect
            </button>
          </>
        ) : status === STATUS.DONE ? (
          <button className="btn" onClick={doDisconnect}>
            Done — Disconnect
          </button>
        ) : null}
      </div>

      <FlashLog text={log} />
    </div>
  );
};

export default Flasher;