import Link from 'next/link';
import { getDocsByCategory } from '@/content/features';

export default function DocSidebar({ currentSlug }) {
  const groups = getDocsByCategory();
  return (
    <aside className="docs-side">
      {groups.map((g) => (
        <div key={g.id} className="docs-side-section">
          <div className="docs-side-head">
            <span>{g.label}</span>
            <span>{g.features.length}</span>
          </div>
          {g.features.map((f) => (
            <Link
              key={f.slug}
              href={`/features/${f.slug}`}
              className={`docs-side-link${f.slug === currentSlug ? ' active' : ''}`}
            >
              {f.title}
            </Link>
          ))}
        </div>
      ))}
    </aside>
  );
}
