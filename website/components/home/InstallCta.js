import Link from 'next/link';

export default function InstallCta() {
  return (
    <section className="section" style={{ paddingBottom: 40 }}>
      <div className="cta-band panel-corners">
        <span className="corner-tl" />
        <span className="corner-br" />
        <div>
          <div className="annotation" style={{ marginBottom: 16 }}>// install / flash / run</div>
          <h2 className="cta-title">
            Flash your board in<br />
            under sixty seconds.
          </h2>
          <p className="cta-sub">
            Web-based flasher, manual .bin downloads, or OTA from the device. Pick your board, plug it in,
            and go.
          </p>
        </div>
        <div className="cta-actions">
          <Link href="/install" className="btn btn-primary" style={{ justifyContent: 'center' }}>
            Install now
          </Link>
          <Link href="/releases" className="btn" style={{ justifyContent: 'center' }}>
            Release notes
          </Link>
        </div>
      </div>
    </section>
  );
}
