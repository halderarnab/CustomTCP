#include <linux/module.h>
#include <net/tcp.h>
#include <linux/math64.h>

/* NOTE: The following included header SHOULD NOT be removed. */
#include "configs.h"

// Additional constants you may find useful.
#define MTU (1500)
#define S_TO_US (1000000)

struct rutcp
{
  u32 rate; /* rate to pace packets, in bytes per second */
  // TODO: Other parameters you want your controller to maintain
  u64 min_rtt_us;
  u64 r_in;
  u64 r_out;
  s64 z;
  u64 prev_r_in;
};

static void tcp_rutcp_init(struct sock *sk)
{
  struct rutcp *ca = inet_csk_ca(sk);
  // TODO: initialize all members of your CC structure
  ca->rate = LINK_CAP;
  sk->sk_pacing_rate = LINK_CAP / 4;
  ca->min_rtt_us = __UINT64_MAX__;
  ca->r_in = 0;
  ca->r_out = 0;
  ca->z = 0;
  ca->prev_r_in = 0;
}

void compute_cross_traffic(struct sock *sk, const struct rate_sample *rs)
{
  u64 r_in;
  u64 r_out;
  s64 new_rate;
  u64 c;
  s64 z;
  struct rutcp *ca = inet_csk_ca(sk);

  if (rs->snd_interval_us != 0)
  {
    r_in = (((u64)rs->delivered) * MTU * S_TO_US) / rs->snd_interval_us; // Bytes per second
    // r_in = rs->delivered * MTU * S_TO_US;
    // r_in = r_in / rs->snd_interval_us;
  }
  else
  {
    r_in = 0;
  }
  ca->r_in = r_in;
  ca->prev_r_in = ca->r_in;
  printk("RUTCP: r_in: %llu Bps\n", r_in);

  if (rs->rcv_interval_us != 0)
  {
    r_out = (((u64)rs->delivered) * MTU * S_TO_US) / rs->rcv_interval_us; // Bytes per second
    // r_out = rs->delivered * MTU * S_TO_US;
    // r_out = r_out / rs->rcv_interval_us;
  }
  else
    r_out = 0;
  ca->r_out = r_out;
  printk("RUTCP: r_out: %llu Bps\n", r_out);

  if (r_in < r_out)
    return;

  c = LINK_CAP;
  printk("RUTCP: c: %llu Bps\n", c); // Bytes per second

  if (rs->rtt_us > (long)(104 * ca->min_rtt_us / 100))
  {
    // z = (((r_in * c) / r_out) - r_in) * 8;
    z = (((r_in * c) / r_out) - r_in);
    ca->z = z;
    if(z == 0)
      r_in = c;
  }
  else
  {
    z = 0;
    ca->z = 0;
  }

  if (r_out == 0)
  {
    z = LINK_CAP;
    ca->z = 0;
  }

  printk("RUTCP: 1 z: %lld Bps\n", z);
  // printk("RUTCP: z: %lld Mbps\n", z / (S_TO_US));
  printk("RUTCP: 2 z: %lld Mbps\n", (z * 8) / (S_TO_US));

  new_rate = (s64)r_in + (s64)(((s64)c - (s64)r_in - (s64)z) / (s64)2);
  printk("RUTCP: new_rate: %lld Bps\n", new_rate);
  // new_rate =((s64)c - (s64)r_in - z);
  // printk("RUTCP: 2 new_rate: %lld Bps\n", new_rate);
  // new_rate = new_rate/2;
  // printk("RUTCP: 3 new_rate: %lld Bps\n", new_rate);
  // new_rate = (s32)r_in + new_rate;
  // printk("RUTCP: 4 new_rate: %lld Bps\n", new_rate);
  if (new_rate > 0)
    ca->r_in = new_rate;
}

