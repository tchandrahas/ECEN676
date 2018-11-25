/*******************************************************************************
* Copyright (c) 2012-2014, The Microsystems Design Labratory (MDL)
* Department of Computer Science and Engineering, The Pennsylvania State University
* All rights reserved.
*
* This source code is part of NVMain - A cycle accurate timing, bit accurate
* energy simulator for both volatile (e.g., DRAM) and non-volatile memory
* (e.g., PCRAM). The source code is free and you can redistribute and/or
* modify it by providing that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Author list:
*   Matt Poremba    ( Email: mrp5060 at psu dot edu
*                     Website: http://www.cse.psu.edu/~poremba/ )
*******************************************************************************/

#ifndef __DRAMCACHE_H__
#define __DRAMCACHE_H__
#include "include/CommonMath.h"
#include "include/CommonHeaders.h"
#include "Utils/Caches/CacheAssoc.h"
#include "include/CacheBlock.hh"
#include "include/CacheSet.hh"
#include "NVM/nvmain.h"
#include "include/Exception.h"
#include "include/CommonCallBack.hh"
#include "src/EventQueue.h"
#include "MemControl/DRAMCache/AbstractDRAMCache.h"
#include <iostream>
namespace NVM
{
  #define DRC_FILL_CACHE tagGen->CreateTag("DRC_FILL_CACHE")
  #define MIGRATION tagGen->CreateTag("MIGRATION")
  #define DRC_MEMFETCH tagGen->CreateTag("DRC_MEMFETCH")
  #define WRITE_BACK tagGen->CreateTag("WRITE_BACK")
  #define DRC_MIGRATION_MEMFETCH tagGen->CreateTag("DRC_MIGRATION_MEMFETCH")

class NVMain;

class WayPredictDRC :public AbstractDRAMCache
{
  public:
      WayPredictDRC( );
      virtual ~WayPredictDRC( );
      virtual void SetConfig( Config *conf , bool CreateChildren = true);
      virtual void SetMainMemory( NVMain* memory);
      virtual void SetDRAMCache( AbstractDRAMCache* dram_cache);
      //issue command related functions
      virtual bool IssueCommand( NVMainRequest* req);
      virtual bool IssueAtomic( NVMainRequest* req);
      virtual bool IssueFunctional( NVMainRequest* req);
      virtual bool RequestComplete( NVMainRequest* req);

      void MemFetch( NVMainRequest* req);
      void FillCache( NVMainRequest*req);
      void IssueToOtherModule( NVMainRequest *req);

      virtual void Cycle( ncycle_t steps = 1);

      //trace stats related
      virtual void RegisterStats();
      virtual void CalculateStats();
      
      uint64_t GetDRAMHitTime();
      uint64_t GetDRAMMissTime();

      NVMain* main_memory;	//point to main memory
      CacheImpl* dram_cache;	//point to dram cache

      uint64_t dram_hit_time, dram_miss_time;

      uint64_t read_time , write_time;	//read/write cache time
      uint64_t cache_line_size;
      uint64_t cache_assoc;
      uint64_t cache_capacity;
      uint64_t cache_sets;		//number of set( calculated according to cache_line_size , cache_asoc and cache capicty)
      //access_count : total times accessing DRAM cache
      //miss_count: access miss , have to fetch data from main memory( specifily: PCM)
      //hit_count: access hit
      //evict_count: evict cache block from DRAM due to replacement
      uint64_t miss_count , hit_count , evict_count , wb_count,fill_count;
      uint64_t write_hit , read_hit , read_miss , write_miss;
      double hit_rate;
      bool perfect_fill;	//fill first
      bool write_allocate;	//write allocate? no-write allocate?
      uint64_t drcQueueSize;
   private:
      NVMTransactionQueue *drcQueue;
      std::map<NVMainRequest* , NVMainRequest*>outstanding_memfetch;
   };

};

#endif
