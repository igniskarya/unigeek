import fs from "fs";
import path from "path";
import { CATALOG } from "./catalog";

const knowledgeDir = path.join(process.cwd(), "../knowledge");

export function getAllFeatures() {
  return CATALOG;
}

export function getFeatureBySlug(slug) {
  const entry = CATALOG.find((f) => f.slug === slug);
  if (!entry || !entry.hasDetail) return null;

  const filePath = path.join(knowledgeDir, `${slug}.md`);
  if (!fs.existsSync(filePath)) return null;

  // Strip the first H1 line — title comes from catalog
  const raw = fs.readFileSync(filePath, "utf8");
  const content = raw.replace(/^#\s+.+\n/, "");
  return { ...entry, content };
}