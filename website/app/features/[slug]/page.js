import { notFound } from 'next/navigation';
import DocSidebar from '@/components/docs/DocSidebar';
import DocArticle from '@/components/docs/DocArticle';
import { getDocSlugs, getFeatureBySlug } from '@/content/features';

export function generateStaticParams() {
  return getDocSlugs().map((slug) => ({ slug }));
}

export function generateMetadata({ params }) {
  const feature = getFeatureBySlug(params.slug);
  if (!feature) return { title: 'Not found — UniGeek' };
  return {
    title: `${feature.title} — UniGeek Docs`,
    description: feature.summary,
  };
}

export default function FeatureDocPage({ params }) {
  const feature = getFeatureBySlug(params.slug);
  if (!feature) notFound();

  return (
    <div className="docs-layout">
      <DocSidebar currentSlug={feature.slug} />
      <DocArticle feature={feature} />
    </div>
  );
}
