const GROUP_ORDER = [
  { key: 'added',   label: 'ADDED' },
  { key: 'changed', label: 'CHANGED' },
  { key: 'fixed',   label: 'FIXED' },
  { key: 'removed', label: 'REMOVED' },
];

function formatReleaseDate(iso) {
  if (!iso) return null;
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return null;
  return d.toLocaleDateString('en-US', {
    year: 'numeric',
    month: 'short',
    day: 'numeric',
  });
}

function ChangeGroup({ groupKey, label, items, note }) {
  if (!items || items.length === 0) return null;
  return (
    <div className="change-group">
      <div className={`change-label ${groupKey}`}>
        <span className="pill">{label}</span>
        <span className="count">
          {items.length} {items.length === 1 ? 'entry' : 'entries'}
        </span>
      </div>
      <ul className={`change-list ${groupKey}`}>
        {items.map((item, i) => (
          <li key={i}>
            {item.title ? <><strong>{item.title}</strong>{item.html ? ' — ' : ''}</> : null}
            {item.html ? <span dangerouslySetInnerHTML={{ __html: item.html }} /> : null}
          </li>
        ))}
      </ul>
      {note ? (
        <div
          style={{ marginTop: 12, fontSize: 13, color: 'var(--ink-muted)', lineHeight: 1.6 }}
          dangerouslySetInnerHTML={{ __html: note }}
        />
      ) : null}
    </div>
  );
}

export default function ReleaseItem({ release, tag }) {
  const isLatest = tag === 'latest';
  const dateLabel = formatReleaseDate(release.date);
  return (
    <article className="release" id={`v${release.version}`}>
      <div className="release-head">
        <div className="release-v">
          <span className="mono dim" style={{ fontSize: 12, letterSpacing: '0.08em' }}>
            VERSION
          </span>
          <span className={`release-num${isLatest ? ' latest' : ''}`}>v{release.version}</span>
          {dateLabel && (
            <span
              className="mono dim"
              style={{ fontSize: 12, letterSpacing: '0.08em' }}
            >
              · {dateLabel}
            </span>
          )}
        </div>
        <div className="release-tags">
          {tag === 'latest' && <span className="badge badge-ok">LATEST</span>}
          {tag === 'archived' && <span className="badge">ARCHIVED</span>}
        </div>
      </div>

      <div className="release-body">
        {release.preamble ? (
          <div
            style={{ marginBottom: 24, color: 'var(--ink-dim)', lineHeight: 1.7 }}
            dangerouslySetInnerHTML={{ __html: release.preamble }}
          />
        ) : null}

        {GROUP_ORDER.map((g) => (
          <ChangeGroup
            key={g.key}
            groupKey={g.key}
            label={g.label}
            items={release[g.key]}
            note={release.notes?.[g.key]}
          />
        ))}
      </div>

      {release.install ? (
        <div
          className="contributors"
          style={{ display: 'block', padding: '16px 32px', color: 'var(--ink-muted)', lineHeight: 1.7 }}
          dangerouslySetInnerHTML={{ __html: release.install }}
        />
      ) : null}
    </article>
  );
}
