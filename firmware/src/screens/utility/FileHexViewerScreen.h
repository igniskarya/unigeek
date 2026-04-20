#pragma once

#include "ui/templates/BaseScreen.h"

class FileHexViewerScreen : public BaseScreen
{
public:
  FileHexViewerScreen(const String& path) : _path(path) {}

  const char* title() override { return _titleBuf; }

  void onInit() override;
  void onUpdate() override;
  void onRender() override;

private:
  static constexpr uint8_t kMaxBytesPerRow = 16;
  static constexpr uint8_t kCharW = 6;
  static constexpr uint8_t kLineH = 10;

  String   _path;
  char     _titleBuf[32] = "Hex Viewer";

  size_t   _fileSize    = 0;
  uint32_t _byteOffset  = 0;
  uint8_t  _bytesPerRow = 8;
  uint8_t  _visibleRows = 0;

  void _renderHex();
  void _goBack();
};