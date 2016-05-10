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

#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>
#include <errno.h>

#include <assert.h>
#include "vrf-utils.h"
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "nl-utils.h"
/*
 * Check for presence of VRF and return VRF row.
 */

const struct ovsrec_vrf *
vrf_lookup (const struct ovsdb_idl *idl, const char *vrf_name)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    OVSREC_VRF_FOR_EACH (vrf_row, idl)
      {
        if (strncmp (vrf_row->name, vrf_name, OVSDB_VRF_NAME_MAXLEN) == 0)
        return vrf_row;
      }
    return NULL;
}/*vrf_lookup*/


/*
 * Return default VRF row
 */
const struct ovsrec_vrf *
get_default_vrf (const struct ovsdb_idl *idl)
{
    /** TBD, Logic can be simplified further if we can store default VRF UUID.*/
    const struct ovsrec_vrf *vrf_row = vrf_lookup(idl, DEFAULT_VRF_NAME);

    return vrf_row;
}

/*
 * Check for presence of VRF using table_id and return VRF row.
 */
const struct ovsrec_vrf *
vrf_lookup_on_table_id (const struct ovsdb_idl *idl, const int64_t table_id)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        if((vrf_row->table_id) && (*(vrf_row->table_id) == table_id))
            return vrf_row;
    }
    return NULL;
}/*vrf_lookup_on_table_id*/

/*
 * Return VRF UUId
 */
const int64_t
get_vrf_uuid_from_table_id (const struct ovsdb_idl *idl, const int64_t table_id,
                            struct uuid *uuid)
{
    const struct ovsrec_vrf *vrf_row = NULL;

    if (uuid == NULL)
       return -1;

    vrf_row = vrf_lookup_on_table_id(idl, table_id);
    if (vrf_row != NULL)
    {
        *uuid = vrf_row->header_.uuid;
        return 0;
    }

    return -1;
}

/*
 * Return VRF table_id
 */
const int64_t
get_vrf_table_id_from_uuid (const struct ovsdb_idl *idl, const struct uuid *uuid)
{
    const struct ovsrec_vrf *vrf_row = NULL;

    if (uuid == NULL)
       return -1;

    vrf_row = ovsrec_vrf_get_for_uuid(idl, uuid);
    if(vrf_row != NULL)
        return *vrf_row->table_id;

    return -1;
}

/*
 * Creates a socket by entering the corresponding namespace
 */
int create_vrf_socket (const struct ovsdb_idl *idl, int64_t table_id,
                      struct vrf_sock_params *params)
{
    const struct ovsrec_vrf *vrf_row = NULL;

    vrf_row = vrf_lookup_on_table_id(idl, table_id);
    if (vrf_row != NULL)
        return nl_create_vrf_socket(vrf_row->name, params);

    return -1;
}

/*
 * Closes a socket by entering the corresponding namespace
 */
int close_vrf_socket (const struct ovsdb_idl *idl, int64_t table_id,
                      int socket_fd)
{
    const struct ovsrec_vrf *vrf_row = NULL;

    vrf_row = vrf_lookup_on_table_id(idl, table_id);
    if (vrf_row != NULL)
    {
        if(nl_close_vrf_socket(vrf_row->name, socket_fd))
            return 0;
    }
    return -1;
}
