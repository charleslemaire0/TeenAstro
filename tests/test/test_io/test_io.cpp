/*
 * test_io.cpp - Unit tests for TeenAstro LX200 IO command protocol
 *
 * Verifies the round-trip encoding/decoding of set/get commands between
 * LX200Client and the wire format, using a MockStream to simulate the
 * serial link without hardware.
 *
 * Pattern: set a value, then get it back and verify the values match.
 */

// Single-translation-unit build: include library .cpp sources directly.
// We include CommandCodec.cpp (has static atoi2) and LX200Client.cpp.
// TeenAstroMath.cpp also defines atoi2 (non-static), so we provide
// only the two functions LX200Client actually needs (gethms, getdms).
#include "CommandCodec.cpp"
#include "TeenAstroMath.h"

void gethms(const long& v, uint8_t& h, uint8_t& m, uint8_t& s)
{
    s = v % 60;
    m = (v / 60) % 60;
    h = v / 3600;
}

void getdms(const long& v, bool& ispos, uint16_t& deg, uint8_t& min, uint8_t& sec)
{
    ispos = v >= 0;
    long vabs = ispos ? v : -v;
    sec = vabs % 60;
    min = (vabs / 60) % 60;
    deg = vabs / 3600;
}

#include "LX200Client.cpp"

#include <unity.h>
#include <cmath>
#include <cstring>

// =====================================================================
//  MockStream - in-memory Stream for testing LX200Client
//
//  Responses are queued and only become readable after a command is
//  fully written (terminated by '#' or a single ACK byte).  This
//  matches real serial behaviour: LX200Client::sendReceive() calls
//  flushInput() before sending, so any pre-loaded bytes would be lost
//  unless we hold them back until after the command is written.
// =====================================================================

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

    // Queue a response that becomes readable after the next command
    void loadResponse(const char* data) {
        int idx = m_queueTail % MOCK_MAX_RESPONSES;
        strncpy(m_responseQueue[idx], data, MOCK_BUF_SIZE - 1);
        m_responseQueue[idx][MOCK_BUF_SIZE - 1] = '\0';
        m_queueTail++;
    }

    // Retrieve what LX200Client wrote (all commands concatenated)
    const char* getSent() const { return m_outBuf; }
    int getSentLen() const { return m_outLen; }

    // Clear only the output (sent) buffer
    void clearSent() {
        m_outLen = 0;
        memset(m_outBuf, 0, sizeof(m_outBuf));
    }

    // Stream interface
    int available() override {
        return m_readLen - m_readPos;
    }

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
        // When we see '#' (end of command frame), deliver the next
        // queued response so it is available for read().
        if (b == '#') {
            deliverNextResponse();
        }
        return 1;
    }

    void flush() override {}

private:
    // Readable buffer (current response)
    char m_readBuf[MOCK_BUF_SIZE];
    int  m_readLen;
    int  m_readPos;

    // Sent (output) buffer
    char m_outBuf[MOCK_BUF_SIZE];
    int  m_outLen;

    // Response queue
    char m_responseQueue[MOCK_MAX_RESPONSES][MOCK_BUF_SIZE];
    int  m_queueHead;
    int  m_queueTail;

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

// =====================================================================
//  Test globals
// =====================================================================

static MockStream mockStream;
static LX200Client* client = nullptr;

void setUp(void) {
    mockStream.reset();
    if (client) delete client;
    client = new LX200Client(mockStream, 50);
}

void tearDown(void) {
    if (client) { delete client; client = nullptr; }
}

// Helper: prepare mock for a set command (expects SHORT_BOOL '1' reply)
static void prepareSetOk() {
    mockStream.loadResponse("1");
}

// Helper: prepare mock for a get command (LONG reply terminated by #)
static void prepareGetLong(const char* value) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%s#", value);
    mockStream.loadResponse(buf);
}

// Helper: prepare mock for a get command with fixed-length reply (pad value with leading spaces)
static void prepareGetLongPadded(const char* value, int expectedLen) {
    char buf[256];
    int len = (int)strlen(value);
    int pad = expectedLen - len;
    if (pad < 0) pad = 0;
    int i = 0;
    for (; i < pad; i++) buf[i] = ' ';
    strcpy(buf + i, value);
    strcat(buf, "#");
    mockStream.loadResponse(buf);
}

// Helper: prepare for set(ok) then get(long value)
static void prepareSetThenGet(const char* getValue) {
    prepareSetOk();
    char buf[128];
    snprintf(buf, sizeof(buf), "%s#", getValue);
    mockStream.loadResponse(buf);
}

// =====================================================================
//  1. Protocol fundamentals
// =====================================================================

void test_getReplyType_get_commands(void) {
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":GR#"));
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":GD#"));
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":GZ#"));
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":GA#"));
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":GS#"));
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":GC#"));
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":GVP#"));
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":GXI#"));
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":GXRA#"));
}

void test_getReplyType_set_commands(void) {
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":St+48:51:00#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":Sg+002:20:00#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":SG+05.0#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":SXRA,5#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":SXLH,-10#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":SXrt,y#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":Sr12:30:45#"));
}

void test_getReplyType_tracking(void) {
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":Te#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":Td#"));
    TEST_ASSERT_EQUAL(CMDR_NO, getReplyType(":TQ#"));
    TEST_ASSERT_EQUAL(CMDR_NO, getReplyType(":T+#"));
    TEST_ASSERT_EQUAL(CMDR_NO, getReplyType(":T-#"));
}

void test_getReplyType_halt(void) {
    TEST_ASSERT_EQUAL(CMDR_NO, getReplyType(":Q#"));
    TEST_ASSERT_EQUAL(CMDR_NO, getReplyType(":Qe#"));
    TEST_ASSERT_EQUAL(CMDR_NO, getReplyType(":Qw#"));
}

void test_getReplyType_home_park(void) {
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":hF#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":hC#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":hP#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":hR#"));
}

void test_getReplyType_rate(void) {
    TEST_ASSERT_EQUAL(CMDR_NO, getReplyType(":RG#"));
    TEST_ASSERT_EQUAL(CMDR_NO, getReplyType(":R0#"));
    TEST_ASSERT_EQUAL(CMDR_NO, getReplyType(":R4#"));
}

void test_getReplyType_alignment(void) {
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":A0#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":A*#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":A1#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":A2#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":A3#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":A9#"));
    TEST_ASSERT_EQUAL(CMDR_SHORT_BOOL, getReplyType(":AW#"));
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":AE#"));
}

void test_getReplyType_invalid(void) {
    TEST_ASSERT_EQUAL(CMDR_INVALID, getReplyType("INVALID"));
    TEST_ASSERT_EQUAL(CMDR_INVALID, getReplyType(":Z#"));
}

void test_frame_format_get(void) {
    prepareGetLong("12:30:45");
    char out[32];
    client->getRaStr(out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING(":GR#", mockStream.getSent());
}

void test_frame_format_set(void) {
    prepareSetOk();
    client->enableTracking(true);
    TEST_ASSERT_EQUAL_STRING(":Te#", mockStream.getSent());
}

void test_reply_no(void) {
    LX200RETURN ret = client->stopSlew();
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);
    TEST_ASSERT_EQUAL_STRING(":Q#", mockStream.getSent());
}

void test_reply_short_bool_true(void) {
    prepareSetOk();
    LX200RETURN ret = client->homeGoto();
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);
}

void test_reply_short_bool_false(void) {
    mockStream.loadResponse("0");
    LX200RETURN ret = client->homeGoto();
    TEST_ASSERT_EQUAL(LX200_SETVALUEFAILED, ret);
}

// Multi-star alignment (OnStepX-style): :A0# start, :A1# 1st star, :An# nth star
void test_align_start_sends_A0(void) {
    prepareSetOk();
    LX200RETURN ret = client->alignStart();
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);
    TEST_ASSERT_EQUAL_STRING(":A0#", mockStream.getSent());
}

