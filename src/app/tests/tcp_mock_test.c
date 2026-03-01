/**
 * TCP Mock Test - 构造假数据包测试 TCP 状态机
 *
 * 最小初始化，不依赖网卡/路由/pcap，适合单步调试。
 * 输出函数(send_ack/transmit)会因无路由失败，但不影响状态机逻辑验证。
 */

#include <stdio.h>
#include <string.h>
#include "tcp.h"
#include "tcp_in.h"
#include "tcp_out.h"
#include "tcp_state.h"
#include "tcp_buf.h"
#include "tool.h"
#include "dbug_module.h"

// tcp_free 定义在 tcp.c 中但未在头文件中导出
extern void tcp_free(tcp_t* tcp);

/* ============================== 测试框架 ============================== */

static int pass_count = 0;
static int fail_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("  FAIL [line %d]: %s\n", __LINE__, msg); \
        fail_count++; \
    } else { \
        printf("  PASS: %s\n", msg); \
        pass_count++; \
    } \
} while (0)

#define TEST_START(name) printf("\n========== %s ==========\n", name)

/* ============================== 地址常量 ============================== */

// 本地: 192.168.1.1:80  远端: 192.168.1.100:12345
static const uint8_t LOCAL_IP_BUF[] = {192, 168, 1, 1};
static const uint8_t REMOTE_IP_BUF[] = {192, 168, 1, 100};
static const uint16_t LOCAL_PORT = 80;
static const uint16_t REMOTE_PORT = 12345;

/* ============================== 辅助函数 ============================== */

/**
 * 构造 TCP 数据包（网络字节序），checksum=0 跳过校验
 */
static pktbuf_t* build_tcp_packet(
    uint16_t src_port, uint16_t dst_port,
    uint32_t seq, uint32_t ack,
    int f_syn, int f_ack, int f_fin, int f_rst,
    const uint8_t* data, int data_len)
{
    int total = (int)sizeof(tcp_header_t) + data_len;
    pktbuf_t* buf = pktbuf_alloc(total);
    if (buf == NULL)
    {
        printf("  ERROR: pktbuf_alloc failed\n");
        return NULL;
    }

    tcp_header_t* hdr = (tcp_header_t*)pktbuf_data(buf);
    memset(hdr, 0, sizeof(tcp_header_t));
    hdr->src_port = x_htons(src_port);
    hdr->dest_port = x_htons(dst_port);
    hdr->seq_num = x_htonl(seq);
    hdr->ack_num = x_htonl(ack);
    hdr->f_data_offset = sizeof(tcp_header_t) / 4; // 5 (20 bytes)
    hdr->f_syn = f_syn;
    hdr->f_ack = f_ack;
    hdr->f_fin = f_fin;
    hdr->f_rst = f_rst;
    hdr->window_size = x_htons(65535);
    hdr->checksum = 0;

    if (data && data_len > 0)
    {
        memcpy((uint8_t*)hdr + sizeof(tcp_header_t), data, data_len);
    }
    return buf;
}

/**
 * 创建一个 TCP socket 并设置地址，状态为 CLOSE
 */
static tcp_t* create_test_tcp(void)
{
    sock_t* sock = tcp_create(AF_INET, IPPROTO_TCP);
    if (sock == NULL)
    {
        printf("  ERROR: tcp_create failed\n");
        return NULL;
    }
    tcp_t* tcp = (tcp_t*)sock;

    ipaddr_from_buf(&sock->local_ip, LOCAL_IP_BUF);
    sock->local_port = LOCAL_PORT;
    ipaddr_from_buf(&sock->remote_ip, REMOTE_IP_BUF);
    sock->remote_port = REMOTE_PORT;

    return tcp;
}

/**
 * 将 TCP 设为 ESTABLISHED 状态（模拟三次握手完成后）
 */
static void setup_established(tcp_t* tcp, uint32_t local_isn, uint32_t remote_isn)
{
    tcp_set_state(tcp, TCP_STATE_ESTABLISHED);
    tcp->send.isn = local_isn;
    tcp->send.un_ack_seq = local_isn + 1; // SYN 已被确认
    tcp->send.next_seq = local_isn + 1;
    tcp->recv.isn = remote_isn;
    tcp->recv.next_seq = remote_isn + 1;
    tcp->flags.syn_out = 0;
    tcp->flags.fin_out = 0;
    tcp->flags.irs_valid = 1;
}

