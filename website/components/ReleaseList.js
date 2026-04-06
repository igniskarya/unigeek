"use client";
import { MDXRemote } from "next-mdx-remote";
import { useState } from "react";

const ReleaseList = ({ releases }) => {
  const [open, setOpen] = useState(releases[0]?.version ?? null);

  return (
    <div className="section blog">
      <div className="content">
        {releases.map(({ version, mdxSource }) => (
          <div
            key={version}
            style={{ borderBottom: "1px solid #2a2a2a", marginBottom: 0 }}
          >
            {/* Version header row */}
            <div
              onClick={() => setOpen(open === version ? null : version)}
              style={{
                display: "flex",
                alignItems: "center",
                justifyContent: "space-between",
                padding: "18px 0",
                cursor: "pointer",
              }}
            >
              <div style={{ display: "flex", alignItems: "center", gap: 16 }}>
                <span
                  className="h-title"
                  style={{
                    fontSize: "1.2em",
                    color: open === version ? "var(--color-main, #0856c1)" : "#fff",
                    transition: "color 0.2s",
                  }}
                >
                  v{version}
                </span>
                <a
                  href={`https://github.com/lshaf/unigeek/releases/tag/${version}`}
                  target="_blank"
                  rel="noreferrer"
                  onClick={(e) => e.stopPropagation()}
                  style={{
                    fontSize: "0.7em",
                    textTransform: "uppercase",
                    letterSpacing: "1px",
                    border: "1px solid #444",
                    color: "#888",
                    padding: "2px 8px",
                    textDecoration: "none",
                    transition: "border-color 0.2s, color 0.2s",
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.borderColor = "var(--color-main, #0856c1)";
                    e.currentTarget.style.color = "var(--color-main, #0856c1)";
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.borderColor = "#444";
                    e.currentTarget.style.color = "#888";
                  }}
                >
                  GitHub Release
                </a>
              </div>
              <span
                className={open === version ? "ion ion-chevron-up" : "ion ion-chevron-down"}
                style={{ color: "#555", fontSize: "1.1em" }}
              />
            </div>

            {/* Release content */}
            {open === version && (
              <div
                className="single-post-text"
                style={{ paddingBottom: 32 }}
              >
                <MDXRemote {...mdxSource} />
              </div>
            )}
          </div>
        ))}
        <div className="clear" />
      </div>
    </div>
  );
};

export default ReleaseList;