void test_align_select_star_1_sends_A1(void) {
    prepareSetOk();
    LX200RETURN ret = client->alignSelectStar(1);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);
    TEST_ASSERT_EQUAL_STRING(":A1#", mockStream.getSent());
}

void test_align_select_star_2_sends_A2(void) {
    prepareSetOk();
    LX200RETURN ret = client->alignSelectStar(2);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);
    TEST_ASSERT_EQUAL_STRING(":A2#", mockStream.getSent());
}

void test_align_select_star_3_sends_A3(void) {
    prepareSetOk();
    LX200RETURN ret = client->alignSelectStar(3);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);
    TEST_ASSERT_EQUAL_STRING(":A3#", mockStream.getSent());
}

void test_reply_long(void) {
    prepareGetLongPadded("TeenAstro", 32);  // :GVP# expects 32 (aligned power of 2)
    char out[64];  // buffer must be > expected length (32) for null terminator
    LX200RETURN ret = client->getProductName(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL_STRING("TeenAstro", out);
}

void test_reply_long_accepts_short_product_board_driver(void) {
    // Short replies (no padding) must be accepted for older MainUnit firmware
    char out[32];
    prepareGetLong("TeenAstro");  // 9 chars, GVP expects 20
    TEST_ASSERT_EQUAL(LX200_VALUEGET, client->getProductName(out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("TeenAstro", out);
    prepareGetLong("240");  // 3 chars, GVB expects 4
    TEST_ASSERT_EQUAL(LX200_VALUEGET, client->getBoardVersion(out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("240", out);
    prepareGetLong("3");  // 1 char, GVb expects 4
    TEST_ASSERT_EQUAL(LX200_VALUEGET, client->getDriverType(out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("3", out);
}

void test_timeout_returns_failure(void) {
    // No response loaded - should timeout
    char out[32];
    LX200RETURN ret = client->getProductName(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_GETVALUEFAILED, ret);
}

// =====================================================================
//  2. Location / Site - set then get
// =====================================================================

void test_set_get_latitude(void) {
    // Set latitude +48:51:00
    prepareSetOk();
    LX200RETURN ret = client->setLatitudeDMS(0, 48, 51, 0);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    // Get latitude back - server would reply with "+48*51:00"
    mockStream.clearSent();
    prepareGetLong("+48*51:00");
    double lat = 0;
    ret = client->getLatitude(lat);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 48.85, lat);
}

void test_set_get_latitude_negative(void) {
    prepareSetOk();
    LX200RETURN ret = client->setLatitudeDMS(1, 33, 52, 0);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("-33*52:00");
    double lat = 0;
    ret = client->getLatitude(lat);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -33.867, lat);
}

void test_set_get_longitude(void) {
    prepareSetOk();
    LX200RETURN ret = client->setLongitudeDMS(0, 2, 20, 0);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("+002*20:00");
    double lon = 0;
    ret = client->getLongitude(lon);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 2.333, lon);
}

void test_set_get_longitude_negative(void) {
    prepareSetOk();
    LX200RETURN ret = client->setLongitudeDMS(1, 122, 25, 0);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("-122*25:00");
    double lon = 0;
    ret = client->getLongitude(lon);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -122.417, lon);
}

void test_set_get_elevation(void) {
    prepareSetOk();
    LX200RETURN ret = client->setElevation(130);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("+130", 6);  // :Ge# expects 6 chars
    char out[20];
    ret = client->getElevation(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(130, atoi(out));
}

void test_set_get_timezone(void) {
    prepareSetOk();
    LX200RETURN ret = client->setTimeZone(-5.0);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("-05.0", 7);  // :GG# expects 7 chars
    char out[20];
    ret = client->getTimeZoneStr(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, -5.0, atof(out));
}

void test_set_get_selected_site(void) {
    // setSite sends :W2# which is CMDR_NO
    int val = 2;
    LX200RETURN ret = client->setSite(val);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);
    TEST_ASSERT_EQUAL_STRING(":W2#", mockStream.getSent());

    mockStream.clearSent();
    prepareGetLongPadded("2", 2);  // :W?# expects 2 chars
    ret = client->getSite(val);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(2, val);
}

void test_set_get_mount_idx(void) {
    prepareSetOk();
    int idx = 1;
    LX200RETURN ret = client->setMount(idx);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("1", 6);  // :GXOI# expects 6 chars
    ret = client->getMountIdx(idx);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(1, idx);
}

void test_set_get_mount_description(void) {
    prepareSetOk();
    LX200RETURN ret = client->setMountDescription("MyScope");
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("MyScope", 16);  // :GXOA# expects 16 chars
    char out[32];
    ret = client->getMountDescription(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL_STRING("MyScope", out);
}

// =====================================================================
//  3. Time / Date - set then get
// =====================================================================

void test_set_get_utc_time(void) {
    prepareSetOk();
    LX200RETURN ret = client->setUTCTimeRaw(14, 30, 45);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("14:30:45");
    unsigned int h, m, s;
    ret = client->getUTCTime(h, m, s);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(14, h);
    TEST_ASSERT_EQUAL(30, m);
    TEST_ASSERT_EQUAL(45, s);
}

void test_set_get_utc_date(void) {
    prepareSetOk();
    LX200RETURN ret = client->setUTCDateRaw(3, 15, 25);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("03/15/25");
    unsigned int d, mo, y;
    ret = client->getUTCDate(d, mo, y);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(15, d);
    TEST_ASSERT_EQUAL(3, mo);
    TEST_ASSERT_EQUAL(2025, y);
}

// =====================================================================
//  4. Rates & Acceleration - set then get
// =====================================================================

void test_set_get_acceleration(void) {
    prepareSetOk();
    LX200RETURN ret = client->setAcceleration(5.0);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("5", 12);  // :GXRA# expects 12 chars
    float val = 0;
    ret = client->getAcceleration(val);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 5.0f, val);
}

void test_set_get_max_rate(void) {
    prepareSetOk();
    LX200RETURN ret = client->setMaxRate(800);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("800", 6);  // :GXRX# expects 6 chars
    int val = 0;
    ret = client->getMaxRate(val);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(800, val);
}

void test_set_get_deadband(void) {
    prepareSetOk();
    LX200RETURN ret = client->setDeadband(15);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("15");
    int val = 0;
    ret = client->getDeadband(val);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(15, val);
}

void test_set_get_speed_rate(void) {
    prepareSetOk();
    LX200RETURN ret = client->setSpeedRate(1, 4.0);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("4");
    float val = 0;
    ret = client->getSpeedRate(1, val);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 4.0f, val);
}

void test_set_get_stored_track_rate_ra(void) {
    prepareSetOk();
    LX200RETURN ret = client->setStoredTrackRateRA(15041);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("15041", 12);  // :GXRe# expects 12 chars
    long val = 0;
    ret = client->getStoredTrackRateRA(val);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(15041, val);
}

void test_set_get_stored_track_rate_dec(void) {
    prepareSetOk();
    LX200RETURN ret = client->setStoredTrackRateDec(100);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("100", 12);  // :GXRf# expects 12 chars
    long val = 0;
    ret = client->getStoredTrackRateDec(val);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(100, val);
}

// =====================================================================
//  5. Limits - set then get
// =====================================================================

void test_set_get_min_altitude(void) {
    prepareSetOk();
    LX200RETURN ret = client->setMinAltitude(-10);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("-10", 5);  // :GXLH# expects 5 chars
    int val = 0;
    ret = client->getMinAltitude(val);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(-10, val);
}

void test_set_get_max_altitude(void) {
    prepareSetOk();
    LX200RETURN ret = client->setMaxAltitude(85);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("85", 5);  // :GXLO# expects 5 chars
    int val = 0;
    ret = client->getMaxAltitude(val);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(85, val);
}

void test_set_get_min_dist_from_pole(void) {
    prepareSetOk();
    LX200RETURN ret = client->setMinDistFromPole(5);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("5", 5);  // :GXLS# expects 5 chars
    int val = 0;
    ret = client->getMinDistFromPole(val);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(5, val);
}

void test_set_get_limit_east(void) {
    prepareSetOk();
    LX200RETURN ret = client->setLimitEast(120);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("120", 12);  // :GXLE# expects 12 chars
    char out[20];
    ret = client->getLimitEast(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(120, atoi(out));
}

void test_set_get_limit_west(void) {
    prepareSetOk();
    LX200RETURN ret = client->setLimitWest(120);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("120", 12);  // :GXLW# expects 12 chars
    char out[20];
    ret = client->getLimitWest(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(120, atoi(out));
}

void test_set_get_steps_per_second(void) {
    prepareSetOk();
    LX200RETURN ret = client->setStepsPerSecond(500);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("500", 6);  // :GXOS# expects 6 chars
    char out[20];
    ret = client->getStepsPerSecond(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(500, atoi(out));
}

// =====================================================================
//  6. Mount flags - set then get
// =====================================================================

void test_set_get_refraction_on(void) {
    prepareSetOk();
    LX200RETURN ret = client->enableRefraction(true);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("y", 2);  // :GXrt# expects 2 chars
    char out[8];
    ret = client->getRefractionEnabled(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL_STRING("y", out);
}

void test_set_get_refraction_off(void) {
    prepareSetOk();
    LX200RETURN ret = client->enableRefraction(false);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("n", 2);  // :GXrt# expects 2 chars
    char out[8];
    ret = client->getRefractionEnabled(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL_STRING("n", out);
}

void test_set_get_polar_align(void) {
    prepareSetOk();
    LX200RETURN ret = client->enablePolarAlign(true);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("y", 2);  // :GXrp# expects 2 chars
    char out[8];
    ret = client->getPolarAlignEnabled(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL_STRING("y", out);
}

void test_set_get_goto_enabled(void) {
    prepareSetOk();
    LX200RETURN ret = client->enableGoTo(true);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("y", 2);  // :GXrg# expects 2 chars
    char out[8];
    ret = client->getGoToEnabled(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL_STRING("y", out);
}

// =====================================================================
//  7. Motor configuration (per-axis) - write then read
// =====================================================================

void test_write_read_reverse_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const bool rev = true;
    LX200RETURN ret = client->writeReverse(axis, rev);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("1", 2);  // :GXMRR# expects 2 chars
    bool readVal = false;
    ret = client->readReverse(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_TRUE(readVal);
}

void test_write_read_reverse_axis2(void) {
    const uint8_t axis = 2;
    prepareSetOk();
    const bool rev = false;
    LX200RETURN ret = client->writeReverse(axis, rev);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("0", 2);  // :GXMRD# expects 2 chars
    bool readVal = true;
    ret = client->readReverse(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_FALSE(readVal);
}

void test_write_read_backlash_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const float bl = 120.0f;
    LX200RETURN ret = client->writeBacklash(axis, bl);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("120", 6);  // :GXMBR# expects 6 chars
    float readVal = 0;
    ret = client->readBacklash(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 120.0f, readVal);
}

void test_write_read_backlash_rate_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const float rate = 50.0f;
    LX200RETURN ret = client->writeBacklashRate(axis, rate);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("50");
    float readVal = 0;
    ret = client->readBacklashRate(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, readVal);
}

void test_write_read_tot_gear_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const float gear = 360.0f;
    LX200RETURN ret = client->writeTotGear(axis, gear);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    // Wire format: gear * 1000 = 360000
    prepareGetLongPadded("360000", 12);  // :GXMGR# expects 12 chars
    float readVal = 0;
    ret = client->readTotGear(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 360.0f, readVal);
}

void test_write_read_step_per_rot_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const float steps = 200.0f;
    LX200RETURN ret = client->writeStepPerRot(axis, steps);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("200", 6);  // :GXMSR# expects 6 chars
    float readVal = 0;
    ret = client->readStepPerRot(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 200.0f, readVal);
}

void test_write_read_micro_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const uint8_t micro = 4;
    LX200RETURN ret = client->writeMicro(axis, micro);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("4", 6);  // :GXMMR# expects 6 chars
    uint8_t readVal = 0;
    ret = client->readMicro(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(4, readVal);
}

void test_write_read_silent_step_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const uint8_t silent = 1;
    LX200RETURN ret = client->writeSilentStep(axis, silent);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("1", 6);  // :GXMmR# expects 6 chars
    uint8_t readVal = 0;
    ret = client->readSilentStep(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(1, readVal);
}

void test_write_read_low_curr_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const unsigned int curr = 400;
    LX200RETURN ret = client->writeLowCurr(axis, curr);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("400", 6);  // :GXMIR# expects 6 chars
    unsigned int readVal = 0;
    ret = client->readLowCurr(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(400, readVal);
}

void test_write_read_high_curr_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const unsigned int curr = 1200;
    LX200RETURN ret = client->writeHighCurr(axis, curr);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("1200", 6);  // :GXMCR# expects 6 chars
    unsigned int readVal = 0;
    ret = client->readHighCurr(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(1200, readVal);
}

void test_write_read_motor_axis2(void) {
    const uint8_t axis = 2;

    // Write backlash on axis 2
    prepareSetOk();
    const float bl = 80.0f;
    LX200RETURN ret = client->writeBacklash(axis, bl);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("80", 6);  // :GXMBD# expects 6 chars
    float readVal = 0;
    ret = client->readBacklash(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 80.0f, readVal);
}

void test_write_read_high_curr_axis2(void) {
    const uint8_t axis = 2;
    prepareSetOk();
    const unsigned int curr = 1800;
    LX200RETURN ret = client->writeHighCurr(axis, curr);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("1800", 6);  // :GXMCD# expects 6 chars
    unsigned int readVal = 0;
    ret = client->readHighCurr(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(1800, readVal);
}

// =====================================================================
//  8. Encoder configuration (per-axis) - write then read
// =====================================================================

void test_write_read_encoder_reverse_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const bool rev = true;
    LX200RETURN ret = client->writeEncoderReverse(axis, rev);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("1", 12);  // :GXErR# expects 12 chars
    bool readVal = false;
    ret = client->readEncoderReverse(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_TRUE(readVal);
}

void test_write_read_pulse_per_degree_axis1(void) {
    const uint8_t axis = 1;
    prepareSetOk();
    const float ppd = 1000.0f;
    LX200RETURN ret = client->writePulsePerDegree(axis, ppd);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("1000", 12);  // :GXEPR# expects 12 chars
    float readVal = 0;
    ret = client->readPulsePerDegree(axis, readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1000.0f, readVal);
}

void test_write_read_encoder_auto_sync(void) {
    prepareSetOk();
    LX200RETURN ret = client->writeEncoderAutoSync(3);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLongPadded("3", 4);  // :GXEO# expects 4 chars
    uint8_t readVal = 0;
    ret = client->readEncoderAutoSync(readVal);
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL(3, readVal);
}

// =====================================================================
//  9. Target setting - set then get back via string
// =====================================================================

void test_set_get_target_ra(void) {
    prepareSetOk();
    uint8_t h = 12, m = 30, s = 45;
    LX200RETURN ret = client->setTargetRA(h, m, s);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("12:30:45");
    char out[20];
    ret = client->getTargetRaStr(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL_STRING("12:30:45", out);
}

void test_set_get_target_dec(void) {
    prepareSetOk();
    bool ispos = true;
    uint16_t deg = 45;
    uint8_t m = 15, s = 30;
    LX200RETURN ret = client->setTargetDec(ispos, deg, m, s);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("+45*15:30");
    char out[20];
    ret = client->getTargetDecStr(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL_STRING("+45*15:30", out);
}

void test_set_get_target_dec_negative(void) {
    prepareSetOk();
    bool ispos = false;
    uint16_t deg = 20;
    uint8_t m = 10, s = 5;
    LX200RETURN ret = client->setTargetDec(ispos, deg, m, s);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);

    mockStream.clearSent();
    prepareGetLong("-20*10:05");
    char out[20];
    ret = client->getTargetDecStr(out, sizeof(out));
    TEST_ASSERT_EQUAL(LX200_VALUEGET, ret);
    TEST_ASSERT_EQUAL_STRING("-20*10:05", out);
}

// =====================================================================
//  10. Tracking / Movement commands - wire format verification
// =====================================================================

void test_enable_tracking_on_off(void) {
    prepareSetOk();
    LX200RETURN ret = client->enableTracking(true);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);
    TEST_ASSERT_EQUAL_STRING(":Te#", mockStream.getSent());

    mockStream.reset();
    prepareSetOk();
    ret = client->enableTracking(false);
    TEST_ASSERT_EQUAL(LX200_VALUESET, ret);
    TEST_ASSERT_EQUAL_STRING(":Td#", mockStream.getSent());
}

void test_set_speed_levels(void) {
    for (uint8_t lvl = 0; lvl <= 4; lvl++) {
        mockStream.reset();
        char expected[8];
        snprintf(expected, sizeof(expected), ":R%d#", lvl);
        client->setSpeed(lvl);
        TEST_ASSERT_EQUAL_STRING(expected, mockStream.getSent());
    }
}

void test_movement_commands_wire_format(void) {
    mockStream.reset();
    client->startMoveNorth();
    TEST_ASSERT_EQUAL_STRING(":Mn#", mockStream.getSent());

    mockStream.reset();
    client->startMoveSouth();
    TEST_ASSERT_EQUAL_STRING(":Ms#", mockStream.getSent());

    mockStream.reset();
    client->startMoveEast();
    TEST_ASSERT_EQUAL_STRING(":Me#", mockStream.getSent());

    mockStream.reset();
    client->startMoveWest();
    TEST_ASSERT_EQUAL_STRING(":Mw#", mockStream.getSent());

    mockStream.reset();
    client->stopSlew();
    TEST_ASSERT_EQUAL_STRING(":Q#", mockStream.getSent());
}

void test_stop_directional_wire_format(void) {
    mockStream.reset();
    client->stopMoveNorth();
    TEST_ASSERT_EQUAL_STRING(":Qn#", mockStream.getSent());

    mockStream.reset();
    client->stopMoveSouth();
    TEST_ASSERT_EQUAL_STRING(":Qs#", mockStream.getSent());

    mockStream.reset();
    client->stopMoveEast();
    TEST_ASSERT_EQUAL_STRING(":Qe#", mockStream.getSent());

    mockStream.reset();
    client->stopMoveWest();
    TEST_ASSERT_EQUAL_STRING(":Qw#", mockStream.getSent());
}

// =====================================================================
//  11. CommandCodec round-trips (pure encoding/decoding)
// =====================================================================

void test_codec_hms_round_trip(void) {
    double original = 14.512;  // 14h 30m 43s approx
    char buf[16];
    doubleToHms(buf, &original, true);

    double parsed = 0;
    TEST_ASSERT_TRUE(hmsToDouble(&parsed, buf, true));
    TEST_ASSERT_DOUBLE_WITHIN(0.001, original, parsed);
}

void test_codec_dms_round_trip_signed(void) {
    double original = 48.85;  // +48*51:00
    char buf[16];
    doubleToDms(buf, &original, false, true, true);

    double parsed = 0;
    TEST_ASSERT_TRUE(dmsToDouble(&parsed, buf, true, true));
    TEST_ASSERT_DOUBLE_WITHIN(0.001, original, parsed);
}

void test_codec_dms_round_trip_negative(void) {
    double original = -33.867;
    char buf[16];
    doubleToDms(buf, &original, false, true, true);

    double parsed = 0;
    TEST_ASSERT_TRUE(dmsToDouble(&parsed, buf, true, true));
    TEST_ASSERT_DOUBLE_WITHIN(0.001, original, parsed);
}

void test_codec_dms_round_trip_unsigned(void) {
    double original = 122.417;  // 122*25:00 (no sign)
    char buf[16];
    doubleToDms(buf, &original, true, false, true);

    double parsed = 0;
    TEST_ASSERT_TRUE(dmsToDouble(&parsed, buf, false, true));
    TEST_ASSERT_DOUBLE_WITHIN(0.001, original, parsed);
}

void test_codec_gethms_getdms(void) {
    // gethms: 52245 seconds -> 14h 30m 45s
    uint8_t h, m, s;
    long v = 52245;
    gethms(v, h, m, s);
    TEST_ASSERT_EQUAL(14, h);
    TEST_ASSERT_EQUAL(30, m);
    TEST_ASSERT_EQUAL(45, s);

    // getdms: -163830 -> negative, 45*30'30"
    bool ispos;
    uint16_t deg;
    uint8_t mi, se;
    long v2 = -163830;
    getdms(v2, ispos, deg, mi, se);
    TEST_ASSERT_FALSE(ispos);
    TEST_ASSERT_EQUAL(45, deg);
    TEST_ASSERT_EQUAL(30, mi);
    TEST_ASSERT_EQUAL(30, se);
}

// =====================================================================
//  Focuser binary protocol helpers (mirror the focuser encoder logic)
// =====================================================================

static const char TEST_B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void testB64Encode(const uint8_t* in, char* out, int len)
{
    int o = 0;
    for (int i = 0; i < len; i += 3)
    {
        uint32_t b = ((uint32_t)in[i] << 16) | ((uint32_t)in[i+1] << 8) | in[i+2];
        out[o++] = TEST_B64[(b >> 18) & 0x3F];
        out[o++] = TEST_B64[(b >> 12) & 0x3F];
        out[o++] = TEST_B64[(b >>  6) & 0x3F];
        out[o++] = TEST_B64[ b        & 0x3F];
    }
    out[o] = 0;
}

static void testPackU16(uint8_t* p, int off, uint16_t v) {
    p[off] = (uint8_t)(v & 0xFF);
    p[off + 1] = (uint8_t)(v >> 8);
}

static void testPackI16(uint8_t* p, int off, int16_t v) {
    testPackU16(p, off, (uint16_t)v);
}

static void testXorChecksum(uint8_t* pkt, int dataLen) {
    uint8_t x = 0;
    for (int i = 0; i < dataLen; i++) x ^= pkt[i];
    pkt[dataLen] = x;
}

// Build a :FA# config packet (150 bytes) exactly as the focuser does, encode, format as reply string
static void buildConfigReply(uint8_t* pkt, char* reply, int replySize,
    uint16_t startPos, uint16_t maxPos, uint16_t lowSpd, uint16_t hiSpd,
    uint8_t cmdA, uint8_t manA, uint8_t manD,
    uint8_t rev, uint8_t mic, uint16_t res, uint8_t cur, uint16_t srot)
{
    memset(pkt, 0, 150);
    testPackU16(pkt, 0,  startPos);
    testPackU16(pkt, 2,  maxPos);
    testPackU16(pkt, 4,  lowSpd);
    testPackU16(pkt, 6,  hiSpd);
    pkt[8]  = cmdA;
    pkt[9]  = manA;
    pkt[10] = manD;
    pkt[11] = rev;
    pkt[12] = mic;
    testPackU16(pkt, 13, res);
    pkt[15] = cur;
    testPackU16(pkt, 16, srot);
    testXorChecksum(pkt, 149);
    char b64[201];
    testB64Encode(pkt, b64, 150);
    snprintf(reply, replySize, "%s#", b64);
}

// Build a :Fa# state packet (9 bytes) exactly as the focuser does
static void buildStateReply(uint8_t* pkt, char* reply, int replySize,
    uint16_t pos, uint16_t spd, int16_t tempX100, uint8_t flags)
{
    memset(pkt, 0, 9);
    testPackU16(pkt, 0, pos);
    testPackU16(pkt, 2, spd);
    testPackI16(pkt, 4, tempX100);
    pkt[6] = flags;
    testXorChecksum(pkt, 8);
    char b64[13];
    testB64Encode(pkt, b64, 9);
    snprintf(reply, replySize, "%s#", b64);
}

// =====================================================================
//  11a. :FA# config — all zeros
// =====================================================================

void test_focuser_config_all_zeros(void)
{
    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply), 0,0,0,0, 0,0,0, 0,0,0,0,0);
    mockStream.loadResponse(reply);

    unsigned int sp, mp, ls, hs, ca, ma, md;
    TEST_ASSERT_EQUAL(LX200_VALUEGET,
        client->readFocuserConfig(sp, mp, ls, hs, ca, ma, md));
    TEST_ASSERT_EQUAL(0, sp);  TEST_ASSERT_EQUAL(0, mp);
    TEST_ASSERT_EQUAL(0, ls);  TEST_ASSERT_EQUAL(0, hs);
    TEST_ASSERT_EQUAL(0, ca);  TEST_ASSERT_EQUAL(0, ma);
    TEST_ASSERT_EQUAL(0, md);
}

void test_focuser_motor_all_zeros(void)
{
    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply), 0,0,0,0, 0,0,0, 0,0,0,0,0);
    mockStream.loadResponse(reply);

    bool rev; unsigned int mic, res, cur, srot;
    TEST_ASSERT_EQUAL(LX200_VALUEGET,
        client->readFocuserMotor(rev, mic, res, cur, srot));
    TEST_ASSERT_FALSE(rev);
    TEST_ASSERT_EQUAL(0, mic); TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(0, cur); TEST_ASSERT_EQUAL(0, srot);
}

// =====================================================================
//  11b. :FA# config — typical values
// =====================================================================

void test_focuser_config_typical(void)
{
    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply),
        500, 10000, 50, 200, 10, 8, 5,
        0, 3, 16, 80, 400);
    mockStream.loadResponse(reply);

    unsigned int sp, mp, ls, hs, ca, ma, md;
    TEST_ASSERT_EQUAL(LX200_VALUEGET,
        client->readFocuserConfig(sp, mp, ls, hs, ca, ma, md));
    TEST_ASSERT_EQUAL(500, sp);  TEST_ASSERT_EQUAL(10000, mp);
    TEST_ASSERT_EQUAL(50, ls);   TEST_ASSERT_EQUAL(200, hs);
    TEST_ASSERT_EQUAL(10, ca);   TEST_ASSERT_EQUAL(8, ma);
    TEST_ASSERT_EQUAL(5, md);
}

void test_focuser_motor_typical(void)
{
    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply),
        500, 10000, 50, 200, 10, 8, 5,
        1, 4, 32, 120, 200);
    mockStream.loadResponse(reply);

    bool rev; unsigned int mic, res, cur, srot;
    TEST_ASSERT_EQUAL(LX200_VALUEGET,
        client->readFocuserMotor(rev, mic, res, cur, srot));
    TEST_ASSERT_TRUE(rev);
    TEST_ASSERT_EQUAL(4, mic); TEST_ASSERT_EQUAL(32, res);
    TEST_ASSERT_EQUAL(120, cur); TEST_ASSERT_EQUAL(200, srot);
}