/**
 * 将包喂给 tcp_input（包会被 tcp_input 释放）
 */
static net_err_t feed_packet(pktbuf_t* pkt)
{
    ipaddr_t src_ip, dst_ip;
    ipaddr_from_buf(&src_ip, REMOTE_IP_BUF);
    ipaddr_from_buf(&dst_ip, LOCAL_IP_BUF);
    return tcp_input(pkt, &src_ip, &dst_ip);
}

/* ============================== 测试用例 ============================== */

/**
 * 测试1: 三次握手（客户端视角）
 * SYN_SENT --收到 SYN+ACK--> ESTABLISHED
 */
static void test_handshake(void)
{
    TEST_START("test_handshake: 三次握手");

    tcp_t* tcp = create_test_tcp();
    if (!tcp) return;

    // 模拟已发送 SYN: ISN=1000, next_seq=1001 (SYN 占一个序列号)
    tcp_set_state(tcp, TCP_STATE_SYN_SENT);
    tcp->send.isn = 1000;
    tcp->send.un_ack_seq = 1000;
    tcp->send.next_seq = 1001;
    tcp->flags.syn_out = 1;

    // 构造服务端的 SYN+ACK: seq=2000, ack=1001
    pktbuf_t* syn_ack = build_tcp_packet(
        REMOTE_PORT, LOCAL_PORT,
        2000, 1001,
        1, 1, 0, 0, // SYN+ACK
        NULL, 0);

    feed_packet(syn_ack);

    // 验证
    TEST_ASSERT(tcp->state == TCP_STATE_ESTABLISHED,
                "state -> ESTABLISHED");
    TEST_ASSERT(tcp->recv.isn == 2000,
                "recv.isn == 2000");
    TEST_ASSERT(tcp->recv.next_seq == 2001,
                "recv.next_seq == 2001 (SYN+1)");
    TEST_ASSERT(tcp->flags.irs_valid == 1,
                "irs_valid 已设置");
    TEST_ASSERT(tcp->flags.syn_out == 0,
                "syn_out 已清除 (SYN 被确认)");
    TEST_ASSERT(tcp->send.un_ack_seq == 1001,
                "send.un_ack_seq == 1001 (SYN 已确认)");

    tcp_free(tcp);
}

/**
 * 测试2: 数据接收
 * ESTABLISHED 收到 ACK+数据 → 数据进入 recv buffer
 *
 * 已知 BUG: tcp_data_in 不更新 recv.next_seq, 不发 ACK, 不唤醒
 */
static void test_data_recv(void)
{
    TEST_START("test_data_recv: 数据接收");

    tcp_t* tcp = create_test_tcp();
    if (!tcp) return;
    setup_established(tcp, 1000, 2000);

    const char* payload = "Hello";
    int payload_len = 5;

    // 服务端发送数据: seq=2001, ack=1001, 携带 "Hello"
    pktbuf_t* data_pkt = build_tcp_packet(
        REMOTE_PORT, LOCAL_PORT,
        2001, 1001,
        0, 1, 0, 0, // ACK
        (const uint8_t*)payload, payload_len);

    feed_packet(data_pkt);

    // 验证: 数据已写入 recv buffer
    TEST_ASSERT(tcp_buf_count(&tcp->recv.buf) == payload_len,
                "recv buffer 应有 5 字节数据");

    // 读出数据验证内容
    uint8_t read_buf[32] = {0};
    int n = tcp_buf_read_recv(&tcp->recv.buf, read_buf, sizeof(read_buf));
    TEST_ASSERT(n == payload_len, "读出 5 字节");
    TEST_ASSERT(memcmp(read_buf, "Hello", 5) == 0,
                "数据内容 == \"Hello\"");

    // BUG 检查: recv.next_seq 应该前进 5
    printf("  [INFO] recv.next_seq = %u (期望 2006)\n", tcp->recv.next_seq);
    TEST_ASSERT(tcp->recv.next_seq == 2006,
                "recv.next_seq == 2006 (BUG: tcp_data_in 未更新)");

    tcp_free(tcp);
}

