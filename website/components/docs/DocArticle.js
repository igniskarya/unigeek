import Link from 'next/link';

export default function DocArticle({ feature }) {
  const { title, summary, html, category, stable, requires, prev, next } = feature;

  return (
    <article className="doc-body">
      <div className="doc-breadcrumb">
        <Link href="/">Home</Link>
        <span className="sep">/</span>
        <Link href="/features">Features</Link>
        <span className="sep">/</span>
        {category ? (
          <>
            <Link href={`/features#${category.id}`}>{category.label}</Link>
            <span className="sep">/</span>
          </>
        ) : null}
        <span style={{ color: 'var(--ink)' }}>{title}</span>
      </div>

      <header className="doc-head">
        <h1 className="doc-title">{title}</h1>
        {summary ? <p className="doc-sub">{summary}</p> : null}
        <div className="doc-meta">
          {stable === false ? (
            <span className="badge badge-warn">BETA</span>
          ) : (
            <span className="badge badge-ok">STABLE</span>
          )}
          {category ? (
            <Link href={`/features#${category.id}`} className="badge">
              {category.label}
            </Link>
          ) : null}
          {Array.isArray(requires)
            ? requires.map((req) => (
                <span key={req} className="badge">REQ · {req}</span>
              ))
            : null}
        </div>
      </header>

      <div dangerouslySetInnerHTML={{ __html: html }} />

      <nav className="doc-nav">
        {prev ? (
          <Link href={`/features/${prev.slug}`}>
            <div className="label">← Previous</div>
            <div className="t">{prev.title}</div>
          </Link>
        ) : (
          <div />
        )}
        {next ? (
          <Link href={`/features/${next.slug}`}>
            <div className="label">Next →</div>
            <div className="t">{next.title}</div>
          </Link>
        ) : (
          <div />
        )}
      </nav>
    </article>
  );
}