// =====================================================================
//  11c. :FA# config — max uint16 boundary values
// =====================================================================

void test_focuser_config_max_values(void)
{
    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply),
        65535, 65535, 999, 999, 99, 99, 99,
        1, 7, 512, 160, 800);
    mockStream.loadResponse(reply);

    unsigned int sp, mp, ls, hs, ca, ma, md;
    TEST_ASSERT_EQUAL(LX200_VALUEGET,
        client->readFocuserConfig(sp, mp, ls, hs, ca, ma, md));
    TEST_ASSERT_EQUAL(65535, sp); TEST_ASSERT_EQUAL(65535, mp);
    TEST_ASSERT_EQUAL(999, ls);   TEST_ASSERT_EQUAL(999, hs);
    TEST_ASSERT_EQUAL(99, ca);    TEST_ASSERT_EQUAL(99, ma);
    TEST_ASSERT_EQUAL(99, md);
}

void test_focuser_motor_max_values(void)
{
    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply),
        65535, 65535, 999, 999, 99, 99, 99,
        1, 7, 512, 160, 800);
    mockStream.loadResponse(reply);

    bool rev; unsigned int mic, res, cur, srot;
    TEST_ASSERT_EQUAL(LX200_VALUEGET,
        client->readFocuserMotor(rev, mic, res, cur, srot));
    TEST_ASSERT_TRUE(rev);
    TEST_ASSERT_EQUAL(7, mic); TEST_ASSERT_EQUAL(512, res);
    TEST_ASSERT_EQUAL(160, cur); TEST_ASSERT_EQUAL(800, srot);
}