/**
 * 测试3: 数据发送（写入 send buffer）
 */
static void test_data_send(void)
{
    TEST_START("test_data_send: 数据写入发送缓冲区");

    tcp_t* tcp = create_test_tcp();
    if (!tcp) return;
    setup_established(tcp, 1000, 2000);

    const char* data = "World!";
    int data_len = 6;

    int written = tcp_write_send_buf(tcp, (const uint8_t*)data, data_len);

    TEST_ASSERT(written == data_len, "写入 6 字节");
    TEST_ASSERT(tcp_buf_count(&tcp->send.buf) == data_len,
                "send buffer count == 6");

    // 写满测试
    uint8_t big_buf[TCP_SEND_BUF_SIZE];
    memset(big_buf, 'A', sizeof(big_buf));
    int remaining = tcp_buf_available(&tcp->send.buf);
    int written2 = tcp_write_send_buf(tcp, big_buf, sizeof(big_buf));
    TEST_ASSERT(written2 == remaining,
                "写满: 实际写入 == 可用空间");
    TEST_ASSERT(tcp_buf_available(&tcp->send.buf) == 0,
                "缓冲区已满");

    tcp_free(tcp);
}

/**
 * 测试4: ACK 处理（确认已发送数据）
 */
static void test_ack_process(void)
{
    TEST_START("test_ack_process: ACK 确认数据");

    tcp_t* tcp = create_test_tcp();
    if (!tcp) return;
    setup_established(tcp, 1000, 2000);

    // 向 send buffer 写入 10 字节数据
    const char* data = "0123456789";
    tcp_write_send_buf(tcp, (const uint8_t*)data, 10);

    // 模拟已发送: next_seq 前进 10
    tcp->send.next_seq = 1001 + 10; // 1011

    printf("  [INFO] before ACK: un_ack_seq=%u, next_seq=%u, buf_count=%d\n",
           tcp->send.un_ack_seq, tcp->send.next_seq,
           tcp_buf_count(&tcp->send.buf));

    // 服务端确认前 5 字节: ack=1006
    pktbuf_t* ack_pkt = build_tcp_packet(
        REMOTE_PORT, LOCAL_PORT,
        2001, 1006,
        0, 1, 0, 0, // ACK
        NULL, 0);

    feed_packet(ack_pkt);

    printf("  [INFO] after ACK:  un_ack_seq=%u, buf_count=%d\n",
           tcp->send.un_ack_seq, tcp_buf_count(&tcp->send.buf));

    TEST_ASSERT(tcp->send.un_ack_seq == 1006,
                "un_ack_seq == 1006 (确认了 5 字节)");
    TEST_ASSERT(tcp_buf_count(&tcp->send.buf) == 5,
                "send buffer 剩余 5 字节");

    // 再确认剩余 5 字节: ack=1011
    pktbuf_t* ack_pkt2 = build_tcp_packet(
        REMOTE_PORT, LOCAL_PORT,
        2001, 1011,
        0, 1, 0, 0,
        NULL, 0);

    feed_packet(ack_pkt2);

    TEST_ASSERT(tcp->send.un_ack_seq == 1011,
                "un_ack_seq == 1011 (全部确认)");
    TEST_ASSERT(tcp_buf_count(&tcp->send.buf) == 0,
                "send buffer 清空");

    tcp_free(tcp);
}

/**
 * 测试5: 被动关闭（收到 FIN）
 * ESTABLISHED --收到 FIN+ACK--> CLOSE_WAIT
 */
static void test_passive_close(void)
{
    TEST_START("test_passive_close: 被动关闭");

    tcp_t* tcp = create_test_tcp();
    if (!tcp) return;
    setup_established(tcp, 1000, 2000);

    uint32_t old_next_seq = tcp->recv.next_seq; // 2001

    // 服务端发送 FIN+ACK: seq=2001, ack=1001
    pktbuf_t* fin_pkt = build_tcp_packet(
        REMOTE_PORT, LOCAL_PORT,
        2001, 1001,
        0, 1, 1, 0, // FIN+ACK
        NULL, 0);

    feed_packet(fin_pkt);

    TEST_ASSERT(tcp->state == TCP_STATE_CLOSE_WAIT,
                "state -> CLOSE_WAIT");
    TEST_ASSERT(tcp->recv.next_seq == old_next_seq + 1,
                "recv.next_seq +1 (FIN 占一个序列号)");

    tcp_free(tcp);
}

