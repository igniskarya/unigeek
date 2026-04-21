import ReleaseList from '@/components/releases/ReleaseList';
import { getAllReleases } from '@/content/releases';

export const metadata = {
  title: 'Releases — UniGeek',
  description:
    'A chronological record of every firmware release. Follow along or jump to a version.',
};

export default function ReleasesPage() {
  const releases = getAllReleases();
  return (
    <>
      <header className="page-header">
        <div className="page-header-meta">
          <span>// releases · changelog</span>
          <span>MIT-licensed · open source</span>
        </div>
        <h1 className="page-title">Release notes</h1>
        <p className="page-subtitle">
          A chronological record of every firmware release. Follow along or jump to a version.
        </p>
      </header>

      <ReleaseList releases={releases} />
    </>
  );
}