// =====================================================================
//  11d. :FA# config — user positions with names
// =====================================================================

void test_focuser_config_user_positions(void)
{
    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply),
        100, 5000, 30, 150, 5, 5, 3,
        0, 3, 8, 50, 200);

    // Fill 3 named positions, leave rest empty
    int base0 = 18 + 0 * 13;
    testPackU16(pkt, base0, 1234);
    memcpy(&pkt[base0 + 2], "Halpha\0\0\0\0\0", 11);

    int base1 = 18 + 1 * 13;
    testPackU16(pkt, base1, 500);
    memcpy(&pkt[base1 + 2], "OIII_filter", 11);

    int base5 = 18 + 5 * 13;
    testPackU16(pkt, base5, 65000);
    memcpy(&pkt[base5 + 2], "MaxPos\0\0\0\0\0", 11);

    testXorChecksum(pkt, 149);
    char b64[201]; testB64Encode(pkt, b64, 150);
    snprintf(reply, sizeof(reply), "%s#", b64);

    // Decode via LX200Client and manually verify user positions
    // We use getFocuserAllConfig to get the raw base64, then decode manually
    mockStream.loadResponse(reply);
    char raw[220];
    TEST_ASSERT_EQUAL(LX200_VALUEGET, client->getFocuserAllConfig(raw, sizeof(raw)));

    // The client strips '#', so len should be 200
    int len = (int)strlen(raw);
    TEST_ASSERT_EQUAL(200, len);

    // Manually decode to verify user positions (same as webserver does)
    static const int8_t DEC[128] = {
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
      52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
      -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
      15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
      -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
      41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    uint8_t dpkt[150];
    int o = 0;
    for (int i = 0; i < 200; i += 4) {
        int8_t v0 = DEC[(uint8_t)raw[i]], v1 = DEC[(uint8_t)raw[i+1]];
        int8_t v2 = DEC[(uint8_t)raw[i+2]], v3 = DEC[(uint8_t)raw[i+3]];
        TEST_ASSERT_TRUE(v0 >= 0 && v1 >= 0 && v2 >= 0 && v3 >= 0);
        uint32_t b = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) | ((uint32_t)v2 << 6) | (uint32_t)v3;
        dpkt[o++] = (uint8_t)(b >> 16); dpkt[o++] = (uint8_t)(b >> 8); dpkt[o++] = (uint8_t)(b);
    }

    // Verify checksum
    uint8_t xc = 0; for (int i = 0; i < 149; i++) xc ^= dpkt[i];
    TEST_ASSERT_EQUAL(dpkt[149], xc);

    // Verify user position 0
    uint16_t p0 = (uint16_t)dpkt[base0] | ((uint16_t)dpkt[base0+1] << 8);
    TEST_ASSERT_EQUAL(1234, p0);
    char name0[12]; memcpy(name0, &dpkt[base0+2], 11); name0[11] = '\0';
    TEST_ASSERT_EQUAL_STRING("Halpha", name0);

    // Verify user position 1
    uint16_t p1 = (uint16_t)dpkt[base1] | ((uint16_t)dpkt[base1+1] << 8);
    TEST_ASSERT_EQUAL(500, p1);
    char name1[12]; memcpy(name1, &dpkt[base1+2], 11); name1[11] = '\0';
    TEST_ASSERT_EQUAL_STRING("OIII_filter", name1);

    // Verify user position 5
    uint16_t p5 = (uint16_t)dpkt[base5] | ((uint16_t)dpkt[base5+1] << 8);
    TEST_ASSERT_EQUAL(65000, p5);
    char name5[12]; memcpy(name5, &dpkt[base5+2], 11); name5[11] = '\0';
    TEST_ASSERT_EQUAL_STRING("MaxPos", name5);

    // Verify empty position 2 is all zeros
    int base2 = 18 + 2 * 13;
    uint16_t p2 = (uint16_t)dpkt[base2] | ((uint16_t)dpkt[base2+1] << 8);
    TEST_ASSERT_EQUAL(0, p2);
    TEST_ASSERT_EQUAL(0, dpkt[base2+2]);
}

