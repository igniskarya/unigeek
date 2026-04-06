"use client";
import { useEffect, useRef } from "react";

const FlashLog = ({ text }) => {
  const el = useRef();

  useEffect(() => {
    if (el.current) {
      el.current.scrollTop = el.current.scrollHeight;
    }
  }, [text]);

  return (
    <div className="flash-log" ref={el}>
      {text || "— Ready —"}
    </div>
  );
};

export default FlashLog;