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
 * File         : utils.h
 * Author       : Hyesoon Kim + Jaekyu Lee
 * Date         : 12/18/2007
 * CVS          : $Id: utils.h,v 1.3 2008-04-10 00:54:07 hyesoon Exp $:
 * Description  : utilities
                  based on utils.h at scarab
                  other utility classes added
**********************************************************************************************/

/////////////////////////////
// Ported from macsim
/////////////////////////////

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <list>

#include "global_types.h"
#include "global_defs.h"

namespace cxlsim {

template <class T>
class pool_c
{
public:
  /**
   * Constructor
   */
  pool_c() {
    m_pool = new std::list<T*>;
    m_poolsize = 0;
    m_poolexpand_unit = 1;
    m_name = "none";
  }

  /**
   * Constructor
   * @param pool_expand_unit number of entries when pool expands
   * @param name pool name
   */
  pool_c(int pool_expand_unit, std::string name) {
    m_pool = new std::list<T*>;
    m_poolsize = 0;
    m_poolexpand_unit = pool_expand_unit;
    m_name = name;
  }

  /**
   * Destructor
   */
  ~pool_c() {
    while (!m_pool->empty()) {
      T* entry = m_pool->front();
      m_pool->pop_front();
      delete entry;
    }
    delete m_pool;
  }

  /**
   * Acquire a new entry
   */
  T* acquire_entry(void) {
    if (m_pool->empty()) {
      expand_pool();
    }
    T* entry = m_pool->front();
    m_pool->pop_front();
    return entry;
  }

  /**
   * Acquire a new entry
   *   whose class requires simBase reference
   */
  T* acquire_entry(cxlsim_c* m_simBase) {
    if (m_pool->empty()) {
      expand_pool(m_simBase);
    }
    T* entry = m_pool->front();
    m_pool->pop_front();
    return entry;
  }

  /**
   * Release a new entry
   */
  void release_entry(T* entry) {
    m_pool->push_front(entry);
  }

  /**
   * Expand the pool
   */
  void expand_pool(void) {
    for (int ii = 0; ii < m_poolexpand_unit; ++ii) {
      T* entries = new T;
      m_pool->push_back(entries);
    }
    m_poolsize += m_poolexpand_unit;
  }

  /**
   * Expand the pool
   *  whose class requires simBase reference
   */
  void expand_pool(cxlsim_c* m_simBase) {
    for (int ii = 0; ii < m_poolexpand_unit; ++ii) {
      T* entries = new T(m_simBase);
      m_pool->push_back(entries);
    }
    m_poolsize += m_poolexpand_unit;
  }

  /**
   * Return the size of a pool
   */
  int size(void) {
    return m_poolsize;
  }

private:
  std::list<T*>* m_pool; /**< pool */
  int m_poolsize; /**< pool size */
  int m_poolexpand_unit; /**< pool expand unit */
  std::string m_name; /**< pool name */
};

} // namespace CXL

#endif  // UTILS_H
