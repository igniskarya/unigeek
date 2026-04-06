import Preloader from "@/layouts/Preloader";
import "@css/animate.css";
import "@css/glitche-basic.css";
import "@css/glitche-layout.css";
import "@css/ionicons.css";
import "@css/magnific-popup.css";
import "@css/template-colors/blue.css";
import "@css/template-dark/dark.css";
import { Roboto_Mono } from "next/font/google";
import "./globals.css";
import State from "/context/context";

const robotoMono = Roboto_Mono({
  subsets: ["latin"],
  weight: ["100", "300", "400", "500", "700"],
  variable: "--font-roboto",
  display: "swap",
});

export const metadata = {
  title: "UniGeek — ESP32 Multi-Tool Firmware",
  description:
    "Open-source ESP32 firmware for WiFi, BLE, NFC, IR, Sub-GHz attacks and utilities. Supports 7 boards.",
  icons: {
    icon: "/images/icon.png",
    apple: "/images/icon.png",
  },
};

export default function RootLayout({ children }) {
  return (
    <html lang="en">
      <body className={robotoMono.variable}>
        <Preloader />
        <State>{children}</State>
      </body>
    </html>
  );
}