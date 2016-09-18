#!/usr/bin/env python
# Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

import ovs.dirs
import ovs.daemon
import ovs.db.idl
import ovs.unixctl
import ovs.unixctl.server
import argparse
import ovs.vlog
import os

global_dstn_ns = ''
vlog = ovs.vlog.Vlog("vrf_utils")

SYSTEM_TABLE = "System"
PORT_TABLE = "Port"
VRF_TABLE = "VRF"

DEFAULT_VRF_NAME = "vrf_default"
SWITCH_NAMESPACE = "swns"
NAMESPACE_NAME_PREFIX = "VRF_"

def  get_mgmt_ip(idl):
    '''
    get the ip address of management interface
    '''
    mgmt_ip = None

    for ovs_rec in idl.tables[SYSTEM_TABLE].rows.itervalues():
        if ovs_rec.mgmt_intf_status and ovs_rec.mgmt_intf_status is not None:
            for key, value in ovs_rec.mgmt_intf_status.iteritems():
                if key == "ip":
                    mgmt_ip = value

    return mgmt_ip

def extract_ip(ip):
    '''
    extract the ip from [u'x.x.x.x/y'] format
    '''
    tmp = str(ip).split('\'')
    tmp = tmp[1].split('/')
    ip = tmp[0]

    return ip

def get_ip_from_interface_name(idl, source_interface):
    '''
    get the ip address configured on the interface
    '''
    ip = None

    for ovs_rec in idl.tables[PORT_TABLE].rows.itervalues():
        if ovs_rec.name == source_interface:
            ip = ovs_rec.ip4_address
            if len(ip) == 0:
                return None
            ip = extract_ip(ip)

    return ip

def get_vrf_name_from_ip(idl, src_ip):
    '''
    get the vrf configured on the interface
    '''
    vrf_name = None

    for ovs_rec in idl.tables[PORT_TABLE].rows.itervalues():
        ip = ovs_rec.ip4_address

        if len(ip) == 0:
            continue

        ip = extract_ip(ip)
        if ip == src_ip:
            vrf_id = ovs_rec.uuid

            for ovs_rec in idl.tables[VRF_TABLE].rows.itervalues():
                for port_row in ovs_rec.ports:

                    if port_row.uuid == vrf_id:
                        vrf_name = ovs_rec.name
                        break
    return vrf_name

def get_source_interface(idl, protocol):
    '''
    get the source interface configured for the protocol
    '''
    source_interface = None

    for ovs_rec in idl.tables["VRF"].rows.itervalues():

        if ovs_rec.name == DEFAULT_VRF_NAME:
            ip = ovs_rec.source_ip

            source_interface = ovs_rec.source_interface

            for key, value in ovs_rec.source_ip.iteritems():
                vlog.info("IP: key = %s, value = %s \n" %(str(key), str(value)))

                if key == protocol:
                    if len(value) != 0:
                        return value

                if key == "all":
                    if len(value) != 0:
                        return value

            for key, value in ovs_rec.source_interface.iteritems():

                if key == protocol:
                    return (extract_ip(value.ip4_address))

                if key == "all":
                    return (extract_ip(value.ip4_address))


def get_namespace_from_vrf(idl, vrf_name):
    '''
    get the namespace corresponding to the VRF
    get clarification from Krishna's team on this logic
    '''

    namespace = None

    for ovs_rec in idl.tables[VRF_TABLE].rows.itervalues():
         if ovs_rec.name == vrf_name:
                table_id = ovs_rec.table_id
                table_id = str(table_id).strip('[]')
                if table_id == '0':
                    namespace = SWITCH_NAMESPACE
                else:
                    namespace = NAMESPACE_NAME_PREFIX + str(table_id)

    return namespace
