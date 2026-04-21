import Link from 'next/link';
import { toCaps, shortChip } from '@/content/board-caps';

export default function BoardSummary({ boards }) {
  return (
    <section className="section">
      <div className="section-label">03 · Supported hardware</div>
      <div
        style={{
          display: 'flex',
          justifyContent: 'space-between',
          alignItems: 'flex-end',
          marginBottom: 40,
          gap: 24,
          flexWrap: 'wrap',
        }}
      >
        <h2
          style={{
            fontSize: 48,
            fontWeight: 400,
            letterSpacing: '-0.025em',
            lineHeight: 1,
            maxWidth: 720,
          }}
        >
          One firmware.<br />
          {boards.length} boards.
        </h2>
        <Link
          href="/install"
          className="mono dim"
          style={{ fontSize: 12, letterSpacing: '0.08em', textTransform: 'uppercase' }}
        >
          Flash your board →
        </Link>
      </div>

      <div className="boards">
        {boards.map((b) => (
          <div key={b.id} className="board">
            <div className="board-head">
              <div className="board-name">{b.name}</div>
              <div className="board-chip">{shortChip(b.chip)}</div>
            </div>
            <div className="board-caps">
              {toCaps(b.tags).map((cap) => (
                <span key={cap} className="cap">{cap}</span>
              ))}
            </div>
          </div>
        ))}
      </div>
    </section>
  );
}
