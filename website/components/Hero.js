"use client";
import Link from "next/link";
import { useEffect } from "react";
import Typed from "typed.js";

const Hero = () => {
  useEffect(() => {
    const timer = setTimeout(() => {
      new Typed(".typed-subtitle", {
        strings: [
          "ESP32 multi-tool firmware.",
          "7 boards, one codebase.",
          "WiFi &bull; BLE &bull; NFC &bull; IR &bull; Sub-GHz",
        ],
        loop: true,
        typeSpeed: 60,
        backSpeed: 30,
      });
    }, 500);
    return () => clearTimeout(timer);
  }, []);

  return (
    <div className="section started" style={{ height: "96vh" }}>
      <div className="centrize full-width">
        <div className="vertical-center">
          <div className="started-content">
            <div className="h-title glitch-effect" data-text="UniGeek">
              Uni<span>Geek</span>
            </div>
            <span className="typed-subtitle" />
            <div
              style={{
                display: "flex",
                gap: "12px",
                marginTop: "28px",
                flexWrap: "wrap",
              }}
            >
              <Link href="/features" className="btn">
                Browse Features
              </Link>
              <Link href="/install" className="btn fill">
                Install Now
              </Link>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Hero;