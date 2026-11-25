// Focused tests for the ETMv4 packet-input lookahead stage.
#include <cstdlib>
#include <iostream>
#include <vector>
#include "opencsd/etmv4/trc_pkt_decode_etmv4i.h"

#define CHECK(x) do { if (!(x)) { std::cerr << "Failed at line " << __LINE__ << "\n"; return 1; } } while (0)
class Probe : public TrcPktDecodeEtmV4I {
public:
    std::vector<EtmV4ITrcPacket> packets;
    std::vector<ocsd_trc_index_t> indexes;
    std::vector<char> operations;
    bool wait;
    size_t wait_at;
    Probe(bool enabled = true, bool timestamps = true) : wait(false), wait_at(0) {
        setComponentOpMode(enabled ? ETM4_OPFLG_PKTDEC_CONTEXT_TS : 0);
        EtmV4Config config;
        ocsd_etmv4_cfg regs = static_cast<const ocsd_etmv4_cfg &>(config);
        if (timestamps)
            regs.reg_configr |= 1U << 11;
        else
            regs.reg_configr &= ~(1U << 11);
        config = &regs;
        setProtocolConfig(&config);
        m_decode_init_ok = true;
    }
protected:
    virtual ocsd_datapath_resp_t processPacket() {
        operations.push_back('D');
        packets.push_back(*m_curr_packet_in);
        indexes.push_back(m_index_curr_pkt);
        if (wait || packets.size() == wait_at) { wait = false; return OCSD_RESP_WAIT; }
        return OCSD_RESP_CONT;
    }
    virtual ocsd_datapath_resp_t onFlush() { operations.push_back('F'); return OCSD_RESP_CONT; }
    virtual ocsd_datapath_resp_t onEOT() { operations.push_back('E'); return OCSD_RESP_CONT; }
    virtual ocsd_datapath_resp_t onReset() { return OCSD_RESP_CONT; }
};
static ocsd_datapath_resp_t send(Probe &p, ocsd_etmv4_i_pkt_type type, unsigned index) {
    EtmV4ITrcPacket packet;
    packet.setType(type);
    if (type == ETM4_PKT_I_TIMESTAMP) { packet.setTS(1234, 64); packet.setCycleCount(42); }
    return p.PacketDataIn(OCSD_OP_DATA, index, &packet);
}
class Sink : public ITrcGenElemIn {
public:
    std::vector<OcsdTraceElement> elements;
    virtual ocsd_datapath_resp_t TraceElemIn(ocsd_trc_index_t, uint8_t,
        const OcsdTraceElement &elem) { elements.push_back(elem); return OCSD_RESP_CONT; }
};
int main() {
#ifdef _WIN32
    _putenv_s("OPENCSD_CONTEXT_TS_BUFFER_SIZE", "");
#else
    unsetenv("OPENCSD_CONTEXT_TS_BUFFER_SIZE");
#endif
    Probe p;
    send(p, ETM4_PKT_I_CTXT, 0);
    send(p, ETM4_PKT_I_EVENT, 1);
    CHECK(p.packets.empty());
    p.wait = true;
    CHECK(send(p, ETM4_PKT_I_TIMESTAMP, 2) == OCSD_RESP_WAIT);
    CHECK(p.PacketDataIn(OCSD_OP_FLUSH, 0, 0) == OCSD_RESP_CONT);
    CHECK(p.packets.size() == 4);
    CHECK(p.packets[1].getType() == ETM4_PKT_I_TIMESTAMP);
    CHECK(p.packets[1].getTS() == 1234 && p.packets[1].getCC() == 0);
    CHECK(!p.packets[1].pkt_valid.bits.cc_valid);
    CHECK(p.packets[1].pkt_valid.bits.timestamp_copied);
    CHECK(!p.packets[3].pkt_valid.bits.timestamp_copied);
    EtmV4ITrcPacket reused = p.packets[1];
    reused.initNextPacket();
    CHECK(!reused.pkt_valid.bits.timestamp_copied);
    CHECK(p.packets[3].getCC() == 42 && p.indexes[1] == 0 && p.indexes[3] == 2);
    Probe copy_wait;
    copy_wait.wait_at = 2;
    send(copy_wait, ETM4_PKT_I_CTXT, 0);
    CHECK(send(copy_wait, ETM4_PKT_I_TIMESTAMP, 1) == OCSD_RESP_WAIT);
    CHECK(copy_wait.packets.size() == 2);
    CHECK(copy_wait.PacketDataIn(OCSD_OP_FLUSH, 0, 0) == OCSD_RESP_CONT);
    CHECK(copy_wait.packets.size() == 3);
    send(copy_wait, ETM4_PKT_I_EVENT, 2);
    CHECK(copy_wait.packets.size() == 4);
    send(copy_wait, ETM4_PKT_I_CTXT, 3);
    CHECK(copy_wait.packets.size() == 4);
    send(copy_wait, ETM4_PKT_I_TIMESTAMP, 4);
    CHECK(copy_wait.packets.size() == 7);
    Probe boundary;
    send(boundary, ETM4_PKT_I_CTXT, 0);
    for (unsigned i = 1; i < 100; ++i) send(boundary, ETM4_PKT_I_EVENT, i);
    CHECK(boundary.packets.empty());
    send(boundary, ETM4_PKT_I_TIMESTAMP, 100);
    CHECK(boundary.packets.size() == 102 && boundary.packets[1].getTS() == 1234);
    Probe beyond;
    send(beyond, ETM4_PKT_I_CTXT, 0);
    for (unsigned i = 1; i <= 100; ++i) send(beyond, ETM4_PKT_I_EVENT, i);
    send(beyond, ETM4_PKT_I_TIMESTAMP, 101);
    CHECK(beyond.packets.size() == 102 && beyond.packets[1].getType() == ETM4_PKT_I_EVENT);
    Probe multiple;
    send(multiple, ETM4_PKT_I_CTXT, 0);
    send(multiple, ETM4_PKT_I_ADDR_CTXT_L_64IS0, 1);
    send(multiple, ETM4_PKT_I_TIMESTAMP, 2);
    CHECK(multiple.packets.size() == 4 && multiple.indexes[2] == 1);
    Probe batch;
    send(batch, ETM4_PKT_I_EVENT, 0);
    CHECK(batch.packets.size() == 1);
    send(batch, ETM4_PKT_I_CTXT, 1);
    for (unsigned i = 1; i < 100; ++i)
        send(batch, i == 99 ? ETM4_PKT_I_CTXT : ETM4_PKT_I_EVENT, i + 1);
    CHECK(batch.packets.size() == 1);
    batch.PacketDataIn(OCSD_OP_FLUSH, 0, 0);
    CHECK(batch.packets.size() == 101);
    CHECK(batch.operations.back() == 'F');
    send(batch, ETM4_PKT_I_EVENT, 101);
    CHECK(batch.packets.size() == 102);
    send(batch, ETM4_PKT_I_TIMESTAMP, 102);
    CHECK(batch.packets.size() == 103);
    send(batch, ETM4_PKT_I_CTXT, 103);
    send(batch, ETM4_PKT_I_TIMESTAMP, 104);
    CHECK(batch.packets.size() == 106);
    Probe eot;
    send(eot, ETM4_PKT_I_CTXT, 0);
    eot.PacketDataIn(OCSD_OP_EOT, 0, 0);
    CHECK(eot.packets.size() == 1);
    CHECK(eot.operations.size() == 2 && eot.operations[0] == 'D' && eot.operations[1] == 'E');
    Probe flush;
    send(flush, ETM4_PKT_I_CTXT, 0);
    send(flush, ETM4_PKT_I_EVENT, 1);
    CHECK(flush.operations.empty());
    CHECK(flush.PacketDataIn(OCSD_OP_FLUSH, 0, 0) == OCSD_RESP_CONT);
    CHECK(flush.operations.size() == 3 && flush.operations[2] == 'F');
    send(flush, ETM4_PKT_I_TIMESTAMP, 2);
    CHECK(flush.packets.size() == 3 && !flush.packets[2].pkt_valid.bits.timestamp_copied);
    for (unsigned i = 0; i < 2; ++i) {
        Probe stalled;
        send(stalled, ETM4_PKT_I_CTXT, 0);
        send(stalled, ETM4_PKT_I_EVENT, 1);
        stalled.wait_at = 1;
        CHECK(stalled.PacketDataIn(i ? OCSD_OP_EOT : OCSD_OP_FLUSH, 0, 0) == OCSD_RESP_WAIT);
        CHECK(stalled.operations.size() == 1 && stalled.operations[0] == 'D');
        CHECK(stalled.PacketDataIn(OCSD_OP_FLUSH, 0, 0) == OCSD_RESP_CONT);
        CHECK(stalled.packets.size() == 2);
        CHECK(stalled.operations.size() == 4);
        CHECK(stalled.operations[1] == 'F' && stalled.operations[2] == 'D');
        CHECK(stalled.operations[3] == (i ? 'E' : 'F'));
    }
    Probe reset;
    send(reset, ETM4_PKT_I_CTXT, 0);
    reset.PacketDataIn(OCSD_OP_RESET, 0, 0);
    send(reset, ETM4_PKT_I_TIMESTAMP, 1);
    CHECK(reset.packets.size() == 1);
    Probe disabled(false);
    send(disabled, ETM4_PKT_I_CTXT, 0);
    CHECK(disabled.packets.size() == 1);
    Probe no_timestamps(true, false);
    send(no_timestamps, ETM4_PKT_I_CTXT, 0);
    CHECK(no_timestamps.packets.size() == 1);
    send(no_timestamps, ETM4_PKT_I_TIMESTAMP, 1);
    CHECK(no_timestamps.packets.size() == 2);
#ifdef _WIN32
    _putenv_s("OPENCSD_CONTEXT_TS_BUFFER_SIZE", "10");
#else
    setenv("OPENCSD_CONTEXT_TS_BUFFER_SIZE", "10", 1);
#endif
    Probe configured;
    send(configured, ETM4_PKT_I_CTXT, 0);
    for (unsigned i = 1; i < 10; ++i)
        send(configured, ETM4_PKT_I_EVENT, i);
    CHECK(configured.packets.empty());
    send(configured, ETM4_PKT_I_EVENT, 10);
    CHECK(configured.packets.size() == 11);
#ifdef _WIN32
    _putenv_s("OPENCSD_CONTEXT_TS_BUFFER_SIZE", "10");
#else
    setenv("OPENCSD_CONTEXT_TS_BUFFER_SIZE", "10", 1);
#endif
    Sink sink;
    TrcPktDecodeEtmV4I actual;
    actual.setComponentOpMode(ETM4_OPFLG_PKTDEC_CONTEXT_TS);
    actual.setUsesMemAccess(false);
    actual.setUsesIDecode(false);
    actual.getTraceElemOutAttachPt()->attach(&sink);
    EtmV4Config config;
    ocsd_etmv4_cfg regs = static_cast<const ocsd_etmv4_cfg &>(config);
    regs.reg_configr |= (1U << 4) | (1U << 11);
    regs.reg_idr1 = (regs.reg_idr1 & ~0xFF0U) | 0x460U;
    regs.reg_idr0 |= 0x800000U; // timestamp marker support
    config = &regs;
    CHECK(actual.setProtocolConfig(&config) == OCSD_OK);
    EtmV4ITrcPacket packet;
    packet.setType(ETM4_PKT_I_ASYNC);
    actual.PacketDataIn(OCSD_OP_DATA, 0, &packet);
    packet.initNextPacket(); packet.setType(ETM4_PKT_I_TRACE_INFO);
    packet.setTraceInfo(0);
    actual.PacketDataIn(OCSD_OP_DATA, 1, &packet);
    packet.initNextPacket(); packet.setType(ETM4_PKT_I_CTXT);
    packet.setContextInfo(true);
    actual.PacketDataIn(OCSD_OP_DATA, 2, &packet);
    packet.initNextPacket(); packet.setType(ETM4_PKT_I_TS_MARKER);
    actual.PacketDataIn(OCSD_OP_DATA, 3, &packet);
    packet.initNextPacket(); packet.setType(ETM4_PKT_I_TIMESTAMP);
    packet.setTS(1234, 64); packet.setCycleCount(42);
    actual.PacketDataIn(OCSD_OP_DATA, 4, &packet);
    CHECK(actual.PacketDataIn(OCSD_OP_EOT, 4, 0) == OCSD_RESP_CONT);
    unsigned copied = 0, original = 0;
    for (size_t i = 0; i < sink.elements.size(); ++i) {
        const OcsdTraceElement &elem = sink.elements[i];
        if (elem.elem_type != OCSD_GEN_TRC_ELEM_TIMESTAMP) continue;
        CHECK(elem.timestamp == 1234);
        if (elem.timestamp_out_of_order) { ++copied; CHECK(!elem.has_cc); }
        else { ++original; CHECK(elem.has_cc && elem.cycle_count == 42); }
    }
    CHECK(copied == 1 && original == 1);
    std::cout << "Context timestamp buffer checks passed\n";
}
