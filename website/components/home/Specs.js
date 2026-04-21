export default function Specs({ moduleCount, boardCount }) {
  return (
    <section style={{ padding: '40px 0' }}>
      <div className="specs">
        <div className="spec">
          <div className="spec-num">[01]</div>
          <div className="spec-val">{moduleCount}<sub>modules</sub></div>
          <div className="spec-label">Feature count</div>
        </div>
        <div className="spec">
          <div className="spec-num">[02]</div>
          <div className="spec-val">{boardCount}<sub>boards</sub></div>
          <div className="spec-label">Target hardware</div>
        </div>
        <div className="spec">
          <div className="spec-num">[03]</div>
          <div className="spec-val">7<sub>radios</sub></div>
          <div className="spec-label">WiFi · BLE · NFC · IR · Sub-GHz · LoRa · NRF24</div>
        </div>
        <div className="spec">
          <div className="spec-num">[04]</div>
          <div className="spec-val">0<sub>cloud</sub></div>
          <div className="spec-label">Fully offline</div>
        </div>
      </div>
    </section>
  );
}
