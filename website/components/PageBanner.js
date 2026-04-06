"use client";
import Link from "next/link";

const PageBanner = ({ pageName, pageLink, parentName, parentLink }) => {
  const handleMouseBtn = (e) => {
    e.preventDefault();
    window.scrollTo({ top: window.innerHeight - 150, behavior: "smooth" });
  };

  return (
    <div className="section started" style={{ height: "96vh" }}>
      <div className="centrize full-width">
        <div className="vertical-center">
          <div className="started-content">
            <div className="h-title glitch-effect" data-text={pageName}>
              {pageName}
            </div>
            <div className="h-subtitle typing-bread">
              <p>
                <Link href="/">Home</Link>
                {parentName && parentLink && (
                  <> / <Link href={parentLink}>{parentName}</Link></>
                )}
                {" "}/ <Link href={pageLink}>{pageName}</Link>
              </p>
            </div>
            <span className="typed-bread" />
          </div>
        </div>
      </div>
      <a href="#" className="mouse_btn" onClick={handleMouseBtn}>
        <span className="ion ion-mouse mouse" />
      </a>
    </div>
  );
};

export default PageBanner;