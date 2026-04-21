import FeatureCatalog from '@/components/features/FeatureCatalog';
import { CATALOG, CATEGORIES, countsByCategory } from '@/content/features/catalog';

export const metadata = {
  title: 'Features — UniGeek',
  description:
    'Every module available across the UniGeek firmware. Browse by category or search for a specific tool.',
};

export default function FeaturesPage() {
  const counts = countsByCategory();
  return (
    <>
      <header className="page-header">
        <div className="page-header-meta">
          <span>// features · module catalog</span>
          <span>
            {CATALOG.length} modules · {CATEGORIES.length} categories
          </span>
        </div>
        <h1 className="page-title">Features</h1>
        <p className="page-subtitle">
          Every module available across the UniGeek firmware. Click{' '}
          <span className="mono" style={{ color: 'var(--accent)' }}>DOCS</span>{' '}
          entries for documentation, or watch{' '}
          <span className="mono" style={{ color: 'var(--ink-muted)' }}>SOON</span>{' '}
          entries in the changelog.
        </p>
      </header>

      <FeatureCatalog categories={CATEGORIES} features={CATALOG} counts={counts} />
    </>
  );
}
