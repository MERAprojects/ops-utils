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

/************************************************************************//**
 * @defgroup nl_utils Core Utilities
 * This library provides common utility functions used by various OpenSwitch
 * processes.
 * @{
 *
 * @defgroup nl_utils_public Public Interface
 * Public API for nl_utils library.
 *
 *
 *
 * @{
 *
 * @file
 * Header for vrf_utils library.
 ***************************************************************************/

#ifndef __NETLINK_UTILS_H_
#define __NETLINK_UTILS_H_

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <fcntl.h>
#include <sched.h>

#include "vswitch-idl.h"
#include "uuid.h"
#include "vrf-utils.h"

#define MAX_BUFFER_LENGTH 64
/**************************************************************************
***************************************************************************/
struct setns_info
{
    char from_ns[MAX_BUFFER_LENGTH];
    char to_ns[MAX_BUFFER_LENGTH];
    char intf_name[MAX_BUFFER_LENGTH];
};

/************************************************************************
* moves an interface from another namespace to default_vrf namespace
*
* @param[in]  setns_local_info : contains from and to vrf names and intf name
*
* @return true if sucessful, else false on failure
***************************************************************************/
extern bool nl_move_intf_to_vrf(struct setns_info *setns_local_info);
/***************************************************************************
* creates an socket by entering the corresponding namespace
*
* @param[in]  vrf_name : this is the namespace in which socket to be opened.
* @param[in]  params : contains socket params to pass to the socket.
*
* @return valid fd if sucessful, else 0 on failure
***************************************************************************/
extern int nl_create_vrf_socket(char* vrf_name, struct vrf_sock_params *params);
/***************************************************************************
* creates an socket by entering the corresponding namespace
*
* @param[in]  vrf_name : this is the namespace in which socket to be opened.
* @param[in]  socket_fd : fd of the socket to close.
*
* @return valid true if sucessful, else false on failure
***************************************************************************/
extern bool nl_close_vrf_socket(char* vrf_name, int socket_fd);

struct rtareq {
    struct nlmsghdr  n;
    struct ifinfomsg i;
    char buf[128];      /* must fit interface name length (IFNAMSIZ)*/
};

#endif /* __NETLINK_UTILS_H_ */
/** @} end of group nl_utils_public */
/** @} end of group nl_utils */
