/*
Copyright (c) <2012>, <Georgia Institute of Technology> All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions
and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or other materials provided
with the distribution.

Neither the name of the <Georgia Institue of Technology> nor the names of its contributors
may be used to endorse or promote products derived from this software without specific prior
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

/**********************************************************************************************
 * File         : packet_info.h
 * Author       : Joonho
 * Date         : 10/8/2021
 * SVN          : $Id: packet_info.h,v 1.5 2021-10-08 21:01:41 kacear Exp $:
 * Description  : PCIe packet information
 *********************************************************************************************/

#ifndef PACKET_INFO_H
#define PACKET_INFO_H

#include <string>
#include <list>
#include <deque>
#include <functional>

#include "cxlsim.h"
#include "uop.h"
#include "global_defs.h"
#include "global_types.h"

namespace cxlsim {

//////////////////////////////////////////////////////////////////////////////

typedef struct cxl_req_s {
  cxl_req_s(cxlsim_c* simBase);
  void init(void);
  void init(Addr addr, bool write, bool isuop, uop_s* uop, void* req);
  void print(void);

  Counter m_id; /**< id of current request */
  Counter m_dram_start; /**< cycle that this request started dram */

  Addr m_addr; /**< if current req is memory request, target */
  bool m_write; /**< if current req is memory request, is write? */

  bool m_isuop; /**< indicates if the current request is a uop request */
  uop_s* m_uop; /**< uop of the current request */
  void* m_req; /**< pointer for saving external simulator requests */
  cxlsim_c* m_simBase;
} cxl_req_s;

//////////////////////////////////////////////////////////////////////////////

typedef enum MSG_TYPE {
  M2S_REQ = 0,
  M2S_RWD,
  M2S_DATA,
  M2S_UOP,
  S2M_NDR,
  S2M_DRS,
  S2M_DATA,
  S2M_UOP,
  INVALID,
  MAX_MSG_TYPES
} MSG_TYPE;

typedef struct message_s {
  /**
   * Constructor
   */
  message_s(cxlsim_c* simBase);
  void init(void);
  void print(void);

  int m_id; /**< unique request id */
  int m_bits;
  MSG_TYPE m_type;

  bool m_data;
  message_s* m_parent; /**< Parent message of this data msg */
  std::list<message_s*> m_childs; /**< Child msg of this req/resp w data msg */
  int m_arrived_child;

  Counter m_txvc_start;
  Counter m_txdll_start;
  Counter m_rxvc_start;
  Counter m_rxtrans_end;  /**< rxlogic finished cycle */

  int m_vc_id; /**< VC id */

  cxl_req_s* m_req; /**< packet may be a result of mem request */
  cxlsim_c* m_simBase; /**< reference to macsim base class for sim globals */
} message_s;

//////////////////////////////////////////////////////////////////////////////

typedef enum SLOT_TYPE {
  INVAL_SLOT = 0,
  H4, // M2S RwD / 2 S2M NDR
  H5, // M2S Req / 2 S2M DRS
  G0, // Data
  G4, // M2S Req + CXL.$ / S2M DRS + 2 S2M NDR
  G5, // M2S RwD + CXL.$ / 2 S2M NDR
  G6, // 3 S2M DRS
  MAX_SLOT_TYPES
} SLOT_TYPE;

typedef struct slot_s {
  slot_s(cxlsim_c* simBase);
  void init(void);
  int size(void);
  int get_wd_msg_cnt(void);
  void print(void);

  int m_id;
  int m_msg_cnt[MAX_MSG_TYPES];
  SLOT_TYPE m_type;
  std::vector<message_s*> m_msgs;
  cxlsim_c* m_simBase;
} slot_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct flit_s {
  flit_s(cxlsim_c* simBase);
  void init(void);
  int size(void);
  bool is_rollover(void);
  void print(void);
  void push_back(slot_s* slot);
  void push_front(slot_s* slot);

  int m_id;
  int m_bits;
  bool m_phys_sent;
  int m_msg_cnt[MAX_MSG_TYPES];

  Counter m_txdll_end;
  Counter m_phys_end;
  Counter m_rxdll_end;

  std::list<slot_s*> m_slots;
  cxlsim_c* m_simBase;
} flit_s;

} // namespace CXL

#endif /* #ifndef MEMORY_H_INCLUDED  */
