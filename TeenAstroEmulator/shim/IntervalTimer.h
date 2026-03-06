/*
 * IntervalTimer.h - Teensy IntervalTimer shim for native builds.
 *
 * Two modes:
 *   1. Manual mode (tests): call fire() to invoke the callback.
 *   2. Auto-fire mode (emulator): a background thread fires callbacks
 *      at the configured interval using wall-clock time.
 *
 * Call IntervalTimerThread::start() once to enable auto-fire for all
 * IntervalTimer instances, and IntervalTimerThread::stop() to shut down.
 */
#pragma once
#include <cstdint>
#include <cstdio>

#ifdef EMU_MAINUNIT
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>
#endif

class IntervalTimer {
public:
    typedef void (*callback_t)();
    callback_t  cb_ = nullptr;
    float       interval_us_ = 0;
    int         prio_ = 128;

    bool begin(callback_t cb, int us) { return begin(cb, (float)us); }
    bool begin(callback_t cb, float us) {
        cb_ = cb;
        interval_us_ = us;
        registerSelf();
        return true;
    }
    bool begin(callback_t cb, double us) { return begin(cb, (float)us); }
    void update(float us) { interval_us_ = us; }
    void update(double us) { interval_us_ = (float)us; }
    void end() { cb_ = nullptr; }
    void priority(int p) { prio_ = p; }

    void fire() { if (cb_) cb_(); }

private:
    void registerSelf();
};

/* ------------------------------------------------------------------ */
/*  Background timer thread (emulator mode only)                       */
/* ------------------------------------------------------------------ */
#ifdef EMU_MAINUNIT

class IntervalTimerThread {
public:
    static IntervalTimerThread& instance() {
        static IntervalTimerThread inst;
        return inst;
    }

    void registerTimer(IntervalTimer* t) {
        std::lock_guard<std::mutex> lk(mtx_);
        for (auto* existing : timers_)
            if (existing == t) return;
        timers_.push_back(t);
    }

    void start() {
        if (running_.load()) return;
        running_ = true;
        thread_ = std::thread([this]() { run(); });
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

    std::mutex& mutex() { return fire_mtx_; }

private:
    IntervalTimerThread() = default;
    ~IntervalTimerThread() { stop(); }

    void run() {
        using clock = std::chrono::steady_clock;
        using us = std::chrono::microseconds;

        struct TimerState { double next_us = 0; };
        std::vector<TimerState> states;

        auto epoch = clock::now();

        while (running_.load()) {
            auto now_us = (double)std::chrono::duration_cast<us>(clock::now() - epoch).count();

            std::lock_guard<std::mutex> lk(mtx_);
            while (states.size() < timers_.size())
                states.push_back({now_us});

            double earliest = now_us + 1e9;
            for (size_t i = 0; i < timers_.size(); i++) {
                IntervalTimer* t = timers_[i];
                if (!t->cb_ || t->interval_us_ <= 0) continue;
                while (states[i].next_us <= now_us) {
                    {
                        std::lock_guard<std::mutex> flk(fire_mtx_);
                        t->cb_();
                    }
                    states[i].next_us += t->interval_us_;
                }
                if (states[i].next_us < earliest)
                    earliest = states[i].next_us;
            }

            double sleep_us = earliest - now_us;
            if (sleep_us > 50) sleep_us = 50;
            if (sleep_us > 0)
                std::this_thread::sleep_for(us((int)sleep_us));
        }
    }

    std::mutex mtx_;
    std::mutex fire_mtx_;
    std::vector<IntervalTimer*> timers_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

inline void IntervalTimer::registerSelf() {
    IntervalTimerThread::instance().registerTimer(this);
}

#else
// Non-emulator mode: no auto-fire
inline void IntervalTimer::registerSelf() {}
#endif