// =====================================================================
//  11e. :FA# config — bad checksum rejected
// =====================================================================

void test_focuser_config_bad_checksum(void)
{
    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply), 100, 5000, 30, 150, 5, 5, 3, 0, 3, 8, 50, 200);

    // Corrupt the checksum
    pkt[149] ^= 0xFF;
    char b64[201]; testB64Encode(pkt, b64, 150);
    snprintf(reply, sizeof(reply), "%s#", b64);
    mockStream.loadResponse(reply);

    unsigned int sp, mp, ls, hs, ca, ma, md;
    TEST_ASSERT_EQUAL(LX200_GETVALUEFAILED,
        client->readFocuserConfig(sp, mp, ls, hs, ca, ma, md));
}

// =====================================================================
//  11f. :FA# config — wrong base64 length rejected
// =====================================================================

void test_focuser_config_wrong_length(void)
{
    // Send 196 chars (not 200) — should fail
    char reply[210];
    for (int i = 0; i < 196; i++) reply[i] = 'A';
    reply[196] = '#';
    reply[197] = '\0';
    mockStream.loadResponse(reply);

    unsigned int sp, mp, ls, hs, ca, ma, md;
    TEST_ASSERT_EQUAL(LX200_GETVALUEFAILED,
        client->readFocuserConfig(sp, mp, ls, hs, ca, ma, md));
}

