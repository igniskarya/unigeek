"use client";
import Link from "next/link";
import { useEffect, useRef, useState } from "react";

const MENUS = [
  { key: "wifi",     label: "WiFi",     icon: "ion-wifi" },
  { key: "ble",      label: "BLE",      icon: "ion-bluetooth" },
  { key: "keyboard", label: "Keyboard", icon: "ion-ios-keypad" },
  { key: "module",   label: "Modules",  icon: "ion-cube" },
  { key: "utility",  label: "Utility",  icon: "ion-ios-settings" },
  { key: "game",     label: "Games",    icon: "ion-ios-game-controller-a" },
  { key: "setting",  label: "Settings", icon: "ion-ios-gear" },
];

const FeatureGrid = ({ features }) => {
  const [active, setActive] = useState("wifi");
  const listRef = useRef(null);
  const initialized = useRef(false);

  const filtered = features.filter((f) => f.category === active);

  useEffect(() => {
    if (!initialized.current) { initialized.current = true; return; }
    if (listRef.current) {
      const top = listRef.current.getBoundingClientRect().top + window.scrollY - 150;
      window.scrollTo({ top, behavior: "smooth" });
    }
  }, [active]);

  const countFor = (key) => features.filter((f) => f.category === key).length;

  return (
    <div className="section blog section_" id="section-features">
      <div className="content">

        {/* Big category menu */}
        <div className="box-items menu-grid">
          {MENUS.map((m) => (
            <div
              key={m.key}
              className="box-item"
              onClick={() => setActive(m.key)}
              style={{ cursor: "pointer" }}
            >
              <div
                className="image"
                style={{
                  display: "flex",
                  alignItems: "center",
                  justifyContent: "center",
                  height: 120,
                  background: active === m.key ? "rgba(245,166,35,0.08)" : "transparent",
                  borderBottom: active === m.key
                    ? "2px solid var(--color-main, #f5a623)"
                    : "2px solid transparent",
                  transition: "all 0.2s",
                }}
              >
                <span
                  className={m.icon}
                  style={{
                    fontSize: 36,
                    color: active === m.key
                      ? "var(--color-main, #f5a623)"
                      : "#666",
                    transition: "color 0.2s",
                  }}
                />
              </div>
              <div className="desc">
                <div
                  className="name"
                  style={{
                    color: active === m.key
                      ? "var(--color-main, #f5a623)"
                      : undefined,
                  }}
                >
                  {m.label}
                </div>
                <div style={{ fontSize: "0.75em", color: "#666", marginTop: 2 }}>
                  {countFor(m.key)} features
                </div>
              </div>
            </div>
          ))}
        </div>

        {/* Feature list for selected category */}
        <div ref={listRef} style={{ marginTop: 32 }}>
            <div className="title">
              <div className="title_inner">
                {MENUS.find((m) => m.key === active)?.label}
              </div>
            </div>
            <div className="box-items">
              {filtered.map((f) => (
                <div key={f.slug} className="box-item">
                  <div className="desc">
                    <div
                      style={{
                        display: "flex",
                        justifyContent: "space-between",
                        alignItems: "center",
                        marginBottom: 4,
                      }}
                    >
                      {f.hasDetail ? (
                        <span
                          style={{
                            fontSize: "0.65em",
                            textTransform: "uppercase",
                            letterSpacing: "1px",
                            color: "var(--color-main, #f5a623)",
                            border: "1px solid var(--color-main, #f5a623)",
                            padding: "1px 5px",
                          }}
                        >
                          docs
                        </span>
                      ) : (
                        <span
                          style={{
                            fontSize: "0.65em",
                            textTransform: "uppercase",
                            letterSpacing: "1px",
                            color: "#555",
                            border: "1px solid #333",
                            padding: "1px 5px",
                          }}
                        >
                          soon
                        </span>
                      )}
                    </div>
                    {f.hasDetail ? (
                      <Link href={`/features/${f.slug}`} className="name">
                        {f.title}
                      </Link>
                    ) : (
                      <span
                        className="name"
                        style={{ color: "#aaa", cursor: "default" }}
                      >
                        {f.title}
                      </span>
                    )}
                    <p style={{ fontSize: "0.85em", color: "#888", marginTop: 6 }}>
                      {f.summary}
                    </p>
                  </div>
                </div>
              ))}
            </div>
          </div>

        <div className="clear" />
      </div>
    </div>
  );
};

export default FeatureGrid;