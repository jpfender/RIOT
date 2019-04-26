/*
 * Copyright (C) 2015 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Basic ccn-lite relay example (produce and consumer via shell)
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 *
 * @}
 */

#include <stdio.h>

#include "tlsf-malloc.h"
#include "msg.h"
#include "shell.h"
#include "ccn-lite-riot.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/pktdump.h"

#include "ccnl-relay.h"
#include "ccnl-qos.h"

/* main thread's message queue */
#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* 10kB buffer for the heap should be enough for everyone */
#define TLSF_BUFFER     (10240 / sizeof(uint32_t))
static uint32_t _tlsf_heap[TLSF_BUFFER];

#define QOS_MAX_TC_ENTRIES (3)

static const qos_traffic_class_t tcs[QOS_MAX_TC_ENTRIES] =
{
    { "/HAW", false, false },
    { "/SafetyIO/Site/A", false, true },
    { "/HAW/Room/481", true, true },
};

int pit_strategy(struct ccnl_relay_s *relay, struct ccnl_interest_s *i, qos_traffic_class_t *tc)
{
    (void) i;

    printf("In PIT replacement tclass: [prefix: %s, reliable: %d, expedited: %d], pit count: %d\n",
           tc->traffic_class, tc->reliable, tc->expedited, relay->pitcnt);

    return 1;
}

int main(void)
{
    tlsf_add_global_pool(_tlsf_heap, sizeof(_tlsf_heap));
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    puts("Basic CCN-Lite example");

    ccnl_core_init();

    ccnl_start();

    /* get the default interface */
    gnrc_netif_t *netif;

    /* set the relay's PID, configure the interface to use CCN nettype */
    if (((netif = gnrc_netif_iter(NULL)) == NULL) ||
        (ccnl_open_netif(netif->pid, GNRC_NETTYPE_CCN) < 0)) {
        puts("Error registering at network interface!");
        return -1;
    }

#ifdef MODULE_NETIF
    gnrc_netreg_entry_t dump = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                          gnrc_pktdump_pid);
    gnrc_netreg_register(GNRC_NETTYPE_CCN_CHUNK, &dump);
#endif

    ccnl_qos_set_tcs((qos_traffic_class_t *) &tcs, sizeof(tcs) / sizeof(tcs[0]));

    ccnl_set_pit_strategy_remove(pit_strategy);

    printf("max pit: %d\n", ccnl_relay.max_pit_entries);

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}