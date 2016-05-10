/*
 Copyright (C) 2016 Hewlett-Packard Development Company, L.P.
 All Rights Reserved.

    Licensed under the Apache License, Version 2.0 (the "License"); you may
    not use this file except in compliance with the License. You may obtain
    a copy of the License at

         http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
    License for the specific language governing permissions and limitations
    under the License.
*/

/*************************************************************************//**
 * @ingroup ops_utils
 * This module contains the DEFINES and functions that comprise the ops-utils
 * library.
 *
 * @file
 * Source file for ops-utils library.
 *
 ****************************************************************************/

#define _GNU_SOURCE
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dynamic-string.h>

#include <assert.h>
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "nl-utils.h"
#include "vrf-utils.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(ops_utils);
/***************************************************************************
* creates an socket by entering the corresponding namespace
*
* @param[in]  vrf_name : this is the namespace in which socket to be opened.
* @param[in]  socket_fd : fd of the socket to close.
*
* @return valid true if sucessful, else false on failure
***************************************************************************/
int  nl_create_vrf_socket(char* vrf_name, struct vrf_sock_params *params)
{
    char set_ns[MAX_BUFFER_LENGTH] = {0};
    int  fd_from_ns;

    int ns_sock;

    /* Open FD to set the thread to a namespace */
    snprintf(set_ns, sizeof(set_ns), "/var/run/netns/%s", vrf_name);
    fd_from_ns = open(set_ns, O_RDONLY);
    if (fd_from_ns == -1) {
        VLOG_ERR("Unable to open fd for namepsace %s", vrf_name);
        return -1;
    }
    if (setns(fd_from_ns, CLONE_NEWNET) == -1) {
        VLOG_ERR("Unable to set %s namespace to the thread", vrf_name);
        close(fd_from_ns);
        return -1;
    }
    /* Open a socket in the namespace set by thread */
    ns_sock = socket(params->family, params->type, params->protocol);

    if (ns_sock < 0) {
        VLOG_ERR("socket creation failed (%s) in namespace %s",strerror(errno), vrf_name);
        close(fd_from_ns);
        close(ns_sock);
        return -1;
    }

    VLOG_DBG("socket created. fd = %d in namespace %s", ns_sock, vrf_name);

    close(fd_from_ns);
    return 0;
}

/***************************************************************************
* creates an socket by entering the corresponding namespace
*
* @param[in]  vrf_name : this is the namespace in which socket to be opened.
* @param[in]  params : contains socket params to pass to the socket.
*
* @return valid fd if sucessful, else 0 on failure
***************************************************************************/
bool nl_close_vrf_socket (char* vrf_name, int socket_fd)
{
    char set_ns[MAX_BUFFER_LENGTH] = {0};
    int  fd_from_ns;


    /* Open FD to set the thread to a namespace */

    snprintf(set_ns, sizeof(set_ns), "/var/run/netns/%s", vrf_name);
    fd_from_ns = open(set_ns, O_RDONLY);
    if (fd_from_ns == -1) {
        VLOG_ERR("Unable to open fd for namepsace %s", vrf_name);
        return false;
    }
    if (setns(fd_from_ns, CLONE_NEWNET) == -1) {
        VLOG_ERR("Unable to set %s namespace to the thread", vrf_name);
        close(fd_from_ns);
        return false;
    }
    /* delete the socket created by the thread */
    close (socket_fd);

    VLOG_DBG("socket closed fd = %d in namespace %s", socket_fd, vrf_name);
    close(fd_from_ns);

    return true;
}

