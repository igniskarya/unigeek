"use client";
import Flasher from "@/components/Flasher";
import PageBanner from "@/components/PageBanner";
import BoardPicker from "@/components/BoardPicker";
import GlitcheLayout from "@/layouts/GlitcheLayout";
import { BOARDS } from "@/content/boards";
import { useEffect, useRef, useState } from "react";

const version = process.env.NEXT_PUBLIC_FIRMWARE_VERSION || "dev";

const InstallPage = () => {
  const [selectedBoard, setSelectedBoard] = useState(null);
  const flasherRef = useRef(null);

  useEffect(() => {
    if (selectedBoard && flasherRef.current) {
      const top = flasherRef.current.getBoundingClientRect().top + window.scrollY - 150;
      window.scrollTo({ top, behavior: "smooth" });
    }
  }, [selectedBoard]);

  return (
    <GlitcheLayout>
      <PageBanner pageName="Install" pageLink="/install" />
      <div className="section blog section_">
        <div className="content">
          <div style={{ marginBottom: "24px" }}>
            <div className="step-label">Firmware version</div>
            <div style={{ fontSize: "1.1em", color: "#fff", letterSpacing: "1px" }}>
              {version}
            </div>
          </div>
          <BoardPicker
            boards={BOARDS}
            selected={selectedBoard}
            onSelect={setSelectedBoard}
          />
          <div ref={flasherRef}>
            {selectedBoard && <Flasher board={selectedBoard} />}
          </div>
        </div>
      </div>
    </GlitcheLayout>
  );
};

export default InstallPage;