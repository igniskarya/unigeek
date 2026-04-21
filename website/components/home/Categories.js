import Link from 'next/link';

function pad2(n) {
  return String(n).padStart(2, '0');
}

export default function Categories({ categories, counts, total }) {
  return (
    <section className="section">
      <div className="section-label">02 · Module catalog</div>
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
          {categories.length} categories.<br />
          {total} modules.
        </h2>
        <Link
          href="/features"
          className="mono dim"
          style={{ fontSize: 12, letterSpacing: '0.08em', textTransform: 'uppercase' }}
        >
          View all features →
        </Link>
      </div>
      <div className="cats">
        {categories.map((cat, i) => (
          <Link key={cat.id} href={`/features#${cat.id}`} className="cat">
            <div className="cat-num">A.{pad2(i + 1)}</div>
            <div className="cat-name">{cat.label}</div>
            <div className="cat-count">{counts[cat.id] || 0} modules</div>
          </Link>
        ))}
      </div>
    </section>
  );
}
