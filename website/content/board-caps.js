// Short capability label used on board chips in compact lists.
export const CAP_LABEL = {
  "WiFi": "WiFi",
  "BLE": "BLE",
  "NFC": "NFC",
  "IR": "IR",
  "Sub-GHz": "SubG",
  "LoRa": "LoRa",
  "NRF24L01": "NRF24",
  "Encoder": "ENC",
  "Keyboard": "KBD",
  "Touch": "TCH",
  "USB HID": "HID",
};

export function shortChip(chip) {
  if (!chip) return "";
  if (chip.includes("S3")) return "S3";
  return chip;
}

export function toCaps(tags = []) {
  return tags.map((t) => CAP_LABEL[t] || t);
}
