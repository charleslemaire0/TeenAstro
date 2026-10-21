/*
 * LX200LinkLock.h - one recursive lock for the main-unit link on ESP32
 *
 * The UART exchange, the mount-status cache, the web servers that read that
 * cache, and EEPROM commits share this lock so two cores never interleave
 * them. On other targets the guard compiles to nothing.
 *
 * Copyright (C) 2026 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#if defined(ARDUINO_ARCH_ESP32) && !defined(TEENASTRO_NATIVE_BUILD) && !defined(EMU_SHC)
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#define LX200_LINK_LOCK_ENABLED 1

inline SemaphoreHandle_t lx200LinkMutex()
{
  static SemaphoreHandle_t s_mutex = xSemaphoreCreateRecursiveMutex();
  return s_mutex;
}

class LX200LinkGuard
{
public:
  LX200LinkGuard()
  {
    m_locked = xSemaphoreTakeRecursive(lx200LinkMutex(), portMAX_DELAY) == pdTRUE;
  }
  /// Try for at most \p waitMs; check locked() before touching shared state.
  explicit LX200LinkGuard(unsigned long waitMs)
  {
    m_locked = xSemaphoreTakeRecursive(lx200LinkMutex(), pdMS_TO_TICKS(waitMs)) == pdTRUE;
  }
  ~LX200LinkGuard() { unlock(); }
  /// Release early; shared state must not be touched afterwards.
  void unlock()
  {
    if (m_locked) xSemaphoreGiveRecursive(lx200LinkMutex());
    m_locked = false;
  }
  bool locked() const { return m_locked; }
  LX200LinkGuard(const LX200LinkGuard&) = delete;
  LX200LinkGuard& operator=(const LX200LinkGuard&) = delete;
private:
  bool m_locked = false;
};

#else

class LX200LinkGuard
{
public:
  LX200LinkGuard() {}
  explicit LX200LinkGuard(unsigned long) {}
  void unlock() {}
  bool locked() const { return true; }
};

#endif
