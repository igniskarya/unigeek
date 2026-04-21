export default function BoardInfo({ board }) {
  if (!board) return null;
  const modes = board.nav || [];

  return (
    <>
      <div className="step-head" style={{ marginTop: 48 }}>
        <div className="step-num">Info</div>
        <div className="step-title">Board reference</div>
        <div className="step-sub">{board.name}</div>
      </div>
      <div className="board-info">
        <div className="bi-left">
          <div className="bi-head">
            <div className="bi-name">{board.name}</div>
            <div className="bi-chip">{board.chip}</div>
          </div>
          <div className="bi-section-label">Connection</div>
          <div className="bi-kv"><span>USB port</span><span>{board.port || '—'}</span></div>
          <div className="bi-kv"><span>Baud rate</span><span>{board.baud ? `${board.baud.toLocaleString()} 8N1` : '—'}</span></div>
          <div className="bi-kv"><span>Bootloader</span><span>{board.bootloader || '—'}</span></div>
          <div className="bi-kv"><span>Storage</span><span>{board.storage || '—'}</span></div>
          {board.bootNotes ? (
            <div className="bi-note">
              <strong>Note</strong>
              <span>{board.bootNotes}</span>
            </div>
          ) : null}
        </div>
        <div className="bi-right">
          {modes.map((mode, mi) => {
            const [modeTitle, items, modeNote] = mode;
            return (
              <div key={mi} className="bi-mode">
                <div className="bi-section-label">
                  {mi === 0 ? 'Navigation' : 'Alt navigation'} · {modeTitle}
                </div>
                <div className="bi-keymap">
                  {(items || []).map((it, i) => (
                    <div key={i} className="bi-km-entry">
                      <span className="bi-km-input">{it.input}</span>
                      <span className="bi-km-action">{it.action}</span>
                    </div>
                  ))}
                </div>
                {modeNote ? <div className="bi-km-note">{modeNote}</div> : null}
              </div>
            );
          })}
        </div>
      </div>
    </>
  );
}