/************************************************************************
* moves an interface from one vrf to another vrf.
*
* @param[in]  setns_local_info : contains from and to vrf names and intf name
*
* @return true if sucessful, else false on failure
***************************************************************************/
bool nl_move_intf_to_vrf (struct setns_info *setns_local_info)
{
    char ns_path[MAX_BUFFER_LENGTH]= {0};
    char set_ns[MAX_BUFFER_LENGTH] = {0};
    int fd, fd_from_ns;
    struct rtattr *rta;
    struct rtareq req;

    int ns_sock;
    bool ns_sock_in_use = false;
    struct sockaddr_nl s_addr;

    /* Open FD to set the thread to a namespace */

    snprintf(set_ns, sizeof(set_ns), "/var/run/netns/%s", setns_local_info->from_ns);
    fd_from_ns = open(set_ns, O_RDONLY);
    if (fd_from_ns == -1) {
        VLOG_ERR("Unable to open fd for namepsace %s", setns_local_info->from_ns);
        return false;
    }
    if (setns(fd_from_ns, CLONE_NEWNET) == -1) {
        VLOG_ERR("Unable to set %s namespace to the thread", setns_local_info->from_ns);
        close(fd_from_ns);
        return false;
    }
    /* Open a socket in the namespace set by thread */
    ns_sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

    if (ns_sock < 0) {
        VLOG_ERR("Netlink socket creation failed (%s) in namespace %s",strerror(errno), setns_local_info->from_ns);
        close(ns_sock);
        close(fd_from_ns);
        return false;
    }

    memset((void *) &s_addr, 0, sizeof(s_addr));
    s_addr.nl_family = AF_NETLINK;
    s_addr.nl_pid = getpid();
    s_addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR | RTMGRP_LINK;

    if (bind(ns_sock, (struct sockaddr *) &s_addr, sizeof(s_addr)) < 0) {
        if (errno != EADDRINUSE) {
            VLOG_ERR("Netlink socket bind failed (%s) in namespace %s",strerror(errno), setns_local_info->from_ns);
            close(ns_sock);
            close(fd_from_ns);
            return false;
        }
        ns_sock_in_use = true;
    }

    VLOG_DBG("Netlink socket created. fd = %d",ns_sock);

    memset(&ns_path[0], 0, MAX_BUFFER_LENGTH);
    /* open a FD to move the interface */
    snprintf(ns_path, sizeof(ns_path), "/var/run/netns/%s", setns_local_info->to_ns);
    fd = open(ns_path, O_RDONLY);
    if (fd == -1) {
        VLOG_ERR("Unable to open fd for namepsace %s", setns_local_info->from_ns);
        close(fd);
        close(ns_sock);
        close(fd_from_ns);
        return false;
    }

    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len     = NLMSG_SPACE(sizeof(struct ifinfomsg));
    req.n.nlmsg_pid     = getpid();
    req.n.nlmsg_type    = RTM_SETLINK;
    req.n.nlmsg_flags   = NLM_F_REQUEST;

    req.i.ifi_family    = AF_UNSPEC;
    req.i.ifi_index     = if_nametoindex(setns_local_info->intf_name);

    if (req.i.ifi_index == 0) {
        VLOG_ERR("Unable to get ifindex for interface: %s", setns_local_info->intf_name);
        close(fd);
        close(ns_sock);
        close(fd_from_ns);
        return false;
    }

    req.i.ifi_change = 0xffffffff;
    rta = (struct rtattr *)(((char *) &req) + NLMSG_ALIGN(req.n.nlmsg_len));
    rta->rta_type = IFLA_NET_NS_FD;
    rta->rta_len = RTA_LENGTH(sizeof(unsigned int));
    req.n.nlmsg_len = NLMSG_ALIGN(req.n.nlmsg_len) + RTA_LENGTH(sizeof(fd));
    memcpy(RTA_DATA(rta), &fd, sizeof(fd));

    if (send(ns_sock, &req, req.n.nlmsg_len, 0) == -1) {
        VLOG_ERR("Netlink failed to set fd %d for interface %s", fd,
                setns_local_info->intf_name);
        close(fd);
        close(ns_sock);
        close(fd_from_ns);
        return false;
    }

    close(fd);
    if (!ns_sock_in_use)
    {
       close(ns_sock);
    }
    close(fd_from_ns);
    return true;
}
