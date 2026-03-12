/*
 * StreamTransport.h - Arduino Stream implementation of IX200Transport
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include "IX200Transport.h"
#include <Arduino.h>

/// Wraps an Arduino Stream to implement IX200Transport.
class StreamTransport : public IX200Transport
{
public:
  explicit StreamTransport(Stream& stream) : m_stream(stream) {}

  void flush() override { m_stream.flush(); }
  size_t write(const uint8_t* buf, size_t len) override {
    size_t n = 0;
    while (n < len) { m_stream.write(buf[n++]); }
    return n;
  }
  int available() override { return m_stream.available(); }
  int read() override { return m_stream.read(); }
  void setTimeout(unsigned long ms) override { m_stream.setTimeout(ms); }

private:
  Stream& m_stream;
};
