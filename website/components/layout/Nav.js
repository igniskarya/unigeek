'use client';

import Link from 'next/link';
import { usePathname } from 'next/navigation';
import { useEffect, useState } from 'react';
import { FIRMWARE_VERSION, FIRMWARE_CHANNEL } from '@/content/meta';

const LINKS = [
  { href: '/', label: 'Home' },
  { href: '/features', label: 'Features' },
  { href: '/releases', label: 'Releases' },
  { href: '/install', label: 'Install' },
];

function isActive(pathname, href) {
  if (href === '/') return pathname === '/' || pathname === '';
  return pathname === href || pathname.startsWith(`${href}/`);
}

export default function Nav() {
  const pathname = usePathname() || '/';
  const [open, setOpen] = useState(false);

  useEffect(() => {
    setOpen(false);
  }, [pathname]);

  useEffect(() => {
    if (typeof document === 'undefined') return;
    document.body.style.overflow = open ? 'hidden' : '';
    return () => {
      document.body.style.overflow = '';
    };
  }, [open]);

  useEffect(() => {
    if (!open) return;
    const onKey = (e) => {
      if (e.key === 'Escape') setOpen(false);
    };
    const mq = window.matchMedia('(min-width: 821px)');
    const onResize = (ev) => {
      if (ev.matches) setOpen(false);
    };
    document.addEventListener('keydown', onKey);
    mq.addEventListener('change', onResize);
    return () => {
      document.removeEventListener('keydown', onKey);
      mq.removeEventListener('change', onResize);
    };
  }, [open]);

  return (
    <nav className="nav">
      <div className="nav-inner">
        <Link className="brand" href="/">
          <span className="brand-mark" aria-hidden="true" />
          <span className="brand-name">
            UNI<span>GEEK</span>
          </span>
        </Link>
        <div className={`nav-links${open ? ' open' : ''}`} id="nav-links">
          {LINKS.map((link) => (
            <Link
              key={link.href}
              href={link.href}
              className={`nav-link${isActive(pathname, link.href) ? ' active' : ''}`}
              onClick={() => setOpen(false)}
            >
              {link.label}
            </Link>
          ))}
          <div className="nav-mobile-status">
            <span className="status-dot" aria-hidden="true" />
            <span>
              v{FIRMWARE_VERSION} · {FIRMWARE_CHANNEL}
            </span>
          </div>
        </div>
        <div className="nav-status">
          <span className="status-dot" aria-hidden="true" />
          <span>
            v{FIRMWARE_VERSION} · {FIRMWARE_CHANNEL}
          </span>
        </div>
        <button
          type="button"
          className="nav-toggle"
          aria-label="Toggle menu"
          aria-expanded={open ? 'true' : 'false'}
          aria-controls="nav-links"
          onClick={() => setOpen((v) => !v)}
        >
          <span className="bars"><span /></span>
        </button>
      </div>
    </nav>
  );
}
