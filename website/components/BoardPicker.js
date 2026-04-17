"use client";

const BoardPicker = ({ boards, selected, onSelect }) => {
  const withIssues = boards.filter((b) => b.knownIssues?.length > 0);

  return (
    <div>
      <div className="install-step">
        <div className="step-label">Step 1 — Select your board</div>
        <div className="board-grid">
          {boards.map((board) => (
            <div
              key={board.id}
              className={`board-card${selected?.id === board.id ? " selected" : ""}`}
              onClick={() => onSelect(board)}
            >
              <div className="board-name">{board.name}</div>
              <div className="board-chip">{board.chip}</div>
              <div className="board-tags">
                {board.tags.map((tag) => (
                  <span key={tag} className="tag">
                    {tag}
                  </span>
                ))}
              </div>
            </div>
          ))}
        </div>
      </div>

      {withIssues.length > 0 && (
        <div className="install-step known-issues-section">
          <div className="step-label">⚠ Known issues</div>
          <ul className="known-issues-list">
            {withIssues.map((board) => (
              <li key={board.id}>
                <span className="known-issues-board">{board.name}</span>
                <span className="known-issues-sep">—</span>
                <span className="known-issues-text">
                  {board.knownIssues.join("; ")}
                </span>
              </li>
            ))}
          </ul>
        </div>
      )}
    </div>
  );
};

export default BoardPicker;