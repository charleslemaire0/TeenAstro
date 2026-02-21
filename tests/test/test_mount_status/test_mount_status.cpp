/*
 * test_mount_status.cpp - Unit tests for TeenAstroMountStatus
 *
 * Covers:
 *   - b64Decode: valid, truncated, corrupted, wrong-length inputs
 *   - safeFloat: NaN, Inf, normal, boundary values
 *   - formatRaStr: normal, negative, very large, edge-case wrapping
 *   - formatDegStr: positive, negative, near-boundary, wrapping
 *   - formatAzStr: normal, negative, >360, near-boundary
 *   - MountState::parseFrom: valid string, truncated, all-zeros, max values
 *   - updateAllState (via mock): valid packet, checksum error, truncated,
 *     corrupt floats (NaN/Inf in position fields)
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
#include <cmath>
#include <cfloat>
#include <climits>

// =====================================================================
//  Expose static functions from TeenAstroMountStatus.cpp for testing.
//  Since we include the .cpp directly, the static functions are visible.
//  We re-declare them here for clarity.
// =====================================================================
extern "C++" {
    // These are file-static in TeenAstroMountStatus.cpp but visible
    // because we #include'd the .cpp directly.
}

// =====================================================================
//  MockStream for LX200Client
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

// =====================================================================
//  1. safeFloat
// =====================================================================
void test_safeFloat_normal(void) {
    TEST_ASSERT_TRUE(safeFloat(0.0f));
    TEST_ASSERT_TRUE(safeFloat(1.0f));
    TEST_ASSERT_TRUE(safeFloat(-1.0f));
    TEST_ASSERT_TRUE(safeFloat(23.999f));
    TEST_ASSERT_TRUE(safeFloat(359.999f));
}

void test_safeFloat_nan(void) {
    float nan = 0.0f / 0.0f;
    TEST_ASSERT_FALSE(safeFloat(nan));
}

void test_safeFloat_inf(void) {
    float pinf = 1.0f / 0.0f;
    float ninf = -1.0f / 0.0f;
    TEST_ASSERT_FALSE(safeFloat(pinf));
    TEST_ASSERT_FALSE(safeFloat(ninf));
}

void test_safeFloat_extreme(void) {
    TEST_ASSERT_FALSE(safeFloat(1e16f));
    TEST_ASSERT_FALSE(safeFloat(-1e16f));
    TEST_ASSERT_TRUE(safeFloat(1e14f));
}

// =====================================================================
//  2. b64Decode
// =====================================================================
void test_b64Decode_valid(void) {
    // "AAAA" decodes to {0, 0, 0}
    uint8_t out[3];
    TEST_ASSERT_TRUE(b64Decode("AAAA", 4, out));
    TEST_ASSERT_EQUAL(0, out[0]);
    TEST_ASSERT_EQUAL(0, out[1]);
    TEST_ASSERT_EQUAL(0, out[2]);
}

void test_b64Decode_known_value(void) {
    // "AQID" = [1, 2, 3]
    uint8_t out[3];
    TEST_ASSERT_TRUE(b64Decode("AQID", 4, out));
    TEST_ASSERT_EQUAL(1, out[0]);
    TEST_ASSERT_EQUAL(2, out[1]);
    TEST_ASSERT_EQUAL(3, out[2]);
}

void test_b64Decode_wrong_length(void) {
    uint8_t out[3];
    TEST_ASSERT_FALSE(b64Decode("AAA", 3, out));   // not multiple of 4
    TEST_ASSERT_FALSE(b64Decode("AAAAA", 5, out));
}

void test_b64Decode_invalid_chars(void) {
    uint8_t out[3];
    TEST_ASSERT_FALSE(b64Decode("AA!A", 4, out));   // '!' is invalid
    TEST_ASSERT_FALSE(b64Decode("AA\x80" "A", 4, out));  // >127
}

void test_b64Decode_empty(void) {
    uint8_t out[1] = {0xFF};
    TEST_ASSERT_TRUE(b64Decode("", 0, out));  // 0 bytes decoded
    TEST_ASSERT_EQUAL(0xFF, out[0]); // untouched
}

// =====================================================================
//  3. formatRaStr
// =====================================================================
void test_formatRa_zero(void) {
    char buf[16];
    formatRaStr(0.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("00:00:00.00", buf);
}

void test_formatRa_twelve(void) {
    char buf[16];
    formatRaStr(12.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("12:00:00.00", buf);
}

void test_formatRa_negative(void) {
    char buf[16];
    formatRaStr(-1.0f, buf, sizeof(buf));
    // -1 fmod 24 + 24 = 23
    TEST_ASSERT_EQUAL_STRING("23:00:00.00", buf);
}

void test_formatRa_very_negative(void) {
    char buf[16];
    formatRaStr(-49.0f, buf, sizeof(buf));
    // -49 fmod 24 = -1, +24 = 23
    TEST_ASSERT_EQUAL_STRING("23:00:00.00", buf);
}

void test_formatRa_over_24(void) {
    char buf[16];
    formatRaStr(25.5f, buf, sizeof(buf));
    // 25.5 fmod 24 = 1.5 => 01:30:00.00
    TEST_ASSERT_EQUAL_STRING("01:30:00.00", buf);
}

void test_formatRa_very_large(void) {
    char buf[16];
    formatRaStr(1000.0f, buf, sizeof(buf));
    // 1000 fmod 24 = 16 => 16:00:00.00
    TEST_ASSERT_EQUAL_STRING("16:00:00.00", buf);
}

void test_formatRa_fractional(void) {
    char buf[16];
    // 6h 30m 45.12s = 6 + 30/60 + 45.12/3600 = 6.51253333...
    formatRaStr(6.5125333f, buf, sizeof(buf));
    // Check starts with "06:30:4"
    TEST_ASSERT_TRUE(strncmp(buf, "06:30:4", 7) == 0);
}

void test_formatRa_short_buffer(void) {
    char buf[16] = "XXXXXXXXXXXXXXX";
    formatRaStr(12.0f, buf, sizeof(buf));
    // Full output fits in 16 bytes (11 chars + null)
    TEST_ASSERT_TRUE(strlen(buf) <= 15);
    TEST_ASSERT_TRUE(strncmp(buf, "12:00:", 6) == 0);
}

// =====================================================================
//  4. formatDegStr
// =====================================================================
void test_formatDeg_zero(void) {
    char buf[16];
    formatDegStr(0.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("+00*00:00.0", buf);
}

void test_formatDeg_positive(void) {
    char buf[16];
    formatDegStr(45.5f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("+45*30:00.0", buf);
}

void test_formatDeg_negative(void) {
    char buf[16];
    formatDegStr(-30.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("-30*00:00.0", buf);
}

void test_formatDeg_very_negative(void) {
    char buf[16];
    formatDegStr(-400.0f, buf, sizeof(buf));
    // -400: abs=400, fmod(400,360)=40 => -40*00:00.0
    TEST_ASSERT_EQUAL_STRING("-40*00:00.0", buf);
}

void test_formatDeg_very_large(void) {
    char buf[16];
    formatDegStr(720.0f, buf, sizeof(buf));
    // fmod(720,360)=0 => +00*00:00.0
    TEST_ASSERT_EQUAL_STRING("+00*00:00.0", buf);
}

void test_formatDeg_near_90(void) {
    char buf[16];
    formatDegStr(89.999f, buf, sizeof(buf));
    // Should be close to +89*59:56.4 or similar
    TEST_ASSERT_TRUE(buf[0] == '+');
    TEST_ASSERT_TRUE(buf[1] == '8');
    TEST_ASSERT_TRUE(buf[2] == '9');
}

// =====================================================================
//  5. formatAzStr
// =====================================================================
void test_formatAz_zero(void) {
    char buf[16];
    formatAzStr(0.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("000*00:00.0", buf);
}

void test_formatAz_180(void) {
    char buf[16];
    formatAzStr(180.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("180*00:00.0", buf);
}

void test_formatAz_negative(void) {
    char buf[16];
    formatAzStr(-90.0f, buf, sizeof(buf));
    // fmod(-90,360)+360 = 270
    TEST_ASSERT_EQUAL_STRING("270*00:00.0", buf);
}

void test_formatAz_over_360(void) {
    char buf[16];
    formatAzStr(450.0f, buf, sizeof(buf));
    // fmod(450,360) = 90
    TEST_ASSERT_EQUAL_STRING("090*00:00.0", buf);
}

void test_formatAz_very_large(void) {
    char buf[16];
    formatAzStr(10000.0f, buf, sizeof(buf));
    // fmod(10000,360) = 280
    TEST_ASSERT_EQUAL_STRING("280*00:00.0", buf);
}

// =====================================================================
//  6. MountState::parseFrom (legacy :GXI# string parser)
// =====================================================================
void test_parseFrom_valid(void) {
    // Build a valid 17-char status string
    //  [0]='1' tracking on, [1]='0' sidereal, [2]='p' unparked,
    //  [3]='H' at home, [4]='2' medium, [5]=' ' no spiral,
    //  [6]=' ' no pulse, [7]=' ' no guide EW, [8]=' ' no guide NS,
    //  [9]=' ' reserved, [10]='1' comp RA, [11]='1' aligned,
    //  [12]='E' GEM, [13]='W' west, [14]='A' GNSS none,
    //  [15]='0' no error, [16]='I' enable=8 (motors on)
    char raw[18] = "10pH2  " "  " " 11EWA0I";
    raw[3] = 'H';
    raw[4] = '2';
    raw[5] = ' ';
    raw[6] = ' ';
    raw[7] = ' ';
    raw[8] = ' ';
    raw[9] = ' ';
    raw[10] = '1';

    MountState ms;
    ms.parseFrom(raw);
    TEST_ASSERT_TRUE(ms.valid);
    TEST_ASSERT_EQUAL(MountState::TRK_ON, ms.tracking);
    TEST_ASSERT_EQUAL(MountState::SID_STAR, ms.sidereal);
    TEST_ASSERT_EQUAL(MountState::PRK_UNPARKED, ms.parkState);
    TEST_ASSERT_TRUE(ms.atHome);
    TEST_ASSERT_EQUAL(MountState::MOUNT_TYPE_GEM, ms.mountType);
    TEST_ASSERT_EQUAL(MountState::PIER_W, ms.pierSide);
    TEST_ASSERT_TRUE(ms.aligned);
    TEST_ASSERT_EQUAL(MountState::ERR_NONE, ms.error);
}

void test_parseFrom_truncated(void) {
    MountState ms;
    ms.parseFrom("10pH");  // only 4 chars, needs 17
    TEST_ASSERT_FALSE(ms.valid);
}

void test_parseFrom_null(void) {
    MountState ms;
    ms.parseFrom(nullptr);
    TEST_ASSERT_FALSE(ms.valid);
}

void test_parseFrom_unknown_tracking(void) {
    char raw[18] = "X0pH2     11EWA0I";
    MountState ms;
    ms.parseFrom(raw);
    TEST_ASSERT_TRUE(ms.valid);
    TEST_ASSERT_EQUAL(MountState::TRK_UNKNOW, ms.tracking);
}

// =====================================================================
//  7. updateAllState via mock (GXAS packet)
// =====================================================================

// Helper: encode 66 bytes into 88 base64 chars (simple encoder for tests)
static const char B64ENC[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void b64Encode(const uint8_t* in, int inLen, char* out) {
    int i = 0, o = 0;
    for (; i + 2 < inLen; i += 3) {
        out[o++] = B64ENC[(in[i] >> 2) & 0x3F];
        out[o++] = B64ENC[((in[i] & 0x3) << 4) | ((in[i+1] >> 4) & 0xF)];
        out[o++] = B64ENC[((in[i+1] & 0xF) << 2) | ((in[i+2] >> 6) & 0x3)];
        out[o++] = B64ENC[in[i+2] & 0x3F];
    }
    out[o] = '\0';
}

static void buildValidPacket(uint8_t* pkt) {
    memset(pkt, 0, 66);
    // byte 0: tracking=on(1), sidereal=star(0), park=unparked(0), atHome=0, pier=E(0)
    pkt[0] = 0x01;
    // byte 1: guidingRate=0(guiding), aligned=1(bit3), mount=GEM(1, bit4-6), spiral=0
    pkt[1] = 0x08 | 0x10;  // aligned + GEM
    // byte 2: guidingEW=none(0), guidingNS=none(0), rateComp=RA(1,bit4-5), pulseGuide=0
    pkt[2] = 0x10;  // rateComp=1(RA) in bits 4-5
    // byte 3: GNSS flags
    pkt[3] = 0x00;
    // byte 4: error
    pkt[4] = 0;  // no error
    // byte 5: enable flags (motors on = bit 3)
    pkt[5] = 0x08;
    // bytes 6-11: UTC 12:30:45 01/15/24
    pkt[6] = 12; pkt[7] = 30; pkt[8] = 45;
    pkt[9] = 1; pkt[10] = 15; pkt[11] = 24;
    // bytes 12-15: RA = 6.5 hours (float LE)
    float ra = 6.5f;
    memcpy(&pkt[12], &ra, 4);
    // bytes 16-19: Dec = 45.25 deg
    float dec = 45.25f;
    memcpy(&pkt[16], &dec, 4);
    // bytes 20-23: Alt = 60.0 deg
    float alt = 60.0f;
    memcpy(&pkt[20], &alt, 4);
    // bytes 24-27: Az = 180.0 deg
    float az = 180.0f;
    memcpy(&pkt[24], &az, 4);
    // bytes 28-31: LST = 18.75 hours
    float lst = 18.75f;
    memcpy(&pkt[28], &lst, 4);
    // bytes 32-35: target RA = 12.0 hours
    float tRA = 12.0f;
    memcpy(&pkt[32], &tRA, 4);
    // bytes 36-39: target Dec = -20.0 deg
    float tDec = -20.0f;
    memcpy(&pkt[36], &tDec, 4);
    // bytes 40-55: tracking rates (all zero for now)
    // bytes 56-61: focuser (ignored, no focuser)
    // byte 65: XOR checksum of bytes 0-64
    uint8_t xorChk = 0;
    for (int i = 0; i < 65; i++) xorChk ^= pkt[i];
    pkt[65] = xorChk;
}

static void loadGXASResponse(const uint8_t* pkt) {
    char b64[92];
    b64Encode(pkt, 66, b64);
    // Mock response includes '#' terminator
    char resp[96];
    snprintf(resp, sizeof(resp), "%s#", b64);
    mockStream.loadResponse(resp);
}

void test_updateAllState_valid(void) {
    uint8_t pkt[66];
    buildValidPacket(pkt);
    loadGXASResponse(pkt);

    status->updateAllState(true);
    TEST_ASSERT_TRUE(status->mountState().valid);
    TEST_ASSERT_EQUAL(MountState::TRK_ON, status->getTrackingState());
    TEST_ASSERT_EQUAL(MountState::PRK_UNPARKED, status->getParkState());
    TEST_ASSERT_TRUE(status->isAligned());
    TEST_ASSERT_EQUAL(MountState::MOUNT_TYPE_GEM, status->getMount());
    TEST_ASSERT_EQUAL(MountState::ERR_NONE, status->getError());

    // Check formatted RA starts with "06:30:"
    TEST_ASSERT_TRUE(strncmp(status->getRa(), "06:30:", 6) == 0);
    // Check Dec starts with "+45*15:"
    TEST_ASSERT_TRUE(strncmp(status->getDec(), "+45*15:", 7) == 0);
    // Check Alt starts with "+60*00:"
    TEST_ASSERT_TRUE(strncmp(status->getAlt(), "+60*00:", 7) == 0);
    // Check Az starts with "180*00:"
    TEST_ASSERT_TRUE(strncmp(status->getAz(), "180*00:", 7) == 0);
    // Check sidereal starts with "18:45:"
    TEST_ASSERT_TRUE(strncmp(status->getSidereal(), "18:45:", 6) == 0);
    // Check UTC
    TEST_ASSERT_EQUAL_STRING("12:30:45", status->getUTC());
    TEST_ASSERT_EQUAL_STRING("01/15/24", status->getUTCdate());
}

void test_updateAllState_bad_checksum(void) {
    uint8_t pkt[66];
    buildValidPacket(pkt);
    pkt[65] ^= 0xFF;  // corrupt checksum
    loadGXASResponse(pkt);

    status->updateAllState(true);
    TEST_ASSERT_FALSE(status->mountState().valid);
    // Position and time caches should be invalidated
    TEST_ASSERT_EQUAL_STRING("?", status->getRa());
    TEST_ASSERT_EQUAL_STRING("?", status->getDec());
    TEST_ASSERT_EQUAL_STRING("?", status->getAlt());
    TEST_ASSERT_EQUAL_STRING("?", status->getAz());
    TEST_ASSERT_EQUAL_STRING("?", status->getSidereal());
    TEST_ASSERT_EQUAL_STRING("?", status->getUTC());
    TEST_ASSERT_EQUAL_STRING("?", status->getUTCdate());
}

// Time/date display: Index.cpp uses temp[128] for each of date, time, sidereal
void test_time_date_display_format(void) {
    uint8_t pkt[66];
    buildValidPacket(pkt);
    loadGXASResponse(pkt);
    status->updateAllState(true);
    TEST_ASSERT_TRUE(status->mountState().valid);
    char temp[128];
    const char* fmtDate = "<span class='c'>%s</span>";
    const char* fmtTime = " <span class='c'>%s</span> UT";
    const char* fmtSid  = " (<span class='c'>%s</span> LST)<br />";
    int dLen = snprintf(temp, sizeof(temp), fmtDate, status->getUTCdate());
    int tLen = snprintf(temp, sizeof(temp), fmtTime, status->getUTC());
    int sLen = snprintf(temp, sizeof(temp), fmtSid, status->getSidereal());
    TEST_ASSERT_LESS_THAN(128, dLen);
    TEST_ASSERT_LESS_THAN(128, tLen);
    TEST_ASSERT_LESS_THAN(128, sLen);
}

void test_updateAllState_short_response(void) {
    // Load a response that's too short (not 88 base64 chars)
    mockStream.loadResponse("AAAA#");

    status->updateAllState(true);
    TEST_ASSERT_FALSE(status->mountState().valid);
    TEST_ASSERT_EQUAL_STRING("?", status->getRa());
}

void test_updateAllState_no_response(void) {
    // Don't load any response -> timeout / no data
    status->updateAllState(true);
    TEST_ASSERT_FALSE(status->mountState().valid);
    TEST_ASSERT_EQUAL_STRING("?", status->getRa());
}

void test_updateAllState_nan_in_ra(void) {
    uint8_t pkt[66];
    buildValidPacket(pkt);
    // Inject NaN into RA (bytes 12-15)
    uint32_t nan_bits = 0x7FC00000;  // quiet NaN
    memcpy(&pkt[12], &nan_bits, 4);
    // Recompute checksum
    uint8_t xorChk = 0;
    for (int i = 0; i < 65; i++) xorChk ^= pkt[i];
    pkt[65] = xorChk;
    loadGXASResponse(pkt);

    status->updateAllState(true);
    TEST_ASSERT_TRUE(status->mountState().valid);
    // RA should be "?" because safeFloat rejects NaN
    TEST_ASSERT_EQUAL_STRING("?", status->getRa());
    // Dec should still be valid
    TEST_ASSERT_TRUE(strncmp(status->getDec(), "+45*15:", 7) == 0);
}

void test_updateAllState_inf_in_dec(void) {
    uint8_t pkt[66];
    buildValidPacket(pkt);
    // Inject +Inf into Dec (bytes 16-19)
    uint32_t inf_bits = 0x7F800000;  // +Inf
    memcpy(&pkt[16], &inf_bits, 4);
    // Recompute checksum
    uint8_t xorChk = 0;
    for (int i = 0; i < 65; i++) xorChk ^= pkt[i];
    pkt[65] = xorChk;
    loadGXASResponse(pkt);

    status->updateAllState(true);
    TEST_ASSERT_TRUE(status->mountState().valid);
    // Dec should be "?" because safeFloat rejects Inf
    TEST_ASSERT_EQUAL_STRING("?", status->getDec());
    // RA should still be valid
    TEST_ASSERT_TRUE(strncmp(status->getRa(), "06:30:", 6) == 0);
}

void test_updateAllState_inf_in_az(void) {
    uint8_t pkt[66];
    buildValidPacket(pkt);
    // Inject -Inf into Az (bytes 24-27)
    uint32_t ninf_bits = 0xFF800000;  // -Inf
    memcpy(&pkt[24], &ninf_bits, 4);
    uint8_t xorChk = 0;
    for (int i = 0; i < 65; i++) xorChk ^= pkt[i];
    pkt[65] = xorChk;
    loadGXASResponse(pkt);

    status->updateAllState(true);
    TEST_ASSERT_TRUE(status->mountState().valid);
    TEST_ASSERT_EQUAL_STRING("?", status->getAz());
    // Alt should still be valid
    TEST_ASSERT_TRUE(strncmp(status->getAlt(), "+60*00:", 7) == 0);
}

void test_updateAllState_corrupt_b64(void) {
    // 88 chars of '!' which are not valid base64
    char resp[92];
    memset(resp, '!', 88);
    resp[88] = '#';
    resp[89] = '\0';
    mockStream.loadResponse(resp);

    status->updateAllState(true);
    TEST_ASSERT_FALSE(status->mountState().valid);
    TEST_ASSERT_EQUAL_STRING("?", status->getRa());
}

// =====================================================================
//  8. getLastErrorMessage safety
// =====================================================================
void test_getLastErrorMessage_none(void) {
    char msg[80];
    status->getLastErrorMessage(msg);
    TEST_ASSERT_EQUAL_STRING("None", msg);
}

// =====================================================================
//  9. invalidatePositionTimeCaches
// =====================================================================
void test_invalidate_sets_question_marks(void) {
    // First load valid data
    uint8_t pkt[66];
    buildValidPacket(pkt);
    loadGXASResponse(pkt);
    status->updateAllState(true);
    TEST_ASSERT_TRUE(strncmp(status->getRa(), "06:30:", 6) == 0);

    // Now invalidate
    status->invalidatePositionTimeCaches();
    TEST_ASSERT_EQUAL_STRING("?", status->getRa());
    TEST_ASSERT_EQUAL_STRING("?", status->getDec());
    TEST_ASSERT_EQUAL_STRING("?", status->getAlt());
    TEST_ASSERT_EQUAL_STRING("?", status->getAz());
    TEST_ASSERT_EQUAL_STRING("?", status->getSidereal());
    TEST_ASSERT_EQUAL_STRING("?", status->getUTC());
    TEST_ASSERT_EQUAL_STRING("?", status->getUTCdate());
}

// =====================================================================
//  10. Boundary enum values from GXAS binary
// =====================================================================
void test_updateAllState_all_maxed_enum_bits(void) {
    uint8_t pkt[66];
    memset(pkt, 0xFF, 66);
    // Compute valid checksum
    uint8_t xorChk = 0;
    for (int i = 0; i < 65; i++) xorChk ^= pkt[i];
    pkt[65] = xorChk;
    loadGXASResponse(pkt);

    status->updateAllState(true);
    TEST_ASSERT_TRUE(status->mountState().valid);
    // All 0xFF bytes should produce valid (possibly unknown) enum values, never crash
    // tracking bits 0x3 => 3 => TRK_SLEWING
    TEST_ASSERT_EQUAL(MountState::TRK_SLEWING, status->getTrackingState());
    // parkState bits (b0>>4)&0x3 = 3 => PRK_FAILED
    TEST_ASSERT_EQUAL(MountState::PRK_FAILED, status->getParkState());
    // All floats will be NaN (0xFFFFFFFF), so positions should be "?"
    TEST_ASSERT_EQUAL_STRING("?", status->getRa());
    TEST_ASSERT_EQUAL_STRING("?", status->getDec());
}

void test_updateAllState_all_zero_bits(void) {
    uint8_t pkt[66];
    memset(pkt, 0, 66);
    // Checksum of 65 zeros = 0, which is already in pkt[65]
    loadGXASResponse(pkt);

    status->updateAllState(true);
    TEST_ASSERT_TRUE(status->mountState().valid);
    TEST_ASSERT_EQUAL(MountState::TRK_OFF, status->getTrackingState());
    TEST_ASSERT_EQUAL(MountState::PRK_UNPARKED, status->getParkState());
    TEST_ASSERT_EQUAL(MountState::PIER_E, status->getPierState());
    TEST_ASSERT_FALSE(status->isAligned());
    // RA = 0.0 => "00:00:00.00"
    TEST_ASSERT_EQUAL_STRING("00:00:00.00", status->getRa());
    TEST_ASSERT_EQUAL_STRING("+00*00:00.0", status->getDec());
}

// =====================================================================
//  main
// =====================================================================
int main(int argc, char** argv) {
    UNITY_BEGIN();

    // safeFloat
    RUN_TEST(test_safeFloat_normal);
    RUN_TEST(test_safeFloat_nan);
    RUN_TEST(test_safeFloat_inf);
    RUN_TEST(test_safeFloat_extreme);

    // b64Decode
    RUN_TEST(test_b64Decode_valid);
    RUN_TEST(test_b64Decode_known_value);
    RUN_TEST(test_b64Decode_wrong_length);
    RUN_TEST(test_b64Decode_invalid_chars);
    RUN_TEST(test_b64Decode_empty);

    // formatRaStr
    RUN_TEST(test_formatRa_zero);
    RUN_TEST(test_formatRa_twelve);
    RUN_TEST(test_formatRa_negative);
    RUN_TEST(test_formatRa_very_negative);
    RUN_TEST(test_formatRa_over_24);
    RUN_TEST(test_formatRa_very_large);
    RUN_TEST(test_formatRa_fractional);
    RUN_TEST(test_formatRa_short_buffer);

    // formatDegStr
    RUN_TEST(test_formatDeg_zero);
    RUN_TEST(test_formatDeg_positive);
    RUN_TEST(test_formatDeg_negative);
    RUN_TEST(test_formatDeg_very_negative);
    RUN_TEST(test_formatDeg_very_large);
    RUN_TEST(test_formatDeg_near_90);

    // formatAzStr
    RUN_TEST(test_formatAz_zero);
    RUN_TEST(test_formatAz_180);
    RUN_TEST(test_formatAz_negative);
    RUN_TEST(test_formatAz_over_360);
    RUN_TEST(test_formatAz_very_large);

    // MountState::parseFrom
    RUN_TEST(test_parseFrom_valid);
    RUN_TEST(test_parseFrom_truncated);
    RUN_TEST(test_parseFrom_null);
    RUN_TEST(test_parseFrom_unknown_tracking);

    // Time/date display (Index-style formatting)
    RUN_TEST(test_time_date_display_format);
    // updateAllState via mock
    RUN_TEST(test_updateAllState_valid);
    RUN_TEST(test_updateAllState_bad_checksum);
    RUN_TEST(test_updateAllState_short_response);
    RUN_TEST(test_updateAllState_no_response);
    RUN_TEST(test_updateAllState_nan_in_ra);
    RUN_TEST(test_updateAllState_inf_in_dec);
    RUN_TEST(test_updateAllState_inf_in_az);
    RUN_TEST(test_updateAllState_corrupt_b64);

    // Error message
    RUN_TEST(test_getLastErrorMessage_none);

    // Invalidation
    RUN_TEST(test_invalidate_sets_question_marks);

    // Boundary enum values
    RUN_TEST(test_updateAllState_all_maxed_enum_bits);
    RUN_TEST(test_updateAllState_all_zero_bits);

    return UNITY_END();
}