// =====================================================================
//  11g. :FA# config — invalid base64 char rejected
// =====================================================================

void test_focuser_config_invalid_b64_char(void)
{
    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply), 100, 5000, 30, 150, 5, 5, 3, 0, 3, 8, 50, 200);

    // Corrupt a base64 character to an invalid char
    reply[10] = '!';
    mockStream.loadResponse(reply);

    unsigned int sp, mp, ls, hs, ca, ma, md;
    TEST_ASSERT_EQUAL(LX200_GETVALUEFAILED,
        client->readFocuserConfig(sp, mp, ls, hs, ca, ma, md));
}

// =====================================================================
//  11h. :FA# config — no response returns failure
// =====================================================================

void test_focuser_config_no_response(void)
{
    // No response loaded
    unsigned int sp, mp, ls, hs, ca, ma, md;
    TEST_ASSERT_EQUAL(LX200_GETVALUEFAILED,
        client->readFocuserConfig(sp, mp, ls, hs, ca, ma, md));
}

// =====================================================================
//  11i. :Fa# state — all zeros
// =====================================================================

void test_focuser_state_all_zeros(void)
{
    uint8_t pkt[9];
    char reply[20];
    buildStateReply(pkt, reply, sizeof(reply), 0, 0, 0, 0);
    mockStream.loadResponse(reply);

    char raw[20];
    TEST_ASSERT_EQUAL(LX200_VALUEGET, client->getFocuserAllState(raw, sizeof(raw)));
    TEST_ASSERT_EQUAL(12, (int)strlen(raw));

    // Verify decoding (same logic as MountStatus)
    static const int8_t DEC[128] = {
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
      52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
      -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
      15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
      -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
      41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    uint8_t dp[9];
    int o = 0;
    for (int i = 0; i < 12; i += 4) {
        int8_t v0 = DEC[(uint8_t)raw[i]], v1 = DEC[(uint8_t)raw[i+1]];
        int8_t v2 = DEC[(uint8_t)raw[i+2]], v3 = DEC[(uint8_t)raw[i+3]];
        uint32_t b = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) | ((uint32_t)v2 << 6) | (uint32_t)v3;
        dp[o++] = (uint8_t)(b >> 16); dp[o++] = (uint8_t)(b >> 8); dp[o++] = (uint8_t)(b);
    }
    uint8_t xc = 0; for (int i = 0; i < 8; i++) xc ^= dp[i];
    TEST_ASSERT_EQUAL(dp[8], xc);
    uint16_t pos = (uint16_t)dp[0] | ((uint16_t)dp[1] << 8);
    uint16_t spd = (uint16_t)dp[2] | ((uint16_t)dp[3] << 8);
    TEST_ASSERT_EQUAL(0, pos);
    TEST_ASSERT_EQUAL(0, spd);
}

// =====================================================================
//  11j. :Fa# state — known values (position, speed, temperature)
// =====================================================================

void test_focuser_state_known_values(void)
{
    uint8_t pkt[9];
    char reply[20];
    buildStateReply(pkt, reply, sizeof(reply), 12345, 88, 2350, 1);
    mockStream.loadResponse(reply);

    char raw[20];
    TEST_ASSERT_EQUAL(LX200_VALUEGET, client->getFocuserAllState(raw, sizeof(raw)));
    TEST_ASSERT_EQUAL(12, (int)strlen(raw));

    static const int8_t DEC[128] = {
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
      52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
      -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
      15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
      -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
      41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    uint8_t dp[9];
    int o = 0;
    for (int i = 0; i < 12; i += 4) {
        int8_t v0 = DEC[(uint8_t)raw[i]], v1 = DEC[(uint8_t)raw[i+1]];
        int8_t v2 = DEC[(uint8_t)raw[i+2]], v3 = DEC[(uint8_t)raw[i+3]];
        uint32_t b = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) | ((uint32_t)v2 << 6) | (uint32_t)v3;
        dp[o++] = (uint8_t)(b >> 16); dp[o++] = (uint8_t)(b >> 8); dp[o++] = (uint8_t)(b);
    }
    uint8_t xc = 0; for (int i = 0; i < 8; i++) xc ^= dp[i];
    TEST_ASSERT_EQUAL(dp[8], xc);

    uint16_t pos = (uint16_t)dp[0] | ((uint16_t)dp[1] << 8);
    uint16_t spd = (uint16_t)dp[2] | ((uint16_t)dp[3] << 8);
    int16_t temp = (int16_t)((uint16_t)dp[4] | ((uint16_t)dp[5] << 8));
    TEST_ASSERT_EQUAL(12345, pos);
    TEST_ASSERT_EQUAL(88, spd);
    TEST_ASSERT_EQUAL(2350, temp);
    TEST_ASSERT_EQUAL(1, dp[6]);
}

// =====================================================================
//  11k. :Fa# state — negative temperature
// =====================================================================

void test_focuser_state_negative_temp(void)
{
    uint8_t pkt[9];
    char reply[20];
    buildStateReply(pkt, reply, sizeof(reply), 0, 0, -1500, 0);
    mockStream.loadResponse(reply);

    char raw[20];
    TEST_ASSERT_EQUAL(LX200_VALUEGET, client->getFocuserAllState(raw, sizeof(raw)));

    static const int8_t DEC[128] = {
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
      52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
      -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
      15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
      -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
      41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    uint8_t dp[9];
    int o = 0;
    for (int i = 0; i < 12; i += 4) {
        int8_t v0 = DEC[(uint8_t)raw[i]], v1 = DEC[(uint8_t)raw[i+1]];
        int8_t v2 = DEC[(uint8_t)raw[i+2]], v3 = DEC[(uint8_t)raw[i+3]];
        uint32_t b = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) | ((uint32_t)v2 << 6) | (uint32_t)v3;
        dp[o++] = (uint8_t)(b >> 16); dp[o++] = (uint8_t)(b >> 8); dp[o++] = (uint8_t)(b);
    }
    int16_t temp = (int16_t)((uint16_t)dp[4] | ((uint16_t)dp[5] << 8));
    TEST_ASSERT_EQUAL(-1500, temp);
}

