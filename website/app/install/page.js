import InstallFlow from '@/components/install/InstallFlow';
import KnownIssues from '@/components/install/KnownIssues';
import Requirements from '@/components/install/Requirements';
import { BOARDS } from '@/content/boards';
import { FIRMWARE_VERSION, BUILD_ID } from '@/content/meta';

export const metadata = {
  title: 'Install — UniGeek',
  description: 'Pick your board, pick a method, and flash. Web-based or manual download — both supported.',
};

export default function InstallPage() {
  return (
    <>
      <header className="page-header">
        <div className="page-header-meta">
          <span>// install · flash · boot</span>
          <span>Firmware v{FIRMWARE_VERSION} · {BUILD_ID}</span>
        </div>
        <h1 className="page-title">Install firmware</h1>
        <p className="page-subtitle">
          Pick your board, pick a method, and flash. Web-based or manual download — both supported.
        </p>
      </header>

      <div className="install-wrap">
        <KnownIssues boards={BOARDS} />
        <InstallFlow boards={BOARDS} />
        <Requirements />
      </div>
    </>
  );
}
