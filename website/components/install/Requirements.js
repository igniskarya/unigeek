const ITEMS = [
  {
    title: 'USB-C or Micro-USB cable',
    desc:
      "Must be a data cable — charge-only cables won't enumerate. Double-check by trying to read the board over Serial first.",
  },
  {
    title: 'Chromium-based browser',
    desc:
      'Web Flasher uses WebSerial. Works in Chrome, Edge, Opera, and Brave. Firefox and Safari are not supported.',
  },
  {
    title: 'Board in bootloader mode',
    desc:
      'Most boards auto-enter bootloader. For T-Display variants, hold BOOT while pressing RESET before connecting.',
  },
];

export default function Requirements() {
  return (
    <>
      <div className="step-head" style={{ marginTop: 64 }}>
        <div className="step-num">Req.</div>
        <div className="step-title">Requirements</div>
        <div className="step-sub">Read before flashing</div>
      </div>
      <div className="requirements">
        {ITEMS.map((r, i) => (
          <div key={i} className="req">
            <div className="req-num">{String(i + 1).padStart(2, '0')}</div>
            <div className="req-title">{r.title}</div>
            <div className="req-desc">{r.desc}</div>
          </div>
        ))}
      </div>
    </>
  );
}
