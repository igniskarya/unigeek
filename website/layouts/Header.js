"use client";
import Link from "next/link";
import { useEffect, useState } from "react";

const Header = () => {
  const [toggle, setToggle] = useState(false);
  const [pathname, setPathname] = useState("/");

  useEffect(() => {
    setPathname(window.location.pathname);
  }, []);

  return (
    <header className={toggle ? "active" : ""}>
      <div className="head-top">
        <a
          href="#"
          className="menu-btn"
          onClick={(e) => {
            e.preventDefault();
            setToggle(!toggle);
          }}
        >
          <span />
        </a>
        <div className="logo">
          <Link href="/">
            <img src="/images/logo.png" alt="UniGeek" />
          </Link>
        </div>
        <div className="top-menu">
          <ul>
            <li className={pathname === "/" ? "active" : ""}>
              <Link href="/" className="lnk">
                Home
              </Link>
            </li>
            <li className={pathname.includes("/features") ? "active" : ""}>
              <Link href="/features" className="lnk">
                Features
              </Link>
            </li>
            <li className={pathname.includes("/release-notes") ? "active" : ""}>
              <Link href="/release-notes" className="lnk">
                Releases
              </Link>
            </li>
            <li>
              <Link href="/install" className="btn">
                Install
              </Link>
            </li>
          </ul>
        </div>
      </div>
    </header>
  );
};

export default Header;