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
#include "global_defs.h"
#include "global_types.h"

namespace cxlsim {

#define TX true
#define RX false

typedef enum channel_type {
  WOD_CHANNEL= 0, /**< req/resp without data */
  WD_CHANNEL,     /**< req/resp with data */
  DATA_CHANNEL,   /**< data */
  UOP_CHANNEL,    /**< uop */
  MAX_CHANNEL
} channel_type;

//////////////////////////////////////////////////////////////////////////////

typedef struct cxl_req_s {
  cxl_req_s(cxlsim_c* simBase);
  void init(void);
  void print(void);

  Counter m_id;
  Addr m_addr;
  bool m_write;
  void* m_req;
  cxlsim_c* m_simBase;
} cxl_req_s;

//////////////////////////////////////////////////////////////////////////////

typedef enum MSG_TYPE {
  M2S_REQ = 0,
  M2S_RWD,
  M2S_DATA,
  S2M_NDR,
  S2M_DRS,
  S2M_DATA,
  INVALID,
  MAX_MSG_TYPES
} MSG_TYPE;

static const std::string msg_type_string[MAX_MSG_TYPES] = {
  "REQ ",
  "RWD ",
  "DATA",
  "NDR ",
  "DRS ",
  "DATA",
  "INVALID"
};

typedef struct message_s {
  /**
   * Constructor
   */
  message_s(cxlsim_c* simBase);
  void init(void);
  void print(void);
  bool is_wdata_msg(void);
  bool txvc_rdy(Counter cycle);
  bool rxvc_rdy(Counter cycle);
  void init_data_msg(message_s* parent);
  void inc_arrived_child();
  bool child_waiting();

  int m_id; /**< unique request id */
  int m_bits;
  MSG_TYPE m_type;

  bool m_data;
  message_s* m_parent;
  std::list<message_s*> m_childs;
  int m_arrived_child;

  Counter m_txvc_insert_start;
  Counter m_txvc_insert_done;
  Counter m_rxvc_insert_start;
  Counter m_rxvc_insert_done;;

  int m_vc_id; /**< VC id */

  cxl_req_s* m_req; /**< packet may be a result of mem request */
  cxlsim_c* m_simBase; /**< reference to macsim base class for sim globals */
} message_s;

//////////////////////////////////////////////////////////////////////////////
typedef enum SLOT_TYPE {
  INVAL_SLOT = 0,
  H4, // M2S RwD / 2 S2M NDR
  H5, // M2S Req / 2 S2M DRS
  HX, // UOP (temporary type for ndp)
  G0, // Data
  G4, // M2S Req + CXL.$ / S2M DRS + 2 S2M NDR
  G5, // M2S RwD + CXL.$ / 2 S2M NDR
  G6, // 3 S2M DRS
  GX, // UOP (temporary type for ndp)
  MAX_SLOT_TYPES
} SLOT_TYPE;

static const std::string slot_type_str[MAX_SLOT_TYPES] = {
  "INVAL_SLOT",
  "H4",
  "H5",
  "HX",
  "G0",
  "G4",
  "G5",
  "G6",
  "GX"
};

typedef struct slot_s {
  slot_s(cxlsim_c* simBase);
  void init(void);
  void push_back(message_s* msg);
  void print(void);
  bool empty(void);
  bool is_data(void);
  bool multi_msg(void);
  void assign_type(void);
  void set_head(void);

#ifdef DEBUG
  int get_req_resp(void);
#endif

  int m_id;
  int m_bits;
  bool m_head;
  SLOT_TYPE m_type;
  int m_msg_cnt[MAX_MSG_TYPES];
  std::list<message_s*> m_msgs;
  cxlsim_c* m_simBase;
} slot_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct flit_s {
  flit_s(cxlsim_c* simBase);
  void init(void);
  void print(void);
  int num_slots(void);
  void push_back(slot_s* slot);
  void push_front(slot_s* slot);
  bool rollover(void);

  int m_id;
  int m_bits;
  bool m_phys_sent;

  Counter m_flit_gen_cycle;

  Counter m_txreplay_insert_start;
  Counter m_txreplay_insert_done;
  Counter m_phys_start;
  Counter m_phys_done;
  Counter m_rxdll_done;

#ifdef DEBUG
  int m_reqresp_cnt;

  int get_req_resp(void);
#endif

  int m_msg_cnt[MAX_MSG_TYPES];
  std::list<slot_s*> m_slots;
  cxlsim_c* m_simBase;
} flit_s;

} // namespace CXL

#endif /* #ifndef MEMORY_H_INCLUDED  */