/**
 * 测试6: RST 处理
 * ESTABLISHED --收到 RST--> CLOSE
 */
static void test_rst(void)
{
    TEST_START("test_rst: RST 重置连接");

    tcp_t* tcp = create_test_tcp();
    if (!tcp) return;
    setup_established(tcp, 1000, 2000);

    // 服务端发送 RST: seq=2001
    pktbuf_t* rst_pkt = build_tcp_packet(
        REMOTE_PORT, LOCAL_PORT,
        2001, 0,
        0, 0, 0, 1, // RST
        NULL, 0);

    feed_packet(rst_pkt);

    TEST_ASSERT(tcp->state == TCP_STATE_CLOSE,
                "state -> CLOSE (连接被重置)");

    tcp_free(tcp);
}

/**
 * 测试7: 环形缓冲区回绕
 * 直接测试 tcp_buf，验证块拷贝优化后的正确性
 */
static void test_tcp_buf(void)
{
    TEST_START("test_tcp_buf: 环形缓冲区回绕");

    // 使用小缓冲区 (16字节) 方便触发回绕
    uint8_t backing[16];
    tcp_buf_t buf;
    tcp_buf_init(&buf, backing, sizeof(backing));

    TEST_ASSERT(tcp_buf_available(&buf) == 16, "初始可用 16");
    TEST_ASSERT(tcp_buf_count(&buf) == 0, "初始 count 0");

    // 写入 12 字节: "ABCDEFGHIJKL"，in 到 12
    uint8_t write_data[12];
    for (int i = 0; i < 12; i++) write_data[i] = 'A' + i;
    tcp_buf_write_send(&buf, write_data, 12);

    TEST_ASSERT(tcp_buf_count(&buf) == 12, "写入后 count == 12");
    TEST_ASSERT(buf.in == 12, "in == 12");

    // 读出 8 字节，out 到 8
    uint8_t read_data[16] = {0};
    int n = tcp_buf_read_recv(&buf, read_data, 8);
    TEST_ASSERT(n == 8, "读出 8 字节");
    TEST_ASSERT(memcmp(read_data, "ABCDEFGH", 8) == 0, "内容 ABCDEFGH");
    TEST_ASSERT(buf.out == 8, "out == 8");
    TEST_ASSERT(tcp_buf_count(&buf) == 4, "剩余 4 字节");

    // 再写入 10 字节 "0123456789"
    // in=12, 写4到末尾(12..15), 再回绕写6到开头(0..5), in=6
    uint8_t write_data2[10] = "0123456789";
    tcp_buf_write_send(&buf, write_data2, 10);

    TEST_ASSERT(tcp_buf_count(&buf) == 14, "count == 14");
    TEST_ASSERT(buf.in == 6, "in == 6 (回绕)");

    // 读出全部 14 字节
    uint8_t read_data2[16] = {0};
    n = tcp_buf_read_recv(&buf, read_data2, 14);
    TEST_ASSERT(n == 14, "读出 14 字节");

    // 前4字节是之前剩余的 "IJKL"，后10字节是 "0123456789"
    TEST_ASSERT(memcmp(read_data2, "IJKL0123456789", 14) == 0,
                "回绕数据完整: IJKL0123456789");
    TEST_ASSERT(tcp_buf_count(&buf) == 0, "全部读完 count == 0");

    printf("  [INFO] 块拷贝优化验证通过\n");
}

/* ============================== 主函数 ============================== */

int main(void)
{
    // 只显示 TCP 模块的调试信息
    dbug_module_enable_only(DBG_MOD_TCP);

    // 最小初始化
    pktbuf_init();
    tcp_init();

    printf("TCP Mock Test\n");
    printf("==============================================\n");

    // 运行全部测试
    test_handshake();
    test_data_recv();
    test_data_send();
    test_ack_process();
    test_passive_close();
    test_rst();
    test_tcp_buf();

    // 汇总
    printf("\n==============================================\n");
    printf("结果: %d PASS, %d FAIL\n", pass_count, fail_count);
    printf("==============================================\n");

    return fail_count > 0 ? 1 : 0;
}
