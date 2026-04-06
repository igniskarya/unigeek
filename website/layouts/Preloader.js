"use client";
import { useEffect, useState } from "react";

const Preloader = () => {
  const [text] = useState("loading ...");

  useEffect(() => {
    const preInner = document.querySelector(".preloader .pre-inner");
    if (!preInner) return;

    let opacity = 1;
    const interval = 1000 / 60;
    const step = opacity / (800 / interval);

    const fadeInterval = setInterval(() => {
      opacity -= step;
      preInner.style.opacity = opacity;
      if (opacity <= 0) {
        clearInterval(fadeInterval);
        const preloader = document.querySelector(".preloader");
        if (preloader) preloader.style.display = "none";
        document.body.classList.add("loaded");
      }
    }, interval);

    return () => clearInterval(fadeInterval);
  }, []);

  return (
    <div className="preloader">
      <div className="centrize full-width">
        <div className="vertical-center">
          <div className="pre-inner">
            <div className="load typing-load">
              <p>{text}</p>
            </div>
            <span className="typed-load" />
          </div>
        </div>
      </div>
    </div>
  );
};

export default Preloader;