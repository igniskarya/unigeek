import { FIRMWARE_VERSION } from '@/content/meta';

export default function KnownIssues({ boards }) {
  const items = [];
  for (const b of boards) {
    if (!b.knownIssues || b.knownIssues.length === 0) continue;
    for (const note of b.knownIssues) {
      items.push({ board: b.name, note });
    }
  }
  if (items.length === 0) return null;

  return (
    <div className="known-issues">
      <div className="marker">Known issues · v{FIRMWARE_VERSION}</div>
      <ul>
        {items.map((it, i) => (
          <li key={i}>
            <code>{it.board}</code> — {it.note}
          </li>
        ))}
      </ul>
    </div>
  );
}
