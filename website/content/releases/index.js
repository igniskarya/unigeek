import fs from "fs";
import path from "path";

const releasesDir = path.join(process.cwd(), "../release-notes");

export function getAllReleases() {
  const files = fs
    .readdirSync(releasesDir)
    .filter((f) => f.endsWith(".md"));

  return files
    .map((filename) => {
      const version = filename.replace(".md", "");
      const content = fs.readFileSync(path.join(releasesDir, filename), "utf8");
      return { version, content };
    })
    .sort((a, b) => {
      // Sort descending: newest first
      const pa = a.version.split(".").map(Number);
      const pb = b.version.split(".").map(Number);
      for (let i = 0; i < 3; i++) {
        if ((pb[i] ?? 0) !== (pa[i] ?? 0)) return (pb[i] ?? 0) - (pa[i] ?? 0);
      }
      return 0;
    });
}