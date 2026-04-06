"use client";

const BoardPicker = ({ boards, selected, onSelect }) => {
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
    </div>
  );
};

export default BoardPicker;