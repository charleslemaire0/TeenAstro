/*
 * test_align_state.cpp - Unit tests for SHC 2-star alignment state machine
 *
 * Verifies TeenAstroMountStatus alignment flow: start at home, select star 1,
 * slew, recenter, add star 1; select star 2, slew, recenter, add star 2 -> done.
 */

#include "CommandCodec.cpp"
#include "TeenAstroMath.h"

void gethms(const long& v, uint8_t& h, uint8_t& m, uint8_t& s) {
    s = v % 60;
    m = (v / 60) % 60;
    h = v / 3600;
}
void getdms(const long& v, bool& ispos, uint16_t& deg, uint8_t& min, uint8_t& sec) {
    ispos = v >= 0;
    long vabs = ispos ? v : -v;
    sec = vabs % 60;
    min = (vabs / 60) % 60;
    deg = vabs / 3600;
}

#include "LX200Client.cpp"

#include "TeenAstroMountStatus.cpp"

#include <unity.h>
#include <cstring>

static const int MOCK_BUF_SIZE = 512;
static const int MOCK_MAX_RESPONSES = 16;

class MockStream : public Stream {
public:
    MockStream() { reset(); }
    void reset() {
        m_readLen = 0; m_readPos = 0;
        m_outLen = 0;
        m_queueHead = 0; m_queueTail = 0;
        memset(m_readBuf, 0, sizeof(m_readBuf));
        memset(m_outBuf, 0, sizeof(m_outBuf));
    }
    void loadResponse(const char* data) {
        int idx = m_queueTail % MOCK_MAX_RESPONSES;
        strncpy(m_responseQueue[idx], data, MOCK_BUF_SIZE - 1);
        m_responseQueue[idx][MOCK_BUF_SIZE - 1] = '\0';
        m_queueTail++;
    }
    const char* getSent() const { return m_outBuf; }
    int available() override { return m_readLen - m_readPos; }
    int read() override {
        if (m_readPos >= m_readLen) return -1;
        return (unsigned char)m_readBuf[m_readPos++];
    }
    int peek() override {
        if (m_readPos >= m_readLen) return -1;
        return (unsigned char)m_readBuf[m_readPos];
    }
    size_t write(uint8_t b) override {
        if (m_outLen < MOCK_BUF_SIZE - 1) {
            m_outBuf[m_outLen++] = (char)b;
            m_outBuf[m_outLen] = '\0';
        }
        if (b == '#') deliverNextResponse();
        return 1;
    }
    void flush() override {}

private:
    char m_readBuf[MOCK_BUF_SIZE];
    int  m_readLen, m_readPos;
    char m_outBuf[MOCK_BUF_SIZE];
    int  m_outLen;
    char m_responseQueue[MOCK_MAX_RESPONSES][MOCK_BUF_SIZE];
    int  m_queueHead, m_queueTail;
    void deliverNextResponse() {
        if (m_queueHead >= m_queueTail) return;
        int idx = m_queueHead % MOCK_MAX_RESPONSES;
        int len = (int)strlen(m_responseQueue[idx]);
        memcpy(m_readBuf, m_responseQueue[idx], len);
        m_readLen = len;
        m_readPos = 0;
        m_queueHead++;
    }
};

static MockStream mockStream;
static LX200Client* client = nullptr;
static TeenAstroMountStatus* status = nullptr;

static void prepareSetOk() { mockStream.loadResponse("1"); }

void setUp(void) {
    mockStream.reset();
    if (client) delete client;
    if (status) delete status;
    client = new LX200Client(mockStream, 50);
    status = new TeenAstroMountStatus();
    status->setClient(*client);
}

void tearDown(void) {
    if (status) { delete status; status = nullptr; }
    if (client) { delete client; client = nullptr; }
}

// ---- 2-star alignment state machine tests ----

void test_align_start_2_stars_enters_select_star_1(void) {
    status->startAlign(TeenAstroMountStatus::ALIM_TWO);
    TEST_ASSERT_TRUE(status->isAligning());
    TEST_ASSERT_TRUE(status->isAlignSelect());
    TEST_ASSERT_EQUAL(1, status->getAlignStar());
}

void test_align_next_step_select_to_slew(void) {
    status->startAlign(TeenAstroMountStatus::ALIM_TWO);
    status->nextStepAlign();
    TEST_ASSERT_TRUE(status->isAlignSlew());
}

void test_align_next_step_slew_to_recenter(void) {
    status->startAlign(TeenAstroMountStatus::ALIM_TWO);
    status->nextStepAlign();
    status->nextStepAlign();
    TEST_ASSERT_TRUE(status->isAlignRecenter());
}

void test_align_add_star_1_then_select_star_2(void) {
    status->startAlign(TeenAstroMountStatus::ALIM_TWO);
    status->nextStepAlign();
    status->nextStepAlign();
    prepareSetOk();
    TeenAstroMountStatus::AlignReply ret = status->addStar();
    TEST_ASSERT_EQUAL(TeenAstroMountStatus::ALIR_ADDED, ret);
    TEST_ASSERT_TRUE(status->isAlignSelect());
    TEST_ASSERT_EQUAL(2, status->getAlignStar());
    TEST_ASSERT_EQUAL_STRING(":A1#", mockStream.getSent());
}

void test_align_add_star_2_completes_alignment(void) {
    status->startAlign(TeenAstroMountStatus::ALIM_TWO);
    // Star 1: select -> slew -> recenter -> add
    status->nextStepAlign();
    status->nextStepAlign();
    prepareSetOk();
    status->addStar();
    // Star 2: select -> slew -> recenter -> add
    status->nextStepAlign();
    status->nextStepAlign();
    prepareSetOk();
    TeenAstroMountStatus::AlignReply ret = status->addStar();
    TEST_ASSERT_EQUAL(TeenAstroMountStatus::ALIR_DONE, ret);
    TEST_ASSERT_FALSE(status->isAligning());
    // Mock accumulates output; first addStar() sends :A1#, second sends :A2#
    TEST_ASSERT_NOT_NULL(strstr(mockStream.getSent(), ":A1#"));
    TEST_ASSERT_NOT_NULL(strstr(mockStream.getSent(), ":A2#"));
}

void test_align_back_from_select_cancels(void) {
    status->startAlign(TeenAstroMountStatus::ALIM_TWO);
    status->backStepAlign();
    TEST_ASSERT_FALSE(status->isAligning());
}

void test_align_add_star_when_not_recenter_fails(void) {
    status->startAlign(TeenAstroMountStatus::ALIM_TWO);
    // addStar() without being in RECENTER
    TeenAstroMountStatus::AlignReply ret = status->addStar();
    TEST_ASSERT_EQUAL(TeenAstroMountStatus::ALIR_FAILED2, ret);
    TEST_ASSERT_FALSE(status->isAligning());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_align_start_2_stars_enters_select_star_1);
    RUN_TEST(test_align_next_step_select_to_slew);
    RUN_TEST(test_align_next_step_slew_to_recenter);
    RUN_TEST(test_align_add_star_1_then_select_star_2);
    RUN_TEST(test_align_add_star_2_completes_alignment);
    RUN_TEST(test_align_back_from_select_cancels);
    RUN_TEST(test_align_add_star_when_not_recenter_fails);
    return UNITY_END();
}
