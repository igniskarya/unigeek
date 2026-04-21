'use client';

import Link from 'next/link';
import { useMemo, useState } from 'react';

function pad2(n) {
  return String(n).padStart(2, '0');
}

function statusOf(feat) {
  if (feat.stable === false) return 'soon';
  return 'stable';
}

function FeatureCard({ index, feat }) {
  const status = statusOf(feat);
  const hasDoc = status === 'stable' && feat.hasDetail;
  const badge =
    status === 'soon'
      ? <span className="badge badge-soon">SOON</span>
      : hasDoc
        ? <span className="badge badge-ok">DOCS</span>
        : null;

  const inner = (
    <>
      <div className="feat-head">
        <div className="feat-name">
          <span className="idx">{pad2(index + 1)}</span>
          {feat.title}
        </div>
        {badge}
      </div>
      <div className="feat-desc">{feat.summary}</div>
      {hasDoc && <div className="feat-link">View docs →</div>}
    </>
  );

  const className = `feat ${status}${hasDoc ? ' has-doc' : ''}`;

  if (hasDoc) {
    return (
      <Link className={className} href={`/features/${feat.slug}`}>
        {inner}
      </Link>
    );
  }
  return <article className={className}>{inner}</article>;
}

export default function FeatureCatalog({ categories, features, counts }) {
  const [filter, setFilter] = useState('all');
  const [q, setQ] = useState('');

  const filtered = useMemo(() => {
    const needle = q.trim().toLowerCase();
    return features.filter((f) => {
      if (filter === 'docs' && !f.hasDetail) return false;
      if (!needle) return true;
      const hay = `${f.title} ${f.summary}`.toLowerCase();
      return hay.includes(needle);
    });
  }, [features, filter, q]);

  const byCat = useMemo(() => {
    const m = {};
    for (const c of categories) m[c.id] = [];
    for (const f of filtered) {
      if (!m[f.category]) m[f.category] = [];
      m[f.category].push(f);
    }
    return m;
  }, [categories, filtered]);

  return (
    <div className="features-layout">
      <aside className="side">
        <div className="side-head">
          <span>Categories</span>
          <span>{categories.length}</span>
        </div>
        <nav className="side-list">
          {categories.map((cat, i) => (
            <a
              key={cat.id}
              href={`#${cat.id}`}
              className={`side-link${i === 0 ? ' active' : ''}`}
            >
              <span>{cat.label}</span>
              <span className="num">{counts[cat.id] || 0}</span>
            </a>
          ))}
        </nav>
      </aside>

      <div>
        <div className="filter-bar">
          <button
            type="button"
            className={`filter-btn${filter === 'all' ? ' active' : ''}`}
            onClick={() => setFilter('all')}
          >
            All
          </button>
          <button
            type="button"
            className={`filter-btn${filter === 'docs' ? ' active' : ''}`}
            onClick={() => setFilter('docs')}
          >
            Docs
          </button>
          <div className="search-box">
            <input
              type="text"
              placeholder="Search modules..."
              value={q}
              onChange={(e) => setQ(e.target.value)}
            />
          </div>
        </div>

        {categories.map((cat, i) => {
          const items = byCat[cat.id] || [];
          if (items.length === 0) return null;
          return (
            <section key={cat.id} className="cat-section" id={cat.id}>
              <div className="cat-header">
                <div className="cat-title-wrap">
                  <span className="mono" style={{ color: 'var(--ink-muted)', fontSize: 12 }}>
                    A.{pad2(i + 1)}
                  </span>
                  <h2 className="cat-title">{cat.label}</h2>
                </div>
                <div className="cat-meta">
                  {items.length} {items.length === 1 ? 'module' : 'modules'}
                  {cat.desc ? <> · {cat.desc}</> : null}
                </div>
              </div>
              <div className="feature-grid">
                {items.map((feat, idx) => (
                  <FeatureCard key={feat.slug} index={idx} feat={feat} />
                ))}
              </div>
            </section>
          );
        })}

        {filtered.length === 0 && (
          <div
            style={{
              padding: 40,
              textAlign: 'center',
              color: 'var(--ink-muted)',
              fontFamily: 'var(--font-mono)',
              fontSize: 13,
            }}
          >
            // no modules match the current filter
          </div>
        )}
      </div>
    </div>
  );
}
