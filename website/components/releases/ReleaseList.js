import ReleaseItem from './ReleaseItem';
import { FIRMWARE_VERSION } from '@/content/meta';

function tagFor(version, latestVersion) {
  return version === latestVersion ? 'latest' : 'archived';
}

function formatSideDate(iso) {
  if (!iso) return null;
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return null;
  return d.toLocaleDateString('en-US', {
    year: 'numeric',
    month: 'short',
    day: 'numeric',
  });
}

export default function ReleaseList({ releases }) {
  // Prefer the env-pinned firmware version; if it isn't in the release list
  // (pre-publish or mismatched tag), fall back to the newest by sort order.
  const latestVersion = releases.some((r) => r.version === FIRMWARE_VERSION)
    ? FIRMWARE_VERSION
    : releases[0]?.version;

  return (
    <div className="rel-layout">
      <aside className="rel-side">
        <div className="rel-side-head">
          <span>Versions</span>
          <span>{releases.length}</span>
        </div>
        <div className="rel-side-list">
          {releases.map((r) => {
            const dateLabel = formatSideDate(r.date);
            const isLatest = r.version === latestVersion;
            return (
              <a
                key={r.version}
                href={`#v${r.version}`}
                className={`rel-side-item${isLatest ? ' active' : ''}`}
              >
                <span className="v">
                  v{r.version}
                  {isLatest ? ' · latest' : ''}
                </span>
                {dateLabel && <span className="d">{dateLabel}</span>}
              </a>
            );
          })}
        </div>
      </aside>

      <div>
        {releases.map((r) => (
          <ReleaseItem
            key={r.version}
            release={r}
            tag={tagFor(r.version, latestVersion)}
          />
        ))}
      </div>
    </div>
  );
}