// =====================================================================
//  11l. :Fa# state — max position (boundary)
// =====================================================================

void test_focuser_state_max_position(void)
{
    uint8_t pkt[9];
    char reply[20];
    buildStateReply(pkt, reply, sizeof(reply), 65535, 999, 0, 0);
    mockStream.loadResponse(reply);

    char raw[20];
    TEST_ASSERT_EQUAL(LX200_VALUEGET, client->getFocuserAllState(raw, sizeof(raw)));

    static const int8_t DEC[128] = {
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
      52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
      -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
      15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
      -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
      41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    uint8_t dp[9];
    int o = 0;
    for (int i = 0; i < 12; i += 4) {
        int8_t v0 = DEC[(uint8_t)raw[i]], v1 = DEC[(uint8_t)raw[i+1]];
        int8_t v2 = DEC[(uint8_t)raw[i+2]], v3 = DEC[(uint8_t)raw[i+3]];
        uint32_t b = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) | ((uint32_t)v2 << 6) | (uint32_t)v3;
        dp[o++] = (uint8_t)(b >> 16); dp[o++] = (uint8_t)(b >> 8); dp[o++] = (uint8_t)(b);
    }
    uint16_t pos = (uint16_t)dp[0] | ((uint16_t)dp[1] << 8);
    uint16_t spd = (uint16_t)dp[2] | ((uint16_t)dp[3] << 8);
    TEST_ASSERT_EQUAL(65535, pos);
    TEST_ASSERT_EQUAL(999, spd);
}

// =====================================================================
//  11m. :Fa# state — bad checksum rejected
// =====================================================================

void test_focuser_state_bad_checksum(void)
{
    uint8_t pkt[9];
    char reply[20];
    buildStateReply(pkt, reply, sizeof(reply), 100, 50, 2000, 1);

    // Corrupt the checksum byte (last 4 base64 chars encode bytes 6,7,8)
    pkt[8] ^= 0xFF;
    char b64[13]; testB64Encode(pkt, b64, 9);
    snprintf(reply, sizeof(reply), "%s#", b64);
    mockStream.loadResponse(reply);

    char raw[20];
    TEST_ASSERT_EQUAL(LX200_VALUEGET, client->getFocuserAllState(raw, sizeof(raw)));
    TEST_ASSERT_EQUAL(12, (int)strlen(raw));

    static const int8_t DEC[128] = {
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
      52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
      -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
      15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
      -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
      41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    uint8_t dp[9];
    int o = 0;
    for (int i = 0; i < 12; i += 4) {
        int8_t v0 = DEC[(uint8_t)raw[i]], v1 = DEC[(uint8_t)raw[i+1]];
        int8_t v2 = DEC[(uint8_t)raw[i+2]], v3 = DEC[(uint8_t)raw[i+3]];
        uint32_t b = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) | ((uint32_t)v2 << 6) | (uint32_t)v3;
        dp[o++] = (uint8_t)(b >> 16); dp[o++] = (uint8_t)(b >> 8); dp[o++] = (uint8_t)(b);
    }
    uint8_t xc = 0; for (int i = 0; i < 8; i++) xc ^= dp[i];
    TEST_ASSERT_NOT_EQUAL(dp[8], xc);
}

// =====================================================================
//  11n. :FA# — encode-decode round-trip consistency
//  Builds packet like the focuser, encodes, feeds through LX200Client,
//  then verifies every decoded field matches the original input.
// =====================================================================

void test_focuser_full_roundtrip(void)
{
    const uint16_t startPos = 777, maxPos = 40000, lowSpd = 100, hiSpd = 500;
    const uint8_t cmdA = 15, manA = 12, manD = 8;
    const uint8_t rev = 1, mic = 5;
    const uint16_t res = 64;
    const uint8_t cur = 100;
    const uint16_t srot = 400;

    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply),
        startPos, maxPos, lowSpd, hiSpd, cmdA, manA, manD,
        rev, mic, res, cur, srot);

    // Verify base64 string is exactly 200 chars (excluding '#')
    TEST_ASSERT_EQUAL('#', reply[200]);
    TEST_ASSERT_EQUAL('\0', reply[201]);

    // Feed to readFocuserConfig
    mockStream.loadResponse(reply);
    unsigned int sp, mp, ls, hs, ca, ma, md;
    TEST_ASSERT_EQUAL(LX200_VALUEGET,
        client->readFocuserConfig(sp, mp, ls, hs, ca, ma, md));
    TEST_ASSERT_EQUAL(startPos, sp);
    TEST_ASSERT_EQUAL(maxPos, mp);
    TEST_ASSERT_EQUAL(lowSpd, ls);
    TEST_ASSERT_EQUAL(hiSpd, hs);
    TEST_ASSERT_EQUAL(cmdA, ca);
    TEST_ASSERT_EQUAL(manA, ma);
    TEST_ASSERT_EQUAL(manD, md);

    // Feed to readFocuserMotor
    mockStream.loadResponse(reply);
    bool rrev; unsigned int rmic, rres, rcur, rsrot;
    TEST_ASSERT_EQUAL(LX200_VALUEGET,
        client->readFocuserMotor(rrev, rmic, rres, rcur, rsrot));
    TEST_ASSERT_TRUE(rrev);
    TEST_ASSERT_EQUAL(mic, rmic);
    TEST_ASSERT_EQUAL(res, rres);
    TEST_ASSERT_EQUAL(cur, rcur);
    TEST_ASSERT_EQUAL(srot, rsrot);
}

// =====================================================================
//  11o. :Fa# — encode-decode round-trip consistency
// =====================================================================

void test_focuser_state_full_roundtrip(void)
{
    const uint16_t pos = 30000, spd = 150;
    const int16_t tempX100 = -525;
    const uint8_t flags = 1;

    uint8_t pkt[9];
    char reply[20];
    buildStateReply(pkt, reply, sizeof(reply), pos, spd, tempX100, flags);

    // Verify base64 string is exactly 12 chars (excluding '#')
    TEST_ASSERT_EQUAL('#', reply[12]);
    TEST_ASSERT_EQUAL('\0', reply[13]);

    mockStream.loadResponse(reply);
    char raw[20];
    TEST_ASSERT_EQUAL(LX200_VALUEGET, client->getFocuserAllState(raw, sizeof(raw)));
    TEST_ASSERT_EQUAL(12, (int)strlen(raw));

    // Decode and verify
    static const int8_t DEC[128] = {
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
      52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
      -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
      15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
      -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
      41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    uint8_t dp[9];
    int o = 0;
    for (int i = 0; i < 12; i += 4) {
        int8_t v0 = DEC[(uint8_t)raw[i]], v1 = DEC[(uint8_t)raw[i+1]];
        int8_t v2 = DEC[(uint8_t)raw[i+2]], v3 = DEC[(uint8_t)raw[i+3]];
        uint32_t b = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) | ((uint32_t)v2 << 6) | (uint32_t)v3;
        dp[o++] = (uint8_t)(b >> 16); dp[o++] = (uint8_t)(b >> 8); dp[o++] = (uint8_t)(b);
    }
    uint8_t xc = 0; for (int i = 0; i < 8; i++) xc ^= dp[i];
    TEST_ASSERT_EQUAL(dp[8], xc);

    uint16_t rpos = (uint16_t)dp[0] | ((uint16_t)dp[1] << 8);
    uint16_t rspd = (uint16_t)dp[2] | ((uint16_t)dp[3] << 8);
    int16_t rtemp = (int16_t)((uint16_t)dp[4] | ((uint16_t)dp[5] << 8));
    TEST_ASSERT_EQUAL(pos, rpos);
    TEST_ASSERT_EQUAL(spd, rspd);
    TEST_ASSERT_EQUAL(tempX100, rtemp);
    TEST_ASSERT_EQUAL(flags, dp[6]);
}