void tcp_rutcp_cong_control(struct sock *sk, const struct rate_sample *rs)
{
  if (rs->delivered <= 0 || rs->rtt_us <= 0 || rs->snd_interval_us > rs->rcv_interval_us)
    return;

  struct rutcp *ca = inet_csk_ca(sk);
  struct tcp_sock *tp = tcp_sk(sk);

  printk("RUTCP: -------Starting Cong Control!!-------");
  printk("RUTCP: rs->delivered: %d MTU", rs->delivered);
  printk("RUTCP: rs->snd_interval_us: %u us", rs->snd_interval_us);
  printk("RUTCP: rs->rcv_interval_us: %u us", rs->rcv_interval_us);
  printk("RUTCP: rs->rtt_us: %ld us", rs->rtt_us);
  printk("RUTCP: ca->min_rtt_us: %llu us", ca->min_rtt_us);
  printk("RUTCP: ---------------------------");

  if (rs->rtt_us < ca->min_rtt_us)
    ca->min_rtt_us = rs->rtt_us;
  printk("RUTCP: ca->min_rtt_us: %llu us", ca->min_rtt_us);

  compute_cross_traffic(sk, rs);

  if (ca->r_in < ca->r_out)
    return;

  printk("RUTCP: sk_pacing_rate: %ld Bps\n", sk->sk_pacing_rate);

  // tp->snd_cwnd = sk->sk_pacing_rate * rs->rtt_us / (MTU * S_TO_US) + 5;
  // printk("RUTCP: ca->r_in in cc: %llu Bps\n", ca->r_in);
  // printk("RUTCP: rtt_us in cc: %ld\n", rs->rtt_us);

  // tp->snd_cwnd = ((ca->r_in * rs->rtt_us) / (MTU * S_TO_US)) + 5;
  u64 tmp = ca->r_in * ((u64)rs->rtt_us);
  printk("RUTCP: 1 tmp: %lld\n", tmp);
  u64 tmp_denom = MTU * S_TO_US;
  // printk("RUTCP: 1 tmp_denom: %lld\n", tmp_denom);
  tmp = tmp / tmp_denom;
  tp->snd_cwnd = (u64)(tmp + 5);
  printk("RUTCP: 2 tmp: %llu\n", tmp);
  // printk("RUTCP: tmp_denom: %lld\n", tmp_denom);
  printk("RUTCP: snd_cwnd: %u\n", tp->snd_cwnd);

  // printk("RUTCP: Here is the cross-traffic estimate: %d\n", z / S_TO_US);
  if (tp->snd_cwnd > 100 || tp->snd_cwnd <= 10)
  {
    printk("RUTCP2: -------tp->snd_cwnd > 1000-------");
    printk("RUTCP2: rs->delivered: %d MTU", rs->delivered);
    printk("RUTCP2: rs->snd_interval_us: %u us", rs->snd_interval_us);
    printk("RUTCP2: rs->rcv_interval_us: %u us", rs->rcv_interval_us);
    printk("RUTCP2: rs->rtt_us: %ld us", rs->rtt_us);
    printk("RUTCP2: ca->min_rtt_us: %llu us", ca->min_rtt_us);
    printk("RUTCP2: ---------------------------");
    printk("RUTCP2: ca->prev_r_in: %llu us", ca->prev_r_in);
    printk("RUTCP2: ca->r_in: %llu us", ca->r_in);
    printk("RUTCP2: ca->r_out: %llu us", ca->r_out);
    printk("RUTCP2: z: %lld Bps\n", ca->z);
    printk("RUTCP2: snd_cwnd: %u\n", tp->snd_cwnd);
    printk("RUTCP2: ---------------------------");
  }
  if (tp->snd_cwnd > 100 || tp->snd_cwnd < 1)
    tp->snd_cwnd = 5;
}

static struct tcp_congestion_ops tcp_rutcp = {
    .init = tcp_rutcp_init,
    .cong_control = tcp_rutcp_cong_control,
    .ssthresh = tcp_reno_ssthresh,
    .undo_cwnd = tcp_reno_undo_cwnd,

    .owner = THIS_MODULE,
    .name = "rutcp",
};

static int __init tcp_rutcp_register(void)
{
  printk(KERN_INFO "Initializing rutcp. Hello!\n");
  BUILD_BUG_ON(sizeof(struct rutcp) > ICSK_CA_PRIV_SIZE);
  tcp_register_congestion_control(&tcp_rutcp);
  return 0;
}

static void __exit tcp_rutcp_unregister(void)
{
  printk(KERN_INFO "Exiting rutcp. Goodbye!\n");
  tcp_unregister_congestion_control(&tcp_rutcp);
}

module_init(tcp_rutcp_register);
module_exit(tcp_rutcp_unregister);

// TODO: Enter your name(s) in the author field
MODULE_AUTHOR("Arnab Halder and Sarthak Dalal.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("The RUTCP congestion controller");