// =====================================================================
//  11p. Wire format — :FA# sends correct command
// =====================================================================

void test_focuser_config_wire_format(void)
{
    uint8_t pkt[150];
    char reply[210];
    buildConfigReply(pkt, reply, sizeof(reply), 0,0,0,0, 0,0,0, 0,0,0,0,0);
    mockStream.loadResponse(reply);

    unsigned int sp, mp, ls, hs, ca, ma, md;
    client->readFocuserConfig(sp, mp, ls, hs, ca, ma, md);
    // Verify the wire command sent was ":FA#"
    const char* sent = mockStream.getSent();
    TEST_ASSERT_EQUAL_STRING(":FA#", sent);
}

void test_focuser_state_wire_format(void)
{
    uint8_t pkt[9];
    char reply[20];
    buildStateReply(pkt, reply, sizeof(reply), 0, 0, 0, 0);
    mockStream.loadResponse(reply);

    char raw[20];
    client->getFocuserAllState(raw, sizeof(raw));
    const char* sent = mockStream.getSent();
    TEST_ASSERT_EQUAL_STRING(":Fa#", sent);
}

// =====================================================================
//  11q. CommandMeta — :FA# and :Fa# recognized as CMDR_LONG
// =====================================================================

void test_focuser_binary_reply_type(void)
{
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":FA#"));
    TEST_ASSERT_EQUAL(CMDR_LONG, getReplyType(":Fa#"));
}

// =====================================================================
//  main
// =====================================================================

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // 1. Protocol fundamentals
    RUN_TEST(test_getReplyType_get_commands);
    RUN_TEST(test_getReplyType_set_commands);
    RUN_TEST(test_getReplyType_tracking);
    RUN_TEST(test_getReplyType_halt);
    RUN_TEST(test_getReplyType_home_park);
    RUN_TEST(test_getReplyType_rate);
    RUN_TEST(test_getReplyType_alignment);
    RUN_TEST(test_getReplyType_invalid);
    RUN_TEST(test_frame_format_get);
    RUN_TEST(test_frame_format_set);
    RUN_TEST(test_reply_no);
    RUN_TEST(test_reply_short_bool_true);
    RUN_TEST(test_reply_short_bool_false);
    RUN_TEST(test_align_start_sends_A0);
    RUN_TEST(test_align_select_star_1_sends_A1);
    RUN_TEST(test_align_select_star_2_sends_A2);
    RUN_TEST(test_align_select_star_3_sends_A3);
    RUN_TEST(test_reply_long);
    RUN_TEST(test_reply_long_accepts_short_product_board_driver);
    RUN_TEST(test_timeout_returns_failure);

    // 2. Location / Site
    RUN_TEST(test_set_get_latitude);
    RUN_TEST(test_set_get_latitude_negative);
    RUN_TEST(test_set_get_longitude);
    RUN_TEST(test_set_get_longitude_negative);
    RUN_TEST(test_set_get_elevation);
    RUN_TEST(test_set_get_timezone);
    RUN_TEST(test_set_get_selected_site);
    RUN_TEST(test_set_get_mount_idx);
    RUN_TEST(test_set_get_mount_description);

    // 3. Time / Date
    RUN_TEST(test_set_get_utc_time);
    RUN_TEST(test_set_get_utc_date);

    // 4. Rates & Acceleration
    RUN_TEST(test_set_get_acceleration);
    RUN_TEST(test_set_get_max_rate);
    RUN_TEST(test_set_get_deadband);
    RUN_TEST(test_set_get_speed_rate);
    RUN_TEST(test_set_get_stored_track_rate_ra);
    RUN_TEST(test_set_get_stored_track_rate_dec);

    // 5. Limits
    RUN_TEST(test_set_get_min_altitude);
    RUN_TEST(test_set_get_max_altitude);
    RUN_TEST(test_set_get_min_dist_from_pole);
    RUN_TEST(test_set_get_limit_east);
    RUN_TEST(test_set_get_limit_west);
    RUN_TEST(test_set_get_steps_per_second);

    // 6. Mount flags
    RUN_TEST(test_set_get_refraction_on);
    RUN_TEST(test_set_get_refraction_off);
    RUN_TEST(test_set_get_polar_align);
    RUN_TEST(test_set_get_goto_enabled);

    // 7. Motor configuration (per-axis)
    RUN_TEST(test_write_read_reverse_axis1);
    RUN_TEST(test_write_read_reverse_axis2);
    RUN_TEST(test_write_read_backlash_axis1);
    RUN_TEST(test_write_read_backlash_rate_axis1);
    RUN_TEST(test_write_read_tot_gear_axis1);
    RUN_TEST(test_write_read_step_per_rot_axis1);
    RUN_TEST(test_write_read_micro_axis1);
    RUN_TEST(test_write_read_silent_step_axis1);
    RUN_TEST(test_write_read_low_curr_axis1);
    RUN_TEST(test_write_read_high_curr_axis1);
    RUN_TEST(test_write_read_motor_axis2);
    RUN_TEST(test_write_read_high_curr_axis2);

    // 8. Encoder configuration
    RUN_TEST(test_write_read_encoder_reverse_axis1);
    RUN_TEST(test_write_read_pulse_per_degree_axis1);
    RUN_TEST(test_write_read_encoder_auto_sync);

    // 9. Target setting
    RUN_TEST(test_set_get_target_ra);
    RUN_TEST(test_set_get_target_dec);
    RUN_TEST(test_set_get_target_dec_negative);

    // 10. Tracking / Movement
    RUN_TEST(test_enable_tracking_on_off);
    RUN_TEST(test_set_speed_levels);
    RUN_TEST(test_movement_commands_wire_format);
    RUN_TEST(test_stop_directional_wire_format);

    // 11. Focuser binary :FA# config
    RUN_TEST(test_focuser_config_all_zeros);
    RUN_TEST(test_focuser_motor_all_zeros);
    RUN_TEST(test_focuser_config_typical);
    RUN_TEST(test_focuser_motor_typical);
    RUN_TEST(test_focuser_config_max_values);
    RUN_TEST(test_focuser_motor_max_values);
    RUN_TEST(test_focuser_config_user_positions);
    RUN_TEST(test_focuser_config_bad_checksum);
    RUN_TEST(test_focuser_config_wrong_length);
    RUN_TEST(test_focuser_config_invalid_b64_char);
    RUN_TEST(test_focuser_config_no_response);
    // 11. Focuser binary :Fa# state
    RUN_TEST(test_focuser_state_all_zeros);
    RUN_TEST(test_focuser_state_known_values);
    RUN_TEST(test_focuser_state_negative_temp);
    RUN_TEST(test_focuser_state_max_position);
    RUN_TEST(test_focuser_state_bad_checksum);
    // 11. Focuser binary round-trip and wire format
    RUN_TEST(test_focuser_full_roundtrip);
    RUN_TEST(test_focuser_state_full_roundtrip);
    RUN_TEST(test_focuser_config_wire_format);
    RUN_TEST(test_focuser_state_wire_format);
    RUN_TEST(test_focuser_binary_reply_type);

    // 12. CommandCodec round-trips
    RUN_TEST(test_codec_hms_round_trip);
    RUN_TEST(test_codec_dms_round_trip_signed);
    RUN_TEST(test_codec_dms_round_trip_negative);
    RUN_TEST(test_codec_dms_round_trip_unsigned);
    RUN_TEST(test_codec_gethms_getdms);

    return UNITY_END();
}
